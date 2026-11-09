---
description: Log contents, log levels and the log file.
---

# Log

The log at the bottom of the main window records the application, target, matching rule, proxy and network adapter of each connection, along with errors and notices.

## Log entries

```
chrome.exe - www.example.com:443 matching Browsers rule : open through proxy 203.0.113.10:1080 SOCKS5, adapter Ethernet
git.exe - github.com:443 matching Default rule : open directly, adapter Wi-Fi
chrome.exe - www.example.com:443 close, 2372 bytes (2.32 KB) sent, 5595 bytes (5.46 KB) received, lifetime 00:10
[Gateway] 192.168.1.20 - www.example.com:443 matching Default rule : via 203.0.113.10:1080, adapter Ethernet
```

- **matching … rule**: the name of the matching rule.
- **open directly / open through proxy … / open through *chain***: how the connection is made.
- **adapter**: the network adapter the connection actually uses.
- **close**: when a connection ends, the bytes sent and received and its duration are recorded.
- Entries beginning with `[Gateway]` are connections made by other devices through the [proxy gateway](./gateway).

## Log menu

Right-click the log area, or use the **Log** menu:

| Command | Function |
| --- | --- |
| Copy, Select All, Clear | Work with the log text |
| Log Level | Debug, Verbose, Normal or Errors Only. More detailed levels record more |
| Log to File | Also write the log to a file, with its own level, or turn this off |
| Font Size | Change the font size of the log |

To record DNS lookups in the log, enable **Profile > Advanced > [Log DNS Activity](./advanced#log-dns-activity)**.
