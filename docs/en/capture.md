---
description: Recording and viewing the data of connections through WinRoute, and exporting it for Wireshark.
---

# Packet Capture

Menu: **Tools > Packet Capture**. When enabled, the main window shows a **Capture** tab.

Packet capture records the application-layer data sent and received by connections passing through WinRoute, for analyzing the network behavior of applications. The recorded data is kept in memory on this computer only and is never uploaded.

![Capture tab](/images/en/capture.png){width=1185}

When capture is started for the first time, WinRoute points out that captured data stays on this computer, and that with HTTPS decryption on it may contain passwords and other sensitive information, so care is needed when exporting or sharing it.

## Toolbar

| Control | Function |
| --- | --- |
| Start / Pause | Start or pause recording |
| Clear | Discard the recorded data |
| Export... | Export to PCAPNG or HAR; see below |
| Filter... | Set which connections are recorded; see below |
| Capture: This computer | Record connections of applications on this computer |
| Capture: Proxy gateway | Record connections made by other devices through the [proxy gateway](./gateway). Unavailable while the gateway is off |
| Decrypt HTTPS | Enable HTTPS decryption separately for this computer and the proxy gateway. See [HTTPS Decryption](./https) |
| Root Certificate... | Manage the root certificate used for decryption |

## Filter

Click **Filter...** to choose what is recorded:

![Capture Filter](/images/en/capture-filter.png){width=563}

- **Everything**
- **Only connections matching a rule (whitelist)**
- **All except connections matching a rule (blacklist)**

In whitelist or blacklist mode, click **Add...** to add a filter rule. Rules are set by application, target host and port, with the same syntax as [proxification rules](./rules#conditions). A rule matches when all filled-in fields match; empty fields are not restricted. For example, you can record a single application, or exclude applications that use certificate pinning.

![Capture Rule](/images/en/capture-rule.png){width=425}

## What is recorded

Each row of the list is one connection; select it to show the details on the right.

### HTTP

HTTP/1.x is parsed into requests and responses, showing the method, URL, status code, headers and body. With decryption enabled, HTTPS connections are shown the same way; rows with a lock icon in the Protocol column are decrypted connections.

### Other TCP

Shown in segments by direction, as hexadecimal and text side by side.

### DNS

DNS queries and answers seen by either interception backend are grouped into one row per process, local port and DNS server. The target is shown as "query name @ server", and the details decode the question, response code and records. Filter rules match the queried host name.

With [Resolve hostnames through proxy](./dns#resolve-hostnames-through-proxy) enabled, applications receive placeholder answers from WinRoute, and the status column shows "Answered locally". The real resolution takes place on the proxy side and cannot be recorded on this computer.

### UDP

UDP forwarded through SOCKS5 is grouped into one row per process and target, recording each datagram sent and received.

### Not recorded

- Direct TCP connections, unless [Handle Direct Connections](./advanced#handle-direct-connections) is enabled;
- UDP that is not forwarded through a proxy;
- WinRoute's own connections and DNS lookups.

## Export

Clicking **Export...** first shows an options dialog:

![Export Capture](/images/en/capture-export.png){width=563}

- **Scope**: all connections are exported by default. To export only the selected connections, select the corresponding option; selecting rows in the list does not narrow the export by itself.
- **Format** (chosen in the save dialog):
  - **PCAPNG**: for Wireshark. TCP connections are written with synthesized handshakes and segments, which Wireshark can reassemble into streams; UDP and DNS are written as real UDP packets.
  - **HAR**: contains HTTP exchanges only; can be imported into browser developer tools and similar tools.

### Decrypted HTTPS in Wireshark

| Option | Effect |
| --- | --- |
| Keep real ports (443) (default) | Use "Decode As → HTTP" for the port in Wireshark; otherwise the data is parsed as TLS and not shown correctly |
| Use port 80 so Wireshark shows HTTP automatically | Wireshark parses the data as HTTP automatically |
| Keep real ports (443) and show HTTP automatically |  Written as Exported PDU with port 443 kept, without the need for Decode As; there is no TCP stream view, and each packet is limited to 256 KB |
