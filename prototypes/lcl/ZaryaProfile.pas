unit ZaryaProfile;

{$IFDEF FPC}
{$mode delphi}{$H+}
{$ENDIF}

interface

uses
  SysUtils;

type
  TZaryaProfile = record
    Id: string;
    Name: string;
    ProtocolName: string;
    Host: string;
    Port: Integer;
    Uuid: string;
    Password: string;
    Encryption: string;
    Method: string;
    SecurityCipher: string;
    Flow: string;
    Remark: string;
    Network: string;
    TransportHost: string;
    Path: string;
    HeaderType: string;
    ServiceName: string;
    Security: string;
    ServerName: string;
    Sni: string;
    Fingerprint: string;
    PublicKey: string;
    ShortId: string;
    SpiderX: string;
    Alpn: string;
    Obfs: string;
    ObfsPassword: string;
    AllowInsecure: Boolean;
    LocalAddress: string;
    AllowedIps: string;
    PreSharedKey: string;
    Reserved: string;
    Mtu: Integer;
    KeepAlive: Integer;
    AlterId: Integer;
    PreferredProviderId: string;
    RawConfig: string;
    RawConfigFormat: string;
    ReadinessHost: string;
    ReadinessPort: Integer;
    SystemProxyKind: string;
    SourceType: string;
    SubscriptionId: string;
    SubscriptionName: string;
    SourceKey: string;
    LastSeenAt: string;
    DeletedBySubscriptionUpdate: Boolean;
    UnsupportedReason: string;
    LastTcpPingMs: Integer;
    LastRealDelayMs: Integer;
    LastTestStatus: string;
    LastTestError: string;
    LastTestedAt: string;
    Source: string;
    Latency: string;
    Enabled: Boolean;
  end;

  TZaryaProfiles = array of TZaryaProfile;

function CreateEmptyProfile: TZaryaProfile;
function CreateDemoProfiles: TZaryaProfiles;
function ProfileEndpoint(const AProfile: TZaryaProfile): string;
function EffectiveServerName(const AProfile: TZaryaProfile): string;
function EffectivePassword(const AProfile: TZaryaProfile): string;
function EffectiveMethod(const AProfile: TZaryaProfile): string;
function NormalizedNetwork(const AProfile: TZaryaProfile): string;
function ComputeProfileSourceKey(const AProfile: TZaryaProfile): string;
function ValidateProfile(const AProfile: TZaryaProfile; out AError: string): Boolean;

implementation

function NewProfileId: string;
begin
  Result := FormatDateTime('yyyymmddhhnnsszzz', Now) + '-' +
    IntToHex(Random(MaxInt), 8);
end;

function CreateEmptyProfile: TZaryaProfile;
begin
  Result.Id := NewProfileId;
  Result.Name := 'Новый профиль';
  Result.ProtocolName := 'VLESS';
  Result.Host := '';
  Result.Port := 443;
  Result.Uuid := '';
  Result.Password := '';
  Result.Encryption := 'none';
  Result.Method := '';
  Result.SecurityCipher := '';
  Result.Flow := '';
  Result.Remark := '';
  Result.Network := 'tcp';
  Result.TransportHost := '';
  Result.Path := '';
  Result.HeaderType := '';
  Result.ServiceName := '';
  Result.Security := '';
  Result.ServerName := '';
  Result.Sni := '';
  Result.Fingerprint := 'chrome';
  Result.PublicKey := '';
  Result.ShortId := '';
  Result.SpiderX := '/';
  Result.Alpn := '';
  Result.Obfs := '';
  Result.ObfsPassword := '';
  Result.AllowInsecure := False;
  Result.LocalAddress := '';
  Result.AllowedIps := '';
  Result.PreSharedKey := '';
  Result.Reserved := '';
  Result.Mtu := 0;
  Result.KeepAlive := 0;
  Result.AlterId := 0;
  Result.PreferredProviderId := 'embedded.xray';
  Result.RawConfig := '';
  Result.RawConfigFormat := '';
  Result.ReadinessHost := '127.0.0.1';
  Result.ReadinessPort := 0;
  Result.SystemProxyKind := 'mixed';
  Result.SourceType := 'manual';
  Result.SubscriptionId := '';
  Result.SubscriptionName := '';
  Result.SourceKey := '';
  Result.LastSeenAt := '';
  Result.DeletedBySubscriptionUpdate := False;
  Result.UnsupportedReason := '';
  Result.LastTcpPingMs := -1;
  Result.LastRealDelayMs := -1;
  Result.LastTestStatus := 'never_tested';
  Result.LastTestError := '';
  Result.LastTestedAt := '';
  Result.Source := 'Вручную';
  Result.Latency := '—';
  Result.Enabled := True;
end;

function DemoProfile(const AName, AProtocol, AHost: string; const APort: Integer;
  const ALatency, ASource: string; const AEnabled: Boolean): TZaryaProfile;
begin
  Result := CreateEmptyProfile;
  Result.Name := AName;
  Result.ProtocolName := AProtocol;
  Result.Host := AHost;
  Result.Port := APort;
  Result.Latency := ALatency;
  Result.Source := ASource;
  Result.Enabled := AEnabled;
  if SameText(AProtocol, 'VLESS') then
    Result.Uuid := '11111111-1111-1111-1111-111111111111';
end;

function EffectiveServerName(const AProfile: TZaryaProfile): string;
begin
  Result := Trim(AProfile.ServerName);
  if Result = '' then
    Result := Trim(AProfile.Sni);
  if Result = '' then
    Result := Trim(AProfile.Host);
end;

function EffectivePassword(const AProfile: TZaryaProfile): string;
begin
  Result := Trim(AProfile.Password);
  if Result = '' then
    Result := Trim(AProfile.Uuid);
end;

function EffectiveMethod(const AProfile: TZaryaProfile): string;
begin
  Result := Trim(AProfile.Method);
  if Result = '' then
    Result := Trim(AProfile.Encryption);
end;

function NormalizedNetwork(const AProfile: TZaryaProfile): string;
begin
  Result := LowerCase(Trim(AProfile.Network));
  if (Result = '') or (Result = 'raw') then
    Result := 'tcp';
end;

function ComputeProfileSourceKey(const AProfile: TZaryaProfile): string;
begin
  if SameText(AProfile.ProtocolName, 'WireGuard') then
    Result := AProfile.ProtocolName + '|' + Trim(AProfile.Host) + '|' +
      IntToStr(AProfile.Port) + '|' + Trim(AProfile.PublicKey) + '|' +
      Trim(AProfile.LocalAddress)
  else
    Result := AProfile.ProtocolName + '|' + Trim(AProfile.Host) + '|' +
      IntToStr(AProfile.Port) + '|' + Trim(AProfile.Uuid) + '|' +
      EffectiveServerName(AProfile) + '|' + Trim(AProfile.Security);
end;

function CreateDemoProfiles: TZaryaProfiles;
begin
  Result := nil;
  SetLength(Result, 4);
  Result[0] := DemoProfile('Амстердам · Demo', 'VLESS',
    'nl.example.invalid', 443, '42 мс', 'Вручную', False);
  Result[1] := DemoProfile('Хельсинки · Demo', 'VLESS',
    'fi.example.invalid', 443, '58 мс', 'Nord Demo', False);
  Result[2] := DemoProfile('Сингапур · Demo', 'Trojan',
    'sg.example.invalid', 443, '184 мс', 'Asia Demo', False);
  Result[3] := DemoProfile('Локальная проверка', 'SOCKS',
    '127.0.0.1', 10808, '—', 'Вручную', False);
end;

function ProfileEndpoint(const AProfile: TZaryaProfile): string;
begin
  if AProfile.RawConfig <> '' then
    Exit('raw · ' + AProfile.ReadinessHost + ':' +
      IntToStr(AProfile.ReadinessPort));
  if AProfile.Host = '' then
    Exit('—');
  Result := AProfile.Host + ':' + IntToStr(AProfile.Port);
end;

function ValidateProfile(const AProfile: TZaryaProfile; out AError: string): Boolean;
begin
  AError := '';
  if Trim(AProfile.Name) = '' then
    AError := 'Введите название профиля.'
  else if Trim(AProfile.ProtocolName) = '' then
    AError := 'Выберите протокол.'
  else if (Trim(AProfile.RawConfig) = '') and not
    (SameText(AProfile.ProtocolName, 'VLESS') or
    SameText(AProfile.ProtocolName, 'VMess') or
    SameText(AProfile.ProtocolName, 'Trojan') or
    SameText(AProfile.ProtocolName, 'Shadowsocks') or
    SameText(AProfile.ProtocolName, 'SOCKS') or
    SameText(AProfile.ProtocolName, 'Hysteria2') or
    SameText(AProfile.ProtocolName, 'WireGuard')) then
    AError := 'Неподдерживаемый протокол: ' + AProfile.ProtocolName
  else if Trim(AProfile.PreferredProviderId) = '' then
    AError := 'Выберите runtime provider.'
  else if Trim(AProfile.UnsupportedReason) <> '' then
    AError := AProfile.UnsupportedReason
  else if (Trim(AProfile.RawConfig) <> '') and
    (Trim(AProfile.RawConfigFormat) = '') then
    AError := 'Для raw config выберите диалект.'
  else if (Trim(AProfile.RawConfig) <> '') and
    (not SameText(AProfile.ReadinessHost, '127.0.0.1')) and
    (not SameText(AProfile.ReadinessHost, 'localhost')) then
    AError := 'Raw readiness endpoint должен быть локальным.'
  else if (Trim(AProfile.RawConfig) <> '') and
    ((AProfile.ReadinessPort < 1) or (AProfile.ReadinessPort > 65535)) then
    AError := 'Для raw config укажите readiness port.'
  else if (Trim(AProfile.RawConfig) <> '') and
    not (SameText(AProfile.SystemProxyKind, 'mixed') or
      SameText(AProfile.SystemProxyKind, 'http') or
      SameText(AProfile.SystemProxyKind, 'socks') or
      SameText(AProfile.SystemProxyKind, 'none')) then
    AError := 'Неизвестный тип системного прокси raw-профиля.'
  else if (Trim(AProfile.RawConfig) = '') and (Trim(AProfile.Host) = '') then
    AError := 'Введите адрес сервера.'
  else if (AProfile.Port < 1) or (AProfile.Port > 65535) then
    AError := 'Порт должен быть в диапазоне от 1 до 65535.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName, 'VLESS') and
    (Trim(AProfile.Uuid) = '') then
    AError := 'Для VLESS требуется UUID.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName,
    'VMess') and (Trim(AProfile.Uuid) = '') then
    AError := 'Для VMess требуется UUID.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName,
    'VMess') and (AProfile.AlterId < 0) then
    AError := 'VMess alterId должен быть неотрицательным.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName,
    'VMess') and SameText(Trim(AProfile.Security), 'reality') then
    AError := 'VMess не поддерживает REALITY.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName,
    'VMess') and not ((Trim(AProfile.Security) = '') or
      SameText(Trim(AProfile.Security), 'none') or
      SameText(Trim(AProfile.Security), 'tls')) then
    AError := 'VMess поддерживает security none или tls.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName,
    'Trojan') and (EffectivePassword(AProfile) = '') then
    AError := 'Для Trojan требуется пароль.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName,
    'Trojan') and not ((Trim(AProfile.Security) = '') or
      SameText(Trim(AProfile.Security), 'none') or
      SameText(Trim(AProfile.Security), 'tls') or
      SameText(Trim(AProfile.Security), 'reality')) then
    AError := 'Trojan поддерживает security none, tls или reality.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName,
    'Trojan') and SameText(Trim(AProfile.Security), 'reality') and
    (NormalizedNetwork(AProfile) <> 'tcp') then
    AError := 'Trojan REALITY требует network=tcp.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName,
    'Trojan') and SameText(Trim(AProfile.Security), 'reality') and
    ((Trim(AProfile.PublicKey) = '') or
     (EffectiveServerName(AProfile) = '')) then
    AError := 'Trojan REALITY требует public key и server name.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName,
    'Shadowsocks') and ((EffectiveMethod(AProfile) = '') or
      SameText(EffectiveMethod(AProfile), 'none')) then
    AError := 'Для Shadowsocks требуется метод шифрования.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName,
    'Shadowsocks') and (EffectivePassword(AProfile) = '') then
    AError := 'Для Shadowsocks требуется пароль.'
  else if (Trim(AProfile.RawConfig) = '') and
    SameText(AProfile.ProtocolName, 'Hysteria2') and
    (EffectivePassword(AProfile) = '') then
    AError := 'Для Hysteria2 требуется auth/password.'
  else if (Trim(AProfile.RawConfig) = '') and
    SameText(AProfile.ProtocolName, 'Hysteria2') and
    not ((Trim(AProfile.Security) = '') or
      SameText(Trim(AProfile.Security), 'none') or
      SameText(Trim(AProfile.Security), 'tls')) then
    AError := 'Hysteria2 поддерживает только TLS security.'
  else if (Trim(AProfile.RawConfig) = '') and
    SameText(AProfile.ProtocolName, 'Hysteria2') and
    not ((Trim(AProfile.Obfs) = '') or SameText(Trim(AProfile.Obfs), 'none') or
      SameText(Trim(AProfile.Obfs), 'salamander')) then
    AError := 'Поддерживается Hysteria2 obfs none или salamander.'
  else if (Trim(AProfile.RawConfig) = '') and
    SameText(AProfile.ProtocolName, 'Hysteria2') and
    SameText(Trim(AProfile.Obfs), 'salamander') and
    (Trim(AProfile.ObfsPassword) = '') then
    AError := 'Hysteria2 salamander требует obfs-password.'
  else if (Trim(AProfile.RawConfig) = '') and
    SameText(AProfile.ProtocolName, 'WireGuard') and
    (EffectivePassword(AProfile) = '') then
    AError := 'Для WireGuard требуется private key.'
  else if (Trim(AProfile.RawConfig) = '') and
    SameText(AProfile.ProtocolName, 'WireGuard') and
    (Trim(AProfile.PublicKey) = '') then
    AError := 'Для WireGuard требуется peer public key.'
  else if (Trim(AProfile.RawConfig) = '') and
    (SameText(AProfile.ProtocolName, 'VLESS') or
     SameText(AProfile.ProtocolName, 'VMess') or
     SameText(AProfile.ProtocolName, 'Trojan')) and
    not ((NormalizedNetwork(AProfile) = 'tcp') or
      (NormalizedNetwork(AProfile) = 'ws') or
      (NormalizedNetwork(AProfile) = 'grpc')) then
    AError := 'Поддерживаются transport-сети tcp, ws и grpc.'
  else if (Trim(AProfile.RawConfig) = '') and
    (SameText(AProfile.ProtocolName, 'VMess') or
     SameText(AProfile.ProtocolName, 'Trojan')) and
    (NormalizedNetwork(AProfile) = 'ws') and
    (Trim(AProfile.Path) = '') then
    AError := AProfile.ProtocolName + ' WebSocket требует path.'
  else if (Trim(AProfile.RawConfig) = '') and
    (SameText(AProfile.ProtocolName, 'VMess') or
     SameText(AProfile.ProtocolName, 'Trojan')) and
    (NormalizedNetwork(AProfile) = 'grpc') and
    (Trim(AProfile.ServiceName) = '') and (Trim(AProfile.Path) = '') then
    AError := AProfile.ProtocolName + ' gRPC требует service name.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName, 'VLESS') and
    not ((NormalizedNetwork(AProfile) = 'tcp') or
      (NormalizedNetwork(AProfile) = 'ws') or
      (NormalizedNetwork(AProfile) = 'grpc')) then
    AError := 'Поддерживаются сети VLESS: tcp, ws и grpc.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName, 'VLESS') and
    SameText(Trim(AProfile.Security), 'reality') and
    (NormalizedNetwork(AProfile) <> 'tcp') then
    AError := 'VLESS REALITY требует network=tcp.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName, 'VLESS') and
    SameText(Trim(AProfile.Security), 'reality') and
    ((Trim(AProfile.PublicKey) = '') or (EffectiveServerName(AProfile) = '')) then
    AError := 'VLESS REALITY требует public key и server name.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName, 'VLESS') and
    not ((Trim(AProfile.Security) = '') or
      SameText(Trim(AProfile.Security), 'none') or
      SameText(Trim(AProfile.Security), 'tls') or
      SameText(Trim(AProfile.Security), 'reality')) then
    AError := 'Поддерживаются security: none, tls и reality.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName, 'VLESS') and
    ((Trim(AProfile.Security) = '') or SameText(Trim(AProfile.Security), 'none')) and
    (NormalizedNetwork(AProfile) <> 'tcp') then
    AError := 'VLESS без TLS поддерживается только с network=tcp.'
  else if (Trim(AProfile.RawConfig) = '') and SameText(AProfile.ProtocolName, 'VLESS') and
    (NormalizedNetwork(AProfile) = 'grpc') and
    (Trim(AProfile.ServiceName) = '') and (Trim(AProfile.Path) = '') then
    AError := 'Для gRPC требуется serviceName или path.';
  Result := AError = '';
end;

end.
