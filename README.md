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
- **自身流量**：驱动规则放行 WinRoute 自己的进程，不会形成环路；连接目标就是代理服务器本身时，“连接环路检测”会改为直连。
- **代理链**：简单链、冗余（失败切换下一个，可设重检间隔和全部失败时直连）、负载均衡（随机，或同一 PID 固定用一个代理）。
- **UDP**：默认直接放行（`mode_bypass`）。在“配置 > 高级 > 通过 SOCKS5 转发 UDP”开启后，命中规则且代理是单个 SOCKS5 的 UDP 会通过 SOCKS5 UDP ASSOCIATE 转发（驱动改为过滤全部出站 UDP，中继线程为每个 UDP 套接字建一个关联，回包用 `nf_udpPostReceive` 注回）。代理链或非 SOCKS5（SOCKS4/HTTPS）不支持 UDP，会直接放行并在日志提示一次；关联失败也回退为直连。

### 拦截后端

有两种拦截后端，在“工具 > 网络驱动”里选择（`QSettings` 的 `backend/active`）：

- **NetFilter（默认）**：上图所示，`netfilter2.sys` 在 connect 回调里改写目标，支持 UDP 代理与假 DNS。
- **WinDivert（`WinDivertInterceptor`）**：仅 TCP。SOCKET 层句柄取每条出站连接的 PID 做规则匹配，NETWORK 层句柄把要代理的连接的数据包做 NAT 重定向到本地中继（目标改成源 IP 的中继端口，**保留应用源端口**，中继据此从 `FlowTable` 还原原始目标）；自身与中继上行流量不在 NAT 表中，天然防环。**不支持** UDP 代理和假 DNS——主机名只来自嗅探到的真实 DNS 应答。首次使用时 WinDivert 自行安装其内核服务（需管理员）。

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

## 多语言

支持简体中文、日语和英语。默认跟随系统界面语言，可在“视图 > 语言”中切换，切换后需要重启程序。

## 切换网络驱动

“工具 > 网络驱动”对话框可在程序内安装、切换或卸载驱动（demo / A / B，安装器已把三个都放到 `{app}\drivers\`）。切换时会先停引擎释放驱动句柄，替换驱动文件后再启动，通常一两秒完成。

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
- NTLM / Kerberos 代理认证、主密码加密、规则的“指定网卡”高级选项
- Proxifier 自带的 “basic” 加密密码无法解密，导入后需要重新输入密码
