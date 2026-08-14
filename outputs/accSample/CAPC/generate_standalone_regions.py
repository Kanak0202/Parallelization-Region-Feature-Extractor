import sys
import re
import os
import shutil
import subprocess

STANDARD_VARS = {'i', 'j', 'k', 't'}

def get_array_bounds_map(full_code):
    bounds_map = {}
    clause_matches = re.findall(
        r'\b(?:create|copyin|copyout|copy|present|pcopy|pcopyin|pcopyout)\s*\(([^)]+)\)',
        full_code, re.IGNORECASE
    )
    for match in clause_matches:
        items = [item.strip() for item in match.split(',')]
        for item in items:
            var_match = re.match(r'^([a-zA-Z0-9_]+)', item)
            if var_match:
                var_name = var_match.group(1)
                if '[' in item and var_name not in bounds_map:
                    bounds_map[var_name] = item
    return bounds_map

def get_target_region_array_specs(target_block, bounds_map):
    target_vars = []
    clause_matches = re.findall(
        r'\b(?:create|copyin|copyout|copy|present|pcopy|pcopyin|pcopyout)\s*\(([^)]+)\)',
        target_block, re.IGNORECASE
    )
    for match in clause_matches:
        items = [item.strip() for item in match.split(',')]
        for item in items:
            var_match = re.match(r'^([a-zA-Z0-9_]+)', item)
            if var_match:
                var_name = var_match.group(1)
                if var_name not in target_vars:
                    target_vars.append(var_name)

    if not target_vars:
        indexed_vars = re.findall(r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\[', target_block)
        for var in indexed_vars:
            if var in bounds_map and var not in target_vars:
                target_vars.append(var)

    specs = []
    for v in target_vars:
        if v in bounds_map:
            specs.append(bounds_map[v])
        else:
            specs.append(v)
    return specs, target_vars

def is_block_unclosed_from_line(start_idx, lines):
    depth = 0
    has_opened = False
    for i in range(start_idx, len(lines)):
        line = lines[i]
        o = line.count('{')
        c = line.count('}')
        if o > 0:
            has_opened = True
        depth += (o - c)
        if has_opened and depth <= 0:
            return False
    return True

def sanitize_c_segment(code_str, state=None):
    if state is None:
        state = {'suppressed_braces': 0}

    lines = code_str.splitlines()
    clean_lines = []
    if_stack = 0
    local_loop_depth = 0

    for idx, line in enumerate(lines):
        stripped = line.strip()

        if 'profitability_region' in stripped:
            continue

        decl_standalone_match = re.match(r'^\s*(int|double|float|long)\s+([a-zA-Z0-9_,\s]+)\s*;\s*$', line)
        if decl_standalone_match:
            vars_list = [v.strip() for v in decl_standalone_match.group(2).split(',')]
            if all(v in STANDARD_VARS for v in vars_list):
                continue

        decl_init_match = re.match(r'^\s*(int|double|float|long)\s+([a-zA-Z0-9_]+)\s*=(.*);', line)
        if decl_init_match:
            var_name = decl_init_match.group(2)
            val_part = decl_init_match.group(3)
            if var_name in STANDARD_VARS:
                line = f"    {var_name} ={val_part};"
                stripped = line.strip()

        if re.match(r'^\s*(for|while|do|if)\b', stripped):
            if is_block_unclosed_from_line(idx, lines):
                if '{' in stripped:
                    state['suppressed_braces'] += stripped.count('{')
                continue

        if stripped == '{':
            if idx > 0 and re.match(r'^\s*(for|while|do|if)\b', lines[idx - 1]):
                if is_block_unclosed_from_line(idx, lines):
                    state['suppressed_braces'] += 1
                    continue

        if stripped.startswith('}'):
            if state['suppressed_braces'] > 0:
                state['suppressed_braces'] -= 1
                remainder = stripped[1:].strip()
                if not remainder:
                    continue
                else:
                    line = remainder
                    stripped = line.strip()

        if re.match(r'^\s*(for|while|do)\b', stripped) and not is_block_unclosed_from_line(idx, lines):
            local_loop_depth += stripped.count('{')
        if '}' in stripped and local_loop_depth > 0:
            local_loop_depth -= stripped.count('}')
            if local_loop_depth < 0:
                local_loop_depth = 0

        if stripped in ('break;', 'continue;') or re.match(r'^\s*(break|continue)\s*;\s*$', stripped):
            if local_loop_depth == 0:
                clean_lines.append(f"    // {stripped}  /* Skipped break/continue outside loop */")
                continue

        if re.match(r'^\s*#\s*(if|ifdef|ifndef)\b', stripped):
            if_stack += 1
            clean_lines.append(line)
        elif re.match(r'^\s*#\s*endif\b', stripped):
            if if_stack > 0:
                if_stack -= 1
                clean_lines.append(line)
            else:
                clean_lines.append(f"// {line}  /* Skipped orphaned #endif */")
        elif re.match(r'^\s*#\s*(else|elif)\b', stripped):
            if if_stack > 0:
                clean_lines.append(line)
            else:
                clean_lines.append(f"// {line}  /* Skipped orphaned preprocessor directive */")
        else:
            clean_lines.append(line)

    while if_stack > 0:
        clean_lines.append("#endif /* Auto-closed for standalone segment balance */")
        if_stack -= 1

    return "\n".join(clean_lines)

def parse_c_file(file_path):
    with open(file_path, 'r') as f:
        content = f.read()

    bounds_map = get_array_bounds_map(content)

    region_pattern = re.compile(
        r'(#pragma\s+capc\s+profitability_region\s+begin[^\n]*\n)(.*?)(#pragma\s+capc\s+profitability_region\s+end[^\n]*)',
        re.DOTALL | re.IGNORECASE
    )

    region_matches = list(region_pattern.finditer(content))
    if not region_matches:
        raise ValueError("No '#pragma capc profitability_region begin/end' markers found in file.")

    main_match = re.search(r'(int\s+main\s*\([^)]*\)\s*\{)', content)
    if not main_match:
        raise ValueError("Could not locate main() function in the input file.")

    main_start = main_match.end()
    header_code = content[:main_match.start()]
    main_opening = main_match.group(1)
    main_body = content[main_start:]

    parsed_regions = []
    for idx, match in enumerate(region_matches, start=1):
        begin_line = match.group(1).strip()
        body_code = match.group(2).strip()
        end_line = match.group(3).strip()

        id_match = re.search(r'begin\s*(?:\(\s*(\w+)\s*\)|\s+(\w+))', begin_line, re.IGNORECASE)
        region_id = id_match.group(1) or id_match.group(2) if id_match else str(idx)

        full_region_block = f"    {begin_line}\n    {body_code}\n    {end_line}"
        is_in_main = match.start() >= main_match.start()
        parsed_regions.append((region_id, full_region_block, match.start(), match.end(), is_in_main))

    main_regions = [r for r in parsed_regions if r[4]]

    raw_main_segments = []
    last_pos = 0
    for r_id, r_block, start_pos, end_pos, _ in main_regions:
        rel_start = start_pos - main_start
        rel_end = end_pos - main_start
        seg = main_body[last_pos:rel_start]
        raw_main_segments.append(seg)
        last_pos = rel_end

    tail_seg = main_body[last_pos:]
    raw_main_segments.append(tail_seg)

    return header_code, main_opening, raw_main_segments, parsed_regions, main_regions, bounds_map

def clean_directory(output_dir):
    if os.path.exists(output_dir):
        print(f"Cleaning previous standalone region files in '{output_dir}'...")
        shutil.rmtree(output_dir)
    os.makedirs(output_dir, exist_ok=True)

def generate_standalone_files(header_code, main_opening, raw_main_segments, parsed_regions, main_regions, bounds_map, output_dir="standalone_regions"):
    clean_directory(output_dir)

    all_global_specs = [bounds_map[v] for v in bounds_map]
    global_specs_str = ", ".join(all_global_specs) if all_global_specs else ""

    generated_files = []
    for target_idx, (target_id, target_block, start_pos, end_pos, is_in_main) in enumerate(parsed_regions):
        filename = os.path.join(output_dir, f"region_{target_id}_standalone.c")
        
        main_preceding = [r for r in main_regions if r[2] < start_pos]
        m_count = len(main_preceding)
        has_prior_dependencies = (m_count > 0)

        state = {'suppressed_braces': 0}

        array_specs, target_var_names = get_target_region_array_specs(target_block, bounds_map)
        target_pragma_lower = target_block.lower()

        # Filter out arrays that are purely write-only / initialized via copyout clauses
        # to avoid unnecessary Host -> Device transfers prior to execution.
        pre_copyin_specs = []
        for spec in array_specs:
            var_name = spec.split('[')[0].strip()
            
            is_copyout = "copyout(" in target_pragma_lower and var_name in target_pragma_lower
            is_copyin_or_used = any(
                clause in target_pragma_lower 
                for clause in [f"copyin({var_name}", f"copy({var_name}", f"present({var_name}"]
            )

            if not (is_copyout and not is_copyin_or_used):
                pre_copyin_specs.append(spec)

        pre_copyin_str = ", ".join(pre_copyin_specs) if pre_copyin_specs else ""

        with open(filename, 'w') as f:
            f.write("#define _GNU_SOURCE\n")
            f.write("#define _POSIX_C_SOURCE 199309L\n")
            f.write("#include <time.h>\n")
            f.write("#include <stdio.h>\n\n")
            f.write(header_code + "\n")
            f.write(main_opening + "\n\n")

            f.write("    int i, j, k, t;\n")
            f.write("    struct timespec t_start, t_end;\n")
            f.write("    double t_in = 0.0, t_gpu = 0.0, t_out = 0.0;\n\n")

            f.write("    /* === STAGE 1 & 2: Interleaved Setup & Prerequisite Regions === */\n")
            
            has_enter_data_in_setup = False

            for k in range(m_count):
                if k < len(raw_main_segments):
                    seg_clean = sanitize_c_segment(raw_main_segments[k], state).strip()
                    if seg_clean:
                        if 'enter data' in seg_clean.lower():
                            has_enter_data_in_setup = True
                        f.write(f"    {seg_clean}\n\n")
                
                f.write(f"    // Dependent Region {main_preceding[k][0]}\n")
                f.write(f"    {main_preceding[k][1]}\n")
                
                if not main_preceding[k][1].strip().endswith("#pragma acc wait"):
                    f.write("    #pragma acc wait\n\n")
                else:
                    f.write("\n")

            if m_count < len(raw_main_segments):
                seg_clean = sanitize_c_segment(raw_main_segments[m_count], state).strip()
                if seg_clean:
                    if 'enter data' in seg_clean.lower():
                        has_enter_data_in_setup = True
                    f.write(f"    {seg_clean}\n\n")

            # Allocate VRAM on device if not done in setup
            if global_specs_str and not has_prior_dependencies and not has_enter_data_in_setup:
                f.write(f"    /* Ensure array allocation on device */\n")
                f.write(f"    #pragma acc enter data create({global_specs_str})\n\n")

            # === STAGE 3A: Transfer In (Host -> Device) Timing ===
            if pre_copyin_str and not has_prior_dependencies:
                f.write(f"    /* === Transfer In (Host -> Device) === */\n")
                f.write("    clock_gettime(CLOCK_MONOTONIC, &t_start);\n")
                f.write(f"    #pragma acc update device({pre_copyin_str})\n")
                f.write("    #pragma acc wait\n")
                f.write("    clock_gettime(CLOCK_MONOTONIC, &t_end);\n")
                f.write("    t_in = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;\n\n")
            else:
                f.write("    /* === Pre-timing Copyin skipped: Region is write-only / copyout or has prior dependencies === */\n\n")

            # === STAGE 3B: Target Kernel Execution Timing ===
            f.write(f"    /* === Isolated Kernel Timing for Target Region {target_id} === */\n")
            f.write("    clock_gettime(CLOCK_MONOTONIC, &t_start);\n\n")

            f.write(f"    {target_block}\n\n")

            f.write("    #pragma acc wait\n")
            f.write("    clock_gettime(CLOCK_MONOTONIC, &t_end);\n")
            f.write("    t_gpu = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;\n\n")

            # === STAGE 3C: Transfer Out (Device -> Host) Timing ===
            has_explicit_copyout = any(clause in target_pragma_lower for clause in ['copyout', 'copy(', 'copyout('])
            
            if not has_explicit_copyout and array_specs:
                specs_str = ", ".join(array_specs)
                f.write("    /* === Transfer Out (Device -> Host) === */\n")
                f.write("    clock_gettime(CLOCK_MONOTONIC, &t_start);\n")
                f.write(f"    #pragma acc update self({specs_str})\n")
                f.write("    #pragma acc wait\n")
                f.write("    clock_gettime(CLOCK_MONOTONIC, &t_end);\n")
                f.write("    t_out = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;\n\n")
            else:
                f.write("    /* === Copyout skipped: Target region uses explicit copyout clauses or has no array specs === */\n\n")

            # === STAGE 4: Reporting ===
            f.write("    /* === STAGE 4: Reporting Breakdown === */\n")
            f.write("    double t_total = t_in + t_gpu + t_out;\n")
            f.write(f'    printf("Region {target_id} Execution Breakdown:\\n");\n')
            f.write('    printf("  - Transfer In  (H2D): %f seconds\\n", t_in);\n')
            f.write('    printf("  - Kernel Time (GPU): %f seconds\\n", t_gpu);\n')
            f.write('    printf("  - Transfer Out (D2H): %f seconds\\n", t_out);\n')
            f.write('    printf("  - Total Region Time : %f seconds\\n", t_total);\n\n')
            f.write("    return 0;\n}")

        print(f"Generated: {filename}")
        generated_files.append((target_id, filename))

    return generated_files

def compile_and_run_regions(generated_files, compiler="nvc", flags=None):
    if flags is None:
        flags = ["-acc", "-mp", "-gpu=cc70", "--diag_suppress", "declared_but_not_referenced"]

    print("\n" + "=" * 50)
    print(" COMPILING & EXECUTING STANDALONE REGIONS")
    print("=" * 50)

    for target_id, c_file in generated_files:
        exe_file = c_file.replace(".c", "")
        
        compile_cmd = [compiler] + flags + [c_file, "-o", exe_file]
        print(f"\n[Compiling Region {target_id}]: {' '.join(compile_cmd)}")
        comp_process = subprocess.run(compile_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        
        if comp_process.stderr.strip():
            print(f"[Compiler Output]:\n{comp_process.stderr.strip()}")

        if comp_process.returncode != 0:
            print(f"❌ Compilation failed for Region {target_id}!")
            continue

        print(f"[Running Region {target_id}]: {exe_file}")
        run_process = subprocess.run([os.path.abspath(exe_file)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        if run_process.returncode == 0:
            print(f"✅ {run_process.stdout.strip()}")
        else:
            print(f"❌ Execution failed for Region {target_id}!\n{run_process.stderr.strip()}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python generate_standalone_regions.py <input_benchmark.c>")
        sys.exit(1)

    input_file = sys.argv[1]
    header, main_open, raw_segs, region_list, main_r_list, bounds_map = parse_c_file(input_file)
    files = generate_standalone_files(header, main_open, raw_segs, region_list, main_r_list, bounds_map)
    compile_and_run_regions(files)