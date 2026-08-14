unit ZaryaSubscriptionService;

{$mode objfpc}{$H+}

interface

uses
  ZaryaProfile, ZaryaSubscription;

function MergeSubscriptionProfiles(var ASubscription: TZaryaSubscription;
  const AIncomingProfiles: TZaryaProfiles; var AProfiles: TZaryaProfiles;
  out AStats: TZaryaSubscriptionUpdateStats; out AError: string): Boolean;
function CountActiveSubscriptionProfiles(const AProfiles: TZaryaProfiles;
  const ASubscriptionId: string): Integer;

implementation

uses
  SysUtils, DateUtils, ZaryaCoreProvider;

function CountActiveSubscriptionProfiles(const AProfiles: TZaryaProfiles;
  const ASubscriptionId: string): Integer;
var
  I: Integer;
begin
  Result := 0;
  for I := 0 to High(AProfiles) do
    if SameText(AProfiles[I].SourceType, 'subscription') and
      (AProfiles[I].SubscriptionId = ASubscriptionId) and
      not AProfiles[I].DeletedBySubscriptionUpdate then
      Inc(Result);
end;

function FindProfileBySourceKey(const AProfiles: TZaryaProfiles;
  const ASubscriptionId, ASourceKey: string): Integer;
var
  I: Integer;
begin
  for I := 0 to High(AProfiles) do
    if SameText(AProfiles[I].SourceType, 'subscription') and
      (AProfiles[I].SubscriptionId = ASubscriptionId) and
      (AProfiles[I].SourceKey = ASourceKey) then
      Exit(I);
  Result := -1;
end;

function SourceKeyWasSeen(const AKeys: array of string;
  const AKey: string): Boolean;
var
  I: Integer;
begin
  for I := 0 to High(AKeys) do
    if AKeys[I] = AKey then
      Exit(True);
  Result := False;
end;

function UtcIsoNow: string;
begin
  Result := FormatDateTime('yyyy-mm-dd"T"hh:nn:ss"Z"',
    LocalTimeToUniversal(Now));
end;

function MergeSubscriptionProfiles(var ASubscription: TZaryaSubscription;
  const AIncomingProfiles: TZaryaProfiles; var AProfiles: TZaryaProfiles;
  out AStats: TZaryaSubscriptionUpdateStats; out AError: string): Boolean;
var
  Incoming: TZaryaProfile;
  ExistingId: string;
  ExistingProviderId: string;
  ExistingEnabled: Boolean;
  SeenKeys: array of string;
  SourceKey: string;
  NowText: string;
  ExistingIndex: Integer;
  ProfileIndex: Integer;
  SeenIndex: Integer;
begin
  AStats := Default(TZaryaSubscriptionUpdateStats);
  AError := '';
  if Trim(ASubscription.Id) = '' then
  begin
    AError := 'У подписки отсутствует id.';
    Exit(False);
  end;
  if Length(AIncomingProfiles) = 0 then
  begin
    AError := 'Обновление не содержит профилей.';
    Exit(False);
  end;
  NowText := UtcIsoNow;
  AStats.ParsedProfiles := Length(AIncomingProfiles);
  SetLength(SeenKeys, 0);
  for ProfileIndex := 0 to High(AIncomingProfiles) do
  begin
    Incoming := AIncomingProfiles[ProfileIndex];
    Incoming.SourceType := 'subscription';
    Incoming.Source := 'Подписка';
    Incoming.SubscriptionId := ASubscription.Id;
    Incoming.SubscriptionName := ASubscription.Name;
    Incoming.LastSeenAt := NowText;
    Incoming.DeletedBySubscriptionUpdate := False;
    Incoming.SourceKey := ComputeProfileSourceKey(Incoming);
    if Incoming.PreferredProviderId = '' then
      Incoming.PreferredProviderId := DefaultProviderForProtocol(
        Incoming.ProtocolName);
    SourceKey := Incoming.SourceKey;
    if SourceKeyWasSeen(SeenKeys, SourceKey) then
      Continue;
    SeenIndex := Length(SeenKeys);
    SetLength(SeenKeys, SeenIndex + 1);
    SeenKeys[SeenIndex] := SourceKey;
    ExistingIndex := FindProfileBySourceKey(AProfiles, ASubscription.Id,
      SourceKey);
    if ExistingIndex >= 0 then
    begin
      ExistingId := AProfiles[ExistingIndex].Id;
      ExistingProviderId := AProfiles[ExistingIndex].PreferredProviderId;
      ExistingEnabled := AProfiles[ExistingIndex].Enabled;
      Incoming.Id := ExistingId;
      if ExistingProviderId <> '' then
        Incoming.PreferredProviderId := ExistingProviderId;
      Incoming.Enabled := ExistingEnabled;
      AProfiles[ExistingIndex] := Incoming;
      Inc(AStats.UpdatedProfiles);
    end
    else
    begin
      ExistingIndex := Length(AProfiles);
      SetLength(AProfiles, ExistingIndex + 1);
      AProfiles[ExistingIndex] := Incoming;
      Inc(AStats.AddedProfiles);
    end;
  end;
  for ProfileIndex := 0 to High(AProfiles) do
    if SameText(AProfiles[ProfileIndex].SourceType, 'subscription') and
      (AProfiles[ProfileIndex].SubscriptionId = ASubscription.Id) and
      not SourceKeyWasSeen(SeenKeys, AProfiles[ProfileIndex].SourceKey) and
      not AProfiles[ProfileIndex].DeletedBySubscriptionUpdate then
    begin
      AProfiles[ProfileIndex].DeletedBySubscriptionUpdate := True;
      Inc(AStats.MarkedMissingProfiles);
    end;
  ASubscription.LastUpdatedAt := NowText;
  ASubscription.LastStatus := ssSuccess;
  ASubscription.LastError := '';
  ASubscription.ProfileCount := CountActiveSubscriptionProfiles(AProfiles,
    ASubscription.Id);
  Result := True;
end;

end.
