---
description: Decrypting HTTPS from this computer and from proxy gateway devices with your own root certificate.
---

# HTTPS Decryption

When enabled, HTTPS connections in the capture are shown in plain text.

## How it works

WinRoute decrypts captured connections to port 443 as a man in the middle:

1. It reads the host name (SNI) from the application's TLS handshake, issues a temporary certificate for that name with the WinRoute root certificate, and establishes a TLS connection with the application.
2. At the same time, it connects to the real server as a client and verifies the server's certificate.
3. It forwards data between the two TLS connections, so the capture records plain text.

If the real server's certificate fails verification, WinRoute closes the connection instead of presenting an untrusted server to the application.

## Decrypting applications on this computer

1. On the Capture tab, select **Decrypt HTTPS: This computer**, and make sure that **Capture: This computer** is selected and capture has started.
2. If no root certificate exists yet, WinRoute asks for its **common name**, **organization** and **validity period**, then creates the certificate and installs it in the Windows Trusted Root Certification Authorities store.
3. Applications that use the system certificate store, such as Chrome and Edge, trust the certificate.

::: tip Firefox
Firefox uses its own certificate store. Export the certificate in **Root Certificate...**, then import it in Firefox under Settings > Privacy & Security > Certificates > View Certificates > Authorities.
:::

## Decrypting proxy gateway devices

1. On the Capture tab, select **Decrypt HTTPS: Proxy gateway**, and make sure that **Capture: Proxy gateway** is selected.
2. If no root certificate exists yet, WinRoute creates one. This certificate is for other devices and is not installed on this computer.
3. **Export** the certificate (`.crt`) in **Root Certificate...** and install it as a trusted root certificate on the phones, computers and other devices.
4. Only connections to port 443 made through SOCKS5 or HTTP CONNECT are decrypted.

Decryption for this computer and for the proxy gateway are set independently; either can be enabled alone.

## Managing the root certificate

![Root certificate window](/images/en/root-cert.png){width=570}

The **Root Certificate...** window shows the status of the certificate (not created; created but not trusted by this computer; created and trusted by this computer), its common name, organization, validity period and SHA-256 fingerprint, and provides these commands:

| Button | Function |
| --- | --- |
| Generate... / Regenerate... | Create the root certificate. After regenerating, every device must install the new certificate |
| Install | Install it in the Trusted Root Certification Authorities store of this computer |
| Remove | Remove it from that store |
| Export... | Save it as a `.crt` file for other devices or Firefox |

::: danger Keep the private key safe
The private key of the root certificate is stored in `%APPDATA%\WinRoute\ca`. Anyone who has it can impersonate any website to devices that trust the certificate. Do not disclose the key, and remove the certificate from this computer and other devices when decryption is no longer needed.
:::

## Limitations

- Only TCP port 443 is decrypted.
- HTTP/2 is negotiated down to HTTP/1.1; QUIC (HTTP/3, over UDP) is not decrypted.
- Applications that use certificate pinning refuse the connection. Exclude them with the [capture filter](./capture#filter).
