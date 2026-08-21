#!/usr/bin/env python3

import os
import sys
import re
import subprocess
import argparse
import tempfile
import resource
from collections import defaultdict


# =============================================================================
# Runtime setup
# =============================================================================

try:
    resource.setrlimit(
        resource.RLIMIT_STACK,
        (resource.RLIM_INFINITY, resource.RLIM_INFINITY),
    )
except Exception:
    pass


# =============================================================================
# CAPC region parsing
# =============================================================================

def parse_regions(source_file):
    regions = []
    current_region = None
    region_id = 1

    with open(source_file, "r") as f:
        for line_num, line in enumerate(f, start=1):
            stripped = line.strip()

            if "#pragma capc profitability_region begin" in stripped:
                current_region = {
                    "id": region_id,
                    "begin_line": line_num,
                    "end_line": None,
                    "count": 0,
                    "resident_time": 0.0,
                    "init_time": 0.0,
                    "one_time_transfer_time": 0.0,
                    "recurring_transfer_time": 0.0,

                    # key = (source line, event tag)
                    # A tag is essential when H2D and D2H originate from the
                    # same compute pragma; otherwise two one-time events on one
                    # line can be mistaken for one recurring event.
                    "_transfer_events": defaultdict(list),
                }

            elif (
                "#pragma capc profitability_region end" in stripped
                and current_region
            ):
                current_region["end_line"] = line_num
                regions.append(current_region)
                region_id += 1
                current_region = None

    return regions


def get_associated_region_id(line_num, regions):
    """
    Attribute non-CAPC OpenACC operations to the nearest logical region using
    the same policy used by the validated profiler:

      * inside a region      -> that region
      * before first region  -> Region 1
      * between regions      -> preceding region
      * after final region   -> final region
    """
    if not regions:
        return 1

    for reg in regions:
        if reg["begin_line"] <= line_num <= reg["end_line"]:
            return reg["id"]

    if line_num < regions[0]["begin_line"]:
        return regions[0]["id"]

    for idx in range(len(regions) - 1):
        if (
            regions[idx]["end_line"]
            < line_num
            < regions[idx + 1]["begin_line"]
        ):
            return regions[idx]["id"]

    return regions[-1]["id"]


# =============================================================================
# Generic C/OpenACC parsing helpers
# =============================================================================

def consume_statement(lines, idx):
    """
    Consume the complete C statement/block controlled by an OpenACC compute
    pragma. This supports:
        pragma + for
        pragma + nested for
        pragma + compound block
        pragma + another pragma + for
    """
    n = len(lines)

    while idx < n:
        stripped = lines[idx].strip()

        if (
            not stripped
            or stripped.startswith("//")
            or stripped.startswith("/*")
        ):
            idx += 1
            continue

        if stripped.startswith("#pragma"):
            idx += 1
            continue

        if "{" in stripped:
            depth = 0

            while idx < n:
                line = lines[idx]
                depth += line.count("{") - line.count("}")
                idx += 1

                if depth <= 0:
                    break

            return idx

        if any(
            stripped.startswith(keyword)
            for keyword in ("for", "while", "if", "do")
        ):
            idx += 1
            return consume_statement(lines, idx)

        idx += 1

        while idx < n and ";" not in stripped:
            stripped = lines[idx].strip()
            idx += 1

        return idx

    return idx


def consume_acc_pragma(lines, start_idx):
    """
    Consume one logical OpenACC pragma, including all backslash-continuation
    lines.

    Returns:
        logical_pragma : normalized one-line pragma, with '\' removed
        next_idx       : first physical line after the pragma
        physical_lines : original physical pragma lines
    """
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

    logical = " ".join(piece for piece in pieces if piece)
    logical = re.sub(r"\s+", " ", logical).strip()

    return logical, idx, physical


def find_parenthesized_span(text, open_pos):
    """
    Given text[open_pos] == '(', return the index immediately after the
    matching ')'.
    """
    depth = 0

    for pos in range(open_pos, len(text)):
        if text[pos] == "(":
            depth += 1
        elif text[pos] == ")":
            depth -= 1

            if depth == 0:
                return pos + 1

    raise ValueError(
        "Unmatched parenthesis while parsing OpenACC pragma"
    )


def split_top_level_commas(text):
    items = []
    current = []
    square_depth = 0
    paren_depth = 0
    brace_depth = 0

    for ch in text:
        if ch == "[":
            square_depth += 1
        elif ch == "]":
            square_depth = max(0, square_depth - 1)
        elif ch == "(":
            paren_depth += 1
        elif ch == ")":
            paren_depth = max(0, paren_depth - 1)
        elif ch == "{":
            brace_depth += 1
        elif ch == "}":
            brace_depth = max(0, brace_depth - 1)

        if (
            ch == ","
            and square_depth == 0
            and paren_depth == 0
            and brace_depth == 0
        ):
            item = "".join(current).strip()

            if item:
                items.append(item)

            current = []
        else:
            current.append(ch)

    final = "".join(current).strip()

    if final:
        items.append(final)

    return items


def unique_preserve_order(items):
    seen = set()
    result = []

    for item in items:
        normalized = re.sub(r"\s+", "", item)

        if normalized not in seen:
            seen.add(normalized)
            result.append(item.strip())

    return result



def mask_comments_and_strings(code):
    """Replace comments and string/character literals with spaces."""
    out = []
    i = 0
    n = len(code)
    state = "code"

    while i < n:
        ch = code[i]
        nxt = code[i + 1] if i + 1 < n else ""

        if state == "code":
            if ch == "/" and nxt == "/":
                out.extend("  ")
                i += 2
                state = "line_comment"
            elif ch == "/" and nxt == "*":
                out.extend("  ")
                i += 2
                state = "block_comment"
            elif ch == '"':
                out.append(" ")
                i += 1
                state = "string"
            elif ch == "'":
                out.append(" ")
                i += 1
                state = "char"
            else:
                out.append(ch)
                i += 1

        elif state == "line_comment":
            if ch == "\n":
                out.append("\n")
                state = "code"
            else:
                out.append(" ")
            i += 1

        elif state == "block_comment":
            if ch == "*" and nxt == "/":
                out.extend("  ")
                i += 2
                state = "code"
            else:
                out.append("\n" if ch == "\n" else " ")
                i += 1

        elif state in ("string", "char"):
            quote = '"' if state == "string" else "'"
            if ch == "\\" and i + 1 < n:
                out.extend("  ")
                i += 2
            elif ch == quote:
                out.append(" ")
                i += 1
                state = "code"
            else:
                out.append("\n" if ch == "\n" else " ")
                i += 1

    return "".join(out)


def get_declared_array_bounds_map(full_code):
    """
    Infer full OpenACC sections from ordinary C array declarations.

    Example:
        double a[2000][2000];
    becomes:
        a -> a[0:2000][0:2000]
    """
    code = mask_comments_and_strings(full_code)
    result = {}

    type_re = (
        r"(?:static\s+|extern\s+|const\s+|volatile\s+|register\s+|"
        r"restrict\s+|_Alignas\s*\([^)]*\)\s+)*"
        r"(?:(?:unsigned|signed)\s+)?"
        r"(?:(?:long\s+long|long|short)\s+)?"
        r"(?:double|float|int|char|size_t|ptrdiff_t|_Bool)\b"
    )

    for m in re.finditer(rf"(?m)^[ \t]*{type_re}([^;]*);", code):
        declarators = m.group(1)
        for am in re.finditer(
            r"\b([A-Za-z_][A-Za-z0-9_]*)\s*"
            r"((?:\[[^\]]+\]\s*)+)",
            declarators,
        ):
            name = am.group(1)
            dims = re.findall(r"\[\s*([^\]]+)\s*\]", am.group(2))
            if not dims:
                continue

            spec = name
            valid = True
            for dim in dims:
                dim = dim.strip()
                if not dim:
                    valid = False
                    break
                spec += f"[0:{dim}]"

            if valid and name not in result:
                result[name] = spec

    return result


def spec_var_name(spec):
    m = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)", spec)
    return m.group(1) if m else None


def infer_used_array_specs(statement_text, declaration_bounds):
    """Return declared arrays actually indexed by one target statement/body."""
    code = mask_comments_and_strings(statement_text)
    names = []
    for name in declaration_bounds:
        if re.search(r"\b" + re.escape(name) + r"\s*\[", code):
            names.append(name)
    return [declaration_bounds[n] for n in names], names


def classify_array_accesses(statement_text, array_names):
    """Classify arrays as read-before/write inputs and modified outputs."""
    code = mask_comments_and_strings(statement_text)
    reads = []
    writes = []

    for var in array_names:
        saw_read = False
        saw_write = False
        pat = re.compile(r"\b" + re.escape(var) + r"\s*(?:\[[^\]]*\]\s*)+")

        for m in pat.finditer(code):
            before = code[max(0, m.start() - 4):m.start()]
            after = code[m.end():m.end() + 10]

            if re.search(r"(?:\+\+|--)\s*$", before) or re.match(r"\s*(?:\+\+|--)", after):
                saw_read = True
                saw_write = True
                continue

            op = re.match(r"\s*(<<=|>>=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|=)", after)
            if op:
                saw_write = True
                if op.group(1) != "=":
                    saw_read = True
            else:
                saw_read = True

        if saw_read:
            reads.append(var)
        if saw_write:
            writes.append(var)

    return reads, writes


# =============================================================================
# OpenACC clause parsing / rewriting
# =============================================================================

# Data clauses which can appear on OpenACC compute constructs.
# Synonyms are included because older code may use pcopy* forms.
ACC_DATA_CLAUSES = (
    "present_or_copyin",
    "present_or_copyout",
    "present_or_copy",
    "present_or_create",
    "pcopyin",
    "pcopyout",
    "pcopy",
    "copyin",
    "copyout",
    "create",
    "present",
    "copy",
    "deviceptr",
)

ACC_CLAUSE_RE = re.compile(
    r"\b("
    + "|".join(
        sorted(
            (re.escape(x) for x in ACC_DATA_CLAUSES),
            key=len,
            reverse=True,
        )
    )
    + r")\s*\(",
    re.IGNORECASE,
)


def parse_acc_data_clauses(logical_pragma):
    """
    Parse OpenACC data clauses from one logical pragma.

    Returns entries:
        {
            "type": clause name,
            "items": [array/scalar specs],
            "start": clause start,
            "end": first char after matching ')'
        }
    """
    clauses = []
    pos = 0

    while True:
        match = ACC_CLAUSE_RE.search(logical_pragma, pos)

        if not match:
            break

        clause_type = match.group(1).lower()
        open_paren = logical_pragma.find(
            "(",
            match.start(),
            match.end(),
        )

        end = find_parenthesized_span(
            logical_pragma,
            open_paren,
        )

        payload = logical_pragma[
            open_paren + 1:end - 1
        ].strip()

        clauses.append(
            {
                "type": clause_type,
                "items": split_top_level_commas(payload),
                "start": match.start(),
                "end": end,
            }
        )

        pos = end

    return clauses



def acc_clause_kind_by_var(logical_pragma):
    result = {}
    for clause in parse_acc_data_clauses(logical_pragma):
        for item in clause['items']:
            name = spec_var_name(item)
            if name:
                result[name] = clause['type']
    return result


def extract_data_directive_var_names(logical_pragma):
    """Extract variable names from OpenACC data-management clauses."""
    names = []
    clause_re = re.compile(
        r'\b(?:create|copyin|copyout|copy|present|pcopy|pcopyin|pcopyout|'
        r'present_or_copy|present_or_copyin|present_or_copyout|'
        r'present_or_create|deviceptr|delete)\s*\(',
        re.IGNORECASE,
    )
    pos = 0
    while True:
        m = clause_re.search(logical_pragma, pos)
        if not m:
            break
        open_pos = logical_pragma.find('(', m.start(), m.end())
        end = find_parenthesized_span(logical_pragma, open_pos)
        payload = logical_pragma[open_pos + 1:end - 1]
        for item in split_top_level_commas(payload):
            name = spec_var_name(item)
            if name and name not in names:
                names.append(name)
        pos = end
    return names


def transfer_sets_from_acc_compute(logical_pragma):
    """
    Convert explicit compute-region data clauses to isolated transfer sets.

    Semantics used:
      copyin / pcopyin / present_or_copyin
            -> H2D

      copyout / pcopyout / present_or_copyout
            -> D2H

      copy / pcopy / present_or_copy
            -> H2D + D2H

      create / present / present_or_create / deviceptr
            -> no explicit transfer generated by this profiler

    Returns:
        h2d_specs, d2h_specs, all_managed_specs, has_transfer_clauses
    """
    h2d = []
    d2h = []
    managed = []
    has_transfer_clauses = False

    for clause in parse_acc_data_clauses(logical_pragma):
        kind = clause["type"]
        items = clause["items"]

        # deviceptr variables are already device pointers and must not be
        # allocated with `enter data create`.
        if kind != "deviceptr":
            managed.extend(items)

        if kind in {
            "copyin",
            "pcopyin",
            "present_or_copyin",
        }:
            h2d.extend(items)
            has_transfer_clauses = True

        elif kind in {
            "copyout",
            "pcopyout",
            "present_or_copyout",
        }:
            d2h.extend(items)
            has_transfer_clauses = True

        elif kind in {
            "copy",
            "pcopy",
            "present_or_copy",
        }:
            h2d.extend(items)
            d2h.extend(items)
            has_transfer_clauses = True

    return (
        unique_preserve_order(h2d),
        unique_preserve_order(d2h),
        unique_preserve_order(managed),
        has_transfer_clauses,
    )


def remove_acc_data_clauses(logical_pragma):
    clauses = parse_acc_data_clauses(logical_pragma)

    if not clauses:
        return logical_pragma

    pieces = []
    last = 0

    for clause in clauses:
        pieces.append(
            logical_pragma[last:clause["start"]]
        )
        last = clause["end"]

    pieces.append(logical_pragma[last:])

    cleaned = "".join(pieces)
    cleaned = re.sub(r"\s+", " ", cleaned).strip()

    return cleaned


def rewrite_acc_compute_as_present(logical_pragma, managed_specs):
    """
    Remove compute-region copy/copyin/copyout/create/present/etc. clauses and
    append one `present(...)` clause for variables explicitly managed by the
    profiler.

    This prevents hidden data movement inside the resident kernel timer.
    """
    base = remove_acc_data_clauses(logical_pragma)

    if managed_specs:
        base += (
            " present("
            + ", ".join(managed_specs)
            + ")"
        )

    return re.sub(r"\s+", " ", base).strip()


def is_acc_compute_directive(logical_pragma):
    lower = logical_pragma.lower()

    return bool(
        re.search(
            r"#\s*pragma\s+acc\s+"
            r"(parallel|kernels|serial)\b",
            lower,
        )
    )


def is_acc_explicit_data_directive(logical_pragma):
    lower = logical_pragma.lower()

    return bool(
        re.search(
            r"#\s*pragma\s+acc\s+"
            r"(enter\s+data|exit\s+data|update)\b",
            lower,
        )
    )


def is_acc_cleanup_only_exit(logical_pragma):
    lower = logical_pragma.lower()

    if not re.search(
        r"#\s*pragma\s+acc\s+exit\s+data\b",
        lower,
    ):
        return False

    # `delete(...)` is cleanup only. `copyout(...)` moves data and is timed.
    has_delete = bool(
        re.search(r"\bdelete\s*\(", lower)
    )
    has_copyout = bool(
        re.search(
            r"\b(?:copyout|pcopyout|present_or_copyout)"
            r"\s*\(",
            lower,
        )
    )

    return has_delete and not has_copyout


def source_has_explicit_acc_init(lines):
    """
    Detect user-written acc_init(). Comment-only occurrences are ignored.
    """
    in_block_comment = False

    for line in lines:
        text = line

        if in_block_comment:
            end = text.find("*/")

            if end < 0:
                continue

            text = text[end + 2:]
            in_block_comment = False

        while "/*" in text:
            start = text.find("/*")
            end = text.find("*/", start + 2)

            if end < 0:
                text = text[:start]
                in_block_comment = True
                break

            text = text[:start] + text[end + 2:]

        text = text.split("//", 1)[0]

        if re.search(r"\bacc_init\s*\(", text):
            return True

    return False


# =============================================================================
# Source instrumentation
# =============================================================================

def instrument_openacc_source(
    source_path,
    temp_path,
    regions,
):
    with open(source_path, "r") as f:
        lines = f.readlines()

    full_source = "".join(lines)
    declaration_bounds = get_declared_array_bounds_map(full_source)
    active_persistent = set()

    explicit_acc_init = source_has_explicit_acc_init(
        lines
    )

    instrumented = [
        "#include <omp.h>\n",
        "#include <stdio.h>\n",
        "#include <openacc.h>\n\n",

        "static double _capc_dt0, _capc_dt1;\n",
        "static double _capc_k0, _capc_k1;\n",
        "static double _capc_init0, _capc_init1;\n",
        "static int _capc_gpu_initialized = 0;\n\n",
    ]

    if not explicit_acc_init:
        instrumented.extend(
            [
                "static void _capc_ensure_gpu_init("
                "int region_id, int line_num)\n",
                "{\n",
                "    if (!_capc_gpu_initialized) {\n",
                "        _capc_init0 = omp_get_wtime();\n",
                "        acc_init(acc_device_nvidia);\n",
                "        _capc_init1 = omp_get_wtime();\n",
                "        _capc_gpu_initialized = 1;\n",
                '        printf("[PROFILER] init '
                'region:%d line:%d | GPU Initialization Time '
                '= %.9f s\\n",\n',
                "               region_id, line_num, "
                "_capc_init1 - _capc_init0);\n",
                "    }\n",
                "}\n\n",
            ]
        )

    i = 0
    n = len(lines)

    while i < n:
        line = lines[i]
        stripped = line.strip()
        line_num = i + 1

        # ---------------------------------------------------------------------
        # User-written explicit acc_init()
        # ---------------------------------------------------------------------
        if (
            explicit_acc_init
            and re.search(r"\bacc_init\s*\(", stripped)
            and not stripped.startswith("//")
        ):
            reg_id = get_associated_region_id(
                line_num,
                regions,
            )

            instrumented.append(
                "  _capc_init0 = omp_get_wtime();\n"
            )
            instrumented.append(line)
            instrumented.append(
                "  _capc_init1 = omp_get_wtime();\n"
            )
            instrumented.append(
                "  _capc_gpu_initialized = 1;\n"
            )
            instrumented.append(
                f'  printf("[PROFILER] init '
                f'region:{reg_id} line:{line_num} | '
                f'GPU Initialization Time = %.9f s\\n", '
                f'_capc_init1 - _capc_init0);\n'
            )

            i += 1
            continue

        # ---------------------------------------------------------------------
        # OpenACC pragmas: consume the complete logical directive first.
        # ---------------------------------------------------------------------
        if re.match(
            r"^\s*#\s*pragma\s+acc\b",
            line,
            flags=re.IGNORECASE,
        ):
            logical, next_idx, physical = consume_acc_pragma(
                lines,
                i,
            )

            reg_id = get_associated_region_id(
                line_num,
                regions,
            )

            # -----------------------------------------------------------------
            # enter data / exit data / update
            # -----------------------------------------------------------------
            if is_acc_explicit_data_directive(logical):
                lower_logical = logical.lower()
                directive_vars = extract_data_directive_var_names(logical)
                is_enter = bool(re.search(r'#\s*pragma\s+acc\s+enter\s+data\b', lower_logical))
                is_exit = bool(re.search(r'#\s*pragma\s+acc\s+exit\s+data\b', lower_logical))

                if is_acc_cleanup_only_exit(logical):
                    instrumented.extend(physical)
                    if is_exit:
                        for name in directive_vars:
                            active_persistent.discard(name)
                    i = next_idx
                    continue

                if not explicit_acc_init:
                    instrumented.append(
                        f"  _capc_ensure_gpu_init("
                        f"{reg_id}, {line_num});\n"
                    )

                instrumented.append("  _capc_dt0 = omp_get_wtime();\n")
                instrumented.extend(physical)
                instrumented.append("  #pragma acc wait\n")
                instrumented.append("  _capc_dt1 = omp_get_wtime();\n")
                instrumented.append(
                    f'  printf("[PROFILER] transfer '
                    f'region:{reg_id} line:{line_num} '
                    f'tag:data | Transfer Time = %.9f s\\n", '
                    f'_capc_dt1 - _capc_dt0);\n'
                )

                if is_enter:
                    active_persistent.update(directive_vars)
                elif is_exit:
                    for name in directive_vars:
                        active_persistent.discard(name)

                i = next_idx
                continue

            # -----------------------------------------------------------------
            # parallel / kernels / serial compute construct
            # -----------------------------------------------------------------
            if is_acc_compute_directive(logical):
                # First find the complete compute statement/body so array use can
                # be inferred even when the pragma has no data clauses.
                end_idx = consume_statement(lines, next_idx)
                statement_text = "".join(lines[next_idx:end_idx])

                (
                    explicit_h2d,
                    explicit_d2h,
                    explicit_managed,
                    has_transfer_clauses,
                ) = transfer_sets_from_acc_compute(logical)

                clause_kinds = acc_clause_kind_by_var(logical)
                explicit_vars = set(clause_kinds)

                inferred_specs, inferred_names = infer_used_array_specs(
                    statement_text,
                    declaration_bounds,
                )
                read_names, write_names = classify_array_accesses(
                    statement_text,
                    inferred_names,
                )

                # Arrays referenced in the body but neither covered by an
                # explicit compute data clause nor already living in a
                # persistent enter-data environment use implicit OpenACC data
                # semantics.  Split those implicit copies from the kernel timer.
                implicit_names = [
                    name for name in inferred_names
                    if name not in explicit_vars and name not in active_persistent
                ]

                inferred_by_name = {
                    name: declaration_bounds[name]
                    for name in inferred_names
                }

                implicit_h2d = [
                    inferred_by_name[name]
                    for name in implicit_names
                    if name in read_names
                ]
                implicit_d2h = [
                    inferred_by_name[name]
                    for name in implicit_names
                    if name in write_names
                ]

                h2d_specs = unique_preserve_order(explicit_h2d + implicit_h2d)
                d2h_specs = unique_preserve_order(explicit_d2h + implicit_d2h)

                # Merge pragma-discovered and declaration/body-discovered arrays
                # for the resident present(...) kernel.
                all_managed_specs = unique_preserve_order(
                    explicit_managed + inferred_specs
                )

                allocation_clause_present = any(
                    kind in {
                        'create', 'present_or_create',
                        'copy', 'copyin', 'copyout',
                        'pcopy', 'pcopyin', 'pcopyout',
                        'present_or_copy', 'present_or_copyin',
                        'present_or_copyout',
                    }
                    for kind in clause_kinds.values()
                )

                needs_split = bool(
                    has_transfer_clauses
                    or allocation_clause_present
                    or implicit_names
                )

                if not explicit_acc_init:
                    instrumented.append(
                        f"  _capc_ensure_gpu_init("
                        f"{reg_id}, {line_num});\n"
                    )

                if needs_split:
                    # Create temporary mappings only for variables that are not
                    # already persistent and are not explicit present/deviceptr
                    # requirements.
                    create_specs = []
                    for spec in all_managed_specs:
                        name = spec_var_name(spec)
                        if not name or name in active_persistent:
                            continue
                        kind = clause_kinds.get(name)
                        if kind in {'present', 'deviceptr'}:
                            continue
                        create_specs.append(spec)
                    create_specs = unique_preserve_order(create_specs)

                    if create_specs:
                        instrumented.append(
                            "  #pragma acc enter data create("
                            + ", ".join(create_specs)
                            + ")\n"
                        )
                        instrumented.append("  #pragma acc wait\n")

                    if h2d_specs:
                        instrumented.append("  _capc_dt0 = omp_get_wtime();\n")
                        instrumented.append(
                            "  #pragma acc update device("
                            + ", ".join(h2d_specs)
                            + ")\n"
                        )
                        instrumented.append("  #pragma acc wait\n")
                        instrumented.append("  _capc_dt1 = omp_get_wtime();\n")
                        instrumented.append(
                            f'  printf("[PROFILER] transfer '
                            f'region:{reg_id} line:{line_num} '
                            f'tag:implicit_in | Transfer Time = %.9f s\\n", '
                            f'_capc_dt1 - _capc_dt0);\n'
                        )

                    resident_pragma = rewrite_acc_compute_as_present(
                        logical,
                        all_managed_specs,
                    )

                    instrumented.append("  _capc_k0 = omp_get_wtime();\n")
                    instrumented.append("  " + resident_pragma + "\n")
                    i = next_idx
                    while i < end_idx:
                        instrumented.append(lines[i])
                        i += 1
                    instrumented.append("  #pragma acc wait\n")
                    instrumented.append("  _capc_k1 = omp_get_wtime();\n")
                    instrumented.append(
                        f'  printf("[PROFILER] kernel '
                        f'region:{reg_id} line:{line_num} | '
                        f'Kernel Execution Time = %.9f s\\n", '
                        f'_capc_k1 - _capc_k0);\n'
                    )

                    if d2h_specs:
                        instrumented.append("  _capc_dt0 = omp_get_wtime();\n")
                        instrumented.append(
                            "  #pragma acc update self("
                            + ", ".join(d2h_specs)
                            + ")\n"
                        )
                        instrumented.append("  #pragma acc wait\n")
                        instrumented.append("  _capc_dt1 = omp_get_wtime();\n")
                        instrumented.append(
                            f'  printf("[PROFILER] transfer '
                            f'region:{reg_id} line:{line_num} '
                            f'tag:implicit_out | Transfer Time = %.9f s\\n", '
                            f'_capc_dt1 - _capc_dt0);\n'
                        )

                    if create_specs:
                        instrumented.append(
                            "  #pragma acc exit data delete("
                            + ", ".join(create_specs)
                            + ")\n"
                        )
                        instrumented.append("  #pragma acc wait\n")

                    continue

                # No implicit/explicit allocation or transfer needs to be split.
                # This is the normal persistent/present resident case.
                instrumented.append("  _capc_k0 = omp_get_wtime();\n")
                if inferred_specs and not any(
                    kind == "deviceptr" for kind in clause_kinds.values()
                ):
                    resident_pragma = rewrite_acc_compute_as_present(
                        logical, all_managed_specs
                    )
                    instrumented.append("  " + resident_pragma + "\n")
                else:
                    instrumented.extend(physical)
                i = next_idx
                while i < end_idx:
                    instrumented.append(lines[i])
                    i += 1
                instrumented.append("  #pragma acc wait\n")
                instrumented.append("  _capc_k1 = omp_get_wtime();\n")
                instrumented.append(
                    f'  printf("[PROFILER] kernel '
                    f'region:{reg_id} line:{line_num} | '
                    f'Kernel Execution Time = %.9f s\\n", '
                    f'_capc_k1 - _capc_k0);\n'
                )
                continue

        # Ordinary source line.
        instrumented.append(line)
        i += 1

    with open(temp_path, "w") as f:
        f.writelines(instrumented)


# =============================================================================
# Compile / execute
# =============================================================================

def compile_openacc_program(
    source_file,
    exec_name,
    gpu_arch="cc70",
):
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

    print(
        f"[*] Compiling OpenACC program: "
        f"{' '.join(compile_cmd)}"
    )

    result = subprocess.run(
        compile_cmd,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        print(
            f"[-] Compilation failed for "
            f"'{source_file}':\n{result.stderr}"
        )
        sys.exit(1)


def run_executable(exec_path, timeout=300):
    print(
        f"[*] Executing target OpenACC binary: "
        f"{exec_path}\n"
    )

    try:
        result = subprocess.run(
            [exec_path],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout or ""
        stderr = exc.stderr or ""

        if isinstance(stdout, bytes):
            stdout = stdout.decode(
                errors="replace"
            )

        if isinstance(stderr, bytes):
            stderr = stderr.decode(
                errors="replace"
            )

        print(
            f"[!] Execution timed out after "
            f"{timeout} seconds."
        )

        return stdout, stderr, 124

    return (
        result.stdout,
        result.stderr,
        result.returncode,
    )


# =============================================================================
# Runtime log processing
# =============================================================================

def process_profiler_output(
    stdout_str,
    stderr_str,
    returncode,
    regions,
):
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
            reg["_transfer_events"][
                (line_num, tag)
            ].append(duration)

    # Classify transfer sites by observed runtime frequency.
    #
    # One execution at a site:
    #     one-time transfer/setup
    #
    # Multiple executions at a site:
    #     recurring transfer cost
    for reg in regions:
        for durations in reg[
            "_transfer_events"
        ].values():
            if len(durations) == 1:
                reg[
                    "one_time_transfer_time"
                ] += durations[0]
            else:
                reg[
                    "recurring_transfer_time"
                ] += sum(durations)

    if matched_events == 0:
        print(
            "[!] Warning: No [PROFILER] output logs "
            "were detected."
        )
        print(
            f"[!] Executable Return Code: {returncode}"
        )


# =============================================================================
# Report
# =============================================================================

def print_results(regions):
    """
    Report semantics retained from the validated OpenACC profiler.

    Total Res(s):
        cumulative kernel-only resident time across all invocations.

    Avg Res(s):
        kernel-only resident time of one invocation.

    Total Obs(s):
        cumulative observed contribution over the original run:
            init
          + one-time transfers/setup
          + recurring transfers
          + all kernel invocations

    Avg Obs(s):
        observed cost of one COLD/FIRST invocation:
            full GPU initialization
          + full one-time transfer/setup cost
          + average recurring cost per invocation

        Initialization and one-time setup are intentionally NOT divided by
        invocation count.
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
        "CAPC PROFITABILITY REGION REPORT (OPENACC)"
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

        avg_resident = (
            reg["resident_time"] / count
        )

        avg_recurring_observed = (
            reg["resident_time"]
            + reg["recurring_transfer_time"]
        ) / count

        avg_observed = (
            reg["init_time"]
            + reg["one_time_transfer_time"]
            + avg_recurring_observed
        )

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

    # Preserve the historical TOTAL-row convention.
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

    print(divider + "\n")


# =============================================================================
# Main
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description=(
            "Compile, run and profile OpenACC CAPC regions while separating "
            "resident kernel time, GPU initialization, one-time transfers, "
            "and recurring transfers. Explicit copy/copyin/copyout clauses "
            "on compute constructs are separated from kernel timing."
        )
    )

    parser.add_argument(
        "source",
        help="Path to OpenACC source C file",
    )

    parser.add_argument(
        "--gpu",
        default="cc70",
        help="GPU architecture (default: cc70)",
    )

    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help=(
            "Execution timeout in seconds "
            "(default: 300)"
        ),
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

    work_dir = os.path.dirname(source_path)

    exec_name = os.path.splitext(
        os.path.basename(source_path)
    )[0]

    exec_path = os.path.join(
        work_dir,
        exec_name,
    )

    regions = parse_regions(source_path)

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
        instrument_openacc_source(
            source_path,
            temp_source_path,
            regions,
        )

        compile_openacc_program(
            temp_source_path,
            exec_path,
            gpu_arch=args.gpu,
        )

        (
            stdout_str,
            stderr_str,
            returncode,
        ) = run_executable(
            exec_path,
            timeout=args.timeout,
        )

        process_profiler_output(
            stdout_str,
            stderr_str,
            returncode,
            regions,
        )

        print_results(regions)

    finally:
        if os.path.exists(temp_source_path):
            os.remove(temp_source_path)


if __name__ == "__main__":
    main()