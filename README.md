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

## 安装器

使用 Inno Setup 6（脚本在 `installer/WinRoute.iss`）。安装好 Inno Setup 6 后执行：

```sh
cmake --build build --target installer    # 生成 build/WinRoute-<版本>-setup.exe
```

这个目标会把 `WinRoute.exe`、`nfapi.dll` 和两个驱动（`drivers/demo/`、`drivers/zhengyun/`）复制到 `build/stage`，用 `windeployqt` 补齐 Qt 和 MinGW 运行库，再调用 `ISCC` 打包。

- **安装**：需要管理员权限，界面支持中文、日语、英语。可选创建桌面快捷方式和开机自启动计划任务。
- **选择驱动**：“网络驱动”页用下拉框选择，默认 **NetFilter SDK 演示驱动**（1.7.2.9，`NETFILTER_DEMO_DRIVER`，默认取自 SDK 的 `wfp/bin/driver/x64/`）。演示驱动拦截的连接数有上限，超过后要重启 Windows 才恢复。另一项是四川征云签名的正式版驱动（1.7.1.8，`NETFILTER_DRIVER`）。升级时沿用上次的选择。只安装选中的那个驱动，文件复制完成后执行 `WinRoute.exe --install-driver --quiet` 注册并启动；如果系统里已装的是另一个驱动，会先停掉并替换。
- **卸载**：删除开机自启动计划任务，执行 `WinRoute.exe --uninstall-driver --quiet` 停止驱动、删除服务和 `System32\drivers
etfilter2.sys`，再询问是否删除配置文件、设置和日志（静默卸载时保留）。
- **运行检测**：WinRoute 运行时会创建互斥量 `WinRoute.AppMutex`，安装和卸载前都会提示先关闭程序。
- **静默安装**：`WinRoute-0.1.0-setup.exe /VERYSILENT /SUPPRESSMSGBOXES /TASKS=autostart /DRIVER=demo`（`/DRIVER=zhengyun` 选择征云驱动；不写时，首次安装用演示驱动，升级时沿用上次的选择）
- **代码签名**：在 `WinRoute.iss` 中启用 `SignTool` 和 `SignedUninstaller` 两行，并给 ISCC 配置签名工具。
- `installer/ChineseSimplified.isl` 是 Inno Setup 6.7.3 的简体中文非官方翻译，取自 jrsoftware/issrc。

## 项目主页和发布

主页在 `site/`，做法与 Glimpse 相同：

- **主页**：`index.html` 和版本页 `releases.html`，中日英三语，语言切换和译文在 `i18n.js`。图标是 Material Symbols（Apache-2.0）。
- **部署**：`.gitlab-ci.yml` 的 `pages` 任务在 `site/` 有改动时，把它发布到 GitLab Pages，并替换 `%PROJECT_URL%` 等占位符。
- **版本列表**：版本页读取 `releases.json`，由 pages.yanlei.org 上的 OpenResty 实时从 GitLab 的 Release 生成。安装包通过 `/winroute-downloads/<版本>/<文件>` 转发自项目的软件包仓库。服务器端配置在 `deploy/nginx/`（项目 ID 151），两个令牌文件只放在服务器上。

## 发布新版本

GitLab Runner 是 Linux，Windows 安装包由 GitHub Actions 构建，流程与 Glimpse 相同：

1. 修改 `CMakeLists.txt` 里的版本号，提交并推送；
2. 打带说明的标签并推送：`git tag -a v0.1.0 -m "WinRoute 0.1.0"`，然后 `git push origin v0.1.0`。标签说明会成为版本说明：以 `- ` 开头的行是列表项，`New`、`Fixes` 这类短行是小标题。

接下来自动完成：

1. GitLab 的 `sync-tag-github` 把标签推到 GitHub；
2. `.github/workflows/release.yml` 在 Windows 上构建、跑测试、打安装包，并创建 GitHub Release；
3. GitLab 的 `release-from-github` 等它完成后，把安装包存进本项目的软件包仓库，并创建 GitLab Release；
4. 一分钟内主页的版本列表就会出现新版本。

NetFilter SDK 和驱动不在仓库里。GitHub 构建时，从本项目的软件包仓库下载 `winroute-build-deps`，并按 `release.yml` 中的 SHA-256 校验。SDK 或驱动有变化时，执行 `tools\upload-build-deps.ps1 -Version <新版本>`，再把输出的版本和 SHA-256 填进 `release.yml`。

需要设置的密钥：

- GitLab CI/CD 变量：`GITHUB_REPO`（如 `kaguya/winroute`）、`GITHUB_TOKEN`（只限这个仓库的细粒度令牌：Contents 和 Workflows 读写，Actions 只读；设为 Masked、Protected，并保护 `v*` 标签）。
- GitHub 仓库 secret：`BUILD_DEPS_TOKEN`，即 GitLab 部署令牌，只勾 `read_package_registry`。

不经过 GitHub 时，也可以在装有 SDK 的本机执行 `tools\publish-release.ps1 -Notes "..."` 直接发布（需要 `GITLAB_TOKEN`，权限 api）。

本地预览主页：

```sh
cd site && python -m http.server 8000
```

占位符不会被替换，所以页面上的链接无效；`releases.json` 需要自己准备一份测试数据。

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
- `--install-driver` / `--uninstall-driver`：安装、卸载驱动（安装器使用）；加 `--quiet` 时不弹窗，只用退出码表示结果（0 成功，1 失败）

## 尚未实现

- UDP 代理（SOCKS5 UDP ASSOCIATE）；UDP 目前直接放行，与 Proxifier 默认的 `mode_bypass` 相同
- NTLM / Kerberos 代理认证、主密码加密、规则的“指定网卡”高级选项
- Proxifier 自带的 “basic” 加密密码无法解密，导入后需要重新输入密码
