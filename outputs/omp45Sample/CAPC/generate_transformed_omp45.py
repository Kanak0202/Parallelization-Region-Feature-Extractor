#!/usr/bin/env python3
import os
import sys
import argparse

def consume_statement(lines, idx):
    """
    Scans and consumes the full C statement/block following an OpenMP directive,
    handling nested non-braced loops (e.g., collapse nests) and compound blocks.
    """
    n = len(lines)
    while idx < n:
        line_str = lines[idx].strip()
        
        # Skip empty lines and comments
        if not line_str or line_str.startswith("//") or line_str.startswith("/*"):
            idx += 1
            continue
        
        # Skip attached pragmas
        if line_str.startswith("#pragma"):
            idx += 1
            continue
            
        # Case 1: Compound block starting with '{'
        if "{" in line_str:
            brace_depth = 0
            while idx < n:
                l = lines[idx]
                brace_depth += l.count("{") - l.count("}")
                idx += 1
                if brace_depth <= 0:
                    break
            return idx

        # Case 2: Control flow construct (for, while, if, do) without '{'
        elif any(line_str.startswith(kw) for kw in ["for", "while", "if", "do"]):
            idx += 1
            idx = consume_statement(lines, idx)
            return idx

        # Case 3: Standard single statement ending with ';'
        else:
            idx += 1
            while idx < n and ";" not in line_str:
                line_str = lines[idx].strip()
                idx += 1
            return idx

    return idx

def instrument_openmp_source(source_path):
    """
    Injects omp_get_wtime() timers and profiler statements into the source code.
    """
    with open(source_path, 'r') as f:
        lines = f.readlines()

    instrumented = ["#include <omp.h>\n#include <stdio.h>\n\n"]
    
    i = 0
    n = len(lines)
    in_region = False
    reg_start_line = 0

    while i < n:
        line = lines[i]
        line_str = line.strip()
        line_num = i + 1

        if "#pragma capc profitability_region begin" in line_str:
            in_region = True
            reg_start_line = line_num
            instrumented.append(line)
            instrumented.append("{\n")
            instrumented.append("  double _capc_t_start, _capc_t_end, _capc_tot;\n")
            instrumented.append("  double _capc_k_sum = 0.0;\n")
            instrumented.append("  double _capc_k0, _capc_k1;\n")
            instrumented.append("  _capc_t_start = omp_get_wtime();\n")
            i += 1
            continue

        if "#pragma capc profitability_region end" in line_str and in_region:
            instrumented.append("  _capc_t_end = omp_get_wtime();\n")
            instrumented.append("  _capc_tot = _capc_t_end - _capc_t_start;\n")
            instrumented.append(
                f'  printf("[PROFILER] line:{reg_start_line} | Transfer Time = %.9f s\\n", '
                f'(_capc_tot - _capc_k_sum > 0.0 ? _capc_tot - _capc_k_sum : 0.0));\n'
            )
            instrumented.append("}\n")
            instrumented.append(line)
            in_region = False
            i += 1
            continue

        if in_region and ("#pragma omp target" in line_str) and ("data" not in line_str):
            target_line_num = line_num
            instrumented.append("  _capc_k0 = omp_get_wtime();\n")
            instrumented.append(line)
            i += 1
            
            end_idx = consume_statement(lines, i)
            while i < end_idx:
                instrumented.append(lines[i])
                i += 1

            instrumented.append("  _capc_k1 = omp_get_wtime();\n")
            instrumented.append("  _capc_k_sum += (_capc_k1 - _capc_k0);\n")
            instrumented.append(
                f'  printf("[PROFILER] line:{target_line_num} | Kernel Execution Time = %.9f s\\n", '
                f'_capc_k1 - _capc_k0);\n'
            )
            continue

        instrumented.append(line)
        i += 1

    return "".join(instrumented)

def main():
    parser = argparse.ArgumentParser(description="Output the instrumented OpenMP C source code.")
    parser.add_argument("source", help="Path to input OpenMP C file (e.g., 3mm_omp45.c)")
    parser.add_argument("-o", "--output", help="Path to write transformed file (default: prints to stdout)")

    args = parser.parse_args()

    if not os.path.exists(args.source):
        print(f"Error: File '{args.source}' not found.", file=sys.stderr)
        sys.exit(1)

    transformed_code = instrument_openmp_source(args.source)

    if args.output:
        with open(args.output, "w") as f:
            f.write(transformed_code)
        print(f"[*] Transformed file successfully saved to: {args.output}")
    else:
        print(transformed_code)

if __name__ == "__main__":
    main()