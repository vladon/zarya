program VlessXrayTest;

{$mode objfpc}{$H+}

uses
  SysUtils, fpjson, jsonparser, ZaryaProfile, ZaryaVlessUri,
  ZaryaXrayConfig;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

const
  RealityLink =
    'vless://11111111-1111-1111-1111-111111111111@host.example.com:443?' +
    'type=tcp&security=reality&pbk=yWrHCV6C0UYNw6nzM0rhDlIUjfLlt28A9h8SkqR52V0' +
    '&fp=chrome&sni=example.com&sid=a1b2c3d4&spx=%2F' +
    '&flow=xtls-rprx-vision#Test%20Reality';

var
  Profile: TZaryaProfile;
  ErrorMessage: string;
  Json: string;
  Data: TJSONData;
  Root: TJSONObject;
  Outbounds: TJSONArray;
  Proxy: TJSONObject;
  Settings: TJSONObject;
  Vnext: TJSONObject;
  User: TJSONObject;
  StreamSettings: TJSONObject;
  Reality: TJSONObject;
begin
  Check(ParseVlessUri(RealityLink, Profile, ErrorMessage),
    'Reality link parse failed: ' + ErrorMessage);
  Check(Profile.Name = 'Test Reality', 'Fragment decoding failed.');
  Check(Profile.Host = 'host.example.com', 'Host parsing failed.');
  Check(Profile.Port = 443, 'Port parsing failed.');
  Check(Profile.Uuid = '11111111-1111-1111-1111-111111111111',
    'UUID parsing failed.');
  Check(Profile.Security = 'reality', 'Security parsing failed.');
  Check(Profile.Network = 'tcp', 'Network parsing failed.');
  Check(Profile.ServerName = 'example.com', 'SNI parsing failed.');
  Check(Profile.PublicKey <> '', 'Public key parsing failed.');
  Check(Profile.SpiderX = '/', 'SpiderX decoding failed.');

  Check(GenerateXrayConfig(Profile, 10808, Json, ErrorMessage),
    'Xray generation failed: ' + ErrorMessage);
  Data := GetJSON(Json);
  try
    Check(Data.JSONType = jtObject, 'Generated JSON root is not an object.');
    Root := TJSONObject(Data);
    Check(Root.Arrays['inbounds'].Objects[0].Get('port', 0) = 10808,
      'Mixed port mismatch.');
    Outbounds := Root.Arrays['outbounds'];
    Check(Outbounds.Count = 3, 'Outbound count mismatch.');
    Proxy := Outbounds.Objects[0];
    Check(Proxy.Get('protocol', '') = 'vless', 'Proxy protocol mismatch.');
    Settings := Proxy.Objects['settings'];
    Vnext := Settings.Arrays['vnext'].Objects[0];
    Check(Vnext.Get('address', '') = 'host.example.com',
      'Generated address mismatch.');
    User := Vnext.Arrays['users'].Objects[0];
    Check(User.Get('flow', '') = 'xtls-rprx-vision',
      'Generated flow mismatch.');
    StreamSettings := Proxy.Objects['streamSettings'];
    Check(StreamSettings.Get('security', '') = 'reality',
      'Generated security mismatch.');
    Reality := StreamSettings.Objects['realitySettings'];
    Check(Reality.Get('serverName', '') = 'example.com',
      'Generated server name mismatch.');
    Check(Reality.Get('shortId', '') = 'a1b2c3d4',
      'Generated short id mismatch.');
    Check(Outbounds.Objects[1].Get('protocol', '') = 'freedom',
      'Direct outbound missing.');
    Check(Outbounds.Objects[2].Get('protocol', '') = 'blackhole',
      'Block outbound missing.');
  finally
    Data.Free;
  end;

  Check(ParseVlessUri(
    'vless://aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa@[2001:db8::1]:8443?' +
    'type=ws&security=tls&host=cdn.example.com&path=%2Fedge' +
    '#%D0%A2%D0%B5%D1%81%D1%82', Profile, ErrorMessage),
    'TLS/WS link parse failed: ' + ErrorMessage);
  Check(Profile.Name = 'Тест', 'UTF-8 fragment decoding failed.');
  Check(Profile.Host = '2001:db8::1', 'IPv6 parsing failed.');
  Check(Profile.TransportHost = 'cdn.example.com', 'WS host parsing failed.');
  Check(Profile.Path = '/edge', 'WS path parsing failed.');
  Check(GenerateXrayConfig(Profile, 2080, Json, ErrorMessage),
    'TLS/WS generation failed: ' + ErrorMessage);
  Data := GetJSON(Json);
  try
    Proxy := TJSONObject(Data).Arrays['outbounds'].Objects[0];
    StreamSettings := Proxy.Objects['streamSettings'];
    Check(StreamSettings.Get('network', '') = 'ws', 'WS network missing.');
    Check(StreamSettings.Objects['wsSettings'].Get('path', '') = '/edge',
      'WS path generation failed.');
  finally
    Data.Free;
  end;

  Check(not ParseVlessUri('https://example.com', Profile, ErrorMessage),
    'Non-VLESS link was accepted.');
  Check(not ParseVlessUri(
    'vless://11111111-1111-1111-1111-111111111111@host.example.com:443?' +
    'type=ws&security=reality&pbk=key&sni=example.com', Profile, ErrorMessage),
    'REALITY over WebSocket was accepted.');
  WriteLn('VLESS import and Xray JSON: PASS');
end.
