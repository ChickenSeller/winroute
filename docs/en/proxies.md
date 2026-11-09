---
description: Adding and managing proxy servers, including protocol, authentication and advanced settings.
---

# Upstream Proxies

Menu: **Profile > Upstream Proxies...**, or **Upstream Proxies** on the toolbar.

This window manages the proxy servers WinRoute can use and the [proxy chains](./chains) built from them.

![Upstream Proxies window](/images/en/proxies.png){width=563}

## Adding a proxy server

Click **Add...** (or double-click an existing proxy to edit it), then set the following.

![Proxy Server window](/images/en/proxy-edit.png){width=468}

### Address and port

The IP address or host name of the proxy server. IPv4 and IPv6 are supported.

### Protocol

| Protocol | Description |
| --- | --- |
| SOCKS Version 5 | The most common protocol. Supports user name and password authentication, and is the only protocol that can forward UDP |
| SOCKS Version 4 | With "Use SOCKS 4A extension" selected, host names are resolved by the proxy server |
| HTTPS | An HTTP proxy that supports CONNECT tunnels. Can forward any TCP connection; supports Basic authentication |
| HTTP | Can be used for HTTP connections only. Must first be enabled in **Profile > Advanced > HTTP Proxy Servers...** |

::: warning HTTP and HTTPS
"HTTPS proxy" here means a proxy server that supports the CONNECT method. Most servers described as "HTTP proxies" support CONNECT and should be added as HTTPS. Select HTTP only if the proxy server is known not to support CONNECT.
:::

### Authentication

Select **Enable** and enter the user name and password. NTLM and Kerberos authentication are not supported.

For how passwords are stored in the profile, see [Advanced Options › Passwords Encryption](./advanced#passwords-encryption).

### Check

Before saving, you can click **Check** to test the proxy server with the [Proxy Checker](./checker).

## Advanced settings

Click **Advanced...** in the Proxy Server window:

![Advanced Proxy Server Settings](/images/en/proxy-advanced.png){width=360}

- **Custom Label**: shown instead of the default "address:port" in the proxy list, rules and Proxy Checker, to tell several proxies apart.
- **Use target hostname in proxy request if available**: send the host name rather than the IP address to the proxy server. Use this for proxy servers that do not accept IP addresses; it also lets the proxy server resolve an address close to itself.

## Using a proxy by default

When a new proxy is saved, WinRoute asks whether to use it by default. Selecting **Yes** sets the action of the Default rule to this proxy, so every connection that matches no other rule goes through it.

## Removing a proxy

When you remove a proxy that is used by rules, WinRoute lists the affected rules and lets you either:

- remove those rules as well; or
- change their action to another proxy, a chain, Direct or Block.

The Default rule cannot be removed; only its action can be changed.
