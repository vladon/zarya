unit ZaryaConfigAdapters;

{$mode objfpc}{$H+}

interface

uses
  SysUtils, ZaryaProfile, ZaryaCoreProvider, ZaryaRuntimeContracts;

function CreateConfigAdapter(const AProvider: TZaryaCoreProvider): IConfigAdapter;
function ProviderCanGenerateConfig(const AProvider: TZaryaCoreProvider;
  const AProfile: TZaryaProfile; out AReason: string): Boolean;

implementation

uses
  ZaryaXrayConfig;

type
  TZaryaKnownConfigAdapter = class(TInterfacedObject, IConfigAdapter)
  private
    FAdapterId: string;
    FFormat: TZaryaConfigFormat;
    function GenerateV2Ray(const AProfile: TZaryaProfile;
      const AContext: TZaryaConfigContext; out AConfig,
      AError: string): Boolean;
    function GenerateSingBox(const AProfile: TZaryaProfile;
      const AContext: TZaryaConfigContext; out AConfig,
      AError: string): Boolean;
    function GenerateMihomo(const AProfile: TZaryaProfile;
      const AContext: TZaryaConfigContext; out AConfig,
      AError: string): Boolean;
    function GenerateHysteria2(const AProfile: TZaryaProfile;
      const AContext: TZaryaConfigContext; out AConfig,
      AError: string): Boolean;
  public
    constructor Create(const AAdapterId: string;
      const AFormat: TZaryaConfigFormat);
    function AdapterId: string;
    function ConfigFormat: TZaryaConfigFormat;
    function Supports(const AProfile: TZaryaProfile;
      out AReason: string): Boolean;
    function Generate(const AProfile: TZaryaProfile;
      const AContext: TZaryaConfigContext; out AConfig,
      AError: string): Boolean;
  end;

function JsonQuote(const AValue: string): string;
var
  I: Integer;
  Ch: Char;
begin
  Result := '"';
  for I := 1 to Length(AValue) do
  begin
    Ch := AValue[I];
    case Ch of
      '"': Result := Result + '\"';
      '\': Result := Result + '\\';
      #8: Result := Result + '\b';
      #9: Result := Result + '\t';
      #10: Result := Result + '\n';
      #12: Result := Result + '\f';
      #13: Result := Result + '\r';
    else
      if Ord(Ch) < 32 then
        Result := Result + '\u' + IntToHex(Ord(Ch), 4)
      else
        Result := Result + Ch;
    end;
  end;
  Result := Result + '"';
end;

function JsonBoolean(const AValue: Boolean): string;
begin
  if AValue then Result := 'true' else Result := 'false';
end;

function EffectiveSni(const AProfile: TZaryaProfile): string;
begin
  Result := EffectiveServerName(AProfile);
  if Result = '' then Result := Trim(AProfile.Host);
end;

function BuildV2RayStream(const AProfile: TZaryaProfile): string;
var
  Network: string;
  Security: string;
  Sni: string;
  Path: string;
  ServiceName: string;
begin
  Network := NormalizedNetwork(AProfile);
  Security := LowerCase(Trim(AProfile.Security));
  Sni := EffectiveSni(AProfile);
  Result := '{"network":' + JsonQuote(Network);
  if Network = 'ws' then
  begin
    Path := Trim(AProfile.Path);
    if Path = '' then Path := '/';
    Result := Result + ',"wsSettings":{"path":' + JsonQuote(Path);
    if Trim(AProfile.TransportHost) <> '' then
      Result := Result + ',"headers":{"Host":' +
        JsonQuote(Trim(AProfile.TransportHost)) + '}';
    Result := Result + '}';
  end
  else if Network = 'grpc' then
  begin
    ServiceName := Trim(AProfile.ServiceName);
    if ServiceName = '' then ServiceName := Trim(AProfile.Path);
    Result := Result + ',"grpcSettings":{"serviceName":' +
      JsonQuote(ServiceName) + '}';
  end;
  if Security = 'tls' then
    Result := Result + ',"security":"tls","tlsSettings":{' +
      '"serverName":' + JsonQuote(Sni) + ',"allowInsecure":' +
      JsonBoolean(AProfile.AllowInsecure) + '}';
  Result := Result + '}';
end;

constructor TZaryaKnownConfigAdapter.Create(const AAdapterId: string;
  const AFormat: TZaryaConfigFormat);
begin
  inherited Create;
  FAdapterId := LowerCase(Trim(AAdapterId));
  FFormat := AFormat;
end;

function TZaryaKnownConfigAdapter.AdapterId: string;
begin
  Result := FAdapterId;
end;

function TZaryaKnownConfigAdapter.ConfigFormat: TZaryaConfigFormat;
begin
  Result := FFormat;
end;

function TZaryaKnownConfigAdapter.Supports(const AProfile: TZaryaProfile;
  out AReason: string): Boolean;
begin
  AReason := '';
  if FAdapterId = 'hysteria2' then
  begin
    Result := SameText(AProfile.ProtocolName, 'Hysteria2');
    if not Result then
      AReason := 'Hysteria 2 adapter принимает только Hysteria2-профили.'
    else if (Trim(AProfile.Password) = '') and
      (Trim(AProfile.Uuid) = '') then
    begin
      Result := False;
      AReason := 'Для Hysteria2 требуется auth/password.';
    end;
    if Result then
      Result := ValidateProfile(AProfile, AReason);
    Exit;
  end;
  if FAdapterId = 'xray' then
  begin
    Result := SameText(AProfile.ProtocolName, 'VLESS') or
      SameText(AProfile.ProtocolName, 'VMess') or
      SameText(AProfile.ProtocolName, 'Trojan') or
      SameText(AProfile.ProtocolName, 'Shadowsocks') or
      SameText(AProfile.ProtocolName, 'SOCKS') or
      SameText(AProfile.ProtocolName, 'Hysteria2') or
      SameText(AProfile.ProtocolName, 'WireGuard');
    if not Result then
      AReason := 'Xray adapter не поддерживает протокол ' +
        AProfile.ProtocolName + '.'
    else
      Result := ValidateProfile(AProfile, AReason);
    Exit;
  end;
  if FAdapterId = 'v2ray' then
  begin
    Result := SameText(AProfile.ProtocolName, 'VLESS') or
      SameText(AProfile.ProtocolName, 'VMess') or
      SameText(AProfile.ProtocolName, 'Trojan') or
      SameText(AProfile.ProtocolName, 'Shadowsocks') or
      SameText(AProfile.ProtocolName, 'SOCKS');
    if not Result then
      AReason := 'V2Ray adapter не поддерживает протокол ' +
        AProfile.ProtocolName + '.'
    else if SameText(Trim(AProfile.Security), 'reality') then
    begin
      Result := False;
      AReason := 'V2Ray adapter не поддерживает REALITY.';
    end
    else
      Result := ValidateProfile(AProfile, AReason);
    Exit;
  end;
  if (FAdapterId = 'sing-box') or (FAdapterId = 'nekobox-core') then
  begin
    Result := SameText(AProfile.ProtocolName, 'VLESS') or
      SameText(AProfile.ProtocolName, 'VMess') or
      SameText(AProfile.ProtocolName, 'Trojan') or
      SameText(AProfile.ProtocolName, 'Shadowsocks') or
      SameText(AProfile.ProtocolName, 'SOCKS') or
      SameText(AProfile.ProtocolName, 'Hysteria2');
    if not Result then
      AReason := 'sing-box adapter не поддерживает этот профиль; ' +
        'для WireGuard новых версий нужен endpoint adapter.'
    else
      Result := ValidateProfile(AProfile, AReason);
    Exit;
  end;
  if FAdapterId = 'mihomo' then
  begin
    Result := SameText(AProfile.ProtocolName, 'VLESS') or
      SameText(AProfile.ProtocolName, 'VMess') or
      SameText(AProfile.ProtocolName, 'Trojan') or
      SameText(AProfile.ProtocolName, 'Shadowsocks') or
      SameText(AProfile.ProtocolName, 'SOCKS') or
      SameText(AProfile.ProtocolName, 'Hysteria2') or
      SameText(AProfile.ProtocolName, 'WireGuard');
    if not Result then
      AReason := 'Mihomo adapter не поддерживает протокол ' +
        AProfile.ProtocolName + '.'
    else
      Result := ValidateProfile(AProfile, AReason);
    Exit;
  end;
  AReason := 'Неизвестный config adapter: ' + FAdapterId + '.';
  Result := False;
end;

function TZaryaKnownConfigAdapter.GenerateV2Ray(
  const AProfile: TZaryaProfile; const AContext: TZaryaConfigContext;
  out AConfig, AError: string): Boolean;
var
  User: string;
  Settings: string;
  Outbound: string;
  Password: string;
  Cipher: string;
  Method: string;
begin
  Password := EffectivePassword(AProfile);
  if SameText(AProfile.ProtocolName, 'VLESS') then
  begin
    User := '{"id":' + JsonQuote(Trim(AProfile.Uuid)) +
      ',"encryption":"none"}';
    Settings := '{"vnext":[{"address":' + JsonQuote(Trim(AProfile.Host)) +
      ',"port":' + IntToStr(AProfile.Port) + ',"users":[' + User + ']}]}';
    Outbound := '{"tag":"proxy","protocol":"vless","settings":' +
      Settings + ',"streamSettings":' + BuildV2RayStream(AProfile) + '}'
  end
  else if SameText(AProfile.ProtocolName, 'VMess') then
  begin
    Cipher := Trim(AProfile.SecurityCipher);
    if Cipher = '' then Cipher := 'auto';
    User := '{"id":' + JsonQuote(Trim(AProfile.Uuid)) + ',"alterId":' +
      IntToStr(AProfile.AlterId) + ',"security":' + JsonQuote(Cipher) + '}';
    Settings := '{"vnext":[{"address":' + JsonQuote(Trim(AProfile.Host)) +
      ',"port":' + IntToStr(AProfile.Port) + ',"users":[' + User + ']}]}';
    Outbound := '{"tag":"proxy","protocol":"vmess","settings":' +
      Settings + ',"streamSettings":' + BuildV2RayStream(AProfile) + '}'
  end
  else if SameText(AProfile.ProtocolName, 'Trojan') then
  begin
    Settings := '{"servers":[{"address":' +
      JsonQuote(Trim(AProfile.Host)) + ',"port":' +
      IntToStr(AProfile.Port) + ',"password":' + JsonQuote(Password) + '}]}';
    Outbound := '{"tag":"proxy","protocol":"trojan","settings":' +
      Settings + ',"streamSettings":' + BuildV2RayStream(AProfile) + '}'
  end
  else if SameText(AProfile.ProtocolName, 'Shadowsocks') then
  begin
    Method := EffectiveMethod(AProfile);
    Settings := '{"servers":[{"address":' +
      JsonQuote(Trim(AProfile.Host)) + ',"port":' +
      IntToStr(AProfile.Port) + ',"method":' + JsonQuote(Method) +
      ',"password":' + JsonQuote(Password) + '}]}';
    Outbound := '{"tag":"proxy","protocol":"shadowsocks","settings":' +
      Settings + '}'
  end
  else
  begin
    Settings := '{"servers":[{"address":' +
      JsonQuote(Trim(AProfile.Host)) + ',"port":' +
      IntToStr(AProfile.Port);
    if Password <> '' then
    begin
      Settings := Settings + ',"users":[{';
      if Trim(AProfile.Uuid) <> '' then
        Settings := Settings + '"user":' + JsonQuote(Trim(AProfile.Uuid)) + ',';
      Settings := Settings + '"pass":' + JsonQuote(Password) + '}]';
    end;
    Settings := Settings + '}]}';
    Outbound := '{"tag":"proxy","protocol":"socks","settings":' +
      Settings + '}'
  end;
  AConfig := '{' +
    '"log":{"loglevel":"warning"},' +
    '"inbounds":[{"listen":"127.0.0.1","port":' +
      IntToStr(AContext.SocksPort) +
      ',"protocol":"socks","tag":"socks-in","settings":{"udp":true}}],' +
    '"outbounds":[' + Outbound + ',' +
      '{"tag":"direct","protocol":"freedom"},' +
      '{"tag":"block","protocol":"blackhole"}],' +
    '"routing":{"domainStrategy":"AsIs","rules":[' +
      '{"type":"field","network":"tcp,udp","outboundTag":"proxy"}]}}';
  AError := '';
  Result := True;
end;

function BuildSingBoxTls(const AProfile: TZaryaProfile): string;
var
  Security: string;
begin
  Security := LowerCase(Trim(AProfile.Security));
  if (Security <> 'tls') and (Security <> 'reality') then
    Exit('');
  Result := ',"tls":{"enabled":true,"server_name":' +
    JsonQuote(EffectiveSni(AProfile)) + ',"insecure":' +
    JsonBoolean(AProfile.AllowInsecure);
  if Trim(AProfile.Fingerprint) <> '' then
    Result := Result + ',"utls":{"enabled":true,"fingerprint":' +
      JsonQuote(Trim(AProfile.Fingerprint)) + '}';
  if Security = 'reality' then
    Result := Result + ',"reality":{"enabled":true,"public_key":' +
      JsonQuote(Trim(AProfile.PublicKey)) + ',"short_id":' +
      JsonQuote(Trim(AProfile.ShortId)) + '}';
  Result := Result + '}';
end;

function BuildSingBoxTransport(const AProfile: TZaryaProfile): string;
var
  Network: string;
  Path: string;
  ServiceName: string;
begin
  Result := '';
  Network := NormalizedNetwork(AProfile);
  if Network = 'ws' then
  begin
    Path := Trim(AProfile.Path);
    if Path = '' then Path := '/';
    Result := ',"transport":{"type":"ws","path":' + JsonQuote(Path);
    if Trim(AProfile.TransportHost) <> '' then
      Result := Result + ',"headers":{"Host":' +
        JsonQuote(Trim(AProfile.TransportHost)) + '}';
    Result := Result + '}';
  end
  else if Network = 'grpc' then
  begin
    ServiceName := Trim(AProfile.ServiceName);
    if ServiceName = '' then ServiceName := Trim(AProfile.Path);
    Result := ',"transport":{"type":"grpc","service_name":' +
      JsonQuote(ServiceName) + '}';
  end;
end;

function TZaryaKnownConfigAdapter.GenerateSingBox(
  const AProfile: TZaryaProfile; const AContext: TZaryaConfigContext;
  out AConfig, AError: string): Boolean;
var
  Flow: string;
  Outbound: string;
  Password: string;
  Cipher: string;
  Method: string;
begin
  Flow := '';
  if Trim(AProfile.Flow) <> '' then
    Flow := ',"flow":' + JsonQuote(Trim(AProfile.Flow));
  Password := EffectivePassword(AProfile);
  if SameText(AProfile.ProtocolName, 'VLESS') then
    Outbound := '{"type":"vless","tag":"proxy","server":' +
      JsonQuote(Trim(AProfile.Host)) + ',"server_port":' +
      IntToStr(AProfile.Port) + ',"uuid":' + JsonQuote(Trim(AProfile.Uuid)) +
      Flow + BuildSingBoxTls(AProfile) + BuildSingBoxTransport(AProfile) + '}'
  else if SameText(AProfile.ProtocolName, 'VMess') then
  begin
    Cipher := Trim(AProfile.SecurityCipher);
    if Cipher = '' then Cipher := 'auto';
    Outbound := '{"type":"vmess","tag":"proxy","server":' +
      JsonQuote(Trim(AProfile.Host)) + ',"server_port":' +
      IntToStr(AProfile.Port) + ',"uuid":' + JsonQuote(Trim(AProfile.Uuid)) +
      ',"security":' + JsonQuote(Cipher) + ',"alter_id":' +
      IntToStr(AProfile.AlterId) + BuildSingBoxTls(AProfile) +
      BuildSingBoxTransport(AProfile) + '}'
  end
  else if SameText(AProfile.ProtocolName, 'Trojan') then
    Outbound := '{"type":"trojan","tag":"proxy","server":' +
      JsonQuote(Trim(AProfile.Host)) + ',"server_port":' +
      IntToStr(AProfile.Port) + ',"password":' + JsonQuote(Password) +
      BuildSingBoxTls(AProfile) + BuildSingBoxTransport(AProfile) + '}'
  else if SameText(AProfile.ProtocolName, 'Shadowsocks') then
  begin
    Method := EffectiveMethod(AProfile);
    Outbound := '{"type":"shadowsocks","tag":"proxy","server":' +
      JsonQuote(Trim(AProfile.Host)) + ',"server_port":' +
      IntToStr(AProfile.Port) + ',"method":' + JsonQuote(Method) +
      ',"password":' + JsonQuote(Password) + '}'
  end
  else if SameText(AProfile.ProtocolName, 'SOCKS') then
  begin
    Outbound := '{"type":"socks","tag":"proxy","server":' +
      JsonQuote(Trim(AProfile.Host)) + ',"server_port":' +
      IntToStr(AProfile.Port) + ',"version":"5"';
    if Trim(AProfile.Uuid) <> '' then
      Outbound := Outbound + ',"username":' +
        JsonQuote(Trim(AProfile.Uuid));
    if Password <> '' then
      Outbound := Outbound + ',"password":' + JsonQuote(Password);
    Outbound := Outbound + '}'
  end
  else
  begin
    Outbound := '{"type":"hysteria2","tag":"proxy","server":' +
      JsonQuote(Trim(AProfile.Host)) + ',"server_port":' +
      IntToStr(AProfile.Port) + ',"password":' + JsonQuote(Password);
    if SameText(Trim(AProfile.Obfs), 'salamander') then
      Outbound := Outbound + ',"obfs":{"type":"salamander","password":' +
        JsonQuote(Trim(AProfile.ObfsPassword)) + '}';
    Outbound := Outbound + BuildSingBoxTls(AProfile) + '}'
  end;
  AConfig := '{' +
    '"log":{"level":"warn"},' +
    '"inbounds":[{"type":"mixed","tag":"mixed-in",' +
      '"listen":"127.0.0.1","listen_port":' +
      IntToStr(AContext.MixedPort) + '}],' +
    '"outbounds":[' + Outbound + ',' +
      '{"type":"direct","tag":"direct"},' +
      '{"type":"block","tag":"block"}],' +
    '"route":{"final":"proxy"}}';
  AError := '';
  Result := True;
end;

function YamlQuoted(const AValue: string): string;
begin
  Result := JsonQuote(AValue);
end;

function FirstListValue(const AValue: string): string;
var
  I: Integer;
begin
  Result := Trim(AValue);
  for I := 1 to Length(Result) do
    if Result[I] in [',', ' ', #9, #10, #13] then
    begin
      Result := Copy(Result, 1, I - 1);
      Break;
    end;
end;

function StripCidr(const AValue: string): string;
var
  Marker: Integer;
begin
  Result := Trim(AValue);
  Marker := Pos('/', Result);
  if Marker > 0 then
    Result := Copy(Result, 1, Marker - 1);
end;

function YamlInlineStringList(const AValue: string): string;
var
  Work: string;
  Item: string;
  I: Integer;
  Separator: string;
begin
  Work := Trim(AValue);
  Result := '[';
  Item := '';
  Separator := '';
  for I := 1 to Length(Work) + 1 do
    if I > Length(Work) then
    begin
      if Trim(Item) <> '' then
      begin
        Result := Result + Separator + YamlQuoted(Trim(Item));
        Separator := ', ';
      end;
      Item := '';
    end
    else if Work[I] in [',', ' ', #9, #10, #13] then
    begin
      if Trim(Item) <> '' then
      begin
        Result := Result + Separator + YamlQuoted(Trim(Item));
        Separator := ', ';
      end;
      Item := '';
    end
    else
      Item := Item + Work[I];
  Result := Result + ']';
end;

function WireGuardReservedYaml(const AValue: string): string;
var
  Work: string;
  Item: string;
  I: Integer;
  Value: Integer;
  Separator: string;
  Bytes: string;
begin
  Work := Trim(AValue);
  if Pos(',', Work) = 0 then
    Exit(YamlQuoted(Work));
  Item := '';
  Bytes := '[';
  Separator := '';
  for I := 1 to Length(Work) + 1 do
  begin
    if I <= Length(Work) then
      if not (Work[I] in [',', ' ', #9, #10, #13]) then
      begin
        Item := Item + Work[I];
        Continue;
      end;
    if Trim(Item) <> '' then
    begin
      if not TryStrToInt(Trim(Item), Value) or (Value < 0) or (Value > 255) then
        Exit(YamlQuoted(Work));
      Bytes := Bytes + Separator + IntToStr(Value);
      Separator := ', ';
    end;
    Item := '';
  end;
  Result := Bytes + ']';
end;

function ListAddressByFamily(const AValue: string;
  const AWantIpv6: Boolean): string;
var
  Work: string;
  Item: string;
  I: Integer;
begin
  Result := '';
  Work := Trim(AValue);
  Item := '';
  for I := 1 to Length(Work) + 1 do
  begin
    if I <= Length(Work) then
      if not (Work[I] in [',', ' ', #9, #10, #13]) then
      begin
        Item := Item + Work[I];
        Continue;
      end;
    Item := Trim(Item);
    if (Item <> '') and ((Pos(':', Item) > 0) = AWantIpv6) then
      Exit(Item);
    Item := '';
  end;
end;

procedure AppendMihomoTlsAndTransport(var AConfig: string;
  const AProfile: TZaryaProfile; const NL: string);
var
  Security: string;
  Network: string;
  Path: string;
  ServiceName: string;
begin
  Security := LowerCase(Trim(AProfile.Security));
  Network := NormalizedNetwork(AProfile);
  AConfig := AConfig + '    network: ' + Network + NL;
  if (Security = 'tls') or (Security = 'reality') then
  begin
    AConfig := AConfig + '    tls: true' + NL +
      '    servername: ' + YamlQuoted(EffectiveSni(AProfile)) + NL +
      '    skip-cert-verify: ' + LowerCase(BoolToStr(
        AProfile.AllowInsecure, True)) + NL;
    if Trim(AProfile.Fingerprint) <> '' then
      AConfig := AConfig + '    client-fingerprint: ' +
        YamlQuoted(Trim(AProfile.Fingerprint)) + NL;
  end;
  if Security = 'reality' then
    AConfig := AConfig + '    reality-opts:' + NL +
      '      public-key: ' + YamlQuoted(Trim(AProfile.PublicKey)) + NL +
      '      short-id: ' + YamlQuoted(Trim(AProfile.ShortId)) + NL;
  if Network = 'ws' then
  begin
    Path := Trim(AProfile.Path);
    if Path = '' then Path := '/';
    AConfig := AConfig + '    ws-opts:' + NL +
      '      path: ' + YamlQuoted(Path) + NL;
    if Trim(AProfile.TransportHost) <> '' then
      AConfig := AConfig + '      headers:' + NL + '        Host: ' +
        YamlQuoted(Trim(AProfile.TransportHost)) + NL;
  end
  else if Network = 'grpc' then
  begin
    ServiceName := Trim(AProfile.ServiceName);
    if ServiceName = '' then ServiceName := Trim(AProfile.Path);
    AConfig := AConfig + '    grpc-opts:' + NL +
      '      grpc-service-name: ' + YamlQuoted(ServiceName) + NL;
  end;
end;

function TZaryaKnownConfigAdapter.GenerateMihomo(
  const AProfile: TZaryaProfile; const AContext: TZaryaConfigContext;
  out AConfig, AError: string): Boolean;
var
  NL: string;
  Password: string;
  Cipher: string;
  Ipv4Address: string;
  Ipv6Address: string;
begin
  NL := LineEnding;
  Password := EffectivePassword(AProfile);
  AConfig := 'mixed-port: ' + IntToStr(AContext.MixedPort) + NL +
    'allow-lan: false' + NL + 'mode: rule' + NL +
    'log-level: warning' + NL + 'proxies:' + NL +
    '  - name: zarya-proxy' + NL;
  if SameText(AProfile.ProtocolName, 'VLESS') then
  begin
    AConfig := AConfig + '    type: vless' + NL +
    '    server: ' + YamlQuoted(Trim(AProfile.Host)) + NL +
    '    port: ' + IntToStr(AProfile.Port) + NL +
    '    uuid: ' + YamlQuoted(Trim(AProfile.Uuid)) + NL;
    if Trim(AProfile.Flow) <> '' then
      AConfig := AConfig + '    flow: ' +
        YamlQuoted(Trim(AProfile.Flow)) + NL;
    AppendMihomoTlsAndTransport(AConfig, AProfile, NL);
  end
  else if SameText(AProfile.ProtocolName, 'VMess') then
  begin
    Cipher := Trim(AProfile.SecurityCipher);
    if Cipher = '' then Cipher := 'auto';
    AConfig := AConfig + '    type: vmess' + NL +
      '    server: ' + YamlQuoted(Trim(AProfile.Host)) + NL +
      '    port: ' + IntToStr(AProfile.Port) + NL +
      '    uuid: ' + YamlQuoted(Trim(AProfile.Uuid)) + NL +
      '    alterId: ' + IntToStr(AProfile.AlterId) + NL +
      '    cipher: ' + YamlQuoted(Cipher) + NL;
    AppendMihomoTlsAndTransport(AConfig, AProfile, NL);
  end
  else if SameText(AProfile.ProtocolName, 'Trojan') then
  begin
    AConfig := AConfig + '    type: trojan' + NL +
      '    server: ' + YamlQuoted(Trim(AProfile.Host)) + NL +
      '    port: ' + IntToStr(AProfile.Port) + NL +
      '    password: ' + YamlQuoted(Password) + NL;
    AppendMihomoTlsAndTransport(AConfig, AProfile, NL);
  end
  else if SameText(AProfile.ProtocolName, 'Shadowsocks') then
    AConfig := AConfig + '    type: ss' + NL +
      '    server: ' + YamlQuoted(Trim(AProfile.Host)) + NL +
      '    port: ' + IntToStr(AProfile.Port) + NL +
      '    cipher: ' + YamlQuoted(EffectiveMethod(AProfile)) + NL +
      '    password: ' + YamlQuoted(Password) + NL +
      '    udp: true' + NL
  else if SameText(AProfile.ProtocolName, 'SOCKS') then
  begin
    AConfig := AConfig + '    type: socks5' + NL +
      '    server: ' + YamlQuoted(Trim(AProfile.Host)) + NL +
      '    port: ' + IntToStr(AProfile.Port) + NL;
    if Trim(AProfile.Uuid) <> '' then
      AConfig := AConfig + '    username: ' +
        YamlQuoted(Trim(AProfile.Uuid)) + NL;
    if Password <> '' then
      AConfig := AConfig + '    password: ' + YamlQuoted(Password) + NL;
  end
  else if SameText(AProfile.ProtocolName, 'Hysteria2') then
  begin
    AConfig := AConfig + '    type: hysteria2' + NL +
      '    server: ' + YamlQuoted(Trim(AProfile.Host)) + NL +
      '    port: ' + IntToStr(AProfile.Port) + NL +
      '    password: ' + YamlQuoted(Password) + NL +
      '    sni: ' + YamlQuoted(EffectiveSni(AProfile)) + NL +
      '    skip-cert-verify: ' + LowerCase(BoolToStr(
        AProfile.AllowInsecure, True)) + NL;
    if SameText(Trim(AProfile.Obfs), 'salamander') then
      AConfig := AConfig + '    obfs: salamander' + NL +
        '    obfs-password: ' +
        YamlQuoted(Trim(AProfile.ObfsPassword)) + NL;
  end
  else
  begin
    Ipv4Address := ListAddressByFamily(AProfile.LocalAddress, False);
    Ipv6Address := ListAddressByFamily(AProfile.LocalAddress, True);
    AConfig := AConfig + '    type: wireguard' + NL +
      '    private-key: ' + YamlQuoted(Password) + NL +
      '    server: ' + YamlQuoted(Trim(AProfile.Host)) + NL +
      '    port: ' + IntToStr(AProfile.Port) + NL +
      '    public-key: ' + YamlQuoted(Trim(AProfile.PublicKey)) + NL +
      '    allowed-ips: ' + YamlInlineStringList(AProfile.AllowedIps) + NL +
      '    udp: true' + NL;
    if Ipv4Address <> '' then
      AConfig := AConfig + '    ip: ' +
        YamlQuoted(StripCidr(Ipv4Address)) + NL;
    if Ipv6Address <> '' then
      AConfig := AConfig + '    ipv6: ' +
        YamlQuoted(StripCidr(Ipv6Address)) + NL;
    if Trim(AProfile.PreSharedKey) <> '' then
      AConfig := AConfig + '    pre-shared-key: ' +
        YamlQuoted(Trim(AProfile.PreSharedKey)) + NL;
    if Trim(AProfile.Reserved) <> '' then
      AConfig := AConfig + '    reserved: ' +
        WireGuardReservedYaml(AProfile.Reserved) + NL;
    if AProfile.KeepAlive > 0 then
      AConfig := AConfig + '    persistent-keepalive: ' +
        IntToStr(AProfile.KeepAlive) + NL;
    if AProfile.Mtu > 0 then
      AConfig := AConfig + '    mtu: ' + IntToStr(AProfile.Mtu) + NL;
  end;
  AConfig := AConfig + 'proxy-groups:' + NL +
    '  - name: PROXY' + NL + '    type: select' + NL +
    '    proxies:' + NL + '      - zarya-proxy' + NL +
    'rules:' + NL + '  - MATCH,PROXY' + NL;
  AError := '';
  Result := True;
end;

function TZaryaKnownConfigAdapter.GenerateHysteria2(
  const AProfile: TZaryaProfile; const AContext: TZaryaConfigContext;
  out AConfig, AError: string): Boolean;
var
  NL: string;
  Authentication: string;
begin
  NL := LineEnding;
  Authentication := Trim(AProfile.Password);
  if Authentication = '' then
    Authentication := Trim(AProfile.Uuid);
  AConfig := 'server: ' + YamlQuoted(Trim(AProfile.Host) + ':' +
      IntToStr(AProfile.Port)) + NL +
    'auth: ' + YamlQuoted(Authentication) + NL +
    'tls:' + NL + '  sni: ' + YamlQuoted(EffectiveSni(AProfile)) + NL +
    '  insecure: ' + LowerCase(BoolToStr(AProfile.AllowInsecure, True)) + NL +
    'socks5:' + NL + '  listen: ' +
      YamlQuoted('127.0.0.1:' + IntToStr(AContext.SocksPort)) + NL +
    'http:' + NL + '  listen: ' +
      YamlQuoted('127.0.0.1:' + IntToStr(AContext.HttpPort)) + NL;
  AError := '';
  Result := True;
end;

function TZaryaKnownConfigAdapter.Generate(const AProfile: TZaryaProfile;
  const AContext: TZaryaConfigContext; out AConfig,
  AError: string): Boolean;
begin
  AConfig := '';
  if not Supports(AProfile, AError) then
    Exit(False);
  if FAdapterId = 'xray' then
    Exit(GenerateXrayConfig(AProfile, AContext.MixedPort, AConfig, AError));
  if FAdapterId = 'v2ray' then
    Exit(GenerateV2Ray(AProfile, AContext, AConfig, AError));
  if (FAdapterId = 'sing-box') or (FAdapterId = 'nekobox-core') then
    Exit(GenerateSingBox(AProfile, AContext, AConfig, AError));
  if FAdapterId = 'mihomo' then
    Exit(GenerateMihomo(AProfile, AContext, AConfig, AError));
  if FAdapterId = 'hysteria2' then
    Exit(GenerateHysteria2(AProfile, AContext, AConfig, AError));
  AError := 'Для adapter ' + FAdapterId + ' нет генератора.';
  Result := False;
end;

function CreateConfigAdapter(const AProvider: TZaryaCoreProvider): IConfigAdapter;
var
  Adapter: string;
begin
  Adapter := LowerCase(Trim(AProvider.AdapterId));
  if (Adapter = 'xray') or (Adapter = 'v2ray') or
    (Adapter = 'sing-box') or (Adapter = 'nekobox-core') or
    (Adapter = 'mihomo') or (Adapter = 'hysteria2') then
    Result := TZaryaKnownConfigAdapter.Create(Adapter, AProvider.ConfigFormat)
  else
    Result := nil;
end;

function ProviderCanGenerateConfig(const AProvider: TZaryaCoreProvider;
  const AProfile: TZaryaProfile; out AReason: string): Boolean;
var
  Adapter: IConfigAdapter;
begin
  Adapter := CreateConfigAdapter(AProvider);
  Result := Assigned(Adapter) and Adapter.Supports(AProfile, AReason);
  if not Assigned(Adapter) then
    AReason := 'Для provider не зарегистрирован config adapter.';
end;

end.
