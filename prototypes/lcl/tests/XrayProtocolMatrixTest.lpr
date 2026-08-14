program XrayProtocolMatrixTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, fpjson, jsonparser, ZaryaProfile, ZaryaXrayConfig,
  ZaryaRuntimeProcess, ZaryaCoreProvider;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

procedure WriteUtf8File(const AFileName, AContent: string);
var
  Stream: TFileStream;
  Bytes: UTF8String;
begin
  Bytes := UTF8String(AContent);
  Stream := TFileStream.Create(AFileName, fmCreate or fmShareExclusive);
  try
    if Length(Bytes) > 0 then
      Stream.WriteBuffer(Bytes[1], Length(Bytes));
  finally
    Stream.Free;
  end;
end;

function ReadUtf8File(const AFileName: string): string;
var
  Stream: TFileStream;
  Bytes: UTF8String;
begin
  if not FileExists(AFileName) then
    Exit('');
  Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyWrite);
  try
    SetLength(Bytes, Stream.Size);
    if Length(Bytes) > 0 then
      Stream.ReadBuffer(Bytes[1], Length(Bytes));
  finally
    Stream.Free;
  end;
  Result := string(Bytes);
end;

procedure GenerateAndValidate(const AProfile: TZaryaProfile;
  const ATempDirectory, AZaryaExe, AAssetDirectory: string);
var
  Config: string;
  ErrorMessage: string;
  ConfigFile: string;
  ErrorFile: string;
  Output: string;
  ExitCode: Integer;
  Arguments: TZaryaStringArray;
  Parsed: TJSONData;
begin
  Check(GenerateXrayConfig(AProfile, 21990, Config, ErrorMessage),
    AProfile.ProtocolName + ' generation failed: ' + ErrorMessage);
  Parsed := GetJSON(Config);
  try
    Check(Parsed.JSONType = jtObject,
      AProfile.ProtocolName + ' config root is not an object.');
  finally
    Parsed.Free;
  end;

  ConfigFile := IncludeTrailingPathDelimiter(ATempDirectory) +
    LowerCase(AProfile.ProtocolName) + '.json';
  ErrorFile := ConfigFile + '.error';
  WriteUtf8File(ConfigFile, Config);
  Arguments := StringArray(['--embedded-validate', ConfigFile,
    AAssetDirectory, ErrorFile]);
  Check(RunProcessProbe(AZaryaExe, ExtractFileDir(AZaryaExe), Arguments,
    15000, Output, ExitCode, ErrorMessage),
    AProfile.ProtocolName + ' rejected by embedded Xray: ' +
    ErrorMessage + ' ' + ReadUtf8File(ErrorFile));
  DeleteFile(ErrorFile);
  DeleteFile(ConfigFile);
end;

var
  Profile: TZaryaProfile;
  TempDirectory: string;
  ZaryaExe: string;
  AssetDirectory: string;
  InvalidConfig: string;
  ErrorMessage: string;
begin
  Randomize;
  TempDirectory := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-xray-matrix-' + IntToHex(Random(MaxInt), 8);
  Check(ForceDirectories(TempDirectory), 'Could not create matrix temp dir.');
  ZaryaExe := ExpandFileName(IncludeTrailingPathDelimiter(
    ExtractFileDir(ParamStr(0))) + '..' + PathDelim + '..' + PathDelim +
    'bin' + PathDelim + 'Zarya.exe');
  AssetDirectory := ExpandFileName(IncludeTrailingPathDelimiter(
    ExtractFileDir(ParamStr(0))) + '..' + PathDelim + '..' + PathDelim +
    '..' + PathDelim + '..' + PathDelim + 'build' + PathDelim + 'Release' +
    PathDelim + 'cores' + PathDelim + 'xray');
  Check(FileExists(ZaryaExe), 'Production Zarya.exe is missing.');

  Profile := CreateEmptyProfile;
  Profile.Name := 'VMess fixture';
  Profile.ProtocolName := 'VMess';
  Profile.Host := 'example.invalid';
  Profile.Uuid := '11111111-1111-1111-1111-111111111111';
  Profile.Security := 'tls';
  Profile.ServerName := 'example.invalid';
  GenerateAndValidate(Profile, TempDirectory, ZaryaExe, AssetDirectory);

  Profile := CreateEmptyProfile;
  Profile.Name := 'Trojan fixture';
  Profile.ProtocolName := 'Trojan';
  Profile.Host := 'example.invalid';
  Profile.Password := 'fixture-password';
  Profile.Security := 'tls';
  Profile.ServerName := 'example.invalid';
  GenerateAndValidate(Profile, TempDirectory, ZaryaExe, AssetDirectory);

  Profile := CreateEmptyProfile;
  Profile.Name := 'Shadowsocks fixture';
  Profile.ProtocolName := 'Shadowsocks';
  Profile.Host := 'example.invalid';
  Profile.Password := 'fixture-password';
  Profile.Method := 'aes-128-gcm';
  GenerateAndValidate(Profile, TempDirectory, ZaryaExe, AssetDirectory);

  Profile := CreateEmptyProfile;
  Profile.Name := 'SOCKS fixture';
  Profile.ProtocolName := 'SOCKS';
  Profile.Host := 'example.invalid';
  Profile.Password := 'fixture-password';
  GenerateAndValidate(Profile, TempDirectory, ZaryaExe, AssetDirectory);

  Profile := CreateEmptyProfile;
  Profile.Name := 'Hysteria2 fixture';
  Profile.ProtocolName := 'Hysteria2';
  Profile.Host := 'example.invalid';
  Profile.Password := 'fixture-password';
  Profile.Security := 'tls';
  Profile.ServerName := 'example.invalid';
  GenerateAndValidate(Profile, TempDirectory, ZaryaExe, AssetDirectory);

  Profile := CreateEmptyProfile;
  Profile.Name := 'WireGuard fixture';
  Profile.ProtocolName := 'WireGuard';
  Profile.Host := 'example.invalid';
  Profile.Password := 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=';
  Profile.PublicKey := 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=';
  Profile.LocalAddress := '172.16.0.2/32';
  Profile.AllowedIps := '0.0.0.0/0,::/0';
  Profile.Reserved := '0,0,0';
  GenerateAndValidate(Profile, TempDirectory, ZaryaExe, AssetDirectory);

  Profile := CreateEmptyProfile;
  Profile.Name := 'Invalid Hysteria2 fixture';
  Profile.ProtocolName := 'Hysteria2';
  Profile.Host := 'example.invalid';
  Profile.Password := 'fixture-password';
  Profile.Obfs := 'salamander';
  Check(not GenerateXrayConfig(Profile, 21990, InvalidConfig, ErrorMessage),
    'Invalid Hysteria2 obfs profile was accepted.');

  RemoveDir(TempDirectory);
  WriteLn('Embedded Xray protocol matrix: PASS');
end.
