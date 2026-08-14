unit ZaryaDiagnostics;

{$mode objfpc}{$H+}

interface

uses
  SysUtils, ZaryaProfile, ZaryaCoreProviderRegistry, ZaryaAppSettings;

function BuildDiagnosticsJson(const AProfiles: TZaryaProfiles;
  const ARegistry: TZaryaCoreProviderRegistry;
  const ASettings: TZaryaAppSettings): string;
function CreateDiagnosticsBundle(const AFileName: string;
  const AProfiles: TZaryaProfiles;
  const ARegistry: TZaryaCoreProviderRegistry;
  const ASettings: TZaryaAppSettings; out AError: string): Boolean;

implementation

uses
  Classes, fpjson, Zipper, ZaryaCoreProvider, ZaryaFileIntegrity;

function UniqueWorkDirectory: string;
var
  Value: TGuid;
  Suffix: string;
begin
  if CreateGuid(Value) = 0 then
  begin
    Suffix := LowerCase(GuidToString(Value));
    Suffix := StringReplace(Suffix, '{', '', []);
    Suffix := StringReplace(Suffix, '}', '', []);
  end
  else
    Suffix := FormatDateTime('yyyymmddhhnnsszzz', Now) + '-' +
      IntToHex(Random(MaxInt), 8);
  Result := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-diagnostics-' + Suffix;
end;

procedure RemoveTree(const ADirectory: string);
var
  Search: TSearchRec;
  ItemPath: string;
begin
  if not DirectoryExists(ADirectory) then
    Exit;
  if FindFirst(IncludeTrailingPathDelimiter(ADirectory) + '*', faAnyFile,
    Search) = 0 then
  try
    repeat
      if (Search.Name = '.') or (Search.Name = '..') then
        Continue;
      ItemPath := IncludeTrailingPathDelimiter(ADirectory) + Search.Name;
      if (Search.Attr and faDirectory) <> 0 then
        RemoveTree(ItemPath)
      else
        DeleteFile(ItemPath);
    until FindNext(Search) <> 0;
  finally
    FindClose(Search);
  end;
  RemoveDir(ADirectory);
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

function LooksLikePath(const AText: string): Boolean;
var
  I: Integer;
begin
  if Pos('\\', AText) > 0 then
    Exit(True);
  for I := 1 to Length(AText) - 2 do
    if (AText[I] in ['A'..'Z', 'a'..'z']) and (AText[I + 1] = ':') and
      (AText[I + 2] in ['\', '/']) then
      Exit(True);
  Result := False;
end;

function SafeVersionText(const AProvider: TZaryaCoreProvider): string;
var
  Value: string;
  I: Integer;
begin
  Value := Trim(AProvider.Version);
  if AProvider.ExecutablePath <> '' then
    Value := StringReplace(Value, AProvider.ExecutablePath,
      '<external-path>', [rfReplaceAll, rfIgnoreCase]);
  if AProvider.WorkingDirectory <> '' then
    Value := StringReplace(Value, AProvider.WorkingDirectory,
      '<external-path>', [rfReplaceAll, rfIgnoreCase]);
  if AProvider.AssetDirectory <> '' then
    Value := StringReplace(Value, AProvider.AssetDirectory,
      '<external-path>', [rfReplaceAll, rfIgnoreCase]);
  for I := 1 to Length(Value) do
    if Ord(Value[I]) < 32 then
      Value[I] := ' ';
  Value := Trim(Value);
  if LooksLikePath(Value) or (Pos('://', Value) > 0) or
    (Pos('token=', LowerCase(Value)) > 0) or
    (Pos('password', LowerCase(Value)) > 0) or
    (Pos('secret', LowerCase(Value)) > 0) or
    (Pos('key=', LowerCase(Value)) > 0) then
    Value := '<redacted-version>';
  if Length(Value) > 160 then
    Value := Copy(Value, 1, 157) + '...';
  Result := Value;
end;

function ShortHash(const AHash: string): string;
var
  Value: string;
  I: Integer;
begin
  Value := Trim(AHash);
  if Length(Value) <> 64 then
    Exit('');
  for I := 1 to Length(Value) do
    if not (Value[I] in ['0'..'9', 'A'..'F', 'a'..'f']) then
      Exit('');
  Result := LowerCase(Copy(Value, 1, 12));
end;

procedure AddProtocolCounts(const AProfiles: TZaryaProfiles;
  const ATarget: TJSONArray);
var
  Names: TStringList;
  Item: TJSONObject;
  Name: string;
  I: Integer;
  Index: Integer;
begin
  Names := TStringList.Create;
  try
    Names.CaseSensitive := False;
    Names.Sorted := True;
    Names.NameValueSeparator := '=';
    for I := 0 to High(AProfiles) do
    begin
      Name := Trim(AProfiles[I].ProtocolName);
      if Name = '' then
        Name := 'unknown';
      Index := Names.IndexOfName(Name);
      if Index < 0 then
        Names.Values[Name] := '1'
      else
        Names.ValueFromIndex[Index] := IntToStr(
          StrToIntDef(Names.ValueFromIndex[Index], 0) + 1);
    end;
    for I := 0 to Names.Count - 1 do
    begin
      Item := TJSONObject.Create;
      Item.Add('protocol', Names.Names[I]);
      Item.Add('count', StrToIntDef(Names.ValueFromIndex[I], 0));
      ATarget.Add(Item);
    end;
  finally
    Names.Free;
  end;
end;

function BuildDiagnosticsJson(const AProfiles: TZaryaProfiles;
  const ARegistry: TZaryaCoreProviderRegistry;
  const ASettings: TZaryaAppSettings): string;
var
  Root: TJSONObject;
  ProfilesObject: TJSONObject;
  Protocols: TJSONArray;
  Providers: TJSONArray;
  ProviderObject: TJSONObject;
  Provider: TZaryaCoreProvider;
  I: Integer;
  RawCount: Integer;
  SubscriptionCount: Integer;
begin
  Root := TJSONObject.Create;
  try
    Root.Add('diagnosticsVersion', 1);
    Root.Add('createdAt', FormatDateTime('yyyy-mm-dd"T"hh:nn:ss', Now));
    Root.Add('application', 'Zarya FPC/LCL');
    {$IFDEF CPUX86_64}
    Root.Add('architecture', 'x86_64');
    {$ELSE}
    Root.Add('architecture', 'unknown');
    {$ENDIF}
    {$IFDEF WINDOWS}
    Root.Add('platform', 'windows');
    {$ELSE}
    Root.Add('platform', 'unknown');
    {$ENDIF}

    RawCount := 0;
    SubscriptionCount := 0;
    for I := 0 to High(AProfiles) do
    begin
      if Trim(AProfiles[I].RawConfig) <> '' then
        Inc(RawCount);
      if Trim(AProfiles[I].SubscriptionId) <> '' then
        Inc(SubscriptionCount);
    end;
    ProfilesObject := TJSONObject.Create;
    Root.Add('profiles', ProfilesObject);
    ProfilesObject.Add('count', Length(AProfiles));
    ProfilesObject.Add('rawCount', RawCount);
    ProfilesObject.Add('subscriptionProfileCount', SubscriptionCount);
    Protocols := TJSONArray.Create;
    ProfilesObject.Add('byProtocol', Protocols);
    AddProtocolCounts(AProfiles, Protocols);

    Root.Add('mixedPort', ASettings.MixedPort);
    Root.Add('autoSystemProxy', ASettings.AutoEnableSystemProxy);
    Providers := TJSONArray.Create;
    Root.Add('providers', Providers);
    for I := 0 to ARegistry.Count - 1 do
    begin
      Provider := ARegistry.ProviderAt(I);
      ProviderObject := TJSONObject.Create;
      ProviderObject.Add('providerId', Provider.ProviderId);
      ProviderObject.Add('adapterId', Provider.AdapterId);
      ProviderObject.Add('distribution', DistributionToString(
        Provider.Distribution));
      ProviderObject.Add('state', StateToString(Provider.State));
      ProviderObject.Add('version', SafeVersionText(Provider));
      ProviderObject.Add('architecture', Provider.Architecture);
      ProviderObject.Add('sha256Prefix', ShortHash(Provider.Sha256));
      ProviderObject.Add('configFormat', ConfigFormatToString(
        Provider.ConfigFormat));
      Providers.Add(ProviderObject);
    end;
    Result := Root.FormatJSON;
  finally
    Root.Free;
  end;
end;

function CreateDiagnosticsBundle(const AFileName: string;
  const AProfiles: TZaryaProfiles;
  const ARegistry: TZaryaCoreProviderRegistry;
  const ASettings: TZaryaAppSettings; out AError: string): Boolean;
var
  WorkDirectory: string;
  DiagnosticsFile: string;
  ManifestFile: string;
  TempFile: string;
  PreviousFile: string;
  Digest: string;
  Manifest: TJSONObject;
  Zipper: TZipper;
begin
  Result := False;
  AError := '';
  if Trim(AFileName) = '' then
  begin
    AError := 'Не указан файл diagnostics bundle.';
    Exit;
  end;
  if not ForceDirectories(ExtractFileDir(ExpandFileName(AFileName))) then
  begin
    AError := 'Не удалось создать каталог diagnostics.';
    Exit;
  end;
  WorkDirectory := UniqueWorkDirectory;
  TempFile := ExpandFileName(AFileName) + '.tmp';
  PreviousFile := ExpandFileName(AFileName) + '.replace-old';
  try
    if not ForceDirectories(WorkDirectory) then
      raise Exception.Create('Не удалось создать staging diagnostics.');
    DiagnosticsFile := IncludeTrailingPathDelimiter(WorkDirectory) +
      'diagnostics.json';
    WriteUtf8File(DiagnosticsFile,
      BuildDiagnosticsJson(AProfiles, ARegistry, ASettings));
    if not Sha256File(DiagnosticsFile, Digest, AError) then
      raise Exception.Create(AError);
    Manifest := TJSONObject.Create;
    try
      Manifest.Add('bundleVersion', 1);
      Manifest.Add('createdAt', FormatDateTime('yyyy-mm-dd"T"hh:nn:ss', Now));
      Manifest.Add('diagnosticsSha256', Digest);
      Manifest.Add('containsRuntimeLogs', False);
      Manifest.Add('containsRawConfigs', False);
      ManifestFile := IncludeTrailingPathDelimiter(WorkDirectory) +
        'manifest.json';
      WriteUtf8File(ManifestFile, Manifest.FormatJSON);
    finally
      Manifest.Free;
    end;
    if FileExists(TempFile) then
      DeleteFile(TempFile);
    Zipper := TZipper.Create;
    try
      Zipper.FileName := TempFile;
      Zipper.Entries.AddFileEntry(ManifestFile, 'manifest.json');
      Zipper.Entries.AddFileEntry(DiagnosticsFile, 'diagnostics.json');
      Zipper.ZipAllFiles;
    finally
      Zipper.Free;
    end;
    if FileExists(PreviousFile) then
      raise Exception.Create('Обнаружен незавершённый файл замены diagnostics.');
    if FileExists(ExpandFileName(AFileName)) and
      (not RenameFile(ExpandFileName(AFileName), PreviousFile)) then
      raise Exception.Create('Не удалось подготовить замену diagnostics.');
    if not RenameFile(TempFile, ExpandFileName(AFileName)) then
    begin
      if FileExists(PreviousFile) then
        RenameFile(PreviousFile, ExpandFileName(AFileName));
      raise Exception.Create('Не удалось установить diagnostics bundle.');
    end;
    if FileExists(PreviousFile) then
      DeleteFile(PreviousFile);
    Result := True;
  except
    on E: Exception do
      AError := E.Message;
  end;
  if FileExists(TempFile) then
    DeleteFile(TempFile);
  RemoveTree(WorkDirectory);
end;

end.
