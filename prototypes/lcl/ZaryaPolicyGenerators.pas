unit ZaryaPolicyGenerators;

{$mode objfpc}{$H+}

interface

uses
  SysUtils, ZaryaRouting, ZaryaDns;

function ApplyXrayPolicies(const AConfig: string;
  const ARouting: TZaryaRoutingProfile; const ADns: TZaryaDnsProfile;
  const AUseRouting, AUseDns: Boolean; out AResult, AError: string): Boolean;
function ApplySingBoxPolicies(const AConfig: string;
  const ARouting: TZaryaRoutingProfile; const ADns: TZaryaDnsProfile;
  const AUseRouting, AUseDns: Boolean; out AResult, AError: string): Boolean;
function ApplyMihomoPolicies(const AConfig: string;
  const ARouting: TZaryaRoutingProfile; const ADns: TZaryaDnsProfile;
  const AUseRouting, AUseDns: Boolean; out AResult, AError: string): Boolean;
function ValidateStandalonePolicies(const ARouting: TZaryaRoutingProfile;
  const ADns: TZaryaDnsProfile; const AUseRouting,
  AUseDns: Boolean; out AError: string): Boolean;

implementation

uses
  Classes, fpjson, jsonparser;

function JsonStringArray(const AValues: TZaryaStringArray;
  const AStripPrefix: string = ''): TJSONArray;
var
  Value: string;
  Normalized: string;
begin
  Result := TJSONArray.Create;
  for Value in AValues do
  begin
    Normalized := Trim(Value);
    if (AStripPrefix <> '') and
      (Pos(LowerCase(AStripPrefix), LowerCase(Normalized)) = 1) then
      Delete(Normalized, 1, Length(AStripPrefix));
    if Normalized <> '' then Result.Add(Normalized);
  end;
end;

function DnsJsonStringArray(const AValues: TZaryaDnsStringArray): TJSONArray;
var
  Value: string;
begin
  Result := TJSONArray.Create;
  for Value in AValues do if Trim(Value) <> '' then Result.Add(Trim(Value));
end;

procedure ReplaceMember(const AObject: TJSONObject; const AName: string;
  const AValue: TJSONData);
var
  Index: Integer;
  Previous: TJSONData;
begin
  Index := AObject.IndexOfName(AName);
  if Index >= 0 then
  begin
    Previous := AObject.Extract(Index);
    Previous.Free;
  end;
  AObject.Add(AName, AValue);
end;

procedure RemoveMember(const AObject: TJSONObject; const AName: string);
var
  Index: Integer;
  Previous: TJSONData;
begin
  Index := AObject.IndexOfName(AName);
  if Index < 0 then Exit;
  Previous := AObject.Extract(Index);
  Previous.Free;
end;

function XrayFieldName(const AType: TZaryaRoutingRuleType): string;
begin
  case AType of
    rrIp: Result := 'ip';
    rrPort: Result := 'port';
    rrProtocol: Result := 'protocol';
  else
    Result := 'domain';
  end;
end;

function OutboundForAction(const AAction: TZaryaRoutingAction): string;
begin
  case AAction of
    raDirect: Result := 'direct';
    raBlock: Result := 'block';
  else
    Result := 'proxy';
  end;
end;

function BuildXrayRouting(const AProfile: TZaryaRoutingProfile;
  out AError: string): TJSONObject;
var
  Rules: TJSONArray;
  Item: TJSONObject;
  Action: TZaryaRoutingAction;
  Rule: TZaryaRoutingRule;
  ActionIndex: Integer;
begin
  Result := nil;
  if not ValidateRoutingProfile(AProfile, AError) then Exit;
  Result := TJSONObject.Create;
  Result.Add('domainStrategy', AProfile.DomainStrategy);
  Rules := TJSONArray.Create;
  Result.Add('rules', Rules);
  for ActionIndex := 0 to 2 do
  begin
    case ActionIndex of
      0: Action := raBlock;
      1: Action := raDirect;
    else
      Action := raProxy;
    end;
    for Rule in AProfile.Rules do
      if Rule.Enabled and (Rule.Action = Action) then
      begin
        Item := TJSONObject.Create;
        Item.Add('type', 'field');
        Item.Add(XrayFieldName(Rule.RuleType), JsonStringArray(Rule.Values));
        Item.Add('outboundTag', OutboundForAction(Rule.Action));
        Rules.Add(Item);
      end;
  end;
  Item := TJSONObject.Create;
  Item.Add('type', 'field');
  Item.Add('network', 'tcp,udp');
  Item.Add('outboundTag', 'proxy');
  Rules.Add(Item);
end;

function EffectiveDnsAddress(const AServer: TZaryaDnsServer): string;
begin
  Result := Trim(AServer.Address);
  if (AServer.Kind = dskPlain) and (AServer.Port > 0) and
    (Pos(':', Result) = 0) then
    Result := Result + ':' + IntToStr(AServer.Port);
end;

function BuildXrayDns(const AProfile: TZaryaDnsProfile;
  out AError: string): TJSONObject;
var
  Hosts: TJSONObject;
  Servers: TJSONArray;
  Item: TJSONObject;
  Host: TZaryaDnsHost;
  Server: TZaryaDnsServer;
  Strategy: string;
  Simple: Boolean;
begin
  Result := nil;
  if not ValidateDnsProfile(AProfile, AError) then Exit;
  if IsDefaultDns(AProfile) or not AProfile.Enabled then
    Exit(TJSONObject.Create);
  Result := TJSONObject.Create;
  if Length(AProfile.Hosts) > 0 then
  begin
    Hosts := TJSONObject.Create;
    for Host in AProfile.Hosts do Hosts.Add(Trim(Host.Host), Trim(Host.Address));
    Result.Add('hosts', Hosts);
  end;
  Servers := TJSONArray.Create;
  Result.Add('servers', Servers);
  for Server in AProfile.Servers do
    if Server.Enabled and (Server.Kind <> dskFakeDns) then
    begin
      Simple := (Length(Server.Domains) = 0) and
        (Length(Server.ExpectIps) = 0) and (Trim(Server.Tag) = '') and
        (Server.TimeoutMs = 0) and not Server.SkipFallback and
        (Trim(Server.QueryStrategy) = '');
      if Simple then
        Servers.Add(EffectiveDnsAddress(Server))
      else
      begin
        Item := TJSONObject.Create;
        Item.Add('address', EffectiveDnsAddress(Server));
        if Length(Server.Domains) > 0 then
          Item.Add('domains', DnsJsonStringArray(Server.Domains));
        if Length(Server.ExpectIps) > 0 then
          Item.Add('expectIPs', DnsJsonStringArray(Server.ExpectIps));
        if Trim(Server.Tag) <> '' then Item.Add('tag', Trim(Server.Tag));
        if Server.TimeoutMs > 0 then Item.Add('timeoutMs', Server.TimeoutMs);
        if Server.SkipFallback then Item.Add('skipFallback', True);
        if Trim(Server.QueryStrategy) <> '' then
          Item.Add('queryStrategy', Trim(Server.QueryStrategy));
        Servers.Add(Item);
      end;
    end;
  Strategy := DnsQueryStrategyToXray(AProfile.QueryStrategy);
  if Strategy <> '' then Result.Add('queryStrategy', Strategy);
  if AProfile.DisableCache then Result.Add('disableCache', True);
  if AProfile.DisableFallback then Result.Add('disableFallback', True);
  if AProfile.DisableFallbackIfMatch then
    Result.Add('disableFallbackIfMatch', True);
end;

function ParseRoot(const AConfig: string; out ARootData: TJSONData;
  out ARoot: TJSONObject; out AError: string): Boolean;
begin
  ARootData := nil;
  ARoot := nil;
  try
    ARootData := GetJSON(AConfig);
    if ARootData.JSONType <> jtObject then
      raise Exception.Create('Generated config root must be an object.');
    ARoot := TJSONObject(ARootData);
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      ARootData.Free;
      ARootData := nil;
      Result := False;
    end;
  end;
end;

function ApplyXrayPolicies(const AConfig: string;
  const ARouting: TZaryaRoutingProfile; const ADns: TZaryaDnsProfile;
  const AUseRouting, AUseDns: Boolean; out AResult, AError: string): Boolean;
var
  Data: TJSONData;
  Root: TJSONObject;
  Routing: TJSONObject;
  Dns: TJSONObject;
begin
  AResult := '';
  AError := '';
  if not ParseRoot(AConfig, Data, Root, AError) then Exit(False);
  try
    if AUseRouting then
    begin
      Routing := BuildXrayRouting(ARouting, AError);
      if not Assigned(Routing) then Exit(False);
      ReplaceMember(Root, 'routing', Routing);
    end;
    if AUseDns then
    begin
      Dns := BuildXrayDns(ADns, AError);
      if not Assigned(Dns) then Exit(False);
      if Dns.Count = 0 then Dns.Free
      else ReplaceMember(Root, 'dns', Dns);
    end;
    AResult := Root.AsJSON;
    Result := True;
  finally
    Data.Free;
  end;
end;

function SingBoxDomainRule(const ARule: TZaryaRoutingRule;
  const AOutbound: string; out AResult: TJSONObject;
  out AError: string): Boolean;
var
  Domain, Suffix, Keyword, Regex: TZaryaStringArray;
  Value, Normalized: string;
  I: Integer;

  procedure AddValue(var AValues: TZaryaStringArray; const AValue: string);
  begin
    SetLength(AValues, Length(AValues) + 1);
    AValues[High(AValues)] := AValue;
  end;

begin
  AResult := TJSONObject.Create;
  AResult.Add('outbound', AOutbound);
  Result := False;
  case ARule.RuleType of
    rrDomain:
      begin
        for Value in ARule.Values do
        begin
          Normalized := Trim(Value);
          if (Pos('geosite:', LowerCase(Normalized)) = 1) or
            (Pos('ext:', LowerCase(Normalized)) = 1) then
          begin
            AError := 'sing-box adapter cannot safely translate geosite/ext routing rule: ' + Value;
            Exit;
          end;
          if Pos('full:', LowerCase(Normalized)) = 1 then
          begin
            Delete(Normalized, 1, 5);
            AddValue(Domain, Normalized);
          end
          else if Pos('domain:', LowerCase(Normalized)) = 1 then
          begin
            Delete(Normalized, 1, 7);
            AddValue(Suffix, Normalized);
          end
          else if Pos('keyword:', LowerCase(Normalized)) = 1 then
          begin
            Delete(Normalized, 1, 8);
            AddValue(Keyword, Normalized);
          end
          else if Pos('regexp:', LowerCase(Normalized)) = 1 then
          begin
            Delete(Normalized, 1, 7);
            AddValue(Regex, Normalized);
          end
          else AddValue(Suffix, Normalized);
        end;
        if Length(Domain) > 0 then AResult.Add('domain', JsonStringArray(Domain));
        if Length(Suffix) > 0 then AResult.Add('domain_suffix', JsonStringArray(Suffix));
        if Length(Keyword) > 0 then AResult.Add('domain_keyword', JsonStringArray(Keyword));
        if Length(Regex) > 0 then AResult.Add('domain_regex', JsonStringArray(Regex));
      end;
    rrIp:
      begin
        for I := 0 to High(ARule.Values) do
        begin
          Normalized := LowerCase(Trim(ARule.Values[I]));
          if Normalized = 'geoip:private' then
            AResult.Add('ip_is_private', True)
          else if Pos('geoip:', Normalized) = 1 then
          begin
            AError := 'sing-box adapter cannot safely translate GeoIP routing rule: ' + ARule.Values[I];
            Exit;
          end
          else if Pos('ext:', Normalized) = 1 then
          begin
            AError := 'sing-box adapter cannot safely translate ext routing rule: ' + ARule.Values[I];
            Exit;
          end;
        end;
        if (AResult.IndexOfName('ip_is_private') < 0) then
          AResult.Add('ip_cidr', JsonStringArray(ARule.Values));
      end;
    rrPort: AResult.Add('port_range', JsonStringArray(ARule.Values));
    rrProtocol:
      begin
        for Value in ARule.Values do
          if not SameText(Trim(Value), 'tcp') and
            not SameText(Trim(Value), 'udp') then
          begin
            AError := 'sing-box adapter only maps tcp/udp protocol routing rules.';
            Exit;
          end;
        AResult.Add('network', JsonStringArray(ARule.Values));
      end;
  end;
  Result := True;
end;

function ApplySingBoxPolicies(const AConfig: string;
  const ARouting: TZaryaRoutingProfile; const ADns: TZaryaDnsProfile;
  const AUseRouting, AUseDns: Boolean; out AResult, AError: string): Boolean;
var
  Data: TJSONData;
  Root, RouteObject, RuleObject, DnsObject, ServerObject: TJSONObject;
  Rules, Servers: TJSONArray;
  Rule: TZaryaRoutingRule;
  Server: TZaryaDnsServer;
  ServerIndex: Integer;
  DnsRules: TJSONArray;
  DomainRule: TZaryaRoutingRule;
  Strategy: string;
  ServerTag: string;
  FirstServerTag: string;
  I: Integer;
begin
  AResult := '';
  AError := '';
  if not ParseRoot(AConfig, Data, Root, AError) then Exit(False);
  try
    if AUseRouting then
    begin
      if not ValidateRoutingProfile(ARouting, AError) then Exit(False);
      RouteObject := TJSONObject.Create;
      Rules := TJSONArray.Create;
      RouteObject.Add('rules', Rules);
      for Rule in ARouting.Rules do
        if Rule.Enabled then
        begin
          if not SingBoxDomainRule(Rule, OutboundForAction(Rule.Action),
            RuleObject, AError) then
          begin
            RuleObject.Free;
            RouteObject.Free;
            Exit(False);
          end;
          Rules.Add(RuleObject);
        end;
      RouteObject.Add('final', 'proxy');
      ReplaceMember(Root, 'route', RouteObject);
    end;
    if AUseDns and not IsDefaultDns(ADns) then
    begin
      if not ValidateDnsProfile(ADns, AError) then Exit(False);
      if DnsUsesGeoData(ADns) then
      begin
        AError := 'sing-box adapter cannot safely translate geodata DNS policies yet.';
        Exit(False);
      end;
      DnsObject := TJSONObject.Create;
      Servers := TJSONArray.Create;
      DnsObject.Add('servers', Servers);
      DnsRules := TJSONArray.Create;
      DnsObject.Add('rules', DnsRules);
      ServerIndex := 0;
      FirstServerTag := '';
      for Server in ADns.Servers do
        if Server.Enabled and (Server.Kind <> dskFakeDns) then
        begin
          if (Length(Server.ExpectIps) > 0) or (Server.TimeoutMs > 0) or
            Server.SkipFallback then
          begin
            DnsObject.Free;
            AError := 'sing-box adapter cannot safely translate DNS expectIPs, timeoutMs or skipFallback.';
            Exit(False);
          end;
          ServerObject := TJSONObject.Create;
          if Trim(Server.Tag) <> '' then
            ServerTag := Trim(Server.Tag)
          else
            ServerTag := 'dns-' + IntToStr(ServerIndex);
          ServerObject.Add('tag', ServerTag);
          if FirstServerTag = '' then FirstServerTag := ServerTag;
          ServerObject.Add('address', EffectiveDnsAddress(Server));
          if SameText(Server.QueryStrategy, 'UseIPv4') then
            ServerObject.Add('strategy', 'ipv4_only')
          else if SameText(Server.QueryStrategy, 'UseIPv6') then
            ServerObject.Add('strategy', 'ipv6_only')
          else if SameText(Server.QueryStrategy, 'UseIP') then
            ServerObject.Add('strategy', 'prefer_ipv4');
          Servers.Add(ServerObject);
          if Length(Server.Domains) > 0 then
          begin
            DomainRule := NewRoutingRule;
            DomainRule.RuleType := rrDomain;
            SetLength(DomainRule.Values, Length(Server.Domains));
            for I := 0 to High(Server.Domains) do
              DomainRule.Values[I] := Server.Domains[I];
            if not SingBoxDomainRule(DomainRule, ServerTag, RuleObject,
              AError) then
            begin
              RuleObject.Free;
              DnsObject.Free;
              Exit(False);
            end;
            RemoveMember(RuleObject, 'outbound');
            RuleObject.Add('server', ServerTag);
            DnsRules.Add(RuleObject);
          end;
          Inc(ServerIndex);
        end;
      if Servers.Count = 0 then
      begin
        DnsObject.Free;
        AError := 'Selected DNS profile has no usable sing-box servers.';
        Exit(False);
      end;
      DnsObject.Add('final', FirstServerTag);
      case ADns.QueryStrategy of
        dqsUseIp: Strategy := 'prefer_ipv4';
        dqsUseIpv4: Strategy := 'ipv4_only';
        dqsUseIpv6: Strategy := 'ipv6_only';
      else
        Strategy := '';
      end;
      if Strategy <> '' then DnsObject.Add('strategy', Strategy);
      ReplaceMember(Root, 'dns', DnsObject);
    end;
    AResult := Root.AsJSON;
    Result := True;
  finally
    Data.Free;
  end;
end;

function MihomoTarget(const AAction: TZaryaRoutingAction): string;
begin
  case AAction of
    raDirect: Result := 'DIRECT';
    raBlock: Result := 'REJECT';
  else
    Result := 'PROXY';
  end;
end;

function MihomoRuleLine(const ARule: TZaryaRoutingRule;
  const AValue: string; out ALine, AError: string): Boolean;
var
  Value: string;
  Lower: string;
  Kind: string;
begin
  Value := Trim(AValue);
  Lower := LowerCase(Value);
  case ARule.RuleType of
    rrDomain:
      begin
        if Pos('geosite:', Lower) = 1 then begin Kind := 'GEOSITE'; Delete(Value, 1, 8); end
        else if Pos('full:', Lower) = 1 then begin Kind := 'DOMAIN'; Delete(Value, 1, 5); end
        else if Pos('domain:', Lower) = 1 then begin Kind := 'DOMAIN-SUFFIX'; Delete(Value, 1, 7); end
        else if Pos('keyword:', Lower) = 1 then begin Kind := 'DOMAIN-KEYWORD'; Delete(Value, 1, 8); end
        else if Pos('regexp:', Lower) = 1 then begin Kind := 'DOMAIN-REGEX'; Delete(Value, 1, 7); end
        else if Pos('ext:', Lower) = 1 then
        begin AError := 'Mihomo adapter cannot safely translate ext routing rule: ' + Value; Exit(False); end
        else Kind := 'DOMAIN-SUFFIX';
      end;
    rrIp:
      begin
        if Pos('geoip:', Lower) = 1 then begin Kind := 'GEOIP'; Delete(Value, 1, 6); end
        else if Pos('ext:', Lower) = 1 then
        begin AError := 'Mihomo adapter cannot safely translate ext routing rule: ' + Value; Exit(False); end
        else if Pos(':', Value) > 0 then Kind := 'IP-CIDR6'
        else Kind := 'IP-CIDR';
      end;
    rrPort: Kind := 'DST-PORT';
    rrProtocol:
      begin
        if not SameText(Value, 'tcp') and not SameText(Value, 'udp') then
        begin AError := 'Mihomo adapter only maps tcp/udp protocol routing rules.'; Exit(False); end;
        Kind := 'NETWORK';
        Value := UpperCase(Value);
      end;
  end;
  ALine := '  - ' + Kind + ',' + Value + ',' + MihomoTarget(ARule.Action);
  if ARule.RuleType = rrIp then ALine := ALine + ',no-resolve';
  Result := True;
end;

function ApplyMihomoPolicies(const AConfig: string;
  const ARouting: TZaryaRoutingProfile; const ADns: TZaryaDnsProfile;
  const AUseRouting, AUseDns: Boolean; out AResult, AError: string): Boolean;
var
  Marker: Integer;
  Rule: TZaryaRoutingRule;
  Value, Line: string;
  Server: TZaryaDnsServer;
  Host: TZaryaDnsHost;
  Domain: string;
  NL: string;
begin
  AResult := AConfig;
  AError := '';
  NL := LineEnding;
  if AUseRouting then
  begin
    if not ValidateRoutingProfile(ARouting, AError) then Exit(False);
    Marker := Pos(NL + 'rules:', AResult);
    if Marker = 0 then Marker := Pos('rules:', AResult);
    if Marker > 0 then AResult := Copy(AResult, 1, Marker - 1);
  end;
  if AUseDns and not IsDefaultDns(ADns) then
  begin
    if not ValidateDnsProfile(ADns, AError) then Exit(False);
    if ADns.DisableCache or ADns.DisableFallback or
      ADns.DisableFallbackIfMatch or
      (ADns.QueryStrategy = dqsUseIpv6) then
    begin
      AError := 'Mihomo adapter cannot safely translate the selected DNS flags/query strategy.';
      Exit(False);
    end;
    for Server in ADns.Servers do
      if Server.Enabled and ((Length(Server.ExpectIps) > 0) or
        (Server.TimeoutMs > 0) or Server.SkipFallback or
        (Trim(Server.QueryStrategy) <> '') or
        (Server.Kind = dskFakeDns)) then
      begin
        AError := 'Mihomo adapter cannot safely translate DNS expectIPs, timeout, skipFallback, per-server strategy or FakeDNS.';
        Exit(False);
      end;
    if Length(ADns.Hosts) > 0 then
    begin
      AResult := AResult + 'hosts:' + NL;
      for Host in ADns.Hosts do
        AResult := AResult + '  ' + Host.Host + ': ' + Host.Address + NL;
    end;
    AResult := AResult + 'dns:' + NL + '  enable: true' + NL +
      '  ipv6: ' + LowerCase(BoolToStr(
        ADns.QueryStrategy <> dqsUseIpv4, True)) + NL +
      '  nameserver:' + NL;
    for Server in ADns.Servers do
      if Server.Enabled and (Server.Kind <> dskFakeDns) then
        AResult := AResult + '    - ' + EffectiveDnsAddress(Server) + NL;
    for Server in ADns.Servers do
      if Server.Enabled and (Length(Server.Domains) > 0) then
      begin
        AResult := AResult + '  nameserver-policy:' + NL;
        Break;
      end;
    for Server in ADns.Servers do
      if Server.Enabled then
        for Domain in Server.Domains do
          AResult := AResult + '    ' + Domain + ': ' +
            EffectiveDnsAddress(Server) + NL;
  end;
  if AUseRouting then
  begin
    AResult := AResult + 'rules:' + NL;
    for Rule in ARouting.Rules do
      if Rule.Enabled then
        for Value in Rule.Values do
        begin
          if not MihomoRuleLine(Rule, Value, Line, AError) then Exit(False);
          AResult := AResult + Line + NL;
        end;
    AResult := AResult + '  - MATCH,PROXY' + NL;
  end;
  Result := True;
end;

function ValidateStandalonePolicies(const ARouting: TZaryaRoutingProfile;
  const ADns: TZaryaDnsProfile; const AUseRouting,
  AUseDns: Boolean; out AError: string): Boolean;
begin
  AError := '';
  if AUseRouting and not IsDefaultRouting(ARouting) then
    AError := 'This provider does not support the selected routing profile.'
  else if AUseDns and not IsDefaultDns(ADns) then
    AError := 'This provider does not support the selected DNS profile.';
  Result := AError = '';
end;

end.
