---
description: Command-line options of WinRoute.exe.
---

# Command Line

| Option | Function |
| --- | --- |
| `--minimized` | Start minimized to the system tray; used for autostart |
| `--install-driver=demo\|a\|b` | Install and start the specified NetFilter driver |
| `--uninstall-driver` | Stop and uninstall the NetFilter driver, and remove the WinDivert service |
| `--quiet` | Used with the two options above: show no dialogs and report the result only through the exit code (0 for success, 1 for failure) |

Example:

```
"C:\Program Files\WinRoute\WinRoute.exe" --install-driver=a --quiet
```

These commands must be run as administrator.
