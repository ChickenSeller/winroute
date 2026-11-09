---
description: Choosing whether host names are resolved locally or by the proxy server.
---

# Name Resolution

Menu: **Profile > Name Resolution...**, or **Name Resolution** on the toolbar.

![Name Resolution window](/images/en/name-resolution.png){width=623}

## Resolution modes

### Detect DNS settings automatically (default)

Host names are resolved locally while local DNS is available. When local DNS is not available, they are resolved by the proxy server instead. The status bar and the log show which mode is in use.

### Resolve hostnames through proxy

Clear **Detect DNS settings automatically** first to make this option available.

When an application looks up a host name, WinRoute answers with a placeholder IP address in the `198.18.0.0/15` range. When the application connects to that address, WinRoute sends the real host name to the proxy server, which resolves it. Placeholder addresses are valid only while WinRoute is running.

This mode:

- keeps the local network and the local DNS server from seeing the host names you visit;
- allows access to host names that can be resolved only on the proxy side, for example hosts in the proxy's internal network;
- lets the proxy server resolve the server address closest to itself.

If a connection to a placeholder address matches a **Direct** rule, WinRoute resolves the host name locally and then connects directly.

## Host name list

You can limit which host names are resolved by the proxy:

- **Do NOT resolve the following** (default): the listed names are resolved locally and all others by the proxy. The default list is `%ComputerName%; localhost; *.local`.
- **Resolve ONLY the following**: only the listed names are resolved by the proxy.

The list can contain `%ComputerName%` (the name of this computer) and `%SimpleHostnames%` (host names without a dot).

## Notes

- This feature relies on DNS interception, which both the NetFilter and WinDivert backends support.
- Browsers that use DNS over HTTPS bypass the system DNS, so WinRoute cannot see their lookups. If necessary, turn off "secure DNS" in the browser settings.
- To record DNS lookups in the log, enable [Log DNS Activity](./advanced#log-dns-activity).
