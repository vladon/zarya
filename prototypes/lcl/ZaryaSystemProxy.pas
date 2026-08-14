unit ZaryaSystemProxy;

{$IFDEF FPC}
{$mode delphi}{$H+}
{$ENDIF}

interface

uses
  SysUtils;

type
  TZaryaSystemProxyState = record
    HasProxyEnable: Boolean;
    ProxyEnabled: Boolean;
    HasProxyServer: Boolean;
    ProxyServer: string;
    HasProxyOverride: Boolean;
    ProxyOverride: string;
    HasAutoDetect: Boolean;
    AutoDetect: Boolean;
    HasAutoConfigUrl: Boolean;
    AutoConfigUrl: string;
  end;

  IZaryaSystemProxyBackend = interface
    ['{5B97418F-0AB9-4EA8-A13C-9DA558AA43E3}']
    function ReadState(out AState: TZaryaSystemProxyState;
      out AError: string): Boolean;
    function ApplyLocalProxy(const APort: Integer; const AKind: string;
      out AError: string): Boolean;
    function RestoreState(const AState: TZaryaSystemProxyState;
      out AError: string): Boolean;
    function BackendName: string;
  end;

  TZaryaSystemProxyController = class
  private
    FBackend: IZaryaSystemProxyBackend;
    FSnapshotFileName: string;
    FSavedState: TZaryaSystemProxyState;
    FHasSavedState: Boolean;
    FEnabledByZarya: Boolean;
    function SaveSnapshot(const AState: TZaryaSystemProxyState;
      out AError: string): Boolean;
    function LoadSnapshot(out AState: TZaryaSystemProxyState;
      out AError: string): Boolean;
    function ClearSnapshot(out AError: string): Boolean;
  public
    constructor Create(const ABackend: IZaryaSystemProxyBackend;
      const ASnapshotFileName: string);
    function HasPersistedSnapshot: Boolean;
    function Enable(const APort: Integer; const AKind: string;
      out AError: string): Boolean;
    function Restore(out AError: string): Boolean;
    function RecoverPersisted(out AError: string): Boolean;
    function BackendName: string;
    property EnabledByZarya: Boolean read FEnabledByZarya;
  end;

implementation

uses
  IniFiles;

constructor TZaryaSystemProxyController.Create(
  const ABackend: IZaryaSystemProxyBackend; const ASnapshotFileName: string);
begin
  inherited Create;
  FBackend := ABackend;
  FSnapshotFileName := ASnapshotFileName;
end;

function TZaryaSystemProxyController.SaveSnapshot(
  const AState: TZaryaSystemProxyState; out AError: string): Boolean;
var
  Ini: TIniFile;
  DirectoryName: string;
begin
  AError := '';
  Ini := nil;
  try
    DirectoryName := ExtractFileDir(FSnapshotFileName);
    if (DirectoryName <> '') and (not ForceDirectories(DirectoryName)) then
      raise Exception.Create('Не удалось создать каталог snapshot системного прокси.');
    Ini := TIniFile.Create(FSnapshotFileName);
    Ini.WriteInteger('snapshot', 'schemaVersion', 1);
    Ini.WriteBool('snapshot', 'hasProxyEnable', AState.HasProxyEnable);
    Ini.WriteBool('snapshot', 'proxyEnabled', AState.ProxyEnabled);
    Ini.WriteBool('snapshot', 'hasProxyServer', AState.HasProxyServer);
    Ini.WriteString('snapshot', 'proxyServer', AState.ProxyServer);
    Ini.WriteBool('snapshot', 'hasProxyOverride', AState.HasProxyOverride);
    Ini.WriteString('snapshot', 'proxyOverride', AState.ProxyOverride);
    Ini.WriteBool('snapshot', 'hasAutoDetect', AState.HasAutoDetect);
    Ini.WriteBool('snapshot', 'autoDetect', AState.AutoDetect);
    Ini.WriteBool('snapshot', 'hasAutoConfigUrl', AState.HasAutoConfigUrl);
    Ini.WriteString('snapshot', 'autoConfigUrl', AState.AutoConfigUrl);
    Ini.UpdateFile;
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Ini.Free;
end;

function TZaryaSystemProxyController.LoadSnapshot(
  out AState: TZaryaSystemProxyState; out AError: string): Boolean;
var
  Ini: TIniFile;
begin
  AState := Default(TZaryaSystemProxyState);
  AError := '';
  if not FileExists(FSnapshotFileName) then
  begin
    AError := 'Сохранённое состояние системного прокси отсутствует.';
    Exit(False);
  end;
  Ini := nil;
  try
    Ini := TIniFile.Create(FSnapshotFileName);
    if Ini.ReadInteger('snapshot', 'schemaVersion', 0) <> 1 then
      raise Exception.Create('Неизвестная версия snapshot системного прокси.');
    AState.HasProxyEnable := Ini.ReadBool('snapshot', 'hasProxyEnable', False);
    AState.ProxyEnabled := Ini.ReadBool('snapshot', 'proxyEnabled', False);
    AState.HasProxyServer := Ini.ReadBool('snapshot', 'hasProxyServer', False);
    AState.ProxyServer := Ini.ReadString('snapshot', 'proxyServer', '');
    AState.HasProxyOverride := Ini.ReadBool('snapshot', 'hasProxyOverride', False);
    AState.ProxyOverride := Ini.ReadString('snapshot', 'proxyOverride', '');
    AState.HasAutoDetect := Ini.ReadBool('snapshot', 'hasAutoDetect', False);
    AState.AutoDetect := Ini.ReadBool('snapshot', 'autoDetect', False);
    AState.HasAutoConfigUrl := Ini.ReadBool('snapshot', 'hasAutoConfigUrl', False);
    AState.AutoConfigUrl := Ini.ReadString('snapshot', 'autoConfigUrl', '');
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Ini.Free;
end;

function TZaryaSystemProxyController.ClearSnapshot(out AError: string): Boolean;
begin
  AError := '';
  if not FileExists(FSnapshotFileName) then
    Exit(True);
  Result := DeleteFile(FSnapshotFileName);
  if not Result then
    AError := 'Не удалось удалить snapshot системного прокси.';
end;

function TZaryaSystemProxyController.HasPersistedSnapshot: Boolean;
begin
  Result := FileExists(FSnapshotFileName);
end;

function TZaryaSystemProxyController.Enable(const APort: Integer;
  const AKind: string; out AError: string): Boolean;
var
  RestoreError: string;
begin
  AError := '';
  if FEnabledByZarya then
    Exit(True);
  if not (SameText(AKind, 'mixed') or SameText(AKind, 'http') or
    SameText(AKind, 'socks')) then
  begin
    AError := 'Неподдерживаемый тип системного прокси: ' + AKind;
    Exit(False);
  end;
  if not Assigned(FBackend) then
  begin
    AError := 'Системный прокси не поддерживается.';
    Exit(False);
  end;
  if not FBackend.ReadState(FSavedState, AError) then
    Exit(False);
  if not SaveSnapshot(FSavedState, AError) then
    Exit(False);
  FHasSavedState := True;
  if not FBackend.ApplyLocalProxy(APort, AKind, AError) then
  begin
    if FBackend.RestoreState(FSavedState, RestoreError) then
    begin
      ClearSnapshot(RestoreError);
      FHasSavedState := False;
    end
    else if RestoreError <> '' then
      AError := AError + LineEnding + 'Откат также не удался: ' + RestoreError;
    Exit(False);
  end;
  FEnabledByZarya := True;
  Result := True;
end;

function TZaryaSystemProxyController.Restore(out AError: string): Boolean;
var
  StateToRestore: TZaryaSystemProxyState;
  ClearError: string;
begin
  AError := '';
  if not FHasSavedState then
  begin
    if not FileExists(FSnapshotFileName) then
      Exit(True);
    if not LoadSnapshot(StateToRestore, AError) then
      Exit(False);
  end
  else
    StateToRestore := FSavedState;
  if not Assigned(FBackend) then
  begin
    AError := 'Системный прокси не поддерживается.';
    Exit(False);
  end;
  if not FBackend.RestoreState(StateToRestore, AError) then
    Exit(False);
  Result := ClearSnapshot(ClearError);
  if not Result then
  begin
    AError := ClearError;
    Exit;
  end;
  FHasSavedState := False;
  FEnabledByZarya := False;
  FSavedState := Default(TZaryaSystemProxyState);
end;

function TZaryaSystemProxyController.RecoverPersisted(
  out AError: string): Boolean;
begin
  if not HasPersistedSnapshot then
  begin
    AError := '';
    Exit(True);
  end;
  Result := Restore(AError);
end;

function TZaryaSystemProxyController.BackendName: string;
begin
  if Assigned(FBackend) then
    Result := FBackend.BackendName
  else
    Result := 'Unsupported';
end;

end.
