---
description: The toolbar, connection list, traffic graph and statistics of the main window.
---

# Main Window

The upper part of the main window contains several tabs; the lower part shows the [log](./log).

## Toolbar

| Button | Function |
| --- | --- |
| Upstream Proxies | Manage proxy servers and proxy chains. See [Upstream Proxies](./proxies) |
| Proxification Rules | Edit rules. See [Proxification Rules](./rules) |
| Name Resolution | Choose whether host names are resolved locally or by the proxy. See [Name Resolution](./dns) |
| Proxy Gateway | Let other devices reach the network through this computer. See [Proxy Gateway](./gateway) |
| Disconnect | Close the selected connection |
| End Process | End the process that owns the selected connection |
| Proxy Checker | Test whether a proxy server works. See [Proxy Checker](./checker) |
| Packet Capture | Open the capture tab. See [Packet Capture](./capture) |

## Connections

Lists all connections currently passing through WinRoute. Click a column header to sort.

![Connection list](/images/en/connections.png){width=1185}

In the screenshot, curl.exe matches the "Command-line tools" rule and connects through the local proxy client (127.0.0.1:10808). The connections of the proxy client itself, python.exe, are sent directly by [connection loop detection](./advanced#connection-loop-detection).

| Column | Content |
| --- | --- |
| Application | Program name and process ID |
| Target | Target host and port. When an application connects by IP address, the corresponding host name is shown where possible |
| Protocol | TCP or UDP, and IPv4 or IPv6 |
| Time/Status | Duration of the connection, or a status such as connecting or failed |
| Rule : Proxy | The matching rule and the address of the proxy, or the chain, used |
| Sent / Received | Amount of data |

While the proxy gateway is running, connections made by other devices through it are also listed; the Application column shows `[Gateway] device IP`.

Right-click a connection for the following commands:

- **Disconnect**: close the connection immediately.
- **End Process**: end the process that opened the connection, after confirmation. WinRoute first closes the process's windows so it exits normally, as when the user closes it; a process that does not exit shortly, or has no window, is terminated.
- **End Process Tree** (context menu only): end the process that opened the connection together with every process on its launch chain. Because several processes are involved, the dialog lists them with a check box each, all selected by default, so some can be left out before ending. Useful for programs that spawn several child processes.
- **Create Rule for this Application...**: create a new rule with this application as its condition.
- **Copy**: copy the connection details.

::: info Why direct connections are not listed
By default, connections that match a **Direct** rule do not pass through WinRoute and are therefore not listed. To list them, enable [Handle Direct Connections](./advanced#handle-direct-connections).
:::

## Traffic

![Traffic graph](/images/en/traffic.png){width=1185}

Shows real-time traffic curves:

- one curve each for the download and upload rate of this computer;
- while the proxy gateway is running, additional curves for the gateway's download and upload rate.

The legend above the graph is arranged in groups, with download above upload in each group, and shows the current rate and the highest rate since the graph was cleared. Click a legend entry to show or hide its curve. **Clear** empties the graph and restarts the totals.

## Statistics

Summarizes, per application and target, the number of connections, active connections, data sent and received, and the time of last activity, so that applications with heavy traffic can be identified, and shows in the **Status** column whether each application is currently **Running** or **Ended**. Right-click to **Copy** or **Reset Statistics**; for a running application, also to **End Process** or **End Process Tree**.

## Capture

This tab is shown after packet capture is enabled in **Tools > Packet Capture**. See [Packet Capture](./capture).

## Status bar

Shows the number of active connections, the current rates, the interception backend, the driver status and the current profile.
