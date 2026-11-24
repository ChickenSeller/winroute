---
description: Interface, system tray, language, updates and anonymous usage statistics.
---

# Interface, Tray and Updates

## View menu

| Command | Function |
| --- | --- |
| Toolbar, Status Bar | Show or hide |
| Refresh Rate | How often the connection list and traffic graph are refreshed. A lower rate uses fewer resources |
| Icon in the System Tray | See below |
| Language | English, 简体中文 or 日本語. Follows the system setting by default; WinRoute must be restarted after a change |

## System tray

With **Icon in the System Tray** enabled:

- closing the main window only hides it in the system tray, and WinRoute keeps running;
- the tooltip of the tray icon shows the current profile, the number of active connections and the rates;
- the tray menu opens the main window or exits WinRoute.

To exit WinRoute, use **File > Exit** or the exit command in the tray menu.

## Updates

The **Help** menu provides these update commands:

| Command | Function |
| --- | --- |
| Check for Updates... | Check for a new version now. After confirmation, WinRoute downloads, verifies and installs it, then restarts |
| Check for Updates via Proxy... | If the update server cannot be reached directly, check and download through a proxy of the profile |
| Automatic Updates | Install All Updates Automatically, Install Required Updates Automatically, Ask Before Updating, or Never Check |
| Automatic Updates > Check Through a Proxy | Check for and download automatic updates through a proxy instead of connecting directly, for networks where the update server cannot be reached directly. The proxy used is the profile's default DNS proxy, or the first proxy if none is set |

Downloaded installers are verified with SHA-256; a file that fails verification is not installed.

## Anonymous usage statistics

**Help > Send Anonymous Usage Statistics**: helps improve WinRoute. No personal data is included, and it can be turned off at any time.

## About

**Help > About WinRoute...** shows version information.
