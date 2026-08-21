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
class FunctionInfo:
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
class RegionInfo:
    region_id: str
    start: int
    end: int
    begin_line: str
    body_code: str
    end_line: str
    full_block: str
    function: FunctionInfo


# =============================================================================
# C-source scanning helpers
# =============================================================================

def mask_comments_and_strings(code: str) -> str:
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


def find_matching_brace(masked_code: str, opening_brace_pos: int) -> int:
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


def find_functions(content: str) -> List[FunctionInfo]:
    """
    Find ordinary C function definitions and bound them using matching braces.
    """
    masked = mask_comments_and_strings(content)

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
            close_brace = find_matching_brace(masked, open_brace)
        except ValueError:
            continue

        key = (match.start(), close_brace)
        if key in seen_ranges:
            continue

        seen_ranges.add(key)

        functions.append(
            FunctionInfo(
                name=name,
                start=match.start(),
                open_brace=open_brace,
                close_brace=close_brace,
                signature=content[match.start():open_brace].strip(),
            )
        )

    functions.sort(key=lambda f: f.start)
    return functions


def enclosing_function(
    functions: List[FunctionInfo],
    start: int,
    end: int,
) -> Optional[FunctionInfo]:
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

def split_clause_items(text: str) -> List[str]:
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


def extract_map_clause_payloads(code: str) -> List[str]:
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


def get_declared_array_specs(full_code: str):
    """Infer array-section specifications from ordinary C array declarations."""
    specs = {}
    type_re = re.compile(
        r"^\s*(?:(?:static|extern|const|volatile|register|auto)\s+)*"
        r"(?:(?:signed|unsigned)\s+)?"
        r"(?:(?:long\s+long|long|short)\s+)?"
        r"(?:long\s+double|double|float|int|char|size_t)\s+"
        r"(.+?)\s*;\s*$"
    )

    # Split by semicolon first so multiple declarations on one physical line
    # are handled as well.
    for statement in full_code.split(';'):
        line = re.sub(r"//.*$", "", statement).strip()
        if not line or line.startswith('#'):
            continue
        candidate = line + ';'
        m = type_re.match(candidate)
        if not m:
            continue
        for decl in split_clause_items(m.group(1)):
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
                specs.setdefault(
                    name,
                    name + ''.join(f"[0:{d.strip()}]" for d in dims),
                )
    return specs


def get_array_bounds_map(full_code: str):
    """Build array sections from declarations plus OpenMP map/update clauses."""
    bounds_map = get_declared_array_specs(full_code)

    for payload in extract_map_clause_payloads(full_code):
        for item in split_clause_items(payload):
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)", item)
            if m and "[" in item:
                bounds_map[m.group(1)] = item.strip()

    update_re = re.compile(
        r"#\s*pragma\s+omp\s+target\s+update[^\n]*?"
        r"\b(?:to|from)\s*\(([^)]*)\)",
        re.IGNORECASE,
    )
    for match in update_re.finditer(full_code):
        for item in split_clause_items(match.group(1)):
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)", item)
            if m and "[" in item:
                bounds_map[m.group(1)] = item.strip()
    return bounds_map


def get_target_region_array_specs(target_block: str, bounds_map):
    """
    Return the UNION of explicit map variables and declared arrays indexed by
    the region body.  This catches implicit mappings such as `result`.
    """
    target_vars = []
    for payload in extract_map_clause_payloads(target_block):
        for item in split_clause_items(payload):
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)", item)
            if m:
                name = m.group(1)
                if name in bounds_map and name not in target_vars:
                    target_vars.append(name)

    body = strip_openmp_pragmas(target_block)
    for name in re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\[", body):
        if name in bounds_map and name not in target_vars:
            target_vars.append(name)

    return [bounds_map[name] for name in target_vars], target_vars


def classify_target_array_accesses(
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
    computational_code = strip_openmp_pragmas(target_body)
    computational_lines = [
        line
        for line in computational_code.splitlines()
        if not line.lstrip().startswith("#")
    ]

    code = mask_comments_and_strings("\n".join(computational_lines))

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


def specs_for_vars(var_names: List[str], bounds_map):
    return [bounds_map.get(var, var) for var in var_names]


# =============================================================================
# OpenMP pragma manipulation
# =============================================================================

def strip_openmp_pragmas(code: str) -> str:
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


def strip_map_clauses_from_pragma(pragma_text: str) -> str:
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


def rewrite_target_mapping_for_standalone(
    target_block: str,
    target_specs: List[str],
) -> str:
    """Rewrite target-data and target-compute mappings to resident map(alloc)."""
    lines = target_block.splitlines()
    groups = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if re.match(r"^\s*#\s*pragma\s+omp\b", line, re.IGNORECASE):
            group = [line]
            i += 1
            while group[-1].rstrip().endswith("\\") and i < len(lines):
                group.append(lines[i])
                i += 1
            groups.append(("pragma", group))
        else:
            groups.append(("normal", [line]))
            i += 1

    rewritten = []
    compute_done = False
    for kind, group in groups:
        if kind != "pragma":
            rewritten.extend(group)
            continue

        parts = []
        for physical in group:
            part = physical.rstrip()
            if part.endswith("\\"):
                part = part[:-1].rstrip()
            parts.append(part.strip())
        logical = re.sub(r"\s+", " ", " ".join(parts)).strip()

        is_data = bool(re.search(
            r"#\s*pragma\s+omp\s+target\s+data\b", logical, re.IGNORECASE
        ))
        is_compute = bool(
            re.search(r"#\s*pragma\s+omp\s+target\b", logical, re.IGNORECASE)
            and not re.search(
                r"#\s*pragma\s+omp\s+target\s+(?:enter\s+data|exit\s+data|update|data\b)",
                logical,
                re.IGNORECASE,
            )
        )

        if is_data or (is_compute and not compute_done):
            clean = strip_map_clauses_from_pragma(logical)
            if target_specs:
                clean += " map(alloc:" + ", ".join(target_specs) + ")"
            rewritten.append(clean)
            if is_compute:
                compute_done = True
        else:
            rewritten.extend(group)

    return "\n".join(rewritten)


# =============================================================================
# Segment cleanup
# =============================================================================

def line_brace_delta(line: str) -> int:
    masked = mask_comments_and_strings(line)
    return masked.count("{") - masked.count("}")


def block_is_unclosed_from_line(
    start_idx: int,
    lines: List[str],
) -> bool:
    """
    True when a control block opened at this point is not closed within the
    current prefix segment.
    """
    joined = "\n".join(lines[start_idx:])
    masked = mask_comments_and_strings(joined)

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


def sanitize_c_segment(code_str: str) -> str:
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
    masked_lines = [mask_comments_and_strings(line) for line in lines]

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

def remove_function_from_source(
    content: str,
    func: FunctionInfo,
) -> str:
    return (
        content[:func.start]
        + content[func.close_brace + 1:]
    )


def strip_capc_markers(code: str) -> str:
    return "\n".join(
        line
        for line in code.splitlines()
        if "profitability_region" not in line
    )



# =============================================================================
# Prior-CAPC replay policy
# =============================================================================

def process_prior_capc_regions(prefix_code: str, bounds_map):
    """
    Build cheap host prerequisite replay for a later standalone region.

    Policy:
      * Ordinary host code outside CAPC regions is preserved.
      * Earlier CAPC regions that are pure array producers (no array reads,
        at least one array write) are replayed on the host.  These are commonly
        initialization regions.
      * Earlier CAPC regions that read arrays are treated as compute regions and
        are NOT replayed serially.  Their array outputs are recorded so that a
        later target that needs those values can receive a cheap deterministic
        zero initialization instead.

    This prevents cases such as 3mm Region 3 from serially replaying the prior
    O(N^3) matrix multiplication before the GPU timer is even reached.

    Returns:
        transformed_prefix,
        skipped_output_vars,
        replayed_initializer_count,
        skipped_compute_count
    """
    lines = prefix_code.splitlines()
    out = []
    skipped_outputs = []
    replayed_initializers = 0
    skipped_compute = 0
    i = 0

    begin_re = re.compile(
        r"^\s*#\s*pragma\s+capc\s+profitability_region\s+begin\b",
        re.IGNORECASE,
    )
    end_re = re.compile(
        r"^\s*#\s*pragma\s+capc\s+profitability_region\s+end\b",
        re.IGNORECASE,
    )

    while i < len(lines):
        if not begin_re.match(lines[i]):
            out.append(lines[i])
            i += 1
            continue

        block_lines = [lines[i]]
        i += 1

        while i < len(lines):
            block_lines.append(lines[i])
            if end_re.match(lines[i]):
                i += 1
                break
            i += 1

        if len(block_lines) < 2:
            continue

        body_lines = [
            line for line in block_lines
            if not begin_re.match(line) and not end_re.match(line)
        ]
        body = "\n".join(body_lines)
        full_block = "\n".join(block_lines)

        _, var_names = get_target_region_array_specs(full_block, bounds_map)
        reads, writes = classify_target_array_accesses(body, var_names)

        # Pure producer => cheap/initialization-like region. Replay body only,
        # with all OpenMP directives removed so it executes on the host.
        if writes and not reads:
            replay = strip_openmp_pragmas(body)
            replay = replay.replace('printf("");', '')
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

    return (
        "\n".join(out),
        skipped_outputs,
        replayed_initializers,
        skipped_compute,
    )


def zero_initialization_for_array_spec(spec: str, serial: int) -> str:
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


def build_synthetic_input_initialization(
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
        code = zero_initialization_for_array_spec(spec, serial)

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

def parse_c_file(file_path: str):
    with open(file_path, "r") as f:
        content = f.read()

    functions = find_functions(content)

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
            "No '#pragma capc profitability_region begin/end' "
            "markers found in file."
        )

    regions = []

    for idx, match in enumerate(region_matches, start=1):
        function = enclosing_function(
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
            RegionInfo(
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

def clean_directory(output_dir: str):
    if os.path.exists(output_dir):
        print(
            f"Cleaning previous standalone region files "
            f"in '{output_dir}'..."
        )
        shutil.rmtree(output_dir)

    os.makedirs(output_dir, exist_ok=True)


def get_function_prefix(
    content: str,
    target: RegionInfo,
) -> str:
    """
    Return all source in target's enclosing function before the CAPC target.
    """
    return content[
        target.function.body_start:target.start
    ]


def generate_standalone_files(
    content: str,
    functions: List[FunctionInfo],
    main_func: FunctionInfo,
    regions: List[RegionInfo],
    bounds_map,
    output_dir: str = "standalone_regions",
):
    clean_directory(output_dir)

    # Keep globals, includes, declarations, prototypes and helper functions,
    # but remove original main because every standalone file gets a synthetic
    # main().
    support_source = remove_function_from_source(
        content,
        main_func,
    )

    # A standalone file must contain exactly ONE CAPC marker pair: the target.
    support_source = strip_capc_markers(
        support_source
    )

    # Any helper called by setup must execute on the host only.
    support_source = strip_openmp_pragmas(
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
            get_target_region_array_specs(
                target.full_block,
                bounds_map,
            )
        )

        read_vars, write_vars = (
            classify_target_array_accesses(
                target.body_code,
                target_var_names,
            )
        )

        # ---------------------------------------------------------------------
        # Host-only prerequisite replay
        # ---------------------------------------------------------------------
        prefix_raw = get_function_prefix(
            content,
            target,
        )

        (
            prefix_policy,
            skipped_prior_outputs,
            replayed_initializer_count,
            skipped_compute_count,
        ) = process_prior_capc_regions(
            prefix_raw,
            bounds_map,
        )

        prefix_clean = sanitize_c_segment(
            prefix_policy
        ).strip()

        # Remove all remaining OpenMP pragmas from setup/prerequisites so no
        # offload, data mapping, or CPU OpenMP execution contaminates timing.
        prefix_clean = strip_openmp_pragmas(
            prefix_clean
        ).strip()

        # If an input needed by the target was produced by an earlier expensive
        # CAPC compute region that we intentionally omitted, create a cheap,
        # deterministic host value for that array section.
        synthetic_init, unresolved_synthetic = (
            build_synthetic_input_initialization(
                skipped_prior_outputs,
                read_vars,
                bounds_map,
            )
        )

        h2d_specs = specs_for_vars(
            read_vars,
            bounds_map,
        )

        d2h_specs = specs_for_vars(
            write_vars,
            bounds_map,
        )

        h2d_str = ", ".join(h2d_specs)
        d2h_str = ", ".join(d2h_specs)
        target_specs_str = ", ".join(array_specs)

        # Rewrite target mapping so original map(to/from/tofrom:...) clauses
        # cannot perform data movement inside kernel timing.
        target_code = rewrite_target_mapping_for_standalone(
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

def compile_and_run_regions(
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

def main():
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
        ) = parse_c_file(input_file)

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

        generated = generate_standalone_files(
            content,
            functions,
            main_func,
            regions,
            bounds_map,
        )

        compile_and_run_regions(
            generated
        )

    except Exception as exc:
        print(
            f"Error: {exc}"
        )
        sys.exit(1)


if __name__ == "__main__":
    main()
