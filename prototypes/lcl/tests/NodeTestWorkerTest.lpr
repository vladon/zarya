program NodeTestWorkerTest;

{$mode objfpc}{$H+}

uses
  SysUtils, ZaryaCoreProvider, ZaryaRuntimeContracts, ZaryaNodeTestWorker,
  ZaryaTcpProbe;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then raise Exception.Create(AMessage);
end;

var
  Request, ParsedRequest: TZaryaNodeTestRequest;
  TestResult, ParsedResult: TZaryaNodeTestResult;
  Json, ErrorMessage: string;
  Port: Integer;
begin
  Request := Default(TZaryaNodeTestRequest);
  Request.SchemaVersion := 1;
  Request.Provider := CreateProviderPreset(ProviderExternalXray);
  Request.Provider.ExecutablePath := 'C:\Program Files\Core\xray.exe';
  Request.Provider.ConfirmedSha256 := StringOfChar('a', 64);
  Request.Config := '{"password":"must-survive-serialization"}';
  Request.DataDirectory := 'C:\ProgramData\Zarya test';
  Request.AssetDirectory := 'C:\ProgramData\Zarya test\assets';
  Request.ReadinessHost := '127.0.0.1';
  Request.ReadinessPort := 18080;
  Request.ProxyKind := 'mixed';
  Request.TestUrl := 'https://www.gstatic.com/generate_204';
  Request.TimeoutMs := 10000;
  Json := NodeTestRequestToJson(Request);
  Check(NodeTestRequestFromJson(Json, ParsedRequest, ErrorMessage),
    'Request round-trip failed: ' + ErrorMessage);
  Check(ParsedRequest.Config = Request.Config, 'Config changed in JSON round-trip.');
  Check(ParsedRequest.Provider.ExecutablePath =
    Request.Provider.ExecutablePath, 'Provider path changed in JSON round-trip.');

  TestResult := Default(TZaryaNodeTestResult);
  TestResult.Success := True;
  TestResult.DelayMs := 42;
  Json := 'untrusted noise' + LineEnding + NodeTestResultToJson(TestResult);
  Check(ParseFinalNodeTestResult(Json, ParsedResult, ErrorMessage),
    'Typed final worker line was not parsed: ' + ErrorMessage);
  Check(ParsedResult.Success and (ParsedResult.DelayMs = 42),
    'Parsed worker result is incorrect.');
  Check(not ParseFinalNodeTestResult('{"success":"yes"}', ParsedResult,
    ErrorMessage), 'Invalid success type was accepted.');

  Check(AllocateLocalTcpPort(Port, ErrorMessage),
    'Dynamic port allocation failed: ' + ErrorMessage);
  Check((Port > 0) and (Port <= 65535), 'Dynamic port is out of range.');
  WriteLn('Node test worker protocol: PASS');
end.
