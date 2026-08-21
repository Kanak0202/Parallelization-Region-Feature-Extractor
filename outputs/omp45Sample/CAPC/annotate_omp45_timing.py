#!/usr/bin/env python3

"""
Combined CAPC OpenMP 4.5 timing annotator.

This file directly contains the finalized OpenMP 4.5 Resident/Observed
profiler logic and finalized OpenMP 4.5 standalone/Isolated generator logic.
No external CAPC timing Python modules are required.

Timing definitions:
    Resident  = kernel execution with device/runtime already initialized and
                required array data resident on the GPU.

    Observed  = cost attributed to one region invocation in the original
                program context. One-time GPU initialization/setup is NOT
                amortized across repeated invocations. Recurring costs are
                averaged per invocation for Avg Obs.

    Isolated  = GPU initialization + required H2D + kernel + required D2H
                when the target region is executed as the only GPU region in
                a standalone program.
"""


# =============================================================================
# FINALIZED OPENMP 4.5 RESIDENT/OBSERVED ENGINE
# =============================================================================
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


def prof_parse_regions(source_file):
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


def prof_get_associated_region_id(line_num, regions):
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


def prof_consume_statement(lines, idx):
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
            return prof_consume_statement(lines, idx)

        else:
            idx += 1

            while idx < n and ";" not in line_str:
                line_str = lines[idx].strip()
                idx += 1

            return idx

    return idx



def prof_consume_omp_pragma(lines, start_idx):
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


def prof_split_top_level_commas(text):
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


def prof_find_parenthesized_span(text, open_pos):
    depth = 0
    for pos in range(open_pos, len(text)):
        if text[pos] == "(":
            depth += 1
        elif text[pos] == ")":
            depth -= 1
            if depth == 0:
                return pos + 1
    raise ValueError("Unmatched parenthesis while parsing OpenMP pragma")


def prof_parse_map_clauses(logical_pragma):
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
        end = prof_find_parenthesized_span(logical_pragma, open_paren)
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
            "items": prof_split_top_level_commas(payload),
            "start": start,
            "end": end,
        })
        pos = end

    return clauses


def prof_unique_preserve_order(items):
    seen = set()
    out = []
    for item in items:
        key = re.sub(r"\s+", "", item)
        if key not in seen:
            seen.add(key)
            out.append(item.strip())
    return out


def prof_remove_map_clauses(logical_pragma):
    clauses = prof_parse_map_clauses(logical_pragma)
    if not clauses:
        return logical_pragma

    pieces = []
    last = 0
    for clause in clauses:
        pieces.append(logical_pragma[last:clause["start"]])
        last = clause["end"]
    pieces.append(logical_pragma[last:])

    return re.sub(r"\s+", " ", "".join(pieces)).strip()


def prof_rewrite_target_as_alloc(logical_pragma, all_specs):
    base = prof_remove_map_clauses(logical_pragma)
    if all_specs:
        base += " map(alloc:" + ", ".join(all_specs) + ")"
    return re.sub(r"\s+", " ", base).strip()


def prof_is_target_data_directive(logical_pragma):
    return bool(re.search(
        r"#\s*pragma\s+omp\s+target\s+(?:enter\s+data|exit\s+data|update|data\b)",
        logical_pragma,
        re.IGNORECASE,
    ))


def prof_is_target_compute_directive(logical_pragma):
    return bool(re.search(
        r"#\s*pragma\s+omp\s+target\b",
        logical_pragma,
        re.IGNORECASE,
    )) and not prof_is_target_data_directive(logical_pragma)


def prof_transfer_sets_from_target_maps(logical_pragma):
    h2d = []
    d2h = []
    all_specs = []
    has_transfer_maps = False

    for clause in prof_parse_map_clauses(logical_pragma):
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
        prof_unique_preserve_order(h2d),
        prof_unique_preserve_order(d2h),
        prof_unique_preserve_order(all_specs),
        has_transfer_maps,
    )


def prof_get_declared_array_specs(source_text):
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
        for decl in prof_split_top_level_commas(m.group(1)):
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


def prof_spec_var_name(spec):
    m = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)", spec)
    return m.group(1) if m else None


def prof_strip_omp_pragmas_for_analysis(code):
    lines = code.splitlines(); out = []; i = 0
    while i < len(lines):
        if re.match(r"^\s*#\s*pragma\s+omp\b", lines[i], re.IGNORECASE):
            while i < len(lines):
                cont = lines[i].rstrip().endswith('\\'); i += 1
                if not cont: break
            continue
        out.append(lines[i]); i += 1
    return '\n'.join(out)


def prof_infer_region_array_specs(logical_pragma, body_text, declared_specs):
    names = []; explicit_specs = []
    for clause in prof_parse_map_clauses(logical_pragma):
        for item in clause['items']:
            name = prof_spec_var_name(item)
            if name and name not in names:
                names.append(name); explicit_specs.append(item.strip())
    body = prof_strip_omp_pragmas_for_analysis(body_text)
    for name in re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\[", body):
        if name in declared_specs and name not in names:
            names.append(name)
    by_name = {prof_spec_var_name(s): s for s in explicit_specs if prof_spec_var_name(s)}
    return names, [by_name.get(n, declared_specs.get(n, n)) for n in names]


def prof_classify_array_accesses(body_text, names):
    code = prof_strip_omp_pragmas_for_analysis(body_text)
    code = re.sub(r'"(?:\\.|[^"\\])*"', '""', code)
    code = re.sub(r"//.*?$", "", code, flags=re.MULTILINE)
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    reads, writes = [], []
    for var in names:
        saw_read = saw_write = False
        pattern = re.compile(r"\b" + re.escape(var) + r"\s*(?:\[[^\]]*\]\s*)+")
        for m in pattern.finditer(code):
            before = code[max(0,m.start()-4):m.start()]; after = code[m.end():m.end()+8]
            if re.search(r"(?:\+\+|--)\s*$", before) or re.match(r"\s*(?:\+\+|--)", after):
                saw_read = saw_write = True; continue
            op = re.match(r"\s*(<<=|>>=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|=)", after)
            if op:
                saw_write = True
                if op.group(1) != '=': saw_read = True
            else:
                saw_read = True
        if saw_read: reads.append(var)
        if saw_write: writes.append(var)
    return reads, writes


def prof_specs_for_names(names, declared_specs, preferred_specs=None):
    preferred_specs = preferred_specs or []
    by_name = {prof_spec_var_name(s): s for s in preferred_specs if prof_spec_var_name(s)}
    return [by_name.get(n, declared_specs.get(n, n)) for n in names]


def prof_is_cleanup_only_target_exit(line_str):
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


def prof_instrument_openmp_source(source_path, temp_path, regions):
    with open(source_path, 'r') as f:
        lines = f.readlines()
    declared_specs = prof_get_declared_array_specs(''.join(lines))
    instrumented = [
        '#include <omp.h>\n', '#include <stdio.h>\n', '#include <stdlib.h>\n\n',
        'static double _capc_dt0, _capc_dt1;\n', 'static double _capc_k0, _capc_k1;\n',
        'static double _capc_init0, _capc_init1;\n', 'static int _capc_gpu_initialized = 0;\n\n',
        'static void _capc_ensure_gpu_init(int region_id, int line_num)\n{\n',
        '    if (!_capc_gpu_initialized) {\n', '        int _capc_device = omp_get_default_device();\n',
        '        void *_capc_init_ptr = NULL;\n', '        _capc_init0 = omp_get_wtime();\n',
        '        _capc_init_ptr = omp_target_alloc(1, _capc_device);\n', '        _capc_init1 = omp_get_wtime();\n',
        '        _capc_gpu_initialized = 1;\n',
        '        printf("[PROFILER] init region:%d line:%d | GPU Initialization Time = %.9f s\\n", region_id, line_num, _capc_init1 - _capc_init0);\n',
        '        if (_capc_init_ptr != NULL) omp_target_free(_capc_init_ptr, _capc_device);\n',
        '    }\n}\n\n',
    ]
    persistent_names=set(); pending_data=None; i=0

    def emit_transfer(reg_id,line_num,direction,specs,tag):
        if not specs: return
        instrumented.append('  _capc_dt0 = omp_get_wtime();\n')
        instrumented.append('  #pragma omp target update '+direction+'('+', '.join(specs)+')\n')
        instrumented.append('  #pragma omp taskwait\n  _capc_dt1 = omp_get_wtime();\n')
        instrumented.append(f'  printf("[PROFILER] transfer region:{reg_id} line:{line_num} tag:{tag} | Transfer Time = %.9f s\\n", _capc_dt1 - _capc_dt0);\n')

    while i < len(lines):
        line=lines[i]; line_num=i+1
        if re.match(r'^\s*#\s*pragma\s+omp\s+target\b',line,re.I):
            logical,next_idx,physical=prof_consume_omp_pragma(lines,i)
            reg_id=prof_get_associated_region_id(line_num,regions)
            if re.search(r'#\s*pragma\s+omp\s+target\s+enter\s+data\b',logical,re.I):
                names=[prof_spec_var_name(x) for c in prof_parse_map_clauses(logical) for x in c['items']]
                persistent_names.update(x for x in names if x)
                instrumented.append(f'  _capc_ensure_gpu_init({reg_id}, {line_num});\n  _capc_dt0 = omp_get_wtime();\n'); instrumented.extend(physical)
                instrumented.append('  #pragma omp taskwait\n  _capc_dt1 = omp_get_wtime();\n')
                instrumented.append(f'  printf("[PROFILER] transfer region:{reg_id} line:{line_num} tag:data | Transfer Time = %.9f s\\n", _capc_dt1 - _capc_dt0);\n')
                i=next_idx; continue
            if re.search(r'#\s*pragma\s+omp\s+target\s+exit\s+data\b',logical,re.I):
                names=[prof_spec_var_name(x) for c in prof_parse_map_clauses(logical) for x in c['items']]
                if prof_is_cleanup_only_target_exit(logical): instrumented.extend(physical)
                else:
                    instrumented.append(f'  _capc_ensure_gpu_init({reg_id}, {line_num});\n  _capc_dt0 = omp_get_wtime();\n'); instrumented.extend(physical)
                    instrumented.append('  #pragma omp taskwait\n  _capc_dt1 = omp_get_wtime();\n')
                    instrumented.append(f'  printf("[PROFILER] transfer region:{reg_id} line:{line_num} tag:data | Transfer Time = %.9f s\\n", _capc_dt1 - _capc_dt0);\n')
                for x in names:
                    if x: persistent_names.discard(x)
                i=next_idx; continue
            if re.search(r'#\s*pragma\s+omp\s+target\s+update\b',logical,re.I):
                instrumented.append(f'  _capc_ensure_gpu_init({reg_id}, {line_num});\n  _capc_dt0 = omp_get_wtime();\n'); instrumented.extend(physical)
                instrumented.append('  #pragma omp taskwait\n  _capc_dt1 = omp_get_wtime();\n')
                instrumented.append(f'  printf("[PROFILER] transfer region:{reg_id} line:{line_num} tag:data | Transfer Time = %.9f s\\n", _capc_dt1 - _capc_dt0);\n')
                i=next_idx; continue
            if re.search(r'#\s*pragma\s+omp\s+target\s+data\b',logical,re.I):
                h2d,d2h,all_specs,_=prof_transfer_sets_from_target_maps(logical)
                instrumented.append(f'  _capc_ensure_gpu_init({reg_id}, {line_num});\n')
                if all_specs: instrumented.append('  #pragma omp target enter data map(alloc:'+", ".join(all_specs)+')\n  #pragma omp taskwait\n')
                emit_transfer(reg_id,line_num,'to',h2d,'target_data_to')
                pending_data={'names':{prof_spec_var_name(x) for x in all_specs if prof_spec_var_name(x)},'all_specs':all_specs,'d2h_specs':d2h,'line_num':line_num,'reg_id':reg_id}
                i=next_idx; continue
            if prof_is_target_compute_directive(logical):
                end_idx=prof_consume_statement(lines,next_idx); body=''.join(lines[next_idx:end_idx])
                names,inferred=prof_infer_region_array_specs(logical,body,declared_specs); reads,writes=prof_classify_array_accesses(body,names)
                eh,ed,es,_=prof_transfer_sets_from_target_maps(logical); en={prof_spec_var_name(x) for x in es if prof_spec_var_name(x)}
                wn=pending_data['names'] if pending_data else set(); resident=persistent_names|wn
                implicit=[x for x in names if x not in resident and x not in en]
                ih=prof_specs_for_names([x for x in implicit if x in reads],declared_specs,inferred); idh=prof_specs_for_names([x for x in implicit if x in writes],declared_specs,inferred)
                h2d=prof_unique_preserve_order(eh+ih); d2h=prof_unique_preserve_order(ed+idh); all_specs=prof_unique_preserve_order(inferred+es)
                temp=[x for x in all_specs if prof_spec_var_name(x) not in resident]
                instrumented.append(f'  _capc_ensure_gpu_init({reg_id}, {line_num});\n')
                if temp: instrumented.append('  #pragma omp target enter data map(alloc:'+", ".join(temp)+')\n  #pragma omp taskwait\n')
                emit_transfer(reg_id,line_num,'to',h2d,'implicit_to')
                instrumented.append('  _capc_k0 = omp_get_wtime();\n  '+prof_rewrite_target_as_alloc(logical,all_specs)+'\n'); instrumented.extend(lines[next_idx:end_idx])
                instrumented.append('  #pragma omp taskwait\n  _capc_k1 = omp_get_wtime();\n')
                instrumented.append(f'  printf("[PROFILER] kernel region:{reg_id} line:{line_num} | Kernel Execution Time = %.9f s\\n", _capc_k1 - _capc_k0);\n')
                emit_transfer(reg_id,line_num,'from',d2h,'implicit_from')
                if pending_data:
                    seen={re.sub(r'\s+','',x) for x in d2h}; extra=[x for x in pending_data['d2h_specs'] if re.sub(r'\s+','',x) not in seen]
                    emit_transfer(pending_data['reg_id'],pending_data['line_num'],'from',extra,'target_data_from')
                if temp: instrumented.append('  #pragma omp target exit data map(delete:'+", ".join(temp)+')\n  #pragma omp taskwait\n')
                if pending_data and pending_data['all_specs']: instrumented.append('  #pragma omp target exit data map(delete:'+", ".join(pending_data['all_specs'])+')\n  #pragma omp taskwait\n')
                pending_data=None; i=end_idx; continue
        instrumented.append(line); i+=1
    with open(temp_path,'w') as f: f.writelines(instrumented)


def prof_compile_openmp_program(
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


def prof_run_executable(exec_path):
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


def prof_process_profiler_output(
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


def prof_print_results(regions):
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


def prof_main():
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

    regions = prof_parse_regions(
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
        prof_instrument_openmp_source(
            source_path,
            temp_source_path,
            regions,
        )

        prof_compile_openmp_program(
            temp_source_path,
            exec_path,
            gpu_arch=args.gpu,
        )

        (
            stdout_str,
            stderr_str,
            returncode,
        ) = prof_run_executable(
            exec_path
        )

        prof_process_profiler_output(
            stdout_str,
            stderr_str,
            returncode,
            regions,
        )

        prof_print_results(
            regions
        )

    finally:
        if os.path.exists(
            temp_source_path
        ):
            os.remove(
                temp_source_path
            )




# =============================================================================
# FINALIZED OPENMP 4.5 STANDALONE/ISOLATED ENGINE
# =============================================================================
#!/usr/bin/env python3

import sys
import re
import os
import shutil
import subprocess
from dataclasses import dataclass
from typing import List, Optional


# =============================================================================
# Data structures
# =============================================================================

@dataclass
class gen_FunctionInfo:
    name: str
    start: int
    open_brace: int
    close_brace: int
    signature: str

    @property
    def body_start(self):
        return self.open_brace + 1

    @property
    def body_end(self):
        return self.close_brace


@dataclass
class gen_RegionInfo:
    region_id: str
    start: int
    end: int
    begin_line: str
    body_code: str
    end_line: str
    full_block: str
    function: gen_FunctionInfo


# =============================================================================
# C-source scanning helpers
# =============================================================================

def gen_mask_comments_and_strings(code: str) -> str:
    """
    Return a same-length copy of code in which comments, string literals and
    character literals are replaced with spaces. Newlines are preserved.

    This makes brace matching/function detection substantially safer than
    counting raw braces.
    """
    out = list(code)
    i = 0
    n = len(code)

    while i < n:
        # // comment
        if i + 1 < n and code[i] == "/" and code[i + 1] == "/":
            j = i
            while j < n and code[j] != "\n":
                out[j] = " "
                j += 1
            i = j
            continue

        # /* ... */ comment
        if i + 1 < n and code[i] == "/" and code[i + 1] == "*":
            out[i] = out[i + 1] = " "
            j = i + 2
            while j + 1 < n and not (code[j] == "*" and code[j + 1] == "/"):
                if code[j] != "\n":
                    out[j] = " "
                j += 1
            if j + 1 < n:
                out[j] = out[j + 1] = " "
                j += 2
            i = j
            continue

        # String / character literal
        if code[i] in ('"', "'"):
            quote = code[i]
            if code[i] != "\n":
                out[i] = " "
            j = i + 1

            while j < n:
                if code[j] == "\\":
                    if code[j] != "\n":
                        out[j] = " "
                    if j + 1 < n:
                        if code[j + 1] != "\n":
                            out[j + 1] = " "
                        j += 2
                        continue

                if code[j] == quote:
                    out[j] = " "
                    j += 1
                    break

                if code[j] != "\n":
                    out[j] = " "
                j += 1

            i = j
            continue

        i += 1

    return "".join(out)


def gen_find_matching_brace(masked_code: str, opening_brace_pos: int) -> int:
    if opening_brace_pos < 0 or masked_code[opening_brace_pos] != "{":
        raise ValueError("find_matching_brace() was not given an opening brace")

    depth = 0

    for pos in range(opening_brace_pos, len(masked_code)):
        ch = masked_code[pos]

        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return pos

    raise ValueError(
        f"Unmatched opening brace at source offset {opening_brace_pos}"
    )


def gen_find_functions(content: str) -> List[gen_FunctionInfo]:
    """
    Find ordinary C function definitions and bound them using matching braces.
    """
    masked = gen_mask_comments_and_strings(content)

    func_re = re.compile(
        r"(?m)^[ \t]*"
        r"(?P<prefix>(?:[A-Za-z_][A-Za-z0-9_]*[ \t\r\n\*]+)+?)"
        r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)"
        r"[ \t\r\n]*\("
        r"(?P<args>[^;{}]*?)"
        r"\)[ \t\r\n]*\{"
    )

    functions = []
    seen_ranges = set()

    for match in func_re.finditer(masked):
        name = match.group("name")

        if name in {"if", "for", "while", "switch"}:
            continue

        open_brace = masked.find("{", match.start(), match.end())
        if open_brace < 0:
            continue

        try:
            close_brace = gen_find_matching_brace(masked, open_brace)
        except ValueError:
            continue

        key = (match.start(), close_brace)
        if key in seen_ranges:
            continue

        seen_ranges.add(key)

        functions.append(
            gen_FunctionInfo(
                name=name,
                start=match.start(),
                open_brace=open_brace,
                close_brace=close_brace,
                signature=content[match.start():open_brace].strip(),
            )
        )

    functions.sort(key=lambda f: f.start)
    return functions


def gen_enclosing_function(
    functions: List[gen_FunctionInfo],
    start: int,
    end: int,
) -> Optional[gen_FunctionInfo]:
    candidates = [
        function
        for function in functions
        if function.body_start <= start and end <= function.body_end
    ]

    if not candidates:
        return None

    return min(
        candidates,
        key=lambda f: f.close_brace - f.start,
    )


# =============================================================================
# OpenMP map-clause helpers
# =============================================================================

def gen_split_clause_items(text: str) -> List[str]:
    """
    Split an OpenMP map/update list at top-level commas.
    """
    items = []
    current = []
    square_depth = 0
    paren_depth = 0

    for ch in text:
        if ch == "[":
            square_depth += 1
        elif ch == "]":
            square_depth = max(0, square_depth - 1)
        elif ch == "(":
            paren_depth += 1
        elif ch == ")":
            paren_depth = max(0, paren_depth - 1)

        if ch == "," and square_depth == 0 and paren_depth == 0:
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


def gen_extract_map_clause_payloads(code: str) -> List[str]:
    """
    Return the payload from every OpenMP map(...) clause.

    Examples:
        map(to:A[0:N], B[0:N])      -> "A[0:N], B[0:N]"
        map(alloc:C[0:N])           -> "C[0:N]"
        map(tofrom:A[0:N])          -> "A[0:N]"
    """
    payloads = []

    map_re = re.compile(
        r"\bmap\s*\(\s*"
        r"(?:(?:always\s*,\s*)?"
        r"(?:tofrom|to|from|alloc|release|delete)\s*:)?"
        r"([^)]+)\)",
        re.IGNORECASE,
    )

    for match in map_re.finditer(code):
        payloads.append(match.group(1))

    return payloads


def gen_get_declared_array_specs(full_code: str):
    specs={}
    type_re=re.compile(r"^\s*(?:(?:static|extern|const|volatile|register|auto)\s+)*(?:(?:signed|unsigned)\s+)?(?:(?:long\s+long|long|short)\s+)?(?:long\s+double|double|float|int|char|size_t)\s+(.+?)\s*;\s*$")
    for statement in full_code.split(';'):
        line=re.sub(r"//.*$","",statement).strip()
        if not line or line.startswith('#'): continue
        m=type_re.match(line+';')
        if not m: continue
        for decl in gen_split_clause_items(m.group(1)):
            decl=decl.split('=',1)[0].strip()
            dm=re.match(r"(?:\*\s*)*([A-Za-z_][A-Za-z0-9_]*)\s*((?:\[[^\]]+\]\s*)+)$",decl)
            if not dm: continue
            name=dm.group(1); dims=re.findall(r"\[([^\]]+)\]",dm.group(2))
            if dims: specs.setdefault(name,name+''.join(f"[0:{d.strip()}]" for d in dims))
    return specs


def gen_get_array_bounds_map(full_code: str):
    bounds=gen_get_declared_array_specs(full_code)
    for payload in gen_extract_map_clause_payloads(full_code):
        for item in gen_split_clause_items(payload):
            m=re.match(r"^([A-Za-z_][A-Za-z0-9_]*)",item)
            if m and '[' in item: bounds[m.group(1)]=item.strip()
    update_re=re.compile(r"#\s*pragma\s+omp\s+target\s+update[^\n]*?\b(?:to|from)\s*\(([^)]*)\)",re.I)
    for match in update_re.finditer(full_code):
        for item in gen_split_clause_items(match.group(1)):
            m=re.match(r"^([A-Za-z_][A-Za-z0-9_]*)",item)
            if m and '[' in item: bounds[m.group(1)]=item.strip()
    return bounds


def gen_get_target_region_array_specs(target_block: str, bounds_map):
    names=[]
    for payload in gen_extract_map_clause_payloads(target_block):
        for item in gen_split_clause_items(payload):
            m=re.match(r"^([A-Za-z_][A-Za-z0-9_]*)",item)
            if m and m.group(1) in bounds_map and m.group(1) not in names: names.append(m.group(1))
    body=gen_strip_openmp_pragmas(target_block)
    for name in re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\[",body):
        if name in bounds_map and name not in names: names.append(name)
    return [bounds_map[n] for n in names],names


def gen_classify_target_array_accesses(
    target_body: str,
    target_vars: List[str],
):
    """
    Classify target arrays by actual computational use.

    Returns:
        reads  -> pre-region value required by target => H2D
        writes -> target modifies/produces value       => D2H

    Rules:
        read-only   -> H2D only
        write-only  -> D2H only
        read-write  -> H2D + D2H

    OpenMP pragmas themselves are ignored during classification so that
    map(...) clauses do not look like computational reads.
    """
    # Remove complete OpenMP pragma groups, including backslash-continued
    # continuation lines.  Looking only for physical lines beginning with '#'
    # is insufficient because a continuation line such as
    #     map(alloc:A[0:N], B[0:N], C[0:N])
    # would otherwise be mistaken for computational reads.
    computational_code = gen_strip_openmp_pragmas(target_body)
    computational_lines = [
        line
        for line in computational_code.splitlines()
        if not line.lstrip().startswith("#")
    ]

    code = gen_mask_comments_and_strings("\n".join(computational_lines))

    reads = []
    writes = []

    for var in target_vars:
        saw_read = False
        saw_write = False

        pattern = re.compile(
            r"\b"
            + re.escape(var)
            + r"\s*(?:\[[^\]]*\]\s*)+"
        )

        for match in pattern.finditer(code):
            before = code[max(0, match.start() - 4):match.start()]
            after = code[match.end():match.end() + 8]

            # Prefix/postfix increment/decrement -> read + write
            if (
                re.search(r"(?:\+\+|--)\s*$", before)
                or re.match(r"\s*(?:\+\+|--)", after)
            ):
                saw_read = True
                saw_write = True
                continue

            op = re.match(
                r"\s*(<<=|>>=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|=)",
                after,
            )

            if op:
                saw_write = True

                # Compound assignment requires old value.
                if op.group(1) != "=":
                    saw_read = True
            else:
                saw_read = True

        if saw_read:
            reads.append(var)

        if saw_write:
            writes.append(var)

    return reads, writes


def gen_specs_for_vars(var_names: List[str], bounds_map):
    return [bounds_map.get(var, var) for var in var_names]


# =============================================================================
# OpenMP pragma manipulation
# =============================================================================

def gen_strip_openmp_pragmas(code: str) -> str:
    """
    Remove OpenMP pragmas from setup/prerequisite code.

    The setup is deliberately host-only so earlier OpenMP target regions,
    target data directives, or CPU OpenMP pragmas cannot contaminate the
    measured target region.

    Backslash-continued pragma lines are removed as a unit.
    """
    out = []
    skipping_continuation = False

    for line in code.splitlines():
        stripped = line.lstrip()

        if skipping_continuation:
            skipping_continuation = line.rstrip().endswith("\\")
            continue

        if re.match(
            r"^#\s*pragma\s+omp\b",
            stripped,
            re.IGNORECASE,
        ):
            skipping_continuation = line.rstrip().endswith("\\")
            continue

        out.append(line)

    return "\n".join(out)


def gen_strip_map_clauses_from_pragma(pragma_text: str) -> str:
    """
    Remove map(...) clauses from one OpenMP pragma string while preserving all
    other clauses.
    """
    out = []
    i = 0
    n = len(pragma_text)

    while i < n:
        match = re.search(
            r"\bmap\s*\(",
            pragma_text[i:],
            re.IGNORECASE,
        )

        if not match:
            out.append(pragma_text[i:])
            break

        start = i + match.start()
        open_paren = i + match.end() - 1

        out.append(pragma_text[i:start])

        depth = 0
        pos = open_paren

        while pos < n:
            if pragma_text[pos] == "(":
                depth += 1
            elif pragma_text[pos] == ")":
                depth -= 1
                if depth == 0:
                    pos += 1
                    break
            pos += 1

        i = pos

    cleaned = "".join(out)

    # Tidy whitespace introduced by clause removal.
    cleaned = re.sub(r"[ \t]+", " ", cleaned)
    cleaned = cleaned.replace(" \n", "\n")

    return cleaned.strip()


def gen_rewrite_target_mapping_for_standalone(target_block: str, target_specs: List[str]) -> str:
    lines=target_block.splitlines(); groups=[]; i=0
    while i<len(lines):
        line=lines[i]
        if re.match(r"^\s*#\s*pragma\s+omp\b",line,re.I):
            g=[line]; i+=1
            while g[-1].rstrip().endswith('\\') and i<len(lines): g.append(lines[i]); i+=1
            groups.append(('pragma',g))
        else: groups.append(('normal',[line])); i+=1
    out=[]; compute_done=False
    for kind,g in groups:
        if kind!='pragma': out.extend(g); continue
        parts=[]
        for ph in g:
            part=ph.rstrip(); part=part[:-1].rstrip() if part.endswith('\\') else part; parts.append(part.strip())
        logical=re.sub(r"\s+"," "," ".join(parts)).strip()
        is_data=bool(re.search(r"#\s*pragma\s+omp\s+target\s+data\b",logical,re.I))
        is_compute=bool(re.search(r"#\s*pragma\s+omp\s+target\b",logical,re.I) and not re.search(r"#\s*pragma\s+omp\s+target\s+(?:enter\s+data|exit\s+data|update|data\b)",logical,re.I))
        if is_data or (is_compute and not compute_done):
            clean=gen_strip_map_clauses_from_pragma(logical)
            if target_specs: clean+=' map(alloc:'+", ".join(target_specs)+')'
            out.append(clean)
            if is_compute: compute_done=True
        else: out.extend(g)
    return '\n'.join(out)



# =============================================================================
# Segment cleanup
# =============================================================================

def gen_line_brace_delta(line: str) -> int:
    masked = gen_mask_comments_and_strings(line)
    return masked.count("{") - masked.count("}")


def gen_block_is_unclosed_from_line(
    start_idx: int,
    lines: List[str],
) -> bool:
    """
    True when a control block opened at this point is not closed within the
    current prefix segment.
    """
    joined = "\n".join(lines[start_idx:])
    masked = gen_mask_comments_and_strings(joined)

    depth = 0
    opened = False

    for ch in masked:
        if ch == "{":
            depth += 1
            opened = True
        elif ch == "}":
            depth -= 1
            if opened and depth <= 0:
                return False

    return opened and depth > 0


def gen_sanitize_c_segment(code_str: str) -> str:
    """
    Clean a function prefix so it can be replayed inside the synthetic main().

    Key rule: if the target lies inside an outer control block, the prefix ends
    before that block's closing brace.  We remove ONLY the unmatched opening
    control construct(s).  We never consume some earlier nested loop's closing
    brace.  This preserves complete prerequisite loops from previous regions.
    """
    lines = code_str.splitlines()

    # ------------------------------------------------------------------
    # Find physical line numbers that contain opening braces which remain
    # unmatched at the end of this prefix.  Those are precisely the outer
    # scopes containing the target.
    # ------------------------------------------------------------------
    brace_stack = []
    masked_lines = [gen_mask_comments_and_strings(line) for line in lines]

    for idx, masked in enumerate(masked_lines):
        for ch in masked:
            if ch == "{":
                brace_stack.append(idx)
            elif ch == "}" and brace_stack:
                brace_stack.pop()

    unmatched_open_lines = set(brace_stack)

    # Map an unmatched standalone "{" line to a preceding control-header line
    # when code is written as:
    #     for (...)
    #     {
    header_lines_to_remove = set()
    brace_lines_to_remove = set()

    control_re = re.compile(r"^\s*(for|while|if|switch|do)\b")

    for open_idx in sorted(unmatched_open_lines):
        stripped = lines[open_idx].strip()

        if control_re.match(stripped) and "{" in stripped:
            header_lines_to_remove.add(open_idx)
            continue

        if stripped == "{":
            prev = open_idx - 1
            while prev >= 0 and not lines[prev].strip():
                prev -= 1
            if prev >= 0 and control_re.match(lines[prev].strip()):
                header_lines_to_remove.add(prev)
                brace_lines_to_remove.add(open_idx)
                continue

        # An unmatched function/local compound scope that is not a recognizable
        # control construct is safer to drop only at its opening brace.  This
        # prevents an unterminated block in generated main().
        brace_lines_to_remove.add(open_idx)

    clean = []
    pp_depth = 0

    for idx, original in enumerate(lines):
        line = original
        stripped = line.strip()

        if "profitability_region" in stripped:
            continue

        if idx in header_lines_to_remove:
            # If header and opening brace share one physical line, remove both.
            continue

        if idx in brace_lines_to_remove:
            continue

        # Orphaned break/continue cannot be replayed safely once the containing
        # outer loop has been removed.
        if re.match(r"^\s*(break|continue)\s*;\s*$", stripped):
            clean.append(
                f"// {stripped}  /* skipped: possibly orphaned in standalone replay */"
            )
            continue

        # Preserve balanced preprocessor structure.
        if re.match(r"^\s*#\s*(if|ifdef|ifndef)\b", stripped):
            pp_depth += 1
            clean.append(line)
        elif re.match(r"^\s*#\s*endif\b", stripped):
            if pp_depth > 0:
                pp_depth -= 1
                clean.append(line)
            else:
                clean.append(f"// {line}  /* skipped orphaned #endif */")
        elif re.match(r"^\s*#\s*(else|elif)\b", stripped):
            if pp_depth > 0:
                clean.append(line)
            else:
                clean.append(
                    f"// {line}  /* skipped orphaned preprocessor directive */"
                )
        else:
            clean.append(line)

    while pp_depth > 0:
        clean.append("#endif /* auto-closed by standalone generator */")
        pp_depth -= 1

    return "\n".join(clean)

def gen_remove_function_from_source(
    content: str,
    func: gen_FunctionInfo,
) -> str:
    return (
        content[:func.start]
        + content[func.close_brace + 1:]
    )


def gen_strip_capc_markers(code: str) -> str:
    return "\n".join(
        line
        for line in code.splitlines()
        if "profitability_region" not in line
    )



# =============================================================================
# Prior-CAPC replay policy
# =============================================================================

def gen_extract_safe_inter_region_scalar_setup(gap_code: str) -> str:
    """
    Keep only cheap scalar setup statements from host code that originally
    appeared between CAPC regions.

    This prevents verification/output code belonging to skipped predecessor
    regions from being replayed into later standalone programs, while retaining
    conservative scalar resets such as:

        dt = 0.0;
        sum = 0.0;

    No output-validation code is added here.
    """
    code = gen_strip_openmp_pragmas(gap_code)
    safe = []

    for original in code.splitlines():
        stripped = original.strip()

        if not stripped:
            continue

        if stripped.startswith("#"):
            continue

        if re.match(r"^(for|while|if|switch|do|else)\b", stripped):
            continue

        if re.search(
            r"\b(?:printf|fprintf|sprintf|snprintf|puts|putchar)\s*\(",
            stripped,
        ):
            continue

        if "[" in stripped or "]" in stripped:
            continue

        if "++" in stripped or "--" in stripped:
            continue

        masked = gen_mask_comments_and_strings(stripped)

        if re.search(r"\b[A-Za-z_][A-Za-z0-9_]*\s*\(", masked):
            continue

        if re.match(
            r"^[A-Za-z_][A-Za-z0-9_]*\s*=\s*[^;]+;\s*$",
            stripped,
        ):
            safe.append(stripped)

    if not safe:
        return ""

    return (
        "/* Safe scalar setup retained from inter-region host code. */\n"
        + "\n".join(safe)
    )


def gen_process_prior_capc_regions(prefix_code: str, bounds_map):
    """
    Build cheap host prerequisite replay for a later standalone region.

    Replay policy:
      1. Preserve ordinary host code before the FIRST earlier CAPC region.
      2. Replay earlier pure producer/initializer CAPC regions on the host.
      3. Skip earlier CAPC compute regions that read arrays.
      4. Do NOT replay all host code between earlier CAPC regions.  Such gaps
         frequently contain verification/output/target-update code whose
         producer was just skipped.  Only conservative scalar assignments are
         retained from those gaps.

    This fixes orphaned break/continue statements and stale verification code in
    later standalone programs, without changing timing/reporting behavior.

    Returns:
        transformed_prefix,
        skipped_output_vars,
        replayed_initializer_count,
        skipped_compute_count
    """
    lines = prefix_code.splitlines()

    begin_re = re.compile(
        r"^\s*#\s*pragma\s+capc\s+profitability_region\s+begin\b",
        re.IGNORECASE,
    )
    end_re = re.compile(
        r"^\s*#\s*pragma\s+capc\s+profitability_region\s+end\b",
        re.IGNORECASE,
    )

    leading = []
    region_blocks = []
    gaps = []

    i = 0

    while i < len(lines) and not begin_re.match(lines[i]):
        leading.append(lines[i])
        i += 1

    while i < len(lines):
        if not begin_re.match(lines[i]):
            gap = []
            while i < len(lines) and not begin_re.match(lines[i]):
                gap.append(lines[i])
                i += 1
            gaps.append(gap)
            continue

        block = [lines[i]]
        i += 1

        while i < len(lines):
            block.append(lines[i])
            if end_re.match(lines[i]):
                i += 1
                break
            i += 1

        region_blocks.append(block)

        gap = []
        while i < len(lines) and not begin_re.match(lines[i]):
            gap.append(lines[i])
            i += 1
        gaps.append(gap)

    out = list(leading)
    skipped_outputs = []
    replayed_initializers = 0
    skipped_compute = 0

    for block_index, block_lines in enumerate(region_blocks):
        body_lines = [
            line
            for line in block_lines
            if not begin_re.match(line) and not end_re.match(line)
        ]

        body = "\n".join(body_lines)
        full_block = "\n".join(block_lines)

        _, var_names = gen_get_target_region_array_specs(
            full_block,
            bounds_map,
        )

        reads, writes = gen_classify_target_array_accesses(
            body,
            var_names,
        )

        if writes and not reads:
            replay = gen_strip_openmp_pragmas(body)
            replay = re.sub(
                r'\bprintf\s*\(\s*""\s*\)\s*;',
                "",
                replay,
            )

            out.append(
                "/* Earlier CAPC producer/initializer replayed on host. */"
            )
            out.extend(replay.splitlines())
            replayed_initializers += 1

        else:
            out.append(
                "/* Earlier CAPC compute region omitted from standalone host replay. */"
            )
            skipped_compute += 1

            for var in writes:
                if var not in skipped_outputs:
                    skipped_outputs.append(var)

        if block_index < len(gaps):
            safe_gap = gen_extract_safe_inter_region_scalar_setup(
                "\n".join(gaps[block_index])
            )

            if safe_gap:
                out.extend(safe_gap.splitlines())

    return (
        "\n".join(out),
        skipped_outputs,
        replayed_initializers,
        skipped_compute,
    )


def gen_zero_initialization_for_array_spec(spec: str, serial: int) -> str:
    """
    Generate deterministic O(number-of-elements) host zero initialization for
    a mapped array section such as:
        A[0:N]
        M[0:N][0:N]

    This is used only when a required target input was produced by an earlier
    expensive CAPC region that we deliberately skipped.

    If the section cannot be parsed as OpenMP lower:length dimensions, return
    an empty string and let the caller emit a warning comment instead.
    """
    m = re.match(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*(.*)$", spec)
    if not m:
        return ""

    var = m.group(1)
    suffix = m.group(2)
    dims = re.findall(r"\[\s*([^:\]]+)\s*:\s*([^\]]+)\]", suffix)

    if not dims:
        return ""

    idx_names = [f"__capc_z{serial}_{d}" for d in range(len(dims))]
    indent = ""
    lines = []

    for d, ((lower, length), idx) in enumerate(zip(dims, idx_names)):
        lines.append(
            indent
            + f"for (size_t {idx} = 0; {idx} < (size_t)({length.strip()}); ++{idx}) {{"
        )
        indent += "    "

    access = var + "".join(
        f"[({lower.strip()}) + {idx}]"
        for (lower, _), idx in zip(dims, idx_names)
    )
    lines.append(indent + f"{access} = 0;")

    for _ in dims:
        indent = indent[:-4]
        lines.append(indent + "}")

    return "\n".join(lines)


def gen_build_synthetic_input_initialization(
    skipped_outputs: List[str],
    target_read_vars: List[str],
    bounds_map,
):
    """
    Initialize only target inputs whose latest prerequisite producer was an
    omitted earlier CAPC compute region.

    Returns generated C code and list of vars that could not be synthesized.
    """
    needed = [
        var for var in target_read_vars
        if var in skipped_outputs
    ]

    blocks = []
    unresolved = []

    for serial, var in enumerate(needed):
        spec = bounds_map.get(var, var)
        code = gen_zero_initialization_for_array_spec(spec, serial)

        if code:
            blocks.append(
                f"/* Synthetic valid input for '{var}': prior CAPC producer was skipped. */\n"
                + code
            )
        else:
            unresolved.append(var)

    return "\n".join(blocks), unresolved

# =============================================================================
# Parsing
# =============================================================================

def gen_parse_c_file(file_path: str):
    with open(file_path, "r") as f:
        content = f.read()

    functions = gen_find_functions(content)

    if not functions:
        raise ValueError(
            "No C function definitions could be found."
        )

    main_func = next(
        (
            function
            for function in functions
            if function.name == "main"
        ),
        None,
    )

    if main_func is None:
        raise ValueError(
            "Could not locate main() function in the input file."
        )

    bounds_map = gen_get_array_bounds_map(content)

    region_pattern = re.compile(
        r"(#pragma\s+capc\s+profitability_region\s+begin[^\n]*\n)"
        r"(.*?)"
        r"(#pragma\s+capc\s+profitability_region\s+end[^\n]*)",
        re.DOTALL | re.IGNORECASE,
    )

    region_matches = list(region_pattern.finditer(content))

    if not region_matches:
        raise ValueError(
            "No '#pragma capc profitability_region begin/end' "
            "markers found in file."
        )

    regions = []

    for idx, match in enumerate(region_matches, start=1):
        function = gen_enclosing_function(
            functions,
            match.start(),
            match.end(),
        )

        if function is None:
            raise ValueError(
                f"Profitability region {idx} is not contained "
                f"in a recognized function."
            )

        begin_line = match.group(1).strip()
        body_code = match.group(2).strip()
        end_line = match.group(3).strip()

        id_match = re.search(
            r"begin\s*(?:\(\s*([A-Za-z0-9_]+)\s*\)"
            r"|\s+([A-Za-z0-9_]+))",
            begin_line,
            re.IGNORECASE,
        )

        if id_match:
            region_id = (
                id_match.group(1)
                or id_match.group(2)
            )
        else:
            region_id = str(idx)

        full_block = (
            f"{begin_line}\n"
            f"{body_code}\n"
            f"{end_line}"
        )

        regions.append(
            gen_RegionInfo(
                region_id=region_id,
                start=match.start(),
                end=match.end(),
                begin_line=begin_line,
                body_code=body_code,
                end_line=end_line,
                full_block=full_block,
                function=function,
            )
        )

    return (
        content,
        functions,
        main_func,
        regions,
        bounds_map,
    )


# =============================================================================
# Standalone generation
#
# FINAL ISOLATED-TIME DEFINITION:
#
#   Isolated Time
#     = OpenMP GPU initialization
#     + required H2D
#     + target kernel
#     + required D2H
#
# Input/output decisions are based on computational access in the target region:
#
#   read-only   -> H2D
#   write-only  -> D2H
#   read-write  -> H2D + D2H
#
# Scalar reductions are handled by the OpenMP target construct itself and are
# therefore naturally included in kernel time rather than an explicit array D2H.
#
# Earlier CAPC pure-producer/initializer regions may be replayed on the host.
# Earlier CAPC regions that read arrays are treated as compute regions and are
# not serially replayed; if their outputs are required by the target, cheap
# deterministic host values are synthesized for those mapped array sections.
# =============================================================================

def gen_clean_directory(output_dir: str):
    if os.path.exists(output_dir):
        print(
            f"Cleaning previous standalone region files "
            f"in '{output_dir}'..."
        )
        shutil.rmtree(output_dir)

    os.makedirs(output_dir, exist_ok=True)


def gen_get_function_prefix(
    content: str,
    target: gen_RegionInfo,
) -> str:
    """
    Return all source in target's enclosing function before the CAPC target.
    """
    return content[
        target.function.body_start:target.start
    ]


def gen_generate_standalone_files(
    content: str,
    functions: List[gen_FunctionInfo],
    main_func: gen_FunctionInfo,
    regions: List[gen_RegionInfo],
    bounds_map,
    output_dir: str = "standalone_regions",
):
    gen_clean_directory(output_dir)

    # Keep globals, includes, declarations, prototypes and helper functions,
    # but remove original main because every standalone file gets a synthetic
    # main().
    support_source = gen_remove_function_from_source(
        content,
        main_func,
    )

    # A standalone file must contain exactly ONE CAPC marker pair: the target.
    support_source = gen_strip_capc_markers(
        support_source
    )

    # Any helper called by setup must execute on the host only.
    support_source = gen_strip_openmp_pragmas(
        support_source
    )

    generated_files = []

    for target in regions:
        filename = os.path.join(
            output_dir,
            f"region_{target.region_id}_standalone.c",
        )

        # ---------------------------------------------------------------------
        # Target data dependence (needed before prerequisite-replay policy)
        # ---------------------------------------------------------------------
        array_specs, target_var_names = (
            gen_get_target_region_array_specs(
                target.full_block,
                bounds_map,
            )
        )

        read_vars, write_vars = (
            gen_classify_target_array_accesses(
                target.body_code,
                target_var_names,
            )
        )

        # ---------------------------------------------------------------------
        # Host-only prerequisite replay
        # ---------------------------------------------------------------------
        prefix_raw = gen_get_function_prefix(
            content,
            target,
        )

        (
            prefix_policy,
            skipped_prior_outputs,
            replayed_initializer_count,
            skipped_compute_count,
        ) = gen_process_prior_capc_regions(
            prefix_raw,
            bounds_map,
        )

        prefix_clean = gen_sanitize_c_segment(
            prefix_policy
        ).strip()

        # Remove all remaining OpenMP pragmas from setup/prerequisites so no
        # offload, data mapping, or CPU OpenMP execution contaminates timing.
        prefix_clean = gen_strip_openmp_pragmas(
            prefix_clean
        ).strip()

        # If an input needed by the target was produced by an earlier expensive
        # CAPC compute region that we intentionally omitted, create a cheap,
        # deterministic host value for that array section.
        synthetic_init, unresolved_synthetic = (
            gen_build_synthetic_input_initialization(
                skipped_prior_outputs,
                read_vars,
                bounds_map,
            )
        )

        h2d_specs = gen_specs_for_vars(
            read_vars,
            bounds_map,
        )

        d2h_specs = gen_specs_for_vars(
            write_vars,
            bounds_map,
        )

        h2d_str = ", ".join(h2d_specs)
        d2h_str = ", ".join(d2h_specs)
        target_specs_str = ", ".join(array_specs)

        # Rewrite target mapping so original map(to/from/tofrom:...) clauses
        # cannot perform data movement inside kernel timing.
        target_code = gen_rewrite_target_mapping_for_standalone(
            target.full_block,
            array_specs,
        )

        with open(filename, "w") as f:
            f.write("#define _GNU_SOURCE\n")
            f.write("#define _POSIX_C_SOURCE 199309L\n")
            f.write("#include <time.h>\n")
            f.write("#include <stdio.h>\n")
            f.write("#include <stdlib.h>\n")
            f.write("#include <omp.h>\n\n")

            f.write(
                "/* ============================================================\n"
            )
            f.write(
                " * Original source support code (original main removed)\n"
            )
            f.write(
                " * ============================================================ */\n"
            )
            f.write(
                support_source.rstrip()
                + "\n\n"
            )

            f.write("int main(void)\n{\n")

            f.write(
                "    struct timespec "
                "__capc_t_start, __capc_t_end;\n"
            )
            f.write(
                "    double __capc_t_init = 0.0;\n"
            )
            f.write(
                "    double __capc_t_in = 0.0;\n"
            )
            f.write(
                "    double __capc_t_gpu = 0.0;\n"
            )
            f.write(
                "    double __capc_t_out = 0.0;\n\n"
            )

            f.write(
                f"    /* Target Region {target.region_id}; "
                f"original function: {target.function.name}() */\n"
            )

            # ---------------------------------------------------------------
            # Host-only setup
            # ---------------------------------------------------------------
            if prefix_clean:
                f.write(
                    "    /* === Host-only input/setup replay "
                    "(NOT timed) === */\n"
                )

                for line in prefix_clean.splitlines():
                    f.write(
                        "    "
                        + line
                        + "\n"
                    )

                f.write("\n")
            else:
                f.write(
                    "    /* No host-side prerequisite/setup code. */\n\n"
                )

            if synthetic_init:
                f.write(
                    "    /* === Synthetic initialization for inputs whose "
                    "prior expensive CAPC producer was omitted === */\n"
                )
                for line in synthetic_init.splitlines():
                    f.write("    " + line + "\n")
                f.write("\n")

            if unresolved_synthetic:
                f.write(
                    "    /* WARNING: could not synthesize deterministic "
                    "values for: "
                    + ", ".join(unresolved_synthetic)
                    + ". */\n\n"
                )

            # ---------------------------------------------------------------
            # GPU/OpenMP runtime initialization
            # ---------------------------------------------------------------
            #
            # OpenMP has no acc_init()-equivalent device initialization call.
            # omp_target_alloc() is used as a minimal explicit device-runtime
            # touch.  The first call initializes the OpenMP target runtime/device
            # context.  The tiny allocation itself is negligible relative to
            # cold runtime initialization and is immediately freed outside the
            # measured interval.
            #
            f.write(
                "    /* === GPU/OpenMP Runtime Initialization === */\n"
            )
            f.write(
                "    int __capc_device = omp_get_default_device();\n"
            )
            f.write(
                "    void *__capc_init_ptr = NULL;\n"
            )
            f.write(
                "    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);\n"
            )
            f.write(
                "    __capc_init_ptr = "
                "omp_target_alloc(1, __capc_device);\n"
            )
            f.write(
                "    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);\n"
            )
            f.write(
                "    __capc_t_init = "
                "(__capc_t_end.tv_sec - __capc_t_start.tv_sec) "
                "+ (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;\n"
            )
            f.write(
                "    if (__capc_init_ptr != NULL) "
                "omp_target_free(__capc_init_ptr, __capc_device);\n\n"
            )

            # ---------------------------------------------------------------
            # Allocation only
            # ---------------------------------------------------------------
            if target_specs_str:
                f.write(
                    "    /* === Device allocation only "
                    "(no data movement) === */\n"
                )
                f.write(
                    f"    #pragma omp target enter data "
                    f"map(alloc:{target_specs_str})\n"
                )
                f.write(
                    "    #pragma omp taskwait\n\n"
                )

            # ---------------------------------------------------------------
            # Required H2D
            # ---------------------------------------------------------------
            if h2d_str:
                f.write(
                    "    /* === Required Transfer In "
                    "(Host -> Device) === */\n"
                )
                f.write(
                    "    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);\n"
                )
                f.write(
                    f"    #pragma omp target update to({h2d_str})\n"
                )
                f.write(
                    "    #pragma omp taskwait\n"
                )
                f.write(
                    "    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);\n"
                )
                f.write(
                    "    __capc_t_in = "
                    "(__capc_t_end.tv_sec - __capc_t_start.tv_sec) "
                    "+ (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;\n\n"
                )
            else:
                f.write(
                    "    /* H2D skipped: target has no "
                    "read-before/write input arrays. */\n\n"
                )

            # ---------------------------------------------------------------
            # Target kernel
            # ---------------------------------------------------------------
            f.write(
                f"    /* === Isolated Kernel Timing "
                f"for Target Region {target.region_id} === */\n"
            )
            f.write(
                "    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);\n\n"
            )

            # Exactly ONE CAPC marker pair is retained here.
            for line in target_code.splitlines():
                f.write(
                    "    "
                    + line
                    + "\n"
                )

            # Handles original target nowait safely; harmless otherwise.
            f.write(
                "\n    #pragma omp taskwait\n"
            )
            f.write(
                "    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);\n"
            )
            f.write(
                "    __capc_t_gpu = "
                "(__capc_t_end.tv_sec - __capc_t_start.tv_sec) "
                "+ (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;\n\n"
            )

            # ---------------------------------------------------------------
            # Required D2H
            # ---------------------------------------------------------------
            if d2h_str:
                f.write(
                    "    /* === Required Transfer Out "
                    "(Device -> Host) === */\n"
                )
                f.write(
                    "    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);\n"
                )
                f.write(
                    f"    #pragma omp target update from({d2h_str})\n"
                )
                f.write(
                    "    #pragma omp taskwait\n"
                )
                f.write(
                    "    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);\n"
                )
                f.write(
                    "    __capc_t_out = "
                    "(__capc_t_end.tv_sec - __capc_t_start.tv_sec) "
                    "+ (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;\n\n"
                )
            else:
                f.write(
                    "    /* D2H skipped: target does not modify "
                    "any detected array. */\n\n"
                )

            # ---------------------------------------------------------------
            # Reporting
            # ---------------------------------------------------------------
            f.write(
                "    double __capc_t_total = "
                "__capc_t_init + __capc_t_in "
                "+ __capc_t_gpu + __capc_t_out;\n"
            )
            f.write(
                f'    printf("Region {target.region_id} '
                f'Execution Breakdown:\\n");\n'
            )
            f.write(
                '    printf("  - GPU Initialization : '
                '%f seconds\\n", __capc_t_init);\n'
            )
            f.write(
                '    printf("  - Transfer In  (H2D): '
                '%f seconds\\n", __capc_t_in);\n'
            )
            f.write(
                '    printf("  - Kernel Time (GPU): '
                '%f seconds\\n", __capc_t_gpu);\n'
            )
            f.write(
                '    printf("  - Transfer Out (D2H): '
                '%f seconds\\n", __capc_t_out);\n'
            )
            f.write(
                '    printf("  - Isolated Region Time: '
                '%f seconds\\n", __capc_t_total);\n\n'
            )

            # ---------------------------------------------------------------
            # Cleanup - intentionally excluded from isolated metric
            # ---------------------------------------------------------------
            if target_specs_str:
                f.write(
                    f"    #pragma omp target exit data "
                    f"map(delete:{target_specs_str})\n"
                )
                f.write(
                    "    #pragma omp taskwait\n\n"
                )

            f.write(
                "    /* Device cleanup is intentionally "
                "not part of isolated time. */\n"
            )
            f.write(
                "    return 0;\n"
            )
            f.write(
                "}\n"
            )

        print(
            f"Generated: {filename} "
            f"[enclosing function: {target.function.name}()]"
        )
        print(
            "  H2D inputs : "
            + (
                ", ".join(read_vars)
                if read_vars
                else "(none)"
            )
        )
        print(
            "  D2H outputs: "
            + (
                ", ".join(write_vars)
                if write_vars
                else "(none)"
            )
        )
        if replayed_initializer_count:
            print(
                f"  Prior CAPC producer/initializer regions replayed: "
                f"{replayed_initializer_count}"
            )
        if skipped_compute_count:
            print(
                f"  Prior CAPC compute regions skipped: "
                f"{skipped_compute_count}"
            )
        synthesized_vars = [
            var for var in read_vars if var in skipped_prior_outputs
        ]
        if synthesized_vars:
            print(
                "  Synthetic valid inputs: "
                + ", ".join(synthesized_vars)
            )
        if unresolved_synthetic:
            print(
                "  WARNING unresolved synthetic inputs: "
                + ", ".join(unresolved_synthetic)
            )

        generated_files.append(
            (
                target.region_id,
                filename,
            )
        )

    return generated_files


# =============================================================================
# Compile and run
# =============================================================================

def gen_compile_and_run_regions(
    generated_files,
    compiler: str = "nvc",
    flags=None,
):
    if flags is None:
        flags = [
            "-mp=gpu",
            "-Minfo=mp",
            "--diag_suppress",
            "declared_but_not_referenced",
        ]

    print(
        "\n"
        + "=" * 50
    )
    print(
        " COMPILING & EXECUTING STANDALONE REGIONS (OPENMP 4.5)"
    )
    print(
        "=" * 50
    )

    for target_id, c_file in generated_files:
        exe_file = os.path.splitext(c_file)[0]

        compile_cmd = (
            [compiler]
            + flags
            + [
                c_file,
                "-o",
                exe_file,
            ]
        )

        print(
            f"\n[Compiling Region {target_id}]: "
            f"{' '.join(compile_cmd)}"
        )

        comp_process = subprocess.run(
            compile_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        compiler_output = "\n".join(
            output
            for output in (
                comp_process.stdout.strip(),
                comp_process.stderr.strip(),
            )
            if output
        )

        if compiler_output:
            print(
                f"[Compiler Output]:\n{compiler_output}"
            )

        if comp_process.returncode != 0:
            print(
                f"❌ Compilation failed for Region {target_id}!"
            )
            continue

        print(
            f"[Running Region {target_id}]: {exe_file}"
        )

        try:
            run_process = subprocess.run(
                [os.path.abspath(exe_file)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=300,
            )
        except subprocess.TimeoutExpired:
            print(
                f"❌ Execution timed out for Region {target_id} "
                f"after 300 seconds."
            )
            continue

        if run_process.returncode == 0:
            print(
                f"✅ {run_process.stdout.strip()}"
            )

            if run_process.stderr.strip():
                print(
                    f"[Runtime stderr]:\n"
                    f"{run_process.stderr.strip()}"
                )
        else:
            print(
                f"❌ Execution failed for Region {target_id}!"
            )

            if run_process.stdout.strip():
                print(
                    run_process.stdout.strip()
                )

            if run_process.stderr.strip():
                print(
                    run_process.stderr.strip()
                )


# =============================================================================
# Main
# =============================================================================

def gen_main():
    if len(sys.argv) < 2:
        print(
            "Usage: python generate_standalone_omp45.py "
            "<input_benchmark.c>"
        )
        sys.exit(1)

    input_file = sys.argv[1]

    if not os.path.isfile(input_file):
        print(
            f"Error: input file not found: {input_file}"
        )
        sys.exit(1)

    try:
        (
            content,
            functions,
            main_func,
            regions,
            bounds_map,
        ) = gen_parse_c_file(input_file)

        print(
            "Detected profitability regions:"
        )

        for region in regions:
            line_no = (
                content.count(
                    "\n",
                    0,
                    region.start,
                )
                + 1
            )

            print(
                f"  Region {region.region_id}: "
                f"line {line_no}, "
                f"function {region.function.name}()"
            )

        generated = gen_generate_standalone_files(
            content,
            functions,
            main_func,
            regions,
            bounds_map,
        )

        gen_compile_and_run_regions(
            generated
        )

    except Exception as exc:
        print(
            f"Error: {exc}"
        )
        sys.exit(1)




# =============================================================================
# Combined OpenMP 4.5 orchestration
# =============================================================================

def _capc_parse_isolated_stdout(text):
    patterns = {
        "init": r"GPU Initialization\s*:\s*([0-9.eE+-]+)\s*seconds",
        "h2d": r"Transfer In\s*\(H2D\)\s*:\s*([0-9.eE+-]+)\s*seconds",
        "kernel": r"Kernel Time\s*\(GPU\)\s*:\s*([0-9.eE+-]+)\s*seconds",
        "d2h": r"Transfer Out\s*\(D2H\)\s*:\s*([0-9.eE+-]+)\s*seconds",
        "isolated": r"Isolated Region Time\s*:\s*([0-9.eE+-]+)\s*seconds",
    }
    values = {}
    for key, pat in patterns.items():
        m = re.search(pat, text)
        values[key] = float(m.group(1)) if m else None
    return values


def _capc_run_isolated_regions(generated_files, gpu_arch="cc70", timeout=300):
    results = {}

    print("\n" + "=" * 88)
    print(" COMPILING & EXECUTING STANDALONE REGIONS (OPENMP 4.5)")
    print("=" * 88)

    for target_id, c_file in generated_files:
        exe_file = os.path.splitext(c_file)[0]
        cmd = [
            "nvc",
            "-mp=gpu",
            f"-gpu={gpu_arch}",
            "-Minfo=mp",
            "--diag_suppress",
            "declared_but_not_referenced",
            c_file,
            "-o",
            exe_file,
        ]

        print(f"\n[Compiling Region {target_id}]: {' '.join(cmd)}")
        cp = subprocess.run(cmd, capture_output=True, text=True)
        compiler_output = "\n".join(x for x in (cp.stdout.strip(), cp.stderr.strip()) if x)
        if compiler_output:
            print(f"[Compiler Output]:\n{compiler_output}")

        if cp.returncode != 0:
            print(f"❌ Compilation failed for Region {target_id}!")
            results[str(target_id)] = {
                "status": "compile_failed",
                "init": None, "h2d": None, "kernel": None,
                "d2h": None, "isolated": None,
            }
            continue

        print(f"[Running Region {target_id}]: {exe_file}")
        try:
            rp = subprocess.run(
                [os.path.abspath(exe_file)],
                capture_output=True,
                text=True,
                timeout=timeout,
            )
        except subprocess.TimeoutExpired:
            print(f"❌ Execution timed out for Region {target_id} after {timeout} seconds.")
            results[str(target_id)] = {
                "status": "timeout",
                "init": None, "h2d": None, "kernel": None,
                "d2h": None, "isolated": None,
            }
            continue

        full = rp.stdout + "\n" + rp.stderr
        if rp.returncode != 0:
            print(f"❌ Execution failed for Region {target_id}!")
            if rp.stdout.strip():
                print(rp.stdout.strip())
            if rp.stderr.strip():
                print(rp.stderr.strip())
            results[str(target_id)] = {
                "status": "runtime_failed",
                "init": None, "h2d": None, "kernel": None,
                "d2h": None, "isolated": None,
            }
            continue

        vals = _capc_parse_isolated_stdout(full)
        vals["status"] = "ok" if vals["isolated"] is not None else "parse_failed"
        results[str(target_id)] = vals

        if vals["isolated"] is None:
            print(f"⚠️ Region {target_id} executed, but isolated timing could not be parsed.")
            if rp.stdout.strip():
                print(rp.stdout.strip())
        else:
            print(f"✅ Region {target_id} isolated timing captured")
            print(
                f"   Init={vals['init']:.6f}  "
                f"H2D={vals['h2d']:.6f}  "
                f"Kernel={vals['kernel']:.6f}  "
                f"D2H={vals['d2h']:.6f}  "
                f"Isolated={vals['isolated']:.6f}"
            )

    return results


def _capc_region_metrics(reg):
    count = max(reg["count"], 1)
    avg_res = reg["resident_time"] / count
    avg_recurring = (
        reg["resident_time"] + reg["recurring_transfer_time"]
    ) / count
    avg_obs = reg["init_time"] + reg["one_time_transfer_time"] + avg_recurring
    total_obs = (
        reg["init_time"]
        + reg["one_time_transfer_time"]
        + reg["recurring_transfer_time"]
        + reg["resident_time"]
    )
    return avg_res, total_obs, avg_obs


def _capc_print_combined_report(prof_regions, isolated_results):
    title = "CAPC COMBINED REGION TIMING REPORT (OPENMP 4.5)"
    header = (
        f"{'Region':<8} | {'Lines':<9} | {'Invocations':<11} | "
        f"{'Total Res(s)':<12} | {'Avg Res(s)':<11} | "
        f"{'Total Obs(s)':<12} | {'Avg Obs(s)':<11} | {'Isolated(s)':<12}"
    )
    divider = "-" * len(header)
    print("\n" + "=" * len(header))
    print(title.center(len(header)))
    print("=" * len(header))
    print(header)
    print(divider)

    total_res = 0.0
    total_obs = 0.0
    total_calls = 0

    for reg in prof_regions:
        avg_res, reg_total_obs, avg_obs = _capc_region_metrics(reg)
        iso = isolated_results.get(str(reg["id"]), {})
        iso_v = iso.get("isolated")
        iso_s = f"{iso_v:.6f}" if isinstance(iso_v, (int, float)) else "N/A"
        lines = f"{reg['begin_line']}-{reg['end_line']}"

        print(
            f"Region {reg['id']:<1} | {lines:<9} | {reg['count']:<11} | "
            f"{reg['resident_time']:<12.6f} | {avg_res:<11.6f} | "
            f"{reg_total_obs:<12.6f} | {avg_obs:<11.6f} | {iso_s:<12}"
        )

        total_res += reg["resident_time"]
        total_obs += reg_total_obs
        total_calls += reg["count"]

    print(divider)
    avg_total_res = total_res / max(total_calls, 1)
    print(
        f"{'TOTAL':<8} | {'-':<9} | {total_calls:<11} | "
        f"{total_res:<12.6f} | {avg_total_res:<11.6f} | "
        f"{total_obs:<12.6f} | {'-':<11} | {'-':<12}"
    )
    print("=" * len(header))

    print("\nIsolated breakdown:")
    iso_header = (
        f"{'Region':<8} | {'GPU Init(s)':<11} | {'H2D(s)':<11} | "
        f"{'Kernel(s)':<11} | {'D2H(s)':<11} | {'Isolated(s)':<12} | {'Status':<14}"
    )
    print("-" * len(iso_header))
    print(iso_header)
    print("-" * len(iso_header))
    for reg in prof_regions:
        x = isolated_results.get(str(reg["id"]), {})
        def fmt(v):
            return f"{v:.6f}" if isinstance(v, (int, float)) else "N/A"
        print(
            f"Region {reg['id']:<1} | {fmt(x.get('init')):<11} | "
            f"{fmt(x.get('h2d')):<11} | {fmt(x.get('kernel')):<11} | "
            f"{fmt(x.get('d2h')):<11} | {fmt(x.get('isolated')):<12} | "
            f"{x.get('status', 'missing'):<14}"
        )
    print("-" * len(iso_header))


def _capc_write_csv(path, prof_regions, isolated_results):
    import csv
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow([
            "Region", "Lines", "Invocations",
            "TotalResident", "AvgResident",
            "TotalObserved", "AvgObserved",
            "Isolated", "IsolatedGPUInit", "IsolatedH2D",
            "IsolatedKernel", "IsolatedD2H", "StandaloneStatus",
        ])
        for reg in prof_regions:
            avg_res, total_obs, avg_obs = _capc_region_metrics(reg)
            iso = isolated_results.get(str(reg["id"]), {})
            w.writerow([
                reg["id"], f"{reg['begin_line']}-{reg['end_line']}", reg["count"],
                reg["resident_time"], avg_res, total_obs, avg_obs,
                iso.get("isolated"), iso.get("init"), iso.get("h2d"),
                iso.get("kernel"), iso.get("d2h"), iso.get("status", "missing"),
            ])
    print(f"\n[*] Combined timing CSV written to: {path}")


def combined_main():
    parser = argparse.ArgumentParser(
        description=(
            "Combined CAPC OpenMP 4.5 timing annotator: Resident + Observed "
            "from original execution, plus Isolated timing from one standalone "
            "program per profitability region."
        )
    )
    parser.add_argument("source", help="OpenMP 4.5 C source file")
    parser.add_argument("--gpu", default="cc70", help="GPU architecture (default: cc70)")
    parser.add_argument("--timeout", type=int, default=300, help="Standalone execution timeout")
    parser.add_argument("--csv", default=None, help="Optional combined CSV output path")
    args = parser.parse_args()

    source_path = os.path.abspath(args.source)
    if not os.path.isfile(source_path):
        print(f"Error: source file '{args.source}' not found.")
        sys.exit(1)

    # ------------------------------------------------------------------
    # Phase 1: Resident + Observed
    # ------------------------------------------------------------------
    print("\n" + "=" * 88)
    print(" PHASE 1: RESIDENT + OBSERVED TIMING (OPENMP 4.5 ORIGINAL EXECUTION)")
    print("=" * 88)

    prof_regions = prof_parse_regions(source_path)
    if not prof_regions:
        print("Error: No CAPC profitability regions found.")
        sys.exit(1)

    work_dir = os.path.dirname(source_path)
    stem = os.path.splitext(os.path.basename(source_path))[0]
    tmp_fd, tmp_source = tempfile.mkstemp(prefix="capc_omp45_profile_", suffix=".c", dir=work_dir)
    os.close(tmp_fd)
    profile_exe = os.path.join(work_dir, f".capc_omp45_profile_{os.getpid()}_{stem}")

    try:
        prof_instrument_openmp_source(source_path, tmp_source, prof_regions)
        prof_compile_openmp_program(tmp_source, profile_exe, gpu_arch=args.gpu)
        out, err, rc = prof_run_executable(profile_exe)
        prof_process_profiler_output(out, err, rc, prof_regions)
    finally:
        if os.path.exists(tmp_source):
            os.remove(tmp_source)
        if os.path.exists(profile_exe):
            try:
                os.remove(profile_exe)
            except OSError:
                pass

    # ------------------------------------------------------------------
    # Phase 2: Isolated
    # ------------------------------------------------------------------
    print("\n" + "=" * 88)
    print(" PHASE 2: ISOLATED TIMING (ONE STANDALONE PROGRAM PER REGION)")
    print("=" * 88)

    content, functions, main_func, gen_regions, bounds_map = gen_parse_c_file(source_path)

    print("Detected profitability regions:")
    for r in gen_regions:
        # gen_RegionInfo stores source character offsets (`start`, `end`),
        # not precomputed line numbers.  Convert the target start offset to
        # a 1-based source line number for reporting.
        source_line = content.count("\n", 0, r.start) + 1
        print(
            f"  Region {r.region_id}: line {source_line}, "
            f"function {r.function.name}()"
        )

    standalone_dir = os.path.join(work_dir, "standalone_regions")
    generated_files = gen_generate_standalone_files(
        content,
        functions,
        main_func,
        gen_regions,
        bounds_map,
        output_dir=standalone_dir,
    )

    isolated_results = _capc_run_isolated_regions(
        generated_files,
        gpu_arch=args.gpu,
        timeout=args.timeout,
    )

    # ------------------------------------------------------------------
    # Final merge
    # ------------------------------------------------------------------
    _capc_print_combined_report(prof_regions, isolated_results)

    if args.csv:
        _capc_write_csv(args.csv, prof_regions, isolated_results)


if __name__ == "__main__":
    combined_main()
