program RuntimeFoundationTest;

{$mode objfpc}{$H+}

uses
  SysUtils, ZaryaAppSettings, ZaryaSystemProxy, ZaryaEmbeddedXray,
  ZaryaTcpProbe, ZaryaTcpLatency;

type
  TFakeProxyBackend = class(TInterfacedObject, IZaryaSystemProxyBackend)
  public
    CurrentState: TZaryaSystemProxyState;
    LastRestoredState: TZaryaSystemProxyState;
    AppliedPort: Integer;
    AppliedKind: string;
    RestoreCalled: Boolean;
    FailApply: Boolean;
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
  AState := CurrentState;
  AError := '';
  Result := True;
end;

function TFakeProxyBackend.ApplyLocalProxy(const APort: Integer;
  const AKind: string; out AError: string): Boolean;
begin
  AppliedPort := APort;
  AppliedKind := AKind;
  Result := not FailApply;
  if Result then
    AError := ''
  else
    AError := 'simulated apply failure';
end;

function TFakeProxyBackend.RestoreState(
  const AState: TZaryaSystemProxyState; out AError: string): Boolean;
begin
  RestoreCalled := True;
  LastRestoredState := AState;
  CurrentState := AState;
  AError := '';
  Result := True;
end;

function TFakeProxyBackend.BackendName: string;
begin
  Result := 'Fake';
end;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

var
  TempDirectory: string;
  SettingsFile: string;
  SnapshotFile: string;
  SettingsStore: TZaryaAppSettingsStore;
  Settings: TZaryaAppSettings;
  LoadedSettings: TZaryaAppSettings;
  Backend: TFakeProxyBackend;
  Controller: TZaryaSystemProxyController;
  ErrorMessage: string;
  BridgePath: string;
  Bridge: TZaryaEmbeddedXray;
  LatencyMs: Integer;
begin
  Randomize;
  TempDirectory := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-lcl-runtime-' + IntToHex(Random(MaxInt), 8);
  Check(ForceDirectories(TempDirectory), 'Could not create temp directory.');
  SettingsFile := IncludeTrailingPathDelimiter(TempDirectory) + 'settings.ini';
  SnapshotFile := IncludeTrailingPathDelimiter(TempDirectory) + 'proxy.ini';

  SettingsStore := TZaryaAppSettingsStore.Create(SettingsFile);
  try
    Settings := DefaultAppSettings;
    Settings.DarkTheme := True;
    Settings.MixedPort := 20808;
    Settings.AutoEnableSystemProxy := False;
    Check(SettingsStore.Save(Settings, ErrorMessage),
      'Settings save failed: ' + ErrorMessage);
    Check(SettingsStore.Load(LoadedSettings, ErrorMessage),
      'Settings load failed: ' + ErrorMessage);
    Check(LoadedSettings.DarkTheme, 'Dark theme did not persist.');
    Check(LoadedSettings.MixedPort = 20808, 'Mixed port did not persist.');
    Check(not LoadedSettings.AutoEnableSystemProxy,
      'Auto proxy flag did not persist.');
  finally
    SettingsStore.Free;
  end;

  Backend := TFakeProxyBackend.Create;
  Backend.CurrentState.HasProxyEnable := True;
  Backend.CurrentState.ProxyEnabled := False;
  Backend.CurrentState.HasProxyServer := True;
  Backend.CurrentState.ProxyServer := 'old.proxy.invalid:3128';
  Controller := TZaryaSystemProxyController.Create(Backend, SnapshotFile);
  try
    Check(Controller.Enable(10808, 'mixed', ErrorMessage),
      'Proxy enable failed: ' + ErrorMessage);
    Check(Backend.AppliedPort = 10808, 'Wrong proxy port was applied.');
    Check(Backend.AppliedKind = 'mixed', 'Wrong proxy kind was applied.');
    Check(FileExists(SnapshotFile), 'Proxy snapshot was not persisted.');
    Check(Controller.Restore(ErrorMessage),
      'Proxy restore failed: ' + ErrorMessage);
    Check(Backend.RestoreCalled, 'Backend restore was not called.');
    Check(Backend.LastRestoredState.ProxyServer = 'old.proxy.invalid:3128',
      'Previous proxy state was not restored.');
    Check(not FileExists(SnapshotFile), 'Proxy snapshot was not cleared.');
  finally
    Controller.Free;
  end;

  Backend := TFakeProxyBackend.Create;
  Backend.CurrentState.HasProxyEnable := True;
  Backend.CurrentState.ProxyEnabled := True;
  Backend.CurrentState.HasProxyServer := True;
  Backend.CurrentState.ProxyServer := 'recovery.proxy.invalid:8080';
  Controller := TZaryaSystemProxyController.Create(Backend, SnapshotFile);
  Check(Controller.Enable(10809, 'socks', ErrorMessage),
    'Recovery setup failed: ' + ErrorMessage);
  Check(Backend.AppliedKind = 'socks', 'SOCKS proxy kind was not applied.');
  Controller.Free;
  Backend := TFakeProxyBackend.Create;
  Controller := TZaryaSystemProxyController.Create(Backend, SnapshotFile);
  try
    Check(Controller.RecoverPersisted(ErrorMessage),
      'Persisted recovery failed: ' + ErrorMessage);
    Check(Backend.LastRestoredState.ProxyServer =
      'recovery.proxy.invalid:8080', 'Persisted state mismatch.');
    Check(not FileExists(SnapshotFile), 'Recovery snapshot was not cleared.');
  finally
    Controller.Free;
  end;

  Backend := TFakeProxyBackend.Create;
  Backend.FailApply := True;
  Controller := TZaryaSystemProxyController.Create(Backend, SnapshotFile);
  try
    Check(not Controller.Enable(10810, 'http', ErrorMessage),
      'Failed apply was reported as success.');
    Check(Backend.RestoreCalled, 'Failed apply did not roll back.');
    Check(not FileExists(SnapshotFile), 'Rollback left a stale snapshot.');
  finally
    Controller.Free;
  end;

  Check(not CanConnectLocalhost(0), 'Invalid TCP port was accepted.');
  Check(not MeasureTcpLatency('', 443, 10, LatencyMs, ErrorMessage),
    'TCP latency accepted an empty host.');

  BridgePath := ExpandFileName(IncludeTrailingPathDelimiter(
    ExtractFileDir(ParamStr(0))) + '..' + PathDelim + '..' + PathDelim +
    'bin' + PathDelim + 'zarya-xray.dll');
  if FileExists(BridgePath) then
  begin
    Bridge := TZaryaEmbeddedXray.Create(BridgePath);
    try
      Check(Bridge.Available, 'Embedded Xray load failed: ' + Bridge.LoadStatus);
      Check(Bridge.AbiVersion = 1, 'Embedded Xray ABI mismatch.');
      Check(Bridge.Version <> '', 'Embedded Xray version is empty.');
    finally
      Bridge.Free;
    end;
  end;

  DeleteFile(SettingsFile);
  RemoveDir(TempDirectory);
  WriteLn('Runtime foundation: PASS');
end.
