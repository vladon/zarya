unit ZaryaAppSettings;

{$IFDEF FPC}
{$mode delphi}{$H+}
{$ENDIF}

interface

uses
  SysUtils;

type
  TZaryaAppSettings = record
    DarkTheme: Boolean;
    MinimizeToTray: Boolean;
    MixedPort: Integer;
    AutoEnableSystemProxy: Boolean;
    RestoreSystemProxy: Boolean;
    SelectedRoutingProfileId: string;
    SelectedDnsProfileId: string;
    GeoSourceId: string;
    GeoAutoCheckOnStartup: Boolean;
    GeoWarnIfMissing: Boolean;
  end;

function DefaultAppSettings: TZaryaAppSettings;

type
  TZaryaAppSettingsStore = class
  private
    FFileName: string;
  public
    constructor Create(const AFileName: string);
    function Load(out ASettings: TZaryaAppSettings;
      out AError: string): Boolean;
    function Save(const ASettings: TZaryaAppSettings;
      out AError: string): Boolean;
    property FileName: string read FFileName;
  end;

implementation

uses
  IniFiles;

function DefaultAppSettings: TZaryaAppSettings;
begin
  Result.DarkTheme := False;
  Result.MinimizeToTray := True;
  Result.MixedPort := 10808;
  Result.AutoEnableSystemProxy := True;
  Result.RestoreSystemProxy := True;
  Result.SelectedRoutingProfileId := 'builtin-bypass-lan';
  Result.SelectedDnsProfileId := 'builtin-dns-system';
  Result.GeoSourceId := 'runetfreedom';
  Result.GeoAutoCheckOnStartup := True;
  Result.GeoWarnIfMissing := True;
end;

constructor TZaryaAppSettingsStore.Create(const AFileName: string);
begin
  inherited Create;
  FFileName := AFileName;
end;

function TZaryaAppSettingsStore.Load(out ASettings: TZaryaAppSettings;
  out AError: string): Boolean;
var
  Ini: TIniFile;
begin
  ASettings := DefaultAppSettings;
  AError := '';
  if not FileExists(FFileName) then
    Exit(True);
  Ini := nil;
  try
    Ini := TIniFile.Create(FFileName);
    ASettings.DarkTheme := Ini.ReadBool('interface', 'darkTheme',
      ASettings.DarkTheme);
    ASettings.MinimizeToTray := Ini.ReadBool('interface', 'minimizeToTray',
      ASettings.MinimizeToTray);
    ASettings.MixedPort := Ini.ReadInteger('proxy', 'mixedPort',
      ASettings.MixedPort);
    if (ASettings.MixedPort < 1) or (ASettings.MixedPort > 65535) then
      ASettings.MixedPort := 10808;
    ASettings.AutoEnableSystemProxy := Ini.ReadBool('proxy',
      'autoEnableSystemProxy', ASettings.AutoEnableSystemProxy);
    ASettings.RestoreSystemProxy := Ini.ReadBool('proxy',
      'restoreSystemProxy', ASettings.RestoreSystemProxy);
    ASettings.SelectedRoutingProfileId := Ini.ReadString('routing',
      'selectedProfileId', ASettings.SelectedRoutingProfileId);
    ASettings.SelectedDnsProfileId := Ini.ReadString('dns',
      'selectedProfileId', ASettings.SelectedDnsProfileId);
    ASettings.GeoSourceId := Ini.ReadString('geodata', 'selectedSourceId',
      ASettings.GeoSourceId);
    ASettings.GeoAutoCheckOnStartup := Ini.ReadBool('geodata',
      'autoCheckOnStartup', ASettings.GeoAutoCheckOnStartup);
    ASettings.GeoWarnIfMissing := Ini.ReadBool('geodata', 'warnIfMissing',
      ASettings.GeoWarnIfMissing);
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Ini.Free;
end;

function TZaryaAppSettingsStore.Save(const ASettings: TZaryaAppSettings;
  out AError: string): Boolean;
var
  Ini: TIniFile;
  DirectoryName: string;
begin
  AError := '';
  Ini := nil;
  try
    DirectoryName := ExtractFileDir(FFileName);
    if (DirectoryName <> '') and (not ForceDirectories(DirectoryName)) then
      raise Exception.Create('Не удалось создать каталог настроек: ' +
        DirectoryName);
    Ini := TIniFile.Create(FFileName);
    Ini.WriteBool('interface', 'darkTheme', ASettings.DarkTheme);
    Ini.WriteBool('interface', 'minimizeToTray', ASettings.MinimizeToTray);
    Ini.WriteInteger('proxy', 'mixedPort', ASettings.MixedPort);
    Ini.WriteBool('proxy', 'autoEnableSystemProxy',
      ASettings.AutoEnableSystemProxy);
    Ini.WriteBool('proxy', 'restoreSystemProxy',
      ASettings.RestoreSystemProxy);
    Ini.WriteString('routing', 'selectedProfileId',
      ASettings.SelectedRoutingProfileId);
    Ini.WriteString('dns', 'selectedProfileId', ASettings.SelectedDnsProfileId);
    Ini.WriteString('geodata', 'selectedSourceId', ASettings.GeoSourceId);
    Ini.WriteBool('geodata', 'autoCheckOnStartup',
      ASettings.GeoAutoCheckOnStartup);
    Ini.WriteBool('geodata', 'warnIfMissing', ASettings.GeoWarnIfMissing);
    Ini.UpdateFile;
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Ini.Free;
end;

end.
