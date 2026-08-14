unit ZaryaSubscription;

{$mode objfpc}{$H+}

interface

uses
  SysUtils;

type
  TZaryaSubscriptionStatus = (ssNeverUpdated, ssUpdating, ssSuccess,
    ssFailed, ssDisabled);

  TZaryaSubscription = record
    Id: string;
    Name: string;
    Url: string;
    Enabled: Boolean;
    LastUpdatedAt: string;
    LastStatus: TZaryaSubscriptionStatus;
    LastError: string;
    ProfileCount: Integer;
    UserAgent: string;
    Remarks: string;
  end;

  TZaryaSubscriptions = array of TZaryaSubscription;

  TZaryaSubscriptionUpdateStats = record
    ParsedProfiles: Integer;
    AddedProfiles: Integer;
    UpdatedProfiles: Integer;
    MarkedMissingProfiles: Integer;
    SkippedLines: Integer;
  end;

function CreateEmptySubscription: TZaryaSubscription;
function SubscriptionStatusToString(const AStatus: TZaryaSubscriptionStatus): string;
function SubscriptionStatusFromString(const AValue: string): TZaryaSubscriptionStatus;
function SubscriptionStatusDisplayName(const AStatus: TZaryaSubscriptionStatus): string;

implementation

function NewSubscriptionId: string;
var
  Value: TGuid;
begin
  if CreateGUID(Value) = 0 then
  begin
    Result := LowerCase(GUIDToString(Value));
    if (Length(Result) >= 2) and (Result[1] = '{') then
      Result := Copy(Result, 2, Length(Result) - 2);
  end
  else
    Result := FormatDateTime('yyyymmddhhnnsszzz', Now) + '-' +
      IntToHex(Random(MaxInt), 8);
end;

function CreateEmptySubscription: TZaryaSubscription;
begin
  Result := Default(TZaryaSubscription);
  Result.Id := NewSubscriptionId;
  Result.Name := 'Новая подписка';
  Result.Enabled := True;
  Result.LastStatus := ssNeverUpdated;
end;

function SubscriptionStatusToString(
  const AStatus: TZaryaSubscriptionStatus): string;
begin
  case AStatus of
    ssUpdating: Result := 'updating';
    ssSuccess: Result := 'success';
    ssFailed: Result := 'failed';
    ssDisabled: Result := 'disabled';
  else
    Result := 'never_updated';
  end;
end;

function SubscriptionStatusFromString(
  const AValue: string): TZaryaSubscriptionStatus;
begin
  if SameText(AValue, 'updating') then Result := ssUpdating
  else if SameText(AValue, 'success') then Result := ssSuccess
  else if SameText(AValue, 'failed') then Result := ssFailed
  else if SameText(AValue, 'disabled') then Result := ssDisabled
  else Result := ssNeverUpdated;
end;

function SubscriptionStatusDisplayName(
  const AStatus: TZaryaSubscriptionStatus): string;
begin
  case AStatus of
    ssUpdating: Result := 'Обновляется';
    ssSuccess: Result := 'Успешно';
    ssFailed: Result := 'Ошибка';
    ssDisabled: Result := 'Отключена';
  else
    Result := 'Не обновлялась';
  end;
end;

end.
