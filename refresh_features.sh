#!/bin/bash
# ============================================================================
# CAPC FEATURE REFRESH (STANDALONE)
#
# Usage:
#     ./refresh_features.sh <existing_features.csv>
#
# Re-runs the AST + LLVM feature extractor over every sweep variant and
# merges the freshly extracted feature columns into the given CSV,
# matched on (FileName, RegionID).
#
# Timing columns already present in the CSV
# (SerialTime, OpenMP3Time, OpenMP45*, OpenACC*) are NEVER touched.
#
# This script does nothing else. There is no Phase 2 or Phase 3 here,
# so there is nothing for control flow to accidentally fall into.
# ============================================================================

set -u
set -o pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <existing_features.csv>"
    exit 1
fi

TARGET_CSV="$1"

if [[ ! -f "$TARGET_CSV" ]]; then
    echo "ERROR: File not found: $TARGET_CSV"
    exit 1
fi

INPUT_DIR="./outputs/serial/CAPC"
FEATURE_EXTRACTOR="./build/ProfitabilityTool"

SWEEP_VALUES=()
for ((v = 1; v <= 200; v++)); do
    SWEEP_VALUES+=("$v")
done
for BASE in 100 1000 10000 100000 1000000 10000000 100000000; do
    for ((MULT = 2; MULT <= 10; MULT++)); do
        VALUE=$((MULT * BASE))
        if [ "$VALUE" -gt 200 ]; then
            SWEEP_VALUES+=("$VALUE")
        fi
    done
done

WORK_DIR=$(mktemp -d ./capc_refresh_work_XXXXXX)
trap 'rm -rf "$WORK_DIR"' EXIT INT TERM

FEATURE_VARIANT_DIR="$WORK_DIR/feature_variants"
FEATURE_OUTPUT_DIR="$WORK_DIR/feature_output"
FEATURE_EXTRACTOR_CSV="$FEATURE_OUTPUT_DIR/features.csv"
TEMP_LOG_DIR="$WORK_DIR/logs"

mkdir -p "$FEATURE_VARIANT_DIR" "$FEATURE_OUTPUT_DIR" "$TEMP_LOG_DIR"

echo "======================================================================"
echo "Validating environment"
echo "======================================================================"

if [ ! -x "$FEATURE_EXTRACTOR" ]; then
    echo "ERROR: Feature extractor not found or not executable: $FEATURE_EXTRACTOR"
    exit 1
fi

if [ ! -d "$INPUT_DIR" ]; then
    echo "ERROR: Dataset directory not found: $INPUT_DIR"
    exit 1
fi

for cmd in python3 clang; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "ERROR: Required command not found: $cmd"
        exit 1
    fi
done

echo "Environment OK."

# ----------------------------------------------------------------------------
# Detect loop-bound #define parameters (same logic as build_dataset.sh)
# ----------------------------------------------------------------------------

detect_loop_bound_defines()
{
    local FILE="$1"
    python3 - "$FILE" <<'PY'
import re, sys
path = sys.argv[1]
with open(path, "r", errors="ignore") as f:
    text = f.read()

define_pattern = re.compile(
    r'^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z0-9_]*)[ \t]+'
    r'\(?[ \t]*[0-9]+[ \t]*(?:[uUlL]+)?[ \t]*\)?[ \t]*(?://.*)?$',
    re.MULTILINE
)
numeric_defines = {m.group(1) for m in define_pattern.finditer(text)}
if not numeric_defines:
    sys.exit(0)

region_pattern = re.compile(
    r'^[ \t]*#[ \t]*pragma[ \t]+capc[ \t]+profitability_region[ \t]+begin'
    r'(.*?)'
    r'^[ \t]*#[ \t]*pragma[ \t]+capc[ \t]+profitability_region[ \t]+end',
    re.MULTILINE | re.DOTALL
)
regions = [m.group(1) for m in region_pattern.finditer(text)]
if not regions:
    sys.exit(0)

def extract_for_headers(region):
    headers = []
    pos = 0
    while True:
        m = re.search(r'\bfor\s*\(', region[pos:])
        if not m:
            break
        start = pos + m.start()
        open_paren = region.find('(', start)
        if open_paren < 0:
            break
        depth = 0
        i = open_paren
        while i < len(region):
            ch = region[i]
            if ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
                if depth == 0:
                    headers.append(region[open_paren + 1:i])
                    pos = i + 1
                    break
            i += 1
        else:
            break
    return headers

used = set()
identifier_pattern = re.compile(r'\b[A-Za-z_][A-Za-z0-9_]*\b')
for region in regions:
    for header in extract_for_headers(region):
        ids = set(identifier_pattern.findall(header))
        used.update(ids & numeric_defines)

for name in sorted(used):
    print(name)
PY
}

modify_define()
{
    local INPUT="$1" OUTPUT="$2" PARAMETER="$3" VALUE="$4"
    python3 - "$INPUT" "$OUTPUT" "$PARAMETER" "$VALUE" <<'PY'
import re, sys
source, destination, parameter, value = sys.argv[1:5]
with open(source, "r", errors="ignore") as f:
    text = f.read()
pattern = re.compile(
    r'^([ \t]*#[ \t]*define[ \t]+' + re.escape(parameter) + r'[ \t]+)'
    r'(\(?[ \t]*[0-9]+[ \t]*(?:[uUlL]+)?[ \t]*\)?)(.*)$',
    re.MULTILINE
)
def replace(m):
    return m.group(1) + value + m.group(3)
new_text, count = pattern.subn(replace, text, count=1)
if count == 0:
    print(f"Could not find numeric #define {parameter}", file=sys.stderr)
    sys.exit(1)
with open(destination, "w") as f:
    f.write(new_text)
PY
}

# ----------------------------------------------------------------------------
# Run the extractor over every benchmark/parameter/value
# ----------------------------------------------------------------------------

echo
echo "======================================================================"
echo "Extracting fresh static features"
echo "======================================================================"

RESOURCE_DIR=$(clang -print-resource-dir)
FEATURE_EXTRACTOR_ABS=$(readlink -f "$FEATURE_EXTRACTOR")

rm -f "$FEATURE_EXTRACTOR_CSV"

shopt -s nullglob
SOURCE_FILES=("$INPUT_DIR"/*.c)
shopt -u nullglob

if [ ${#SOURCE_FILES[@]} -eq 0 ]; then
    echo "ERROR: No C source files found in: $INPUT_DIR"
    exit 1
fi

for SOURCE in "${SOURCE_FILES[@]}"; do
    BASE=$(basename "$SOURCE")
    BENCHMARK="${BASE%.c}"
    BENCHMARK="${BENCHMARK%_serial}"

    echo
    echo "Benchmark: $BENCHMARK"

    mapfile -t PARAMETERS < <(detect_loop_bound_defines "$SOURCE")

    if [ ${#PARAMETERS[@]} -eq 0 ]; then
        echo "  No numeric #define used in CAPC loop bounds. Skipping."
        continue
    fi

    for PARAMETER in "${PARAMETERS[@]}"; do
        echo "  Parameter: $PARAMETER"

        for VALUE in "${SWEEP_VALUES[@]}"; do
            VARIANT="$FEATURE_VARIANT_DIR/${BENCHMARK}_serial_${PARAMETER}_${VALUE}.c"
            LOG="$TEMP_LOG_DIR/${BENCHMARK}_feature_${PARAMETER}_${VALUE}.log"

            if ! modify_define "$SOURCE" "$VARIANT" "$PARAMETER" "$VALUE"; then
                echo "    Failed to modify $PARAMETER. Stopping larger values."
                break
            fi

            VARIANT_ABS=$(readlink -f "$VARIANT")

            (
                cd "$FEATURE_OUTPUT_DIR" || exit 1
                "$FEATURE_EXTRACTOR_ABS" "$VARIANT_ABS" -- "-resource-dir=$RESOURCE_DIR"
            ) > "$LOG" 2>&1

            if [ $? -ne 0 ]; then
                echo "    [$PARAMETER=$VALUE] Feature extraction FAILED. Stopping larger values."
                break
            fi

            rm -f "$LOG"
        done
    done
done

if [ ! -f "$FEATURE_EXTRACTOR_CSV" ]; then
    echo "ERROR: Feature extractor never produced: $FEATURE_EXTRACTOR_CSV"
    exit 1
fi

echo
echo "Fresh extraction complete: $FEATURE_EXTRACTOR_CSV"

# ----------------------------------------------------------------------------
# Merge fresh features into the target CSV, preserving timing columns
# ----------------------------------------------------------------------------

echo
echo "======================================================================"
echo "Merging into $TARGET_CSV"
echo "======================================================================"

python3 - "$TARGET_CSV" "$FEATURE_EXTRACTOR_CSV" <<'PY'
import csv, os, sys

old_csv, new_csv = sys.argv[1], sys.argv[2]

TIME_COLUMNS = [
    "SerialTime", "OpenMP3Time",
    "OpenMP45ResidentTime", "OpenMP45ObservedTime", "OpenMP45IsolatedTime",
    "OpenACCResidentTime", "OpenACCObservedTime", "OpenACCIsolatedTime",
]

def fail(msg):
    print(f"ERROR: {msg}")
    sys.exit(1)

def load(path):
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        return list(reader.fieldnames or []), list(reader)

def key_of(row):
    return (os.path.basename(row.get("FileName", "")), row.get("RegionID", ""))

old_fields, old_rows = load(old_csv)
new_fields, new_rows = load(new_csv)

missing = [c for c in TIME_COLUMNS if c not in old_fields]
if missing:
    fail("Existing CSV is missing timing column(s): " + ", ".join(missing))

def clean(row):
    return {k: v for k, v in row.items() if k is not None}

old_by_key = {key_of(r): clean(r) for r in old_rows}
new_by_key = {key_of(r): clean(r) for r in new_rows}

merged_fields = list(new_fields)
for c in TIME_COLUMNS:
    if c not in merged_fields:
        merged_fields.append(c)

merged_rows = []
updated = appended = kept_stale = 0
seen = set()

for key, new_row in new_by_key.items():
    seen.add(key)
    old_row = old_by_key.get(key)
    merged = dict(new_row)
    if old_row is not None:
        for c in TIME_COLUMNS:
            merged[c] = old_row.get(c, "")
        updated += 1
    else:
        for c in TIME_COLUMNS:
            merged[c] = ""
        appended += 1
    merged_rows.append(merged)

for key, old_row in old_by_key.items():
    if key in seen:
        continue
    merged_rows.append({c: old_row.get(c, "") for c in merged_fields})
    kept_stale += 1

temp = old_csv + f".tmp.{os.getpid()}"
try:
    with open(temp, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=merged_fields, extrasaction='ignore')
        writer.writeheader()
        writer.writerows(merged_rows)
        f.flush()
        os.fsync(f.fileno())
    os.replace(temp, old_csv)
except Exception:
    if os.path.exists(temp):
        os.remove(temp)
    raise

print(f"Updated {updated} row(s), appended {appended} new row(s), "
      f"kept {kept_stale} stale row(s) unchanged.")
PY

if [ $? -ne 0 ]; then
    echo "ERROR: Merge failed. $TARGET_CSV was not modified."
    exit 1
fi

echo
echo "======================================================================"
echo "DONE. Timing columns were preserved. Feature columns are up to date."
echo "======================================================================"