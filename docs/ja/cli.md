---
description: WinRoute.exe のコマンドラインオプション。
---

# コマンドライン

| オプション | 機能 |
| --- | --- |
| `--minimized` | システムトレイに最小化した状態で起動します。自動起動で使用されます |
| `--install-driver=demo\|a\|b` | 指定した NetFilter ドライバーをインストールして開始します |
| `--uninstall-driver` | NetFilter ドライバーを停止してアンインストールし、WinDivert サービスも削除します |
| `--quiet` | 上記 2 つのオプションと組み合わせて使用します。ダイアログを表示せず、結果を終了コードのみで返します（0 は成功、1 は失敗） |

例：

```
"C:\Program Files\WinRoute\WinRoute.exe" --install-driver=a --quiet
```

これらのコマンドは管理者として実行する必要があります。
