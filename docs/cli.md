---
description: WinRoute.exe 的命令行参数。
---

# 命令行

| 参数 | 作用 |
| --- | --- |
| `--minimized` | 启动后直接到系统托盘，开机自启动时使用 |
| `--install-driver=demo\|a\|b` | 安装并启动指定的 NetFilter 驱动 |
| `--uninstall-driver` | 停止并卸载 NetFilter 驱动，同时移除 WinDivert 服务 |
| `--quiet` | 配合上面两个参数使用：不弹窗，只用退出码表示结果（0 成功，1 失败） |

示例：

```
"C:\Program Files\WinRoute\WinRoute.exe" --install-driver=a --quiet
```

这些命令需要以管理员身份运行。
