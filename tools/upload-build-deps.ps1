<#
.SYNOPSIS
  Packs the build dependencies that are not in the repository and uploads them
  to a private GitHub repository, for the GitHub Actions build.

.DESCRIPTION
  The NetFilter SDK and the drivers are not in the repository. The release
  build on GitHub (.github/workflows/release.yml) downloads them from the
  release "deps-<Version>" of a private repository (BUILD_DEPS_REPO) with a
  read-only token, and checks the file against the SHA-256 that is written in
  the workflow.

  This script makes build/winroute-build-deps-<Version>.zip with only what the
  build uses, prints its SHA-256 and, with a token, uploads it. Put the printed
  hash and version into BUILD_DEPS_SHA256 / BUILD_DEPS_VERSION in release.yml.

  Contents (paths as in the source tree):
    netfilter/nfsdk/include/*.h
    netfilter/nfsdk/wfp/include/*.h
    netfilter/nfsdk/wfp/bin/release_c_api/x64/nfapi.dll
    netfilter/driver/demo/netfilter2.sys   (demo driver)
    netfilter/driver/a/netfilter2.sys      (driver A)
    netfilter/driver/b/netfilter2.sys      (driver B)
    netfilter/driver/c/netfilter2.sys      (driver C)
  The drivers are the WFP x64 builds that NETFILTER_DRIVERS in CMakeLists.txt
  lists; keep the two lists the same.

  The private repository needs at least one commit (create it with a README),
  as a release needs a commit to tag.

.PARAMETER Version
  Version of the dependency package; use a new one whenever the SDK or a
  driver changes (old releases are kept).

.PARAMETER Token
  GitHub fine-grained token for the private repository with Contents: read and
  write. Defaults to the GITHUB_TOKEN environment variable. Without one the zip
  is only built (it can also be attached to the release by hand on GitHub).

.EXAMPLE
  $env:GITHUB_TOKEN = "github_pat_..."
  tools\upload-build-deps.ps1 -Version 4
#>
param(
    [Parameter(Mandatory)][string]$Version,
    [string]$Token = $env:GITHUB_TOKEN,
    [string]$Repo = "ChickenSeller/winroute-build-deps"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$files = @(
    Get-ChildItem netfilter/nfsdk/include/*.h
    Get-ChildItem netfilter/nfsdk/wfp/include/*.h
    Get-Item netfilter/nfsdk/wfp/bin/release_c_api/x64/nfapi.dll
    Get-Item netfilter/driver/demo/netfilter2.sys
    Get-Item netfilter/driver/a/netfilter2.sys
    Get-Item netfilter/driver/b/netfilter2.sys
    Get-Item netfilter/driver/c/netfilter2.sys
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
$tag = "deps-$Version"
Write-Host "$zip"
Write-Host "BUILD_DEPS_VERSION: '$Version'"
Write-Host "BUILD_DEPS_SHA256: $hash"

if (-not $Token) {
    Write-Host "No GITHUB_TOKEN: not uploaded. To attach it by hand, create the release $tag in"
    Write-Host "https://github.com/$Repo/releases/new and add the zip as winroute-build-deps.zip."
    return
}

$headers = @{
    Authorization          = "Bearer $Token"
    Accept                 = "application/vnd.github+json"
    "X-GitHub-Api-Version" = "2022-11-28"
}
$api = "https://api.github.com/repos/$Repo"
try {
    $release = Invoke-RestMethod -Headers $headers "$api/releases/tags/$tag"
} catch [Microsoft.PowerShell.Commands.HttpResponseException] {
    if ($_.Exception.Response.StatusCode -ne 404) { throw }
    $body = @{ tag_name = $tag; name = "Build dependencies $Version"; body = "SHA-256 of winroute-build-deps.zip: $hash" } | ConvertTo-Json
    $release = Invoke-RestMethod -Headers $headers -Method Post -ContentType "application/json" -Body $body "$api/releases"
}
if ($release.assets | Where-Object name -eq "winroute-build-deps.zip") {
    throw "Release $tag already has winroute-build-deps.zip. Use a new -Version for new contents."
}
$upload = "https://uploads.github.com/repos/$Repo/releases/$($release.id)/assets?name=winroute-build-deps.zip"
Invoke-RestMethod -Headers $headers -Method Post -ContentType "application/zip" -InFile $zip $upload | Out-Null
Write-Host "Uploaded: https://github.com/$Repo/releases/tag/$tag"
