#!/usr/bin/env python3
import os
import sys
import re
import subprocess
import argparse
import resource

# Expand stack size to prevent segmentation faults on large array allocations
try:
    resource.setrlimit(resource.RLIMIT_STACK, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
except Exception:
    pass

PROFILER_SRC = "acc_profiler.c"
PROFILER_SO = "libaccprof.so"

def ensure_profiler_so(base_dir):
    """Compiles libaccprof.so using nvc to match NVHPC OpenACC runtime ABI."""
    so_path = os.path.abspath(os.path.join(base_dir, PROFILER_SO))
    src_path = os.path.abspath(os.path.join(base_dir, PROFILER_SRC))

    if not os.path.exists(src_path):
        print(f"[-] Error: '{src_path}' not found in '{base_dir}'.")
        sys.exit(1)

    print(f"[*] Compiling OpenACC profiler library: nvc -shared -fPIC -acc {src_path} -o {so_path}")
    compile_cmd = ["nvc", "-shared", "-fPIC", "-acc", src_path, "-o", so_path]
    res = subprocess.run(compile_cmd, capture_output=True, text=True)
    
    if res.returncode != 0:
        print(f"[!] nvc shared build failed, retrying with gcc...")
        compile_cmd = ["gcc", "-shared", "-fPIC", src_path, "-o", so_path]
        res = subprocess.run(compile_cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"[-] Failed to build profiler library:\n{res.stderr}")
            sys.exit(1)
            
    return so_path

def compile_openacc_program(source_file, exec_name, gpu_arch="cc70"):
    """Compiles the target program using nvc."""
    compile_cmd = [
        "nvc",
        "-acc",
        f"-gpu={gpu_arch}",
        "-Minfo=accel",
        source_file,
        "-o",
        exec_name
    ]
    print(f"[*] Compiling program: {' '.join(compile_cmd)}")
    res = subprocess.run(compile_cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"[-] Compilation failed for '{source_file}':\n{res.stderr}")
        sys.exit(1)

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

def find_target_region(line_no, regions):
    """Maps profiler event line numbers to the corresponding CAPC region."""
    for i, reg in enumerate(regions):
        if reg["begin_line"] <= line_no <= reg["end_line"]:
            return reg
        
        next_begin = regions[i + 1]["begin_line"] if i + 1 < len(regions) else float('inf')
        if reg["end_line"] < line_no < next_begin:
            return reg

    return None

def run_executable(exec_path, profiler_so_path):
    """Executes binary with ACC_PROFLIB environment variables."""
    env = os.environ.copy()
    env["ACC_PROFLIB"] = profiler_so_path
    env["NV_ACC_PROFLIB"] = profiler_so_path
    env["LD_LIBRARY_PATH"] = os.path.dirname(profiler_so_path) + ":" + env.get("LD_LIBRARY_PATH", "")
    
    print(f"[*] Executing target binary with ACC_PROFLIB={profiler_so_path}\n")
    res = subprocess.run([exec_path], env=env, capture_output=True, text=True)
    return res.stdout, res.stderr, res.returncode

def process_profiler_output(stdout_str, stderr_str, returncode, regions):
    """Parses kernel and transfer logs into Resident, Transfer, and Invocation metrics."""
    combined_log = stdout_str + "\n" + stderr_str
    pattern = re.compile(r"\[PROFILER\].*?:(\d+)\s+\|\s+(.*?)\s+=\s+([\d\.]+)\s+s")

    matched_events = 0
    for line in combined_log.splitlines():
        match = pattern.search(line)
        if match:
            matched_events += 1
            line_no = int(match.group(1))
            event_type = match.group(2)
            duration = float(match.group(3))

            region = find_target_region(line_no, regions)
            if region:
                if "Kernel Execution Time" in event_type:
                    region["resident_time"] += duration
                    region["count"] += 1  # Increment invocation count on kernel launch
                elif "Transfer" in event_type:
                    region["transfer_time"] += duration

    if matched_events == 0:
        print("[!] Warning: No [PROFILER] output logs were detected.")
        print(f"[!] Executable Return Code: {returncode}")

def print_results(regions):
    """Displays formatted results including Invocation Counts and Averages."""
    header = (
        f"{'Region':<8} | {'Lines':<8} | {'Invocations':<11} | "
        f"{'Total Res(s)':<12} | {'Avg Res(s)':<12} | "
        f"{'Total Obs(s)':<12} | {'Avg Obs(s)':<12}"
    )
    divider = "-" * len(header)

    print(divider)
    print("                           CAPC PROFITABILITY REGION REPORT")
    print(divider)
    print(header)
    print(divider)

    total_resident = 0.0
    total_observed = 0.0
    total_invocations = 0

    for reg in regions:
        count = max(reg["count"], 1)  # Guard against division by zero
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
    parser = argparse.ArgumentParser(description="Compile, Run, and Profile OpenACC Regions in one command.")
    parser.add_argument("source", help="Path to OpenACC source C file (e.g., jacobi-1D_acc.c)")
    parser.add_argument("--gpu", default="cc70", help="GPU compute capability architecture (default: cc70)")

    args = parser.parse_args()

    source_path = os.path.abspath(args.source)
    if not os.path.exists(source_path):
        print(f"Error: Source file '{args.source}' not found.")
        sys.exit(1)

    work_dir = os.path.dirname(source_path)
    exec_name = os.path.splitext(os.path.basename(source_path))[0]
    exec_path = os.path.join(work_dir, exec_name)

    # Step 1: Ensure profiler .so exists
    profiler_so_path = ensure_profiler_so(work_dir)

    # Step 2: Parse source code regions
    regions = parse_regions(source_path)
    if not regions:
        print("Error: No '#pragma capc profitability_region' blocks found in source file.")
        sys.exit(1)

    # Step 3: Compile source code with nvc
    compile_openacc_program(source_path, exec_path, gpu_arch=args.gpu)

    # Step 4: Run executable & process metrics
    stdout_str, stderr_str, returncode = run_executable(exec_path, profiler_so_path)
    process_profiler_output(stdout_str, stderr_str, returncode, regions)

    # Step 5: Output report
    print_results(regions)

if __name__ == "__main__":
    main()