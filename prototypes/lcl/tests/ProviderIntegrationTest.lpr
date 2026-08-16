program ProviderIntegrationTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, ZaryaProfile, ZaryaCoreProvider, ZaryaRuntimeContracts,
  ZaryaConfigAdapters, ZaryaRuntimeProcess, ZaryaTcpProbe;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

procedure WriteUtf8(const AFileName, AContent: string);
var
  Stream: TFileStream;
  Bytes: UTF8String;
begin
  Bytes := UTF8String(AContent);
  Stream := TFileStream.Create(AFileName, fmCreate);
  try
    if Length(Bytes) > 0 then
      Stream.WriteBuffer(Bytes[1], Length(Bytes));
  finally
    Stream.Free;
  end;
end;

procedure ValidateAndStart(const AExecutable: string;
  const AProvider: TZaryaCoreProvider; const AProfile: TZaryaProfile;
  const ATempDirectory: string);
var
  Adapter: IConfigAdapter;
  Context: TZaryaConfigContext;
  Config: string;
  ConfigFile: string;
  ErrorMessage: string;
  Output: string;
  ExitCode: Integer;
  Arguments: TZaryaStringArray;
  ProcessContext: TZaryaProcessContext;
  Runtime: IZaryaRuntimeProcess;
  Deadline: QWord;
  Ready: Boolean;
  Attempt: Integer;
  RuntimeOutput: string;
  ReadinessPort: Integer;
begin
  Adapter := CreateConfigAdapter(AProvider);
  Check(Assigned(Adapter), AProvider.ProviderId + ': adapter missing.');
  ConfigFile := IncludeTrailingPathDelimiter(ATempDirectory) +
    StringReplace(AProvider.ProviderId, '.', '-', [rfReplaceAll]) +
    AProvider.ConfigExtension;
  Ready := False;
  RuntimeOutput := '';
  for Attempt := 1 to 3 do
  begin
    Context := Default(TZaryaConfigContext);
    Check(AllocateLocalTcpUdpPort(Context.MixedPort, ErrorMessage),
      'Could not allocate a mixed provider smoke port: ' + ErrorMessage);
    repeat
      Check(AllocateLocalTcpPort(Context.HttpPort, ErrorMessage),
        'Could not allocate an HTTP provider smoke port: ' + ErrorMessage);
    until Context.HttpPort <> Context.MixedPort;
    repeat
      Check(AllocateLocalTcpPort(Context.SocksPort, ErrorMessage),
        'Could not allocate a SOCKS provider smoke port: ' + ErrorMessage);
    until (Context.SocksPort <> Context.MixedPort) and
      (Context.SocksPort <> Context.HttpPort);
    Check(Adapter.Generate(AProfile, Context, Config, ErrorMessage),
      AProvider.ProviderId + ': generation failed: ' + ErrorMessage);
    WriteUtf8(ConfigFile, Config);

    ProcessContext := Default(TZaryaProcessContext);
    ProcessContext.ConfigPath := ConfigFile;
    ProcessContext.DataDirectory := ATempDirectory;
    ProcessContext.AssetDirectory := ATempDirectory;
    ProcessContext.MixedPort := Context.MixedPort;
    ProcessContext.HttpPort := Context.HttpPort;
    ProcessContext.SocksPort := Context.SocksPort;
    ProcessContext.LogLevel := 'warning';
    if Length(AProvider.ValidateArguments) > 0 then
    begin
      Check(ExpandProviderArguments(AProvider.ValidateArguments, ProcessContext,
        Arguments, ErrorMessage), AProvider.ProviderId +
        ': validation args failed: ' + ErrorMessage);
      Check(RunProcessProbe(AExecutable, ExtractFileDir(AExecutable), Arguments,
        10000, Output, ExitCode, ErrorMessage), AProvider.ProviderId +
        ': real validation failed: ' + ErrorMessage + LineEnding + Output);
    end;
    Check(ExpandProviderArguments(AProvider.RunArguments, ProcessContext,
      Arguments, ErrorMessage), AProvider.ProviderId +
      ': run args failed: ' + ErrorMessage);
    Runtime := TZaryaExternalProcess.Create;
    Check(Runtime.Start(AExecutable, ExtractFileDir(AExecutable), Arguments,
      ErrorMessage), AProvider.ProviderId + ': start failed: ' + ErrorMessage);
    try
      Deadline := GetTickCount64 + 5000;
      case AProvider.ReadinessKind of
        rkHttpTcp: ReadinessPort := Context.HttpPort;
        rkSocksTcp: ReadinessPort := Context.SocksPort;
      else
        ReadinessPort := Context.MixedPort;
      end;
      while Runtime.IsRunning and (GetTickCount64 < Deadline) do
      begin
        if CanConnectLocalhost(ReadinessPort) then
        begin
          Ready := True;
          Break;
        end;
        Sleep(100);
      end;
      RuntimeOutput := Runtime.DrainOutput;
    finally
      Runtime.Stop;
      Runtime := nil;
    end;
    if Ready then
      Break;
    if (Pos('bind:', LowerCase(RuntimeOutput)) = 0) and
      (Pos('address already in use', LowerCase(RuntimeOutput)) = 0) and
      (Pos('only one usage of each socket address',
        LowerCase(RuntimeOutput)) = 0) then
      Break;
  end;
  Check(Ready, AProvider.ProviderId + ': readiness failed after ' +
    IntToStr(Attempt) + ' attempt(s).' + LineEnding + RuntimeOutput);
  Deadline := GetTickCount64 + 3000;
  while CanConnectLocalhost(ReadinessPort) and
    (GetTickCount64 < Deadline) do
    Sleep(20);
  Check(not CanConnectLocalhost(ReadinessPort),
    AProvider.ProviderId + ': listener remained after Job stop.');
  DeleteFile(ConfigFile);
end;

procedure ConfigureProfileForProvider(var AProfile: TZaryaProfile;
  const AProviderId: string);
begin
  AProfile := CreateEmptyProfile;
  AProfile.Name := 'Provider integration';
  AProfile.Host := '127.0.0.1';
  AProfile.Port := 9;
  if SameText(AProviderId, ProviderExternalHysteria2) then
  begin
    AProfile.ProtocolName := 'Hysteria2';
    AProfile.Password := 'integration-password';
    AProfile.ServerName := 'localhost';
    AProfile.AllowInsecure := True;
  end
  else
  begin
    AProfile.ProtocolName := 'VLESS';
    AProfile.Uuid := '11111111-1111-1111-1111-111111111111';
    AProfile.Security := 'none';
    AProfile.Network := 'tcp';
  end;
end;

function YamlPath(const AValue: string): string;
begin
  Result := '"' + StringReplace(AValue, '\', '\\', [rfReplaceAll]) + '"';
end;

procedure ValidateHysteriaWithLocalServer(const AExecutable: string;
  const AProvider: TZaryaCoreProvider; var AProfile: TZaryaProfile;
  const ATempDirectory: string);
var
  ServerPort: Integer;
  CertificatePath, KeyPath, ServerConfigPath, ServerConfig: string;
  ErrorMessage, ServerOutput: string;
  ServerRuntime: IZaryaRuntimeProcess;
  Arguments: TZaryaStringArray;
begin
  CertificatePath := GetEnvironmentVariable('ZARYA_HYSTERIA_TEST_CERT');
  KeyPath := GetEnvironmentVariable('ZARYA_HYSTERIA_TEST_KEY');
  Check(FileExists(CertificatePath) and FileExists(KeyPath),
    'Hysteria integration test certificate is missing.');
  Check(AllocateLocalTcpPort(ServerPort, ErrorMessage),
    'Could not allocate Hysteria server port: ' + ErrorMessage);
  ServerConfigPath := IncludeTrailingPathDelimiter(ATempDirectory) +
    'hysteria-server.yaml';
  ServerConfig := 'listen: 127.0.0.1:' + IntToStr(ServerPort) + LineEnding +
    'tls:' + LineEnding +
    '  cert: ' + YamlPath(CertificatePath) + LineEnding +
    '  key: ' + YamlPath(KeyPath) + LineEnding +
    'auth:' + LineEnding +
    '  type: password' + LineEnding +
    '  password: integration-password' + LineEnding;
  WriteUtf8(ServerConfigPath, ServerConfig);
  Arguments := StringArray(['server', '-c', ServerConfigPath]);
  ServerRuntime := TZaryaExternalProcess.Create;
  Check(ServerRuntime.Start(AExecutable, ExtractFileDir(AExecutable),
    Arguments, ErrorMessage), 'Hysteria test server failed to start: ' +
    ErrorMessage);
  try
    Sleep(750);
    ServerOutput := ServerRuntime.DrainOutput;
    Check(ServerRuntime.IsRunning, 'Hysteria test server stopped early: ' +
      ServerOutput);
    AProfile.Port := ServerPort;
    ValidateAndStart(AExecutable, AProvider, AProfile, ATempDirectory);
  finally
    ServerRuntime.Stop;
    ServerRuntime := nil;
    DeleteFile(ServerConfigPath);
  end;
end;

var
  Profile: TZaryaProfile;
  TempDirectory: string;
  Argument, ProviderId, ExecutablePath: string;
  SeparatorIndex, I: Integer;
  Provider: TZaryaCoreProvider;
begin
  Randomize;
  Check(ParamCount >= 1,
    'Usage: ProviderIntegrationTest <provider-id=core.exe> [...]');
  TempDirectory := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-provider-integration-' + IntToHex(Random(MaxInt), 8);
  Check(ForceDirectories(TempDirectory), 'Could not create integration temp dir.');
  try
    for I := 1 to ParamCount do
    begin
      Argument := ParamStr(I);
      SeparatorIndex := Pos('=', Argument);
      Check(SeparatorIndex > 1, 'Invalid provider fixture argument: ' + Argument);
      ProviderId := Copy(Argument, 1, SeparatorIndex - 1);
      ExecutablePath := Copy(Argument, SeparatorIndex + 1, MaxInt);
      Check(FileExists(ExecutablePath), ProviderId + ': fixture is missing.');
      Provider := CreateProviderPreset(ProviderId);
      Check(Provider.ProviderId = ProviderId, 'Unknown provider: ' + ProviderId);
      ConfigureProfileForProvider(Profile, ProviderId);
      if SameText(ProviderId, ProviderExternalHysteria2) then
        ValidateHysteriaWithLocalServer(ExecutablePath, Provider, Profile,
          TempDirectory)
      else
        ValidateAndStart(ExecutablePath, Provider, Profile, TempDirectory);
    end;
  finally
    RemoveDir(TempDirectory);
  end;
  WriteLn('Real external provider adapters: PASS');
end.
