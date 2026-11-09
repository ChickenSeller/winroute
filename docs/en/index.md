---
description: What WinRoute does, its main features and system requirements.
---

# Introduction

WinRoute is a network proxy tool for Windows. It takes over the network connections of applications at the system level and, according to rules you define, sends each connection directly, through a proxy server or proxy chain, or blocks it. Applications need no proxy support and no configuration of their own.

WinRoute works the same way as Proxifier and uses the same `.ppx` profile format, so Proxifier profiles can be imported directly.

## Main features

| Feature | Description | See |
| --- | --- | --- |
| Rule-based routing | Decide how a connection is handled by application, target host, IP range and port | [Proxification Rules](./rules) |
| Proxy protocols | SOCKS4/4A, SOCKS5 (with UDP), HTTPS (CONNECT) and HTTP | [Upstream Proxies](./proxies) |
| Proxy chains | Simple chains, failover (redundancy) and load balancing | [Proxy Chains](./chains) |
| Name resolution through the proxy | Host names are resolved by the proxy server, so the local network does not see them | [Name Resolution](./dns) |
| Network interface per rule | Send specific connections out through a chosen network adapter | [Proxification Rules](./rules#network-interface) |
| Monitoring | Connection list, traffic graph, per-application statistics and log | [Main Window](./main-window) |
| Packet capture | View HTTP, DNS and UDP data per connection; export to PCAPNG or HAR | [Packet Capture](./capture) |
| HTTPS decryption | Decrypt HTTPS from this computer and from proxy gateway devices with your own root certificate | [HTTPS Decryption](./https) |
| Proxy gateway | Run this computer as an HTTP/SOCKS5 proxy server for other devices on the network | [Proxy Gateway](./gateway) |

## System requirements

- Windows 10 version 1809 or later, or Windows 11; x64 only.
- Administrator rights. WinRoute loads a network driver to intercept connections.
- Interface languages: English, Simplified Chinese and Japanese.

To get started, see [Getting Started](./getting-started).
