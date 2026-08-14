program ProfileStoreTest;

{$mode objfpc}{$H+}

uses
  SysUtils, ZaryaProfile, ZaryaProfileStore, FpcProfileStore;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

var
  FileName: string;
  Store: IZaryaProfileStore;
  SourceProfiles: TZaryaProfiles;
  LoadedProfiles: TZaryaProfiles;
  ErrorMessage: string;
begin
  Randomize;
  FileName := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-lcl-profile-store-' + IntToHex(Random(MaxInt), 8) + '.json';
  Store := TFpcProfileStore.Create(FileName);
  try
    SourceProfiles := CreateDemoProfiles;
    SourceProfiles[0].Name := 'Тестовый профиль «Амстердам»';
    SourceProfiles[0].Host := 'xn--e1afmkfd.example.invalid';
    SourceProfiles[0].Port := 8443;
    SourceProfiles[0].Security := 'reality';
    SourceProfiles[0].Network := 'tcp';
    SourceProfiles[0].ServerName := 'example.com';
    SourceProfiles[0].PublicKey := 'test-public-key';
    SourceProfiles[0].ShortId := 'a1b2c3d4';
    SourceProfiles[0].Flow := 'xtls-rprx-vision';
    SourceProfiles[0].Password := 'fixture-password';
    SourceProfiles[0].Method := 'aes-128-gcm';
    SourceProfiles[0].SubscriptionId := 'subscription-fixture';
    SourceProfiles[0].SubscriptionName := 'Fixture subscription';
    SourceProfiles[0].LastTcpPingMs := 37;
    SourceProfiles[0].LastRealDelayMs := 64;
    SourceProfiles[0].LastTestStatus := 'success';
    SourceProfiles[0].LastTestedAt := '2026-08-14T00:00:00Z';
    SourceProfiles[0].Enabled := False;
    SourceProfiles[0].PreferredProviderId := 'external.mihomo';
    SourceProfiles[0].RawConfigFormat := 'mihomo-yaml';
    SourceProfiles[0].ReadinessHost := '127.0.0.1';
    SourceProfiles[0].ReadinessPort := 17890;

    Check(Store.Save(SourceProfiles, ErrorMessage), 'Save failed: ' + ErrorMessage);
    Check(Store.Load(LoadedProfiles, ErrorMessage), 'Load failed: ' + ErrorMessage);
    Check(Length(LoadedProfiles) = Length(SourceProfiles), 'Profile count mismatch.');
    Check(LoadedProfiles[0].Name = SourceProfiles[0].Name, 'Unicode name mismatch.');
    Check(LoadedProfiles[0].Host = SourceProfiles[0].Host, 'Host mismatch.');
    Check(LoadedProfiles[0].Port = 8443, 'Port mismatch.');
    Check(LoadedProfiles[0].Security = 'reality', 'Security mismatch.');
    Check(LoadedProfiles[0].PublicKey = 'test-public-key', 'Public key mismatch.');
    Check(LoadedProfiles[0].Flow = 'xtls-rprx-vision', 'Flow mismatch.');
    Check(LoadedProfiles[0].Password = 'fixture-password',
      'Password mismatch.');
    Check(LoadedProfiles[0].Method = 'aes-128-gcm', 'Method mismatch.');
    Check(LoadedProfiles[0].SubscriptionId = 'subscription-fixture',
      'Subscription id mismatch.');
    Check(LoadedProfiles[0].LastTcpPingMs = 37,
      'TCP ping result mismatch.');
    Check(LoadedProfiles[0].LastRealDelayMs = 64,
      'Real delay result mismatch.');
    Check(LoadedProfiles[0].LastTestStatus = 'success',
      'Test status mismatch.');
    Check(not LoadedProfiles[0].Enabled, 'Enabled flag mismatch.');
    Check(LoadedProfiles[0].PreferredProviderId = 'external.mihomo',
      'Preferred provider mismatch.');
    Check(LoadedProfiles[0].RawConfigFormat = 'mihomo-yaml',
      'Raw config format mismatch.');
    Check(LoadedProfiles[0].ReadinessPort = 17890,
      'Readiness port mismatch.');
    Check(ProfileEndpoint(LoadedProfiles[0]) =
      'xn--e1afmkfd.example.invalid:8443', 'Endpoint mismatch.');
    SetLength(SourceProfiles, 0);
    Check(Store.Save(SourceProfiles, ErrorMessage), 'Empty save failed: ' + ErrorMessage);
    Check(Store.Load(LoadedProfiles, ErrorMessage), 'Empty load failed: ' + ErrorMessage);
    Check(Length(LoadedProfiles) = 0, 'Empty profile list did not persist.');
    WriteLn('ProfileStore round-trip: PASS');
  finally
    Store := nil;
    if FileExists(FileName) then
      DeleteFile(FileName);
    if FileExists(FileName + '.bak') then
      DeleteFile(FileName + '.bak');
    if FileExists(FileName + '.tmp') then
      DeleteFile(FileName + '.tmp');
  end;
end.
