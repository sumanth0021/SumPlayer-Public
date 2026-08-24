[Setup]
AppName=SUM PLAYER
AppVersion=1.0.0
AppPublisher=SancTech
DefaultDirName={autopf}\SumPlayer
DefaultGroupName=SUM PLAYER
UninstallDisplayIcon={app}\SumPlayer.exe
OutputDir=output
OutputBaseFilename=SumPlayerSetup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "..\build\Release\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{group}\SUM PLAYER"; Filename: "{app}\SumPlayer.exe"
Name: "{group}\Uninstall SUM PLAYER"; Filename: "{uninstallexe}"
Name: "{autodesktop}\SUM PLAYER"; Filename: "{app}\SumPlayer.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Run]
Filename: "{app}\vc_redist.x64.exe"; Parameters: "/quiet /norestart"; StatusMsg: "Installing required components..."; Flags: waituntilterminated skipifsilent
Filename: "{app}\SumPlayer.exe"; Description: "Launch SUM PLAYER"; Flags: nowait postinstall skipifsilent