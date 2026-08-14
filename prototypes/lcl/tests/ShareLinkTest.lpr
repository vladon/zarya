program ShareLinkTest;

{$mode objfpc}{$H+}

uses
  SysUtils, base64, ZaryaProfile, ZaryaShareLink, ZaryaXrayConfig;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

procedure CheckGenerates(const AProfile: TZaryaProfile);
var
  Config: string;
  ErrorMessage: string;
begin
  Check(GenerateXrayConfig(AProfile, 22990, Config, ErrorMessage),
    AProfile.ProtocolName + ' imported profile did not generate: ' +
    ErrorMessage);
end;

var
  Profile: TZaryaProfile;
  ErrorMessage: string;
  Link: string;
begin
  Check(IsSupportedShareLink('vmess://fixture'),
    'VMess scheme was not recognized.');
  Check(not IsSupportedShareLink('https://example.invalid'),
    'Unsupported scheme was recognized.');

  Link := 'vmess://' + EncodeStringBase64(
    '{"v":"2","ps":"VMess fixture","add":"example.invalid",' +
    '"port":"443","id":"11111111-1111-1111-1111-111111111111",' +
    '"aid":"0","scy":"auto","net":"tcp","tls":"tls",' +
    '"sni":"example.invalid"}');
  Check(ParseShareLink(Link, Profile, ErrorMessage),
    'VMess parse failed: ' + ErrorMessage);
  Check(Profile.ProtocolName = 'VMess', 'VMess protocol mismatch.');
  Check(Profile.SecurityCipher = 'auto', 'VMess cipher mismatch.');
  CheckGenerates(Profile);

  Check(ParseShareLink(
    'trojan://fixture-password@example.invalid:443?security=tls' +
    '&sni=example.invalid#Trojan%20fixture', Profile, ErrorMessage),
    'Trojan parse failed: ' + ErrorMessage);
  Check(Profile.Password = 'fixture-password', 'Trojan password mismatch.');
  Check(Profile.Name = 'Trojan fixture', 'Trojan name mismatch.');
  CheckGenerates(Profile);

  Link := 'ss://' + EncodeStringBase64('aes-128-gcm:fixture-password') +
    '@example.invalid:8388#SS%20fixture';
  Check(ParseShareLink(Link, Profile, ErrorMessage),
    'Shadowsocks parse failed: ' + ErrorMessage);
  Check(Profile.Method = 'aes-128-gcm', 'Shadowsocks method mismatch.');
  Check(Profile.Password = 'fixture-password',
    'Shadowsocks password mismatch.');
  CheckGenerates(Profile);

  Check(ParseShareLink(
    'socks://fixture-user:fixture-password@example.invalid:1080' +
    '#SOCKS%20fixture', Profile, ErrorMessage),
    'SOCKS parse failed: ' + ErrorMessage);
  Check(Profile.Uuid = 'fixture-user', 'SOCKS username mismatch.');
  Check(Profile.Password = 'fixture-password', 'SOCKS password mismatch.');
  CheckGenerates(Profile);

  Check(ParseShareLink(
    'hy2://fixture-password@example.invalid:443/?sni=example.invalid' +
    '&obfs=salamander&obfs-password=fixture-obfs#Hy2%20fixture',
    Profile, ErrorMessage), 'Hysteria2 parse failed: ' + ErrorMessage);
  Check(Profile.Obfs = 'salamander', 'Hysteria2 obfs mismatch.');
  Check(Profile.ObfsPassword = 'fixture-obfs',
    'Hysteria2 obfs password mismatch.');
  CheckGenerates(Profile);

  Check(ParseShareLink(
    'wg://AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA%3D@' +
    'example.invalid:51820?publickey=' +
    'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA%3D' +
    '&address=172.16.0.2%2F32&allowedips=0.0.0.0%2F0%2C%3A%3A%2F0' +
    '&reserved=0%2C0%2C0#WG%20fixture', Profile, ErrorMessage),
    'WireGuard parse failed: ' + ErrorMessage);
  Check(Profile.LocalAddress = '172.16.0.2/32',
    'WireGuard local address mismatch.');
  Check(Profile.AllowedIps = '0.0.0.0/0,::/0',
    'WireGuard allowed IPs mismatch.');
  CheckGenerates(Profile);

  Check(not ParseShareLink('https://example.invalid', Profile, ErrorMessage),
    'Unsupported share link was accepted.');
  WriteLn('Share-link parser matrix: PASS');
end.
