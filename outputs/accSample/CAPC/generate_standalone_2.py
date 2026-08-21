#!/usr/bin/env python3

import sys
import re
import os
import shutil
import subprocess
from dataclasses import dataclass
from typing import List, Optional, Tuple


# -----------------------------------------------------------------------------
# Data structures
# -----------------------------------------------------------------------------

@dataclass
class FunctionInfo:
    name: str
    start: int               # beginning of function signature
    open_brace: int          # position of '{'
    close_brace: int         # position of matching '}'
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


# -----------------------------------------------------------------------------
# C source scanning helpers
# -----------------------------------------------------------------------------

def mask_comments_and_strings(code: str) -> str:
    """
    Return a same-length string where comments/string/char literal contents are
    replaced by spaces. Newlines are preserved. This makes brace matching and
    function discovery substantially safer than counting raw '{' and '}'.
    """
    out = list(code)
    i = 0
    n = len(code)

    while i < n:
        # // comment
        if i + 1 < n and code[i] == '/' and code[i + 1] == '/':
            j = i
            while j < n and code[j] != '\n':
                out[j] = ' '
                j += 1
            i = j
            continue

        # /* comment */
        if i + 1 < n and code[i] == '/' and code[i + 1] == '*':
            out[i] = out[i + 1] = ' '
            j = i + 2
            while j + 1 < n and not (code[j] == '*' and code[j + 1] == '/'):
                if code[j] != '\n':
                    out[j] = ' '
                j += 1
            if j + 1 < n:
                out[j] = out[j + 1] = ' '
                j += 2
            i = j
            continue

        # string / character literal
        if code[i] in ('"', "'"):
            quote = code[i]
            if code[i] != '\n':
                out[i] = ' '
            j = i + 1
            while j < n:
                if code[j] == '\\':
                    if code[j] != '\n':
                        out[j] = ' '
                    if j + 1 < n:
                        if code[j + 1] != '\n':
                            out[j + 1] = ' '
                        j += 2
                        continue
                if code[j] == quote:
                    out[j] = ' '
                    j += 1
                    break
                if code[j] != '\n':
                    out[j] = ' '
                j += 1
            i = j
            continue

        i += 1

    return ''.join(out)


def find_matching_brace(masked_code: str, opening_brace_pos: int) -> int:
    if opening_brace_pos < 0 or masked_code[opening_brace_pos] != '{':
        raise ValueError("find_matching_brace() was not given an opening brace")

    depth = 0
    for pos in range(opening_brace_pos, len(masked_code)):
        ch = masked_code[pos]
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                return pos

    raise ValueError(f"Unmatched opening brace at offset {opening_brace_pos}")


def find_functions(content: str) -> List[FunctionInfo]:
    """
    Find ordinary C function definitions. This is intentionally lightweight,
    but unlike the previous script, functions are bounded by matching braces.
    """
    masked = mask_comments_and_strings(content)

    # A pragmatic C function-definition matcher. It permits qualifiers/pointers
    # in the return type and multiline argument lists, while excluding control
    # statements because they do not have a return-type prefix.
    func_re = re.compile(
        r'(?m)^[ \t]*'
        r'(?P<prefix>(?:[A-Za-z_][A-Za-z0-9_]*[ \t\r\n\*]+)+?)'
        r'(?P<name>[A-Za-z_][A-Za-z0-9_]*)'
        r'[ \t\r\n]*\('
        r'(?P<args>[^;{}]*?)'
        r'\)[ \t\r\n]*\{'
    )

    functions = []
    seen_ranges = set()

    for m in func_re.finditer(masked):
        name = m.group('name')
        if name in {'if', 'for', 'while', 'switch'}:
            continue

        open_brace = masked.find('{', m.start(), m.end())
        if open_brace < 0:
            continue

        try:
            close_brace = find_matching_brace(masked, open_brace)
        except ValueError:
            continue

        key = (m.start(), close_brace)
        if key in seen_ranges:
            continue
        seen_ranges.add(key)

        signature = content[m.start():open_brace].strip()
        functions.append(
            FunctionInfo(
                name=name,
                start=m.start(),
                open_brace=open_brace,
                close_brace=close_brace,
                signature=signature,
            )
        )

    functions.sort(key=lambda f: f.start)
    return functions


def enclosing_function(functions: List[FunctionInfo], start: int, end: int) -> Optional[FunctionInfo]:
    candidates = [
        f for f in functions
        if f.body_start <= start and end <= f.body_end
    ]
    if not candidates:
        return None
    # Smallest containing function range wins.
    return min(candidates, key=lambda f: f.close_brace - f.start)


# -----------------------------------------------------------------------------
# OpenACC array-clause helpers
# -----------------------------------------------------------------------------

def split_acc_clause_items(text: str) -> List[str]:
    """Split a simple OpenACC data clause list at top-level commas."""
    items = []
    current = []
    square_depth = 0
    paren_depth = 0

    for ch in text:
        if ch == '[':
            square_depth += 1
        elif ch == ']':
            square_depth = max(0, square_depth - 1)
        elif ch == '(':
            paren_depth += 1
        elif ch == ')':
            paren_depth = max(0, paren_depth - 1)

        if ch == ',' and square_depth == 0 and paren_depth == 0:
            item = ''.join(current).strip()
            if item:
                items.append(item)
            current = []
        else:
            current.append(ch)

    item = ''.join(current).strip()
    if item:
        items.append(item)
    return items




def get_declared_array_bounds_map(full_code: str):
    """
    Infer full OpenACC array sections from ordinary C array declarations.

    This is the fallback required for CAPC/OpenACC regions whose compute pragma
    does not contain present/copy/create clauses, for example:

        double a[2000][2000];
        #pragma acc parallel loop gang vector
        ... a[i][k] ...

    The declaration above becomes:
        a -> a[0:2000][0:2000]

    Existing explicit OpenACC sections remain authoritative and override these
    declaration-derived fallbacks in get_array_bounds_map().
    """
    code = mask_comments_and_strings(full_code)
    result = {}

    # Conservative C declaration recognizer.  It intentionally targets normal
    # scalar/array declarations used by benchmark kernels rather than trying to
    # implement a full C parser.
    type_re = (
        r'(?:static\s+|extern\s+|const\s+|volatile\s+|register\s+|'
        r'restrict\s+|_Alignas\s*\([^)]*\)\s+)*'
        r'(?:(?:unsigned|signed)\s+)?'
        r'(?:(?:long\s+long|long|short)\s+)?'
        r'(?:double|float|int|char|size_t|ptrdiff_t|_Bool)\b'
    )

    for m in re.finditer(
        rf'(?m)^[ \t]*{type_re}([^;]*);',
        code,
    ):
        declarators = m.group(1)
        for am in re.finditer(
            r'\b([A-Za-z_][A-Za-z0-9_]*)\s*'
            r'((?:\[[^\]]+\]\s*)+)',
            declarators,
        ):
            name = am.group(1)
            dims_text = am.group(2)
            dims = re.findall(r'\[\s*([^\]]+)\s*\]', dims_text)
            if not dims:
                continue

            spec = name
            valid = True
            for dim in dims:
                dim = dim.strip()
                if not dim:
                    valid = False
                    break
                spec += f'[0:{dim}]'

            if valid and name not in result:
                result[name] = spec

    return result


def get_array_bounds_map(full_code: str):
    """
    Build var -> array-section specification.

    Priority:
      1. Explicit OpenACC data/update clauses (most precise).
      2. Ordinary C array declarations (fallback).

    This means regions with no data clause on the compute pragma can still be
    isolated correctly from their C body and declarations.
    """
    bounds_map = {}

    clause_matches = re.findall(
        r'\b(?:create|copyin|copyout|copy|present|pcopy|pcopyin|pcopyout|'
        r'present_or_copy|present_or_copyin|present_or_copyout|'
        r'deviceptr)\s*\(([^)]*)\)',
        full_code,
        re.IGNORECASE,
    )

    for match in clause_matches:
        for item in split_acc_clause_items(match):
            var_match = re.match(r'^([A-Za-z_][A-Za-z0-9_]*)', item)
            if not var_match:
                continue
            var_name = var_match.group(1)
            if '[' in item and var_name not in bounds_map:
                bounds_map[var_name] = item.strip()

    update_matches = re.findall(
        r'#\s*pragma\s+acc\s+update[^\n]*?'
        r'\b(?:device|self|host)\s*\(([^)]*)\)',
        full_code,
        re.IGNORECASE,
    )

    for match in update_matches:
        for item in split_acc_clause_items(match):
            var_match = re.match(r'^([A-Za-z_][A-Za-z0-9_]*)', item)
            if not var_match:
                continue
            var_name = var_match.group(1)
            if '[' in item and var_name not in bounds_map:
                bounds_map[var_name] = item.strip()

    # Declaration-derived sections are fallback only.  Never replace a more
    # precise section that appeared explicitly in OpenACC source.
    for name, spec in get_declared_array_bounds_map(full_code).items():
        bounds_map.setdefault(name, spec)

    return bounds_map


def get_target_region_array_specs(target_block: str, bounds_map):
    """
    Determine array variables referenced by the target region.

    OpenACC data clauses are used first. If they only name variables without
    sections, the global bounds map supplies the full section. As a fallback,
    indexed array references in the computational body are used.
    """
    target_vars = []

    clause_matches = re.findall(
        r'\b(?:create|copyin|copyout|copy|present|pcopy|pcopyin|pcopyout|'
        r'present_or_copy|present_or_copyin|present_or_copyout|deviceptr)'
        r'\s*\(([^)]*)\)',
        target_block,
        re.IGNORECASE,
    )

    for match in clause_matches:
        for item in split_acc_clause_items(match):
            var_match = re.match(r'^([A-Za-z_][A-Za-z0-9_]*)', item)
            if var_match:
                var_name = var_match.group(1)
                if var_name not in target_vars:
                    target_vars.append(var_name)

    indexed_vars = re.findall(
        r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\[',
        strip_openacc_pragmas(target_block),
    )
    for var in indexed_vars:
        if var in bounds_map and var not in target_vars:
            target_vars.append(var)

    specs = [bounds_map.get(v, v) for v in target_vars]
    return specs, target_vars



def classify_target_array_accesses(target_body: str, target_vars: List[str]):
    """
    Classify each target array as read-only, write-only, or read-write.

    Returns:
        reads  - arrays whose pre-region value is required (H2D)
        writes - arrays modified/produced by the region (D2H)

    Complete OpenACC pragma groups, including backslash continuation lines,
    are removed before classification so data clauses never look like reads.
    """
    computational_code = strip_openacc_pragmas(target_body)
    computational_lines = [
        line for line in computational_code.splitlines()
        if not line.lstrip().startswith('#')
    ]
    code = mask_comments_and_strings('\n'.join(computational_lines))

    reads = []
    writes = []

    for var in target_vars:
        saw_read = False
        saw_write = False
        pat = re.compile(
            r'\b' + re.escape(var) + r'\s*(?:\[[^\]]*\]\s*)+'
        )

        for m in pat.finditer(code):
            before = code[max(0, m.start() - 4):m.start()]
            after = code[m.end():m.end() + 8]

            if (
                re.search(r'(?:\+\+|--)\s*$', before)
                or re.match(r'\s*(?:\+\+|--)', after)
            ):
                saw_read = True
                saw_write = True
                continue

            op = re.match(
                r'\s*(<<=|>>=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|=)',
                after,
            )
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


def specs_for_vars(var_names: List[str], bounds_map):
    return [bounds_map.get(v, v) for v in var_names]


def strip_openacc_pragmas(code: str) -> str:
    """
    Remove OpenACC pragmas from replay/setup code so prerequisite computation is
    executed on the host only and cannot contaminate target GPU timing.
    Continuation lines belonging to a backslash-continued pragma are removed too.
    """
    out = []
    skipping_continuation = False
    for line in code.splitlines():
        stripped = line.lstrip()
        if skipping_continuation:
            skipping_continuation = line.rstrip().endswith('\\')
            continue
        if re.match(r'^#\s*pragma\s+acc\b', stripped, re.IGNORECASE):
            skipping_continuation = line.rstrip().endswith('\\')
            continue
        out.append(line)
    return '\n'.join(out)



def _normalize_acc_pragma_group(group: List[str]) -> str:
    """Collapse one backslash-continued OpenACC pragma into one legal line."""
    parts = []
    for physical in group:
        part = physical.rstrip()
        if part.endswith('\\'):
            part = part[:-1].rstrip()
        parts.append(part.strip())
    return re.sub(r'\s+', ' ', ' '.join(p for p in parts if p)).strip()


def strip_acc_data_clauses_from_pragma(pragma_text: str) -> str:
    """
    Remove OpenACC data-environment clauses that can allocate or transfer data
    from a compute pragma. The standalone driver performs allocation and H2D/D2H
    explicitly outside the kernel timer.
    """
    clause_names = (
        'copy', 'copyin', 'copyout', 'create', 'present',
        'pcopy', 'pcopyin', 'pcopyout',
        'present_or_copy', 'present_or_copyin', 'present_or_copyout',
        'deviceptr'
    )
    name_re = '|'.join(re.escape(x) for x in clause_names)

    out = []
    i = 0
    n = len(pragma_text)

    while i < n:
        match = re.search(
            rf'\b(?:{name_re})\s*\(',
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
            if pragma_text[pos] == '(':
                depth += 1
            elif pragma_text[pos] == ')':
                depth -= 1
                if depth == 0:
                    pos += 1
                    break
            pos += 1
        i = pos

    cleaned = ''.join(out)
    cleaned = re.sub(r'[ \t]+', ' ', cleaned).strip()
    return cleaned


def rewrite_target_mapping_for_standalone(
    target_block: str,
    target_specs: List[str],
) -> str:
    """
    Normalize OpenACC multiline pragmas and rewrite the target compute directive
    so no implicit allocation/H2D/D2H is hidden inside kernel timing.

    Original data clauses such as copy/copyin/copyout/create/present are removed
    from the first OpenACC compute directive and replaced with present(...).
    Device storage has already been created and required H2D was timed before
    this region.
    """
    lines = target_block.splitlines()
    groups = []
    i = 0

    while i < len(lines):
        line = lines[i]
        if re.match(r'^\s*#\s*pragma\s+acc\b', line, re.IGNORECASE):
            group = [line]
            i += 1
            while group[-1].rstrip().endswith('\\') and i < len(lines):
                group.append(lines[i])
                i += 1
            groups.append(('pragma', group))
        else:
            groups.append(('normal', [line]))
            i += 1

    rewritten = []
    target_rewritten = False

    for kind, group in groups:
        if kind != 'pragma':
            rewritten.extend(group)
            continue

        logical = _normalize_acc_pragma_group(group)
        lower = logical.lower()

        is_compute = bool(
            re.search(r'#\s*pragma\s+acc\s+(?:parallel|kernels|serial)\b', lower)
        )

        # Data-management directives are never the target kernel pragma.
        is_data_mgmt = bool(
            re.search(
                r'#\s*pragma\s+acc\s+(?:enter\s+data|exit\s+data|data\b|update\b|wait\b)',
                lower,
            )
        )

        if is_compute and not is_data_mgmt and not target_rewritten:
            clean = strip_acc_data_clauses_from_pragma(logical)
            if target_specs:
                clean = clean.rstrip() + ' present(' + ', '.join(target_specs) + ')'
            rewritten.append(clean)
            target_rewritten = True
        else:
            # Even untouched ACC pragma groups are normalized so a literal
            # backslash cannot be stranded in generated code.
            rewritten.append(logical)

    return '\n'.join(rewritten)


def extract_safe_inter_region_scalar_setup(gap_code: str) -> str:
    """
    Keep only cheap scalar setup statements from code that originally appeared
    between CAPC regions.

    Why this exists:
      When an earlier CAPC compute region is intentionally skipped, its
      verification/output/post-processing must NOT be replayed.  However, some
      real programs place scalar resets between regions, e.g.:

          dt = 0.0;
          sum = 0.0;
          alpha = beta + 1.0;

      Those cheap scalar statements can be prerequisites for the next target.

    Conservative policy:
      * OpenACC pragmas are discarded.
      * control-flow blocks, printf/fprintf/puts, array accesses, function calls,
        increments/decrements, and preprocessor conditionals are discarded.
      * only a simple scalar assignment statement is retained.

    This deliberately prefers a clean, deterministic standalone setup over
    replaying arbitrary host-side post-processing.
    """
    code = strip_openacc_pragmas(gap_code)
    safe = []

    # Remove comments for recognition, but preserve the original statement text.
    for original in code.splitlines():
        stripped = original.strip()

        if not stripped:
            continue

        if stripped.startswith("#"):
            continue

        if re.match(r"^(for|while|if|switch|do|else)\b", stripped):
            continue

        if re.search(r"\b(?:printf|fprintf|sprintf|snprintf|puts|putchar)\s*\(", stripped):
            continue

        if "[" in stripped or "]" in stripped:
            continue

        if "++" in stripped or "--" in stripped:
            continue

        # Do not keep obvious function calls.  Parentheses are allowed only on
        # the RHS as arithmetic grouping/casts, not as identifier(...).
        no_strings = mask_comments_and_strings(stripped)
        if re.search(r"\b[A-Za-z_][A-Za-z0-9_]*\s*\(", no_strings):
            # Permit common C casts such as (double)x by not rejecting a line
            # merely because it contains parentheses; reject identifier(...).
            continue

        # Simple scalar assignment only.  This also accepts compound arithmetic
        # expressions on the RHS, but not declarations or array statements.
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


def process_prior_capc_regions(prefix_code: str, bounds_map):
    """
    Build cheap host prerequisite replay for a later standalone region.

    FINAL replay policy
    -------------------
    1. Host code before the FIRST earlier CAPC region is preserved.  This is
       where declarations, input initialization, loop-bound setup, etc. usually
       live.

    2. Earlier CAPC regions that are pure array producers/initializers
       (write arrays but read no arrays) are replayed on the host.

    3. Earlier CAPC regions that read arrays are treated as compute regions and
       are NOT replayed serially.  Their written arrays are recorded so a target
       that truly needs such an output can receive cheap deterministic synthetic
       initialization instead.

    4. Host code BETWEEN earlier CAPC regions is NOT replayed wholesale.
       Verification loops, prints, result checking, target updates, and other
       consumers of skipped results are therefore removed.

       Only conservative scalar assignments such as:
           dt = 0.0;
           sum = 0.0;
       are retained.

    This fixes two important failure modes:
      * O(N^3) predecessor kernels being replayed on the CPU (3mm case).
      * verification/output code executing after its producer was skipped
        (vector-arithmetic Region 3/4 case).
    """
    lines = prefix_code.splitlines()

    begin_re = re.compile(
        r'^\s*#\s*pragma\s+capc\s+profitability_region\s+begin\b',
        re.IGNORECASE,
    )
    end_re = re.compile(
        r'^\s*#\s*pragma\s+capc\s+profitability_region\s+end\b',
        re.IGNORECASE,
    )

    # Split prefix into:
    #   leading ordinary host code
    #   [CAPC block, following gap] ...
    leading = []
    region_blocks = []
    gaps = []

    i = 0
    while i < len(lines) and not begin_re.match(lines[i]):
        leading.append(lines[i])
        i += 1

    while i < len(lines):
        # If malformed/unexpected ordinary text appears before another region,
        # treat it as a gap rather than blindly replaying it.
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
            line for line in block_lines
            if not begin_re.match(line) and not end_re.match(line)
        ]
        body = '\n'.join(body_lines)
        full_block = '\n'.join(block_lines)

        _, var_names = get_target_region_array_specs(full_block, bounds_map)
        reads, writes = classify_target_array_accesses(body, var_names)

        if writes and not reads:
            replay = strip_openacc_pragmas(body)
            # Remove the common anti-optimization dummy print without touching
            # meaningful assignments.
            replay = re.sub(
                r'\bprintf\s*\(\s*""\s*\)\s*;',
                '',
                replay,
            )

            out.append(
                '/* Earlier CAPC producer/initializer replayed on host. */'
            )
            out.extend(replay.splitlines())
            replayed_initializers += 1

        else:
            out.append(
                '/* Earlier CAPC compute region omitted from standalone host replay. */'
            )
            skipped_compute += 1
            for var in writes:
                if var not in skipped_outputs:
                    skipped_outputs.append(var)

        # Never replay the complete gap after a CAPC region.  It often contains
        # verification/output code for that region.  Retain only conservative
        # scalar setup that may be needed by a following target.
        if block_index < len(gaps):
            safe_gap = extract_safe_inter_region_scalar_setup(
                '\n'.join(gaps[block_index])
            )
            if safe_gap:
                out.extend(safe_gap.splitlines())

    return (
        '\n'.join(out),
        skipped_outputs,
        replayed_initializers,
        skipped_compute,
    )


def zero_initialization_for_array_spec(spec: str, serial: int) -> str:
    """
    Generate deterministic O(number-of-elements) host zero initialization for
    an OpenACC array section such as A[0:N] or M[0:N][0:N].
    """
    m = re.match(r'^\s*([A-Za-z_][A-Za-z0-9_]*)\s*(.*)$', spec)
    if not m:
        return ''

    var = m.group(1)
    suffix = m.group(2)
    dims = re.findall(r'\[\s*([^:\]]+)\s*:\s*([^\]]+)\]', suffix)
    if not dims:
        return ''

    idx_names = [f'__capc_z{serial}_{d}' for d in range(len(dims))]
    indent = ''
    lines = []

    for (lower, length), idx in zip(dims, idx_names):
        lines.append(
            indent
            + f'for (size_t {idx} = 0; {idx} < (size_t)({length.strip()}); ++{idx}) {{'
        )
        indent += '    '

    access = var + ''.join(
        f'[({lower.strip()}) + {idx}]'
        for (lower, _), idx in zip(dims, idx_names)
    )
    lines.append(indent + f'{access} = 0;')

    for _ in dims:
        indent = indent[:-4]
        lines.append(indent + '}')

    return '\n'.join(lines)


def build_synthetic_input_initialization(
    skipped_outputs: List[str],
    target_read_vars: List[str],
    bounds_map,
):
    """
    Cheaply initialize target inputs whose latest prerequisite producer was an
    omitted expensive CAPC compute region.
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

    return '\n'.join(blocks), unresolved


# -----------------------------------------------------------------------------
# Segment cleanup
# -----------------------------------------------------------------------------

def line_brace_delta(line: str) -> int:
    masked = mask_comments_and_strings(line)
    return masked.count('{') - masked.count('}')


def block_is_unclosed_from_line(start_idx: int, lines: List[str]) -> bool:
    """True when a control block opened here is not closed within this segment."""
    depth = 0
    opened = False
    joined = '\n'.join(lines[start_idx:])
    masked = mask_comments_and_strings(joined)

    for ch in masked:
        if ch == '{':
            depth += 1
            opened = True
        elif ch == '}':
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


def remove_function_from_source(content: str, func: FunctionInfo) -> str:
    """Remove one function definition while preserving all other source text."""
    return content[:func.start] + content[func.close_brace + 1:]


def contains_acc_data_allocation(code: str) -> bool:
    lower = code.lower()
    return bool(re.search(r'#\s*pragma\s+acc\s+(enter\s+data|data\b)', lower))


def strip_capc_markers(code: str) -> str:
    return '\n'.join(
        line for line in code.splitlines()
        if 'profitability_region' not in line
    )


# -----------------------------------------------------------------------------
# Parsing
# -----------------------------------------------------------------------------

def parse_c_file(file_path: str):
    with open(file_path, 'r') as f:
        content = f.read()

    functions = find_functions(content)
    if not functions:
        raise ValueError("No C function definitions could be found.")

    main_func = next((f for f in functions if f.name == 'main'), None)
    if main_func is None:
        raise ValueError("Could not locate main() function in the input file.")

    bounds_map = get_array_bounds_map(content)

    region_pattern = re.compile(
        r'(#pragma\s+capc\s+profitability_region\s+begin[^\n]*\n)'
        r'(.*?)'
        r'(#pragma\s+capc\s+profitability_region\s+end[^\n]*)',
        re.DOTALL | re.IGNORECASE,
    )

    region_matches = list(region_pattern.finditer(content))
    if not region_matches:
        raise ValueError("No '#pragma capc profitability_region begin/end' markers found in file.")

    regions = []
    for idx, match in enumerate(region_matches, start=1):
        fn = enclosing_function(functions, match.start(), match.end())
        if fn is None:
            raise ValueError(
                f"Profitability region {idx} is not contained in a recognized function."
            )

        begin_line = match.group(1).strip()
        body_code = match.group(2).strip()
        end_line = match.group(3).strip()

        id_match = re.search(
            r'begin\s*(?:\(\s*([A-Za-z0-9_]+)\s*\)|\s+([A-Za-z0-9_]+))',
            begin_line,
            re.IGNORECASE,
        )
        if id_match:
            region_id = id_match.group(1) or id_match.group(2)
        else:
            region_id = str(idx)

        full_block = f"{begin_line}\n{body_code}\n{end_line}"
        regions.append(
            RegionInfo(
                region_id=region_id,
                start=match.start(),
                end=match.end(),
                begin_line=begin_line,
                body_code=body_code,
                end_line=end_line,
                full_block=full_block,
                function=fn,
            )
        )

    return content, functions, main_func, regions, bounds_map


# -----------------------------------------------------------------------------
# Standalone generation
# FINAL: Isolated time = GPU runtime initialization + H2D + kernel + D2H
# -----------------------------------------------------------------------------

def clean_directory(output_dir: str):
    if os.path.exists(output_dir):
        print(f"Cleaning previous standalone region files in '{output_dir}'...")
        shutil.rmtree(output_dir)
    os.makedirs(output_dir, exist_ok=True)


def get_function_prefix(content: str, target: RegionInfo) -> str:
    """Everything in the target's enclosing function before the target region."""
    return content[target.function.body_start:target.start]


def get_prior_regions_in_same_function(target: RegionInfo, regions: List[RegionInfo]) -> List[RegionInfo]:
    """Return CAPC regions that execute earlier in the same enclosing function."""
    return [
        r for r in regions
        if r.function.start == target.function.start and r.end <= target.start
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

    support_source = remove_function_from_source(content, main_func)
    support_source = strip_capc_markers(support_source)
    # Any helper invoked during prerequisite setup must run on the host.
    support_source = strip_openacc_pragmas(support_source)

    generated_files = []

    for target in regions:
        filename = os.path.join(
            output_dir, f"region_{target.region_id}_standalone.c"
        )

        # ------------------------------------------------------------------
        # Target data dependence
        # ------------------------------------------------------------------
        array_specs, target_var_names = get_target_region_array_specs(
            target.full_block, bounds_map
        )
        read_vars, write_vars = classify_target_array_accesses(
            target.body_code, target_var_names
        )

        # ------------------------------------------------------------------
        # Host-only prerequisite replay with expensive-predecessor avoidance
        # ------------------------------------------------------------------
        prefix_raw = get_function_prefix(content, target)
        (
            prefix_policy,
            skipped_prior_outputs,
            replayed_initializer_count,
            skipped_compute_count,
        ) = process_prior_capc_regions(prefix_raw, bounds_map)

        prefix_clean = sanitize_c_segment(prefix_policy).strip()
        prefix_clean = strip_openacc_pragmas(prefix_clean).strip()

        synthetic_init, unresolved_synthetic = (
            build_synthetic_input_initialization(
                skipped_prior_outputs,
                read_vars,
                bounds_map,
            )
        )

        h2d_specs = specs_for_vars(read_vars, bounds_map)
        d2h_specs = specs_for_vars(write_vars, bounds_map)

        h2d_str = ", ".join(h2d_specs)
        d2h_str = ", ".join(d2h_specs)
        target_specs_str = ", ".join(array_specs)

        # Ensure original copy/copyin/copyout clauses cannot contaminate the
        # kernel timer; normalize multiline OpenACC pragmas as well.
        target_code = rewrite_target_mapping_for_standalone(
            target.full_block,
            array_specs,
        )

        with open(filename, 'w') as f:
            f.write("#define _GNU_SOURCE\n")
            f.write("#define _POSIX_C_SOURCE 199309L\n")
            f.write("#include <time.h>\n")
            f.write("#include <stdio.h>\n")
            f.write("#include <stdlib.h>\n")
            f.write("#include <openacc.h>\n\n")

            f.write("/* ============================================================\n")
            f.write(" * Original source support code (original main removed)\n")
            f.write(" * ============================================================ */\n")
            f.write(support_source.rstrip() + "\n\n")

            f.write("int main(void)\n{\n")
            f.write("    struct timespec __capc_t_start, __capc_t_end;\n")
            f.write("    double __capc_t_init = 0.0;\n")
            f.write("    double __capc_t_in = 0.0;\n")
            f.write("    double __capc_t_gpu = 0.0;\n")
            f.write("    double __capc_t_out = 0.0;\n\n")

            f.write(
                f"    /* Target Region {target.region_id}; original function: "
                f"{target.function.name}() */\n"
            )

            if prefix_clean:
                f.write("    /* === Host-only input/setup replay (NOT timed) === */\n")
                for line in prefix_clean.splitlines():
                    f.write("    " + line + "\n")
                f.write("\n")
            else:
                f.write("    /* No host-side prerequisite/setup code. */\n\n")

            if synthetic_init:
                f.write(
                    "    /* === Synthetic initialization for inputs whose prior "
                    "expensive CAPC producer was omitted === */\n"
                )
                for line in synthetic_init.splitlines():
                    f.write("    " + line + "\n")
                f.write("\n")

            if unresolved_synthetic:
                f.write(
                    "    /* WARNING: could not synthesize deterministic values for: "
                    + ", ".join(unresolved_synthetic)
                    + ". */\n\n"
                )

            # --------------------------------------------------------------
            # GPU/OpenACC runtime initialization
            # --------------------------------------------------------------
            f.write("    /* === GPU/OpenACC Runtime Initialization === */\n")
            f.write("    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);\n")
            f.write("    acc_init(acc_device_nvidia);\n")
            f.write("    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);\n")
            f.write(
                "    __capc_t_init = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) "
                "+ (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;\n\n"
            )

            # Allocation is intentionally outside the isolated-time sum except
            # for runtime initialization, matching the established methodology.
            if target_specs_str:
                f.write("    /* === Device allocation only (no data movement) === */\n")
                f.write(
                    f"    #pragma acc enter data create({target_specs_str})\n"
                )
                f.write("    #pragma acc wait\n\n")

            if h2d_str:
                f.write("    /* === Required Transfer In (Host -> Device) === */\n")
                f.write("    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);\n")
                f.write(f"    #pragma acc update device({h2d_str})\n")
                f.write("    #pragma acc wait\n")
                f.write("    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);\n")
                f.write(
                    "    __capc_t_in = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) "
                    "+ (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;\n\n"
                )
            else:
                f.write(
                    "    /* H2D skipped: target has no read-before/write input arrays. */\n\n"
                )

            f.write(
                f"    /* === Isolated Kernel Timing for Target Region "
                f"{target.region_id} === */\n"
            )
            f.write("    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);\n\n")

            for line in target_code.splitlines():
                f.write("    " + line + "\n")

            f.write("\n    #pragma acc wait\n")
            f.write("    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);\n")
            f.write(
                "    __capc_t_gpu = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) "
                "+ (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;\n\n"
            )

            if d2h_str:
                f.write("    /* === Required Transfer Out (Device -> Host) === */\n")
                f.write("    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);\n")
                f.write(f"    #pragma acc update self({d2h_str})\n")
                f.write("    #pragma acc wait\n")
                f.write("    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);\n")
                f.write(
                    "    __capc_t_out = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) "
                    "+ (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;\n\n"
                )
            else:
                f.write(
                    "    /* D2H skipped: target does not modify any detected array. */\n\n"
                )

            f.write(
                "    double __capc_t_total = __capc_t_init + __capc_t_in "
                "+ __capc_t_gpu + __capc_t_out;\n"
            )
            f.write(f'    printf("Region {target.region_id} Execution Breakdown:\\n");\n')
            f.write('    printf("  - GPU Initialization : %f seconds\\n", __capc_t_init);\n')
            f.write('    printf("  - Transfer In  (H2D): %f seconds\\n", __capc_t_in);\n')
            f.write('    printf("  - Kernel Time (GPU): %f seconds\\n", __capc_t_gpu);\n')
            f.write('    printf("  - Transfer Out (D2H): %f seconds\\n", __capc_t_out);\n')
            f.write('    printf("  - Isolated Region Time: %f seconds\\n", __capc_t_total);\n\n')

            if target_specs_str:
                f.write(
                    f"    #pragma acc exit data delete({target_specs_str})\n"
                )
                f.write("    #pragma acc wait\n\n")

            f.write(
                "    /* Runtime shutdown is cleanup and is intentionally not "
                "part of isolated time. */\n"
            )
            f.write("    acc_shutdown(acc_device_nvidia);\n\n")
            f.write("    return 0;\n")
            f.write("}\n")

        print(
            f"Generated: {filename} "
            f"[enclosing function: {target.function.name}()]"
        )
        print(
            f"  H2D inputs : {', '.join(read_vars) if read_vars else '(none)'}"
        )
        print(
            f"  D2H outputs: {', '.join(write_vars) if write_vars else '(none)'}"
        )
        print(
            f"  Prior CAPC producer/initializer regions replayed: "
            f"{replayed_initializer_count}"
        )
        print(
            f"  Prior CAPC compute regions skipped: {skipped_compute_count}"
        )
        if synthetic_init:
            synthesized = [
                v for v in read_vars if v in skipped_prior_outputs
            ]
            print(
                f"  Synthetic valid inputs: "
                f"{', '.join(synthesized) if synthesized else '(none)'}"
            )
        if unresolved_synthetic:
            print(
                f"  WARNING unresolved synthetic inputs: "
                f"{', '.join(unresolved_synthetic)}"
            )

        generated_files.append((target.region_id, filename))

    return generated_files



def compile_and_run_regions(
    generated_files,
    compiler: str = "nvc",
    flags=None,
    timeout_seconds: int = 300,
):
    if flags is None:
        flags = [
            "-acc",
            "-mp",
            "-gpu=cc70",
            "--diag_suppress",
            "declared_but_not_referenced",
        ]

    print("\n" + "=" * 50)
    print(" COMPILING & EXECUTING STANDALONE REGIONS (OPENACC)")
    print("=" * 50)

    for target_id, c_file in generated_files:
        exe_file = os.path.splitext(c_file)[0]
        compile_cmd = [compiler] + flags + [c_file, "-o", exe_file]

        print(f"\n[Compiling Region {target_id}]: {' '.join(compile_cmd)}")

        comp_process = subprocess.run(
            compile_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        compiler_output = "\n".join(
            x for x in (
                comp_process.stdout.strip(),
                comp_process.stderr.strip(),
            )
            if x
        )
        if compiler_output:
            print(f"[Compiler Output]:\n{compiler_output}")

        if comp_process.returncode != 0:
            print(f"❌ Compilation failed for Region {target_id}!")
            continue

        print(f"[Running Region {target_id}]: {exe_file}")

        try:
            run_process = subprocess.run(
                [os.path.abspath(exe_file)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=timeout_seconds,
            )
        except subprocess.TimeoutExpired:
            print(
                f"❌ Execution timed out for Region {target_id} "
                f"after {timeout_seconds} seconds."
            )
            continue

        if run_process.returncode == 0:
            print(f"✅ {run_process.stdout.strip()}")
            if run_process.stderr.strip():
                print(f"[Runtime stderr]:\n{run_process.stderr.strip()}")
        else:
            print(f"❌ Execution failed for Region {target_id}!")
            if run_process.stdout.strip():
                print(run_process.stdout.strip())
            if run_process.stderr.strip():
                print(run_process.stderr.strip())


def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_standalone_regions_openacc_FINAL.py <input_benchmark.c>")
        sys.exit(1)

    input_file = sys.argv[1]
    if not os.path.isfile(input_file):
        print(f"Error: input file not found: {input_file}")
        sys.exit(1)

    try:
        content, functions, main_func, regions, bounds_map = parse_c_file(input_file)

        print("Detected profitability regions:")
        for r in regions:
            line_no = content.count('\n', 0, r.start) + 1
            print(
                f"  Region {r.region_id}: line {line_no}, "
                f"function {r.function.name}()"
            )

        generated = generate_standalone_files(
            content,
            functions,
            main_func,
            regions,
            bounds_map,
        )
        compile_and_run_regions(generated)

    except Exception as exc:
        print(f"Error: {exc}")
        sys.exit(1)


if __name__ == "__main__":
    main()