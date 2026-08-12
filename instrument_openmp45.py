#!/usr/bin/env python3
"""
instrument_openmp.py
Source-to-source instrumenter for OpenMP 4.5 "target" offload codes.

Wraps every

    #pragma capc profitability_region begin
        #pragma omp target ...
        ... loop ...
    #pragma capc profitability_region end

block with omp_get_wtime() timers, accumulates a running total/count per
region (so regions inside an outer loop get averaged automatically), and
adds a printf report at the end of main() that prints, per region:

  resident_avg  -> average kernel-only time (data already on the device,
                   i.e. what you actually measure around the pragma)
  isolated_avg  -> resident_avg + (H2D + D2H) time for the arrays that the
                   region's map() clause touches.

H2D / D2H times are measured for real, ONCE per array, via a calibration
transfer inserted right after the first "target enter data" that put the
arrays on the device (values are copied to themselves, so correctness is
unaffected). Those per-array H2D/D2H times are printed both at the point
they are calibrated (i.e. once, near program start) AND again broken out
per region in the final report. All times are printed in plain decimal
seconds (e.g. 0.012345), not scientific notation.

USAGE (directory mode):
    python3 instrument_openmp.py <input_dir>

Example:
    python3 instrument_openmp.py ./outputs/omp45/CAPC

Every *.c file found (recursively) under <input_dir> is instrumented and
written to the mirrored location under .../annotated/... -- i.e. the first
"outputs" path component becomes "outputs/annotated", so:

    ./outputs/omp45/CAPC/foo.c  ->  ./outputs/annotated/omp45/CAPC/foo.c

If the input path does not contain an "outputs" component, output is
written under ./outputs/annotated/<input_dir_name>/ instead.

ASSUMPTIONS / LIMITATIONS (read before trusting numbers):
  * Each profitability_region is expected to contain exactly one
    "#pragma omp target ..." line, written without backslash line
    continuation.
  * map(alloc: ...) is conservatively treated as needing BOTH an H2D
    and a D2H transfer for the isolated estimate (worst case: the
    region's inputs/outputs would need transferring in AND out if the
    data were not resident). map(to:...) -> H2D only. map(from:...)
    -> D2H only. map(tofrom:...) -> both.
  * If a program uses more than one "target enter data" for different
    arrays, calibration is inserted after the FIRST one found; arrays
    entered later will have no calibration data and isolated_avg will
    silently equal resident_avg for them. Check the per-file summary
    printed while running.
"""
import re
import sys
from pathlib import Path

REGION_BEGIN = re.compile(r'#pragma\s+capc\s+profitability_region\s+begin')
REGION_END = re.compile(r'#pragma\s+capc\s+profitability_region\s+end')
OMP_TARGET = re.compile(r'#pragma\s+omp\s+target\b[^\n]*')
ENTER_DATA = re.compile(r'#pragma\s+omp\s+target\s+enter\s+data[^\n]*')
MAP_CLAUSE = re.compile(r'map\(\s*(alloc|to|from|tofrom)\s*:\s*([^)]*)\)')
ARR_REF = re.compile(r'(\w+)\s*\[\s*([^:\]]+)\s*:\s*([^\]]+)\s*\]')

TIME_FMT = "%.6f"  # decimal seconds, not scientific notation


def parse_map_clause(pragma_line):
    """Return dict: array_name -> (direction, length_expr)."""
    result = {}
    for direction, body in MAP_CLAUSE.findall(pragma_line):
        for name, lo, length in ARR_REF.findall(body):
            result[name] = (direction, length.strip())
    return result


def instrument_source(src, filename="<file>"):
    """Instrument a single C source string. Returns (out_src, regions)."""
    lines = src.split('\n')

    # ---- 1. locate regions -------------------------------------------
    regions = []
    stack = []
    for i, ln in enumerate(lines):
        if REGION_BEGIN.search(ln):
            stack.append(i)
        elif REGION_END.search(ln):
            if not stack:
                continue
            b = stack.pop()
            pragma_idx = None
            for j in range(b + 1, i):
                if OMP_TARGET.search(lines[j]):
                    pragma_idx = j
                    break
            if pragma_idx is None:
                print(f"  WARNING [{filename}]: region at line {b+1} has "
                      f"no '#pragma omp target' -- skipped.")
                continue
            arrays = parse_map_clause(lines[pragma_idx])
            regions.append({"begin": b, "end": i, "pragma": pragma_idx,
                             "arrays": arrays})

    if not regions:
        return None, []

    all_arrays = {}
    for r in regions:
        for name, (direction, length) in r["arrays"].items():
            info = all_arrays.setdefault(name, {"direction": set(),
                                                  "length": length})
            info["direction"].add(direction)

    # ---- 2. globals / declarations block -------------------------------
    decl_lines = ["", "/* ---- capc timing instrumentation: globals ---- */",
                  "#include <omp.h>"]
    for idx in range(len(regions)):
        decl_lines.append(f"static double __capc_region_{idx}_total = 0.0;")
        decl_lines.append(f"static long   __capc_region_{idx}_count = 0;")
    for name in all_arrays:
        decl_lines.append(f"static double __capc_h2d_{name} = -1.0;")
        decl_lines.append(f"static double __capc_d2h_{name} = -1.0;")
    decl_lines.append("/* ---- end globals ---- */\n")

    # ---- 3. calibration block (after first target enter data) ----------
    calib_lines = ["", "/* ---- capc timing instrumentation: one-shot "
                        "transfer calibration ---- */"]
    for name, info in all_arrays.items():
        length = info["length"]
        dirs = info["direction"]
        if dirs & {"to", "alloc", "tofrom"}:
            calib_lines.append("{ double __t0 = omp_get_wtime();")
            calib_lines.append(
                f"  #pragma omp target update to({name}[0:{length}])")
            calib_lines.append(
                f"  __capc_h2d_{name} = omp_get_wtime() - __t0;")
            calib_lines.append(
                f'  printf("[capc] H2D transfer time for \'{name}\': '
                f'{TIME_FMT} s\\n", __capc_h2d_{name}); }}')
        if dirs & {"from", "alloc", "tofrom"}:
            calib_lines.append("{ double __t0 = omp_get_wtime();")
            calib_lines.append(
                f"  #pragma omp target update from({name}[0:{length}])")
            calib_lines.append(
                f"  __capc_d2h_{name} = omp_get_wtime() - __t0;")
            calib_lines.append(
                f'  printf("[capc] D2H transfer time for \'{name}\': '
                f'{TIME_FMT} s\\n", __capc_d2h_{name}); }}')
    calib_lines.append("/* ---- end calibration ---- */\n")

    # ---- 4. report block (before final 'return 0;' in main) ------------
    report_lines = ["", "/* ---- capc timing instrumentation: report ---- */"]
    for idx, r in enumerate(regions):
        report_lines.append("{")
        report_lines.append(
            f"  double __resident = (__capc_region_{idx}_count > 0) ? "
            f"(__capc_region_{idx}_total / __capc_region_{idx}_count) : 0.0;")
        report_lines.append("  double __xfer = 0.0;")
        for name in r["arrays"]:
            report_lines.append(
                f"  if (__capc_h2d_{name} > 0) __xfer += __capc_h2d_{name};")
            report_lines.append(
                f"  if (__capc_d2h_{name} > 0) __xfer += __capc_d2h_{name};")
        report_lines.append(
            f'  printf("region_{idx} (pragma at original line '
            f'{r["pragma"]+1}): resident_avg={TIME_FMT} s '
            f'isolated_avg={TIME_FMT} s calls=%ld\\n", '
            f'__resident, __resident + __xfer, __capc_region_{idx}_count);')
        for name in r["arrays"]:
            report_lines.append(
                f'  printf("    {name}: h2d={TIME_FMT} s d2h={TIME_FMT} '
                f's\\n", __capc_h2d_{name} > 0 ? __capc_h2d_{name} : 0.0, '
                f'__capc_d2h_{name} > 0 ? __capc_d2h_{name} : 0.0);')
        report_lines.append("}")
    report_lines.append("/* ---- end report ---- */\n")

    # ---- 5. wrap each region with timers, bottom-up ---------------------
    for r in sorted(regions, key=lambda r: r["begin"], reverse=True):
        idx = regions.index(r)
        b, e = r["begin"], r["end"]
        start_timer = "{ double __capc_t0 = omp_get_wtime();"
        lines.insert(b + 1, start_timer)
        e_shifted = e + 1
        end_timer = (f"double __capc_t1 = omp_get_wtime(); "
                     f"__capc_region_{idx}_total += (__capc_t1 - __capc_t0); "
                     f"__capc_region_{idx}_count += 1; }}")
        lines.insert(e_shifted, end_timer)

    # ---- 6. re-locate anchor points on the now-shifted file --------------
    enter_idx = None
    for i, ln in enumerate(lines):
        if ENTER_DATA.search(ln):
            enter_idx = i
            break
    return_idxs = [i for i, l in enumerate(lines)
                   if re.search(r'\breturn\s+0\s*;', l)]
    report_at = return_idxs[-1] if return_idxs else len(lines) - 1
    include_idxs = [i for i, l in enumerate(lines)
                     if l.strip().startswith("#include")]
    insert_after = include_idxs[-1] if include_idxs else 0

    inserts = [(report_at, report_lines)]
    if enter_idx is not None:
        inserts.append((enter_idx + 1, calib_lines))
    else:
        print(f"  WARNING [{filename}]: no 'target enter data' pragma "
              f"found; calibration block NOT inserted -- isolated_avg "
              f"will equal resident_avg.")
    inserts.append((insert_after + 1, decl_lines))

    inserts.sort(key=lambda t: t[0], reverse=True)
    for at, block in inserts:
        lines[at:at] = block

    return "\n".join(lines), regions


def compute_output_root(input_dir):
    """
    ./outputs/omp45/CAPC  ->  ./outputs/annotated/omp45/CAPC
    (first 'outputs' path component gets '/annotated' inserted after it;
    if there is no 'outputs' component, fall back to
    ./outputs/annotated/<input_dir_name>)
    """
    p = Path(input_dir)
    parts = list(p.parts)
    if "outputs" in parts:
        idx = parts.index("outputs")
        new_parts = parts[:idx + 1] + ["annotated"] + parts[idx + 1:]
        return Path(*new_parts)
    return Path("outputs") / "annotated" / p.name


def main():
    if len(sys.argv) != 2:
        print("usage: python3 instrument_openmp.py <input_dir>")
        print("example: python3 instrument_openmp.py ./outputs/omp45/CAPC")
        sys.exit(1)

    input_dir = Path(sys.argv[1])
    if not input_dir.is_dir():
        print(f"error: '{input_dir}' is not a directory")
        sys.exit(1)

    output_root = compute_output_root(input_dir)
    print(f"Input dir : {input_dir}")
    print(f"Output dir: {output_root}\n")

    c_files = sorted(input_dir.rglob("*.c"))
    if not c_files:
        print(f"No .c files found under {input_dir}")
        sys.exit(1)

    total_instrumented = 0
    for c_file in c_files:
        rel = c_file.relative_to(input_dir)
        out_file = output_root / rel
        out_file.parent.mkdir(parents=True, exist_ok=True)

        src = c_file.read_text()
        out_src, regions = instrument_source(src, filename=str(rel))

        if out_src is None:
            print(f"SKIP  {rel}: no 'capc profitability_region' blocks found")
            continue

        out_file.write_text(out_src)
        total_instrumented += 1
        print(f"DONE  {rel}  ->  {out_file}")
        for idx, r in enumerate(regions):
            print(f"        region_{idx}: pragma at original line "
                  f"{r['pragma']+1}, arrays: {list(r['arrays'].keys())}")

    print(f"\nInstrumented {total_instrumented}/{len(c_files)} file(s).")
    print("Compile each output with your normal offload toolchain, e.g.:")
    print("  clang -fopenmp -fopenmp-targets=<target> <file>.c -o a.out")
    print("  (or nvc -mp=gpu / gcc -fopenmp)")


if __name__ == "__main__":
    main()