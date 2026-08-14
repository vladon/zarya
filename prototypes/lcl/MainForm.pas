unit MainForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ComCtrls, Grids, Menus, ZaryaThemes, SettingsForm, ZaryaProfile,
  ZaryaProfileStore, FpcProfileStore, ProfileForm, ImportVlessForm,
  XrayConfigForm, ZaryaShareLink, ZaryaAppSettings,
  ZaryaEmbeddedXray, ZaryaSystemProxy, WindowsSystemProxy, ZaryaTcpProbe,
  ZaryaCoreProvider, ZaryaCoreProviderRegistry, FpcCoreProviderStore,
  CoreManagerForm, ZaryaRuntimeProcess, ZaryaRuntimeConfigFile,
  ProviderChoiceForm, ZaryaFileIntegrity, ZaryaRuntimeContracts,
  ZaryaConfigAdapters, SubscriptionManagerForm, ZaryaTcpLatency,
  ZaryaBackup, ZaryaDiagnostics;

type
  TRuntimeState = (rsStopped, rsConnecting, rsRunning);

  TZaryaTcpTestResult = record
    Tested: Boolean;
    Success: Boolean;
    LatencyMs: Integer;
    ErrorMessage: string;
  end;

  TZaryaTcpTestResults = array of TZaryaTcpTestResult;

  TProfileTcpTestThread = class(TThread)
  private
    FProfiles: TZaryaProfiles;
    FResults: TZaryaTcpTestResults;
    FCancelFlag: LongInt;
    FDoneFlag: LongInt;
    FCurrentIndex: Integer;
  protected
    procedure Execute; override;
  public
    constructor Create(const AProfiles: TZaryaProfiles);
    procedure RequestCancel;
    function IsDone: Boolean;
    property CurrentIndex: Integer read FCurrentIndex;
    property Results: TZaryaTcpTestResults read FResults;
  end;

  TMainForm = class(TForm)
  private
    FRuntimeState: TRuntimeState;
    FProfiles: TZaryaProfiles;
    FProfileStore: IZaryaProfileStore;
    FAppSettings: TZaryaAppSettings;
    FSettingsStore: TZaryaAppSettingsStore;
    FEmbeddedXray: TZaryaEmbeddedXray;
    FCoreRegistry: TZaryaCoreProviderRegistry;
    FExternalProcess: IZaryaRuntimeProcess;
    FActiveProvider: TZaryaCoreProvider;
    FActiveConfigPath: string;
    FReadinessHost: string;
    FReadinessPort: Integer;
    FActiveSystemProxyKind: string;
    FSystemProxy: TZaryaSystemProxyController;
    FXrayAssetDirectory: string;
    FReadyDeadline: QWord;
    FActiveProfile: TZaryaProfile;
    FDarkTheme: Boolean;
    FMinimizeToTray: Boolean;
    FQuitting: Boolean;
    FStatusPanel: TPanel;
    FStateTitle: TLabel;
    FStateBadge: TPanel;
    FStateDetail: TLabel;
    FStartButton: TButton;
    FStopButton: TButton;
    FSettingsButton: TButton;
    FToolbar: TPanel;
    FGrid: TStringGrid;
    FLogPanel: TPanel;
    FLogToolbar: TPanel;
    FLogFilter: TComboBox;
    FLogMemo: TMemo;
    FStatusBar: TStatusBar;
    FReadyTimer: TTimer;
    FTestTimer: TTimer;
    FTestThread: TProfileTcpTestThread;
    FTestButton: TButton;
    FTrayIcon: TTrayIcon;
    FTrayMenu: TPopupMenu;
    FStartMenuItem: TMenuItem;
    FStopMenuItem: TMenuItem;
    procedure BuildMenus;
    procedure BuildInterface;
    procedure BuildTray;
    procedure LoadAppSettings;
    procedure SaveAppSettings;
    function ResolveXrayAssetDirectory: string;
    function RedactRuntimeText(const AText: string): string;
    procedure DrainRuntimeLogs;
    procedure HandleRuntimeFailure(const AMessage: string);
    procedure StopRuntime(const AShowErrors: Boolean);
    procedure LoadProfiles;
    procedure RefreshProfileGrid(const ASelectedId: string = '');
    function SaveProfiles: Boolean;
    function SelectedProfileIndex: Integer;
    procedure UpdateRuntimeSurface;
    procedure ApplyCurrentTheme;
    procedure AppendLog(const AText: string);
    function SelectedProfileName: string;
    function ProviderUsableForProfile(const AProvider: TZaryaCoreProvider;
      const AProfile: TZaryaProfile): Boolean;
    function ResolveRuntimeProvider(const AProfileIndex: Integer;
      out AProvider: TZaryaCoreProvider): Boolean;
    function PrepareRuntimeConfig(const AProfile: TZaryaProfile;
      const AProvider: TZaryaCoreProvider; out AConfig,
      AError: string): Boolean;
    function StartExternalRuntime(const AProvider: TZaryaCoreProvider;
      const AConfig: string; out AError: string): Boolean;
    function ValidateEmbeddedRuntime(const AProvider: TZaryaCoreProvider;
      const AConfig: string; out AError: string): Boolean;
    function ActiveRuntimeIsRunning: Boolean;
    procedure StartClick(Sender: TObject);
    procedure StopClick(Sender: TObject);
    procedure ReadyTimerTimer(Sender: TObject);
    procedure TestTimerTimer(Sender: TObject);
    procedure TestClick(Sender: TObject);
    procedure PreviewConfigClick(Sender: TObject);
    procedure AddClick(Sender: TObject);
    procedure EditClick(Sender: TObject);
    procedure DeleteClick(Sender: TObject);
    procedure ImportClick(Sender: TObject);
    procedure SubscriptionsClick(Sender: TObject);
    procedure SettingsClick(Sender: TObject);
    procedure CoreManagerClick(Sender: TObject);
    procedure CreateBackupClick(Sender: TObject);
    procedure RestoreBackupClick(Sender: TObject);
    procedure DiagnosticsClick(Sender: TObject);
    procedure AboutClick(Sender: TObject);
    procedure ExitClick(Sender: TObject);
    procedure TrayShowClick(Sender: TObject);
    procedure ClearLogClick(Sender: TObject);
    procedure GridClick(Sender: TObject);
    procedure FormResize(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
  end;

implementation

constructor TProfileTcpTestThread.Create(const AProfiles: TZaryaProfiles);
var
  I: Integer;
begin
  inherited Create(True);
  FreeOnTerminate := False;
  SetLength(FProfiles, Length(AProfiles));
  for I := 0 to High(AProfiles) do FProfiles[I] := AProfiles[I];
  SetLength(FResults, Length(AProfiles));
  FCurrentIndex := -1;
  Start;
end;

procedure TProfileTcpTestThread.Execute;
var
  I: Integer;
begin
  try
    for I := 0 to High(FProfiles) do
    begin
      if InterlockedCompareExchange(FCancelFlag, 0, 0) <> 0 then Break;
      FCurrentIndex := I;
      if not FProfiles[I].Enabled or FProfiles[I].DeletedBySubscriptionUpdate then
        Continue;
      FResults[I].Tested := True;
      FResults[I].Success := MeasureTcpLatency(FProfiles[I].Host,
        FProfiles[I].Port, 3000, FResults[I].LatencyMs,
        FResults[I].ErrorMessage);
    end;
  finally
    FCurrentIndex := -1;
    InterlockedExchange(FDoneFlag, 1);
  end;
end;

procedure TProfileTcpTestThread.RequestCancel;
begin
  InterlockedExchange(FCancelFlag, 1);
end;

function TProfileTcpTestThread.IsDone: Boolean;
begin
  Result := InterlockedCompareExchange(FDoneFlag, 0, 0) <> 0;
end;

function NewMenuItem(AOwner: TComponent; const ACaption: string;
  AHandler: TNotifyEvent): TMenuItem;
begin
  Result := TMenuItem.Create(AOwner);
  Result.Caption := ACaption;
  Result.OnClick := AHandler;
end;

constructor TMainForm.Create(AOwner: TComponent);
var
  ErrorMessage: string;
begin
  inherited CreateNew(AOwner, 1);
  Caption := 'Zarya';
  Position := poScreenCenter;
  ClientWidth := 1040;
  ClientHeight := 720;
  Constraints.MinWidth := 800;
  Constraints.MinHeight := 560;
  Scaled := True;
  Randomize;
  FRuntimeState := rsStopped;
  FMinimizeToTray := True;
  OnResize := @FormResize;
  OnCloseQuery := @FormCloseQuery;
  BuildMenus;
  BuildInterface;
  BuildTray;
  FProfileStore := TFpcProfileStore.Create(ProfileStorePathFromCommandLine);
  FCoreRegistry := TZaryaCoreProviderRegistry.Create(TFpcCoreProviderStore.Create(
    IncludeTrailingPathDelimiter(ExtractFileDir(FProfileStore.FileName)) +
    'providers.json'));
  if not FCoreRegistry.Load(ErrorMessage) then
    MessageDlg('Ядра', 'Не удалось прочитать providers.json:' + LineEnding +
      ErrorMessage, mtWarning, [mbOK], 0);
  FSettingsStore := TZaryaAppSettingsStore.Create(
    IncludeTrailingPathDelimiter(ExtractFileDir(FProfileStore.FileName)) +
    'settings.ini');
  LoadAppSettings;
  FXrayAssetDirectory := ResolveXrayAssetDirectory;
  FEmbeddedXray := TZaryaEmbeddedXray.Create(
    IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) + 'zarya-xray.dll');
  FCoreRegistry.SetEmbeddedState(ProviderEmbeddedXray,
    FEmbeddedXray.Version, FEmbeddedXray.Available, FEmbeddedXray.LoadStatus);
  FSystemProxy := TZaryaSystemProxyController.Create(
    TWindowsSystemProxyBackend.Create,
    IncludeTrailingPathDelimiter(ExtractFileDir(FProfileStore.FileName)) +
    'proxy-previous-state.ini');
  LoadProfiles;
  ApplyCurrentTheme;
  UpdateRuntimeSurface;
  AppendLog('Zarya LCL started. Profiles: ' + FProfileStore.FileName);
  if FEmbeddedXray.Available then
    AppendLog(Format('Embedded Xray loaded: %s (ABI %d).',
      [FEmbeddedXray.Version, FEmbeddedXray.AbiVersion]))
  else
    AppendLog('Embedded Xray unavailable: ' + FEmbeddedXray.LoadStatus);
  AppendLog('System proxy backend: ' + FSystemProxy.BackendName + '.');
  if FSystemProxy.HasPersistedSnapshot then
  begin
    AppendLog('Found a previous proxy snapshot; attempting startup recovery.');
    if FSystemProxy.RecoverPersisted(ErrorMessage) then
      AppendLog('System proxy restored during startup recovery.')
    else
    begin
      AppendLog('System proxy recovery failed: ' + ErrorMessage);
      MessageDlg('Восстановление системного прокси', ErrorMessage, mtWarning,
        [mbOK], 0);
    end;
  end;
end;

procedure TMainForm.LoadAppSettings;
var
  ErrorMessage: string;
begin
  if not FSettingsStore.Load(FAppSettings, ErrorMessage) then
  begin
    FAppSettings := DefaultAppSettings;
    MessageDlg('Настройки', 'Не удалось прочитать settings.ini:' +
      LineEnding + ErrorMessage, mtWarning, [mbOK], 0);
  end;
  FDarkTheme := FAppSettings.DarkTheme;
  FMinimizeToTray := FAppSettings.MinimizeToTray;
end;

procedure TMainForm.SaveAppSettings;
var
  ErrorMessage: string;
begin
  if not FSettingsStore.Save(FAppSettings, ErrorMessage) then
    MessageDlg('Настройки', 'Не удалось сохранить settings.ini:' +
      LineEnding + ErrorMessage, mtError, [mbOK], 0);
end;

function TMainForm.ResolveXrayAssetDirectory: string;
var
  ApplicationAssets: string;
  DevelopmentAssets: string;
begin
  ApplicationAssets := IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
    'cores' + PathDelim + 'xray';
  DevelopmentAssets := ExpandFileName(
    IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
    '..' + PathDelim + '..' + PathDelim + '..' + PathDelim + 'build' +
    PathDelim + 'Release' + PathDelim + 'cores' + PathDelim + 'xray');
  if FileExists(IncludeTrailingPathDelimiter(ApplicationAssets) + 'geoip.dat') or
    FileExists(IncludeTrailingPathDelimiter(ApplicationAssets) + 'geosite.dat') then
    Result := ApplicationAssets
  else if DirectoryExists(DevelopmentAssets) then
    Result := DevelopmentAssets
  else
  begin
    Result := ApplicationAssets;
    ForceDirectories(Result);
  end;
end;

function TMainForm.RedactRuntimeText(const AText: string): string;
begin
  Result := AText;
  if FActiveProfile.Uuid <> '' then
    Result := StringReplace(Result, FActiveProfile.Uuid, '[uuid-redacted]',
      [rfReplaceAll]);
  if FActiveProfile.PublicKey <> '' then
    Result := StringReplace(Result, FActiveProfile.PublicKey,
      '[public-key-redacted]', [rfReplaceAll]);
  if FActiveProfile.Host <> '' then
    Result := StringReplace(Result, FActiveProfile.Host, '[server-redacted]',
      [rfReplaceAll, rfIgnoreCase]);
end;

procedure TMainForm.DrainRuntimeLogs;
var
  Lines: TStringList;
  I: Integer;
  RuntimeText: string;
  Prefix: string;
begin
  if FActiveProvider.Distribution = pdExternal then
  begin
    if not Assigned(FExternalProcess) then
      Exit;
    RuntimeText := FExternalProcess.DrainOutput;
    Prefix := '[' + FActiveProvider.DisplayName + '] ';
  end
  else
  begin
    if not Assigned(FEmbeddedXray) or not FEmbeddedXray.Available then
      Exit;
    RuntimeText := FEmbeddedXray.DrainLogs;
    Prefix := '[Xray] ';
  end;
  Lines := TStringList.Create;
  try
    Lines.Text := RedactRuntimeText(RuntimeText);
    for I := 0 to Lines.Count - 1 do
      if Trim(Lines[I]) <> '' then
        AppendLog(Prefix + Trim(Lines[I]));
  finally
    Lines.Free;
  end;
end;

procedure TMainForm.HandleRuntimeFailure(const AMessage: string);
var
  ErrorMessage: string;
begin
  FReadyTimer.Enabled := False;
  DrainRuntimeLogs;
  if Assigned(FSystemProxy) and FSystemProxy.EnabledByZarya then
  begin
    if FSystemProxy.Restore(ErrorMessage) then
      AppendLog('System proxy restored after runtime failure.')
    else
      AppendLog('System proxy restore failed: ' + ErrorMessage);
  end;
  if Assigned(FExternalProcess) then
  begin
    FExternalProcess.Stop;
    FExternalProcess := nil;
  end
  else if Assigned(FEmbeddedXray) then
    FEmbeddedXray.Stop(ErrorMessage);
  DeleteRuntimeConfig(FActiveConfigPath);
  FActiveConfigPath := '';
  FRuntimeState := rsStopped;
  AppendLog(AMessage);
  FActiveProfile := Default(TZaryaProfile);
  FActiveProvider := Default(TZaryaCoreProvider);
  UpdateRuntimeSurface;
  MessageDlg('Runtime provider', AMessage, mtError, [mbOK], 0);
end;

procedure TMainForm.StopRuntime(const AShowErrors: Boolean);
var
  ErrorMessage: string;
begin
  FReadyTimer.Enabled := False;
  if Assigned(FSystemProxy) and FSystemProxy.EnabledByZarya then
  begin
    if FAppSettings.RestoreSystemProxy then
    begin
      if FSystemProxy.Restore(ErrorMessage) then
        AppendLog('System proxy restored.')
      else
      begin
        AppendLog('System proxy restore failed: ' + ErrorMessage);
        if AShowErrors then
          MessageDlg('Системный прокси', ErrorMessage, mtError, [mbOK], 0);
      end;
    end
    else
      AppendLog('System proxy was left unchanged by user setting.');
  end;
  if Assigned(FEmbeddedXray) and
    (FActiveProvider.Distribution = pdEmbedded) and
    (FEmbeddedXray.State <> xrsStopped) then
  begin
    if not FEmbeddedXray.Stop(ErrorMessage) then
    begin
      AppendLog('Embedded Xray stop failed: ' + RedactRuntimeText(ErrorMessage));
      if AShowErrors then
        MessageDlg('Xray runtime', ErrorMessage, mtError, [mbOK], 0);
    end;
    DrainRuntimeLogs;
  end;
  if Assigned(FExternalProcess) then
  begin
    FExternalProcess.Stop;
    DrainRuntimeLogs;
    FExternalProcess := nil;
  end;
  DeleteRuntimeConfig(FActiveConfigPath);
  FActiveConfigPath := '';
  FRuntimeState := rsStopped;
  FActiveProfile := Default(TZaryaProfile);
  FActiveProvider := Default(TZaryaCoreProvider);
  if Assigned(FStatusBar) then
    UpdateRuntimeSurface;
end;

destructor TMainForm.Destroy;
begin
  if Assigned(FTestThread) then
  begin
    FTestThread.RequestCancel;
    FTestThread.WaitFor;
    FreeAndNil(FTestThread);
  end;
  StopRuntime(False);
  FSystemProxy.Free;
  FEmbeddedXray.Free;
  FCoreRegistry.Free;
  FSettingsStore.Free;
  FProfileStore := nil;
  inherited Destroy;
end;

procedure TMainForm.BuildMenus;
var
  RootItem: TMenuItem;
begin
  Menu := TMainMenu.Create(Self);

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Файл';
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, 'Создать &backup…', @CreateBackupClick));
  RootItem.Add(NewMenuItem(Menu, '&Восстановить backup…', @RestoreBackupClick));
  RootItem.Add(NewMenuItem(Menu, '-', nil));
  RootItem.Add(NewMenuItem(Menu, 'Скрыть в &tray', @TrayShowClick));
  RootItem.Add(NewMenuItem(Menu, '-', nil));
  RootItem.Add(NewMenuItem(Menu, 'Вы&ход', @ExitClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Профили';
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, '&Добавить…', @AddClick));
  RootItem.Add(NewMenuItem(Menu, '&Изменить…', @EditClick));
  RootItem.Add(NewMenuItem(Menu, '&Удалить', @DeleteClick));
  RootItem.Add(NewMenuItem(Menu, '-', nil));
  RootItem.Add(NewMenuItem(Menu, '&Импортировать…', @ImportClick));
  RootItem.Add(NewMenuItem(Menu, 'Под&писки…', @SubscriptionsClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Core';
  Menu.Items.Add(RootItem);
  FStartMenuItem := NewMenuItem(Menu, '&Запустить', @StartClick);
  FStopMenuItem := NewMenuItem(Menu, '&Остановить', @StopClick);
  RootItem.Add(FStartMenuItem);
  RootItem.Add(FStopMenuItem);
  RootItem.Add(NewMenuItem(Menu, '-', nil));
  RootItem.Add(NewMenuItem(Menu, '&Менеджер ядер…', @CoreManagerClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Инструменты';
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, '&Runtime config…', @PreviewConfigClick));
  RootItem.Add(NewMenuItem(Menu, '&Диагностика…', @DiagnosticsClick));
  RootItem.Add(NewMenuItem(Menu, '&Настройки…', @SettingsClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Справка';
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, '&О прототипе', @AboutClick));
end;

procedure TMainForm.BuildInterface;
var
  AddButton: TButton;
  EditButton: TButton;
  DeleteButton: TButton;
  ImportButton: TButton;
  SubscriptionsButton: TButton;
  CoresButton: TButton;
  ConfigButton: TButton;
  ClearButton: TButton;
  LogLabel: TLabel;
  Splitter: TSplitter;
begin
  FStatusBar := TStatusBar.Create(Self);
  FStatusBar.Parent := Self;
  FStatusBar.Align := alBottom;
  FStatusBar.SimplePanel := True;
  FStatusBar.SimpleText := 'Готово · embedded Xray';

  FStatusPanel := TPanel.Create(Self);
  FStatusPanel.Parent := Self;
  FStatusPanel.Align := alTop;
  FStatusPanel.Height := 174;
  FStatusPanel.BevelOuter := bvNone;

  FStateTitle := TLabel.Create(Self);
  FStateTitle.Parent := FStatusPanel;
  FStateTitle.SetBounds(20, 16, 650, 26);
  FStateTitle.Font.Size := 12;
  FStateTitle.Font.Style := [fsBold];

  FStateBadge := TPanel.Create(Self);
  FStateBadge.Parent := FStatusPanel;
  FStateBadge.BevelOuter := bvNone;
  FStateBadge.SetBounds(20, 48, 184, 28);
  FStateBadge.Caption := 'Остановлено';

  FStateDetail := TLabel.Create(Self);
  FStateDetail.Parent := FStatusPanel;
  FStateDetail.WordWrap := True;
  FStateDetail.SetBounds(20, 88, 650, 70);

  FStartButton := TButton.Create(Self);
  FStartButton.Parent := FStatusPanel;
  FStartButton.Caption := 'Запустить';
  FStartButton.OnClick := @StartClick;
  FStartButton.Default := True;
  FStartButton.SetBounds(708, 30, 132, 36);

  FStopButton := TButton.Create(Self);
  FStopButton.Parent := FStatusPanel;
  FStopButton.Caption := 'Остановить';
  FStopButton.OnClick := @StopClick;
  FStopButton.SetBounds(850, 30, 132, 36);

  FSettingsButton := TButton.Create(Self);
  FSettingsButton.Parent := FStatusPanel;
  FSettingsButton.Caption := 'Настройки…';
  FSettingsButton.OnClick := @SettingsClick;
  FSettingsButton.SetBounds(850, 76, 132, 32);

  FToolbar := TPanel.Create(Self);
  FToolbar.Parent := Self;
  FToolbar.Align := alTop;
  FToolbar.Top := FStatusPanel.Height;
  FToolbar.Height := 48;
  FToolbar.BevelOuter := bvNone;

  AddButton := TButton.Create(Self);
  AddButton.Parent := FToolbar;
  AddButton.Caption := 'Добавить';
  AddButton.OnClick := @AddClick;
  AddButton.SetBounds(12, 8, 92, 32);

  EditButton := TButton.Create(Self);
  EditButton.Parent := FToolbar;
  EditButton.Caption := 'Изменить';
  EditButton.OnClick := @EditClick;
  EditButton.SetBounds(110, 8, 92, 32);

  DeleteButton := TButton.Create(Self);
  DeleteButton.Parent := FToolbar;
  DeleteButton.Caption := 'Удалить';
  DeleteButton.OnClick := @DeleteClick;
  DeleteButton.SetBounds(208, 8, 92, 32);

  ImportButton := TButton.Create(Self);
  ImportButton.Parent := FToolbar;
  ImportButton.Caption := 'Импорт';
  ImportButton.OnClick := @ImportClick;
  ImportButton.SetBounds(312, 8, 92, 32);

  SubscriptionsButton := TButton.Create(Self);
  SubscriptionsButton.Parent := FToolbar;
  SubscriptionsButton.Caption := 'Подписки';
  SubscriptionsButton.OnClick := @SubscriptionsClick;
  SubscriptionsButton.SetBounds(410, 8, 100, 32);

  FTestButton := TButton.Create(Self);
  FTestButton.Parent := FToolbar;
  FTestButton.Caption := 'TCP ping';
  FTestButton.OnClick := @TestClick;
  FTestButton.SetBounds(522, 8, 100, 32);

  CoresButton := TButton.Create(Self);
  CoresButton.Parent := FToolbar;
  CoresButton.Caption := 'Ядра';
  CoresButton.OnClick := @CoreManagerClick;
  CoresButton.SetBounds(634, 8, 88, 32);

  ConfigButton := TButton.Create(Self);
  ConfigButton.Parent := FToolbar;
  ConfigButton.Caption := 'Runtime config…';
  ConfigButton.OnClick := @PreviewConfigClick;
  ConfigButton.SetBounds(730, 8, 112, 32);

  FTestTimer := TTimer.Create(Self);
  FTestTimer.Enabled := False;
  FTestTimer.Interval := 100;
  FTestTimer.OnTimer := @TestTimerTimer;

  FLogPanel := TPanel.Create(Self);
  FLogPanel.Parent := Self;
  FLogPanel.Align := alBottom;
  FLogPanel.Height := 220;
  FLogPanel.BevelOuter := bvNone;

  FLogToolbar := TPanel.Create(Self);
  FLogToolbar.Parent := FLogPanel;
  FLogToolbar.Align := alTop;
  FLogToolbar.Height := 40;
  FLogToolbar.BevelOuter := bvNone;

  LogLabel := TLabel.Create(Self);
  LogLabel.Parent := FLogToolbar;
  LogLabel.Caption := 'Журнал';
  LogLabel.Font.Style := [fsBold];
  LogLabel.SetBounds(12, 12, 70, 20);

  FLogFilter := TComboBox.Create(Self);
  FLogFilter.Parent := FLogToolbar;
  FLogFilter.Style := csDropDownList;
  FLogFilter.Items.Add('Все сообщения');
  FLogFilter.Items.Add('Ошибки');
  FLogFilter.Items.Add('Runtime');
  FLogFilter.ItemIndex := 0;
  FLogFilter.SetBounds(92, 6, 160, 30);

  ClearButton := TButton.Create(Self);
  ClearButton.Parent := FLogToolbar;
  ClearButton.Caption := 'Очистить';
  ClearButton.OnClick := @ClearLogClick;
  ClearButton.SetBounds(262, 6, 88, 30);

  FLogMemo := TMemo.Create(Self);
  FLogMemo.Parent := FLogPanel;
  FLogMemo.Align := alClient;
  FLogMemo.ReadOnly := True;
  FLogMemo.ScrollBars := ssAutoVertical;
  FLogMemo.Font.Name := 'Consolas';
  FLogMemo.Font.Size := 9;

  Splitter := TSplitter.Create(Self);
  Splitter.Parent := Self;
  Splitter.Align := alBottom;
  Splitter.Height := 6;
  Splitter.Top := FLogPanel.Top - 6;

  FGrid := TStringGrid.Create(Self);
  FGrid.Parent := Self;
  FGrid.Align := alClient;
  FGrid.BorderSpacing.Around := 12;
  FGrid.FixedCols := 0;
  FGrid.FixedRows := 1;
  FGrid.ColCount := 7;
  FGrid.RowCount := 5;
  FGrid.Row := 1;
  FGrid.Options := FGrid.Options + [goRowSelect, goColSizing, goDblClickAutoSize];
  FGrid.OnClick := @GridClick;
  FGrid.OnDblClick := @EditClick;
  FGrid.Cells[0, 0] := 'Профиль';
  FGrid.Cells[1, 0] := 'Протокол';
  FGrid.Cells[2, 0] := 'Сервер';
  FGrid.Cells[3, 0] := 'Задержка';
  FGrid.Cells[4, 0] := 'Источник';
  FGrid.Cells[5, 0] := 'Provider';
  FGrid.Cells[6, 0] := 'Статус';
  FGrid.ColWidths[0] := 190;
  FGrid.ColWidths[1] := 90;
  FGrid.ColWidths[2] := 230;
  FGrid.ColWidths[3] := 90;
  FGrid.ColWidths[4] := 130;
  FGrid.ColWidths[5] := 150;
  FGrid.ColWidths[6] := 90;

  FReadyTimer := TTimer.Create(Self);
  FReadyTimer.Enabled := False;
  FReadyTimer.Interval := 100;
  FReadyTimer.OnTimer := @ReadyTimerTimer;
end;

procedure TMainForm.BuildTray;
var
  Item: TMenuItem;
begin
  FTrayMenu := TPopupMenu.Create(Self);
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, 'Показать Zarya', @TrayShowClick));
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, 'Запустить', @StartClick));
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, 'Остановить', @StopClick));
  Item := NewMenuItem(FTrayMenu, '-', nil);
  FTrayMenu.Items.Add(Item);
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, 'Выход', @ExitClick));

  FTrayIcon := TTrayIcon.Create(Self);
  FTrayIcon.Hint := 'Zarya — остановлено';
  FTrayIcon.PopUpMenu := FTrayMenu;
  FTrayIcon.OnDblClick := @TrayShowClick;
  if not Application.Icon.Empty then
    FTrayIcon.Icon.Assign(Application.Icon);
  FTrayIcon.Visible := True;
end;

procedure TMainForm.LoadProfiles;
var
  ErrorMessage: string;
begin
  if not FProfileStore.Load(FProfiles, ErrorMessage) then
  begin
    MessageDlg('Профили', 'Не удалось прочитать profiles.json:' + LineEnding +
      ErrorMessage, mtWarning, [mbOK], 0);
    SetLength(FProfiles, 0);
  end;
  RefreshProfileGrid;
end;

procedure TMainForm.RefreshProfileGrid(const ASelectedId: string);
var
  I: Integer;
  SelectedRow: Integer;
begin
  FGrid.RowCount := Length(FProfiles) + 1;
  SelectedRow := 1;
  for I := 0 to High(FProfiles) do
  begin
    FGrid.Cells[0, I + 1] := FProfiles[I].Name;
    FGrid.Cells[1, I + 1] := FProfiles[I].ProtocolName;
    FGrid.Cells[2, I + 1] := ProfileEndpoint(FProfiles[I]);
    FGrid.Cells[3, I + 1] := FProfiles[I].Latency;
    FGrid.Cells[4, I + 1] := FProfiles[I].Source;
    if FProfiles[I].Enabled then
      FGrid.Cells[6, I + 1] := 'Готов'
    else
      FGrid.Cells[6, I + 1] := 'Выключен';
    FGrid.Cells[5, I + 1] := FProfiles[I].PreferredProviderId;
    if (ASelectedId <> '') and (FProfiles[I].Id = ASelectedId) then
      SelectedRow := I + 1;
  end;
  if Length(FProfiles) > 0 then
    FGrid.Row := SelectedRow;
  UpdateRuntimeSurface;
end;

function TMainForm.SaveProfiles: Boolean;
var
  ErrorMessage: string;
begin
  Result := FProfileStore.Save(FProfiles, ErrorMessage);
  if not Result then
    MessageDlg('Профили', 'Не удалось сохранить profiles.json:' + LineEnding +
      ErrorMessage, mtError, [mbOK], 0);
end;

function TMainForm.SelectedProfileIndex: Integer;
begin
  Result := FGrid.Row - 1;
  if (Result < 0) or (Result > High(FProfiles)) then
    Result := -1;
end;

function TMainForm.SelectedProfileName: string;
var
  Index: Integer;
begin
  if (FRuntimeState <> rsStopped) and (FActiveProfile.Name <> '') then
    Exit(FActiveProfile.Name);
  Index := SelectedProfileIndex;
  if Index >= 0 then
    Result := FProfiles[Index].Name
  else
    Result := '—';
end;

procedure TMainForm.AppendLog(const AText: string);
begin
  FLogMemo.Lines.Add(FormatDateTime('hh:nn:ss', Now) + '  ' + AText);
  FLogMemo.SelStart := Length(FLogMemo.Text);
end;

procedure TMainForm.UpdateRuntimeSurface;
var
  Theme: TZaryaTheme;
  ProxyStatus: string;
  ProviderText: string;
  Index: Integer;
begin
  if FDarkTheme then
    Theme := ZaryaThemes.DarkTheme
  else
    Theme := ZaryaThemes.LightTheme;

  case FRuntimeState of
    rsStopped:
      begin
        Index := SelectedProfileIndex;
        if Index >= 0 then
          ProviderText := FProfiles[Index].PreferredProviderId
        else
          ProviderText := '—';
        FStateTitle.Caption := 'Runtime: остановлен';
        FStateBadge.Caption := 'Остановлено';
        FStateBadge.Color := Theme.Panel;
        FStateBadge.Font.Color := Theme.Muted;
        FStateDetail.Caption :=
          'Выбранный профиль: ' + SelectedProfileName + LineEnding +
          'Provider: ' + ProviderText +
          '     Системный прокси: выключен';
        FStartButton.Enabled := True;
        FStopButton.Enabled := False;
        FStartMenuItem.Enabled := True;
        FStopMenuItem.Enabled := False;
        FStatusBar.SimpleText := 'Готово · системный прокси выключен';
        FTrayIcon.Hint := 'Zarya — остановлено';
      end;
    rsConnecting:
      begin
        FStateTitle.Caption := 'Runtime: подключение — ' +
          FActiveProvider.ProviderId;
        FStateBadge.Caption := 'Проверка готовности';
        FStateBadge.Color := Theme.WarningSurface;
        FStateBadge.Font.Color := Theme.Warning;
        FStateDetail.Caption :=
          'Профиль: ' + SelectedProfileName + LineEnding +
          Format('Local endpoint: %s:%d     Системный прокси: пока выключен',
            [FReadinessHost, FReadinessPort]);
        FStartButton.Enabled := False;
        FStopButton.Enabled := True;
        FStartMenuItem.Enabled := False;
        FStopMenuItem.Enabled := True;
        FStatusBar.SimpleText := Format('Ожидание готовности %s:%d…',
          [FReadinessHost, FReadinessPort]);
        FTrayIcon.Hint := 'Zarya — подключение';
      end;
    rsRunning:
      begin
        if Assigned(FSystemProxy) and FSystemProxy.EnabledByZarya then
          ProxyStatus := 'включён'
        else
          ProxyStatus := 'выключен';
        FStateTitle.Caption := 'Runtime: работает — ' +
          FActiveProvider.ProviderId;
        FStateBadge.Caption := 'Подключено';
        FStateBadge.Color := Theme.SuccessSurface;
        FStateBadge.Font.Color := Theme.Success;
        FStateDetail.Caption :=
          'Профиль: ' + SelectedProfileName + LineEnding +
          Format('Local endpoint: %s:%d     Системный прокси: %s',
            [FReadinessHost, FReadinessPort, ProxyStatus]);
        FStartButton.Enabled := False;
        FStopButton.Enabled := True;
        FStartMenuItem.Enabled := False;
        FStopMenuItem.Enabled := True;
        FStatusBar.SimpleText := Format('Подключено · %s:%d · %s',
          [FReadinessHost, FReadinessPort, FActiveProvider.ProviderId]);
        FTrayIcon.Hint := 'Zarya — подключено';
      end;
  end;
end;

procedure TMainForm.ApplyCurrentTheme;
var
  Theme: TZaryaTheme;
begin
  if FDarkTheme then
    Theme := ZaryaThemes.DarkTheme
  else
    Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
  FStatusPanel.Color := Theme.Surface;
  FToolbar.Color := Theme.Panel;
  FLogToolbar.Color := Theme.Panel;
  FLogPanel.Color := Theme.Surface;
  FStateTitle.Font.Size := 12;
  FStateTitle.Font.Style := [fsBold];
  UpdateRuntimeSurface;
end;

function TMainForm.ProviderUsableForProfile(
  const AProvider: TZaryaCoreProvider;
  const AProfile: TZaryaProfile): Boolean;
var
  RequiredFormat: string;
begin
  Result := False;
  if AProvider.State <> psAvailable then
    Exit;
  if not ProviderSupportsProtocol(AProvider, AProfile.ProtocolName) then
    Exit;
  if AProfile.RawConfig <> '' then
  begin
    RequiredFormat := AProfile.RawConfigFormat;
    if RequiredFormat = '' then
      RequiredFormat := 'raw';
    Exit(SameText(ConfigFormatToString(AProvider.ConfigFormat),
      RequiredFormat));
  end;
  Result := ProviderCanGenerateConfig(AProvider, AProfile, RequiredFormat);
end;

function TMainForm.ResolveRuntimeProvider(const AProfileIndex: Integer;
  out AProvider: TZaryaCoreProvider): Boolean;
var
  PreferredId: string;
  CompatibleIds: TZaryaStringArray;
  Candidate: TZaryaCoreProvider;
  I: Integer;
  CountFound: Integer;
  SelectedId: string;
  SaveToProfile: Boolean;
begin
  Result := False;
  AProvider := Default(TZaryaCoreProvider);
  FCoreRegistry.RefreshLocalState;
  PreferredId := FProfiles[AProfileIndex].PreferredProviderId;
  if PreferredId = '' then
    PreferredId := DefaultProviderForProtocol(
      FProfiles[AProfileIndex].ProtocolName);
  if FCoreRegistry.TryGet(PreferredId, Candidate) and
    ProviderUsableForProfile(Candidate, FProfiles[AProfileIndex]) then
  begin
    AProvider := Candidate;
    Exit(True);
  end;

  CompatibleIds := nil;
  CountFound := 0;
  for I := 0 to FCoreRegistry.Count - 1 do
  begin
    Candidate := FCoreRegistry.ProviderAt(I);
    if not ProviderUsableForProfile(Candidate, FProfiles[AProfileIndex]) then
      Continue;
    SetLength(CompatibleIds, CountFound + 1);
    CompatibleIds[CountFound] := Candidate.ProviderId;
    Inc(CountFound);
  end;
  if CountFound = 0 then
  begin
    MessageDlg('Runtime provider',
      'Нет доступного provider с совместимым протоколом и форматом конфигурации. ' +
      'Добавьте ядро в Core Manager или выберите подходящий raw-диалект.',
      mtError, [mbOK], 0);
    Exit;
  end;
  if not TProviderChoiceDialog.Execute(Self, FCoreRegistry, CompatibleIds,
    PreferredId, FDarkTheme, SelectedId, SaveToProfile) then
    Exit;
  if not FCoreRegistry.TryGet(SelectedId, AProvider) then
    Exit;
  if SaveToProfile then
  begin
    FProfiles[AProfileIndex].PreferredProviderId := SelectedId;
    if not SaveProfiles then
      Exit;
    RefreshProfileGrid(FProfiles[AProfileIndex].Id);
  end;
  Result := True;
end;

function TMainForm.PrepareRuntimeConfig(const AProfile: TZaryaProfile;
  const AProvider: TZaryaCoreProvider; out AConfig,
  AError: string): Boolean;
var
  Adapter: IConfigAdapter;
  Context: TZaryaConfigContext;
begin
  AConfig := '';
  AError := '';
  if AProfile.RawConfig <> '' then
  begin
    if not SameText(ConfigFormatToString(AProvider.ConfigFormat),
      AProfile.RawConfigFormat) then
    begin
      AError := 'Raw-конфигурация не соответствует диалекту provider.';
      Exit(False);
    end;
    if not SameText(AProfile.ReadinessHost, '127.0.0.1') and
      not SameText(AProfile.ReadinessHost, 'localhost') then
    begin
      AError := 'Readiness endpoint raw-профиля должен быть локальным.';
      Exit(False);
    end;
    if (AProfile.ReadinessPort < 1) or
      (AProfile.ReadinessPort > 65535) then
    begin
      AError := 'Для raw-профиля укажите readiness port.';
      Exit(False);
    end;
    AConfig := AProfile.RawConfig;
    FReadinessHost := AProfile.ReadinessHost;
    FReadinessPort := AProfile.ReadinessPort;
    FActiveSystemProxyKind := AProfile.SystemProxyKind;
    Exit(True);
  end;
  Adapter := CreateConfigAdapter(AProvider);
  if not Assigned(Adapter) then
  begin
    AError := 'Для adapter ' + AProvider.AdapterId +
      ' не зарегистрирован генератор; используйте raw config.';
    Exit(False);
  end;
  Context := Default(TZaryaConfigContext);
  Context.MixedPort := FAppSettings.MixedPort;
  Context.HttpPort := FAppSettings.MixedPort;
  Context.SocksPort := FAppSettings.MixedPort;
  if SameText(AProvider.AdapterId, 'hysteria2') then
    if FAppSettings.MixedPort < 65535 then
      Context.SocksPort := FAppSettings.MixedPort + 1
    else
      Context.SocksPort := FAppSettings.MixedPort - 1;
  Result := Adapter.Generate(AProfile, Context, AConfig, AError);
  if Result then
  begin
    FReadinessHost := '127.0.0.1';
    case AProvider.ReadinessKind of
      rkMixedTcp:
        begin
          FReadinessPort := Context.MixedPort;
          FActiveSystemProxyKind := 'mixed';
        end;
      rkHttpTcp:
        begin
          FReadinessPort := Context.HttpPort;
          FActiveSystemProxyKind := 'http';
        end;
      rkSocksTcp:
        begin
          FReadinessPort := Context.SocksPort;
          FActiveSystemProxyKind := 'socks';
        end;
    else
      FReadinessPort := Context.MixedPort;
      FActiveSystemProxyKind := 'none';
    end;
  end;
end;

function TMainForm.StartExternalRuntime(
  const AProvider: TZaryaCoreProvider; const AConfig: string;
  out AError: string): Boolean;
var
  DataDirectory: string;
  Context: TZaryaProcessContext;
  Arguments: TZaryaStringArray;
  Output: string;
  CurrentDigest: string;
  ExitCode: Integer;
begin
  Result := False;
  AError := '';
  if not Sha256File(AProvider.ExecutablePath, CurrentDigest, AError) then
    Exit;
  if not SameText(CurrentDigest, AProvider.ConfirmedSha256) then
  begin
    AError := 'EXE provider изменился после последнего подтверждения. ' +
      'Откройте Core Manager и подтвердите новый SHA-256.';
    Exit;
  end;
  DataDirectory := ExtractFileDir(FProfileStore.FileName);
  if not WriteRuntimeConfig(DataDirectory, AProvider, AConfig,
    FActiveConfigPath, AError) then
    Exit;
  Context := Default(TZaryaProcessContext);
  Context.ConfigPath := FActiveConfigPath;
  Context.DataDirectory := DataDirectory;
  Context.AssetDirectory := AProvider.AssetDirectory;
  if Context.AssetDirectory = '' then
    Context.AssetDirectory := FXrayAssetDirectory;
  Context.MixedPort := FReadinessPort;
  Context.HttpPort := FReadinessPort;
  Context.SocksPort := FReadinessPort;
  Context.LogLevel := 'warning';

  if Length(AProvider.ValidateArguments) > 0 then
  begin
    if not ExpandProviderArguments(AProvider.ValidateArguments, Context,
      Arguments, AError) then
      Exit;
    AppendLog('Validating configuration with ' + AProvider.ProviderId + '…');
    if not RunProcessProbe(AProvider.ExecutablePath,
      AProvider.WorkingDirectory, Arguments, 5000, Output, ExitCode,
      AError) then
    begin
      if Trim(Output) <> '' then
        AError := AError + LineEnding + RedactRuntimeText(Output);
      Exit;
    end;
  end
  else
    AppendLog(AProvider.ProviderId + ' does not provide config validation.');

  if not ExpandProviderArguments(AProvider.RunArguments, Context,
    Arguments, AError) then
    Exit;
  FExternalProcess := TZaryaExternalProcess.Create;
  if not FExternalProcess.Start(AProvider.ExecutablePath,
    AProvider.WorkingDirectory, Arguments, AError) then
  begin
    FExternalProcess := nil;
    Exit;
  end;
  Result := True;
end;

function TMainForm.ActiveRuntimeIsRunning: Boolean;
begin
  if FActiveProvider.Distribution = pdExternal then
    Result := Assigned(FExternalProcess) and FExternalProcess.IsRunning
  else
    Result := Assigned(FEmbeddedXray) and
      not (FEmbeddedXray.State in [xrsFailed, xrsStopped]);
end;

function TMainForm.ValidateEmbeddedRuntime(
  const AProvider: TZaryaCoreProvider; const AConfig: string;
  out AError: string): Boolean;
var
  Arguments: TZaryaStringArray;
  ConfigPath: string;
  ErrorFile: string;
  DataDirectory: string;
  Output: string;
  ExitCode: Integer;
  ErrorLines: TStringList;
begin
  Result := False;
  AError := '';
  ConfigPath := '';
  DataDirectory := ExtractFileDir(FProfileStore.FileName);
  if not WriteRuntimeConfig(DataDirectory, AProvider, AConfig, ConfigPath,
    AError) then
    Exit;
  ErrorFile := ConfigPath + '.validation-error';
  try
    DeleteFile(ErrorFile);
    SetLength(Arguments, 4);
    Arguments[0] := '--embedded-validate';
    Arguments[1] := ConfigPath;
    Arguments[2] := FXrayAssetDirectory;
    Arguments[3] := ErrorFile;
    AppendLog('Validating embedded Xray configuration in an isolated worker…');
    Result := RunProcessProbe(ParamStr(0), ExtractFileDir(ParamStr(0)),
      Arguments, 10000, Output, ExitCode, AError);
    if not Result then
    begin
      if FileExists(ErrorFile) then
      begin
        ErrorLines := TStringList.Create;
        try
          ErrorLines.LoadFromFile(ErrorFile);
          if Trim(ErrorLines.Text) <> '' then
            AError := RedactRuntimeText(Trim(ErrorLines.Text));
        finally
          ErrorLines.Free;
        end;
      end
      else if Trim(Output) <> '' then
        AError := AError + LineEnding + RedactRuntimeText(Output);
    end;
  finally
    DeleteFile(ErrorFile);
    DeleteRuntimeConfig(ConfigPath);
  end;
end;

procedure TMainForm.StartClick(Sender: TObject);
var
  Index: Integer;
  Config: string;
  ErrorMessage: string;
  Provider: TZaryaCoreProvider;
begin
  if FRuntimeState <> rsStopped then
    Exit;
  Index := SelectedProfileIndex;
  if Index < 0 then
  begin
    MessageDlg('Запуск', 'Сначала выберите профиль.', mtInformation, [mbOK], 0);
    Exit;
  end;
  if not FProfiles[Index].Enabled then
  begin
    MessageDlg('Запуск', 'Выбранный профиль выключен.', mtInformation, [mbOK], 0);
    Exit;
  end;
  if not ResolveRuntimeProvider(Index, Provider) then
    Exit;
  if not PrepareRuntimeConfig(FProfiles[Index], Provider, Config,
    ErrorMessage) then
  begin
    MessageDlg('Конфигурация provider', ErrorMessage, mtWarning, [mbOK], 0);
    Exit;
  end;
  if CanConnectLocalhost(FReadinessPort) then
  begin
    MessageDlg('Runtime provider', Format(
      'Порт %s:%d уже занят. Выберите другой локальный порт.',
      [FReadinessHost, FReadinessPort]), mtWarning, [mbOK], 0);
    Exit;
  end;
  ForceDirectories(FXrayAssetDirectory);
  FActiveProfile := FProfiles[Index];
  FActiveProvider := Provider;
  if Provider.Distribution = pdExternal then
  begin
    AppendLog('Starting external provider ' + Provider.ProviderId + '…');
    if not StartExternalRuntime(Provider, Config, ErrorMessage) then
    begin
      DeleteRuntimeConfig(FActiveConfigPath);
      FActiveConfigPath := '';
      FActiveProfile := Default(TZaryaProfile);
      FActiveProvider := Default(TZaryaCoreProvider);
      MessageDlg('External runtime', RedactRuntimeText(ErrorMessage),
        mtError, [mbOK], 0);
      Exit;
    end;
  end
  else
  begin
    if not Assigned(FEmbeddedXray) or not FEmbeddedXray.Available then
    begin
      MessageDlg('Xray runtime', FEmbeddedXray.LoadStatus, mtError, [mbOK], 0);
      Exit;
    end;
    if not ValidateEmbeddedRuntime(Provider, Config, ErrorMessage) then
    begin
      AppendLog('Embedded Xray validation failed.');
      ErrorMessage := RedactRuntimeText(ErrorMessage);
      FActiveProfile := Default(TZaryaProfile);
      FActiveProvider := Default(TZaryaCoreProvider);
      MessageDlg('Xray runtime', ErrorMessage, mtError, [mbOK], 0);
      Exit;
    end;
    AppendLog('Starting embedded Xray…');
    if not FEmbeddedXray.Start(Config, FXrayAssetDirectory, ErrorMessage) then
    begin
      AppendLog('Embedded Xray start failed.');
      ErrorMessage := RedactRuntimeText(ErrorMessage);
      FActiveProfile := Default(TZaryaProfile);
      FActiveProvider := Default(TZaryaCoreProvider);
      MessageDlg('Xray runtime', ErrorMessage, mtError, [mbOK], 0);
      Exit;
    end;
  end;
  FRuntimeState := rsConnecting;
  FReadyDeadline := GetTickCount64 + 5000;
  FReadyTimer.Interval := 100;
  AppendLog(Format('Waiting for local endpoint %s:%d before enabling system proxy…',
    [FReadinessHost, FReadinessPort]));
  FReadyTimer.Enabled := True;
  UpdateRuntimeSurface;
end;

procedure TMainForm.StopClick(Sender: TObject);
begin
  if FRuntimeState = rsStopped then
    Exit;
  AppendLog('Stopping runtime provider ' + FActiveProvider.ProviderId + '…');
  StopRuntime(True);
end;

procedure TMainForm.ReadyTimerTimer(Sender: TObject);
var
  ErrorMessage: string;
begin
  DrainRuntimeLogs;
  if not ActiveRuntimeIsRunning then
  begin
    HandleRuntimeFailure('Runtime provider неожиданно остановился.');
    Exit;
  end;
  if FRuntimeState = rsConnecting then
  begin
    if CanConnectLocalhost(FReadinessPort) then
    begin
      AppendLog('Declared local endpoint accepted a connection; runtime is ready.');
      FRuntimeState := rsRunning;
      FReadyTimer.Interval := 200;
      if FAppSettings.AutoEnableSystemProxy and
        (SameText(FActiveSystemProxyKind, 'mixed') or
         SameText(FActiveSystemProxyKind, 'http') or
         SameText(FActiveSystemProxyKind, 'socks')) then
      begin
        if FSystemProxy.Enable(FReadinessPort, FActiveSystemProxyKind,
          ErrorMessage) then
          AppendLog('System proxy (' + FActiveSystemProxyKind +
            ') enabled after runtime readiness.')
        else
        begin
          AppendLog('System proxy enable failed: ' + ErrorMessage);
          MessageDlg('Системный прокси', ErrorMessage, mtWarning, [mbOK], 0);
        end;
      end
      else if not FAppSettings.AutoEnableSystemProxy then
        AppendLog('Automatic system proxy activation is disabled.');
      if FAppSettings.AutoEnableSystemProxy and
        not (SameText(FActiveSystemProxyKind, 'mixed') or
             SameText(FActiveSystemProxyKind, 'http') or
             SameText(FActiveSystemProxyKind, 'socks')) then
        AppendLog('System proxy was not enabled: this profile does not expose a supported local proxy endpoint.');
      UpdateRuntimeSurface;
    end
    else if GetTickCount64 >= FReadyDeadline then
    begin
      if Assigned(FExternalProcess) then
      begin
        FExternalProcess.Stop;
        FExternalProcess := nil;
      end
      else
        FEmbeddedXray.Stop(ErrorMessage);
      DeleteRuntimeConfig(FActiveConfigPath);
      FActiveConfigPath := '';
      FReadyTimer.Enabled := False;
      FRuntimeState := rsStopped;
      FActiveProfile := Default(TZaryaProfile);
      FActiveProvider := Default(TZaryaCoreProvider);
      AppendLog('Local runtime endpoint did not become ready; system proxy remains off.');
      UpdateRuntimeSurface;
      MessageDlg('Runtime provider',
        'Локальный endpoint provider не стал доступен за 5 секунд. Системный прокси не изменён.',
        mtError, [mbOK], 0);
    end;
  end;
end;

procedure TMainForm.TestClick(Sender: TObject);
begin
  if Assigned(FTestThread) then
  begin
    FTestThread.RequestCancel;
    FTestButton.Enabled := False;
    FStatusBar.SimpleText := 'Отмена TCP ping после текущего соединения…';
    Exit;
  end;
  if Length(FProfiles) = 0 then
  begin
    MessageDlg('TCP ping', 'Нет профилей для проверки.', mtInformation,
      [mbOK], 0);
    Exit;
  end;
  FTestThread := TProfileTcpTestThread.Create(FProfiles);
  FTestButton.Caption := 'Отменить';
  FTestTimer.Enabled := True;
  FStatusBar.SimpleText := 'TCP ping запущен…';
  AppendLog('TCP profile test started.');
end;

procedure TMainForm.TestTimerTimer(Sender: TObject);
var
  I: Integer;
  SelectedId: string;
  CompletedCount: Integer;
  SuccessCount: Integer;
  CurrentIndex: Integer;
  TestResults: TZaryaTcpTestResults;
begin
  if not Assigned(FTestThread) then Exit;
  if not FTestThread.IsDone then
  begin
    CurrentIndex := FTestThread.CurrentIndex;
    if (CurrentIndex >= 0) and (CurrentIndex <= High(FProfiles)) then
      FStatusBar.SimpleText := Format('TCP ping %d/%d: %s',
        [CurrentIndex + 1, Length(FProfiles), FProfiles[CurrentIndex].Name]);
    Exit;
  end;
  FTestTimer.Enabled := False;
  FTestThread.WaitFor;
  TestResults := FTestThread.Results;
  FreeAndNil(FTestThread);
  if SelectedProfileIndex >= 0 then
    SelectedId := FProfiles[SelectedProfileIndex].Id
  else
    SelectedId := '';
  CompletedCount := 0;
  SuccessCount := 0;
  for I := 0 to High(FProfiles) do
    if (I <= High(TestResults)) and TestResults[I].Tested then
    begin
      Inc(CompletedCount);
      FProfiles[I].LastTestedAt := FormatDateTime(
        'yyyy-mm-dd"T"hh:nn:ss', Now);
      if TestResults[I].Success then
      begin
        Inc(SuccessCount);
        FProfiles[I].LastTcpPingMs := TestResults[I].LatencyMs;
        FProfiles[I].Latency := IntToStr(TestResults[I].LatencyMs) + ' мс';
        FProfiles[I].LastTestStatus := 'success';
        FProfiles[I].LastTestError := '';
      end
      else
      begin
        FProfiles[I].LastTcpPingMs := -1;
        FProfiles[I].Latency := 'Ошибка';
        FProfiles[I].LastTestStatus := 'failed';
        FProfiles[I].LastTestError := TestResults[I].ErrorMessage;
      end;
    end;
  SaveProfiles;
  RefreshProfileGrid(SelectedId);
  FTestButton.Caption := 'TCP ping';
  FTestButton.Enabled := True;
  AppendLog(Format('TCP profile test completed: success=%d total=%d.',
    [SuccessCount, CompletedCount]));
  FStatusBar.SimpleText := Format('TCP ping: успешно %d из %d',
    [SuccessCount, CompletedCount]);
end;

procedure TMainForm.PreviewConfigClick(Sender: TObject);
var
  Index: Integer;
  Config: string;
  ErrorMessage: string;
  ProviderId: string;
  Provider: TZaryaCoreProvider;
  Adapter: IConfigAdapter;
  Context: TZaryaConfigContext;
begin
  Index := SelectedProfileIndex;
  if Index < 0 then
  begin
    MessageDlg('Runtime config', 'Сначала выберите профиль.', mtInformation,
      [mbOK], 0);
    Exit;
  end;
  ProviderId := FProfiles[Index].PreferredProviderId;
  if ProviderId = '' then
    ProviderId := DefaultProviderForProtocol(FProfiles[Index].ProtocolName);
  if not FCoreRegistry.TryGet(ProviderId, Provider) then
    Provider := CreateProviderPreset(ProviderId);
  if FProfiles[Index].RawConfig <> '' then
    Config := FProfiles[Index].RawConfig
  else
  begin
    Adapter := CreateConfigAdapter(Provider);
    if not Assigned(Adapter) then
    begin
      MessageDlg('Runtime config',
        'Для выбранного provider нет config adapter.', mtWarning, [mbOK], 0);
      Exit;
    end;
    Context := Default(TZaryaConfigContext);
    Context.MixedPort := FAppSettings.MixedPort;
    Context.HttpPort := FAppSettings.MixedPort;
    Context.SocksPort := FAppSettings.MixedPort;
    if SameText(Provider.AdapterId, 'hysteria2') then
      if FAppSettings.MixedPort < 65535 then
        Context.SocksPort := FAppSettings.MixedPort + 1
      else
        Context.SocksPort := FAppSettings.MixedPort - 1;
    if not Adapter.Generate(FProfiles[Index], Context, Config,
      ErrorMessage) then
    begin
      MessageDlg('Runtime config', ErrorMessage, mtWarning, [mbOK], 0);
      Exit;
    end;
  end;
  AppendLog('Runtime config generated for provider: ' + Provider.ProviderId);
  TXrayConfigDialog.Execute(Self, FProfiles[Index].Name, Config,
    ConfigFormatToString(Provider.ConfigFormat), Provider.ConfigExtension,
    FDarkTheme);
end;

procedure TMainForm.AddClick(Sender: TObject);
var
  Profile: TZaryaProfile;
  NewIndex: Integer;
begin
  Profile := CreateEmptyProfile;
  if not TProfileDialog.Execute(Self, Profile, FDarkTheme) then
    Exit;
  NewIndex := Length(FProfiles);
  SetLength(FProfiles, NewIndex + 1);
  FProfiles[NewIndex] := Profile;
  if SaveProfiles then
  begin
    RefreshProfileGrid(Profile.Id);
    AppendLog('Profile added: ' + Profile.Name);
  end;
end;

procedure TMainForm.EditClick(Sender: TObject);
var
  Index: Integer;
  Profile: TZaryaProfile;
begin
  Index := SelectedProfileIndex;
  if Index < 0 then
  begin
    MessageDlg('Профиль', 'Выберите профиль для изменения.', mtInformation,
      [mbOK], 0);
    Exit;
  end;
  Profile := FProfiles[Index];
  if not TProfileDialog.Execute(Self, Profile, FDarkTheme) then
    Exit;
  FProfiles[Index] := Profile;
  if SaveProfiles then
  begin
    RefreshProfileGrid(Profile.Id);
    AppendLog('Profile updated: ' + Profile.Name);
  end;
end;

procedure TMainForm.DeleteClick(Sender: TObject);
var
  Index: Integer;
  I: Integer;
  ProfileName: string;
begin
  Index := SelectedProfileIndex;
  if Index < 0 then
  begin
    MessageDlg('Профиль', 'Выберите профиль для удаления.', mtInformation,
      [mbOK], 0);
    Exit;
  end;
  ProfileName := FProfiles[Index].Name;
  if MessageDlg('Удалить профиль', 'Удалить профиль «' + ProfileName + '»?',
    mtConfirmation, [mbYes, mbNo], 0) <> mrYes then
    Exit;
  for I := Index to High(FProfiles) - 1 do
    FProfiles[I] := FProfiles[I + 1];
  SetLength(FProfiles, Length(FProfiles) - 1);
  if SaveProfiles then
  begin
    RefreshProfileGrid;
    AppendLog('Profile deleted: ' + ProfileName);
  end;
end;

procedure TMainForm.ImportClick(Sender: TObject);
var
  LinksText: string;
  Links: TStringList;
  Errors: TStringList;
  Profile: TZaryaProfile;
  ErrorMessage: string;
  I: Integer;
  NewIndex: Integer;
  ImportedCount: Integer;
  LastImportedId: string;
  OriginalCount: Integer;
begin
  if not TImportVlessDialog.Execute(Self, FDarkTheme, LinksText) then
    Exit;
  Links := TStringList.Create;
  Errors := TStringList.Create;
  try
    Links.Text := LinksText;
    OriginalCount := Length(FProfiles);
    ImportedCount := 0;
    LastImportedId := '';
    for I := 0 to Links.Count - 1 do
    begin
      if Trim(Links[I]) = '' then
        Continue;
      if not ParseShareLink(Trim(Links[I]), Profile, ErrorMessage) then
      begin
        Errors.Add(Format('Строка %d: %s', [I + 1, ErrorMessage]));
        Continue;
      end;
      NewIndex := Length(FProfiles);
      SetLength(FProfiles, NewIndex + 1);
      FProfiles[NewIndex] := Profile;
      LastImportedId := Profile.Id;
      Inc(ImportedCount);
    end;

    if ImportedCount > 0 then
    begin
      if SaveProfiles then
      begin
        RefreshProfileGrid(LastImportedId);
        AppendLog(Format('Imported share-link profiles: %d', [ImportedCount]));
      end
      else
      begin
        SetLength(FProfiles, OriginalCount);
        RefreshProfileGrid;
        Exit;
      end;
    end;
    if Errors.Count > 0 then
      MessageDlg('Импорт профилей', Format('Импортировано: %d', [ImportedCount]) +
        LineEnding + LineEnding + Errors.Text, mtWarning, [mbOK], 0)
    else if ImportedCount > 0 then
      MessageDlg('Импорт профилей', Format('Импортировано профилей: %d',
        [ImportedCount]), mtInformation, [mbOK], 0)
    else
      MessageDlg('Импорт профилей', 'Вставьте хотя бы одну поддерживаемую share link.',
        mtInformation, [mbOK], 0);
  finally
    Errors.Free;
    Links.Free;
  end;
end;

procedure TMainForm.SubscriptionsClick(Sender: TObject);
var
  Dialog: TSubscriptionManagerDialog;
begin
  Dialog := TSubscriptionManagerDialog.Create(Self, FProfiles, FProfileStore,
    ExtractFileDir(FProfileStore.FileName), @AppendLog);
  try
    Dialog.ShowModal;
    if Dialog.ProfilesChanged then
    begin
      Dialog.CopyProfiles(FProfiles);
      RefreshProfileGrid;
      AppendLog('Profiles refreshed after subscription changes.');
    end;
  finally
    Dialog.Free;
  end;
end;

procedure TMainForm.SettingsClick(Sender: TObject);
var
  Dialog: TSettingsDialog;
begin
  if FRuntimeState <> rsStopped then
  begin
    MessageDlg('Настройки',
      'Остановите runtime перед изменением mixed-порта и системного прокси.',
      mtInformation, [mbOK], 0);
    Exit;
  end;
  Dialog := TSettingsDialog.Create(Self, FDarkTheme, FMinimizeToTray,
    FAppSettings.MixedPort, FAppSettings.AutoEnableSystemProxy,
    FAppSettings.RestoreSystemProxy);
  try
    if Dialog.ShowModal = mrOk then
    begin
      FDarkTheme := Dialog.DarkThemeSelected;
      FMinimizeToTray := Dialog.MinimizeToTraySelected;
      FAppSettings.DarkTheme := FDarkTheme;
      FAppSettings.MinimizeToTray := FMinimizeToTray;
      FAppSettings.MixedPort := Dialog.MixedPortSelected;
      FAppSettings.AutoEnableSystemProxy :=
        Dialog.AutoEnableSystemProxySelected;
      FAppSettings.RestoreSystemProxy := Dialog.RestoreSystemProxySelected;
      SaveAppSettings;
      ApplyCurrentTheme;
      AppendLog('Interface settings applied.');
    end;
  finally
    Dialog.Free;
  end;
end;

procedure TMainForm.CoreManagerClick(Sender: TObject);
var
  ProviderId: string;
  Provider: TZaryaCoreProvider;
  I: Integer;
  CompatibleCount: Integer;
begin
  if FRuntimeState <> rsStopped then
  begin
    MessageDlg('Ядра', 'Остановите runtime перед изменением ядер.',
      mtInformation, [mbOK], 0);
    Exit;
  end;
  if not TCoreManagerDialog.Execute(Self, FCoreRegistry, FDarkTheme,
    ProviderId) then
    Exit;
  if not FCoreRegistry.TryGet(ProviderId, Provider) then
    Exit;
  CompatibleCount := 0;
  for I := 0 to High(FProfiles) do
    if ProviderUsableForProfile(Provider, FProfiles[I]) then
      Inc(CompatibleCount);
  if CompatibleCount = 0 then
  begin
    MessageDlg('Provider профилей',
      'Совместимых профилей не найдено. Raw-конфигурации требуют тот же диалект.',
      mtInformation, [mbOK], 0);
    Exit;
  end;
  if MessageDlg('Provider профилей', Format(
    'Назначить %s для совместимых профилей (%d)?',
    [Provider.DisplayName, CompatibleCount]), mtConfirmation,
    [mbYes, mbNo], 0) <> mrYes then
    Exit;
  for I := 0 to High(FProfiles) do
    if ProviderUsableForProfile(Provider, FProfiles[I]) then
      FProfiles[I].PreferredProviderId := ProviderId;
  if SaveProfiles then
  begin
    RefreshProfileGrid;
    AppendLog(Format('Provider %s assigned to %d compatible profiles.',
      [ProviderId, CompatibleCount]));
  end;
end;

procedure TMainForm.CreateBackupClick(Sender: TObject);
var
  Dialog: TSaveDialog;
  DataDirectory: string;
  BackupDirectory: string;
  ErrorMessage: string;
begin
  DataDirectory := ExtractFileDir(FProfileStore.FileName);
  BackupDirectory := IncludeTrailingPathDelimiter(
    ExtractFileDir(DataDirectory)) + 'backups';
  ForceDirectories(BackupDirectory);
  Dialog := TSaveDialog.Create(Self);
  try
    Dialog.Title := 'Создать резервную копию Zarya';
    Dialog.Filter := 'Zarya backup (*.zarya-backup.zip)|*.zarya-backup.zip';
    Dialog.DefaultExt := 'zarya-backup.zip';
    Dialog.Options := Dialog.Options + [ofOverwritePrompt, ofPathMustExist];
    Dialog.InitialDir := BackupDirectory;
    Dialog.FileName := 'zarya-' + FormatDateTime('yyyymmdd-hhnnss', Now) +
      '.zarya-backup.zip';
    if not Dialog.Execute then
      Exit;
    if not CreateZaryaBackup(DataDirectory, Dialog.FileName,
      ErrorMessage) then
    begin
      MessageDlg('Backup', 'Не удалось создать backup:' + LineEnding +
        ErrorMessage, mtError, [mbOK], 0);
      Exit;
    end;
    AppendLog('Verified Zarya backup created.');
    MessageDlg('Backup', 'Проверяемая резервная копия создана:' + LineEnding +
      Dialog.FileName, mtInformation, [mbOK], 0);
  finally
    Dialog.Free;
  end;
end;

procedure TMainForm.RestoreBackupClick(Sender: TObject);
var
  Dialog: TOpenDialog;
  DataDirectory: string;
  PreRestoreBackup: string;
  ErrorMessage: string;
begin
  if FRuntimeState <> rsStopped then
  begin
    MessageDlg('Восстановление backup',
      'Сначала остановите активный runtime.', mtWarning, [mbOK], 0);
    Exit;
  end;
  Dialog := TOpenDialog.Create(Self);
  try
    Dialog.Title := 'Восстановить резервную копию Zarya';
    Dialog.Filter := 'Zarya backup (*.zarya-backup.zip)|*.zarya-backup.zip|' +
      'ZIP archives (*.zip)|*.zip';
    Dialog.Options := Dialog.Options + [ofFileMustExist, ofPathMustExist];
    if not Dialog.Execute then
      Exit;
    if MessageDlg('Восстановление backup',
      'Текущие файлы данных будут заменены после staging-проверки.' +
      LineEnding +
      'Пути к внешним EXE не переносятся и их потребуется выбрать заново.' +
      LineEnding + LineEnding + 'Продолжить?', mtConfirmation,
      [mbYes, mbNo], 0) <> mrYes then
      Exit;
    DataDirectory := ExtractFileDir(FProfileStore.FileName);
    if not RestoreZaryaBackup(Dialog.FileName, DataDirectory,
      PreRestoreBackup, ErrorMessage) then
    begin
      MessageDlg('Восстановление backup',
        'Данные не восстановлены:' + LineEnding + ErrorMessage,
        mtError, [mbOK], 0);
      Exit;
    end;
    if not FCoreRegistry.Load(ErrorMessage) then
      MessageDlg('Ядра', 'Backup восстановлен, но providers.json не прочитан:' +
        LineEnding + ErrorMessage, mtWarning, [mbOK], 0);
    FCoreRegistry.SetEmbeddedState(ProviderEmbeddedXray,
      FEmbeddedXray.Version, FEmbeddedXray.Available,
      FEmbeddedXray.LoadStatus);
    LoadAppSettings;
    LoadProfiles;
    ApplyCurrentTheme;
    UpdateRuntimeSurface;
    AppendLog('Verified Zarya backup restored.');
    MessageDlg('Восстановление backup',
      'Данные восстановлены и перечитаны.' + LineEnding +
      'Pre-restore backup: ' + PreRestoreBackup,
      mtInformation, [mbOK], 0);
  finally
    Dialog.Free;
  end;
end;

procedure TMainForm.DiagnosticsClick(Sender: TObject);
var
  Dialog: TSaveDialog;
  ErrorMessage: string;
begin
  Dialog := TSaveDialog.Create(Self);
  try
    Dialog.Title := 'Сохранить безопасный diagnostics bundle';
    Dialog.Filter :=
      'Zarya diagnostics (*.zarya-diagnostics.zip)|*.zarya-diagnostics.zip';
    Dialog.DefaultExt := 'zarya-diagnostics.zip';
    Dialog.Options := Dialog.Options + [ofOverwritePrompt, ofPathMustExist];
    Dialog.FileName := 'zarya-diagnostics-' +
      FormatDateTime('yyyymmdd-hhnnss', Now) + '.zarya-diagnostics.zip';
    if not Dialog.Execute then
      Exit;
    FCoreRegistry.RefreshLocalState;
    if not CreateDiagnosticsBundle(Dialog.FileName, FProfiles,
      FCoreRegistry, FAppSettings, ErrorMessage) then
    begin
      MessageDlg('Диагностика', 'Не удалось создать bundle:' + LineEnding +
        ErrorMessage, mtError, [mbOK], 0);
      Exit;
    end;
    AppendLog('Redacted diagnostics bundle created.');
    MessageDlg('Диагностика',
      'Bundle создан без runtime-логов, raw config, credentials и путей EXE.' +
      LineEnding + Dialog.FileName, mtInformation, [mbOK], 0);
  finally
    Dialog.Free;
  end;
end;

procedure TMainForm.AboutClick(Sender: TObject);
begin
  MessageDlg('О Zarya',
    'Zarya · FPC/LCL' + LineEnding + LineEnding +
    'Локальные профили, подписки, share links, runtime-конфигурации, ' +
    'встроенные и внешние providers.' + LineEnding +
    'Production-сборка статически линкует Xray через общий zarya-xray ABI. ' +
    'Системный прокси включается только после подтверждённой готовности ' +
    'объявленного локального endpoint.',
    mtInformation, [mbOK], 0);
end;

procedure TMainForm.ExitClick(Sender: TObject);
begin
  FQuitting := True;
  StopRuntime(True);
  FTrayIcon.Visible := False;
  Close;
end;

procedure TMainForm.TrayShowClick(Sender: TObject);
begin
  if Visible then
    Hide
  else
  begin
    Show;
    WindowState := wsNormal;
    BringToFront;
  end;
end;

procedure TMainForm.ClearLogClick(Sender: TObject);
begin
  FLogMemo.Clear;
end;

procedure TMainForm.GridClick(Sender: TObject);
begin
  UpdateRuntimeSurface;
end;

procedure TMainForm.FormResize(Sender: TObject);
begin
  if not Assigned(FStartButton) then
    Exit;
  FStartButton.Left := ClientWidth - 318;
  FStopButton.Left := ClientWidth - 176;
  FSettingsButton.Left := ClientWidth - 176;
  FStateDetail.Width := ClientWidth - 390;
end;

procedure TMainForm.FormCloseQuery(Sender: TObject; var CanClose: Boolean);
begin
  if FMinimizeToTray and (not FQuitting) then
  begin
    Hide;
    CanClose := False;
    AppendLog('Main window hidden to tray.');
  end
  else
  begin
    StopRuntime(True);
    CanClose := True;
  end;
end;

end.
