#!/usr/bin/env python3

import os
import sys
import re
import subprocess
import argparse
import tempfile
import resource
from collections import defaultdict


# Expand stack size to prevent limits on large matrix allocations
try:
    resource.setrlimit(
        resource.RLIMIT_STACK,
        (resource.RLIM_INFINITY, resource.RLIM_INFINITY)
    )
except Exception:
    pass


def parse_regions(source_file):
    """
    Parse CAPC profitability regions and initialize timing accumulators.

    resident_time:
        Sum of GPU target kernel execution times only.

    init_time:
        One-time OpenMP GPU/runtime initialization cost, attributed to the
        profitability region associated with the first encountered target
        operation.

    one_time_transfer_time:
        Sum of target data/update directives that executed exactly once.

    recurring_transfer_time:
        Sum of target data/update directives that executed more than once.
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

                    "init_time": 0.0,
                    "one_time_transfer_time": 0.0,
                    "recurring_transfer_time": 0.0,

                    # Runtime transfer events grouped by source line.
                    "_transfer_events_by_line": defaultdict(list),
                }

            elif (
                "#pragma capc profitability_region end" in line_str
                and current_region
            ):
                current_region["end_line"] = line_num
                regions.append(current_region)
                region_id += 1
                current_region = None

    return regions


def get_associated_region_id(line_num, regions):
    """
    Map an OpenMP target/data operation to a profitability region.

    Policy retained from the original profiler:
      1. Inside a region       -> that region
      2. Before first region   -> Region 1
      3. Between two regions   -> preceding region
      4. After final region    -> final region

    Transfer cleanup-only directives are filtered separately and therefore
    do not contaminate observed region timing.
    """
    if not regions:
        return 1

    # Operation inside a profitability region
    for reg in regions:
        if reg["begin_line"] <= line_num <= reg["end_line"]:
            return reg["id"]

    # Operation before first profitability region
    if line_num < regions[0]["begin_line"]:
        return regions[0]["id"]

    # Operation between profitability regions
    for i in range(len(regions) - 1):
        if (
            regions[i]["end_line"]
            < line_num
            < regions[i + 1]["begin_line"]
        ):
            return regions[i]["id"]

    # Operation after final profitability region
    return regions[-1]["id"]


def consume_statement(lines, idx):
    """
    Recursively consume the complete C statement/block associated with an
    OpenMP target compute directive.
    """
    n = len(lines)

    while idx < n:
        line_str = lines[idx].strip()

        if (
            not line_str
            or line_str.startswith("//")
            or line_str.startswith("/*")
        ):
            idx += 1
            continue

        # Skip any extra pragmas between target directive and body/loop.
        if line_str.startswith("#pragma"):
            idx += 1
            continue

        if "{" in line_str:
            brace_depth = 0

            while idx < n:
                line = lines[idx]
                brace_depth += line.count("{") - line.count("}")
                idx += 1

                if brace_depth <= 0:
                    break

            return idx

        elif any(
            line_str.startswith(keyword)
            for keyword in ["for", "while", "if", "do"]
        ):
            idx += 1
            return consume_statement(lines, idx)

        else:
            idx += 1

            while idx < n and ";" not in line_str:
                line_str = lines[idx].strip()
                idx += 1

            return idx

    return idx



def consume_omp_pragma(lines, start_idx):
    """Consume one logical OpenMP pragma, including backslash continuations."""
    physical = [lines[start_idx]]
    idx = start_idx + 1

    while physical[-1].rstrip().endswith("\\") and idx < len(lines):
        physical.append(lines[idx])
        idx += 1

    pieces = []
    for line in physical:
        part = line.rstrip("\n").rstrip()
        if part.endswith("\\"):
            part = part[:-1].rstrip()
        pieces.append(part.strip())

    logical = re.sub(r"\s+", " ", " ".join(pieces)).strip()
    return logical, idx, physical


def split_top_level_commas(text):
    items = []
    current = []
    square = 0
    paren = 0

    for ch in text:
        if ch == "[":
            square += 1
        elif ch == "]":
            square = max(0, square - 1)
        elif ch == "(":
            paren += 1
        elif ch == ")":
            paren = max(0, paren - 1)

        if ch == "," and square == 0 and paren == 0:
            item = "".join(current).strip()
            if item:
                items.append(item)
            current = []
        else:
            current.append(ch)

    item = "".join(current).strip()
    if item:
        items.append(item)

    return items


def find_parenthesized_span(text, open_pos):
    depth = 0
    for pos in range(open_pos, len(text)):
        if text[pos] == "(":
            depth += 1
        elif text[pos] == ")":
            depth -= 1
            if depth == 0:
                return pos + 1
    raise ValueError("Unmatched parenthesis while parsing OpenMP pragma")


def parse_map_clauses(logical_pragma):
    """
    Parse all map(...) clauses from one logical target pragma.
    Returns dictionaries with type/items/start/end.
    """
    clauses = []
    lower = logical_pragma.lower()
    pos = 0

    while True:
        m = re.search(r"\bmap\s*\(", lower[pos:])
        if not m:
            break

        start = pos + m.start()
        open_paren = pos + m.end() - 1
        end = find_parenthesized_span(logical_pragma, open_paren)
        inside = logical_pragma[open_paren + 1:end - 1].strip()

        inside = re.sub(
            r"^\s*always\s*,\s*",
            "",
            inside,
            flags=re.IGNORECASE,
        )

        map_type = "tofrom"
        payload = inside

        tm = re.match(
            r"^\s*(tofrom|to|from|alloc|release|delete)\s*:\s*(.*)$",
            inside,
            flags=re.IGNORECASE,
        )
        if tm:
            map_type = tm.group(1).lower()
            payload = tm.group(2)

        clauses.append({
            "type": map_type,
            "items": split_top_level_commas(payload),
            "start": start,
            "end": end,
        })
        pos = end

    return clauses


def unique_preserve_order(items):
    seen = set()
    out = []
    for item in items:
        key = re.sub(r"\s+", "", item)
        if key not in seen:
            seen.add(key)
            out.append(item.strip())
    return out


def remove_map_clauses(logical_pragma):
    clauses = parse_map_clauses(logical_pragma)
    if not clauses:
        return logical_pragma

    pieces = []
    last = 0
    for clause in clauses:
        pieces.append(logical_pragma[last:clause["start"]])
        last = clause["end"]
    pieces.append(logical_pragma[last:])

    return re.sub(r"\s+", " ", "".join(pieces)).strip()


def rewrite_target_as_alloc(logical_pragma, all_specs):
    base = remove_map_clauses(logical_pragma)
    if all_specs:
        base += " map(alloc:" + ", ".join(all_specs) + ")"
    return re.sub(r"\s+", " ", base).strip()


def is_target_data_directive(logical_pragma):
    return bool(re.search(
        r"#\s*pragma\s+omp\s+target\s+(?:enter\s+data|exit\s+data|update|data\b)",
        logical_pragma,
        re.IGNORECASE,
    ))


def is_target_compute_directive(logical_pragma):
    return bool(re.search(
        r"#\s*pragma\s+omp\s+target\b",
        logical_pragma,
        re.IGNORECASE,
    )) and not is_target_data_directive(logical_pragma)


def transfer_sets_from_target_maps(logical_pragma):
    h2d = []
    d2h = []
    all_specs = []
    has_transfer_maps = False

    for clause in parse_map_clauses(logical_pragma):
        map_type = clause["type"]
        items = clause["items"]
        all_specs.extend(items)

        if map_type == "to":
            h2d.extend(items)
            has_transfer_maps = True
        elif map_type == "from":
            d2h.extend(items)
            has_transfer_maps = True
        elif map_type == "tofrom":
            h2d.extend(items)
            d2h.extend(items)
            has_transfer_maps = True

    return (
        unique_preserve_order(h2d),
        unique_preserve_order(d2h),
        unique_preserve_order(all_specs),
        has_transfer_maps,
    )


def get_declared_array_specs(source_text):
    """Infer OpenMP array-section specs from ordinary C declarations."""
    specs = {}
    type_re = re.compile(
        r"^\s*(?:(?:static|extern|const|volatile|register|auto)\s+)*"
        r"(?:(?:signed|unsigned)\s+)?"
        r"(?:(?:long\s+long|long|short)\s+)?"
        r"(?:long\s+double|double|float|int|char|size_t)\s+"
        r"(.+?)\s*;\s*$"
    )
    for statement in source_text.split(';'):
        line = re.sub(r"//.*$", "", statement).strip()
        if not line or line.startswith('#'):
            continue
        m = type_re.match(line + ';')
        if not m:
            continue
        for decl in split_top_level_commas(m.group(1)):
            decl = decl.split('=', 1)[0].strip()
            dm = re.match(
                r"(?:\*\s*)*([A-Za-z_][A-Za-z0-9_]*)\s*"
                r"((?:\[[^\]]+\]\s*)+)$",
                decl,
            )
            if not dm:
                continue
            name = dm.group(1)
            dims = re.findall(r"\[([^\]]+)\]", dm.group(2))
            if dims:
                specs.setdefault(name, name + ''.join(f"[0:{d.strip()}]" for d in dims))
    return specs


def spec_var_name(spec):
    m = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)", spec)
    return m.group(1) if m else None


def strip_omp_pragmas_for_analysis(code):
    lines = code.splitlines()
    out = []
    i = 0
    while i < len(lines):
        if re.match(r"^\s*#\s*pragma\s+omp\b", lines[i], re.IGNORECASE):
            while i < len(lines):
                continued = lines[i].rstrip().endswith('\\')
                i += 1
                if not continued:
                    break
            continue
        out.append(lines[i])
        i += 1
    return '\n'.join(out)


def infer_region_array_specs(logical_pragma, body_text, declared_specs):
    names = []
    explicit_specs = []
    for clause in parse_map_clauses(logical_pragma):
        for item in clause['items']:
            name = spec_var_name(item)
            if name and name not in names:
                names.append(name)
                explicit_specs.append(item.strip())

    analysis_body = strip_omp_pragmas_for_analysis(body_text)
    for name in re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\[", analysis_body):
        if name in declared_specs and name not in names:
            names.append(name)

    by_name = {spec_var_name(s): s for s in explicit_specs if spec_var_name(s)}
    specs = [by_name.get(name, declared_specs.get(name, name)) for name in names]
    return names, specs


def classify_array_accesses(body_text, names):
    code = strip_omp_pragmas_for_analysis(body_text)
    code = re.sub(r'"(?:\\.|[^"\\])*"', '""', code)
    code = re.sub(r"//.*?$", "", code, flags=re.MULTILINE)
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)

    reads, writes = [], []
    for var in names:
        saw_read = saw_write = False
        pattern = re.compile(r"\b" + re.escape(var) + r"\s*(?:\[[^\]]*\]\s*)+")
        for m in pattern.finditer(code):
            before = code[max(0, m.start()-4):m.start()]
            after = code[m.end():m.end()+8]
            if re.search(r"(?:\+\+|--)\s*$", before) or re.match(r"\s*(?:\+\+|--)", after):
                saw_read = saw_write = True
                continue
            op = re.match(r"\s*(<<=|>>=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|=)", after)
            if op:
                saw_write = True
                if op.group(1) != '=':
                    saw_read = True
            else:
                saw_read = True
        if saw_read:
            reads.append(var)
        if saw_write:
            writes.append(var)
    return reads, writes


def specs_for_names(names, declared_specs, preferred_specs=None):
    preferred_specs = preferred_specs or []
    by_name = {spec_var_name(s): s for s in preferred_specs if spec_var_name(s)}
    return [by_name.get(name, declared_specs.get(name, name)) for name in names]


def is_cleanup_only_target_exit(line_str):
    """
    Return True for cleanup-only OpenMP target exit-data deletion.

    Examples excluded from observed transfer timing:
        #pragma omp target exit data map(delete:A[0:N])
        #pragma omp target exit data map(release:A[0:N])

    But copyout-like exits must remain measurable:
        #pragma omp target exit data map(from:A[0:N])
    """
    lower = line_str.lower()

    if "target exit data" not in lower:
        return False

    has_delete_or_release = (
        "map(delete:" in lower
        or "map( delete:" in lower
        or "map(release:" in lower
        or "map( release:" in lower
    )

    has_from_or_tofrom = (
        "map(from:" in lower
        or "map( from:" in lower
        or "map(tofrom:" in lower
        or "map( tofrom:" in lower
    )

    return has_delete_or_release and not has_from_or_tofrom


def instrument_openmp_source(source_path, temp_path, regions):
    """Instrument OpenMP 4.5 while separating implicit/explicit transfers."""
    with open(source_path, 'r') as f:
        lines = f.readlines()
    declared_specs = get_declared_array_specs(''.join(lines))

    instrumented = [
        '#include <omp.h>\n', '#include <stdio.h>\n', '#include <stdlib.h>\n\n',
        'static double _capc_dt0, _capc_dt1;\n',
        'static double _capc_k0, _capc_k1;\n',
        'static double _capc_init0, _capc_init1;\n',
        'static int _capc_gpu_initialized = 0;\n\n',
        'static void _capc_ensure_gpu_init(int region_id, int line_num)\n', '{\n',
        '    if (!_capc_gpu_initialized) {\n',
        '        int _capc_device = omp_get_default_device();\n',
        '        void *_capc_init_ptr = NULL;\n',
        '        _capc_init0 = omp_get_wtime();\n',
        '        _capc_init_ptr = omp_target_alloc(1, _capc_device);\n',
        '        _capc_init1 = omp_get_wtime();\n',
        '        _capc_gpu_initialized = 1;\n',
        '        printf("[PROFILER] init region:%d line:%d | GPU Initialization Time = %.9f s\\n",\n',
        '               region_id, line_num, _capc_init1 - _capc_init0);\n',
        '        if (_capc_init_ptr != NULL) omp_target_free(_capc_init_ptr, _capc_device);\n',
        '    }\n', '}\n\n',
    ]

    persistent_names = set()
    pending_data = None
    i = 0

    def emit_transfer(reg_id, line_num, direction, specs, tag):
        if not specs:
            return
        instrumented.append('  _capc_dt0 = omp_get_wtime();\n')
        instrumented.append('  #pragma omp target update ' + direction + '(' + ', '.join(specs) + ')\n')
        instrumented.append('  #pragma omp taskwait\n')
        instrumented.append('  _capc_dt1 = omp_get_wtime();\n')
        instrumented.append(
            f'  printf("[PROFILER] transfer region:{reg_id} line:{line_num} tag:{tag} | Transfer Time = %.9f s\\n", _capc_dt1 - _capc_dt0);\n'
        )

    while i < len(lines):
        line = lines[i]
        line_num = i + 1

        if re.match(r'^\s*#\s*pragma\s+omp\s+target\b', line, re.IGNORECASE):
            logical, next_idx, physical = consume_omp_pragma(lines, i)
            reg_id = get_associated_region_id(line_num, regions)

            if re.search(r'#\s*pragma\s+omp\s+target\s+enter\s+data\b', logical, re.I):
                names = [spec_var_name(x) for c in parse_map_clauses(logical) for x in c['items']]
                persistent_names.update(x for x in names if x)
                instrumented.append(f'  _capc_ensure_gpu_init({reg_id}, {line_num});\n')
                instrumented.append('  _capc_dt0 = omp_get_wtime();\n')
                instrumented.extend(physical)
                instrumented.append('  #pragma omp taskwait\n  _capc_dt1 = omp_get_wtime();\n')
                instrumented.append(f'  printf("[PROFILER] transfer region:{reg_id} line:{line_num} tag:data | Transfer Time = %.9f s\\n", _capc_dt1 - _capc_dt0);\n')
                i = next_idx; continue

            if re.search(r'#\s*pragma\s+omp\s+target\s+exit\s+data\b', logical, re.I):
                names = [spec_var_name(x) for c in parse_map_clauses(logical) for x in c['items']]
                if is_cleanup_only_target_exit(logical):
                    instrumented.extend(physical)
                else:
                    instrumented.append(f'  _capc_ensure_gpu_init({reg_id}, {line_num});\n  _capc_dt0 = omp_get_wtime();\n')
                    instrumented.extend(physical)
                    instrumented.append('  #pragma omp taskwait\n  _capc_dt1 = omp_get_wtime();\n')
                    instrumented.append(f'  printf("[PROFILER] transfer region:{reg_id} line:{line_num} tag:data | Transfer Time = %.9f s\\n", _capc_dt1 - _capc_dt0);\n')
                for x in names:
                    if x: persistent_names.discard(x)
                i = next_idx; continue

            if re.search(r'#\s*pragma\s+omp\s+target\s+update\b', logical, re.I):
                instrumented.append(f'  _capc_ensure_gpu_init({reg_id}, {line_num});\n  _capc_dt0 = omp_get_wtime();\n')
                instrumented.extend(physical)
                instrumented.append('  #pragma omp taskwait\n  _capc_dt1 = omp_get_wtime();\n')
                instrumented.append(f'  printf("[PROFILER] transfer region:{reg_id} line:{line_num} tag:data | Transfer Time = %.9f s\\n", _capc_dt1 - _capc_dt0);\n')
                i = next_idx; continue

            # Convert a structured `target data map(...)` wrapper into an
            # explicit lifetime around its following compute construct.
            if re.search(r'#\s*pragma\s+omp\s+target\s+data\b', logical, re.I):
                h2d, d2h, all_specs, _ = transfer_sets_from_target_maps(logical)
                instrumented.append(f'  _capc_ensure_gpu_init({reg_id}, {line_num});\n')
                if all_specs:
                    instrumented.append('  #pragma omp target enter data map(alloc:' + ', '.join(all_specs) + ')\n  #pragma omp taskwait\n')
                emit_transfer(reg_id, line_num, 'to', h2d, 'target_data_to')
                pending_data = {
                    'names': {spec_var_name(x) for x in all_specs if spec_var_name(x)},
                    'all_specs': all_specs,
                    'd2h_specs': d2h,
                    'line_num': line_num,
                    'reg_id': reg_id,
                }
                i = next_idx; continue

            if is_target_compute_directive(logical):
                end_idx = consume_statement(lines, next_idx)
                body_text = ''.join(lines[next_idx:end_idx])
                names, inferred_specs = infer_region_array_specs(logical, body_text, declared_specs)
                reads, writes = classify_array_accesses(body_text, names)
                exp_h2d, exp_d2h, exp_specs, _ = transfer_sets_from_target_maps(logical)
                exp_names = {spec_var_name(x) for x in exp_specs if spec_var_name(x)}
                wrapper_names = pending_data['names'] if pending_data else set()
                resident_names = persistent_names | wrapper_names

                implicit_names = [x for x in names if x not in resident_names and x not in exp_names]
                imp_h2d = specs_for_names([x for x in implicit_names if x in reads], declared_specs, inferred_specs)
                imp_d2h = specs_for_names([x for x in implicit_names if x in writes], declared_specs, inferred_specs)
                h2d_specs = unique_preserve_order(exp_h2d + imp_h2d)
                d2h_specs = unique_preserve_order(exp_d2h + imp_d2h)
                all_specs = unique_preserve_order(inferred_specs + exp_specs)
                temp_specs = [x for x in all_specs if spec_var_name(x) not in resident_names]

                instrumented.append(f'  _capc_ensure_gpu_init({reg_id}, {line_num});\n')
                if temp_specs:
                    instrumented.append('  #pragma omp target enter data map(alloc:' + ', '.join(temp_specs) + ')\n  #pragma omp taskwait\n')
                emit_transfer(reg_id, line_num, 'to', h2d_specs, 'implicit_to')

                instrumented.append('  _capc_k0 = omp_get_wtime();\n')
                instrumented.append('  ' + rewrite_target_as_alloc(logical, all_specs) + '\n')
                instrumented.extend(lines[next_idx:end_idx])
                instrumented.append('  #pragma omp taskwait\n  _capc_k1 = omp_get_wtime();\n')
                instrumented.append(f'  printf("[PROFILER] kernel region:{reg_id} line:{line_num} | Kernel Execution Time = %.9f s\\n", _capc_k1 - _capc_k0);\n')
                emit_transfer(reg_id, line_num, 'from', d2h_specs, 'implicit_from')

                if pending_data:
                    seen = {re.sub(r'\s+', '', x) for x in d2h_specs}
                    extra = [x for x in pending_data['d2h_specs'] if re.sub(r'\s+', '', x) not in seen]
                    emit_transfer(pending_data['reg_id'], pending_data['line_num'], 'from', extra, 'target_data_from')

                if temp_specs:
                    instrumented.append('  #pragma omp target exit data map(delete:' + ', '.join(temp_specs) + ')\n  #pragma omp taskwait\n')
                if pending_data and pending_data['all_specs']:
                    instrumented.append('  #pragma omp target exit data map(delete:' + ', '.join(pending_data['all_specs']) + ')\n  #pragma omp taskwait\n')
                pending_data = None
                i = end_idx; continue

        instrumented.append(line)
        i += 1

    with open(temp_path, 'w') as f:
        f.writelines(instrumented)


def compile_openmp_program(
    source_file,
    exec_name,
    gpu_arch="cc70",
):
    """
    Compile instrumented OpenMP GPU-offload program using NVHPC nvc.
    """
    compile_cmd = [
        "nvc",
        "-mp=gpu",
        f"-gpu={gpu_arch}",
        "-Minfo=mp",
        source_file,
        "-o",
        exec_name,
    ]

    print(
        f"[*] Compiling OpenMP program: "
        f"{' '.join(compile_cmd)}"
    )

    result = subprocess.run(
        compile_cmd,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        print(
            f"[-] Compilation failed for '{source_file}':\n"
            f"{result.stderr}"
        )
        sys.exit(1)


def run_executable(exec_path):
    """
    Execute target binary and capture stdout/stderr.
    """
    print(
        f"[*] Executing target OpenMP binary: "
        f"{exec_path}\n"
    )

    result = subprocess.run(
        [exec_path],
        capture_output=True,
        text=True,
    )

    return (
        result.stdout,
        result.stderr,
        result.returncode,
    )


def process_profiler_output(
    stdout_str,
    stderr_str,
    returncode,
    regions,
):
    """
    Parse runtime profiler events.

    Transfer events are grouped by (source line, transfer tag). This is
    important for a target line that contains both map(to:) and map(from:):
    the H2D and D2H events must not be mistaken for two executions of one
    recurring transfer.
    """
    combined_log = stdout_str + "\n" + stderr_str

    pattern = re.compile(
        r"\[PROFILER\]\s+"
        r"(kernel|transfer|init)\s+"
        r"region:(\d+)\s+"
        r"line:(\d+)"
        r"(?:\s+tag:([A-Za-z0-9_]+))?"
        r"\s+\|\s+"
        r"(.*?)\s+=\s+"
        r"([\d\.]+)\s+s"
    )

    matched_events = 0
    region_map = {
        reg["id"]: reg
        for reg in regions
    }

    for line in combined_log.splitlines():
        match = pattern.search(line)

        if not match:
            continue

        matched_events += 1

        event_cat = match.group(1)
        reg_id = int(match.group(2))
        line_num = int(match.group(3))
        tag = match.group(4) or event_cat
        duration = float(match.group(6))

        if reg_id not in region_map:
            continue

        reg = region_map[reg_id]

        if event_cat == "kernel":
            reg["resident_time"] += duration
            reg["count"] += 1

        elif event_cat == "init":
            reg["init_time"] += duration

        elif event_cat == "transfer":
            reg["_transfer_events_by_line"][
                (line_num, tag)
            ].append(duration)

    for reg in regions:
        for _, durations in reg[
            "_transfer_events_by_line"
        ].items():
            if len(durations) == 1:
                reg["one_time_transfer_time"] += durations[0]
            else:
                reg["recurring_transfer_time"] += sum(durations)

    if matched_events == 0:
        print(
            "[!] Warning: No [PROFILER] output logs were detected."
        )
        print(
            f"[!] Executable Return Code: {returncode}"
        )


def print_results(regions):
    """
    Render the original CAPC report layout.

    Definitions
    -----------

    Total Res(s)
        Sum of kernel execution times across all invocations.

    Avg Res(s)
        Resident/kernel time of one invocation:
            Total Resident / Invocations

    Total Obs(s)
        Cumulative observed contribution across the entire original execution:
            GPU initialization
          + one-time transfer/setup
          + recurring transfers
          + all kernel invocations

    Avg Obs(s)
        Observed time of ONE cold/first invocation:
            GPU initialization
          + one-time transfer/setup
          + average recurring observed cost

        where:
            average recurring observed cost
              = (kernel total + recurring transfer total) / invocations

    IMPORTANT:
        GPU initialization and one-time setup are NOT divided by invocation
        count.
    """
    header = (
        f"{'Region':<8} | "
        f"{'Lines':<8} | "
        f"{'Invocations':<11} | "
        f"{'Total Res(s)':<12} | "
        f"{'Avg Res(s)':<12} | "
        f"{'Total Obs(s)':<12} | "
        f"{'Avg Obs(s)':<12}"
    )

    divider = "-" * len(header)

    print(divider)
    print(
        "                    "
        "CAPC PROFITABILITY REGION REPORT (OPENMP 4.5)"
    )
    print(divider)
    print(header)
    print(divider)

    total_resident = 0.0
    total_observed = 0.0
    total_invocations = 0

    for reg in regions:
        actual_count = reg["count"]
        count = max(actual_count, 1)

        # Resident = kernel only.
        avg_resident = (
            reg["resident_time"]
            / count
        )

        # Recurring cost per invocation.
        avg_recurring_observed = (
            reg["resident_time"]
            + reg["recurring_transfer_time"]
        ) / count

        # Observed time of ONE cold/first invocation.
        avg_observed = (
            reg["init_time"]
            + reg["one_time_transfer_time"]
            + avg_recurring_observed
        )

        # Actual cumulative observed contribution.
        total_observed_region = (
            reg["init_time"]
            + reg["one_time_transfer_time"]
            + reg["recurring_transfer_time"]
            + reg["resident_time"]
        )

        total_resident += reg["resident_time"]
        total_observed += total_observed_region
        total_invocations += actual_count

        line_range = (
            f"{reg['begin_line']}-"
            f"{reg['end_line']}"
        )

        print(
            f"Region {reg['id']:<1} | "
            f"{line_range:<8} | "
            f"{actual_count:<11} | "
            f"{reg['resident_time']:<12.6f} | "
            f"{avg_resident:<12.6f} | "
            f"{total_observed_region:<12.6f} | "
            f"{avg_observed:<12.6f}"
        )

    print(divider)

    avg_total_res = (
        total_resident
        / max(total_invocations, 1)
    )

    # Kept for compatibility with the old table format.
    # This TOTAL average is a whole-run average over all region invocations.
    avg_total_obs = (
        total_observed
        / max(total_invocations, 1)
    )

    print(
        f"{'TOTAL':<8} | "
        f"{'-':<8} | "
        f"{total_invocations:<11} | "
        f"{total_resident:<12.6f} | "
        f"{avg_total_res:<12.6f} | "
        f"{total_observed:<12.6f} | "
        f"{avg_total_obs:<12.6f}"
    )

    print(
        divider
        + "\n"
    )


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Compile, run, and profile OpenMP 4.5 GPU regions while "
            "separating resident kernel time, GPU initialization, "
            "one-time transfers, and recurring transfers."
        )
    )

    parser.add_argument(
        "source",
        help="Path to OpenMP 4.5 source C file",
    )

    parser.add_argument(
        "--gpu",
        default="cc70",
        help="GPU architecture (default: cc70)",
    )

    args = parser.parse_args()

    source_path = os.path.abspath(
        args.source
    )

    if not os.path.exists(source_path):
        print(
            f"Error: Source file "
            f"'{args.source}' not found."
        )
        sys.exit(1)

    work_dir = os.path.dirname(
        source_path
    )

    exec_name = os.path.splitext(
        os.path.basename(source_path)
    )[0]

    exec_path = os.path.join(
        work_dir,
        exec_name,
    )

    regions = parse_regions(
        source_path
    )

    if not regions:
        print(
            "Error: No "
            "'#pragma capc profitability_region' "
            "blocks found in source file."
        )
        sys.exit(1)

    temp_fd, temp_source_path = tempfile.mkstemp(
        suffix=".c",
        dir=work_dir,
    )

    os.close(temp_fd)

    try:
        instrument_openmp_source(
            source_path,
            temp_source_path,
            regions,
        )

        compile_openmp_program(
            temp_source_path,
            exec_path,
            gpu_arch=args.gpu,
        )

        (
            stdout_str,
            stderr_str,
            returncode,
        ) = run_executable(
            exec_path
        )

        process_profiler_output(
            stdout_str,
            stderr_str,
            returncode,
            regions,
        )

        print_results(
            regions
        )

    finally:
        if os.path.exists(
            temp_source_path
        ):
            os.remove(
                temp_source_path
            )


if __name__ == "__main__":
    main()
