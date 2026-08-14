#!/bin/bash

# =====================================================================
# CAPC Execution-Time Sweep + CSV Update
#
# Expected directory structure:
#
# outputs/
# └── annotated/
#     ├── serial/
#     │   └── CAPC/
#     ├── omp3/
#     │   └── CAPC/
#     ├── omp45/
#     │   └── CAPC/
#     └── openacc/
#         └── CAPC/
#
#
# Expected FileName format already present in features.csv:
#
#   3mm_serial_N_1.c
#   3mm_serial_N_10.c
#   3mm_serial_N_100.c
#   jacobi-1D_serial_N_1000.c
#   ...
#
#
# Added CSV columns:
#
#   SerialTime
#   OpenMP3Time
#   OpenMP45ResidentTime
#   OpenMP45IsolatedTime
#   OpenACCResidentTime
#   OpenACCIsolatedTime
#
#
# Compilation commands:
#
# Serial:
#   gcc -fopenmp filename.c -o filename
#
# OpenMP 3:
#   gcc -fopenmp filename.c -o filename
#
# OpenMP 4.5:
#   nvc -mp=gpu -Minfo=mp filename.c -o filename
#
# OpenACC:
#   nvc -acc -gpu=cc70 -Minfo=accel filename.c -o filename
#
#
# Usage:
#
#   ./collect_execution_times.sh features.csv
#
# Run from the project root.
# =====================================================================


# =====================================================================
# Arguments
# =====================================================================

ulimit -s unlimited

if [ $# -ne 1 ]; then
    echo "Usage: $0 <features.csv>"
    exit 1
fi

FEATURES_CSV="$1"


# =====================================================================
# Paths
# =====================================================================

ANNOTATED_ROOT="./outputs/annotated"

SERIAL_DIR="$ANNOTATED_ROOT/serial/CAPC"
OMP3_DIR="$ANNOTATED_ROOT/omp3/CAPC"
OMP45_DIR="$ANNOTATED_ROOT/omp45/CAPC"
OPENACC_DIR="$ANNOTATED_ROOT/openacc/CAPC"


# =====================================================================
# Runtime limit
#
# Prevents huge problem sizes from running forever.
#
# 300 seconds = 5 minutes
#
# Change if required.
# =====================================================================

RUN_TIMEOUT=10000


# =====================================================================
# Validation
# =====================================================================

if [ ! -f "$FEATURES_CSV" ]; then
    echo "Error: CSV file not found:"
    echo "       $FEATURES_CSV"
    exit 1
fi


for DIR in \
    "$SERIAL_DIR" \
    "$OMP3_DIR" \
    "$OMP45_DIR" \
    "$OPENACC_DIR"
do
    if [ ! -d "$DIR" ]; then
        echo "Error: Directory not found:"
        echo "       $DIR"
        exit 1
    fi
done


if ! command -v gcc >/dev/null 2>&1; then
    echo "Error: gcc not found."
    exit 1
fi


if ! command -v nvc >/dev/null 2>&1; then
    echo "Error: nvc not found."
    exit 1
fi


if ! command -v python3 >/dev/null 2>&1; then
    echo "Error: python3 not found."
    exit 1
fi


if ! command -v timeout >/dev/null 2>&1; then
    echo "Error: timeout command not found."
    exit 1
fi


# =====================================================================
# Temporary working directories
# =====================================================================

WORK_DIR="./execution_sweep_temp"

SOURCE_DIR="$WORK_DIR/source"
BIN_DIR="$WORK_DIR/bin"
LOG_DIR="$WORK_DIR/logs"
RESULT_DIR="$WORK_DIR/results"

rm -rf "$WORK_DIR"

mkdir -p "$SOURCE_DIR"
mkdir -p "$BIN_DIR"
mkdir -p "$LOG_DIR"
mkdir -p "$RESULT_DIR"


# =====================================================================
# Build experiment list from features.csv
#
# Example filename:
#
#   3mm_serial_N_100.c
#
# becomes:
#
#   benchmark = 3mm
#   parameter = N
#   value     = 100
#
# Each unique configuration is executed only once.
# =====================================================================

EXPERIMENT_FILE="$WORK_DIR/experiments.txt"


python3 - "$FEATURES_CSV" > "$EXPERIMENT_FILE" <<'PY'

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

        # Expected:
        #
        # benchmark_serial_PARAMETER_VALUE.c
        #
        # Final component must be numeric value.

        match = re.match(
            r"^(.*?)_serial_(.+)_([0-9]+)\.c$",
            filename
        )

        if not match:
            continue

        benchmark = match.group(1)
        parameter = match.group(2)
        value = int(match.group(3))

        experiments.add(
            (benchmark, parameter, value)
        )


for benchmark, parameter, value in sorted(
    experiments,
    key=lambda x: (x[0], x[1], x[2])
):
    print(
        f"{benchmark}|{parameter}|{value}"
    )

PY


TOTAL_EXPERIMENTS=$(wc -l < "$EXPERIMENT_FILE")


echo
echo "======================================================================"
echo "CAPC Execution-Time Sweep"
echo "======================================================================"
echo "CSV                  : $FEATURES_CSV"
echo "Annotated root       : $ANNOTATED_ROOT"
echo "Experiments          : $TOTAL_EXPERIMENTS"
echo "Timeout per execution: ${RUN_TIMEOUT}s"
echo "======================================================================"
echo


# =====================================================================
# Find source program for a benchmark/paradigm
#
# Typical examples:
#
# Serial:
#   3mm_serial.c
#
# OMP3:
#   3mm_omp3.c
#
# OMP45:
#   3mm_omp45.c
#
# OpenACC:
#   3mm_acc.c
#
# =====================================================================

find_source()
{
    local TOOL="$1"
    local BENCHMARK="$2"

    local DIR=""
    local EXACT=""


    case "$TOOL" in

        serial)
            DIR="$SERIAL_DIR"
            EXACT="$DIR/${BENCHMARK}_serial.c"
            ;;

        omp3)
            DIR="$OMP3_DIR"
            EXACT="$DIR/${BENCHMARK}_omp3.c"
            ;;

        omp45)
            DIR="$OMP45_DIR"
            EXACT="$DIR/${BENCHMARK}_omp45.c"
            ;;

        openacc)
            DIR="$OPENACC_DIR"
            EXACT="$DIR/${BENCHMARK}_acc.c"
            ;;

        *)
            return 1
            ;;

    esac


    # Prefer exact expected filename

    if [ -f "$EXACT" ]; then
        echo "$EXACT"
        return 0
    fi


    # Fallback search

    local matches=()

    shopt -s nullglob

    matches=(
        "$DIR/${BENCHMARK}"*.c
    )

    shopt -u nullglob


    if [ ${#matches[@]} -eq 1 ]; then

        echo "${matches[0]}"
        return 0

    elif [ ${#matches[@]} -gt 1 ]; then

        echo "Error: Multiple source candidates found for $BENCHMARK in $DIR" >&2

        for f in "${matches[@]}"; do
            echo "       $f" >&2
        done

        return 1

    fi


    return 1
}


# =====================================================================
# Modify the numeric #define in a temporary copy
#
# Handles:
#
#   #define N 100
#   #define N (100)
#   #define N 100L
#   #define N 100UL
# =====================================================================

modify_define()
{
    local INPUT="$1"
    local OUTPUT="$2"
    local PARAMETER="$3"
    local VALUE="$4"


    python3 - "$INPUT" "$OUTPUT" "$PARAMETER" "$VALUE" <<'PY'

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

    prefix = match.group(1)
    suffix = match.group(3)

    return prefix + value + suffix


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


# =====================================================================
# CPU timing parser
#
# Handles serial / OpenMP3:
#
# Region 0: total=0.334075656 s, executions=1, average=0.334075656 s
#
# Result:
#
#   0|0.334075656
#   1|0.125216586
# =====================================================================

extract_cpu_times()
{
    local LOG_FILE="$1"
    local RESULT_FILE="$2"


    python3 - "$LOG_FILE" "$RESULT_FILE" <<'PY'

import re
import sys

log_file = sys.argv[1]
result_file = sys.argv[2]


with open(log_file, "r", errors="ignore") as f:
    text = f.read()


results = {}


pattern = re.compile(
    r'Region\s+(\d+)\s*:.*?'
    r'average\s*=\s*([0-9.eE+-]+)\s*s',
    re.IGNORECASE
)


for match in pattern.finditer(text):

    region = int(match.group(1))
    average = match.group(2)

    results[region] = average


with open(result_file, "w") as f:

    for region in sorted(results):

        f.write(
            f"{region}|{results[region]}\n"
        )

PY
}


# =====================================================================
# GPU timing parser
#
# Handles OMP45 / OpenACC:
#
# region_1 (...):
# resident_avg=0.001038 s
# isolated_avg=0.429311 s
# calls=500
#
#
# Result:
#
#   RegionID | ResidentTime | IsolatedTime
#
#   0|0.001678|0.429951
#   1|0.001038|0.429311
# =====================================================================

extract_gpu_times()
{
    local LOG_FILE="$1"
    local RESULT_FILE="$2"


    python3 - "$LOG_FILE" "$RESULT_FILE" <<'PY'

import re
import sys

log_file = sys.argv[1]
result_file = sys.argv[2]


with open(log_file, "r", errors="ignore") as f:
    text = f.read()


results = {}


pattern = re.compile(
    r'region_(\d+)'
    r'.*?'
    r'resident_avg\s*=\s*([0-9.eE+-]+)\s*s'
    r'.*?'
    r'isolated_avg\s*=\s*([0-9.eE+-]+)\s*s',
    re.IGNORECASE
)


for line in text.splitlines():

    match = pattern.search(line)

    if not match:
        continue

    region = int(match.group(1))
    resident = match.group(2)
    isolated = match.group(3)

    results[region] = (
        resident,
        isolated
    )


with open(result_file, "w") as f:

    for region in sorted(results):

        resident, isolated = results[region]

        f.write(
            f"{region}|{resident}|{isolated}\n"
        )

PY
}


# =====================================================================
# Compile + execute one configuration
# =====================================================================

compile_and_run()
{
    local TOOL="$1"
    local BENCHMARK="$2"
    local PARAMETER="$3"
    local VALUE="$4"


    # -----------------------------------------------------------------
    # Locate source
    # -----------------------------------------------------------------

    ORIGINAL_SOURCE=$(find_source "$TOOL" "$BENCHMARK")

    if [ $? -ne 0 ] || [ -z "$ORIGINAL_SOURCE" ]; then

        echo "    [$TOOL] Source not found."

        return 1
    fi


    # -----------------------------------------------------------------
    # Temporary filenames
    # -----------------------------------------------------------------

    TEMP_SOURCE="$SOURCE_DIR/${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.c"

    BINARY="$BIN_DIR/${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}"

    COMPILE_LOG="$LOG_DIR/${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}_compile.log"

    RUN_LOG="$LOG_DIR/${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.log"

    RESULT_FILE="$RESULT_DIR/${BENCHMARK}_${TOOL}_${PARAMETER}_${VALUE}.txt"

    if [ -s "$RESULT_FILE" ]; then
        echo "    [$TOOL] Existing timing result found -- skipping."
        return 0
    fi

    # -----------------------------------------------------------------
    # Modify parameter
    # -----------------------------------------------------------------

    modify_define \
        "$ORIGINAL_SOURCE" \
        "$TEMP_SOURCE" \
        "$PARAMETER" \
        "$VALUE"


    if [ $? -ne 0 ]; then

        echo "    [$TOOL] Failed to modify #define $PARAMETER."

        return 1
    fi


    # -----------------------------------------------------------------
    # Compile
    # -----------------------------------------------------------------

    echo "    [$TOOL] Compiling..."


    case "$TOOL" in

        serial)

            gcc \
                -fopenmp \
                "$TEMP_SOURCE" \
                -o "$BINARY" \
                -lm \
                > "$COMPILE_LOG" 2>&1
            ;;


        omp3)

            gcc \
                -fopenmp \
                "$TEMP_SOURCE" \
                -o "$BINARY" \
                -lm \
                > "$COMPILE_LOG" 2>&1
            ;;


        omp45)

            nvc \
                -mp=gpu \
                -Minfo=mp \
                "$TEMP_SOURCE" \
                -o "$BINARY" \
                -lm \
                > "$COMPILE_LOG" 2>&1
            ;;


        openacc)

            nvc \
                -acc \
                -gpu=cc70 \
                -Minfo=accel \
                "$TEMP_SOURCE" \
                -o "$BINARY" \
                -lm \
                > "$COMPILE_LOG" 2>&1
            ;;


        *)

            echo "    Unknown tool: $TOOL"
            return 1
            ;;

    esac


    COMPILE_STATUS=$?


    if [ $COMPILE_STATUS -ne 0 ]; then

        echo "    [$TOOL] Compilation FAILED."
        echo "    Compile log: $COMPILE_LOG"

        # Keep only a small part of the compile log
        if [ -f "$COMPILE_LOG" ]; then
            tail -n 100 "$COMPILE_LOG" > "${COMPILE_LOG}.tmp"
            mv "${COMPILE_LOG}.tmp" "$COMPILE_LOG"
        fi

        rm -f "$TEMP_SOURCE"
        rm -f "$BINARY"

        # Special return code:
        # 2 = compilation failure
        return 2
    fi


    # -----------------------------------------------------------------
    # Execute
    # -----------------------------------------------------------------

    echo "    [$TOOL] Running..."
    
    set -o pipefail
    
    if [ "$RUN_TIMEOUT" -gt 0 ]; then
    
        timeout "$RUN_TIMEOUT" \
            "$BINARY" \
            2>&1 | \
            grep -E 'Region [0-9]+:|region_[0-9]+' \
            > "$RUN_LOG"
    
        RUN_STATUS=${PIPESTATUS[0]}
    
    else
    
        "$BINARY" \
            2>&1 | \
            grep -E 'Region [0-9]+:|region_[0-9]+' \
            > "$RUN_LOG"
    
        RUN_STATUS=${PIPESTATUS[0]}
    
    fi
    
    
    if [ $RUN_STATUS -eq 124 ]; then
    
        echo "    [$TOOL] TIMEOUT after ${RUN_TIMEOUT}s."
    
        rm -f "$TEMP_SOURCE"
        rm -f "$BINARY"
        rm -f "$COMPILE_LOG"
    
        return 1
    
    elif [ $RUN_STATUS -ne 0 ]; then
    
        echo "    [$TOOL] Execution FAILED."
        echo "    Exit code: $RUN_STATUS"
        echo "    Run log : $RUN_LOG"
    
        rm -f "$TEMP_SOURCE"
        rm -f "$BINARY"
        rm -f "$COMPILE_LOG"
    
        return 1
    fi

    # -----------------------------------------------------------------
    # Parse region timings
    # -----------------------------------------------------------------

    case "$TOOL" in

        serial|omp3)

            extract_cpu_times \
                "$RUN_LOG" \
                "$RESULT_FILE"
            ;;


        omp45|openacc)

            extract_gpu_times \
                "$RUN_LOG" \
                "$RESULT_FILE"
            ;;

    esac


    # -----------------------------------------------------------------
    # Verify timing data exists
    # -----------------------------------------------------------------

    if [ ! -s "$RESULT_FILE" ]; then

        echo "    [$TOOL] Execution completed but no region timings were found."
        echo "    Run log: $RUN_LOG"

        return 1
    fi


    REGION_COUNT=$(wc -l < "$RESULT_FILE")


    echo "    [$TOOL] Success: $REGION_COUNT region(s) extracted."

    # Clean large temporary files
    rm -f "$RUN_LOG"
    rm -f "$COMPILE_LOG"
    rm -f "$TEMP_SOURCE"
    rm -f "$BINARY"


    return 0
}


# =====================================================================
# Main execution sweep
#
# If ANY paradigm fails to compile for a Benchmark+Parameter at value X,
# all larger values for that same Benchmark+Parameter are skipped.
# =====================================================================

CURRENT=0

# Stores Benchmark|Parameter combinations that should no longer be tested
declare -A STOP_SWEEP


while IFS='|' read -r BENCHMARK PARAMETER VALUE
do

    [ -z "$BENCHMARK" ] && continue

    CURRENT=$((CURRENT + 1))

    SWEEP_KEY="${BENCHMARK}|${PARAMETER}"


    # -----------------------------------------------------------------
    # Check whether a smaller value already failed compilation
    # -----------------------------------------------------------------

    if [ "${STOP_SWEEP[$SWEEP_KEY]}" = "1" ]; then

        echo
        echo "======================================================================"
        echo "Experiment $CURRENT / $TOTAL_EXPERIMENTS"
        echo "======================================================================"
        echo "Benchmark : $BENCHMARK"
        echo "Parameter : $PARAMETER"
        echo "Value     : $VALUE"
        echo "======================================================================"
        echo
        echo "SKIPPED:"
        echo "A compilation or execution failure occurred at a smaller value for:"
        echo
        echo "    Benchmark : $BENCHMARK"
        echo "    Parameter : $PARAMETER"
        echo
        echo "Therefore value $VALUE and all subsequent larger values are skipped."
        echo

        continue
    fi


    echo
    echo "======================================================================"
    echo "Experiment $CURRENT / $TOTAL_EXPERIMENTS"
    echo "======================================================================"
    echo "Benchmark : $BENCHMARK"
    echo "Parameter : $PARAMETER"
    echo "Value     : $VALUE"
    echo "======================================================================"
    echo


    CONFIGURATION_FAILED=0


    # =================================================================
    # Serial
    # =================================================================

    compile_and_run \
        "serial" \
        "$BENCHMARK" \
        "$PARAMETER" \
        "$VALUE"

    STATUS=$?

    if [ $STATUS -ne 0 ]; then

        echo
        echo "    Serial failed at this value."

        CONFIGURATION_FAILED=1
    fi


    # =================================================================
    # OpenMP 3
    #
    # We still test all paradigms at the CURRENT value so we know
    # exactly which ones compile and which do not.
    # =================================================================

    compile_and_run \
        "omp3" \
        "$BENCHMARK" \
        "$PARAMETER" \
        "$VALUE"

    STATUS=$?

    if [ $STATUS -ne 0 ]; then

        echo
        echo "    OpenMP3 failed at this value."

        CONFIGURATION_FAILED=1
    fi


    # =================================================================
    # OpenMP 4.5
    # =================================================================

    compile_and_run \
        "omp45" \
        "$BENCHMARK" \
        "$PARAMETER" \
        "$VALUE"

    STATUS=$?

    if [ $STATUS -ne 0 ]; then

        echo
        echo "    OpenMP4.5 failed at this value."

        CONFIGURATION_FAILED=1
    fi


    # =================================================================
    # OpenACC
    # =================================================================

    compile_and_run \
        "openacc" \
        "$BENCHMARK" \
        "$PARAMETER" \
        "$VALUE"

    STATUS=$?

    if [ $STATUS -ne 0 ]; then

        echo
        echo "    OpenACC failed at this value."

        CONFIGURATION_FAILED=1
    fi


    # =================================================================
    # Stop larger values if ANY compilation failed
    # =================================================================

    if [ $CONFIGURATION_FAILED -eq 1 ]; then

        STOP_SWEEP[$SWEEP_KEY]=1

        echo
        echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        echo "Execution/Compilation limit reached"
        echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        echo "Benchmark : $BENCHMARK"
        echo "Parameter : $PARAMETER"
        echo "Value     : $VALUE"
        echo
        echo "At least one paradigm failed to compile or execute."
        echo
        echo "All larger values for this Benchmark/Parameter will now be skipped."
        echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        echo
    fi


done < "$EXPERIMENT_FILE"



# =====================================================================
# Update features.csv
# =====================================================================

echo
echo "======================================================================"
echo "Updating features.csv"
echo "======================================================================"


python3 - "$FEATURES_CSV" "$RESULT_DIR" <<'PY'

import csv
import os
import re
import shutil
import sys


csv_file = sys.argv[1]
result_dir = sys.argv[2]


# =====================================================================
# Read CPU result
# =====================================================================

def read_cpu_result(
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

            line = line.strip()

            if not line:
                continue


            parts = line.split("|")


            if len(parts) != 2:
                continue


            try:

                region = int(parts[0])
                timing = parts[1]

                result[region] = timing

            except ValueError:

                continue


    return result


# =====================================================================
# Read GPU result
# =====================================================================

def read_gpu_result(
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

            line = line.strip()

            if not line:
                continue


            parts = line.split("|")


            if len(parts) != 3:
                continue


            try:

                region = int(parts[0])

                resident = parts[1]
                isolated = parts[2]

                result[region] = (
                    resident,
                    isolated
                )

            except ValueError:

                continue


    return result


# =====================================================================
# Read CSV
# =====================================================================

with open(csv_file, newline="") as f:

    reader = csv.DictReader(f)

    fieldnames = list(
        reader.fieldnames or []
    )

    rows = list(reader)


# =====================================================================
# Add timing columns
# =====================================================================

TIME_COLUMNS = [

    "SerialTime",

    "OpenMP3Time",

    "OpenMP45ResidentTime",
    "OpenMP45IsolatedTime",

    "OpenACCResidentTime",
    "OpenACCIsolatedTime"
]


for column in TIME_COLUMNS:

    if column not in fieldnames:
        fieldnames.append(column)


# =====================================================================
# Timing cache
# =====================================================================

cpu_cache = {}
gpu_cache = {}


# =====================================================================
# Update CSV rows
# =====================================================================

updated_rows = 0


for row in rows:

    source_name = os.path.basename(
        row.get("FileName", "")
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

    except (KeyError, ValueError):

        continue


    # -----------------------------------------------------------------
    # Serial
    # -----------------------------------------------------------------

    key = (
        benchmark,
        "serial",
        parameter,
        value
    )


    if key not in cpu_cache:

        cpu_cache[key] = read_cpu_result(
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


    # -----------------------------------------------------------------
    # OpenMP 3
    # -----------------------------------------------------------------

    key = (
        benchmark,
        "omp3",
        parameter,
        value
    )


    if key not in cpu_cache:

        cpu_cache[key] = read_cpu_result(
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


    # -----------------------------------------------------------------
    # OpenMP 4.5
    # -----------------------------------------------------------------

    key = (
        benchmark,
        "omp45",
        parameter,
        value
    )


    if key not in gpu_cache:

        gpu_cache[key] = read_gpu_result(
            benchmark,
            "omp45",
            parameter,
            value
        )


    omp45_result = gpu_cache[key]


    if region_id in omp45_result:

        resident, isolated = omp45_result[
            region_id
        ]

        row[
            "OpenMP45ResidentTime"
        ] = resident

        row[
            "OpenMP45IsolatedTime"
        ] = isolated


    # -----------------------------------------------------------------
    # OpenACC
    # -----------------------------------------------------------------

    key = (
        benchmark,
        "openacc",
        parameter,
        value
    )


    if key not in gpu_cache:

        gpu_cache[key] = read_gpu_result(
            benchmark,
            "openacc",
            parameter,
            value
        )


    acc_result = gpu_cache[key]


    if region_id in acc_result:

        resident, isolated = acc_result[
            region_id
        ]

        row[
            "OpenACCResidentTime"
        ] = resident

        row[
            "OpenACCIsolatedTime"
        ] = isolated


    updated_rows += 1


# =====================================================================
# Backup original CSV
# =====================================================================

backup_file = (
    csv_file + ".before_execution_times.bak"
)


shutil.copy2(
    csv_file,
    backup_file
)


# =====================================================================
# Write updated CSV atomically
# =====================================================================

temp_csv = csv_file + ".tmp"


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


# =====================================================================
# Final summary
# =====================================================================

echo
echo "======================================================================"
echo "CAPC Timing Collection Complete"
echo "======================================================================"
echo
echo "CSV:"
echo "    $FEATURES_CSV"
echo
echo "Timing columns:"
echo "    SerialTime"
echo "    OpenMP3Time"
echo "    OpenMP45ResidentTime"
echo "    OpenMP45IsolatedTime"
echo "    OpenACCResidentTime"
echo "    OpenACCIsolatedTime"
echo
echo "Execution logs:"
echo "    $LOG_DIR"
echo
echo "Timing result files:"
echo "    $RESULT_DIR"
echo
echo "Backup:"
echo "    ${FEATURES_CSV}.before_execution_times.bak"
echo
echo "======================================================================"