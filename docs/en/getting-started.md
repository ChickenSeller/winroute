---
description: Install WinRoute, add a proxy server and create a rule.
---

# Getting Started

## 1. Install

Download the latest installer from the [releases page](https://pages.yanlei.org/kaguya/winroute/releases.html) and run it. The default driver choice is suitable for most users; see [Installation and Network Driver](./install) for the differences between drivers.

Start WinRoute when the installation is complete. It requires administrator rights, so Windows shows a User Account Control prompt at startup.

::: tip Migrating from Proxifier
On first start, WinRoute imports `%APPDATA%\Proxifier4\Profiles\Default.ppx` automatically, so existing proxies and rules can be used as they are. Proxifier and WinRoute both intercept network connections and must not run at the same time.
:::

## 2. Add a proxy server

1. Click **Upstream Proxies** on the toolbar, then click **Add...**.
2. Enter the **Address** and **Port** of the proxy server and select the **Protocol** (most proxy servers use SOCKS5). If the proxy server requires authentication, select **Enable** under **Authentication** and enter the user name and password.
3. Optionally click **Check** to verify that the proxy server works.
4. Click **OK**. WinRoute asks whether to use the proxy by default:
   - **Yes**: every connection that matches no other rule goes through this proxy.
   - **No**: only connections whose rule specifies this proxy go through it.

## 3. Create a rule (optional)

To route only some applications through the proxy:

1. Click **Proxification Rules** on the toolbar, then click **Add...**.
2. Enter a rule name in **Name**, for example "Browsers".
3. Enter `chrome.exe; msedge.exe` in **Applications**.
4. Select the proxy added in the previous step in **Action**, then click **OK**.
5. Set the action of the **Default** rule at the end of the list to **Direct**, so that other applications connect directly.

For details on rules, see [Proxification Rules](./rules).

## 4. Check the result

- The **Connections** tab of the main window lists each connection with its matching rule and proxy in real time.
- The log at the bottom of the window records how each connection is handled, for example:

```
chrome.exe - www.example.com:443 matching Browsers rule : open through proxy 203.0.113.10:1080 SOCKS5, adapter Ethernet
```

If a rule does not take effect as expected, first check in the log which rule the connection matched.

## Next steps

- Keep the local network from seeing the host names you visit: [Name Resolution](./dns)
- Send UDP traffic such as games and voice calls through a proxy: [Advanced Options](./advanced#route-udp-through-socks5)
- Let phones, TVs and other devices use the proxy through this computer: [Proxy Gateway](./gateway)
- See the data applications send and receive: [Packet Capture](./capture)
