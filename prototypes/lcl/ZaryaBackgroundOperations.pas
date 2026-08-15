unit ZaryaBackgroundOperations;

{$mode objfpc}{$H+}

interface

uses
  ZaryaProfile, ZaryaProfileTesting, ZaryaRealDelay, ZaryaCoreProviderRegistry,
  ZaryaRouting, ZaryaDns, ZaryaGeoData, ZaryaRuntimeCoordinator,
  ZaryaAppSettings;

type
  TZaryaBackgroundOperations = class
  private
    FProfileTest: TProfileTcpTestThread;
    FRealDelay: TZaryaRealDelayBatchThread;
  public
    destructor Destroy; override;

    function ProfileTestRunning: Boolean;
    procedure StartProfileTest(const AProfiles: TZaryaProfiles);
    procedure CancelProfileTest;
    function ProfileTestDone: Boolean;
    function ProfileTestCurrentIndex: Integer;
    function TakeProfileTestResults(out AResults: TZaryaTcpTestResults): Boolean;

    function RealDelayRunning: Boolean;
    procedure StartRealDelay(const AItems: TZaryaRealDelayWorkItems;
      const AConcurrency: Integer);
    procedure CancelRealDelay;
    function RealDelayDone: Boolean;
    function RealDelayCompletedCount: Integer;
    function RealDelayResultCount: Integer;
    function RealDelayProgressText: string;
    function TakeRealDelayResults(out AResults: TZaryaRealDelayResults): Boolean;
    function BuildRealDelayItems(const AProfiles: TZaryaProfiles;
      const ARegistry: TZaryaCoreProviderRegistry;
      const ARuntimeCoordinator: TZaryaRuntimeCoordinator;
      const ARoutingProfile: TZaryaRoutingProfile;
      const ADnsProfile: TZaryaDnsProfile;
      const AGeoDataManager: IGeoDataManager;
      const ASettings: TZaryaAppSettings;
      const ADataDirectory, AAssetDirectory: string;
      out AItems: TZaryaRealDelayWorkItems; out AError: string): Boolean;

    procedure CancelAll;
  end;

implementation

uses
  SysUtils, ZaryaCoreProvider, ZaryaConfigAdapters, ZaryaRuntimeContracts,
  ZaryaTcpProbe;

function SafeTestDirectoryPart(const AValue: string): string;
var
  I: Integer;
begin
  Result := AValue;
  for I := 1 to Length(Result) do
    if not (Result[I] in ['a'..'z', 'A'..'Z', '0'..'9', '-', '_']) then
      Result[I] := '-';
  if Result = '' then Result := 'profile';
end;

destructor TZaryaBackgroundOperations.Destroy;
begin
  CancelAll;
  inherited Destroy;
end;

function TZaryaBackgroundOperations.ProfileTestRunning: Boolean;
begin
  Result := Assigned(FProfileTest);
end;

procedure TZaryaBackgroundOperations.StartProfileTest(
  const AProfiles: TZaryaProfiles);
begin
  if Assigned(FProfileTest) then
    raise Exception.Create('A profile test is already running.');
  FProfileTest := TProfileTcpTestThread.Create(AProfiles);
end;

procedure TZaryaBackgroundOperations.CancelProfileTest;
begin
  if Assigned(FProfileTest) then
    FProfileTest.RequestCancel;
end;

function TZaryaBackgroundOperations.ProfileTestDone: Boolean;
begin
  Result := Assigned(FProfileTest) and FProfileTest.IsDone;
end;

function TZaryaBackgroundOperations.ProfileTestCurrentIndex: Integer;
begin
  if Assigned(FProfileTest) then
    Result := FProfileTest.CurrentIndex
  else
    Result := -1;
end;

function TZaryaBackgroundOperations.TakeProfileTestResults(
  out AResults: TZaryaTcpTestResults): Boolean;
begin
  AResults := nil;
  Result := Assigned(FProfileTest) and FProfileTest.IsDone;
  if not Result then Exit;
  FProfileTest.WaitFor;
  AResults := Copy(FProfileTest.Results);
  FreeAndNil(FProfileTest);
end;

function TZaryaBackgroundOperations.RealDelayRunning: Boolean;
begin
  Result := Assigned(FRealDelay);
end;

procedure TZaryaBackgroundOperations.StartRealDelay(
  const AItems: TZaryaRealDelayWorkItems; const AConcurrency: Integer);
begin
  if Assigned(FRealDelay) then
    raise Exception.Create('A Real delay batch is already running.');
  FRealDelay := TZaryaRealDelayBatchThread.Create(AItems, AConcurrency);
end;

procedure TZaryaBackgroundOperations.CancelRealDelay;
begin
  if Assigned(FRealDelay) then
    FRealDelay.RequestCancel;
end;

function TZaryaBackgroundOperations.RealDelayDone: Boolean;
begin
  Result := Assigned(FRealDelay) and FRealDelay.IsDone;
end;

function TZaryaBackgroundOperations.RealDelayCompletedCount: Integer;
begin
  if Assigned(FRealDelay) then
    Result := FRealDelay.CompletedCount
  else
    Result := 0;
end;

function TZaryaBackgroundOperations.RealDelayResultCount: Integer;
begin
  if Assigned(FRealDelay) then
    Result := Length(FRealDelay.Results)
  else
    Result := 0;
end;

function TZaryaBackgroundOperations.RealDelayProgressText: string;
begin
  if Assigned(FRealDelay) then
    Result := FRealDelay.ProgressText
  else
    Result := '';
end;

function TZaryaBackgroundOperations.TakeRealDelayResults(
  out AResults: TZaryaRealDelayResults): Boolean;
begin
  AResults := nil;
  Result := Assigned(FRealDelay) and FRealDelay.IsDone;
  if not Result then Exit;
  FRealDelay.WaitFor;
  AResults := Copy(FRealDelay.Results);
  FreeAndNil(FRealDelay);
end;

function TZaryaBackgroundOperations.BuildRealDelayItems(
  const AProfiles: TZaryaProfiles;
  const ARegistry: TZaryaCoreProviderRegistry;
  const ARuntimeCoordinator: TZaryaRuntimeCoordinator;
  const ARoutingProfile: TZaryaRoutingProfile;
  const ADnsProfile: TZaryaDnsProfile;
  const AGeoDataManager: IGeoDataManager;
  const ASettings: TZaryaAppSettings;
  const ADataDirectory, AAssetDirectory: string;
  out AItems: TZaryaRealDelayWorkItems; out AError: string): Boolean;
var
  I, Count: Integer;
  Profile: TZaryaProfile;
  Provider: TZaryaCoreProvider;
  ProviderId: string;
  Adapter: IConfigAdapter;
  Context: TZaryaConfigContext;
  RuntimeRequest: TZaryaRuntimeRequest;
  Config, ItemError, GeoError, MissingGeoData: string;
  Item: TZaryaRealDelayWorkItem;
  Ports: array[0..2] of Integer;

  function AllocateDistinctPorts: Boolean;
  var
    Candidate: Integer;
    PortIndex, Attempt: Integer;
    Duplicate: Boolean;
  begin
    Result := False;
    for PortIndex := 0 to High(Ports) do
    begin
      Ports[PortIndex] := 0;
      for Attempt := 1 to 20 do
      begin
        if not AllocateLocalTcpUdpPort(Candidate, ItemError) then Exit;
        Duplicate := (PortIndex > 0) and (Candidate = Ports[0]);
        if PortIndex > 1 then Duplicate := Duplicate or (Candidate = Ports[1]);
        if not Duplicate then
        begin
          Ports[PortIndex] := Candidate;
          Break;
        end;
      end;
      if Ports[PortIndex] = 0 then
      begin
        ItemError := 'Could not allocate distinct local test ports.';
        Exit;
      end;
    end;
    Result := True;
  end;

  procedure FailItem(const AMessage: string);
  begin
    Item.Prepared := False;
    Item.PreparationError := AMessage;
  end;

begin
  Result := False;
  AError := '';
  AItems := nil;
  MissingGeoData := '';
  GeoError := '';
  if Assigned(AGeoDataManager) and not AGeoDataManager.RequiredFilesPresent(
    ARoutingProfile, ADnsProfile, MissingGeoData, GeoError) then
    if GeoError = '' then GeoError := 'Missing geo data: ' + MissingGeoData;

  Count := 0;
  for I := 0 to High(AProfiles) do
  begin
    Profile := AProfiles[I];
    if not Profile.Enabled or Profile.DeletedBySubscriptionUpdate then Continue;
    Item := Default(TZaryaRealDelayWorkItem);
    ItemError := '';
    Item.ProfileId := Profile.Id;
    Item.ProfileName := Profile.Name;
    if GeoError <> '' then
      FailItem(GeoError)
    else
    begin
      ProviderId := Profile.PreferredProviderId;
      if ProviderId = '' then
        ProviderId := DefaultProviderForProtocol(Profile.ProtocolName);
      if not ARegistry.TryGet(ProviderId, Provider) then
        FailItem('Provider ' + ProviderId + ' is not registered.')
      else if not ARuntimeCoordinator.ProviderUsableForProfile(Provider,
        Profile) then
        FailItem('Provider ' + ProviderId +
          ' is unavailable or incompatible with the profile.')
      else
      begin
        Config := '';
        if Profile.RawConfig <> '' then
        begin
          if not IsDefaultRouting(ARoutingProfile) or
            not IsDefaultDns(ADnsProfile) then
            FailItem('Raw config requires Proxy All and System DNS.')
          else if not (SameText(Profile.SystemProxyKind, 'mixed') or
            SameText(Profile.SystemProxyKind, 'http') or
            SameText(Profile.SystemProxyKind, 'socks')) then
            FailItem('Raw profile needs an HTTP, mixed or SOCKS readiness endpoint.')
          else
            Config := Profile.RawConfig;
          Context := Default(TZaryaConfigContext);
          Context.MixedPort := Profile.ReadinessPort;
          Context.HttpPort := Profile.ReadinessPort;
          Context.SocksPort := Profile.ReadinessPort;
        end
        else if AllocateDistinctPorts then
        begin
          Context := Default(TZaryaConfigContext);
          Context.MixedPort := Ports[0];
          Context.HttpPort := Ports[1];
          Context.SocksPort := Ports[2];
          RuntimeRequest := CreateRuntimeRequest(Profile, Provider, Context);
          RuntimeRequest.DataDirectory := ADataDirectory;
          RuntimeRequest.AssetDirectory := AAssetDirectory;
          RuntimeRequest.RoutingProfile := ARoutingProfile;
          RuntimeRequest.DnsProfile := ADnsProfile;
          Adapter := CreateConfigAdapter(Provider);
          if not Assigned(Adapter) then
            FailItem('Provider has no registered config adapter.')
          else if not Adapter.GenerateRequest(RuntimeRequest, Config,
            ItemError) then
            FailItem(ItemError);
        end
        else
          FailItem(ItemError);

        if (Config <> '') and (Item.PreparationError = '') then
        begin
          Item.Prepared := True;
          Item.Request := Default(TZaryaNodeTestRequest);
          Item.Request.SchemaVersion := 1;
          Item.Request.Provider := Provider;
          Item.Request.Config := Config;
          Item.Request.DataDirectory := IncludeTrailingPathDelimiter(
            ADataDirectory) + 'runtime-tests' + PathDelim +
            SafeTestDirectoryPart(Profile.Id);
          Item.Request.AssetDirectory := Provider.AssetDirectory;
          if Item.Request.AssetDirectory = '' then
            Item.Request.AssetDirectory := AAssetDirectory;
          if Profile.RawConfig <> '' then
          begin
            Item.Request.ReadinessHost := Profile.ReadinessHost;
            Item.Request.ReadinessPort := Profile.ReadinessPort;
            Item.Request.ProxyKind := Profile.SystemProxyKind;
          end
          else
          begin
            Item.Request.ReadinessHost := '127.0.0.1';
            case Provider.ReadinessKind of
              rkMixedTcp:
                begin
                  Item.Request.ReadinessPort := Context.MixedPort;
                  Item.Request.ProxyKind := 'mixed';
                end;
              rkHttpTcp:
                begin
                  Item.Request.ReadinessPort := Context.HttpPort;
                  Item.Request.ProxyKind := 'http';
                end;
              rkSocksTcp:
                begin
                  Item.Request.ReadinessPort := Context.SocksPort;
                  Item.Request.ProxyKind := 'socks';
                end;
            else
              FailItem('Provider does not declare a testable local proxy endpoint.');
            end;
          end;
          Item.Request.TestUrl := ASettings.RealDelayTestUrl;
          Item.Request.TimeoutMs := ASettings.RealDelayTimeoutSeconds * 1000;
        end;
      end;
    end;
    SetLength(AItems, Count + 1);
    AItems[Count] := Item;
    Inc(Count);
  end;
  if Count = 0 then
  begin
    AError := 'Нет включённых профилей для Real delay.';
    Exit;
  end;
  Result := True;
end;

procedure TZaryaBackgroundOperations.CancelAll;
begin
  if Assigned(FProfileTest) then
  begin
    FProfileTest.RequestCancel;
    FProfileTest.WaitFor;
    FreeAndNil(FProfileTest);
  end;
  if Assigned(FRealDelay) then
  begin
    FRealDelay.RequestCancel;
    FRealDelay.WaitFor;
    FreeAndNil(FRealDelay);
  end;
end;

end.
