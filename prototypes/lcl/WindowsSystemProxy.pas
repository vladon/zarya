unit WindowsSystemProxy;

{$mode objfpc}{$H+}

interface

uses
  ZaryaSystemProxy;

type
  TWindowsSystemProxyBackend = class(TInterfacedObject,
    IZaryaSystemProxyBackend)
  public
    function ReadState(out AState: TZaryaSystemProxyState;
      out AError: string): Boolean;
    function ApplyLocalProxy(const APort: Integer; const AKind: string;
      out AError: string): Boolean;
    function RestoreState(const AState: TZaryaSystemProxyState;
      out AError: string): Boolean;
    function BackendName: string;
  end;

implementation

uses
  SysUtils, Windows, Registry;

const
  InternetSettingsKey =
    '\Software\Microsoft\Windows\CurrentVersion\Internet Settings';
  InternetOptionRefresh = 37;
  InternetOptionSettingsChanged = 39;

function InternetSetOptionW(AInternet: Pointer; AOption: DWORD;
  ABuffer: Pointer; ABufferLength: DWORD): BOOL; stdcall;
  external 'wininet.dll' name 'InternetSetOptionW';

function NotifySettingsChanged(out AError: string): Boolean;
var
  Changed: BOOL;
  Refreshed: BOOL;
begin
  Changed := InternetSetOptionW(nil, InternetOptionSettingsChanged, nil, 0);
  Refreshed := InternetSetOptionW(nil, InternetOptionRefresh, nil, 0);
  Result := Changed and Refreshed;
  if Result then
    AError := ''
  else
    AError := 'Windows не подтвердил обновление настроек прокси: ' +
      SysErrorMessage(GetLastError);
end;

procedure DeleteIfPresent(ARegistry: TRegistry; const AName: string);
begin
  if ARegistry.ValueExists(AName) then
    ARegistry.DeleteValue(AName);
end;

procedure WriteIntegerOrDelete(ARegistry: TRegistry; const AName: string;
  const AExists: Boolean; const AValue: Integer);
begin
  if AExists then
    ARegistry.WriteInteger(AName, AValue)
  else
    DeleteIfPresent(ARegistry, AName);
end;

procedure WriteStringOrDelete(ARegistry: TRegistry; const AName: string;
  const AExists: Boolean; const AValue: string);
begin
  if AExists then
    ARegistry.WriteString(AName, AValue)
  else
    DeleteIfPresent(ARegistry, AName);
end;

function TWindowsSystemProxyBackend.ReadState(
  out AState: TZaryaSystemProxyState; out AError: string): Boolean;
var
  Registry: TRegistry;
begin
  AState := Default(TZaryaSystemProxyState);
  AError := '';
  Registry := nil;
  try
    Registry := TRegistry.Create(KEY_READ);
    Registry.RootKey := HKEY_CURRENT_USER;
    if not Registry.OpenKeyReadOnly(InternetSettingsKey) then
      raise Exception.Create('Не удалось открыть настройки WinINet для чтения.');
    AState.HasProxyEnable := Registry.ValueExists('ProxyEnable');
    if AState.HasProxyEnable then
      AState.ProxyEnabled := Registry.ReadInteger('ProxyEnable') <> 0;
    AState.HasProxyServer := Registry.ValueExists('ProxyServer');
    if AState.HasProxyServer then
      AState.ProxyServer := Registry.ReadString('ProxyServer');
    AState.HasProxyOverride := Registry.ValueExists('ProxyOverride');
    if AState.HasProxyOverride then
      AState.ProxyOverride := Registry.ReadString('ProxyOverride');
    AState.HasAutoDetect := Registry.ValueExists('AutoDetect');
    if AState.HasAutoDetect then
      AState.AutoDetect := Registry.ReadInteger('AutoDetect') <> 0;
    AState.HasAutoConfigUrl := Registry.ValueExists('AutoConfigURL');
    if AState.HasAutoConfigUrl then
      AState.AutoConfigUrl := Registry.ReadString('AutoConfigURL');
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Registry.Free;
end;

function TWindowsSystemProxyBackend.ApplyLocalProxy(const APort: Integer;
  const AKind: string; out AError: string): Boolean;
var
  Registry: TRegistry;
  ProxyServer: string;
begin
  AError := '';
  if (APort < 1) or (APort > 65535) then
  begin
    AError := 'Некорректный локальный proxy-порт: ' + IntToStr(APort);
    Exit(False);
  end;
  if not (SameText(AKind, 'mixed') or SameText(AKind, 'http') or
    SameText(AKind, 'socks')) then
  begin
    AError := 'Неподдерживаемый тип системного прокси: ' + AKind;
    Exit(False);
  end;
  Registry := nil;
  try
    Registry := TRegistry.Create(KEY_READ or KEY_WRITE);
    Registry.RootKey := HKEY_CURRENT_USER;
    if not Registry.OpenKey(InternetSettingsKey, True) then
      raise Exception.Create('Не удалось открыть настройки WinINet для записи.');
    if SameText(AKind, 'socks') then
      ProxyServer := Format('socks=127.0.0.1:%d', [APort])
    else
      ProxyServer := Format('http=127.0.0.1:%d;https=127.0.0.1:%d',
        [APort, APort]);
    Registry.WriteInteger('ProxyEnable', 1);
    Registry.WriteString('ProxyServer', ProxyServer);
    Registry.WriteString('ProxyOverride', '<local>');
    Registry.CloseKey;
    Result := NotifySettingsChanged(AError);
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Registry.Free;
end;

function TWindowsSystemProxyBackend.RestoreState(
  const AState: TZaryaSystemProxyState; out AError: string): Boolean;
var
  Registry: TRegistry;
begin
  AError := '';
  Registry := nil;
  try
    Registry := TRegistry.Create(KEY_READ or KEY_WRITE);
    Registry.RootKey := HKEY_CURRENT_USER;
    if not Registry.OpenKey(InternetSettingsKey, True) then
      raise Exception.Create('Не удалось открыть настройки WinINet для восстановления.');
    WriteIntegerOrDelete(Registry, 'ProxyEnable', AState.HasProxyEnable,
      Ord(AState.ProxyEnabled));
    WriteStringOrDelete(Registry, 'ProxyServer', AState.HasProxyServer,
      AState.ProxyServer);
    WriteStringOrDelete(Registry, 'ProxyOverride', AState.HasProxyOverride,
      AState.ProxyOverride);
    WriteIntegerOrDelete(Registry, 'AutoDetect', AState.HasAutoDetect,
      Ord(AState.AutoDetect));
    WriteStringOrDelete(Registry, 'AutoConfigURL', AState.HasAutoConfigUrl,
      AState.AutoConfigUrl);
    Registry.CloseKey;
    Result := NotifySettingsChanged(AError);
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Registry.Free;
end;

function TWindowsSystemProxyBackend.BackendName: string;
begin
  Result := 'Windows WinINet';
end;

end.
