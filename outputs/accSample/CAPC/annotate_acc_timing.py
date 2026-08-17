#!/usr/bin/env python3
"""
annotate_acc_timing.py

Combined CAPC OpenACC timing profiler.

For each:
    #pragma capc profitability_region begin
        ...
    #pragma capc profitability_region end

the script reports:

    Resident Time
        GPU kernel execution time in the original OpenACC program.

    Observed Time
        Resident Time + explicit OpenACC data-transfer time observed in
        the original program (enter data / exit data / update).

    Isolated Time
        Transfer In + Kernel + Transfer Out measured by executing a
        generated standalone version of the target profitability region.

Usage:
    python annotate_acc_timing.py program_acc.c

Optional:
    python annotate_acc_timing.py program_acc.c --gpu cc70
    python annotate_acc_timing.py program_acc.c --keep-generated
    python annotate_acc_timing.py program_acc.c --isolated-runs 3
"""

import os
import re
import sys
import shutil
import argparse
import tempfile
import subprocess
import resource
from pathlib import Path


# ---------------------------------------------------------------------------
# General configuration
# ---------------------------------------------------------------------------

STANDARD_VARS = {"i", "j", "k", "t"}


try:
    resource.setrlimit(
        resource.RLIMIT_STACK,
        (resource.RLIM_INFINITY, resource.RLIM_INFINITY)
    )
except Exception:
    pass


# ===========================================================================
# PART 1: REGION PARSING
# ===========================================================================

def parse_regions(source_file):
    """
    Parse CAPC profitability regions using source line numbers.

    Returns:
        [
            {
                "id": 1,
                "begin_line": ...,
                "end_line": ...,
                "count": 0,
                "resident_time": 0.0,
                "transfer_time": 0.0,
                "isolated_time": None,
                "isolated_h2d": None,
                "isolated_kernel": None,
                "isolated_d2h": None,
            },
            ...
        ]
    """
    regions = []
    current_region = None
    region_id = 1

    with open(source_file, "r") as f:
        for line_num, line in enumerate(f, start=1):
            line_str = line.strip()

            if "#pragma capc profitability_region begin" in line_str:
                current_region = {
                    "id": region_id,
                    "begin_line": line_num,
                    "end_line": None,
                    "count": 0,
                    "resident_time": 0.0,
                    "transfer_time": 0.0,
                    "isolated_time": None,
                    "isolated_h2d": None,
                    "isolated_kernel": None,
                    "isolated_d2h": None,
                }

            elif (
                "#pragma capc profitability_region end" in line_str
                and current_region is not None
            ):
                current_region["end_line"] = line_num
                regions.append(current_region)
                region_id += 1
                current_region = None

    return regions


def get_associated_region_id(line_num, regions):
    """
    Associate an OpenACC transfer/kernel line with a CAPC region.

    Policy retained from the resident/observed script:
      1. Inside a region              -> that region
      2. Before first region          -> first region
      3. Between two regions          -> preceding region
      4. After final region           -> final region
    """
    if not regions:
        return 1

    for reg in regions:
        if reg["begin_line"] <= line_num <= reg["end_line"]:
            return reg["id"]

    if line_num < regions[0]["begin_line"]:
        return regions[0]["id"]

    for i in range(len(regions) - 1):
        if regions[i]["end_line"] < line_num < regions[i + 1]["begin_line"]:
            return regions[i]["id"]

    return regions[-1]["id"]


# ===========================================================================
# PART 2: RESIDENT + OBSERVED TIMING
# ===========================================================================

def consume_statement(lines, idx):
    """
    Consume the complete C statement/block following an OpenACC compute pragma.

    Returns the first source-line index after that statement/block.
    """
    n = len(lines)

    while idx < n:
        line_str = lines[idx].strip()

        if not line_str or line_str.startswith("//") or line_str.startswith("/*"):
            idx += 1
            continue

        if line_str.startswith("#pragma"):
            idx += 1
            continue

        if "{" in line_str:
            brace_depth = 0

            while idx < n:
                current = lines[idx]
                brace_depth += current.count("{") - current.count("}")
                idx += 1

                if brace_depth <= 0:
                    break

            return idx

        if any(line_str.startswith(kw) for kw in ["for", "while", "if", "do"]):
            idx += 1
            return consume_statement(lines, idx)

        # Ordinary statement.
        while idx < n:
            current = lines[idx]
            idx += 1
            if ";" in current:
                break

        return idx

    return idx


def instrument_openacc_source(source_path, temp_path, regions):
    """
    Instrument the original OpenACC program.

    Kernel timing:
        timer
        #pragma acc parallel/kernels/serial ...
        ...
        #pragma acc wait
        timer

    Transfer timing:
        timer
        #pragma acc enter data / exit data / update ...
        #pragma acc wait
        timer
    """
    with open(source_path, "r") as f:
        lines = f.readlines()

    instrumented = [
        "#include <omp.h>\n",
        "#include <stdio.h>\n",
        "#include <openacc.h>\n",
        "static double _capc_dt0, _capc_dt1;\n",
        "static double _capc_k0, _capc_k1;\n\n",
    ]

    i = 0
    n = len(lines)

    while i < n:
        line = lines[i]
        line_str = line.strip()
        line_num = i + 1

        # ------------------------------------------------------------------
        # Explicit OpenACC data movement
        # ------------------------------------------------------------------
        if (
            "#pragma acc" in line_str
            and any(
                keyword in line_str
                for keyword in ["enter data", "exit data", "update"]
            )
        ):
            reg_id = get_associated_region_id(line_num, regions)

            instrumented.append("  _capc_dt0 = omp_get_wtime();\n")
            instrumented.append(line)
            instrumented.append("  #pragma acc wait\n")
            instrumented.append("  _capc_dt1 = omp_get_wtime();\n")
            instrumented.append(
                f'  printf("[PROFILER] transfer region:{reg_id} '
                f'line:{line_num} | Transfer Time = %.9f s\\n", '
                f'_capc_dt1 - _capc_dt0);\n'
            )

            i += 1
            continue

        # ------------------------------------------------------------------
        # OpenACC compute construct
        # ------------------------------------------------------------------
        if (
            "#pragma acc" in line_str
            and any(
                keyword in line_str
                for keyword in ["parallel", "kernels", "serial"]
            )
        ):
            reg_id = get_associated_region_id(line_num, regions)

            instrumented.append("  _capc_k0 = omp_get_wtime();\n")
            instrumented.append(line)

            i += 1
            end_idx = consume_statement(lines, i)

            while i < end_idx:
                instrumented.append(lines[i])
                i += 1

            instrumented.append("  #pragma acc wait\n")
            instrumented.append("  _capc_k1 = omp_get_wtime();\n")
            instrumented.append(
                f'  printf("[PROFILER] kernel region:{reg_id} '
                f'line:{line_num} | Kernel Execution Time = %.9f s\\n", '
                f'_capc_k1 - _capc_k0);\n'
            )

            continue

        instrumented.append(line)
        i += 1

    with open(temp_path, "w") as f:
        f.writelines(instrumented)


def compile_openacc_program(source_file, exec_name, gpu_arch="cc70"):
    """Compile the instrumented original OpenACC program."""
    compile_cmd = [
        "nvc",
        "-acc",
        "-mp",
        f"-gpu={gpu_arch}",
        "-Minfo=accel",
        source_file,
        "-o",
        exec_name,
    ]

    print(f"[*] Compiling original instrumented program:")
    print("    " + " ".join(compile_cmd))

    result = subprocess.run(
        compile_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    if result.stderr.strip():
        print(result.stderr.strip())

    if result.returncode != 0:
        raise RuntimeError(
            f"Compilation failed for instrumented source:\n{source_file}"
        )


def run_executable(exec_path):
    """Execute a binary and capture stdout/stderr."""
    result = subprocess.run(
        [os.path.abspath(exec_path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    return result.stdout, result.stderr, result.returncode


def process_profiler_output(stdout_str, stderr_str, returncode, regions):
    """
    Parse resident/observed profiler logs and aggregate times.
    """
    combined_log = stdout_str + "\n" + stderr_str

    pattern = re.compile(
        r"\[PROFILER\]\s+"
        r"(kernel|transfer)\s+"
        r"region:(\d+)\s+"
        r"line:(\d+)\s+\|\s+"
        r"(.*?)\s+=\s+"
        r"([\d.eE+\-]+)\s+s"
    )

    matched_events = 0
    region_map = {reg["id"]: reg for reg in regions}

    for line in combined_log.splitlines():
        match = pattern.search(line)

        if not match:
            continue

        matched_events += 1

        event_category = match.group(1)
        region_id = int(match.group(2))
        duration = float(match.group(5))

        if region_id not in region_map:
            continue

        reg = region_map[region_id]

        if event_category == "kernel":
            reg["resident_time"] += duration
            reg["count"] += 1

        elif event_category == "transfer":
            reg["transfer_time"] += duration

    if matched_events == 0:
        print("[!] Warning: no [PROFILER] events were detected.")
        print(f"[!] Original executable return code: {returncode}")


# ===========================================================================
# PART 3: STANDALONE / ISOLATED REGION GENERATION
# ===========================================================================

def get_array_bounds_map(full_code):
    """
    Build:
        variable_name -> OpenACC array section

    Example:
        a -> a[0:N]
    """
    bounds_map = {}

    clause_matches = re.findall(
        r"\b(?:create|copyin|copyout|copy|present|pcopy|pcopyin|pcopyout)"
        r"\s*\(([^)]+)\)",
        full_code,
        re.IGNORECASE,
    )

    for match in clause_matches:
        items = [item.strip() for item in match.split(",")]

        for item in items:
            var_match = re.match(r"^([a-zA-Z_][a-zA-Z0-9_]*)", item)

            if var_match:
                var_name = var_match.group(1)

                if "[" in item and var_name not in bounds_map:
                    bounds_map[var_name] = item

    return bounds_map


def get_target_region_array_specs(target_block, bounds_map):
    """
    Determine arrays referenced by a target profitability region.

    First preference:
        arrays explicitly appearing in OpenACC clauses.

    Fallback:
        indexed array references in the target block.
    """
    target_vars = []

    clause_matches = re.findall(
        r"\b(?:create|copyin|copyout|copy|present|pcopy|pcopyin|pcopyout)"
        r"\s*\(([^)]+)\)",
        target_block,
        re.IGNORECASE,
    )

    for match in clause_matches:
        items = [item.strip() for item in match.split(",")]

        for item in items:
            var_match = re.match(r"^([a-zA-Z_][a-zA-Z0-9_]*)", item)

            if var_match:
                var_name = var_match.group(1)

                if var_name not in target_vars:
                    target_vars.append(var_name)

    if not target_vars:
        indexed_vars = re.findall(
            r"\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\[",
            target_block,
        )

        for var_name in indexed_vars:
            if var_name in bounds_map and var_name not in target_vars:
                target_vars.append(var_name)

    specs = []

    for variable in target_vars:
        if variable in bounds_map:
            specs.append(bounds_map[variable])
        else:
            specs.append(variable)

    return specs, target_vars


def is_block_unclosed_from_line(start_idx, lines):
    depth = 0
    has_opened = False

    for i in range(start_idx, len(lines)):
        line = lines[i]

        opened = line.count("{")
        closed = line.count("}")

        if opened > 0:
            has_opened = True

        depth += opened - closed

        if has_opened and depth <= 0:
            return False

    return True


def sanitize_c_segment(code_str, state=None):
    """
    Clean fragments copied from the original main() into generated
    standalone programs.

    This preserves the logic from the standalone script while avoiding
    dangling profitability markers, declarations of standard loop indices,
    unmatched control blocks, and orphaned preprocessor directives.
    """
    if state is None:
        state = {"suppressed_braces": 0}

    lines = code_str.splitlines()
    clean_lines = []

    if_stack = 0
    local_loop_depth = 0

    for idx, line in enumerate(lines):
        stripped = line.strip()

        if "profitability_region" in stripped:
            continue

        decl_standalone_match = re.match(
            r"^\s*(int|double|float|long)\s+([a-zA-Z0-9_,\s]+)\s*;\s*$",
            line,
        )

        if decl_standalone_match:
            variables = [
                value.strip()
                for value in decl_standalone_match.group(2).split(",")
            ]

            if all(variable in STANDARD_VARS for variable in variables):
                continue

        decl_init_match = re.match(
            r"^\s*(int|double|float|long)\s+"
            r"([a-zA-Z0-9_]+)\s*=(.*);",
            line,
        )

        if decl_init_match:
            var_name = decl_init_match.group(2)
            val_part = decl_init_match.group(3)

            if var_name in STANDARD_VARS:
                line = f"    {var_name} ={val_part};"
                stripped = line.strip()

        if re.match(r"^\s*(for|while|do|if)\b", stripped):
            if is_block_unclosed_from_line(idx, lines):
                if "{" in stripped:
                    state["suppressed_braces"] += stripped.count("{")
                continue

        if stripped == "{":
            if idx > 0 and re.match(
                r"^\s*(for|while|do|if)\b",
                lines[idx - 1],
            ):
                if is_block_unclosed_from_line(idx, lines):
                    state["suppressed_braces"] += 1
                    continue

        if stripped.startswith("}"):
            if state["suppressed_braces"] > 0:
                state["suppressed_braces"] -= 1
                remainder = stripped[1:].strip()

                if not remainder:
                    continue

                line = remainder
                stripped = line.strip()

        if (
            re.match(r"^\s*(for|while|do)\b", stripped)
            and not is_block_unclosed_from_line(idx, lines)
        ):
            local_loop_depth += stripped.count("{")

        if "}" in stripped and local_loop_depth > 0:
            local_loop_depth -= stripped.count("}")

            if local_loop_depth < 0:
                local_loop_depth = 0

        if (
            stripped in ("break;", "continue;")
            or re.match(r"^\s*(break|continue)\s*;\s*$", stripped)
        ):
            if local_loop_depth == 0:
                clean_lines.append(
                    f"    // {stripped}  "
                    f"/* Skipped break/continue outside loop */"
                )
                continue

        if re.match(r"^\s*#\s*(if|ifdef|ifndef)\b", stripped):
            if_stack += 1
            clean_lines.append(line)

        elif re.match(r"^\s*#\s*endif\b", stripped):
            if if_stack > 0:
                if_stack -= 1
                clean_lines.append(line)
            else:
                clean_lines.append(
                    f"// {line}  /* Skipped orphaned #endif */"
                )

        elif re.match(r"^\s*#\s*(else|elif)\b", stripped):
            if if_stack > 0:
                clean_lines.append(line)
            else:
                clean_lines.append(
                    f"// {line}  "
                    f"/* Skipped orphaned preprocessor directive */"
                )

        else:
            clean_lines.append(line)

    while if_stack > 0:
        clean_lines.append(
            "#endif /* Auto-closed for standalone segment balance */"
        )
        if_stack -= 1

    return "\n".join(clean_lines)


def parse_c_file_for_isolated(file_path):
    """
    Parse original source for standalone-region generation.
    """
    with open(file_path, "r") as f:
        content = f.read()

    bounds_map = get_array_bounds_map(content)

    region_pattern = re.compile(
        r"(#pragma\s+capc\s+profitability_region\s+begin[^\n]*\n)"
        r"(.*?)"
        r"(#pragma\s+capc\s+profitability_region\s+end[^\n]*)",
        re.DOTALL | re.IGNORECASE,
    )

    region_matches = list(region_pattern.finditer(content))

    if not region_matches:
        raise ValueError(
            "No '#pragma capc profitability_region begin/end' markers found."
        )

    main_match = re.search(
        r"(int\s+main\s*\([^)]*\)\s*\{)",
        content,
    )

    if not main_match:
        raise ValueError("Could not locate main() in input file.")

    main_start = main_match.end()
    header_code = content[:main_match.start()]
    main_opening = main_match.group(1)
    main_body = content[main_start:]

    parsed_regions = []

    for idx, match in enumerate(region_matches, start=1):
        begin_line = match.group(1).strip()
        body_code = match.group(2).strip()
        end_line = match.group(3).strip()

        id_match = re.search(
            r"begin\s*(?:\(\s*(\w+)\s*\)|\s+(\w+))",
            begin_line,
            re.IGNORECASE,
        )

        if id_match:
            region_id = id_match.group(1) or id_match.group(2)
        else:
            region_id = str(idx)

        full_region_block = (
            f"    {begin_line}\n"
            f"    {body_code}\n"
            f"    {end_line}"
        )

        is_in_main = match.start() >= main_match.start()

        parsed_regions.append(
            (
                region_id,
                full_region_block,
                match.start(),
                match.end(),
                is_in_main,
            )
        )

    main_regions = [region for region in parsed_regions if region[4]]

    raw_main_segments = []
    last_pos = 0

    for _, _, start_pos, end_pos, _ in main_regions:
        rel_start = start_pos - main_start
        rel_end = end_pos - main_start

        raw_main_segments.append(
            main_body[last_pos:rel_start]
        )

        last_pos = rel_end

    raw_main_segments.append(main_body[last_pos:])

    return (
        header_code,
        main_opening,
        raw_main_segments,
        parsed_regions,
        main_regions,
        bounds_map,
    )


def clean_directory(output_dir):
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)

    os.makedirs(output_dir, exist_ok=True)


def generate_standalone_files(
    header_code,
    main_opening,
    raw_main_segments,
    parsed_regions,
    main_regions,
    bounds_map,
    output_dir,
):
    """
    Generate one standalone C source for every profitability region.
    """
    clean_directory(output_dir)

    all_global_specs = [bounds_map[v] for v in bounds_map]
    global_specs_str = (
        ", ".join(all_global_specs)
        if all_global_specs
        else ""
    )

    generated_files = []

    for target_index, (
        target_id,
        target_block,
        start_pos,
        end_pos,
        is_in_main,
    ) in enumerate(parsed_regions):

        filename = os.path.join(
            output_dir,
            f"region_{target_id}_standalone.c",
        )

        main_preceding = [
            region
            for region in main_regions
            if region[2] < start_pos
        ]

        m_count = len(main_preceding)
        has_prior_dependencies = m_count > 0

        state = {"suppressed_braces": 0}

        array_specs, target_var_names = get_target_region_array_specs(
            target_block,
            bounds_map,
        )

        target_pragma_lower = target_block.lower()

        # ---------------------------------------------------------------
        # Decide which arrays need host -> device movement before target.
        # ---------------------------------------------------------------
        pre_copyin_specs = []

        for spec in array_specs:
            var_name = spec.split("[")[0].strip()

            is_copyout = (
                "copyout(" in target_pragma_lower
                and var_name in target_pragma_lower
            )

            is_copyin_or_used = any(
                clause in target_pragma_lower
                for clause in [
                    f"copyin({var_name}",
                    f"copy({var_name}",
                    f"present({var_name}",
                ]
            )

            if not (is_copyout and not is_copyin_or_used):
                pre_copyin_specs.append(spec)

        pre_copyin_str = (
            ", ".join(pre_copyin_specs)
            if pre_copyin_specs
            else ""
        )

        with open(filename, "w") as f:
            f.write("#define _GNU_SOURCE\n")
            f.write("#define _POSIX_C_SOURCE 199309L\n")
            f.write("#include <time.h>\n")
            f.write("#include <stdio.h>\n\n")

            f.write(header_code + "\n")
            f.write(main_opening + "\n\n")

            f.write("    int i, j, k, t;\n")
            f.write("    struct timespec t_start, t_end;\n")
            f.write(
                "    double t_in = 0.0, "
                "t_gpu = 0.0, "
                "t_out = 0.0;\n\n"
            )

            f.write(
                "    /* === STAGE 1 & 2: "
                "Setup + prerequisite regions === */\n"
            )

            has_enter_data_in_setup = False

            for k in range(m_count):
                if k < len(raw_main_segments):
                    seg_clean = sanitize_c_segment(
                        raw_main_segments[k],
                        state,
                    ).strip()

                    if seg_clean:
                        if "enter data" in seg_clean.lower():
                            has_enter_data_in_setup = True

                        f.write(f"    {seg_clean}\n\n")

                f.write(
                    f"    /* Dependent Region "
                    f"{main_preceding[k][0]} */\n"
                )
                f.write(f"    {main_preceding[k][1]}\n")

                if not main_preceding[k][1].strip().endswith(
                    "#pragma acc wait"
                ):
                    f.write("    #pragma acc wait\n\n")
                else:
                    f.write("\n")

            if m_count < len(raw_main_segments):
                seg_clean = sanitize_c_segment(
                    raw_main_segments[m_count],
                    state,
                ).strip()

                if seg_clean:
                    if "enter data" in seg_clean.lower():
                        has_enter_data_in_setup = True

                    f.write(f"    {seg_clean}\n\n")

            # -----------------------------------------------------------
            # Device allocation when setup has not already done it.
            # -----------------------------------------------------------
            if (
                global_specs_str
                and not has_prior_dependencies
                and not has_enter_data_in_setup
            ):
                f.write(
                    "    /* Ensure array allocation on device */\n"
                )
                f.write(
                    f"    #pragma acc enter data "
                    f"create({global_specs_str})\n"
                )
                f.write("    #pragma acc wait\n\n")

            # -----------------------------------------------------------
            # Transfer In
            # -----------------------------------------------------------
            if pre_copyin_str and not has_prior_dependencies:
                f.write(
                    "    /* === Transfer In (Host -> Device) === */\n"
                )
                f.write(
                    "    clock_gettime(CLOCK_MONOTONIC, &t_start);\n"
                )
                f.write(
                    f"    #pragma acc update "
                    f"device({pre_copyin_str})\n"
                )
                f.write("    #pragma acc wait\n")
                f.write(
                    "    clock_gettime(CLOCK_MONOTONIC, &t_end);\n"
                )
                f.write(
                    "    t_in = "
                    "(t_end.tv_sec - t_start.tv_sec) + "
                    "(t_end.tv_nsec - t_start.tv_nsec) / 1e9;\n\n"
                )
            else:
                f.write(
                    "    /* Pre-timing copyin skipped: "
                    "write-only/no input arrays or prior dependencies. */\n\n"
                )

            # -----------------------------------------------------------
            # Target kernel
            # -----------------------------------------------------------
            f.write(
                f"    /* === Isolated kernel timing: "
                f"Region {target_id} === */\n"
            )
            f.write(
                "    clock_gettime(CLOCK_MONOTONIC, &t_start);\n\n"
            )

            f.write(f"    {target_block}\n\n")

            f.write("    #pragma acc wait\n")
            f.write(
                "    clock_gettime(CLOCK_MONOTONIC, &t_end);\n"
            )
            f.write(
                "    t_gpu = "
                "(t_end.tv_sec - t_start.tv_sec) + "
                "(t_end.tv_nsec - t_start.tv_nsec) / 1e9;\n\n"
            )

            # -----------------------------------------------------------
            # Transfer Out
            # -----------------------------------------------------------
            has_explicit_copyout = any(
                clause in target_pragma_lower
                for clause in ["copyout", "copy("]
            )

            if not has_explicit_copyout and array_specs:
                specs_str = ", ".join(array_specs)

                f.write(
                    "    /* === Transfer Out (Device -> Host) === */\n"
                )
                f.write(
                    "    clock_gettime(CLOCK_MONOTONIC, &t_start);\n"
                )
                f.write(
                    f"    #pragma acc update self({specs_str})\n"
                )
                f.write("    #pragma acc wait\n")
                f.write(
                    "    clock_gettime(CLOCK_MONOTONIC, &t_end);\n"
                )
                f.write(
                    "    t_out = "
                    "(t_end.tv_sec - t_start.tv_sec) + "
                    "(t_end.tv_nsec - t_start.tv_nsec) / 1e9;\n\n"
                )
            else:
                f.write(
                    "    /* Copyout skipped: explicit copyout/copy "
                    "or no detected array sections. */\n\n"
                )

            # -----------------------------------------------------------
            # Machine-readable output for Python parser
            # -----------------------------------------------------------
            f.write(
                "    double t_total = t_in + t_gpu + t_out;\n\n"
            )

            f.write(
                f'    printf("[ISOLATED] region:{target_id} '
                f'| H2D = %.9f s '
                f'| Kernel = %.9f s '
                f'| D2H = %.9f s '
                f'| Total = %.9f s\\n", '
                f't_in, t_gpu, t_out, t_total);\n'
            )

            f.write("    return 0;\n")
            f.write("}\n")

        generated_files.append((str(target_id), filename))

    return generated_files


# ===========================================================================
# PART 4: COMPILE + RUN ISOLATED REGIONS
# ===========================================================================

def compile_isolated_region(
    c_file,
    gpu_arch="cc70",
):
    exe_file = os.path.splitext(c_file)[0]

    compile_cmd = [
        "nvc",
        "-acc",
        "-mp",
        f"-gpu={gpu_arch}",
        "--diag_suppress",
        "declared_but_not_referenced",
        c_file,
        "-o",
        exe_file,
    ]

    print("    " + " ".join(compile_cmd))

    result = subprocess.run(
        compile_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    if result.stderr.strip():
        print(result.stderr.strip())

    if result.returncode != 0:
        return None

    return exe_file


def parse_isolated_output(text):
    pattern = re.compile(
        r"\[ISOLATED\]\s+region:(\S+)\s+\|\s+"
        r"H2D\s*=\s*([\d.eE+\-]+)\s+s\s+\|\s+"
        r"Kernel\s*=\s*([\d.eE+\-]+)\s+s\s+\|\s+"
        r"D2H\s*=\s*([\d.eE+\-]+)\s+s\s+\|\s+"
        r"Total\s*=\s*([\d.eE+\-]+)\s+s"
    )

    match = pattern.search(text)

    if not match:
        return None

    return {
        "region_id": match.group(1),
        "h2d": float(match.group(2)),
        "kernel": float(match.group(3)),
        "d2h": float(match.group(4)),
        "total": float(match.group(5)),
    }


def run_isolated_regions(
    generated_files,
    regions,
    gpu_arch="cc70",
    isolated_runs=1,
):
    """
    Compile and execute every standalone region.

    If --isolated-runs N is used, run the standalone executable N times
    and store the arithmetic mean of H2D, kernel, D2H and total.
    """
    region_map = {str(reg["id"]): reg for reg in regions}

    print("\n" + "=" * 76)
    print(" ISOLATED REGION COMPILATION / EXECUTION")
    print("=" * 76)

    for target_id, c_file in generated_files:
        print(f"\n[*] Isolated Region {target_id}")
        print("[*] Compiling:")

        exe_file = compile_isolated_region(
            c_file,
            gpu_arch=gpu_arch,
        )

        if exe_file is None:
            print(
                f"[!] Region {target_id}: standalone compilation failed."
            )
            continue

        measurements = []

        for run_index in range(isolated_runs):
            stdout_str, stderr_str, returncode = run_executable(exe_file)
            combined = stdout_str + "\n" + stderr_str

            if returncode != 0:
                print(
                    f"[!] Region {target_id}: run "
                    f"{run_index + 1} failed with code {returncode}."
                )

                if stderr_str.strip():
                    print(stderr_str.strip())

                continue

            parsed = parse_isolated_output(combined)

            if parsed is None:
                print(
                    f"[!] Region {target_id}: isolated timing "
                    f"output could not be parsed."
                )
                continue

            measurements.append(parsed)

        if not measurements:
            continue

        avg_h2d = sum(x["h2d"] for x in measurements) / len(measurements)
        avg_kernel = (
            sum(x["kernel"] for x in measurements)
            / len(measurements)
        )
        avg_d2h = sum(x["d2h"] for x in measurements) / len(measurements)
        avg_total = (
            sum(x["total"] for x in measurements)
            / len(measurements)
        )

        if target_id in region_map:
            reg = region_map[target_id]
            reg["isolated_h2d"] = avg_h2d
            reg["isolated_kernel"] = avg_kernel
            reg["isolated_d2h"] = avg_d2h
            reg["isolated_time"] = avg_total

        print(
            f"    H2D={avg_h2d:.9f} s, "
            f"Kernel={avg_kernel:.9f} s, "
            f"D2H={avg_d2h:.9f} s, "
            f"Isolated={avg_total:.9f} s"
        )


# ===========================================================================
# PART 5: FINAL REPORT
# ===========================================================================

def format_time(value):
    if value is None:
        return "N/A"

    return f"{value:.6f}"


def print_results(regions):
    """
    Final combined CAPC report.

    Resident:
        kernel time from original execution.

    Observed:
        resident + original-program explicit transfers.

    Isolated:
        H2D + standalone target kernel + D2H.
    """
    header = (
        f"{'Region':<8} | "
        f"{'Lines':<11} | "
        f"{'Calls':<8} | "
        f"{'Total Res(s)':<13} | "
        f"{'Avg Res(s)':<12} | "
        f"{'Total Obs(s)':<13} | "
        f"{'Avg Obs(s)':<12} | "
        f"{'Isolated(s)':<12}"
    )

    divider = "-" * len(header)

    print("\n" + divider)
    print(
        "             CAPC PROFITABILITY REGION TIMING REPORT "
        "(OPENACC)"
    )
    print(divider)
    print(header)
    print(divider)

    total_resident = 0.0
    total_observed = 0.0
    total_invocations = 0

    for reg in regions:
        count_for_average = max(reg["count"], 1)

        observed_time = (
            reg["resident_time"]
            + reg["transfer_time"]
        )

        avg_resident = (
            reg["resident_time"]
            / count_for_average
        )

        avg_observed = (
            observed_time
            / count_for_average
        )

        total_resident += reg["resident_time"]
        total_observed += observed_time
        total_invocations += reg["count"]

        line_range = (
            f"{reg['begin_line']}-"
            f"{reg['end_line']}"
        )

        print(
            f"Region {reg['id']:<1} | "
            f"{line_range:<11} | "
            f"{reg['count']:<8} | "
            f"{reg['resident_time']:<13.6f} | "
            f"{avg_resident:<12.6f} | "
            f"{observed_time:<13.6f} | "
            f"{avg_observed:<12.6f} | "
            f"{format_time(reg['isolated_time']):<12}"
        )

    print(divider)

    avg_total_resident = (
        total_resident / max(total_invocations, 1)
    )

    avg_total_observed = (
        total_observed / max(total_invocations, 1)
    )

    print(
        f"{'TOTAL':<8} | "
        f"{'-':<11} | "
        f"{total_invocations:<8} | "
        f"{total_resident:<13.6f} | "
        f"{avg_total_resident:<12.6f} | "
        f"{total_observed:<13.6f} | "
        f"{avg_total_observed:<12.6f} | "
        f"{'-':<12}"
    )

    print(divider)

    print("\nIsolated timing breakdown:")
    breakdown_header = (
        f"{'Region':<8} | "
        f"{'H2D(s)':<12} | "
        f"{'Kernel(s)':<12} | "
        f"{'D2H(s)':<12} | "
        f"{'Isolated(s)':<12}"
    )

    breakdown_divider = "-" * len(breakdown_header)

    print(breakdown_divider)
    print(breakdown_header)
    print(breakdown_divider)

    for reg in regions:
        print(
            f"Region {reg['id']:<1} | "
            f"{format_time(reg['isolated_h2d']):<12} | "
            f"{format_time(reg['isolated_kernel']):<12} | "
            f"{format_time(reg['isolated_d2h']):<12} | "
            f"{format_time(reg['isolated_time']):<12}"
        )

    print(breakdown_divider)


# ===========================================================================
# MAIN
# ===========================================================================

def main():
    parser = argparse.ArgumentParser(
        description=(
            "Combined CAPC OpenACC profiler for Resident, "
            "Observed and Isolated profitability-region timing."
        )
    )

    parser.add_argument(
        "source",
        help="Path to OpenACC C source file",
    )

    parser.add_argument(
        "--gpu",
        default="cc70",
        help="GPU architecture passed to nvc (default: cc70)",
    )

    parser.add_argument(
        "--isolated-runs",
        type=int,
        default=1,
        help=(
            "Number of executions of each standalone region "
            "to average (default: 1)"
        ),
    )

    parser.add_argument(
        "--keep-generated",
        action="store_true",
        help="Keep generated standalone C files and binaries",
    )

    args = parser.parse_args()

    if args.isolated_runs < 1:
        parser.error("--isolated-runs must be >= 1")

    source_path = os.path.abspath(args.source)

    if not os.path.exists(source_path):
        print(
            f"Error: source file '{args.source}' does not exist.",
            file=sys.stderr,
        )
        sys.exit(1)

    work_dir = os.path.dirname(source_path)
    source_base = os.path.splitext(
        os.path.basename(source_path)
    )[0]

    # ------------------------------------------------------------------
    # Parse source regions once.
    # ------------------------------------------------------------------
    regions = parse_regions(source_path)

    if not regions:
        print(
            "Error: no '#pragma capc profitability_region' "
            "blocks found.",
            file=sys.stderr,
        )
        sys.exit(1)

    print(f"[*] Source: {source_path}")
    print(f"[*] Detected CAPC regions: {len(regions)}")
    print(f"[*] GPU architecture: {args.gpu}")

    # ------------------------------------------------------------------
    # Resident + Observed
    # ------------------------------------------------------------------
    print("\n" + "=" * 76)
    print(" RESIDENT + OBSERVED PROFILING")
    print("=" * 76)

    temp_fd, instrumented_path = tempfile.mkstemp(
        suffix="_capc_profiled.c",
        dir=work_dir,
    )
    os.close(temp_fd)

    resident_exec_path = os.path.join(
        work_dir,
        f".{source_base}_capc_profiled",
    )

    standalone_dir = os.path.join(
        work_dir,
        f".{source_base}_standalone_regions",
    )

    try:
        instrument_openacc_source(
            source_path,
            instrumented_path,
            regions,
        )

        compile_openacc_program(
            instrumented_path,
            resident_exec_path,
            gpu_arch=args.gpu,
        )

        print("\n[*] Running original instrumented program...")

        stdout_str, stderr_str, returncode = run_executable(
            resident_exec_path
        )

        if returncode != 0:
            print(
                f"[!] Original instrumented program returned "
                f"code {returncode}."
            )

            if stderr_str.strip():
                print(stderr_str.strip())

        process_profiler_output(
            stdout_str,
            stderr_str,
            returncode,
            regions,
        )

        # --------------------------------------------------------------
        # Isolated
        # --------------------------------------------------------------
        (
            header_code,
            main_opening,
            raw_main_segments,
            parsed_regions,
            main_regions,
            bounds_map,
        ) = parse_c_file_for_isolated(source_path)

        generated_files = generate_standalone_files(
            header_code,
            main_opening,
            raw_main_segments,
            parsed_regions,
            main_regions,
            bounds_map,
            standalone_dir,
        )

        run_isolated_regions(
            generated_files,
            regions,
            gpu_arch=args.gpu,
            isolated_runs=args.isolated_runs,
        )

        # --------------------------------------------------------------
        # Final combined output
        # --------------------------------------------------------------
        print_results(regions)

    finally:
        if os.path.exists(instrumented_path):
            os.remove(instrumented_path)

        if os.path.exists(resident_exec_path):
            os.remove(resident_exec_path)

        if not args.keep_generated:
            if os.path.exists(standalone_dir):
                shutil.rmtree(standalone_dir)
        else:
            print(
                f"\n[*] Generated standalone files retained in:\n"
                f"    {standalone_dir}"
            )


if __name__ == "__main__":
    main()