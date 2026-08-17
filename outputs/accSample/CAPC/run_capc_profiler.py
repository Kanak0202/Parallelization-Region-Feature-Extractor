#!/usr/bin/env python3
import os
import sys
import re
import subprocess
import argparse
import tempfile
import resource

# Expand stack size to prevent limits on large matrix allocations
try:
    resource.setrlimit(resource.RLIMIT_STACK, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
except Exception:
    pass

def parse_regions(source_file):
    """Parses #pragma capc profitability_region begin / end line ranges."""
    regions = []
    current_region = None
    region_id = 1

    with open(source_file, 'r') as f:
        for line_num, line in enumerate(f, start=1):
            line_str = line.strip()
            if "#pragma capc profitability_region begin" in line_str:
                current_region = {
                    "id": region_id,
                    "begin_line": line_num,
                    "end_line": None,
                    "count": 0,
                    "resident_time": 0.0,
                    "transfer_time": 0.0
                }
            elif "#pragma capc profitability_region end" in line_str and current_region:
                current_region["end_line"] = line_num
                regions.append(current_region)
                region_id += 1
                current_region = None

    return regions

def get_associated_region_id(line_num, regions):
    """Maps any statement line (including data pragmas outside regions) to its parent region."""
    if not regions:
        return 1
    
    # 1. Statement inside a region
    for reg in regions:
        if reg["begin_line"] <= line_num <= reg["end_line"]:
            return reg["id"]
            
    # 2. Statement before first region -> attribute to Region 1
    if line_num < regions[0]["begin_line"]:
        return regions[0]["id"]
        
    # 3. Statement between regions -> attribute to preceding region
    for i in range(len(regions) - 1):
        if regions[i]["end_line"] < line_num < regions[i+1]["begin_line"]:
            return regions[i]["id"]
            
    # 4. Statement after last region -> attribute to final region
    return regions[-1]["id"]

def consume_statement(lines, idx):
    """
    Recursively scans and consumes the complete C statement/block following an OpenACC directive.
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
                l = lines[idx]
                brace_depth += l.count("{") - l.count("}")
                idx += 1
                if brace_depth <= 0:
                    break
            return idx

        elif any(line_str.startswith(kw) for kw in ["for", "while", "if", "do"]):
            idx += 1
            idx = consume_statement(lines, idx)
            return idx

        else:
            idx += 1
            while idx < n and ";" not in line_str:
                line_str = lines[idx].strip()
                idx += 1
            return idx

    return idx

def instrument_openacc_source(source_path, temp_path, regions):
    """
    Instruments C source by inserting timers around OpenACC compute kernels and data transfers.
    """
    with open(source_path, 'r') as f:
        lines = f.readlines()

    instrumented = [
        "#include <omp.h>\n#include <stdio.h>\n#include <openacc.h>\n",
        "static double _capc_dt0, _capc_dt1;\n",
        "static double _capc_k0, _capc_k1;\n\n"
    ]
    
    i = 0
    n = len(lines)

    while i < n:
        line = lines[i]
        line_str = line.strip()
        line_num = i + 1

        # Case A: Detect OpenACC Data Movement Directives (enter data, exit data, update)
        if "#pragma acc" in line_str and any(kw in line_str for kw in ["enter data", "exit data", "update"]):
            reg_id = get_associated_region_id(line_num, regions)
            instrumented.append("  _capc_dt0 = omp_get_wtime();\n")
            instrumented.append(line)
            instrumented.append("  #pragma acc wait\n")  # Ensure data transfer synchronizes
            instrumented.append("  _capc_dt1 = omp_get_wtime();\n")
            instrumented.append(
                f'  printf("[PROFILER] transfer region:{reg_id} line:{line_num} | Transfer Time = %.9f s\\n", '
                f'_capc_dt1 - _capc_dt0);\n'
            )
            i += 1
            continue

        # Case B: Detect OpenACC GPU Compute Kernels (parallel, kernels, serial)
        if "#pragma acc" in line_str and any(kw in line_str for kw in ["parallel", "kernels", "serial"]):
            reg_id = get_associated_region_id(line_num, regions)
            instrumented.append("  _capc_k0 = omp_get_wtime();\n")
            instrumented.append(line)
            i += 1
            
            end_idx = consume_statement(lines, i)
            while i < end_idx:
                instrumented.append(lines[i])
                i += 1

            instrumented.append("  #pragma acc wait\n")  # Ensure GPU kernel completion before timing
            instrumented.append("  _capc_k1 = omp_get_wtime();\n")
            instrumented.append(
                f'  printf("[PROFILER] kernel region:{reg_id} line:{line_num} | Kernel Execution Time = %.9f s\\n", '
                f'_capc_k1 - _capc_k0);\n'
            )
            continue

        instrumented.append(line)
        i += 1

    with open(temp_path, 'w') as f:
        f.writelines(instrumented)

def compile_openacc_program(source_file, exec_name, gpu_arch="cc70"):
    """Compiles instrumented OpenACC C program using nvc."""
    compile_cmd = [
        "nvc",
        "-acc",
        "-mp",  # Enabled for omp_get_wtime()
        f"-gpu={gpu_arch}",
        "-Minfo=accel",
        source_file,
        "-o",
        exec_name
    ]
    print(f"[*] Compiling OpenACC program: {' '.join(compile_cmd)}")
    res = subprocess.run(compile_cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"[-] Compilation failed for '{source_file}':\n{res.stderr}")
        sys.exit(1)

def run_executable(exec_path):
    """Executes target binary and captures standard output."""
    print(f"[*] Executing target OpenACC binary: {exec_path}\n")
    res = subprocess.run([exec_path], capture_output=True, text=True)
    return res.stdout, res.stderr, res.returncode

def process_profiler_output(stdout_str, stderr_str, returncode, regions):
    """Parses kernel and data transfer logs to aggregate Resident and Observed region times."""
    combined_log = stdout_str + "\n" + stderr_str
    pattern = re.compile(r"\[PROFILER\]\s+(kernel|transfer)\s+region:(\d+)\s+line:(\d+)\s+\|\s+(.*?)\s+=\s+([\d\.]+)\s+s")

    matched_events = 0
    region_map = {reg["id"]: reg for reg in regions}

    for line in combined_log.splitlines():
        match = pattern.search(line)
        if match:
            matched_events += 1
            event_cat = match.group(1)
            reg_id = int(match.group(2))
            duration = float(match.group(5))

            if reg_id in region_map:
                reg = region_map[reg_id]
                if event_cat == "kernel":
                    reg["resident_time"] += duration
                    reg["count"] += 1
                elif event_cat == "transfer":
                    reg["transfer_time"] += duration

    if matched_events == 0:
        print("[!] Warning: No [PROFILER] output logs were detected.")
        print(f"[!] Executable Return Code: {returncode}")

def print_results(regions):
    """Renders the CAPC Profitability Region Report table."""
    header = (
        f"{'Region':<8} | {'Lines':<8} | {'Invocations':<11} | "
        f"{'Total Res(s)':<12} | {'Avg Res(s)':<12} | "
        f"{'Total Obs(s)':<12} | {'Avg Obs(s)':<12}"
    )
    divider = "-" * len(header)

    print(divider)
    print("                    CAPC PROFITABILITY REGION REPORT (OPENACC)")
    print(divider)
    print(header)
    print(divider)

    total_resident = 0.0
    total_observed = 0.0
    total_invocations = 0

    for reg in regions:
        count = max(reg["count"], 1)
        observed_time = reg["resident_time"] + reg["transfer_time"]

        avg_resident = reg["resident_time"] / count
        avg_observed = observed_time / count

        total_resident += reg["resident_time"]
        total_observed += observed_time
        total_invocations += reg["count"]

        line_range = f"{reg['begin_line']}-{reg['end_line']}"
        print(
            f"Region {reg['id']:<1} | {line_range:<8} | {reg['count']:<11} | "
            f"{reg['resident_time']:<12.6f} | {avg_resident:<12.6f} | "
            f"{observed_time:<12.6f} | {avg_observed:<12.6f}"
        )

    print(divider)
    avg_tot_res = total_resident / max(total_invocations, 1)
    avg_tot_obs = total_observed / max(total_invocations, 1)
    print(
        f"{'TOTAL':<8} | {'-':<8} | {total_invocations:<11} | "
        f"{total_resident:<12.6f} | {avg_tot_res:<12.6f} | "
        f"{total_observed:<12.6f} | {avg_tot_obs:<12.6f}"
    )
    print(divider + "\n")

def main():
    parser = argparse.ArgumentParser(description="Compile, Run, and Profile OpenACC Regions with Data Transfer Tracking.")
    parser.add_argument("source", help="Path to OpenACC source C file")
    parser.add_argument("--gpu", default="cc70", help="GPU architecture (default: cc70)")

    args = parser.parse_args()

    source_path = os.path.abspath(args.source)
    if not os.path.exists(source_path):
        print(f"Error: Source file '{args.source}' not found.")
        sys.exit(1)

    work_dir = os.path.dirname(source_path)
    exec_name = os.path.splitext(os.path.basename(source_path))[0]
    exec_path = os.path.join(work_dir, exec_name)

    regions = parse_regions(source_path)
    if not regions:
        print("Error: No '#pragma capc profitability_region' blocks found in source file.")
        sys.exit(1)

    temp_fd, temp_source_path = tempfile.mkstemp(suffix=".c", dir=work_dir)
    os.close(temp_fd)

    try:
        instrument_openacc_source(source_path, temp_source_path, regions)
        compile_openacc_program(temp_source_path, exec_path, gpu_arch=args.gpu)
        stdout_str, stderr_str, returncode = run_executable(exec_path)
        process_profiler_output(stdout_str, stderr_str, returncode, regions)
        print_results(regions)

    finally:
        if os.path.exists(temp_source_path):
            os.remove(temp_source_path)

if __name__ == "__main__":
    main()