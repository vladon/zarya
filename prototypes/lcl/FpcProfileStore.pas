unit FpcProfileStore;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, ZaryaProfile, ZaryaProfileStore;

type
  TFpcProfileStore = class(TInterfacedObject, IZaryaProfileStore)
  private
    FFileName: string;
    function GetFileName: string;
  public
    constructor Create(const AFileName: string);
    function Load(out AProfiles: TZaryaProfiles; out AError: string): Boolean;
    function Save(const AProfiles: TZaryaProfiles; out AError: string): Boolean;
    property FileName: string read FFileName;
  end;

function DefaultProfileStorePath: string;
function DefaultLclDataDirectory: string;
function ProfileStorePathFromCommandLine: string;

implementation

uses
  fpjson, jsonparser;

function DefaultLclDataDirectory: string;
var
  LocalAppData: string;
begin
  LocalAppData := Trim(GetEnvironmentVariable('LOCALAPPDATA'));
  if LocalAppData <> '' then
    Result := IncludeTrailingPathDelimiter(LocalAppData) + 'Zarya' +
      PathDelim + 'LCL'
  else
    Result := IncludeTrailingPathDelimiter(GetAppConfigDir(False)) + 'LCL';
  Result := ExcludeTrailingPathDelimiter(ExpandFileName(Result));
end;

function DefaultProfileStorePath: string;
begin
  Result := IncludeTrailingPathDelimiter(DefaultLclDataDirectory) +
    'profiles.json';
end;

function ProfileStorePathFromCommandLine: string;
var
  I: Integer;
  DataDirectory: string;
  Portable: Boolean;
begin
  DataDirectory := '';
  Portable := FileExists(ChangeFileExt(ParamStr(0), '') + '.portable.flag') or
    FileExists(IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
      'portable.flag');
  I := 1;
  while I <= ParamCount do
  begin
    if SameText(ParamStr(I), '--portable') then
      Portable := True
    else if SameText(ParamStr(I), '--data-dir') and (I < ParamCount) then
    begin
      Inc(I);
      DataDirectory := ParamStr(I);
    end
    else if Pos('--data-dir=', LowerCase(ParamStr(I))) = 1 then
      DataDirectory := Copy(ParamStr(I), Length('--data-dir=') + 1, MaxInt);
    Inc(I);
  end;

  if DataDirectory <> '' then
    Result := IncludeTrailingPathDelimiter(ExpandFileName(DataDirectory)) +
      'profiles.json'
  else if Portable then
    Result := IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
      'data' + PathDelim + 'profiles.json'
  else
    Result := DefaultProfileStorePath;
end;

constructor TFpcProfileStore.Create(const AFileName: string);
begin
  inherited Create;
  FFileName := AFileName;
end;

function TFpcProfileStore.GetFileName: string;
begin
  Result := FFileName;
end;

function TFpcProfileStore.Load(out AProfiles: TZaryaProfiles;
  out AError: string): Boolean;
var
  Stream: TFileStream;
  RootData: TJSONData;
  Root: TJSONObject;
  Items: TJSONArray;
  Item: TJSONObject;
  I: Integer;
  LegacyCoreType: string;
begin
  SetLength(AProfiles, 0);
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
      raise Exception.Create('Корневой элемент profiles.json должен быть объектом.');
    Root := TJSONObject(RootData);
    Items := Root.Arrays['profiles'];
    SetLength(AProfiles, Items.Count);
    for I := 0 to Items.Count - 1 do
    begin
      if Items.Items[I].JSONType <> jtObject then
        raise Exception.CreateFmt('Профиль #%d должен быть объектом.', [I + 1]);
      Item := TJSONObject(Items.Objects[I]);
      AProfiles[I] := CreateEmptyProfile;
      AProfiles[I].Id := Item.Get('id', '');
      AProfiles[I].Name := Item.Get('name', '');
      AProfiles[I].ProtocolName := Item.Get('protocol', 'VLESS');
      AProfiles[I].Host := Item.Get('address', Item.Get('host', ''));
      AProfiles[I].Port := Item.Get('port', 443);
      AProfiles[I].Uuid := Item.Get('uuid', Item.Get('uuidPassword', ''));
      AProfiles[I].Password := Item.Get('password', '');
      AProfiles[I].Encryption := Item.Get('encryption', 'none');
      AProfiles[I].Method := Item.Get('method', '');
      AProfiles[I].SecurityCipher := Item.Get('securityCipher',
        Item.Get('scy', ''));
      AProfiles[I].Flow := Item.Get('flow', '');
      AProfiles[I].Remark := Item.Get('remark', '');
      AProfiles[I].Network := Item.Get('network', 'tcp');
      AProfiles[I].TransportHost := Item.Get('transportHost',
        Item.Get('host', ''));
      AProfiles[I].Path := Item.Get('path', '');
      AProfiles[I].HeaderType := Item.Get('headerType', '');
      AProfiles[I].ServiceName := Item.Get('serviceName', '');
      AProfiles[I].Security := Item.Get('security', '');
      AProfiles[I].ServerName := Item.Get('serverName', '');
      AProfiles[I].Sni := Item.Get('sni', '');
      AProfiles[I].Fingerprint := Item.Get('fingerprint', 'chrome');
      AProfiles[I].PublicKey := Item.Get('publicKey', '');
      AProfiles[I].ShortId := Item.Get('shortId', '');
      AProfiles[I].SpiderX := Item.Get('spiderX', '/');
      AProfiles[I].Alpn := Item.Get('alpn', '');
      AProfiles[I].Obfs := Item.Get('obfs', '');
      AProfiles[I].ObfsPassword := Item.Get('obfsPassword',
        Item.Get('obfs-password', ''));
      AProfiles[I].AllowInsecure := Item.Get('allowInsecure', False);
      AProfiles[I].LocalAddress := Item.Get('localAddress', '');
      AProfiles[I].AllowedIps := Item.Get('allowedIps', '');
      AProfiles[I].PreSharedKey := Item.Get('preSharedKey', '');
      AProfiles[I].Reserved := Item.Get('reserved', '');
      AProfiles[I].Mtu := Item.Get('mtu', 0);
      AProfiles[I].KeepAlive := Item.Get('keepAlive', 0);
      AProfiles[I].AlterId := Item.Get('alterId', 0);
      AProfiles[I].PreferredProviderId := Item.Get('preferredProviderId', '');
      if AProfiles[I].PreferredProviderId = '' then
      begin
        LegacyCoreType := Item.Get('coreType', 'Xray');
        if SameText(LegacyCoreType, 'SingBox') or
          SameText(LegacyCoreType, 'sing-box') then
          AProfiles[I].PreferredProviderId := 'embedded.singbox'
        else
          AProfiles[I].PreferredProviderId := 'embedded.xray';
      end;
      AProfiles[I].RawConfig := Item.Get('rawConfig', '');
      AProfiles[I].RawConfigFormat := Item.Get('rawConfigFormat', '');
      AProfiles[I].ReadinessHost := Item.Get('readinessHost', '127.0.0.1');
      AProfiles[I].ReadinessPort := Item.Get('readinessPort', 0);
      AProfiles[I].SystemProxyKind := Item.Get('systemProxyKind', 'mixed');
      AProfiles[I].SourceType := Item.Get('sourceType', 'manual');
      AProfiles[I].SubscriptionId := Item.Get('subscriptionId', '');
      AProfiles[I].SubscriptionName := Item.Get('subscriptionName', '');
      AProfiles[I].SourceKey := Item.Get('sourceKey', '');
      AProfiles[I].LastSeenAt := Item.Get('lastSeenAt', '');
      AProfiles[I].DeletedBySubscriptionUpdate :=
        Item.Get('deletedBySubscriptionUpdate', False);
      AProfiles[I].UnsupportedReason := Item.Get('unsupportedReason', '');
      AProfiles[I].LastTcpPingMs := Item.Get('lastTcpPingMs', -1);
      AProfiles[I].LastRealDelayMs := Item.Get('lastRealDelayMs', -1);
      AProfiles[I].LastTestStatus := Item.Get('lastTestStatus', 'never_tested');
      AProfiles[I].LastTestError := Item.Get('lastTestError', '');
      AProfiles[I].LastTestedAt := Item.Get('lastTestedAt', '');
      AProfiles[I].Source := Item.Get('source', 'Вручную');
      AProfiles[I].Latency := Item.Get('latency', '—');
      AProfiles[I].Enabled := Item.Get('enabled', True);
      if AProfiles[I].Id = '' then
        AProfiles[I].Id := CreateEmptyProfile.Id;
    end;
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      SetLength(AProfiles, 0);
      Result := False;
    end;
  end;
  RootData.Free;
end;

function TFpcProfileStore.Save(const AProfiles: TZaryaProfiles;
  out AError: string): Boolean;
var
  Root: TJSONObject;
  Items: TJSONArray;
  Item: TJSONObject;
  Stream: TFileStream;
  Json: UTF8String;
  I: Integer;
  DirectoryName: string;
  TempFile: string;
  BackupFile: string;
begin
  AError := '';
  Root := nil;
  TempFile := FFileName + '.tmp';
  BackupFile := FFileName + '.bak';
  try
    DirectoryName := ExtractFileDir(FFileName);
    if (DirectoryName <> '') and (not ForceDirectories(DirectoryName)) then
      raise Exception.Create('Не удалось создать каталог данных: ' + DirectoryName);

    Root := TJSONObject.Create;
    Root.Add('schemaVersion', 4);
    Items := TJSONArray.Create;
    Root.Add('profiles', Items);
    for I := 0 to High(AProfiles) do
    begin
      Item := TJSONObject.Create;
      Item.Add('id', AProfiles[I].Id);
      Item.Add('name', AProfiles[I].Name);
      Item.Add('protocol', AProfiles[I].ProtocolName);
      Item.Add('address', AProfiles[I].Host);
      Item.Add('port', AProfiles[I].Port);
      Item.Add('uuid', AProfiles[I].Uuid);
      Item.Add('password', AProfiles[I].Password);
      Item.Add('encryption', AProfiles[I].Encryption);
      Item.Add('method', AProfiles[I].Method);
      Item.Add('securityCipher', AProfiles[I].SecurityCipher);
      Item.Add('flow', AProfiles[I].Flow);
      Item.Add('remark', AProfiles[I].Remark);
      Item.Add('network', AProfiles[I].Network);
      Item.Add('transportHost', AProfiles[I].TransportHost);
      Item.Add('path', AProfiles[I].Path);
      Item.Add('headerType', AProfiles[I].HeaderType);
      Item.Add('serviceName', AProfiles[I].ServiceName);
      Item.Add('security', AProfiles[I].Security);
      Item.Add('serverName', AProfiles[I].ServerName);
      Item.Add('sni', AProfiles[I].Sni);
      Item.Add('fingerprint', AProfiles[I].Fingerprint);
      Item.Add('publicKey', AProfiles[I].PublicKey);
      Item.Add('shortId', AProfiles[I].ShortId);
      Item.Add('spiderX', AProfiles[I].SpiderX);
      Item.Add('alpn', AProfiles[I].Alpn);
      Item.Add('obfs', AProfiles[I].Obfs);
      Item.Add('obfsPassword', AProfiles[I].ObfsPassword);
      Item.Add('allowInsecure', AProfiles[I].AllowInsecure);
      Item.Add('localAddress', AProfiles[I].LocalAddress);
      Item.Add('allowedIps', AProfiles[I].AllowedIps);
      Item.Add('preSharedKey', AProfiles[I].PreSharedKey);
      Item.Add('reserved', AProfiles[I].Reserved);
      Item.Add('mtu', AProfiles[I].Mtu);
      Item.Add('keepAlive', AProfiles[I].KeepAlive);
      Item.Add('alterId', AProfiles[I].AlterId);
      Item.Add('preferredProviderId', AProfiles[I].PreferredProviderId);
      Item.Add('rawConfig', AProfiles[I].RawConfig);
      Item.Add('rawConfigFormat', AProfiles[I].RawConfigFormat);
      Item.Add('readinessHost', AProfiles[I].ReadinessHost);
      Item.Add('readinessPort', AProfiles[I].ReadinessPort);
      Item.Add('systemProxyKind', AProfiles[I].SystemProxyKind);
      Item.Add('sourceType', AProfiles[I].SourceType);
      Item.Add('subscriptionId', AProfiles[I].SubscriptionId);
      Item.Add('subscriptionName', AProfiles[I].SubscriptionName);
      Item.Add('sourceKey', AProfiles[I].SourceKey);
      Item.Add('lastSeenAt', AProfiles[I].LastSeenAt);
      Item.Add('deletedBySubscriptionUpdate',
        AProfiles[I].DeletedBySubscriptionUpdate);
      Item.Add('unsupportedReason', AProfiles[I].UnsupportedReason);
      Item.Add('lastTcpPingMs', AProfiles[I].LastTcpPingMs);
      Item.Add('lastRealDelayMs', AProfiles[I].LastRealDelayMs);
      Item.Add('lastTestStatus', AProfiles[I].LastTestStatus);
      Item.Add('lastTestError', AProfiles[I].LastTestError);
      Item.Add('lastTestedAt', AProfiles[I].LastTestedAt);
      Item.Add('source', AProfiles[I].Source);
      Item.Add('latency', AProfiles[I].Latency);
      Item.Add('enabled', AProfiles[I].Enabled);
      Items.Add(Item);
    end;

    Json := UTF8String(Root.FormatJSON);
    Stream := TFileStream.Create(TempFile, fmCreate or fmShareExclusive);
    try
      if Length(Json) > 0 then
        Stream.WriteBuffer(Json[1], Length(Json));
    finally
      Stream.Free;
    end;
    if FileExists(BackupFile) then
      DeleteFile(BackupFile);
    if FileExists(FFileName) and (not RenameFile(FFileName, BackupFile)) then
      raise Exception.Create('Не удалось подготовить атомарную запись profiles.json.');
    if not RenameFile(TempFile, FFileName) then
    begin
      if FileExists(BackupFile) then
        RenameFile(BackupFile, FFileName);
      raise Exception.Create('Не удалось установить новый profiles.json.');
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
