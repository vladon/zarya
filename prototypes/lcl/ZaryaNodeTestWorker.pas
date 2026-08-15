unit ZaryaNodeTestWorker;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, ZaryaRuntimeContracts;

type
  TZaryaNodeTestWorker = class(TInterfacedObject, INodeTestWorker)
  private
    FCancelFlag: LongInt;
  public
    function Run(const ARequest: TZaryaNodeTestRequest;
      out AResult: TZaryaNodeTestResult; out AWorkerLog: string): Boolean;
    procedure Cancel;
  end;

function NodeTestRequestToJson(const ARequest: TZaryaNodeTestRequest): string;
function NodeTestRequestFromJson(const AJson: string;
  out ARequest: TZaryaNodeTestRequest; out AError: string): Boolean;
function NodeTestResultToJson(const AResult: TZaryaNodeTestResult): string;
function ParseFinalNodeTestResult(const AOutput: string;
  out AResult: TZaryaNodeTestResult; out AError: string): Boolean;
function RunCoreTestWorkerMode(out AExitCode: Integer): Boolean;

implementation

uses
  Process, Pipes, fpjson, jsonparser, ZaryaCoreProvider,
  ZaryaRuntimeProcess, ZaryaRuntimeConfigFile, ZaryaEmbeddedXray,
  ZaryaTcpProbe, ZaryaFileIntegrity
  {$IFDEF WINDOWS}, Windows{$ENDIF};

const
  NodeTestSchemaVersion = 1;
  MaxWorkerRequestBytes = 32 * 1024 * 1024;

function MinInt64(const A, B: Int64): Int64;
begin
  if A < B then Result := A else Result := B;
end;

function StringsToJson(const AValues: TZaryaStringArray): TJSONArray;
var
  Value: string;
begin
  Result := TJSONArray.Create;
  for Value in AValues do Result.Add(Value);
end;

function JsonToStrings(const AValue: TJSONData): TZaryaStringArray;
var
  Items: TJSONArray;
  I: Integer;
begin
  Result := nil;
  if not Assigned(AValue) or (AValue.JSONType <> jtArray) then Exit;
  Items := TJSONArray(AValue);
  SetLength(Result, Items.Count);
  for I := 0 to Items.Count - 1 do Result[I] := Items.Strings[I];
end;

function NodeTestRequestToJson(const ARequest: TZaryaNodeTestRequest): string;
var
  Root, Provider: TJSONObject;
begin
  Root := TJSONObject.Create;
  try
    Root.Add('schemaVersion', NodeTestSchemaVersion);
    Provider := TJSONObject.Create;
    Root.Add('provider', Provider);
    Provider.Add('providerId', ARequest.Provider.ProviderId);
    Provider.Add('distribution', DistributionToString(
      ARequest.Provider.Distribution));
    Provider.Add('executablePath', ARequest.Provider.ExecutablePath);
    Provider.Add('workingDirectory', ARequest.Provider.WorkingDirectory);
    Provider.Add('assetDirectory', ARequest.Provider.AssetDirectory);
    Provider.Add('configExtension', ARequest.Provider.ConfigExtension);
    Provider.Add('confirmedSha256', ARequest.Provider.ConfirmedSha256);
    Provider.Add('validateArguments', StringsToJson(
      ARequest.Provider.ValidateArguments));
    Provider.Add('runArguments', StringsToJson(ARequest.Provider.RunArguments));
    Root.Add('config', ARequest.Config);
    Root.Add('dataDirectory', ARequest.DataDirectory);
    Root.Add('assetDirectory', ARequest.AssetDirectory);
    Root.Add('readinessHost', ARequest.ReadinessHost);
    Root.Add('readinessPort', ARequest.ReadinessPort);
    Root.Add('proxyKind', ARequest.ProxyKind);
    Root.Add('testUrl', ARequest.TestUrl);
    Root.Add('timeoutMs', ARequest.TimeoutMs);
    Result := Root.AsJSON;
  finally
    Root.Free;
  end;
end;

function NodeTestRequestFromJson(const AJson: string;
  out ARequest: TZaryaNodeTestRequest; out AError: string): Boolean;
var
  Data: TJSONData;
  Root, Provider: TJSONObject;
  Distribution: string;
begin
  Result := False;
  ARequest := Default(TZaryaNodeTestRequest);
  AError := '';
  Data := nil;
  try
    if Length(UTF8String(AJson)) > MaxWorkerRequestBytes then
      raise Exception.Create('Worker request exceeds the size limit.');
    Data := GetJSON(AJson);
    if Data.JSONType <> jtObject then
      raise Exception.Create('Worker request root must be an object.');
    Root := TJSONObject(Data);
    ARequest.SchemaVersion := Root.Get('schemaVersion', 0);
    if ARequest.SchemaVersion <> NodeTestSchemaVersion then
      raise Exception.Create('Unsupported worker request schema.');
    if not Assigned(Root.Find('provider')) or
      (Root.Find('provider').JSONType <> jtObject) then
      raise Exception.Create('Worker request has no provider object.');
    Provider := TJSONObject(Root.Find('provider'));
    ARequest.Provider := CreateProviderPreset(
      Provider.Get('providerId', ''));
    ARequest.Provider.ProviderId := Provider.Get('providerId', '');
    if Trim(ARequest.Provider.ProviderId) = '' then
      raise Exception.Create('Worker provider id is empty.');
    Distribution := Provider.Get('distribution', 'external');
    if SameText(Distribution, 'embedded') then
      ARequest.Provider.Distribution := pdEmbedded
    else if SameText(Distribution, 'external') then
      ARequest.Provider.Distribution := pdExternal
    else
      raise Exception.Create('Worker provider distribution is invalid.');
    ARequest.Provider.ExecutablePath := Provider.Get('executablePath', '');
    ARequest.Provider.WorkingDirectory := Provider.Get('workingDirectory', '');
    ARequest.Provider.AssetDirectory := Provider.Get('assetDirectory', '');
    ARequest.Provider.ConfigExtension := Provider.Get('configExtension', '.conf');
    ARequest.Provider.ConfirmedSha256 := LowerCase(
      Provider.Get('confirmedSha256', ''));
    ARequest.Provider.ValidateArguments := JsonToStrings(
      Provider.Find('validateArguments'));
    ARequest.Provider.RunArguments := JsonToStrings(
      Provider.Find('runArguments'));
    ARequest.Config := Root.Get('config', '');
    ARequest.DataDirectory := Root.Get('dataDirectory', '');
    ARequest.AssetDirectory := Root.Get('assetDirectory', '');
    ARequest.ReadinessHost := Root.Get('readinessHost', '');
    ARequest.ReadinessPort := Root.Get('readinessPort', 0);
    ARequest.ProxyKind := LowerCase(Trim(Root.Get('proxyKind', '')));
    ARequest.TestUrl := Trim(Root.Get('testUrl', ''));
    ARequest.TimeoutMs := Root.Get('timeoutMs', 0);
    if ARequest.Config = '' then
      raise Exception.Create('Worker config is empty.');
    if Trim(ARequest.DataDirectory) = '' then
      raise Exception.Create('Worker data directory is empty.');
    if not SameText(ARequest.ReadinessHost, '127.0.0.1') and
      not SameText(ARequest.ReadinessHost, 'localhost') then
      raise Exception.Create('Worker readiness endpoint must be local.');
    if (ARequest.ReadinessPort < 1) or (ARequest.ReadinessPort > 65535) then
      raise Exception.Create('Worker readiness port is invalid.');
    if not ((ARequest.ProxyKind = 'mixed') or
      (ARequest.ProxyKind = 'http') or (ARequest.ProxyKind = 'socks')) then
      raise Exception.Create('Worker proxy kind is invalid.');
    if (Pos('https://', LowerCase(ARequest.TestUrl)) <> 1) and
      (Pos('http://', LowerCase(ARequest.TestUrl)) <> 1) then
      raise Exception.Create('Worker test URL must use HTTP or HTTPS.');
    if (ARequest.TimeoutMs < 1000) or (ARequest.TimeoutMs > 60000) then
      raise Exception.Create('Worker timeout must be between 1000 and 60000 ms.');
    if ARequest.Provider.Distribution = pdExternal then
    begin
      if Trim(ARequest.Provider.ExecutablePath) = '' then
        raise Exception.Create('External worker provider path is empty.');
      if Trim(ARequest.Provider.ConfirmedSha256) = '' then
        raise Exception.Create('External worker provider hash is not confirmed.');
      if Length(ARequest.Provider.RunArguments) = 0 then
        raise Exception.Create('External worker provider has no start command.');
    end;
    Result := True;
  except
    on E: Exception do AError := E.Message;
  end;
  Data.Free;
end;

function NodeTestResultToJson(const AResult: TZaryaNodeTestResult): string;
var
  Root: TJSONObject;
begin
  Root := TJSONObject.Create;
  try
    Root.Add('success', AResult.Success);
    Root.Add('errorCode', AResult.ErrorCode);
    Root.Add('message', AResult.MessageText);
    Root.Add('delayMs', AResult.DelayMs);
    Result := Root.AsJSON;
  finally
    Root.Free;
  end;
end;

function TryParseNodeTestResultLine(const ALine: string;
  out AResult: TZaryaNodeTestResult): Boolean;
var
  Data: TJSONData;
  Root: TJSONObject;
begin
  Result := False;
  Data := nil;
  try
    Data := GetJSON(ALine);
    if Data.JSONType <> jtObject then Exit;
    Root := TJSONObject(Data);
    if not Assigned(Root.Find('success')) or
      (Root.Find('success').JSONType <> jtBoolean) then Exit;
    AResult := Default(TZaryaNodeTestResult);
    AResult.Success := Root.Booleans['success'];
    AResult.ErrorCode := Root.Get('errorCode', '');
    AResult.MessageText := Root.Get('message', '');
    AResult.DelayMs := Root.Get('delayMs', Int64(-1));
    if AResult.Success and (AResult.DelayMs < 0) then Exit;
    if (not AResult.Success) and (Trim(AResult.ErrorCode) = '') then Exit;
    Result := True;
  except
    Result := False;
  end;
  Data.Free;
end;

function ParseFinalNodeTestResult(const AOutput: string;
  out AResult: TZaryaNodeTestResult; out AError: string): Boolean;
var
  Lines: TStringList;
  I: Integer;
begin
  Result := False;
  AResult := Default(TZaryaNodeTestResult);
  AError := 'Worker did not return a typed JSON result.';
  Lines := TStringList.Create;
  try
    Lines.Text := AOutput;
    for I := Lines.Count - 1 downto 0 do
      if (Trim(Lines[I]) <> '') and
        TryParseNodeTestResultLine(Trim(Lines[I]), AResult) then
      begin
        Result := True;
        AError := '';
        Exit;
      end;
  finally
    Lines.Free;
  end;
end;

function CreateFailure(const ACode, AMessage: string): TZaryaNodeTestResult;
begin
  Result := Default(TZaryaNodeTestResult);
  Result.Success := False;
  Result.ErrorCode := ACode;
  Result.MessageText := AMessage;
  Result.DelayMs := -1;
end;

function RemainingMs(const ADeadline: QWord): Integer;
var
  NowTicks: QWord;
begin
  NowTicks := GetTickCount64;
  if NowTicks >= ADeadline then Exit(0);
  Result := Integer(MinInt64(ADeadline - NowTicks, High(Integer)));
end;

function WriteWorkerConfig(const ARequest: TZaryaNodeTestRequest;
  out AFileName, AError: string): Boolean;
var
  DirectoryName: string;
  Stream: TFileStream;
  Bytes: UTF8String;
begin
  Result := False;
  AFileName := '';
  AError := '';
  DirectoryName := IncludeTrailingPathDelimiter(ARequest.DataDirectory) +
    'run-tests';
  if not ForceDirectories(DirectoryName) then
  begin
    AError := 'Cannot create the worker config directory.';
    Exit;
  end;
  AFileName := IncludeTrailingPathDelimiter(DirectoryName) + 'test-' +
    IntToHex(GetTickCount64, 16) + '-' + IntToHex(Random(MaxInt), 8) +
    ARequest.Provider.ConfigExtension;
  Bytes := UTF8String(ARequest.Config);
  try
    Stream := TFileStream.Create(AFileName, fmCreate or fmShareExclusive);
    try
      if Length(Bytes) > 0 then Stream.WriteBuffer(Bytes[1], Length(Bytes));
    finally
      Stream.Free;
    end;
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      AFileName := '';
    end;
  end;
end;

function ExecuteNodeTestInWorker(const ARequest: TZaryaNodeTestRequest;
  out AResult: TZaryaNodeTestResult): Boolean;
var
  Embedded: TZaryaEmbeddedXray;
  ExternalRuntime: IZaryaRuntimeProcess;
  Context: TZaryaProcessContext;
  Arguments: TZaryaStringArray;
  ConfigPath, ErrorMessage, Output, Digest: string;
  ExitCode, ProbeTimeout: Integer;
  Deadline, ReadyDeadline: QWord;
  DelayMs: Int64;
  StartedEmbedded: Boolean;
begin
  Result := False;
  AResult := CreateFailure('internal_error', 'Node test did not complete.');
  Embedded := nil;
  ExternalRuntime := nil;
  ConfigPath := '';
  StartedEmbedded := False;
  Deadline := GetTickCount64 + QWord(ARequest.TimeoutMs);
  try
    if not WriteWorkerConfig(ARequest, ConfigPath, ErrorMessage) then
    begin
      AResult := CreateFailure('config_write_failed', ErrorMessage);
      Exit;
    end;
    Embedded := TZaryaEmbeddedXray.Create('');
    if not Embedded.Available then
    begin
      AResult := CreateFailure('embedded_bridge_unavailable',
        Embedded.LoadStatus);
      Exit;
    end;
    if ARequest.Provider.Distribution = pdEmbedded then
    begin
      if not SameText(ARequest.Provider.ProviderId, ProviderEmbeddedXray) then
      begin
        AResult := CreateFailure('provider_unavailable',
          'This embedded provider is not available in the stable build.');
        Exit;
      end;
      if not Embedded.Validate(ARequest.Config, ARequest.AssetDirectory,
        ErrorMessage) then
      begin
        AResult := CreateFailure('validation_failed', ErrorMessage);
        Exit;
      end;
      if not Embedded.Start(ARequest.Config, ARequest.AssetDirectory,
        ErrorMessage) then
      begin
        AResult := CreateFailure('start_failed', ErrorMessage);
        Exit;
      end;
      StartedEmbedded := True;
    end
    else
    begin
      if not Sha256File(ARequest.Provider.ExecutablePath, Digest,
        ErrorMessage) or not SameText(Digest,
        ARequest.Provider.ConfirmedSha256) then
      begin
        if ErrorMessage = '' then
          ErrorMessage := 'External provider hash changed.';
        AResult := CreateFailure('provider_changed', ErrorMessage);
        Exit;
      end;
      Context := Default(TZaryaProcessContext);
      Context.ConfigPath := ConfigPath;
      Context.DataDirectory := ARequest.DataDirectory;
      Context.AssetDirectory := ARequest.AssetDirectory;
      Context.MixedPort := ARequest.ReadinessPort;
      Context.HttpPort := ARequest.ReadinessPort;
      Context.SocksPort := ARequest.ReadinessPort;
      Context.LogLevel := 'warning';
      if Length(ARequest.Provider.ValidateArguments) > 0 then
      begin
        if not ExpandProviderArguments(ARequest.Provider.ValidateArguments,
          Context, Arguments, ErrorMessage) then
        begin
          AResult := CreateFailure('validation_failed', ErrorMessage);
          Exit;
        end;
        ProbeTimeout := RemainingMs(Deadline);
        if ProbeTimeout > 5000 then ProbeTimeout := 5000;
        if (ProbeTimeout < 1) or not RunProcessProbe(
          ARequest.Provider.ExecutablePath,
          ARequest.Provider.WorkingDirectory, Arguments, ProbeTimeout,
          Output, ExitCode, ErrorMessage) then
        begin
          AResult := CreateFailure('validation_failed', ErrorMessage);
          Exit;
        end;
      end;
      if not ExpandProviderArguments(ARequest.Provider.RunArguments,
        Context, Arguments, ErrorMessage) then
      begin
        AResult := CreateFailure('start_failed', ErrorMessage);
        Exit;
      end;
      ExternalRuntime := TZaryaExternalProcess.Create;
      if not ExternalRuntime.Start(ARequest.Provider.ExecutablePath,
        ARequest.Provider.WorkingDirectory, Arguments, ErrorMessage) then
      begin
        AResult := CreateFailure('start_failed', ErrorMessage);
        Exit;
      end;
    end;

    ReadyDeadline := GetTickCount64 + QWord(MinInt64(5000,
      RemainingMs(Deadline)));
    while (GetTickCount64 < ReadyDeadline) and
      not CanConnectLocalhost(ARequest.ReadinessPort) do
    begin
      if Assigned(ExternalRuntime) and not ExternalRuntime.IsRunning then
      begin
        AResult := CreateFailure('runtime_crashed',
          'Runtime stopped before readiness.');
        Exit;
      end;
      Sleep(100);
    end;
    if not CanConnectLocalhost(ARequest.ReadinessPort) then
    begin
      AResult := CreateFailure('readiness_timeout',
        'Local runtime endpoint did not become ready.');
      Exit;
    end;
    ProbeTimeout := RemainingMs(Deadline);
    if ProbeTimeout < 1 then
    begin
      AResult := CreateFailure('timeout', 'Node test timed out.');
      Exit;
    end;
    if not Embedded.ProbeUrl(ARequest.TestUrl, ARequest.ProxyKind,
      ARequest.ReadinessHost, ARequest.ReadinessPort, ProbeTimeout,
      DelayMs, ErrorMessage) then
    begin
      AResult := CreateFailure('probe_failed', ErrorMessage);
      Exit;
    end;
    AResult := Default(TZaryaNodeTestResult);
    AResult.Success := True;
    AResult.DelayMs := DelayMs;
    Result := True;
  finally
    if Assigned(ExternalRuntime) then ExternalRuntime.Stop;
    if StartedEmbedded and Assigned(Embedded) then
      Embedded.Stop(ErrorMessage);
    Embedded.Free;
    DeleteRuntimeConfig(ConfigPath);
  end;
end;

procedure DrainPipe(const APipe: TInputPipeStream; var AText: UTF8String);
var
  Buffer: array[0..8191] of Byte;
  Available, ReadCount: LongInt;
  Chunk: UTF8String;
begin
  if not Assigned(APipe) then Exit;
  repeat
    Available := APipe.NumBytesAvailable;
    if Available <= 0 then Exit;
    if Available > SizeOf(Buffer) then Available := SizeOf(Buffer);
    ReadCount := APipe.Read(Buffer, Available);
    if ReadCount <= 0 then Exit;
    SetString(Chunk, PAnsiChar(@Buffer[0]), ReadCount);
    AText := AText + Chunk;
  until False;
end;

function TZaryaNodeTestWorker.Run(const ARequest: TZaryaNodeTestRequest;
  out AResult: TZaryaNodeTestResult; out AWorkerLog: string): Boolean;
var
  Child: TProcess;
  RequestText, ErrorMessage: string;
  InputBytes, StdoutText, StderrText: UTF8String;
  Deadline: QWord;
begin
  Result := False;
  AResult := CreateFailure('worker_failed', 'Worker did not start.');
  AWorkerLog := '';
  RequestText := NodeTestRequestToJson(ARequest) + LineEnding;
  InputBytes := UTF8String(RequestText);
  StdoutText := '';
  StderrText := '';
  Child := TProcess.Create(nil);
  try
    Child.Executable := ParamStr(0);
    Child.Parameters.Add('--core-test-worker');
    Child.CurrentDirectory := ExtractFileDir(ParamStr(0));
    Child.Options := [poUsePipes, poNoConsole];
    try
      Child.Execute;
      if Length(InputBytes) > 0 then
        Child.Input.WriteBuffer(InputBytes[1], Length(InputBytes));
      Deadline := GetTickCount64 + QWord(ARequest.TimeoutMs + 1500);
      while Child.Running and (GetTickCount64 < Deadline) and
        (InterlockedCompareExchange(FCancelFlag, 0, 0) = 0) do
      begin
        DrainPipe(Child.Output, StdoutText);
        DrainPipe(Child.Stderr, StderrText);
        Sleep(10);
      end;
      if Child.Running then
      begin
        Child.Terminate(1);
        Child.WaitOnExit(1500);
        if InterlockedCompareExchange(FCancelFlag, 0, 0) <> 0 then
          AResult := CreateFailure('cancelled', 'Node test was cancelled.')
        else
          AResult := CreateFailure('worker_timeout', 'Node test worker timed out.');
        Exit(False);
      end;
      Child.WaitOnExit;
      DrainPipe(Child.Output, StdoutText);
      DrainPipe(Child.Stderr, StderrText);
      AWorkerLog := string(StderrText);
      if not ParseFinalNodeTestResult(string(StdoutText), AResult,
        ErrorMessage) then
      begin
        AResult := CreateFailure('worker_protocol', ErrorMessage);
        Exit(False);
      end;
      Result := AResult.Success;
    except
      on E: Exception do
      begin
        AResult := CreateFailure('worker_start_failed', E.Message);
        Result := False;
      end;
    end;
  finally
    Child.Free;
  end;
end;

procedure TZaryaNodeTestWorker.Cancel;
begin
  InterlockedExchange(FCancelFlag, 1);
end;

function ReadWorkerRequest: string;
var
  Stream: THandleStream;
  Value: UTF8String;
  ByteValue: Byte;
begin
  {$IFDEF WINDOWS}
  Value := '';
  Stream := THandleStream.Create(GetStdHandle(STD_INPUT_HANDLE));
  try
    while Length(Value) <= MaxWorkerRequestBytes do
    begin
      if Stream.Read(ByteValue, 1) <> 1 then Break;
      if ByteValue = 10 then Break;
      if ByteValue <> 13 then Value := Value + AnsiChar(ByteValue);
    end;
  finally
    Stream.Free;
  end;
  if Length(Value) > MaxWorkerRequestBytes then
    raise Exception.Create('Worker request exceeds the size limit.');
  Result := string(Value);
  {$ELSE}
  ReadLn(Input, Result);
  {$ENDIF}
end;

procedure WriteStandardLine(const AHandle: THandle; const AText: string);
var
  Stream: THandleStream;
  Value: UTF8String;
begin
  Value := UTF8String(AText + LineEnding);
  Stream := THandleStream.Create(AHandle);
  try
    if Length(Value) > 0 then Stream.WriteBuffer(Value[1], Length(Value));
  finally
    Stream.Free;
  end;
end;

procedure WriteWorkerOutput(const AText: string);
begin
  {$IFDEF WINDOWS}
  WriteStandardLine(GetStdHandle(STD_OUTPUT_HANDLE), AText);
  {$ELSE}
  WriteLn(Output, AText);
  Flush(Output);
  {$ENDIF}
end;

procedure WriteWorkerLog(const AText: string);
begin
  {$IFDEF WINDOWS}
  WriteStandardLine(GetStdHandle(STD_ERROR_HANDLE), AText);
  {$ELSE}
  WriteLn(StdErr, AText);
  Flush(StdErr);
  {$ENDIF}
end;

function RunCoreTestWorkerMode(out AExitCode: Integer): Boolean;
var
  Request: TZaryaNodeTestRequest;
  TestResult: TZaryaNodeTestResult;
  RequestText, ErrorMessage: string;
begin
  Result := (ParamCount >= 1) and SameText(ParamStr(1), '--core-test-worker');
  if not Result then Exit;
  AExitCode := 2;
  try
    RequestText := ReadWorkerRequest;
    if not NodeTestRequestFromJson(RequestText, Request, ErrorMessage) then
      TestResult := CreateFailure('invalid_request', ErrorMessage)
    else
    begin
      WriteWorkerLog('core-test-worker: provider=' +
        Request.Provider.ProviderId);
      ExecuteNodeTestInWorker(Request, TestResult);
      if TestResult.Success then AExitCode := 0 else AExitCode := 1;
    end;
  except
    on E: Exception do
      TestResult := CreateFailure('worker_exception', E.Message);
  end;
  WriteWorkerOutput(NodeTestResultToJson(TestResult));
end;

end.
