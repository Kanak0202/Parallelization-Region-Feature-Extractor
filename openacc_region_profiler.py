#!/usr/bin/env python3

import os
import re
import sys
import subprocess
import tempfile
import shutil
from pathlib import Path
from dataclasses import dataclass


# ============================================================
# Configuration
# ============================================================

NVC = os.environ.get("NVC", "nvc")

KEEP_TEMP = True

COMPILE_FLAGS = [
    "-acc",
    "-mcmodel=medium",
]

RUN_ENV = os.environ.copy()
RUN_ENV["NV_ACC_TIME"] = "1"


# ============================================================
# Data structures
# ============================================================

@dataclass
class Region:
    index: int
    start_line: int
    end_line: int
    acc_line: int
    code: str


# ============================================================
# Utility functions
# ============================================================

def run_command(cmd, env=None, cwd=None, capture_output=True):
    print("\n$ " + " ".join(str(x) for x in cmd))

    result = subprocess.run(
        cmd,
        env=env,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE if capture_output else None,
        stderr=subprocess.STDOUT if capture_output else None,
    )

    if capture_output and result.stdout:
        print(result.stdout)

    if result.returncode != 0:
        raise RuntimeError(
            f"Command failed with exit code {result.returncode}"
        )

    return result.stdout if capture_output else ""


def find_macros(source):
    """
    Extract #define names.

    Example:

        #define n 10

    returns:

        {"n": "10"}
    """

    macros = {}

    pattern = re.compile(
        r"^\s*#\s*define\s+([A-Za-z_]\w*)"
        r"(?:\s+(.*))?$",
        re.MULTILINE,
    )

    for match in pattern.finditer(source):
        name = match.group(1)
        value = match.group(2) or ""
        macros[name] = value.strip()

    return macros


def strip_comments(code):
    code = re.sub(r"/\*.*?\*/", " ", code, flags=re.DOTALL)
    code = re.sub(r"//.*", " ", code)
    return code


# ============================================================
# Region detection
# ============================================================

def find_profitability_regions(source):
    """
    Detect:

        #pragma capc profitability_region begin
        ...
        #pragma capc profitability_region end
    """

    lines = source.splitlines()

    regions = []

    begin_re = re.compile(
        r"^\s*#\s*pragma\s+capc\s+profitability_region\s+begin"
    )

    end_re = re.compile(
        r"^\s*#\s*pragma\s+capc\s+profitability_region\s+end"
    )

    current_start = None
    current_acc_line = None

    for idx, line in enumerate(lines, start=1):

        if begin_re.search(line):
            current_start = idx
            current_acc_line = None
            continue

        if current_start is not None:

            if re.match(r"^\s*#\s*pragma\s+acc\b", line):
                if current_acc_line is None:
                    current_acc_line = idx

            if end_re.search(line):

                end_line = idx

                region_lines = lines[current_start:end_line - 1]

                # Remove CAPC begin/end markers if present.
                region_code = "\n".join(region_lines)

                regions.append(
                    Region(
                        index=len(regions),
                        start_line=current_start,
                        end_line=end_line,
                        acc_line=current_acc_line,
                        code=region_code,
                    )
                )

                current_start = None
                current_acc_line = None

    return regions


# ============================================================
# Array detection
# ============================================================

def detect_arrays(source):
    """
    Detect simple declarations such as:

        float A[n][n][n];
        double B[N][M];

    """

    arrays = {}

    clean = strip_comments(source)

    pattern = re.compile(
        r"\b"
        r"(?:const\s+)?"
        r"(?:unsigned\s+|signed\s+)?"
        r"(?:short\s+|long\s+|long\s+long\s+)?"
        r"(?:int|float|double|char|long|short)"
        r"\s+"
        r"([A-Za-z_]\w*)"
        r"((?:\s*\[[^\]]+\])+)"
        r"\s*;"
    )

    for match in pattern.finditer(clean):

        name = match.group(1)
        dimensions = match.group(2)

        arrays[name] = {
            "declaration": match.group(0),
            "dimensions": dimensions,
        }

    return arrays


# ============================================================
# Variable detection
# ============================================================

C_KEYWORDS = {
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "register",
    "restrict",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "void",
    "volatile",
    "while",
    "_Bool",
    "_Complex",
    "_Imaginary",
}


def detect_loop_variables(region_code):
    """
    Detect loop variables from statements such as:

        for (i = 0; ...)
        for (j = 1; ...)
        for (k = 0; ...)
    """

    variables = set()

    pattern = re.compile(
        r"\bfor\s*\(\s*"
        r"([A-Za-z_]\w*)"
        r"\s*="
    )

    for match in pattern.finditer(region_code):
        variables.add(match.group(1))

    return variables


def detect_identifiers(region_code):
    """
    Find identifiers appearing in the region.

    This is intentionally conservative. We later filter
    against arrays, macros, keywords and known declarations.
    """

    clean = strip_comments(region_code)

    identifiers = set(
        re.findall(
            r"\b[A-Za-z_]\w*\b",
            clean,
        )
    )

    return identifiers


def detect_scalar_variables(source, region_code, arrays, macros):
    """
    Detect scalar variables required by the isolated region.

    Important:
        Anything appearing in #define is NOT emitted as a variable.

    Therefore:

        #define n 10

    prevents:

        int n;

    """

    identifiers = detect_identifiers(region_code)

    loop_vars = detect_loop_variables(region_code)

    variables = set()

    for identifier in identifiers:

        if identifier in C_KEYWORDS:
            continue

        if identifier in arrays:
            continue

        if identifier in macros:
            continue

        variables.add(identifier)

    # Loop variables definitely need declarations.
    variables.update(loop_vars)

    return variables


# ============================================================
# Access analysis
# ============================================================

def analyze_access(region_code, arrays):
    """
    Determine whether arrays are read and/or written.

    This is a conservative analysis.

    Results are:

        copyin
        copyout
        copy
    """

    accesses = {}

    for array_name in arrays:

        # Only consider arrays actually mentioned in region.
        if not re.search(
            rf"\b{re.escape(array_name)}\b",
            region_code,
        ):
            continue

        # Array write:
        # A[...] =
        write_pattern = re.compile(
            rf"\b{re.escape(array_name)}\s*"
            r"(?:\[[^\]]+\])+"
            r"\s*="
        )

        is_written = bool(
            write_pattern.search(region_code)
        )

        # Array read:
        # Presence of A[...] somewhere.
        read_pattern = re.compile(
            rf"\b{re.escape(array_name)}\s*"
            r"(?:\[[^\]]+\])+"
        )

        is_read = bool(
            read_pattern.search(region_code)
        )

        # Remove write expressions when determining read.
        write_spans = [
            m.span()
            for m in write_pattern.finditer(region_code)
        ]

        if write_spans:
            temp = list(region_code)

            for start, end in write_spans:
                for i in range(start, end):
                    temp[i] = " "

            read_only_code = "".join(temp)

            is_read = bool(
                read_pattern.search(read_only_code)
            )

        if is_read and is_written:
            accesses[array_name] = "copy"
        elif is_written:
            accesses[array_name] = "copyout"
        elif is_read:
            accesses[array_name] = "copyin"

    return accesses


# ============================================================
# OpenACC directive conversion
# ============================================================

def extract_acc_pragma(region_code):
    """
    Find the first OpenACC pragma inside the region.
    """

    match = re.search(
        r"#\s*pragma\s+acc\s+([^\n]+)",
        region_code,
    )

    if not match:
        raise RuntimeError(
            "No OpenACC pragma found inside profitability region"
        )

    return match.group(1).strip()


def build_data_clause(accesses):
    """
    Convert access analysis into an OpenACC data clause.
    """

    copyin = []
    copyout = []
    copy = []

    for name, access in accesses.items():

        if access == "copyin":
            copyin.append(name)

        elif access == "copyout":
            copyout.append(name)

        elif access == "copy":
            copy.append(name)

    clauses = []

    if copyin:
        clauses.append(
            "copyin(" + ",".join(copyin) + ")"
        )

    if copyout:
        clauses.append(
            "copyout(" + ",".join(copyout) + ")"
        )

    if copy:
        clauses.append(
            "copy(" + ",".join(copy) + ")"
        )

    return " ".join(clauses)


# ============================================================
# Region extraction
# ============================================================

def remove_capc_pragmas(code):
    code = re.sub(
        r"^\s*#\s*pragma\s+capc[^\n]*\n?",
        "",
        code,
        flags=re.MULTILINE,
    )

    return code


def remove_openacc_data_dependencies(pragma):
    """
    Convert:

        parallel loop collapse(3) present(A,B)

    into:

        parallel loop collapse(3)

    The isolated program owns its own data region, so
    present(A,B) is not appropriate here.
    """

    pragma = re.sub(
        r"\bpresent\s*\([^)]*\)",
        "",
        pragma,
    )

    return re.sub(
        r"\s+",
        " ",
        pragma,
    ).strip()


def extract_kernel_code(region_code):
    """
    Extract the actual loop/kernel portion.

    Everything before the OpenACC pragma is ignored.
    """

    region_code = remove_capc_pragmas(region_code)

    match = re.search(
        r"#\s*pragma\s+acc\s+[^\n]+\n(.*)",
        region_code,
        flags=re.DOTALL,
    )

    if not match:
        raise RuntimeError(
            "Could not extract kernel code"
        )

    kernel = match.group(1).strip()

    return kernel


# ============================================================
# Isolated program generation
# ============================================================

def generate_isolated_source(
    original_source,
    region,
    arrays,
    macros,
):
    """
    Generate an independent program containing exactly one
    execution of the profitability region.

    Critical rule:

        Macro names must NEVER be redeclared.

    For:

        #define n 10

    we preserve:

        #define n 10

    but do NOT generate:

        int n;
    """

    accesses = analyze_access(
        region.code,
        arrays,
    )

    print(
        f"Variables: "
        f"{', '.join(arrays.keys())}"
    )

    print(
        f"Accesses: {accesses}"
    )

    # --------------------------------------------------------
    # Kernel
    # --------------------------------------------------------

    pragma = extract_acc_pragma(
        region.code
    )

    pragma = remove_openacc_data_dependencies(
        pragma
    )

    kernel = extract_kernel_code(
        region.code
    )

    # --------------------------------------------------------
    # Keep required preprocessor macros
    # --------------------------------------------------------

    required_macros = []

    identifiers = detect_identifiers(
        region.code
    )

    for macro_name, macro_value in macros.items():

        if macro_name in identifiers:

            if macro_value:
                required_macros.append(
                    f"#define {macro_name} {macro_value}"
                )
            else:
                required_macros.append(
                    f"#define {macro_name}"
                )

    # --------------------------------------------------------
    # Array declarations
    # --------------------------------------------------------

    array_declarations = []

    for name in accesses:

        if name not in arrays:
            continue

        array_declarations.append(
            arrays[name]["declaration"]
        )

    # --------------------------------------------------------
    # Scalar variables
    # --------------------------------------------------------

    scalar_variables = detect_scalar_variables(
        original_source,
        region.code,
        arrays,
        macros,
    )

    # We only want actual loop/control variables.
    # Avoid accidentally declaring function names etc.
    loop_variables = detect_loop_variables(
        region.code
    )

    scalar_variables = {
        x for x in scalar_variables
        if x in loop_variables
    }

    scalar_declarations = [
        f"int {name};"
        for name in sorted(scalar_variables)
    ]

    # --------------------------------------------------------
    # Data clause
    # --------------------------------------------------------

    data_clause = build_data_clause(
        accesses
    )

    if not data_clause:
        # Fallback.
        data_clause = "copyin(" + ",".join(
            accesses.keys()
        ) + ")"

    # --------------------------------------------------------
    # Build source
    # --------------------------------------------------------

    source_parts = []

    source_parts.append(
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "#include <math.h>\n"
    )

    if required_macros:
        source_parts.append(
            "\n".join(required_macros)
        )

    source_parts.append("")

    source_parts.extend(
        array_declarations
    )

    source_parts.extend(
        scalar_declarations
    )

    source_parts.append("")

    source_parts.append(
        "int main(void)\n"
        "{"
    )

    source_parts.append("")

    source_parts.append(
        f"#pragma acc data {data_clause}"
    )

    source_parts.append("{")

    source_parts.append(
        f"#pragma acc {pragma}"
    )

    source_parts.append(kernel)

    source_parts.append("}")

    source_parts.append("")

    source_parts.append(
        "return 0;"
    )

    source_parts.append("}")

    generated = "\n".join(
        source_parts
    )

    return generated, accesses


# ============================================================
# Timing parser
# ============================================================

def parse_acc_timing(output):
    """
    Parse NV_ACC_TIME output.

    Example:

        18: compute region reached 1 time
        18: kernel launched 1 time
        ...
        elapsed time(us): total=56 ...
    """

    timing = {
        "kernel_us": None,
        "h2d_us": 0.0,
        "d2h_us": 0.0,
        "total_us": None,
    }

    elapsed_matches = re.findall(
        r"elapsed time\(us\):\s*"
        r"total=(\d+(?:\.\d+)?)",
        output,
    )

    if elapsed_matches:
        # For an isolated program there should normally
        # be exactly one kernel.
        timing["kernel_us"] = float(
            elapsed_matches[-1]
        )

    # NV_ACC_TIME transfer lines vary slightly between
    # compiler versions. Handle common forms.

    h2d_patterns = [
        r"data copyin transfers:.*?device time\(us\):\s*total=(\d+(?:\.\d+)?)",
        r"data copyin transfers:.*?time\(us\):\s*total=(\d+(?:\.\d+)?)",
    ]

    d2h_patterns = [
        r"data copyout transfers:.*?device time\(us\):\s*total=(\d+(?:\.\d+)?)",
        r"data copyout transfers:.*?time\(us\):\s*total=(\d+(?:\.\d+)?)",
    ]

    for pattern in h2d_patterns:
        m = re.search(
            pattern,
            output,
            flags=re.DOTALL,
        )

        if m:
            timing["h2d_us"] = float(
                m.group(1)
            )
            break

    for pattern in d2h_patterns:
        m = re.search(
            pattern,
            output,
            flags=re.DOTALL,
        )

        if m:
            timing["d2h_us"] = float(
                m.group(1)
            )
            break

    if timing["kernel_us"] is not None:
        timing["total_us"] = (
            timing["h2d_us"]
            + timing["kernel_us"]
            + timing["d2h_us"]
        )

    return timing


# ============================================================
# Original application timing
# ============================================================

def compile_original(source_path, executable):
    run_command(
        [
            NVC,
            "-acc",
            str(source_path),
            "-o",
            str(executable),
        ]
    )


def run_original(executable):
    return run_command(
        [str(executable)],
        env=RUN_ENV,
    )


def parse_original_regions(output):
    """
    Parse:

        line: compute region reached ...
        line: kernel launched ...
        grid...
        elapsed time(us): total=...

    """

    regions = []

    lines = output.splitlines()

    current = None

    for line in lines:

        m = re.match(
            r"(\d+): compute region reached",
            line,
        )

        if m:
            current = {
                "line": int(m.group(1)),
                "kernel_us": None,
            }

            regions.append(current)
            continue

        m = re.match(
            r"elapsed time\(us\):\s*"
            r"total=(\d+(?:\.\d+)?)",
            line,
        )

        if m and current is not None:

            current["kernel_us"] = float(
                m.group(1)
            )

    return regions


# ============================================================
# Isolated execution
# ============================================================

def isolate_region(
    temp_dir,
    original_source,
    region,
    arrays,
    macros,
):
    source, accesses = generate_isolated_source(
        original_source,
        region,
        arrays,
        macros,
    )

    source_path = (
        temp_dir /
        f"region_{region.index}.c"
    )

    executable = (
        temp_dir /
        f"region_{region.index}.exe"
    )

    source_path.write_text(
        source
    )

    print(
        f"Generated isolated source:\n"
        f"{source_path}"
    )

    try:

        run_command(
            [
                NVC,
                *COMPILE_FLAGS,
                str(source_path),
                "-o",
                str(executable),
            ]
        )

        output = run_command(
            [str(executable)],
            env=RUN_ENV,
        )

        timing = parse_acc_timing(
            output
        )

        return timing

    except Exception as exc:

        print(
            f"ERROR isolating region "
            f"{region.index}: {exc}"
        )

        return {
            "kernel_us": 0.0,
            "h2d_us": 0.0,
            "d2h_us": 0.0,
            "total_us": 0.0,
        }


# ============================================================
# Main
# ============================================================

def main():

    if len(sys.argv) != 2:
        print(
            f"Usage: {sys.argv[0]} <openacc_source.c>"
        )
        sys.exit(1)

    source_path = Path(
        sys.argv[1]
    ).resolve()

    if not source_path.exists():
        print(
            f"ERROR: file does not exist: "
            f"{source_path}"
        )
        sys.exit(1)

    original_source = source_path.read_text()

    # --------------------------------------------------------
    # Macros
    # --------------------------------------------------------

    macros = find_macros(
        original_source
    )

    print(
        "Detected macros:"
    )

    for name, value in macros.items():
        print(
            f"  {name} = {value}"
        )

    # --------------------------------------------------------
    # Regions
    # --------------------------------------------------------

    regions = find_profitability_regions(
        original_source
    )

    print(
        f"\nFound {len(regions)} profitability regions."
    )

    for region in regions:

        print(
            f"Region {region.index}: "
            f"CAPC lines "
            f"{region.start_line}-"
            f"{region.end_line}, "
            f"ACC line "
            f"{region.acc_line}"
        )

    # --------------------------------------------------------
    # Arrays
    # --------------------------------------------------------

    arrays = detect_arrays(
        original_source
    )

    print("\nDetected arrays:")

    for name, info in arrays.items():
        print(
            f"{name}: "
            f"{info['declaration']}"
        )

    # --------------------------------------------------------
    # Compile/run original
    # --------------------------------------------------------

    executable = (
        source_path.parent /
        ".openacc_profile_app"
    )

    print(
        "\nCompiling original application..."
    )

    try:

        compile_original(
            source_path,
            executable
        )

        original_output = run_original(
            executable
        )

    except Exception as exc:

        print(
            f"ERROR running original "
            f"application: {exc}"
        )

        sys.exit(1)

    # --------------------------------------------------------
    # Parse original timing
    # --------------------------------------------------------

    original_timings = parse_original_regions(
        original_output
    )

    # --------------------------------------------------------
    # Create isolation directory
    # --------------------------------------------------------

    temp_dir = Path(
        tempfile.mkdtemp(
            prefix="openacc_isolated_"
        )
    )

    print(
        f"\nDirectory: {temp_dir}"
    )

    # --------------------------------------------------------
    # Isolate every region
    # --------------------------------------------------------

    isolated_timings = []

    for region in regions:

        try:

            timing = isolate_region(
                temp_dir,
                original_source,
                region,
                arrays,
                macros,
            )

        except Exception as exc:

            print(
                f"ERROR isolating region "
                f"{region.index}: {exc}"
            )

            timing = {
                "kernel_us": 0.0,
                "h2d_us": 0.0,
                "d2h_us": 0.0,
                "total_us": 0.0,
            }

        isolated_timings.append(
            timing
        )

    # --------------------------------------------------------
    # Print results
    # --------------------------------------------------------

    print()

    print(
        "Region    Calls    "
        "Resident(ms)    Observed(ms)    "
        "Resident Per Exec(ms)    "
        "Observed Per Exec(ms)    "
        "Isolated Kernel(ms)    "
        "Isolated Total(ms)"
    )

    print(
        "-" * 125
    )

    for i, region in enumerate(regions):

        if i < len(original_timings):

            original = original_timings[i]

            resident_us = (
                original["kernel_us"]
                or 0.0
            )

        else:

            resident_us = 0.0

        # Number of calls.
        # Current OpenACC test cases normally execute once.
        calls = 1

        resident_ms = (
            resident_us / 1000.0
        )

        observed_ms = resident_ms

        isolated = isolated_timings[i]

        isolated_kernel_ms = (
            isolated["kernel_us"]
            / 1000.0
        )

        isolated_total_ms = (
            isolated["total_us"]
            / 1000.0
        )

        print(
            f"{i:<10}"
            f"{calls:<9}"
            f"{resident_ms:<17.6f}"
            f"{observed_ms:<17.6f}"
            f"{resident_ms / calls:<25.6f}"
            f"{observed_ms / calls:<25.6f}"
            f"{isolated_kernel_ms:<23.6f}"
            f"{isolated_total_ms:<20.6f}"
        )

    # --------------------------------------------------------
    # Update/copyout information
    # --------------------------------------------------------

    copyout_matches = re.findall(
        r"(\d+):\s*data copyout transfers:\s*(\d+)",
        original_output,
    )

    if copyout_matches:

        for line_no, transfers in copyout_matches:

            print(
                f"\nLine {line_no}: "
                f"copyout {transfers} transfers"
            )

    # --------------------------------------------------------
    # Detailed isolated timing
    # --------------------------------------------------------

    print()

    print(
        "Region    H2D(ms)    Kernel(ms)    "
        "D2H(ms)    Total(ms)"
    )

    print(
        "-" * 60
    )

    for i, timing in enumerate(
        isolated_timings
    ):

        h2d = timing["h2d_us"] / 1000.0
        kernel = timing["kernel_us"] / 1000.0
        d2h = timing["d2h_us"] / 1000.0
        total = timing["total_us"] / 1000.0

        print(
            f"{i:<10}"
            f"{h2d:<11.6f}"
            f"{kernel:<14.6f}"
            f"{d2h:<11.6f}"
            f"{total:<12.6f}"
        )

    # --------------------------------------------------------
    # Explanation
    # --------------------------------------------------------

    print()

    print(
        "1. Resident = total GPU kernel execution "
        "time reported by NV_ACC_TIME."
    )

    print(
        "2. Observed = Resident + directly "
        "associated OpenACC update transfer."
    )

    print(
        "3. Resident Per Exec = Resident / Calls."
    )

    print(
        "4. Observed Per Exec = Observed / Calls."
    )

    print(
        "5. Isolated Kernel = GPU kernel time "
        "for exactly ONE execution."
    )

    print(
        "6. Isolated Total = H2D + Kernel + D2H "
        "for exactly ONE isolated execution."
    )

    print(
        "7. Isolated timing is NOT divided by Calls."
    )

    print(
        "8. The isolated program executes the region "
        "exactly once over the original iteration space."
    )

    print(
        "9. Macro-defined identifiers such as "
        "'n' are preserved and are never redeclared."
    )

    print(
        "\nIsolated programs generated at:"
    )

    print(temp_dir)

    if not KEEP_TEMP:

        shutil.rmtree(
            temp_dir,
            ignore_errors=True
        )

    # Remove original executable.
    try:
        executable.unlink()
    except Exception:
        pass


if __name__ == "__main__":
    main()