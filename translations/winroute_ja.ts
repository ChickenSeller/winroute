<?xml version='1.0' encoding='utf-8'?>
<TS version="2.1" language="ja">
  <context>
    <name>ChainTypeDialog</name>
    <message>
      <source>Chain Type</source>
      <translation>チェーンの種類</translation>
    </message>
    <message>
      <source>Simple Chain</source>
      <translation>シンプルチェーン</translation>
    </message>
    <message>
      <source>Connection to a remote host will be performed sequentially from one proxy server to another.</source>
      <translation>チェーン内のプロキシサーバーを順番に経由してリモートホストに接続します。</translation>
    </message>
    <message>
      <source>Redundancy</source>
      <translation>冗長化</translation>
    </message>
    <message>
      <source>If the first proxy fails, the next one will be used, etc.</source>
      <translation>最初のプロキシが失敗した場合は次のプロキシを使用します。</translation>
    </message>
    <message>
      <source>Options...</source>
      <translation>オプション...</translation>
    </message>
    <message>
      <source>Load Balancing</source>
      <translation>負荷分散</translation>
    </message>
    <message>
      <source>Each connection will be made through a random proxy from the list.</source>
      <translation>接続ごとにリストからランダムにプロキシを選択します。</translation>
    </message>
    <message>
      <source>Use the same proxy for the same process PID</source>
      <translation>同じプロセス PID には同じプロキシを使用する</translation>
    </message>
  </context>
  <context>
    <name>ConnectionsModel</name>
    <message>
      <source>Closed</source>
      <translation>切断済み</translation>
    </message>
    <message>
      <source>Connecting</source>
      <translation>接続中</translation>
    </message>
    <message>
      <source>Error : %1</source>
      <translation>エラー：%1</translation>
    </message>
    <message>
      <source>%1 (pid %2)</source>
      <translation>%1（PID %2）</translation>
    </message>
    <message>
      <source>%1 (UDP)</source>
      <translation>%1 (UDP)</translation>
    </message>
    <message>
      <source>Application</source>
      <translation>アプリケーション</translation>
    </message>
    <message>
      <source>Target</source>
      <translation>接続先</translation>
    </message>
    <message>
      <source>Time/Status</source>
      <translation>時間/状態</translation>
    </message>
    <message>
      <source>Rule : Proxy</source>
      <translation>ルール : プロキシ</translation>
    </message>
    <message>
      <source>Sent</source>
      <translation>送信</translation>
    </message>
    <message>
      <source>Received</source>
      <translation>受信</translation>
    </message>
  </context>
  <context>
    <name>DnsClient</name>
    <message>
      <source>resolver is not available</source>
      <translation>リゾルバーを利用できません</translation>
    </message>
    <message>
      <source>no DNS servers configured</source>
      <translation>DNS サーバーが設定されていません</translation>
    </message>
    <message>
      <source>host not found</source>
      <translation>ホストが見つかりません</translation>
    </message>
    <message>
      <source>DNS error %1</source>
      <translation>DNS エラー %1</translation>
    </message>
    <message>
      <source>DNS request timed out</source>
      <translation>DNS 要求がタイムアウトしました</translation>
    </message>
  </context>
  <context>
    <name>DriverDialog</name>
    <message>
      <source>Network Driver</source>
      <translation>ネットワークドライバー</translation>
    </message>
    <message>
      <source>WinRoute captures connections with a NetFilter SDK driver. Choose which driver to use, or uninstall it.</source>
      <translation>WinRoute は NetFilter SDK のドライバーで接続を捕捉します。使用するドライバーを選択するか、アンインストールしてください。</translation>
    </message>
    <message>
      <source>Driver:</source>
      <translation>ドライバー：</translation>
    </message>
    <message>
      <source>Use This Driver</source>
      <translation>このドライバーを使用</translation>
    </message>
    <message>
      <source>Uninstall Driver</source>
      <translation>ドライバーをアンインストール</translation>
    </message>
    <message>
      <source>an unrecognized driver</source>
      <translation>認識できないドライバー</translation>
    </message>
    <message>
      <source>Installed: %1 (running)</source>
      <translation>インストール済み：%1（実行中）</translation>
    </message>
    <message>
      <source>Installed: %1 (stopped)</source>
      <translation>インストール済み：%1（停止中）</translation>
    </message>
    <message>
      <source>No driver is installed.</source>
      <translation>ドライバーがインストールされていません。</translation>
    </message>
    <message>
      <source>The evaluation driver from the NetFilter SDK. It stops capturing connections after a limited number of them, until Windows restarts.</source>
      <translation>NetFilter SDK に付属する評価版ドライバーです。捕捉した接続数が上限に達すると、Windows を再起動するまで動作しません。</translation>
    </message>
    <message>
      <source>A release build of the driver, without a connection limit. In rare cases this driver may need a Windows restart before it can be switched or removed.</source>
      <translation>製品版ドライバーで、接続数の制限はありません。まれに、このドライバーの切り替えや削除の前に Windows の再起動が必要になることがあります。</translation>
    </message>
    <message>
      <source>A release build of the driver, without a connection limit.</source>
      <translation>製品版ドライバーで、接続数の制限はありません。</translation>
    </message>
    <message>
      <source>Switch to This Driver</source>
      <translation>このドライバーに切り替え</translation>
    </message>
    <message>
      <source>Install This Driver</source>
      <translation>このドライバーをインストール</translation>
    </message>
    <message>
      <source>Network driver switched to %1</source>
      <translation>ネットワークドライバーを %1 に切り替えました</translation>
    </message>
    <message>
      <source>Could not switch the network driver: %1</source>
      <translation>ネットワークドライバーを切り替えできませんでした：%1</translation>
    </message>
    <message>
      <source>Could not switch to %1:
%2</source>
      <translation>%1 に切り替えできませんでした：
%2</translation>
    </message>
    <message>
      <source>Uninstall the network driver? WinRoute cannot capture connections without it.</source>
      <translation>ネットワークドライバーをアンインストールしますか？これがないと WinRoute は接続を捕捉できません。</translation>
    </message>
    <message>
      <source>Network driver uninstalled</source>
      <translation>ネットワークドライバーをアンインストールしました</translation>
    </message>
    <message>
      <source>Could not uninstall the network driver: %1</source>
      <translation>ネットワークドライバーをアンインストールできませんでした：%1</translation>
    </message>
    <message>
      <source>Could not uninstall the driver:
%1</source>
      <translation>ドライバーをアンインストールできませんでした：
%1</translation>
    </message>
  </context>
  <context>
    <name>EncryptionDialog</name>
    <message>
      <source>Profile Passwords Encryption</source>
      <translation>プロファイルのパスワード暗号化</translation>
    </message>
    <message>
      <source>To provide additional security WinRoute can store proxy passwords in the profile file in encrypted form.</source>
      <translation>セキュリティを高めるため、WinRoute はプロキシのパスワードを暗号化してプロファイルに保存できます。</translation>
    </message>
    <message>
      <source>Please choose one of the following modes:</source>
      <translation>次のいずれかのモードを選択してください：</translation>
    </message>
    <message>
      <source>Disabled</source>
      <translation>無効</translation>
    </message>
    <message>
      <source>Passwords are saved in plain text. Proxy passwords can be used by everyone who has access to the profile file.</source>
      <translation>パスワードは平文で保存されます。プロファイルにアクセスできる人は誰でもプロキシのパスワードを使用できます。</translation>
    </message>
    <message>
      <source>Basic</source>
      <translation>基本</translation>
    </message>
    <message>
      <source>Encryption with a WinRoute static key. Proxy passwords can be used by everyone who opens the profile with WinRoute.</source>
      <translation>WinRoute の固定キーで暗号化します。WinRoute でプロファイルを開く人は誰でもプロキシのパスワードを使用できます。</translation>
    </message>
    <message>
      <source>Current user account</source>
      <translation>現在のユーザーアカウント</translation>
    </message>
    <message>
      <source>Encryption using current user login credentials. Only current user can access proxy passwords.</source>
      <translation>現在のユーザーのログオン資格情報で暗号化します。現在のユーザーだけがプロキシのパスワードにアクセスできます。</translation>
    </message>
    <message>
      <source>Profile master password</source>
      <translation>プロファイルのマスターパスワード</translation>
    </message>
    <message>
      <source>Encryption with a profile master password (not supported).</source>
      <translation>プロファイルのマスターパスワードで暗号化します（未対応）。</translation>
    </message>
  </context>
  <context>
    <name>Engine</name>
    <message>
      <source>Local relay is listening on port %1</source>
      <translation>ローカルリレーはポート %1 で待ち受けています</translation>
    </message>
  </context>
  <context>
    <name>HttpProxiesDialog</name>
    <message>
      <source>HTTP Proxy Servers Support</source>
      <translation>HTTP プロキシサーバーのサポート</translation>
    </message>
    <message>
      <source>HTTP Proxy Servers</source>
      <translation>HTTP プロキシサーバー</translation>
    </message>
    <message>
      <source>Enable HTTP proxy servers support</source>
      <translation>HTTP プロキシサーバーのサポートを有効にする</translation>
    </message>
    <message>
      <source>When this option is enabled you can add HTTP proxy servers at the Proxy Servers dialog.</source>
      <translation>このオプションを有効にすると、「プロキシサーバー」ダイアログで HTTP プロキシサーバーを追加できます。</translation>
    </message>
    <message>
      <source>WARNING</source>
      <translation>警告</translation>
    </message>
    <message>
      <source>HTTP proxy server can only be used for HTTP connections (web browsers and etc). WinRoute talks to HTTP proxies with CONNECT requests.</source>
      <translation>HTTP プロキシサーバーは HTTP 接続（Web ブラウザーなど）にのみ使用できます。WinRoute は CONNECT 要求で HTTP プロキシと通信します。</translation>
    </message>
    <message>
      <source>Not to be confused with HTTPS connections and HTTPS proxy servers!</source>
      <translation>HTTPS 接続や HTTPS プロキシサーバーと混同しないでください！</translation>
    </message>
  </context>
  <context>
    <name>Interceptor</name>
    <message>
      <source>Cannot load %1 (error %2)</source>
      <translation>%1 を読み込めません（エラー %2）</translation>
    </message>
    <message>
      <source>nfapi.dll is missing exports: </source>
      <translation>nfapi.dll にエクスポートがありません：</translation>
    </message>
    <message>
      <source> (the C API build is required)</source>
      <translation>（C API 版が必要です）</translation>
    </message>
    <message>
      <source>success</source>
      <translation>成功</translation>
    </message>
    <message>
      <source>failed to open the driver</source>
      <translation>ドライバーを開けません</translation>
    </message>
    <message>
      <source>invalid endpoint id</source>
      <translation>無効なエンドポイント ID</translation>
    </message>
    <message>
      <source>not initialized</source>
      <translation>初期化されていません</translation>
    </message>
    <message>
      <source>driver I/O error</source>
      <translation>ドライバーの I/O エラー</translation>
    </message>
    <message>
      <source>reboot required</source>
      <translation>再起動が必要です</translation>
    </message>
    <message>
      <source>status %1</source>
      <translation>状態 %1</translation>
    </message>
    <message>
      <source>Windows DNS cache flushed</source>
      <translation>Windows の DNS キャッシュを消去しました</translation>
    </message>
    <message>
      <source>Cannot open the service manager (error %1)</source>
      <translation>サービスマネージャーを開けません（エラー %1）</translation>
    </message>
    <message>
      <source>Cannot open the driver service (error %1)</source>
      <translation>ドライバーのサービスを開けません（エラー %1）</translation>
    </message>
    <message>
      <source>Cannot delete the driver service (error %1)</source>
      <translation>ドライバーのサービスを削除できません（エラー %1）</translation>
    </message>
    <message>
      <source>The driver could not be stopped; it will be removed after a reboot</source>
      <translation>ドライバーを停止できませんでした。再起動後に削除されます</translation>
    </message>
    <message>
      <source>Driver file %1 was not found</source>
      <translation>ドライバーファイル %1 が見つかりません</translation>
    </message>
    <message>
      <source>Cannot copy the driver to %1 (error %2)</source>
      <translation>ドライバーを %1 にコピーできません（エラー %2）</translation>
    </message>
    <message>
      <source>Cannot register the driver: %1</source>
      <translation>ドライバーを登録できません：%1</translation>
    </message>
    <message>
      <source>Network driver %1 installed</source>
      <translation>ネットワークドライバー %1 をインストールしました</translation>
    </message>
    <message>
      <source>Cannot delete %1 (error %2)</source>
      <translation>%1 を削除できません（エラー %2）</translation>
    </message>
    <message>
      <source>Cannot delete %1: the driver is still loaded (close WinRoute, or restart Windows)</source>
      <translation>%1 を削除できません：ドライバーがまだ読み込まれています（WinRoute を終了するか、Windows を再起動してください）</translation>
    </message>
    <message>
      <source>NetFilter SDK demo driver</source>
      <translation>NetFilter SDK デモドライバー</translation>
    </message>
    <message>
      <source>Driver %1</source>
      <translation>ドライバー %1</translation>
    </message>
    <message>
      <source>Cannot add driver rule (%1): %2</source>
      <translation>ドライバーのルールを追加できません（%1）：%2</translation>
    </message>
    <message>
      <source>Administrator rights are required to control the network driver</source>
      <translation>ネットワークドライバーの制御には管理者権限が必要です</translation>
    </message>
    <message>
      <source>Cannot connect to the network driver %1: %2</source>
      <translation>ネットワークドライバー %1 に接続できません：%2</translation>
    </message>
    <message>
      <source>Cannot update UDP filtering: %1</source>
      <translation>UDP フィルタリングを更新できません：%1</translation>
    </message>
    <message>
      <source>%1 - %2 error : unknown fake DNS address, the name was resolved before WinRoute started</source>
      <translation>%1 - %2 エラー：不明な仮想 DNS アドレスです。この名前は WinRoute の起動前に解決されました</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : blocked</source>
      <translation>%1 - %2 ルール %3 に一致：ブロック</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : direct connection</source>
      <translation>%1 - %2 ルール %3 に一致：直接接続</translation>
    </message>
    <message>
      <source>%1 - %2 is a proxy server; connecting directly to prevent a connection loop</source>
      <translation>%1 - %2 はプロキシサーバーです。接続ループを防ぐため直接接続します</translation>
    </message>
    <message>
      <source>%1 - %2 error : IPv6 relay is not available, connecting directly</source>
      <translation>%1 - %2 エラー：IPv6 リレーを利用できないため直接接続します</translation>
    </message>
    <message>
      <source>DNS %1 resolved through proxy as %2</source>
      <translation>DNS %1 をプロキシ経由で %2 として解決しました</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : blocked (UDP)</source>
      <translation>%1 - %2 がルール %3 に一致：ブロック（UDP）</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : %4 does not support UDP, sending directly</source>
      <translation>%1 - %2 がルール %3 に一致：%4 は UDP に対応していないため直接接続します</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : proxy chains do not support UDP, sending directly</source>
      <translation>%1 - %2 がルール %3 に一致：プロキシチェーンは UDP に対応していないため直接接続します</translation>
    </message>
  </context>
  <context>
    <name>Language</name>
    <message>
      <source>System Default</source>
      <translation>システムの既定</translation>
    </message>
  </context>
  <context>
    <name>MainWindow</name>
    <message>
      <source>Welcome to WinRoute v%1</source>
      <translation>WinRoute v%1 へようこそ</translation>
    </message>
    <message>
      <source>%1 active connections</source>
      <translation>アクティブな接続 %1 件</translation>
    </message>
    <message>
      <source>&amp;New Profile</source>
      <translation>新しいプロファイル(&amp;N)</translation>
    </message>
    <message>
      <source>&amp;Save Profile As...</source>
      <translation>名前を付けてプロファイルを保存(&amp;S)...</translation>
    </message>
    <message>
      <source>&amp;Manage Profiles...</source>
      <translation>プロファイルの管理(&amp;M)...</translation>
    </message>
    <message>
      <source>&amp;Import Profile...</source>
      <translation>プロファイルのインポート(&amp;I)...</translation>
    </message>
    <message>
      <source>&amp;Export Profile...</source>
      <translation>プロファイルのエクスポート(&amp;E)...</translation>
    </message>
    <message>
      <source>&amp;Autostart</source>
      <translation>自動起動(&amp;A)</translation>
    </message>
    <message>
      <source>Start WinRoute automatically when Windows starts for the current user</source>
      <translation>現在のユーザーが Windows にサインインしたときに WinRoute を自動的に起動します</translation>
    </message>
    <message>
      <source>&amp;Close WinRoute Window</source>
      <translation>WinRoute ウィンドウを閉じる(&amp;C)</translation>
    </message>
    <message>
      <source>E&amp;xit</source>
      <translation>終了(&amp;X)</translation>
    </message>
    <message>
      <source>&amp;Proxy Servers...</source>
      <translation>プロキシサーバー(&amp;P)...</translation>
    </message>
    <message>
      <source>Configure proxy servers settings</source>
      <translation>プロキシサーバーを設定します</translation>
    </message>
    <message>
      <source>Proxification &amp;Rules...</source>
      <translation>プロキシ化ルール(&amp;R)...</translation>
    </message>
    <message>
      <source>Edit proxification rules</source>
      <translation>プロキシ化ルールを編集します</translation>
    </message>
    <message>
      <source>&amp;Name Resolution...</source>
      <translation>名前解決(&amp;N)...</translation>
    </message>
    <message>
      <source>Change the way WinRoute handles host name resolution</source>
      <translation>WinRoute のホスト名解決の方法を変更します</translation>
    </message>
    <message>
      <source>&amp;HTTP Proxy Servers...</source>
      <translation>HTTP プロキシサーバー(&amp;H)...</translation>
    </message>
    <message>
      <source>&amp;Services and Other Users...</source>
      <translation>サービスと他のユーザー(&amp;S)...</translation>
    </message>
    <message>
      <source>Handle &amp;Direct Connections</source>
      <translation>直接接続を処理する(&amp;D)</translation>
    </message>
    <message>
      <source>Display and handle connections that are established directly</source>
      <translation>直接確立される接続も表示して処理します</translation>
    </message>
    <message>
      <source>Connection &amp;Loop Detection</source>
      <translation>接続ループの検出(&amp;L)</translation>
    </message>
    <message>
      <source>Connect directly to proxy servers to prevent infinite connection loops</source>
      <translation>無限の接続ループを防ぐため、プロキシサーバーには直接接続します</translation>
    </message>
    <message>
      <source>Route &amp;UDP through SOCKS5</source>
      <translation>UDP を SOCKS5 で転送(&amp;U)</translation>
    </message>
    <message>
      <source>Tunnel UDP through the proxy when a rule selects a SOCKS5 proxy (otherwise UDP goes directly)</source>
      <translation>ルールが SOCKS5 プロキシを選択したとき、UDP をプロキシ経由で転送します（それ以外は UDP は直接接続）</translation>
    </message>
    <message>
      <source>&amp;Passwords Encryption...</source>
      <translation>パスワードの暗号化(&amp;P)...</translation>
    </message>
    <message>
      <source>&amp;Disconnect</source>
      <translation>切断(&amp;D)</translation>
    </message>
    <message>
      <source>Disconnect selected connection</source>
      <translation>選択した接続を切断します</translation>
    </message>
    <message>
      <source>Disconnect</source>
      <translation>切断</translation>
    </message>
    <message>
      <source>&amp;End Process</source>
      <translation>プロセスの終了(&amp;E)</translation>
    </message>
    <message>
      <source>Terminate parent process of the selected connection</source>
      <translation>選択した接続の親プロセスを終了します</translation>
    </message>
    <message>
      <source>&amp;Create Rule for this Application...</source>
      <translation>このアプリケーションのルールを作成(&amp;C)...</translation>
    </message>
    <message>
      <source>&amp;Copy</source>
      <translation>コピー(&amp;C)</translation>
    </message>
    <message>
      <source>Select A&amp;ll</source>
      <translation>すべて選択(&amp;L)</translation>
    </message>
    <message>
      <source>Clea&amp;r</source>
      <translation>クリア(&amp;R)</translation>
    </message>
    <message>
      <source>Icon in the System &amp;Tray</source>
      <translation>システムトレイにアイコンを表示(&amp;T)</translation>
    </message>
    <message>
      <source>Show WinRoute icon in the System Tray and hide the main window when closed</source>
      <translation>システムトレイに WinRoute のアイコンを表示し、閉じたときはメインウィンドウを隠します</translation>
    </message>
    <message>
      <source>&amp;Toolbar</source>
      <translation>ツールバー(&amp;T)</translation>
    </message>
    <message>
      <source>&amp;Status Bar</source>
      <translation>ステータスバー(&amp;S)</translation>
    </message>
    <message>
      <source>Proxy &amp;Checker...</source>
      <translation>プロキシチェッカー(&amp;C)...</translation>
    </message>
    <message>
      <source>Test proxy servers</source>
      <translation>プロキシサーバーをテストします</translation>
    </message>
    <message>
      <source>&amp;Network Driver...</source>
      <translation>ネットワークドライバー(&amp;N)...</translation>
    </message>
    <message>
      <source>Install, switch or uninstall the network driver</source>
      <translation>ネットワークドライバーのインストール・切り替え・アンインストール</translation>
    </message>
    <message>
      <source>&amp;About WinRoute...</source>
      <translation>WinRoute について(&amp;A)...</translation>
    </message>
    <message>
      <source>Check for &amp;Updates...</source>
      <translation>更新の確認(&amp;U)...</translation>
    </message>
    <message>
      <source>&amp;File</source>
      <translation>ファイル(&amp;F)</translation>
    </message>
    <message>
      <source>&amp;Load Profile</source>
      <translation>プロファイルの読み込み(&amp;L)</translation>
    </message>
    <message>
      <source>&amp;Profile</source>
      <translation>プロファイル(&amp;P)</translation>
    </message>
    <message>
      <source>&amp;Advanced</source>
      <translation>詳細設定(&amp;A)</translation>
    </message>
    <message>
      <source>&amp;Log</source>
      <translation>ログ(&amp;L)</translation>
    </message>
    <message>
      <source>L&amp;og Level</source>
      <translation>ログレベル(&amp;O)</translation>
    </message>
    <message>
      <source>&amp;Errors Only</source>
      <translation>エラーのみ(&amp;E)</translation>
    </message>
    <message>
      <source>&amp;Normal</source>
      <translation>標準(&amp;N)</translation>
    </message>
    <message>
      <source>&amp;Verbose</source>
      <translation>詳細(&amp;V)</translation>
    </message>
    <message>
      <source>&amp;Debug</source>
      <translation>デバッグ(&amp;D)</translation>
    </message>
    <message>
      <source>Log to &amp;File</source>
      <translation>ファイルにログを記録(&amp;F)</translation>
    </message>
    <message>
      <source>&amp;Disabled</source>
      <translation>無効(&amp;D)</translation>
    </message>
    <message>
      <source>De&amp;bug</source>
      <translation>デバッグ(&amp;B)</translation>
    </message>
    <message>
      <source>Writing log to </source>
      <translation>ログの書き込み先：</translation>
    </message>
    <message>
      <source>&amp;Font Size</source>
      <translation>フォントサイズ(&amp;F)</translation>
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
      <translation>表示(&amp;V)</translation>
    </message>
    <message>
      <source>&amp;Refresh Rate</source>
      <translation>更新間隔(&amp;R)</translation>
    </message>
    <message>
      <source>&amp;Fast</source>
      <translation>速い(&amp;F)</translation>
    </message>
    <message>
      <source>&amp;Slow</source>
      <translation>遅い(&amp;S)</translation>
    </message>
    <message>
      <source>&amp;Language</source>
      <translation>言語(&amp;L)</translation>
    </message>
    <message>
      <source>&amp;Tools</source>
      <translation>ツール(&amp;T)</translation>
    </message>
    <message>
      <source>&amp;Help</source>
      <translation>ヘルプ(&amp;H)</translation>
    </message>
    <message>
      <source>Automatic U&amp;pdates</source>
      <translation>自動更新(&amp;P)</translation>
    </message>
    <message>
      <source>Install All Updates Automatically</source>
      <translation>すべての更新を自動でインストール</translation>
    </message>
    <message>
      <source>Install Required Updates Automatically</source>
      <translation>必須の更新を自動でインストール</translation>
    </message>
    <message>
      <source>Ask Before Updating</source>
      <translation>更新前に確認</translation>
    </message>
    <message>
      <source>Never Check</source>
      <translation>確認しない</translation>
    </message>
    <message>
      <source>Toolbar</source>
      <translation>ツールバー</translation>
    </message>
    <message>
      <source>Proxy Servers</source>
      <translation>プロキシサーバー</translation>
    </message>
    <message>
      <source>Proxification Rules</source>
      <translation>プロキシ化ルール</translation>
    </message>
    <message>
      <source>Name Resolution</source>
      <translation>名前解決</translation>
    </message>
    <message>
      <source>Proxy Checker</source>
      <translation>プロキシチェッカー</translation>
    </message>
    <message>
      <source>Connections</source>
      <translation>接続</translation>
    </message>
    <message>
      <source>Traffic</source>
      <translation>トラフィック</translation>
    </message>
    <message>
      <source>Statistics</source>
      <translation>統計</translation>
    </message>
    <message>
      <source>0 active connections</source>
      <translation>アクティブな接続 0 件</translation>
    </message>
    <message>
      <source>&amp;Open WinRoute</source>
      <translation>WinRoute を開く(&amp;O)</translation>
    </message>
    <message>
      <source>Profiles</source>
      <translation>プロファイル</translation>
    </message>
    <message>
      <source>WinRoute</source>
      <translation>WinRoute</translation>
    </message>
    <message>
      <source>Network interception started (%1)</source>
      <translation>ネットワークの捕捉を開始しました（%1）</translation>
    </message>
    <message>
      <source>Ready</source>
      <translation>準備完了</translation>
    </message>
    <message>
      <source>Cannot start network interception: </source>
      <translation>ネットワークの捕捉を開始できません：</translation>
    </message>
    <message>
      <source>Not running: </source>
      <translation>停止中：</translation>
    </message>
    <message>
      <source>WinRoute cannot start network interception:

</source>
      <translation>WinRoute はネットワークの捕捉を開始できません：

</translation>
    </message>
    <message>
      <source>WinRoute - %1</source>
      <translation>WinRoute - %1</translation>
    </message>
    <message>
      <source>Profile: </source>
      <translation>プロファイル：</translation>
    </message>
    <message>
      <source>Imported Proxifier profile </source>
      <translation>Proxifier のプロファイルをインポートしました：</translation>
    </message>
    <message>
      <source>Cannot load profile %1: %2</source>
      <translation>プロファイル %1 を読み込めません：%2</translation>
    </message>
    <message>
      <source>Profile %1 loaded</source>
      <translation>プロファイル %1 を読み込みました</translation>
    </message>
    <message>
      <source>Cannot save profile %1: %2</source>
      <translation>プロファイル %1 を保存できません：%2</translation>
    </message>
    <message>
      <source>No profiles defined</source>
      <translation>プロファイルがありません</translation>
    </message>
    <message>
      <source>Load this profile</source>
      <translation>このプロファイルを読み込みます</translation>
    </message>
    <message>
      <source>Profile Name:

Hint: WinRoute automatically loads the most recently used profile on start.</source>
      <translation>プロファイル名：

ヒント：WinRoute は起動時に最後に使用したプロファイルを自動的に読み込みます。</translation>
    </message>
    <message>
      <source>The profile name is empty or contains characters not allowed in file names.</source>
      <translation>プロファイル名が空か、ファイル名に使用できない文字が含まれています。</translation>
    </message>
    <message>
      <source>New Profile</source>
      <translation>新しいプロファイル</translation>
    </message>
    <message>
      <source>Profile %1 already exists. Overwrite it?</source>
      <translation>プロファイル %1 は既に存在します。上書きしますか？</translation>
    </message>
    <message>
      <source>New profile %1 created</source>
      <translation>新しいプロファイル %1 を作成しました</translation>
    </message>
    <message>
      <source>Save Profile As</source>
      <translation>名前を付けてプロファイルを保存</translation>
    </message>
    <message>
      <source>Profile saved as %1</source>
      <translation>プロファイルを %1 として保存しました</translation>
    </message>
    <message>
      <source>Import Profile</source>
      <translation>プロファイルのインポート</translation>
    </message>
    <message>
      <source>Profile Files (*.ppx);;All Files (*.*)</source>
      <translation>プロファイル (*.ppx);;すべてのファイル (*.*)</translation>
    </message>
    <message>
      <source>Cannot import %1:
%2</source>
      <translation>%1 をインポートできません：
%2</translation>
    </message>
    <message>
      <source>Profile %1 imported from %2</source>
      <translation>プロファイル %1 を %2 からインポートしました</translation>
    </message>
    <message>
      <source>Export Profile</source>
      <translation>プロファイルのエクスポート</translation>
    </message>
    <message>
      <source>Profile Files (*.ppx)</source>
      <translation>プロファイル (*.ppx)</translation>
    </message>
    <message>
      <source>Profile exported to %1</source>
      <translation>プロファイルを %1 にエクスポートしました</translation>
    </message>
    <message>
      <source>Fail status of all proxies was reset</source>
      <translation>すべてのプロキシの失敗状態をリセットしました</translation>
    </message>
    <message>
      <source>Chain %1</source>
      <translation>チェーン %1</translation>
    </message>
    <message>
      <source>Please add a proxy server first (Profile &gt; Proxy Servers).</source>
      <translation>先にプロキシサーバーを追加してください（プロファイル &gt; プロキシサーバー）。</translation>
    </message>
    <message>
      <source>About WinRoute</source>
      <translation>WinRoute について</translation>
    </message>
    <message>
      <source>&lt;h3&gt;WinRoute %1&lt;/h3&gt;&lt;p&gt;Routes the network connections of Windows applications through SOCKS and HTTPS proxy servers according to proxification rules.&lt;/p&gt;&lt;p&gt;Interception: %2&lt;br&gt;Qt %3&lt;/p&gt;</source>
      <translation>&lt;h3&gt;WinRoute %1&lt;/h3&gt;&lt;p&gt;プロキシ化ルールに従って、Windows アプリケーションのネットワーク接続を SOCKS / HTTPS プロキシサーバー経由にします。&lt;/p&gt;&lt;p&gt;捕捉方式：%2&lt;br&gt;Qt %3&lt;/p&gt;</translation>
    </message>
    <message>
      <source>%1 (pid %2)</source>
      <translation>%1（PID %2）</translation>
    </message>
    <message>
      <source>End Process</source>
      <translation>プロセスの終了</translation>
    </message>
    <message>
      <source>Terminate the following process(es)?

</source>
      <translation>次のプロセスを終了しますか？

</translation>
    </message>
    <message>
      <source>Cannot terminate process %1: %2</source>
      <translation>プロセス %1 を終了できません：%2</translation>
    </message>
    <message>
      <source>Down %1, Up %2</source>
      <translation>受信 %1、送信 %2</translation>
    </message>
    <message>
      <source>WinRoute - %1
%2 active connections
Down %3, Up %4</source>
      <translation>WinRoute - %1
アクティブな接続 %2 件
受信 %3、送信 %4</translation>
    </message>
    <message>
      <source>Local DNS is available, host names are resolved locally</source>
      <translation>ローカル DNS を利用できます。ホスト名はローカルで解決されます</translation>
    </message>
    <message>
      <source>Local DNS is not available, host names are resolved through proxy</source>
      <translation>ローカル DNS を利用できません。ホスト名はプロキシ経由で解決されます</translation>
    </message>
    <message>
      <source>Language</source>
      <translation>言語</translation>
    </message>
    <message>
      <source>The new language will be used after WinRoute restarts.

Restart WinRoute now?</source>
      <translation>新しい言語は WinRoute の再起動後に適用されます。

今すぐ WinRoute を再起動しますか？</translation>
    </message>
    <message>
      <source>Cannot change autostart: </source>
      <translation>自動起動を変更できません：</translation>
    </message>
    <message>
      <source>Autostart enabled</source>
      <translation>自動起動を有効にしました</translation>
    </message>
    <message>
      <source>Autostart disabled</source>
      <translation>自動起動を無効にしました</translation>
    </message>
  </context>
  <context>
    <name>NameResolutionDialog</name>
    <message>
      <source>Name Resolution</source>
      <translation>名前解決</translation>
    </message>
    <message>
      <source>WinRoute DNS settings</source>
      <translation>WinRoute の DNS 設定</translation>
    </message>
    <message>
      <source>Detect DNS settings automatically</source>
      <translation>DNS 設定を自動的に検出する</translation>
    </message>
    <message>
      <source>Resolve hostnames through proxy</source>
      <translation>プロキシ経由でホスト名を解決する</translation>
    </message>
    <message>
      <source>Try to resolve via local DNS service first
(may cause significant delays if local DNS is unavailable!)</source>
      <translation>最初にローカル DNS サービスで解決を試みる
（ローカル DNS が利用できない場合は大きな遅延が発生することがあります！）</translation>
    </message>
    <message>
      <source>Note</source>
      <translation>注意</translation>
    </message>
    <message>
      <source>It is impossible to get real IP addresses through proxy.
WinRoute assigns "fake" IP addresses (198.18.*.*) that are only valid when it is running.</source>
      <translation>プロキシ経由では実際の IP アドレスを取得できません。
WinRoute は「仮想」IP アドレス（198.18.*.*）を割り当てます。これは WinRoute の実行中のみ有効です。</translation>
    </message>
    <message>
      <source>DNS name list</source>
      <translation>DNS 名リスト</translation>
    </message>
    <message>
      <source>Do NOT resolve the following</source>
      <translation>次の名前は解決しない</translation>
    </message>
    <message>
      <source>Resolve ONLY the following</source>
      <translation>次の名前のみ解決する</translation>
    </message>
    <message>
      <source>Example:
localhost; *.example.com; %ComputerName%; %SimpleHostnames%</source>
      <translation>例：
localhost; *.example.com; %ComputerName%; %SimpleHostnames%</translation>
    </message>
  </context>
  <context>
    <name>Profile</name>
    <message>
      <source>Direct</source>
      <translation>直接接続</translation>
    </message>
    <message>
      <source>Block</source>
      <translation>ブロック</translation>
    </message>
    <message>
      <source>Proxy %1 %2</source>
      <translation>プロキシ %1 %2</translation>
    </message>
    <message>
      <source>Proxy &lt;missing&gt;</source>
      <translation>プロキシ &lt;見つかりません&gt;</translation>
    </message>
    <message>
      <source>Chain %1</source>
      <translation>チェーン %1</translation>
    </message>
    <message>
      <source>Chain &lt;missing&gt;</source>
      <translation>チェーン &lt;見つかりません&gt;</translation>
    </message>
    <message>
      <source>Not a Proxifier/WinRoute profile</source>
      <translation>Proxifier/WinRoute のプロファイルではありません</translation>
    </message>
    <message>
      <source>XML error at line %1: %2</source>
      <translation>%1 行目の XML エラー：%2</translation>
    </message>
    <message>
      <source>Password of proxy %1 is encrypted in a format WinRoute cannot read; please re-enter it.</source>
      <translation>プロキシ %1 のパスワードは WinRoute で読み取れない形式で暗号化されています。再入力してください。</translation>
    </message>
  </context>
  <context>
    <name>ProxyAdvancedDialog</name>
    <message>
      <source>Advanced Proxy Server Settings</source>
      <translation>プロキシサーバーの詳細設定</translation>
    </message>
    <message>
      <source>Appearance</source>
      <translation>表示</translation>
    </message>
    <message>
      <source>("address:port" by default)</source>
      <translation>（既定は「アドレス:ポート」）</translation>
    </message>
    <message>
      <source>Custom Label:</source>
      <translation>カスタムラベル：</translation>
    </message>
    <message>
      <source>Other</source>
      <translation>その他</translation>
    </message>
    <message>
      <source>Use target hostname in proxy request if available</source>
      <translation>可能な場合はプロキシ要求に接続先のホスト名を使用する</translation>
    </message>
  </context>
  <context>
    <name>ProxyCheckerDialog</name>
    <message>
      <source>Chain</source>
      <translation>チェーン</translation>
    </message>
    <message>
      <source>Proxy Checker</source>
      <translation>プロキシチェッカー</translation>
    </message>
    <message>
      <source>Proxy server:</source>
      <translation>プロキシサーバー：</translation>
    </message>
    <message>
      <source>Port:</source>
      <translation>ポート：</translation>
    </message>
    <message>
      <source>Target host:</source>
      <translation>接続先ホスト：</translation>
    </message>
    <message>
      <source>Start Testing</source>
      <translation>テスト開始</translation>
    </message>
    <message>
      <source>Close</source>
      <translation>閉じる</translation>
    </message>
    <message>
      <source>&lt;b&gt;Testing Started...&lt;/b&gt;</source>
      <translation>&lt;b&gt;テストを開始しました...&lt;/b&gt;</translation>
    </message>
    <message>
      <source>Proxy Server%1&lt;br&gt;&amp;nbsp;&amp;nbsp;Address: %2:%3&lt;br&gt;&amp;nbsp;&amp;nbsp;Protocol: %4&lt;br&gt;&amp;nbsp;&amp;nbsp;Authentication: %5</source>
      <translation>プロキシサーバー%1&lt;br&gt;&amp;nbsp;&amp;nbsp;アドレス：%2:%3&lt;br&gt;&amp;nbsp;&amp;nbsp;プロトコル：%4&lt;br&gt;&amp;nbsp;&amp;nbsp;認証：%5</translation>
    </message>
    <message>
      <source>YES</source>
      <translation>あり</translation>
    </message>
    <message>
      <source>NO</source>
      <translation>なし</translation>
    </message>
    <message>
      <source>&lt;b&gt;Connection Test&lt;/b&gt;</source>
      <translation>&lt;b&gt;接続テスト&lt;/b&gt;</translation>
    </message>
    <message>
      <source>Connecting to %1:%2 ...</source>
      <translation>%1:%2 に接続しています...</translation>
    </message>
    <message>
      <source>Connection established %1 (%2 ms)</source>
      <translation>接続しました %1（%2 ミリ秒）</translation>
    </message>
    <message>
      <source>&lt;b&gt;Connection through the Proxy Server Test&lt;/b&gt;</source>
      <translation>&lt;b&gt;プロキシサーバー経由の接続テスト&lt;/b&gt;</translation>
    </message>
    <message>
      <source>Connecting to %1:%2 through the proxy ...</source>
      <translation>プロキシ経由で %1:%2 に接続しています...</translation>
    </message>
    <message>
      <source>Connection to %1 established through the proxy %2 (%3 ms)</source>
      <translation>プロキシ経由で %1 に接続しました %2（%3 ミリ秒）</translation>
    </message>
    <message>
      <source>&lt;b&gt;HTTP Request Test&lt;/b&gt;</source>
      <translation>&lt;b&gt;HTTP 要求テスト&lt;/b&gt;</translation>
    </message>
    <message>
      <source>Response: %1 %2</source>
      <translation>応答：%1 %2</translation>
    </message>
    <message>
      <source>%1 : connection closed without a response</source>
      <translation>%1：応答がないまま接続が閉じられました</translation>
    </message>
    <message>
      <source>%1 : no response within 10 seconds</source>
      <translation>%1：10 秒以内に応答がありません</translation>
    </message>
    <message>
      <source>&lt;b&gt;Testing Finished.&lt;/b&gt; Proxy server is working correctly!</source>
      <translation>&lt;b&gt;テスト完了。&lt;/b&gt;プロキシサーバーは正常に動作しています！</translation>
    </message>
    <message>
      <source>&lt;b&gt;Testing Finished.&lt;/b&gt; &lt;span style='color:#c0201a'&gt;The proxy server test failed.&lt;/span&gt;</source>
      <translation>&lt;b&gt;テスト完了。&lt;/b&gt; &lt;span style='color:#c0201a'&gt;プロキシサーバーのテストに失敗しました。&lt;/span&gt;</translation>
    </message>
  </context>
  <context>
    <name>ProxyConnector</name>
    <message>
      <source>Could not connect to %1 - connection timeout</source>
      <translation>%1 に接続できません - 接続タイムアウト</translation>
    </message>
    <message>
      <source>Could not connect to proxy %1 - connection timeout</source>
      <translation>プロキシ %1 に接続できません - 接続タイムアウト</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - proxy handshake timeout</source>
      <translation>プロキシ %1 経由で接続できません - ハンドシェイクのタイムアウト</translation>
    </message>
    <message>
      <source>No target address</source>
      <translation>接続先アドレスがありません</translation>
    </message>
    <message>
      <source>No resolver available</source>
      <translation>利用できるリゾルバーがありません</translation>
    </message>
    <message>
      <source>Could not resolve %1 - %2</source>
      <translation>%1 を解決できません - %2</translation>
    </message>
    <message>
      <source>Connection closed</source>
      <translation>接続が閉じられました</translation>
    </message>
    <message>
      <source>Connected to proxy %1</source>
      <translation>プロキシ %1 に接続しました</translation>
    </message>
    <message>
      <source>Could not connect to %1 - %2</source>
      <translation>%1 に接続できません - %2</translation>
    </message>
    <message>
      <source>Could not connect to proxy %1 - %2</source>
      <translation>プロキシ %1 に接続できません - %2</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - %2</source>
      <translation>プロキシ %1 経由で接続できません - %2</translation>
    </message>
    <message>
      <source>Connected to proxy %1 through the chain</source>
      <translation>チェーン経由でプロキシ %1 に接続しました</translation>
    </message>
    <message>
      <source>general SOCKS server failure</source>
      <translation>SOCKS サーバーの一般的な障害</translation>
    </message>
    <message>
      <source>connection not allowed by ruleset</source>
      <translation>ルールセットにより接続が許可されていません</translation>
    </message>
    <message>
      <source>network unreachable</source>
      <translation>ネットワークに到達できません</translation>
    </message>
    <message>
      <source>host unreachable</source>
      <translation>ホストに到達できません</translation>
    </message>
    <message>
      <source>connection refused by the target</source>
      <translation>接続先が接続を拒否しました</translation>
    </message>
    <message>
      <source>TTL expired</source>
      <translation>TTL の期限切れ</translation>
    </message>
    <message>
      <source>command not supported</source>
      <translation>コマンドはサポートされていません</translation>
    </message>
    <message>
      <source>address type not supported</source>
      <translation>アドレスの種類はサポートされていません</translation>
    </message>
    <message>
      <source>error code %1</source>
      <translation>エラーコード %1</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - not a SOCKS5 proxy</source>
      <translation>プロキシ %1 経由で接続できません - SOCKS5 プロキシではありません</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - proxy requires authentication</source>
      <translation>プロキシ %1 経由で接続できません - プロキシは認証を必要としています</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - unsupported authentication method %2</source>
      <translation>プロキシ %1 経由で接続できません - 未対応の認証方式 %2</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - authentication failed (wrong username or password)</source>
      <translation>プロキシ %1 経由で接続できません - 認証に失敗しました（ユーザー名またはパスワードが違います）</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - malformed reply</source>
      <translation>プロキシ %1 経由で接続できません - 不正な応答</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - SOCKS4 does not support IPv6 targets</source>
      <translation>プロキシ %1 経由で接続できません - SOCKS4 は IPv6 の接続先に対応していません</translation>
    </message>
    <message>
      <source>identd authentication failed</source>
      <translation>identd 認証に失敗しました</translation>
    </message>
    <message>
      <source>request rejected or failed</source>
      <translation>要求が拒否されたか失敗しました</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - %2 (code %3)</source>
      <translation>プロキシ %1 経由で接続できません - %2（コード %3）</translation>
    </message>
    <message>
      <source>Could not connect through proxy %1 - response header too large</source>
      <translation>プロキシ %1 経由で接続できません - 応答ヘッダーが大きすぎます</translation>
    </message>
    <message>
      <source>proxy requires authentication (HTTP 407)</source>
      <translation>プロキシは認証を必要としています（HTTP 407）</translation>
    </message>
    <message>
      <source>not an HTTP proxy</source>
      <translation>HTTP プロキシではありません</translation>
    </message>
    <message>
      <source>proxy returned HTTP %1</source>
      <translation>プロキシが HTTP %1 を返しました</translation>
    </message>
  </context>
  <context>
    <name>ProxyServerDialog</name>
    <message>
      <source>Proxy Server</source>
      <translation>プロキシサーバー</translation>
    </message>
    <message>
      <source>Server</source>
      <translation>サーバー</translation>
    </message>
    <message>
      <source>IP address or host name</source>
      <translation>IP アドレスまたはホスト名</translation>
    </message>
    <message>
      <source>SOCKS Version 4</source>
      <translation>SOCKS バージョン 4</translation>
    </message>
    <message>
      <source>SOCKS Version 5</source>
      <translation>SOCKS バージョン 5</translation>
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
      <translation>アドレス：</translation>
    </message>
    <message>
      <source>Port:</source>
      <translation>ポート：</translation>
    </message>
    <message>
      <source>Protocol:</source>
      <translation>プロトコル：</translation>
    </message>
    <message>
      <source>Authentication</source>
      <translation>認証</translation>
    </message>
    <message>
      <source>Enable</source>
      <translation>有効</translation>
    </message>
    <message>
      <source>Use NTLM</source>
      <translation>NTLM を使用</translation>
    </message>
    <message>
      <source>Use Kerberos</source>
      <translation>Kerberos を使用</translation>
    </message>
    <message>
      <source>Not supported</source>
      <translation>未対応</translation>
    </message>
    <message>
      <source>Username:</source>
      <translation>ユーザー名：</translation>
    </message>
    <message>
      <source>Password:</source>
      <translation>パスワード：</translation>
    </message>
    <message>
      <source>Options</source>
      <translation>オプション</translation>
    </message>
    <message>
      <source>Use SOCKS 4A extension (remote hostname resolving feature)</source>
      <translation>SOCKS 4A 拡張を使用する（リモートでのホスト名解決）</translation>
    </message>
    <message>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <source>Check</source>
      <translation>チェック</translation>
    </message>
    <message>
      <source>Advanced...</source>
      <translation>詳細設定...</translation>
    </message>
    <message>
      <source>Cancel</source>
      <translation>キャンセル</translation>
    </message>
    <message>
      <source>No options for SOCKS5 are available.</source>
      <translation>SOCKS5 には設定できるオプションがありません。</translation>
    </message>
    <message>
      <source>The proxy must support HTTP tunneling (CONNECT requests).</source>
      <translation>プロキシは HTTP トンネリング（CONNECT 要求）に対応している必要があります。</translation>
    </message>
    <message>
      <source>HTTP proxy servers can be used for HTTP connections only.</source>
      <translation>HTTP プロキシサーバーは HTTP 接続にのみ使用できます。</translation>
    </message>
    <message>
      <source>Please enter the proxy server address.</source>
      <translation>プロキシサーバーのアドレスを入力してください。</translation>
    </message>
  </context>
  <context>
    <name>ProxyServersDialog</name>
    <message>
      <source>Proxy Servers</source>
      <translation>プロキシサーバー</translation>
    </message>
    <message>
      <source>Address</source>
      <translation>アドレス</translation>
    </message>
    <message>
      <source>Port</source>
      <translation>ポート</translation>
    </message>
    <message>
      <source>Type</source>
      <translation>種類</translation>
    </message>
    <message>
      <source>Add...</source>
      <translation>追加...</translation>
    </message>
    <message>
      <source>Edit...</source>
      <translation>編集...</translation>
    </message>
    <message>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
    <message>
      <source>Check...</source>
      <translation>チェック...</translation>
    </message>
    <message>
      <source>You can chain several proxy servers together:</source>
      <translation>複数のプロキシサーバーをチェーンとしてつなげられます：</translation>
    </message>
    <message>
      <source>Proxy Chains...</source>
      <translation>プロキシチェーン...</translation>
    </message>
    <message>
      <source>Create</source>
      <translation>作成</translation>
    </message>
    <message>
      <source>Type...</source>
      <translation>種類...</translation>
    </message>
    <message>
      <source>Drag and Drop proxy servers within a chain to alter the order.
Use check boxes to enable/disable a proxy in a chain.</source>
      <translation>チェーン内でプロキシサーバーをドラッグ＆ドロップすると順序を変更できます。
チェックボックスでチェーン内のプロキシを有効/無効にします。</translation>
    </message>
    <message>
      <source>Proxy Chain</source>
      <translation>プロキシチェーン</translation>
    </message>
    <message>
      <source>Chain name:</source>
      <translation>チェーン名：</translation>
    </message>
    <message>
      <source>Drag and Drop a proxy server from the list to add it into a chain.</source>
      <translation>リストからプロキシサーバーをドラッグ＆ドロップしてチェーンに追加します。</translation>
    </message>
    <message>
      <source> (Redundancy)</source>
      <translation>（冗長化）</translation>
    </message>
    <message>
      <source> (Load Balancing)</source>
      <translation>（負荷分散）</translation>
    </message>
    <message>
      <source>&lt;missing proxy %1&gt;</source>
      <translation>&lt;見つからないプロキシ %1&gt;</translation>
    </message>
    <message>
      <source>Do you want to use this proxy by default?

The Default rule will be changed to use %1.
You can change this later in Proxification Rules.</source>
      <translation>このプロキシを既定で使用しますか？

Default ルールが %1 を使用するように変更されます。
後で「プロキシ化ルール」で変更できます。</translation>
    </message>
    <message>
      <source>Proxy Removal</source>
      <translation>プロキシの削除</translation>
    </message>
    <message>
      <source>Chain Removal</source>
      <translation>チェーンの削除</translation>
    </message>
    <message>
      <source>%1 %2 is assigned as the Action for the following Rule(s):</source>
      <translation>%1 %2 は次のルールのアクションとして使用されています：</translation>
    </message>
    <message>
      <source>Proxy</source>
      <translation>プロキシ</translation>
    </message>
    <message>
      <source>Chain</source>
      <translation>チェーン</translation>
    </message>
    <message>
      <source>To proceed with the removal you can:</source>
      <translation>削除を続行するには、次のいずれかを選択します：</translation>
    </message>
    <message>
      <source>Remove the Rule(s)</source>
      <translation>ルールを削除する</translation>
    </message>
    <message>
      <source>Select a new Action:</source>
      <translation>新しいアクションを選択：</translation>
    </message>
    <message>
      <source>Direct</source>
      <translation>直接接続</translation>
    </message>
    <message>
      <source>Block</source>
      <translation>ブロック</translation>
    </message>
    <message>
      <source>Proxy %1 %2</source>
      <translation>プロキシ %1 %2</translation>
    </message>
    <message>
      <source>Chain %1</source>
      <translation>チェーン %1</translation>
    </message>
    <message>
      <source>The Default rule is used when no other rule matches, so it cannot be removed.</source>
      <translation>Default ルールは他のルールに一致しない場合に使用されるため、削除できません。</translation>
    </message>
    <message>
      <source>Chain%1</source>
      <translation>チェーン%1</translation>
    </message>
  </context>
  <context>
    <name>RedundancyOptionsDialog</name>
    <message>
      <source>Redundancy Options</source>
      <translation>冗長化オプション</translation>
    </message>
    <message>
      <source>Recheck failed proxy each (sec.):</source>
      <translation>失敗したプロキシの再確認間隔（秒）：</translation>
    </message>
    <message>
      <source>Connect directly if all proxies fail</source>
      <translation>すべてのプロキシが失敗した場合は直接接続する</translation>
    </message>
    <message>
      <source>Reset Fail Status for All Proxies</source>
      <translation>すべてのプロキシの失敗状態をリセット</translation>
    </message>
    <message>
      <source>Connection to proxy timeout (sec.):</source>
      <translation>プロキシへの接続タイムアウト（秒）：</translation>
    </message>
  </context>
  <context>
    <name>Relay</name>
    <message>
      <source>%1 bytes</source>
      <translation>%1 バイト</translation>
    </message>
    <message>
      <source>%1 bytes (%2)</source>
      <translation>%1 バイト（%2）</translation>
    </message>
    <message>
      <source>no active profile</source>
      <translation>有効なプロファイルがありません</translation>
    </message>
    <message>
      <source>Direct</source>
      <translation>直接接続</translation>
    </message>
    <message>
      <source>proxy server #%1 used by rule %2 does not exist</source>
      <translation>ルール %2 で使用しているプロキシサーバー #%1 は存在しません</translation>
    </message>
    <message>
      <source>proxy chain #%1 used by rule %2 does not exist</source>
      <translation>ルール %2 で使用しているプロキシチェーン #%1 は存在しません</translation>
    </message>
    <message>
      <source>Chain %1</source>
      <translation>チェーン %1</translation>
    </message>
    <message>
      <source>proxy chain %1 has no enabled proxy servers</source>
      <translation>プロキシチェーン %1 に有効なプロキシサーバーがありません</translation>
    </message>
    <message>
      <source>%1 - %2 matching %3 rule : using %4</source>
      <translation>%1 - %2 ルール %3 に一致：%4 を使用</translation>
    </message>
    <message>
      <source>direct connection</source>
      <translation>直接接続</translation>
    </message>
    <message>
      <source>%1 - %2 %3, trying the next proxy in the chain</source>
      <translation>%1 - %2 %3、チェーン内の次のプロキシを試します</translation>
    </message>
    <message>
      <source>%1 - %2 open directly</source>
      <translation>%1 - %2 直接接続を開始</translation>
    </message>
    <message>
      <source>%1 - %2 open through proxy %3:%4 %5</source>
      <translation>%1 - %2 プロキシ %3:%4 %5 経由で接続</translation>
    </message>
    <message>
      <source>%1 - %2 open through %3</source>
      <translation>%1 - %2 %3 経由で接続</translation>
    </message>
    <message>
      <source>%1 - %2 error : %3</source>
      <translation>%1 - %2 エラー：%3</translation>
    </message>
    <message>
      <source>%1 - %2 close, %3 sent, %4 received, lifetime %5</source>
      <translation>%1 - %2 切断、送信 %3、受信 %4、接続時間 %5</translation>
    </message>
    <message>
      <source>%1 - %2 closed by the application before the connection was established</source>
      <translation>%1 - %2 接続の確立前にアプリケーションが閉じました</translation>
    </message>
    <message>
      <source>Cannot bind the DNS resolver socket</source>
      <translation>DNS リゾルバーのソケットをバインドできません</translation>
    </message>
    <message>
      <source>Cannot start the local relay: </source>
      <translation>ローカルリレーを開始できません：</translation>
    </message>
    <message>
      <source>IPv6 relay is not available: </source>
      <translation>IPv6 リレーを利用できません：</translation>
    </message>
    <message>
      <source>Relay: unexpected connection from %1 dropped</source>
      <translation>リレー：%1 からの想定外の接続を破棄しました</translation>
    </message>
    <message>
      <source>%1 - UDP proxy #%2 used by rule %3 does not exist</source>
      <translation>%1 - ルール %3 が使用する UDP プロキシ #%2 は存在しません</translation>
    </message>
    <message>
      <source>the network driver API is unavailable</source>
      <translation>ネットワークドライバー API を利用できません</translation>
    </message>
    <message>
      <source>no resolver available</source>
      <translation>リゾルバーを利用できません</translation>
    </message>
    <message>
      <source>could not resolve proxy %1 - %2</source>
      <translation>プロキシ %1 を解決できませんでした - %2</translation>
    </message>
    <message>
      <source>not a SOCKS5 proxy</source>
      <translation>SOCKS5 プロキシではありません</translation>
    </message>
    <message>
      <source>SOCKS5 authentication method not accepted</source>
      <translation>SOCKS5 認証方式が受け入れられませんでした</translation>
    </message>
    <message>
      <source>SOCKS5 authentication failed</source>
      <translation>SOCKS5 認証に失敗しました</translation>
    </message>
    <message>
      <source>malformed SOCKS5 reply</source>
      <translation>SOCKS5 応答の形式が不正です</translation>
    </message>
    <message>
      <source>the proxy does not support UDP (SOCKS5 reply %1)</source>
      <translation>プロキシは UDP に対応していません（SOCKS5 応答 %1）</translation>
    </message>
    <message>
      <source>cannot open the local UDP socket</source>
      <translation>ローカル UDP ソケットを開けません</translation>
    </message>
    <message>
      <source>%1 - %2 open through proxy %3:%4 SOCKS5 (UDP)</source>
      <translation>%1 - %2 をプロキシ %3:%4 SOCKS5 経由で接続（UDP）</translation>
    </message>
    <message>
      <source>%1 - %2 UDP cannot use the proxy (%3); sending directly</source>
      <translation>%1 - %2 UDP はプロキシを使用できません（%3）。直接接続します</translation>
    </message>
    <message>
      <source>the proxy closed the control connection</source>
      <translation>プロキシが制御接続を閉じました</translation>
    </message>
    <message>
      <source>%1 - %2 UDP closed, %3 sent, %4 received</source>
      <translation>%1 - %2 UDP 切断、送信 %3、受信 %4</translation>
    </message>
  </context>
  <context>
    <name>RuleDialog</name>
    <message>
      <source>Direct</source>
      <translation>直接接続</translation>
    </message>
    <message>
      <source>Block</source>
      <translation>ブロック</translation>
    </message>
    <message>
      <source>Proxy %1 %2</source>
      <translation>プロキシ %1 %2</translation>
    </message>
    <message>
      <source>Chain %1</source>
      <translation>チェーン %1</translation>
    </message>
    <message>
      <source>Proxification Rule</source>
      <translation>プロキシ化ルール</translation>
    </message>
    <message>
      <source>Enabled</source>
      <translation>有効</translation>
    </message>
    <message>
      <source>Name:</source>
      <translation>名前：</translation>
    </message>
    <message>
      <source>Applications</source>
      <translation>アプリケーション</translation>
    </message>
    <message>
      <source>Any</source>
      <translation>すべて</translation>
    </message>
    <message>
      <source>Browse...</source>
      <translation>参照...</translation>
    </message>
    <message>
      <source>Example: iexplore.exe; "C:\some app.exe"; fire*.exe; *.bin</source>
      <translation>例：iexplore.exe; "C:\some app.exe"; fire*.exe; *.bin</translation>
    </message>
    <message>
      <source>Target hosts</source>
      <translation>接続先ホスト</translation>
    </message>
    <message>
      <source>Example: 127.0.0.1; *.example.com; 192.168.1.*; 10.1.0.0-10.5.255.255</source>
      <translation>例：127.0.0.1; *.example.com; 192.168.1.*; 10.1.0.0-10.5.255.255</translation>
    </message>
    <message>
      <source>Target ports</source>
      <translation>接続先ポート</translation>
    </message>
    <message>
      <source>Example: 80; 8000-9000; 3128</source>
      <translation>例：80; 8000-9000; 3128</translation>
    </message>
    <message>
      <source>Advanced...</source>
      <translation>詳細設定...</translation>
    </message>
    <message>
      <source>Per-rule network interface selection is not supported yet</source>
      <translation>ルールごとのネットワークインターフェイスの指定にはまだ対応していません</translation>
    </message>
    <message>
      <source>Action:</source>
      <translation>アクション：</translation>
    </message>
    <message>
      <source>Select Applications</source>
      <translation>アプリケーションの選択</translation>
    </message>
    <message>
      <source>Applications (*.exe);;All files (*.*)</source>
      <translation>アプリケーション (*.exe);;すべてのファイル (*.*)</translation>
    </message>
    <message>
      <source>Please enter the rule name.</source>
      <translation>ルール名を入力してください。</translation>
    </message>
    <message>
      <source>Target ports must look like: 80; 8000-9000; 3128</source>
      <translation>接続先ポートは次の形式で入力してください：80; 8000-9000; 3128</translation>
    </message>
  </context>
  <context>
    <name>RulesDialog</name>
    <message>
      <source>Proxification Rules</source>
      <translation>プロキシ化ルール</translation>
    </message>
    <message>
      <source>Name</source>
      <translation>名前</translation>
    </message>
    <message>
      <source>Applications</source>
      <translation>アプリケーション</translation>
    </message>
    <message>
      <source>Target Hosts</source>
      <translation>接続先ホスト</translation>
    </message>
    <message>
      <source>Target Ports</source>
      <translation>接続先ポート</translation>
    </message>
    <message>
      <source>Action</source>
      <translation>アクション</translation>
    </message>
    <message>
      <source>Move up</source>
      <translation>上へ移動</translation>
    </message>
    <message>
      <source>Move down</source>
      <translation>下へ移動</translation>
    </message>
    <message>
      <source>Add...</source>
      <translation>追加...</translation>
    </message>
    <message>
      <source>Clone</source>
      <translation>複製</translation>
    </message>
    <message>
      <source>Edit...</source>
      <translation>編集...</translation>
    </message>
    <message>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
    <message>
      <source>Rules are processed from top to bottom. Use the arrows at the right to change the order.

Use check boxes to enable/disable a rule.</source>
      <translation>ルールは上から順に処理されます。右側の矢印で順序を変更できます。

チェックボックスでルールを有効/無効にします。</translation>
    </message>
    <message>
      <source>Any</source>
      <translation>すべて</translation>
    </message>
    <message>
      <source>New</source>
      <translation>新規</translation>
    </message>
    <message>
      <source> (copy)</source>
      <translation>（コピー）</translation>
    </message>
    <message>
      <source>The Default rule is used when all other rules do not match the connection parameters, so it cannot be removed.</source>
      <translation>Default ルールは他のすべてのルールが接続に一致しない場合に使用されるため、削除できません。</translation>
    </message>
  </context>
  <context>
    <name>ServicesDialog</name>
    <message>
      <source>Services and other users</source>
      <translation>サービスと他のユーザー</translation>
    </message>
    <message>
      <source>Info</source>
      <translation>情報</translation>
    </message>
    <message>
      <source>Normally WinRoute processes connections for the current user only. You can change this behavior if needed.</source>
      <translation>通常、WinRoute は現在のユーザーの接続のみを処理します。必要に応じてこの動作を変更できます。</translation>
    </message>
    <message>
      <source>Additional WinRoute targets</source>
      <translation>WinRoute の追加対象</translation>
    </message>
    <message>
      <source>Applications run by other users on this computer</source>
      <translation>このコンピューターで他のユーザーが実行するアプリケーション</translation>
    </message>
    <message>
      <source>Windows services and other system processes</source>
      <translation>Windows サービスとその他のシステムプロセス</translation>
    </message>
  </context>
  <context>
    <name>StatisticsWidget</name>
    <message>
      <source>Application / Target</source>
      <translation>アプリケーション / 接続先</translation>
    </message>
    <message>
      <source>Connections</source>
      <translation>接続</translation>
    </message>
    <message>
      <source>Active</source>
      <translation>アクティブ</translation>
    </message>
    <message>
      <source>Sent</source>
      <translation>送信</translation>
    </message>
    <message>
      <source>Received</source>
      <translation>受信</translation>
    </message>
    <message>
      <source>Last Activity</source>
      <translation>最終アクティビティ</translation>
    </message>
    <message>
      <source>&amp;Copy</source>
      <translation>コピー(&amp;C)</translation>
    </message>
    <message>
      <source>&amp;Reset Statistics</source>
      <translation>統計のリセット(&amp;R)</translation>
    </message>
  </context>
  <context>
    <name>TrafficGraph</name>
    <message>
      <source>-%1 min</source>
      <translation>-%1 分</translation>
    </message>
  </context>
  <context>
    <name>TrafficWidget</name>
    <message>
      <source>Show Both</source>
      <translation>両方を表示</translation>
    </message>
    <message>
      <source>Incoming Only</source>
      <translation>受信のみ</translation>
    </message>
    <message>
      <source>Outgoing Only</source>
      <translation>送信のみ</translation>
    </message>
    <message>
      <source>Select the kind of the traffic to display</source>
      <translation>表示するトラフィックの種類を選択します</translation>
    </message>
    <message>
      <source>Clear</source>
      <translation>クリア</translation>
    </message>
    <message>
      <source>Incoming: %1 (max %2)</source>
      <translation>受信：%1（最大 %2）</translation>
    </message>
    <message>
      <source>Outgoing: %1 (max %2)</source>
      <translation>送信：%1（最大 %2）</translation>
    </message>
    <message>
      <source>Total: %1 received, %2 sent</source>
      <translation>合計：受信 %1、送信 %2</translation>
    </message>
  </context>
  <context>
    <name>Units</name>
    <message>
      <source>%1 bytes</source>
      <translation>%1 バイト</translation>
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
      <translation>WinRoute の更新</translation>
    </message>
    <message>
      <source>Could not check for updates:
%1</source>
      <translation>更新を確認できませんでした：
%1</translation>
    </message>
    <message>
      <source>WinRoute %1 is the latest version.</source>
      <translation>WinRoute %1 は最新バージョンです。</translation>
    </message>
    <message>
      <source>WinRoute %1 is available; you have %2.
Update now? WinRoute restarts when it is done.</source>
      <translation>WinRoute %1 が利用できます（現在のバージョンは %2）。
今すぐ更新しますか？完了後に WinRoute が再起動します。</translation>
    </message>
    <message>
      <source>Update</source>
      <translation>更新</translation>
    </message>
    <message>
      <source>Later</source>
      <translation>後で</translation>
    </message>
    <message>
      <source>Downloading WinRoute %1...</source>
      <translation>WinRoute %1 をダウンロードしています...</translation>
    </message>
    <message>
      <source>Cancel</source>
      <translation>キャンセル</translation>
    </message>
    <message>
      <source>Downloading WinRoute %1; WinRoute restarts once it is installed.</source>
      <translation>WinRoute %1 をダウンロードしています。インストール後に WinRoute が再起動します。</translation>
    </message>
    <message>
      <source>The download is damaged (checksum mismatch).</source>
      <translation>ダウンロードしたファイルが壊れています（チェックサムが一致しません）。</translation>
    </message>
    <message>
      <source>Could not download WinRoute %1:
%2</source>
      <translation>WinRoute %1 をダウンロードできませんでした：
%2</translation>
    </message>
    <message>
      <source>Could not prepare the update.</source>
      <translation>更新を準備できませんでした。</translation>
    </message>
    <message>
      <source>Could not start the update.</source>
      <translation>更新を開始できませんでした。</translation>
    </message>
  </context>
  <context>
    <name>main</name>
    <message>
      <source>Cannot install the network driver: %1</source>
      <translation>ネットワークドライバーをインストールできません：%1</translation>
    </message>
    <message>
      <source>Cannot uninstall the network driver: %1</source>
      <translation>ネットワークドライバーをアンインストールできません：%1</translation>
    </message>
  </context>
</TS>