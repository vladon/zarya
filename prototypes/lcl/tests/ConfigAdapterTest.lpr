program ConfigAdapterTest;

{$mode objfpc}{$H+}

uses
  SysUtils, fpjson, jsonparser, ZaryaProfile, ZaryaCoreProvider,
  ZaryaRuntimeContracts, ZaryaConfigAdapters;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

procedure CheckJson(const AText, AName: string);
var
  Data: TJSONData;
begin
  Data := GetJSON(AText);
  try
    Check(Data.JSONType = jtObject, AName + ' root is not an object.');
  finally
    Data.Free;
  end;
end;

function Fixture(const AProtocol: string): TZaryaProfile;
begin
  Result := CreateEmptyProfile;
  Result.Name := AProtocol + ' adapter fixture';
  Result.ProtocolName := AProtocol;
  Result.Host := 'example.invalid';
  Result.Port := 443;
  Result.Network := 'tcp';
  Result.Security := 'tls';
  Result.ServerName := 'server.example.invalid';
  if SameText(AProtocol, 'VLESS') or SameText(AProtocol, 'VMess') then
    Result.Uuid := '11111111-1111-1111-1111-111111111111'
  else if SameText(AProtocol, 'Trojan') then
    Result.Password := 'trojan-secret'
  else if SameText(AProtocol, 'Shadowsocks') then
  begin
    Result.Method := 'aes-128-gcm';
    Result.Password := 'shadowsocks-secret';
  end
  else if SameText(AProtocol, 'SOCKS') then
  begin
    Result.Uuid := 'proxy-user';
    Result.Password := 'proxy-password';
    Result.Security := '';
  end
  else if SameText(AProtocol, 'Hysteria2') then
  begin
    Result.Password := 'hysteria-secret';
    Result.Obfs := 'salamander';
    Result.ObfsPassword := 'obfs-secret';
  end
  else if SameText(AProtocol, 'WireGuard') then
  begin
    Result.Password := 'private-key';
    Result.PublicKey := 'public-key';
    Result.LocalAddress := '172.16.0.2/32, fd00::2/128';
    Result.AllowedIps := '0.0.0.0/0, ::/0';
    Result.PreSharedKey := 'pre-shared-key';
    Result.Reserved := '1, 2, 255';
    Result.Mtu := 1408;
    Result.KeepAlive := 25;
    Result.Security := '';
  end;
end;

procedure CheckJsonProtocol(const AAdapter: IConfigAdapter;
  const AProfile: TZaryaProfile; const AContext: TZaryaConfigContext;
  const ANeedle, AName: string);
var
  Config: string;
  ErrorMessage: string;
begin
  Check(AAdapter.Generate(AProfile, AContext, Config, ErrorMessage),
    AName + ' generation failed: ' + ErrorMessage);
  CheckJson(Config, AName);
  Check(Pos(ANeedle, Config) > 0,
    AName + ' generated an unexpected protocol configuration.');
end;

procedure CheckYamlProtocol(const AAdapter: IConfigAdapter;
  const AProfile: TZaryaProfile; const AContext: TZaryaConfigContext;
  const ANeedle, AName: string; out AConfig: string);
var
  ErrorMessage: string;
begin
  Check(AAdapter.Generate(AProfile, AContext, AConfig, ErrorMessage),
    AName + ' generation failed: ' + ErrorMessage);
  Check(Pos(ANeedle, AConfig) > 0,
    AName + ' generated an unexpected protocol configuration.');
end;

var
  Profile: TZaryaProfile;
  Provider: TZaryaCoreProvider;
  Adapter: IConfigAdapter;
  Context: TZaryaConfigContext;
  Config: string;
  ErrorMessage: string;
begin
  Context.MixedPort := 21808;
  Context.HttpPort := 21809;
  Context.SocksPort := 21810;

  Provider := CreateProviderPreset(ProviderExternalV2Ray);
  Adapter := CreateConfigAdapter(Provider);
  Check(Assigned(Adapter), 'V2Ray adapter is missing.');
  Profile := Fixture('VLESS');
  Profile.Network := 'ws';
  Profile.Path := '/proxy';
  Profile.TransportHost := 'host.example.invalid';
  CheckJsonProtocol(Adapter, Profile, Context, '"protocol":"vless"',
    'V2Ray VLESS');
  Check(Adapter.Generate(Profile, Context, Config, ErrorMessage),
    'V2Ray VLESS regeneration failed: ' + ErrorMessage);
  Check(Pos('"protocol":"socks"', Config) > 0,
    'V2Ray config has no SOCKS readiness inbound.');
  Profile.Security := 'reality';
  Profile.PublicKey := 'public-key';
  Check(not Adapter.Generate(Profile, Context, Config, ErrorMessage),
    'V2Ray silently accepted REALITY.');
  CheckJsonProtocol(Adapter, Fixture('VMess'), Context,
    '"protocol":"vmess"', 'V2Ray VMess');
  CheckJsonProtocol(Adapter, Fixture('Trojan'), Context,
    '"protocol":"trojan"', 'V2Ray Trojan');
  CheckJsonProtocol(Adapter, Fixture('Shadowsocks'), Context,
    '"protocol":"shadowsocks"', 'V2Ray Shadowsocks');
  CheckJsonProtocol(Adapter, Fixture('SOCKS'), Context,
    '"protocol":"socks"', 'V2Ray SOCKS');

  Provider := CreateProviderPreset(ProviderExternalSingBox);
  Adapter := CreateConfigAdapter(Provider);
  Profile := Fixture('VLESS');
  Profile.Security := 'reality';
  Profile.PublicKey := 'public-key';
  Profile.ShortId := '0123456789abcdef';
  CheckJsonProtocol(Adapter, Profile, Context, '"type":"vless"',
    'sing-box VLESS');
  Check(Adapter.Generate(Profile, Context, Config, ErrorMessage),
    'sing-box REALITY generation failed: ' + ErrorMessage);
  Check(Pos('"reality":{"enabled":true', Config) > 0,
    'sing-box REALITY options are missing.');
  CheckJsonProtocol(Adapter, Fixture('VMess'), Context,
    '"type":"vmess"', 'sing-box VMess');
  CheckJsonProtocol(Adapter, Fixture('Trojan'), Context,
    '"type":"trojan"', 'sing-box Trojan');
  CheckJsonProtocol(Adapter, Fixture('Shadowsocks'), Context,
    '"type":"shadowsocks"', 'sing-box Shadowsocks');
  CheckJsonProtocol(Adapter, Fixture('SOCKS'), Context,
    '"type":"socks"', 'sing-box SOCKS');
  CheckJsonProtocol(Adapter, Fixture('Hysteria2'), Context,
    '"type":"hysteria2"', 'sing-box Hysteria2');
  Profile := Fixture('WireGuard');
  Check(not Adapter.Generate(Profile, Context, Config, ErrorMessage),
    'sing-box silently accepted an unversioned WireGuard outbound.');

  Provider := CreateProviderPreset(ProviderExternalNekoBoxCore);
  Adapter := CreateConfigAdapter(Provider);
  CheckJsonProtocol(Adapter, Fixture('Hysteria2'), Context,
    '"type":"hysteria2"', 'NekoBox core Hysteria2');

  Provider := CreateProviderPreset(ProviderExternalMihomo);
  Adapter := CreateConfigAdapter(Provider);
  Profile := Fixture('VLESS');
  Profile.Security := 'reality';
  Profile.PublicKey := 'public-key';
  Profile.ShortId := '0123456789abcdef';
  CheckYamlProtocol(Adapter, Profile, Context, '    type: vless',
    'Mihomo VLESS', Config);
  Check(Pos('mixed-port: 21808', Config) > 0,
    'Mihomo mixed port is missing.');
  Check(Pos('reality-opts:', Config) > 0,
    'Mihomo REALITY options are missing.');
  CheckYamlProtocol(Adapter, Fixture('VMess'), Context,
    '    type: vmess', 'Mihomo VMess', Config);
  CheckYamlProtocol(Adapter, Fixture('Trojan'), Context,
    '    type: trojan', 'Mihomo Trojan', Config);
  CheckYamlProtocol(Adapter, Fixture('Shadowsocks'), Context,
    '    type: ss', 'Mihomo Shadowsocks', Config);
  CheckYamlProtocol(Adapter, Fixture('SOCKS'), Context,
    '    type: socks5', 'Mihomo SOCKS', Config);
  CheckYamlProtocol(Adapter, Fixture('Hysteria2'), Context,
    '    type: hysteria2', 'Mihomo Hysteria2', Config);
  CheckYamlProtocol(Adapter, Fixture('WireGuard'), Context,
    '    type: wireguard', 'Mihomo WireGuard', Config);
  Check(Pos('    ip: "172.16.0.2"', Config) > 0,
    'Mihomo WireGuard IPv4 address is missing.');
  Check(Pos('    ipv6: "fd00::2"', Config) > 0,
    'Mihomo WireGuard IPv6 address is missing.');
  Check(Pos('    reserved: [1, 2, 255]', Config) > 0,
    'Mihomo WireGuard reserved bytes are malformed.');

  Profile := Fixture('Hysteria2');
  Profile.Uuid := 'legacy-hysteria-secret';
  Provider := CreateProviderPreset(ProviderExternalHysteria2);
  Adapter := CreateConfigAdapter(Provider);
  CheckYamlProtocol(Adapter, Profile, Context,
    'auth: "hysteria-secret"', 'Hysteria2', Config);
  Check(Pos('legacy-hysteria-secret', Config) = 0,
    'Hysteria2 adapter did not prefer the dedicated password field.');
  Check(Pos('listen: "127.0.0.1:21809"', Config) > 0,
    'Hysteria2 HTTP readiness listener is missing.');
  Check(Pos('socks5:', Config) > 0,
    'Hysteria2 SOCKS5 listener is missing.');

  WriteLn('Config adapter matrix: PASS');
end.
