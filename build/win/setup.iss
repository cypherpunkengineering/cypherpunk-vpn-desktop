#define MyAppID "CypherpunkPrivacy"
#define MyAppName "Cypherpunk Privacy"
#define MyAppPublisher "Cypherpunk Partners, slf."
#define MyAppURL "https://cypherpunk.com/"
#define MyAppExeName "CypherpunkPrivacy.exe"
#define MyAppCopyright "Copyright © 2017 " + MyAppPublisher
#define MyInstallerName "cypherpunk-privacy-windows"

#define MyAppVersion "0.8.0-beta"
#define MyAppNumericVersion "0.8.0"
#define MyInstallerSuffix "-0.8.0-beta"

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
SetupIconFile=..\..\res\win\logo2.ico
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
Source: "..\..\out\win\client\CypherpunkPrivacy-win32-ia32\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion overwritereadonly sign; BeforeInstall: StopClient
Source: "..\..\out\win\client\CypherpunkPrivacy-win32-ia32\*"; DestDir: "{app}"; Flags: 32bit createallsubdirs overwritereadonly recursesubdirs
Source: "..\..\out\win\daemon\Release\32\cypherpunk-privacy-service.exe"; DestDir: "{app}"; DestName: "cypherpunk-privacy-service.exe"; Flags: ignoreversion overwritereadonly sign; Check: not Is64BitInstallMode; BeforeInstall: StopService
Source: "..\..\out\win\daemon\Release\64\cypherpunk-privacy-service.exe"; DestDir: "{app}"; DestName: "cypherpunk-privacy-service.exe"; Flags: ignoreversion overwritereadonly sign; Check: Is64BitInstallMode; BeforeInstall: StopService
Source: "..\..\out\win\daemon\Release\32\openvpn\*"; DestDir: "{app}\openvpn"; Flags: ignoreversion recursesubdirs createallsubdirs overwritereadonly; Check: not Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\64\openvpn\*"; DestDir: "{app}\openvpn"; Flags: ignoreversion recursesubdirs createallsubdirs overwritereadonly; Check: Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\32\tap\*"; DestDir: "{app}\tap"; Flags: ignoreversion recursesubdirs createallsubdirs overwritereadonly; Check: not Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\64\tap\*"; DestDir: "{app}\tap"; Flags: ignoreversion recursesubdirs createallsubdirs overwritereadonly; Check: Is64BitInstallMode
Source: "..\..\out\win\client\CypherpunkPrivacy-win32-ia32\*.exe"; DestDir: "{app}"; Flags: ignoreversion overwritereadonly sign
Source: "..\..\out\win\client\CypherpunkPrivacy-win32-ia32\*.dll"; DestDir: "{app}"; Flags: ignoreversion overwritereadonly sign
Source: "..\..\out\win\daemon\Release\32\openvpn\*.exe"; DestDir: "{app}\openvpn"; Flags: ignoreversion overwritereadonly sign; Check: not Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\32\openvpn\*.dll"; DestDir: "{app}\openvpn"; Flags: ignoreversion overwritereadonly sign; Check: not Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\64\openvpn\*.exe"; DestDir: "{app}\openvpn"; Flags: ignoreversion overwritereadonly sign; Check: Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\64\openvpn\*.dll"; DestDir: "{app}\openvpn"; Flags: ignoreversion overwritereadonly sign; Check: Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\32\tap\*.exe"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly sign; Check: not Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\64\tap\*.exe"; DestDir: "{app}\tap"; Flags: ignoreversion overwritereadonly sign; Check: Is64BitInstallMode

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

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
;Name: "{commonstartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Parameters: "--background"
;Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "{#MyAppID}"; ValueData: """{app}\{#MyAppExeName}"" --background"; Flags: uninsdeletevalue

[InstallDelete]
Type: filesandordirs; Name: "{app}"

[Run]
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "addtap 2"; WorkingDir: "{app}"; Flags: runhidden; StatusMsg: "Installing network adapter..."
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "install"; WorkingDir: "{app}"; Flags: runhidden; StatusMsg: "Installing background service..."
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "start"; WorkingDir: "{app}"; Flags: runhidden; StatusMsg: "Starting background service..."
Filename: "{app}\{#MyAppExeName}"; Parameters: "--first"; WorkingDir: "{app}"; Flags: postinstall, skipifsilent; Description: "Launch Cypherpunk Privacy"

[UninstallRun]
Filename: "taskkill.exe"; Parameters: "/im {#MyAppExeName} /f"; WorkingDir: "{app}"; Flags: runhidden
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "stop"; WorkingDir: "{app}"; Flags: runhidden
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "uninstall"; WorkingDir: "{app}"; Flags: runhidden
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "removetap"; WorkingDir: "{app}"; Flags: runhidden

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
Type: filesandordirs; Name: "C:\ProgramData\{#MyAppName}"
