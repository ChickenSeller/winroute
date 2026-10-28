<?xml version='1.0' encoding='utf-8'?>
<TS version="2.1" language="zh_CN">
  <context>
    <name>ChainTypeDialog</name>
    <message>
      <source>Chain Type</source>
      <translation>代理链类型</translation>
    </message>
    <message>
      <source>Simple Chain</source>
      <translation>简单链</translation>
    </message>
    <message>
      <source>Connection to a remote host will be performed sequentially from one proxy server to another.</source>
      <translation>依次经过链中的每个代理服务器连接到远程主机。</translation>
    </message>
    <message>
      <source>Redundancy</source>
      <translation>冗余</translation>
    </message>
    <message>
      <source>If the first proxy fails, the next one will be used, etc.</source>
      <translation>第一个代理失败时使用下一个，依此类推。</translation>
    </message>
    <message>
      <source>Options...</source>
      <translation>选项...</translation>
    </message>
    <message>
      <source>Load Balancing</source>
      <translation>负载均衡</translation>
    </message>
    <message>
      <source>Each connection will be made through a random proxy from the list.</source>
      <translation>每个连接随机使用列表中的一个代理。</translation>
    </message>
    <message>
      <source>Use the same proxy for the same process PID</source>
      <translation>同一进程 PID 使用同一个代理</translation>
    </message>
  </context>
  <context>
    <name>ConnectionsModel</name>
    <message>
      <source>Closed</source>
      <translation>已关闭</translation>
    </message>
    <message>
      <source>Connecting</source>
      <translation>正在连接</translation>
    </message>
    <message>
      <source>Error : %1</source>
      <translation>错误：%1</translation>
    </message>
    <message>
      <source>%1 (pid %2)</source>
      <translation>%1（PID %2）</translation>
    </message>
    <message>
      <source>Application</source>
      <translation>应用程序</translation>
    </message>
    <message>
      <source>Target</source>
      <translation>目标</translation>
    </message>
    <message>
      <source>Protocol</source>
      <translation>协议</translation>
    </message>
    <message>
      <source>Time/Status</source>
      <translation>时间/状态</translation>
    </message>
    <message>
      <source>Rule : Proxy</source>
      <translation>规则 : 代理</translation>
    </message>
    <message>
      <source>Sent</source>
      <translation>已发送</translation>
    </message>
    <message>
      <source>Received</source>
      <translation>已接收</translation>
    </message>
  </context>
  <context>
    <name>DnsClient</name>
    <message>
      <source>resolver is not available</source>
      <translation>解析器不可用</translation>
    </message>
    <message>
      <source>no DNS servers configured</source>
      <translation>未配置 DNS 服务器</translation>
    </message>
    <message>
      <source>host not found</source>
      <translation>找不到主机</translation>
    </message>
    <message>
      <source>DNS error %1</source>
      <translation>DNS 错误 %1</translation>
    </message>
    <message>
      <source>DNS request timed out</source>
      <translation>DNS 请求超时</translation>
    </message>
  </context>
  <context>
    <name>DriverDialog</name>
    <message>
      <source>Network Driver</source>
      <translation>网络驱动</translation>
    </message>
    <message>
      <source>WinRoute can capture connections with the NetFilter SDK driver or with WinDivert. Choose a backend; for NetFilter, choose which driver to use.</source>
      <translation>WinRoute 可以用 NetFilter SDK 驱动或 WinDivert 拦截连接。请选择后端；选 NetFilter 时再选择使用哪个驱动。</translation>
    </message>
    <message>
      <source>NetFilter SDK driver</source>
      <translation>NetFilter SDK 驱动</translation>
    </message>
    <message>
      <source>WinDivert</source>
      <translation>WinDivert</translation>
    </message>
    <message>
      <source>WinDivert (not available)</source>
      <translation>WinDivert（不可用）</translation>
    </message>
    <message>
      <source>Backend:</source>
      <translation>后端：</translation>
    </message>
    <message>
      <source>Driver:</source>
      <translation>驱动：</translation>
    </message>
    <message>
      <source>Use This Driver</source>
      <translation>使用此驱动</translation>
    </message>
    <message>
      <source>Uninstall Driver</source>
      <translation>卸载驱动</translation>
    </message>
    <message>
      <source>an unrecognized driver</source>
      <translation>无法识别的驱动</translation>
    </message>
    <message>
      <source>Installed: %1 (running)</source>
      <translation>已安装：%1（运行中）</translation>
    </message>
    <message>
      <source>Installed: %1 (stopped)</source>
      <translation>已安装：%1（已停止）</translation>
    </message>
    <message>
      <source>No NetFilter driver is installed.</source>
      <translation>未安装 NetFilter 驱动。</translation>
    </message>
    <message>
      <source>WinDivert.dll or its driver is missing next to WinRoute.exe, so this backend cannot be used.</source>
      <translation>WinRoute.exe 旁缺少 WinDivert.dll 或其驱动，无法使用此后端。</translation>
    </message>
    <message>
      <source>WinDivert captures TCP connections at the packet level. It does not support UDP over the proxy or fake-DNS name resolution; host names come from observed DNS responses.</source>
      <translation>WinDivert 在数据包层面拦截 TCP 连接。不支持经代理的 UDP 和假 DNS 解析；主机名来自嗅探到的 DNS 应答。</translation>
    </message>
    <message>
      <source>running</source>
      <translation>运行中</translation>
    </message>
    <message>
      <source>not loaded</source>
      <translation>未加载</translation>
    </message>
    <message>
      <source>WinDivert: %1</source>
      <translation>WinDivert：%1</translation>
    </message>
    <message>
      <source>Uninstall WinDivert</source>
      <translation>卸载 WinDivert</translation>
    </message>
    <message>
      <source>Capture backend switched to WinDivert</source>
      <translation>拦截后端已切换到 WinDivert</translation>
    </message>
    <message>
      <source>Could not switch the capture backend: %1</source>
      <translation>无法切换拦截后端：%1</translation>
    </message>
    <message>
      <source>Could not apply the change:
%1</source>
      <translation>无法应用更改：
%1</translation>
    </message>
    <message>
      <source>Uninstall the NetFilter driver? WinRoute cannot capture connections with this backend without it.</source>
      <translation>卸载 NetFilter 驱动？没有它，此后端无法拦截连接。</translation>
    </message>
    <message>
      <source>Uninstall the WinDivert driver service?</source>
      <translation>卸载 WinDivert 驱动服务？</translation>
    </message>
    <message>
      <source>WinDivert service uninstalled</source>
      <translation>WinDivert 服务已卸载</translation>
    </message>
    <message>
      <source>Could not uninstall:
%1</source>
      <translation>无法卸载：
%1</translation>
    </message>
    <message>
      <source>The evaluation driver from the NetFilter SDK. It stops capturing connections after a limited number of them, until Windows restarts.</source>
      <translation>NetFilter SDK 自带的评估版驱动。拦截的连接数达到上限后会停止工作，重启 Windows 后恢复。</translation>
    </message>
    <message>
      <source>A release build of the driver, without a connection limit.</source>
      <translation>正式版驱动，没有连接数限制。</translation>
    </message>
    <message>
      <source>Switch to This Driver</source>
      <translation>切换到此驱动</translation>
    </message>
    <message>
      <source>Install This Driver</source>
      <translation>安装此驱动</translation>
    </message>
    <message>
      <source>Use WinDivert</source>
      <translation>使用 WinDivert</translation>
    </message>
    <message>
      <source>Network driver switched to %1</source>
      <translation>网络驱动已切换到 %1</translation>
    </message>
    <message>
      <source>Network driver uninstalled</source>
      <translation>网络驱动已卸载</translation>
    </message>
  </context>
  <context>
    <name>EncryptionDialog</name>
    <message>
      <source>Profile Passwords Encryption</source>
      <translation>配置文件密码加密</translation>
    </message>
    <message>
      <source>To provide additional security WinRoute can store proxy passwords in the profile file in encrypted form.</source>
      <translation>为提高安全性，WinRoute 可以将代理密码加密后保存在配置文件中。</translation>
    </message>
    <message>
      <source>Please choose one of the following modes:</source>
      <translation>请选择以下一种模式：</translation>
    </message>
    <message>
      <source>Disabled</source>
      <translation>禁用</translation>
    </message>
    <message>
      <source>Passwords are saved in plain text. Proxy passwords can be used by everyone who has access to the profile file.</source>
      <translation>密码以明文保存。任何能访问配置文件的人都能使用代理密码。</translation>
    </message>
    <message>
      <source>Basic</source>
      <translation>基本</translation>
    </message>
    <message>
      <source>Encryption with a WinRoute static key. Proxy passwords can be used by everyone who opens the profile with WinRoute.</source>
      <translation>使用 WinRoute 固定密钥加密。任何用 WinRoute 打开该配置文件的人都能使用代理密码。</translation>
    </message>
    <message>
      <source>Current user account</source>
      <translation>当前用户账户</translation>
    </message>
    <message>
      <source>Encryption using current user login credentials. Only current user can access proxy passwords.</source>
      <translation>使用当前用户的登录凭据加密。只有当前用户能访问代理密码。</translation>
    </message>
    <message>
      <source>Profile master password</source>
      <translation>配置文件主密码</translation>
    </message>
    <message>
      <source>Encryption with a profile master password (not supported).</source>
      <translation>使用配置文件主密码加密（暂不支持）。</translation>
    </message>
  </context>
  <context>
    <name>Engine</name>
    <message>
      <source>WinDivert is not available (WinDivert.dll or its driver is missing).</source>
      <translation>WinDivert 不可用（缺少 WinDivert.dll 或其驱动）。</translation>
    </message>
    <message>
      <source>Local relay is listening on port %1</source>
      <translation>本地中继正在监听端口 %1</translation>
    </message>
  </context>
  <context>
    <name>HttpProxiesDialog</name>
    <message>
      <source>HTTP Proxy Servers Support</source>
      <translation>HTTP 代理服务器支持</translation>
    </message>
    <message>
      <source>HTTP Proxy Servers</source>
      <translation>HTTP 代理服务器</translation>
    </message>
    <message>
      <source>Enable HTTP proxy servers support</source>
      <translation>启用 HTTP 代理服务器支持</translation>
    </message>
    <message>
      <source>When this option is enabled you can add HTTP proxy servers at the Proxy Servers dialog.</source>
      <translation>启用此选项后，可以在“代理服务器”对话框中添加 HTTP 代理服务器。</translation>
    </message>
    <message>
      <source>WARNING</source>
      <translation>警告</translation>
    </message>
    <message>
      <source>HTTP proxy server can only be used for HTTP connections (web browsers and etc). WinRoute talks to HTTP proxies with CONNECT requests.</source>
      <translation>HTTP 代理服务器只能用于 HTTP 连接（网页浏览器等）。WinRoute 使用 CONNECT 请求与 HTTP 代理通信。</translation>
    </message>
    <message>
      <source>Not to be confused with HTTPS connections and HTTPS proxy servers!</source>
      <translation>请不要与 HTTPS 连接和 HTTPS 代理服务器混淆！</translation>
    </message>
  </context>
  <context>
    <name>Interceptor</name>
    <message>
      <source>Cannot load %1 (error %2)</source>
      <translation>无法加载 %1（错误 %2）</translation>
    </message>
    <message>
      <source>nfapi.dll is missing exports: </source>
      <translation>nfapi.dll 缺少导出函数：</translation>
    </message>
    <message>
      <source> (the C API build is required)</source>
      <translation>（需要 C API 版本）</translation>
    </message>
    <message>
      <source>success</source>
      <translation>成功</translation>
    </message>
    <message>
      <source>failed to open the driver</source>
      <translation>无法打开驱动</translation>
    </message>
    <message>
      <source>invalid endpoint id</source>
      <translation>无效的端点 ID</translation>
    </message>
    <message>
      <source>not initialized</source>
      <translation>未初始化</translation>
    </message>
    <message>
      <source>driver I/O error</source>
      <translation>驱动 I/O 错误</translation>
    </message>
    <message>
      <source>reboot required</source>
      <translation>需要重启</translation>
    </message>
    <message>
      <source>status %1</source>
      <translation>状态 %1</translation>
    </message>
    <message>
      <source>Windows DNS cache flushed</source>
      <translation>已清空 Windows DNS 缓存</translation>
    </message>
    <message>
      <source>Cannot open the service manager (error %1)</source>
      <translation>无法打开服务管理器（错误 %1）</translation>
    </message>
    <message>
      <source>Cannot open the driver service (error %1)</source>
      <translation>无法打开驱动服务（错误 %1）</translation>
    </message>
    <message>
      <source>Cannot delete the driver service (error %1)</source>
      <translation>无法删除驱动服务（错误 %1）</translation>
    </message>
    <message>
      <source>The driver could not be stopped; it will be removed after a reboot</source>
      <translation>无法停止驱动，将在重启后删除</translation>
    </message>
    <message>
      <source>Driver file %1 was not found</source>
      <translation>找不到驱动文件 %1</translation>
    </message>
    <message>
      <source>Cannot copy the driver to %1 (error %2)</source>
      <translation>无法将驱动复制到 %1（错误 %2）</translation>
    </message>
    <message>
      <source>Cannot register the driver: %1</source>
      <translation>无法注册驱动：%1</translation>
    </message>
    <message>
      <source>Network driver %1 installed</source>
      <translation>已安装网络驱动 %1</translation>
    </message>
    <message>
      <source>Cannot delete %1 (error %2)</source>
      <translation>无法删除 %1（错误 %2）</translation>
    </message>
    <message>
      <source>Cannot restart the driver service (error %1)</source>
      <translation>无法重启驱动服务（错误 %1）</translation>
    </message>
    <message>
      <source>Cannot delete %1: the driver is still loaded (close WinRoute, or restart Windows)</source>
      <translation>无法删除 %1：驱动仍在内核中（请关闭 WinRoute，或重启 Windows）</translation>
    </message>
    <message>
      <source>NetFilter SDK demo driver</source>
      <translation>NetFilter SDK 演示驱动</translation>
    </message>
    <message>
      <source>Driver %1</source>
      <translation>驱动 %1</translation>
    </message>
    <message>
      <source>Unrecognized driver</source>
      <translation>无法识别的驱动</translation>
    </message>
    <message>
      <source>Cannot add driver rule (%1): %2</source>
      <translation>无法添加驱动规则（%1）：%2</translation>
    </message>
    <message>
      <source>Administrator rights are required to control the network driver</source>
      <translation>控制网络驱动需要管理员权限</translation>
    </message>
    <message>
      <source>Restarting the network driver to recover from an unclean shutdown</source>
      <translation>正在重启网络驱动以从非正常关闭中恢复</translation>
    </message>
    <message>
      <source>Cannot connect to the network driver %1: %2</source>
      <translation>无法连接网络驱动 %1：%2</translation>
    </message>
    <message>
      <source>Cannot update UDP filtering: %1</source>
      <translation>无法更新 UDP 过滤：%1</translation>
    </message>
    <message>
      <source>%1 - %2 error : unknown fake DNS address, the name was resolved before WinRoute started</source>
      <translation>%1 - %2 错误：未知的虚拟 DNS 地址，该域名是在 WinRoute 启动前解析的</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : blocked</source>
      <translation>%1 - %2 匹配规则 %3：已阻止</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : direct connection</source>
      <translation>%1 - %2 匹配规则 %3：直接连接</translation>
    </message>
    <message>
      <source>%1 - %2 is a proxy server; connecting directly to prevent a connection loop</source>
      <translation>%1 - %2 是代理服务器，为防止连接环路改为直接连接</translation>
    </message>
    <message>
      <source>%1 - %2 error : IPv6 relay is not available, connecting directly</source>
      <translation>%1 - %2 错误：IPv6 中继不可用，改为直接连接</translation>
    </message>
    <message>
      <source>DNS %1 resolved through proxy as %2</source>
      <translation>DNS %1 通过代理解析为 %2</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : blocked (UDP)</source>
      <translation>%1 - %2 匹配规则 %3：已阻止（UDP）</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : %4 does not support UDP, sending directly</source>
      <translation>%1 - %2 匹配规则 %3：%4 不支持 UDP，直接连接</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : proxy chains do not support UDP, sending directly</source>
      <translation>%1 - %2 匹配规则 %3：代理链不支持 UDP，直接连接</translation>
    </message>
    <message>
      <source>Cannot open the WinDivert service (error %1)</source>
      <translation>无法打开 WinDivert 服务（错误 %1）</translation>
    </message>
    <message>
      <source>Cannot delete the WinDivert service (error %1)</source>
      <translation>无法删除 WinDivert 服务（错误 %1）</translation>
    </message>
    <message>
      <source>WinDivert.dll could not be loaded; is it next to WinRoute.exe?</source>
      <translation>无法加载 WinDivert.dll；它是否在 WinRoute.exe 旁边？</translation>
    </message>
    <message>
      <source>WinDivert could not open the network layer (error %1). Administrator rights are required.</source>
      <translation>WinDivert 无法打开网络层（错误 %1）。需要管理员权限。</translation>
    </message>
  </context>
  <context>
    <name>Language</name>
    <message>
      <source>System Default</source>
      <translation>跟随系统</translation>
    </message>
  </context>
  <context>
    <name>MainWindow</name>
    <message>
      <source>Welcome to WinRoute v%1</source>
      <translation>欢迎使用 WinRoute v%1</translation>
    </message>
    <message>
      <source>%1 active connections</source>
      <translation>%1 个活动连接</translation>
    </message>
    <message>
      <source>&amp;New Profile</source>
      <translation>新建配置文件(&amp;N)</translation>
    </message>
    <message>
      <source>&amp;Save Profile As...</source>
      <translation>配置文件另存为(&amp;S)...</translation>
    </message>
    <message>
      <source>&amp;Manage Profiles...</source>
      <translation>管理配置文件(&amp;M)...</translation>
    </message>
    <message>
      <source>&amp;Import Profile...</source>
      <translation>导入配置文件(&amp;I)...</translation>
    </message>
    <message>
      <source>&amp;Export Profile...</source>
      <translation>导出配置文件(&amp;E)...</translation>
    </message>
    <message>
      <source>&amp;Autostart</source>
      <translation>开机自动启动(&amp;A)</translation>
    </message>
    <message>
      <source>Start WinRoute automatically when Windows starts for the current user</source>
      <translation>当前用户登录 Windows 时自动启动 WinRoute</translation>
    </message>
    <message>
      <source>&amp;Close WinRoute Window</source>
      <translation>关闭 WinRoute 窗口(&amp;C)</translation>
    </message>
    <message>
      <source>E&amp;xit</source>
      <translation>退出(&amp;X)</translation>
    </message>
    <message>
      <source>&amp;Proxy Servers...</source>
      <translation>代理服务器(&amp;P)...</translation>
    </message>
    <message>
      <source>Configure proxy servers settings</source>
      <translation>配置代理服务器</translation>
    </message>
    <message>
      <source>Proxification &amp;Rules...</source>
      <translation>代理规则(&amp;R)...</translation>
    </message>
    <message>
      <source>Edit proxification rules</source>
      <translation>编辑代理规则</translation>
    </message>
    <message>
      <source>&amp;Name Resolution...</source>
      <translation>域名解析(&amp;N)...</translation>
    </message>
    <message>
      <source>Change the way WinRoute handles host name resolution</source>
      <translation>更改 WinRoute 处理主机名解析的方式</translation>
    </message>
    <message>
      <source>&amp;HTTP Proxy Servers...</source>
      <translation>HTTP 代理服务器(&amp;H)...</translation>
    </message>
    <message>
      <source>&amp;Services and Other Users...</source>
      <translation>服务和其他用户(&amp;S)...</translation>
    </message>
    <message>
      <source>Handle &amp;Direct Connections</source>
      <translation>处理直接连接(&amp;D)</translation>
    </message>
    <message>
      <source>Display and handle connections that are established directly</source>
      <translation>显示并处理直接建立的连接</translation>
    </message>
    <message>
      <source>Connection &amp;Loop Detection</source>
      <translation>连接环路检测(&amp;L)</translation>
    </message>
    <message>
      <source>Connect directly to proxy servers to prevent infinite connection loops</source>
      <translation>直接连接代理服务器，防止无限连接环路</translation>
    </message>
    <message>
      <source>Route &amp;UDP through SOCKS5</source>
      <translation>通过 SOCKS5 转发 UDP(&amp;U)</translation>
    </message>
    <message>
      <source>Tunnel UDP through the proxy when a rule selects a SOCKS5 proxy (otherwise UDP goes directly)</source>
      <translation>当规则命中 SOCKS5 代理时，通过代理转发 UDP（否则 UDP 直接连接）</translation>
    </message>
    <message>
      <source>&amp;Passwords Encryption...</source>
      <translation>密码加密(&amp;P)...</translation>
    </message>
    <message>
      <source>&amp;Disconnect</source>
      <translation>断开连接(&amp;D)</translation>
    </message>
    <message>
      <source>Disconnect selected connection</source>
      <translation>断开所选连接</translation>
    </message>
    <message>
      <source>Disconnect</source>
      <translation>断开连接</translation>
    </message>
    <message>
      <source>&amp;End Process</source>
      <translation>结束进程(&amp;E)</translation>
    </message>
    <message>
      <source>Terminate parent process of the selected connection</source>
      <translation>结束所选连接所属的进程</translation>
    </message>
    <message>
      <source>&amp;Create Rule for this Application...</source>
      <translation>为此应用程序创建规则(&amp;C)...</translation>
    </message>
    <message>
      <source>&amp;Copy</source>
      <translation>复制(&amp;C)</translation>
    </message>
    <message>
      <source>Select A&amp;ll</source>
      <translation>全选(&amp;L)</translation>
    </message>
    <message>
      <source>Clea&amp;r</source>
      <translation>清除(&amp;R)</translation>
    </message>
    <message>
      <source>Icon in the System &amp;Tray</source>
      <translation>在系统托盘显示图标(&amp;T)</translation>
    </message>
    <message>
      <source>Show WinRoute icon in the System Tray and hide the main window when closed</source>
      <translation>在系统托盘显示 WinRoute 图标，关闭主窗口时隐藏到托盘</translation>
    </message>
    <message>
      <source>&amp;Toolbar</source>
      <translation>工具栏(&amp;T)</translation>
    </message>
    <message>
      <source>&amp;Status Bar</source>
      <translation>状态栏(&amp;S)</translation>
    </message>
    <message>
      <source>Proxy &amp;Checker...</source>
      <translation>代理检测(&amp;C)...</translation>
    </message>
    <message>
      <source>Test proxy servers</source>
      <translation>测试代理服务器</translation>
    </message>
    <message>
      <source>&amp;Network Driver...</source>
      <translation>网络驱动(&amp;N)...</translation>
    </message>
    <message>
      <source>Install, switch or uninstall the network driver</source>
      <translation>安装、切换或卸载网络驱动</translation>
    </message>
    <message>
      <source>&amp;About WinRoute...</source>
      <translation>关于 WinRoute(&amp;A)...</translation>
    </message>
    <message>
      <source>Check for &amp;Updates...</source>
      <translation>检查更新(&amp;U)...</translation>
    </message>
    <message>
      <source>&amp;File</source>
      <translation>文件(&amp;F)</translation>
    </message>
    <message>
      <source>&amp;Load Profile</source>
      <translation>加载配置文件(&amp;L)</translation>
    </message>
    <message>
      <source>&amp;Profile</source>
      <translation>配置文件(&amp;P)</translation>
    </message>
    <message>
      <source>&amp;Advanced</source>
      <translation>高级(&amp;A)</translation>
    </message>
    <message>
      <source>&amp;Log</source>
      <translation>日志(&amp;L)</translation>
    </message>
    <message>
      <source>L&amp;og Level</source>
      <translation>日志级别(&amp;O)</translation>
    </message>
    <message>
      <source>&amp;Errors Only</source>
      <translation>仅错误(&amp;E)</translation>
    </message>
    <message>
      <source>&amp;Normal</source>
      <translation>普通(&amp;N)</translation>
    </message>
    <message>
      <source>&amp;Verbose</source>
      <translation>详细(&amp;V)</translation>
    </message>
    <message>
      <source>&amp;Debug</source>
      <translation>调试(&amp;D)</translation>
    </message>
    <message>
      <source>Log to &amp;File</source>
      <translation>写入日志文件(&amp;F)</translation>
    </message>
    <message>
      <source>&amp;Disabled</source>
      <translation>禁用(&amp;D)</translation>
    </message>
    <message>
      <source>De&amp;bug</source>
      <translation>调试(&amp;B)</translation>
    </message>
    <message>
      <source>Writing log to </source>
      <translation>日志写入到 </translation>
    </message>
    <message>
      <source>&amp;Font Size</source>
      <translation>字体大小(&amp;F)</translation>
    </message>
    <message>
      <source>&amp;Small</source>
      <translation>小(&amp;S)</translation>
    </message>
    <message>
      <source>&amp;Medium</source>
      <translation>中(&amp;M)</translation>
    </message>
    <message>
      <source>&amp;Large</source>
      <translation>大(&amp;L)</translation>
    </message>
    <message>
      <source>&amp;View</source>
      <translation>视图(&amp;V)</translation>
    </message>
    <message>
      <source>&amp;Refresh Rate</source>
      <translation>刷新频率(&amp;R)</translation>
    </message>
    <message>
      <source>&amp;Fast</source>
      <translation>快(&amp;F)</translation>
    </message>
    <message>
      <source>&amp;Slow</source>
      <translation>慢(&amp;S)</translation>
    </message>
    <message>
      <source>&amp;Language</source>
      <translation>语言(&amp;L)</translation>
    </message>
    <message>
      <source>&amp;Tools</source>
      <translation>工具(&amp;T)</translation>
    </message>
    <message>
      <source>&amp;Help</source>
      <translation>帮助(&amp;H)</translation>
    </message>
    <message>
      <source>Automatic U&amp;pdates</source>
      <translation>自动更新(&amp;P)</translation>
    </message>
    <message>
      <source>Install All Updates Automatically</source>
      <translation>自动安装所有更新</translation>
    </message>
    <message>
      <source>Install Required Updates Automatically</source>
      <translation>自动安装必须的更新</translation>
    </message>
    <message>
      <source>Ask Before Updating</source>
      <translation>更新前询问</translation>
    </message>
    <message>
      <source>Never Check</source>
      <translation>从不检查</translation>
    </message>
    <message>
      <source>Send Anonymous Usage &amp;Statistics</source>
      <translation>发送匿名使用统计(&amp;S)</translation>
    </message>
    <message>
      <source>Send anonymous usage statistics to help improve WinRoute (no personal data).</source>
      <translation>发送匿名使用统计以帮助改进 WinRoute（不含个人信息）。</translation>
    </message>
    <message>
      <source>Toolbar</source>
      <translation>工具栏</translation>
    </message>
    <message>
      <source>Proxy Servers</source>
      <translation>代理服务器</translation>
    </message>
    <message>
      <source>Proxification Rules</source>
      <translation>代理规则</translation>
    </message>
    <message>
      <source>Name Resolution</source>
      <translation>域名解析</translation>
    </message>
    <message>
      <source>Proxy Checker</source>
      <translation>代理检测</translation>
    </message>
    <message>
      <source>Connections</source>
      <translation>连接</translation>
    </message>
    <message>
      <source>Traffic</source>
      <translation>流量</translation>
    </message>
    <message>
      <source>Statistics</source>
      <translation>统计</translation>
    </message>
    <message>
      <source>0 active connections</source>
      <translation>0 个活动连接</translation>
    </message>
    <message>
      <source>Backend: WinDivert (unavailable)</source>
      <translation>后端：WinDivert（不可用）</translation>
    </message>
    <message>
      <source>running</source>
      <translation>运行中</translation>
    </message>
    <message>
      <source>not loaded</source>
      <translation>未加载</translation>
    </message>
    <message>
      <source>Backend: %1</source>
      <translation>后端：%1</translation>
    </message>
    <message>
      <source>WinDivert is capturing connections</source>
      <translation>WinDivert 正在拦截连接</translation>
    </message>
    <message>
      <source>WinDivert is not loaded</source>
      <translation>WinDivert 未加载</translation>
    </message>
    <message>
      <source>Driver: not installed</source>
      <translation>驱动：未安装</translation>
    </message>
    <message>
      <source>Demo</source>
      <translation>Demo</translation>
    </message>
    <message>
      <source>Unknown</source>
      <translation>未知</translation>
    </message>
    <message>
      <source>Unlimited</source>
      <translation>无限制</translation>
    </message>
    <message>
      <source>Driver: %1</source>
      <translation>驱动：%1</translation>
    </message>
    <message>
      <source>The network driver is running</source>
      <translation>网络驱动正在运行</translation>
    </message>
    <message>
      <source>The network driver is installed but not running</source>
      <translation>网络驱动已安装但未运行</translation>
    </message>
    <message>
      <source>Signed by %1 — click for details</source>
      <translation>签名者：%1 — 点击查看详情</translation>
    </message>
    <message>
      <source>%1 signatures — click for details</source>
      <translation>%1 张签名 — 点击查看详情</translation>
    </message>
    <message>
      <source>Driver Signature</source>
      <translation>驱动签名</translation>
    </message>
    <message>
      <source>Subject:</source>
      <translation>主体：</translation>
    </message>
    <message>
      <source>Issued by: %1</source>
      <translation>颁发者：%1</translation>
    </message>
    <message>
      <source>&amp;Open WinRoute</source>
      <translation>打开 WinRoute(&amp;O)</translation>
    </message>
    <message>
      <source>Profiles</source>
      <translation>配置文件</translation>
    </message>
    <message>
      <source>WinRoute</source>
      <translation>WinRoute</translation>
    </message>
    <message>
      <source>Network interception started (%1)</source>
      <translation>网络拦截已启动（%1）</translation>
    </message>
    <message>
      <source>Ready</source>
      <translation>就绪</translation>
    </message>
    <message>
      <source>Cannot start network interception: </source>
      <translation>无法启动网络拦截：</translation>
    </message>
    <message>
      <source>Not running: </source>
      <translation>未运行：</translation>
    </message>
    <message>
      <source>WinRoute cannot start network interception:

</source>
      <translation>WinRoute 无法启动网络拦截：

</translation>
    </message>
    <message>
      <source>WinRoute - %1</source>
      <translation>WinRoute - %1</translation>
    </message>
    <message>
      <source>Profile: </source>
      <translation>配置文件：</translation>
    </message>
    <message>
      <source>Imported Proxifier profile </source>
      <translation>已导入 Proxifier 配置文件 </translation>
    </message>
    <message>
      <source>Cannot load profile %1: %2</source>
      <translation>无法加载配置文件 %1：%2</translation>
    </message>
    <message>
      <source>Profile %1 loaded</source>
      <translation>已加载配置文件 %1</translation>
    </message>
    <message>
      <source>Cannot save profile %1: %2</source>
      <translation>无法保存配置文件 %1：%2</translation>
    </message>
    <message>
      <source>No profiles defined</source>
      <translation>没有配置文件</translation>
    </message>
    <message>
      <source>Load this profile</source>
      <translation>加载此配置文件</translation>
    </message>
    <message>
      <source>Profile Name:

Hint: WinRoute automatically loads the most recently used profile on start.</source>
      <translation>配置文件名称：

提示：WinRoute 启动时会自动加载最近使用的配置文件。</translation>
    </message>
    <message>
      <source>The profile name is empty or contains characters not allowed in file names.</source>
      <translation>配置文件名称为空，或包含文件名中不允许的字符。</translation>
    </message>
    <message>
      <source>New Profile</source>
      <translation>新建配置文件</translation>
    </message>
    <message>
      <source>Profile %1 already exists. Overwrite it?</source>
      <translation>配置文件 %1 已存在。要覆盖吗？</translation>
    </message>
    <message>
      <source>New profile %1 created</source>
      <translation>已创建新配置文件 %1</translation>
    </message>
    <message>
      <source>Save Profile As</source>
      <translation>配置文件另存为</translation>
    </message>
    <message>
      <source>Profile saved as %1</source>
      <translation>配置文件已另存为 %1</translation>
    </message>
    <message>
      <source>Import Profile</source>
      <translation>导入配置文件</translation>
    </message>
    <message>
      <source>Profile Files (*.ppx);;All Files (*.*)</source>
      <translation>配置文件 (*.ppx);;所有文件 (*.*)</translation>
    </message>
    <message>
      <source>Cannot import %1:
%2</source>
      <translation>无法导入 %1：
%2</translation>
    </message>
    <message>
      <source>Profile %1 imported from %2</source>
      <translation>已从 %2 导入配置文件 %1</translation>
    </message>
    <message>
      <source>Export Profile</source>
      <translation>导出配置文件</translation>
    </message>
    <message>
      <source>Profile Files (*.ppx)</source>
      <translation>配置文件 (*.ppx)</translation>
    </message>
    <message>
      <source>Profile exported to %1</source>
      <translation>配置文件已导出到 %1</translation>
    </message>
    <message>
      <source>Fail status of all proxies was reset</source>
      <translation>已重置所有代理的失败状态</translation>
    </message>
    <message>
      <source>Chain %1</source>
      <translation>代理链 %1</translation>
    </message>
    <message>
      <source>Please add a proxy server first (Profile &gt; Proxy Servers).</source>
      <translation>请先添加代理服务器（配置文件 &gt; 代理服务器）。</translation>
    </message>
    <message>
      <source>About WinRoute</source>
      <translation>关于 WinRoute</translation>
    </message>
    <message>
      <source>&lt;h3&gt;WinRoute %1&lt;/h3&gt;&lt;p&gt;Routes the network connections of Windows applications through SOCKS and HTTPS proxy servers according to proxification rules.&lt;/p&gt;&lt;p&gt;Interception: %2&lt;br&gt;Qt %3&lt;/p&gt;</source>
      <translation>&lt;h3&gt;WinRoute %1&lt;/h3&gt;&lt;p&gt;按照代理规则，让 Windows 应用程序的网络连接通过 SOCKS 和 HTTPS 代理服务器。&lt;/p&gt;&lt;p&gt;拦截方式：%2&lt;br&gt;Qt %3&lt;/p&gt;</translation>
    </message>
    <message>
      <source>%1 (pid %2)</source>
      <translation>%1（PID %2）</translation>
    </message>
    <message>
      <source>End Process</source>
      <translation>结束进程</translation>
    </message>
    <message>
      <source>The driver is not signed, or its signature could not be read.</source>
      <translation>驱动未签名，或无法读取其签名。</translation>
    </message>
    <message>
      <source>Signing certificates of %1:</source>
      <translation>%1 的签名证书：</translation>
    </message>
    <message>
      <source>Certificate %1</source>
      <translation>证书 %1</translation>
    </message>
    <message>
      <source>Terminate the following process(es)?

</source>
      <translation>要结束以下进程吗？

</translation>
    </message>
    <message>
      <source>Cannot terminate process %1: %2</source>
      <translation>无法结束进程 %1：%2</translation>
    </message>
    <message>
      <source>Down %1, Up %2</source>
      <translation>下行 %1，上行 %2</translation>
    </message>
    <message>
      <source>WinRoute - %1
%2 active connections
Down %3, Up %4</source>
      <translation>WinRoute - %1
%2 个活动连接
下行 %3，上行 %4</translation>
    </message>
    <message>
      <source>Local DNS is available, host names are resolved locally</source>
      <translation>本地 DNS 可用，主机名在本地解析</translation>
    </message>
    <message>
      <source>Local DNS is not available, host names are resolved through proxy</source>
      <translation>本地 DNS 不可用，主机名通过代理解析</translation>
    </message>
    <message>
      <source>Language</source>
      <translation>语言</translation>
    </message>
    <message>
      <source>The new language will be used after WinRoute restarts.

Restart WinRoute now?</source>
      <translation>新语言将在 WinRoute 重启后生效。

现在重启 WinRoute 吗？</translation>
    </message>
    <message>
      <source>Cannot change autostart: </source>
      <translation>无法更改开机自动启动：</translation>
    </message>
    <message>
      <source>Autostart enabled</source>
      <translation>已启用开机自动启动</translation>
    </message>
    <message>
      <source>Autostart disabled</source>
      <translation>已禁用开机自动启动</translation>
    </message>
  </context>
  <context>
    <name>NameResolutionDialog</name>
    <message>
      <source>Name Resolution</source>
      <translation>域名解析</translation>
    </message>
    <message>
      <source>WinRoute DNS settings</source>
      <translation>WinRoute DNS 设置</translation>
    </message>
    <message>
      <source>Detect DNS settings automatically</source>
      <translation>自动检测 DNS 设置</translation>
    </message>
    <message>
      <source>Resolve hostnames through proxy</source>
      <translation>通过代理解析主机名</translation>
    </message>
    <message>
      <source>Try to resolve via local DNS service first
(may cause significant delays if local DNS is unavailable!)</source>
      <translation>先尝试使用本地 DNS 服务解析
（本地 DNS 不可用时可能导致明显延迟！）</translation>
    </message>
    <message>
      <source>Note</source>
      <translation>注意</translation>
    </message>
    <message>
      <source>It is impossible to get real IP addresses through proxy.
WinRoute assigns "fake" IP addresses (198.18.*.*) that are only valid when it is running.</source>
      <translation>通过代理无法获得真实的 IP 地址。
WinRoute 会分配“虚拟”IP 地址（198.18.*.*），它们只在 WinRoute 运行时有效。</translation>
    </message>
    <message>
      <source>DNS name list</source>
      <translation>域名列表</translation>
    </message>
    <message>
      <source>Do NOT resolve the following</source>
      <translation>不解析以下域名</translation>
    </message>
    <message>
      <source>Resolve ONLY the following</source>
      <translation>只解析以下域名</translation>
    </message>
    <message>
      <source>Example:
localhost; *.example.com; %ComputerName%; %SimpleHostnames%</source>
      <translation>示例：
localhost; *.example.com; %ComputerName%; %SimpleHostnames%</translation>
    </message>
  </context>
  <context>
    <name>Profile</name>
    <message>
      <source>Direct</source>
      <translation>直接连接</translation>
    </message>
    <message>
      <source>Block</source>
      <translation>阻止</translation>
    </message>
    <message>
      <source>Proxy %1 %2</source>
      <translation>代理 %1 %2</translation>
    </message>
    <message>
      <source>Proxy &lt;missing&gt;</source>
      <translation>代理 &lt;不存在&gt;</translation>
    </message>
    <message>
      <source>Chain %1</source>
      <translation>代理链 %1</translation>
    </message>
    <message>
      <source>Chain &lt;missing&gt;</source>
      <translation>代理链 &lt;不存在&gt;</translation>
    </message>
    <message>
      <source>Not a Proxifier/WinRoute profile</source>
      <translation>不是 Proxifier/WinRoute 配置文件</translation>
    </message>
    <message>
      <source>XML error at line %1: %2</source>
      <translation>第 %1 行 XML 错误：%2</translation>
    </message>
    <message>
      <source>Password of proxy %1 is encrypted in a format WinRoute cannot read; please re-enter it.</source>
      <translation>代理 %1 的密码采用了 WinRoute 无法读取的加密格式，请重新输入。</translation>
    </message>
  </context>
  <context>
    <name>ProxyAdvancedDialog</name>
    <message>
      <source>Advanced Proxy Server Settings</source>
      <translation>代理服务器高级设置</translation>
    </message>
    <message>
      <source>Appearance</source>
      <translation>外观</translation>
    </message>
    <message>
      <source>("address:port" by default)</source>
      <translation>（默认为“地址:端口”）</translation>
    </message>
    <message>
      <source>Custom Label:</source>
      <translation>自定义标签：</translation>
    </message>
    <message>
      <source>Other</source>
      <translation>其他</translation>
    </message>
    <message>
      <source>Use target hostname in proxy request if available</source>
      <translation>代理请求中尽量使用目标主机名</translation>
    </message>
  </context>
  <context>
    <name>ProxyCheckerDialog</name>
    <message>
      <source>Chain</source>
      <translation>代理链</translation>
    </message>
    <message>
      <source>Proxy Checker</source>
      <translation>代理检测</translation>
    </message>
    <message>
      <source>Proxy server:</source>
      <translation>代理服务器：</translation>
    </message>
    <message>
      <source>Port:</source>
      <translation>端口：</translation>
    </message>
    <message>
      <source>Target host:</source>
      <translation>目标主机：</translation>
    </message>
    <message>
      <source>Start Testing</source>
      <translation>开始测试</translation>
    </message>
    <message>
      <source>Close</source>
      <translation>关闭</translation>
    </message>
    <message>
      <source>&lt;b&gt;Testing Started...&lt;/b&gt;</source>
      <translation>&lt;b&gt;开始测试...&lt;/b&gt;</translation>
    </message>
    <message>
      <source>Proxy Server%1&lt;br&gt;&amp;nbsp;&amp;nbsp;Address: %2:%3&lt;br&gt;&amp;nbsp;&amp;nbsp;Protocol: %4&lt;br&gt;&amp;nbsp;&amp;nbsp;Authentication: %5</source>
      <translation>代理服务器%1&lt;br&gt;&amp;nbsp;&amp;nbsp;地址：%2:%3&lt;br&gt;&amp;nbsp;&amp;nbsp;协议：%4&lt;br&gt;&amp;nbsp;&amp;nbsp;身份验证：%5</translation>
    </message>
    <message>
      <source>YES</source>
      <translation>是</translation>
    </message>
    <message>
      <source>NO</source>
      <translation>否</translation>
    </message>
    <message>
      <source>&lt;b&gt;Connection Test&lt;/b&gt;</source>
      <translation>&lt;b&gt;连接测试&lt;/b&gt;</translation>
    </message>
    <message>
      <source>Connecting to %1:%2 ...</source>
      <translation>正在连接 %1:%2 ...</translation>
    </message>
    <message>
      <source>Connection established %1 (%2 ms)</source>
      <translation>连接已建立 %1（%2 毫秒）</translation>
    </message>
    <message>
      <source>&lt;b&gt;Connection through the Proxy Server Test&lt;/b&gt;</source>
      <translation>&lt;b&gt;通过代理服务器连接测试&lt;/b&gt;</translation>
    </message>
    <message>
      <source>Connecting to %1:%2 through the proxy ...</source>
      <translation>正在通过代理连接 %1:%2 ...</translation>
    </message>
    <message>
      <source>Connection to %1 established through the proxy %2 (%3 ms)</source>
      <translation>已通过代理连接到 %1 %2（%3 毫秒）</translation>
    </message>
    <message>
      <source>TLS backend unavailable; skipping the HTTPS request test (the tunnel is up).</source>
      <translation>没有可用的 TLS 后端，跳过 HTTPS 请求测试（隧道已建立）。</translation>
    </message>
    <message>
      <source>&lt;b&gt;TLS Handshake Test&lt;/b&gt;</source>
      <translation>&lt;b&gt;TLS 握手测试&lt;/b&gt;</translation>
    </message>
    <message>
      <source>TLS handshake %1 (%2 ms)</source>
      <translation>TLS 握手 %1（%2 毫秒）</translation>
    </message>
    <message>
      <source>TLS warning: %1</source>
      <translation>TLS 警告：%1</translation>
    </message>
    <message>
      <source>%1 : TLS handshake timed out</source>
      <translation>%1：TLS 握手超时</translation>
    </message>
    <message>
      <source>&lt;b&gt;HTTP Request Test&lt;/b&gt;</source>
      <translation>&lt;b&gt;HTTP 请求测试&lt;/b&gt;</translation>
    </message>
    <message>
      <source>Response: %1 %2</source>
      <translation>响应：%1 %2</translation>
    </message>
    <message>
      <source>%1 : connection closed without a response</source>
      <translation>%1：连接已关闭，没有收到响应</translation>
    </message>
    <message>
      <source>%1 : no response within 10 seconds</source>
      <translation>%1：10 秒内没有响应</translation>
    </message>
    <message>
      <source>UDP test skipped: a proxy chain cannot be tested for UDP.</source>
      <translation>跳过 UDP 测试：代理链无法测试 UDP。</translation>
    </message>
    <message>
      <source>UDP test skipped: UDP applies to SOCKS5 proxies only.</source>
      <translation>跳过 UDP 测试：UDP 仅适用于 SOCKS5 代理。</translation>
    </message>
    <message>
      <source>&lt;b&gt;UDP Support Test (SOCKS5 UDP ASSOCIATE)&lt;/b&gt;</source>
      <translation>&lt;b&gt;UDP 支持测试（SOCKS5 UDP ASSOCIATE）&lt;/b&gt;</translation>
    </message>
    <message>
      <source>Requesting a UDP relay from %1:%2 ...</source>
      <translation>正在向 %1:%2 请求 UDP 中继 ...</translation>
    </message>
    <message>
      <source>the proxy did not grant a UDP relay in time</source>
      <translation>代理未能及时分配 UDP 中继</translation>
    </message>
    <message>
      <source>not a SOCKS5 proxy</source>
      <translation>不是 SOCKS5 代理</translation>
    </message>
    <message>
      <source>proxy requires authentication</source>
      <translation>代理需要身份验证</translation>
    </message>
    <message>
      <source>unsupported authentication method</source>
      <translation>不支持的身份验证方法</translation>
    </message>
    <message>
      <source>authentication failed</source>
      <translation>身份验证失败</translation>
    </message>
    <message>
      <source>malformed reply</source>
      <translation>回复格式错误</translation>
    </message>
    <message>
      <source>UDP ASSOCIATE rejected (code %1)</source>
      <translation>UDP ASSOCIATE 被拒绝（代码 %1）</translation>
    </message>
    <message>
      <source>Relay granted at %1:%2</source>
      <translation>已分配中继 %1:%2</translation>
    </message>
    <message>
      <source>cannot open a local UDP socket</source>
      <translation>无法打开本地 UDP 套接字</translation>
    </message>
    <message>
      <source>Sending a DNS query to 8.8.8.8:53 through the relay ...</source>
      <translation>正在通过中继向 8.8.8.8:53 发送 DNS 查询 ...</translation>
    </message>
    <message>
      <source>no reply from the relay within 5 seconds (UDP may be blocked)</source>
      <translation>5 秒内没有收到中继的回复（UDP 可能被阻止）</translation>
    </message>
    <message>
      <source>UDP ASSOCIATE: %1 supported (a DNS reply came back through the relay)</source>
      <translation>UDP ASSOCIATE：%1 支持（已通过中继收到 DNS 回复）</translation>
    </message>
    <message>
      <source>UDP ASSOCIATE: %1 not available%2</source>
      <translation>UDP ASSOCIATE：%1 不可用%2</translation>
    </message>
    <message>
      <source>&lt;b&gt;Testing Finished.&lt;/b&gt; Proxy server is working correctly!</source>
      <translation>&lt;b&gt;测试完成。&lt;/b&gt;代理服务器工作正常！</translation>
    </message>
    <message>
      <source>&lt;b&gt;Testing Finished.&lt;/b&gt; &lt;span style='color:#c0201a'&gt;The proxy server test failed.&lt;/span&gt;</source>
      <translation>&lt;b&gt;测试完成。&lt;/b&gt; &lt;span style='color:#c0201a'&gt;代理服务器测试失败。&lt;/span&gt;</translation>
    </message>
  </context>
  <context>
    <name>ProxyConnector</name>
    <message>
      <source>Could not connect to %1 - connection timeout</source>
      <translation>无法连接到 %1 - 连接超时</translation>
    </message>
    <message>
      <source>Could not connect to proxy %1 - connection timeout</source>
      <translation>无法连接到代理 %1 - 连接超时</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - proxy handshake timeout</source>
      <translation>无法通过代理 %1 连接 - 代理握手超时</translation>
    </message>
    <message>
      <source>No target address</source>
      <translation>没有目标地址</translation>
    </message>
    <message>
      <source>No resolver available</source>
      <translation>没有可用的解析器</translation>
    </message>
    <message>
      <source>Could not resolve %1 - %2</source>
      <translation>无法解析 %1 - %2</translation>
    </message>
    <message>
      <source>Connection closed</source>
      <translation>连接已关闭</translation>
    </message>
    <message>
      <source>Connected to proxy %1</source>
      <translation>已连接到代理 %1</translation>
    </message>
    <message>
      <source>Could not connect to %1 - %2</source>
      <translation>无法连接到 %1 - %2</translation>
    </message>
    <message>
      <source>Could not connect to proxy %1 - %2</source>
      <translation>无法连接到代理 %1 - %2</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - %2</source>
      <translation>无法通过代理 %1 连接 - %2</translation>
    </message>
    <message>
      <source>Connected to proxy %1 through the chain</source>
      <translation>已通过代理链连接到代理 %1</translation>
    </message>
    <message>
      <source>general SOCKS server failure</source>
      <translation>SOCKS 服务器一般性故障</translation>
    </message>
    <message>
      <source>connection not allowed by ruleset</source>
      <translation>规则集不允许此连接</translation>
    </message>
    <message>
      <source>network unreachable</source>
      <translation>网络不可达</translation>
    </message>
    <message>
      <source>host unreachable</source>
      <translation>主机不可达</translation>
    </message>
    <message>
      <source>connection refused by the target</source>
      <translation>目标拒绝连接</translation>
    </message>
    <message>
      <source>TTL expired</source>
      <translation>TTL 已过期</translation>
    </message>
    <message>
      <source>command not supported</source>
      <translation>不支持该命令</translation>
    </message>
    <message>
      <source>address type not supported</source>
      <translation>不支持该地址类型</translation>
    </message>
    <message>
      <source>error code %1</source>
      <translation>错误代码 %1</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - not a SOCKS5 proxy</source>
      <translation>无法通过代理 %1 连接 - 不是 SOCKS5 代理</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - proxy requires authentication</source>
      <translation>无法通过代理 %1 连接 - 代理需要身份验证</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - unsupported authentication method %2</source>
      <translation>无法通过代理 %1 连接 - 不支持的身份验证方式 %2</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - authentication failed (wrong username or password)</source>
      <translation>无法通过代理 %1 连接 - 身份验证失败（用户名或密码错误）</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - malformed reply</source>
      <translation>无法通过代理 %1 连接 - 应答格式错误</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - SOCKS4 does not support IPv6 targets</source>
      <translation>无法通过代理 %1 连接 - SOCKS4 不支持 IPv6 目标</translation>
    </message>
    <message>
      <source>identd authentication failed</source>
      <translation>identd 身份验证失败</translation>
    </message>
    <message>
      <source>request rejected or failed</source>
      <translation>请求被拒绝或失败</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - %2 (code %3)</source>
      <translation>无法通过代理 %1 连接 - %2（代码 %3）</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - response header too large</source>
      <translation>无法通过代理 %1 连接 - 响应头过大</translation>
    </message>
    <message>
      <source>proxy requires authentication (HTTP 407)</source>
      <translation>代理需要身份验证（HTTP 407）</translation>
    </message>
    <message>
      <source>not an HTTP proxy</source>
      <translation>不是 HTTP 代理</translation>
    </message>
    <message>
      <source>proxy returned HTTP %1</source>
      <translation>代理返回 HTTP %1</translation>
    </message>
  </context>
  <context>
    <name>ProxyServerDialog</name>
    <message>
      <source>Proxy Server</source>
      <translation>代理服务器</translation>
    </message>
    <message>
      <source>Server</source>
      <translation>服务器</translation>
    </message>
    <message>
      <source>IP address or host name</source>
      <translation>IP 地址或主机名</translation>
    </message>
    <message>
      <source>SOCKS Version 4</source>
      <translation>SOCKS 版本 4</translation>
    </message>
    <message>
      <source>SOCKS Version 5</source>
      <translation>SOCKS 版本 5</translation>
    </message>
    <message>
      <source>HTTPS</source>
      <translation>HTTPS</translation>
    </message>
    <message>
      <source>HTTP</source>
      <translation>HTTP</translation>
    </message>
    <message>
      <source>Address:</source>
      <translation>地址：</translation>
    </message>
    <message>
      <source>Port:</source>
      <translation>端口：</translation>
    </message>
    <message>
      <source>Protocol:</source>
      <translation>协议：</translation>
    </message>
    <message>
      <source>Authentication</source>
      <translation>身份验证</translation>
    </message>
    <message>
      <source>Enable</source>
      <translation>启用</translation>
    </message>
    <message>
      <source>Use NTLM</source>
      <translation>使用 NTLM</translation>
    </message>
    <message>
      <source>Use Kerberos</source>
      <translation>使用 Kerberos</translation>
    </message>
    <message>
      <source>Not supported</source>
      <translation>不支持</translation>
    </message>
    <message>
      <source>Username:</source>
      <translation>用户名：</translation>
    </message>
    <message>
      <source>Password:</source>
      <translation>密码：</translation>
    </message>
    <message>
      <source>Options</source>
      <translation>选项</translation>
    </message>
    <message>
      <source>Use SOCKS 4A extension (remote hostname resolving feature)</source>
      <translation>使用 SOCKS 4A 扩展（远程解析主机名）</translation>
    </message>
    <message>
      <source>OK</source>
      <translation>确定</translation>
    </message>
    <message>
      <source>Check</source>
      <translation>检测</translation>
    </message>
    <message>
      <source>Advanced...</source>
      <translation>高级...</translation>
    </message>
    <message>
      <source>Cancel</source>
      <translation>取消</translation>
    </message>
    <message>
      <source>No options for SOCKS5 are available.</source>
      <translation>SOCKS5 没有可用选项。</translation>
    </message>
    <message>
      <source>The proxy must support HTTP tunneling (CONNECT requests).</source>
      <translation>代理必须支持 HTTP 隧道（CONNECT 请求）。</translation>
    </message>
    <message>
      <source>HTTP proxy servers can be used for HTTP connections only.</source>
      <translation>HTTP 代理服务器只能用于 HTTP 连接。</translation>
    </message>
    <message>
      <source>Please enter the proxy server address.</source>
      <translation>请输入代理服务器地址。</translation>
    </message>
  </context>
  <context>
    <name>ProxyServersDialog</name>
    <message>
      <source>Proxy Servers</source>
      <translation>代理服务器</translation>
    </message>
    <message>
      <source>Address</source>
      <translation>地址</translation>
    </message>
    <message>
      <source>Port</source>
      <translation>端口</translation>
    </message>
    <message>
      <source>Type</source>
      <translation>类型</translation>
    </message>
    <message>
      <source>Add...</source>
      <translation>添加...</translation>
    </message>
    <message>
      <source>Edit...</source>
      <translation>编辑...</translation>
    </message>
    <message>
      <source>Remove</source>
      <translation>删除</translation>
    </message>
    <message>
      <source>Check...</source>
      <translation>检测...</translation>
    </message>
    <message>
      <source>You can chain several proxy servers together:</source>
      <translation>可以将多个代理服务器串成代理链：</translation>
    </message>
    <message>
      <source>Proxy Chains...</source>
      <translation>代理链...</translation>
    </message>
    <message>
      <source>Create</source>
      <translation>创建</translation>
    </message>
    <message>
      <source>Type...</source>
      <translation>类型...</translation>
    </message>
    <message>
      <source>Drag and Drop proxy servers within a chain to alter the order.
Use check boxes to enable/disable a proxy in a chain.</source>
      <translation>在代理链内拖放代理服务器可以调整顺序。
用复选框启用/禁用链中的代理。</translation>
    </message>
    <message>
      <source>Proxy Chain</source>
      <translation>代理链</translation>
    </message>
    <message>
      <source>Chain name:</source>
      <translation>代理链名称：</translation>
    </message>
    <message>
      <source>Drag and Drop a proxy server from the list to add it into a chain.</source>
      <translation>从列表中把代理服务器拖放到代理链中即可添加。</translation>
    </message>
    <message>
      <source> (Redundancy)</source>
      <translation>（冗余）</translation>
    </message>
    <message>
      <source> (Load Balancing)</source>
      <translation>（负载均衡）</translation>
    </message>
    <message>
      <source>&lt;missing proxy %1&gt;</source>
      <translation>&lt;不存在的代理 %1&gt;</translation>
    </message>
    <message>
      <source>Do you want to use this proxy by default?

The Default rule will be changed to use %1.
You can change this later in Proxification Rules.</source>
      <translation>要默认使用此代理吗？

Default 规则将改为使用 %1。
之后可以在“代理规则”中修改。</translation>
    </message>
    <message>
      <source>Proxy Removal</source>
      <translation>删除代理</translation>
    </message>
    <message>
      <source>Chain Removal</source>
      <translation>删除代理链</translation>
    </message>
    <message>
      <source>%1 %2 is assigned as the Action for the following Rule(s):</source>
      <translation>%1 %2 被以下规则用作操作：</translation>
    </message>
    <message>
      <source>Proxy</source>
      <translation>代理</translation>
    </message>
    <message>
      <source>Chain</source>
      <translation>代理链</translation>
    </message>
    <message>
      <source>To proceed with the removal you can:</source>
      <translation>要继续删除，可以：</translation>
    </message>
    <message>
      <source>Remove the Rule(s)</source>
      <translation>删除这些规则</translation>
    </message>
    <message>
      <source>Select a new Action:</source>
      <translation>选择新的操作：</translation>
    </message>
    <message>
      <source>Direct</source>
      <translation>直接连接</translation>
    </message>
    <message>
      <source>Block</source>
      <translation>阻止</translation>
    </message>
    <message>
      <source>Proxy %1 %2</source>
      <translation>代理 %1 %2</translation>
    </message>
    <message>
      <source>Chain %1</source>
      <translation>代理链 %1</translation>
    </message>
    <message>
      <source>The Default rule is used when no other rule matches, so it cannot be removed.</source>
      <translation>Default 规则在其他规则都不匹配时使用，因此不能删除。</translation>
    </message>
    <message>
      <source>Chain%1</source>
      <translation>代理链%1</translation>
    </message>
  </context>
  <context>
    <name>RedundancyOptionsDialog</name>
    <message>
      <source>Redundancy Options</source>
      <translation>冗余选项</translation>
    </message>
    <message>
      <source>Recheck failed proxy each (sec.):</source>
      <translation>重新检测失败代理的间隔（秒）：</translation>
    </message>
    <message>
      <source>Connect directly if all proxies fail</source>
      <translation>所有代理都失败时直接连接</translation>
    </message>
    <message>
      <source>Reset Fail Status for All Proxies</source>
      <translation>重置所有代理的失败状态</translation>
    </message>
    <message>
      <source>Connection to proxy timeout (sec.):</source>
      <translation>连接代理超时（秒）：</translation>
    </message>
  </context>
  <context>
    <name>Relay</name>
    <message>
      <source>%1 bytes</source>
      <translation>%1 字节</translation>
    </message>
    <message>
      <source>%1 bytes (%2)</source>
      <translation>%1 字节（%2）</translation>
    </message>
    <message>
      <source>no active profile</source>
      <translation>没有活动的配置文件</translation>
    </message>
    <message>
      <source>Direct</source>
      <translation>直接连接</translation>
    </message>
    <message>
      <source>proxy server #%1 used by rule %2 does not exist</source>
      <translation>规则 %2 使用的代理服务器 #%1 不存在</translation>
    </message>
    <message>
      <source>proxy chain #%1 used by rule %2 does not exist</source>
      <translation>规则 %2 使用的代理链 #%1 不存在</translation>
    </message>
    <message>
      <source>Chain %1</source>
      <translation>代理链 %1</translation>
    </message>
    <message>
      <source>proxy chain %1 has no enabled proxy servers</source>
      <translation>代理链 %1 中没有启用的代理服务器</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : using %4</source>
      <translation>%1 - %2 匹配规则 %3：使用 %4</translation>
    </message>
    <message>
      <source>direct connection</source>
      <translation>直接连接</translation>
    </message>
    <message>
      <source>%1 - %2 %3, trying the next proxy in the chain</source>
      <translation>%1 - %2 %3，尝试链中的下一个代理</translation>
    </message>
    <message>
      <source>%1 - %2 open directly</source>
      <translation>%1 - %2 直接连接</translation>
    </message>
    <message>
      <source>%1 - %2 open through proxy %3:%4 %5</source>
      <translation>%1 - %2 通过代理 %3:%4 %5 连接</translation>
    </message>
    <message>
      <source>%1 - %2 open through %3</source>
      <translation>%1 - %2 通过 %3 连接</translation>
    </message>
    <message>
      <source>%1 - %2 error : %3</source>
      <translation>%1 - %2 错误：%3</translation>
    </message>
    <message>
      <source>%1 - %2 close, %3 sent, %4 received, lifetime %5</source>
      <translation>%1 - %2 关闭，发送 %3，接收 %4，持续 %5</translation>
    </message>
    <message>
      <source>%1 - %2 closed by the application before the connection was established</source>
      <translation>%1 - %2 在连接建立前被应用程序关闭</translation>
    </message>
    <message>
      <source>Cannot bind the DNS resolver socket</source>
      <translation>无法绑定 DNS 解析器套接字</translation>
    </message>
    <message>
      <source>Cannot start the local relay: </source>
      <translation>无法启动本地中继：</translation>
    </message>
    <message>
      <source>IPv6 relay is not available: </source>
      <translation>IPv6 中继不可用：</translation>
    </message>
    <message>
      <source>Relay: unexpected connection from %1 dropped</source>
      <translation>中继：已丢弃来自 %1 的意外连接</translation>
    </message>
    <message>
      <source>%1 - UDP proxy #%2 used by rule %3 does not exist</source>
      <translation>%1 - 规则 %3 使用的 UDP 代理 #%2 不存在</translation>
    </message>
    <message>
      <source>the network driver API is unavailable</source>
      <translation>网络驱动接口不可用</translation>
    </message>
    <message>
      <source>no resolver available</source>
      <translation>解析器不可用</translation>
    </message>
    <message>
      <source>could not resolve proxy %1 - %2</source>
      <translation>无法解析代理 %1 - %2</translation>
    </message>
    <message>
      <source>not a SOCKS5 proxy</source>
      <translation>不是 SOCKS5 代理</translation>
    </message>
    <message>
      <source>SOCKS5 authentication method not accepted</source>
      <translation>SOCKS5 认证方式未被接受</translation>
    </message>
    <message>
      <source>SOCKS5 authentication failed</source>
      <translation>SOCKS5 认证失败</translation>
    </message>
    <message>
      <source>malformed SOCKS5 reply</source>
      <translation>SOCKS5 应答格式错误</translation>
    </message>
    <message>
      <source>the proxy does not support UDP (SOCKS5 reply %1)</source>
      <translation>代理不支持 UDP（SOCKS5 应答 %1）</translation>
    </message>
    <message>
      <source>cannot open the local UDP socket</source>
      <translation>无法打开本地 UDP 套接字</translation>
    </message>
    <message>
      <source>%1 - %2 open through proxy %3:%4 SOCKS5 (UDP)</source>
      <translation>%1 - %2 通过代理 %3:%4 SOCKS5 连接（UDP）</translation>
    </message>
    <message>
      <source>%1 - %2 UDP cannot use the proxy (%3); sending directly</source>
      <translation>%1 - %2 UDP 无法使用代理（%3），改为直接连接</translation>
    </message>
    <message>
      <source>the proxy closed the control connection</source>
      <translation>代理关闭了控制连接</translation>
    </message>
    <message>
      <source>%1 - %2 UDP closed, %3 sent, %4 received</source>
      <translation>%1 - %2 UDP 关闭，发送 %3，接收 %4</translation>
    </message>
  </context>
  <context>
    <name>RuleDialog</name>
    <message>
      <source>Direct</source>
      <translation>直接连接</translation>
    </message>
    <message>
      <source>Block</source>
      <translation>阻止</translation>
    </message>
    <message>
      <source>Proxy %1 %2</source>
      <translation>代理 %1 %2</translation>
    </message>
    <message>
      <source>Chain %1</source>
      <translation>代理链 %1</translation>
    </message>
    <message>
      <source>Proxification Rule</source>
      <translation>代理规则</translation>
    </message>
    <message>
      <source>Enabled</source>
      <translation>启用</translation>
    </message>
    <message>
      <source>Name:</source>
      <translation>名称：</translation>
    </message>
    <message>
      <source>Applications</source>
      <translation>应用程序</translation>
    </message>
    <message>
      <source>Any</source>
      <translation>任意</translation>
    </message>
    <message>
      <source>Browse...</source>
      <translation>浏览...</translation>
    </message>
    <message>
      <source>Add Files...</source>
      <translation>添加文件...</translation>
    </message>
    <message>
      <source>Add Folder...</source>
      <translation>添加文件夹...</translation>
    </message>
    <message>
      <source>Example: iexplore.exe; "C:\some app.exe"; C:\Games\; fire*.exe</source>
      <translation>示例：iexplore.exe; "C:\some app.exe"; C:\Games\; fire*.exe</translation>
    </message>
    <message>
      <source>Target hosts</source>
      <translation>目标主机</translation>
    </message>
    <message>
      <source>Example: 127.0.0.1; *.example.com; 192.168.1.*; 10.1.0.0-10.5.255.255</source>
      <translation>示例：127.0.0.1; *.example.com; 192.168.1.*; 10.1.0.0-10.5.255.255</translation>
    </message>
    <message>
      <source>Target ports</source>
      <translation>目标端口</translation>
    </message>
    <message>
      <source>Example: 80; 8000-9000; 3128</source>
      <translation>示例：80; 8000-9000; 3128</translation>
    </message>
    <message>
      <source>Advanced...</source>
      <translation>高级...</translation>
    </message>
    <message>
      <source>Per-rule network interface selection is not supported yet</source>
      <translation>暂不支持为规则指定网络接口</translation>
    </message>
    <message>
      <source>Action:</source>
      <translation>操作：</translation>
    </message>
    <message>
      <source>Select Applications</source>
      <translation>选择应用程序</translation>
    </message>
    <message>
      <source>Applications (*.exe);;All files (*.*)</source>
      <translation>应用程序 (*.exe);;所有文件 (*.*)</translation>
    </message>
    <message>
      <source>Select a Folder</source>
      <translation>选择文件夹</translation>
    </message>
    <message>
      <source>Please enter the rule name.</source>
      <translation>请输入规则名称。</translation>
    </message>
    <message>
      <source>Target ports must look like: 80; 8000-9000; 3128</source>
      <translation>目标端口的格式应为：80; 8000-9000; 3128</translation>
    </message>
  </context>
  <context>
    <name>RulesDialog</name>
    <message>
      <source>Proxification Rules</source>
      <translation>代理规则</translation>
    </message>
    <message>
      <source>Name</source>
      <translation>名称</translation>
    </message>
    <message>
      <source>Applications</source>
      <translation>应用程序</translation>
    </message>
    <message>
      <source>Target Hosts</source>
      <translation>目标主机</translation>
    </message>
    <message>
      <source>Target Ports</source>
      <translation>目标端口</translation>
    </message>
    <message>
      <source>Action</source>
      <translation>操作</translation>
    </message>
    <message>
      <source>Move up</source>
      <translation>上移</translation>
    </message>
    <message>
      <source>Move down</source>
      <translation>下移</translation>
    </message>
    <message>
      <source>Add...</source>
      <translation>添加...</translation>
    </message>
    <message>
      <source>Clone</source>
      <translation>复制</translation>
    </message>
    <message>
      <source>Edit...</source>
      <translation>编辑...</translation>
    </message>
    <message>
      <source>Remove</source>
      <translation>删除</translation>
    </message>
    <message>
      <source>Rules are processed from top to bottom. Use the arrows at the right to change the order.

Use check boxes to enable/disable a rule.</source>
      <translation>规则按从上到下的顺序处理。用右侧的箭头调整顺序。

用复选框启用/禁用规则。</translation>
    </message>
    <message>
      <source>Any</source>
      <translation>任意</translation>
    </message>
    <message>
      <source>New</source>
      <translation>新规则</translation>
    </message>
    <message>
      <source> (copy)</source>
      <translation>（副本）</translation>
    </message>
    <message>
      <source>The Default rule is used when all other rules do not match the connection parameters, so it cannot be removed.</source>
      <translation>Default 规则在所有其他规则都不匹配连接参数时使用，因此不能删除。</translation>
    </message>
  </context>
  <context>
    <name>ServicesDialog</name>
    <message>
      <source>Services and other users</source>
      <translation>服务和其他用户</translation>
    </message>
    <message>
      <source>Info</source>
      <translation>说明</translation>
    </message>
    <message>
      <source>Normally WinRoute processes connections for the current user only. You can change this behavior if needed.</source>
      <translation>默认情况下 WinRoute 只处理当前用户的连接。如有需要可以更改此行为。</translation>
    </message>
    <message>
      <source>Note: these options only affect TCP and UDP. ICMP traffic, such as ping and tracert, cannot be routed through a proxy regardless of this setting.</source>
      <translation>注意：这些选项只对 TCP 和 UDP 生效。ICMP 流量（如 ping、tracert）无论此设置如何都无法经由代理转发。</translation>
    </message>
    <message>
      <source>Additional WinRoute targets</source>
      <translation>WinRoute 额外处理的对象</translation>
    </message>
    <message>
      <source>Applications run by other users on this computer</source>
      <translation>此计算机上其他用户运行的应用程序</translation>
    </message>
    <message>
      <source>Windows services and other system processes</source>
      <translation>Windows 服务和其他系统进程</translation>
    </message>
  </context>
  <context>
    <name>StatisticsWidget</name>
    <message>
      <source>Application / Target</source>
      <translation>应用程序 / 目标</translation>
    </message>
    <message>
      <source>Connections</source>
      <translation>连接</translation>
    </message>
    <message>
      <source>Active</source>
      <translation>活动</translation>
    </message>
    <message>
      <source>Sent</source>
      <translation>已发送</translation>
    </message>
    <message>
      <source>Received</source>
      <translation>已接收</translation>
    </message>
    <message>
      <source>Last Activity</source>
      <translation>最近活动</translation>
    </message>
    <message>
      <source>&amp;Copy</source>
      <translation>复制(&amp;C)</translation>
    </message>
    <message>
      <source>&amp;Reset Statistics</source>
      <translation>重置统计(&amp;R)</translation>
    </message>
  </context>
  <context>
    <name>TrafficGraph</name>
    <message>
      <source>-%1 min</source>
      <translation>-%1 分钟</translation>
    </message>
  </context>
  <context>
    <name>TrafficWidget</name>
    <message>
      <source>Show Both</source>
      <translation>显示全部</translation>
    </message>
    <message>
      <source>Incoming Only</source>
      <translation>仅接收</translation>
    </message>
    <message>
      <source>Outgoing Only</source>
      <translation>仅发送</translation>
    </message>
    <message>
      <source>Select the kind of the traffic to display</source>
      <translation>选择要显示的流量类型</translation>
    </message>
    <message>
      <source>Clear</source>
      <translation>清除</translation>
    </message>
    <message>
      <source>Incoming: %1 (max %2)</source>
      <translation>接收：%1（最大 %2）</translation>
    </message>
    <message>
      <source>Outgoing: %1 (max %2)</source>
      <translation>发送：%1（最大 %2）</translation>
    </message>
    <message>
      <source>Total: %1 received, %2 sent</source>
      <translation>合计：接收 %1，发送 %2</translation>
    </message>
  </context>
  <context>
    <name>Units</name>
    <message>
      <source>%1 bytes</source>
      <translation>%1 字节</translation>
    </message>
    <message>
      <source>%1 B/sec</source>
      <translation>%1 B/秒</translation>
    </message>
    <message>
      <source>%1 KB/sec</source>
      <translation>%1 KB/秒</translation>
    </message>
    <message>
      <source>%1 MB/sec</source>
      <translation>%1 MB/秒</translation>
    </message>
  </context>
  <context>
    <name>Updater</name>
    <message>
      <source>WinRoute Update</source>
      <translation>WinRoute 更新</translation>
    </message>
    <message>
      <source>Could not check for updates:
%1</source>
      <translation>无法检查更新：
%1</translation>
    </message>
    <message>
      <source>WinRoute %1 is the latest version.</source>
      <translation>WinRoute %1 已是最新版本。</translation>
    </message>
    <message>
      <source>WinRoute %1 is available; you have %2.
Update now? WinRoute restarts when it is done.</source>
      <translation>WinRoute %1 已发布，当前版本是 %2。
现在更新吗？更新完成后 WinRoute 会重新启动。</translation>
    </message>
    <message>
      <source>Update</source>
      <translation>更新</translation>
    </message>
    <message>
      <source>Later</source>
      <translation>以后再说</translation>
    </message>
    <message>
      <source>Downloading WinRoute %1...</source>
      <translation>正在下载 WinRoute %1...</translation>
    </message>
    <message>
      <source>Cancel</source>
      <translation>取消</translation>
    </message>
    <message>
      <source>Downloading WinRoute %1; WinRoute restarts once it is installed.</source>
      <translation>正在下载 WinRoute %1，安装完成后 WinRoute 会重新启动。</translation>
    </message>
    <message>
      <source>The download is damaged (checksum mismatch).</source>
      <translation>下载的文件已损坏（校验和不一致）。</translation>
    </message>
    <message>
      <source>Could not download WinRoute %1:
%2</source>
      <translation>无法下载 WinRoute %1：
%2</translation>
    </message>
    <message>
      <source>Could not prepare the update.</source>
      <translation>无法准备更新。</translation>
    </message>
    <message>
      <source>Could not start the update.</source>
      <translation>无法开始更新。</translation>
    </message>
  </context>
  <context>
    <name>main</name>
    <message>
      <source>Cannot install the network driver: %1</source>
      <translation>无法安装网络驱动：%1</translation>
    </message>
    <message>
      <source>Cannot uninstall the network driver: %1</source>
      <translation>无法卸载网络驱动：%1</translation>
    </message>
  </context>
</TS>