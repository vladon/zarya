program EmbeddedRuntimeTest;

{$mode objfpc}{$H+}

uses
  SysUtils, ZaryaProfile, ZaryaXrayConfig, ZaryaEmbeddedXray, ZaryaTcpProbe;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

var
  BridgePath: string;
  AssetDirectory: string;
  Bridge: TZaryaEmbeddedXray;
  Profile: TZaryaProfile;
  Config: string;
  ErrorMessage: string;
  MixedPort: Integer;
  Ready: Boolean;
  OperationOk: Boolean;
  Attempt: Integer;
begin
  Randomize;
  BridgePath := IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
    'zarya-xray.dll';
  {$IFNDEF ZARYA_STATIC_XRAY}
  Check(FileExists(BridgePath), 'Embedded Xray bridge is missing: ' + BridgePath);
  {$ENDIF}
  AssetDirectory := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-lcl-xray-assets';
  Check(ForceDirectories(AssetDirectory), 'Could not create Xray asset directory.');
  Check(AllocateLocalTcpUdpPort(MixedPort, ErrorMessage),
    'Could not allocate a mixed port: ' + ErrorMessage);

  Profile := CreateEmptyProfile;
  Profile.Name := 'Embedded runtime smoke';
  Profile.Host := '127.0.0.1';
  Profile.Port := 9;
  Profile.Uuid := '11111111-1111-1111-1111-111111111111';
  Profile.Network := 'tcp';
  Profile.Security := 'none';
  Check(GenerateXrayConfig(Profile, MixedPort, Config, ErrorMessage),
    'Config generation failed: ' + ErrorMessage);

  Bridge := TZaryaEmbeddedXray.Create(BridgePath);
  try
    Check(Bridge.Available, 'Bridge load failed: ' + Bridge.LoadStatus);
    OperationOk := Bridge.Start(Config, AssetDirectory, ErrorMessage);
    Check(OperationOk, 'Embedded start failed: ' + ErrorMessage +
      LineEnding + Bridge.DrainLogs);
    try
      Ready := False;
      for Attempt := 1 to 50 do
      begin
        if CanConnectLocalhost(MixedPort) then
        begin
          Ready := True;
          Break;
        end;
        Sleep(100);
      end;
      Check(Ready, 'Embedded Xray mixed port did not become ready.');
      Check(Bridge.State = xrsRunning, 'Embedded Xray did not report running state.');
    finally
      OperationOk := Bridge.Stop(ErrorMessage);
      Check(OperationOk, 'Embedded stop failed: ' + ErrorMessage);
    end;
    Check(Bridge.State = xrsStopped, 'Embedded Xray did not stop.');
  finally
    Bridge.Free;
  end;
  RemoveDir(AssetDirectory);
  WriteLn('Embedded Xray runtime: PASS');
end.
