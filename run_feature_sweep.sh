#!/bin/bash

# =====================================================================
# Adaptive CAPC Feature Sweep
#
# For every C file in a directory:
#
#   1. Detect numeric #define parameters that control for-loop bounds
#      inside:
#
#        #pragma capc profitability_region begin
#        ...
#        #pragma capc profitability_region end
#
#   2. Sweep each parameter independently using:
#
#        1, 2, 5,
#        10, 20, 50,
#        100, 200, 500,
#        ...
#
#   3. Run ProfitabilityTool for each generated configuration.
#
#   4. Examine the newly generated IterationSpace values.
#
#   5. Stop increasing that parameter after the largest region
#      iteration space reaches/exceeds MAX_ITERATION_SPACE.
#
#
# Original source files are NEVER modified.
#
# ProfitabilityTool appends directly to features.csv.
#
#
# Usage:
#
#   ./run_feature_sweep.sh <directory>
#
# Example:
#
#   ./run_feature_sweep.sh "../Dataset Files"
#
# Run from the directory containing:
#
#   ./ProfitabilityTool
#   features.csv
#
# =====================================================================


# =====================================================================
# Configuration
# =====================================================================

if [ $# -ne 1 ]; then
    echo "Usage: $0 <directory-containing-serial-programs>"
    exit 1
fi

INPUT_DIR="$1"

FEATURE_EXTRACTOR="./build/ProfitabilityTool"

CLANG_RESOURCE_DIR="$(clang -print-resource-dir)"


# ---------------------------------------------------------------------
# Maximum iteration space we intentionally sample.
#
# IMPORTANT:
#
# This is NOT a maximum #define value.
#
# It refers to the IterationSpace reported by ProfitabilityTool.
#
# Example:
#
#   N = 1000
#
# might produce:
#
#   one-loop region   -> 1e3
#   two-loop region   -> 1e6
#   three-loop region -> 1e9
#
# Once the largest affected region crosses this limit, that parameter
# sweep stops.
#
# Change this later if required.
# ---------------------------------------------------------------------

MAX_ITERATION_SPACE=1000000000


# ---------------------------------------------------------------------
# Safety limit on the raw macro value.
#
# Normally the adaptive iteration-space cutoff should stop first.
# ---------------------------------------------------------------------

MAX_PARAMETER_VALUE=1000000000


# ---------------------------------------------------------------------
# Adaptive logarithmic candidate values.
#
# This provides more useful resolution than powers of ten alone.
#
# 1, 2, 5 values are especially useful near profitability crossovers.
# ---------------------------------------------------------------------

VALUES=(
    1
    2
    5

    10
    20
    50

    100
    200
    500

    1000
    2000
    5000

    10000
    20000
    50000

    100000
    200000
    500000

    1000000
    2000000
    5000000

    10000000
    20000000
    50000000

    100000000
    200000000
    500000000

    1000000000
)


# =====================================================================
# Validation
# =====================================================================

if [ ! -d "$INPUT_DIR" ]; then
    echo "Error: Directory not found:"
    echo "       $INPUT_DIR"
    exit 1
fi


if [ ! -x "$FEATURE_EXTRACTOR" ]; then
    echo "Error: ProfitabilityTool not found or not executable:"
    echo "       $FEATURE_EXTRACTOR"
    exit 1
fi


if ! command -v python3 >/dev/null 2>&1; then
    echo "Error: python3 not found."
    exit 1
fi


if ! command -v clang >/dev/null 2>&1; then
    echo "Error: clang not found."
    exit 1
fi


if [ -z "$CLANG_RESOURCE_DIR" ]; then
    echo "Error: Could not determine clang resource directory."
    exit 1
fi


# =====================================================================
# Find source files
# =====================================================================

shopt -s nullglob
FILES=("$INPUT_DIR"/*.c)

if [ ${#FILES[@]} -eq 0 ]; then
    echo "Error: No .c files found in:"
    echo "       $INPUT_DIR"
    exit 1
fi


# =====================================================================
# Temporary working directory
# =====================================================================

TEMP_DIR="./feature_sweep_temp"

rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR"

trap 'rm -rf "$TEMP_DIR"' EXIT


# =====================================================================
# Counters
# =====================================================================

TOTAL_RUNS=0
SUCCESS=0
FAILED=0
SKIPPED=0


echo
echo "======================================================================"
echo "Adaptive CAPC Feature Sweep"
echo "======================================================================"
echo "Input directory       : $INPUT_DIR"
echo "Programs              : ${#FILES[@]}"
echo "Maximum iteration space: $MAX_ITERATION_SPACE"
echo "Output                 : features.csv"
echo "======================================================================"
echo


# =====================================================================
# Function:
#
# Detect numeric #defines influencing for-loop headers inside CAPC
# profitability regions.
#
# Also resolves derived macros such as:
#
#   #define N 2000
#   #define _PB_N N
#
#   for (... < _PB_N ...)
#
# Result:
#
#   N
# =====================================================================

detect_parameters()
{
    local FILE="$1"


    python3 - "$FILE" <<'PY'

import sys
import re

filename = sys.argv[1]

with open(filename, "r", errors="ignore") as f:
    text = f.read()


# ---------------------------------------------------------------------
# Remove comments
# ---------------------------------------------------------------------

clean = re.sub(
    r'/\*.*?\*/',
    '',
    text,
    flags=re.S
)

clean = re.sub(
    r'//.*',
    '',
    clean
)


# ---------------------------------------------------------------------
# Read object-style macros
# ---------------------------------------------------------------------

macros = {}

define_re = re.compile(
    r'^[ \t]*#[ \t]*define[ \t]+'
    r'([A-Za-z_][A-Za-z0-9_]*)'
    r'[ \t]+(.+?)$',
    re.M
)


for match in define_re.finditer(clean):

    name = match.group(1)
    expression = match.group(2).strip()

    macros[name] = expression


# ---------------------------------------------------------------------
# Numeric source macros
# ---------------------------------------------------------------------

numeric_re = re.compile(
    r'^\(?\s*[0-9]+\s*(?:[uUlL]+)?\s*\)?$'
)

numeric_macros = {
    name
    for name, expression in macros.items()
    if numeric_re.match(expression)
}


# ---------------------------------------------------------------------
# Extract CAPC regions
# ---------------------------------------------------------------------

begin_re = re.compile(
    r'^[ \t]*#[ \t]*pragma[ \t]+'
    r'capc[ \t]+profitability_region[ \t]+begin',
    re.M
)

end_re = re.compile(
    r'^[ \t]*#[ \t]*pragma[ \t]+'
    r'capc[ \t]+profitability_region[ \t]+end',
    re.M
)


regions = []

position = 0


while True:

    begin = begin_re.search(
        clean,
        position
    )

    if not begin:
        break


    end = end_re.search(
        clean,
        begin.end()
    )

    if not end:
        break


    regions.append(
        clean[
            begin.end():
            end.start()
        ]
    )

    position = end.end()


if not regions:
    sys.exit(0)


# ---------------------------------------------------------------------
# Extract identifiers from for(...) headers
# ---------------------------------------------------------------------

identifier_re = re.compile(
    r'\b[A-Za-z_][A-Za-z0-9_]*\b'
)

for_re = re.compile(
    r'\bfor\s*\((.*?)\)',
    re.S
)


used = set()


for region in regions:

    for match in for_re.finditer(region):

        header = match.group(1)

        for identifier in identifier_re.findall(header):

            used.add(identifier)


# ---------------------------------------------------------------------
# Macro dependency graph
# ---------------------------------------------------------------------

dependencies = {}


for name, expression in macros.items():

    dependencies[name] = {
        x
        for x in identifier_re.findall(expression)
        if x in macros and x != name
    }


# ---------------------------------------------------------------------
# Resolve a derived macro back to numeric macro(s)
# ---------------------------------------------------------------------

def numeric_sources(name, visited=None):

    if visited is None:
        visited = set()

    if name in visited:
        return set()

    visited.add(name)


    if name in numeric_macros:
        return {name}


    result = set()


    for dependency in dependencies.get(name, set()):

        result.update(
            numeric_sources(
                dependency,
                visited.copy()
            )
        )


    return result


parameters = set()


for identifier in used:

    if identifier in macros:

        parameters.update(
            numeric_sources(identifier)
        )


for parameter in sorted(parameters):

    print(parameter)

PY
}


# =====================================================================
# Function:
#
# Create source with selected numeric macro changed.
# =====================================================================

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


def replacement(match):

    return (
        match.group(1)
        + value
        + match.group(3)
    )


new_text, count = pattern.subn(
    replacement,
    text,
    count=1
)


if count == 0:

    print(
        f"Could not modify #define {parameter}",
        file=sys.stderr
    )

    sys.exit(1)


with open(destination, "w") as f:
    f.write(new_text)

PY
}


# =====================================================================
# Main sweep
# =====================================================================

for FILE in "${FILES[@]}"; do

    BASENAME=$(basename "$FILE")

    # Remove .c extension
    PROGRAM="${BASENAME%.c}"
    
    # If source filename already ends in _serial, remove that suffix.
    # Example:
    #   3mm_serial.c       -> 3mm
    #   jacobi-1D_serial.c -> jacobi-1D
    #
    # If the filename does not end in _serial, this does nothing.
    PROGRAM="${PROGRAM%_serial}"


    echo
    echo "######################################################################"
    echo "Program: $BASENAME"
    echo "######################################################################"


    mapfile -t PARAMETERS < <(
        detect_parameters "$FILE"
    )


    if [ ${#PARAMETERS[@]} -eq 0 ]; then

        echo "No iteration-space #define detected."
        echo "Skipping."

        SKIPPED=$((SKIPPED + 1))

        continue
    fi


    echo
    echo "Iteration-space parameter(s):"

    for PARAMETER in "${PARAMETERS[@]}"; do
        echo "    $PARAMETER"
    done


    # =================================================================
    # Sweep each parameter independently
    # =================================================================

    for PARAMETER in "${PARAMETERS[@]}"; do

        echo
        echo "======================================================================"
        echo "Parameter: $PARAMETER"
        echo "======================================================================"


        for VALUE in "${VALUES[@]}"; do


            if [ "$VALUE" -gt "$MAX_PARAMETER_VALUE" ]; then
                break
            fi


            TEMP_FILE="$TEMP_DIR/${PROGRAM}_serial_${PARAMETER}_${VALUE}.c"


            echo
            echo "----------------------------------------------------------------------"
            echo "Program   : $PROGRAM"
            echo "Parameter : $PARAMETER"
            echo "Value     : $VALUE"
            echo "----------------------------------------------------------------------"


            # ----------------------------------------------------------
            # Create modified source
            # ----------------------------------------------------------

            modify_define \
                "$FILE" \
                "$TEMP_FILE" \
                "$PARAMETER" \
                "$VALUE"


            if [ $? -ne 0 ]; then

                echo "Failed to create modified source."

                FAILED=$((FAILED + 1))

                continue
            fi


            # ----------------------------------------------------------
            # Record number of CSV rows before extraction
            # ----------------------------------------------------------

            if [ -f features.csv ]; then

                BEFORE_LINES=$(wc -l < features.csv)

            else

                BEFORE_LINES=0
            fi


            # ----------------------------------------------------------
            # Feature extraction
            # ----------------------------------------------------------

            "$FEATURE_EXTRACTOR" \
                "$TEMP_FILE" \
                -- \
                -resource-dir="$CLANG_RESOURCE_DIR" \
                -lm


            STATUS=$?

            TOTAL_RUNS=$((TOTAL_RUNS + 1))


            if [ $STATUS -ne 0 ]; then

                echo "Feature extraction FAILED."

                FAILED=$((FAILED + 1))

                rm -f "$TEMP_FILE"

                continue
            fi


            SUCCESS=$((SUCCESS + 1))


            # ----------------------------------------------------------
            # Determine maximum IterationSpace among ONLY the rows just
            # appended by this ProfitabilityTool execution.
            # ----------------------------------------------------------

            MAX_NEW_ITERATION_SPACE=$(
                python3 - \
                    features.csv \
                    "$BEFORE_LINES" <<'PY'

import csv
import sys


filename = sys.argv[1]
before_lines = int(sys.argv[2])


with open(filename, newline="") as f:

    lines = f.readlines()


# ---------------------------------------------------------------------
# features.csv contains one header line.
#
# before_lines is the number of lines present before this run.
#
# Therefore all lines after that point belong to this configuration.
# ---------------------------------------------------------------------

if not lines:

    print(0)
    sys.exit(0)


header = lines[0]


if before_lines == 0:

    new_lines = lines[1:]

else:

    new_lines = lines[before_lines:]


if not new_lines:

    print(0)
    sys.exit(0)


reader = csv.DictReader(
    [header] + new_lines
)


values = []


for row in reader:

    try:

        value = int(
            row["IterationSpace"]
        )

        values.append(value)

    except (ValueError, KeyError, TypeError):

        pass


if values:

    print(max(values))

else:

    print(0)

PY
            )


            echo "Maximum region IterationSpace = $MAX_NEW_ITERATION_SPACE"


            # ----------------------------------------------------------
            # We no longer need the generated source.
            #
            # The identifying filename has already been stored in the
            # FileName column by ProfitabilityTool.
            # ----------------------------------------------------------

            rm -f "$TEMP_FILE"


            # ----------------------------------------------------------
            # Adaptive stop
            #
            # IMPORTANT:
            #
            # We KEEP the configuration that crossed the threshold.
            #
            # This gives us one sample on/above the upper boundary.
            #
            # We then stop larger values for this parameter.
            # ----------------------------------------------------------

            if python3 - \
                "$MAX_NEW_ITERATION_SPACE" \
                "$MAX_ITERATION_SPACE" <<'PY'
import sys

actual = int(sys.argv[1])
limit = int(sys.argv[2])

sys.exit(
    0 if actual >= limit else 1
)
PY
            then

                echo
                echo "Iteration-space limit reached."
                echo
                echo "    actual : $MAX_NEW_ITERATION_SPACE"
                echo "    limit  : $MAX_ITERATION_SPACE"
                echo
                echo "Stopping larger values for $PARAMETER."

                break
            fi

        done

    done

done


# =====================================================================
# Summary
# =====================================================================

echo
echo "======================================================================"
echo "Adaptive Feature Sweep Completed"
echo "======================================================================"
echo "Programs found        : ${#FILES[@]}"
echo "Programs skipped      : $SKIPPED"
echo "Extractor runs        : $TOTAL_RUNS"
echo "Successful runs       : $SUCCESS"
echo "Failed runs           : $FAILED"
echo "Output                : features.csv"
echo "Maximum IterationSpace: $MAX_ITERATION_SPACE"
echo "======================================================================"