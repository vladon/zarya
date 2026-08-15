program RoutingDnsTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, ZaryaRouting, ZaryaDns, ZaryaPolicyStore,
  FpcPolicyStore, ZaryaProfile, ZaryaCoreProvider, ZaryaRuntimeContracts,
  ZaryaConfigAdapters;

procedure Require(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

procedure RemoveIfPresent(const AFileName: string);
begin
  if FileExists(AFileName) then DeleteFile(AFileName);
end;

var
  BaseDir: string;
  RoutingFile: string;
  DnsFile: string;
  RoutingStore: IRoutingProfileStore;
  DnsStore: IDnsProfileStore;
  RoutingProfiles, LoadedRouting: TZaryaRoutingProfiles;
  DnsProfiles, LoadedDns: TZaryaDnsProfiles;
  CustomRouting: TZaryaRoutingProfile;
  CustomDns: TZaryaDnsProfile;
  Rule: TZaryaRoutingRule;
  Server: TZaryaDnsServer;
  ErrorMessage: string;
  Profile: TZaryaProfile;
  Provider: TZaryaCoreProvider;
  Context: TZaryaConfigContext;
  Request: TZaryaRuntimeRequest;
  Adapter: IConfigAdapter;
  Config: string;
begin
  Randomize;
  BaseDir := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-routing-dns-' + IntToHex(Random(MaxInt), 8);
  ForceDirectories(BaseDir);
  RoutingFile := IncludeTrailingPathDelimiter(BaseDir) + 'routing.json';
  DnsFile := IncludeTrailingPathDelimiter(BaseDir) + 'dns.json';
  try
    RoutingStore := TFpcRoutingProfileStore.Create(RoutingFile);
    DnsStore := TFpcDnsProfileStore.Create(DnsFile);
    Require(RoutingStore.Load(RoutingProfiles, ErrorMessage), ErrorMessage);
    Require(Length(RoutingProfiles) = 5, 'Routing built-ins are missing.');
    Require(DnsStore.Load(DnsProfiles, ErrorMessage), ErrorMessage);
    Require(Length(DnsProfiles) = 4, 'DNS built-ins are missing.');

    CustomRouting := BuiltInProxyAllRouting;
    CustomRouting.Id := 'custom-routing';
    CustomRouting.Name := 'Custom routing';
    CustomRouting.Mode := rmCustom;
    CustomRouting.IsBuiltIn := False;
    Rule := NewRoutingRule;
    Rule.Action := raBlock;
    SetLength(Rule.Values, 1);
    Rule.Values[0] := 'domain:ads.example';
    SetLength(CustomRouting.Rules, 1);
    CustomRouting.Rules[0] := Rule;
    SetLength(RoutingProfiles, Length(RoutingProfiles) + 1);
    RoutingProfiles[High(RoutingProfiles)] := CustomRouting;
    Require(RoutingStore.Save(RoutingProfiles, ErrorMessage), ErrorMessage);
    Require(RoutingStore.Load(LoadedRouting, ErrorMessage), ErrorMessage);
    Require(Length(LoadedRouting) = 6, 'Routing custom round-trip failed.');
    Require(LoadedRouting[5].Rules[0].Action = raBlock,
      'Routing rule action changed.');

    CustomDns := BuiltInSystemDns;
    CustomDns.Id := 'custom-dns';
    CustomDns.Name := 'Custom DNS';
    CustomDns.Mode := dmCustom;
    CustomDns.IsBuiltIn := False;
    Server := NewDnsServer;
    Server.Kind := dskDoh;
    Server.Address := 'https://dns.example/dns-query';
    SetLength(CustomDns.Servers, 1);
    CustomDns.Servers[0] := Server;
    SetLength(DnsProfiles, Length(DnsProfiles) + 1);
    DnsProfiles[High(DnsProfiles)] := CustomDns;
    Require(DnsStore.Save(DnsProfiles, ErrorMessage), ErrorMessage);
    Require(DnsStore.Load(LoadedDns, ErrorMessage), ErrorMessage);
    Require(Length(LoadedDns) = 5, 'DNS custom round-trip failed.');
    Require(LoadedDns[4].Servers[0].Kind = dskDoh,
      'DNS server kind changed.');
    Require(RoutingUsesGeoData(RoutingProfiles[1]),
      'Bypass LAN must require geodata.');
    Require(DnsUsesGeoData(DnsProfiles[2]),
      'China DNS must require geodata.');

    Profile := CreateEmptyProfile;
    Profile.Name := 'Test';
    Profile.Host := '127.0.0.1';
    Profile.Port := 443;
    Profile.Uuid := '11111111-1111-1111-1111-111111111111';
    Provider := CreateProviderPreset(ProviderEmbeddedXray);
    Context.MixedPort := 10808;
    Context.HttpPort := 10809;
    Context.SocksPort := 10810;
    Request := CreateRuntimeRequest(Profile, Provider, Context);
    Request.RoutingProfile := CustomRouting;
    Request.DnsProfile := CustomDns;
    Adapter := CreateConfigAdapter(Provider);
    Require(Adapter.GenerateRequest(Request, Config, ErrorMessage), ErrorMessage);
    Require(Pos('domain:ads.example', Config) > 0,
      'Xray routing rule was not generated.');
    Require(Pos('https://dns.example/dns-query', Config) > 0,
      'Xray DNS server was not generated.');

    Request.Profile.RawConfig := '{}';
    Require(not Adapter.GenerateRequest(Request, Config, ErrorMessage),
      'Raw config accepted non-default common policies.');
    Require(Pos('Raw config owns routing', ErrorMessage) > 0,
      'Raw policy rejection is not actionable.');
    WriteLn('Routing/DNS schema and stores: PASS');
  finally
    RoutingStore := nil;
    DnsStore := nil;
    RemoveIfPresent(RoutingFile);
    RemoveIfPresent(RoutingFile + '.tmp');
    RemoveIfPresent(RoutingFile + '.bak');
    RemoveIfPresent(DnsFile);
    RemoveIfPresent(DnsFile + '.tmp');
    RemoveIfPresent(DnsFile + '.bak');
    RemoveDir(BaseDir);
  end;
end.
