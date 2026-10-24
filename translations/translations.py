# -*- coding: utf-8 -*-
# Chinese (Simplified) and Japanese translations of WinRoute's UI and log texts.
# Fill the .ts files with:  python translations/translations.py
# (run "cmake --build build --target update_translations" first to pick up new source texts)
#
# Keys are the English source texts exactly as they appear in the code.
# Placeholders (%1, %2, ...), HTML tags and "&" accelerators must be kept.

T = {
    # ---- chains ----
    "Chain Type": ("代理链类型", "チェーンの種類"),
    "Simple Chain": ("简单链", "シンプルチェーン"),
    "Connection to a remote host will be performed sequentially from one proxy server to another.":
        ("依次经过链中的每个代理服务器连接到远程主机。", "チェーン内のプロキシサーバーを順番に経由してリモートホストに接続します。"),
    "Redundancy": ("冗余", "冗長化"),
    "If the first proxy fails, the next one will be used, etc.":
        ("第一个代理失败时使用下一个，依此类推。", "最初のプロキシが失敗した場合は次のプロキシを使用します。"),
    "Options...": ("选项...", "オプション..."),
    "Load Balancing": ("负载均衡", "負荷分散"),
    "Each connection will be made through a random proxy from the list.":
        ("每个连接随机使用列表中的一个代理。", "接続ごとにリストからランダムにプロキシを選択します。"),
    "Use the same proxy for the same process PID": ("同一进程 PID 使用同一个代理", "同じプロセス PID には同じプロキシを使用する"),
    "Redundancy Options": ("冗余选项", "冗長化オプション"),
    "Recheck failed proxy each (sec.):": ("重新检测失败代理的间隔（秒）：", "失敗したプロキシの再確認間隔（秒）："),
    "Connect directly if all proxies fail": ("所有代理都失败时直接连接", "すべてのプロキシが失敗した場合は直接接続する"),
    "Reset Fail Status for All Proxies": ("重置所有代理的失败状态", "すべてのプロキシの失敗状態をリセット"),
    "Connection to proxy timeout (sec.):": ("连接代理超时（秒）：", "プロキシへの接続タイムアウト（秒）："),

    # ---- connections list ----
    "Closed": ("已关闭", "切断済み"),
    "Connecting": ("正在连接", "接続中"),
    "Error : %1": ("错误：%1", "エラー：%1"),
    "%1 (pid %2)": ("%1（PID %2）", "%1（PID %2）"),
    "Application": ("应用程序", "アプリケーション"),
    "Target": ("目标", "接続先"),
    "Time/Status": ("时间/状态", "時間/状態"),
    "Rule : Proxy": ("规则 : 代理", "ルール : プロキシ"),
    "Sent": ("已发送", "送信"),
    "Received": ("已接收", "受信"),

    # ---- DNS client ----
    "resolver is not available": ("解析器不可用", "リゾルバーを利用できません"),
    "no DNS servers configured": ("未配置 DNS 服务器", "DNS サーバーが設定されていません"),
    "host not found": ("找不到主机", "ホストが見つかりません"),
    "DNS error %1": ("DNS 错误 %1", "DNS エラー %1"),
    "DNS request timed out": ("DNS 请求超时", "DNS 要求がタイムアウトしました"),

    # ---- advanced dialogs ----
    "Profile Passwords Encryption": ("配置文件密码加密", "プロファイルのパスワード暗号化"),
    "To provide additional security WinRoute can store proxy passwords in the profile file in encrypted form.":
        ("为提高安全性，WinRoute 可以将代理密码加密后保存在配置文件中。", "セキュリティを高めるため、WinRoute はプロキシのパスワードを暗号化してプロファイルに保存できます。"),
    "Please choose one of the following modes:": ("请选择以下一种模式：", "次のいずれかのモードを選択してください："),
    "Disabled": ("禁用", "無効"),
    "Passwords are saved in plain text. Proxy passwords can be used by everyone who has access to the profile file.":
        ("密码以明文保存。任何能访问配置文件的人都能使用代理密码。", "パスワードは平文で保存されます。プロファイルにアクセスできる人は誰でもプロキシのパスワードを使用できます。"),
    "Basic": ("基本", "基本"),
    "Encryption with a WinRoute static key. Proxy passwords can be used by everyone who opens the profile with WinRoute.":
        ("使用 WinRoute 固定密钥加密。任何用 WinRoute 打开该配置文件的人都能使用代理密码。", "WinRoute の固定キーで暗号化します。WinRoute でプロファイルを開く人は誰でもプロキシのパスワードを使用できます。"),
    "Current user account": ("当前用户账户", "現在のユーザーアカウント"),
    "Encryption using current user login credentials. Only current user can access proxy passwords.":
        ("使用当前用户的登录凭据加密。只有当前用户能访问代理密码。", "現在のユーザーのログオン資格情報で暗号化します。現在のユーザーだけがプロキシのパスワードにアクセスできます。"),
    "Profile master password": ("配置文件主密码", "プロファイルのマスターパスワード"),
    "Encryption with a profile master password (not supported).": ("使用配置文件主密码加密（暂不支持）。", "プロファイルのマスターパスワードで暗号化します（未対応）。"),
    "HTTP Proxy Servers Support": ("HTTP 代理服务器支持", "HTTP プロキシサーバーのサポート"),
    "HTTP Proxy Servers": ("HTTP 代理服务器", "HTTP プロキシサーバー"),
    "Enable HTTP proxy servers support": ("启用 HTTP 代理服务器支持", "HTTP プロキシサーバーのサポートを有効にする"),
    "When this option is enabled you can add HTTP proxy servers at the Proxy Servers dialog.":
        ("启用此选项后，可以在“代理服务器”对话框中添加 HTTP 代理服务器。", "このオプションを有効にすると、「プロキシサーバー」ダイアログで HTTP プロキシサーバーを追加できます。"),
    "WARNING": ("警告", "警告"),
    "HTTP proxy server can only be used for HTTP connections (web browsers and etc). WinRoute talks to HTTP proxies with CONNECT requests.":
        ("HTTP 代理服务器只能用于 HTTP 连接（网页浏览器等）。WinRoute 使用 CONNECT 请求与 HTTP 代理通信。", "HTTP プロキシサーバーは HTTP 接続（Web ブラウザーなど）にのみ使用できます。WinRoute は CONNECT 要求で HTTP プロキシと通信します。"),
    "Not to be confused with HTTPS connections and HTTPS proxy servers!":
        ("请不要与 HTTPS 连接和 HTTPS 代理服务器混淆！", "HTTPS 接続や HTTPS プロキシサーバーと混同しないでください！"),
    "Services and other users": ("服务和其他用户", "サービスと他のユーザー"),
    "Info": ("说明", "情報"),
    "Normally WinRoute processes connections for the current user only. You can change this behavior if needed.":
        ("默认情况下 WinRoute 只处理当前用户的连接。如有需要可以更改此行为。", "通常、WinRoute は現在のユーザーの接続のみを処理します。必要に応じてこの動作を変更できます。"),
    "Additional WinRoute targets": ("WinRoute 额外处理的对象", "WinRoute の追加対象"),
    "Applications run by other users on this computer": ("此计算机上其他用户运行的应用程序", "このコンピューターで他のユーザーが実行するアプリケーション"),
    "Windows services and other system processes": ("Windows 服务和其他系统进程", "Windows サービスとその他のシステムプロセス"),

    # ---- engine / driver ----
    "Local relay is listening on port %1": ("本地中继正在监听端口 %1", "ローカルリレーはポート %1 で待ち受けています"),
    "Cannot load %1 (error %2)": ("无法加载 %1（错误 %2）", "%1 を読み込めません（エラー %2）"),
    "nfapi.dll is missing exports: ": ("nfapi.dll 缺少导出函数：", "nfapi.dll にエクスポートがありません："),
    " (the C API build is required)": ("（需要 C API 版本）", "（C API 版が必要です）"),
    "success": ("成功", "成功"),
    "failed to open the driver": ("无法打开驱动", "ドライバーを開けません"),
    "invalid endpoint id": ("无效的端点 ID", "無効なエンドポイント ID"),
    "not initialized": ("未初始化", "初期化されていません"),
    "driver I/O error": ("驱动 I/O 错误", "ドライバーの I/O エラー"),
    "reboot required": ("需要重启", "再起動が必要です"),
    "status %1": ("状态 %1", "状態 %1"),
    "Windows DNS cache flushed": ("已清空 Windows DNS 缓存", "Windows の DNS キャッシュを消去しました"),
    "Driver file %1 was not found": ("找不到驱动文件 %1", "ドライバーファイル %1 が見つかりません"),
    "Cannot copy the driver to %1 (error %2)": ("无法将驱动复制到 %1（错误 %2）", "ドライバーを %1 にコピーできません（エラー %2）"),
    "Cannot register the driver: %1": ("无法注册驱动：%1", "ドライバーを登録できません：%1"),
    "Network driver %1 installed": ("已安装网络驱动 %1", "ネットワークドライバー %1 をインストールしました"),
    "Cannot add driver rule (%1): %2": ("无法添加驱动规则（%1）：%2", "ドライバーのルールを追加できません（%1）：%2"),
    "Administrator rights are required to control the network driver": ("控制网络驱动需要管理员权限", "ネットワークドライバーの制御には管理者権限が必要です"),
    "Cannot connect to the network driver %1: %2": ("无法连接网络驱动 %1：%2", "ネットワークドライバー %1 に接続できません：%2"),
    "%1 - %2 error : unknown fake DNS address, the name was resolved before WinRoute started":
        ("%1 - %2 错误：未知的虚拟 DNS 地址，该域名是在 WinRoute 启动前解析的", "%1 - %2 エラー：不明な仮想 DNS アドレスです。この名前は WinRoute の起動前に解決されました"),
    "%1 - %2 matching %3 rule : blocked": ("%1 - %2 匹配规则 %3：已阻止", "%1 - %2 ルール %3 に一致：ブロック"),
    "%1 - %2 matching %3 rule : direct connection": ("%1 - %2 匹配规则 %3：直接连接", "%1 - %2 ルール %3 に一致：直接接続"),
    "%1 - %2 is a proxy server; connecting directly to prevent a connection loop":
        ("%1 - %2 是代理服务器，为防止连接环路改为直接连接", "%1 - %2 はプロキシサーバーです。接続ループを防ぐため直接接続します"),
    "%1 - %2 error : IPv6 relay is not available, connecting directly":
        ("%1 - %2 错误：IPv6 中继不可用，改为直接连接", "%1 - %2 エラー：IPv6 リレーを利用できないため直接接続します"),
    "DNS %1 resolved through proxy as %2": ("DNS %1 通过代理解析为 %2", "DNS %1 をプロキシ経由で %2 として解決しました"),
    "Cannot uninstall the network driver: %1": ("无法卸载网络驱动：%1", "ネットワークドライバーをアンインストールできません：%1"),
    "Cannot install the network driver: %1": ("无法安装网络驱动：%1", "ネットワークドライバーをインストールできません：%1"),
    "Cannot open the service manager (error %1)": ("无法打开服务管理器（错误 %1）", "サービスマネージャーを開けません（エラー %1）"),
    "Cannot open the driver service (error %1)": ("无法打开驱动服务（错误 %1）", "ドライバーのサービスを開けません（エラー %1）"),
    "Cannot delete the driver service (error %1)": ("无法删除驱动服务（错误 %1）", "ドライバーのサービスを削除できません（エラー %1）"),
    "The driver could not be stopped; it will be removed after a reboot": ("无法停止驱动，将在重启后删除", "ドライバーを停止できませんでした。再起動後に削除されます"),
    "Cannot delete %1 (error %2)": ("无法删除 %1（错误 %2）", "%1 を削除できません（エラー %2）"),

    # ---- language ----
    "System Default": ("跟随系统", "システムの既定"),
    "Language": ("语言", "言語"),
    "&Language": ("语言(&L)", "言語(&L)"),
    "The new language will be used after WinRoute restarts.\n\nRestart WinRoute now?":
        ("新语言将在 WinRoute 重启后生效。\n\n现在重启 WinRoute 吗？", "新しい言語は WinRoute の再起動後に適用されます。\n\n今すぐ WinRoute を再起動しますか？"),

    # ---- main window ----
    "Welcome to WinRoute v%1": ("欢迎使用 WinRoute v%1", "WinRoute v%1 へようこそ"),
    "%1 active connections": ("%1 个活动连接", "アクティブな接続 %1 件"),
    "0 active connections": ("0 个活动连接", "アクティブな接続 0 件"),
    "&New Profile": ("新建配置文件(&N)", "新しいプロファイル(&N)"),
    "&Save Profile As...": ("配置文件另存为(&S)...", "名前を付けてプロファイルを保存(&S)..."),
    "&Manage Profiles...": ("管理配置文件(&M)...", "プロファイルの管理(&M)..."),
    "&Import Profile...": ("导入配置文件(&I)...", "プロファイルのインポート(&I)..."),
    "&Export Profile...": ("导出配置文件(&E)...", "プロファイルのエクスポート(&E)..."),
    "&Autostart": ("开机自动启动(&A)", "自動起動(&A)"),
    "Start WinRoute automatically when Windows starts for the current user":
        ("当前用户登录 Windows 时自动启动 WinRoute", "現在のユーザーが Windows にサインインしたときに WinRoute を自動的に起動します"),
    "&Close WinRoute Window": ("关闭 WinRoute 窗口(&C)", "WinRoute ウィンドウを閉じる(&C)"),
    "E&xit": ("退出(&X)", "終了(&X)"),
    "&Proxy Servers...": ("代理服务器(&P)...", "プロキシサーバー(&P)..."),
    "Configure proxy servers settings": ("配置代理服务器", "プロキシサーバーを設定します"),
    "Proxification &Rules...": ("代理规则(&R)...", "プロキシ化ルール(&R)..."),
    "Edit proxification rules": ("编辑代理规则", "プロキシ化ルールを編集します"),
    "&Name Resolution...": ("域名解析(&N)...", "名前解決(&N)..."),
    "Change the way WinRoute handles host name resolution": ("更改 WinRoute 处理主机名解析的方式", "WinRoute のホスト名解決の方法を変更します"),
    "&HTTP Proxy Servers...": ("HTTP 代理服务器(&H)...", "HTTP プロキシサーバー(&H)..."),
    "&Services and Other Users...": ("服务和其他用户(&S)...", "サービスと他のユーザー(&S)..."),
    "Handle &Direct Connections": ("处理直接连接(&D)", "直接接続を処理する(&D)"),
    "Display and handle connections that are established directly": ("显示并处理直接建立的连接", "直接確立される接続も表示して処理します"),
    "Connection &Loop Detection": ("连接环路检测(&L)", "接続ループの検出(&L)"),
    "Connect directly to proxy servers to prevent infinite connection loops": ("直接连接代理服务器，防止无限连接环路", "無限の接続ループを防ぐため、プロキシサーバーには直接接続します"),
    "&Passwords Encryption...": ("密码加密(&P)...", "パスワードの暗号化(&P)..."),
    "&Disconnect": ("断开连接(&D)", "切断(&D)"),
    "Disconnect": ("断开连接", "切断"),
    "Disconnect selected connection": ("断开所选连接", "選択した接続を切断します"),
    "&End Process": ("结束进程(&E)", "プロセスの終了(&E)"),
    "Terminate parent process of the selected connection": ("结束所选连接所属的进程", "選択した接続の親プロセスを終了します"),
    "&Create Rule for this Application...": ("为此应用程序创建规则(&C)...", "このアプリケーションのルールを作成(&C)..."),
    "&Copy": ("复制(&C)", "コピー(&C)"),
    "Select A&ll": ("全选(&L)", "すべて選択(&L)"),
    "Clea&r": ("清除(&R)", "クリア(&R)"),
    "Icon in the System &Tray": ("在系统托盘显示图标(&T)", "システムトレイにアイコンを表示(&T)"),
    "Show WinRoute icon in the System Tray and hide the main window when closed":
        ("在系统托盘显示 WinRoute 图标，关闭主窗口时隐藏到托盘", "システムトレイに WinRoute のアイコンを表示し、閉じたときはメインウィンドウを隠します"),
    "&Toolbar": ("工具栏(&T)", "ツールバー(&T)"),
    "&Status Bar": ("状态栏(&S)", "ステータスバー(&S)"),
    "Proxy &Checker...": ("代理检测(&C)...", "プロキシチェッカー(&C)..."),
    "Test proxy servers": ("测试代理服务器", "プロキシサーバーをテストします"),
    "&About WinRoute...": ("关于 WinRoute(&A)...", "WinRoute について(&A)..."),
    "&File": ("文件(&F)", "ファイル(&F)"),
    "&Load Profile": ("加载配置文件(&L)", "プロファイルの読み込み(&L)"),
    "&Profile": ("配置文件(&P)", "プロファイル(&P)"),
    "&Advanced": ("高级(&A)", "詳細設定(&A)"),
    "&Log": ("日志(&L)", "ログ(&L)"),
    "L&og Level": ("日志级别(&O)", "ログレベル(&O)"),
    "&Errors Only": ("仅错误(&E)", "エラーのみ(&E)"),
    "&Normal": ("普通(&N)", "標準(&N)"),
    "&Verbose": ("详细(&V)", "詳細(&V)"),
    "&Debug": ("调试(&D)", "デバッグ(&D)"),
    "Log to &File": ("写入日志文件(&F)", "ファイルにログを記録(&F)"),
    "&Disabled": ("禁用(&D)", "無効(&D)"),
    "De&bug": ("调试(&B)", "デバッグ(&B)"),
    "Writing log to ": ("日志写入到 ", "ログの書き込み先："),
    "&Font Size": ("字体大小(&F)", "フォントサイズ(&F)"),
    "&Small": ("小(&S)", "小(&S)"),
    "&Medium": ("中(&M)", "中(&M)"),
    "&Large": ("大(&L)", "大(&L)"),
    "&View": ("视图(&V)", "表示(&V)"),
    "&Refresh Rate": ("刷新频率(&R)", "更新間隔(&R)"),
    "&Fast": ("快(&F)", "速い(&F)"),
    "&Slow": ("慢(&S)", "遅い(&S)"),
    "&Tools": ("工具(&T)", "ツール(&T)"),
    "&Help": ("帮助(&H)", "ヘルプ(&H)"),
    "Toolbar": ("工具栏", "ツールバー"),
    "Proxy Servers": ("代理服务器", "プロキシサーバー"),
    "Proxification Rules": ("代理规则", "プロキシ化ルール"),
    "Name Resolution": ("域名解析", "名前解決"),
    "Proxy Checker": ("代理检测", "プロキシチェッカー"),
    "Connections": ("连接", "接続"),
    "Traffic": ("流量", "トラフィック"),
    "Statistics": ("统计", "統計"),
    "&Open WinRoute": ("打开 WinRoute(&O)", "WinRoute を開く(&O)"),
    "Profiles": ("配置文件", "プロファイル"),
    "WinRoute": ("WinRoute", "WinRoute"),
    "Network interception started (%1)": ("网络拦截已启动（%1）", "ネットワークの捕捉を開始しました（%1）"),
    "Ready": ("就绪", "準備完了"),
    "Cannot start network interception: ": ("无法启动网络拦截：", "ネットワークの捕捉を開始できません："),
    "Not running: ": ("未运行：", "停止中："),
    "WinRoute cannot start network interception:\n\n": ("WinRoute 无法启动网络拦截：\n\n", "WinRoute はネットワークの捕捉を開始できません：\n\n"),
    "WinRoute - %1": ("WinRoute - %1", "WinRoute - %1"),
    "Profile: ": ("配置文件：", "プロファイル："),
    "Imported Proxifier profile ": ("已导入 Proxifier 配置文件 ", "Proxifier のプロファイルをインポートしました："),
    "Cannot load profile %1: %2": ("无法加载配置文件 %1：%2", "プロファイル %1 を読み込めません：%2"),
    "Profile %1 loaded": ("已加载配置文件 %1", "プロファイル %1 を読み込みました"),
    "Cannot save profile %1: %2": ("无法保存配置文件 %1：%2", "プロファイル %1 を保存できません：%2"),
    "No profiles defined": ("没有配置文件", "プロファイルがありません"),
    "Load this profile": ("加载此配置文件", "このプロファイルを読み込みます"),
    "Profile Name:\n\nHint: WinRoute automatically loads the most recently used profile on start.":
        ("配置文件名称：\n\n提示：WinRoute 启动时会自动加载最近使用的配置文件。", "プロファイル名：\n\nヒント：WinRoute は起動時に最後に使用したプロファイルを自動的に読み込みます。"),
    "The profile name is empty or contains characters not allowed in file names.":
        ("配置文件名称为空，或包含文件名中不允许的字符。", "プロファイル名が空か、ファイル名に使用できない文字が含まれています。"),
    "New Profile": ("新建配置文件", "新しいプロファイル"),
    "Profile %1 already exists. Overwrite it?": ("配置文件 %1 已存在。要覆盖吗？", "プロファイル %1 は既に存在します。上書きしますか？"),
    "New profile %1 created": ("已创建新配置文件 %1", "新しいプロファイル %1 を作成しました"),
    "Save Profile As": ("配置文件另存为", "名前を付けてプロファイルを保存"),
    "Profile saved as %1": ("配置文件已另存为 %1", "プロファイルを %1 として保存しました"),
    "Import Profile": ("导入配置文件", "プロファイルのインポート"),
    "Profile Files (*.ppx);;All Files (*.*)": ("配置文件 (*.ppx);;所有文件 (*.*)", "プロファイル (*.ppx);;すべてのファイル (*.*)"),
    "Cannot import %1:\n%2": ("无法导入 %1：\n%2", "%1 をインポートできません：\n%2"),
    "Profile %1 imported from %2": ("已从 %2 导入配置文件 %1", "プロファイル %1 を %2 からインポートしました"),
    "Export Profile": ("导出配置文件", "プロファイルのエクスポート"),
    "Profile Files (*.ppx)": ("配置文件 (*.ppx)", "プロファイル (*.ppx)"),
    "Profile exported to %1": ("配置文件已导出到 %1", "プロファイルを %1 にエクスポートしました"),
    "Fail status of all proxies was reset": ("已重置所有代理的失败状态", "すべてのプロキシの失敗状態をリセットしました"),
    "Chain %1": ("代理链 %1", "チェーン %1"),
    "Please add a proxy server first (Profile > Proxy Servers).": ("请先添加代理服务器（配置文件 > 代理服务器）。", "先にプロキシサーバーを追加してください（プロファイル > プロキシサーバー）。"),
    "About WinRoute": ("关于 WinRoute", "WinRoute について"),
    "<h3>WinRoute %1</h3><p>Routes the network connections of Windows applications through SOCKS and HTTPS proxy servers according to proxification rules.</p><p>Interception: %2<br>Qt %3</p>":
        ("<h3>WinRoute %1</h3><p>按照代理规则，让 Windows 应用程序的网络连接通过 SOCKS 和 HTTPS 代理服务器。</p><p>拦截方式：%2<br>Qt %3</p>",
         "<h3>WinRoute %1</h3><p>プロキシ化ルールに従って、Windows アプリケーションのネットワーク接続を SOCKS / HTTPS プロキシサーバー経由にします。</p><p>捕捉方式：%2<br>Qt %3</p>"),
    "End Process": ("结束进程", "プロセスの終了"),
    "Terminate the following process(es)?\n\n": ("要结束以下进程吗？\n\n", "次のプロセスを終了しますか？\n\n"),
    "Cannot terminate process %1: %2": ("无法结束进程 %1：%2", "プロセス %1 を終了できません：%2"),
    "Down %1, Up %2": ("下行 %1，上行 %2", "受信 %1、送信 %2"),
    "WinRoute - %1\n%2 active connections\nDown %3, Up %4": ("WinRoute - %1\n%2 个活动连接\n下行 %3，上行 %4", "WinRoute - %1\nアクティブな接続 %2 件\n受信 %3、送信 %4"),
    "Local DNS is available, host names are resolved locally": ("本地 DNS 可用，主机名在本地解析", "ローカル DNS を利用できます。ホスト名はローカルで解決されます"),
    "Local DNS is not available, host names are resolved through proxy": ("本地 DNS 不可用，主机名通过代理解析", "ローカル DNS を利用できません。ホスト名はプロキシ経由で解決されます"),
    "Cannot change autostart: ": ("无法更改开机自动启动：", "自動起動を変更できません："),
    "Autostart enabled": ("已启用开机自动启动", "自動起動を有効にしました"),
    "Autostart disabled": ("已禁用开机自动启动", "自動起動を無効にしました"),

    # ---- name resolution ----
    "WinRoute DNS settings": ("WinRoute DNS 设置", "WinRoute の DNS 設定"),
    "Detect DNS settings automatically": ("自动检测 DNS 设置", "DNS 設定を自動的に検出する"),
    "Resolve hostnames through proxy": ("通过代理解析主机名", "プロキシ経由でホスト名を解決する"),
    "Try to resolve via local DNS service first\n(may cause significant delays if local DNS is unavailable!)":
        ("先尝试使用本地 DNS 服务解析\n（本地 DNS 不可用时可能导致明显延迟！）", "最初にローカル DNS サービスで解決を試みる\n（ローカル DNS が利用できない場合は大きな遅延が発生することがあります！）"),
    "Note": ("注意", "注意"),
    "It is impossible to get real IP addresses through proxy.\nWinRoute assigns \"fake\" IP addresses (198.18.*.*) that are only valid when it is running.":
        ("通过代理无法获得真实的 IP 地址。\nWinRoute 会分配“虚拟”IP 地址（198.18.*.*），它们只在 WinRoute 运行时有效。", "プロキシ経由では実際の IP アドレスを取得できません。\nWinRoute は「仮想」IP アドレス（198.18.*.*）を割り当てます。これは WinRoute の実行中のみ有効です。"),
    "DNS name list": ("域名列表", "DNS 名リスト"),
    "Do NOT resolve the following": ("不解析以下域名", "次の名前は解決しない"),
    "Resolve ONLY the following": ("只解析以下域名", "次の名前のみ解決する"),
    "Example:\nlocalhost; *.example.com; %ComputerName%; %SimpleHostnames%":
        ("示例：\nlocalhost; *.example.com; %ComputerName%; %SimpleHostnames%", "例：\nlocalhost; *.example.com; %ComputerName%; %SimpleHostnames%"),

    # ---- profile ----
    "Direct": ("直接连接", "直接接続"),
    "Block": ("阻止", "ブロック"),
    "Proxy %1 %2": ("代理 %1 %2", "プロキシ %1 %2"),
    "Proxy <missing>": ("代理 <不存在>", "プロキシ <見つかりません>"),
    "Chain <missing>": ("代理链 <不存在>", "チェーン <見つかりません>"),
    "Not a Proxifier/WinRoute profile": ("不是 Proxifier/WinRoute 配置文件", "Proxifier/WinRoute のプロファイルではありません"),
    "XML error at line %1: %2": ("第 %1 行 XML 错误：%2", "%1 行目の XML エラー：%2"),
    "Password of proxy %1 is encrypted in a format WinRoute cannot read; please re-enter it.":
        ("代理 %1 的密码采用了 WinRoute 无法读取的加密格式，请重新输入。", "プロキシ %1 のパスワードは WinRoute で読み取れない形式で暗号化されています。再入力してください。"),

    # ---- proxy server dialog ----
    "Advanced Proxy Server Settings": ("代理服务器高级设置", "プロキシサーバーの詳細設定"),
    "Appearance": ("外观", "表示"),
    "(\"address:port\" by default)": ("（默认为“地址:端口”）", "（既定は「アドレス:ポート」）"),
    "Custom Label:": ("自定义标签：", "カスタムラベル："),
    "Other": ("其他", "その他"),
    "Use target hostname in proxy request if available": ("代理请求中尽量使用目标主机名", "可能な場合はプロキシ要求に接続先のホスト名を使用する"),
    "Proxy Server": ("代理服务器", "プロキシサーバー"),
    "Server": ("服务器", "サーバー"),
    "IP address or host name": ("IP 地址或主机名", "IP アドレスまたはホスト名"),
    "SOCKS Version 4": ("SOCKS 版本 4", "SOCKS バージョン 4"),
    "SOCKS Version 5": ("SOCKS 版本 5", "SOCKS バージョン 5"),
    "HTTPS": ("HTTPS", "HTTPS"),
    "HTTP": ("HTTP", "HTTP"),
    "Address:": ("地址：", "アドレス："),
    "Port:": ("端口：", "ポート："),
    "Protocol:": ("协议：", "プロトコル："),
    "Authentication": ("身份验证", "認証"),
    "Enable": ("启用", "有効"),
    "Use NTLM": ("使用 NTLM", "NTLM を使用"),
    "Use Kerberos": ("使用 Kerberos", "Kerberos を使用"),
    "Not supported": ("不支持", "未対応"),
    "Username:": ("用户名：", "ユーザー名："),
    "Password:": ("密码：", "パスワード："),
    "Options": ("选项", "オプション"),
    "Use SOCKS 4A extension (remote hostname resolving feature)": ("使用 SOCKS 4A 扩展（远程解析主机名）", "SOCKS 4A 拡張を使用する（リモートでのホスト名解決）"),
    "OK": ("确定", "OK"),
    "Check": ("检测", "チェック"),
    "Advanced...": ("高级...", "詳細設定..."),
    "Cancel": ("取消", "キャンセル"),
    "No options for SOCKS5 are available.": ("SOCKS5 没有可用选项。", "SOCKS5 には設定できるオプションがありません。"),
    "The proxy must support HTTP tunneling (CONNECT requests).": ("代理必须支持 HTTP 隧道（CONNECT 请求）。", "プロキシは HTTP トンネリング（CONNECT 要求）に対応している必要があります。"),
    "HTTP proxy servers can be used for HTTP connections only.": ("HTTP 代理服务器只能用于 HTTP 连接。", "HTTP プロキシサーバーは HTTP 接続にのみ使用できます。"),
    "Please enter the proxy server address.": ("请输入代理服务器地址。", "プロキシサーバーのアドレスを入力してください。"),

    # ---- proxy checker ----
    "Chain": ("代理链", "チェーン"),
    "Proxy server:": ("代理服务器：", "プロキシサーバー："),
    "Target host:": ("目标主机：", "接続先ホスト："),
    "Start Testing": ("开始测试", "テスト開始"),
    "Close": ("关闭", "閉じる"),
    "<b>Testing Started...</b>": ("<b>开始测试...</b>", "<b>テストを開始しました...</b>"),
    "Proxy Server%1<br>&nbsp;&nbsp;Address: %2:%3<br>&nbsp;&nbsp;Protocol: %4<br>&nbsp;&nbsp;Authentication: %5":
        ("代理服务器%1<br>&nbsp;&nbsp;地址：%2:%3<br>&nbsp;&nbsp;协议：%4<br>&nbsp;&nbsp;身份验证：%5",
         "プロキシサーバー%1<br>&nbsp;&nbsp;アドレス：%2:%3<br>&nbsp;&nbsp;プロトコル：%4<br>&nbsp;&nbsp;認証：%5"),
    "YES": ("是", "あり"),
    "NO": ("否", "なし"),
    "<b>Connection Test</b>": ("<b>连接测试</b>", "<b>接続テスト</b>"),
    "Connecting to %1:%2 ...": ("正在连接 %1:%2 ...", "%1:%2 に接続しています..."),
    "Connection established %1 (%2 ms)": ("连接已建立 %1（%2 毫秒）", "接続しました %1（%2 ミリ秒）"),
    "<b>Connection through the Proxy Server Test</b>": ("<b>通过代理服务器连接测试</b>", "<b>プロキシサーバー経由の接続テスト</b>"),
    "Connecting to %1:%2 through the proxy ...": ("正在通过代理连接 %1:%2 ...", "プロキシ経由で %1:%2 に接続しています..."),
    "Connection to %1 established through the proxy %2 (%3 ms)": ("已通过代理连接到 %1 %2（%3 毫秒）", "プロキシ経由で %1 に接続しました %2（%3 ミリ秒）"),
    "<b>TLS Handshake Test</b>": ("<b>TLS 握手测试</b>", "<b>TLS ハンドシェイクテスト</b>"),
    "TLS handshake %1 (%2 ms)": ("TLS 握手 %1（%2 毫秒）", "TLS ハンドシェイク %1（%2 ミリ秒）"),
    "TLS warning: %1": ("TLS 警告：%1", "TLS 警告：%1"),
    "%1 : TLS handshake timed out": ("%1：TLS 握手超时", "%1：TLS ハンドシェイクがタイムアウトしました"),
    "TLS backend unavailable; skipping the HTTPS request test (the tunnel is up).":
        ("没有可用的 TLS 后端，跳过 HTTPS 请求测试（隧道已建立）。", "TLS バックエンドが利用できないため、HTTPS 要求テストをスキップします（トンネルは確立済み）。"),
    "<b>HTTP Request Test</b>": ("<b>HTTP 请求测试</b>", "<b>HTTP 要求テスト</b>"),
    "Response: %1 %2": ("响应：%1 %2", "応答：%1 %2"),
    "%1 : connection closed without a response": ("%1：连接已关闭，没有收到响应", "%1：応答がないまま接続が閉じられました"),
    "%1 : no response within 10 seconds": ("%1：10 秒内没有响应", "%1：10 秒以内に応答がありません"),
    "UDP test skipped: a proxy chain cannot be tested for UDP.":
        ("跳过 UDP 测试：代理链无法测试 UDP。", "UDP テストをスキップ：プロキシチェーンは UDP をテストできません。"),
    "UDP test skipped: UDP applies to SOCKS5 proxies only.":
        ("跳过 UDP 测试：UDP 仅适用于 SOCKS5 代理。", "UDP テストをスキップ：UDP は SOCKS5 プロキシにのみ適用されます。"),
    "<b>UDP Support Test (SOCKS5 UDP ASSOCIATE)</b>":
        ("<b>UDP 支持测试（SOCKS5 UDP ASSOCIATE）</b>", "<b>UDP 対応テスト（SOCKS5 UDP ASSOCIATE）</b>"),
    "Requesting a UDP relay from %1:%2 ...": ("正在向 %1:%2 请求 UDP 中继 ...", "%1:%2 に UDP リレーを要求しています ..."),
    "not a SOCKS5 proxy": ("不是 SOCKS5 代理", "SOCKS5 プロキシではありません"),
    "proxy requires authentication": ("代理需要身份验证", "プロキシは認証が必要です"),
    "unsupported authentication method": ("不支持的身份验证方法", "サポートされていない認証方式です"),
    "authentication failed": ("身份验证失败", "認証に失敗しました"),
    "malformed reply": ("回复格式错误", "応答の形式が不正です"),
    "UDP ASSOCIATE rejected (code %1)": ("UDP ASSOCIATE 被拒绝（代码 %1）", "UDP ASSOCIATE が拒否されました（コード %1）"),
    "Relay granted at %1:%2": ("已分配中继 %1:%2", "リレーが %1:%2 に割り当てられました"),
    "cannot open a local UDP socket": ("无法打开本地 UDP 套接字", "ローカル UDP ソケットを開けません"),
    "Sending a DNS query to 8.8.8.8:53 through the relay ...":
        ("正在通过中继向 8.8.8.8:53 发送 DNS 查询 ...", "リレー経由で 8.8.8.8:53 に DNS クエリを送信しています ..."),
    "no reply from the relay within 5 seconds (UDP may be blocked)":
        ("5 秒内没有收到中继的回复（UDP 可能被阻止）", "5 秒以内にリレーから応答がありません（UDP がブロックされている可能性があります）"),
    "the proxy did not grant a UDP relay in time": ("代理未能及时分配 UDP 中继", "プロキシが UDP リレーを時間内に割り当てませんでした"),
    "UDP ASSOCIATE: %1 supported (a DNS reply came back through the relay)":
        ("UDP ASSOCIATE：%1 支持（已通过中继收到 DNS 回复）", "UDP ASSOCIATE：%1 対応（リレー経由で DNS 応答を受信）"),
    "UDP ASSOCIATE: %1 not available%2": ("UDP ASSOCIATE：%1 不可用%2", "UDP ASSOCIATE：%1 利用不可%2"),
    "<b>Testing Finished.</b> Proxy server is working correctly!": ("<b>测试完成。</b>代理服务器工作正常！", "<b>テスト完了。</b>プロキシサーバーは正常に動作しています！"),
    "<b>Testing Finished.</b> <span style='color:#c0201a'>The proxy server test failed.</span>":
        ("<b>测试完成。</b> <span style='color:#c0201a'>代理服务器测试失败。</span>", "<b>テスト完了。</b> <span style='color:#c0201a'>プロキシサーバーのテストに失敗しました。</span>"),

    # ---- proxy protocols ----
    "Could not connect to %1 - connection timeout": ("无法连接到 %1 - 连接超时", "%1 に接続できません - 接続タイムアウト"),
    "Could not connect to proxy %1 - connection timeout": ("无法连接到代理 %1 - 连接超时", "プロキシ %1 に接続できません - 接続タイムアウト"),
    "Could not connect through proxy %1 - proxy handshake timeout": ("无法通过代理 %1 连接 - 代理握手超时", "プロキシ %1 経由で接続できません - ハンドシェイクのタイムアウト"),
    "No target address": ("没有目标地址", "接続先アドレスがありません"),
    "No resolver available": ("没有可用的解析器", "利用できるリゾルバーがありません"),
    "Could not resolve %1 - %2": ("无法解析 %1 - %2", "%1 を解決できません - %2"),
    "Connection closed": ("连接已关闭", "接続が閉じられました"),
    "Connected to proxy %1": ("已连接到代理 %1", "プロキシ %1 に接続しました"),
    "Could not connect to %1 - %2": ("无法连接到 %1 - %2", "%1 に接続できません - %2"),
    "Could not connect to proxy %1 - %2": ("无法连接到代理 %1 - %2", "プロキシ %1 に接続できません - %2"),
    "Could not connect through proxy %1 - %2": ("无法通过代理 %1 连接 - %2", "プロキシ %1 経由で接続できません - %2"),
    "Connected to proxy %1 through the chain": ("已通过代理链连接到代理 %1", "チェーン経由でプロキシ %1 に接続しました"),
    "general SOCKS server failure": ("SOCKS 服务器一般性故障", "SOCKS サーバーの一般的な障害"),
    "connection not allowed by ruleset": ("规则集不允许此连接", "ルールセットにより接続が許可されていません"),
    "network unreachable": ("网络不可达", "ネットワークに到達できません"),
    "host unreachable": ("主机不可达", "ホストに到達できません"),
    "connection refused by the target": ("目标拒绝连接", "接続先が接続を拒否しました"),
    "TTL expired": ("TTL 已过期", "TTL の期限切れ"),
    "command not supported": ("不支持该命令", "コマンドはサポートされていません"),
    "address type not supported": ("不支持该地址类型", "アドレスの種類はサポートされていません"),
    "error code %1": ("错误代码 %1", "エラーコード %1"),
    "Could not connect through proxy %1 - not a SOCKS5 proxy": ("无法通过代理 %1 连接 - 不是 SOCKS5 代理", "プロキシ %1 経由で接続できません - SOCKS5 プロキシではありません"),
    "Could not connect through proxy %1 - proxy requires authentication": ("无法通过代理 %1 连接 - 代理需要身份验证", "プロキシ %1 経由で接続できません - プロキシは認証を必要としています"),
    "Could not connect through proxy %1 - unsupported authentication method %2": ("无法通过代理 %1 连接 - 不支持的身份验证方式 %2", "プロキシ %1 経由で接続できません - 未対応の認証方式 %2"),
    "Could not connect through proxy %1 - authentication failed (wrong username or password)":
        ("无法通过代理 %1 连接 - 身份验证失败（用户名或密码错误）", "プロキシ %1 経由で接続できません - 認証に失敗しました（ユーザー名またはパスワードが違います）"),
    "Could not connect through proxy %1 - malformed reply": ("无法通过代理 %1 连接 - 应答格式错误", "プロキシ %1 経由で接続できません - 不正な応答"),
    "Could not connect through proxy %1 - SOCKS4 does not support IPv6 targets": ("无法通过代理 %1 连接 - SOCKS4 不支持 IPv6 目标", "プロキシ %1 経由で接続できません - SOCKS4 は IPv6 の接続先に対応していません"),
    "identd authentication failed": ("identd 身份验证失败", "identd 認証に失敗しました"),
    "request rejected or failed": ("请求被拒绝或失败", "要求が拒否されたか失敗しました"),
    "Could not connect through proxy %1 - %2 (code %3)": ("无法通过代理 %1 连接 - %2（代码 %3）", "プロキシ %1 経由で接続できません - %2（コード %3）"),
    "Could not connect through proxy %1 - response header too large": ("无法通过代理 %1 连接 - 响应头过大", "プロキシ %1 経由で接続できません - 応答ヘッダーが大きすぎます"),
    "proxy requires authentication (HTTP 407)": ("代理需要身份验证（HTTP 407）", "プロキシは認証を必要としています（HTTP 407）"),
    "not an HTTP proxy": ("不是 HTTP 代理", "HTTP プロキシではありません"),
    "proxy returned HTTP %1": ("代理返回 HTTP %1", "プロキシが HTTP %1 を返しました"),

    # ---- proxy servers dialog ----
    "Address": ("地址", "アドレス"),
    "Port": ("端口", "ポート"),
    "Type": ("类型", "種類"),
    "Add...": ("添加...", "追加..."),
    "Edit...": ("编辑...", "編集..."),
    "Remove": ("删除", "削除"),
    "Check...": ("检测...", "チェック..."),
    "You can chain several proxy servers together:": ("可以将多个代理服务器串成代理链：", "複数のプロキシサーバーをチェーンとしてつなげられます："),
    "Proxy Chains...": ("代理链...", "プロキシチェーン..."),
    "Create": ("创建", "作成"),
    "Type...": ("类型...", "種類..."),
    "Drag and Drop proxy servers within a chain to alter the order.\nUse check boxes to enable/disable a proxy in a chain.":
        ("在代理链内拖放代理服务器可以调整顺序。\n用复选框启用/禁用链中的代理。", "チェーン内でプロキシサーバーをドラッグ＆ドロップすると順序を変更できます。\nチェックボックスでチェーン内のプロキシを有効/無効にします。"),
    "Proxy Chain": ("代理链", "プロキシチェーン"),
    "Chain name:": ("代理链名称：", "チェーン名："),
    "Drag and Drop a proxy server from the list to add it into a chain.": ("从列表中把代理服务器拖放到代理链中即可添加。", "リストからプロキシサーバーをドラッグ＆ドロップしてチェーンに追加します。"),
    " (Redundancy)": ("（冗余）", "（冗長化）"),
    " (Load Balancing)": ("（负载均衡）", "（負荷分散）"),
    "<missing proxy %1>": ("<不存在的代理 %1>", "<見つからないプロキシ %1>"),
    "Do you want to use this proxy by default?\n\nThe Default rule will be changed to use %1.\nYou can change this later in Proxification Rules.":
        ("要默认使用此代理吗？\n\nDefault 规则将改为使用 %1。\n之后可以在“代理规则”中修改。", "このプロキシを既定で使用しますか？\n\nDefault ルールが %1 を使用するように変更されます。\n後で「プロキシ化ルール」で変更できます。"),
    "Proxy Removal": ("删除代理", "プロキシの削除"),
    "Chain Removal": ("删除代理链", "チェーンの削除"),
    "%1 %2 is assigned as the Action for the following Rule(s):": ("%1 %2 被以下规则用作操作：", "%1 %2 は次のルールのアクションとして使用されています："),
    "Proxy": ("代理", "プロキシ"),
    "To proceed with the removal you can:": ("要继续删除，可以：", "削除を続行するには、次のいずれかを選択します："),
    "Remove the Rule(s)": ("删除这些规则", "ルールを削除する"),
    "Select a new Action:": ("选择新的操作：", "新しいアクションを選択："),
    "The Default rule is used when no other rule matches, so it cannot be removed.": ("Default 规则在其他规则都不匹配时使用，因此不能删除。", "Default ルールは他のルールに一致しない場合に使用されるため、削除できません。"),
    "Chain%1": ("代理链%1", "チェーン%1"),

    # ---- relay ----
    "%1 bytes": ("%1 字节", "%1 バイト"),
    "%1 bytes (%2)": ("%1 字节（%2）", "%1 バイト（%2）"),
    "no active profile": ("没有活动的配置文件", "有効なプロファイルがありません"),
    "proxy server #%1 used by rule %2 does not exist": ("规则 %2 使用的代理服务器 #%1 不存在", "ルール %2 で使用しているプロキシサーバー #%1 は存在しません"),
    "proxy chain #%1 used by rule %2 does not exist": ("规则 %2 使用的代理链 #%1 不存在", "ルール %2 で使用しているプロキシチェーン #%1 は存在しません"),
    "proxy chain %1 has no enabled proxy servers": ("代理链 %1 中没有启用的代理服务器", "プロキシチェーン %1 に有効なプロキシサーバーがありません"),
    "%1 - %2 matching %3 rule : using %4": ("%1 - %2 匹配规则 %3：使用 %4", "%1 - %2 ルール %3 に一致：%4 を使用"),
    "direct connection": ("直接连接", "直接接続"),
    "%1 - %2 %3, trying the next proxy in the chain": ("%1 - %2 %3，尝试链中的下一个代理", "%1 - %2 %3、チェーン内の次のプロキシを試します"),
    "%1 - %2 open directly": ("%1 - %2 直接连接", "%1 - %2 直接接続を開始"),
    "%1 - %2 open through proxy %3:%4 %5": ("%1 - %2 通过代理 %3:%4 %5 连接", "%1 - %2 プロキシ %3:%4 %5 経由で接続"),
    "%1 - %2 open through %3": ("%1 - %2 通过 %3 连接", "%1 - %2 %3 経由で接続"),
    "%1 - %2 error : %3": ("%1 - %2 错误：%3", "%1 - %2 エラー：%3"),
    "%1 - %2 close, %3 sent, %4 received, lifetime %5": ("%1 - %2 关闭，发送 %3，接收 %4，持续 %5", "%1 - %2 切断、送信 %3、受信 %4、接続時間 %5"),
    "%1 - %2 closed by the application before the connection was established": ("%1 - %2 在连接建立前被应用程序关闭", "%1 - %2 接続の確立前にアプリケーションが閉じました"),
    "Cannot bind the DNS resolver socket": ("无法绑定 DNS 解析器套接字", "DNS リゾルバーのソケットをバインドできません"),
    "Cannot start the local relay: ": ("无法启动本地中继：", "ローカルリレーを開始できません："),
    "IPv6 relay is not available: ": ("IPv6 中继不可用：", "IPv6 リレーを利用できません："),
    "Relay: unexpected connection from %1 dropped": ("中继：已丢弃来自 %1 的意外连接", "リレー：%1 からの想定外の接続を破棄しました"),

    # ---- rules ----
    "Proxification Rule": ("代理规则", "プロキシ化ルール"),
    "Enabled": ("启用", "有効"),
    "Name:": ("名称：", "名前："),
    "Applications": ("应用程序", "アプリケーション"),
    "Any": ("任意", "すべて"),
    "Browse...": ("浏览...", "参照..."),
    "Add Files...": ("添加文件...", "ファイルを追加..."),
    "Add Folder...": ("添加文件夹...", "フォルダーを追加..."),
    "Select a Folder": ("选择文件夹", "フォルダーの選択"),
    "Example: iexplore.exe; \"C:\\some app.exe\"; C:\\Games\\; fire*.exe": ("示例：iexplore.exe; \"C:\\some app.exe\"; C:\\Games\\; fire*.exe", "例：iexplore.exe; \"C:\\some app.exe\"; C:\\Games\\; fire*.exe"),
    "Target hosts": ("目标主机", "接続先ホスト"),
    "Example: 127.0.0.1; *.example.com; 192.168.1.*; 10.1.0.0-10.5.255.255": ("示例：127.0.0.1; *.example.com; 192.168.1.*; 10.1.0.0-10.5.255.255", "例：127.0.0.1; *.example.com; 192.168.1.*; 10.1.0.0-10.5.255.255"),
    "Target ports": ("目标端口", "接続先ポート"),
    "Example: 80; 8000-9000; 3128": ("示例：80; 8000-9000; 3128", "例：80; 8000-9000; 3128"),
    "Per-rule network interface selection is not supported yet": ("暂不支持为规则指定网络接口", "ルールごとのネットワークインターフェイスの指定にはまだ対応していません"),
    "Action:": ("操作：", "アクション："),
    "Select Applications": ("选择应用程序", "アプリケーションの選択"),
    "Applications (*.exe);;All files (*.*)": ("应用程序 (*.exe);;所有文件 (*.*)", "アプリケーション (*.exe);;すべてのファイル (*.*)"),
    "Please enter the rule name.": ("请输入规则名称。", "ルール名を入力してください。"),
    "Target ports must look like: 80; 8000-9000; 3128": ("目标端口的格式应为：80; 8000-9000; 3128", "接続先ポートは次の形式で入力してください：80; 8000-9000; 3128"),
    "Name": ("名称", "名前"),
    "Target Hosts": ("目标主机", "接続先ホスト"),
    "Target Ports": ("目标端口", "接続先ポート"),
    "Action": ("操作", "アクション"),
    "Move up": ("上移", "上へ移動"),
    "Move down": ("下移", "下へ移動"),
    "Clone": ("复制", "複製"),
    "Rules are processed from top to bottom. Use the arrows at the right to change the order.\n\nUse check boxes to enable/disable a rule.":
        ("规则按从上到下的顺序处理。用右侧的箭头调整顺序。\n\n用复选框启用/禁用规则。", "ルールは上から順に処理されます。右側の矢印で順序を変更できます。\n\nチェックボックスでルールを有効/無効にします。"),
    "New": ("新规则", "新規"),
    " (copy)": ("（副本）", "（コピー）"),
    "The Default rule is used when all other rules do not match the connection parameters, so it cannot be removed.":
        ("Default 规则在所有其他规则都不匹配连接参数时使用，因此不能删除。", "Default ルールは他のすべてのルールが接続に一致しない場合に使用されるため、削除できません。"),

    # ---- statistics / traffic ----
    "Application / Target": ("应用程序 / 目标", "アプリケーション / 接続先"),
    "Active": ("活动", "アクティブ"),
    "Last Activity": ("最近活动", "最終アクティビティ"),
    "&Reset Statistics": ("重置统计(&R)", "統計のリセット(&R)"),
    "-%1 min": ("-%1 分钟", "-%1 分"),
    "Show Both": ("显示全部", "両方を表示"),
    "Incoming Only": ("仅接收", "受信のみ"),
    "Outgoing Only": ("仅发送", "送信のみ"),
    "Select the kind of the traffic to display": ("选择要显示的流量类型", "表示するトラフィックの種類を選択します"),
    "Clear": ("清除", "クリア"),
    "Incoming: %1 (max %2)": ("接收：%1（最大 %2）", "受信：%1（最大 %2）"),
    "Outgoing: %1 (max %2)": ("发送：%1（最大 %2）", "送信：%1（最大 %2）"),
    "Total: %1 received, %2 sent": ("合计：接收 %1，发送 %2", "合計：受信 %1、送信 %2"),
    "%1 B/sec": ("%1 B/秒", "%1 B/秒"),
    "%1 KB/sec": ("%1 KB/秒", "%1 KB/秒"),
    "%1 MB/sec": ("%1 MB/秒", "%1 MB/秒"),

    # ---- UDP over SOCKS5 ----
    "%1 (UDP)": ("%1 (UDP)", "%1 (UDP)"),
    "Route &UDP through SOCKS5": ("通过 SOCKS5 转发 UDP(&U)", "UDP を SOCKS5 で転送(&U)"),
    "Tunnel UDP through the proxy when a rule selects a SOCKS5 proxy (otherwise UDP goes directly)":
        ("当规则命中 SOCKS5 代理时，通过代理转发 UDP（否则 UDP 直接连接）",
         "ルールが SOCKS5 プロキシを選択したとき、UDP をプロキシ経由で転送します（それ以外は UDP は直接接続）"),
    "Cannot update UDP filtering: %1": ("无法更新 UDP 过滤：%1", "UDP フィルタリングを更新できません：%1"),
    "%1 - %2 matching %3 rule : blocked (UDP)": ("%1 - %2 匹配规则 %3：已阻止（UDP）", "%1 - %2 がルール %3 に一致：ブロック（UDP）"),
    "%1 - %2 matching %3 rule : %4 does not support UDP, sending directly":
        ("%1 - %2 匹配规则 %3：%4 不支持 UDP，直接连接", "%1 - %2 がルール %3 に一致：%4 は UDP に対応していないため直接接続します"),
    "%1 - %2 matching %3 rule : proxy chains do not support UDP, sending directly":
        ("%1 - %2 匹配规则 %3：代理链不支持 UDP，直接连接", "%1 - %2 がルール %3 に一致：プロキシチェーンは UDP に対応していないため直接接続します"),
    "%1 - UDP proxy #%2 used by rule %3 does not exist":
        ("%1 - 规则 %3 使用的 UDP 代理 #%2 不存在", "%1 - ルール %3 が使用する UDP プロキシ #%2 は存在しません"),
    "Cannot delete %1: the driver is still loaded (close WinRoute, or restart Windows)":
        ("无法删除 %1：驱动仍在内核中（请关闭 WinRoute，或重启 Windows）", "%1 を削除できません：ドライバーがまだ読み込まれています（WinRoute を終了するか、Windows を再起動してください）"),
    "the network driver API is unavailable": ("网络驱动接口不可用", "ネットワークドライバー API を利用できません"),
    "no resolver available": ("解析器不可用", "リゾルバーを利用できません"),
    "could not resolve proxy %1 - %2": ("无法解析代理 %1 - %2", "プロキシ %1 を解決できませんでした - %2"),
    "not a SOCKS5 proxy": ("不是 SOCKS5 代理", "SOCKS5 プロキシではありません"),
    "SOCKS5 authentication method not accepted": ("SOCKS5 认证方式未被接受", "SOCKS5 認証方式が受け入れられませんでした"),
    "SOCKS5 authentication failed": ("SOCKS5 认证失败", "SOCKS5 認証に失敗しました"),
    "malformed SOCKS5 reply": ("SOCKS5 应答格式错误", "SOCKS5 応答の形式が不正です"),
    "the proxy does not support UDP (SOCKS5 reply %1)": ("代理不支持 UDP（SOCKS5 应答 %1）", "プロキシは UDP に対応していません（SOCKS5 応答 %1）"),
    "cannot open the local UDP socket": ("无法打开本地 UDP 套接字", "ローカル UDP ソケットを開けません"),
    "the proxy closed the control connection": ("代理关闭了控制连接", "プロキシが制御接続を閉じました"),
    "%1 - %2 open through proxy %3:%4 SOCKS5 (UDP)": ("%1 - %2 通过代理 %3:%4 SOCKS5 连接（UDP）", "%1 - %2 をプロキシ %3:%4 SOCKS5 経由で接続（UDP）"),
    "%1 - %2 UDP cannot use the proxy (%3); sending directly":
        ("%1 - %2 UDP 无法使用代理（%3），改为直接连接", "%1 - %2 UDP はプロキシを使用できません（%3）。直接接続します"),
    "%1 - %2 UDP closed, %3 sent, %4 received": ("%1 - %2 UDP 关闭，发送 %3，接收 %4", "%1 - %2 UDP 切断、送信 %3、受信 %4"),

    # ---- network driver ----
    "NetFilter SDK demo driver": ("NetFilter SDK 演示驱动", "NetFilter SDK デモドライバー"),
    "Driver %1": ("驱动 %1", "ドライバー %1"),
    "&Network Driver...": ("网络驱动(&N)...", "ネットワークドライバー(&N)..."),
    "Install, switch or uninstall the network driver": ("安装、切换或卸载网络驱动", "ネットワークドライバーのインストール・切り替え・アンインストール"),
    "Network Driver": ("网络驱动", "ネットワークドライバー"),
    "WinRoute captures connections with a NetFilter SDK driver. Choose which driver to use, or uninstall it.":
        ("WinRoute 通过 NetFilter SDK 驱动拦截网络连接。请选择要使用的驱动，或卸载它。",
         "WinRoute は NetFilter SDK のドライバーで接続を捕捉します。使用するドライバーを選択するか、アンインストールしてください。"),
    "Driver:": ("驱动：", "ドライバー："),
    "Use This Driver": ("使用此驱动", "このドライバーを使用"),
    "Uninstall Driver": ("卸载驱动", "ドライバーをアンインストール"),
    "an unrecognized driver": ("无法识别的驱动", "認識できないドライバー"),
    "Installed: %1 (running)": ("已安装：%1（运行中）", "インストール済み：%1（実行中）"),
    "Installed: %1 (stopped)": ("已安装：%1（已停止）", "インストール済み：%1（停止中）"),
    "No driver is installed.": ("未安装驱动。", "ドライバーがインストールされていません。"),
    "The evaluation driver from the NetFilter SDK. It stops capturing connections after a limited number of them, until Windows restarts.":
        ("NetFilter SDK 自带的评估版驱动。拦截的连接数达到上限后会停止工作，重启 Windows 后恢复。",
         "NetFilter SDK に付属する評価版ドライバーです。捕捉した接続数が上限に達すると、Windows を再起動するまで動作しません。"),
    "A release build of the driver, without a connection limit. In rare cases this driver may need a Windows restart before it can be switched or removed.":
        ("正式版驱动，没有连接数限制。极少数情况下，切换或卸载此驱动前可能需要重启 Windows。",
         "製品版ドライバーで、接続数の制限はありません。まれに、このドライバーの切り替えや削除の前に Windows の再起動が必要になることがあります。"),
    "A release build of the driver, without a connection limit.": ("正式版驱动，没有连接数限制。", "製品版ドライバーで、接続数の制限はありません。"),
    "Switch to This Driver": ("切换到此驱动", "このドライバーに切り替え"),
    "Install This Driver": ("安装此驱动", "このドライバーをインストール"),
    "Network driver switched to %1": ("网络驱动已切换到 %1", "ネットワークドライバーを %1 に切り替えました"),
    "Could not switch the network driver: %1": ("无法切换网络驱动：%1", "ネットワークドライバーを切り替えできませんでした：%1"),
    "Could not switch to %1:\n%2": ("无法切换到 %1：\n%2", "%1 に切り替えできませんでした：\n%2"),
    "Uninstall the network driver? WinRoute cannot capture connections without it.":
        ("卸载网络驱动？没有它 WinRoute 无法拦截连接。", "ネットワークドライバーをアンインストールしますか？これがないと WinRoute は接続を捕捉できません。"),
    "Network driver uninstalled": ("网络驱动已卸载", "ネットワークドライバーをアンインストールしました"),
    "Could not uninstall the network driver: %1": ("无法卸载网络驱动：%1", "ネットワークドライバーをアンインストールできませんでした：%1"),
    "Could not uninstall the driver:\n%1": ("无法卸载驱动：\n%1", "ドライバーをアンインストールできませんでした：\n%1"),

    # ---- updates ----
    "Check for &Updates...": ("检查更新(&U)...", "更新の確認(&U)..."),
    "Automatic U&pdates": ("自动更新(&P)", "自動更新(&P)"),
    "Install All Updates Automatically": ("自动安装所有更新", "すべての更新を自動でインストール"),
    "Install Required Updates Automatically": ("自动安装必须的更新", "必須の更新を自動でインストール"),
    "Ask Before Updating": ("更新前询问", "更新前に確認"),
    "Never Check": ("从不检查", "確認しない"),
    "WinRoute Update": ("WinRoute 更新", "WinRoute の更新"),
    "Could not check for updates:\n%1": ("无法检查更新：\n%1", "更新を確認できませんでした：\n%1"),
    "WinRoute %1 is the latest version.": ("WinRoute %1 已是最新版本。", "WinRoute %1 は最新バージョンです。"),
    "WinRoute %1 is available; you have %2.\nUpdate now? WinRoute restarts when it is done.":
        ("WinRoute %1 已发布，当前版本是 %2。\n现在更新吗？更新完成后 WinRoute 会重新启动。",
         "WinRoute %1 が利用できます（現在のバージョンは %2）。\n今すぐ更新しますか？完了後に WinRoute が再起動します。"),
    "Update": ("更新", "更新"),
    "Later": ("以后再说", "後で"),
    "Downloading WinRoute %1...": ("正在下载 WinRoute %1...", "WinRoute %1 をダウンロードしています..."),
    "Downloading WinRoute %1; WinRoute restarts once it is installed.":
        ("正在下载 WinRoute %1，安装完成后 WinRoute 会重新启动。", "WinRoute %1 をダウンロードしています。インストール後に WinRoute が再起動します。"),
    "The download is damaged (checksum mismatch).": ("下载的文件已损坏（校验和不一致）。", "ダウンロードしたファイルが壊れています（チェックサムが一致しません）。"),
    "Could not download WinRoute %1:\n%2": ("无法下载 WinRoute %1：\n%2", "WinRoute %1 をダウンロードできませんでした：\n%2"),
    "Could not prepare the update.": ("无法准备更新。", "更新を準備できませんでした。"),
    "Could not start the update.": ("无法开始更新。", "更新を開始できませんでした。"),
}


def fill(ts_path, index):
    import re
    import xml.etree.ElementTree as ET
    tree = ET.parse(ts_path)
    missing, placeholder_errors = [], []
    for msg in tree.iter('message'):
        src = msg.find('source').text or ''
        tr = msg.find('translation')
        if src not in T:
            missing.append(src)
            continue
        text = T[src][index]
        if sorted(re.findall(r'%\d', src)) != sorted(re.findall(r'%\d', text)):
            placeholder_errors.append(src)
        tr.text = text
        tr.attrib.pop('type', None)
    ET.indent(tree)
    tree.write(ts_path, encoding='utf-8', xml_declaration=True)
    return missing, placeholder_errors


if __name__ == '__main__':
    import os
    here = os.path.dirname(os.path.abspath(__file__))
    ok = True
    for name, idx in (('winroute_zh_CN.ts', 0), ('winroute_ja.ts', 1)):
        missing, bad = fill(os.path.join(here, name), idx)
        print(f'{name}: {len(missing)} untranslated, {len(bad)} placeholder mismatches')
        for s in missing:
            print('  missing:', repr(s))
        for s in bad:
            print('  placeholders:', repr(s))
        ok = ok and not missing and not bad
    raise SystemExit(0 if ok else 1)
