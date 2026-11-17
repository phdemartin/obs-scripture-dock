#define MyAppName "OBS Scripture Dock"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Philippe De Martin"
#define MyAppURL "https://github.com/phdemartin/obs-scripture-dock"

[Setup]
AppId={{8E9D4F21-7C3A-4B5E-9A12-3F8D6C0E4A11}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
DefaultDirName={code:GetObsDir}
DisableProgramGroupPage=yes
OutputDir=..\..\dist
OutputBaseFilename=obs-scripture-dock-windows-x64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayName={#MyAppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Files]
Source: "..\..\dist\obs-scripture-dock\bin\64bit\obs-scripture-dock.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion

[Code]
function GetObsDir(Param: string): string;
var
  ObsPath: string;
begin
  if RegQueryStringValue(HKLM64, 'SOFTWARE\OBS Studio', '', ObsPath) and (ObsPath <> '') then
    Result := ObsPath
  else
    Result := ExpandConstant('{commonpf64}\obs-studio');
end;
