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

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart"; Description: "{cm:AutostartTask}"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

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
