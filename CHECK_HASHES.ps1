param([string]$Root = $PSScriptRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
try {
    # A PowerShell 7 parent can pass a PSModulePath that omits Windows
    # PowerShell's modules. Import the built-in utility module explicitly.
    Import-Module -Name (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility\Microsoft.PowerShell.Utility.psd1') -Force -ErrorAction Stop
    $bundleRoot = [System.IO.Path]::GetFullPath($Root)
    $prefix = $bundleRoot.TrimEnd([char[]]'\/') + [System.IO.Path]::DirectorySeparatorChar
    $manifest = Join-Path $bundleRoot 'SHA256SUMS.txt'
    if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) {
        throw 'Missing SHA256SUMS.txt.'
    }
    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    $checked = 0
    foreach ($line in [System.IO.File]::ReadAllLines($manifest)) {
        if ([string]::IsNullOrWhiteSpace($line) -or $line.StartsWith('#')) { continue }
        if ($line -notmatch '^([0-9A-Fa-f]{64}) [ *](.+)$') {
            throw "Malformed SHA256SUMS.txt line: $line"
        }
        $expected = $Matches[1]
        $relative = $Matches[2]
        if ([System.IO.Path]::IsPathRooted($relative)) {
            throw "Absolute path in manifest: $relative"
        }
        $path = [System.IO.Path]::GetFullPath((Join-Path $bundleRoot $relative))
        if (-not $path.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Path outside bundle in manifest: $relative"
        }
        if (-not $seen.Add($path)) { throw "Duplicate manifest path: $relative" }
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing file: $relative" }
        $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        if ($actual -ne $expected) { throw "SHA256 mismatch: $relative" }
        $checked++
    }
    if ($checked -eq 0) { throw 'The checksum manifest contains no files.' }
    Write-Output "HASH CHECK PASSED: $checked files."
    Write-Output 'File hashes identify the distributed bytes; they do not establish the mathematical result.'
    exit 0
} catch {
    [Console]::Error.WriteLine('HASH CHECK FAILED: ' + $_.Exception.Message)
    exit 1
}
