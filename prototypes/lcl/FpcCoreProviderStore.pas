unit FpcCoreProviderStore;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, ZaryaCoreProvider, ZaryaCoreProviderStore;

type
  TFpcCoreProviderStore = class(TInterfacedObject, IZaryaCoreProviderStore)
  private
    FFileName: string;
    function GetFileName: string;
    function EncodePath(const APath: string): string;
    function DecodePath(const APath: string): string;
  public
    constructor Create(const AFileName: string);
    function Load(out AProviders: TZaryaCoreProviders;
      out AError: string): Boolean;
    function Save(const AProviders: TZaryaCoreProviders;
      out AError: string): Boolean;
  end;

implementation

uses
  fpjson, jsonparser;

function JsonArrayToStrings(AArray: TJSONArray): TZaryaStringArray;
var
  I: Integer;
begin
  if not Assigned(AArray) then
    Exit(nil);
  SetLength(Result, AArray.Count);
  for I := 0 to AArray.Count - 1 do
    Result[I] := AArray.Strings[I];
end;

function StringsToJsonArray(const AValues: TZaryaStringArray): TJSONArray;
var
  I: Integer;
begin
  Result := TJSONArray.Create;
  for I := 0 to High(AValues) do
    Result.Add(AValues[I]);
end;

constructor TFpcCoreProviderStore.Create(const AFileName: string);
begin
  inherited Create;
  FFileName := ExpandFileName(AFileName);
end;

function TFpcCoreProviderStore.GetFileName: string;
begin
  Result := FFileName;
end;

function PathHasPrefix(const APath, APrefix: string): Boolean;
var
  NormalPath: string;
  NormalPrefix: string;
begin
  NormalPath := LowerCase(ExpandFileName(APath));
  NormalPrefix := LowerCase(IncludeTrailingPathDelimiter(ExpandFileName(APrefix)));
  Result := Copy(NormalPath, 1, Length(NormalPrefix)) = NormalPrefix;
end;

function TFpcCoreProviderStore.EncodePath(const APath: string): string;
var
  AppDirectory: string;
begin
  if Trim(APath) = '' then
    Exit('');
  AppDirectory := ExtractFileDir(ParamStr(0));
  if PathHasPrefix(APath, AppDirectory) then
    Result := '$APP$\' + ExtractRelativePath(
      IncludeTrailingPathDelimiter(AppDirectory), ExpandFileName(APath))
  else
    Result := ExpandFileName(APath);
end;

function TFpcCoreProviderStore.DecodePath(const APath: string): string;
begin
  if Pos('$APP$\', APath) = 1 then
    Result := ExpandFileName(IncludeTrailingPathDelimiter(
      ExtractFileDir(ParamStr(0))) + Copy(APath, 7, MaxInt))
  else
    Result := APath;
end;

function TFpcCoreProviderStore.Load(out AProviders: TZaryaCoreProviders;
  out AError: string): Boolean;
var
  Stream: TFileStream;
  RootData: TJSONData;
  Root: TJSONObject;
  Items: TJSONArray;
  Item: TJSONObject;
  Provider: TZaryaCoreProvider;
  I: Integer;
begin
  SetLength(AProviders, 0);
  AError := '';
  if not FileExists(FFileName) then
    Exit(True);
  RootData := nil;
  try
    Stream := TFileStream.Create(FFileName, fmOpenRead or fmShareDenyWrite);
    try
      RootData := GetJSON(Stream);
    finally
      Stream.Free;
    end;
    if RootData.JSONType <> jtObject then
      raise Exception.Create('Корневой элемент core-providers.json должен быть объектом.');
    Root := TJSONObject(RootData);
    Items := Root.Arrays['providers'];
    SetLength(AProviders, Items.Count);
    for I := 0 to Items.Count - 1 do
    begin
      Item := Items.Objects[I];
      Provider := CreateProviderPreset(Item.Get('providerId', ''));
      Provider.ProviderId := Item.Get('providerId', Provider.ProviderId);
      Provider.DisplayName := Item.Get('displayName', Provider.DisplayName);
      Provider.AdapterId := Item.Get('adapterId', Provider.AdapterId);
      Provider.ExecutablePath := DecodePath(Item.Get('executablePath', ''));
      Provider.WorkingDirectory := DecodePath(Item.Get('workingDirectory', ''));
      Provider.AssetDirectory := DecodePath(Item.Get('assetDirectory', ''));
      Provider.VersionArguments := JsonArrayToStrings(
        Item.FindPath('versionArguments') as TJSONArray);
      Provider.ValidateArguments := JsonArrayToStrings(
        Item.FindPath('validateArguments') as TJSONArray);
      Provider.RunArguments := JsonArrayToStrings(
        Item.FindPath('runArguments') as TJSONArray);
      Provider.ConfigFormat := ConfigFormatFromString(
        Item.Get('configFormat', ConfigFormatToString(Provider.ConfigFormat)));
      Provider.ConfigExtension := Item.Get('configExtension',
        Provider.ConfigExtension);
      Provider.SupportedProtocols := Item.Get('supportedProtocols',
        Provider.SupportedProtocols);
      Provider.SupportsRouting := Item.Get('supportsRouting',
        Provider.SupportsRouting);
      Provider.SupportsDns := Item.Get('supportsDns', Provider.SupportsDns);
      Provider.SupportsSystemProxy := Item.Get('supportsSystemProxy',
        Provider.SupportsSystemProxy);
      Provider.SupportsTun := Item.Get('supportsTun', Provider.SupportsTun);
      Provider.ReadinessKind := ReadinessKindFromString(
        Item.Get('readinessKind', ReadinessKindToString(Provider.ReadinessKind)));
      Provider.Version := Item.Get('version', '');
      Provider.Architecture := Item.Get('architecture', '');
      Provider.Sha256 := Item.Get('sha256', '');
      Provider.ConfirmedSha256 := Item.Get('confirmedSha256', '');
      Provider.State := psMissing;
      AProviders[I] := Provider;
    end;
    Result := True;
  except
    on E: Exception do
    begin
      SetLength(AProviders, 0);
      AError := E.Message;
      Result := False;
    end;
  end;
  RootData.Free;
end;

function TFpcCoreProviderStore.Save(const AProviders: TZaryaCoreProviders;
  out AError: string): Boolean;
var
  Root: TJSONObject;
  Items: TJSONArray;
  Item: TJSONObject;
  Stream: TFileStream;
  Json: UTF8String;
  TempFile: string;
  BackupFile: string;
  DirectoryName: string;
  I: Integer;
begin
  AError := '';
  Root := nil;
  TempFile := FFileName + '.tmp';
  BackupFile := FFileName + '.bak';
  try
    DirectoryName := ExtractFileDir(FFileName);
    if (DirectoryName <> '') and (not ForceDirectories(DirectoryName)) then
      raise Exception.Create('Не удалось создать каталог providers.');
    Root := TJSONObject.Create;
    Root.Add('schemaVersion', 1);
    Items := TJSONArray.Create;
    Root.Add('providers', Items);
    for I := 0 to High(AProviders) do
    begin
      if AProviders[I].Distribution <> pdExternal then
        Continue;
      Item := TJSONObject.Create;
      Item.Add('providerId', AProviders[I].ProviderId);
      Item.Add('displayName', AProviders[I].DisplayName);
      Item.Add('adapterId', AProviders[I].AdapterId);
      Item.Add('executablePath', EncodePath(AProviders[I].ExecutablePath));
      Item.Add('workingDirectory', EncodePath(AProviders[I].WorkingDirectory));
      Item.Add('assetDirectory', EncodePath(AProviders[I].AssetDirectory));
      Item.Add('versionArguments', StringsToJsonArray(
        AProviders[I].VersionArguments));
      Item.Add('validateArguments', StringsToJsonArray(
        AProviders[I].ValidateArguments));
      Item.Add('runArguments', StringsToJsonArray(AProviders[I].RunArguments));
      Item.Add('configFormat', ConfigFormatToString(AProviders[I].ConfigFormat));
      Item.Add('configExtension', AProviders[I].ConfigExtension);
      Item.Add('supportedProtocols', AProviders[I].SupportedProtocols);
      Item.Add('supportsRouting', AProviders[I].SupportsRouting);
      Item.Add('supportsDns', AProviders[I].SupportsDns);
      Item.Add('supportsSystemProxy', AProviders[I].SupportsSystemProxy);
      Item.Add('supportsTun', AProviders[I].SupportsTun);
      Item.Add('readinessKind', ReadinessKindToString(
        AProviders[I].ReadinessKind));
      Item.Add('version', AProviders[I].Version);
      Item.Add('architecture', AProviders[I].Architecture);
      Item.Add('sha256', AProviders[I].Sha256);
      Item.Add('confirmedSha256', AProviders[I].ConfirmedSha256);
      Items.Add(Item);
    end;
    Json := UTF8String(Root.FormatJSON);
    Stream := TFileStream.Create(TempFile, fmCreate);
    try
      if Length(Json) > 0 then
        Stream.WriteBuffer(Json[1], Length(Json));
    finally
      Stream.Free;
    end;
    if FileExists(BackupFile) then
      DeleteFile(BackupFile);
    if FileExists(FFileName) and (not RenameFile(FFileName, BackupFile)) then
      raise Exception.Create('Не удалось подготовить атомарную замену providers.');
    if not RenameFile(TempFile, FFileName) then
    begin
      if FileExists(BackupFile) then
        RenameFile(BackupFile, FFileName);
      raise Exception.Create('Не удалось установить новый core-providers.json.');
    end;
    if FileExists(BackupFile) then
      DeleteFile(BackupFile);
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  if FileExists(TempFile) then
    DeleteFile(TempFile);
  Root.Free;
end;

end.
