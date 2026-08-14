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

function FindCandidatePort: Integer;
var
  Attempt: Integer;
  Port: Integer;
begin
  for Attempt := 1 to 200 do
  begin
    Port := 20000 + Random(30000);
    if not CanConnectLocalhost(Port) then
      Exit(Port);
  end;
  Result := 0;
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
    Context.MixedPort := FindCandidatePort;
    Context.HttpPort := Context.MixedPort;
    Context.SocksPort := Context.MixedPort;
    Check(Context.MixedPort <> 0, 'Could not find a provider smoke port.');
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
    Check(ExpandProviderArguments(AProvider.ValidateArguments, ProcessContext,
      Arguments, ErrorMessage), AProvider.ProviderId +
      ': validation args failed: ' + ErrorMessage);
    Check(RunProcessProbe(AExecutable, ExtractFileDir(AExecutable), Arguments,
      10000, Output, ExitCode, ErrorMessage), AProvider.ProviderId +
      ': real validation failed: ' + ErrorMessage + LineEnding + Output);
    Check(ExpandProviderArguments(AProvider.RunArguments, ProcessContext,
      Arguments, ErrorMessage), AProvider.ProviderId +
      ': run args failed: ' + ErrorMessage);
    Runtime := TZaryaExternalProcess.Create;
    Check(Runtime.Start(AExecutable, ExtractFileDir(AExecutable), Arguments,
      ErrorMessage), AProvider.ProviderId + ': start failed: ' + ErrorMessage);
    try
      Deadline := GetTickCount64 + 5000;
      while Runtime.IsRunning and (GetTickCount64 < Deadline) do
      begin
        if CanConnectLocalhost(Context.MixedPort) then
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
  while CanConnectLocalhost(Context.MixedPort) and
    (GetTickCount64 < Deadline) do
    Sleep(20);
  Check(not CanConnectLocalhost(Context.MixedPort),
    AProvider.ProviderId + ': listener remained after Job stop.');
  DeleteFile(ConfigFile);
end;

var
  Profile: TZaryaProfile;
  TempDirectory: string;
begin
  Randomize;
  Check(ParamCount >= 2,
    'Usage: ProviderIntegrationTest <xray.exe> <sing-box.exe>');
  Check(FileExists(ParamStr(1)), 'External Xray fixture is missing.');
  Check(FileExists(ParamStr(2)), 'External sing-box fixture is missing.');
  TempDirectory := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-provider-integration-' + IntToHex(Random(MaxInt), 8);
  Check(ForceDirectories(TempDirectory), 'Could not create integration temp dir.');
  try
    Profile := CreateEmptyProfile;
    Profile.Name := 'Provider integration';
    Profile.ProtocolName := 'VLESS';
    Profile.Host := '127.0.0.1';
    Profile.Port := 9;
    Profile.Uuid := '11111111-1111-1111-1111-111111111111';
    Profile.Security := 'none';
    Profile.Network := 'tcp';
    ValidateAndStart(ParamStr(1),
      CreateProviderPreset(ProviderExternalXray), Profile, TempDirectory);
    ValidateAndStart(ParamStr(2),
      CreateProviderPreset(ProviderExternalSingBox), Profile, TempDirectory);
  finally
    RemoveDir(TempDirectory);
  end;
  WriteLn('Real external Xray/sing-box adapters: PASS');
end.
