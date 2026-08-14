unit ZaryaCoreProvider;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils;

type
  TZaryaProviderDistribution = (pdEmbedded, pdExternal);
  TZaryaConfigFormat = (cfXrayJson, cfV2RayJson, cfSingBoxJson,
    cfMihomoYaml, cfHysteriaYaml, cfRaw);
  TZaryaProviderState = (psAvailable, psMissing, psIncompatible, psChanged,
    psRunning, psFailed);
  TZaryaReadinessKind = (rkMixedTcp, rkHttpTcp, rkSocksTcp, rkCustomTcp);
  TZaryaStringArray = array of string;

  TZaryaCoreProvider = record
    ProviderId: string;
    DisplayName: string;
    Distribution: TZaryaProviderDistribution;
    AdapterId: string;
    ExecutablePath: string;
    WorkingDirectory: string;
    AssetDirectory: string;
    VersionArguments: TZaryaStringArray;
    ValidateArguments: TZaryaStringArray;
    RunArguments: TZaryaStringArray;
    ConfigFormat: TZaryaConfigFormat;
    ConfigExtension: string;
    SupportedProtocols: string;
    SupportsRouting: Boolean;
    SupportsDns: Boolean;
    SupportsSystemProxy: Boolean;
    SupportsTun: Boolean;
    ReadinessKind: TZaryaReadinessKind;
    Version: string;
    Architecture: string;
    Sha256: string;
    ConfirmedSha256: string;
    State: TZaryaProviderState;
    LastError: string;
  end;

  TZaryaCoreProviders = array of TZaryaCoreProvider;

const
  ProviderEmbeddedXray = 'embedded.xray';
  ProviderEmbeddedSingBox = 'embedded.singbox';
  ProviderExternalXray = 'external.xray';
  ProviderExternalSingBox = 'external.singbox';
  ProviderExternalV2Ray = 'external.v2ray';
  ProviderExternalMihomo = 'external.mihomo';
  ProviderExternalNekoBoxCore = 'external.nekobox-core';
  ProviderExternalHysteria2 = 'external.hysteria2';

function StringArray(const AValues: array of string): TZaryaStringArray;
function CreateProviderPreset(const AProviderId: string): TZaryaCoreProvider;
function CreateEmbeddedProviders: TZaryaCoreProviders;
function ProviderSupportsProtocol(const AProvider: TZaryaCoreProvider;
  const AProtocol: string): Boolean;
function ProviderRequiresRawConfig(const AProvider: TZaryaCoreProvider): Boolean;
function DefaultProviderForProtocol(const AProtocol: string): string;
function ConfigFormatToString(const AValue: TZaryaConfigFormat): string;
function ConfigFormatFromString(const AValue: string): TZaryaConfigFormat;
function DistributionToString(const AValue: TZaryaProviderDistribution): string;
function StateToString(const AValue: TZaryaProviderState): string;
function ReadinessKindToString(const AValue: TZaryaReadinessKind): string;
function ReadinessKindFromString(const AValue: string): TZaryaReadinessKind;

implementation

function StringArray(const AValues: array of string): TZaryaStringArray;
var
  I: Integer;
begin
  Result := nil;
  SetLength(Result, Length(AValues));
  for I := 0 to High(AValues) do
    Result[I] := AValues[I];
end;

procedure ApplyCommonExternalDefaults(var AProvider: TZaryaCoreProvider);
begin
  AProvider.Distribution := pdExternal;
  AProvider.State := psMissing;
  AProvider.WorkingDirectory := '';
  AProvider.AssetDirectory := '';
  AProvider.Version := '';
  AProvider.Architecture := '';
  AProvider.Sha256 := '';
  AProvider.ConfirmedSha256 := '';
  AProvider.LastError := '';
  AProvider.SupportsSystemProxy := True;
  AProvider.ReadinessKind := rkMixedTcp;
end;

function CreateProviderPreset(const AProviderId: string): TZaryaCoreProvider;
begin
  Result := Default(TZaryaCoreProvider);
  Result.ProviderId := AProviderId;
  ApplyCommonExternalDefaults(Result);

  if SameText(AProviderId, ProviderEmbeddedXray) then
  begin
    Result.DisplayName := 'Встроенный Xray';
    Result.Distribution := pdEmbedded;
    Result.AdapterId := 'xray';
    Result.ConfigFormat := cfXrayJson;
    Result.ConfigExtension := '.json';
    Result.SupportedProtocols :=
      'VLESS,VMess,Trojan,Shadowsocks,SOCKS,Hysteria2,WireGuard';
    Result.SupportsRouting := True;
    Result.SupportsDns := True;
    Result.SupportsTun := False;
    Result.State := psAvailable;
    Exit;
  end;

  if SameText(AProviderId, ProviderEmbeddedSingBox) then
  begin
    Result.DisplayName := 'Встроенный sing-box';
    Result.Distribution := pdEmbedded;
    Result.AdapterId := 'sing-box';
    Result.ConfigFormat := cfSingBoxJson;
    Result.ConfigExtension := '.json';
    Result.SupportedProtocols :=
      'VLESS,VMess,Trojan,Shadowsocks,SOCKS,Hysteria2';
    Result.SupportsRouting := True;
    Result.SupportsDns := True;
    Result.SupportsTun := True;
    Result.State := psMissing;
    Exit;
  end;

  if SameText(AProviderId, ProviderExternalXray) then
  begin
    Result.DisplayName := 'Внешний Xray';
    Result.AdapterId := 'xray';
    Result.ConfigFormat := cfXrayJson;
    Result.ConfigExtension := '.json';
    Result.VersionArguments := StringArray(['version']);
    Result.ValidateArguments := StringArray(['run', '-test', '-config', '{config}']);
    Result.RunArguments := StringArray(['run', '-config', '{config}']);
    Result.SupportedProtocols :=
      'VLESS,VMess,Trojan,Shadowsocks,SOCKS,Hysteria2,WireGuard';
    Result.SupportsRouting := True;
    Result.SupportsDns := True;
  end
  else if SameText(AProviderId, ProviderExternalSingBox) then
  begin
    Result.DisplayName := 'Внешний sing-box';
    Result.AdapterId := 'sing-box';
    Result.ConfigFormat := cfSingBoxJson;
    Result.ConfigExtension := '.json';
    Result.VersionArguments := StringArray(['version']);
    Result.ValidateArguments := StringArray(['check', '-c', '{config}']);
    Result.RunArguments := StringArray(['run', '-c', '{config}']);
    Result.SupportedProtocols :=
      'VLESS,VMess,Trojan,Shadowsocks,SOCKS,Hysteria2';
    Result.SupportsRouting := True;
    Result.SupportsDns := True;
    Result.SupportsTun := True;
  end
  else if SameText(AProviderId, ProviderExternalV2Ray) then
  begin
    Result.DisplayName := 'Внешний V2Ray';
    Result.AdapterId := 'v2ray';
    Result.ConfigFormat := cfV2RayJson;
    Result.ConfigExtension := '.json';
    Result.VersionArguments := StringArray(['version']);
    Result.ValidateArguments := StringArray(['test', '-c', '{config}']);
    Result.RunArguments := StringArray(['run', '-c', '{config}']);
    Result.SupportedProtocols := 'VLESS,VMess,Trojan,Shadowsocks,SOCKS';
    Result.SupportsRouting := True;
    Result.SupportsDns := True;
    Result.ReadinessKind := rkSocksTcp;
  end
  else if SameText(AProviderId, ProviderExternalMihomo) then
  begin
    Result.DisplayName := 'Внешний Mihomo';
    Result.AdapterId := 'mihomo';
    Result.ConfigFormat := cfMihomoYaml;
    Result.ConfigExtension := '.yaml';
    Result.VersionArguments := StringArray(['-v']);
    Result.ValidateArguments := StringArray(['-t', '-f', '{config}']);
    Result.RunArguments := StringArray(['-f', '{config}', '-d', '{dataDir}']);
    Result.SupportedProtocols :=
      'VLESS,VMess,Trojan,Shadowsocks,SOCKS,Hysteria2,WireGuard';
    Result.SupportsRouting := True;
    Result.SupportsDns := True;
    Result.SupportsTun := True;
  end
  else if SameText(AProviderId, ProviderExternalNekoBoxCore) then
  begin
    Result.DisplayName := 'Внешний NekoBox core';
    Result.AdapterId := 'nekobox-core';
    Result.ConfigFormat := cfSingBoxJson;
    Result.ConfigExtension := '.json';
    Result.VersionArguments := StringArray(['sing-box', 'version']);
    Result.ValidateArguments :=
      StringArray(['sing-box', 'check', '-c', '{config}']);
    Result.RunArguments :=
      StringArray(['sing-box', 'run', '-c', '{config}']);
    Result.SupportedProtocols :=
      'VLESS,VMess,Trojan,Shadowsocks,SOCKS,Hysteria2';
    Result.SupportsRouting := True;
    Result.SupportsDns := True;
    Result.SupportsTun := True;
  end
  else if SameText(AProviderId, ProviderExternalHysteria2) then
  begin
    Result.DisplayName := 'Внешний Hysteria 2';
    Result.AdapterId := 'hysteria2';
    Result.ConfigFormat := cfHysteriaYaml;
    Result.ConfigExtension := '.yaml';
    Result.VersionArguments := StringArray(['version']);
    Result.ValidateArguments := nil;
    Result.RunArguments := StringArray(['client', '-c', '{config}']);
    Result.SupportedProtocols := 'Hysteria2';
    Result.SupportsRouting := False;
    Result.SupportsDns := False;
    Result.ReadinessKind := rkHttpTcp;
  end
  else
  begin
    Result.DisplayName := 'Пользовательское ядро';
    Result.AdapterId := 'raw';
    Result.ConfigFormat := cfRaw;
    Result.ConfigExtension := '.conf';
    Result.VersionArguments := StringArray(['--version']);
    Result.ValidateArguments := nil;
    Result.RunArguments := StringArray(['{config}']);
    Result.SupportedProtocols := '*';
    Result.SupportsRouting := False;
    Result.SupportsDns := False;
    Result.ProviderId := AProviderId;
  end;
end;

function CreateEmbeddedProviders: TZaryaCoreProviders;
begin
  Result := nil;
  SetLength(Result, 2);
  Result[0] := CreateProviderPreset(ProviderEmbeddedXray);
  Result[1] := CreateProviderPreset(ProviderEmbeddedSingBox);
end;

function ProviderSupportsProtocol(const AProvider: TZaryaCoreProvider;
  const AProtocol: string): Boolean;
var
  Values: TStringList;
begin
  if Trim(AProvider.SupportedProtocols) = '*' then
    Exit(True);
  Values := TStringList.Create;
  try
    Values.StrictDelimiter := True;
    Values.Delimiter := ',';
    Values.DelimitedText := AProvider.SupportedProtocols;
    Result := Values.IndexOf(Trim(AProtocol)) >= 0;
  finally
    Values.Free;
  end;
end;

function ProviderRequiresRawConfig(const AProvider: TZaryaCoreProvider): Boolean;
begin
  Result := (AProvider.ConfigFormat = cfRaw) or
    SameText(AProvider.AdapterId, 'raw');
end;

function DefaultProviderForProtocol(const AProtocol: string): string;
begin
  Result := ProviderEmbeddedXray;
end;

function ConfigFormatToString(const AValue: TZaryaConfigFormat): string;
begin
  case AValue of
    cfXrayJson: Result := 'xray-json';
    cfV2RayJson: Result := 'v2ray-json';
    cfSingBoxJson: Result := 'sing-box-json';
    cfMihomoYaml: Result := 'mihomo-yaml';
    cfHysteriaYaml: Result := 'hysteria-yaml';
  else
    Result := 'raw';
  end;
end;

function ConfigFormatFromString(const AValue: string): TZaryaConfigFormat;
begin
  if SameText(AValue, 'xray-json') then Result := cfXrayJson
  else if SameText(AValue, 'v2ray-json') then Result := cfV2RayJson
  else if SameText(AValue, 'sing-box-json') then Result := cfSingBoxJson
  else if SameText(AValue, 'mihomo-yaml') then Result := cfMihomoYaml
  else if SameText(AValue, 'hysteria-yaml') then Result := cfHysteriaYaml
  else Result := cfRaw;
end;

function DistributionToString(const AValue: TZaryaProviderDistribution): string;
begin
  if AValue = pdEmbedded then Result := 'embedded' else Result := 'external';
end;

function StateToString(const AValue: TZaryaProviderState): string;
begin
  case AValue of
    psAvailable: Result := 'available';
    psMissing: Result := 'missing';
    psIncompatible: Result := 'incompatible';
    psChanged: Result := 'changed';
    psRunning: Result := 'running';
  else
    Result := 'failed';
  end;
end;

function ReadinessKindToString(const AValue: TZaryaReadinessKind): string;
begin
  case AValue of
    rkHttpTcp: Result := 'http-tcp';
    rkSocksTcp: Result := 'socks-tcp';
    rkCustomTcp: Result := 'custom-tcp';
  else
    Result := 'mixed-tcp';
  end;
end;

function ReadinessKindFromString(const AValue: string): TZaryaReadinessKind;
begin
  if SameText(AValue, 'http-tcp') then Result := rkHttpTcp
  else if SameText(AValue, 'socks-tcp') then Result := rkSocksTcp
  else if SameText(AValue, 'custom-tcp') then Result := rkCustomTcp
  else Result := rkMixedTcp;
end;

end.
