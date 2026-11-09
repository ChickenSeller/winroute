import { defineConfig } from 'vitepress'

// The guide is published under the homepage, at <homepage>/guide/. The pages
// job passes the path through DOCS_BASE; the default is the GitLab Pages path.
const base = process.env.DOCS_BASE || '/kaguya/winroute/guide/'
const home = process.env.DOCS_HOME || 'https://pages.yanlei.org/kaguya/winroute/'

export default defineConfig({
  lang: 'zh-CN',
  title: 'WinRoute 使用说明',
  description: 'WinRoute 的使用说明：让任何 Windows 程序按规则走代理。',
  base,
  head: [
    ['link', { rel: 'icon', type: 'image/svg+xml', href: `${base}logo.svg` }],
    ['meta', { name: 'theme-color', content: '#2d7dd2' }],
  ],
  lastUpdated: false,
  themeConfig: {
    logo: '/logo.svg',
    siteTitle: 'WinRoute',
    nav: [
      { text: '使用说明', link: '/getting-started', activeMatch: '^/(?!$)' },
      { text: '下载', link: `${home}releases.html` },
      { text: '主页', link: home },
    ],
    sidebar: [
      {
        text: '开始',
        items: [
          { text: '简介', link: '/' },
          { text: '快速上手', link: '/getting-started' },
          { text: '安装与网络驱动', link: '/install' },
          { text: '主窗口', link: '/main-window' },
        ],
      },
      {
        text: '代理与规则',
        items: [
          { text: '上游代理', link: '/proxies' },
          { text: '代理链', link: '/chains' },
          { text: '代理规则', link: '/rules' },
          { text: '域名解析', link: '/dns' },
          { text: '高级选项', link: '/advanced' },
          { text: '代理检测', link: '/checker' },
        ],
      },
      {
        text: '抓包与网关',
        items: [
          { text: '抓包', link: '/capture' },
          { text: '解密 HTTPS', link: '/https' },
          { text: '代理网关', link: '/gateway' },
        ],
      },
      {
        text: '其他',
        items: [
          { text: '配置文件', link: '/profiles' },
          { text: '日志', link: '/log' },
          { text: '界面、托盘与更新', link: '/settings' },
          { text: '命令行', link: '/cli' },
          { text: '常见问题', link: '/faq' },
        ],
      },
    ],
    outline: { level: [2, 3], label: '本页目录' },
    docFooter: { prev: '上一页', next: '下一页' },
    darkModeSwitchLabel: '外观',
    lightModeSwitchTitle: '切换到浅色模式',
    darkModeSwitchTitle: '切换到深色模式',
    sidebarMenuLabel: '目录',
    returnToTopLabel: '回到顶部',
    notFound: { title: '页面不存在', quote: '这个页面不存在或已经移走。', linkText: '回到首页' },
    search: {
      provider: 'local',
      options: {
        translations: {
          button: { buttonText: '搜索', buttonAriaLabel: '搜索' },
          modal: {
            displayDetails: '显示详情',
            resetButtonTitle: '清除',
            backButtonTitle: '关闭',
            noResultsText: '没有找到结果',
            footer: { selectText: '选择', navigateText: '切换', closeText: '关闭' },
          },
        },
      },
    },
  },
})
