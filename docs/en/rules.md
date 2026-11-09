---
description: Rule order, conditions, actions and the network interface of a rule.
---

# Proxification Rules

Menu: **Profile > Proxification Rules...**, or **Proxification Rules** on the toolbar.

![Proxification Rules window](/images/en/rules.png){width=863}

## Rule order

Each new connection is compared with the rules from top to bottom and handled by the **first matching** rule; the remaining rules are not evaluated.

- The **Default** rule at the end of the list handles every connection that matches no other rule. It cannot be removed; only its action can be changed.
- Select a rule and use the up and down arrows on the right to change the order. Place more specific rules higher.
- Click **Clone** to copy the selected rule.
- Clear the check box in front of a rule to disable it temporarily.
- The action of a rule can be changed directly in the **Action** column of the list.

Double-click a rule or click **Edit...** to open the rule window:

![Proxification Rule window](/images/en/rule-edit.png){width=563}

## Conditions

A rule matches only when all three conditions are met. An empty condition means "any". Separate multiple values with semicolons.

### Applications

Program names, full paths, folders or wildcards:

```
chrome.exe; "C:\some app.exe"; C:\Games\; fire*.exe
```

- A file name alone matches a program of that name in any location.
- A folder (ending with `\`) matches every program in that folder.
- Enclose paths that contain spaces in double quotation marks.
- Programs can be selected with **Add Files...** or **Add Folder...** in the **Browse...** drop-down menu.

### Target hosts

IP addresses, host names, wildcards or IP address ranges:

```
127.0.0.1; *.example.com; 192.168.1.*; 10.1.0.0-10.5.255.255
```

When an application connects by IP address, WinRoute restores the host name from earlier DNS answers, so rules based on host names still apply.

### Target ports

Single ports or port ranges:

```
80; 443; 8000-9000
```

## Actions

| Action | Description |
| --- | --- |
| Direct | Connect without a proxy |
| Block | Refuse the connection |
| Proxy | Connect through the selected proxy server |
| Chain | Connect through the selected proxy chain |

## Network interface

Click **Advanced...** in the rule window to send connections matching the rule out through a specific network adapter:

![Rule Network Interface](/images/en/rule-interface.png){width=443}

- For a direct connection, the adapter connects to the target host;
- for a proxied connection, the adapter connects to the first proxy server.

This is useful on computers with several network connections, for example a wired network and a mobile network, or a corporate VPN.

**When the interface is down (disconnected)**, you can choose:

- **Use the system default interface**: connect as usual, with the system choosing the adapter.
- **Ignore this rule (the next matching rule applies)**: for rules that apply only on a particular network.

::: warning
If the selected adapter has no route to the target, the connection fails rather than using another adapter. Name resolution still uses the system default route.
:::

The interface is stored by GUID, in the same format as the rule network interface of Proxifier 4, so profiles can be read by both. UDP forwarding and the proxy gateway also follow the interface set by a rule.

## Troubleshooting rules

The log records the rule matched by each connection and the adapter actually used:

```
chrome.exe - www.example.com:443 matching Browsers rule : open through proxy 203.0.113.10:1080 SOCKS5, adapter Ethernet
```

If a rule does not take effect, check in the log which rule the connection matched. A common cause is a broader rule placed higher in the list.

You can also right-click a connection in the [connection list](./main-window#connections) and select **Create Rule for this Application...** to create a rule for that program.
