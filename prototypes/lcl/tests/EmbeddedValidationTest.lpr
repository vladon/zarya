program EmbeddedValidationTest;

{$mode objfpc}{$H+}

uses
  SysUtils, ZaryaProfile, ZaryaXrayConfig, ZaryaEmbeddedXray;

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
  OperationOk: Boolean;
begin
  BridgePath := IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
    'zarya-xray.dll';
  {$IFNDEF ZARYA_STATIC_XRAY}
  Check(FileExists(BridgePath), 'Embedded Xray bridge is missing: ' + BridgePath);
  {$ENDIF}
  AssetDirectory := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-lcl-xray-validation-assets';
  Check(ForceDirectories(AssetDirectory), 'Could not create Xray asset directory.');

  Profile := CreateEmptyProfile;
  Profile.Name := 'Embedded validation smoke';
  Profile.Host := '127.0.0.1';
  Profile.Port := 9;
  Profile.Uuid := '11111111-1111-1111-1111-111111111111';
  Profile.Network := 'tcp';
  Profile.Security := 'none';
  Check(GenerateXrayConfig(Profile, 20808, Config, ErrorMessage),
    'Config generation failed: ' + ErrorMessage);

  Bridge := TZaryaEmbeddedXray.Create(BridgePath);
  try
    Check(Bridge.Available, 'Bridge load failed: ' + Bridge.LoadStatus);
    OperationOk := Bridge.Validate(Config, AssetDirectory, ErrorMessage);
    Check(OperationOk, 'Embedded validation failed: ' + ErrorMessage +
      LineEnding + Bridge.DrainLogs);
  finally
    Bridge.Free;
  end;
  RemoveDir(AssetDirectory);
  WriteLn('Embedded Xray validation: PASS');
end.
