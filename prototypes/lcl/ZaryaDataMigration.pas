unit ZaryaDataMigration;

{$mode objfpc}{$H+}

interface

uses
  SysUtils;

type
  TZaryaMigrationResult = (mrNotNeeded, mrMigrated, mrFailed);

function LegacyQtDataDirectory: string;
function MigrateQtData(const ALegacyDirectory, ATargetDirectory: string;
  out ABackupFile, AError: string): TZaryaMigrationResult;
function EnsureFirstRunQtMigration(const ATargetDirectory: string;
  out ABackupFile, AError: string): TZaryaMigrationResult;

implementation

uses
  Classes, Math, fpjson, jsonparser, Zipper, ZaryaProfile, ZaryaProfileStore,
  FpcProfileStore, ZaryaFileIntegrity, ZaryaRouting, ZaryaDns,
  ZaryaPolicyStore, FpcPolicyStore, ZaryaAppSettings
  {$IFDEF MSWINDOWS}, Registry{$ENDIF};

const
  MigratedFiles: array[0..3] of string = (
    'profiles.json', 'subscriptions.json', 'routing.json', 'dns.json');

function LegacyQtDataDirectory: string;
var
  LocalAppData: string;
begin
  LocalAppData := Trim(GetEnvironmentVariable('LOCALAPPDATA'));
  if LocalAppData = '' then
    Exit('');
  Result := IncludeTrailingPathDelimiter(LocalAppData) + 'Zarya' +
    PathDelim + 'Zarya';
end;

procedure CopyFileExact(const ASource, ADestination: string);
var
  SourceStream: TFileStream;
  DestinationStream: TFileStream;
begin
  SourceStream := TFileStream.Create(ASource, fmOpenRead or fmShareDenyWrite);
  try
    DestinationStream := TFileStream.Create(ADestination,
      fmCreate or fmShareExclusive);
    try
      DestinationStream.CopyFrom(SourceStream, 0);
    finally
      DestinationStream.Free;
    end;
  finally
    SourceStream.Free;
  end;
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

function ValidateJsonFile(const AFileName: string; out AError: string): Boolean;
var
  Stream: TFileStream;
  Data: TJSONData;
begin
  Result := False;
  Data := nil;
  try
    Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyWrite);
    try
      Data := GetJSON(Stream);
    finally
      Stream.Free;
    end;
    if Data.JSONType <> jtObject then
      raise Exception.Create(ExtractFileName(AFileName) +
        ': корневой JSON должен быть объектом.');
    Result := True;
  except
    on E: Exception do
      AError := E.Message;
  end;
  Data.Free;
end;

function ReadSubscriptionIds(const AFileName: string;
  AIds: TStringList; out AError: string): Boolean;
var
  Stream: TFileStream;
  Data: TJSONData;
  Root: TJSONObject;
  Items: TJSONArray;
  I: Integer;
  Id: string;
begin
  Result := False;
  Data := nil;
  if not FileExists(AFileName) then
    Exit(True);
  try
    Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyWrite);
    try
      Data := GetJSON(Stream);
    finally
      Stream.Free;
    end;
    if Data.JSONType <> jtObject then
      raise Exception.Create('subscriptions.json: invalid root.');
    Root := TJSONObject(Data);
    Items := Root.Arrays['subscriptions'];
    for I := 0 to Items.Count - 1 do
    begin
      Id := Items.Objects[I].Get('id', '');
      if Id = '' then
        raise Exception.CreateFmt('Subscription #%d has no id.', [I + 1]);
      if AIds.IndexOf(Id) >= 0 then
        raise Exception.Create('Duplicate subscription id: ' + Id);
      AIds.Add(Id);
    end;
    Result := True;
  except
    on E: Exception do
      AError := E.Message;
  end;
  Data.Free;
end;

{$IFDEF MSWINDOWS}
function TryReadQtRegistryValue(const AName: string; out AValue: string): Boolean;
var
  Registry: TRegistry;
  Marker: Integer;
  GroupName: string;
  ValueName: string;
  DataType: TRegDataType;
begin
  Result := False;
  AValue := '';
  Marker := LastDelimiter('/', AName);
  if Marker < 1 then Exit;
  GroupName := StringReplace(Copy(AName, 1, Marker - 1), '/', '\', [rfReplaceAll]);
  ValueName := Copy(AName, Marker + 1, MaxInt);
  Registry := TRegistry.Create;
  try
    Registry.RootKey := QWord($80000001);
    if not Registry.OpenKeyReadOnly('\Software\Zarya\Zarya\' + GroupName) or
      not Registry.ValueExists(ValueName) then Exit;
    DataType := Registry.GetDataType(ValueName);
    case DataType of
      rdInteger: AValue := IntToStr(Registry.ReadInteger(ValueName));
      rdString, rdExpandString: AValue := Registry.ReadString(ValueName);
    else
      Exit;
    end;
    Result := True;
  finally
    Registry.Free;
  end;
end;
{$ELSE}
function TryReadQtRegistryValue(const AName: string; out AValue: string): Boolean;
begin
  AValue := '';
  Result := False;
end;
{$ENDIF}

function TextToBool(const AValue: string; const ADefault: Boolean): Boolean;
begin
  if SameText(Trim(AValue), 'true') or (Trim(AValue) = '1') then Exit(True);
  if SameText(Trim(AValue), 'false') or (Trim(AValue) = '0') then Exit(False);
  Result := ADefault;
end;

function RoutingIdExists(const AProfiles: TZaryaRoutingProfiles;
  const AId: string): Boolean;
var
  Profile: TZaryaRoutingProfile;
begin
  for Profile in AProfiles do if SameText(Profile.Id, AId) then Exit(True);
  Result := False;
end;

function DnsIdExists(const AProfiles: TZaryaDnsProfiles;
  const AId: string): Boolean;
var
  Profile: TZaryaDnsProfile;
begin
  for Profile in AProfiles do if SameText(Profile.Id, AId) then Exit(True);
  Result := False;
end;

procedure AddJsonString(const AArray: TJSONArray; const AValue: string);
begin
  AArray.Add(AValue);
end;

function MigrateQtSettings(const AStagingDirectory: string;
  const ARoutingProfiles: TZaryaRoutingProfiles;
  const ADnsProfiles: TZaryaDnsProfiles; out AMigrationJson,
  AError: string): Boolean;
var
  Settings: TZaryaAppSettings;
  Store: ISettingsStore;
  Value: string;
  Number: Integer;
  Root: TJSONObject;
  Migrated, Skipped, Fallbacks: TJSONArray;

  procedure ReadStringValue(const AQtName: string; var ADestination: string);
  begin
    if TryReadQtRegistryValue(AQtName, Value) then
    begin
      ADestination := Value;
      AddJsonString(Migrated, AQtName);
    end;
  end;

  procedure ReadBoolValue(const AQtName: string; var ADestination: Boolean);
  begin
    if TryReadQtRegistryValue(AQtName, Value) then
    begin
      ADestination := TextToBool(Value, ADestination);
      AddJsonString(Migrated, AQtName);
    end;
  end;

begin
  Result := False;
  AError := '';
  AMigrationJson := '';
  Settings := DefaultAppSettings;
  Root := TJSONObject.Create;
  try
    Root.Add('schemaVersion', 2);
    Root.Add('source', 'qt');
    Migrated := TJSONArray.Create;
    Skipped := TJSONArray.Create;
    Fallbacks := TJSONArray.Create;
    Root.Add('migratedSettings', Migrated);
    Root.Add('skippedSettings', Skipped);
    Root.Add('fallbacks', Fallbacks);

    if TryReadQtRegistryValue('desktop/themeMode', Value) then
    begin
      Settings.DarkTheme := SameText(Value, 'dark');
      AddJsonString(Migrated, 'desktop/themeMode');
    end;
    ReadStringValue('desktop/languageCode', Settings.Language);
    ReadBoolValue('desktop/minimizeToTrayOnClose', Settings.MinimizeToTray);
    if TryReadQtRegistryValue('proxy/mixedPort', Value) and
      TryStrToInt(Value, Number) and (Number >= 1) and (Number <= 65535) then
    begin
      Settings.MixedPort := Number;
      AddJsonString(Migrated, 'proxy/mixedPort');
    end;
    ReadBoolValue('proxy/autoEnableSystemProxyOnStart',
      Settings.AutoEnableSystemProxy);
    ReadBoolValue('proxy/restoreProxyOnExit', Settings.RestoreSystemProxy);
    ReadStringValue('routing/selectedProfileId',
      Settings.SelectedRoutingProfileId);
    ReadStringValue('dns/selectedProfileId', Settings.SelectedDnsProfileId);
    ReadBoolValue('startup/startAtLogin', Settings.StartAtLogin);
    ReadBoolValue('startup/startMinimizedToTray',
      Settings.StartMinimizedToTray);
    ReadBoolValue('startup/autoStartLastProfile',
      Settings.AutoStartLastProfile);
    ReadBoolValue('startup/autoEnableSystemProxyAfterAutoStart',
      Settings.AutoEnableSystemProxyAfterAutoStart);
    if TryReadQtRegistryValue('startup/autoStartDelaySeconds', Value) and
      TryStrToInt(Value, Number) then
    begin
      Settings.AutoStartDelaySeconds := EnsureRange(Number, 0, 120);
      AddJsonString(Migrated, 'startup/autoStartDelaySeconds');
    end;
    ReadStringValue('startup/lastStartedProfileId',
      Settings.LastStartedProfileId);
    if TryReadQtRegistryValue('testing/maxConcurrentTests', Value) and
      TryStrToInt(Value, Number) then
    begin
      Settings.RealDelayConcurrency := EnsureRange(Number, 1, 10);
      AddJsonString(Migrated, 'testing/maxConcurrentTests');
    end;
    if TryReadQtRegistryValue('testing/realDelayTimeoutMs', Value) and
      TryStrToInt(Value, Number) then
    begin
      Settings.RealDelayTimeoutSeconds := EnsureRange((Number + 999) div 1000,
        1, 60);
      AddJsonString(Migrated, 'testing/realDelayTimeoutMs');
    end;
    ReadStringValue('testing/testUrl', Settings.RealDelayTestUrl);
    ReadStringValue('geodata/selectedSourceId', Settings.GeoSourceId);
    ReadBoolValue('geodata/autoCheckOnStartup',
      Settings.GeoAutoCheckOnStartup);
    ReadBoolValue('geodata/warnIfMissing', Settings.GeoWarnIfMissing);

    if not RoutingIdExists(ARoutingProfiles,
      Settings.SelectedRoutingProfileId) then
    begin
      Settings.SelectedRoutingProfileId := RoutingBypassLanId;
      AddJsonString(Fallbacks, 'routing/selectedProfileId -> builtin-bypass-lan');
    end;
    if not DnsIdExists(ADnsProfiles, Settings.SelectedDnsProfileId) then
    begin
      Settings.SelectedDnsProfileId := DnsSystemId;
      AddJsonString(Fallbacks, 'dns/selectedProfileId -> builtin-dns-system');
    end;
    AddJsonString(Skipped, 'machine-specific core/provider paths');
    AddJsonString(Skipped, 'helper and privileged settings');
    AddJsonString(Skipped, 'tokens and credentials');
    AddJsonString(Skipped, 'experimental feature flags');
    Settings.FirstRunCompleted := True;
    Store := TZaryaAppSettingsStore.Create(IncludeTrailingPathDelimiter(
      AStagingDirectory) + 'settings.ini');
    if not Store.Save(Settings, AError) then Exit(False);
    Store := nil;
    AMigrationJson := Root.FormatJSON;
    Result := True;
  finally
    Root.Free;
  end;
end;

function ValidateRelations(const AProfiles: TZaryaProfiles;
  const ASubscriptionsFile: string; out AError: string): Boolean;
var
  ProfileIds: TStringList;
  SubscriptionIds: TStringList;
  I: Integer;
begin
  Result := False;
  ProfileIds := TStringList.Create;
  SubscriptionIds := TStringList.Create;
  try
    ProfileIds.CaseSensitive := True;
    ProfileIds.Sorted := True;
    SubscriptionIds.CaseSensitive := True;
    SubscriptionIds.Sorted := True;
    if not ReadSubscriptionIds(ASubscriptionsFile, SubscriptionIds,
      AError) then
      Exit;
    for I := 0 to High(AProfiles) do
    begin
      if Trim(AProfiles[I].Id) = '' then
      begin
        AError := Format('Профиль #%d не имеет id.', [I + 1]);
        Exit;
      end;
      if ProfileIds.IndexOf(AProfiles[I].Id) >= 0 then
      begin
        AError := 'Повторяющийся profile id: ' + AProfiles[I].Id;
        Exit;
      end;
      ProfileIds.Add(AProfiles[I].Id);
      if (AProfiles[I].SubscriptionId <> '') and
        (SubscriptionIds.IndexOf(AProfiles[I].SubscriptionId) < 0) then
      begin
        AError := 'Профиль ссылается на отсутствующую subscription: ' +
          AProfiles[I].SubscriptionId;
        Exit;
      end;
    end;
    Result := True;
  finally
    SubscriptionIds.Free;
    ProfileIds.Free;
  end;
end;

function CreateLegacyBackup(const ALegacyDirectory, ABackupDirectory,
  AWorkDirectory: string; out ABackupFile, AError: string): Boolean;
var
  Manifest: TJSONObject;
  Files: TJSONArray;
  Entry: TJSONObject;
  ManifestFile: string;
  SourceFile: string;
  SnapshotDirectory: string;
  SnapshotFile: string;
  Digest: string;
  I: Integer;
  Suffix: Integer;
  BackupStem: string;
  Zipper: TZipper;
begin
  Result := False;
  if not ForceDirectories(ABackupDirectory) then
  begin
    AError := 'Не удалось создать каталог backup.';
    Exit;
  end;
  SnapshotDirectory := IncludeTrailingPathDelimiter(AWorkDirectory) + 'data';
  if not ForceDirectories(SnapshotDirectory) then
  begin
    AError := 'Не удалось создать snapshot-каталог backup.';
    Exit;
  end;
  Manifest := TJSONObject.Create;
  try
    Manifest.Add('backupVersion', 1);
    Manifest.Add('createdAt', FormatDateTime('yyyy-mm-dd"T"hh:nn:ss', Now));
    Manifest.Add('source', 'qt');
    Files := TJSONArray.Create;
    Manifest.Add('files', Files);
    for I := Low(MigratedFiles) to High(MigratedFiles) do
    begin
      SourceFile := IncludeTrailingPathDelimiter(ALegacyDirectory) +
        MigratedFiles[I];
      if not FileExists(SourceFile) then
        Continue;
      SnapshotFile := IncludeTrailingPathDelimiter(SnapshotDirectory) +
        MigratedFiles[I];
      CopyFileExact(SourceFile, SnapshotFile);
      if not Sha256File(SnapshotFile, Digest, AError) then
        Exit;
      Entry := TJSONObject.Create;
      Entry.Add('path', 'data/' + MigratedFiles[I]);
      Entry.Add('sha256', Digest);
      Files.Add(Entry);
    end;
    ManifestFile := IncludeTrailingPathDelimiter(AWorkDirectory) +
      'manifest.json';
    WriteUtf8File(ManifestFile, Manifest.FormatJSON);
  finally
    Manifest.Free;
  end;

  BackupStem := IncludeTrailingPathDelimiter(ABackupDirectory) +
    'qt-pre-lcl-' + FormatDateTime('yyyymmdd-hhnnss-zzz', Now);
  ABackupFile := BackupStem + '.zarya-backup.zip';
  Suffix := 1;
  while FileExists(ABackupFile) do
  begin
    ABackupFile := BackupStem + '-' + IntToStr(Suffix) +
      '.zarya-backup.zip';
    Inc(Suffix);
  end;
  Zipper := TZipper.Create;
  try
    Zipper.FileName := ABackupFile;
    Zipper.Entries.AddFileEntry(ManifestFile, 'manifest.json');
    for I := Low(MigratedFiles) to High(MigratedFiles) do
    begin
      SnapshotFile := IncludeTrailingPathDelimiter(SnapshotDirectory) +
        MigratedFiles[I];
      if FileExists(SnapshotFile) then
        Zipper.Entries.AddFileEntry(SnapshotFile,
          'data/' + MigratedFiles[I]);
    end;
    Zipper.ZipAllFiles;
    Result := FileExists(ABackupFile);
    if not Result then
      AError := 'Backup ZIP не был создан.';
  except
    on E: Exception do
    begin
      AError := E.Message;
      if FileExists(ABackupFile) then
        DeleteFile(ABackupFile);
    end;
  end;
  Zipper.Free;
end;

function MigrateQtData(const ALegacyDirectory, ATargetDirectory: string;
  out ABackupFile, AError: string): TZaryaMigrationResult;
var
  LegacyDirectory: string;
  TargetDirectory: string;
  ParentDirectory: string;
  WorkDirectory: string;
  StagingDirectory: string;
  BackupDirectory: string;
  SourceFile: string;
  StagingFile: string;
  Store: IZaryaProfileStore;
  VerifyStore: IZaryaProfileStore;
  RoutingStore: IRoutingProfileStore;
  DnsStore: IDnsProfileStore;
  Profiles: TZaryaProfiles;
  VerifiedProfiles: TZaryaProfiles;
  RoutingProfiles, VerifiedRouting: TZaryaRoutingProfiles;
  DnsProfiles, VerifiedDns: TZaryaDnsProfiles;
  MigrationJson: string;
  I: Integer;
begin
  Result := mrNotNeeded;
  ABackupFile := '';
  AError := '';
  TargetDirectory := ExcludeTrailingPathDelimiter(ExpandFileName(
    ATargetDirectory));
  if FileExists(IncludeTrailingPathDelimiter(TargetDirectory) +
    'profiles.json') then
    Exit;
  if Trim(ALegacyDirectory) = '' then
    Exit;
  LegacyDirectory := ExcludeTrailingPathDelimiter(ExpandFileName(
    ALegacyDirectory));
  if (LegacyDirectory = '') or
    (not FileExists(IncludeTrailingPathDelimiter(LegacyDirectory) +
      'profiles.json')) then
    Exit;
  if DirectoryExists(TargetDirectory) then
  begin
    AError := 'Каталог LCL уже существует без profiles.json; автоматическая ' +
      'миграция остановлена, чтобы не перезаписать данные.';
    Exit(mrFailed);
  end;

  ParentDirectory := ExtractFileDir(TargetDirectory);
  if not ForceDirectories(ParentDirectory) then
  begin
    AError := 'Не удалось создать родительский каталог миграции.';
    Exit(mrFailed);
  end;
  WorkDirectory := IncludeTrailingPathDelimiter(ParentDirectory) +
    '.lcl-migration-work-' + FormatDateTime('yyyymmddhhnnsszzz', Now);
  StagingDirectory := TargetDirectory + '.staging-' +
    FormatDateTime('yyyymmddhhnnsszzz', Now);
  BackupDirectory := IncludeTrailingPathDelimiter(ParentDirectory) + 'backups';
  if not ForceDirectories(WorkDirectory) or
    not ForceDirectories(StagingDirectory) then
  begin
    AError := 'Не удалось создать staging каталоги миграции.';
    RemoveTree(WorkDirectory);
    RemoveTree(StagingDirectory);
    Exit(mrFailed);
  end;

  try
    if not CreateLegacyBackup(LegacyDirectory, BackupDirectory,
      WorkDirectory, ABackupFile, AError) then
      raise Exception.Create(AError);

    Store := TFpcProfileStore.Create(IncludeTrailingPathDelimiter(
      LegacyDirectory) + 'profiles.json');
    if not Store.Load(Profiles, AError) then
      raise Exception.Create(AError);
    Store := TFpcProfileStore.Create(IncludeTrailingPathDelimiter(
      StagingDirectory) + 'profiles.json');
    if not Store.Save(Profiles, AError) then
      raise Exception.Create(AError);

    SourceFile := IncludeTrailingPathDelimiter(LegacyDirectory) +
      'subscriptions.json';
    if FileExists(SourceFile) then
    begin
      StagingFile := IncludeTrailingPathDelimiter(StagingDirectory) +
        'subscriptions.json';
      CopyFileExact(SourceFile, StagingFile);
      if not ValidateJsonFile(StagingFile, AError) then
        raise Exception.Create(AError);
    end;

    RoutingStore := TFpcRoutingProfileStore.Create(
      IncludeTrailingPathDelimiter(LegacyDirectory) + 'routing.json');
    if not RoutingStore.Load(RoutingProfiles, AError) then
      raise Exception.Create(AError);
    RoutingStore := TFpcRoutingProfileStore.Create(
      IncludeTrailingPathDelimiter(StagingDirectory) + 'routing.json');
    if not RoutingStore.Save(RoutingProfiles, AError) then
      raise Exception.Create(AError);
    DnsStore := TFpcDnsProfileStore.Create(
      IncludeTrailingPathDelimiter(LegacyDirectory) + 'dns.json');
    if not DnsStore.Load(DnsProfiles, AError) then
      raise Exception.Create(AError);
    DnsStore := TFpcDnsProfileStore.Create(
      IncludeTrailingPathDelimiter(StagingDirectory) + 'dns.json');
    if not DnsStore.Save(DnsProfiles, AError) then
      raise Exception.Create(AError);

    VerifyStore := TFpcProfileStore.Create(IncludeTrailingPathDelimiter(
      StagingDirectory) + 'profiles.json');
    if not VerifyStore.Load(VerifiedProfiles, AError) then
      raise Exception.Create(AError);
    if Length(VerifiedProfiles) <> Length(Profiles) then
    begin
      AError := 'Количество профилей изменилось при staging verification.';
      raise Exception.Create(AError);
    end;
    if not ValidateRelations(VerifiedProfiles,
      IncludeTrailingPathDelimiter(StagingDirectory) +
      'subscriptions.json', AError) then
      raise Exception.Create(AError);
    RoutingStore := TFpcRoutingProfileStore.Create(
      IncludeTrailingPathDelimiter(StagingDirectory) + 'routing.json');
    if not RoutingStore.Load(VerifiedRouting, AError) then
      raise Exception.Create(AError);
    if Length(VerifiedRouting) <> Length(RoutingProfiles) then
      raise Exception.Create('Routing profile count changed during staging verification.');
    DnsStore := TFpcDnsProfileStore.Create(
      IncludeTrailingPathDelimiter(StagingDirectory) + 'dns.json');
    if not DnsStore.Load(VerifiedDns, AError) then
      raise Exception.Create(AError);
    if Length(VerifiedDns) <> Length(DnsProfiles) then
      raise Exception.Create('DNS profile count changed during staging verification.');
    if not MigrateQtSettings(StagingDirectory, VerifiedRouting, VerifiedDns,
      MigrationJson, AError) then
      raise Exception.Create(AError);
    WriteUtf8File(IncludeTrailingPathDelimiter(StagingDirectory) +
      'migration.json', MigrationJson);

    if not RenameFile(StagingDirectory, TargetDirectory) then
    begin
      AError := 'Не удалось атомарно установить проверенные LCL-данные.';
      raise Exception.Create(AError);
    end;
    Result := mrMigrated;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := mrFailed;
    end;
  end;
  Store := nil;
  VerifyStore := nil;
  RoutingStore := nil;
  DnsStore := nil;
  RemoveTree(WorkDirectory);
  if Result <> mrMigrated then
    RemoveTree(StagingDirectory);
end;

function EnsureFirstRunQtMigration(const ATargetDirectory: string;
  out ABackupFile, AError: string): TZaryaMigrationResult;
begin
  Result := MigrateQtData(LegacyQtDataDirectory, ATargetDirectory,
    ABackupFile, AError);
end;

end.
