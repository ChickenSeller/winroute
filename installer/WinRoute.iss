; WinRoute installer (Inno Setup 6).
; Built by the CMake "installer" target, which stages the deployed files and calls:
;   ISCC /DAppVersion=x.y.z /DStageDir=<dir> /DOutputDir=<dir> WinRoute.iss

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef StageDir
  #define StageDir "..\build\stage"
#endif
#ifndef OutputDir
  #define OutputDir "..\build"
#endif

[Setup]
AppId={{C51D2EF1-0170-4EE1-A0FA-427817F3A93D}
AppName=WinRoute
AppVersion={#AppVersion}
AppVerName=WinRoute {#AppVersion}
VersionInfoVersion={#AppVersion}
DefaultDirName={autopf}\WinRoute
DefaultGroupName=WinRoute
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#OutputDir}
OutputBaseFilename=WinRoute-{#AppVersion}-setup
SetupIconFile=..\resources\app.ico
UninstallDisplayIcon={app}\WinRoute.exe
UninstallDisplayName=WinRoute
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; Set by WinRoute.exe while it runs; setup and uninstall ask to close it first.
AppMutex=WinRoute.AppMutex
CloseApplications=no
ShowLanguageDialog=auto
LanguageDetectionMethod=uilanguage
; Code signing: define a sign tool in the IDE or pass /S"signtool=..." to ISCC, then uncomment.
;SignTool=signtool
;SignedUninstaller=yes

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "zh_CN"; MessagesFile: "ChineseSimplified.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"

[CustomMessages]
en.AutostartTask=Start WinRoute when I sign in to Windows
zh_CN.AutostartTask=登录 Windows 时自动启动 WinRoute
ja.AutostartTask=Windows へのサインイン時に WinRoute を起動する
en.InstallingDriver=Installing the network driver...
zh_CN.InstallingDriver=正在安装网络驱动...
ja.InstallingDriver=ネットワークドライバーをインストールしています...
en.DriverFailed=The network driver could not be installed (exit code %1).%n%nWinRoute will try to install it again when it starts.
zh_CN.DriverFailed=网络驱动安装失败（退出代码 %1）。%n%nWinRoute 启动时会再次尝试安装。
ja.DriverFailed=ネットワークドライバーをインストールできませんでした（終了コード %1）。%n%nWinRoute の起動時に再度インストールを試みます。
en.RemoveSettings=Do you also want to delete your WinRoute profiles, settings and log files?
zh_CN.RemoveSettings=是否同时删除 WinRoute 的配置文件、设置和日志？
ja.RemoveSettings=WinRoute のプロファイル、設定、ログファイルも削除しますか？
en.DriverPageTitle=Network Driver
zh_CN.DriverPageTitle=网络驱动
ja.DriverPageTitle=ネットワークドライバー
en.DriverPageDescription=Which network driver should WinRoute use?
zh_CN.DriverPageDescription=WinRoute 要使用哪个网络驱动？
ja.DriverPageDescription=WinRoute で使用するネットワークドライバーを選択してください。
en.DriverLabel=WinRoute captures connections with a NetFilter SDK driver. Select the driver to install, then click Next.
zh_CN.DriverLabel=WinRoute 通过 NetFilter SDK 驱动拦截网络连接。请选择要安装的驱动，然后点击“下一步”。
ja.DriverLabel=WinRoute は NetFilter SDK のドライバーで接続を捕捉します。インストールするドライバーを選択して「次へ」をクリックしてください。
en.DriverDemo=NetFilter SDK demo driver
zh_CN.DriverDemo=NetFilter SDK 演示驱动
ja.DriverDemo=NetFilter SDK デモドライバー
en.DriverZhengyun=Driver signed by Sichuan Zhengyun
zh_CN.DriverZhengyun=四川征云签名驱动
ja.DriverZhengyun=四川征雲署名のドライバー
en.DriverDemoNote=The evaluation driver from the NetFilter SDK (version 1.7.2.9). It stops capturing connections after a limited number of them, until Windows restarts.
zh_CN.DriverDemoNote=NetFilter SDK 自带的评估版驱动（版本 1.7.2.9）。拦截的连接数达到上限后会停止工作，重启 Windows 后恢复。
ja.DriverDemoNote=NetFilter SDK に付属する評価版ドライバー（バージョン 1.7.2.9）です。捕捉した接続数が上限に達すると、Windows を再起動するまで動作しません。
en.DriverZhengyunNote=The release build of the driver (version 1.7.1.8), signed by Sichuan Zhengyun Network Technology Co., Ltd., without a connection limit.
zh_CN.DriverZhengyunNote=正式版驱动（版本 1.7.1.8），由四川征云网络科技有限公司签名，没有连接数限制。
ja.DriverZhengyunNote=四川征雲網絡科技有限公司が署名した製品版ドライバー（バージョン 1.7.1.8）です。接続数の制限はありません。
en.DriverReady=Network driver:
zh_CN.DriverReady=网络驱动：
ja.DriverReady=ネットワークドライバー：

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart"; Description: "{cm:AutostartTask}"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; Excludes: "\drivers"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Only the driver chosen on the Network Driver page is installed (WinRoute.exe
; --install-driver puts it in place, replacing a different one).
Source: "{#StageDir}\drivers\demo\netfilter2.sys"; DestDir: "{app}"; Flags: ignoreversion; Check: DriverIs('demo')
Source: "{#StageDir}\drivers\zhengyun\netfilter2.sys"; DestDir: "{app}"; Flags: ignoreversion; Check: DriverIs('zhengyun')

[Icons]
Name: "{autoprograms}\WinRoute"; Filename: "{app}\WinRoute.exe"
Name: "{autodesktop}\WinRoute"; Filename: "{app}\WinRoute.exe"; Tasks: desktopicon

[Run]
; Same scheduled task as File > Autostart in WinRoute (highest privileges, so no UAC prompt at logon).
Filename: "{sys}\schtasks.exe"; Parameters: "/Create /F /RL HIGHEST /SC ONLOGON /TN WinRoute /TR ""\""{app}\WinRoute.exe\"" --minimized"""; Flags: runhidden; Tasks: autostart
; postinstall entries run as the original, non-elevated user by default; WinRoute
; requires administrator rights (CreateProcess would fail with error 740), so it
; inherits Setup's elevated credentials instead.
Filename: "{app}\WinRoute.exe"; Description: "{cm:LaunchProgram,WinRoute}"; Flags: nowait postinstall skipifsilent runascurrentuser

[UninstallRun]
; Runs before the files are removed, while WinRoute.exe and nfapi.dll are still there.
Filename: "{sys}\schtasks.exe"; Parameters: "/Delete /F /TN WinRoute"; Flags: runhidden; RunOnceId: "RemoveAutostart"
Filename: "{app}\WinRoute.exe"; Parameters: "--uninstall-driver --quiet"; Flags: runhidden waituntilterminated; RunOnceId: "RemoveDriver"

[Code]
// The Network Driver page: a drop-down with the two drivers, the demo one by
// default. An upgrade keeps the previous choice; /DRIVER=demo|zhengyun on the
// command line picks one for silent installs.
var
  DriverPage: TWizardPage;
  DriverCombo: TNewComboBox;
  DriverNote: TNewStaticText;

function DriverKey(Index: Integer): String;
begin
  if Index = 1 then Result := 'zhengyun' else Result := 'demo';
end;

function DriverName(Key: String): String;
begin
  if Key = 'zhengyun' then Result := CustomMessage('DriverZhengyun') else Result := CustomMessage('DriverDemo');
end;

function SelectedDriver(): String;
begin
  Result := DriverKey(DriverCombo.ItemIndex);
end;

function DriverIs(Key: String): Boolean;
begin
  Result := SelectedDriver() = Key;
end;

procedure UpdateDriverNote(Sender: TObject);
begin
  if SelectedDriver() = 'zhengyun' then
    DriverNote.Caption := CustomMessage('DriverZhengyunNote')
  else
    DriverNote.Caption := CustomMessage('DriverDemoNote');
end;

procedure InitializeWizard();
var
  Intro: TNewStaticText;
  Initial: String;
begin
  DriverPage := CreateCustomPage(wpSelectTasks, CustomMessage('DriverPageTitle'), CustomMessage('DriverPageDescription'));

  Intro := TNewStaticText.Create(DriverPage);
  Intro.Parent := DriverPage.Surface;
  Intro.AutoSize := False;
  Intro.WordWrap := True;
  Intro.Width := DriverPage.SurfaceWidth;
  Intro.Height := ScaleY(40);
  Intro.Caption := CustomMessage('DriverLabel');

  DriverCombo := TNewComboBox.Create(DriverPage);
  DriverCombo.Parent := DriverPage.Surface;
  DriverCombo.Style := csDropDownList;
  DriverCombo.Top := Intro.Top + Intro.Height + ScaleY(8);
  DriverCombo.Width := DriverPage.SurfaceWidth;
  DriverCombo.Items.Add(DriverName('demo'));
  DriverCombo.Items.Add(DriverName('zhengyun'));
  DriverCombo.OnChange := @UpdateDriverNote;

  DriverNote := TNewStaticText.Create(DriverPage);
  DriverNote.Parent := DriverPage.Surface;
  DriverNote.AutoSize := False;
  DriverNote.WordWrap := True;
  DriverNote.Top := DriverCombo.Top + DriverCombo.Height + ScaleY(12);
  DriverNote.Width := DriverPage.SurfaceWidth;
  DriverNote.Height := ScaleY(60);

  Initial := Lowercase(ExpandConstant('{param:DRIVER|}'));
  if Initial = '' then
    Initial := GetPreviousData('Driver', 'demo');
  if Initial = 'zhengyun' then DriverCombo.ItemIndex := 1 else DriverCombo.ItemIndex := 0;
  UpdateDriverNote(nil);
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  SetPreviousData(PreviousDataKey, 'Driver', SelectedDriver());
end;

// The Ready page lists the chosen driver after Inno's own sections.
function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo,
  MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
begin
  Result := '';
  if MemoDirInfo <> '' then Result := Result + MemoDirInfo + NewLine + NewLine;
  if MemoGroupInfo <> '' then Result := Result + MemoGroupInfo + NewLine + NewLine;
  if MemoTasksInfo <> '' then Result := Result + MemoTasksInfo + NewLine + NewLine;
  Result := Result + CustomMessage('DriverReady') + NewLine + Space + DriverName(SelectedDriver());
end;

procedure InstallDriver();
var
  ResultCode: Integer;
begin
  WizardForm.StatusLabel.Caption := CustomMessage('InstallingDriver');
  if not Exec(ExpandConstant('{app}\WinRoute.exe'), '--install-driver --quiet', '', SW_HIDE,
              ewWaitUntilTerminated, ResultCode) or (ResultCode <> 0) then
    SuppressibleMsgBox(FmtMessage(CustomMessage('DriverFailed'), [IntToStr(ResultCode)]),
                       mbError, MB_OK, IDOK);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    InstallDriver();
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if (CurUninstallStep = usPostUninstall) and not UninstallSilent() then
    if MsgBox(CustomMessage('RemoveSettings'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
    begin
      DelTree(ExpandConstant('{userappdata}\WinRoute'), True, True, True);
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\WinRoute');
    end;
end;
