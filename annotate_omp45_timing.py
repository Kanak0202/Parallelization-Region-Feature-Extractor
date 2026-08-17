#!/usr/bin/env python3
import os
import sys
import re
import shutil
import subprocess
import argparse
import tempfile
import resource

# ============================================================
# CAPC OpenMP 4.5 Timing Profiler
#
# Measures:
#   1. Resident time:
#        GPU target-region execution time with the data behavior
#        of the original OpenMP program.
#
#   2. Observed time:
#        Resident time + explicit OpenMP target data-transfer
#        operations observed in the original program.
#
#   3. Isolated time:
#        Standalone execution of each CAPC region:
#        H2D + region execution + D2H.
#
# Usage:
#   python annotate_omp45_timing.py program.c
#   python annotate_omp45_timing.py program.c --gpu cc70
#
# Default compiler:
#   nvc -mp=gpu -gpu=cc70 -Minfo=mp
# ============================================================

try:
    resource.setrlimit(
        resource.RLIMIT_STACK,
        (resource.RLIM_INFINITY, resource.RLIM_INFINITY)
    )
except Exception:
    pass


STANDARD_VARS = {"i", "j", "k", "t"}


# ============================================================
# COMMON REGION PARSING
# ============================================================

def parse_regions(source_file):
    """
    Parse:
        #pragma capc profitability_region begin
        ...
        #pragma capc profitability_region end

    Region IDs are assigned in source order starting from 1.
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

                    # Resident / observed measurement
                    "count": 0,
                    "resident_time": 0.0,
                    "transfer_time": 0.0,

                    # Isolated measurement
                    "isolated_h2d": 0.0,
                    "isolated_kernel": 0.0,
                    "isolated_d2h": 0.0,
                    "isolated_time": None,
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
    Associate an OpenMP target/data operation with a CAPC region.

    Rules:
      1. If inside a CAPC region -> that region.
      2. Before first region -> Region 1.
      3. Between two regions -> preceding region.
      4. After final region -> final region.

    This preserves the behavior of the original resident/observed script.
    """
    if not regions:
        return 1

    for reg in regions:
        if reg["begin_line"] <= line_num <= reg["end_line"]:
            return reg["id"]

    if line_num < regions[0]["begin_line"]:
        return regions[0]["id"]

    for i in range(len(regions) - 1):
        if (
            regions[i]["end_line"]
            < line_num
            < regions[i + 1]["begin_line"]
        ):
            return regions[i]["id"]

    return regions[-1]["id"]


# ============================================================
# RESIDENT + OBSERVED PROFILING
# ============================================================

def consume_statement(lines, idx):
    """
    Consume the complete C statement/block following an OpenMP directive.

    Returns the first source-line index after the associated statement.
    """
    n = len(lines)

    while idx < n:
        line_str = lines[idx].strip()

        if not line_str or line_str.startswith("//") or line_str.startswith("/*"):
            idx += 1
            continue

        # Skip stacked pragmas and find the actual statement.
        if line_str.startswith("#pragma"):
            idx += 1
            continue

        # Braced statement/block.
        if "{" in line_str:
            brace_depth = 0

            while idx < n:
                l = lines[idx]
                brace_depth += l.count("{") - l.count("}")
                idx += 1

                if brace_depth <= 0:
                    break

            return idx

        # Control construct whose body may be on following line.
        if any(
            line_str.startswith(kw)
            for kw in ["for", "while", "if", "do"]
        ):
            idx += 1
            idx = consume_statement(lines, idx)
            return idx

        # Ordinary statement.
        idx += 1
        while idx < n and ";" not in line_str:
            line_str = lines[idx].strip()
            idx += 1

        return idx

    return idx


def instrument_openmp_source(source_path, temp_path, regions):
    """
    Create a temporary copy of the original OpenMP source.

    Timers are inserted around:
      * explicit target enter/exit/update operations -> transfer time
      * OpenMP target compute constructs             -> resident time

    The original input file is never modified.
    """
    with open(source_path, "r") as f:
        lines = f.readlines()

    instrumented = [
        "#include <omp.h>\n",
        "#include <stdio.h>\n",
        "static double _capc_dt0, _capc_dt1;\n",
        "static double _capc_k0, _capc_k1;\n\n",
    ]

    i = 0
    n = len(lines)

    while i < n:
        line = lines[i]
        line_str = line.strip()
        line_num = i + 1
        lower = line_str.lower()

        # ----------------------------------------------------
        # A. Explicit target data movement
        # ----------------------------------------------------
        is_explicit_transfer = (
            "#pragma omp target" in lower
            and any(
                kw in lower
                for kw in ["enter data", "exit data", "update"]
            )
        )

        if is_explicit_transfer:
            reg_id = get_associated_region_id(line_num, regions)

            instrumented.append("  _capc_dt0 = omp_get_wtime();\n")
            instrumented.append(line)
            instrumented.append("  _capc_dt1 = omp_get_wtime();\n")
            instrumented.append(
                f'  printf("[PROFILER] transfer region:{reg_id} '
                f'line:{line_num} | Transfer Time = %.9f s\\n", '
                f'_capc_dt1 - _capc_dt0);\n'
            )

            i += 1
            continue

        # ----------------------------------------------------
        # B. OpenMP target compute region
        # ----------------------------------------------------
        is_target_compute = (
            "#pragma omp target" in lower
            and "enter data" not in lower
            and "exit data" not in lower
            and "update" not in lower
            and "target data" not in lower
        )

        if is_target_compute:
            reg_id = get_associated_region_id(line_num, regions)

            instrumented.append("  _capc_k0 = omp_get_wtime();\n")
            instrumented.append(line)

            i += 1
            end_idx = consume_statement(lines, i)

            while i < end_idx:
                instrumented.append(lines[i])
                i += 1

            # omp_get_wtime() after the target construct forces us to
            # measure completion for synchronous target constructs.
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


def compile_openmp_program(
    source_file,
    exec_name,
    compiler="nvc",
    gpu_arch="cc70"
):
    compile_cmd = [
        compiler,
        "-mp=gpu",
        f"-gpu={gpu_arch}",
        "-Minfo=mp",
        source_file,
        "-o",
        exec_name,
    ]

    print(f"[*] Compiling original/instrumented program:")
    print("    " + " ".join(compile_cmd))

    res = subprocess.run(
        compile_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    if res.stdout.strip():
        print(res.stdout.strip())

    if res.returncode != 0:
        print("\n[-] Compilation failed.")
        print(res.stderr)
        raise RuntimeError("Compilation of instrumented OpenMP program failed.")

    if res.stderr.strip():
        print(res.stderr.strip())


def run_executable(exec_path):
    print(f"\n[*] Executing instrumented program: {exec_path}\n")

    res = subprocess.run(
        [exec_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    return res.stdout, res.stderr, res.returncode


def process_profiler_output(stdout_str, stderr_str, returncode, regions):
    """
    Populate:
        resident_time
        transfer_time
        count
    """
    combined_log = stdout_str + "\n" + stderr_str

    pattern = re.compile(
        r"\[PROFILER\]\s+"
        r"(kernel|transfer)\s+"
        r"region:(\d+)\s+"
        r"line:(\d+)\s+\|\s+"
        r"(.*?)\s+=\s+"
        r"([\d\.eE+\-]+)\s+s"
    )

    matched_events = 0
    region_map = {reg["id"]: reg for reg in regions}

    for line in combined_log.splitlines():
        match = pattern.search(line)

        if not match:
            continue

        matched_events += 1
        event_cat = match.group(1)
        reg_id = int(match.group(2))
        duration = float(match.group(5))

        if reg_id not in region_map:
            continue

        reg = region_map[reg_id]

        if event_cat == "kernel":
            reg["resident_time"] += duration
            reg["count"] += 1

        elif event_cat == "transfer":
            reg["transfer_time"] += duration

    if matched_events == 0:
        print("[!] Warning: no [PROFILER] timing records were detected.")
        print(f"[!] Executable return code: {returncode}")

    if returncode != 0:
        print(
            f"[!] Warning: instrumented executable returned code {returncode}."
        )


# ============================================================
# ISOLATED PROFILING
# ============================================================

def get_array_bounds_map(full_code):
    """
    Build a mapping such as:
        a -> a[0:N]
        A -> A[0:N][0:N]

    The information is recovered from existing OpenMP/OpenACC
    data clauses in the generated source.
    """
    bounds_map = {}

    clause_matches = re.findall(
        r"\b(?:map|create|copyin|copyout|copy|present)"
        r"\s*\(\s*"
        r"(?:to|from|tofrom|alloc|release|delete)?"
        r"\s*:?\s*([^)]+)\)",
        full_code,
        re.IGNORECASE,
    )

    for match in clause_matches:
        items = [item.strip() for item in match.split(",")]

        for item in items:
            var_match = re.match(r"^([a-zA-Z_][a-zA-Z0-9_]*)", item)

            if not var_match:
                continue

            var_name = var_match.group(1)

            if "[" in item and var_name not in bounds_map:
                bounds_map[var_name] = item

    return bounds_map


def get_target_region_array_specs(target_block, bounds_map):
    """
    Return:
        specs: array sections where available
        vars : variable names
    """
    target_vars = []

    clause_matches = re.findall(
        r"\b(?:map|create|copyin|copyout|copy|present)"
        r"\s*\(\s*"
        r"(?:to|from|tofrom|alloc|release|delete)?"
        r"\s*:?\s*([^)]+)\)",
        target_block,
        re.IGNORECASE,
    )

    for match in clause_matches:
        items = [item.strip() for item in match.split(",")]

        for item in items:
            var_match = re.match(
                r"^([a-zA-Z_][a-zA-Z0-9_]*)",
                item
            )

            if var_match:
                var_name = var_match.group(1)

                if var_name not in target_vars:
                    target_vars.append(var_name)

    # Fallback: inspect indexed variables in the region.
    if not target_vars:
        indexed_vars = re.findall(
            r"\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\[",
            target_block
        )

        for var in indexed_vars:
            if var in bounds_map and var not in target_vars:
                target_vars.append(var)

    specs = []

    for var in target_vars:
        if var in bounds_map:
            specs.append(bounds_map[var])
        else:
            specs.append(var)

    return specs, target_vars


def is_block_unclosed_from_line(start_idx, lines):
    depth = 0
    has_opened = False

    for i in range(start_idx, len(lines)):
        line = lines[i]
        opens = line.count("{")
        closes = line.count("}")

        if opens > 0:
            has_opened = True

        depth += opens - closes

        if has_opened and depth <= 0:
            return False

    return True


def sanitize_c_segment(code_str, state=None):
    """
    Retain setup statements needed before a standalone region while
    removing CAPC markers and problematic dangling control structures.

    This is intentionally based on the original isolated script.
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

        # Avoid duplicate declarations of common loop variables.
        decl_standalone_match = re.match(
            r"^\s*(int|double|float|long)\s+"
            r"([a-zA-Z0-9_,\s]+)\s*;\s*$",
            line
        )

        if decl_standalone_match:
            vars_list = [
                v.strip()
                for v in decl_standalone_match.group(2).split(",")
            ]

            if vars_list and all(v in STANDARD_VARS for v in vars_list):
                continue

        # Convert declaration+initialization of standard loop variables
        # into assignment because i/j/k/t are declared in standalone main.
        decl_init_match = re.match(
            r"^\s*(int|double|float|long)\s+"
            r"([a-zA-Z0-9_]+)\s*=(.*);",
            line
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
            if (
                idx > 0
                and re.match(
                    r"^\s*(for|while|do|if)\b",
                    lines[idx - 1]
                )
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
                    f"    // {stripped} "
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
                    f"// {line} /* Skipped orphaned #endif */"
                )

        elif re.match(r"^\s*#\s*(else|elif)\b", stripped):
            if if_stack > 0:
                clean_lines.append(line)
            else:
                clean_lines.append(
                    f"// {line} "
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
    Parse the original program in the same form used by the standalone
    region generator.
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
        content
    )

    if not main_match:
        raise ValueError("Could not locate main() function.")

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

    main_regions = [r for r in parsed_regions if r[4]]

    raw_main_segments = []
    last_pos = 0

    for _, _, start_pos, end_pos, _ in main_regions:
        rel_start = start_pos - main_start
        rel_end = end_pos - main_start

        seg = main_body[last_pos:rel_start]
        raw_main_segments.append(seg)
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


def remove_existing_target_data_pragmas(region_block):
    """
    For isolated measurement, the target compute construct itself is retained.

    Explicit target enter/exit/update pragmas inside the CAPC region are
    removed so that H2D/D2H are measured only by the standalone wrapper.
    """
    out = []

    for line in region_block.splitlines():
        lower = line.strip().lower()

        if (
            "#pragma omp target" in lower
            and any(
                x in lower
                for x in ["enter data", "exit data", "update"]
            )
        ):
            continue

        out.append(line)

    return "\n".join(out)


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
    Generate one standalone C program per CAPC profitability region.
    """
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)

    os.makedirs(output_dir, exist_ok=True)

    all_global_specs = [bounds_map[v] for v in bounds_map]
    global_specs_str = (
        ", ".join(all_global_specs)
        if all_global_specs
        else ""
    )

    generated_files = []

    for (
        target_idx,
        (
            target_id,
            target_block,
            start_pos,
            end_pos,
            is_in_main,
        ),
    ) in enumerate(parsed_regions):

        filename = os.path.join(
            output_dir,
            f"region_{target_id}_standalone.c"
        )

        main_preceding = [
            r for r in main_regions
            if r[2] < start_pos
        ]

        m_count = len(main_preceding)
        has_prior_dependencies = m_count > 0
        state = {"suppressed_braces": 0}

        array_specs, target_var_names = get_target_region_array_specs(
            target_block,
            bounds_map
        )

        target_pragma_lower = target_block.lower()

        # Determine which region arrays need H2D before isolated execution.
        pre_copyin_specs = []

        for spec in array_specs:
            var_name = spec.split("[")[0].strip()

            # Per-variable clause checks.
            write_only_patterns = [
                rf"map\s*\(\s*from\s*:\s*[^)]*\b{re.escape(var_name)}\b",
                rf"map\s*\(\s*alloc\s*:\s*[^)]*\b{re.escape(var_name)}\b",
            ]

            read_patterns = [
                rf"map\s*\(\s*to\s*:\s*[^)]*\b{re.escape(var_name)}\b",
                rf"map\s*\(\s*tofrom\s*:\s*[^)]*\b{re.escape(var_name)}\b",
            ]

            is_write_only = any(
                re.search(p, target_block, re.IGNORECASE)
                for p in write_only_patterns
            )

            is_read_used = any(
                re.search(p, target_block, re.IGNORECASE)
                for p in read_patterns
            )

            if not (is_write_only and not is_read_used):
                pre_copyin_specs.append(spec)

        pre_copyin_str = ", ".join(pre_copyin_specs)

        with open(filename, "w") as f:
            f.write("#define _GNU_SOURCE\n")
            f.write("#define _POSIX_C_SOURCE 199309L\n")
            f.write("#include <time.h>\n")
            f.write("#include <omp.h>\n")
            f.write("#include <stdio.h>\n\n")

            f.write(header_code)
            f.write("\n")
            f.write(main_opening)
            f.write("\n\n")

            f.write("    int i, j, k, t;\n")
            f.write(
                "    struct timespec "
                "start_h2d, end_h2d, "
                "start_exec, end_exec, "
                "start_d2h, end_d2h;\n"
            )
            f.write(
                "    double t_h2d = 0.0, "
                "t_exec = 0.0, "
                "t_d2h = 0.0;\n\n"
            )

            # ------------------------------------------------
            # Setup / prerequisite execution
            # ------------------------------------------------
            f.write(
                "    /* === SETUP / PREREQUISITE REGIONS === */\n"
            )

            has_enter_data_in_setup = False

            for k_idx in range(m_count):
                if k_idx < len(raw_main_segments):
                    seg_clean = sanitize_c_segment(
                        raw_main_segments[k_idx],
                        state
                    ).strip()

                    if seg_clean:
                        if "target enter data" in seg_clean.lower():
                            has_enter_data_in_setup = True

                        f.write(f"    {seg_clean}\n\n")

                f.write(
                    f"    /* Dependent Region "
                    f"{main_preceding[k_idx][0]} */\n"
                )
                f.write(
                    f"    {main_preceding[k_idx][1]}\n"
                )

                if "nowait" in main_preceding[k_idx][1].lower():
                    f.write("    #pragma omp taskwait\n\n")
                else:
                    f.write("\n")

            if m_count < len(raw_main_segments):
                seg_clean = sanitize_c_segment(
                    raw_main_segments[m_count],
                    state
                ).strip()

                if seg_clean:
                    if "target enter data" in seg_clean.lower():
                        has_enter_data_in_setup = True

                    f.write(f"    {seg_clean}\n\n")

            # ------------------------------------------------
            # Device allocation
            # ------------------------------------------------
            if (
                global_specs_str
                and not has_prior_dependencies
                and not has_enter_data_in_setup
            ):
                f.write(
                    "    /* Allocate arrays on device before "
                    "timed isolated transfers. */\n"
                )
                f.write(
                    f"    #pragma omp target enter data "
                    f"map(alloc:{global_specs_str})\n\n"
                )

            # ------------------------------------------------
            # H2D
            # ------------------------------------------------
            if pre_copyin_str:
                f.write(
                    "    /* === ISOLATED H2D === */\n"
                )
                f.write(
                    "    clock_gettime("
                    "CLOCK_MONOTONIC, &start_h2d);\n"
                )
                f.write(
                    f"    #pragma omp target update "
                    f"to({pre_copyin_str})\n"
                )
                f.write(
                    "    clock_gettime("
                    "CLOCK_MONOTONIC, &end_h2d);\n"
                )
                f.write(
                    "    t_h2d = "
                    "(end_h2d.tv_sec - start_h2d.tv_sec) + "
                    "(end_h2d.tv_nsec - start_h2d.tv_nsec) "
                    "/ 1e9;\n\n"
                )
            else:
                f.write(
                    "    /* No H2D required/detected for this region. */\n\n"
                )

            # ------------------------------------------------
            # Region execution
            # ------------------------------------------------
            isolated_target_block = remove_existing_target_data_pragmas(
                target_block
            )

            f.write(
                f"    /* === ISOLATED REGION {target_id} === */\n"
            )
            f.write(
                "    clock_gettime("
                "CLOCK_MONOTONIC, &start_exec);\n"
            )
            f.write(f"\n{isolated_target_block}\n\n")

            if "nowait" in target_pragma_lower:
                f.write("    #pragma omp taskwait\n")

            f.write(
                "    clock_gettime("
                "CLOCK_MONOTONIC, &end_exec);\n"
            )
            f.write(
                "    t_exec = "
                "(end_exec.tv_sec - start_exec.tv_sec) + "
                "(end_exec.tv_nsec - start_exec.tv_nsec) "
                "/ 1e9;\n\n"
            )

            # ------------------------------------------------
            # D2H
            # ------------------------------------------------
            if array_specs:
                specs_str = ", ".join(array_specs)

                f.write(
                    "    /* === ISOLATED D2H === */\n"
                )
                f.write(
                    "    clock_gettime("
                    "CLOCK_MONOTONIC, &start_d2h);\n"
                )
                f.write(
                    f"    #pragma omp target update "
                    f"from({specs_str})\n"
                )
                f.write(
                    "    clock_gettime("
                    "CLOCK_MONOTONIC, &end_d2h);\n"
                )
                f.write(
                    "    t_d2h = "
                    "(end_d2h.tv_sec - start_d2h.tv_sec) + "
                    "(end_d2h.tv_nsec - start_d2h.tv_nsec) "
                    "/ 1e9;\n\n"
                )
            else:
                f.write(
                    "    /* No D2H array section detected. */\n\n"
                )

            # ------------------------------------------------
            # Machine-readable result
            # ------------------------------------------------
            f.write(
                "    double t_transfer = t_h2d + t_d2h;\n"
            )
            f.write(
                "    double t_total = t_exec + t_transfer;\n"
            )

            f.write(
                f'    printf("[ISOLATED] region:{target_id} '
                f'H2D=%.9f KERNEL=%.9f D2H=%.9f TOTAL=%.9f\\n", '
                f't_h2d, t_exec, t_d2h, t_total);\n'
            )

            f.write("    return 0;\n")
            f.write("}\n")

        generated_files.append(
            (str(target_id), filename)
        )

    return generated_files


def compile_and_run_isolated_regions(
    generated_files,
    compiler="nvc",
    gpu_arch="cc70"
):
    """
    Compile/run every standalone region and return:
        {
            region_id: {
                "h2d": ...,
                "kernel": ...,
                "d2h": ...,
                "total": ...
            }
        }
    """
    results = {}

    print("\n" + "=" * 76)
    print(" ISOLATED REGION PROFILING")
    print("=" * 76)

    pattern = re.compile(
        r"\[ISOLATED\]\s+region:(\S+)\s+"
        r"H2D=([\d\.eE+\-]+)\s+"
        r"KERNEL=([\d\.eE+\-]+)\s+"
        r"D2H=([\d\.eE+\-]+)\s+"
        r"TOTAL=([\d\.eE+\-]+)"
    )

    for target_id, c_file in generated_files:
        exe_file = os.path.splitext(c_file)[0]

        compile_cmd = [
            compiler,
            "-mp=gpu",
            f"-gpu={gpu_arch}",
            "-Minfo=mp",
            "--diag_suppress",
            "declared_but_not_referenced",
            c_file,
            "-o",
            exe_file,
        ]

        print(f"\n[*] Compiling isolated Region {target_id}:")
        print("    " + " ".join(compile_cmd))

        comp_process = subprocess.run(
            compile_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        if comp_process.returncode != 0:
            print(
                f"[!] Isolated Region {target_id} compilation failed."
            )
            if comp_process.stderr.strip():
                print(comp_process.stderr.strip())
            continue

        print(f"[*] Running isolated Region {target_id}")

        run_process = subprocess.run(
            [os.path.abspath(exe_file)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        combined = (
            run_process.stdout + "\n" + run_process.stderr
        )

        match = pattern.search(combined)

        if run_process.returncode != 0:
            print(
                f"[!] Isolated Region {target_id} execution failed "
                f"(return code {run_process.returncode})."
            )
            if run_process.stderr.strip():
                print(run_process.stderr.strip())
            continue

        if not match:
            print(
                f"[!] No isolated timing result found "
                f"for Region {target_id}."
            )
            continue

        result_id = match.group(1)

        results[str(result_id)] = {
            "h2d": float(match.group(2)),
            "kernel": float(match.group(3)),
            "d2h": float(match.group(4)),
            "total": float(match.group(5)),
        }

        print(
            f"    H2D={results[str(result_id)]['h2d']:.9f} s, "
            f"Kernel={results[str(result_id)]['kernel']:.9f} s, "
            f"D2H={results[str(result_id)]['d2h']:.9f} s, "
            f"Isolated={results[str(result_id)]['total']:.9f} s"
        )

    return results


def attach_isolated_results(regions, isolated_results):
    """
    Merge standalone measurements into the region records.

    CAPC region numbers from parse_regions() are source-order IDs.
    Standalone IDs may come from explicit marker IDs; when possible,
    match numerically/source-order.
    """
    for reg in regions:
        key = str(reg["id"])

        result = isolated_results.get(key)

        if result is None:
            # No standalone result available.
            continue

        reg["isolated_h2d"] = result["h2d"]
        reg["isolated_kernel"] = result["kernel"]
        reg["isolated_d2h"] = result["d2h"]
        reg["isolated_time"] = result["total"]


# ============================================================
# FINAL COMBINED REPORT
# ============================================================

def fmt_time(value):
    if value is None:
        return "N/A"
    return f"{value:.6f}"


def print_results(regions):
    """
    Final combined table.

    Definitions:
      Resident:
          Original-program target compute time.

      Observed:
          Resident + explicit original-program transfer time
          attributed to the region.

      Isolated:
          Standalone H2D + standalone region execution + standalone D2H.

    For isolated timing:
      * Avg Iso = one standalone isolated execution.
      * Total Iso = Avg Iso * original invocation count.

    The latter is a derived comparable total, not an independently timed
    repeated standalone run.
    """
    headers = [
        ("Region", 8),
        ("Lines", 11),
        ("Calls", 8),
        ("Tot Res", 12),
        ("Avg Res", 12),
        ("Tot Obs", 12),
        ("Avg Obs", 12),
        ("Tot Iso*", 12),
        ("Avg Iso", 12),
    ]

    header = " | ".join(
        f"{name:<{width}}"
        for name, width in headers
    )

    divider = "-" * len(header)

    print("\n" + divider)
    print(
        "CAPC PROFITABILITY REGION REPORT "
        "(OPENMP 4.5: RESIDENT / OBSERVED / ISOLATED)"
    )
    print(divider)
    print(header)
    print(divider)

    total_resident = 0.0
    total_observed = 0.0
    total_isolated_derived = 0.0
    total_calls = 0

    for reg in regions:
        count = reg["count"]

        resident_total = reg["resident_time"]
        observed_total = (
            reg["resident_time"] + reg["transfer_time"]
        )

        if count > 0:
            resident_avg = resident_total / count
            observed_avg = observed_total / count
        else:
            resident_avg = None
            observed_avg = None

        isolated_avg = reg["isolated_time"]

        if isolated_avg is not None and count > 0:
            isolated_total = isolated_avg * count
        elif isolated_avg is not None:
            isolated_total = isolated_avg
        else:
            isolated_total = None

        total_resident += resident_total
        total_observed += observed_total
        total_calls += count

        if isolated_total is not None:
            total_isolated_derived += isolated_total

        line_range = (
            f"{reg['begin_line']}-{reg['end_line']}"
        )

        values = [
            (f"Region {reg['id']}", 8),
            (line_range, 11),
            (str(count), 8),
            (fmt_time(resident_total), 12),
            (fmt_time(resident_avg), 12),
            (fmt_time(observed_total), 12),
            (fmt_time(observed_avg), 12),
            (fmt_time(isolated_total), 12),
            (fmt_time(isolated_avg), 12),
        ]

        print(
            " | ".join(
                f"{value:<{width}}"
                for value, width in values
            )
        )

    print(divider)

    if total_calls > 0:
        avg_total_res = total_resident / total_calls
        avg_total_obs = total_observed / total_calls
        avg_total_iso = (
            total_isolated_derived / total_calls
            if total_isolated_derived > 0.0
            else None
        )
    else:
        avg_total_res = None
        avg_total_obs = None
        avg_total_iso = None

    total_values = [
        ("TOTAL", 8),
        ("-", 11),
        (str(total_calls), 8),
        (fmt_time(total_resident), 12),
        (fmt_time(avg_total_res), 12),
        (fmt_time(total_observed), 12),
        (fmt_time(avg_total_obs), 12),
        (fmt_time(total_isolated_derived), 12),
        (fmt_time(avg_total_iso), 12),
    ]

    print(
        " | ".join(
            f"{value:<{width}}"
            for value, width in total_values
        )
    )

    print(divider)

    print(
        "\nDefinitions:"
        "\n  Resident = target compute time in the original program."
        "\n  Observed = Resident + explicit original-program "
        "target enter/exit/update transfer time."
        "\n  Avg Iso  = measured standalone H2D + execution + D2H."
        "\n  Tot Iso* = Avg Iso x original number of target invocations."
        "\n             It is a derived comparison value, not a separately "
        "timed repeated standalone execution."
    )

    print("\nIsolated transfer breakdown:")

    iso_header = (
        f"{'Region':<10} | "
        f"{'H2D(s)':<12} | "
        f"{'Kernel(s)':<12} | "
        f"{'D2H(s)':<12} | "
        f"{'Isolated(s)':<12}"
    )
    iso_divider = "-" * len(iso_header)

    print(iso_divider)
    print(iso_header)
    print(iso_divider)

    for reg in regions:
        print(
            f"{'Region ' + str(reg['id']):<10} | "
            f"{fmt_time(reg['isolated_h2d']):<12} | "
            f"{fmt_time(reg['isolated_kernel']):<12} | "
            f"{fmt_time(reg['isolated_d2h']):<12} | "
            f"{fmt_time(reg['isolated_time']):<12}"
        )

    print(iso_divider + "\n")


# ============================================================
# MAIN
# ============================================================

def main():
    parser = argparse.ArgumentParser(
        description=(
            "Measure Resident, Observed and Isolated timing "
            "for CAPC OpenMP 4.5 profitability regions."
        )
    )

    parser.add_argument(
        "source",
        help="Path to OpenMP 4.5 C source file"
    )

    parser.add_argument(
        "--gpu",
        default="cc70",
        help="GPU architecture passed to nvc (default: cc70)"
    )

    parser.add_argument(
        "--compiler",
        default="nvc",
        help="Compiler executable (default: nvc)"
    )

    parser.add_argument(
        "--keep-standalone",
        action="store_true",
        help="Keep generated standalone region source/executables"
    )

    args = parser.parse_args()

    source_path = os.path.abspath(args.source)

    if not os.path.exists(source_path):
        print(f"Error: source file '{args.source}' not found.")
        sys.exit(1)

    regions = parse_regions(source_path)

    if not regions:
        print(
            "Error: no '#pragma capc profitability_region' "
            "blocks found."
        )
        sys.exit(1)

    work_dir = os.path.dirname(source_path)
    base_name = os.path.splitext(
        os.path.basename(source_path)
    )[0]

    instrumented_exec = os.path.join(
        work_dir,
        base_name + "_capc_profiled"
    )

    temp_fd, temp_source_path = tempfile.mkstemp(
        prefix=base_name + "_capc_instrumented_",
        suffix=".c",
        dir=work_dir,
    )
    os.close(temp_fd)

    standalone_dir = os.path.join(
        work_dir,
        base_name + "_standalone_regions"
    )

    try:
        # ====================================================
        # PHASE 1: Resident + Observed
        # ====================================================
        print("\n" + "=" * 76)
        print(" PHASE 1: RESIDENT + OBSERVED PROFILING")
        print("=" * 76)

        instrument_openmp_source(
            source_path,
            temp_source_path,
            regions
        )

        compile_openmp_program(
            temp_source_path,
            instrumented_exec,
            compiler=args.compiler,
            gpu_arch=args.gpu,
        )

        stdout_str, stderr_str, returncode = run_executable(
            instrumented_exec
        )

        process_profiler_output(
            stdout_str,
            stderr_str,
            returncode,
            regions
        )

        # ====================================================
        # PHASE 2: Isolated
        # ====================================================
        print("\n" + "=" * 76)
        print(" PHASE 2: ISOLATED PROFILING")
        print("=" * 76)

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
            output_dir=standalone_dir,
        )

        isolated_results = compile_and_run_isolated_regions(
            generated_files,
            compiler=args.compiler,
            gpu_arch=args.gpu,
        )

        attach_isolated_results(
            regions,
            isolated_results
        )

        # ====================================================
        # PHASE 3: Combined table
        # ====================================================
        print_results(regions)

    except RuntimeError as exc:
        print(f"\nError: {exc}")
        sys.exit(1)

    except Exception as exc:
        print(f"\nUnexpected error: {exc}")
        sys.exit(1)

    finally:
        if os.path.exists(temp_source_path):
            os.remove(temp_source_path)

        if os.path.exists(instrumented_exec):
            try:
                os.remove(instrumented_exec)
            except OSError:
                pass

        if (
            os.path.exists(standalone_dir)
            and not args.keep_standalone
        ):
            shutil.rmtree(standalone_dir, ignore_errors=True)


if __name__ == "__main__":
    main()