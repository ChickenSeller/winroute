---
description: Combining proxy servers into simple chains, redundancy chains or load balancing.
---

# Proxy Chains

Proxy chains are managed in the lower part of the **Upstream Proxies** window.

## Creating a proxy chain

1. Click **Create** and enter a name for the chain.
2. **Drag** proxies from the list above into the chain. The order in the chain is the order of use; drag within the chain to change it.
3. Clear the check box in front of a proxy in the chain to disable it temporarily.
4. Select the chain and click **Type...** to set how it works.

![Proxy chains in the lower part of the Upstream Proxies window](/images/en/proxies.png){width=563}

Once created, a chain can be used as a rule action in the same way as a single proxy. See [Proxification Rules](./rules).

## Chain types

![Chain Type](/images/en/chain-type.png){width=618}

### Simple Chain

The connection passes through every proxy in the chain in turn: WinRoute connects to the first proxy, through it to the second, and so on; the last proxy connects to the target host. Use this when multiple hops are required.

### Redundancy

The first proxy is used; if it fails, the next one is used, and so on. Click **Options...** to set the following:

![Redundancy Options](/images/en/redundancy.png){width=348}

| Option | Description |
| --- | --- |
| Connection to proxy timeout (sec.) | How long to wait for a proxy before treating it as failed |
| Recheck failed proxy each (sec.) | A failed proxy is skipped for this period and then tried again |
| Connect directly if all proxies fail | If not selected, the connection fails; if selected, it is made directly |
| Reset Fail Status for All Proxies | Return all failed proxies to use immediately |

### Load Balancing

Each connection uses a proxy chosen at random.

With **Use the same proxy for the same process PID** selected, all connections of a process use the same proxy. Some websites require signing in again when the outgoing IP address changes; this option avoids that.

::: info
Proxy chains forward TCP only. UDP can be forwarded through a single SOCKS5 proxy only; UDP traffic that matches a chain is sent directly, and the log notes this once.
:::
