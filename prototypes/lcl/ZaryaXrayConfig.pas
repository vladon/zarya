unit ZaryaXrayConfig;

{$IFDEF FPC}
{$mode delphi}{$H+}
{$ENDIF}

interface

uses
  SysUtils, ZaryaProfile;

function GenerateXrayConfig(const AProfile: TZaryaProfile;
  const AMixedPort: Integer; out AJson, AError: string): Boolean;

implementation

uses
  Classes, base64;

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
  if AValue then
    Result := 'true'
  else
    Result := 'false';
end;

function JsonStringArray(const ACommaSeparated: string): string;
var
  Work: string;
  StartIndex: Integer;
  EndIndex: Integer;
  Item: string;
  Separator: string;
begin
  Work := Trim(ACommaSeparated);
  Result := '[';
  Separator := '';
  StartIndex := 1;
  while StartIndex <= Length(Work) do
  begin
    EndIndex := StartIndex;
    while (EndIndex <= Length(Work)) and (Work[EndIndex] <> ',') do
      Inc(EndIndex);
    Item := Trim(Copy(Work, StartIndex, EndIndex - StartIndex));
    if Item <> '' then
    begin
      Result := Result + Separator + JsonQuote(Item);
      Separator := ',';
    end;
    StartIndex := EndIndex + 1;
  end;
  Result := Result + ']';
end;

function BuildStreamSettings(const AProfile: TZaryaProfile): string;
var
  Network: string;
  Security: string;
  ServerName: string;
  Fingerprint: string;
  SpiderX: string;
  Path: string;
  TransportHost: string;
  ServiceName: string;
begin
  Network := NormalizedNetwork(AProfile);
  Security := LowerCase(Trim(AProfile.Security));
  ServerName := EffectiveServerName(AProfile);
  Fingerprint := Trim(AProfile.Fingerprint);
  if Fingerprint = '' then
    Fingerprint := 'chrome';

  Result := '{"network":' + JsonQuote(Network);
  if Network = 'ws' then
  begin
    Path := Trim(AProfile.Path);
    if Path = '' then
      Path := '/';
    TransportHost := Trim(AProfile.TransportHost);
    if TransportHost = '' then
      TransportHost := ServerName;
    Result := Result + ',"wsSettings":{"path":' + JsonQuote(Path);
    if TransportHost <> '' then
      Result := Result + ',"headers":{"Host":' + JsonQuote(TransportHost) + '}';
    Result := Result + '}';
  end
  else if Network = 'grpc' then
  begin
    ServiceName := Trim(AProfile.ServiceName);
    if ServiceName = '' then
      ServiceName := Trim(AProfile.Path);
    Result := Result + ',"grpcSettings":{"serviceName":' +
      JsonQuote(ServiceName) + '}';
  end;

  if Security = 'reality' then
  begin
    SpiderX := Trim(AProfile.SpiderX);
    if SpiderX = '' then
      SpiderX := '/';
    Result := Result + ',"security":"reality","realitySettings":{' +
      '"show":false,' +
      '"fingerprint":' + JsonQuote(Fingerprint) + ',' +
      '"serverName":' + JsonQuote(ServerName) + ',' +
      '"publicKey":' + JsonQuote(Trim(AProfile.PublicKey)) + ',' +
      '"shortId":' + JsonQuote(Trim(AProfile.ShortId)) + ',' +
      '"spiderX":' + JsonQuote(SpiderX) + '}';
  end
  else if Security = 'tls' then
  begin
    Result := Result + ',"security":"tls","tlsSettings":{' +
      '"serverName":' + JsonQuote(ServerName) + ',' +
      '"fingerprint":' + JsonQuote(Fingerprint);
    if AProfile.AllowInsecure then
      Result := Result + ',"allowInsecure":true';
    if Trim(AProfile.Alpn) <> '' then
      Result := Result + ',"alpn":' + JsonStringArray(AProfile.Alpn);
    Result := Result + '}';
  end;
  Result := Result + '}';
end;

function SplitJsonStringArray(const AValue: string): string;
var
  Parts: TStringList;
  Work: string;
  I: Integer;
  Ch: Char;
  Item: string;
  Separator: string;
begin
  Parts := TStringList.Create;
  try
    Work := Trim(AValue);
    Item := '';
    for I := 1 to Length(Work) do
    begin
      Ch := Work[I];
      if Ch in [',', ' ', #9, #10, #13] then
      begin
        if Trim(Item) <> '' then
          Parts.Add(Trim(Item));
        Item := '';
      end
      else
        Item := Item + Ch;
    end;
    if Trim(Item) <> '' then
      Parts.Add(Trim(Item));
    Result := '[';
    Separator := '';
    for I := 0 to Parts.Count - 1 do
    begin
      Result := Result + Separator + JsonQuote(Parts[I]);
      Separator := ',';
    end;
    Result := Result + ']';
  finally
    Parts.Free;
  end;
end;

function TryReservedJson(const AValue: string; out AJson: string): Boolean;
var
  Parts: TStringList;
  Work: string;
  Decoded: string;
  I: Integer;
  Value: Integer;
  Item: string;
begin
  Work := Trim(AValue);
  AJson := '';
  if Work = '' then
    Exit(True);
  if (Pos(',', Work) = 0) and (Pos(' ', Work) = 0) then
  begin
    try
      Decoded := DecodeStringBase64(Work);
    except
      Exit(False);
    end;
    if Length(Decoded) <> 3 then
      Exit(False);
    AJson := Format('[%d,%d,%d]', [Ord(Decoded[1]), Ord(Decoded[2]),
      Ord(Decoded[3])]);
    Exit(True);
  end;
  Parts := TStringList.Create;
  try
    Parts.StrictDelimiter := True;
    Parts.Delimiter := ',';
    Parts.DelimitedText := StringReplace(Work, ' ', ',', [rfReplaceAll]);
    AJson := '[';
    I := 0;
    while I < Parts.Count do
    begin
      Item := Trim(Parts[I]);
      if Item = '' then
      begin
        Parts.Delete(I);
        Continue;
      end;
      Value := StrToIntDef(Item, -1);
      if (Value < 0) or (Value > 255) then
        Exit(False);
      if AJson <> '[' then
        AJson := AJson + ',';
      AJson := AJson + IntToStr(Value);
      Inc(I);
    end;
    Result := Parts.Count = 3;
    if Result then
      AJson := AJson + ']'
    else
      AJson := '';
  finally
    Parts.Free;
  end;
end;

function BuildHysteriaStreamSettings(const AProfile: TZaryaProfile;
  out AError: string): string;
var
  Alpn: string;
  Fingerprint: string;
begin
  Alpn := Trim(AProfile.Alpn);
  if Alpn = '' then
    Alpn := 'h3';
  Fingerprint := Trim(AProfile.Fingerprint);
  if Fingerprint = '' then
    Fingerprint := 'chrome';
  Result := '{"network":"hysteria","security":"tls","tlsSettings":{' +
    '"serverName":' + JsonQuote(EffectiveServerName(AProfile)) + ',' +
    '"fingerprint":' + JsonQuote(Fingerprint);
  if AProfile.AllowInsecure then
    Result := Result + ',"allowInsecure":true';
  Result := Result + ',"alpn":' + JsonStringArray(Alpn) + '},' +
    '"hysteriaSettings":{"version":2,"auth":' +
    JsonQuote(EffectivePassword(AProfile)) + '}';
  if SameText(Trim(AProfile.Obfs), 'salamander') then
  begin
    if Trim(AProfile.ObfsPassword) = '' then
    begin
      AError := 'Hysteria2 salamander требует obfs-password.';
      Exit('');
    end;
    Result := Result + ',"finalmask":{"udp":[{"type":"salamander",' +
      '"settings":{"password":' + JsonQuote(Trim(AProfile.ObfsPassword)) +
      '}}]}';
  end;
  Result := Result + '}';
end;

function BuildProxyOutbound(const AProfile: TZaryaProfile;
  out AError: string): string;
var
  Encryption: string;
  User: string;
  Password: string;
  Method: string;
  SecurityCipher: string;
  Settings: string;
  StreamSettings: string;
  ReservedJson: string;
begin
  AError := '';
  Encryption := Trim(AProfile.Encryption);
  if Encryption = '' then
    Encryption := 'none';
  Password := EffectivePassword(AProfile);

  if SameText(AProfile.ProtocolName, 'VLESS') then
  begin
    User := '{"id":' + JsonQuote(Trim(AProfile.Uuid)) +
      ',"encryption":' + JsonQuote(Encryption);
    if Trim(AProfile.Flow) <> '' then
      User := User + ',"flow":' + JsonQuote(Trim(AProfile.Flow));
    User := User + '}';
    Settings := '{"vnext":[{"address":' + JsonQuote(Trim(AProfile.Host)) +
      ',"port":' + IntToStr(AProfile.Port) + ',"users":[' + User + ']}]}';
    StreamSettings := BuildStreamSettings(AProfile);
    Result := '{"tag":"proxy","protocol":"vless","settings":' +
      Settings + ',"streamSettings":' + StreamSettings + '}';
  end
  else if SameText(AProfile.ProtocolName, 'VMess') then
  begin
    SecurityCipher := Trim(AProfile.SecurityCipher);
    if SecurityCipher = '' then
      SecurityCipher := 'auto';
    User := '{"id":' + JsonQuote(Trim(AProfile.Uuid)) + ',"alterId":' +
      IntToStr(AProfile.AlterId) + ',"security":' +
      JsonQuote(SecurityCipher) + '}';
    Settings := '{"vnext":[{"address":' + JsonQuote(Trim(AProfile.Host)) +
      ',"port":' + IntToStr(AProfile.Port) + ',"users":[' + User + ']}]}';
    Result := '{"tag":"proxy","protocol":"vmess","settings":' +
      Settings + ',"streamSettings":' + BuildStreamSettings(AProfile) + '}';
  end
  else if SameText(AProfile.ProtocolName, 'Trojan') then
  begin
    Settings := '{"servers":[{"address":' +
      JsonQuote(Trim(AProfile.Host)) + ',"port":' +
      IntToStr(AProfile.Port) + ',"password":' + JsonQuote(Password) + '}]}';
    Result := '{"tag":"proxy","protocol":"trojan","settings":' +
      Settings + ',"streamSettings":' + BuildStreamSettings(AProfile) + '}';
  end
  else if SameText(AProfile.ProtocolName, 'Shadowsocks') then
  begin
    Method := EffectiveMethod(AProfile);
    Settings := '{"servers":[{"address":' +
      JsonQuote(Trim(AProfile.Host)) + ',"port":' +
      IntToStr(AProfile.Port) + ',"method":' + JsonQuote(Method) +
      ',"password":' + JsonQuote(Password) + '}]}';
    Result := '{"tag":"proxy","protocol":"shadowsocks","settings":' +
      Settings + '}';
  end
  else if SameText(AProfile.ProtocolName, 'SOCKS') then
  begin
    Settings := '{"servers":[{"address":' +
      JsonQuote(Trim(AProfile.Host)) + ',"port":' +
      IntToStr(AProfile.Port);
    if Password <> '' then
    begin
      Settings := Settings + ',"users":[{';
      if Trim(AProfile.Uuid) <> '' then
        Settings := Settings + '"user":' +
          JsonQuote(Trim(AProfile.Uuid)) + ',';
      Settings := Settings + '"pass":' + JsonQuote(Password) + '}]';
    end;
    Settings := Settings + '}]}';
    Result := '{"tag":"proxy","protocol":"socks","settings":' +
      Settings + '}';
  end
  else if SameText(AProfile.ProtocolName, 'Hysteria2') then
  begin
    StreamSettings := BuildHysteriaStreamSettings(AProfile, AError);
    if AError <> '' then
      Exit('');
    Settings := '{"version":2,"address":' +
      JsonQuote(Trim(AProfile.Host)) + ',"port":' +
      IntToStr(AProfile.Port) + '}';
    Result := '{"tag":"proxy","protocol":"hysteria","settings":' +
      Settings + ',"streamSettings":' + StreamSettings + '}';
  end
  else if SameText(AProfile.ProtocolName, 'WireGuard') then
  begin
    if not TryReservedJson(AProfile.Reserved, ReservedJson) then
    begin
      AError := 'WireGuard reserved должен содержать 3 байта.';
      Exit('');
    end;
    Settings := '{"secretKey":' + JsonQuote(Password) + ',"peers":[{' +
      '"endpoint":' + JsonQuote(Trim(AProfile.Host) + ':' +
        IntToStr(AProfile.Port)) + ',"publicKey":' +
        JsonQuote(Trim(AProfile.PublicKey));
    if Trim(AProfile.PreSharedKey) <> '' then
      Settings := Settings + ',"preSharedKey":' +
        JsonQuote(Trim(AProfile.PreSharedKey));
    if AProfile.KeepAlive > 0 then
      Settings := Settings + ',"keepAlive":' + IntToStr(AProfile.KeepAlive);
    if Trim(AProfile.AllowedIps) <> '' then
      Settings := Settings + ',"allowedIPs":' +
        SplitJsonStringArray(AProfile.AllowedIps);
    Settings := Settings + '}],"noKernelTun":true';
    if Trim(AProfile.LocalAddress) <> '' then
      Settings := Settings + ',"address":' +
        SplitJsonStringArray(AProfile.LocalAddress);
    if AProfile.Mtu > 0 then
      Settings := Settings + ',"mtu":' + IntToStr(AProfile.Mtu);
    if ReservedJson <> '' then
      Settings := Settings + ',"reserved":' + ReservedJson;
    Settings := Settings + '}';
    Result := '{"tag":"proxy","protocol":"wireguard","settings":' +
      Settings + '}';
  end
  else
  begin
    AError := 'Xray adapter не поддерживает протокол ' +
      AProfile.ProtocolName + '.';
    Result := '';
  end;
end;

function GenerateXrayConfig(const AProfile: TZaryaProfile;
  const AMixedPort: Integer; out AJson, AError: string): Boolean;
var
  ProxyOutbound: string;
  NL: string;
begin
  AJson := '';
  AError := '';
  if not ValidateProfile(AProfile, AError) then
    Exit(False);
  if (AMixedPort < 1) or (AMixedPort > 65535) then
  begin
    AError := 'Mixed-порт должен быть в диапазоне от 1 до 65535.';
    Exit(False);
  end;

  ProxyOutbound := BuildProxyOutbound(AProfile, AError);
  if (ProxyOutbound = '') or (AError <> '') then
    Exit(False);
  NL := LineEnding;
  AJson := '{' + NL +
    '  "log": {"loglevel":"warning"},' + NL +
    '  "inbounds": [' + NL +
    '    {"listen":"127.0.0.1","port":' + IntToStr(AMixedPort) +
      ',"protocol":"mixed","tag":"mixed-in","settings":{"udp":true}}' + NL +
    '  ],' + NL +
    '  "outbounds": [' + NL +
    '    ' + ProxyOutbound + ',' + NL +
    '    {"tag":"direct","protocol":"freedom"},' + NL +
    '    {"tag":"block","protocol":"blackhole"}' + NL +
    '  ],' + NL +
    '  "routing": {"domainStrategy":"AsIs","rules":[' +
      '{"type":"field","network":"tcp,udp","outboundTag":"proxy"}]}' + NL +
    '}' + NL;
  Result := True;
end;

end.
