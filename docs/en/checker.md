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

At the end, the checker reports that the proxy server is working correctly, or gives the reason at the step that failed, for example:

- authentication failed
- proxy requires authentication
- no response within 10 seconds
- not a SOCKS5 proxy
- no reply from the relay within 5 seconds (UDP may be blocked)

A failed UDP test does not affect the overall result; it only means that the proxy server cannot forward UDP.

::: tip
The **Check** button in the window for adding or editing a proxy server also opens this tool.
:::
