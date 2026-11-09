---
description: Installation, network driver selection, interception backends and uninstallation.
---

# Installation and Network Driver

## Installation

Run the installer. You can create a desktop shortcut and start WinRoute automatically at logon. The installer is available in English, Simplified Chinese and Japanese. If WinRoute is running, the installer and uninstaller ask you to close it first.

### Selecting a driver

The "Network Driver" page of the installer selects the driver used to intercept connections:

| Driver | Description |
| --- | --- |
| NetFilter SDK demo driver (default) | Fully functional, but the total number of intercepted connections is limited. When the limit is reached, Windows must be restarted |
| Driver A / Driver B | Release drivers with no connection limit. If one is unstable on your system, use the other |

Upgrades keep the previous choice. The driver can also be changed at any time in the application, as described below.

### Silent installation

```
WinRoute-x.y.z-setup.exe /VERYSILENT /SUPPRESSMSGBOXES /TASKS=autostart /DRIVER=a
```

- `/TASKS=autostart`: create the autostart task.
- `/DRIVER=demo|a|b`: select the driver. If omitted, a new installation uses the demo driver and an upgrade keeps the previous choice.

## Changing the interception backend and driver

Menu: **Tools > Network Driver...**

WinRoute provides two interception backends, which can be switched at any time without reinstalling.

### NetFilter (recommended)

Based on a WFP driver; rewrites the destination when an application opens a connection. Supports all features, including UDP forwarding and name resolution through the proxy.

In this window you can install, switch or uninstall the demo driver and drivers A and B. When switching, WinRoute stops interception, replaces the driver file and starts interception again, which usually takes a few seconds.

### WinDivert

Intercepts TCP and UDP at the packet level and takes effect as soon as it is selected. Its kernel service is installed automatically the first time it starts. Use it where the NetFilter driver cannot be loaded. The WinDivert service can also be uninstalled in this window.

::: tip
The status bar at the bottom of the main window shows the current backend, the driver and whether it is running. Click the driver signature to view the details of the signing certificate.
:::

## Uninstallation

Uninstall WinRoute from **Settings > Apps** in Windows. The uninstaller:

1. removes the autostart task;
2. stops and removes the network driver;
3. asks whether to delete profiles, settings and logs (they are kept in a silent uninstallation).
