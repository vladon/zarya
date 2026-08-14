unit ZaryaRuntimeContracts;

{$mode objfpc}{$H+}

interface

uses
  ZaryaProfile, ZaryaCoreProvider, ZaryaCoreProviderStore,
  ZaryaRuntimeProcess;

type
  IRuntimeProcess = IZaryaRuntimeProcess;
  ICoreProviderStore = IZaryaCoreProviderStore;

  TZaryaConfigContext = record
    MixedPort: Integer;
    HttpPort: Integer;
    SocksPort: Integer;
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

implementation

end.
