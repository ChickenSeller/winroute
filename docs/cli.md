---
description: WinRoute.exe 的命令行参数。
---

# 命令行

| 参数 | 功能 |
| --- | --- |
| `--minimized` | 启动后直接最小化到系统托盘，用于开机自动启动 |
| `--install-driver=demo\|a\|b` | 安装并启动指定的 NetFilter 驱动 |
| `--uninstall-driver` | 停止并卸载 NetFilter 驱动，同时移除 WinDivert 服务 |
| `--quiet` | 与上述两个参数配合使用：不显示对话框，仅以退出代码表示结果（0 为成功，1 为失败） |

示例：

```
"C:\Program Files\WinRoute\WinRoute.exe" --install-driver=a --quiet
```

上述命令须以管理员身份运行。
