param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$InputFile
)

$ErrorActionPreference = "Stop"

$resolved = (Resolve-Path -LiteralPath $InputFile).Path
if ([IO.Path]::GetExtension($resolved) -ne ".ms") {
    throw "Input must be a .ms MAXScript file."
}

$text = [IO.File]::ReadAllText($resolved)

$matches = [regex]::Matches(
    $text,
    '(?m)^(?<indent>[ \t]*)local[ \t]+(?<name>obj_[0-9]+)[ \t]*='
)

if ($matches.Count -eq 0) {
    throw "No top-level 'local obj_N =' declarations were found. The file may already be fixed or may be a different script."
}

$fixed = [regex]::Replace(
    $text,
    '(?m)^(?<indent>[ \t]*)local[ \t]+(?<name>obj_[0-9]+)[ \t]*=',
    '${indent}global ${name} ='
)

$directory = [IO.Path]::GetDirectoryName($resolved)
$basename = [IO.Path]::GetFileNameWithoutExtension($resolved)
$outputFile = [IO.Path]::Combine($directory, "${basename}_GMAX_FIXED.ms")

[IO.File]::WriteAllText(
    $outputFile,
    $fixed,
    [Text.UTF8Encoding]::new($false)
)

$remaining = [regex]::Matches(
    $fixed,
    '(?m)^[ \t]*local[ \t]+obj_[0-9]+[ \t]*='
).Count

Write-Host ""
Write-Host "Patched declarations: $($matches.Count)"
Write-Host "Remaining invalid object locals: $remaining"
Write-Host "Created: $outputFile"
Write-Host ""

if ($remaining -ne 0) {
    throw "Patch verification failed."
}
