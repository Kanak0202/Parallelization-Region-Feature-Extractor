```bash
#!/bin/bash

BASE_DIR="$(pwd)/outputs"

# PolyBench benchmark names
FILES=(
    "imageFiltering"
    "matrixTranspose"
    "particleFilter"
    "vectorNormalization"
)

created=0
skipped=0

# Create OpenACC, OpenMP 3, and OpenMP 4.5 files
for variant in openacc omp3 omp45; do
    DIR="$BASE_DIR/$variant/CAPC2"

    mkdir -p "$DIR"

    case "$variant" in
        openacc)
            suffix="_acc.c"
            ;;
        omp3)
            suffix="_omp3.c"
            ;;
        omp45)
            suffix="_omp45.c"
            ;;
    esac

    for file in "${FILES[@]}"; do
        output_file="$DIR/polybench-${file}${suffix}"

        if [[ -e "$output_file" ]]; then
            echo "Skipping existing: $output_file"
            ((skipped++))
        else
            touch "$output_file"
            echo "Created: $output_file"
            ((created++))
        fi
    done
done

# Create serial files
DIR="$BASE_DIR/serial/CAPC2"

mkdir -p "$DIR"

for file in "${FILES[@]}"; do
    output_file="$DIR/polybench-${file}_serial.c"

    if [[ -e "$output_file" ]]; then
        echo "Skipping existing: $output_file"
        ((skipped++))
    else
        touch "$output_file"
        echo "Created: $output_file"
        ((created++))
    fi
done

echo
echo "========================================"
echo "Finished."
echo "Created : $created files"
echo "Skipped : $skipped existing files"
echo "Total   : $((created + skipped)) files"
echo "========================================"
```
