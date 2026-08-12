$serialDir = "serial\CAPC"

$targets = @{
    "omp3\CAPC"    = "omp3"
    "omp45\CAPC"   = "omp45"
    "openacc\CAPC" = "acc"
}

Get-ChildItem $serialDir -Filter "*_serial.c" | ForEach-Object {
    $prefix = $_.BaseName -replace "_serial$", ""

    foreach ($dir in $targets.Keys) {
        $suffix = $targets[$dir]
        $newFile = Join-Path $dir "$prefix`_$suffix.c"

        if (!(Test-Path $newFile)) {
            New-Item -ItemType File -Path $newFile | Out-Null
            Write-Host "Created $newFile"
        } else {
            Write-Host "Already exists: $newFile"
        }
    }
}
