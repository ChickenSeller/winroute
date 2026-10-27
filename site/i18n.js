// English is in the page itself; this holds the Chinese and Japanese text and the switch.
(function () {
  var text = {
    zh: {
      'nav.features': '功能',
      'nav.releases': '版本',
      'nav.how': '工作原理',
      'nav.support': '支持情况',
      'nav.build': '构建',
      'hero.title': '让任何 Windows 程序走代理。',
      'hero.lead': '很多程序自己没有代理设置。WinRoute 接管它们的网络连接，按你写的规则转发到 SOCKS 或 HTTPS 代理，或者多个代理组成的代理链。Proxifier 的配置文件可以直接用。',
      'hero.download': '下载',
      'hero.source': '源代码',
      'hero.meta': 'Windows 10 和 11，x64 · English、简体中文、日本語',
      'dl.site': '从本站下载',
      'dl.github': '从 GitHub 下载',
      'dl.gitlab': '从 GitLab 下载',
      'shot.connections': '连接',
      'shot.traffic': '流量',
      'shot.statistics': '统计',
      'shot.app': '应用程序',
      'shot.target': '目标',
      'shot.rule': '规则 : 代理',
      'shot.received': '已接收',
      'features.title': 'Proxifier 有的功能，一个新程序',
      'f.rules.t': '按程序设置规则',
      'f.rules.d': '按程序（名称、路径或通配符）、目标主机、IP 范围或端口匹配，让连接走代理、直连或者阻止。规则从上到下依次匹配。',
      'f.proxies.t': 'SOCKS 和 HTTPS 代理',
      'f.proxies.d': 'SOCKS4、SOCKS4A、带用户名密码的 SOCKS5，以及支持 CONNECT 和 Basic 认证的 HTTPS 代理，IPv4 和 IPv6 都可以。',
      'f.chains.t': '代理链',
      'f.chains.d': '依次经过多个代理；一个失败时自动换下一个；或者把连接随机分配到多个代理上。',
      'f.dns.t': '通过代理解析域名',
      'f.dns.d': '由代理来解析主机名，本地网络看不到你访问的域名；程序即使按 IP 连接，也能按域名写规则。',
      'f.monitor.t': '实时连接和流量',
      'f.monitor.d': '每个连接的程序、目标、规则和流量一目了然，另有流量图、按程序的统计和日志。一键断开连接或结束进程。',
      'f.profiles.t': 'Proxifier 配置文件',
      'f.profiles.d': '直接读写 Proxifier 的 .ppx 文件，首次启动时自动导入你的 Proxifier 配置。可以在托盘里切换配置文件。',
      'f.checker.t': '代理检测',
      'f.checker.d': '逐步测试一个代理或整条代理链：能否连上代理、能否通过它连接目标、HTTP 请求是否正常。',
      'f.lang.t': '中文、日文和英文',
      'f.lang.d': '程序、日志和安装器都跟随 Windows 的语言，也可以自己选。',
      'how.title': '工作原理',
      'how.lead': '不修改任何程序，也不向程序注入代码。网络驱动在每个新连接离开本机之前拦下它，由 WinRoute 决定它去哪里。',
      'how.s1.t': '拦截',
      'how.s1.d': '网络驱动——NetFilter SDK 的 WFP 驱动，或 WinDivert——报告每一个向外的 TCP 连接（开启后还包括 UDP），以及发起它的程序。',
      'how.s2.t': '判断',
      'how.s2.d': '第一条与程序、主机、端口相符的规则决定怎么处理：走某个代理、走代理链、直连，或者阻止。',
      'how.s3.t': '重定向',
      'how.s3.d': '要走代理的连接被指向 WinRoute 的本地中继，直连的连接原样放行。',
      'how.s4.t': '转发',
      'how.s4.d': '中继用 SOCKS 或 HTTPS 协议与代理握手，然后转发数据并统计流量。',
      'how.note': 'WinRoute 自己的连接不会被拦截；连向你的代理服务器本身的连接会直接放行，所以不会形成环路。',
      'sp.title': '支持情况',
      'sp.status': '状态',
      'sp.socks5': 'SOCKS5，支持用户名和密码',
      'sp.https': 'HTTPS（CONNECT），支持 Basic 认证',
      'sp.chains': '简单链、冗余链和负载均衡链',
      'sp.dns': '通过代理解析主机名',
      'sp.ipv6': 'IPv6 目标',
      'sp.ppx': 'Proxifier .ppx 配置文件',
      'sp.services': 'Windows 服务和其他用户（可选）',
      'sp.udp': '通过 SOCKS5 转发 UDP',
      'sp.ntlm': 'NTLM 和 Kerberos 代理认证',
      'sp.planned': '计划中',
      'b.title': '从源码构建',
      'b.lead': '需要 Qt 6（MinGW 64 位）、CMake 3.21，以及 NetFilter SDK 和已签名的 netfilter2.sys（这两样不在仓库里）。安装器用 Inno Setup 6 生成。',
      'foot.readme': '文档',
      'foot.driver': '网络驱动：NetFilter SDK 或 WinDivert',
      'foot.icons': '图标：Material Symbols',
      'rel.title': '版本',
      'rel.lead': '从本站下载各个版本的 Windows 安装包，与 GitLab 上的发布内容相同。',
      'rel.elsewhere': '也可以从这里下载：',
      'rel.loading': '正在读取版本列表...',
      'rel.none': '还没有发布任何版本。',
      'rel.error': '暂时无法读取版本列表，可以先到 GitLab 下载。',
      'rel.latest': '最新',
      'rel.download': '下载 Windows 安装包'
    },
    ja: {
      'nav.features': '機能',
      'nav.releases': 'リリース',
      'nav.how': 'しくみ',
      'nav.support': '対応状況',
      'nav.build': 'ビルド',
      'hero.title': 'どの Windows アプリもプロキシ経由に。',
      'hero.lead': 'プロキシ設定を持たないプログラムは少なくありません。WinRoute はそうしたプログラムの接続を捕捉し、決めたルールに従って SOCKS / HTTPS プロキシや、複数のプロキシをつないだチェーンに転送します。Proxifier のプロファイルはそのまま使えます。',
      'hero.download': 'ダウンロード',
      'hero.source': 'ソースコード',
      'hero.meta': 'Windows 10 / 11、x64 · English、简体中文、日本語',
      'dl.site': 'このサイトから',
      'dl.github': 'GitHub から',
      'dl.gitlab': 'GitLab から',
      'shot.connections': '接続',
      'shot.traffic': 'トラフィック',
      'shot.statistics': '統計',
      'shot.app': 'アプリケーション',
      'shot.target': '接続先',
      'shot.rule': 'ルール : プロキシ',
      'shot.received': '受信',
      'features.title': 'Proxifier の機能を、新しいアプリで',
      'f.rules.t': 'アプリごとのルール',
      'f.rules.d': 'プログラム（名前、パス、ワイルドカード）、接続先ホスト、IP 範囲、ポートで判定し、プロキシ経由・直接接続・ブロックを選べます。ルールは上から順に適用されます。',
      'f.proxies.t': 'SOCKS と HTTPS プロキシ',
      'f.proxies.d': 'SOCKS4、SOCKS4A、ユーザー名とパスワード付きの SOCKS5、CONNECT と Basic 認証に対応した HTTPS プロキシ。IPv4 と IPv6 の両方に対応します。',
      'f.chains.t': 'プロキシチェーン',
      'f.chains.d': '複数のプロキシを順に経由したり、失敗したら次のプロキシに切り替えたり、接続をランダムに振り分けたりできます。',
      'f.dns.t': 'プロキシ経由の名前解決',
      'f.dns.d': 'ホスト名の解決をプロキシに任せるので、ローカルネットワークにアクセス先の名前が漏れません。プログラムが IP で接続しても、ホスト名でルールを書けます。',
      'f.monitor.t': '接続とトラフィックをリアルタイムに',
      'f.monitor.d': '接続ごとのプログラム、接続先、ルール、転送量に加え、トラフィックのグラフ、プログラム別の統計、ログを表示します。接続の切断やプロセスの終了もワンクリックです。',
      'f.profiles.t': 'Proxifier のプロファイル',
      'f.profiles.d': 'Proxifier の .ppx ファイルを読み書きでき、初回起動時には Proxifier のプロファイルを取り込みます。プロファイルはトレイから切り替えられます。',
      'f.checker.t': 'プロキシチェッカー',
      'f.checker.d': 'プロキシやチェーン全体を段階的にテストします。プロキシへの接続、プロキシ経由の接続、HTTP 要求の順に確認します。',
      'f.lang.t': '英語、中国語、日本語',
      'f.lang.d': 'アプリ、ログ、インストーラーは Windows の言語に合わせて表示されます。言語を選ぶこともできます。',
      'how.title': 'しくみ',
      'how.lead': 'プログラムを書き換えたり、コードを注入したりはしません。ネットワークドライバーが新しい接続を送出前に捕捉し、行き先を WinRoute が決めます。',
      'how.s1.t': '捕捉',
      'how.s1.d': 'ネットワークドライバー（NetFilter SDK の WFP ドライバー、または WinDivert）が、外向きの TCP 接続（有効にすれば UDP も）と、それを開いたプログラムを通知します。',
      'how.s2.t': '判定',
      'how.s2.d': 'プログラム・ホスト・ポートに最初に一致したルールが、プロキシ、チェーン、直接接続、ブロックのどれにするかを決めます。',
      'how.s3.t': 'リダイレクト',
      'how.s3.d': 'プロキシ経由にする接続は WinRoute のローカルリレーに向け直し、直接接続はそのまま通します。',
      'how.s4.t': '中継',
      'how.s4.d': 'リレーが SOCKS / HTTPS でプロキシとやり取りし、データを転送しながら転送量を数えます。',
      'how.note': 'WinRoute 自身の接続は捕捉されず、プロキシサーバーそのものへの接続は直接接続になるため、ループは起きません。',
      'sp.title': '対応状況',
      'sp.status': '状態',
      'sp.socks5': 'SOCKS5（ユーザー名とパスワード付き）',
      'sp.https': 'HTTPS（CONNECT、Basic 認証付き）',
      'sp.chains': 'シンプル、冗長化、負荷分散の各チェーン',
      'sp.dns': 'プロキシ経由のホスト名解決',
      'sp.ipv6': 'IPv6 の接続先',
      'sp.ppx': 'Proxifier の .ppx プロファイル',
      'sp.services': 'Windows サービスと他のユーザー（任意）',
      'sp.udp': 'SOCKS5 経由の UDP',
      'sp.ntlm': 'NTLM / Kerberos によるプロキシ認証',
      'sp.planned': '予定',
      'b.title': 'ソースからビルド',
      'b.lead': 'Qt 6（MinGW、64 ビット）、CMake 3.21、NetFilter SDK と署名済みの netfilter2.sys が必要です（後者 2 つはリポジトリに含まれません）。インストーラーは Inno Setup 6 で作成します。',
      'foot.readme': 'ドキュメント',
      'foot.driver': 'ネットワークドライバー：NetFilter SDK または WinDivert',
      'foot.icons': 'アイコン：Material Symbols',
      'rel.title': 'リリース',
      'rel.lead': '各バージョンの Windows インストーラーをこのサイトからダウンロードできます。GitLab のリリースと同じ内容です。',
      'rel.elsewhere': 'こちらからも入手できます：',
      'rel.loading': 'リリース一覧を読み込んでいます...',
      'rel.none': 'まだリリースはありません。',
      'rel.error': '現在リリース一覧を読み込めません。GitLab からダウンロードしてください。',
      'rel.latest': '最新',
      'rel.download': 'Windows インストーラーをダウンロード'
    }
  };
  var names = {en: 'English', zh: '简体中文', ja: '日本語'};
  var htmlLang = {en: 'en', zh: 'zh-CN', ja: 'ja'};

  var button = document.getElementById('lang');
  var menu = button.parentNode;

  // Rescans the page each time, so parts added later (the release list) are
  // translated too; each element keeps its English text in data-en.
  function apply(lang) {
    var table = text[lang] || {};
    var nodes = document.querySelectorAll('[data-i18n]');
    for (var i = 0; i < nodes.length; i++) {
      var node = nodes[i];
      if (node.dataset.en === undefined) node.dataset.en = node.textContent;
      node.textContent = table[node.getAttribute('data-i18n')] || node.dataset.en;
    }
    document.documentElement.lang = htmlLang[lang];
    button.textContent = names[lang];
    var items = menu.querySelectorAll('[data-lang]');
    for (var j = 0; j < items.length; j++)
      items[j].setAttribute('aria-current', items[j].getAttribute('data-lang') === lang ? 'true' : 'false');
    try { localStorage.setItem('winroute-lang', lang); } catch (e) {}
  }

  function preferred() {
    var saved = null;
    try { saved = localStorage.getItem('winroute-lang'); } catch (e) {}
    if (names[saved]) return saved;
    var langs = navigator.languages || [navigator.language || ''];
    for (var i = 0; i < langs.length; i++) {
      if (/^zh/i.test(langs[i])) return 'zh';
      if (/^ja/i.test(langs[i])) return 'ja';
      if (/^en/i.test(langs[i])) return 'en';
    }
    return 'en';
  }

  var lang = preferred();
  apply(lang);

  menu.addEventListener('click', function (event) {
    var item = event.target.closest('[data-lang]');
    if (!item) return;
    event.preventDefault();
    lang = item.getAttribute('data-lang');
    apply(lang);
    menu.open = false;
  });
  window.winrouteRetranslate = function () { apply(lang); };

  // Drop-down menus (download, language, the release list's) close on a click
  // elsewhere or Escape; one is open at a time.
  function openMenus() {
    return Array.prototype.slice.call(document.querySelectorAll('details.dropdown[open]'));
  }
  document.addEventListener('click', function (event) {
    openMenus().forEach(function (open) {
      if (!open.contains(event.target)) open.open = false;
    });
  });
  document.addEventListener('keydown', function (event) {
    if (event.key !== 'Escape') return;
    openMenus().forEach(function (open) {
      open.open = false;
      open.querySelector('summary').focus();
    });
  });
})();
