import { defineConfig, type DefaultTheme } from 'vitepress'

// The guide is published under the homepage, at <homepage>/guide/, with the
// Chinese pages at the root and the English and Japanese ones under /en/ and
// /ja/. DOCS_BASE and DOCS_HOME can override the paths.
const base = process.env.DOCS_BASE || '/kaguya/winroute/guide/'
const home = process.env.DOCS_HOME || 'https://pages.yanlei.org/kaguya/winroute/'

type Names = Record<string, string>

// One sidebar per language, from the page titles in that language.
function sidebar(prefix: string, groups: [string, Names][]): DefaultTheme.SidebarItem[] {
  return groups.map(([text, pages]) => ({
    text,
    items: Object.entries(pages).map(([page, title]) => ({ text: title, link: `${prefix}/${page}` })),
  }))
}

const zh = sidebar('', [
  ['开始', { '': '简介', 'getting-started': '快速上手', install: '安装与网络驱动', 'main-window': '主窗口' }],
  ['代理与规则', { proxies: '上游代理', chains: '代理链', rules: '代理规则', dns: '域名解析', advanced: '高级选项', checker: '代理检测' }],
  ['抓包与网关', { capture: '抓包', https: '解密 HTTPS', gateway: '代理网关' }],
  ['其他', { profiles: '配置文件', log: '日志', settings: '界面、托盘与更新', cli: '命令行', faq: '常见问题' }],
])
const en = sidebar('/en', [
  ['Basics', { '': 'Introduction', 'getting-started': 'Getting Started', install: 'Installation and Network Driver', 'main-window': 'Main Window' }],
  ['Proxies and Rules', { proxies: 'Upstream Proxies', chains: 'Proxy Chains', rules: 'Proxification Rules', dns: 'Name Resolution', advanced: 'Advanced Options', checker: 'Proxy Checker' }],
  ['Capture and Gateway', { capture: 'Packet Capture', https: 'HTTPS Decryption', gateway: 'Proxy Gateway' }],
  ['More', { profiles: 'Profiles', log: 'Log', settings: 'Interface, Tray and Updates', cli: 'Command Line', faq: 'FAQ' }],
])
const ja = sidebar('/ja', [
  ['基本', { '': 'はじめに', 'getting-started': 'クイックスタート', install: 'インストールとネットワークドライバー', 'main-window': 'メインウィンドウ' }],
  ['プロキシとルール', { proxies: '上流プロキシ', chains: 'プロキシチェーン', rules: 'プロキシ化ルール', dns: '名前解決', advanced: '詳細設定', checker: 'プロキシチェッカー' }],
  ['キャプチャとゲートウェイ', { capture: 'パケットキャプチャ', https: 'HTTPS の復号', gateway: 'プロキシゲートウェイ' }],
  ['その他', { profiles: 'プロファイル', log: 'ログ', settings: '表示、トレイ、更新', cli: 'コマンドライン', faq: 'よくある質問' }],
])

export default defineConfig({
  base,
  head: [
    ['link', { rel: 'icon', type: 'image/svg+xml', href: `${base}logo.svg` }],
    ['meta', { name: 'theme-color', content: '#2d7dd2' }],
  ],
  lastUpdated: false,
  // Dark by default, like the homepage; the switch in the nav bar still
  // changes it, and the choice is remembered.
  appearance: 'dark',
  locales: {
    root: {
      label: '简体中文',
      lang: 'zh-CN',
      title: 'WinRoute 使用说明',
      description: 'WinRoute 使用说明：按规则让 Windows 应用程序经由代理连接。',
      themeConfig: {
        nav: [
          { text: '使用说明', link: '/getting-started', activeMatch: '^/(?!en/|ja/).+' },
          { text: '下载', link: `${home}releases.html` },
          { text: '主页', link: home },
        ],
        sidebar: zh,
        outline: { level: [2, 3], label: '本页目录' },
        docFooter: { prev: '上一页', next: '下一页' },
        darkModeSwitchLabel: '外观',
        lightModeSwitchTitle: '切换到浅色模式',
        darkModeSwitchTitle: '切换到深色模式',
        sidebarMenuLabel: '目录',
        returnToTopLabel: '返回顶部',
        langMenuLabel: '语言',
        notFound: { title: '页面不存在', quote: '所请求的页面不存在或已移动。', linkText: '返回首页' },
      },
    },
    en: {
      label: 'English',
      lang: 'en',
      link: '/en/',
      title: 'WinRoute Guide',
      description: 'WinRoute user guide: route the connections of Windows applications through proxies by rules.',
      themeConfig: {
        nav: [
          { text: 'Guide', link: '/en/getting-started', activeMatch: '^/en/.+' },
          { text: 'Download', link: `${home}releases.html` },
          { text: 'Home', link: home },
        ],
        sidebar: en,
        outline: { level: [2, 3], label: 'On this page' },
      },
    },
    ja: {
      label: '日本語',
      lang: 'ja',
      link: '/ja/',
      title: 'WinRoute ユーザーガイド',
      description: 'WinRoute ユーザーガイド：Windows アプリケーションの接続をルールに従ってプロキシ経由にします。',
      themeConfig: {
        nav: [
          { text: 'ガイド', link: '/ja/getting-started', activeMatch: '^/ja/.+' },
          { text: 'ダウンロード', link: `${home}releases.html` },
          { text: 'ホーム', link: home },
        ],
        sidebar: ja,
        outline: { level: [2, 3], label: '目次' },
        docFooter: { prev: '前のページ', next: '次のページ' },
        darkModeSwitchLabel: '外観',
        lightModeSwitchTitle: 'ライトモードに切り替え',
        darkModeSwitchTitle: 'ダークモードに切り替え',
        sidebarMenuLabel: 'メニュー',
        returnToTopLabel: 'トップに戻る',
        langMenuLabel: '言語',
        notFound: { title: 'ページが見つかりません', quote: '要求されたページは存在しないか、移動されました。', linkText: 'ホームに戻る' },
      },
    },
  },
  themeConfig: {
    logo: '/logo.svg',
    siteTitle: 'WinRoute',
    search: {
      provider: 'local',
      options: {
        locales: {
          root: {
            translations: {
              button: { buttonText: '搜索', buttonAriaLabel: '搜索' },
              modal: {
                displayDetails: '显示详细信息',
                resetButtonTitle: '清除',
                backButtonTitle: '关闭',
                noResultsText: '未找到结果',
                footer: { selectText: '选择', navigateText: '切换', closeText: '关闭' },
              },
            },
          },
          ja: {
            translations: {
              button: { buttonText: '検索', buttonAriaLabel: '検索' },
              modal: {
                displayDetails: '詳細を表示',
                resetButtonTitle: 'クリア',
                backButtonTitle: '閉じる',
                noResultsText: '結果が見つかりません',
                footer: { selectText: '選択', navigateText: '移動', closeText: '閉じる' },
              },
            },
          },
        },
      },
    },
  },
})
