unit ZaryaDns;

{$mode objfpc}{$H+}

interface

uses
  SysUtils;

type
  TZaryaDnsMode = (dmSystem, dmSecureRemote, dmChinaDirectGlobalRemote,
    dmCustom);
  TZaryaDnsQueryStrategy = (dqsSystemDefault, dqsUseIp, dqsUseIpv4,
    dqsUseIpv6);
  TZaryaDnsServerKind = (dskPlain, dskDoh, dskLocal, dskFakeDns);
  TZaryaDnsStringArray = array of string;

  TZaryaDnsHost = record
    Host: string;
    Address: string;
  end;
  TZaryaDnsHosts = array of TZaryaDnsHost;

  TZaryaDnsServer = record
    Id: string;
    Enabled: Boolean;
    Kind: TZaryaDnsServerKind;
    Address: string;
    Port: Integer;
    QueryStrategy: string;
    TimeoutMs: Integer;
    Tag: string;
    SkipFallback: Boolean;
    Note: string;
    Domains: TZaryaDnsStringArray;
    ExpectIps: TZaryaDnsStringArray;
  end;
  TZaryaDnsServers = array of TZaryaDnsServer;

  TZaryaDnsProfile = record
    Id: string;
    Name: string;
    Mode: TZaryaDnsMode;
    Enabled: Boolean;
    IsBuiltIn: Boolean;
    QueryStrategy: TZaryaDnsQueryStrategy;
    DisableCache: Boolean;
    DisableFallback: Boolean;
    DisableFallbackIfMatch: Boolean;
    Hosts: TZaryaDnsHosts;
    Servers: TZaryaDnsServers;
    CreatedAt: string;
    UpdatedAt: string;
  end;
  TZaryaDnsProfiles = array of TZaryaDnsProfile;

const
  DnsSystemId = 'builtin-dns-system';
  DnsSecureRemoteId = 'builtin-dns-secure-remote';
  DnsChinaDirectGlobalRemoteId = 'builtin-dns-china-direct-global-remote';
  DnsCustomTemplateId = 'builtin-dns-custom-template';

function DnsModeToString(const AValue: TZaryaDnsMode): string;
function DnsModeFromString(const AValue: string): TZaryaDnsMode;
function DnsQueryStrategyToString(const AValue: TZaryaDnsQueryStrategy): string;
function DnsQueryStrategyFromString(const AValue: string): TZaryaDnsQueryStrategy;
function DnsQueryStrategyToXray(const AValue: TZaryaDnsQueryStrategy): string;
function DnsServerKindToString(const AValue: TZaryaDnsServerKind): string;
function DnsServerKindFromString(const AValue: string): TZaryaDnsServerKind;
function NewDnsServer: TZaryaDnsServer;
function CreateBuiltInDnsProfiles: TZaryaDnsProfiles;
function BuiltInSystemDns: TZaryaDnsProfile;
function IsDefaultDns(const AProfile: TZaryaDnsProfile): Boolean;
function ValidateDnsProfile(const AProfile: TZaryaDnsProfile;
  out AError: string): Boolean;
function DnsUsesGeoData(const AProfile: TZaryaDnsProfile): Boolean;

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

function StringArray(const AValues: array of string): TZaryaDnsStringArray;
var
  I: Integer;
begin
  Result := nil;
  SetLength(Result, Length(AValues));
  for I := 0 to High(AValues) do Result[I] := AValues[I];
end;

function DnsModeToString(const AValue: TZaryaDnsMode): string;
begin
  case AValue of
    dmSystem: Result := 'system';
    dmSecureRemote: Result := 'secure-remote';
    dmChinaDirectGlobalRemote: Result := 'china-direct-global-remote';
  else
    Result := 'custom';
  end;
end;

function DnsModeFromString(const AValue: string): TZaryaDnsMode;
begin
  if SameText(Trim(AValue), 'system') then Result := dmSystem
  else if SameText(Trim(AValue), 'secure-remote') then Result := dmSecureRemote
  else if SameText(Trim(AValue), 'china-direct-global-remote') then
    Result := dmChinaDirectGlobalRemote
  else Result := dmCustom;
end;

function DnsQueryStrategyToString(const AValue: TZaryaDnsQueryStrategy): string;
begin
  case AValue of
    dqsUseIp: Result := 'use-ip';
    dqsUseIpv4: Result := 'use-ipv4';
    dqsUseIpv6: Result := 'use-ipv6';
  else
    Result := 'system-default';
  end;
end;

function DnsQueryStrategyFromString(const AValue: string): TZaryaDnsQueryStrategy;
begin
  if SameText(Trim(AValue), 'use-ip') then Result := dqsUseIp
  else if SameText(Trim(AValue), 'use-ipv4') then Result := dqsUseIpv4
  else if SameText(Trim(AValue), 'use-ipv6') then Result := dqsUseIpv6
  else Result := dqsSystemDefault;
end;

function DnsQueryStrategyToXray(const AValue: TZaryaDnsQueryStrategy): string;
begin
  case AValue of
    dqsUseIp: Result := 'UseIP';
    dqsUseIpv4: Result := 'UseIPv4';
    dqsUseIpv6: Result := 'UseIPv6';
  else
    Result := '';
  end;
end;

function DnsServerKindToString(const AValue: TZaryaDnsServerKind): string;
begin
  case AValue of
    dskDoh: Result := 'doh';
    dskLocal: Result := 'local';
    dskFakeDns: Result := 'fakedns';
  else
    Result := 'plain';
  end;
end;

function DnsServerKindFromString(const AValue: string): TZaryaDnsServerKind;
begin
  if SameText(Trim(AValue), 'doh') then Result := dskDoh
  else if SameText(Trim(AValue), 'local') then Result := dskLocal
  else if SameText(Trim(AValue), 'fakedns') then Result := dskFakeDns
  else Result := dskPlain;
end;

function NewDnsServer: TZaryaDnsServer;
begin
  Result := Default(TZaryaDnsServer);
  Result.Id := NewId;
  Result.Enabled := True;
  Result.Kind := dskPlain;
end;

function MakeServer(const AAddress: string; const AKind: TZaryaDnsServerKind;
  const ADomains, AExpectIps: array of string): TZaryaDnsServer;
begin
  Result := NewDnsServer;
  Result.Address := AAddress;
  Result.Kind := AKind;
  Result.Domains := StringArray(ADomains);
  Result.ExpectIps := StringArray(AExpectIps);
end;

function MakeBuiltIn(const AId, AName: string; const AMode: TZaryaDnsMode;
  const AStrategy: TZaryaDnsQueryStrategy;
  const AServers: TZaryaDnsServers): TZaryaDnsProfile;
begin
  Result := Default(TZaryaDnsProfile);
  Result.Id := AId;
  Result.Name := AName;
  Result.Mode := AMode;
  Result.Enabled := True;
  Result.IsBuiltIn := True;
  Result.QueryStrategy := AStrategy;
  Result.Servers := AServers;
  Result.CreatedAt := NowIso;
  Result.UpdatedAt := Result.CreatedAt;
end;

function BuiltInSystemDns: TZaryaDnsProfile;
begin
  Result := MakeBuiltIn(DnsSystemId, 'System DNS', dmSystem,
    dqsSystemDefault, nil);
end;

function CreateBuiltInDnsProfiles: TZaryaDnsProfiles;
var
  Servers: TZaryaDnsServers;
begin
  Result := nil;
  SetLength(Result, 4);
  Result[0] := BuiltInSystemDns;
  SetLength(Servers, 2);
  Servers[0] := MakeServer('https://cloudflare-dns.com/dns-query', dskDoh, [], []);
  Servers[1] := MakeServer('https://dns.google/dns-query', dskDoh, [], []);
  Result[1] := MakeBuiltIn(DnsSecureRemoteId, 'Secure Remote DNS',
    dmSecureRemote, dqsUseIp, Servers);
  SetLength(Servers, 3);
  Servers[0] := MakeServer('223.5.5.5', dskPlain,
    ['geosite:cn'], ['geoip:cn']);
  Servers[1] := MakeServer('https://cloudflare-dns.com/dns-query', dskDoh,
    ['geosite:geolocation-!cn'], []);
  Servers[2] := MakeServer('https://dns.google/dns-query', dskDoh, [], []);
  Result[2] := MakeBuiltIn(DnsChinaDirectGlobalRemoteId,
    'China Direct / Global Remote', dmChinaDirectGlobalRemote,
    dqsUseIp, Servers);
  Result[3] := MakeBuiltIn(DnsCustomTemplateId, 'Custom', dmCustom,
    dqsSystemDefault, nil);
end;

function IsDefaultDns(const AProfile: TZaryaDnsProfile): Boolean;
begin
  Result := SameText(AProfile.Id, DnsSystemId) or
    ((AProfile.Mode = dmSystem) and (Length(AProfile.Servers) = 0));
end;

function ValidServerStrategy(const AValue: string): Boolean;
begin
  Result := (Trim(AValue) = '') or SameText(AValue, 'UseIP') or
    SameText(AValue, 'UseIPv4') or SameText(AValue, 'UseIPv6');
end;

function ValidateDnsProfile(const AProfile: TZaryaDnsProfile;
  out AError: string): Boolean;
var
  I, J: Integer;
begin
  AError := '';
  if Trim(AProfile.Id) = '' then AError := 'DNS profile id is required.'
  else if Trim(AProfile.Name) = '' then AError := 'DNS profile name is required.';
  if AError <> '' then Exit(False);

  for I := 0 to High(AProfile.Hosts) do
  begin
    if (Trim(AProfile.Hosts[I].Host) = '') or
      (Trim(AProfile.Hosts[I].Address) = '') then
    begin
      AError := Format('DNS host #%d is incomplete.', [I + 1]);
      Exit(False);
    end;
    for J := 0 to I - 1 do
      if SameText(AProfile.Hosts[J].Host, AProfile.Hosts[I].Host) then
      begin
        AError := 'Duplicate DNS host: ' + AProfile.Hosts[I].Host;
        Exit(False);
      end;
  end;
  for I := 0 to High(AProfile.Servers) do
  begin
    if Trim(AProfile.Servers[I].Id) = '' then
    begin
      AError := Format('DNS server #%d has no id.', [I + 1]);
      Exit(False);
    end;
    for J := 0 to I - 1 do
      if SameText(AProfile.Servers[J].Id, AProfile.Servers[I].Id) then
      begin
        AError := 'Duplicate DNS server id: ' + AProfile.Servers[I].Id;
        Exit(False);
      end;
    if AProfile.Servers[I].Enabled and
      (AProfile.Servers[I].Kind <> dskFakeDns) and
      (Trim(AProfile.Servers[I].Address) = '') then
    begin
      AError := Format('Enabled DNS server #%d has no address.', [I + 1]);
      Exit(False);
    end;
    if (AProfile.Servers[I].Port < 0) or
      (AProfile.Servers[I].Port > 65535) then
    begin
      AError := Format('DNS server #%d has an invalid port.', [I + 1]);
      Exit(False);
    end;
    if (AProfile.Servers[I].TimeoutMs < 0) or
      (AProfile.Servers[I].TimeoutMs > 120000) then
    begin
      AError := Format('DNS server #%d has an invalid timeout.', [I + 1]);
      Exit(False);
    end;
    if not ValidServerStrategy(AProfile.Servers[I].QueryStrategy) then
    begin
      AError := Format('DNS server #%d has an invalid query strategy.', [I + 1]);
      Exit(False);
    end;
  end;
  Result := True;
end;

function IsGeoReference(const AValue: string): Boolean;
var
  Value: string;
begin
  Value := LowerCase(Trim(AValue));
  Result := (Pos('geoip:', Value) = 1) or (Pos('geosite:', Value) = 1) or
    (Pos('ext:', Value) = 1);
end;

function DnsUsesGeoData(const AProfile: TZaryaDnsProfile): Boolean;
var
  Server: TZaryaDnsServer;
  Value: string;
begin
  for Server in AProfile.Servers do
    if Server.Enabled then
    begin
      for Value in Server.Domains do if IsGeoReference(Value) then Exit(True);
      for Value in Server.ExpectIps do if IsGeoReference(Value) then Exit(True);
    end;
  Result := False;
end;

end.
