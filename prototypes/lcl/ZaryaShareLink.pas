unit ZaryaShareLink;

{$mode objfpc}{$H+}

interface

uses
  SysUtils, ZaryaProfile;

function IsSupportedShareLink(const ALink: string): Boolean;
function ParseShareLink(const ALink: string; out AProfile: TZaryaProfile;
  out AError: string): Boolean;

implementation

uses
  Classes, fpjson, jsonparser, base64, ZaryaVlessUri;

function HexValue(const C: AnsiChar): Integer;
begin
  case C of
    '0'..'9': Result := Ord(C) - Ord('0');
    'a'..'f': Result := Ord(C) - Ord('a') + 10;
    'A'..'F': Result := Ord(C) - Ord('A') + 10;
  else
    Result := -1;
  end;
end;

function PercentDecode(const AValue: string): string;
var
  Bytes: RawByteString;
  Decoded: RawByteString;
  I: Integer;
  HiValue: Integer;
  LoValue: Integer;
begin
  Bytes := RawByteString(AValue);
  Decoded := '';
  I := 1;
  while I <= Length(Bytes) do
  begin
    if (Bytes[I] = '%') and (I + 2 <= Length(Bytes)) then
    begin
      HiValue := HexValue(AnsiChar(Bytes[I + 1]));
      LoValue := HexValue(AnsiChar(Bytes[I + 2]));
      if (HiValue >= 0) and (LoValue >= 0) then
      begin
        Decoded := Decoded + AnsiChar((HiValue shl 4) or LoValue);
        Inc(I, 3);
        Continue;
      end;
    end;
    Decoded := Decoded + Bytes[I];
    Inc(I);
  end;
  Result := string(Decoded);
end;

function DecodeBase64Flexible(const AValue: string): string;
var
  Normalized: string;
begin
  Normalized := StringReplace(Trim(AValue), '-', '+', [rfReplaceAll]);
  Normalized := StringReplace(Normalized, '_', '/', [rfReplaceAll]);
  Normalized := StringReplace(Normalized, ' ', '', [rfReplaceAll]);
  Normalized := StringReplace(Normalized, #9, '', [rfReplaceAll]);
  Normalized := StringReplace(Normalized, #10, '', [rfReplaceAll]);
  Normalized := StringReplace(Normalized, #13, '', [rfReplaceAll]);
  while (Length(Normalized) mod 4) <> 0 do
    Normalized := Normalized + '=';
  try
    Result := DecodeStringBase64(Normalized);
  except
    Result := '';
  end;
end;

function NewImportedProfile(const AProtocol: string): TZaryaProfile;
begin
  Result := CreateEmptyProfile;
  Result.ProtocolName := AProtocol;
  Result.PreferredProviderId := 'embedded.xray';
  Result.Source := 'Share link';
  Result.SourceType := 'manual';
  Result.Enabled := True;
end;

procedure SplitFragmentAndQuery(const AValue: string; out AAuthority,
  AQuery, AFragment: string);
var
  Work: string;
  Marker: Integer;
begin
  Work := AValue;
  Marker := Pos('#', Work);
  if Marker > 0 then
  begin
    AFragment := Copy(Work, Marker + 1, MaxInt);
    Delete(Work, Marker, MaxInt);
  end
  else
    AFragment := '';
  Marker := Pos('?', Work);
  if Marker > 0 then
  begin
    AQuery := Copy(Work, Marker + 1, MaxInt);
    AAuthority := Copy(Work, 1, Marker - 1);
  end
  else
  begin
    AQuery := '';
    AAuthority := Work;
  end;
end;

function ParseHostPort(AHostPort: string; const ADefaultPort: Integer;
  out AHost: string; out APort: Integer): Boolean;
var
  CloseBracket: Integer;
  ColonIndex: Integer;
  SlashIndex: Integer;
  I: Integer;
begin
  AHost := '';
  APort := ADefaultPort;
  SlashIndex := Pos('/', AHostPort);
  if SlashIndex > 0 then
    AHostPort := Copy(AHostPort, 1, SlashIndex - 1);
  AHostPort := Trim(AHostPort);
  if AHostPort = '' then
    Exit(False);
  if AHostPort[1] = '[' then
  begin
    CloseBracket := Pos(']', AHostPort);
    if CloseBracket <= 1 then
      Exit(False);
    AHost := Copy(AHostPort, 2, CloseBracket - 2);
    if CloseBracket < Length(AHostPort) then
    begin
      if AHostPort[CloseBracket + 1] <> ':' then
        Exit(False);
      APort := StrToIntDef(Copy(AHostPort, CloseBracket + 2, MaxInt), 0);
    end;
  end
  else
  begin
    ColonIndex := 0;
    for I := Length(AHostPort) downto 1 do
      if AHostPort[I] = ':' then
      begin
        ColonIndex := I;
        Break;
      end;
    if ColonIndex > 0 then
    begin
      AHost := Copy(AHostPort, 1, ColonIndex - 1);
      APort := StrToIntDef(Copy(AHostPort, ColonIndex + 1, MaxInt), 0);
    end
    else
      AHost := AHostPort;
  end;
  AHost := PercentDecode(AHost);
  Result := (Trim(AHost) <> '') and (APort >= 1) and (APort <= 65535);
end;

function ParseStandardUri(const ALink: string; const ADefaultPort: Integer;
  out AUserInfo, AHost: string; out APort: Integer; out AQuery,
  AFragment: string): Boolean;
var
  Marker: Integer;
  Authority: string;
  AtIndex: Integer;
  I: Integer;
begin
  Marker := Pos('://', ALink);
  if Marker = 0 then
    Exit(False);
  SplitFragmentAndQuery(Copy(ALink, Marker + 3, MaxInt), Authority,
    AQuery, AFragment);
  AtIndex := 0;
  for I := Length(Authority) downto 1 do
    if Authority[I] = '@' then
    begin
      AtIndex := I;
      Break;
    end;
  if AtIndex > 0 then
  begin
    AUserInfo := PercentDecode(Copy(Authority, 1, AtIndex - 1));
    Authority := Copy(Authority, AtIndex + 1, MaxInt);
  end
  else
    AUserInfo := '';
  Result := ParseHostPort(Authority, ADefaultPort, AHost, APort);
end;

procedure ApplyQueryValue(var AProfile: TZaryaProfile; const AName,
  AValue: string);
var
  Name: string;
  Value: string;
begin
  Name := LowerCase(PercentDecode(AName));
  Value := PercentDecode(AValue);
  if Name = 'type' then AProfile.Network := LowerCase(Value)
  else if Name = 'security' then AProfile.Security := LowerCase(Value)
  else if Name = 'pbk' then AProfile.PublicKey := Value
  else if Name = 'fp' then AProfile.Fingerprint := Value
  else if Name = 'sni' then
  begin
    AProfile.ServerName := Value;
    AProfile.Sni := Value;
  end
  else if Name = 'sid' then AProfile.ShortId := Value
  else if Name = 'spx' then AProfile.SpiderX := Value
  else if Name = 'flow' then AProfile.Flow := Value
  else if Name = 'encryption' then AProfile.Encryption := Value
  else if Name = 'host' then AProfile.TransportHost := Value
  else if Name = 'path' then AProfile.Path := Value
  else if Name = 'headertype' then AProfile.HeaderType := Value
  else if Name = 'servicename' then AProfile.ServiceName := Value
  else if Name = 'alpn' then AProfile.Alpn := Value
  else if Name = 'obfs' then AProfile.Obfs := Value
  else if (Name = 'obfs-password') or (Name = 'obfspassword') then
    AProfile.ObfsPassword := Value
  else if (Name = 'auth') or (Name = 'password') then
  begin
    if AProfile.Password = '' then AProfile.Password := Value;
    if AProfile.Uuid = '' then AProfile.Uuid := Value;
  end
  else if (Name = 'publickey') or (Name = 'peerpublickey') then
    AProfile.PublicKey := Value
  else if ((Name = 'address') or (Name = 'localaddress') or
    (Name = 'ip')) and SameText(AProfile.ProtocolName, 'WireGuard') then
    AProfile.LocalAddress := Value
  else if Name = 'mtu' then AProfile.Mtu := StrToIntDef(Value, 0)
  else if Name = 'reserved' then AProfile.Reserved := Value
  else if (Name = 'presharedkey') or (Name = 'psk') then
    AProfile.PreSharedKey := Value
  else if (Name = 'allowedips') or (Name = 'allowedip') then
    AProfile.AllowedIps := Value
  else if (Name = 'keepalive') or (Name = 'persistentkeepalive') then
    AProfile.KeepAlive := StrToIntDef(Value, 0)
  else if (Name = 'privatekey') or (Name = 'secretkey') then
  begin
    if AProfile.Password = '' then AProfile.Password := Value;
    if AProfile.Uuid = '' then AProfile.Uuid := Value;
  end
  else if (Name = 'allowinsecure') or (Name = 'insecure') then
    AProfile.AllowInsecure := (Value = '1') or SameText(Value, 'true');
end;

procedure ApplyQuery(var AProfile: TZaryaProfile; const AQuery: string);
var
  Pairs: TStringList;
  I: Integer;
  Marker: Integer;
  Name: string;
  Value: string;
begin
  Pairs := TStringList.Create;
  try
    Pairs.StrictDelimiter := True;
    Pairs.Delimiter := '&';
    Pairs.DelimitedText := AQuery;
    for I := 0 to Pairs.Count - 1 do
    begin
      Marker := Pos('=', Pairs[I]);
      if Marker > 0 then
      begin
        Name := Copy(Pairs[I], 1, Marker - 1);
        Value := Copy(Pairs[I], Marker + 1, MaxInt);
      end
      else
      begin
        Name := Pairs[I];
        Value := '';
      end;
      if Name <> '' then
        ApplyQueryValue(AProfile, Name, Value);
    end;
  finally
    Pairs.Free;
  end;
end;

procedure FinishImportedProfile(var AProfile: TZaryaProfile;
  const AFragment: string);
begin
  AProfile.Name := Trim(PercentDecode(AFragment));
  if AProfile.Name = '' then
    AProfile.Name := AProfile.Host + ':' + IntToStr(AProfile.Port);
end;

function JsonString(const AObject: TJSONObject; const AName,
  ADefault: string): string;
var
  Data: TJSONData;
begin
  Data := AObject.Find(AName);
  if Assigned(Data) then Result := Data.AsString else Result := ADefault;
end;

function JsonInteger(const AObject: TJSONObject; const AName: string;
  const ADefault: Integer): Integer;
var
  Data: TJSONData;
begin
  Data := AObject.Find(AName);
  if not Assigned(Data) then Exit(ADefault);
  Result := StrToIntDef(Data.AsString, ADefault);
end;

function ParseVmess(const ALink: string; out AProfile: TZaryaProfile;
  out AError: string): Boolean;
var
  Data: TJSONData;
  Obj: TJSONObject;
  Payload: string;
begin
  Result := False;
  Data := nil;
  Payload := DecodeBase64Flexible(Copy(ALink, Length('vmess://') + 1,
    MaxInt));
  if Payload = '' then
  begin
    AError := 'Некорректный base64 payload VMess.';
    Exit;
  end;
  try
    Data := GetJSON(Payload);
    if Data.JSONType <> jtObject then
      raise Exception.Create('VMess payload должен быть JSON object.');
    Obj := TJSONObject(Data);
    AProfile := NewImportedProfile('VMess');
    AProfile.Name := JsonString(Obj, 'ps', '');
    AProfile.Host := JsonString(Obj, 'add', '');
    AProfile.Port := JsonInteger(Obj, 'port', 443);
    AProfile.Uuid := JsonString(Obj, 'id', '');
    AProfile.AlterId := JsonInteger(Obj, 'aid', 0);
    AProfile.Network := JsonString(Obj, 'net', 'tcp');
    AProfile.HeaderType := JsonString(Obj, 'type', '');
    AProfile.TransportHost := JsonString(Obj, 'host', '');
    AProfile.Path := JsonString(Obj, 'path', '');
    AProfile.Security := JsonString(Obj, 'tls', '');
    AProfile.ServerName := JsonString(Obj, 'sni', '');
    AProfile.Sni := AProfile.ServerName;
    AProfile.Fingerprint := JsonString(Obj, 'fp', 'chrome');
    AProfile.SecurityCipher := JsonString(Obj, 'scy', 'auto');
    if (AProfile.Host = '') or (AProfile.Uuid = '') or
      (AProfile.Port < 1) or (AProfile.Port > 65535) then
      raise Exception.Create('VMess link содержит неполные поля.');
    if AProfile.Name = '' then
      AProfile.Name := AProfile.Host + ':' + IntToStr(AProfile.Port);
    Result := True;
  except
    on E: Exception do AError := E.Message;
  end;
  Data.Free;
end;

function ParseTrojan(const ALink: string; out AProfile: TZaryaProfile;
  out AError: string): Boolean;
var
  UserInfo: string;
  Query: string;
  Fragment: string;
begin
  AProfile := NewImportedProfile('Trojan');
  Result := ParseStandardUri(ALink, 443, UserInfo, AProfile.Host,
    AProfile.Port, Query, Fragment);
  if not Result then
  begin
    AError := 'Некорректный Trojan host или port.';
    Exit;
  end;
  AProfile.Password := UserInfo;
  AProfile.Uuid := UserInfo;
  ApplyQuery(AProfile, Query);
  if (AProfile.Password = '') or (AProfile.Host = '') then
  begin
    AError := 'Trojan link не содержит пароль или адрес.';
    Exit(False);
  end;
  if (AProfile.Security = '') and (AProfile.PublicKey <> '') then
    AProfile.Security := 'reality'
  else if (AProfile.Security = '') and (AProfile.Port = 443) then
    AProfile.Security := 'tls';
  FinishImportedProfile(AProfile, Fragment);
end;

function ParseHysteria2(const ALink: string; out AProfile: TZaryaProfile;
  out AError: string): Boolean;
var
  UserInfo: string;
  Query: string;
  Fragment: string;
begin
  AProfile := NewImportedProfile('Hysteria2');
  AProfile.Network := 'hysteria';
  Result := ParseStandardUri(ALink, 443, UserInfo, AProfile.Host,
    AProfile.Port, Query, Fragment);
  if not Result then
  begin
    AError := 'Некорректный Hysteria2 host или port.';
    Exit;
  end;
  AProfile.Password := UserInfo;
  AProfile.Uuid := UserInfo;
  ApplyQuery(AProfile, Query);
  if AProfile.Password = '' then
  begin
    AError := 'Hysteria2 link не содержит auth.';
    Exit(False);
  end;
  AProfile.Security := 'tls';
  if not ((AProfile.Obfs = '') or SameText(AProfile.Obfs, 'none') or
    SameText(AProfile.Obfs, 'salamander')) then
    AProfile.UnsupportedReason := 'Неподдерживаемый Hysteria2 obfs: ' +
      AProfile.Obfs;
  if SameText(AProfile.Obfs, 'salamander') and
    (AProfile.ObfsPassword = '') then
    AProfile.UnsupportedReason :=
      'Hysteria2 salamander требует obfs-password.';
  FinishImportedProfile(AProfile, Fragment);
end;

function ParseWireGuard(const ALink: string; out AProfile: TZaryaProfile;
  out AError: string): Boolean;
var
  UserInfo: string;
  Query: string;
  Fragment: string;
begin
  AProfile := NewImportedProfile('WireGuard');
  AProfile.Network := 'udp';
  Result := ParseStandardUri(ALink, 51820, UserInfo, AProfile.Host,
    AProfile.Port, Query, Fragment);
  if not Result then
  begin
    AError := 'Некорректный WireGuard host или port.';
    Exit;
  end;
  AProfile.Password := UserInfo;
  AProfile.Uuid := UserInfo;
  ApplyQuery(AProfile, Query);
  if AProfile.Password = '' then
  begin
    AError := 'WireGuard link не содержит private key.';
    Exit(False);
  end;
  if AProfile.PublicKey = '' then
  begin
    AError := 'WireGuard link не содержит peer public key.';
    Exit(False);
  end;
  FinishImportedProfile(AProfile, Fragment);
end;

function ParseSocks(const ALink: string; out AProfile: TZaryaProfile;
  out AError: string): Boolean;
var
  UserInfo: string;
  Query: string;
  Fragment: string;
  Marker: Integer;
begin
  AProfile := NewImportedProfile('SOCKS');
  Result := ParseStandardUri(ALink, 1080, UserInfo, AProfile.Host,
    AProfile.Port, Query, Fragment);
  if not Result then
  begin
    AError := 'Некорректный SOCKS host или port.';
    Exit;
  end;
  Marker := Pos(':', UserInfo);
  if Marker > 0 then
  begin
    AProfile.Uuid := Copy(UserInfo, 1, Marker - 1);
    AProfile.Password := Copy(UserInfo, Marker + 1, MaxInt);
  end
  else
    AProfile.Password := UserInfo;
  ApplyQuery(AProfile, Query);
  FinishImportedProfile(AProfile, Fragment);
end;

function ParseShadowsocks(const ALink: string; out AProfile: TZaryaProfile;
  out AError: string): Boolean;
var
  Remainder: string;
  Query: string;
  Fragment: string;
  Authority: string;
  UserPart: string;
  UserInfo: string;
  HostPart: string;
  Decoded: string;
  Marker: Integer;
  AtIndex: Integer;
begin
  Result := False;
  AProfile := NewImportedProfile('Shadowsocks');
  Remainder := Copy(ALink, Length('ss://') + 1, MaxInt);
  SplitFragmentAndQuery(Remainder, Authority, Query, Fragment);
  AtIndex := Pos('@', Authority);
  if AtIndex > 0 then
  begin
    UserPart := Copy(Authority, 1, AtIndex - 1);
    HostPart := Copy(Authority, AtIndex + 1, MaxInt);
    UserInfo := PercentDecode(UserPart);
    if Pos(':', UserInfo) = 0 then
      UserInfo := DecodeBase64Flexible(UserPart);
  end
  else
  begin
    Decoded := DecodeBase64Flexible(Authority);
    AtIndex := Pos('@', Decoded);
    if AtIndex <= 0 then
    begin
      AError := 'Неподдерживаемый формат Shadowsocks link.';
      Exit;
    end;
    UserInfo := Copy(Decoded, 1, AtIndex - 1);
    HostPart := Copy(Decoded, AtIndex + 1, MaxInt);
  end;
  Marker := Pos(':', UserInfo);
  if Marker <= 1 then
  begin
    AError := 'Shadowsocks link содержит некорректные credentials.';
    Exit;
  end;
  AProfile.Method := PercentDecode(Copy(UserInfo, 1, Marker - 1));
  AProfile.Encryption := AProfile.Method;
  AProfile.Password := PercentDecode(Copy(UserInfo, Marker + 1, MaxInt));
  AProfile.Uuid := AProfile.Password;
  if not ParseHostPort(HostPart, 0, AProfile.Host, AProfile.Port) then
  begin
    AError := 'Shadowsocks link содержит некорректный host или port.';
    Exit;
  end;
  if Pos('plugin=', LowerCase(Query)) > 0 then
    AProfile.UnsupportedReason :=
      'Shadowsocks plugin options пока не поддерживаются.';
  ApplyQuery(AProfile, Query);
  FinishImportedProfile(AProfile, Fragment);
  Result := True;
end;

function IsSupportedShareLink(const ALink: string): Boolean;
var
  Link: string;
begin
  Link := LowerCase(Trim(ALink));
  Result := (Pos('vless://', Link) = 1) or
    (Pos('vmess://', Link) = 1) or (Pos('trojan://', Link) = 1) or
    (Pos('ss://', Link) = 1) or (Pos('socks://', Link) = 1) or
    (Pos('hysteria2://', Link) = 1) or (Pos('hy2://', Link) = 1) or
    (Pos('wireguard://', Link) = 1) or (Pos('wg://', Link) = 1);
end;

function ParseShareLink(const ALink: string; out AProfile: TZaryaProfile;
  out AError: string): Boolean;
var
  Link: string;
begin
  Link := Trim(ALink);
  AProfile := CreateEmptyProfile;
  AError := '';
  if Pos('vless://', LowerCase(Link)) = 1 then
    Exit(ParseVlessUri(Link, AProfile, AError));
  if Pos('vmess://', LowerCase(Link)) = 1 then
    Exit(ParseVmess(Link, AProfile, AError));
  if Pos('trojan://', LowerCase(Link)) = 1 then
    Exit(ParseTrojan(Link, AProfile, AError));
  if Pos('ss://', LowerCase(Link)) = 1 then
    Exit(ParseShadowsocks(Link, AProfile, AError));
  if Pos('socks://', LowerCase(Link)) = 1 then
    Exit(ParseSocks(Link, AProfile, AError));
  if (Pos('hysteria2://', LowerCase(Link)) = 1) or
    (Pos('hy2://', LowerCase(Link)) = 1) then
    Exit(ParseHysteria2(Link, AProfile, AError));
  if (Pos('wireguard://', LowerCase(Link)) = 1) or
    (Pos('wg://', LowerCase(Link)) = 1) then
    Exit(ParseWireGuard(Link, AProfile, AError));
  AError := 'Неподдерживаемая схема share link.';
  Result := False;
end;

end.
