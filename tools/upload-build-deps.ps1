<#
.SYNOPSIS
  Packs the build dependencies that are not in the repository and uploads them
  to the GitLab package registry, for the GitHub Actions build.

.DESCRIPTION
  The NetFilter SDK and the signed netfilter2.sys are not in the repository.
  The release build on GitHub (.github/workflows/release.yml) downloads them
  from this project's generic package registry (package "winroute-build-deps")
  with a read-only deploy token, and checks the file against the SHA-256 that
  is written in the workflow.

  This script makes build/winroute-build-deps-<Version>.zip with only what the
  build uses, prints its SHA-256 and, with a token, uploads it. Put the printed
  hash and version into BUILD_DEPS_SHA256 / BUILD_DEPS_VERSION in release.yml.

  Contents (paths as in the source tree):
    netfilter/nfsdk/include/*.h
    netfilter/nfsdk/wfp/include/*.h
    netfilter/nfsdk/wfp/bin/release_c_api/x64/nfapi.dll
    netfilter/nfsdk/wfp/bin/driver/x64/netfilter2.sys   (demo driver)
    netfilter2.sys                                       (driver signed by Sichuan Zhengyun)

.PARAMETER Version
  Version of the dependency package; use a new one whenever the SDK or the
  driver changes (the registry keeps the old files).

.PARAMETER Token
  GitLab token with the "api" scope. Without one the zip is only built.

.EXAMPLE
  $env:GITLAB_TOKEN = "glpat-..."
  tools\upload-build-deps.ps1 -Version 1
#>
param(
    [Parameter(Mandatory)][string]$Version,
    [string]$Token = $env:GITLAB_TOKEN,
    [string]$GitLab = "https://git.yanlei.org",
    [string]$Project = "kaguya/winroute"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$files = @(
    Get-ChildItem netfilter/nfsdk/include/*.h
    Get-ChildItem netfilter/nfsdk/wfp/include/*.h
    Get-Item netfilter/nfsdk/wfp/bin/release_c_api/x64/nfapi.dll
    Get-Item netfilter/nfsdk/wfp/bin/driver/x64/netfilter2.sys
    Get-Item netfilter2.sys
)

New-Item -ItemType Directory -Force build | Out-Null
$zip = Join-Path $root "build/winroute-build-deps-$Version.zip"
# An existing zip is reused: a new one has new timestamps, so a new SHA-256.
if (-not (Test-Path $zip)) {
    Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::Open($zip, [IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($file in $files) {
            $name = [IO.Path]::GetRelativePath($root, $file.FullName).Replace('\', '/')
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $name) | Out-Null
            Write-Host "  $name"
        }
    } finally {
        $archive.Dispose()
    }
}
$hash = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
Write-Host "$zip"
Write-Host "BUILD_DEPS_VERSION: '$Version'"
Write-Host "BUILD_DEPS_SHA256: $hash"

if (-not $Token) {
    Write-Host "No GITLAB_TOKEN: not uploaded."
    return
}
$api = "$GitLab/api/v4/projects/$([uri]::EscapeDataString($Project))"
$url = "$api/packages/generic/winroute-build-deps/$Version/winroute-build-deps.zip"
Invoke-RestMethod -Headers @{ "PRIVATE-TOKEN" = $Token } -Method Put -InFile $zip `
    -ContentType "application/octet-stream" $url | Out-Null
Write-Host "Uploaded: $url"
