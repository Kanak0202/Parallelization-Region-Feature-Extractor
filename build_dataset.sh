#!/bin/bash

# ============================================================================
# CAPC MASTER DATASET BUILDER
#
# Run from ProfitabilityTool root:
#
#     ./build_dataset.sh
#
# Pipeline:
#   1. Detect numeric #define variables used in loop bounds inside
#      CAPC profitability regions.
#   2. Sweep each detected variable independently.
#   3. Generate temporary serial variants.
#   4. Run AST + LLVM feature extractor.
#   5. Create features.csv.
#   6. Run Serial and OpenMP3 annotation scripts.
#   7. Read Benchmark + Parameter + Value configurations from features.csv.
#   8. For each configuration:
#          Serial
#          OpenMP3
#          OpenMP4.5
#          OpenACC
#   9. Extract per-region timing.
#  10. Populate timing columns in features.csv.
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
#   features.csv
#   features.csv.before_execution_times.bak
#   dataset_logs/   (ONLY compact error logs)
# ============================================================================


set -u
set -o pipefail


# ============================================================================
# CONFIGURATION
# ============================================================================

ulimit -s unlimited


# ----------------------------------------------------------------------------
# Static feature extraction
# ----------------------------------------------------------------------------

INPUT_DIR="./outputs/serial/CAPC"

FEATURE_EXTRACTOR="./build/ProfitabilityTool"

FEATURES_CSV="./features.csv"


# ----------------------------------------------------------------------------
# Source directories
# ----------------------------------------------------------------------------

SERIAL_SOURCE_DIR="./outputs/serial/CAPC"

OMP3_SOURCE_DIR="./outputs/omp3/CAPC"

OMP45_SOURCE_DIR="./outputs/omp45/CAPC"

OPENACC_SOURCE_DIR="./outputs/openacc/CAPC"


# ----------------------------------------------------------------------------
# CPU annotated directories
# ----------------------------------------------------------------------------

ANNOTATED_ROOT="./outputs/annotated"

SERIAL_ANNOTATED_DIR="$ANNOTATED_ROOT/serial/CAPC"

OMP3_ANNOTATED_DIR="$ANNOTATED_ROOT/omp3/CAPC"


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
# Dense decade-wise sweep:
#
#   1       -> 10           increment 1
#   10      -> 100          increment 10
#   100     -> 1000         increment 100
#   1000    -> 10000        increment 1000
#   ...
#   1e8     -> 1e9          increment 1e8
#
# Boundary values are included only once.
#
# Generated sequence:
#
#   1 2 3 ... 10
#   20 30 ... 100
#   200 300 ... 1000
#   ...
#   200000000 ... 1000000000
# ----------------------------------------------------------------------------

SWEEP_VALUES=()

# 1, 2, ..., 10
for ((v = 1; v <= 10; v++))
do
    SWEEP_VALUES+=("$v")
done

# 20,30,...,100
# 200,300,...,1000
# ...
# 200000000,...,1000000000
for BASE in \
    10 \
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
        SWEEP_VALUES+=("$((MULT * BASE))")
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


mkdir -p \
    "$TEMP_SOURCE_DIR" \
    "$TEMP_BIN_DIR" \
    "$TEMP_LOG_DIR" \
    "$TEMP_RESULT_DIR" \
    "$FEATURE_VARIANT_DIR"


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
    local SOURCE_LOG="$1"

    local DEST_LOG="$2"


    if [ ! -f "$SOURCE_LOG" ]; then

        return

    fi


    {
        head -n "$LOG_HEAD_LINES" "$SOURCE_LOG"

        echo
        echo "================ LOG TRUNCATED ================"
        echo

        tail -n "$LOG_TAIL_LINES" "$SOURCE_LOG"

    } > "$DEST_LOG"
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


    rm -f "$FEATURES_CSV"


    local RESOURCE_DIR

    RESOURCE_DIR=$(clang -print-resource-dir)


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


                echo
                echo "  [$PARAMETER=$VALUE] Extracting features..."


                "$FEATURE_EXTRACTOR" \
                    "$VARIANT" \
                    -- \
                    "-resource-dir=$RESOURCE_DIR" \
                    > "$LOG" 2>&1


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


    if [ ! -f "$FEATURES_CSV" ]; then

        echo
        echo "ERROR:"
        echo "Feature extractor did not create:"
        echo
        echo "    $FEATURES_CSV"

        exit 1

    fi


    echo
    echo "Feature extraction complete."
    echo
    echo "Generated:"
    echo "    $FEATURES_CSV"
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


experiments = set()


with open(csv_file, newline="") as f:

    reader = csv.DictReader(f)


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

        value = int(
            match.group(3)
        )


        experiments.add(
            (
                benchmark,
                parameter,
                value
            )
        )


for benchmark, parameter, value in sorted(
    experiments,
    key=lambda x: (
        x[0],
        x[1],
        x[2]
    )
):

    print(
        f"{benchmark}|{parameter}|{value}"
    )

PY


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

    local RESULT_FILE


    TEMP_SOURCE="$TEMP_SOURCE_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.c"


    BINARY="$TEMP_BIN_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}"


    COMPILE_LOG="$TEMP_LOG_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_compile.log"


    RUN_LOG="$TEMP_LOG_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_run.log"


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


    if [ "$RUN_TIMEOUT" -gt 0 ]; then


        timeout \
            "$RUN_TIMEOUT" \
            "$BINARY" \
            > "$RUN_LOG" 2>&1


        STATUS=$?


    else


        "$BINARY" \
            > "$RUN_LOG" 2>&1


        STATUS=$?


    fi


    if [ $STATUS -eq 124 ]; then


        echo "    [$TOOL] TIMEOUT after ${RUN_TIMEOUT}s."


        save_small_log \
            "$RUN_LOG" \
            "$EXEC_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_timeout.log"


        rm -f "$TEMP_SOURCE"

        rm -f "$BINARY"


        return 6


    elif [ $STATUS -ne 0 ]; then


        echo "    [$TOOL] Execution FAILED."


        save_small_log \
            "$RUN_LOG" \
            "$EXEC_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.log"


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
            "$RUN_LOG" \
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

    local RESULT_FILE


    TEMP_SOURCE="$TEMP_SOURCE_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.c"


    RUN_LOG="$TEMP_LOG_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.log"


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


    if [ "$RUN_TIMEOUT" -gt 0 ]; then


        timeout \
            "$RUN_TIMEOUT" \
            python3 \
            "$PROFILER" \
            "$TEMP_SOURCE" \
            --timeout "$RUN_TIMEOUT" \
            > "$RUN_LOG" 2>&1


        STATUS=$?


    else


        python3 \
            "$PROFILER" \
            "$TEMP_SOURCE" \
            --timeout "$RUN_TIMEOUT" \
            > "$RUN_LOG" 2>&1


        STATUS=$?


    fi


    if [ $STATUS -eq 124 ]; then


        echo "    [$TOOL] TIMEOUT after ${RUN_TIMEOUT}s."


        save_small_log \
            "$RUN_LOG" \
            "$EXEC_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_timeout.log"


        rm -f "$TEMP_SOURCE"


        return 6


    elif [ $STATUS -ne 0 ]; then


        echo "    [$TOOL] Profiler execution FAILED."


        save_small_log \
            "$RUN_LOG" \
            "$EXEC_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.log"


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
            "$RUN_LOG" \
            "$OTHER_ERROR_DIR/"\
"${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_timing_parse.log"


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


    echo
    echo "Unique configurations: $TOTAL_EXPERIMENTS"


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


        echo
        echo "SUCCESS:"
        echo "$BENCHMARK | $PARAMETER=$VALUE completed for all paradigms."


    done < "$EXPERIMENT_FILE"
}


# ============================================================================
# PHASE 4
#
# UPDATE features.csv
#
#
# Matching key:
#
#     Benchmark
#     Parameter
#     Value
#     RegionID
#
#
# Columns:
#
#     SerialTime
#     OpenMP3Time
#
#     OpenMP45ResidentTime
#     OpenMP45ObservedTime
#     OpenMP45IsolatedTime
#
#     OpenACCResidentTime
#     OpenACCObservedTime
#     OpenACCIsolatedTime
# ============================================================================

update_csv()
{
    echo
    echo "======================================================================"
    echo "PHASE 4: UPDATING features.csv"
    echo "======================================================================"


    python3 - \
        "$FEATURES_CSV" \
        "$TEMP_RESULT_DIR" <<'PY'

import csv
import os
import re
import shutil
import sys


csv_file = sys.argv[1]

result_dir = sys.argv[2]


# ======================================================================
# CPU result reader
# ======================================================================

def read_cpu(
    benchmark,
    tool,
    parameter,
    value
):

    filename = os.path.join(
        result_dir,
        f"{benchmark}_{tool}_{parameter}_{value}.txt"
    )


    if not os.path.exists(filename):

        return {}


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


                result[region] = timing


            except ValueError:

                continue


    return result


# ======================================================================
# GPU result reader
# ======================================================================

def read_gpu(
    benchmark,
    tool,
    parameter,
    value
):

    filename = os.path.join(
        result_dir,
        f"{benchmark}_{tool}_{parameter}_{value}.txt"
    )


    if not os.path.exists(filename):

        return {}


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


                result[region] = (
                    resident,
                    observed,
                    isolated
                )


            except ValueError:

                continue


    return result


# ======================================================================
# Read features.csv
# ======================================================================

with open(
    csv_file,
    newline=""
) as f:


    reader = csv.DictReader(f)


    fieldnames = list(
        reader.fieldnames or []
    )


    rows = list(reader)


# ======================================================================
# Required timing columns
# ======================================================================

TIME_COLUMNS = [

    "SerialTime",

    "OpenMP3Time",

    "OpenMP45ResidentTime",

    "OpenMP45ObservedTime",

    "OpenMP45IsolatedTime",

    "OpenACCResidentTime",

    "OpenACCObservedTime",

    "OpenACCIsolatedTime"
]


for column in TIME_COLUMNS:


    if column not in fieldnames:

        fieldnames.append(column)


# ======================================================================
# Result caches
#
# Avoid repeatedly reopening same timing files for multiple regions.
# ======================================================================

cpu_cache = {}

gpu_cache = {}


updated_rows = 0


# ======================================================================
# Update every static-feature row
# ======================================================================

for row in rows:


    source_name = os.path.basename(
        row.get(
            "FileName",
            ""
        )
    )


    match = re.match(
        r"^(.*?)_serial_(.+)_([0-9]+)\.c$",
        source_name
    )


    if not match:

        continue


    benchmark = match.group(1)

    parameter = match.group(2)

    value = match.group(3)


    try:

        region_id = int(
            row["RegionID"]
        )


    except (
        KeyError,
        ValueError
    ):

        continue


    # ==================================================================
    # SERIAL
    # ==================================================================

    key = (
        benchmark,
        "serial",
        parameter,
        value
    )


    if key not in cpu_cache:


        cpu_cache[key] = read_cpu(
            benchmark,
            "serial",
            parameter,
            value
        )


    serial_result = cpu_cache[key]


    if region_id in serial_result:


        row["SerialTime"] = serial_result[
            region_id
        ]


    # ==================================================================
    # OPENMP3
    # ==================================================================

    key = (
        benchmark,
        "omp3",
        parameter,
        value
    )


    if key not in cpu_cache:


        cpu_cache[key] = read_cpu(
            benchmark,
            "omp3",
            parameter,
            value
        )


    omp3_result = cpu_cache[key]


    if region_id in omp3_result:


        row["OpenMP3Time"] = omp3_result[
            region_id
        ]


    # ==================================================================
    # OPENMP4.5
    # ==================================================================

    key = (
        benchmark,
        "omp45",
        parameter,
        value
    )


    if key not in gpu_cache:


        gpu_cache[key] = read_gpu(
            benchmark,
            "omp45",
            parameter,
            value
        )


    omp45_result = gpu_cache[key]


    if region_id in omp45_result:


        (
            resident,
            observed,
            isolated
        ) = omp45_result[
            region_id
        ]


        row[
            "OpenMP45ResidentTime"
        ] = resident


        row[
            "OpenMP45ObservedTime"
        ] = observed


        row[
            "OpenMP45IsolatedTime"
        ] = isolated


    # ==================================================================
    # OPENACC
    # ==================================================================

    key = (
        benchmark,
        "openacc",
        parameter,
        value
    )


    if key not in gpu_cache:


        gpu_cache[key] = read_gpu(
            benchmark,
            "openacc",
            parameter,
            value
        )


    openacc_result = gpu_cache[key]


    if region_id in openacc_result:


        (
            resident,
            observed,
            isolated
        ) = openacc_result[
            region_id
        ]


        row[
            "OpenACCResidentTime"
        ] = resident


        row[
            "OpenACCObservedTime"
        ] = observed


        row[
            "OpenACCIsolatedTime"
        ] = isolated


    updated_rows += 1


# ======================================================================
# Backup the static-feature CSV before timing insertion.
# ======================================================================

backup_file = (
    csv_file
    + ".before_execution_times.bak"
)


shutil.copy2(
    csv_file,
    backup_file
)


# ======================================================================
# Atomic CSV replacement
# ======================================================================

temp_csv = (
    csv_file
    + ".tmp"
)


with open(
    temp_csv,
    "w",
    newline=""
) as f:


    writer = csv.DictWriter(
        f,
        fieldnames=fieldnames
    )


    writer.writeheader()


    writer.writerows(rows)


os.replace(
    temp_csv,
    csv_file
)


print()

print(
    f"Rows processed : {updated_rows}"
)

print(
    f"Backup         : {backup_file}"
)

print(
    f"Updated CSV    : {csv_file}"
)

PY
}


# ============================================================================
# MAIN
# ============================================================================

echo
echo "======================================================================"
echo "             CAPC MASTER DATASET CREATION"
echo "======================================================================"
echo


validate_environment


generate_static_features


run_cpu_annotation


run_execution_sweep


update_csv


echo
echo "======================================================================"
echo "                  DATASET CREATION COMPLETE"
echo "======================================================================"
echo
echo "Final dataset:"
echo
echo "    $FEATURES_CSV"
echo
echo "Backup before timing insertion:"
echo
echo "    ${FEATURES_CSV}.before_execution_times.bak"
echo
echo "Compact error logs:"
echo
echo "    $LOG_ROOT"
echo
echo "Temporary workspace:"
echo
echo "    automatically deleted"
echo
echo "======================================================================"