---
description: The settings in the Profile > Advanced menu.
---

# Advanced Options

These settings are in the **Profile > Advanced** menu and are saved with the profile.

## HTTP Proxy Servers

When enabled, proxy servers with the HTTP protocol can be added in [Upstream Proxies](./proxies). HTTP proxies can be used for HTTP connections only (for example, browsers); do not confuse them with HTTPS proxies.

## Services and Other Users

By default, only applications run by the current user are handled. You can additionally select:

- **Windows services and other system processes**
- **Applications run by other users on this computer**

::: info
These settings affect TCP and UDP only. ICMP, as used by ping and tracert, cannot be forwarded through a proxy.
:::

## Handle Direct Connections

- **Off** (default): connections that match a **Direct** rule do not pass through WinRoute. They are not listed or counted, which keeps overhead to a minimum.
- **On**: direct connections also pass through WinRoute. They are listed, included in traffic and statistics, and can be recorded by [packet capture](./capture).

Direct connections of rules that [set a network interface](./rules#network-interface) always pass through WinRoute.

## Connection Loop Detection

Prevents connections from looping. It is recommended to keep this enabled:

- If the target of a connection is the proxy server itself, the connection is made directly.
- If a proxy server runs on this computer (for example, a proxy client listening on `127.0.0.1:1080`), the connections of the process listening on that port are made directly instead of being sent back to it.

When a loop is detected, the log notes it once per process.

## Route UDP through SOCKS5

By default, UDP traffic is passed through unchanged. When enabled, UDP traffic (such as games and voice calls) that matches a rule whose action is a **single SOCKS5 proxy** is forwarded with SOCKS5 UDP ASSOCIATE.

- Proxy chains and SOCKS4 or HTTPS proxies do not support UDP; such traffic is sent directly, and the log notes this once.
- If the proxy server does not support UDP, the traffic is also sent directly. Use the [Proxy Checker](./checker) to find out whether a proxy server supports UDP.

This is the global default; a rule can override it, see [Proxification Rules > UDP](./rules#udp).

## Log DNS Activity

Records DNS lookups in the log, which helps when troubleshooting host-name-based rules.

## Passwords Encryption

Sets how proxy passwords are stored in the profile:

| Mode | Description |
| --- | --- |
| Disabled | Stored in plain text; anyone who can read the profile file can read the passwords |
| Basic | Encrypted with a fixed WinRoute key; anyone who opens the profile with WinRoute can use the passwords |
| Current user account | Encrypted with the credentials of the current Windows user; only that user can use the passwords |

A profile master password is not supported.
