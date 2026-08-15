unit ZaryaRuntimeContracts;

{$mode objfpc}{$H+}

interface

uses
  ZaryaProfile, ZaryaCoreProvider, ZaryaCoreProviderStore,
  ZaryaRuntimeProcess, ZaryaRouting, ZaryaDns;

type
  IRuntimeProcess = IZaryaRuntimeProcess;
  ICoreProviderStore = IZaryaCoreProviderStore;

  TZaryaConfigContext = record
    MixedPort: Integer;
    HttpPort: Integer;
    SocksPort: Integer;
  end;

  TZaryaRuntimeRequest = record
    Profile: TZaryaProfile;
    Provider: TZaryaCoreProvider;
    MixedPort: Integer;
    HttpPort: Integer;
    SocksPort: Integer;
    DataDirectory: string;
    AssetDirectory: string;
    LogLevel: string;
    RoutingProfile: TZaryaRoutingProfile;
    DnsProfile: TZaryaDnsProfile;
    UseRouting: Boolean;
    UseDns: Boolean;
    RawConfig: string;
    RawConfigFormat: string;
    RawReadinessHost: string;
    RawReadinessPort: Integer;
    RawProxyKind: string;
  end;

  TZaryaNodeTestRequest = record
    SchemaVersion: Integer;
    Provider: TZaryaCoreProvider;
    Config: string;
    DataDirectory: string;
    AssetDirectory: string;
    ReadinessHost: string;
    ReadinessPort: Integer;
    ProxyKind: string;
    TestUrl: string;
    TimeoutMs: Integer;
  end;

  TZaryaNodeTestResult = record
    Success: Boolean;
    ErrorCode: string;
    MessageText: string;
    DelayMs: Int64;
  end;

  IConfigAdapter = interface
    ['{817CB3C5-BD0A-4685-9B21-799E75EC67D2}']
    function AdapterId: string;
    function ConfigFormat: TZaryaConfigFormat;
    function Supports(const AProfile: TZaryaProfile;
      out AReason: string): Boolean;
    function Generate(const AProfile: TZaryaProfile;
      const AContext: TZaryaConfigContext; out AConfig,
      AError: string): Boolean;
    function GenerateRequest(const ARequest: TZaryaRuntimeRequest;
      out AConfig, AError: string): Boolean;
  end;

  IReadinessProbe = interface
    ['{569FA6F7-C73E-4F1D-9D04-B0B4B5A4D0EE}']
    function IsReady(const AHost: string; const APort: Integer): Boolean;
  end;

  IRuntimeProvider = interface
    ['{59124A97-042E-48EA-9388-2CD51D1425E0}']
    function Definition: TZaryaCoreProvider;
    function Validate(const AConfigPath: string; out AError: string): Boolean;
    function Start(const AConfigPath: string; out AError: string): Boolean;
    procedure Stop;
    function IsRunning: Boolean;
    function DrainLogs: string;
  end;

  INodeTestWorker = interface
    ['{6632E206-325A-4F55-88DC-8D5FD4B47D7D}']
    function Run(const ARequest: TZaryaNodeTestRequest;
      out AResult: TZaryaNodeTestResult; out AWorkerLog: string): Boolean;
    procedure Cancel;
  end;

function CreateRuntimeRequest(const AProfile: TZaryaProfile;
  const AProvider: TZaryaCoreProvider; const AContext: TZaryaConfigContext):
  TZaryaRuntimeRequest;

implementation

function CreateRuntimeRequest(const AProfile: TZaryaProfile;
  const AProvider: TZaryaCoreProvider; const AContext: TZaryaConfigContext):
  TZaryaRuntimeRequest;
begin
  Result := Default(TZaryaRuntimeRequest);
  Result.Profile := AProfile;
  Result.Provider := AProvider;
  Result.MixedPort := AContext.MixedPort;
  Result.HttpPort := AContext.HttpPort;
  Result.SocksPort := AContext.SocksPort;
  Result.LogLevel := 'warning';
  Result.RoutingProfile := BuiltInProxyAllRouting;
  Result.DnsProfile := BuiltInSystemDns;
  Result.UseRouting := True;
  Result.UseDns := True;
  Result.RawConfig := AProfile.RawConfig;
  Result.RawConfigFormat := AProfile.RawConfigFormat;
  Result.RawReadinessHost := AProfile.ReadinessHost;
  Result.RawReadinessPort := AProfile.ReadinessPort;
  Result.RawProxyKind := AProfile.SystemProxyKind;
end;

end.
