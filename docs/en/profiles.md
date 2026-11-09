---
description: Creating, switching, importing and exporting profiles, and compatibility with Proxifier.
---

# Profiles

Proxies, chains, rules, name resolution and advanced options are saved in a profile. Profiles use the same `.ppx` format as Proxifier and are stored in `%APPDATA%\WinRoute\Profiles`.

## File menu

| Command | Function |
| --- | --- |
| New Profile | Create an empty profile |
| Load Profile | Switch to another profile; takes effect immediately |
| Save Profile As... | Save the current profile under a new name |
| Manage Profiles... | Open the profile folder in File Explorer, where profiles can be renamed or deleted |
| Import Profile... | Import a `.ppx` file, including profiles exported from Proxifier |
| Export Profile... | Export to a `.ppx` file for another computer or for Proxifier |
| Autostart | Start WinRoute automatically when you sign in to Windows |
| Close WinRoute Window | Hide the main window; WinRoute keeps running in the system tray |
| Exit | Exit WinRoute and stop interception |

You can keep a separate profile for each network environment, such as home and office, and switch between them at any time.

## Compatibility with Proxifier

- On first start, WinRoute imports the default Proxifier 4 profile automatically.
- Settings such as the rule network interface of Proxifier 4 can be read by both applications.
- Passwords encrypted by Proxifier in "basic" mode cannot be decrypted and must be entered again after import.

## Autostart

Implemented as a scheduled task that runs with the highest privileges, so no User Account Control prompt appears at sign-in. WinRoute starts minimized to the system tray.
