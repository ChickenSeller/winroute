---
description: Testing a proxy server or proxy chain step by step.
---

# Proxy Checker

Menu: **Tools > Proxy Checker...**, or **Proxy Checker** on the toolbar.

Select the proxy server or chain to test, enter the **Target host** and **Port**, and click **Start Testing**.

![Proxy Checker](/images/en/checker.png){width=623}

## Tests

WinRoute runs the following tests in order and shows the result and duration of each:

1. **Connection Test**: whether the proxy server can be reached.
2. **Connection through the Proxy Server Test**: whether the target host can be reached through the proxy server.
3. **HTTP Request Test**: sends an HTTP request through the proxy server and shows the response status. For port 443, a **TLS Handshake Test** runs first.
4. **UDP Support Test**: for SOCKS5 proxies only. Requests a UDP relay from the proxy server and sends a DNS query to `8.8.8.8` through it; a reply means the proxy can forward UDP.
5. **UDP Reachability and NAT Test (STUN)**: for SOCKS5 proxies only. Sends requests through the relay to several public STUN servers at once, proving the proxy can carry UDP to an **arbitrary host** (not just DNS); then compares the public endpoint two servers report to classify the exit's NAT mapping: **endpoint-independent (full-cone)** suits P2P games, while **symmetric** means P2P/relay traffic (e.g. Steam) will likely fail even when plain UDP works. Servers are chosen from the checkable presets (name on the box, address in the tooltip) or added in the **Custom** field (`host:port`, separated by `;` or spaces); the whole test can be switched off with the checkbox in its title, and tolerates individual servers being unreachable — one reply counts as reachable.

At the end, the checker reports that the proxy server is working correctly, or gives the reason at the step that failed, for example:

- authentication failed
- proxy requires authentication
- no response within 10 seconds
- not a SOCKS5 proxy
- no reply from the relay within 5 seconds (UDP may be blocked)

A failed UDP test does not affect the overall result; it only means that the proxy server cannot forward UDP.

::: tip
The **UDP Support Test** only proves the relay can complete a single DNS round-trip. Some proxies forward UDP on port 53 only (common with port-based xray routing), so they pass here yet games still fail to connect. The **UDP Reachability and NAT Test** is there to tell the two cases apart.
:::

::: tip
The **Check** button in the window for adding or editing a proxy server also opens this tool.
:::
