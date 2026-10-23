# WinRoute

用 Qt 实现的 Proxifier 替代品：按规则把 Windows 上任意程序的 TCP 连接转发到 SOCKS4/4A、SOCKS5、HTTPS 代理或代理链。配置文件直接使用 Proxifier 的 `.ppx` 格式，首次启动会自动导入 `%APPDATA%\Proxifier4\Profiles\Default.ppx`。

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

## 目录

| 路径 | 内容 |
| --- | --- |
| `src/core` | 配置读写、规则引擎、驱动接入、DNS、代理协议、中继 |
| `src/ui` | 主窗口（连接 / 流量 / 统计 / 日志）和全部设置对话框 |
| `tests/tests.cpp` | 配置往返、规则匹配、DNS 报文、各代理协议和代理链的握手（进程内假代理） |
| `netfilter/` | NetFilter SDK（头文件和 `nfapi.dll`，C API 版本）。**不在仓库中**，需自行放置，至少包含 `nfsdk/include`、`nfsdk/wfp/include` 和 `nfsdk/wfp/bin/release_c_api/x64/nfapi.dll` |
| `netfilter2.sys` | 已签名的驱动。**不在仓库中**，需自行放在仓库根目录 |

## 构建

需要 Qt 6（MinGW 64 位）和 CMake，并按上表放好 NetFilter SDK 和驱动。路径也可以用 `-DNETFILTER_SDK_DIR=...`、`-DNETFILTER_DRIVER=...` 指定。

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=D:/Qt/6.12.0/mingw_64
cmake --build build
build/winroute_tests.exe
windeployqt --release build/WinRoute.exe
```

构建后，`nfapi.dll` 和 `netfilter2.sys` 会被复制到 `WinRoute.exe` 旁边。程序 manifest 要求管理员权限。首次启动时，程序把驱动复制到 `System32\drivers` 并注册 `netfilter2` 服务。卸载驱动：`WinRoute.exe --uninstall-driver`。

加 `-DWINROUTE_REQUIRE_ADMIN=OFF` 可以构建不带管理员 manifest 的开发版，用于不提权调试界面（此时网络拦截不会启动）。

## 多语言

支持简体中文、日语和英语。默认跟随系统界面语言，可在“视图 > 语言”中切换，切换后需要重启程序。

- `translations/winroute_zh_CN.ts`、`translations/winroute_ja.ts` 是 Qt Linguist 翻译文件，构建时编译成 `.qm` 并嵌入程序。Qt 自带的 `qtbase_zh_CN` / `qtbase_ja` 也一并嵌入，用于翻译“确定 / 取消”等标准按钮。
- 中日文译文统一维护在 `translations/translations.py` 中。修改源码里的界面文字后，执行：

```sh
cmake --build build --target update_translations   # 从源码提取文字，更新 .ts
python translations/translations.py                 # 写入译文；有缺失或占位符不一致时报错
cmake --build build
```

## 命令行

- `--minimized`：启动到系统托盘（开机自启动用，自启动通过“最高权限”计划任务实现）
- `--uninstall-driver`：停止并删除驱动服务

## 尚未实现

- UDP 代理（SOCKS5 UDP ASSOCIATE）；UDP 目前直接放行，与 Proxifier 默认的 `mode_bypass` 相同
- NTLM / Kerberos 代理认证、主密码加密、规则的“指定网卡”高级选项
- Proxifier 自带的 “basic” 加密密码无法解密，导入后需要重新输入密码
