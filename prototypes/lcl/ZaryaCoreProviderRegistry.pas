unit ZaryaCoreProviderRegistry;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, ZaryaCoreProvider, ZaryaCoreProviderStore;

type
  TZaryaCoreProviderRegistry = class
  private
    FStore: IZaryaCoreProviderStore;
    FProviders: TZaryaCoreProviders;
    function DetectProviderId(const AFileName, AOutput: string): string;
    function ProbeVersion(var AProvider: TZaryaCoreProvider;
      out AError: string): Boolean;
  public
    constructor Create(const AStore: IZaryaCoreProviderStore);
    function Load(out AError: string): Boolean;
    function Save(out AError: string): Boolean;
    procedure RefreshLocalState;
    function Count: Integer;
    function ProviderAt(const AIndex: Integer): TZaryaCoreProvider;
    function IndexOf(const AProviderId: string): Integer;
    function TryGet(const AProviderId: string;
      out AProvider: TZaryaCoreProvider): Boolean;
    function RegisterExecutable(const AFileName: string;
      out AProvider: TZaryaCoreProvider; out AError: string): Boolean;
    function ChangeExecutable(const AProviderId, AFileName: string;
      out AProvider: TZaryaCoreProvider; out AError: string): Boolean;
    function CheckProvider(const AProviderId: string;
      out AProvider: TZaryaCoreProvider; out AError: string): Boolean;
    function UpdateProvider(const AProvider: TZaryaCoreProvider;
      out AError: string): Boolean;
    function Remove(const AProviderId: string; out AError: string): Boolean;
    function ConfirmChanged(const AProviderId: string;
      out AError: string): Boolean;
    procedure SetEmbeddedState(const AProviderId, AVersion: string;
      const AAvailable: Boolean; const AError: string);
    function CompatibleProviderIds(const AProtocol,
      AConfigFormat: string): TZaryaStringArray;
  end;

implementation

uses
  ZaryaFileIntegrity, ZaryaRuntimeProcess, ZaryaFeatureGate;

function FirstNonEmptyLine(const AText: string): string;
var
  Lines: TStringList;
  I: Integer;
begin
  Result := '';
  Lines := TStringList.Create;
  try
    Lines.Text := AText;
    for I := 0 to Lines.Count - 1 do
      if Trim(Lines[I]) <> '' then
        Exit(Trim(Lines[I]));
  finally
    Lines.Free;
  end;
end;

function NewCustomProviderId: string;
var
  Value: TGuid;
begin
  if CreateGuid(Value) <> 0 then
    raise Exception.Create('Не удалось создать идентификатор custom provider.');
  Result := LowerCase(GuidToString(Value));
  Result := StringReplace(Result, '{', '', []);
  Result := StringReplace(Result, '}', '', []);
  Result := 'external.custom.' + Result;
end;

constructor TZaryaCoreProviderRegistry.Create(
  const AStore: IZaryaCoreProviderStore);
begin
  inherited Create;
  FStore := AStore;
end;

function TZaryaCoreProviderRegistry.Load(out AError: string): Boolean;
var
  Embedded: TZaryaCoreProviders;
  ExternalProviders: TZaryaCoreProviders;
  I: Integer;
begin
  Embedded := CreateEmbeddedProviders;
  if not TZaryaFeatureGate.EmbeddedSingBoxVisible then
    SetLength(Embedded, 1);
  if not FStore.Load(ExternalProviders, AError) then
    Exit(False);
  SetLength(FProviders, Length(Embedded) + Length(ExternalProviders));
  for I := 0 to High(Embedded) do
    FProviders[I] := Embedded[I];
  for I := 0 to High(ExternalProviders) do
    FProviders[Length(Embedded) + I] := ExternalProviders[I];
  RefreshLocalState;
  Result := True;
end;

function TZaryaCoreProviderRegistry.Save(out AError: string): Boolean;
begin
  Result := FStore.Save(FProviders, AError);
end;

function TZaryaCoreProviderRegistry.Count: Integer;
begin
  Result := Length(FProviders);
end;

function TZaryaCoreProviderRegistry.ProviderAt(
  const AIndex: Integer): TZaryaCoreProvider;
begin
  if (AIndex < 0) or (AIndex > High(FProviders)) then
    raise ERangeError.Create('Provider index out of range.');
  Result := FProviders[AIndex];
end;

function TZaryaCoreProviderRegistry.IndexOf(const AProviderId: string): Integer;
begin
  for Result := 0 to High(FProviders) do
    if SameText(FProviders[Result].ProviderId, AProviderId) then
      Exit;
  Result := -1;
end;

function TZaryaCoreProviderRegistry.TryGet(const AProviderId: string;
  out AProvider: TZaryaCoreProvider): Boolean;
var
  Index: Integer;
begin
  Index := IndexOf(AProviderId);
  Result := Index >= 0;
  if Result then
    AProvider := FProviders[Index]
  else
    AProvider := Default(TZaryaCoreProvider);
end;

function TZaryaCoreProviderRegistry.DetectProviderId(const AFileName,
  AOutput: string): string;
var
  Evidence: string;
begin
  Evidence := LowerCase(ExtractFileName(AFileName) + ' ' + AOutput);
  if (Pos('nekobox_core', Evidence) > 0) or
    (Pos('nekobox core', Evidence) > 0) then
    Exit(ProviderExternalNekoBoxCore);
  if Pos('mihomo', Evidence) > 0 then
    Exit(ProviderExternalMihomo);
  if (Pos('hysteria', Evidence) > 0) then
    Exit(ProviderExternalHysteria2);
  if (Pos('sing-box', Evidence) > 0) or (Pos('sing_box', Evidence) > 0) then
    Exit(ProviderExternalSingBox);
  if (Pos('v2ray', Evidence) > 0) and (Pos('xray', Evidence) = 0) then
    Exit(ProviderExternalV2Ray);
  if Pos('xray', Evidence) > 0 then
    Exit(ProviderExternalXray);
  Result := '';
end;

function TZaryaCoreProviderRegistry.ProbeVersion(
  var AProvider: TZaryaCoreProvider; out AError: string): Boolean;
var
  Output: string;
  ExitCode: Integer;
  DetectedProviderId: string;
begin
  if Length(AProvider.VersionArguments) = 0 then
  begin
    AProvider.Version := 'не определена';
    Exit(True);
  end;
  Result := RunProcessProbe(AProvider.ExecutablePath,
    AProvider.WorkingDirectory, AProvider.VersionArguments, 5000,
    Output, ExitCode, AError);
  if not Result then
  begin
    AProvider.LastError := AError;
    Exit;
  end;
  AProvider.Version := FirstNonEmptyLine(Output);
  if AProvider.Version = '' then
    AProvider.Version := 'не определена';
  DetectedProviderId := DetectProviderId(AProvider.ExecutablePath, Output);
  if (DetectedProviderId <> '') and
    (Pos('external.custom.', LowerCase(AProvider.ProviderId)) <> 1) and
    (not SameText(DetectedProviderId, AProvider.ProviderId)) then
  begin
    AError := Format('Version probe определил %s вместо %s.',
      [DetectedProviderId, AProvider.ProviderId]);
    AProvider.LastError := AError;
    Exit(False);
  end;
end;

function TZaryaCoreProviderRegistry.RegisterExecutable(
  const AFileName: string; out AProvider: TZaryaCoreProvider;
  out AError: string): Boolean;
var
  Architecture: string;
  Digest: string;
  ProviderId: string;
  ProbeOutput: string;
  ProbeError: string;
  ExitCode: Integer;
  Index: Integer;
begin
  AError := '';
  AProvider := Default(TZaryaCoreProvider);
  if not PeArchitecture(AFileName, Architecture, AError) then
    Exit(False);
  if not Sha256File(AFileName, Digest, AError) then
    Exit(False);

  ProviderId := DetectProviderId(AFileName, '');
  if ProviderId = '' then
  begin
    RunProcessProbe(AFileName, ExtractFileDir(AFileName),
      StringArray(['--version']), 5000, ProbeOutput, ExitCode, ProbeError);
    ProviderId := DetectProviderId(AFileName, ProbeOutput);
  end;
  if ProviderId = '' then
    ProviderId := NewCustomProviderId;

  AProvider := CreateProviderPreset(ProviderId);
  AProvider.ExecutablePath := ExpandFileName(AFileName);
  AProvider.WorkingDirectory := ExtractFileDir(AProvider.ExecutablePath);
  AProvider.Architecture := Architecture;
  AProvider.Sha256 := Digest;
  AProvider.ConfirmedSha256 := Digest;
  if not ProbeVersion(AProvider, AError) then
  begin
    AProvider.State := psFailed;
    Exit(False);
  end;
  AProvider.State := psAvailable;
  Index := IndexOf(AProvider.ProviderId);
  if Index < 0 then
  begin
    SetLength(FProviders, Length(FProviders) + 1);
    FProviders[High(FProviders)] := AProvider;
  end
  else if FProviders[Index].Distribution = pdExternal then
    FProviders[Index] := AProvider
  else
  begin
    AError := 'Нельзя заменить встроенный provider внешним EXE.';
    Exit(False);
  end;
  if not Save(AError) then
    Exit(False);
  Result := True;
end;

function TZaryaCoreProviderRegistry.UpdateProvider(
  const AProvider: TZaryaCoreProvider; out AError: string): Boolean;
var
  Index: Integer;
begin
  Index := IndexOf(AProvider.ProviderId);
  if Index < 0 then
  begin
    AError := 'Provider не найден.';
    Exit(False);
  end;
  if FProviders[Index].Distribution <> pdExternal then
  begin
    AError := 'Встроенный provider нельзя изменять.';
    Exit(False);
  end;
  FProviders[Index] := AProvider;
  Result := Save(AError);
end;

function TZaryaCoreProviderRegistry.ChangeExecutable(const AProviderId,
  AFileName: string; out AProvider: TZaryaCoreProvider;
  out AError: string): Boolean;
var
  Index: Integer;
  Architecture: string;
  Digest: string;
  DetectedId: string;
begin
  AError := '';
  AProvider := Default(TZaryaCoreProvider);
  Index := IndexOf(AProviderId);
  if (Index < 0) or (FProviders[Index].Distribution <> pdExternal) then
  begin
    AError := 'Внешний provider не найден.';
    Exit(False);
  end;
  if not PeArchitecture(AFileName, Architecture, AError) then
    Exit(False);
  if not Sha256File(AFileName, Digest, AError) then
    Exit(False);
  DetectedId := DetectProviderId(AFileName, '');
  if (DetectedId <> '') and (not SameText(DetectedId, AProviderId)) and
    (Pos('external.custom.', LowerCase(AProviderId)) <> 1) then
  begin
    AError := Format('Выбранный EXE похож на %s, а изменяется %s.',
      [DetectedId, AProviderId]);
    Exit(False);
  end;

  AProvider := FProviders[Index];
  AProvider.ExecutablePath := ExpandFileName(AFileName);
  AProvider.WorkingDirectory := ExtractFileDir(AProvider.ExecutablePath);
  AProvider.Architecture := Architecture;
  AProvider.Sha256 := Digest;
  AProvider.ConfirmedSha256 := Digest;
  if not ProbeVersion(AProvider, AError) then
  begin
    AProvider.State := psFailed;
    Exit(False);
  end;
  AProvider.State := psAvailable;
  FProviders[Index] := AProvider;
  Result := Save(AError);
end;

function TZaryaCoreProviderRegistry.CheckProvider(const AProviderId: string;
  out AProvider: TZaryaCoreProvider; out AError: string): Boolean;
var
  Index: Integer;
begin
  AError := '';
  RefreshLocalState;
  Index := IndexOf(AProviderId);
  if Index < 0 then
  begin
    AError := 'Provider не найден.';
    Exit(False);
  end;
  if FProviders[Index].Distribution = pdEmbedded then
  begin
    AProvider := FProviders[Index];
    Exit(AProvider.State = psAvailable);
  end;
  if FProviders[Index].State <> psAvailable then
  begin
    AProvider := FProviders[Index];
    if AProvider.LastError <> '' then
      AError := AProvider.LastError
    else
      AError := 'Provider недоступен: ' + StateToString(AProvider.State);
    Exit(False);
  end;
  Result := ProbeVersion(FProviders[Index], AError);
  if Result then
    Result := Save(AError);
  AProvider := FProviders[Index];
end;

function TZaryaCoreProviderRegistry.Remove(const AProviderId: string;
  out AError: string): Boolean;
var
  Index: Integer;
  I: Integer;
begin
  Index := IndexOf(AProviderId);
  if Index < 0 then
  begin
    AError := 'Provider не найден.';
    Exit(False);
  end;
  if FProviders[Index].Distribution <> pdExternal then
  begin
    AError := 'Встроенный provider нельзя удалить.';
    Exit(False);
  end;
  for I := Index to High(FProviders) - 1 do
    FProviders[I] := FProviders[I + 1];
  SetLength(FProviders, Length(FProviders) - 1);
  Result := Save(AError);
end;

procedure TZaryaCoreProviderRegistry.RefreshLocalState;
var
  I: Integer;
  Digest: string;
  ErrorMessage: string;
  Architecture: string;
begin
  for I := 0 to High(FProviders) do
  begin
    if FProviders[I].Distribution = pdEmbedded then
      Continue;
    FProviders[I].LastError := '';
    if not FileExists(FProviders[I].ExecutablePath) then
    begin
      FProviders[I].State := psMissing;
      Continue;
    end;
    if not PeArchitecture(FProviders[I].ExecutablePath, Architecture,
      ErrorMessage) then
    begin
      FProviders[I].Architecture := Architecture;
      FProviders[I].State := psIncompatible;
      FProviders[I].LastError := ErrorMessage;
      Continue;
    end;
    FProviders[I].Architecture := Architecture;
    if not Sha256File(FProviders[I].ExecutablePath, Digest, ErrorMessage) then
    begin
      FProviders[I].State := psFailed;
      FProviders[I].LastError := ErrorMessage;
      Continue;
    end;
    FProviders[I].Sha256 := Digest;
    if not SameText(Digest, FProviders[I].ConfirmedSha256) then
      FProviders[I].State := psChanged
    else
      FProviders[I].State := psAvailable;
  end;
end;

function TZaryaCoreProviderRegistry.ConfirmChanged(
  const AProviderId: string; out AError: string): Boolean;
var
  Index: Integer;
begin
  Index := IndexOf(AProviderId);
  if (Index < 0) or (FProviders[Index].Distribution <> pdExternal) then
  begin
    AError := 'Внешний provider не найден.';
    Exit(False);
  end;
  RefreshLocalState;
  if FProviders[Index].State = psMissing then
  begin
    AError := 'Файл provider не найден.';
    Exit(False);
  end;
  FProviders[Index].ConfirmedSha256 := FProviders[Index].Sha256;
  if not ProbeVersion(FProviders[Index], AError) then
  begin
    FProviders[Index].State := psFailed;
    Exit(False);
  end;
  FProviders[Index].State := psAvailable;
  Result := Save(AError);
end;

procedure TZaryaCoreProviderRegistry.SetEmbeddedState(const AProviderId,
  AVersion: string; const AAvailable: Boolean; const AError: string);
var
  Index: Integer;
begin
  Index := IndexOf(AProviderId);
  if Index < 0 then
    Exit;
  FProviders[Index].Version := AVersion;
  FProviders[Index].LastError := AError;
  if AAvailable then
    FProviders[Index].State := psAvailable
  else
    FProviders[Index].State := psMissing;
end;

function TZaryaCoreProviderRegistry.CompatibleProviderIds(
  const AProtocol, AConfigFormat: string): TZaryaStringArray;
var
  I: Integer;
  CountFound: Integer;
begin
  Result := nil;
  SetLength(Result, 0);
  CountFound := 0;
  for I := 0 to High(FProviders) do
  begin
    if FProviders[I].State <> psAvailable then
      Continue;
    if not ProviderSupportsProtocol(FProviders[I], AProtocol) then
      Continue;
    if (Trim(AConfigFormat) <> '') and
      (not SameText(ConfigFormatToString(FProviders[I].ConfigFormat),
        AConfigFormat)) then
      Continue;
    SetLength(Result, CountFound + 1);
    Result[CountFound] := FProviders[I].ProviderId;
    Inc(CountFound);
  end;
end;

end.
