---
description: Running this computer as an HTTP and SOCKS5 proxy server for other devices on the network.
---

# Proxy Gateway

Toolbar: **Proxy Gateway**.

The proxy gateway runs WinRoute as an HTTP and SOCKS5 proxy server. Other devices on the local network (phones, TVs, game consoles, virtual machines and other computers) that use this computer as their proxy server reach the network through it and use the proxies and chains configured here.

## Settings

![Proxy Gateway window](/images/en/gateway.png){width=622}

### Enabling

Select **Run a proxy server so other machines can reach the network through this one**. This is the master switch of the feature.

### Listen

| Item | Default | Description |
| --- | --- | --- |
| Address | `0.0.0.0` | The address to listen on; by default, all network adapters. To serve a particular network only, enter the address of that adapter |
| HTTP port | 8080 | HTTP proxy, supporting CONNECT and plain HTTP requests. Can be turned off separately |
| SOCKS5 port | 1080 | SOCKS5 proxy. Can be turned off separately |

### Outgoing connections

| Mode | Description |
| --- | --- |
| Follow this machine's proxy rules | Handled by the [proxification rules](./rules) of this computer. Remote connections have no local process, so rules that match by application are skipped |
| Always connect directly | Connect to the network directly, without any proxy |
| Send every connection through one proxy or chain | Every connection goes through the selected proxy or chain |

With **Send every connection through one proxy or chain**, **If the selected proxy cannot connect** offers:

- **Block**: refuse the connection, so that no traffic bypasses the proxy;
- **Connect directly**: make the connection directly instead.

### Authentication

When a user name and password are set, clients must authenticate: with user name/password authentication for SOCKS5 and Basic authentication for HTTP.

If the user name is left empty, clients can use the gateway without authentication.

::: warning
Without authentication, any device on the same network can use the gateway. Use this only on trusted networks.
:::

## Using the gateway from other devices

For example, if the local IP address of this computer is `192.168.1.10`:

- **HTTP proxy**: server `192.168.1.10`, port `8080`. The proxy setting in a phone's Wi-Fi settings is usually of this type.
- **SOCKS5 proxy**: server `192.168.1.10`, port `1080`.

::: tip If devices cannot connect
- Make sure that Windows Firewall allows other devices to reach these ports.
- Make sure that the device is on the same network as this computer and that the router does not isolate clients (AP isolation).
- Check the log for connections from the device or for authentication failures.
:::

## Viewing gateway traffic

- **Connection list**: gateway connections are shown as `[Gateway] device IP`.
- **Traffic graph**: while the gateway is running, its download and upload curves are shown.
- **Log**: records the rule matched by each connection and the adapter used.
- **Packet capture**: select **Capture: Proxy gateway** to record gateway connections, which can also be [decrypted](./https#decrypting-proxy-gateway-devices).
