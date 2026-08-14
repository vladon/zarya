program SubscriptionTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, base64, ZaryaProfile, ZaryaSubscription,
  FpcSubscriptionStore, ZaryaSubscriptionParser, ZaryaSubscriptionService,
  WindowsSubscriptionDownloader;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

function TempDirectory: string;
begin
  Result := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-lcl-subscription-' + IntToHex(Random(MaxInt), 8);
  if not ForceDirectories(Result) then
    raise Exception.Create('Could not create subscription test directory.');
end;

procedure DeleteDirectoryFiles(const ADirectory: string);
var
  Search: TSearchRec;
begin
  if FindFirst(IncludeTrailingPathDelimiter(ADirectory) + '*', faAnyFile,
    Search) = 0 then
  try
    repeat
      if (Search.Name <> '.') and (Search.Name <> '..') then
        DeleteFile(IncludeTrailingPathDelimiter(ADirectory) + Search.Name);
    until FindNext(Search) <> 0;
  finally
    FindClose(Search);
  end;
  RemoveDir(ADirectory);
end;

var
  DirectoryName: string;
  Store: TFpcSubscriptionStore;
  Subscriptions: TZaryaSubscriptions;
  Loaded: TZaryaSubscriptions;
  Subscription: TZaryaSubscription;
  Parsed: TZaryaSubscriptionParseResult;
  Profiles: TZaryaProfiles;
  Incoming: TZaryaProfiles;
  Stats: TZaryaSubscriptionUpdateStats;
  ErrorMessage: string;
  ExistingId: string;
  Download: TZaryaSubscriptionDownloadResult;
  Content: RawByteString;
begin
  Randomize;
  DirectoryName := TempDirectory;
  try
    Store := TFpcSubscriptionStore.Create(
      IncludeTrailingPathDelimiter(DirectoryName) + 'subscriptions.json');
    try
      SetLength(Subscriptions, 1);
      Subscriptions[0] := CreateEmptySubscription;
      Subscriptions[0].Name := 'Fixture';
      Subscriptions[0].Url := 'https://example.invalid/subscription';
      Subscriptions[0].UserAgent := 'Fixture/1';
      Subscriptions[0].Remarks := 'Local test';
      Check(Store.Save(Subscriptions, ErrorMessage),
        'Subscription save failed: ' + ErrorMessage);
      Check(Store.Load(Loaded, ErrorMessage),
        'Subscription load failed: ' + ErrorMessage);
      Check(Length(Loaded) = 1, 'Subscription count changed after round-trip.');
      Check(Loaded[0].Url = Subscriptions[0].Url,
        'Subscription URL changed after round-trip.');
      Check(Loaded[0].UserAgent = 'Fixture/1',
        'Subscription user agent changed after round-trip.');
    finally
      Store.Free;
    end;

    Content := 'vless://11111111-1111-1111-1111-111111111111@example.invalid:443' +
      '?security=tls#One' + LineEnding +
      'trojan://secret@example.invalid:8443?security=tls#Two' + LineEnding +
      'unsupported://value';
    Parsed := ParseSubscriptionContent(Content);
    Check(Parsed.Success, 'Plain subscription parsing failed: ' +
      Parsed.ErrorMessage);
    Check(Length(Parsed.Profiles) = 2, 'Plain subscription profile count mismatch.');
    Check(Parsed.SkippedLines = 1, 'Unsupported subscription line was not counted.');

    Parsed := ParseSubscriptionContent(RawByteString(
      EncodeStringBase64(string(Content))));
    Check(Parsed.Success, 'Base64 subscription parsing failed: ' +
      Parsed.ErrorMessage);
    Check(Length(Parsed.Profiles) = 2,
      'Base64 subscription profile count mismatch.');

    Subscription := Subscriptions[0];
    SetLength(Profiles, 1);
    Profiles[0] := CreateEmptyProfile;
    Profiles[0].Name := 'Manual profile';
    Profiles[0].Host := 'manual.example.invalid';
    Profiles[0].Uuid := '22222222-2222-2222-2222-222222222222';
    Incoming := Parsed.Profiles;
    Check(MergeSubscriptionProfiles(Subscription, Incoming, Profiles, Stats,
      ErrorMessage), 'Subscription merge failed: ' + ErrorMessage);
    Check(Length(Profiles) = 3, 'Subscription merge changed manual profiles.');
    Check(Stats.AddedProfiles = 2, 'Subscription added count mismatch.');
    ExistingId := Profiles[1].Id;
    Profiles[1].PreferredProviderId := 'external.mihomo';
    Profiles[1].Enabled := False;
    Incoming[0].Name := 'Updated name';
    SetLength(Incoming, 1);
    Check(MergeSubscriptionProfiles(Subscription, Incoming, Profiles, Stats,
      ErrorMessage), 'Second subscription merge failed: ' + ErrorMessage);
    Check(Stats.UpdatedProfiles = 1, 'Subscription update count mismatch.');
    Check(Stats.MarkedMissingProfiles = 1,
      'Missing subscription profile was not marked.');
    Check(Profiles[1].Id = ExistingId,
      'Subscription update replaced the stable profile id.');
    Check(Profiles[1].PreferredProviderId = 'external.mihomo',
      'Subscription update silently changed the selected provider.');
    Check(not Profiles[1].Enabled,
      'Subscription update silently re-enabled a profile.');
    Check(Subscription.ProfileCount = 1,
      'Subscription active profile count mismatch.');

    SetLength(Incoming, 0);
    Check(not MergeSubscriptionProfiles(Subscription, Incoming, Profiles,
      Stats, ErrorMessage), 'Empty update silently removed old profiles.');
    Check(Length(Profiles) = 3,
      'Failed subscription update modified the old profile set.');

    Download := DownloadSubscriptionWinHttp('ftp://example.invalid/file',
      'Zarya-Test/1', 1000, nil, nil);
    Check(not Download.Success,
      'WinHTTP downloader silently accepted a non-HTTP URL.');

    WriteLn('Subscription store/parser/atomic merge: PASS');
  finally
    DeleteDirectoryFiles(DirectoryName);
  end;
end.
