#!/bin/bash

# ============================================================
# Run Feature Extractor for all C files in a given directory
#
# Usage:
#   ./run_feature_extractor.sh <path-to-directory>
#
# Example:
#   ./run_feature_extractor.sh "./Dataset Files"
#
# Run this script from the root directory of the project.
# ============================================================

if [ $# -ne 1 ]; then
    echo "Usage: $0 <directory-containing-serial-programs>"
    exit 1
fi

INPUT_DIR="$1"

# Check whether the input directory exists
if [ ! -d "$INPUT_DIR" ]; then
    echo "Error: Directory does not exist: $INPUT_DIR"
    exit 1
fi

# Change this if your feature extractor executable has a different name/path
FEATURE_EXTRACTOR="./build/ProfitabilityTool"

# Check whether the feature extractor exists
if [ ! -x "$FEATURE_EXTRACTOR" ]; then
    echo "Error: Feature extractor not found or not executable:"
    echo "       $FEATURE_EXTRACTOR"
    exit 1
fi

# Check whether at least one .c file exists
shopt -s nullglob
FILES=("$INPUT_DIR"/*.c)

if [ ${#FILES[@]} -eq 0 ]; then
    echo "No .c files found in: $INPUT_DIR"
    exit 1
fi

echo "============================================================"
echo "Running Feature Extractor"
echo "Input directory : $INPUT_DIR"
echo "Programs found  : ${#FILES[@]}"
echo "Output           : features.csv"
echo "============================================================"
echo

SUCCESS=0
FAILED=0

for FILE in "${FILES[@]}"; do

    echo "------------------------------------------------------------"
    echo "Processing: $FILE"
    echo "------------------------------------------------------------"

    "$FEATURE_EXTRACTOR" "$FILE"

    STATUS=$?

    if [ $STATUS -eq 0 ]; then
        echo "Completed: $(basename "$FILE")"
        SUCCESS=$((SUCCESS + 1))
    else
        echo "FAILED: $(basename "$FILE")"
        echo "Exit code: $STATUS"
        FAILED=$((FAILED + 1))
    fi

    echo
done

echo "============================================================"
echo "Feature extraction completed"
echo "============================================================"
echo "Total programs : ${#FILES[@]}"
echo "Successful     : $SUCCESS"
echo "Failed         : $FAILED"
echo "Output file    : features.csv"
echo "============================================================"