#define MyAppID "CypherpunkPrivacy"
#define MyAppName "Cypherpunk Privacy"
#define MyAppPublisher "Cypherpunk Partners, slf."
#define MyAppURL "https://cypherpunk.com/"
#define MyAppExeName "CypherpunkPrivacy.exe"
#define MyAppCopyright "Copyright © 2017 " + MyAppPublisher
#define MyInstallerName "cypherpunk-privacy-windows"

#define MyAppVersion "0.9.0-preview"
#define MyAppNumericVersion "0.9.0"
#define MyInstallerSuffix "-0.9.0-preview"

[Setup]
AppId={#MyAppID}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
PrivilegesRequired=admin
DisableProgramGroupPage=yes
OutputDir=..\..\out\win\
OutputBaseFilename={#MyInstallerName}{#MyInstallerSuffix}
LicenseFile=..\..\LICENSE.txt
SetupIconFile=..\..\res\win\logo3.ico
WizardSmallImageFile=..\..\res\win\installer_banner.bmp
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={uninstallexe}
Compression=lzma/ultra
SolidCompression=yes
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64
AppCopyright={#MyAppCopyright}
TimeStampsInUTC=True
DisableDirPage=yes
ShowLanguageDialog=no
CloseApplications=False
RestartApplications=False
CloseApplicationsFilter=*.exe,*.dll
VersionInfoVersion={#MyAppNumericVersion}
VersionInfoTextVersion={#MyAppVersion}
MinVersion=0,6.0
#ifdef ENABLE_SIGNING
SignTool=standard
SignedUninstaller=yes
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
;Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: checkedonce

[Files]
; Directories
Source: "..\..\out\win\client\CypherpunkPrivacy-win32-ia32\*"; DestDir: "{app}"; Flags: 32bit createallsubdirs overwritereadonly recursesubdirs
Source: "..\..\out\win\daemon\Release\32\openvpn\*"; DestDir: "{app}\openvpn"; Flags: ignoreversion recursesubdirs createallsubdirs overwritereadonly; Check: not Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\64\openvpn\*"; DestDir: "{app}\openvpn"; Flags: ignoreversion recursesubdirs createallsubdirs overwritereadonly; Check: Is64BitInstallMode
; Binaries
Source: "..\..\out\win\client\CypherpunkPrivacy-win32-ia32\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion overwritereadonly sign; BeforeInstall: StopClient
Source: "..\..\out\win\client\CypherpunkPrivacy-win32-ia32\*.exe"; DestDir: "{app}"; Flags: ignoreversion overwritereadonly sign
Source: "..\..\out\win\client\CypherpunkPrivacy-win32-ia32\*.dll"; DestDir: "{app}"; Flags: ignoreversion overwritereadonly sign
Source: "..\..\out\win\daemon\Release\32\cypherpunk-privacy-service.exe"; DestDir: "{app}"; DestName: "cypherpunk-privacy-service.exe"; Flags: ignoreversion overwritereadonly sign; Check: not Is64BitInstallMode; BeforeInstall: StopService
Source: "..\..\out\win\daemon\Release\64\cypherpunk-privacy-service.exe"; DestDir: "{app}"; DestName: "cypherpunk-privacy-service.exe"; Flags: ignoreversion overwritereadonly sign; Check: Is64BitInstallMode; BeforeInstall: StopService
Source: "..\..\out\win\daemon\Release\32\openvpn\*.exe"; DestDir: "{app}\openvpn"; Flags: ignoreversion overwritereadonly sign; Check: not Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\32\openvpn\*.dll"; DestDir: "{app}\openvpn"; Flags: ignoreversion overwritereadonly sign; Check: not Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\64\openvpn\*.exe"; DestDir: "{app}\openvpn"; Flags: ignoreversion overwritereadonly sign; Check: Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\64\openvpn\*.dll"; DestDir: "{app}\openvpn"; Flags: ignoreversion overwritereadonly sign; Check: Is64BitInstallMode
; TAP adapter files
Source: "..\..\daemon\third_party\tuntap_win\32\OemVista.inf"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly; Check: not Is64BitInstallMode
Source: "..\..\daemon\third_party\tuntap_win\32\tap91337.sys"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly; Check: not Is64BitInstallMode; OnlyBelowVersion: 10.0
Source: "..\..\daemon\third_party\tuntap_win\32\tap91337.cat"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly; Check: not Is64BitInstallMode; OnlyBelowVersion: 10.0
Source: "..\..\daemon\third_party\tuntap_win\32\10\tap91337.sys"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly; Check: not Is64BitInstallMode; MinVersion: 10.0
Source: "..\..\daemon\third_party\tuntap_win\32\10\tap91337.cat"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly; Check: not Is64BitInstallMode; MinVersion: 10.0
Source: "..\..\daemon\third_party\tuntap_win\32\tapinstall.exe"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly signonce; Check: not Is64BitInstallMode
Source: "..\..\daemon\third_party\tuntap_win\64\OemVista.inf"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly; Check: Is64BitInstallMode
Source: "..\..\daemon\third_party\tuntap_win\64\tap91337.sys"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly; Check: Is64BitInstallMode; OnlyBelowVersion: 10.0
Source: "..\..\daemon\third_party\tuntap_win\64\tap91337.cat"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly; Check: Is64BitInstallMode; OnlyBelowVersion: 10.0
Source: "..\..\daemon\third_party\tuntap_win\64\10\tap91337.sys"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly; Check: Is64BitInstallMode; MinVersion: 10.0
Source: "..\..\daemon\third_party\tuntap_win\64\10\tap91337.cat"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly; Check: Is64BitInstallMode; MinVersion: 10.0
Source: "..\..\daemon\third_party\tuntap_win\64\tapinstall.exe"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly signonce; Check: Is64BitInstallMode

[PreCompile]
;Name: "build.bat"; Flags: abortonerror cmdprompt

[Code]
var ResultCode: Integer;
procedure StopService();
begin
    Exec('net.exe', 'stop CypherpunkPrivacyService', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;
procedure StopClient();
begin
    Exec('taskkill.exe', ExpandConstant('/im {#MyAppExeName} /f'), ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;
function IsUpgrade: Boolean;
var
  S: string;
  InnoSetupReg: string;
begin
  InnoSetupReg :=
    ExpandConstant(
      'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#SetupSetting("AppId")}_is1');
  Result :=
    RegQueryStringValue(HKLM, InnoSetupReg, 'DisplayVersion', S) or
    RegQueryStringValue(HKCU, InnoSetupReg, 'DisplayVersion', S);
end;
function IsPreviewUpgrade: Boolean;
var
  S: string;
  InnoSetupReg: string;
begin
  InnoSetupReg :=
    ExpandConstant(
      'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#SetupSetting("AppId")}_is1');
  Result :=
    (RegQueryStringValue(HKLM, InnoSetupReg, 'DisplayVersion', S) or
     RegQueryStringValue(HKCU, InnoSetupReg, 'DisplayVersion', S)) and
	(pos('0.9.0-preview', S) = 1);
end;

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
;Name: "{commonstartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Parameters: "--background"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Comment: "{#MyAppName}"

[Registry]
Root: HKCU; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "{#MyAppID}"; ValueData: """{app}\{#MyAppExeName}"" --background"; Flags: uninsdeletevalue

[InstallDelete]
;Type: filesandordirs; Name: "{app}"

[Run]
; Temporary item to clean up old generic TAP adapter from previous install
Filename: "{app}\tap\tapinstall.exe"; Parameters: "remove tap0901"; WorkingDir: "{app}\tap"; Flags: runhidden skipifdoesntexist; StatusMsg: "Removing previous network adapter..."; Check: IsPreviewUpgrade
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "addtap 1"; WorkingDir: "{app}"; Flags: runhidden; StatusMsg: "Installing network adapter..."
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "install"; WorkingDir: "{app}"; Flags: runhidden; StatusMsg: "Installing background service..."
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "start"; WorkingDir: "{app}"; Flags: runhidden; StatusMsg: "Starting background service..."
Filename: "{app}\{#MyAppExeName}"; Parameters: "--first"; WorkingDir: "{app}"; Flags: postinstall skipifsilent nowait; Description: "Launch {#MyAppName}"

[UninstallRun]
Filename: "taskkill.exe"; Parameters: "/im {#MyAppExeName} /f"; WorkingDir: "{app}"; Flags: runhidden
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "stop"; WorkingDir: "{app}"; Flags: runhidden
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "uninstall"; WorkingDir: "{app}"; Flags: runhidden
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "removetap"; WorkingDir: "{app}"; Flags: runhidden

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
Type: filesandordirs; Name: "{userappdata}\{#MyAppName}"
Type: filesandordirs; Name: "{commonappdata}\{#MyAppName}"
Type: filesandordirs; Name: "{userpf}\{#MyAppName}"
