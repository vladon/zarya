program DataMigrationTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, ZaryaProfile, ZaryaProfileStore, FpcProfileStore,
  ZaryaDataMigration, ZaryaFileIntegrity;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

procedure WriteText(const AFileName, AText: string);
var
  Lines: TStringList;
begin
  Lines := TStringList.Create;
  try
    Lines.Text := AText;
    Lines.SaveToFile(AFileName);
  finally
    Lines.Free;
  end;
end;

function FileHasContent(const AFileName: string): Boolean;
var
  Stream: TFileStream;
begin
  Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyWrite);
  try
    Result := Stream.Size > 0;
  finally
    Stream.Free;
  end;
end;

function ReadText(const AFileName: string): string;
var
  Lines: TStringList;
begin
  Lines := TStringList.Create;
  try
    Lines.LoadFromFile(AFileName);
    Result := Lines.Text;
  finally
    Lines.Free;
  end;
end;

procedure RemoveTree(const ADirectory: string);
var
  Search: TSearchRec;
  ItemPath: string;
begin
  if not DirectoryExists(ADirectory) then Exit;
  if FindFirst(IncludeTrailingPathDelimiter(ADirectory) + '*', faAnyFile,
    Search) = 0 then
  try
    repeat
      if (Search.Name = '.') or (Search.Name = '..') then Continue;
      ItemPath := IncludeTrailingPathDelimiter(ADirectory) + Search.Name;
      if (Search.Attr and faDirectory) <> 0 then RemoveTree(ItemPath)
      else DeleteFile(ItemPath);
    until FindNext(Search) <> 0;
  finally
    FindClose(Search);
  end;
  RemoveDir(ADirectory);
end;

var
  Root: string;
  Legacy: string;
  Target: string;
  BadLegacy: string;
  BadTarget: string;
  ConflictTarget: string;
  DuplicateLegacy: string;
  DuplicateTarget: string;
  CorruptPolicyLegacy: string;
  CorruptPolicyTarget: string;
  ProfileFile: string;
  OriginalHash: string;
  CurrentHash: string;
  BackupFile: string;
  ErrorMessage: string;
  Store: IZaryaProfileStore;
  Profiles: TZaryaProfiles;
  MigrationResult: TZaryaMigrationResult;
begin
  Randomize;
  Root := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-data-migration-' + IntToHex(Random(MaxInt), 8);
  Legacy := IncludeTrailingPathDelimiter(Root) + 'qt';
  Target := IncludeTrailingPathDelimiter(Root) + 'lcl';
  Check(ForceDirectories(Legacy), 'Could not create legacy fixture directory.');
  ProfileFile := IncludeTrailingPathDelimiter(Legacy) + 'profiles.json';
  WriteText(ProfileFile,
    '{"version":4,"profiles":[{' +
    '"id":"profile-1","name":"Migrated","protocol":"VLESS",' +
    '"coreType":"SingBox","address":"example.invalid","port":443,' +
    '"uuid":"11111111-1111-1111-1111-111111111111",' +
    '"password":"secret-fixture","method":"aes-128-gcm",' +
    '"network":"tcp","security":"tls","serverName":"sni.invalid",' +
    '"sourceType":"subscription","subscriptionId":"subscription-1",' +
    '"lastTcpPingMs":42,"lastRealDelayMs":91,' +
    '"lastTestStatus":"success","enabled":true}]}');
  WriteText(IncludeTrailingPathDelimiter(Legacy) + 'subscriptions.json',
    '{"version":1,"subscriptions":[{"id":"subscription-1",' +
    '"name":"Fixture","url":"https://example.invalid/sub"}]}');
  WriteText(IncludeTrailingPathDelimiter(Legacy) + 'routing.json',
    '{"version":1,"profiles":[]}');
  WriteText(IncludeTrailingPathDelimiter(Legacy) + 'dns.json',
    '{"version":1,"profiles":[]}');
  Check(Sha256File(ProfileFile, OriginalHash, ErrorMessage),
    'Could not hash legacy profile fixture.');

  MigrationResult := MigrateQtData(Legacy, Target, BackupFile, ErrorMessage);
  Check(MigrationResult = mrMigrated, 'Migration failed: ' + ErrorMessage);
  Check(FileExists(BackupFile), 'Pre-migration backup was not created.');
  Check(FileHasContent(BackupFile), 'Pre-migration backup is empty.');
  Check(FileExists(IncludeTrailingPathDelimiter(Target) + 'migration.json'),
    'Migration marker is missing.');
  Check(Pos('"schemaVersion" : 2', ReadText(
    IncludeTrailingPathDelimiter(Target) + 'migration.json')) > 0,
    'Migration journal is not schema v2.');
  Check(Sha256File(ProfileFile, CurrentHash, ErrorMessage),
    'Could not re-hash legacy profile fixture.');
  Check(CurrentHash = OriginalHash, 'Legacy profiles.json was modified.');

  Store := TFpcProfileStore.Create(IncludeTrailingPathDelimiter(Target) +
    'profiles.json');
  Check(Store.Load(Profiles, ErrorMessage),
    'Migrated profiles failed to load: ' + ErrorMessage);
  Check(Length(Profiles) = 1, 'Migrated profile count mismatch.');
  Check(Profiles[0].Password = 'secret-fixture',
    'Password field was lost during migration.');
  Check(Profiles[0].Method = 'aes-128-gcm',
    'Method field was lost during migration.');
  Check(Profiles[0].LastTcpPingMs = 42,
    'Test result was lost during migration.');
  Check(Profiles[0].PreferredProviderId = 'embedded.singbox',
    'Legacy core type was not mapped to provider id.');

  Check(MigrateQtData(Legacy, Target, BackupFile, ErrorMessage) = mrNotNeeded,
    'Repeated migration was not idempotent.');

  BadLegacy := IncludeTrailingPathDelimiter(Root) + 'qt-bad';
  BadTarget := IncludeTrailingPathDelimiter(Root) + 'lcl-bad';
  Check(ForceDirectories(BadLegacy),
    'Could not create invalid migration fixture directory.');
  ProfileFile := IncludeTrailingPathDelimiter(BadLegacy) + 'profiles.json';
  WriteText(ProfileFile,
    '{"version":4,"profiles":[{"id":"dangling-profile",' +
    '"name":"Dangling","protocol":"VLESS",' +
    '"address":"example.invalid","port":443,' +
    '"uuid":"11111111-1111-1111-1111-111111111111",' +
    '"subscriptionId":"missing-subscription"}]}');
  WriteText(IncludeTrailingPathDelimiter(BadLegacy) + 'subscriptions.json',
    '{"version":1,"subscriptions":[]}');
  Check(Sha256File(ProfileFile, OriginalHash, ErrorMessage),
    'Could not hash invalid legacy fixture.');
  Check(MigrateQtData(BadLegacy, BadTarget, BackupFile, ErrorMessage) =
    mrFailed, 'Dangling subscription relation was accepted.');
  Check(not DirectoryExists(BadTarget),
    'Failed migration installed its staging directory.');
  Check(FileExists(BackupFile),
    'Failed migration did not retain the pre-import backup.');
  Check(Sha256File(ProfileFile, CurrentHash, ErrorMessage),
    'Could not re-hash invalid legacy fixture.');
  Check(CurrentHash = OriginalHash,
    'Failed migration modified its legacy source.');

  ConflictTarget := IncludeTrailingPathDelimiter(Root) + 'lcl-conflict';
  Check(ForceDirectories(ConflictTarget),
    'Could not create target-conflict fixture directory.');
  WriteText(IncludeTrailingPathDelimiter(ConflictTarget) + 'unrelated.txt',
    'keep');
  Check(MigrateQtData(Legacy, ConflictTarget, BackupFile, ErrorMessage) =
    mrFailed, 'Existing non-LCL target directory was overwritten.');
  Check(FileExists(IncludeTrailingPathDelimiter(ConflictTarget) +
    'unrelated.txt'), 'Target conflict file was modified.');

  DuplicateLegacy := IncludeTrailingPathDelimiter(Root) + 'qt-duplicate';
  DuplicateTarget := IncludeTrailingPathDelimiter(Root) + 'lcl-duplicate';
  Check(ForceDirectories(DuplicateLegacy),
    'Could not create duplicate migration fixture.');
  WriteText(IncludeTrailingPathDelimiter(DuplicateLegacy) + 'profiles.json',
    '{"version":4,"profiles":[' +
    '{"id":"same","name":"One","protocol":"SOCKS","address":"127.0.0.1","port":1080},' +
    '{"id":"same","name":"Two","protocol":"SOCKS","address":"127.0.0.1","port":1081}]}');
  WriteText(IncludeTrailingPathDelimiter(DuplicateLegacy) +
    'subscriptions.json', '{"version":1,"subscriptions":[]}');
  Check(MigrateQtData(DuplicateLegacy, DuplicateTarget, BackupFile,
    ErrorMessage) = mrFailed, 'Duplicate profile ids were accepted.');
  Check(not DirectoryExists(DuplicateTarget),
    'Duplicate-id migration installed partial data.');

  CorruptPolicyLegacy := IncludeTrailingPathDelimiter(Root) +
    'qt-corrupt-policy';
  CorruptPolicyTarget := IncludeTrailingPathDelimiter(Root) +
    'lcl-corrupt-policy';
  Check(ForceDirectories(CorruptPolicyLegacy),
    'Could not create corrupt policy fixture.');
  WriteText(IncludeTrailingPathDelimiter(CorruptPolicyLegacy) +
    'profiles.json', '{"version":4,"profiles":[]}');
  WriteText(IncludeTrailingPathDelimiter(CorruptPolicyLegacy) +
    'subscriptions.json', '{"version":1,"subscriptions":[]}');
  WriteText(IncludeTrailingPathDelimiter(CorruptPolicyLegacy) +
    'routing.json', '{not-json');
  Check(MigrateQtData(CorruptPolicyLegacy, CorruptPolicyTarget, BackupFile,
    ErrorMessage) = mrFailed, 'Corrupt routing data was accepted.');
  Check(not DirectoryExists(CorruptPolicyTarget),
    'Corrupt-policy migration installed partial data.');
  Store := nil;
  RemoveTree(Root);
  WriteLn('Qt to LCL staging migration: PASS');
end.
