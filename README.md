# WinRoute

用 Qt 实现的 Proxifier 替代品：按规则把 Windows 上任意程序的 TCP 连接转发到 SOCKS4/4A、SOCKS5、HTTPS 代理或代理链。配置文件直接使用 Proxifier 的 `.ppx` 格式，首次启动会自动导入 `%APPDATA%\Proxifier4\Profiles\Default.ppx`。

- 项目主页：<https://pages.yanlei.org/kaguya/winroute/>
- 源码（GitHub）：<https://github.com/ChickenSeller/winroute>

## 架构

```
应用程序 connect()
   │  netfilter2.sys (NetFilter SDK WFP 驱动) ── tcpConnectRequest 回调
   ▼
NetFilterInterceptor ── 规则匹配 (RuleEngine) ── 改写目标地址到本地中继
   │                                         └─ 记录原始目标到 FlowTable
   ▼
Relay (独立线程, 0.0.0.0/:: 随机端口)
   └─ ProxyConnector: 直连 / SOCKS4/4A / SOCKS5 / HTTPS CONNECT / 多级链
```

- **DNS**：驱动过滤 UDP 53。应答被嗅探成 IP→域名映射，所以基于域名的规则和连接列表能显示主机名。开启“通过代理解析主机名”后返回 `198.18.0.0/15` 内的假 IP（Proxifier 用的是 127.8.\*.\*），连接时把域名交给代理解析。
- **自身流量**：驱动规则放行 WinRoute 自己的进程，不会形成环路；连接目标就是代理服务器本身时，“连接环路检测”会改为直连；代理服务器在本机时（如 127.0.0.1:1080 的代理客户端），监听该端口的程序自己的连接也会自动直连，不会再被送回它自己。
- **代理链**：简单链、冗余（失败切换下一个，可设重检间隔和全部失败时直连）、负载均衡（随机，或同一 PID 固定用一个代理）。
- **UDP**：默认直接放行（`mode_bypass`）。在“配置 > 高级 > 通过 SOCKS5 转发 UDP”开启后，命中规则且代理是单个 SOCKS5 的 UDP 会通过 SOCKS5 UDP ASSOCIATE 转发（驱动改为过滤全部出站 UDP，中继线程为每个 UDP 套接字建一个关联，回包用 `nf_udpPostReceive` 注回）。代理链或非 SOCKS5（SOCKS4/HTTPS）不支持 UDP，会直接放行并在日志提示一次；关联失败也回退为直连。
- **指定网卡**：规则的“高级...”里可以选一块网卡（同 Proxifier 4 的规则网卡），匹配的连接从这块网卡发出：直连时连目标，走代理时连第一个代理。做法是把出站套接字的源地址绑定到该网卡的 IP，Windows 按强主机模型只在这块网卡上查路由，所以该网卡没有通往目标的路由时连接会失败，而不是改走别的网卡。绑定了网卡的直连规则也会经过中转。网卡断开时可选“使用系统默认网卡”或“忽略此规则”（交给下一条匹配的规则，适合只在某个网络下生效的规则）。网卡按 GUID 记录，和 Proxifier 4 一样存成 .ppx 里 `<Rule>` 的 `interfaceId`、`skipIfInterfaceDown` 属性，两边的配置可以互相读取。UDP 在开启“通过 SOCKS5 转发 UDP”时同样从该网卡发出；代理网关按规则转发时也遵循规则的网卡。主机名解析（含“通过代理解析”时直连的假 IP）仍走系统默认路由。

### 拦截后端

有两种拦截后端，在“工具 > 网络驱动”里选择（`QSettings` 的 `backend/active`）：

- **NetFilter（默认）**：上图所示，`netfilter2.sys` 在 connect 回调里改写目标，支持 UDP 代理与假 DNS。
- **WinDivert（`WinDivertInterceptor`）**：NETWORK 层全量抓出站流量。TCP 在每条连接的 SYN 上分类（进程归属取自 TCP 表，无竞态），把要代理的连接的数据包做 NAT 重定向到本地中继（目标改成源 IP 的中继端口，**保留应用源端口**，中继据此从 `FlowTable` 还原原始目标）；自身与中继上行流量归属本进程，天然放过。开启"通过 SOCKS5 转发 UDP"后，UDP 也支持（命中单个 SOCKS5 的流量由中继做 UDP ASSOCIATE 隧道，回包由后端自行构造并重注入；进程归属取自 UDP 表）。另开一个 DNS 句柄（UDP 53 两个方向）：嗅探应答填 `DnsCache`，使中继能把**域名**交给代理（对拒绝裸 IP 的代理很关键）；开启"通过代理解析主机名"后，像 NetFilter 一样把出站查询假解析成 `198.18.0.0/15` 的假 IP（连接命中假 IP 时在 SYN 分类处还原成域名，本进程自身的查询不假解析）。首次使用时 WinDivert 自行安装其内核服务（需管理员）。

### 抓包（本机调试）

“工具 > 抓包”可选开一个抓包开关（默认关，状态存 `QSettings` 的 `capture/enabled`）：开启后，经本地中继的每条连接的应用层字节被 `Capture`（`src/core/Capture.*`）记录，“抓包”标签页按连接查看——HTTP/1.x 解析成请求/响应（`src/core/HttpParse.*`），其余按十六进制显示；可导出 **PCAPNG**（合成握手与分段，供 Wireshark 重组）或 **HAR**。抓取内容只在本机，不上报。

**DNS 与 UDP**：两个拦截后端看到的 DNS 查询与应答（UDP 53）也记入抓包（`Capture::dnsMessage`），按“进程 + 本地端口 + DNS 服务器”归为一条，目标显示为“查询名 @ 服务器”，详情里解码成问题、应答码和记录；抓包过滤按查询的域名匹配。开启“通过代理解析主机名”时，程序收到的是 WinRoute 合成的假 IP 应答（状态列显示“本地应答”），真实解析发生在代理端，本机看不到。经 SOCKS5 转发的 UDP（`UdpRelaySession`）同样按“进程 + 目标”归为一条，记录发出和收回的每个数据报；代理无法关联而改为直连时，只能看到发出的一侧。不走代理的普通 UDP 不经过 WinRoute，抓不到。导出 PCAPNG 时 UDP 写成真实的 UDP 包（无握手），Wireshark 直接按 DNS 等协议解析；HAR 只含 HTTP，不含这些条目。WinRoute 自身的 DNS 查询不记录。

**解密 HTTPS（MITM）**：抓包标签页的“解密 HTTPS”复选框（默认关，状态存 `capture/decrypt`）开启后，WinRoute 对经中继且命中抓包过滤的 443 端口连接做中间人——用自建根 CA（`src/core/CertAuthority.*`，基于 OpenSSL，密钥存 `%APPDATA%\WinRoute\ca`，不入 git）按主机名临时签发叶证书，在 client 侧做 TLS 服务端、对真实服务器做 TLS 客户端（校验其证书，校验失败则断开），于是抓包里看到明文。首次开启解密且还没有根证书时，会先让你填写证书主题（通用名称、组织、有效期），按此生成并安装，不会静默套用默认值。“根证书…”按钮显示当前证书的详情（主题、有效期、序列号、SHA-256 指纹），并可重新生成、安装到 Windows 受信任根（供 Chrome/Edge；Firefox 需手动导入导出的证书）、移除和导出根证书；安装、检测和移除都按证书本身匹配，自定义名称也能正确识别。界面需走 Qt 的 OpenSSL 后端（启动时 `QSslSocket::setActiveBackend("openssl")`），安装包随附 `libcrypto`/`libssl` 运行期 DLL。已知限制：仅解密 TCP 443；v1 把 ALPN 降到 http/1.1，不解密 HTTP/2 与 QUIC（UDP）；证书固定的应用需用抓包过滤排除。

**导出**：导出用 Windows 原生保存对话框，之前先弹一个选项框——默认导出全部连接（只导出选中的连接需显式勾选，表格里选了行不会悄悄缩小导出范围），并选择解密后的 HTTPS 在 Wireshark（PCAPNG）里如何呈现：

- **保留真实端口（443）**（默认）：Wireshark 需对该端口“Decode As → HTTP”才会按明文解析（否则按 TLS 解析明文会乱码）。
- **改用 80 端口**：解密的 HTTP 流写到 80 端口，Wireshark 直接按 HTTP 解析。
- **保留真实端口并自动显示 HTTP**：以 Wireshark 的 “Exported PDU”（链路类型 252）写入，端口保持 443，无需 Decode As 即按 HTTP 解析；代价是没有 TCP 流视图，每个 PDU 上限 256 KB（Wireshark 单包大小限制）。

## 目录

| 路径 | 内容 |
| --- | --- |
| `src/core` | 配置读写、规则引擎、驱动接入、DNS、代理协议、中继 |
| `src/ui` | 主窗口（连接 / 流量 / 统计 / 日志）和全部设置对话框 |
| `tests/tests.cpp` | 配置往返、规则匹配、DNS 报文、各代理协议和代理链的握手（进程内假代理） |
| `netfilter/` | NetFilter SDK（头文件和 `nfapi.dll`，C API 版本）。**不在仓库中**，需自行放置，至少包含 `nfsdk/include`、`nfsdk/wfp/include` 和 `nfsdk/wfp/bin/release_c_api/x64/nfapi.dll` |
| `netfilter/driver/` | 已签名的驱动，每个都是 `<驱动>/netfilter2.sys`：`demo`（SDK 演示驱动）和 `a`、`b`（正式版驱动），都是 WFP 的 x64 版。**不在仓库中** |
| `windivert/` | WinDivert 后端资产，`include/windivert.h` 和 `x64/`（`WinDivert.dll`、`WinDivert64.sys`）。**不在仓库中**，自行放置或用 `-DWINDIVERT_DIR=...` 指定 |

## 构建

需要 Qt 6（MinGW 64 位）和 CMake，并按上表放好 NetFilter SDK 和驱动。路径也可以用 `-DNETFILTER_SDK_DIR=...`、`-DNETFILTER_DRIVER_DIR=...` 指定。

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=D:/Qt/6.12.0/mingw_64
cmake --build build
build/winroute_tests.exe
windeployqt --release build/WinRoute.exe
```

构建后，`nfapi.dll` 复制到 `WinRoute.exe` 旁边，三个驱动复制到 `drivers/<代号>/`（没有多余的顶层 `netfilter2.sys`）。放好 `windivert/` 时，`WinDivert.dll` 和 `WinDivert64.sys` 也一并复制到 exe 旁（供 WinDivert 后端运行时加载）。程序 manifest 要求管理员权限。首次启动时，程序把选定的驱动从 `drivers/<代号>/` 复制到 `System32\drivers` 并注册 `netfilter2` 服务。卸载驱动：`WinRoute.exe --uninstall-driver`（同时移除 WinDivert 服务）。

HTTPS 解密用的根 CA 由 OpenSSL 的 `libcrypto` 签发。Qt 自带的 MinGW 工具链里就有 OpenSSL（头文件和导入库在 `<工具链>/opt`），CMake 会自动找到（也可用 `-DOPENSSL_ROOT_DIR=...` 指定）。`libcrypto`/`libssl` 运行期 DLL 会在构建时复制到 exe 旁（`windeployqt` 不复制这两个，但会复制 `plugins/tls/qopensslbackend.dll`）。

加 `-DWINROUTE_REQUIRE_ADMIN=OFF` 可以构建不带管理员 manifest 的开发版，用于不提权调试界面（此时网络拦截不会启动）。

## 安装器

使用 Inno Setup 6（脚本在 `installer/WinRoute.iss`）。安装好 Inno Setup 6 后执行：

```sh
cmake --build build --target installer    # 生成 build/WinRoute-<版本>-setup.exe
```

这个目标会把 `WinRoute.exe`、`nfapi.dll` 和三个驱动（`drivers/demo/`、`drivers/a/`、`drivers/b/`）复制到 `build/stage`，用 `windeployqt` 补齐 Qt 和 MinGW 运行库，再调用 `ISCC` 打包。

- **安装**：只支持 x64 版的 Windows 10 1809 及以上（Qt 6 的要求；驱动是 x64 版，Arm 版 Windows 不能装），需要管理员权限，界面支持中文、日语、英语。可选创建桌面快捷方式和开机自启动计划任务。
- **选择驱动**：“网络驱动”页用下拉框选择，默认 **NetFilter SDK 演示驱动**（1.7.2.9）。演示驱动拦截的连接数有上限，超过后要重启 Windows 才恢复。其余是正式版的**驱动A / B**，没有连接数限制，安装界面只显示代号。升级时沿用上次的选择。三个驱动都装到 `{app}\drivers\<代号>\`，选中的那个由 `WinRoute.exe --install-driver=<代号> --quiet` 从那里注册启动（不再在 `{app}` 放多余的 `netfilter2.sys`），程序内“工具 > 网络驱动”可随时切换。
- **卸载**：删除开机自启动计划任务，执行 `WinRoute.exe --uninstall-driver --quiet` 停止驱动、删除服务和 `System32\drivers\netfilter2.sys`，再询问是否删除配置文件、设置和日志（静默卸载时保留）。
- **运行检测**：WinRoute 运行时会创建互斥量 `WinRoute.AppMutex`，安装和卸载前都会提示先关闭程序。
- **静默安装**：`WinRoute-0.1.0-setup.exe /VERYSILENT /SUPPRESSMSGBOXES /TASKS=autostart /DRIVER=demo`（`/DRIVER=a|b` 选择对应的正式版驱动；不写时，首次安装用演示驱动，升级时沿用上次的选择）
- **代码签名**：在 `WinRoute.iss` 中启用 `SignTool` 和 `SignedUninstaller` 两行，并给 ISCC 配置签名工具。
- `installer/ChineseSimplified.isl` 是 Inno Setup 6.7.3 的简体中文非官方翻译，取自 jrsoftware/issrc。

### 论坛版（吾爱破解）

吾爱破解论坛的版规不允许程序弹出更新提示，也不允许程序里出现论坛以外的网址。为此有一个单独的构建开关，用独立的构建目录生成：

```sh
cmake -S . -B build-forum -G Ninja -DCMAKE_BUILD_TYPE=Release -DWINROUTE_FORUM_BUILD=ON ...
cmake --build build-forum --target installer   # 生成 build-forum/WinRoute-<版本>-52pojie-setup.exe
```

和正常版的区别：

- **更新**：启动时不检查，“帮助”里没有“自动更新”子菜单；只保留“检查更新”和“通过代理检查更新”，用户点了才联网，确认后安装的是正常版。
- **统计**：不含统计地址，不发送任何使用统计，菜单里也没有这一项。
- **链接**：“关于”里不显示任何链接（没有源码、项目主页，也没有论坛链接）。
- **发布**：打版本标签时 GitHub Actions 同时构建两个安装包，都上传到 GitHub release；GitLab release、主页版本列表和 `update.json` 只有正常版。
- **设置默认值**：论坛版启动时把自动更新设为“从不检查”、统计设为关闭，但只在这两项**还没有设置过**时写入（`Updater::setDefaultMode`、`UsageStats::setDefaultEnabled`），已有的选择不改。手动更新到正常版后，正常版读到的是这两个值，所以仍然关闭，之后只有用户自己在菜单里改才会变。

## 多语言

支持简体中文、日语和英语。默认跟随系统界面语言，可在“视图 > 语言”中切换，切换后需要重启程序。

## 切换网络驱动

“工具 > 网络驱动”对话框可切换**拦截后端**：

- **NetFilter**：在程序内安装、切换或卸载驱动（demo / A / B，安装器已把三个都放到 `{app}\drivers\`）。切换时会先停引擎释放驱动句柄，替换驱动文件后再启动，通常一两秒完成。
- **WinDivert**：选择即生效，无需单独安装驱动——首次启动引擎时 WinDivert 自行安装其内核服务（`WinDivert.dll`、`WinDivert64.sys` 随程序分发）。可在同一对话框里卸载该服务。

后端选择持久化在 `QSettings` 的 `backend/active`，安装器的“网络驱动”页也能选（选 WinDivert 时只记录后端偏好）。

- `translations/winroute_zh_CN.ts`、`translations/winroute_ja.ts` 是 Qt Linguist 翻译文件，构建时编译成 `.qm` 并嵌入程序。Qt 自带的 `qtbase_zh_CN` / `qtbase_ja` 也一并嵌入，用于翻译“确定 / 取消”等标准按钮。
- 中日文译文统一维护在 `translations/translations.py` 中。修改源码里的界面文字后，执行：

```sh
cmake --build build --target update_translations   # 从源码提取文字，更新 .ts
python translations/translations.py                 # 写入译文；有缺失或占位符不一致时报错
cmake --build build
```

## 命令行

- `--minimized`：启动到系统托盘（开机自启动用，自启动通过“最高权限”计划任务实现）
- `--install-driver` / `--uninstall-driver`：安装、卸载驱动（安装器使用）；加 `--quiet` 时不弹窗，只用退出码表示结果（0 成功，1 失败）

## 尚未实现

- 代理链和非 SOCKS5 代理的 UDP（SOCKS4/HTTPS 协议本身不支持；单个 SOCKS5 的 UDP 已支持，见上）
- NTLM / Kerberos 代理认证、主密码加密
- Proxifier 自带的 “basic” 加密密码无法解密，导入后需要重新输入密码
