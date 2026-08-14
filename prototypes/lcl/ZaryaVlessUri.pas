unit ZaryaVlessUri;

{$IFDEF FPC}
{$mode delphi}{$H+}
{$ENDIF}

interface

uses
  SysUtils, ZaryaProfile;

function ParseVlessUri(const AUri: string; out AProfile: TZaryaProfile;
  out AError: string): Boolean;

implementation

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
  {$IFDEF FPC}
  Bytes := RawByteString(AValue);
  {$ELSE}
  Bytes := UTF8Encode(AValue);
  {$ENDIF}
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
  {$IFDEF FPC}
  Result := string(Decoded);
  {$ELSE}
  Result := UTF8ToString(UTF8String(Decoded));
  {$ENDIF}
end;

procedure SetQueryValue(var AProfile: TZaryaProfile; const AName,
  AValue: string);
var
  Name: string;
  Value: string;
begin
  Name := LowerCase(PercentDecode(AName));
  Value := PercentDecode(AValue);
  if Name = 'type' then
    AProfile.Network := LowerCase(Value)
  else if Name = 'security' then
    AProfile.Security := LowerCase(Value)
  else if Name = 'pbk' then
    AProfile.PublicKey := Value
  else if Name = 'fp' then
    AProfile.Fingerprint := Value
  else if Name = 'sni' then
  begin
    AProfile.ServerName := Value;
    AProfile.Sni := Value;
  end
  else if Name = 'sid' then
    AProfile.ShortId := Value
  else if Name = 'spx' then
    AProfile.SpiderX := Value
  else if Name = 'flow' then
    AProfile.Flow := Value
  else if Name = 'encryption' then
    AProfile.Encryption := Value
  else if Name = 'host' then
    AProfile.TransportHost := Value
  else if Name = 'path' then
    AProfile.Path := Value
  else if Name = 'headertype' then
    AProfile.HeaderType := Value
  else if Name = 'servicename' then
    AProfile.ServiceName := Value
  else if Name = 'alpn' then
    AProfile.Alpn := Value
  else if (Name = 'allowinsecure') or (Name = 'insecure') then
    AProfile.AllowInsecure := SameText(Value, 'true') or (Value = '1');
end;

procedure ParseQuery(var AProfile: TZaryaProfile; const AQuery: string);
var
  StartIndex: Integer;
  EndIndex: Integer;
  EqualIndex: Integer;
  Pair: string;
  Name: string;
  Value: string;
begin
  StartIndex := 1;
  while StartIndex <= Length(AQuery) do
  begin
    EndIndex := StartIndex;
    while (EndIndex <= Length(AQuery)) and (AQuery[EndIndex] <> '&') do
      Inc(EndIndex);
    Pair := Copy(AQuery, StartIndex, EndIndex - StartIndex);
    EqualIndex := Pos('=', Pair);
    if EqualIndex > 0 then
    begin
      Name := Copy(Pair, 1, EqualIndex - 1);
      Value := Copy(Pair, EqualIndex + 1, MaxInt);
    end
    else
    begin
      Name := Pair;
      Value := '';
    end;
    if Name <> '' then
      SetQueryValue(AProfile, Name, Value);
    StartIndex := EndIndex + 1;
  end;
end;

function ParseAuthority(const AAuthority: string; out AUser, AHost: string;
  out APort: Integer; out AError: string): Boolean;
var
  AtIndex: Integer;
  HostPort: string;
  CloseBracket: Integer;
  ColonIndex: Integer;
  I: Integer;
  PortText: string;
begin
  Result := False;
  AError := '';
  APort := 443;
  AtIndex := 0;
  for I := Length(AAuthority) downto 1 do
    if AAuthority[I] = '@' then
    begin
      AtIndex := I;
      Break;
    end;
  if AtIndex = 0 then
  begin
    AError := 'VLESS URI не содержит UUID перед символом @.';
    Exit;
  end;
  AUser := PercentDecode(Copy(AAuthority, 1, AtIndex - 1));
  HostPort := Copy(AAuthority, AtIndex + 1, MaxInt);
  if HostPort = '' then
  begin
    AError := 'VLESS URI не содержит адрес сервера.';
    Exit;
  end;

  if HostPort[1] = '[' then
  begin
    CloseBracket := Pos(']', HostPort);
    if CloseBracket = 0 then
    begin
      AError := 'Некорректный IPv6-адрес в VLESS URI.';
      Exit;
    end;
    AHost := Copy(HostPort, 2, CloseBracket - 2);
    if CloseBracket < Length(HostPort) then
    begin
      if HostPort[CloseBracket + 1] <> ':' then
      begin
        AError := 'После IPv6-адреса ожидался порт.';
        Exit;
      end;
      PortText := Copy(HostPort, CloseBracket + 2, MaxInt);
      APort := StrToIntDef(PortText, 0);
    end;
  end
  else
  begin
    ColonIndex := 0;
    for I := Length(HostPort) downto 1 do
      if HostPort[I] = ':' then
      begin
        ColonIndex := I;
        Break;
      end;
    if ColonIndex > 0 then
    begin
      AHost := Copy(HostPort, 1, ColonIndex - 1);
      PortText := Copy(HostPort, ColonIndex + 1, MaxInt);
      APort := StrToIntDef(PortText, 0);
    end
    else
      AHost := HostPort;
  end;
  AHost := PercentDecode(AHost);
  if Trim(AHost) = '' then
    AError := 'VLESS URI не содержит адрес сервера.'
  else if Trim(AUser) = '' then
    AError := 'VLESS URI не содержит UUID.'
  else if (APort < 1) or (APort > 65535) then
    AError := 'Порт VLESS URI должен быть в диапазоне от 1 до 65535.';
  Result := AError = '';
end;

function ParseVlessUri(const AUri: string; out AProfile: TZaryaProfile;
  out AError: string): Boolean;
var
  Work: string;
  Fragment: string;
  Query: string;
  Authority: string;
  Marker: Integer;
begin
  AProfile := CreateEmptyProfile;
  AError := '';
  Work := Trim(AUri);
  if not SameText(Copy(Work, 1, 8), 'vless://') then
  begin
    AError := 'Поддерживаются только ссылки vless://.';
    Exit(False);
  end;
  Delete(Work, 1, 8);

  Marker := Pos('#', Work);
  if Marker > 0 then
  begin
    Fragment := Copy(Work, Marker + 1, MaxInt);
    Delete(Work, Marker, MaxInt);
  end
  else
    Fragment := '';

  Marker := Pos('?', Work);
  if Marker > 0 then
  begin
    Query := Copy(Work, Marker + 1, MaxInt);
    Authority := Copy(Work, 1, Marker - 1);
  end
  else
  begin
    Query := '';
    Authority := Work;
  end;

  if not ParseAuthority(Authority, AProfile.Uuid, AProfile.Host,
    AProfile.Port, AError) then
    Exit(False);
  ParseQuery(AProfile, Query);
  if Trim(AProfile.Encryption) = '' then
    AProfile.Encryption := 'none';
  if Trim(AProfile.Network) = '' then
    AProfile.Network := 'tcp';
  if (Trim(AProfile.Security) = '') and (Trim(AProfile.PublicKey) <> '') then
    AProfile.Security := 'reality';
  if Trim(AProfile.Fingerprint) = '' then
    AProfile.Fingerprint := 'chrome';
  if Trim(AProfile.SpiderX) = '' then
    AProfile.SpiderX := '/';

  AProfile.Name := Trim(PercentDecode(Fragment));
  if AProfile.Name = '' then
    AProfile.Name := AProfile.Host + ':' + IntToStr(AProfile.Port);
  AProfile.ProtocolName := 'VLESS';
  AProfile.Source := 'VLESS URI';
  AProfile.Latency := '—';
  AProfile.Enabled := True;
  Result := ValidateProfile(AProfile, AError);
end;

end.
