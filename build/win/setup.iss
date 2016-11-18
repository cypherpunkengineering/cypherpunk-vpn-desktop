#define MyAppID "CypherpunkPrivacy"
#define MyAppName "Cypherpunk Privacy"
#define MyAppVersion "0.2.0-prealpha"
#define MyAppNumericVersion "0.2.0"
#define MyAppPublisher "Cypherpunk Partners, slf."
#define MyAppURL "https://www.cypherpunk.com/"
#define MyAppExeName "CypherpunkPrivacy.exe"
#define MyAppCopyright "Copyright © 2016 " + MyAppPublisher
#define MyInstallerName "cypherpunk-privacy-windows"
#define MyInstallerSuffix "-0.2.0-prealpha"

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
RestartApplications=False
CloseApplicationsFilter=*.exe,*.dll
VersionInfoVersion={#MyAppNumericVersion}
VersionInfoTextVersion={#MyAppVersion}
MinVersion=0,6.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
;Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: checkedonce

[Files]
Source: "..\..\out\win\client\CypherpunkPrivacy-win32-ia32\*"; DestDir: "{app}"; Flags: 32bit createallsubdirs overwritereadonly recursesubdirs
Source: "..\..\out\win\daemon\Release\32\cypherpunk-privacy-service.exe"; DestDir: "{app}"; DestName: "cypherpunk-privacy-service.exe"; Flags: ignoreversion overwritereadonly; Check: not Is64BitInstallMode; BeforeInstall: StopService
Source: "..\..\out\win\daemon\Release\64\cypherpunk-privacy-service.exe"; DestDir: "{app}"; DestName: "cypherpunk-privacy-service.exe"; Flags: ignoreversion overwritereadonly; Check: Is64BitInstallMode; BeforeInstall: StopService
Source: "..\..\out\win\daemon\Release\32\openvpn\*"; DestDir: "{app}\openvpn"; Flags: ignoreversion recursesubdirs createallsubdirs overwritereadonly; Check: not Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\64\openvpn\*"; DestDir: "{app}\openvpn"; Flags: ignoreversion recursesubdirs createallsubdirs overwritereadonly; Check: Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\32\tap\*"; DestDir: "{app}\tap"; Flags: ignoreversion recursesubdirs createallsubdirs overwritereadonly; Check: not Is64BitInstallMode
Source: "..\..\out\win\daemon\Release\64\tap\*"; DestDir: "{app}\tap"; Flags: ignoreversion recursesubdirs createallsubdirs overwritereadonly; Check: Is64BitInstallMode

[PreCompile]
;Name: "build.bat"; Flags: abortonerror cmdprompt

[Code]
var ResultCode: Integer;
procedure StopService();
begin
    Exec('net.exe', 'stop CypherpunkPrivacyService', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{#MyAppName} (Semantic)"; Filename: "{app}\{#MyAppExeName}"; Parameters: "semantic"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
;Name: "{commonstartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Parameters: "--background"
;Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
;Name: "{commondesktop}\{#MyAppName} (Semantic)"; Filename: "{app}\{#MyAppExeName}"; Parameters: "semantic"; Tasks: desktopicon

[Registry]
;Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "{#MyAppName}"; ValueData: """{app}\{#MyAppExeName}"" --background"; Flags: uninsdeletevalue

[InstallDelete]
Type: filesandordirs; Name: "{app}"

[Run]
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "addtap 2"; WorkingDir: "{app}"; Flags: runhidden; StatusMsg: "Installing network adapter..."
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "install"; WorkingDir: "{app}"; Flags: runhidden; StatusMsg: "Installing background service..."
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "start"; WorkingDir: "{app}"; Flags: runhidden; StatusMsg: "Starting background service..."

[UninstallRun]
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "stop"; WorkingDir: "{app}"; Flags: runhidden
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "uninstall"; WorkingDir: "{app}"; Flags: runhidden
Filename: "{app}\cypherpunk-privacy-service.exe"; Parameters: "removetap"; WorkingDir: "{app}"; Flags: runhidden

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
Type: filesandordirs; Name: "C:\ProgramData\{#MyAppName}"
