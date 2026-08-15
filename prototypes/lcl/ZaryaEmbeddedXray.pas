unit ZaryaEmbeddedXray;

{$IFDEF FPC}
{$mode delphi}{$H+}
{$ENDIF}

interface

uses
  SysUtils;

type
  TZaryaXrayRuntimeState = (xrsStopped, xrsStarting, xrsRunning,
    xrsStopping, xrsFailed);

  TZaryaAbiVersionFunction = function: Integer; cdecl;
  TZaryaStringFunction = function: PAnsiChar; cdecl;
  TZaryaConfigFunction = function(AConfig: PAnsiChar;
    AConfigSize: NativeUInt; AAssetDirectory: PAnsiChar): PAnsiChar; cdecl;
  TZaryaStateFunction = function: Integer; cdecl;
  TZaryaProbeUrlFunction = function(AUrl, AProxyKind, AProxyHost: PAnsiChar;
    AProxyPort, ATimeoutMs: Integer; ADelayMs: PInt64): PAnsiChar; cdecl;
  TZaryaFreeFunction = procedure(AValue: Pointer); cdecl;

  TZaryaEmbeddedXray = class
  private
    FLibraryHandle: THandle;
    FLibraryPath: string;
    FLoadStatus: string;
    FVersion: string;
    FAbiVersion: Integer;
    FAbiVersionFunction: TZaryaAbiVersionFunction;
    FVersionFunction: TZaryaStringFunction;
    FValidateFunction: TZaryaConfigFunction;
    FStartFunction: TZaryaConfigFunction;
    FStopFunction: TZaryaStringFunction;
    FStateFunction: TZaryaStateFunction;
    FDrainLogsFunction: TZaryaStringFunction;
    FProbeUrlFunction: TZaryaProbeUrlFunction;
    FFreeFunction: TZaryaFreeFunction;
    function TakeString(AValue: PAnsiChar): string;
    function CallConfig(const AFunction: TZaryaConfigFunction; const AConfig,
      AAssetDirectory: string; out AError: string): Boolean;
  public
    constructor Create(const ALibraryPath: string);
    function Available: Boolean;
    function Validate(const AConfig, AAssetDirectory: string;
      out AError: string): Boolean;
    function Start(const AConfig, AAssetDirectory: string;
      out AError: string): Boolean;
    function Stop(out AError: string): Boolean;
    function State: TZaryaXrayRuntimeState;
    function DrainLogs: string;
    function ProbeUrl(const AUrl, AProxyKind, AProxyHost: string;
      const AProxyPort, ATimeoutMs: Integer; out ADelayMs: Int64;
      out AError: string): Boolean;
    property LibraryPath: string read FLibraryPath;
    property LoadStatus: string read FLoadStatus;
    property Version: string read FVersion;
    property AbiVersion: Integer read FAbiVersion;
  end;

implementation

{$IF defined(FPC) and not defined(ZARYA_STATIC_XRAY)}
uses
  Dynlibs;
{$ELSEIF not defined(FPC)}
uses
  Winapi.Windows;
{$ENDIF}

const
  ExpectedAbiVersion = 2;

{$IFDEF ZARYA_STATIC_XRAY}
function ZaryaXrayAbiVersion: LongInt; cdecl; external;
function ZaryaXrayVersion: PAnsiChar; cdecl; external;
function ZaryaXrayValidate(AConfig: PAnsiChar; AConfigSize: NativeUInt;
  AAssetDirectory: PAnsiChar): PAnsiChar; cdecl; external;
function ZaryaXrayStart(AConfig: PAnsiChar; AConfigSize: NativeUInt;
  AAssetDirectory: PAnsiChar): PAnsiChar; cdecl; external;
function ZaryaXrayStop: PAnsiChar; cdecl; external;
function ZaryaXrayState: Integer; cdecl; external;
function ZaryaXrayDrainLogs: PAnsiChar; cdecl; external;
function ZaryaXrayProbeURL(AUrl, AProxyKind, AProxyHost: PAnsiChar;
  AProxyPort, ATimeoutMs: Integer; ADelayMs: PInt64): PAnsiChar; cdecl; external;
procedure ZaryaXrayFree(AValue: Pointer); cdecl; external;
procedure ZaryaXrayRuntimeInit; cdecl;
  external name '_rt0_amd64_windows_lib';

var
  StaticRuntimeInitialized: Boolean = False;
{$ENDIF}

{$IFNDEF ZARYA_STATIC_XRAY}
function LoadDynamicLibrary(const APath: string): THandle;
begin
  {$IFDEF FPC}
  Result := Dynlibs.LoadLibrary(APath);
  {$ELSE}
  Result := Winapi.Windows.LoadLibrary(PChar(APath));
  {$ENDIF}
end;

function ResolveSymbol(const AHandle: THandle; const AName: PAnsiChar): Pointer;
begin
  {$IFDEF FPC}
  Result := Dynlibs.GetProcedureAddress(TLibHandle(AHandle), AName);
  {$ELSE}
  Result := Winapi.Windows.GetProcAddress(AHandle, AName);
  {$ENDIF}
end;
{$ENDIF}

constructor TZaryaEmbeddedXray.Create(const ALibraryPath: string);
begin
  inherited Create;
  {$IFDEF ZARYA_STATIC_XRAY}
  FLibraryPath := '(statically linked)';
  FLibraryHandle := 1;
  if not StaticRuntimeInitialized then
  begin
    ZaryaXrayRuntimeInit;
    StaticRuntimeInitialized := True;
  end;
  FAbiVersionFunction := @ZaryaXrayAbiVersion;
  FVersionFunction := @ZaryaXrayVersion;
  FValidateFunction := @ZaryaXrayValidate;
  FStartFunction := @ZaryaXrayStart;
  FStopFunction := @ZaryaXrayStop;
  FStateFunction := @ZaryaXrayState;
  FDrainLogsFunction := @ZaryaXrayDrainLogs;
  FProbeUrlFunction := @ZaryaXrayProbeURL;
  FFreeFunction := @ZaryaXrayFree;
  {$ELSE}
  FLibraryPath := ExpandFileName(ALibraryPath);
  FLoadStatus := '';
  if not FileExists(FLibraryPath) then
  begin
    FLoadStatus := 'Встроенная библиотека Xray не найдена: ' + FLibraryPath;
    Exit;
  end;
  FLibraryHandle := LoadDynamicLibrary(FLibraryPath);
  if FLibraryHandle = 0 then
  begin
    FLoadStatus := 'Не удалось загрузить встроенную библиотеку Xray.';
    Exit;
  end;

  FAbiVersionFunction := TZaryaAbiVersionFunction(ResolveSymbol(FLibraryHandle,
    'ZaryaXrayAbiVersion'));
  FVersionFunction := TZaryaStringFunction(ResolveSymbol(FLibraryHandle,
    'ZaryaXrayVersion'));
  FValidateFunction := TZaryaConfigFunction(ResolveSymbol(FLibraryHandle,
    'ZaryaXrayValidate'));
  FStartFunction := TZaryaConfigFunction(ResolveSymbol(FLibraryHandle,
    'ZaryaXrayStart'));
  FStopFunction := TZaryaStringFunction(ResolveSymbol(FLibraryHandle,
    'ZaryaXrayStop'));
  FStateFunction := TZaryaStateFunction(ResolveSymbol(FLibraryHandle,
    'ZaryaXrayState'));
  FDrainLogsFunction := TZaryaStringFunction(ResolveSymbol(FLibraryHandle,
    'ZaryaXrayDrainLogs'));
  FProbeUrlFunction := TZaryaProbeUrlFunction(ResolveSymbol(FLibraryHandle,
    'ZaryaXrayProbeURL'));
  FFreeFunction := TZaryaFreeFunction(ResolveSymbol(FLibraryHandle,
    'ZaryaXrayFree'));
  {$ENDIF}
  if not Assigned(FAbiVersionFunction) or not Assigned(FVersionFunction) or
    not Assigned(FValidateFunction) or not Assigned(FStartFunction) or
    not Assigned(FStopFunction) or not Assigned(FStateFunction) or
    not Assigned(FDrainLogsFunction) or not Assigned(FProbeUrlFunction) or
    not Assigned(FFreeFunction) then
  begin
    FLoadStatus := 'ABI встроенного Xray неполон.';
    Exit;
  end;
  FAbiVersion := FAbiVersionFunction();
  if FAbiVersion <> ExpectedAbiVersion then
  begin
    FLoadStatus := Format('Несовместимый ABI Xray: ожидался %d, получен %d.',
      [ExpectedAbiVersion, FAbiVersion]);
    Exit;
  end;
  FVersion := TakeString(FVersionFunction());
  FLoadStatus := 'Loaded';
end;

function TZaryaEmbeddedXray.TakeString(AValue: PAnsiChar): string;
var
  Value: UTF8String;
begin
  if AValue = nil then
    Exit('');
  Value := UTF8String(AValue);
  {$IFDEF FPC}
  Result := string(Value);
  {$ELSE}
  Result := UTF8ToString(Value);
  {$ENDIF}
  if Assigned(FFreeFunction) then
    FFreeFunction(AValue);
end;

function TZaryaEmbeddedXray.Available: Boolean;
begin
  Result := (FLibraryHandle <> 0) and (FLoadStatus = 'Loaded');
end;

function TZaryaEmbeddedXray.CallConfig(const AFunction: TZaryaConfigFunction;
  const AConfig, AAssetDirectory: string; out AError: string): Boolean;
var
  Config: UTF8String;
  AssetDirectory: UTF8String;
  Returned: PAnsiChar;
begin
  AError := '';
  if not Available then
  begin
    AError := FLoadStatus;
    Exit(False);
  end;
  if not Assigned(AFunction) then
  begin
    AError := 'Функция ABI Xray недоступна.';
    Exit(False);
  end;
  Config := UTF8String(AConfig);
  AssetDirectory := UTF8String(AAssetDirectory);
  Returned := AFunction(PAnsiChar(Config), Length(Config),
    PAnsiChar(AssetDirectory));
  AError := TakeString(Returned);
  Result := AError = '';
end;

function TZaryaEmbeddedXray.Validate(const AConfig, AAssetDirectory: string;
  out AError: string): Boolean;
begin
  Result := CallConfig(FValidateFunction, AConfig, AAssetDirectory,
    AError);
end;

function TZaryaEmbeddedXray.Start(const AConfig, AAssetDirectory: string;
  out AError: string): Boolean;
begin
  Result := CallConfig(FStartFunction, AConfig, AAssetDirectory,
    AError);
end;

function TZaryaEmbeddedXray.Stop(out AError: string): Boolean;
begin
  AError := '';
  if not Available then
    Exit(True);
  AError := TakeString(FStopFunction());
  Result := AError = '';
end;

function TZaryaEmbeddedXray.State: TZaryaXrayRuntimeState;
begin
  if not Available then
    Exit(xrsFailed);
  case FStateFunction() of
    1: Result := xrsStarting;
    2: Result := xrsRunning;
    3: Result := xrsStopping;
    4: Result := xrsFailed;
  else
    Result := xrsStopped;
  end;
end;

function TZaryaEmbeddedXray.DrainLogs: string;
begin
  if not Available then
    Exit('');
  Result := TakeString(FDrainLogsFunction());
end;

function TZaryaEmbeddedXray.ProbeUrl(const AUrl, AProxyKind,
  AProxyHost: string; const AProxyPort, ATimeoutMs: Integer;
  out ADelayMs: Int64; out AError: string): Boolean;
var
  UrlValue, KindValue, HostValue: UTF8String;
  Returned: PAnsiChar;
begin
  ADelayMs := -1;
  AError := '';
  if not Available then
  begin
    AError := FLoadStatus;
    Exit(False);
  end;
  UrlValue := UTF8String(AUrl);
  KindValue := UTF8String(AProxyKind);
  HostValue := UTF8String(AProxyHost);
  Returned := FProbeUrlFunction(PAnsiChar(UrlValue), PAnsiChar(KindValue),
    PAnsiChar(HostValue), AProxyPort, ATimeoutMs, @ADelayMs);
  AError := TakeString(Returned);
  Result := AError = '';
end;

end.
