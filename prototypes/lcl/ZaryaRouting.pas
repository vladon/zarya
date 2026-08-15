unit ZaryaRouting;

{$mode objfpc}{$H+}

interface

uses
  SysUtils;

type
  TZaryaRoutingMode = (rmProxyAll, rmBypassLan, rmBypassRu,
    rmBypassLanAndRu, rmCustom);
  TZaryaRoutingAction = (raProxy, raDirect, raBlock);
  TZaryaRoutingRuleType = (rrDomain, rrIp, rrPort, rrProtocol);
  TZaryaStringArray = array of string;

  TZaryaRoutingRule = record
    Id: string;
    Enabled: Boolean;
    RuleType: TZaryaRoutingRuleType;
    Action: TZaryaRoutingAction;
    Values: TZaryaStringArray;
    Note: string;
  end;

  TZaryaRoutingRules = array of TZaryaRoutingRule;

  TZaryaRoutingProfile = record
    Id: string;
    Name: string;
    Mode: TZaryaRoutingMode;
    Enabled: Boolean;
    DomainStrategy: string;
    Rules: TZaryaRoutingRules;
    CreatedAt: string;
    UpdatedAt: string;
    IsBuiltIn: Boolean;
  end;

  TZaryaRoutingProfiles = array of TZaryaRoutingProfile;

const
  RoutingProxyAllId = 'builtin-proxy-all';
  RoutingBypassLanId = 'builtin-bypass-lan';
  RoutingBypassRuId = 'builtin-bypass-ru';
  RoutingBypassLanAndRuId = 'builtin-bypass-lan-ru';
  RoutingCustomTemplateId = 'builtin-custom-template';

function RoutingModeToString(const AValue: TZaryaRoutingMode): string;
function RoutingModeFromString(const AValue: string): TZaryaRoutingMode;
function RoutingActionToString(const AValue: TZaryaRoutingAction): string;
function RoutingActionFromString(const AValue: string): TZaryaRoutingAction;
function RoutingRuleTypeToString(const AValue: TZaryaRoutingRuleType): string;
function RoutingRuleTypeFromString(const AValue: string): TZaryaRoutingRuleType;
function NewRoutingRule: TZaryaRoutingRule;
function CreateBuiltInRoutingProfiles: TZaryaRoutingProfiles;
function BuiltInProxyAllRouting: TZaryaRoutingProfile;
function IsDefaultRouting(const AProfile: TZaryaRoutingProfile): Boolean;
function ValidateRoutingProfile(const AProfile: TZaryaRoutingProfile;
  out AError: string): Boolean;
function RoutingUsesGeoData(const AProfile: TZaryaRoutingProfile): Boolean;

implementation

function NewId: string;
begin
  Result := FormatDateTime('yyyymmddhhnnsszzz', Now) + '-' +
    IntToHex(Random(MaxInt), 8);
end;

function NowIso: string;
begin
  Result := FormatDateTime('yyyy-mm-dd"T"hh:nn:ss', Now);
end;

function StringArray(const AValues: array of string): TZaryaStringArray;
var
  I: Integer;
begin
  Result := nil;
  SetLength(Result, Length(AValues));
  for I := 0 to High(AValues) do
    Result[I] := AValues[I];
end;

function RoutingModeToString(const AValue: TZaryaRoutingMode): string;
begin
  case AValue of
    rmProxyAll: Result := 'proxy_all';
    rmBypassLan: Result := 'bypass_lan';
    rmBypassRu: Result := 'bypass_ru';
    rmBypassLanAndRu: Result := 'bypass_lan_and_ru';
  else
    Result := 'custom';
  end;
end;

function RoutingModeFromString(const AValue: string): TZaryaRoutingMode;
begin
  if SameText(Trim(AValue), 'proxy_all') then Result := rmProxyAll
  else if SameText(Trim(AValue), 'bypass_lan') then Result := rmBypassLan
  else if SameText(Trim(AValue), 'bypass_ru') then Result := rmBypassRu
  else if SameText(Trim(AValue), 'bypass_lan_and_ru') then Result := rmBypassLanAndRu
  else Result := rmCustom;
end;

function RoutingActionToString(const AValue: TZaryaRoutingAction): string;
begin
  case AValue of
    raDirect: Result := 'direct';
    raBlock: Result := 'block';
  else
    Result := 'proxy';
  end;
end;

function RoutingActionFromString(const AValue: string): TZaryaRoutingAction;
begin
  if SameText(Trim(AValue), 'direct') then Result := raDirect
  else if SameText(Trim(AValue), 'block') then Result := raBlock
  else Result := raProxy;
end;

function RoutingRuleTypeToString(const AValue: TZaryaRoutingRuleType): string;
begin
  case AValue of
    rrIp: Result := 'ip';
    rrPort: Result := 'port';
    rrProtocol: Result := 'protocol';
  else
    Result := 'domain';
  end;
end;

function RoutingRuleTypeFromString(const AValue: string): TZaryaRoutingRuleType;
begin
  if SameText(Trim(AValue), 'ip') then Result := rrIp
  else if SameText(Trim(AValue), 'port') then Result := rrPort
  else if SameText(Trim(AValue), 'protocol') then Result := rrProtocol
  else Result := rrDomain;
end;

function NewRoutingRule: TZaryaRoutingRule;
begin
  Result := Default(TZaryaRoutingRule);
  Result.Id := NewId;
  Result.Enabled := True;
  Result.RuleType := rrDomain;
  Result.Action := raProxy;
end;

function MakeRule(const ARuleType: TZaryaRoutingRuleType;
  const AAction: TZaryaRoutingAction; const AValues: array of string): TZaryaRoutingRule;
begin
  Result := NewRoutingRule;
  Result.RuleType := ARuleType;
  Result.Action := AAction;
  Result.Values := StringArray(AValues);
end;

function MakeBuiltIn(const AId, AName: string; const AMode: TZaryaRoutingMode;
  const ARules: TZaryaRoutingRules): TZaryaRoutingProfile;
begin
  Result := Default(TZaryaRoutingProfile);
  Result.Id := AId;
  Result.Name := AName;
  Result.Mode := AMode;
  Result.Enabled := True;
  Result.DomainStrategy := 'AsIs';
  Result.Rules := ARules;
  Result.CreatedAt := NowIso;
  Result.UpdatedAt := Result.CreatedAt;
  Result.IsBuiltIn := True;
end;

function BuiltInProxyAllRouting: TZaryaRoutingProfile;
begin
  Result := MakeBuiltIn(RoutingProxyAllId, 'Proxy All', rmProxyAll, nil);
end;

function CreateBuiltInRoutingProfiles: TZaryaRoutingProfiles;
var
  Rules: TZaryaRoutingRules;
begin
  Result := nil;
  SetLength(Result, 5);
  Result[0] := BuiltInProxyAllRouting;
  SetLength(Rules, 2);
  Rules[0] := MakeRule(rrDomain, raDirect, ['geosite:private']);
  Rules[1] := MakeRule(rrIp, raDirect, ['geoip:private']);
  Result[1] := MakeBuiltIn(RoutingBypassLanId, 'Bypass LAN', rmBypassLan, Rules);
  SetLength(Rules, 2);
  Rules[0] := MakeRule(rrDomain, raDirect, ['geosite:ru']);
  Rules[1] := MakeRule(rrIp, raDirect, ['geoip:ru']);
  Result[2] := MakeBuiltIn(RoutingBypassRuId, 'Bypass RU', rmBypassRu, Rules);
  SetLength(Rules, 2);
  Rules[0] := MakeRule(rrDomain, raDirect,
    ['geosite:private', 'geosite:ru']);
  Rules[1] := MakeRule(rrIp, raDirect, ['geoip:private', 'geoip:ru']);
  Result[3] := MakeBuiltIn(RoutingBypassLanAndRuId,
    'Bypass LAN + RU', rmBypassLanAndRu, Rules);
  Result[4] := MakeBuiltIn(RoutingCustomTemplateId, 'Custom', rmCustom, nil);
end;

function IsDefaultRouting(const AProfile: TZaryaRoutingProfile): Boolean;
begin
  Result := SameText(AProfile.Id, RoutingProxyAllId) or
    ((AProfile.Mode = rmProxyAll) and (Length(AProfile.Rules) = 0));
end;

function ValidDomainStrategy(const AValue: string): Boolean;
begin
  Result := SameText(Trim(AValue), 'AsIs') or
    SameText(Trim(AValue), 'IPIfNonMatch') or
    SameText(Trim(AValue), 'IPOnDemand');
end;

function ValidateRoutingProfile(const AProfile: TZaryaRoutingProfile;
  out AError: string): Boolean;
var
  I, J: Integer;
  SeenIds: TStringArray;
  Value: string;
begin
  AError := '';
  if Trim(AProfile.Id) = '' then
    AError := 'Routing profile id is required.'
  else if Trim(AProfile.Name) = '' then
    AError := 'Routing profile name is required.'
  else if not ValidDomainStrategy(AProfile.DomainStrategy) then
    AError := 'Unsupported routing domainStrategy: ' + AProfile.DomainStrategy;
  if AError <> '' then Exit(False);

  SetLength(SeenIds, Length(AProfile.Rules));
  for I := 0 to High(AProfile.Rules) do
  begin
    if Trim(AProfile.Rules[I].Id) = '' then
    begin
      AError := Format('Routing rule #%d has no id.', [I + 1]);
      Exit(False);
    end;
    for J := 0 to I - 1 do
      if SameText(SeenIds[J], AProfile.Rules[I].Id) then
      begin
        AError := 'Duplicate routing rule id: ' + AProfile.Rules[I].Id;
        Exit(False);
      end;
    SeenIds[I] := AProfile.Rules[I].Id;
    if AProfile.Rules[I].Enabled and (Length(AProfile.Rules[I].Values) = 0) then
    begin
      AError := Format('Enabled routing rule #%d has no values.', [I + 1]);
      Exit(False);
    end;
    for Value in AProfile.Rules[I].Values do
      if Trim(Value) = '' then
      begin
        AError := Format('Routing rule #%d contains an empty value.', [I + 1]);
        Exit(False);
      end;
  end;
  Result := True;
end;

function RoutingUsesGeoData(const AProfile: TZaryaRoutingProfile): Boolean;
var
  Rule: TZaryaRoutingRule;
  Value: string;
  Normalized: string;
begin
  for Rule in AProfile.Rules do
    if Rule.Enabled then
      for Value in Rule.Values do
      begin
        Normalized := LowerCase(Trim(Value));
        if (Pos('geoip:', Normalized) = 1) or
          (Pos('geosite:', Normalized) = 1) or
          (Pos('ext:', Normalized) = 1) then
          Exit(True);
      end;
  Result := False;
end;

end.
