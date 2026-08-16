program KnownCoresManifestTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, fpjson, jsonparser, ZaryaCoreProvider;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then raise Exception.Create(AMessage);
end;

function IsLowerHexSha256(const AValue: string): Boolean;
var
  Ch: Char;
begin
  Result := Length(AValue) = 64;
  if not Result then Exit;
  for Ch in AValue do
    if not (Ch in ['0'..'9', 'a'..'f']) then Exit(False);
end;

function ArraysEqual(const AJson: TJSONArray;
  const AExpected: TZaryaStringArray): Boolean;
var
  I: Integer;
begin
  Result := Assigned(AJson) and (AJson.Count = Length(AExpected));
  if not Result then Exit;
  for I := 0 to High(AExpected) do
    if AJson.Strings[I] <> AExpected[I] then Exit(False);
end;

var
  Stream: TFileStream;
  Data: TJSONData;
  Root, Item: TJSONObject;
  Items: TJSONArray;
  Provider: TZaryaCoreProvider;
  ProviderId, Url, ExecutablePath: string;
  I: Integer;
begin
  Stream := TFileStream.Create('known-cores.json', fmOpenRead or fmShareDenyWrite);
  Data := nil;
  try
    Data := GetJSON(Stream);
  finally
    Stream.Free;
  end;
  try
    Check(Data.JSONType = jtObject, 'Manifest root must be an object.');
    Root := TJSONObject(Data);
    Check(Root.Get('schemaVersion', 0) = 1, 'Manifest schema must be v1.');
    Check(Root.Get('architecture', '') = 'windows-x86_64',
      'Manifest architecture is not pinned.');
    Items := Root.Arrays['providers'];
    Check(Items.Count = 6, 'Known-core provider count is wrong.');
    for I := 0 to Items.Count - 1 do
    begin
      Item := Items.Objects[I];
      ProviderId := Item.Get('providerId', '');
      Provider := CreateProviderPreset(ProviderId);
      Check(Provider.ProviderId = ProviderId, 'Unknown manifest provider: ' + ProviderId);
      Url := Item.Get('url', '');
      Check(Pos('https://github.com/', Url) = 1,
        ProviderId + ': download URL is not official HTTPS GitHub.');
      Check(Pos('/latest/', LowerCase(Url)) = 0,
        ProviderId + ': mutable latest URL is forbidden.');
      Check(IsLowerHexSha256(Item.Get('downloadSha256', '')),
        ProviderId + ': SHA-256 is invalid.');
      ExecutablePath := Item.Get('executablePath', '');
      Check((ExecutablePath <> '') and
        (ExtractFileDrive(ExecutablePath) = '') and
        (Pos('..', ExecutablePath) = 0),
        ProviderId + ': executable path must stay archive-relative.');
      Check(ArraysEqual(Item.Arrays['probeArguments'],
        Provider.VersionArguments), ProviderId + ': probe command drifted.');
      Check(ArraysEqual(Item.Arrays['validateArguments'],
        Provider.ValidateArguments), ProviderId + ': validate command drifted.');
      Check(ArraysEqual(Item.Arrays['startArguments'], Provider.RunArguments),
        ProviderId + ': start command drifted.');
    end;
  finally
    Data.Free;
  end;
  WriteLn('Known cores pinned manifest: PASS');
end.
