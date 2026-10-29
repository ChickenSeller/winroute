; WinRoute installer (Inno Setup 6).
; Built by the CMake "installer" target, which stages the deployed files and calls:
;   ISCC /DAppVersion=x.y.z /DStageDir=<dir> /DOutputDir=<dir> WinRoute.iss

#ifndef AppVersion
  #define AppVersion "0.5.4"
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
; x64 Windows only: the driver is an x64 build, and a kernel driver does not
; run under the x64 emulation of Windows on Arm.
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
; Qt 6 needs Windows 10 1809 or later.
MinVersion=10.0.17763
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
en.DriverLabel=WinRoute captures connections with a NetFilter SDK driver or with WinDivert. Choose one, then click Next. You can switch later in Tools > Network Driver.
zh_CN.DriverLabel=WinRoute 通过 NetFilter SDK 驱动或 WinDivert 拦截网络连接。请选择其一，然后点击“下一步”。之后也可在“工具 > 网络驱动”中切换。
ja.DriverLabel=WinRoute は NetFilter SDK ドライバーまたは WinDivert で接続を捕捉します。いずれかを選んで「次へ」をクリックしてください。後で「ツール > ネットワークドライバー」から切り替えられます。
en.DriverDemo=NetFilter SDK demo driver
zh_CN.DriverDemo=NetFilter SDK 演示驱动
ja.DriverDemo=NetFilter SDK デモドライバー
en.DriverA=Driver A
zh_CN.DriverA=驱动 A
ja.DriverA=ドライバー A
en.DriverB=Driver B
zh_CN.DriverB=驱动 B
ja.DriverB=ドライバー B
en.DriverDemoNote=The evaluation driver from the NetFilter SDK (version 1.7.2.9). It stops capturing connections after a limited number of them, until Windows restarts.
zh_CN.DriverDemoNote=NetFilter SDK 自带的评估版驱动（版本 1.7.2.9）。拦截的连接数达到上限后会停止工作，重启 Windows 后恢复。
ja.DriverDemoNote=NetFilter SDK に付属する評価版ドライバー（バージョン 1.7.2.9）です。捕捉した接続数が上限に達すると、Windows を再起動するまで動作しません。
en.DriverANote=A release build of the driver (version 1.7.1.8), without a connection limit.
zh_CN.DriverANote=正式版驱动（版本 1.7.1.8），没有连接数限制。
ja.DriverANote=製品版ドライバー（バージョン 1.7.1.8）です。接続数の制限はありません。
en.DriverBNote=A release build of the driver, without a connection limit.
zh_CN.DriverBNote=正式版驱动，没有连接数限制。
ja.DriverBNote=製品版ドライバーです。接続数の制限はありません。
en.DriverWinDivert=WinDivert (TCP + UDP)
zh_CN.DriverWinDivert=WinDivert（TCP + UDP）
ja.DriverWinDivert=WinDivert（TCP + UDP）
en.DriverWinDivertNote=An alternative backend based on WinDivert. Captures TCP and UDP at the packet level and installs its own driver on first use.
zh_CN.DriverWinDivertNote=基于 WinDivert 的备选后端。在数据包层面拦截 TCP 和 UDP，首次使用时自行安装其驱动。
ja.DriverWinDivertNote=WinDivert ベースの代替バックエンド。パケットレベルで TCP と UDP を捕捉し、初回使用時に自身のドライバーをインストールします。
en.DriverReady=Network driver:
zh_CN.DriverReady=网络驱动：
ja.DriverReady=ネットワークドライバー：

; No GroupDescription: the Tasks page is shown through our own checkboxes (see
; [Code]), which index the list items directly, so group headers must not shift them.
[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; Flags: unchecked
Name: "autostart"; Description: "{cm:AutostartTask}"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; Excludes: "\drivers"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Every driver is shipped under {app}\drivers\<key>\; WinRoute installs the one
; chosen on the Network Driver page (--install-driver=<key>) from there, and
; Tools > Network Driver can switch between them later. No copy is placed next
; to WinRoute.exe.
Source: "{#StageDir}\drivers\*"; DestDir: "{app}\drivers"; Flags: ignoreversion recursesubdirs createallsubdirs

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
// The Network Driver page: a drop-down with the drivers, the demo one by
// default. An upgrade keeps the previous choice; /DRIVER=demo|a|b on the
// command line picks one for silent installs.
var
  DriverPage: TWizardPage;
  DriverCombo: TNewComboBox;
  DriverNote: TNewStaticText;
  // Own checkboxes for the Tasks page: the modern wizard clips the left edge of
  // the TNewCheckListBox's own checkboxes, so we hide it and mirror its items.
  DesktopCheck, AutostartCheck: TNewCheckBox;

function DriverKey(Index: Integer): String;
begin
  case Index of
    1: Result := 'a';
    2: Result := 'b';
    3: Result := 'windivert';
  else
    Result := 'demo';
  end;
end;

function DriverIndex(Key: String): Integer;
begin
  if Key = 'a' then Result := 1
  else if Key = 'b' then Result := 2
  else if Key = 'windivert' then Result := 3
  else Result := 0;
end;

function DriverName(Key: String): String;
begin
  if Key = 'a' then Result := CustomMessage('DriverA')
  else if Key = 'b' then Result := CustomMessage('DriverB')
  else if Key = 'windivert' then Result := CustomMessage('DriverWinDivert')
  else Result := CustomMessage('DriverDemo');
end;

function DriverNoteText(Key: String): String;
begin
  if Key = 'a' then Result := CustomMessage('DriverANote')
  else if Key = 'b' then Result := CustomMessage('DriverBNote')
  else if Key = 'windivert' then Result := CustomMessage('DriverWinDivertNote')
  else Result := CustomMessage('DriverDemoNote');
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
  DriverNote.Caption := DriverNoteText(SelectedDriver());
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
  DriverCombo.Items.Add(DriverName('a'));
  DriverCombo.Items.Add(DriverName('b'));
  DriverCombo.Items.Add(DriverName('windivert'));
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
  DriverCombo.ItemIndex := DriverIndex(Initial);
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
  if not Exec(ExpandConstant('{app}\WinRoute.exe'), '--install-driver=' + SelectedDriver() + ' --quiet', '', SW_HIDE,
              ewWaitUntilTerminated, ResultCode) or (ResultCode <> 0) then
    SuppressibleMsgBox(FmtMessage(CustomMessage('DriverFailed'), [IntToStr(ResultCode)]),
                       mbError, MB_OK, IDOK);
end;

// On the Tasks page, hide the (left-clipped) check-list and show our own
// checkboxes mirroring its items. The underlying tasks still drive [Icons]/[Run]
// and silent /TASKS; we only change how they are displayed and sync on Next.
procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID <> wpSelectTasks then
    Exit;
  if DesktopCheck = nil then
  begin
    WizardForm.TasksList.Visible := False;

    DesktopCheck := TNewCheckBox.Create(WizardForm);
    DesktopCheck.Parent := WizardForm.TasksList.Parent;
    DesktopCheck.Left := WizardForm.TasksList.Left;
    DesktopCheck.Top := WizardForm.TasksList.Top;
    DesktopCheck.Width := WizardForm.TasksList.Width;
    DesktopCheck.Height := ScaleY(18);
    DesktopCheck.Caption := WizardForm.TasksList.ItemCaption[0];

    AutostartCheck := TNewCheckBox.Create(WizardForm);
    AutostartCheck.Parent := WizardForm.TasksList.Parent;
    AutostartCheck.Left := WizardForm.TasksList.Left;
    AutostartCheck.Top := DesktopCheck.Top + ScaleY(28);
    AutostartCheck.Width := WizardForm.TasksList.Width;
    AutostartCheck.Height := ScaleY(18);
    AutostartCheck.Caption := WizardForm.TasksList.ItemCaption[1];
  end;
  DesktopCheck.Checked := WizardForm.TasksList.Checked[0];
  AutostartCheck.Checked := WizardForm.TasksList.Checked[1];
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if (CurPageID = wpSelectTasks) and (DesktopCheck <> nil) then
  begin
    WizardForm.TasksList.Checked[0] := DesktopCheck.Checked;
    WizardForm.TasksList.Checked[1] := AutostartCheck.Checked;
  end;
  Result := True;
end;

// {app}\WinDivert64.sys is held open by the "WinDivert" kernel service while it
// runs; WinRoute starts it for the WinDivert backend and leaves it installed, so
// an in-place upgrade cannot replace the file (DeleteFile fails with access
// denied). Stop and remove the service before the files are copied - WinRoute
// reinstalls it automatically the next time it uses that backend. "net stop"
// waits for the driver to unload, which releases the file.
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Result := '';
  Exec(ExpandConstant('{sys}\net.exe'), 'stop WinDivert', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Exec(ExpandConstant('{sys}\sc.exe'), 'delete WinDivert', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
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
      DeleteFile(ExpandConstant('{app}\WinRoute.ini'));   // portable settings file
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\WinRoute');   // settings of versions before the INI
    end;
end;
