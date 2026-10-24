# Installs a WinRoute update; started by WinRoute (src/ui/Updater.cpp) right
# before it quits. Waits for WinRoute to exit, runs the downloaded setup
# silently (it keeps the installed driver and the tasks chosen before) and
# starts WinRoute again (the old version, should the setup fail). Problems are
# logged to %TEMP%\winroute-update\update.log.
param(
    [Parameter(Mandatory)] [string] $Setup,
    [Parameter(Mandatory)] [int] $ProcessId,
    [Parameter(Mandatory)] [string] $Exe,
    # Started again in the tray when it was there (--minimized).
    [switch] $Minimized
)

$ErrorActionPreference = 'Stop'
$log = Join-Path (Split-Path $Setup) 'update.log'

try {
    try { Wait-Process -Id $ProcessId -Timeout 60 } catch { }
    Start-Sleep -Milliseconds 500

    # WinRoute runs elevated, so this script and the setup do too: no UAC prompt.
    $process = Start-Process -FilePath $Setup -Wait -PassThru `
        -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-'
    if ($process.ExitCode -ne 0) { throw "The setup failed (exit code $($process.ExitCode))" }

    Remove-Item -LiteralPath $Setup -Force
}
catch {
    "$(Get-Date -Format s) $_" | Out-File -FilePath $log -Append -Encoding utf8
}

if ($Minimized) { Start-Process -FilePath $Exe -ArgumentList '--minimized' } else { Start-Process -FilePath $Exe }
