program BackupDiagnosticsTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, ZaryaProfile, ZaryaProfileStore, FpcProfileStore,
  ZaryaSubscription, FpcSubscriptionStore, ZaryaCoreProvider,
  ZaryaCoreProviderStore, FpcCoreProviderStore, ZaryaCoreProviderRegistry,
  ZaryaAppSettings, ZaryaBackup, ZaryaDiagnostics;

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

var
  Root: string;
  SourceDirectory: string;
  TargetDirectory: string;
  BackupFile: string;
  DiagnosticsFile: string;
  PreRestoreBackup: string;
  ErrorMessage: string;
  ProfileStore: IZaryaProfileStore;
  Profiles: TZaryaProfiles;
  LoadedProfiles: TZaryaProfiles;
  SubscriptionStore: TFpcSubscriptionStore;
  Subscriptions: TZaryaSubscriptions;
  ProviderStore: IZaryaCoreProviderStore;
  Providers: TZaryaCoreProviders;
  LoadedProviders: TZaryaCoreProviders;
  Provider: TZaryaCoreProvider;
  Registry: TZaryaCoreProviderRegistry;
  SettingsStore: TZaryaAppSettingsStore;
  Settings: TZaryaAppSettings;
  DiagnosticsJson: string;
  Hash: string;
begin
  Randomize;
  Root := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-backup-diagnostics-' + IntToHex(Random(MaxInt), 8);
  SourceDirectory := IncludeTrailingPathDelimiter(Root) + 'source';
  TargetDirectory := IncludeTrailingPathDelimiter(Root) + 'target';
  BackupFile := IncludeTrailingPathDelimiter(Root) +
    'fixture.zarya-backup.zip';
  DiagnosticsFile := IncludeTrailingPathDelimiter(Root) +
    'fixture.zarya-diagnostics.zip';
  Check(ForceDirectories(SourceDirectory),
    'Could not create source data directory.');
  try
    SetLength(Profiles, 1);
    Profiles[0] := CreateEmptyProfile;
    Profiles[0].Id := 'profile-secret-id';
    Profiles[0].Name := 'private profile name';
    Profiles[0].ProtocolName := 'VLESS';
    Profiles[0].Host := 'server.secret.invalid';
    Profiles[0].Uuid := '11111111-2222-3333-4444-555555555555';
    Profiles[0].Password := 'credential-secret-token';
    Profiles[0].RawConfig := '{"private":"raw-secret-token"}';
    Profiles[0].RawConfigFormat := 'mihomo-yaml';
    Profiles[0].ReadinessHost := '127.0.0.1';
    Profiles[0].ReadinessPort := 10808;
    Profiles[0].PreferredProviderId := ProviderExternalMihomo;
    Profiles[0].SubscriptionId := 'subscription-1';
    ProfileStore := TFpcProfileStore.Create(
      IncludeTrailingPathDelimiter(SourceDirectory) + 'profiles.json');
    Check(ProfileStore.Save(Profiles, ErrorMessage),
      'Profile fixture save failed: ' + ErrorMessage);

    SetLength(Subscriptions, 1);
    Subscriptions[0] := CreateEmptySubscription;
    Subscriptions[0].Id := 'subscription-1';
    Subscriptions[0].Name := 'Private subscription';
    Subscriptions[0].Url := 'https://subscription.secret.invalid/token';
    SubscriptionStore := TFpcSubscriptionStore.Create(
      IncludeTrailingPathDelimiter(SourceDirectory) + 'subscriptions.json');
    try
      Check(SubscriptionStore.Save(Subscriptions, ErrorMessage),
        'Subscription fixture save failed: ' + ErrorMessage);
    finally
      SubscriptionStore.Free;
    end;

    Hash := StringOfChar('a', 64);
    Provider := CreateProviderPreset(ProviderExternalMihomo);
    Provider.ExecutablePath := 'C:\external cores\mihomo.exe';
    Provider.WorkingDirectory := 'C:\external cores';
    Provider.AssetDirectory := 'C:\external cores\assets';
    Provider.Version := 'Mihomo 1.2 from C:\external cores\mihomo.exe';
    Provider.Architecture := 'x86_64';
    Provider.Sha256 := Hash;
    Provider.ConfirmedSha256 := Hash;
    SetLength(Providers, 2);
    Providers[0] := Provider;
    Provider := CreateProviderPreset('external.custom.fixture');
    Provider.DisplayName := 'Custom from C:\machine-only\core.exe';
    Provider.RunArguments := StringArray([
      '--asset', 'C:\machine-only\assets', '{config}']);
    Providers[1] := Provider;
    ProviderStore := TFpcCoreProviderStore.Create(
      IncludeTrailingPathDelimiter(SourceDirectory) + 'providers.json');
    Check(ProviderStore.Save(Providers, ErrorMessage),
      'Provider fixture save failed: ' + ErrorMessage);

    Settings := DefaultAppSettings;
    Settings.MixedPort := 19080;
    SettingsStore := TZaryaAppSettingsStore.Create(
      IncludeTrailingPathDelimiter(SourceDirectory) + 'settings.ini');
    try
      Check(SettingsStore.Save(Settings, ErrorMessage),
        'Settings fixture save failed: ' + ErrorMessage);
    finally
      SettingsStore.Free;
    end;
    WriteText(IncludeTrailingPathDelimiter(SourceDirectory) + 'routing.json',
      '{"version":1,"rules":[]}');
    WriteText(IncludeTrailingPathDelimiter(SourceDirectory) + 'dns.json',
      '{"version":1,"servers":[]}');

    Registry := TZaryaCoreProviderRegistry.Create(
      TFpcCoreProviderStore.Create(IncludeTrailingPathDelimiter(
        SourceDirectory) + 'providers.json'));
    try
      Check(Registry.Load(ErrorMessage),
        'Registry fixture load failed: ' + ErrorMessage);
      DiagnosticsJson := BuildDiagnosticsJson(Profiles, Registry, Settings);
      Check(Pos('credential-secret-token', DiagnosticsJson) = 0,
        'Diagnostics leaked credentials.');
      Check(Pos('raw-secret-token', DiagnosticsJson) = 0,
        'Diagnostics leaked raw config.');
      Check(Pos('server.secret.invalid', DiagnosticsJson) = 0,
        'Diagnostics leaked profile endpoint.');
      Check(Pos('C:\external cores', DiagnosticsJson) = 0,
        'Diagnostics leaked an external path.');
      Check(Pos(ProviderExternalMihomo, DiagnosticsJson) > 0,
        'Diagnostics omitted provider id.');
      Check(Pos(Copy(Hash, 1, 12), DiagnosticsJson) > 0,
        'Diagnostics omitted shortened provider hash.');
      Check(CreateDiagnosticsBundle(DiagnosticsFile, Profiles, Registry,
        Settings, ErrorMessage),
        'Diagnostics bundle failed: ' + ErrorMessage);
      Check(FileExists(DiagnosticsFile), 'Diagnostics ZIP is missing.');
    finally
      Registry.Free;
    end;

    Check(IsAllowedBackupEntry('data/profiles.json'),
      'Valid backup path was rejected.');
    Check(not IsAllowedBackupEntry('../profiles.json'),
      'Traversal backup path was accepted.');
    Check(not IsAllowedBackupEntry('data/../../evil.exe'),
      'Nested traversal backup path was accepted.');
    Check(CreateZaryaBackup(SourceDirectory, BackupFile, ErrorMessage),
      'Backup creation failed: ' + ErrorMessage);
    Check(FileExists(BackupFile), 'Backup ZIP is missing.');

    Check(RestoreZaryaBackup(BackupFile, TargetDirectory, PreRestoreBackup,
      ErrorMessage), 'Backup restore failed: ' + ErrorMessage);
    Check(PreRestoreBackup = '',
      'Fresh target unexpectedly created a pre-restore backup.');
    ProfileStore := TFpcProfileStore.Create(
      IncludeTrailingPathDelimiter(TargetDirectory) + 'profiles.json');
    Check(ProfileStore.Load(LoadedProfiles, ErrorMessage),
      'Restored profiles did not load: ' + ErrorMessage);
    Check((Length(LoadedProfiles) = 1) and
      (LoadedProfiles[0].Password = '') and
      (LoadedProfiles[0].Uuid = '') and
      (LoadedProfiles[0].RawConfig = '') and
      (not LoadedProfiles[0].Enabled),
      'Backup retained credentials, raw config, or an enabled profile.');
    Check(Pos('credentials omitted', LoadedProfiles[0].UnsupportedReason) > 0,
      'Sanitized profile does not explain why it is disabled.');
    SubscriptionStore := TFpcSubscriptionStore.Create(
      IncludeTrailingPathDelimiter(TargetDirectory) + 'subscriptions.json');
    try
      Check(SubscriptionStore.Load(Subscriptions, ErrorMessage),
        'Restored subscriptions did not load: ' + ErrorMessage);
    finally
      SubscriptionStore.Free;
    end;
    Check((Length(Subscriptions) = 1) and (Subscriptions[0].Url = '') and
      (Subscriptions[0].UserAgent = '') and (not Subscriptions[0].Enabled),
      'Backup retained a subscription URL or request metadata.');
    ProviderStore := TFpcCoreProviderStore.Create(
      IncludeTrailingPathDelimiter(TargetDirectory) + 'providers.json');
    Check(ProviderStore.Load(LoadedProviders, ErrorMessage),
      'Restored providers did not load: ' + ErrorMessage);
    Check((Length(LoadedProviders) = 2) and
      (LoadedProviders[0].ProviderId = ProviderExternalMihomo),
      'Provider definition was not restored.');
    Check((LoadedProviders[0].ExecutablePath = '') and
      (LoadedProviders[0].WorkingDirectory = '') and
      (LoadedProviders[0].AssetDirectory = '') and
      (LoadedProviders[0].Sha256 = ''),
      'Machine-specific provider fields entered the backup.');
    Check((Pos('C:\machine-only', LoadedProviders[1].DisplayName) = 0) and
      (Pos('C:\machine-only', LoadedProviders[1].RunArguments[1]) = 0),
      'Absolute path in a custom provider definition entered the backup.');

    LoadedProfiles[0].Name := 'mutated after first restore';
    Check(ProfileStore.Save(LoadedProfiles, ErrorMessage),
      'Could not mutate restore target: ' + ErrorMessage);
    Check(RestoreZaryaBackup(BackupFile, TargetDirectory, PreRestoreBackup,
      ErrorMessage), 'Second backup restore failed: ' + ErrorMessage);
    Check((PreRestoreBackup <> '') and FileExists(PreRestoreBackup),
      'Pre-restore backup was not created for an existing target.');
    Check(ProfileStore.Load(LoadedProfiles, ErrorMessage),
      'Twice-restored profiles did not load: ' + ErrorMessage);
    Check(LoadedProfiles[0].Name = 'private profile name',
      'Second restore did not replace the staged profile atomically.');
    ProfileStore := nil;
    ProviderStore := nil;
  finally
    RemoveTree(Root);
  end;
  WriteLn('Backup/restore and redacted diagnostics: PASS');
end.
