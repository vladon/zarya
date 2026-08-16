unit ZaryaRuntimeCoordinator;

{$mode objfpc}{$H+}

interface

uses
  ZaryaProfile, ZaryaCoreProvider, ZaryaAppSettings, ZaryaRouting, ZaryaDns,
  ZaryaGeoData, ZaryaSystemProxy;

type
  TRuntimeState = (rsStopped, rsConnecting, rsRunning);

  TZaryaRuntimePollKind = (rpkNone, rpkReady, rpkStoppedUnexpectedly,
    rpkReadinessTimeout);

  TZaryaRuntimePollResult = record
    Kind: TZaryaRuntimePollKind;
    SystemProxyEnabled: Boolean;
    SystemProxyDisabledBySetting: Boolean;
    UnsupportedSystemProxyKind: Boolean;
    ErrorMessage: string;
  end;

  TZaryaRuntimeCoordinator = class
  private
    FState: TRuntimeState;
    FActiveProfile: TZaryaProfile;
    FActiveProvider: TZaryaCoreProvider;
    FReadinessHost: string;
    FReadinessPort: Integer;
    FSystemProxyKind: string;
    FReadyDeadline: QWord;
    FDataDirectory: string;
    FAssetDirectory: string;
    FGeoDataManager: IGeoDataManager;
    FSystemProxy: TZaryaSystemProxyController;
  public
    constructor Create(const ADataDirectory, AAssetDirectory: string;
      const AGeoDataManager: IGeoDataManager;
      const ASystemProxy: TZaryaSystemProxyController);
    function ProviderUsableForProfile(const AProvider: TZaryaCoreProvider;
      const AProfile: TZaryaProfile): Boolean;
    function PrepareConfig(const AProfile: TZaryaProfile;
      const AProvider: TZaryaCoreProvider; const ASettings: TZaryaAppSettings;
      const ARoutingProfile: TZaryaRoutingProfile;
      const ADnsProfile: TZaryaDnsProfile; out AConfig,
      AError: string): Boolean;
    procedure Activate(const AProfile: TZaryaProfile;
      const AProvider: TZaryaCoreProvider);
    procedure BeginConnecting;
    function Poll(const ARuntimeIsRunning, AEnableSystemProxy: Boolean;
      out APoll: TZaryaRuntimePollResult): Boolean;
    procedure Reset;
    property State: TRuntimeState read FState;
    property ActiveProfile: TZaryaProfile read FActiveProfile;
    property ActiveProvider: TZaryaCoreProvider read FActiveProvider;
    property ReadinessHost: string read FReadinessHost;
    property ReadinessPort: Integer read FReadinessPort;
    property SystemProxyKind: string read FSystemProxyKind;
  end;

implementation

uses
  SysUtils, ZaryaRuntimeContracts, ZaryaConfigAdapters, ZaryaTcpProbe, ZaryaTr;

constructor TZaryaRuntimeCoordinator.Create(const ADataDirectory,
  AAssetDirectory: string; const AGeoDataManager: IGeoDataManager;
  const ASystemProxy: TZaryaSystemProxyController);
begin
  inherited Create;
  FDataDirectory := ADataDirectory;
  FAssetDirectory := AAssetDirectory;
  FGeoDataManager := AGeoDataManager;
  FSystemProxy := ASystemProxy;
  Reset;
end;

function TZaryaRuntimeCoordinator.ProviderUsableForProfile(
  const AProvider: TZaryaCoreProvider; const AProfile: TZaryaProfile): Boolean;
var
  RequiredFormat: string;
begin
  Result := False;
  if AProvider.State <> psAvailable then Exit;
  if not ProviderSupportsProtocol(AProvider, AProfile.ProtocolName) then Exit;
  if AProfile.RawConfig <> '' then
  begin
    RequiredFormat := AProfile.RawConfigFormat;
    if RequiredFormat = '' then RequiredFormat := 'raw';
    Exit(SameText(ConfigFormatToString(AProvider.ConfigFormat),
      RequiredFormat));
  end;
  Result := ProviderCanGenerateConfig(AProvider, AProfile, RequiredFormat);
end;

function TZaryaRuntimeCoordinator.PrepareConfig(
  const AProfile: TZaryaProfile; const AProvider: TZaryaCoreProvider;
  const ASettings: TZaryaAppSettings;
  const ARoutingProfile: TZaryaRoutingProfile;
  const ADnsProfile: TZaryaDnsProfile; out AConfig,
  AError: string): Boolean;
var
  Adapter: IConfigAdapter;
  Context: TZaryaConfigContext;
  Request: TZaryaRuntimeRequest;
  MissingGeoData: string;
begin
  Result := False;
  AConfig := '';
  AError := '';
  if Assigned(FGeoDataManager) and not FGeoDataManager.RequiredFilesPresent(
    ARoutingProfile, ADnsProfile, MissingGeoData, AError) then
  begin
    if AError = '' then
      AError := TZaryaTr.Tr(
        'Для активных routing/DNS отсутствуют файлы: ',
        'Files required by active routing/DNS are missing: ') +
        MissingGeoData + TZaryaTr.Tr('. Откройте Инструменты → Geo data.',
        '. Open Tools → Geo data.');
    Exit;
  end;
  if AProfile.RawConfig <> '' then
  begin
    if not IsDefaultRouting(ARoutingProfile) or
      not IsDefaultDns(ADnsProfile) then
    begin
      AError := TZaryaTr.Tr(
        'Raw config управляет routing и DNS самостоятельно. Выберите Proxy All и System DNS.',
        'Raw config manages routing and DNS itself. Select Proxy All and System DNS.');
      Exit;
    end;
    if not SameText(ConfigFormatToString(AProvider.ConfigFormat),
      AProfile.RawConfigFormat) then
    begin
      AError := TZaryaTr.Tr(
        'Raw-конфигурация не соответствует диалекту provider.',
        'The raw configuration does not match the provider dialect.');
      Exit;
    end;
    if not SameText(AProfile.ReadinessHost, '127.0.0.1') and
      not SameText(AProfile.ReadinessHost, 'localhost') then
    begin
      AError := TZaryaTr.Tr(
        'Readiness endpoint raw-профиля должен быть локальным.',
        'The raw profile readiness endpoint must be local.');
      Exit;
    end;
    if (AProfile.ReadinessPort < 1) or (AProfile.ReadinessPort > 65535) then
    begin
      AError := TZaryaTr.Tr('Для raw-профиля укажите readiness port.',
        'Specify a readiness port for the raw profile.');
      Exit;
    end;
    AConfig := AProfile.RawConfig;
    FReadinessHost := AProfile.ReadinessHost;
    FReadinessPort := AProfile.ReadinessPort;
    FSystemProxyKind := AProfile.SystemProxyKind;
    Exit(True);
  end;

  Adapter := CreateConfigAdapter(AProvider);
  if not Assigned(Adapter) then
  begin
    AError := TZaryaTr.Tr('Для adapter ',
      'No generator is registered for adapter ') + AProvider.AdapterId +
      TZaryaTr.Tr(' не зарегистрирован генератор; используйте raw config.',
      '; use raw config.');
    Exit;
  end;
  Context := Default(TZaryaConfigContext);
  Context.MixedPort := ASettings.MixedPort;
  Context.HttpPort := ASettings.MixedPort;
  Context.SocksPort := ASettings.MixedPort;
  if SameText(AProvider.AdapterId, 'hysteria2') then
    if ASettings.MixedPort < 65535 then
      Context.SocksPort := ASettings.MixedPort + 1
    else
      Context.SocksPort := ASettings.MixedPort - 1;
  Request := CreateRuntimeRequest(AProfile, AProvider, Context);
  Request.DataDirectory := FDataDirectory;
  Request.AssetDirectory := FAssetDirectory;
  Request.RoutingProfile := ARoutingProfile;
  Request.DnsProfile := ADnsProfile;
  Result := Adapter.GenerateRequest(Request, AConfig, AError);
  if not Result then Exit;
  FReadinessHost := '127.0.0.1';
  case AProvider.ReadinessKind of
    rkMixedTcp:
      begin
        FReadinessPort := Context.MixedPort;
        FSystemProxyKind := 'mixed';
      end;
    rkHttpTcp:
      begin
        FReadinessPort := Context.HttpPort;
        FSystemProxyKind := 'http';
      end;
    rkSocksTcp:
      begin
        FReadinessPort := Context.SocksPort;
        FSystemProxyKind := 'socks';
      end;
  else
    FReadinessPort := Context.MixedPort;
    FSystemProxyKind := 'none';
  end;
end;

procedure TZaryaRuntimeCoordinator.Activate(const AProfile: TZaryaProfile;
  const AProvider: TZaryaCoreProvider);
begin
  FActiveProfile := AProfile;
  FActiveProvider := AProvider;
end;

procedure TZaryaRuntimeCoordinator.BeginConnecting;
begin
  FState := rsConnecting;
  FReadyDeadline := GetTickCount64 + 5000;
end;

function TZaryaRuntimeCoordinator.Poll(const ARuntimeIsRunning,
  AEnableSystemProxy: Boolean; out APoll: TZaryaRuntimePollResult): Boolean;
begin
  APoll := Default(TZaryaRuntimePollResult);
  Result := False;
  if FState = rsStopped then Exit;
  if not ARuntimeIsRunning then
  begin
    APoll.Kind := rpkStoppedUnexpectedly;
    Exit(True);
  end;
  if FState <> rsConnecting then Exit;
  if CanConnectLocalhost(FReadinessPort) then
  begin
    FState := rsRunning;
    APoll.Kind := rpkReady;
    if not AEnableSystemProxy then
      APoll.SystemProxyDisabledBySetting := True
    else if SameText(FSystemProxyKind, 'mixed') or
      SameText(FSystemProxyKind, 'http') or
      SameText(FSystemProxyKind, 'socks') then
    begin
      if Assigned(FSystemProxy) and FSystemProxy.Enable(FReadinessPort,
        FSystemProxyKind, APoll.ErrorMessage) then
        APoll.SystemProxyEnabled := True;
    end
    else
      APoll.UnsupportedSystemProxyKind := True;
    Exit(True);
  end;
  if GetTickCount64 >= FReadyDeadline then
  begin
    APoll.Kind := rpkReadinessTimeout;
    Result := True;
  end;
end;

procedure TZaryaRuntimeCoordinator.Reset;
begin
  FState := rsStopped;
  FActiveProfile := Default(TZaryaProfile);
  FActiveProvider := Default(TZaryaCoreProvider);
  FReadinessHost := '';
  FReadinessPort := 0;
  FSystemProxyKind := 'none';
  FReadyDeadline := 0;
end;

end.
