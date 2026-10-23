<#
.SYNOPSIS
  Builds the WinRoute installer and publishes it as a GitLab release.

.DESCRIPTION
  The release build needs the NetFilter SDK and the signed driver, which are not
  in the repository, so releases are made from a machine that has them:

    1. reads the version from project(... VERSION x.y.z) in CMakeLists.txt;
    2. builds build/WinRoute-<version>-setup.exe (CMake target "installer");
    3. uploads it to the project's generic package registry (package "winroute");
    4. creates the release v<version> on the current commit, linking the package.

  The home page's release list (site/releases.html) reads the releases through
  the web server (deploy/nginx/), at most a minute later.

  Push the commit first: GitLab creates the tag v<version> on it.

.PARAMETER Notes
  Release notes, plain text: "- " starts a list item, short lines such as
  "New" or "Fixes" are headings. Defaults to "WinRoute <version>".

.PARAMETER Token
  GitLab personal or project access token with the "api" scope.
  Defaults to the GITLAB_TOKEN environment variable.

.EXAMPLE
  $env:GITLAB_TOKEN = "glpat-..."
  tools\publish-release.ps1 -Notes "New`n- UDP through SOCKS5"
#>
param(
    [string]$Notes,
    [string]$Token = $env:GITLAB_TOKEN,
    [string]$GitLab = "https://git.yanlei.org",
    [string]$Project = "kaguya/winroute",
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (-not $Token) { throw "Set GITLAB_TOKEN (or pass -Token) to a GitLab token with the api scope." }

$cmake = Get-Content CMakeLists.txt -Raw
if ($cmake -notmatch 'project\(WinRoute VERSION (\d+\.\d+\.\d+)') { throw "No version in CMakeLists.txt" }
$version = $Matches[1]
$tag = "v$version"
if (-not $Notes) { $Notes = "WinRoute $version" }

$commit = (git rev-parse HEAD).Trim()
if (git status --porcelain --untracked-files=no) { throw "The working tree has uncommitted changes." }
git fetch --quiet origin
if (-not (git branch -r --contains $commit)) { throw "Commit $commit is not pushed yet." }

$api = "$GitLab/api/v4/projects/$([uri]::EscapeDataString($Project))"
$headers = @{ "PRIVATE-TOKEN" = $Token }

try {
    Invoke-RestMethod -Headers $headers "$api/releases/$tag" | Out-Null
    throw "Release $tag already exists. Bump the version in CMakeLists.txt first."
} catch [Microsoft.PowerShell.Commands.HttpResponseException] {
    if ($_.Exception.Response.StatusCode -ne 404) { throw }
}

Write-Host "Building the installer for WinRoute $version"
cmake --build $BuildDir --target installer
if ($LASTEXITCODE -ne 0) { throw "The installer build failed." }
$file = "WinRoute-$version-setup.exe"
$path = Join-Path $BuildDir $file
if (-not (Test-Path $path)) { throw "$path was not built." }

Write-Host "Uploading $file"
$package = "$api/packages/generic/winroute/$version/$file"
Invoke-RestMethod -Headers $headers -Method Put -InFile $path -ContentType "application/octet-stream" $package | Out-Null

Write-Host "Creating the release $tag on $commit"
$release = @{
    tag_name    = $tag
    ref         = $commit
    name        = "WinRoute $version"
    description = $Notes
    assets      = @{ links = @(@{
        name              = "Windows x64 installer ($file)"
        url               = $package
        direct_asset_path = "/$file"
        link_type         = "package"
    }) }
} | ConvertTo-Json -Depth 5
Invoke-RestMethod -Headers $headers -Method Post -ContentType "application/json; charset=utf-8" `
    -Body ([Text.Encoding]::UTF8.GetBytes($release)) "$api/releases" | Out-Null

Write-Host "Released: $GitLab/$Project/-/releases/$tag"
