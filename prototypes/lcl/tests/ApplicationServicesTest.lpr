program ApplicationServicesTest;

{$mode objfpc}{$H+}

uses
  SysUtils, Process, ZaryaProfile, FpcProfileStore, ZaryaProfileService,
  ZaryaBackgroundOperations, ZaryaProfileTesting, ZaryaRuntimeCoordinator,
  ZaryaCoreProvider, ZaryaAppSettings, ZaryaRouting, ZaryaDns,
  ZaryaSystemProxy, ZaryaTcpProbe;

type
  TFakeProxyBackend = class(TInterfacedObject, IZaryaSystemProxyBackend)
  public
    AppliedPort: Integer;
    AppliedKind: string;
    function ReadState(out AState: TZaryaSystemProxyState;
      out AError: string): Boolean;
    function ApplyLocalProxy(const APort: Integer; const AKind: string;
      out AError: string): Boolean;
    function RestoreState(const AState: TZaryaSystemProxyState;
      out AError: string): Boolean;
    function BackendName: string;
  end;

function TFakeProxyBackend.ReadState(out AState: TZaryaSystemProxyState;
  out AError: string): Boolean;
begin
  AState := Default(TZaryaSystemProxyState);
  AError := '';
  Result := True;
end;

function TFakeProxyBackend.ApplyLocalProxy(const APort: Integer;
  const AKind: string; out AError: string): Boolean;
begin
  AppliedPort := APort;
  AppliedKind := AKind;
  AError := '';
  Result := True;
end;

function TFakeProxyBackend.RestoreState(const AState: TZaryaSystemProxyState;
  out AError: string): Boolean;
begin
  AError := '';
  Result := True;
end;

function TFakeProxyBackend.BackendName: string;
begin
  Result := 'Fake';
end;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then raise Exception.Create(AMessage);
end;

function StartListener(const AFakeCorePath: string; const APort: Integer): TProcess;
begin
  Result := TProcess.Create(nil);
  Result.Executable := AFakeCorePath;
  Result.Parameters.Add('listen-once');
  Result.Parameters.Add(IntToStr(APort));
  Result.Options := [poNoConsole];
  Result.Execute;
  Sleep(100);
  Check(Result.Running, 'Readiness fixture did not start.');
end;

var
  TempDirectory, ProfilesFile, SnapshotFile, FakeCorePath: string;
  Profiles, LoadedProfiles: TZaryaProfiles;
  ProfileService: TZaryaProfileService;
  Background: TZaryaBackgroundOperations;
  Results: TZaryaTcpTestResults;
  ErrorMessage, Config: string;
  Deadline: QWord;
  Backend: TFakeProxyBackend;
  Proxy: TZaryaSystemProxyController;
  Coordinator: TZaryaRuntimeCoordinator;
  Provider: TZaryaCoreProvider;
  Settings: TZaryaAppSettings;
  Poll: TZaryaRuntimePollResult;
  Port: Integer;
  Listener: TProcess;
begin
  Randomize;
  TempDirectory := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-services-' + IntToHex(Random(MaxInt), 8);
  Check(ForceDirectories(TempDirectory), 'Could not create test directory.');
  ProfilesFile := IncludeTrailingPathDelimiter(TempDirectory) + 'profiles.json';
  SnapshotFile := IncludeTrailingPathDelimiter(TempDirectory) + 'proxy.ini';
  FakeCorePath := IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
    'FakeCore.exe';
  Check(FileExists(FakeCorePath), 'FakeCore.exe is missing.');

  ProfileService := TZaryaProfileService.Create(TFpcProfileStore.Create(
    ProfilesFile));
  try
    SetLength(Profiles, 1);
    Profiles[0] := CreateEmptyProfile;
    Profiles[0].Name := 'Service profile';
    Profiles[0].Enabled := True;
    Check(ProfileService.Save(Profiles, ErrorMessage),
      'Profile service save failed: ' + ErrorMessage);
    Check(ProfileService.Load(LoadedProfiles, ErrorMessage),
      'Profile service load failed: ' + ErrorMessage);
    Check(ProfileService.FindRunnableById(LoadedProfiles,
      Profiles[0].Id) = 0, 'Runnable profile lookup failed.');
  finally
    ProfileService.Free;
  end;

  Background := TZaryaBackgroundOperations.Create;
  try
    SetLength(Profiles, 0);
    Background.StartProfileTest(Profiles);
    Deadline := GetTickCount64 + 3000;
    while not Background.ProfileTestDone do
    begin
      Check(GetTickCount64 < Deadline, 'Background profile test timed out.');
      Sleep(10);
    end;
    Check(Background.TakeProfileTestResults(Results),
      'Background results were not released.');
    Check(not Background.ProfileTestRunning,
      'Background operation retained a completed thread.');
  finally
    Background.Free;
  end;

  Backend := TFakeProxyBackend.Create;
  Proxy := TZaryaSystemProxyController.Create(Backend, SnapshotFile);
  Coordinator := TZaryaRuntimeCoordinator.Create(TempDirectory,
    TempDirectory, nil, Proxy);
  Listener := nil;
  try
    Check(AllocateLocalTcpUdpPort(Port, ErrorMessage),
      'Could not allocate readiness port: ' + ErrorMessage);
    Profiles := CreateDemoProfiles;
    Profiles[0].RawConfig := '{}';
    Profiles[0].RawConfigFormat := 'xray-json';
    Profiles[0].ReadinessHost := '127.0.0.1';
    Profiles[0].ReadinessPort := Port;
    Profiles[0].SystemProxyKind := 'mixed';
    Provider := CreateProviderPreset(ProviderEmbeddedXray);
    Settings := DefaultAppSettings;
    Check(Coordinator.ProviderUsableForProfile(Provider, Profiles[0]),
      'Runtime compatibility delegation failed.');
    Check(Coordinator.PrepareConfig(Profiles[0], Provider, Settings,
      BuiltInProxyAllRouting, BuiltInSystemDns, Config, ErrorMessage),
      'Runtime preparation failed: ' + ErrorMessage);
    Coordinator.Activate(Profiles[0], Provider);
    Coordinator.BeginConnecting;
    Listener := StartListener(FakeCorePath, Port);
    Check(Coordinator.Poll(True, True, Poll),
      'Ready endpoint did not produce a runtime transition.');
    Check(Poll.Kind = rpkReady, 'Unexpected readiness transition.');
    Check(Poll.SystemProxyEnabled, 'System proxy was not readiness-gated.');
    Check((Backend.AppliedPort = Port) and SameText(Backend.AppliedKind,
      'mixed'), 'Runtime coordinator applied the wrong proxy endpoint.');
    Listener.WaitOnExit;
    Check(Proxy.Restore(ErrorMessage), 'Proxy restore failed: ' + ErrorMessage);
    Coordinator.Reset;
  finally
    if Assigned(Listener) then
    begin
      if Listener.Running then Listener.Terminate(1);
      Listener.Free;
    end;
    Coordinator.Free;
    Proxy.Free;
  end;

  DeleteFile(ProfilesFile);
  DeleteFile(SnapshotFile);
  RemoveDir(TempDirectory);
  WriteLn('Application services and readiness coordinator: PASS');
end.
