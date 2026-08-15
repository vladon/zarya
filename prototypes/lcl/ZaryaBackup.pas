unit ZaryaBackup;

{$mode objfpc}{$H+}

interface

uses
  SysUtils;

function CreateZaryaBackup(const ADataDirectory, ABackupFile: string;
  out AError: string): Boolean;
function RestoreZaryaBackup(const ABackupFile, ADataDirectory: string;
  out APreRestoreBackup, AError: string): Boolean;
function IsAllowedBackupEntry(const AEntryName: string): Boolean;

implementation

uses
  Classes, fpjson, jsonparser, Zipper, ZaryaProfile, ZaryaProfileStore,
  FpcProfileStore, ZaryaSubscription, FpcSubscriptionStore,
  ZaryaCoreProvider, ZaryaCoreProviderStore, FpcCoreProviderStore,
  ZaryaAppSettings, ZaryaFileIntegrity;

const
  BackupVersion = 1;
  MaxArchiveBytes = 256 * 1024 * 1024;
  MaxEntryBytes = 128 * 1024 * 1024;
  BackupDataFiles: array[0..5] of string = (
    'profiles.json',
    'subscriptions.json',
    'providers.json',
    'settings.ini',
    'routing.json',
    'dns.json'
  );

type
  TZaryaBackupStrings = array of string;
  TZaryaBackupBooleans = array of Boolean;

function NormalizedEntryName(const AEntryName: string): string;
begin
  Result := StringReplace(Trim(AEntryName), '\', '/', [rfReplaceAll]);
end;

function IsKnownDataFile(const AFileName: string): Boolean;
var
  I: Integer;
begin
  for I := Low(BackupDataFiles) to High(BackupDataFiles) do
    if SameText(AFileName, BackupDataFiles[I]) then
      Exit(True);
  Result := False;
end;

function IsAllowedBackupEntry(const AEntryName: string): Boolean;
var
  Name: string;
begin
  Name := NormalizedEntryName(AEntryName);
  if Name = 'manifest.json' then
    Exit(True);
  Result := (Copy(Name, 1, 5) = 'data/') and
    IsKnownDataFile(Copy(Name, 6, MaxInt)) and
    (Pos('..', Name) = 0) and (Pos(':', Name) = 0) and
    (Copy(Name, 1, 1) <> '/');
end;

function UniqueWorkDirectory(const APrefix: string): string;
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
  Result := IncludeTrailingPathDelimiter(GetTempDir(False)) + APrefix + Suffix;
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

function FileSizeByName(const AFileName: string): Int64;
var
  Stream: TFileStream;
begin
  Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyNone);
  try
    Result := Stream.Size;
  finally
    Stream.Free;
  end;
end;

function ContainsAbsolutePath(const AValue: string): Boolean;
var
  I: Integer;
begin
  if Pos('\\', AValue) > 0 then
    Exit(True);
  for I := 1 to Length(AValue) - 2 do
    if (AValue[I] in ['A'..'Z', 'a'..'z']) and (AValue[I + 1] = ':') and
      (AValue[I + 2] in ['\', '/']) then
      Exit(True);
  Result := (AValue <> '') and (AValue[1] = '/') and
    (Copy(AValue, 1, 2) <> '//');
end;

function PortableProviderText(const AValue: string): string;
begin
  if ContainsAbsolutePath(AValue) then
    Result := '<machine-specific-value-removed>'
  else
    Result := AValue;
end;

procedure SanitizeProviderArguments(var AValues: TZaryaStringArray);
var
  I: Integer;
begin
  for I := 0 to High(AValues) do
    AValues[I] := PortableProviderText(AValues[I]);
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

function LoadJsonObject(const AFileName: string; out AData: TJSONData;
  out AError: string): Boolean;
var
  Stream: TFileStream;
begin
  Result := False;
  AData := nil;
  AError := '';
  try
    Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyWrite);
    try
      AData := GetJSON(Stream);
    finally
      Stream.Free;
    end;
    if AData.JSONType <> jtObject then
      raise Exception.Create('Корневой JSON должен быть объектом: ' +
        ExtractFileName(AFileName));
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      FreeAndNil(AData);
    end;
  end;
end;

function ValidateDataFile(const AFileName, ABaseName: string;
  out AError: string): Boolean;
var
  ProfileStore: IZaryaProfileStore;
  Profiles: TZaryaProfiles;
  SubscriptionStore: TFpcSubscriptionStore;
  Subscriptions: TZaryaSubscriptions;
  ProviderStore: IZaryaCoreProviderStore;
  Providers: TZaryaCoreProviders;
  SettingsStore: TZaryaAppSettingsStore;
  Settings: TZaryaAppSettings;
  Json: TJSONData;
begin
  AError := '';
  if SameText(ABaseName, 'profiles.json') then
  begin
    ProfileStore := TFpcProfileStore.Create(AFileName);
    Result := ProfileStore.Load(Profiles, AError);
  end
  else if SameText(ABaseName, 'subscriptions.json') then
  begin
    SubscriptionStore := TFpcSubscriptionStore.Create(AFileName);
    try
      Result := SubscriptionStore.Load(Subscriptions, AError);
    finally
      SubscriptionStore.Free;
    end;
  end
  else if SameText(ABaseName, 'providers.json') then
  begin
    ProviderStore := TFpcCoreProviderStore.Create(AFileName);
    Result := ProviderStore.Load(Providers, AError);
  end
  else if SameText(ABaseName, 'settings.ini') then
  begin
    SettingsStore := TZaryaAppSettingsStore.Create(AFileName);
    try
      Result := SettingsStore.Load(Settings, AError);
    finally
      SettingsStore.Free;
    end;
  end
  else
  begin
    Result := LoadJsonObject(AFileName, Json, AError);
    Json.Free;
  end;
end;

function SanitizeProviderFile(const ASource, ADestination: string;
  out AError: string): Boolean;
var
  SourceStore: IZaryaCoreProviderStore;
  DestinationStore: IZaryaCoreProviderStore;
  Providers: TZaryaCoreProviders;
  I: Integer;
begin
  SourceStore := TFpcCoreProviderStore.Create(ASource);
  if not SourceStore.Load(Providers, AError) then
    Exit(False);
  for I := 0 to High(Providers) do
  begin
    Providers[I].DisplayName := PortableProviderText(Providers[I].DisplayName);
    Providers[I].ExecutablePath := '';
    Providers[I].WorkingDirectory := '';
    Providers[I].AssetDirectory := '';
    Providers[I].Version := '';
    Providers[I].Architecture := '';
    Providers[I].Sha256 := '';
    Providers[I].ConfirmedSha256 := '';
    Providers[I].State := psMissing;
    Providers[I].LastError := '';
    SanitizeProviderArguments(Providers[I].VersionArguments);
    SanitizeProviderArguments(Providers[I].ValidateArguments);
    SanitizeProviderArguments(Providers[I].RunArguments);
  end;
  DestinationStore := TFpcCoreProviderStore.Create(ADestination);
  Result := DestinationStore.Save(Providers, AError);
end;

function SanitizeProfileFile(const ASource, ADestination: string;
  out AError: string): Boolean;
var
  SourceStore: IZaryaProfileStore;
  DestinationStore: IZaryaProfileStore;
  Profiles: TZaryaProfiles;
  I: Integer;
begin
  SourceStore := TFpcProfileStore.Create(ASource);
  if not SourceStore.Load(Profiles, AError) then Exit(False);
  for I := 0 to High(Profiles) do
  begin
    Profiles[I].Uuid := '';
    Profiles[I].Password := '';
    Profiles[I].ObfsPassword := '';
    Profiles[I].PreSharedKey := '';
    Profiles[I].RawConfig := '';
    Profiles[I].SourceKey := '';
    Profiles[I].LastTestError := '';
    Profiles[I].Enabled := False;
    Profiles[I].UnsupportedReason :=
      'credentials omitted from portable backup';
  end;
  DestinationStore := TFpcProfileStore.Create(ADestination);
  Result := DestinationStore.Save(Profiles, AError);
end;

function SanitizeSubscriptionFile(const ASource, ADestination: string;
  out AError: string): Boolean;
var
  SourceStore: TFpcSubscriptionStore;
  DestinationStore: TFpcSubscriptionStore;
  Subscriptions: TZaryaSubscriptions;
  I: Integer;
begin
  SourceStore := TFpcSubscriptionStore.Create(ASource);
  try
    if not SourceStore.Load(Subscriptions, AError) then Exit(False);
  finally
    SourceStore.Free;
  end;
  for I := 0 to High(Subscriptions) do
  begin
    Subscriptions[I].Url := '';
    Subscriptions[I].UserAgent := '';
    Subscriptions[I].Remarks := '';
    Subscriptions[I].LastError := '';
    Subscriptions[I].Enabled := False;
    Subscriptions[I].LastStatus := ssDisabled;
  end;
  DestinationStore := TFpcSubscriptionStore.Create(ADestination);
  try
    Result := DestinationStore.Save(Subscriptions, AError);
  finally
    DestinationStore.Free;
  end;
end;

function ValidateSubscriptionRelations(const AStagingDirectory: string;
  out AError: string): Boolean;
var
  Profiles: TZaryaProfiles;
  Subscriptions: TZaryaSubscriptions;
  ProfileStore: IZaryaProfileStore;
  SubscriptionStore: TFpcSubscriptionStore;
  ProfileFile: string;
  SubscriptionFile: string;
  I: Integer;
  J: Integer;
  Found: Boolean;
begin
  AError := '';
  ProfileFile := IncludeTrailingPathDelimiter(AStagingDirectory) +
    'profiles.json';
  SubscriptionFile := IncludeTrailingPathDelimiter(AStagingDirectory) +
    'subscriptions.json';
  if not FileExists(ProfileFile) then
    Exit(True);
  ProfileStore := TFpcProfileStore.Create(ProfileFile);
  if not ProfileStore.Load(Profiles, AError) then
    Exit(False);
  SetLength(Subscriptions, 0);
  if FileExists(SubscriptionFile) then
  begin
    SubscriptionStore := TFpcSubscriptionStore.Create(SubscriptionFile);
    try
      if not SubscriptionStore.Load(Subscriptions, AError) then
        Exit(False);
    finally
      SubscriptionStore.Free;
    end;
  end;
  for I := 0 to High(Profiles) do
  begin
    if Trim(Profiles[I].SubscriptionId) = '' then
      Continue;
    Found := False;
    for J := 0 to High(Subscriptions) do
      if SameText(Profiles[I].SubscriptionId, Subscriptions[J].Id) then
      begin
        Found := True;
        Break;
      end;
    if not Found then
    begin
      AError := 'Профиль ссылается на отсутствующую подписку.';
      Exit(False);
    end;
  end;
  Result := True;
end;

function CreateZaryaBackup(const ADataDirectory, ABackupFile: string;
  out AError: string): Boolean;
var
  DataDirectory: string;
  WorkDirectory: string;
  SnapshotDirectory: string;
  SourceFile: string;
  SnapshotFile: string;
  ManifestFile: string;
  TempBackupFile: string;
  PreviousBackupFile: string;
  Manifest: TJSONObject;
  Files: TJSONArray;
  Entry: TJSONObject;
  Zipper: TZipper;
  Digest: string;
  I: Integer;
begin
  Result := False;
  AError := '';
  DataDirectory := ExcludeTrailingPathDelimiter(ExpandFileName(ADataDirectory));
  if not FileExists(IncludeTrailingPathDelimiter(DataDirectory) +
    'profiles.json') then
  begin
    AError := 'profiles.json отсутствует; создавать пустой backup нельзя.';
    Exit;
  end;
  if Trim(ABackupFile) = '' then
  begin
    AError := 'Не указан файл backup.';
    Exit;
  end;
  if not ForceDirectories(ExtractFileDir(ExpandFileName(ABackupFile))) then
  begin
    AError := 'Не удалось создать каталог backup.';
    Exit;
  end;

  WorkDirectory := UniqueWorkDirectory('zarya-backup-');
  SnapshotDirectory := IncludeTrailingPathDelimiter(WorkDirectory) + 'data';
  TempBackupFile := ExpandFileName(ABackupFile) + '.tmp';
  PreviousBackupFile := ExpandFileName(ABackupFile) + '.replace-old';
  try
    if not ForceDirectories(SnapshotDirectory) then
      raise Exception.Create('Не удалось создать staging-каталог backup.');
    Manifest := TJSONObject.Create;
    try
      Manifest.Add('backupVersion', BackupVersion);
      Manifest.Add('createdAt', FormatDateTime('yyyy-mm-dd"T"hh:nn:ss', Now));
      Manifest.Add('source', 'lcl');
      Manifest.Add('containsRawConfigs', False);
      Manifest.Add('containsCredentials', False);
      Manifest.Add('containsExternalExecutables', False);
      Manifest.Add('secretsOmitted', True);
      Files := TJSONArray.Create;
      Manifest.Add('files', Files);
      for I := Low(BackupDataFiles) to High(BackupDataFiles) do
      begin
        SourceFile := IncludeTrailingPathDelimiter(DataDirectory) +
          BackupDataFiles[I];
        if not FileExists(SourceFile) then
          Continue;
        SnapshotFile := IncludeTrailingPathDelimiter(SnapshotDirectory) +
          BackupDataFiles[I];
        if SameText(BackupDataFiles[I], 'profiles.json') then
        begin
          if not SanitizeProfileFile(SourceFile, SnapshotFile, AError) then
            raise Exception.Create(AError);
        end
        else if SameText(BackupDataFiles[I], 'subscriptions.json') then
        begin
          if not SanitizeSubscriptionFile(SourceFile, SnapshotFile,
            AError) then raise Exception.Create(AError);
        end
        else if SameText(BackupDataFiles[I], 'providers.json') then
        begin
          if not SanitizeProviderFile(SourceFile, SnapshotFile, AError) then
            raise Exception.Create(AError);
        end
        else
        begin
          if not ValidateDataFile(SourceFile, BackupDataFiles[I], AError) then
            raise Exception.Create(AError);
          CopyFileExact(SourceFile, SnapshotFile);
        end;
        if not Sha256File(SnapshotFile, Digest, AError) then
          raise Exception.Create(AError);
        Entry := TJSONObject.Create;
        Entry.Add('path', 'data/' + BackupDataFiles[I]);
        Entry.Add('sha256', Digest);
        Entry.Add('size', FileSizeByName(SnapshotFile));
        Files.Add(Entry);
      end;
      ManifestFile := IncludeTrailingPathDelimiter(WorkDirectory) +
        'manifest.json';
      WriteUtf8File(ManifestFile, Manifest.FormatJSON);
    finally
      Manifest.Free;
    end;

    if FileExists(TempBackupFile) then
      DeleteFile(TempBackupFile);
    Zipper := TZipper.Create;
    try
      Zipper.FileName := TempBackupFile;
      Zipper.Entries.AddFileEntry(ManifestFile, 'manifest.json');
      for I := Low(BackupDataFiles) to High(BackupDataFiles) do
      begin
        SnapshotFile := IncludeTrailingPathDelimiter(SnapshotDirectory) +
          BackupDataFiles[I];
        if FileExists(SnapshotFile) then
          Zipper.Entries.AddFileEntry(SnapshotFile,
            'data/' + BackupDataFiles[I]);
      end;
      Zipper.ZipAllFiles;
    finally
      Zipper.Free;
    end;
    if not FileExists(TempBackupFile) then
      raise Exception.Create('Backup ZIP не был создан.');
    if FileExists(PreviousBackupFile) then
      raise Exception.Create('Обнаружен незавершённый файл замены backup.');
    if FileExists(ExpandFileName(ABackupFile)) and
      (not RenameFile(ExpandFileName(ABackupFile), PreviousBackupFile)) then
      raise Exception.Create('Не удалось подготовить замену backup.');
    if not RenameFile(TempBackupFile, ExpandFileName(ABackupFile)) then
    begin
      if FileExists(PreviousBackupFile) then
        RenameFile(PreviousBackupFile, ExpandFileName(ABackupFile));
      raise Exception.Create('Не удалось установить backup ZIP.');
    end;
    if FileExists(PreviousBackupFile) then
      DeleteFile(PreviousBackupFile);
    Result := True;
  except
    on E: Exception do
      AError := E.Message;
  end;
  if FileExists(TempBackupFile) then
    DeleteFile(TempBackupFile);
  RemoveTree(WorkDirectory);
end;

function ArchiveEntryIndex(const AUnZipper: TUnZipper;
  const AName: string): Integer;
begin
  for Result := 0 to AUnZipper.Entries.Count - 1 do
    if SameText(NormalizedEntryName(
      AUnZipper.Entries[Result].ArchiveFileName), AName) then
      Exit;
  Result := -1;
end;

function ExamineBackup(const ABackupFile: string; const AUnZipper: TUnZipper;
  out AError: string): Boolean;
var
  Seen: TStringList;
  Name: string;
  I: Integer;
  TotalSize: Int64;
begin
  Result := False;
  AError := '';
  if (not FileExists(ABackupFile)) or (FileSizeByName(ABackupFile) <= 0) then
  begin
    AError := 'Backup ZIP не найден или пуст.';
    Exit;
  end;
  if FileSizeByName(ABackupFile) > MaxArchiveBytes then
  begin
    AError := 'Backup ZIP превышает допустимый размер.';
    Exit;
  end;
  Seen := TStringList.Create;
  try
    Seen.CaseSensitive := False;
    Seen.Sorted := True;
    Seen.Duplicates := dupError;
    AUnZipper.FileName := ABackupFile;
    AUnZipper.Examine;
    TotalSize := 0;
    try
      for I := 0 to AUnZipper.Entries.Count - 1 do
      begin
        Name := NormalizedEntryName(
          AUnZipper.Entries[I].ArchiveFileName);
        if not IsAllowedBackupEntry(Name) then
          raise Exception.Create('Недопустимая ZIP-запись: ' + Name);
        Seen.Add(Name);
        if (AUnZipper.Entries[I].Size < 0) or
          (AUnZipper.Entries[I].Size > MaxEntryBytes) then
          raise Exception.Create('Недопустимый размер ZIP-записи: ' + Name);
        Inc(TotalSize, AUnZipper.Entries[I].Size);
        if TotalSize > MaxArchiveBytes then
          raise Exception.Create('Распакованные данные слишком велики.');
      end;
      if Seen.IndexOf('manifest.json') < 0 then
        raise Exception.Create('В backup отсутствует manifest.json.');
      Result := True;
    except
      on E: Exception do
        AError := E.Message;
    end;
  finally
    Seen.Free;
  end;
end;

function ValidateExtractedBackup(const AWorkDirectory: string;
  const AUnZipper: TUnZipper; out AFiles: TZaryaBackupStrings;
  out AError: string): Boolean;
var
  ManifestData: TJSONData;
  Manifest: TJSONObject;
  Files: TJSONArray;
  Item: TJSONObject;
  Seen: TStringList;
  EntryName: string;
  BaseName: string;
  FileName: string;
  ExpectedHash: string;
  ActualHash: string;
  SourceName: string;
  I: Integer;
begin
  Result := False;
  AError := '';
  SetLength(AFiles, 0);
  ManifestData := nil;
  Seen := TStringList.Create;
  try
    Seen.CaseSensitive := False;
    Seen.Sorted := True;
    Seen.Duplicates := dupError;
    if not LoadJsonObject(IncludeTrailingPathDelimiter(AWorkDirectory) +
      'manifest.json', ManifestData, AError) then
      Exit;
    Manifest := TJSONObject(ManifestData);
    if Manifest.Get('backupVersion', 0) <> BackupVersion then
      raise Exception.Create('Неподдерживаемая версия backup.');
    SourceName := Manifest.Get('source', '');
    if not (SameText(SourceName, 'lcl') or SameText(SourceName, 'qt')) then
      raise Exception.Create('Неизвестный источник backup.');
    Files := Manifest.Arrays['files'];
    if Files.Count = 0 then
      raise Exception.Create('Backup не содержит файлов данных.');
    SetLength(AFiles, Files.Count);
    for I := 0 to Files.Count - 1 do
    begin
      if Files.Items[I].JSONType <> jtObject then
        raise Exception.Create('Некорректная запись manifest.');
      Item := Files.Objects[I];
      EntryName := NormalizedEntryName(Item.Get('path', ''));
      if (EntryName = 'manifest.json') or
        (not IsAllowedBackupEntry(EntryName)) then
        raise Exception.Create('Недопустимый путь в manifest: ' + EntryName);
      Seen.Add(EntryName);
      if ArchiveEntryIndex(AUnZipper, EntryName) < 0 then
        raise Exception.Create('Файл из manifest отсутствует: ' + EntryName);
      BaseName := Copy(EntryName, 6, MaxInt);
      FileName := IncludeTrailingPathDelimiter(AWorkDirectory) + 'data' +
        DirectorySeparator + BaseName;
      if not FileExists(FileName) then
        raise Exception.Create('Файл backup не распакован: ' + EntryName);
      ExpectedHash := LowerCase(Item.Get('sha256', ''));
      if (Length(ExpectedHash) <> 64) or
        (not Sha256File(FileName, ActualHash, AError)) or
        (not SameText(ExpectedHash, ActualHash)) then
        raise Exception.Create('SHA-256 не совпадает: ' + EntryName);
      if not ValidateDataFile(FileName, BaseName, AError) then
        raise Exception.Create('Проверка ' + EntryName + ': ' + AError);
      AFiles[I] := BaseName;
    end;
    if Seen.Count <> AUnZipper.Entries.Count - 1 then
      raise Exception.Create('ZIP и manifest содержат разный набор файлов.');
    if not ValidateSubscriptionRelations(
      IncludeTrailingPathDelimiter(AWorkDirectory) + 'data', AError) then
      raise Exception.Create(AError);
    Result := True;
  except
    on E: Exception do
      AError := E.Message;
  end;
  Seen.Free;
  ManifestData.Free;
end;

function InstallStagedFiles(const AStagingDirectory, ADataDirectory: string;
  const AFiles: TZaryaBackupStrings; out AError: string): Boolean;
var
  Destinations: TZaryaBackupStrings;
  NewFiles: TZaryaBackupStrings;
  OldFiles: TZaryaBackupStrings;
  HadOld: TZaryaBackupBooleans;
  InstalledCount: Integer;
  I: Integer;
begin
  Result := False;
  AError := '';
  InstalledCount := 0;
  SetLength(Destinations, Length(AFiles));
  SetLength(NewFiles, Length(AFiles));
  SetLength(OldFiles, Length(AFiles));
  SetLength(HadOld, Length(AFiles));
  try
    if not ForceDirectories(ADataDirectory) then
      raise Exception.Create('Не удалось создать каталог данных.');
    for I := 0 to High(AFiles) do
    begin
      Destinations[I] := IncludeTrailingPathDelimiter(ADataDirectory) +
        AFiles[I];
      NewFiles[I] := Destinations[I] + '.restore-new';
      OldFiles[I] := Destinations[I] + '.restore-old';
      if FileExists(NewFiles[I]) or FileExists(OldFiles[I]) then
        raise Exception.Create('Обнаружены незавершённые restore-файлы для ' +
          AFiles[I] + '.');
      CopyFileExact(IncludeTrailingPathDelimiter(AStagingDirectory) +
        AFiles[I], NewFiles[I]);
    end;
    for I := 0 to High(AFiles) do
    begin
      HadOld[I] := FileExists(Destinations[I]);
      if HadOld[I] and (not RenameFile(Destinations[I], OldFiles[I])) then
        raise Exception.Create('Не удалось подготовить замену ' + AFiles[I]);
      if not RenameFile(NewFiles[I], Destinations[I]) then
      begin
        if HadOld[I] then
          RenameFile(OldFiles[I], Destinations[I]);
        raise Exception.Create('Не удалось установить ' + AFiles[I]);
      end;
      Inc(InstalledCount);
    end;
    for I := 0 to High(AFiles) do
      if FileExists(OldFiles[I]) then
        DeleteFile(OldFiles[I]);
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      for I := InstalledCount - 1 downto 0 do
      begin
        if FileExists(Destinations[I]) then
          DeleteFile(Destinations[I]);
        if HadOld[I] and FileExists(OldFiles[I]) then
          RenameFile(OldFiles[I], Destinations[I]);
      end;
    end;
  end;
  for I := 0 to High(AFiles) do
    if FileExists(NewFiles[I]) then
      DeleteFile(NewFiles[I]);
end;

function RestoreZaryaBackup(const ABackupFile, ADataDirectory: string;
  out APreRestoreBackup, AError: string): Boolean;
var
  WorkDirectory: string;
  DataDirectory: string;
  BackupDirectory: string;
  Files: TZaryaBackupStrings;
  UnZipper: TUnZipper;
begin
  Result := False;
  AError := '';
  APreRestoreBackup := '';
  DataDirectory := ExcludeTrailingPathDelimiter(ExpandFileName(ADataDirectory));
  WorkDirectory := UniqueWorkDirectory('zarya-restore-');
  try
    if not ForceDirectories(WorkDirectory) then
      raise Exception.Create('Не удалось создать staging-каталог restore.');
    UnZipper := TUnZipper.Create;
    try
      if not ExamineBackup(ExpandFileName(ABackupFile), UnZipper,
        AError) then
        raise Exception.Create(AError);
      UnZipper.OutputPath := WorkDirectory;
      UnZipper.UnZipAllFiles;
      if not ValidateExtractedBackup(WorkDirectory, UnZipper, Files,
        AError) then
        raise Exception.Create(AError);
    finally
      UnZipper.Free;
    end;

    if FileExists(IncludeTrailingPathDelimiter(DataDirectory) +
      'profiles.json') then
    begin
      BackupDirectory := IncludeTrailingPathDelimiter(
        ExtractFileDir(DataDirectory)) + 'backups';
      if not ForceDirectories(BackupDirectory) then
        raise Exception.Create('Не удалось создать каталог pre-restore backup.');
      APreRestoreBackup := IncludeTrailingPathDelimiter(BackupDirectory) +
        'pre-restore-' + FormatDateTime('yyyymmdd-hhnnss-zzz', Now) +
        '.zarya-backup.zip';
      if not CreateZaryaBackup(DataDirectory, APreRestoreBackup,
        AError) then
        raise Exception.Create('Pre-restore backup: ' + AError);
    end;
    if not InstallStagedFiles(IncludeTrailingPathDelimiter(WorkDirectory) +
      'data', DataDirectory, Files, AError) then
      raise Exception.Create(AError);
    Result := True;
  except
    on E: Exception do
      AError := E.Message;
  end;
  RemoveTree(WorkDirectory);
end;

end.
