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

PROFILER_SRC_NAME = "omp_profiler.c"
PROFILER_SO_NAME = "libompprof.so"

# Fully functional self-contained OMPT profiler source
DEFAULT_OMP_PROFILER_C = r"""#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

typedef uint64_t ompt_id_t;

typedef struct ompt_data_s {
    uint64_t value;
    void *ptr;
} ompt_data_t;

typedef enum ompt_target_e {
    ompt_target = 1,
    ompt_target_enter_data = 2,
    ompt_target_exit_data = 3,
    ompt_target_update = 4
} ompt_target_t;

typedef enum ompt_scope_endpoint_e {
    ompt_scope_begin = 1,
    ompt_scope_end = 2,
    ompt_scope_beginend = 3
} ompt_scope_endpoint_t;

typedef enum ompt_target_data_op_e {
    ompt_target_data_alloc = 1,
    ompt_target_data_transfer_to_device = 2,
    ompt_target_data_transfer_from_device = 3,
    ompt_target_data_delete = 4
} ompt_target_data_op_t;

typedef enum ompt_callbacks_e {
    ompt_callback_target = 50,
    ompt_callback_target_data_op = 51,
    ompt_callback_target_submit = 52
} ompt_callbacks_t;

typedef void (*ompt_callback_t)(void);
typedef int (*ompt_set_callback_t)(ompt_callbacks_t event, ompt_callback_t callback);
typedef void *(*ompt_function_lookup_t)(const char *entrypoint);

typedef struct ompt_start_tool_result_s {
    int (*initialize)(ompt_function_lookup_t lookup, int initial_device_num, ompt_data_t *tool_data);
    void (*finalize)(ompt_data_t *tool_data);
    ompt_data_t tool_data;
} ompt_start_tool_result_t;

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

#define MAX_TARGETS 1024
typedef struct {
    ompt_id_t target_id;
    double start_time;
    int line_no;
    bool active;
} target_record_t;

static target_record_t target_records[MAX_TARGETS];

static int get_line_from_address(const void *codeptr_ra) {
    if (!codeptr_ra) return 0;

    Dl_info info;
    if (dladdr(codeptr_ra, &info) && info.dli_fname) {
        char cmd[512];
        uintptr_t addr = (uintptr_t)codeptr_ra;
        snprintf(cmd, sizeof(cmd), "addr2line -e %s %p 2>/dev/null", info.dli_fname, (void*)addr);
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char buf[256];
            if (fgets(buf, sizeof(buf), fp)) {
                char *colon = strrchr(buf, ':');
                if (colon) {
                    int line = atoi(colon + 1);
                    pclose(fp);
                    if (line > 0) return line;
                }
            }
            pclose(fp);
        }
    }
    return 0;
}

static void on_ompt_callback_target(
    ompt_target_t kind,
    ompt_scope_endpoint_t endpoint,
    int device_num,
    ompt_data_t *task_data,
    ompt_id_t target_id,
    const void *codeptr_ra
) {
    double now = get_time_sec();
    if (endpoint == ompt_scope_begin) {
        int line = get_line_from_address(codeptr_ra);
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (!target_records[i].active) {
                target_records[i].target_id = target_id;
                target_records[i].start_time = now;
                target_records[i].line_no = line;
                target_records[i].active = true;
                break;
            }
        }
    } else if (endpoint == ompt_scope_end) {
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (target_records[i].active && target_records[i].target_id == target_id) {
                double duration = now - target_records[i].start_time;
                int line = target_records[i].line_no;
                target_records[i].active = false;
                fprintf(stderr, "[PROFILER] line:%d | Target Execution Time = %.6f s\n", line, duration);
                fflush(stderr);
                break;
            }
        }
    }
}

static void on_ompt_callback_target_data_op(
    ompt_id_t target_id,
    ompt_id_t host_op_id,
    ompt_target_data_op_t optype,
    void *src_addr,
    int src_device_num,
    void *dest_addr,
    int dest_device_num,
    size_t bytes,
    const void *codeptr_ra
) {
    /* Optional: Data Transfer Logging */
}

int ompt_initialize(ompt_function_lookup_t lookup, int initial_device_num, ompt_data_t *tool_data) {
    ompt_set_callback_t ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");

    if (ompt_set_callback) {
        ompt_set_callback(ompt_callback_target, (ompt_callback_t)on_ompt_callback_target);
        ompt_set_callback(ompt_callback_target_data_op, (ompt_callback_t)on_ompt_callback_target_data_op);
    }
    return 1;
}

void ompt_finalize(ompt_data_t *tool_data) {
}

#ifdef __cplusplus
extern "C" {
#endif
ompt_start_tool_result_t *ompt_start_tool(unsigned int omp_version, const char *runtime_version) {
    static ompt_start_tool_result_t result = { &ompt_initialize, &ompt_finalize, {0} };
    return &result;
}
#ifdef __cplusplus
}
#endif
"""

def locate_or_create_profiler_src(work_dir, script_dir):
    """Finds existing omp_profiler.c or overwrites if missing logging logic."""
    target_path = os.path.abspath(os.path.join(work_dir, PROFILER_SRC_NAME))

    needs_overwrite = False
    if os.path.exists(target_path):
        with open(target_path, 'r') as f:
            content = f.read()
            if "[PROFILER]" not in content:
                needs_overwrite = True

    if not os.path.exists(target_path) or needs_overwrite:
        print(f"[*] Auto-generating functional profiler source at: {target_path}")
        with open(target_path, 'w') as f:
            f.write(DEFAULT_OMP_PROFILER_C)

    return target_path

def ensure_profiler_so(work_dir, script_dir):
    """Compiles libompprof.so using nvc or gcc."""
    src_path = locate_or_create_profiler_src(work_dir, script_dir)
    so_path = os.path.abspath(os.path.join(os.path.dirname(src_path), PROFILER_SO_NAME))

    print(f"[*] Compiling OpenMP profiler library: nvc -shared -fPIC -mp=gpu {src_path} -o {so_path}")
    compile_cmd = ["nvc", "-shared", "-fPIC", "-mp=gpu", src_path, "-o", so_path]
    res = subprocess.run(compile_cmd, capture_output=True, text=True)
    
    if res.returncode != 0:
        print(f"[!] nvc shared build failed, retrying with gcc...")
        compile_cmd = ["gcc", "-shared", "-fPIC", src_path, "-o", so_path]
        res = subprocess.run(compile_cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"[-] Failed to build profiler library:\n{res.stderr}")
            sys.exit(1)
            
    return so_path

def compile_openmp_program(source_file, exec_name, gpu_arch="cc70"):
    """Compiles the OpenMP 4.5 target program using nvc with debug flags."""
    compile_cmd = [
        "nvc",
        "-mp=gpu",
        "-g",
        f"-gpu={gpu_arch}",
        "-Minfo=mp",
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

def find_target_region(line_no, regions, event_index=0):
    """Maps profiler event line numbers to CAPC regions with sequential fallback."""
    if line_no > 0:
        for i, reg in enumerate(regions):
            if reg["begin_line"] <= line_no <= reg["end_line"]:
                return reg
            
            next_begin = regions[i + 1]["begin_line"] if i + 1 < len(regions) else float('inf')
            if reg["end_line"] < line_no < next_begin:
                return reg

    # Fallback to sequential region matching if line number resolution is 0
    if regions:
        return regions[event_index % len(regions)]

    return None

def run_executable(exec_path, profiler_so_path):
    """Executes binary with OMPT_TOOL_LIBRARIES environment variables."""
    env = os.environ.copy()
    env["OMPT_TOOL_LIBRARIES"] = profiler_so_path
    env["NV_OMP_PROFLIB"] = profiler_so_path
    env["LD_LIBRARY_PATH"] = os.path.dirname(profiler_so_path) + ":" + env.get("LD_LIBRARY_PATH", "")
    
    print(f"[*] Executing target binary with OMPT_TOOL_LIBRARIES={profiler_so_path}\n")
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
            line_no = int(match.group(1))
            event_type = match.group(2)
            duration = float(match.group(3))

            region = find_target_region(line_no, regions, matched_events)
            if region:
                matched_events += 1
                if "Kernel Execution Time" in event_type or "Target Execution Time" in event_type:
                    region["resident_time"] += duration
                    region["count"] += 1
                elif "Transfer" in event_type or "Data" in event_type:
                    region["transfer_time"] += duration

    if matched_events == 0:
        print("[!] Warning: No [PROFILER] output logs were detected in stdout/stderr.")
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
    print("                            CAPC PROFITABILITY REGION REPORT (OpenMP 4.5)")
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
    parser = argparse.ArgumentParser(description="Compile, Run, and Profile OpenMP 4.5 Target Regions in one command.")
    parser.add_argument("source", help="Path to OpenMP 4.5 target source C file (e.g., 3mm_omp45.c)")
    parser.add_argument("--gpu", default="cc70", help="GPU compute capability architecture (default: cc70)")

    args = parser.parse_args()

    source_path = os.path.abspath(args.source)
    if not os.path.exists(source_path):
        print(f"Error: Source file '{args.source}' not found.")
        sys.exit(1)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    work_dir = os.path.dirname(source_path)
    exec_name = os.path.splitext(os.path.basename(source_path))[0]
    exec_path = os.path.join(work_dir, exec_name)

    # Step 1: Ensure profiler .so exists
    profiler_so_path = ensure_profiler_so(work_dir, script_dir)

    # Step 2: Parse source code regions
    regions = parse_regions(source_path)
    if not regions:
        print("Error: No '#pragma capc profitability_region' blocks found in source file.")
        sys.exit(1)

    # Step 3: Compile source code with nvc (-mp=gpu)
    compile_openmp_program(source_path, exec_path, gpu_arch=args.gpu)

    # Step 4: Run executable & process metrics
    stdout_str, stderr_str, returncode = run_executable(exec_path, profiler_so_path)
    process_profiler_output(stdout_str, stderr_str, returncode, regions)

    # Step 5: Output report
    print_results(regions)

if __name__ == "__main__":
    main()