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
    Language: string;
    SelectedRoutingProfileId: string;
    SelectedDnsProfileId: string;
    StartAtLogin: Boolean;
    StartMinimizedToTray: Boolean;
    AutoStartLastProfile: Boolean;
    AutoEnableSystemProxyAfterAutoStart: Boolean;
    AutoStartDelaySeconds: Integer;
    LastStartedProfileId: string;
    RealDelayConcurrency: Integer;
    RealDelayTimeoutSeconds: Integer;
    RealDelayTestUrl: string;
    GeoSourceId: string;
    GeoAutoCheckOnStartup: Boolean;
    GeoWarnIfMissing: Boolean;
    FirstRunCompleted: Boolean;
  end;

function DefaultAppSettings: TZaryaAppSettings;

type
  ISettingsStore = interface
    ['{74CF43BA-A54C-44C4-B2F8-1D0B44778812}']
    function Load(out ASettings: TZaryaAppSettings;
      out AError: string): Boolean;
    function Save(const ASettings: TZaryaAppSettings;
      out AError: string): Boolean;
    function GetFileName: string;
  end;

  TZaryaAppSettingsStore = class(TInterfacedObject, ISettingsStore)
  private
    FFileName: string;
    function GetFileName: string;
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
  Result.Language := 'system';
  Result.SelectedRoutingProfileId := 'builtin-bypass-lan';
  Result.SelectedDnsProfileId := 'builtin-dns-system';
  Result.StartAtLogin := False;
  Result.StartMinimizedToTray := False;
  Result.AutoStartLastProfile := False;
  Result.AutoEnableSystemProxyAfterAutoStart := False;
  Result.AutoStartDelaySeconds := 3;
  Result.LastStartedProfileId := '';
  Result.RealDelayConcurrency := 3;
  Result.RealDelayTimeoutSeconds := 10;
  Result.RealDelayTestUrl := 'https://www.gstatic.com/generate_204';
  Result.GeoSourceId := 'runetfreedom';
  Result.GeoAutoCheckOnStartup := True;
  Result.GeoWarnIfMissing := True;
  Result.FirstRunCompleted := False;
end;

constructor TZaryaAppSettingsStore.Create(const AFileName: string);
begin
  inherited Create;
  FFileName := AFileName;
end;

function TZaryaAppSettingsStore.GetFileName: string;
begin
  Result := FFileName;
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
    ASettings.Language := LowerCase(Trim(Ini.ReadString('interface',
      'language', ASettings.Language)));
    if not ((ASettings.Language = 'system') or
      (ASettings.Language = 'ru') or (ASettings.Language = 'en')) then
      ASettings.Language := 'system';
    ASettings.SelectedRoutingProfileId := Ini.ReadString('routing',
      'selectedProfileId', ASettings.SelectedRoutingProfileId);
    ASettings.SelectedDnsProfileId := Ini.ReadString('dns',
      'selectedProfileId', ASettings.SelectedDnsProfileId);
    ASettings.StartAtLogin := Ini.ReadBool('startup', 'startAtLogin',
      ASettings.StartAtLogin);
    ASettings.StartMinimizedToTray := Ini.ReadBool('startup',
      'startMinimizedToTray', ASettings.StartMinimizedToTray);
    ASettings.AutoStartLastProfile := Ini.ReadBool('startup',
      'autoStartLastProfile', ASettings.AutoStartLastProfile);
    ASettings.AutoEnableSystemProxyAfterAutoStart := Ini.ReadBool('startup',
      'autoEnableSystemProxyAfterAutoStart',
      ASettings.AutoEnableSystemProxyAfterAutoStart);
    ASettings.AutoStartDelaySeconds := Ini.ReadInteger('startup',
      'delaySeconds', ASettings.AutoStartDelaySeconds);
    if (ASettings.AutoStartDelaySeconds < 0) or
      (ASettings.AutoStartDelaySeconds > 120) then
      ASettings.AutoStartDelaySeconds := 3;
    ASettings.LastStartedProfileId := Ini.ReadString('startup',
      'lastStartedProfileId', '');
    ASettings.RealDelayConcurrency := Ini.ReadInteger('testing',
      'realDelayConcurrency', ASettings.RealDelayConcurrency);
    if (ASettings.RealDelayConcurrency < 1) or
      (ASettings.RealDelayConcurrency > 10) then
      ASettings.RealDelayConcurrency := 3;
    ASettings.RealDelayTimeoutSeconds := Ini.ReadInteger('testing',
      'realDelayTimeoutSeconds', ASettings.RealDelayTimeoutSeconds);
    if (ASettings.RealDelayTimeoutSeconds < 1) or
      (ASettings.RealDelayTimeoutSeconds > 60) then
      ASettings.RealDelayTimeoutSeconds := 10;
    ASettings.RealDelayTestUrl := Ini.ReadString('testing',
      'realDelayTestUrl', ASettings.RealDelayTestUrl);
    ASettings.GeoSourceId := Ini.ReadString('geodata', 'selectedSourceId',
      ASettings.GeoSourceId);
    ASettings.GeoAutoCheckOnStartup := Ini.ReadBool('geodata',
      'autoCheckOnStartup', ASettings.GeoAutoCheckOnStartup);
    ASettings.GeoWarnIfMissing := Ini.ReadBool('geodata', 'warnIfMissing',
      ASettings.GeoWarnIfMissing);
    ASettings.FirstRunCompleted := Ini.ReadBool('application',
      'firstRunCompleted', ASettings.FirstRunCompleted);
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
    Ini.WriteString('interface', 'language', ASettings.Language);
    Ini.WriteString('routing', 'selectedProfileId',
      ASettings.SelectedRoutingProfileId);
    Ini.WriteString('dns', 'selectedProfileId', ASettings.SelectedDnsProfileId);
    Ini.WriteBool('startup', 'startAtLogin', ASettings.StartAtLogin);
    Ini.WriteBool('startup', 'startMinimizedToTray',
      ASettings.StartMinimizedToTray);
    Ini.WriteBool('startup', 'autoStartLastProfile',
      ASettings.AutoStartLastProfile);
    Ini.WriteBool('startup', 'autoEnableSystemProxyAfterAutoStart',
      ASettings.AutoEnableSystemProxyAfterAutoStart);
    Ini.WriteInteger('startup', 'delaySeconds',
      ASettings.AutoStartDelaySeconds);
    Ini.WriteString('startup', 'lastStartedProfileId',
      ASettings.LastStartedProfileId);
    Ini.WriteInteger('testing', 'realDelayConcurrency',
      ASettings.RealDelayConcurrency);
    Ini.WriteInteger('testing', 'realDelayTimeoutSeconds',
      ASettings.RealDelayTimeoutSeconds);
    Ini.WriteString('testing', 'realDelayTestUrl',
      ASettings.RealDelayTestUrl);
    Ini.WriteString('geodata', 'selectedSourceId', ASettings.GeoSourceId);
    Ini.WriteBool('geodata', 'autoCheckOnStartup',
      ASettings.GeoAutoCheckOnStartup);
    Ini.WriteBool('geodata', 'warnIfMissing', ASettings.GeoWarnIfMissing);
    Ini.WriteBool('application', 'firstRunCompleted',
      ASettings.FirstRunCompleted);
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
