#!/bin/bash

# ============================================================================
# CAPC MASTER DATASET BUILDER
#
# Run from ProfitabilityTool root:
#
#     ./build_dataset.sh <output.csv>
#
# Pipeline:
#   1. Detect numeric #define variables used in loop bounds inside
#      CAPC profitability regions.
#   2. Sweep each detected variable independently.
#   3. Generate temporary serial variants.
#   4. Run AST + LLVM feature extractor.
#   5. Create the user-selected output CSV.
#   6. Run Serial and OpenMP3 annotation scripts.
#   7. Read Benchmark + Parameter + Value configurations from the output CSV.
#   8. For each configuration:
#          Serial
#          OpenMP3
#          OpenMP4.5
#          OpenACC
#   9. Extract per-region timing.
#  10. After all paradigms succeed for one configuration, immediately
#      commit its timings atomically to the output CSV and delete result files.
#
# Failure policy:
#
#   If any paradigm fails for:
#
#       benchmark + parameter + value
#
#   then:
#
#       - remaining paradigms for that value are skipped
#       - all larger values for that benchmark+parameter are skipped
#       - next benchmark+parameter continues normally
#
# Temporary files:
#
#   Modified programs, binaries, profiler intermediates and successful
#   run logs are temporary and deleted automatically.
#
# Persistent files:
#
#   user-selected output CSV
#   <output.csv>.before_execution_times.bak
#   dataset_logs/   (ONLY compact error logs)
# ============================================================================

set -u
set -o pipefail

FEATURES_CSV=""


# ============================================================================
# CONFIGURATION
# ============================================================================

ulimit -s unlimited

# ----------------------------------------------------------------------------
# Static feature extraction
# ----------------------------------------------------------------------------

INPUT_DIR="./outputs/serial/CAPC4"

FEATURE_EXTRACTOR="./build/ProfitabilityTool"


# ----------------------------------------------------------------------------
# Source directories
# ----------------------------------------------------------------------------

SERIAL_SOURCE_DIR="./outputs/serial/CAPC4"

OMP3_SOURCE_DIR="./outputs/omp3/CAPC4"

OMP45_SOURCE_DIR="./outputs/omp45/CAPC4"

OPENACC_SOURCE_DIR="./outputs/openacc/CAPC4"


# ----------------------------------------------------------------------------
# CPU annotated directories
# ----------------------------------------------------------------------------

ANNOTATED_ROOT="./outputs/annotated"

SERIAL_ANNOTATED_DIR="$ANNOTATED_ROOT/serial/CAPC4"

OMP3_ANNOTATED_DIR="$ANNOTATED_ROOT/omp3/CAPC4"


# ----------------------------------------------------------------------------
# Annotation / profiling scripts
# ----------------------------------------------------------------------------

SERIAL_ANNOTATOR="./annotate_serial_timing.py"

OMP3_ANNOTATOR="./annotate_omp3_timing.py"

OMP45_PROFILER="./annotate_omp45_timing.py"

OPENACC_PROFILER="./annotate_acc_timing.py"


# ----------------------------------------------------------------------------
# Sweep values
#
# Dense low-range sweep followed by progressively larger increments:
#
#   1       -> 200          increment 1
#   300     -> 1000         increment 100
#   2000    -> 10000        increment 1000
#   20000   -> 100000       increment 10000
#   200000  -> 1000000      increment 100000
#   ...
#
# Boundary values are included only once.
#
# Generated sequence:
#
#   1 2 3 ... 199 200
#   300 400 ... 900 1000
#   2000 3000 ... 9000 10000
#   20000 30000 ... 90000 100000
#   ...
# ----------------------------------------------------------------------------

SWEEP_VALUES=()

# --------------------------------------------------------------------------
# 1, 2, 3, ..., 200
# --------------------------------------------------------------------------

for ((v = 1; v <= 200; v++))
do
    SWEEP_VALUES+=("$v")
done


# --------------------------------------------------------------------------
# 300, 400, ..., 1000
# 2000, 3000, ..., 10000
# 20000, 30000, ..., 100000
# ...
# --------------------------------------------------------------------------

for BASE in \
    100 \
    1000 \
    10000 \
    100000 \
    1000000 \
    10000000 \
    100000000
do
    for ((MULT = 2; MULT <= 10; MULT++))
    do
        VALUE=$((MULT * BASE))

        # Avoid adding a value that is already present.
        if [ "$VALUE" -gt 200 ]; then
            SWEEP_VALUES+=("$VALUE")
        fi
    done
done


# ----------------------------------------------------------------------------
# Maximum time for ONE execution / GPU profiler invocation.
#
# 0 = no timeout
# ----------------------------------------------------------------------------

RUN_TIMEOUT=10000


# ----------------------------------------------------------------------------
# Persistent compact error logs
# ----------------------------------------------------------------------------

LOG_ROOT="./dataset_logs"

COMPILE_ERROR_DIR="$LOG_ROOT/compilation_errors"

EXEC_ERROR_DIR="$LOG_ROOT/execution_errors"

OTHER_ERROR_DIR="$LOG_ROOT/other_errors"

LOG_HEAD_LINES=30

LOG_TAIL_LINES=100


# ============================================================================
# TEMPORARY WORKSPACE
# ============================================================================

WORK_DIR=$(mktemp -d ./capc_dataset_work_XXXXXX)

TEMP_SOURCE_DIR="$WORK_DIR/sources"

TEMP_BIN_DIR="$WORK_DIR/bin"

TEMP_LOG_DIR="$WORK_DIR/logs"

TEMP_RESULT_DIR="$WORK_DIR/results"

FEATURE_VARIANT_DIR="$WORK_DIR/feature_variants"

FEATURE_OUTPUT_DIR="$WORK_DIR/feature_output"

FEATURE_EXTRACTOR_CSV="$FEATURE_OUTPUT_DIR/features.csv"


mkdir -p \
    "$TEMP_SOURCE_DIR" \
    "$TEMP_BIN_DIR" \
    "$TEMP_LOG_DIR" \
    "$TEMP_RESULT_DIR" \
    "$FEATURE_VARIANT_DIR" \
    "$FEATURE_OUTPUT_DIR"


mkdir -p \
    "$COMPILE_ERROR_DIR" \
    "$EXEC_ERROR_DIR" \
    "$OTHER_ERROR_DIR"


cleanup()
{
    rm -rf "$WORK_DIR"
}


trap cleanup EXIT INT TERM


# ============================================================================
# COMPACT ERROR LOG
#
# Successful logs are deleted.
#
# Failed logs retain:
#
#   first 30 lines
#   last 100 lines
#
# This prevents compiler/profiler logs from consuming excessive disk space.
# ============================================================================

save_small_log()
{
    local SOURCE="$1"
    local DEST="$2"

    mkdir -p "$(dirname "$DEST")"

    if [ -f "$SOURCE" ]; then
        tail -n 100 "$SOURCE" > "$DEST"
    else
        : > "$DEST"
    fi
}


# ============================================================================
# VALIDATION
# ============================================================================

validate_environment()
{
    echo
    echo "======================================================================"
    echo "Validating CAPC dataset environment"
    echo "======================================================================"


    if [ ! -x "$FEATURE_EXTRACTOR" ]; then

        echo "ERROR: Feature extractor not found or not executable:"
        echo
        echo "    $FEATURE_EXTRACTOR"

        exit 1

    fi


    if [ ! -d "$INPUT_DIR" ]; then

        echo "ERROR: Dataset directory not found:"
        echo
        echo "    $INPUT_DIR"

        exit 1

    fi


    for SCRIPT in \
        "$SERIAL_ANNOTATOR" \
        "$OMP3_ANNOTATOR" \
        "$OMP45_PROFILER" \
        "$OPENACC_PROFILER"
    do

        if [ ! -f "$SCRIPT" ]; then

            echo "ERROR: Script not found:"
            echo
            echo "    $SCRIPT"

            exit 1

        fi

    done


    for cmd in python3 gcc nvc clang timeout
    do

        if ! command -v "$cmd" >/dev/null 2>&1; then

            echo "ERROR: Required command not found:"
            echo
            echo "    $cmd"

            exit 1

        fi

    done


    echo
    echo "Environment validation successful."
}


# ============================================================================
# DETECT LOOP-BOUND #define PARAMETERS
#
# Goal:
#
# Do NOT sweep every numeric #define.
#
# Example:
#
#   #define N        2000
#   #define TSTEPS   500
#   #define SCALE    9
#
# Region:
#
#   #pragma capc profitability_region begin
#
#   for (t = 0; t < TSTEPS; t++)
#       for (i = 0; i < N; i++)
#           A[i] = B[i] / SCALE;
#
#   #pragma capc profitability_region end
#
# Result:
#
#   N
#   TSTEPS
#
# SCALE is ignored.
#
#
# Detection strategy:
#
#   1. Find numeric object-like #defines.
#   2. Extract CAPC profitability regions.
#   3. Extract for(...) headers from those regions.
#   4. Find which numeric macros occur in those for headers.
#
# Note:
#
# This is intentionally conservative and targeted to normal C loop syntax.
# ============================================================================

detect_loop_bound_defines()
{
    local FILE="$1"


    python3 - "$FILE" <<'PY'

import re
import sys


path = sys.argv[1]


with open(path, "r", errors="ignore") as f:

    text = f.read()


# ----------------------------------------------------------------------
# Numeric object-like #defines.
# ----------------------------------------------------------------------

define_pattern = re.compile(
    r'^[ \t]*#[ \t]*define[ \t]+'
    r'([A-Za-z_][A-Za-z0-9_]*)'
    r'[ \t]+'
    r'\(?[ \t]*'
    r'[0-9]+'
    r'[ \t]*(?:[uUlL]+)?'
    r'[ \t]*\)?'
    r'[ \t]*(?://.*)?$',
    re.MULTILINE
)


numeric_defines = {
    match.group(1)
    for match in define_pattern.finditer(text)
}


if not numeric_defines:

    sys.exit(0)


# ----------------------------------------------------------------------
# Extract CAPC profitability regions.
#
# Handles:
#
#   #pragma capc profitability_region begin
#        ...
#   #pragma capc profitability_region end
# ----------------------------------------------------------------------

region_pattern = re.compile(
    r'^[ \t]*#[ \t]*pragma[ \t]+'
    r'capc[ \t]+profitability_region[ \t]+begin'
    r'(.*?)'
    r'^[ \t]*#[ \t]*pragma[ \t]+'
    r'capc[ \t]+profitability_region[ \t]+end',
    re.MULTILINE | re.DOTALL
)


regions = [
    match.group(1)
    for match in region_pattern.finditer(text)
]


if not regions:

    sys.exit(0)


# ----------------------------------------------------------------------
# Extract text inside for(...)
#
# We use a small parenthesis-aware scanner instead of relying entirely
# on regex because loop headers may contain nested parentheses.
# ----------------------------------------------------------------------

def extract_for_headers(region):

    headers = []

    pos = 0

    while True:

        match = re.search(r'\bfor\s*\(', region[pos:])

        if not match:

            break

        start = pos + match.start()

        open_paren = region.find(
            '(',
            start
        )

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

                    headers.append(
                        region[open_paren + 1:i]
                    )

                    pos = i + 1

                    break

            i += 1

        else:

            break

    return headers


used = set()


identifier_pattern = re.compile(
    r'\b[A-Za-z_][A-Za-z0-9_]*\b'
)


for region in regions:

    for header in extract_for_headers(region):

        identifiers = set(
            identifier_pattern.findall(header)
        )

        used.update(
            identifiers & numeric_defines
        )


for name in sorted(used):

    print(name)

PY
}


# ============================================================================
# MODIFY ONE NUMERIC #define
#
# Supports examples:
#
#   #define N 100
#   #define N (100)
#   #define N 100L
#   #define N 100UL
# ============================================================================

modify_define()
{
    local INPUT="$1"

    local OUTPUT="$2"

    local PARAMETER="$3"

    local VALUE="$4"


    python3 - \
        "$INPUT" \
        "$OUTPUT" \
        "$PARAMETER" \
        "$VALUE" <<'PY'

import re
import sys


source = sys.argv[1]

destination = sys.argv[2]

parameter = sys.argv[3]

value = sys.argv[4]


with open(source, "r", errors="ignore") as f:

    text = f.read()


pattern = re.compile(
    r'^([ \t]*#[ \t]*define[ \t]+'
    + re.escape(parameter)
    + r'[ \t]+)'
      r'(\(?[ \t]*[0-9]+[ \t]*(?:[uUlL]+)?[ \t]*\)?)'
      r'(.*)$',
    re.MULTILINE
)


def replace(match):

    return (
        match.group(1)
        + value
        + match.group(3)
    )


new_text, count = pattern.subn(
    replace,
    text,
    count=1
)


if count == 0:

    print(
        f"Could not find numeric #define {parameter}",
        file=sys.stderr
    )

    sys.exit(1)


with open(destination, "w") as f:

    f.write(new_text)

PY
}


# ============================================================================
# PHASE 1
#
# FEATURE SWEEP + STATIC FEATURE EXTRACTION
#
#
# Input:
#
#     Dataset Files/adi.c
#
#
# Suppose N is detected.
#
# Temporary files:
#
#     adi_serial_N_1.c
#     adi_serial_N_10.c
#     adi_serial_N_100.c
#     ...
#
#
# Feature extractor command:
#
# ./build/ProfitabilityTool file.c \
#     -- \
#     -resource-dir=$(clang -print-resource-dir)
#
#
# Result:
#
#     features.csv
# ============================================================================

generate_static_features()
{
    echo
    echo "======================================================================"
    echo "PHASE 1: STATIC FEATURE EXTRACTION"
    echo "======================================================================"
    echo "Output CSV:"
    echo "    $FEATURES_CSV"


    local RESOURCE_DIR

    local FEATURE_EXTRACTOR_ABS


    RESOURCE_DIR=$(clang -print-resource-dir)

    FEATURE_EXTRACTOR_ABS=$(readlink -f "$FEATURE_EXTRACTOR")


    # The feature extractor writes a file named features.csv in its current
    # working directory. Run it from an isolated temporary directory so an
    # existing project-level features.csv is never touched.
    rm -f "$FEATURE_EXTRACTOR_CSV"


    shopt -s nullglob

    local SOURCE_FILES=("$INPUT_DIR"/*.c)

    shopt -u nullglob


    if [ ${#SOURCE_FILES[@]} -eq 0 ]; then

        echo
        echo "ERROR: No C source files found in:"
        echo
        echo "    $INPUT_DIR"

        exit 1

    fi


    for SOURCE in "${SOURCE_FILES[@]}"
    do

        local BASE

        local BENCHMARK


        BASE=$(basename "$SOURCE")

        BENCHMARK="${BASE%.c}"

        BENCHMARK="${BENCHMARK%_serial}"


        echo
        echo "----------------------------------------------------------------------"
        echo "Benchmark : $BENCHMARK"
        echo "----------------------------------------------------------------------"


        mapfile -t PARAMETERS < <(
            detect_loop_bound_defines "$SOURCE"
        )


        if [ ${#PARAMETERS[@]} -eq 0 ]; then

            echo "No numeric #define used in CAPC loop bounds."
            echo "Skipping benchmark."

            continue

        fi


        echo "Detected sweep parameter(s):"

        for P in "${PARAMETERS[@]}"
        do

            echo "    $P"

        done


        for PARAMETER in "${PARAMETERS[@]}"
        do

            echo
            echo "  Parameter: $PARAMETER"


            for VALUE in "${SWEEP_VALUES[@]}"
            do

                local VARIANT

                local VARIANT_ABS

                local LOG


                VARIANT="$FEATURE_VARIANT_DIR/"\
"${BENCHMARK}_serial_${PARAMETER}_${VALUE}.c"


                LOG="$TEMP_LOG_DIR/"\
"${BENCHMARK}_feature_${PARAMETER}_${VALUE}.log"


                if ! modify_define \
                    "$SOURCE" \
                    "$VARIANT" \
                    "$PARAMETER" \
                    "$VALUE"
                then

                    echo
                    echo "  Failed to modify $PARAMETER."
                    echo "  Larger values for this parameter will not be generated."

                    break

                fi


                VARIANT_ABS=$(readlink -f "$VARIANT")


                echo
                echo "  [$PARAMETER=$VALUE] Extracting features..."


                (
                    cd "$FEATURE_OUTPUT_DIR" || exit 1

                    "$FEATURE_EXTRACTOR_ABS" \
                        "$VARIANT_ABS" \
                        -- \
                        "-resource-dir=$RESOURCE_DIR"
                ) > "$LOG" 2>&1


                local STATUS=$?


                if [ $STATUS -ne 0 ]; then

                    echo "  Feature extraction FAILED."


                    save_small_log \
                        "$LOG" \
                        "$OTHER_ERROR_DIR/"\
"${BENCHMARK}_feature_${PARAMETER}_${VALUE}.log"


                    echo "  Larger values for this parameter will be skipped."

                    break

                fi


                rm -f "$LOG"

            done

        done

    done


    if [ ! -f "$FEATURE_EXTRACTOR_CSV" ]; then

        echo
        echo "ERROR:"
        echo "Feature extractor did not create its temporary CSV:"
        echo
        echo "    $FEATURE_EXTRACTOR_CSV"

        exit 1

    fi


    # Move the completed static-feature CSV to the filename selected by the
    # user. The destination was checked at startup and cannot already exist.
    mv "$FEATURE_EXTRACTOR_CSV" "$FEATURES_CSV" || {

        echo
        echo "ERROR: Could not move generated feature CSV to:"
        echo "    $FEATURES_CSV"

        exit 1
    }


    # Preserve one feature-only backup before any timings are inserted.
    cp -p \
        "$FEATURES_CSV" \
        "${FEATURES_CSV}.before_execution_times.bak" || {

        echo
        echo "ERROR: Could not create feature-only backup:"
        echo "    ${FEATURES_CSV}.before_execution_times.bak"

        exit 1
    }


    echo
    echo "Feature extraction complete."
    echo
    echo "Generated:"
    echo "    $FEATURES_CSV"
    echo
    echo "Feature-only backup:"
    echo "    ${FEATURES_CSV}.before_execution_times.bak"
}

# ============================================================================
# PHASE 2
#
# SERIAL AND OPENMP3 ANNOTATION
#
# Both annotation scripts receive their source directory.
# ============================================================================

run_cpu_annotation()
{
    echo
    echo "======================================================================"
    echo "PHASE 2: SERIAL + OPENMP3 ANNOTATION"
    echo "======================================================================"


    mkdir -p "$SERIAL_ANNOTATED_DIR"

    mkdir -p "$OMP3_ANNOTATED_DIR"


    local LOG


    # ------------------------------------------------------------------
    # Serial
    # ------------------------------------------------------------------

    echo
    echo "[Serial] Running annotation..."


    LOG="$TEMP_LOG_DIR/serial_annotation.log"


    python3 \
        "$SERIAL_ANNOTATOR" \
        "$SERIAL_SOURCE_DIR" \
        > "$LOG" 2>&1


    if [ $? -ne 0 ]; then

        echo "Serial annotation FAILED."


        save_small_log \
            "$LOG" \
            "$OTHER_ERROR_DIR/serial_annotation.log"


        exit 1

    fi


    rm -f "$LOG"


    echo "[Serial] Annotation successful."


    # ------------------------------------------------------------------
    # OpenMP3
    # ------------------------------------------------------------------

    echo
    echo "[OpenMP3] Running annotation..."


    LOG="$TEMP_LOG_DIR/omp3_annotation.log"


    python3 \
        "$OMP3_ANNOTATOR" \
        "$OMP3_SOURCE_DIR" \
        > "$LOG" 2>&1


    if [ $? -ne 0 ]; then

        echo "OpenMP3 annotation FAILED."


        save_small_log \
            "$LOG" \
            "$OTHER_ERROR_DIR/omp3_annotation.log"


        exit 1

    fi


    rm -f "$LOG"


    echo "[OpenMP3] Annotation successful."
}


# ============================================================================
# BUILD EXPERIMENT LIST FROM features.csv
#
# Example filename:
#
#     jacobi-1D_serial_N_1000.c
#
# Becomes:
#
#     jacobi-1D|N|1000
#
#
# The set removes duplicate entries caused by multiple RegionID rows.
# ============================================================================

build_experiment_list()
{
    local EXPERIMENT_FILE="$WORK_DIR/experiments.txt"

    python3 - "$FEATURES_CSV" \
            > "$EXPERIMENT_FILE" <<'PY'

import csv
import os
import re
import sys


csv_file = sys.argv[1]


# ----------------------------------------------------------------------
# Timing columns required for a configuration to be COMPLETE.
# ----------------------------------------------------------------------

TIME_COLUMNS = [
    "SerialTime",
    "OpenMP3Time",
    "OpenMP45ResidentTime",
    "OpenMP45ObservedTime",
    "OpenMP45IsolatedTime",
    "OpenACCResidentTime",
    "OpenACCObservedTime",
    "OpenACCIsolatedTime",
]


# ----------------------------------------------------------------------
# IMPORTANT:
#
# Do NOT sort configurations.
#
# The CSV is already in the original sweep order.
# We must preserve that order for resumability.
#
# configurations:
#
#     (benchmark, parameter, value)
#
# Each key contains all RegionID rows belonging to that
# configuration.
# ----------------------------------------------------------------------

configurations = {}

with open(csv_file, newline="") as f:

    reader = csv.DictReader(f)

    fieldnames = reader.fieldnames or []

    missing_columns = [
        col for col in TIME_COLUMNS
        if col not in fieldnames
    ]

    if missing_columns:

        print(
            "ERROR: Timing columns missing from CSV: "
            + ", ".join(missing_columns),
            file=sys.stderr
        )

        sys.exit(1)


    for row in reader:

        filename = os.path.basename(
            row.get("FileName", "")
        )

        match = re.match(
            r"^(.*?)_serial_(.+)_([0-9]+)\.c$",
            filename
        )

        if not match:
            continue


        benchmark = match.group(1)
        parameter = match.group(2)
        value = int(match.group(3))

        key = (
            benchmark,
            parameter,
            value
        )

        configurations.setdefault(
            key,
            []
        ).append(row)


# ----------------------------------------------------------------------
# Check whether a CSV value contains a valid timing.
# ----------------------------------------------------------------------

def valid_number(value):

    if value is None:
        return False

    value = value.strip()

    if not value:
        return False

    try:
        float(value)
        return True

    except ValueError:
        return False


# ----------------------------------------------------------------------
# A configuration is COMPLETE only when ALL RegionID rows contain
# ALL timing values.
# ----------------------------------------------------------------------

def configuration_complete(rows):

    if not rows:
        return False

    for row in rows:

        for column in TIME_COLUMNS:

            if not valid_number(
                row.get(column, "")
            ):
                return False

    return True


# ----------------------------------------------------------------------
# Preserve the ORIGINAL order of configurations.
#
# Python dictionaries preserve insertion order.
# ----------------------------------------------------------------------

ordered_keys = list(configurations.keys())


# ----------------------------------------------------------------------
# Determine benchmark order.
#
# Example:
#
#     3mm
#     heat-3D
#     jacobi-1D
#     ...
# ----------------------------------------------------------------------

benchmark_order = []

for key in ordered_keys:

    benchmark = key[0]

    if benchmark not in benchmark_order:
        benchmark_order.append(benchmark)


# ----------------------------------------------------------------------
# Determine which benchmarks have actually been reached.
#
# A benchmark is considered REACHED if at least one timing value exists
# anywhere in that benchmark.
#
# This is important for the interrupted-run case.
#
# Example:
#
#     heat-3D N=1 ... N=800  -> timings exist
#     heat-3D N=900          -> empty
#
#     jacobi-1D N=1 ...      -> empty
#
# Therefore:
#
#     heat-3D was reached
#     jacobi-1D was NOT reached
#
# ----------------------------------------------------------------------

benchmark_reached = {}

for benchmark in benchmark_order:

    benchmark_reached[benchmark] = False

    for key in ordered_keys:

        if key[0] != benchmark:
            continue

        for row in configurations[key]:

            for column in TIME_COLUMNS:

                if valid_number(
                    row.get(column, "")
                ):

                    benchmark_reached[benchmark] = True
                    break

            if benchmark_reached[benchmark]:
                break

        if benchmark_reached[benchmark]:
            break


# ----------------------------------------------------------------------
# Find the LAST benchmark that was actually reached.
#
# This prevents an incomplete tail from an earlier benchmark from
# becoming the resume point.
#
# Example:
#
#     3mm:
#         N=1 ... 8000 complete
#         N=9000+ incomplete
#
#     heat-3D:
#         N=1 ... 800 complete
#         N=900 incomplete
#
#     jacobi-1D:
#         nothing executed
#
# LAST REACHED = heat-3D
# ----------------------------------------------------------------------

last_reached_benchmark = None

for benchmark in benchmark_order:

    if benchmark_reached[benchmark]:

        last_reached_benchmark = benchmark


# ----------------------------------------------------------------------
# Find the first INCOMPLETE configuration inside the last reached
# benchmark.
# ----------------------------------------------------------------------

resume_index = None

if last_reached_benchmark is not None:

    for index, key in enumerate(ordered_keys):

        benchmark = key[0]

        if benchmark != last_reached_benchmark:
            continue

        rows = configurations[key]

        if not configuration_complete(rows):

            resume_index = index
            break


# ----------------------------------------------------------------------
# If the last reached benchmark is completely complete, resume from
# the first configuration of the NEXT benchmark.
# ----------------------------------------------------------------------

if resume_index is None and last_reached_benchmark is not None:

    found_last = False

    for index, key in enumerate(ordered_keys):

        benchmark = key[0]

        if benchmark == last_reached_benchmark:

            found_last = True
            continue

        if found_last:

            resume_index = index
            break


# ----------------------------------------------------------------------
# If no benchmark has been reached at all, start from the beginning.
# ----------------------------------------------------------------------

if last_reached_benchmark is None:

    resume_index = 0


# ----------------------------------------------------------------------
# Count completed configurations.
# ----------------------------------------------------------------------

completed = 0

for key in ordered_keys:

    if configuration_complete(
        configurations[key]
    ):

        completed += 1


# ----------------------------------------------------------------------
# Write configurations from the resume point onward.
#
# IMPORTANT:
#
# We intentionally DO NOT filter incomplete configurations here.
#
# Once we reach the resume point, every later configuration must remain
# in the experiment list.
#
# This allows:
#
#     heat-3D N=900
#     heat-3D N=1000
#     ...
#     jacobi-1D ...
#
# to run normally.
# ----------------------------------------------------------------------

remaining = 0

if resume_index is not None:

    for key in ordered_keys[resume_index:]:

        benchmark, parameter, value = key

        print(
            f"{benchmark}|{parameter}|{value}"
        )

        remaining += 1


# ----------------------------------------------------------------------
# Resume information goes to stderr so it does not enter
# experiments.txt.
# ----------------------------------------------------------------------

print(
    f"Resume scan: "
    f"{completed} configuration(s) already complete, "
    f"{remaining} configuration(s) remaining.",
    file=sys.stderr
)


if last_reached_benchmark is not None:

    print(
        f"Last reached benchmark: "
        f"{last_reached_benchmark}",
        file=sys.stderr
    )


if resume_index is not None:

    benchmark, parameter, value = ordered_keys[resume_index]

    print(
        f"Resume point: "
        f"{benchmark}|{parameter}|{value}",
        file=sys.stderr
    )

else:

    print(
        "No configurations remain.",
        file=sys.stderr
    )

PY

    local STATUS=$?

    if [ $STATUS -ne 0 ]; then
        echo
        echo "ERROR: Could not build resumable experiment list."
        exit 1
    fi

    echo "$EXPERIMENT_FILE"
}

# ============================================================================
# FIND SOURCE PROGRAM
#
# Exact preferred filenames:
#
# Serial:
#     benchmark_serial.c
#
# OMP3:
#     benchmark_omp3.c
#
# OMP45:
#     benchmark_omp45.c
#
# OpenACC:
#     benchmark_acc.c
# ============================================================================

find_source()
{
    local TOOL="$1"

    local BENCHMARK="$2"


    local DIR=""

    local EXACT=""


    case "$TOOL" in


        serial)

            DIR="$SERIAL_ANNOTATED_DIR"

            EXACT="$DIR/${BENCHMARK}_serial.c"

            ;;


        omp3)

            DIR="$OMP3_ANNOTATED_DIR"

            EXACT="$DIR/${BENCHMARK}_omp3.c"

            ;;


        omp45)

            DIR="$OMP45_SOURCE_DIR"

            EXACT="$DIR/${BENCHMARK}_omp45.c"

            ;;


        openacc)

            DIR="$OPENACC_SOURCE_DIR"

            EXACT="$DIR/${BENCHMARK}_acc.c"

            ;;


        *)

            return 1

            ;;

    esac


    if [ -f "$EXACT" ]; then

        echo "$EXACT"

        return 0

    fi


    shopt -s nullglob


    local MATCHES=(
        "$DIR/${BENCHMARK}"*.c
    )


    shopt -u nullglob


    if [ ${#MATCHES[@]} -eq 1 ]; then

        echo "${MATCHES[0]}"

        return 0

    fi


    if [ ${#MATCHES[@]} -gt 1 ]; then

        echo \
            "Multiple source candidates for $BENCHMARK in $DIR" \
            >&2

    fi


    return 1
}


# ============================================================================
# CPU TIMING PARSER
#
# Expected:
#
# Region 0: total=..., executions=..., average=0.123456 s
#
#
# Result file:
#
# 0|0.123456
# 1|...
# ============================================================================

extract_cpu_times()
{
    local LOG_FILE="$1"

    local RESULT_FILE="$2"


    python3 - \
        "$LOG_FILE" \
        "$RESULT_FILE" <<'PY'

import re
import sys


log_file = sys.argv[1]

result_file = sys.argv[2]


with open(log_file, "r", errors="ignore") as f:

    text = f.read()


pattern = re.compile(
    r'Region\s+(\d+)\s*:.*?'
    r'average\s*=\s*'
    r'([0-9.eE+-]+)'
    r'\s*s',
    re.IGNORECASE
)


results = {}


for match in pattern.finditer(text):

    region = int(
        match.group(1)
    )

    timing = match.group(2)


    results[region] = timing


with open(result_file, "w") as f:

    for region in sorted(results):

        f.write(
            f"{region}|"
            f"{results[region]}\n"
        )

PY
}


# ============================================================================
# GPU TIMING PARSER
#
#
# OpenMP 4.5 table:
#
# Region | Lines | Calls |
# Tot Res | Avg Res |
# Tot Obs | Avg Obs |
# Tot Iso | Avg Iso
#
#
# OpenACC table:
#
# Region | Lines | Calls |
# Total Res | Avg Res |
# Total Obs | Avg Obs |
# Isolated
#
#
# Stored result:
#
# RegionID|Resident|Observed|Isolated
# ============================================================================

extract_gpu_times()
{
    local TOOL="$1"
    local LOG_FILE="$2"
    local RESULT_FILE="$3"

    python3 - \
        "$TOOL" \
        "$LOG_FILE" \
        "$RESULT_FILE" <<'PY'

import re
import sys


tool = sys.argv[1]
log_file = sys.argv[2]
result_file = sys.argv[3]


with open(log_file, "r", errors="ignore") as f:
    lines = f.readlines()


results = {}


# New combined GPU timing table:
#
# 0 Region
# 1 Lines
# 2 Invocations
# 3 Total Res(s)
# 4 Avg Res(s)
# 5 Total Obs(s)
# 6 Avg Obs(s)
# 7 Isolated(s)
#
# GPU scripts number regions from 1.
# Feature extractor RegionID is 0-based.
# Therefore:
#
#     GPU Region 1 -> RegionID 0
#     GPU Region 2 -> RegionID 1
#     ...


for raw_line in lines:

    line = raw_line.strip()

    if not re.match(
        r'^Region\s+\d+\s*\|',
        line
    ):
        continue


    parts = [
        part.strip()
        for part in line.split("|")
    ]


    if len(parts) < 8:
        continue


    region_match = re.match(
        r'^Region\s+(\d+)',
        parts[0]
    )


    if not region_match:
        continue


    try:

        # --------------------------------------------------------------
        # IMPORTANT:
        #
        # Combined GPU profiler:
        #     Region 1, Region 2, ...
        #
        # LLVM feature extractor:
        #     RegionID 0, RegionID 1, ...
        # --------------------------------------------------------------

        region = int(
            region_match.group(1)
        ) - 1


        if region < 0:
            continue


        resident = parts[4]

        observed = parts[6]

        isolated = parts[7]


        # Validate timing values.

        float(resident)
        float(observed)
        float(isolated)


        results[region] = (
            resident,
            observed,
            isolated
        )


    except (
        ValueError,
        IndexError
    ):
        continue


with open(result_file, "w") as f:

    for region in sorted(results):

        resident, observed, isolated = results[region]

        f.write(
            f"{region}|"
            f"{resident}|"
            f"{observed}|"
            f"{isolated}\n"
        )

PY
}

# ============================================================================
# RUN CPU PROGRAM WITHOUT STORING NORMAL STDOUT
#
# Only CAPC CPU timing lines are retained.
# Normal program output is discarded.
# stderr is captured separately for failure diagnostics.
#
# Return:
#   normal program exit code
#   124 = timeout
# ============================================================================

run_cpu_filtered()
{
    local TIMING_FILE="$1"
    local ERROR_FILE="$2"
    local TIMEOUT_SECONDS="$3"
    shift 3

    local PID
    local STATUS
    local START_TIME
    local NOW

    # --------------------------------------------------------------
    # stdout:
    #   Keep ONLY CAPC timing report lines.
    #   Everything else is consumed by grep and discarded.
    #
    # stderr:
    #   Capture separately so it can be retained only on failure.
    # --------------------------------------------------------------

    "$@" \
        > >(grep --line-buffered -E \
            '^Region[[:space:]]+[0-9]+[[:space:]]*:.*average[[:space:]]*=' \
            > "$TIMING_FILE") \
        2> "$ERROR_FILE" &

    PID=$!

    START_TIME=$(date +%s)

    while kill -0 "$PID" 2>/dev/null; do

        if [ "$TIMEOUT_SECONDS" -gt 0 ]; then

            NOW=$(date +%s)

            if [ $((NOW - START_TIME)) -ge "$TIMEOUT_SECONDS" ]; then

                echo "    Execution exceeded ${TIMEOUT_SECONDS}s."
                echo "    Terminating execution."

                kill "$PID" 2>/dev/null
                sleep 1
                kill -9 "$PID" 2>/dev/null

                wait "$PID" 2>/dev/null

                return 124
            fi

        fi

        sleep 1
    done

    wait "$PID"
    STATUS=$?

    return "$STATUS"
}


# ============================================================================
# RUN GPU PROFILER WITHOUT STORING NORMAL STDOUT
#
# Only GPU timing-table Region lines are retained.
# Other profiler/program stdout is discarded.
# stderr is captured separately for failure diagnostics.
#
# Return:
#   normal profiler exit code
#   124 = timeout
# ============================================================================

run_gpu_filtered()
{
    local TIMING_FILE="$1"
    local ERROR_FILE="$2"
    local TIMEOUT_SECONDS="$3"
    shift 3

    local PID
    local STATUS
    local START_TIME
    local NOW

    # --------------------------------------------------------------
    # Keep only lines such as:
    #
    # Region 1 | ... | ... | ...
    # Region 2 | ... | ... | ...
    #
    # These are exactly the lines needed by extract_gpu_times().
    # --------------------------------------------------------------

    "$@" \
        > >(grep --line-buffered -E \
            '^Region[[:space:]]+[0-9]+[[:space:]]*\|' \
            > "$TIMING_FILE") \
        2> "$ERROR_FILE" &

    PID=$!

    START_TIME=$(date +%s)

    while kill -0 "$PID" 2>/dev/null; do

        if [ "$TIMEOUT_SECONDS" -gt 0 ]; then

            NOW=$(date +%s)

            if [ $((NOW - START_TIME)) -ge "$TIMEOUT_SECONDS" ]; then

                echo "    Execution exceeded ${TIMEOUT_SECONDS}s."
                echo "    Terminating execution."

                kill "$PID" 2>/dev/null
                sleep 1
                kill -9 "$PID" 2>/dev/null

                wait "$PID" 2>/dev/null

                return 124
            fi

        fi

        sleep 1
    done

    wait "$PID"
    STATUS=$?

    return "$STATUS"
}

# ============================================================================
# RUN CPU PROGRAM
#
# Return codes:
#
# 0 = success
# 1 = source missing
# 2 = #define replacement failure
# 3 = compilation failure
# 4 = execution failure
# 5 = timing parsing failure
# 6 = timeout
# ============================================================================

run_cpu()
{
    local TOOL="$1"

    local BENCHMARK="$2"

    local PARAMETER="$3"

    local VALUE="$4"


    local SOURCE


    SOURCE=$(
        find_source \
            "$TOOL" \
            "$BENCHMARK"
    )


    if [ $? -ne 0 ] || [ -z "$SOURCE" ]; then

        echo "    [$TOOL] Source not found."

        return 1

    fi


    local TEMP_SOURCE

    local BINARY

    local COMPILE_LOG

    local RUN_LOG
    local ERROR_LOG

    local RESULT_FILE


    TEMP_SOURCE="$TEMP_SOURCE_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.c"


    BINARY="$TEMP_BIN_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}"


    COMPILE_LOG="$TEMP_LOG_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_compile.log"


    RUN_LOG="$TEMP_LOG_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_run.log"

    ERROR_LOG="$TEMP_LOG_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_stderr.log"


    RESULT_FILE="$TEMP_RESULT_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.txt"


    # ------------------------------------------------------------------
    # Change problem-size macro in temporary source.
    # ------------------------------------------------------------------

    if ! modify_define \
        "$SOURCE" \
        "$TEMP_SOURCE" \
        "$PARAMETER" \
        "$VALUE"
    then

        echo "    [$TOOL] #define replacement FAILED."

        return 2

    fi


    # ------------------------------------------------------------------
    # Compile
    # ------------------------------------------------------------------

    echo "    [$TOOL] Compiling..."


    gcc \
        -O1 \
        -fopenmp \
        "$TEMP_SOURCE" \
        -o "$BINARY" \
        -lm \
        > "$COMPILE_LOG" 2>&1


    local STATUS=$?


    if [ $STATUS -ne 0 ]; then

        echo "    [$TOOL] Compilation FAILED."


        save_small_log \
            "$COMPILE_LOG" \
            "$COMPILE_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.log"


        rm -f "$TEMP_SOURCE"

        rm -f "$BINARY"


        return 3

    fi


    rm -f "$COMPILE_LOG"


    # ------------------------------------------------------------------
    # Execute
    # ------------------------------------------------------------------

    echo "    [$TOOL] Running..."


    run_cpu_filtered \
        "$RUN_LOG" \
        "$ERROR_LOG" \
        "$RUN_TIMEOUT" \
        "$BINARY"

    STATUS=$?

    if [ $STATUS -eq 124 ]; then

        echo "    [$TOOL] TIMEOUT after ${RUN_TIMEOUT}s."

        save_small_log \
            "$ERROR_LOG" \
            "$EXEC_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_timeout.log"

        rm -f "$RUN_LOG"
        rm -f "$ERROR_LOG"
        rm -f "$TEMP_SOURCE"
        rm -f "$BINARY"

        return 6

    elif [ $STATUS -ne 0 ]; then

        echo "    [$TOOL] Execution FAILED."

        save_small_log \
            "$ERROR_LOG" \
            "$EXEC_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.log"

        rm -f "$RUN_LOG"
        rm -f "$ERROR_LOG"
        rm -f "$TEMP_SOURCE"
        rm -f "$BINARY"

        return 4
    fi


    # ------------------------------------------------------------------
    # Parse timing
    # ------------------------------------------------------------------

    extract_cpu_times \
        "$RUN_LOG" \
        "$RESULT_FILE"


    if [ ! -s "$RESULT_FILE" ]; then


        echo "    [$TOOL] No region timing information found."


        save_small_log \
            "$ERROR_LOG" \
            "$OTHER_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_timing_parse.log"


        rm -f "$TEMP_SOURCE"

        rm -f "$BINARY"


        return 5

    fi


    local REGION_COUNT


    REGION_COUNT=$(
        wc -l < "$RESULT_FILE"
    )


    echo \
        "    [$TOOL] Success: "\
"$REGION_COUNT region(s) extracted."


    # Successful logs / compilation artifacts are unnecessary.

    rm -f "$RUN_LOG"

    rm -f "$ERROR_LOG"

    rm -f "$TEMP_SOURCE"

    rm -f "$BINARY"


    return 0
}


# ============================================================================
# RUN GPU PROFILER
#
# OMP45 and OpenACC profiling scripts already:
#
#   instrument
#   compile
#   execute
#   isolate regions
#   print timing report
#
# Master script only:
#
#   creates parameterized temporary input
#   invokes profiler
#   captures output
#   extracts Resident / Observed / Isolated
# ============================================================================

run_gpu()
{
    local TOOL="$1"

    local BENCHMARK="$2"

    local PARAMETER="$3"

    local VALUE="$4"


    local SOURCE

    local PROFILER


    SOURCE=$(
        find_source \
            "$TOOL" \
            "$BENCHMARK"
    )


    if [ $? -ne 0 ] || [ -z "$SOURCE" ]; then

        echo "    [$TOOL] Source not found."

        return 1

    fi


    case "$TOOL" in


        omp45)

            PROFILER="$OMP45_PROFILER"

            ;;


        openacc)

            PROFILER="$OPENACC_PROFILER"

            ;;


        *)

            echo "Unknown GPU tool: $TOOL"

            return 1

            ;;

    esac


    local TEMP_SOURCE

    local RUN_LOG

    local ERROR_LOG

    local RESULT_FILE


    TEMP_SOURCE="$TEMP_SOURCE_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.c"


    RUN_LOG="$TEMP_LOG_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.log"

    ERROR_LOG="$TEMP_LOG_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_stderr.log"


    RESULT_FILE="$TEMP_RESULT_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.txt"


    # ------------------------------------------------------------------
    # Modify temporary source.
    # ------------------------------------------------------------------

    if ! modify_define \
        "$SOURCE" \
        "$TEMP_SOURCE" \
        "$PARAMETER" \
        "$VALUE"
    then

        echo "    [$TOOL] #define replacement FAILED."

        return 2

    fi


    # ------------------------------------------------------------------
    # Run profiler.
    # ------------------------------------------------------------------

    echo "    [$TOOL] Profiling..."


    local STATUS


    run_gpu_filtered \
        "$RUN_LOG" \
        "$ERROR_LOG" \
        "$RUN_TIMEOUT" \
        python3 "$PROFILER" "$TEMP_SOURCE"

    STATUS=$?

    if [ $STATUS -eq 124 ]; then

        echo "    [$TOOL] TIMEOUT after ${RUN_TIMEOUT}s."

        save_small_log \
            "$ERROR_LOG" \
            "$EXEC_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_timeout.log"

        rm -f "$RUN_LOG"
        rm -f "$ERROR_LOG"
        rm -f "$TEMP_SOURCE"

        return 6

    elif [ $STATUS -ne 0 ]; then

        echo "    [$TOOL] Profiler execution FAILED."

        save_small_log \
            "$ERROR_LOG" \
            "$EXEC_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.log"

        rm -f "$RUN_LOG"
        rm -f "$ERROR_LOG"
        rm -f "$TEMP_SOURCE"

        return 4
    fi


    # ------------------------------------------------------------------
    # Extract GPU timing table.
    # ------------------------------------------------------------------

    extract_gpu_times \
        "$TOOL" \
        "$RUN_LOG" \
        "$RESULT_FILE"


    if [ ! -s "$RESULT_FILE" ]; then


        echo "    [$TOOL] GPU timing extraction FAILED."


        save_small_log \
            "$ERROR_LOG" \
            "$OTHER_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_timing_parse.log"


        rm -f "$RUN_LOG"
        rm -f "$ERROR_LOG"
        rm -f "$TEMP_SOURCE"


        return 5

    fi


    local REGION_COUNT


    REGION_COUNT=$(
        wc -l < "$RESULT_FILE"
    )


    echo \
        "    [$TOOL] Success: "\
"$REGION_COUNT region(s) extracted."


    rm -f "$RUN_LOG"
    rm -f "$ERROR_LOG"
    rm -f "$TEMP_SOURCE"

    return 0
    
}


# ============================================================================
# PHASE 3
#
# EXECUTION SWEEP
#
#
# Example:
#
# N=100
#
#   Serial  success
#   OMP3    success
#   OMP45   success
#   OpenACC success
#
#
# N=1000
#
#   Serial  success
#   OMP3    success
#   OMP45   FAIL
#
#   OpenACC SKIPPED
#
#
# N=2000 and everything larger:
#
#   ALL SKIPPED
#
#
# Successful configurations are committed to the CSV immediately.
# Then next Benchmark+Parameter begins normally.
# ============================================================================

run_execution_sweep()
{
    echo
    echo "======================================================================"
    echo "PHASE 3: EXECUTION-TIME SWEEP"
    echo "======================================================================"


    local EXPERIMENT_FILE


    EXPERIMENT_FILE=$(
        build_experiment_list
    )


    local TOTAL_EXPERIMENTS


    TOTAL_EXPERIMENTS=$(
        wc -l < "$EXPERIMENT_FILE"
    )

    if [ "$TOTAL_EXPERIMENTS" -eq 0 ]; then

        echo
        echo "======================================================================"
        echo "PHASE 3 ALREADY COMPLETE"
        echo "======================================================================"
        echo
        echo "All configurations in:"
        echo
        echo "    $FEATURES_CSV"
        echo
        echo "already contain complete timing information."
        echo
        echo "Nothing to resume."
        echo

        return 0
    fi

    echo "Resume CSV:"
    echo "    $FEATURES_CSV"
    echo
    echo "Configurations remaining: $TOTAL_EXPERIMENTS"


    local CURRENT_KEY=""

    local STOP_CURRENT_KEY=0

    local CURRENT=0


    while IFS='|' read -r \
        BENCHMARK \
        PARAMETER \
        VALUE
    do


        [ -z "$BENCHMARK" ] && continue


        CURRENT=$((CURRENT + 1))


        local KEY="${BENCHMARK}|${PARAMETER}"


        # ------------------------------------------------------------------
        # New benchmark/parameter starts a fresh sweep.
        # ------------------------------------------------------------------

        if [ "$KEY" != "$CURRENT_KEY" ]; then


            CURRENT_KEY="$KEY"

            STOP_CURRENT_KEY=0


            echo
            echo "======================================================================"
            echo "NEW SWEEP"
            echo "======================================================================"
            echo "Benchmark : $BENCHMARK"
            echo "Parameter : $PARAMETER"
            echo "======================================================================"

        fi


        # ------------------------------------------------------------------
        # A smaller value already failed.
        # ------------------------------------------------------------------

        if [ $STOP_CURRENT_KEY -eq 1 ]; then


            echo
            echo "[$CURRENT/$TOTAL_EXPERIMENTS]"
            echo "SKIPPED: $BENCHMARK | $PARAMETER=$VALUE"
            echo "Reason : smaller value already failed."


            continue

        fi


        echo
        echo "----------------------------------------------------------------------"
        echo "Experiment $CURRENT / $TOTAL_EXPERIMENTS"
        echo "----------------------------------------------------------------------"
        echo "Benchmark : $BENCHMARK"
        echo "Parameter : $PARAMETER"
        echo "Value     : $VALUE"
        echo "----------------------------------------------------------------------"


        # ==================================================================
        # SERIAL
        # ==================================================================

        run_cpu \
            "serial" \
            "$BENCHMARK" \
            "$PARAMETER" \
            "$VALUE"


        local STATUS=$?


        if [ $STATUS -ne 0 ]; then


            echo
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "SERIAL FAILED"
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "Benchmark : $BENCHMARK"
            echo "Parameter : $PARAMETER"
            echo "Value     : $VALUE"
            echo
            echo "OpenMP3 / OpenMP4.5 / OpenACC at this value will NOT run."
            echo "All larger values for this benchmark+parameter will be skipped."
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"


            STOP_CURRENT_KEY=1


            continue

        fi


        # ==================================================================
        # OPENMP 3
        # ==================================================================

        run_cpu \
            "omp3" \
            "$BENCHMARK" \
            "$PARAMETER" \
            "$VALUE"


        STATUS=$?


        if [ $STATUS -ne 0 ]; then


            echo
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "OPENMP3 FAILED"
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "Benchmark : $BENCHMARK"
            echo "Parameter : $PARAMETER"
            echo "Value     : $VALUE"
            echo
            echo "OpenMP4.5 / OpenACC at this value will NOT run."
            echo "All larger values for this benchmark+parameter will be skipped."
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"


            STOP_CURRENT_KEY=1


            continue

        fi


        # ==================================================================
        # OPENMP 4.5
        # ==================================================================

        run_gpu \
            "omp45" \
            "$BENCHMARK" \
            "$PARAMETER" \
            "$VALUE"


        STATUS=$?


        if [ $STATUS -ne 0 ]; then


            echo
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "OPENMP4.5 FAILED"
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "Benchmark : $BENCHMARK"
            echo "Parameter : $PARAMETER"
            echo "Value     : $VALUE"
            echo
            echo "OpenACC at this value will NOT run."
            echo "All larger values for this benchmark+parameter will be skipped."
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"


            STOP_CURRENT_KEY=1


            continue

        fi


        # ==================================================================
        # OPENACC
        # ==================================================================

        run_gpu \
            "openacc" \
            "$BENCHMARK" \
            "$PARAMETER" \
            "$VALUE"


        STATUS=$?


        if [ $STATUS -ne 0 ]; then


            echo
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "OPENACC FAILED"
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "Benchmark : $BENCHMARK"
            echo "Parameter : $PARAMETER"
            echo "Value     : $VALUE"
            echo
            echo "All larger values for this benchmark+parameter will be skipped."
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"


            STOP_CURRENT_KEY=1


            continue

        fi


        # ==================================================================
        # COMMIT THIS COMPLETE CONFIGURATION IMMEDIATELY
        # ==================================================================

        echo
        echo "    [csv] All paradigm timings found."
        echo "    [csv] Committing this configuration immediately..."


        commit_configuration_to_csv \
            "$BENCHMARK" \
            "$PARAMETER" \
            "$VALUE"


        STATUS=$?


        if [ $STATUS -ne 0 ]; then

            echo
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "CSV TIMING COMMIT FAILED"
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "Benchmark : $BENCHMARK"
            echo "Parameter : $PARAMETER"
            echo "Value     : $VALUE"
            echo
            echo "Timing result files were NOT deleted."
            echo "All larger values for this benchmark+parameter will be skipped."
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"


            STOP_CURRENT_KEY=1


            continue

        fi


        # The CSV is now safely updated. Remove the temporary result files
        # immediately so they do not accumulate throughout a long sweep.
        delete_committed_result_files \
            "$BENCHMARK" \
            "$PARAMETER" \
            "$VALUE"


        echo "    [results] Timing result files deleted after successful CSV commit."


        echo
        echo "SUCCESS:"
        echo "$BENCHMARK | $PARAMETER=$VALUE completed and stored in $FEATURES_CSV."


    done < "$EXPERIMENT_FILE"
}


# ============================================================================
# INCREMENTAL CSV COMMIT
#
# Called immediately after Serial + OpenMP3 + OpenMP4.5 + OpenACC timings have
# all been extracted successfully for one:
#
#     benchmark + parameter + value
#
# The function:
#
#   1. Finds all CSV rows / RegionIDs for that configuration.
#   2. Requires complete timings for every expected RegionID from all paradigms.
#   3. Writes a temporary CSV.
#   4. Atomically replaces the real CSV with os.replace().
#   5. Returns success only after the atomic replacement succeeds.
#
# The caller deletes the four result files ONLY after this function succeeds.
# ============================================================================

commit_configuration_to_csv()
{
    local BENCHMARK="$1"

    local PARAMETER="$2"

    local VALUE="$3"


    local COMMIT_LOG="$TEMP_LOG_DIR/"\
"${BENCHMARK}_commit_${PARAMETER}_${VALUE}.log"


    python3 - \
        "$FEATURES_CSV" \
        "$TEMP_RESULT_DIR" \
        "$BENCHMARK" \
        "$PARAMETER" \
        "$VALUE" \
        > "$COMMIT_LOG" 2>&1 <<'PY'

import csv
import os
import re
import sys


csv_file = sys.argv[1]
result_dir = sys.argv[2]
benchmark = sys.argv[3]
parameter = sys.argv[4]
value = sys.argv[5]


TIME_COLUMNS = [
    "SerialTime",
    "OpenMP3Time",
    "OpenMP45ResidentTime",
    "OpenMP45ObservedTime",
    "OpenMP45IsolatedTime",
    "OpenACCResidentTime",
    "OpenACCObservedTime",
    "OpenACCIsolatedTime",
]


def fail(message):
    print(f"ERROR: {message}")
    sys.exit(1)


def read_cpu(tool):
    filename = os.path.join(
        result_dir,
        f"{benchmark}_{tool}_{parameter}_{value}.txt",
    )

    if not os.path.isfile(filename):
        fail(f"Missing result file: {filename}")

    result = {}

    with open(filename) as f:
        for line in f:
            parts = line.strip().split("|")

            if len(parts) != 2:
                continue

            try:
                region = int(parts[0])
                timing = parts[1]
                float(timing)
            except ValueError:
                continue

            result[region] = timing

    if not result:
        fail(f"No valid timings found in: {filename}")

    return result


def read_gpu(tool):
    filename = os.path.join(
        result_dir,
        f"{benchmark}_{tool}_{parameter}_{value}.txt",
    )

    if not os.path.isfile(filename):
        fail(f"Missing result file: {filename}")

    result = {}

    with open(filename) as f:
        for line in f:
            parts = line.strip().split("|")

            if len(parts) != 4:
                continue

            try:
                region = int(parts[0])
                resident = parts[1]
                observed = parts[2]
                isolated = parts[3]

                float(resident)
                float(observed)
                float(isolated)
            except ValueError:
                continue

            result[region] = (
                resident,
                observed,
                isolated,
            )

    if not result:
        fail(f"No valid timings found in: {filename}")

    return result


with open(csv_file, newline="") as f:
    reader = csv.DictReader(f)
    fieldnames = list(reader.fieldnames or [])
    rows = list(reader)


# The feature extractor is expected to create these columns beforehand.
missing_columns = [
    column
    for column in TIME_COLUMNS
    if column not in fieldnames
]

if missing_columns:
    fail(
        "Timing column(s) missing from feature-extractor CSV: "
        + ", ".join(missing_columns)
    )


target_rows = []

for index, row in enumerate(rows):
    source_name = os.path.basename(
        row.get("FileName", "")
    )

    match = re.match(
        r"^(.*?)_serial_(.+)_([0-9]+)\.c$",
        source_name,
    )

    if not match:
        continue

    row_benchmark = match.group(1)
    row_parameter = match.group(2)
    row_value = match.group(3)

    if (
        row_benchmark != benchmark
        or row_parameter != parameter
        or row_value != value
    ):
        continue

    try:
        region_id = int(row["RegionID"])
    except (KeyError, ValueError):
        fail(
            f"Invalid RegionID in CSV for "
            f"{benchmark}|{parameter}|{value}"
        )

    target_rows.append(
        (index, region_id)
    )


if not target_rows:
    fail(
        f"No CSV rows matched "
        f"{benchmark}|{parameter}|{value}"
    )


expected_regions = {
    region_id
    for _, region_id in target_rows
}


serial = read_cpu("serial")
omp3 = read_cpu("omp3")
omp45 = read_gpu("omp45")
openacc = read_gpu("openacc")


def validate_regions(name, result):
    actual_regions = set(result)

    missing = sorted(
        expected_regions - actual_regions
    )

    extra = sorted(
        actual_regions - expected_regions
    )

    if missing:
        fail(
            f"{name} is missing RegionID(s) "
            + ", ".join(map(str, missing))
        )

    if extra:
        print(
            f"WARNING: {name} contains extra RegionID(s): "
            + ", ".join(map(str, extra))
        )


validate_regions("Serial", serial)
validate_regions("OpenMP3", omp3)
validate_regions("OpenMP4.5", omp45)
validate_regions("OpenACC", openacc)


for index, region_id in target_rows:
    row = rows[index]

    row["SerialTime"] = serial[region_id]

    row["OpenMP3Time"] = omp3[region_id]

    (
        row["OpenMP45ResidentTime"],
        row["OpenMP45ObservedTime"],
        row["OpenMP45IsolatedTime"],
    ) = omp45[region_id]

    (
        row["OpenACCResidentTime"],
        row["OpenACCObservedTime"],
        row["OpenACCIsolatedTime"],
    ) = openacc[region_id]


temp_csv = (
    csv_file
    + f".tmp.{os.getpid()}"
)


try:
    with open(
        temp_csv,
        "w",
        newline="",
    ) as f:

        writer = csv.DictWriter(
            f,
            fieldnames=fieldnames,
        )

        writer.writeheader()
        writer.writerows(rows)

        f.flush()
        os.fsync(f.fileno())


    os.replace(
        temp_csv,
        csv_file,
    )

except Exception:
    try:
        if os.path.exists(temp_csv):
            os.remove(temp_csv)
    finally:
        raise


print(
    f"Committed {len(target_rows)} region row(s) for "
    f"{benchmark}|{parameter}|{value}"
)

PY


    local STATUS=$?


    if [ $STATUS -ne 0 ]; then

        echo "    [csv] Timing commit FAILED."


        save_small_log \
            "$COMMIT_LOG" \
            "$OTHER_ERROR_DIR/"\
"${BENCHMARK}_commit_${PARAMETER}_${VALUE}.log"


        return 1

    fi


    cat "$COMMIT_LOG"

    rm -f "$COMMIT_LOG"


    return 0
}


# ============================================================================
# DELETE RESULT FILES FOR ONE SUCCESSFULLY COMMITTED CONFIGURATION
# ============================================================================

delete_committed_result_files()
{
    local BENCHMARK="$1"

    local PARAMETER="$2"

    local VALUE="$3"


    rm -f \
        "$TEMP_RESULT_DIR/${BENCHMARK}_serial_${PARAMETER}_${VALUE}.txt" \
        "$TEMP_RESULT_DIR/${BENCHMARK}_omp3_${PARAMETER}_${VALUE}.txt" \
        "$TEMP_RESULT_DIR/${BENCHMARK}_omp45_${PARAMETER}_${VALUE}.txt" \
        "$TEMP_RESULT_DIR/${BENCHMARK}_openacc_${PARAMETER}_${VALUE}.txt"
}


# ============================================================================
                                # MAIN
# ============================================================================

START_PHASE=1
FEATURES_CSV_OVERRIDE=""

# ----------------------------------------------------------------------------
# Command-line arguments
# ----------------------------------------------------------------------------

REFRESH_FEATURES_CSV=""

while [[ $# -gt 0 ]]; do
    case "$1" in

        --start-phase)
            START_PHASE="${2:-}"
            if [[ -z "$START_PHASE" ]]; then
                echo "ERROR: --start-phase requires a value."
                exit 1
            fi
            shift 2
            ;;

        --features-csv)
            FEATURES_CSV_OVERRIDE="${2:-}"
            if [[ -z "$FEATURES_CSV_OVERRIDE" ]]; then
                echo "ERROR: --features-csv requires a CSV file."
                exit 1
            fi
            shift 2
            ;;

        --refresh-features)
            if [[ -z "${2:-}" ]]; then
                echo "ERROR: --refresh-features requires an existing CSV file."
                exit 1
            fi

            REFRESH_FEATURES_CSV="$2"
            shift 2
            ;;

        -h|--help)
            echo
            echo "Usage:"
            echo
            echo "  $0"
            echo "      Run the complete pipeline."
            echo
            echo "  $0 --start-phase 2 --features-csv features_2.csv"
            echo "      Skip Phase 1 and start from Phase 2."
            echo
            echo "  $0 --refresh-features features.csv"
            echo "      Re-run the static feature extractor and update ONLY"
            echo "      the extractor-derived columns in features.csv."
            echo "      Existing timing columns (Serial/OpenMP3/OpenMP4.5/OpenACC)"
            echo "      are left untouched. Nothing else runs."
            echo
            echo "Options:"
            echo "  --start-phase N"
            echo "      Start from phase N (1, 2, or 3)."
            echo
            echo "  --features-csv FILE"
            echo "      Existing feature CSV to use when starting"
            echo "      from Phase 2 or later."
            echo
            echo "  --refresh-features FILE"
            echo "      Regenerate static features only, merging into FILE"
            echo "      in place. Timing columns are preserved."
            echo
            exit 0
            ;;

        *)
            echo "ERROR: Unknown argument: $1"
            echo "Use '$0 --help' for usage."
            exit 1
            ;;
    esac
done

# Apply --features-csv regardless of which phase we're starting from.
if [[ -n "$FEATURES_CSV_OVERRIDE" ]]; then
    FEATURES_CSV="$FEATURES_CSV_OVERRIDE"
fi

# ============================================================================
# REFRESH STATIC FEATURES IN AN EXISTING CSV
#
# Re-runs the AST + LLVM feature extractor exactly as Phase 1 does, then
# merges the freshly extracted columns into an existing features CSV,
# matched on (FileName, RegionID).
#
# Timing columns already present in the existing CSV are NEVER touched,
# even if the newly extracted row order or region count differs.
#
# Rows present in the OLD csv but absent from the NEW extraction are kept
# as-is (their feature columns are stale but timings are preserved).
# Rows present in the NEW extraction but absent from the OLD csv are
# appended with empty timing columns.
# ============================================================================

refresh_static_features()
{
    local TARGET_CSV="$1"

    echo
    echo "======================================================================"
    echo "REFRESH: STATIC FEATURE EXTRACTION"
    echo "======================================================================"
    echo "Target CSV (in place):"
    echo "    $TARGET_CSV"


    # Re-use the normal extraction pipeline, but write to a private
    # scratch CSV rather than the real FEATURES_CSV, so a failure midway
    # never corrupts the existing file.

    local OLD_FEATURES_CSV="$FEATURES_CSV"

    FEATURES_CSV="$WORK_DIR/refreshed_features.csv"

    generate_static_features

    local NEW_CSV="$FEATURES_CSV"

    FEATURES_CSV="$OLD_FEATURES_CSV"


    echo
    echo "----------------------------------------------------------------------"
    echo "Merging refreshed features into existing CSV..."
    echo "----------------------------------------------------------------------"


    local MERGE_LOG="$TEMP_LOG_DIR/refresh_merge.log"


    python3 - \
        "$TARGET_CSV" \
        "$NEW_CSV" \
        > "$MERGE_LOG" 2>&1 <<'PY'

import csv
import os
import sys


old_csv = sys.argv[1]
new_csv = sys.argv[2]


TIME_COLUMNS = [
    "SerialTime",
    "OpenMP3Time",
    "OpenMP45ResidentTime",
    "OpenMP45ObservedTime",
    "OpenMP45IsolatedTime",
    "OpenACCResidentTime",
    "OpenACCObservedTime",
    "OpenACCIsolatedTime",
]


def fail(message):
    print(f"ERROR: {message}")
    sys.exit(1)


def load(path):
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        fieldnames = list(reader.fieldnames or [])
        rows = list(reader)
    return fieldnames, rows


def key_of(row):
    filename = os.path.basename(row.get("FileName", ""))
    region = row.get("RegionID", "")
    return (filename, region)


old_fields, old_rows = load(old_csv)
new_fields, new_rows = load(new_csv)

missing_time_cols = [c for c in TIME_COLUMNS if c not in old_fields]
if missing_time_cols:
    fail(
        "Existing CSV is missing expected timing column(s): "
        + ", ".join(missing_time_cols)
    )

old_by_key = {key_of(r): r for r in old_rows}
new_by_key = {key_of(r): r for r in new_rows}

# Final column order: feature columns from the NEW extraction (this is
# authoritative for feature schema, e.g. a newly added Select Count
# column), followed by any timing columns from the OLD csv that aren't
# already present.
merged_fields = list(new_fields)
for c in TIME_COLUMNS:
    if c not in merged_fields:
        merged_fields.append(c)

merged_rows = []
updated = 0
appended = 0
kept_stale = 0

seen_keys = set()

# Walk new extraction rows first, preserving its ordering.
for key, new_row in new_by_key.items():
    seen_keys.add(key)

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

# Any old rows whose (FileName, RegionID) no longer appear in the new
# extraction are preserved as-is, so previously collected timings are
# never silently discarded.
for key, old_row in old_by_key.items():
    if key in seen_keys:
        continue

    merged = {c: old_row.get(c, "") for c in merged_fields}
    merged_rows.append(merged)
    kept_stale += 1


temp_csv = old_csv + f".tmp.{os.getpid()}"

try:
    with open(temp_csv, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=merged_fields)
        writer.writeheader()
        writer.writerows(merged_rows)
        f.flush()
        os.fsync(f.fileno())

    os.replace(temp_csv, old_csv)

except Exception:
    try:
        if os.path.exists(temp_csv):
            os.remove(temp_csv)
    finally:
        raise


print(
    f"Merge complete: {updated} row(s) updated with fresh features "
    f"(timings preserved), {appended} new row(s) appended "
    f"(no timings yet), {kept_stale} old row(s) kept unchanged "
    f"(not present in new extraction)."
)

PY


    local STATUS=$?


    if [ $STATUS -ne 0 ]; then

        echo
        echo "ERROR: Feature refresh merge FAILED."
        echo "See log:"
        echo "    $MERGE_LOG"

        exit 1

    fi


    cat "$MERGE_LOG"

    rm -f "$MERGE_LOG"


    echo
    echo "Feature refresh complete. Timing columns were not modified."
    echo
    echo "Updated CSV:"
    echo "    $TARGET_CSV"
}

if [[ -n "$REFRESH_FEATURES_CSV" ]]; then

    if [[ ! -f "$REFRESH_FEATURES_CSV" ]]; then
        echo
        echo "ERROR: --refresh-features target not found:"
        echo "    $REFRESH_FEATURES_CSV"
        exit 1
    fi

    validate_environment

    refresh_static_features "$REFRESH_FEATURES_CSV"

    echo
    echo "======================================================================"
    echo "                  FEATURE REFRESH COMPLETE"
    echo "======================================================================"

    exit 0
fi


# ----------------------------------------------------------------------------
# Validate start phase
# ----------------------------------------------------------------------------

if [[ "$START_PHASE" != "1" &&
      "$START_PHASE" != "2" &&
      "$START_PHASE" != "3" ]]; then

    echo "ERROR: Invalid start phase: $START_PHASE"
    echo "Valid values are: 1, 2, or 3."
    exit 1
fi


# ----------------------------------------------------------------------------
# If starting from Phase 2 or later, an existing CSV is required
# ----------------------------------------------------------------------------

if [[ "$START_PHASE" -ge 2 ]]; then

    if [[ -z "$FEATURES_CSV_OVERRIDE" ]]; then
        echo
        echo "ERROR: Starting from Phase $START_PHASE requires"
        echo "       an existing feature CSV."
        echo
        echo "Example:"
        echo "    $0 --start-phase 2 --features-csv features_2.csv"
        echo
        exit 1
    fi

    if [[ ! -f "$FEATURES_CSV_OVERRIDE" ]]; then
        echo
        echo "ERROR: Feature CSV not found:"
        echo "    $FEATURES_CSV_OVERRIDE"
        echo
        exit 1
    fi

    echo
    echo "Using existing feature CSV:"
    echo "    $FEATURES_CSV"
    echo
fi


echo
echo "======================================================================"
echo "             CAPC MASTER DATASET CREATION"
echo "======================================================================"
echo

echo "Starting from Phase $START_PHASE"
echo


# ----------------------------------------------------------------------------
# Environment validation
# ----------------------------------------------------------------------------

validate_environment


# ----------------------------------------------------------------------------
# PHASE 1: STATIC FEATURE EXTRACTION
# ----------------------------------------------------------------------------

if [[ "$START_PHASE" -le 1 ]]; then

    generate_static_features

else

    echo "======================================================================"
    echo "PHASE 1: STATIC FEATURE EXTRACTION"
    echo "======================================================================"
    echo
    echo "SKIPPED (--start-phase $START_PHASE)"
    echo
fi


# ----------------------------------------------------------------------------
# PHASE 2: SERIAL + OPENMP3/OPENMP4.5/OPENACC ANNOTATION
# ----------------------------------------------------------------------------

if [[ "$START_PHASE" -le 2 ]]; then

    run_cpu_annotation

else

    echo "======================================================================"
    echo "PHASE 2: SERIAL + OPENMP3/OPENMP4.5/OPENACC ANNOTATION"
    echo "======================================================================"
    echo
    echo "SKIPPED (--start-phase $START_PHASE)"
    echo
fi


# ----------------------------------------------------------------------------
# PHASE 3: EXECUTION SWEEP
# ----------------------------------------------------------------------------

if [[ "$START_PHASE" -le 3 ]]; then

    run_execution_sweep

else

    echo "======================================================================"
    echo "PHASE 3: EXECUTION SWEEP"
    echo "======================================================================"
    echo
    echo "SKIPPED (--start-phase $START_PHASE)"
    echo
fi


echo
echo "======================================================================"
echo "                  DATASET CREATION COMPLETE"
echo "======================================================================"
echo

echo "Final dataset:"
echo
echo "    $FEATURES_CSV"
echo

echo "Feature-only backup:"
echo
echo "    ${FEATURES_CSV}.before_execution_times.bak"
echo

echo "Compact error logs:"
echo
echo "    $LOG_ROOT"
echo

echo "Timing storage policy:"
echo
echo "    committed to CSV after every complete configuration"
echo "    committed result files deleted immediately"
echo

echo "Temporary workspace:"
echo
echo "    automatically deleted"
echo

echo "======================================================================"