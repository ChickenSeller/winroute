---
description: Frequently asked questions and troubleshooting.
---

# FAQ

## An application does not connect through the proxy

1. Check in the log which rule the connection matched. Rules are matched from top to bottom, and a broader rule placed higher may match first.
2. If the application is a Windows service or runs as another user, enable the corresponding option in [Services and Other Users](./advanced#services-and-other-users).
3. UDP is passed through by default. Enable [Route UDP through SOCKS5](./advanced#route-udp-through-socks5); the rule action must be a single SOCKS5 proxy. QUIC (HTTP/3) in browsers is UDP traffic.
4. ping and tracert use ICMP, which cannot be forwarded through a proxy.

## All connections stop working after some time

With the demo driver, interception stops when the total number of intercepted connections reaches its limit, and Windows must be restarted. Switch to driver A or B, or to the WinDivert backend, in **Tools > Network Driver**. See [Installation and Network Driver](./install#changing-the-interception-backend-and-driver).

## A local proxy client (such as v2ray or Clash) has its own connections forwarded, causing a loop

Make sure that [Connection Loop Detection](./advanced#connection-loop-detection) is enabled. WinRoute recognizes the process listening on the proxy port and sends its connections directly. You can also create a **Direct** rule for the program and place it at the top of the list.

## The proxy server accepts host names only, not IP addresses

Select [Use target hostname in proxy request if available](./proxies#advanced-settings) in the proxy's **Advanced...** settings, or enable [Resolve hostnames through proxy](./dns#resolve-hostnames-through-proxy). You can also choose **Through a proxy** in that rule's [Host name resolution](./rules#host-name-resolution) only.

## Browsers report a certificate error after HTTPS decryption is enabled

The device does not trust the WinRoute root certificate. On this computer, click **Install** in **Root Certificate...**; for Firefox and other devices, export the certificate and import it manually. If one application keeps reporting errors, it may use certificate pinning; exclude it with the capture filter. See [HTTPS Decryption](./https).

## Other devices cannot connect to the proxy gateway

Make sure that Windows Firewall allows the gateway ports, that the device is on the same network as this computer, and that the router does not isolate clients. See [Proxy Gateway](./gateway#using-the-gateway-from-other-devices).

## Only some rules should resolve host names through a proxy

Leave the profile's **Resolve hostnames through proxy** off, and set **Resolve host names** to **Through a proxy** under **Advanced...** of the rules that need it. A specific proxy or chain can also be chosen to resolve the names; see [Host name resolution](./rules#host-name-resolution).

## A VPN (such as ZeroTier or WireGuard) disconnects when WinRoute starts

Since 0.9.1, UDP connections that are never forwarded (unhandled services, programs with only Direct rules) are handed back to the system after their first packet and no longer pass through WinRoute. If the VPN still disconnects, restart its service; if [Route UDP through SOCKS5](./advanced#route-udp-through-socks5) is on, you can also turn it off for a while to narrow the problem down.

## Can WinRoute be used together with Proxifier?

Profiles can be exchanged in both directions, but both applications intercept network connections and cannot run at the same time.
