unit MainForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ComCtrls, Grids, Menus, ZaryaThemes, ZaryaSettingsForm, ZaryaProfile,
  FpcProfileStore, ProfileForm, ImportVlessForm,
  XrayConfigForm, ZaryaShareLink, ZaryaAppSettings,
  ZaryaEmbeddedXray, ZaryaSystemProxy, WindowsSystemProxy, ZaryaTcpProbe,
  ZaryaCoreProvider, ZaryaCoreProviderRegistry, FpcCoreProviderStore,
  CoreManagerForm, ZaryaRuntimeProcess, ZaryaRuntimeConfigFile,
  ProviderChoiceForm, ZaryaFileIntegrity, ZaryaRuntimeContracts,
  ZaryaConfigAdapters, SubscriptionManagerForm, ZaryaProfileTesting,
  ZaryaBackup, ZaryaDiagnostics, ZaryaRouting, ZaryaDns, ZaryaPolicyStore,
  FpcPolicyStore, PolicyManagerForm, ZaryaGeoData, GeoDataManagerForm,
  WindowsAutostart, ZaryaRealDelay, ZaryaTr, ZaryaProfileService,
  ZaryaBackgroundOperations, ZaryaRuntimeCoordinator;

type
  TMainForm = class(TForm)
  private
    FProfiles: TZaryaProfiles;
    FProfileService: TZaryaProfileService;
    FAppSettings: TZaryaAppSettings;
    FSettingsStore: TZaryaAppSettingsStore;
    FRoutingStore: IRoutingProfileStore;
    FDnsStore: IDnsProfileStore;
    FRoutingProfiles: TZaryaRoutingProfiles;
    FDnsProfiles: TZaryaDnsProfiles;
    FGeoDataManager: IGeoDataManager;
    FAutostartManager: IAutostartManager;
    FBackgroundOperations: TZaryaBackgroundOperations;
    FRuntimeCoordinator: TZaryaRuntimeCoordinator;
    FEmbeddedXray: TZaryaEmbeddedXray;
    FCoreRegistry: TZaryaCoreProviderRegistry;
    FExternalProcess: IZaryaRuntimeProcess;
    FActiveConfigPath: string;
    FSystemProxy: TZaryaSystemProxyController;
    FXrayAssetDirectory: string;
    FDarkTheme: Boolean;
    FMinimizeToTray: Boolean;
    FQuitting: Boolean;
    FAutoStarting: Boolean;
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
    FAutoStartTimer: TTimer;
    FTestButton: TButton;
    FRealDelayButton: TButton;
    FRealDelayTimer: TTimer;
    FTrayIcon: TTrayIcon;
    FTrayMenu: TPopupMenu;
    FStartMenuItem: TMenuItem;
    FStopMenuItem: TMenuItem;
    procedure BuildMenus;
    procedure BuildInterface;
    procedure BuildTray;
    procedure LayoutToolbar;
    procedure LoadAppSettings;
    procedure SaveAppSettings;
    procedure ConfigureStartup;
    procedure AutoStartTimerTick(Sender: TObject);
    function AutoProxyEnabledForCurrentStart: Boolean;
    procedure LoadPolicies;
    function SavePolicies: Boolean;
    function ActiveRoutingProfile: TZaryaRoutingProfile;
    function ActiveDnsProfile: TZaryaDnsProfile;
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
    procedure RealDelayClick(Sender: TObject);
    procedure RealDelayTimerTimer(Sender: TObject);
    function BuildRealDelayItems(out AItems: TZaryaRealDelayWorkItems;
      out AError: string): Boolean;
    procedure PreviewConfigClick(Sender: TObject);
    procedure AddClick(Sender: TObject);
    procedure EditClick(Sender: TObject);
    procedure DeleteClick(Sender: TObject);
    procedure ImportClick(Sender: TObject);
    procedure SubscriptionsClick(Sender: TObject);
    procedure SettingsClick(Sender: TObject);
    procedure PoliciesClick(Sender: TObject);
    procedure GeoDataClick(Sender: TObject);
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
  FMinimizeToTray := True;
  OnResize := @FormResize;
  OnCloseQuery := @FormCloseQuery;
  BuildMenus;
  BuildInterface;
  BuildTray;
  FProfileService := TZaryaProfileService.Create(
    TFpcProfileStore.Create(ProfileStorePathFromCommandLine));
  FCoreRegistry := TZaryaCoreProviderRegistry.Create(TFpcCoreProviderStore.Create(
    IncludeTrailingPathDelimiter(ExtractFileDir(FProfileService.FileName)) +
    'providers.json'));
  if not FCoreRegistry.Load(ErrorMessage) then
    MessageDlg(TZaryaTr.Tr('Ядра'), TZaryaTr.Tr(
      'Не удалось прочитать providers.json:',
      'Could not read providers.json:') + LineEnding +
      ErrorMessage, mtWarning, [mbOK], 0);
  FSettingsStore := TZaryaAppSettingsStore.Create(
    IncludeTrailingPathDelimiter(ExtractFileDir(FProfileService.FileName)) +
    'settings.ini');
  LoadAppSettings;
  FAutostartManager := TWindowsAutostartManager.Create;
  FRoutingStore := TFpcRoutingProfileStore.Create(
    IncludeTrailingPathDelimiter(ExtractFileDir(FProfileService.FileName)) +
    'routing.json');
  FDnsStore := TFpcDnsProfileStore.Create(
    IncludeTrailingPathDelimiter(ExtractFileDir(FProfileService.FileName)) +
    'dns.json');
  LoadPolicies;
  FGeoDataManager := TZaryaGeoDataManager.Create(
    IncludeTrailingPathDelimiter(ExtractFileDir(FProfileService.FileName)) +
    'geodata' + PathDelim + 'xray');
  FXrayAssetDirectory := FGeoDataManager.TargetDirectory;
  FEmbeddedXray := TZaryaEmbeddedXray.Create(
    IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) + 'zarya-xray.dll');
  FCoreRegistry.SetEmbeddedState(ProviderEmbeddedXray,
    FEmbeddedXray.Version, FEmbeddedXray.Available, FEmbeddedXray.LoadStatus);
  FSystemProxy := TZaryaSystemProxyController.Create(
    TWindowsSystemProxyBackend.Create,
    IncludeTrailingPathDelimiter(ExtractFileDir(FProfileService.FileName)) +
    'proxy-previous-state.ini');
  FRuntimeCoordinator := TZaryaRuntimeCoordinator.Create(
    ExtractFileDir(FProfileService.FileName), FXrayAssetDirectory,
    FGeoDataManager, FSystemProxy);
  FBackgroundOperations := TZaryaBackgroundOperations.Create;
  LoadProfiles;
  ApplyCurrentTheme;
  UpdateRuntimeSurface;
  AppendLog('Zarya LCL started. Profiles: ' + FProfileService.FileName);
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
      MessageDlg(TZaryaTr.Tr('Восстановление системного прокси',
        'System proxy recovery'), ErrorMessage, mtWarning,
        [mbOK], 0);
    end;
  end;
  ConfigureStartup;
end;

procedure TMainForm.LoadAppSettings;
var
  ErrorMessage: string;
begin
  if not FSettingsStore.Load(FAppSettings, ErrorMessage) then
  begin
    FAppSettings := DefaultAppSettings;
    MessageDlg(TZaryaTr.Tr('Настройки'), TZaryaTr.Tr(
      'Не удалось прочитать settings.ini:', 'Could not read settings.ini:') +
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
    MessageDlg(TZaryaTr.Tr('Настройки'), TZaryaTr.Tr(
      'Не удалось сохранить settings.ini:', 'Could not save settings.ini:') +
      LineEnding + ErrorMessage, mtError, [mbOK], 0);
end;

procedure TMainForm.ConfigureStartup;
var
  I, ProfileIndex: Integer;
  DisableProfileAutostart: Boolean;
begin
  DisableProfileAutostart := False;
  for I := 1 to ParamCount do
    if SameText(ParamStr(I), '--no-autostart-profile') then
      DisableProfileAutostart := True;
  if DisableProfileAutostart or not FAppSettings.AutoStartLastProfile or
    (Trim(FAppSettings.LastStartedProfileId) = '') then Exit;
  ProfileIndex := FProfileService.FindRunnableById(FProfiles,
    FAppSettings.LastStartedProfileId);
  if ProfileIndex >= 0 then
  begin
    RefreshProfileGrid(FProfiles[ProfileIndex].Id);
    FAutoStartTimer := TTimer.Create(Self);
    FAutoStartTimer.Enabled := False;
    FAutoStartTimer.Interval := FAppSettings.AutoStartDelaySeconds * 1000;
    if FAutoStartTimer.Interval < 1 then FAutoStartTimer.Interval := 1;
    FAutoStartTimer.OnTimer := @AutoStartTimerTick;
    FAutoStartTimer.Enabled := True;
    AppendLog(Format('Auto-start of profile scheduled in %d second(s).',
      [FAppSettings.AutoStartDelaySeconds]));
    Exit;
  end;
  AppendLog('Configured auto-start profile is missing or disabled; skipped.');
end;

procedure TMainForm.AutoStartTimerTick(Sender: TObject);
begin
  FAutoStartTimer.Enabled := False;
  FAutoStarting := True;
  StartClick(Sender);
  if FRuntimeCoordinator.State = rsStopped then FAutoStarting := False;
end;

function TMainForm.AutoProxyEnabledForCurrentStart: Boolean;
begin
  if FAutoStarting then
    Result := FAppSettings.AutoEnableSystemProxyAfterAutoStart
  else
    Result := FAppSettings.AutoEnableSystemProxy;
end;

procedure TMainForm.LoadPolicies;
var
  ErrorMessage: string;
  I: Integer;
  FoundRouting, FoundDns: Boolean;
begin
  if not FRoutingStore.Load(FRoutingProfiles, ErrorMessage) then
  begin
    FRoutingProfiles := CreateBuiltInRoutingProfiles;
    MessageDlg('Routing', TZaryaTr.Tr('Не удалось прочитать routing.json:',
      'Could not read routing.json:') + LineEnding +
      ErrorMessage, mtWarning, [mbOK], 0);
  end;
  if not FDnsStore.Load(FDnsProfiles, ErrorMessage) then
  begin
    FDnsProfiles := CreateBuiltInDnsProfiles;
    MessageDlg('DNS', TZaryaTr.Tr('Не удалось прочитать dns.json:',
      'Could not read dns.json:') + LineEnding +
      ErrorMessage, mtWarning, [mbOK], 0);
  end;
  FoundRouting := False;
  for I := 0 to High(FRoutingProfiles) do
    if SameText(FRoutingProfiles[I].Id,
      FAppSettings.SelectedRoutingProfileId) then
      FoundRouting := True;
  if not FoundRouting then
    FAppSettings.SelectedRoutingProfileId := RoutingBypassLanId;
  FoundDns := False;
  for I := 0 to High(FDnsProfiles) do
    if SameText(FDnsProfiles[I].Id, FAppSettings.SelectedDnsProfileId) then
      FoundDns := True;
  if not FoundDns then FAppSettings.SelectedDnsProfileId := DnsSystemId;
end;

function TMainForm.SavePolicies: Boolean;
var
  ErrorMessage: string;
begin
  Result := False;
  if not FRoutingStore.Save(FRoutingProfiles, ErrorMessage) then
  begin
    MessageDlg('Routing', TZaryaTr.Tr('Не удалось сохранить routing.json:',
      'Could not save routing.json:') + LineEnding +
      ErrorMessage, mtError, [mbOK], 0);
    Exit;
  end;
  if not FDnsStore.Save(FDnsProfiles, ErrorMessage) then
  begin
    MessageDlg('DNS', TZaryaTr.Tr('Не удалось сохранить dns.json:',
      'Could not save dns.json:') + LineEnding +
      ErrorMessage, mtError, [mbOK], 0);
    Exit;
  end;
  SaveAppSettings;
  Result := True;
end;

function TMainForm.ActiveRoutingProfile: TZaryaRoutingProfile;
var
  I: Integer;
begin
  for I := 0 to High(FRoutingProfiles) do
    if SameText(FRoutingProfiles[I].Id,
      FAppSettings.SelectedRoutingProfileId) then
      Exit(FRoutingProfiles[I]);
  Result := BuiltInProxyAllRouting;
end;

function TMainForm.ActiveDnsProfile: TZaryaDnsProfile;
var
  I: Integer;
begin
  for I := 0 to High(FDnsProfiles) do
    if SameText(FDnsProfiles[I].Id, FAppSettings.SelectedDnsProfileId) then
      Exit(FDnsProfiles[I]);
  Result := BuiltInSystemDns;
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
var
  ActiveProfile: TZaryaProfile;
begin
  ActiveProfile := FRuntimeCoordinator.ActiveProfile;
  Result := AText;
  if ActiveProfile.Uuid <> '' then
    Result := StringReplace(Result, ActiveProfile.Uuid, '[uuid-redacted]',
      [rfReplaceAll]);
  if ActiveProfile.PublicKey <> '' then
    Result := StringReplace(Result, ActiveProfile.PublicKey,
      '[public-key-redacted]', [rfReplaceAll]);
  if ActiveProfile.Host <> '' then
    Result := StringReplace(Result, ActiveProfile.Host, '[server-redacted]',
      [rfReplaceAll, rfIgnoreCase]);
end;

procedure TMainForm.DrainRuntimeLogs;
var
  Lines: TStringList;
  I: Integer;
  RuntimeText: string;
  Prefix: string;
  ActiveProvider: TZaryaCoreProvider;
begin
  ActiveProvider := FRuntimeCoordinator.ActiveProvider;
  if ActiveProvider.Distribution = pdExternal then
  begin
    if not Assigned(FExternalProcess) then
      Exit;
    RuntimeText := FExternalProcess.DrainOutput;
    Prefix := '[' + ActiveProvider.DisplayName + '] ';
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
  AppendLog(AMessage);
  FRuntimeCoordinator.Reset;
  FAutoStarting := False;
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
          MessageDlg(TZaryaTr.Tr('Системный прокси', 'System proxy'),
            ErrorMessage, mtError, [mbOK], 0);
      end;
    end
    else
      AppendLog('System proxy was left unchanged by user setting.');
  end;
  if Assigned(FEmbeddedXray) and
    (FRuntimeCoordinator.ActiveProvider.Distribution = pdEmbedded) and
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
  FRuntimeCoordinator.Reset;
  FAutoStarting := False;
  if Assigned(FStatusBar) then
    UpdateRuntimeSurface;
end;

destructor TMainForm.Destroy;
begin
  FBackgroundOperations.Free;
  StopRuntime(False);
  FRuntimeCoordinator.Free;
  FSystemProxy.Free;
  FEmbeddedXray.Free;
  FCoreRegistry.Free;
  FSettingsStore.Free;
  FRoutingStore := nil;
  FDnsStore := nil;
  FGeoDataManager := nil;
  FAutostartManager := nil;
  FProfileService.Free;
  inherited Destroy;
end;

procedure TMainForm.BuildMenus;
var
  RootItem: TMenuItem;
begin
  Menu := TMainMenu.Create(Self);

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := TZaryaTr.Tr('&Файл', '&File');
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('Создать &backup…',
    'Create &backup…'), @CreateBackupClick));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('&Восстановить backup…',
    '&Restore backup…'), @RestoreBackupClick));
  RootItem.Add(NewMenuItem(Menu, '-', nil));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('Скрыть в &tray',
    'Hide to &tray'), @TrayShowClick));
  RootItem.Add(NewMenuItem(Menu, '-', nil));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('Вы&ход', 'E&xit'), @ExitClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := TZaryaTr.Tr('&Профили', '&Profiles');
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('&Добавить…', '&Add…'), @AddClick));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('&Изменить…', '&Edit…'), @EditClick));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('&Удалить', '&Delete'), @DeleteClick));
  RootItem.Add(NewMenuItem(Menu, '-', nil));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('&Импортировать…',
    '&Import…'), @ImportClick));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('Под&писки…',
    'Sub&scriptions…'), @SubscriptionsClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Core';
  Menu.Items.Add(RootItem);
  FStartMenuItem := NewMenuItem(Menu, TZaryaTr.Tr('&Запустить', '&Start'), @StartClick);
  FStopMenuItem := NewMenuItem(Menu, TZaryaTr.Tr('&Остановить', 'S&top'), @StopClick);
  RootItem.Add(FStartMenuItem);
  RootItem.Add(FStopMenuItem);
  RootItem.Add(NewMenuItem(Menu, '-', nil));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('&Менеджер ядер…',
    '&Core Manager…'), @CoreManagerClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := TZaryaTr.Tr('&Инструменты', '&Tools');
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, '&Runtime config…', @PreviewConfigClick));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('Routing и &DNS…',
    'Routing and &DNS…'), @PoliciesClick));
  RootItem.Add(NewMenuItem(Menu, '&Geo data…', @GeoDataClick));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('&Диагностика…',
    '&Diagnostics…'), @DiagnosticsClick));
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('&Настройки…',
    '&Settings…'), @SettingsClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := TZaryaTr.Tr('&Справка', '&Help');
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, TZaryaTr.Tr('&О Zarya', '&About Zarya'), @AboutClick));
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
  FStatusBar.SimpleText := TZaryaTr.Tr('Готово · embedded Xray',
    'Ready · embedded Xray');

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
  FStateBadge.Caption := TZaryaTr.Tr('Остановлено');

  FStateDetail := TLabel.Create(Self);
  FStateDetail.Parent := FStatusPanel;
  FStateDetail.WordWrap := True;
  FStateDetail.SetBounds(20, 88, 650, 70);

  FStartButton := TButton.Create(Self);
  FStartButton.Parent := FStatusPanel;
  FStartButton.Caption := TZaryaTr.Tr('Запустить');
  FStartButton.OnClick := @StartClick;
  FStartButton.Default := True;
  FStartButton.SetBounds(708, 30, 132, 36);

  FStopButton := TButton.Create(Self);
  FStopButton.Parent := FStatusPanel;
  FStopButton.Caption := TZaryaTr.Tr('Остановить');
  FStopButton.OnClick := @StopClick;
  FStopButton.SetBounds(850, 30, 132, 36);

  FSettingsButton := TButton.Create(Self);
  FSettingsButton.Parent := FStatusPanel;
  FSettingsButton.Caption := TZaryaTr.Tr('Настройки', 'Settings') + '…';
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
  AddButton.Caption := TZaryaTr.Tr('Добавить');
  AddButton.OnClick := @AddClick;
  AddButton.SetBounds(12, 8, 92, 32);

  EditButton := TButton.Create(Self);
  EditButton.Parent := FToolbar;
  EditButton.Caption := TZaryaTr.Tr('Изменить');
  EditButton.OnClick := @EditClick;
  EditButton.SetBounds(110, 8, 92, 32);

  DeleteButton := TButton.Create(Self);
  DeleteButton.Parent := FToolbar;
  DeleteButton.Caption := TZaryaTr.Tr('Удалить');
  DeleteButton.OnClick := @DeleteClick;
  DeleteButton.SetBounds(208, 8, 92, 32);

  ImportButton := TButton.Create(Self);
  ImportButton.Parent := FToolbar;
  ImportButton.Caption := TZaryaTr.Tr('Импорт', 'Import');
  ImportButton.OnClick := @ImportClick;
  ImportButton.SetBounds(312, 8, 92, 32);

  SubscriptionsButton := TButton.Create(Self);
  SubscriptionsButton.Parent := FToolbar;
  SubscriptionsButton.Caption := TZaryaTr.Tr('Подписки');
  SubscriptionsButton.OnClick := @SubscriptionsClick;
  SubscriptionsButton.SetBounds(410, 8, 100, 32);

  FTestButton := TButton.Create(Self);
  FTestButton.Parent := FToolbar;
  FTestButton.Caption := 'TCP ping';
  FTestButton.OnClick := @TestClick;
  FTestButton.SetBounds(522, 8, 100, 32);

  FRealDelayButton := TButton.Create(Self);
  FRealDelayButton.Parent := FToolbar;
  FRealDelayButton.Caption := 'Real delay';
  FRealDelayButton.OnClick := @RealDelayClick;
  FRealDelayButton.SetBounds(628, 8, 100, 32);

  CoresButton := TButton.Create(Self);
  CoresButton.Parent := FToolbar;
  CoresButton.Caption := TZaryaTr.Tr('Ядра');
  CoresButton.OnClick := @CoreManagerClick;
  CoresButton.SetBounds(734, 8, 78, 32);

  ConfigButton := TButton.Create(Self);
  ConfigButton.Parent := FToolbar;
  ConfigButton.Caption := 'Runtime config…';
  ConfigButton.OnClick := @PreviewConfigClick;
  ConfigButton.SetBounds(818, 8, 132, 32);

  FTestTimer := TTimer.Create(Self);
  FTestTimer.Enabled := False;
  FTestTimer.Interval := 100;
  FTestTimer.OnTimer := @TestTimerTimer;

  FRealDelayTimer := TTimer.Create(Self);
  FRealDelayTimer.Enabled := False;
  FRealDelayTimer.Interval := 100;
  FRealDelayTimer.OnTimer := @RealDelayTimerTimer;

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
  LogLabel.Caption := TZaryaTr.Tr('Журнал');
  LogLabel.Font.Style := [fsBold];
  LogLabel.SetBounds(12, 12, 70, 20);

  FLogFilter := TComboBox.Create(Self);
  FLogFilter.Parent := FLogToolbar;
  FLogFilter.Style := csDropDownList;
  FLogFilter.Items.Add(TZaryaTr.Tr('Все сообщения', 'All messages'));
  FLogFilter.Items.Add(TZaryaTr.Tr('Ошибки', 'Errors'));
  FLogFilter.Items.Add('Runtime');
  FLogFilter.ItemIndex := 0;
  FLogFilter.SetBounds(92, 6, 160, 30);

  ClearButton := TButton.Create(Self);
  ClearButton.Parent := FLogToolbar;
  ClearButton.Caption := TZaryaTr.Tr('Очистить');
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
  FGrid.Cells[0, 0] := TZaryaTr.Tr('Профиль', 'Profile');
  FGrid.Cells[1, 0] := TZaryaTr.Tr('Протокол', 'Protocol');
  FGrid.Cells[2, 0] := TZaryaTr.Tr('Сервер', 'Server');
  FGrid.Cells[3, 0] := TZaryaTr.Tr('Задержка', 'Delay');
  FGrid.Cells[4, 0] := TZaryaTr.Tr('Источник', 'Source');
  FGrid.Cells[5, 0] := 'Provider';
  FGrid.Cells[6, 0] := TZaryaTr.Tr('Статус', 'Status');
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
  LayoutToolbar;
end;

procedure TMainForm.LayoutToolbar;
var
  I, X, Y, AvailableWidth: Integer;
  Control: TControl;
begin
  if not Assigned(FToolbar) then Exit;
  AvailableWidth := ClientWidth - 24;
  if AvailableWidth < 300 then AvailableWidth := 300;
  X := 12;
  Y := 8;
  for I := 0 to FToolbar.ControlCount - 1 do
  begin
    Control := FToolbar.Controls[I];
    if not (Control is TButton) then Continue;
    if (X > 12) and (X + Control.Width > AvailableWidth) then
    begin
      X := 12;
      Inc(Y, 40);
    end;
    Control.Left := X;
    Control.Top := Y;
    Inc(X, Control.Width + 6);
  end;
  FToolbar.Height := Y + 40;
end;

procedure TMainForm.BuildTray;
var
  Item: TMenuItem;
begin
  FTrayMenu := TPopupMenu.Create(Self);
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, TZaryaTr.Tr('Показать Zarya',
    'Show Zarya'), @TrayShowClick));
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, TZaryaTr.Tr('Запустить'), @StartClick));
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, TZaryaTr.Tr('Остановить'), @StopClick));
  Item := NewMenuItem(FTrayMenu, '-', nil);
  FTrayMenu.Items.Add(Item);
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, TZaryaTr.Tr('Выход'), @ExitClick));

  FTrayIcon := TTrayIcon.Create(Self);
  FTrayIcon.Hint := TZaryaTr.Tr('Zarya — остановлено', 'Zarya — stopped');
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
  if not FProfileService.Load(FProfiles, ErrorMessage) then
  begin
    MessageDlg(TZaryaTr.Tr('Профили'), TZaryaTr.Tr(
      'Не удалось прочитать profiles.json:', 'Could not read profiles.json:') +
      LineEnding +
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
    if SameText(FProfiles[I].Source, 'Вручную') then
      FGrid.Cells[4, I + 1] := TZaryaTr.Tr('Вручную', 'Manual')
    else
      FGrid.Cells[4, I + 1] := FProfiles[I].Source;
    if FProfiles[I].Enabled then
      FGrid.Cells[6, I + 1] := TZaryaTr.Tr('Готов', 'Ready')
    else
      FGrid.Cells[6, I + 1] := TZaryaTr.Tr('Выключен', 'Disabled');
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
  Result := FProfileService.Save(FProfiles, ErrorMessage);
  if not Result then
    MessageDlg(TZaryaTr.Tr('Профили'), TZaryaTr.Tr(
      'Не удалось сохранить profiles.json:', 'Could not save profiles.json:') +
      LineEnding +
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
  if (FRuntimeCoordinator.State <> rsStopped) and
    (FRuntimeCoordinator.ActiveProfile.Name <> '') then
    Exit(FRuntimeCoordinator.ActiveProfile.Name);
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

  case FRuntimeCoordinator.State of
    rsStopped:
      begin
        Index := SelectedProfileIndex;
        if Index >= 0 then
          ProviderText := FProfiles[Index].PreferredProviderId
        else
          ProviderText := '—';
        FStateTitle.Caption := TZaryaTr.Tr('Runtime: остановлен',
          'Runtime: stopped');
        FStateBadge.Caption := TZaryaTr.Tr('Остановлено');
        FStateBadge.Color := Theme.Panel;
        FStateBadge.Font.Color := Theme.Muted;
        FStateDetail.Caption :=
          TZaryaTr.Tr('Выбранный профиль: ', 'Selected profile: ') +
          SelectedProfileName + LineEnding +
          'Provider: ' + ProviderText +
          TZaryaTr.Tr('     Системный прокси: выключен',
            '     System proxy: disabled');
        FStartButton.Enabled := True;
        FStopButton.Enabled := False;
        FStartMenuItem.Enabled := True;
        FStopMenuItem.Enabled := False;
        FStatusBar.SimpleText := TZaryaTr.Tr(
          'Готово · системный прокси выключен',
          'Ready · system proxy disabled');
        FTrayIcon.Hint := TZaryaTr.Tr('Zarya — остановлено', 'Zarya — stopped');
      end;
    rsConnecting:
      begin
        FStateTitle.Caption := TZaryaTr.Tr('Runtime: подключение — ',
          'Runtime: connecting — ') +
          FRuntimeCoordinator.ActiveProvider.ProviderId;
        FStateBadge.Caption := TZaryaTr.Tr('Проверка готовности');
        FStateBadge.Color := Theme.WarningSurface;
        FStateBadge.Font.Color := Theme.Warning;
        FStateDetail.Caption :=
          TZaryaTr.Tr('Профиль: ', 'Profile: ') + SelectedProfileName +
          LineEnding + Format(TZaryaTr.Tr(
            'Local endpoint: %s:%d     Системный прокси: пока выключен',
            'Local endpoint: %s:%d     System proxy: still disabled'),
            [FRuntimeCoordinator.ReadinessHost,
             FRuntimeCoordinator.ReadinessPort]);
        FStartButton.Enabled := False;
        FStopButton.Enabled := True;
        FStartMenuItem.Enabled := False;
        FStopMenuItem.Enabled := True;
        FStatusBar.SimpleText := Format(TZaryaTr.Tr(
          'Ожидание готовности %s:%d…', 'Waiting for %s:%d readiness…'),
          [FRuntimeCoordinator.ReadinessHost,
           FRuntimeCoordinator.ReadinessPort]);
        FTrayIcon.Hint := TZaryaTr.Tr('Zarya — подключение',
          'Zarya — connecting');
      end;
    rsRunning:
      begin
        if Assigned(FSystemProxy) and FSystemProxy.EnabledByZarya then
          ProxyStatus := TZaryaTr.Tr('включён', 'enabled')
        else
          ProxyStatus := TZaryaTr.Tr('выключен', 'disabled');
        FStateTitle.Caption := TZaryaTr.Tr('Runtime: работает — ',
          'Runtime: running — ') +
          FRuntimeCoordinator.ActiveProvider.ProviderId;
        FStateBadge.Caption := TZaryaTr.Tr('Подключено');
        FStateBadge.Color := Theme.SuccessSurface;
        FStateBadge.Font.Color := Theme.Success;
        FStateDetail.Caption :=
          TZaryaTr.Tr('Профиль: ', 'Profile: ') + SelectedProfileName +
          LineEnding + Format(TZaryaTr.Tr(
            'Local endpoint: %s:%d     Системный прокси: %s',
            'Local endpoint: %s:%d     System proxy: %s'),
            [FRuntimeCoordinator.ReadinessHost,
             FRuntimeCoordinator.ReadinessPort, ProxyStatus]);
        FStartButton.Enabled := False;
        FStopButton.Enabled := True;
        FStartMenuItem.Enabled := False;
        FStopMenuItem.Enabled := True;
        FStatusBar.SimpleText := Format(TZaryaTr.Tr(
          'Подключено · %s:%d · %s', 'Connected · %s:%d · %s'),
          [FRuntimeCoordinator.ReadinessHost,
           FRuntimeCoordinator.ReadinessPort,
           FRuntimeCoordinator.ActiveProvider.ProviderId]);
        FTrayIcon.Hint := TZaryaTr.Tr('Zarya — подключено',
          'Zarya — connected');
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
begin
  Result := FRuntimeCoordinator.ProviderUsableForProfile(AProvider, AProfile);
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
begin
  Result := FRuntimeCoordinator.PrepareConfig(AProfile, AProvider,
    FAppSettings, ActiveRoutingProfile, ActiveDnsProfile, AConfig, AError);
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
  DataDirectory := ExtractFileDir(FProfileService.FileName);
  if not WriteRuntimeConfig(DataDirectory, AProvider, AConfig,
    FActiveConfigPath, AError) then
    Exit;
  Context := Default(TZaryaProcessContext);
  Context.ConfigPath := FActiveConfigPath;
  Context.DataDirectory := DataDirectory;
  Context.AssetDirectory := AProvider.AssetDirectory;
  if Context.AssetDirectory = '' then
    Context.AssetDirectory := FXrayAssetDirectory;
  Context.MixedPort := FRuntimeCoordinator.ReadinessPort;
  Context.HttpPort := FRuntimeCoordinator.ReadinessPort;
  Context.SocksPort := FRuntimeCoordinator.ReadinessPort;
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
  if FRuntimeCoordinator.ActiveProvider.Distribution = pdExternal then
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
  DataDirectory := ExtractFileDir(FProfileService.FileName);
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
  if FRuntimeCoordinator.State <> rsStopped then
    Exit;
  Index := SelectedProfileIndex;
  if Index < 0 then
  begin
    MessageDlg(TZaryaTr.Tr('Запуск', 'Start'), TZaryaTr.Tr(
      'Сначала выберите профиль.', 'Select a profile first.'),
      mtInformation, [mbOK], 0);
    Exit;
  end;
  if not FProfiles[Index].Enabled then
  begin
    MessageDlg(TZaryaTr.Tr('Запуск', 'Start'), TZaryaTr.Tr(
      'Выбранный профиль выключен.', 'The selected profile is disabled.'),
      mtInformation, [mbOK], 0);
    Exit;
  end;
  if not ResolveRuntimeProvider(Index, Provider) then
    Exit;
  if not PrepareRuntimeConfig(FProfiles[Index], Provider, Config,
    ErrorMessage) then
  begin
    if (Pos('отсутствуют файлы', ErrorMessage) > 0) and
      (MessageDlg('Geo data', ErrorMessage + LineEnding + LineEnding +
        TZaryaTr.Tr('Открыть Geo Data Manager?', 'Open Geo Data Manager?'),
        mtWarning, [mbYes, mbNo], 0) = mrYes) then
      GeoDataClick(Self)
    else if Pos('отсутствуют файлы', ErrorMessage) = 0 then
      MessageDlg('Конфигурация provider', ErrorMessage, mtWarning, [mbOK], 0);
    Exit;
  end;
  if CanConnectLocalhost(FRuntimeCoordinator.ReadinessPort) then
  begin
    MessageDlg('Runtime provider', Format(
      TZaryaTr.Tr('Порт %s:%d уже занят. Выберите другой локальный порт.',
        'Port %s:%d is already in use. Choose another local port.'),
      [FRuntimeCoordinator.ReadinessHost,
       FRuntimeCoordinator.ReadinessPort]), mtWarning, [mbOK], 0);
    Exit;
  end;
  ForceDirectories(FXrayAssetDirectory);
  FRuntimeCoordinator.Activate(FProfiles[Index], Provider);
  if Provider.Distribution = pdExternal then
  begin
    AppendLog('Starting external provider ' + Provider.ProviderId + '…');
    if not StartExternalRuntime(Provider, Config, ErrorMessage) then
    begin
      DeleteRuntimeConfig(FActiveConfigPath);
      FActiveConfigPath := '';
      MessageDlg('External runtime', RedactRuntimeText(ErrorMessage),
        mtError, [mbOK], 0);
      FRuntimeCoordinator.Reset;
      Exit;
    end;
  end
  else
  begin
    if not Assigned(FEmbeddedXray) or not FEmbeddedXray.Available then
    begin
      MessageDlg('Xray runtime', FEmbeddedXray.LoadStatus, mtError, [mbOK], 0);
      FRuntimeCoordinator.Reset;
      Exit;
    end;
    if not ValidateEmbeddedRuntime(Provider, Config, ErrorMessage) then
    begin
      AppendLog('Embedded Xray validation failed.');
      ErrorMessage := RedactRuntimeText(ErrorMessage);
      MessageDlg('Xray runtime', ErrorMessage, mtError, [mbOK], 0);
      FRuntimeCoordinator.Reset;
      Exit;
    end;
    AppendLog('Starting embedded Xray…');
    if not FEmbeddedXray.Start(Config, FXrayAssetDirectory, ErrorMessage) then
    begin
      AppendLog('Embedded Xray start failed.');
      ErrorMessage := RedactRuntimeText(ErrorMessage);
      MessageDlg('Xray runtime', ErrorMessage, mtError, [mbOK], 0);
      FRuntimeCoordinator.Reset;
      Exit;
    end;
  end;
  FRuntimeCoordinator.BeginConnecting;
  FAppSettings.LastStartedProfileId := FProfiles[Index].Id;
  SaveAppSettings;
  FReadyTimer.Interval := 100;
  AppendLog(Format('Waiting for local endpoint %s:%d before enabling system proxy…',
    [FRuntimeCoordinator.ReadinessHost,
     FRuntimeCoordinator.ReadinessPort]));
  FReadyTimer.Enabled := True;
  UpdateRuntimeSurface;
end;

procedure TMainForm.StopClick(Sender: TObject);
begin
  if FRuntimeCoordinator.State = rsStopped then
    Exit;
  AppendLog('Stopping runtime provider ' +
    FRuntimeCoordinator.ActiveProvider.ProviderId + '…');
  StopRuntime(True);
end;

procedure TMainForm.ReadyTimerTimer(Sender: TObject);
var
  ErrorMessage: string;
  Poll: TZaryaRuntimePollResult;
begin
  DrainRuntimeLogs;
  if not FRuntimeCoordinator.Poll(ActiveRuntimeIsRunning,
    AutoProxyEnabledForCurrentStart, Poll) then Exit;
  case Poll.Kind of
    rpkStoppedUnexpectedly:
      HandleRuntimeFailure(TZaryaTr.Tr(
        'Runtime provider неожиданно остановился.',
        'The runtime provider stopped unexpectedly.'));
    rpkReady:
    begin
      AppendLog('Declared local endpoint accepted a connection; runtime is ready.');
      FReadyTimer.Interval := 200;
      if Poll.SystemProxyEnabled then
        AppendLog('System proxy (' + FRuntimeCoordinator.SystemProxyKind +
          ') enabled after runtime readiness.')
      else if Poll.ErrorMessage <> '' then
      begin
        AppendLog('System proxy enable failed: ' + Poll.ErrorMessage);
        MessageDlg(TZaryaTr.Tr('Системный прокси', 'System proxy'),
          Poll.ErrorMessage, mtWarning, [mbOK], 0);
      end
      else if Poll.SystemProxyDisabledBySetting then
        AppendLog('Automatic system proxy activation is disabled.');
      if Poll.UnsupportedSystemProxyKind then
        AppendLog('System proxy was not enabled: this profile does not expose a supported local proxy endpoint.');
      FAutoStarting := False;
      UpdateRuntimeSurface;
    end;
    rpkReadinessTimeout:
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
      FRuntimeCoordinator.Reset;
      FAutoStarting := False;
      AppendLog('Local runtime endpoint did not become ready; system proxy remains off.');
      UpdateRuntimeSurface;
      MessageDlg('Runtime provider',
        TZaryaTr.Tr(
          'Локальный endpoint provider не стал доступен за 5 секунд. Системный прокси не изменён.',
          'The provider local endpoint was not ready within 5 seconds. The system proxy was not changed.'),
        mtError, [mbOK], 0);
    end;
  end;
end;

procedure TMainForm.TestClick(Sender: TObject);
begin
  if FBackgroundOperations.ProfileTestRunning then
  begin
    FBackgroundOperations.CancelProfileTest;
    FTestButton.Enabled := False;
    FStatusBar.SimpleText := TZaryaTr.Tr(
      'Отмена TCP ping после текущего соединения…',
      'Canceling TCP ping after the current connection…');
    Exit;
  end;
  if Length(FProfiles) = 0 then
  begin
    MessageDlg('TCP ping', TZaryaTr.Tr('Нет профилей для проверки.',
      'There are no profiles to test.'), mtInformation,
      [mbOK], 0);
    Exit;
  end;
  FBackgroundOperations.StartProfileTest(FProfiles);
  FTestButton.Caption := TZaryaTr.Tr('Отмена');
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
  if not FBackgroundOperations.ProfileTestRunning then Exit;
  if not FBackgroundOperations.ProfileTestDone then
  begin
    CurrentIndex := FBackgroundOperations.ProfileTestCurrentIndex;
    if (CurrentIndex >= 0) and (CurrentIndex <= High(FProfiles)) then
      FStatusBar.SimpleText := Format('TCP ping %d/%d: %s',
        [CurrentIndex + 1, Length(FProfiles), FProfiles[CurrentIndex].Name]);
    Exit;
  end;
  FTestTimer.Enabled := False;
  if not FBackgroundOperations.TakeProfileTestResults(TestResults) then Exit;
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

function TMainForm.BuildRealDelayItems(out AItems: TZaryaRealDelayWorkItems;
  out AError: string): Boolean;
begin
  Result := FBackgroundOperations.BuildRealDelayItems(FProfiles,
    FCoreRegistry, FRuntimeCoordinator, ActiveRoutingProfile,
    ActiveDnsProfile, FGeoDataManager, FAppSettings,
    ExtractFileDir(FProfileService.FileName), FXrayAssetDirectory,
    AItems, AError);
end;
procedure TMainForm.RealDelayClick(Sender: TObject);
var
  Items: TZaryaRealDelayWorkItems;
  ErrorMessage: string;
begin
  if FBackgroundOperations.RealDelayRunning then
  begin
    FBackgroundOperations.CancelRealDelay;
    FRealDelayButton.Enabled := False;
    FStatusBar.SimpleText := 'Отмена Real delay…';
    Exit;
  end;
  if not BuildRealDelayItems(Items, ErrorMessage) then
  begin
    MessageDlg('Real delay', ErrorMessage, mtInformation, [mbOK], 0);
    Exit;
  end;
  FBackgroundOperations.StartRealDelay(Items,
    FAppSettings.RealDelayConcurrency);
  FRealDelayButton.Caption := TZaryaTr.Tr('Отмена');
  FRealDelayTimer.Enabled := True;
  FStatusBar.SimpleText := Format('Real delay: 0/%d', [Length(Items)]);
  AppendLog(Format('Real delay batch started: profiles=%d concurrency=%d.',
    [Length(Items), FAppSettings.RealDelayConcurrency]));
end;

procedure TMainForm.RealDelayTimerTimer(Sender: TObject);
var
  Results: TZaryaRealDelayResults;
  I, J, Completed, SuccessCount: Integer;
  CurrentNames, SelectedId: string;
begin
  if not FBackgroundOperations.RealDelayRunning then Exit;
  if not FBackgroundOperations.RealDelayDone then
  begin
    Completed := FBackgroundOperations.RealDelayCompletedCount;
    CurrentNames := FBackgroundOperations.RealDelayProgressText;
    FStatusBar.SimpleText := Format('Real delay %d/%d: %s',
      [Completed, FBackgroundOperations.RealDelayResultCount, CurrentNames]);
    Exit;
  end;
  FRealDelayTimer.Enabled := False;
  if not FBackgroundOperations.TakeRealDelayResults(Results) then Exit;
  if SelectedProfileIndex >= 0 then SelectedId :=
    FProfiles[SelectedProfileIndex].Id else SelectedId := '';
  Completed := 0;
  SuccessCount := 0;
  for I := 0 to High(Results) do
    if Results[I].Tested then
    begin
      Inc(Completed);
      for J := 0 to High(FProfiles) do
        if SameText(FProfiles[J].Id, Results[I].ProfileId) then
        begin
          FProfiles[J].LastTestedAt := FormatDateTime(
            'yyyy-mm-dd"T"hh:nn:ss', Now);
          if Results[I].Success then
          begin
            Inc(SuccessCount);
            FProfiles[J].LastRealDelayMs := Results[I].DelayMs;
            FProfiles[J].Latency := IntToStr(Results[I].DelayMs) + ' мс';
            FProfiles[J].LastTestStatus := 'success';
            FProfiles[J].LastTestError := '';
          end
          else
          begin
            FProfiles[J].LastRealDelayMs := -1;
            FProfiles[J].Latency := 'Ошибка';
            FProfiles[J].LastTestStatus := 'failed';
            FProfiles[J].LastTestError := Results[I].ErrorCode + ': ' +
              Results[I].ErrorMessage;
          end;
          Break;
        end;
    end;
  SaveProfiles;
  RefreshProfileGrid(SelectedId);
  FRealDelayButton.Caption := 'Real delay';
  FRealDelayButton.Enabled := True;
  FStatusBar.SimpleText := Format(TZaryaTr.Tr(
    'Real delay: успешно %d из %d', 'Real delay: %d of %d succeeded'),
    [SuccessCount, Completed]);
  AppendLog(Format('Real delay batch completed: success=%d total=%d.',
    [SuccessCount, Completed]));
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
    MessageDlg('Runtime config', TZaryaTr.Tr('Сначала выберите профиль.',
      'Select a profile first.'), mtInformation,
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
        TZaryaTr.Tr('Для выбранного provider нет config adapter.',
          'There is no config adapter for the selected provider.'),
        mtWarning, [mbOK], 0);
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
    MessageDlg(TZaryaTr.Tr('Профиль', 'Profile'), TZaryaTr.Tr(
      'Выберите профиль для изменения.', 'Select a profile to edit.'), mtInformation,
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
    MessageDlg(TZaryaTr.Tr('Профиль', 'Profile'), TZaryaTr.Tr(
      'Выберите профиль для удаления.', 'Select a profile to delete.'), mtInformation,
      [mbOK], 0);
    Exit;
  end;
  ProfileName := FProfiles[Index].Name;
  if MessageDlg(TZaryaTr.Tr('Удалить профиль', 'Delete profile'),
    TZaryaTr.Tr('Удалить профиль «', 'Delete profile “') + ProfileName +
    TZaryaTr.Tr('»?', '”?'),
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
        Errors.Add(Format(TZaryaTr.Tr('Строка %d: %s', 'Line %d: %s'),
          [I + 1, ErrorMessage]));
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
      MessageDlg(TZaryaTr.Tr('Импорт профилей', 'Import profiles'),
        Format(TZaryaTr.Tr('Импортировано: %d', 'Imported: %d'), [ImportedCount]) +
        LineEnding + LineEnding + Errors.Text, mtWarning, [mbOK], 0)
    else if ImportedCount > 0 then
      MessageDlg(TZaryaTr.Tr('Импорт профилей', 'Import profiles'),
        Format(TZaryaTr.Tr('Импортировано профилей: %d', 'Profiles imported: %d'),
        [ImportedCount]), mtInformation, [mbOK], 0)
    else
      MessageDlg(TZaryaTr.Tr('Импорт профилей', 'Import profiles'),
        TZaryaTr.Tr('Вставьте хотя бы одну поддерживаемую share link.',
          'Paste at least one supported share link.'),
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
  Dialog := TSubscriptionManagerDialog.Create(Self, FProfiles,
    FProfileService.Store, ExtractFileDir(FProfileService.FileName),
    @AppendLog);
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
  Dialog: TZaryaSettingsDialog;
  Candidate: TZaryaAppSettings;
  ErrorMessage: string;
begin
  if FRuntimeCoordinator.State <> rsStopped then
  begin
    MessageDlg(TZaryaTr.Tr('Настройки'), TZaryaTr.Tr(
      'Остановите runtime перед изменением mixed-порта и системного прокси.',
      'Stop the runtime before changing the mixed port or system proxy.'),
      mtInformation, [mbOK], 0);
    Exit;
  end;
  Dialog := TZaryaSettingsDialog.Create(Self, FAppSettings);
  try
    if Dialog.ShowModal = mrOk then
    begin
      Candidate := FAppSettings;
      Dialog.ApplyTo(Candidate);
      if Candidate.StartAtLogin then
      begin
        if Candidate.StartMinimizedToTray then
        begin
          if not FAutostartManager.SetEnabled(True, ExpandFileName(ParamStr(0)),
            ['--minimized'], ErrorMessage) then
          begin
            MessageDlg(TZaryaTr.Tr('Автозапуск', 'Startup'), ErrorMessage,
              mtError, [mbOK], 0);
            Exit;
          end;
        end
        else if not FAutostartManager.SetEnabled(True,
          ExpandFileName(ParamStr(0)), [], ErrorMessage) then
        begin
          MessageDlg(TZaryaTr.Tr('Автозапуск', 'Startup'), ErrorMessage,
            mtError, [mbOK], 0);
          Exit;
        end;
      end
      else if not FAutostartManager.SetEnabled(False,
        ExpandFileName(ParamStr(0)), [], ErrorMessage) then
      begin
        MessageDlg(TZaryaTr.Tr('Автозапуск', 'Startup'), ErrorMessage,
          mtError, [mbOK], 0);
        Exit;
      end;
      FAppSettings := Candidate;
      FDarkTheme := FAppSettings.DarkTheme;
      FMinimizeToTray := FAppSettings.MinimizeToTray;
      SaveAppSettings;
      ApplyCurrentTheme;
      AppendLog('Application settings applied.');
    end;
  finally
    Dialog.Free;
  end;
end;

procedure TMainForm.PoliciesClick(Sender: TObject);
var
  RoutingProfiles: TZaryaRoutingProfiles;
  DnsProfiles: TZaryaDnsProfiles;
  RoutingId, DnsId: string;
begin
  if FRuntimeCoordinator.State <> rsStopped then
  begin
    MessageDlg(TZaryaTr.Tr('Routing и DNS', 'Routing and DNS'), TZaryaTr.Tr(
      'Остановите runtime перед изменением активных политик.',
      'Stop the runtime before changing active policies.'),
      mtInformation, [mbOK], 0);
    Exit;
  end;
  RoutingProfiles := Copy(FRoutingProfiles);
  DnsProfiles := Copy(FDnsProfiles);
  RoutingId := FAppSettings.SelectedRoutingProfileId;
  DnsId := FAppSettings.SelectedDnsProfileId;
  if not TPolicyManagerDialog.Execute(Self, RoutingProfiles, DnsProfiles,
    RoutingId, DnsId, FDarkTheme) then
    Exit;
  FRoutingProfiles := RoutingProfiles;
  FDnsProfiles := DnsProfiles;
  FAppSettings.SelectedRoutingProfileId := RoutingId;
  FAppSettings.SelectedDnsProfileId := DnsId;
  if SavePolicies then
    AppendLog('Active policies: routing=' + RoutingId + ', dns=' + DnsId + '.');
end;

procedure TMainForm.GeoDataClick(Sender: TObject);
var
  SourceId: string;
begin
  SourceId := FAppSettings.GeoSourceId;
  TGeoDataManagerDialog.Execute(Self, FGeoDataManager, SourceId, FDarkTheme);
  if not SameText(SourceId, FAppSettings.GeoSourceId) then
  begin
    FAppSettings.GeoSourceId := SourceId;
    SaveAppSettings;
  end;
end;

procedure TMainForm.CoreManagerClick(Sender: TObject);
var
  ProviderId: string;
  Provider: TZaryaCoreProvider;
  I: Integer;
  CompatibleCount: Integer;
begin
  if FRuntimeCoordinator.State <> rsStopped then
  begin
    MessageDlg(TZaryaTr.Tr('Ядра'), TZaryaTr.Tr(
      'Остановите runtime перед изменением ядер.',
      'Stop the runtime before changing cores.'),
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
    MessageDlg(TZaryaTr.Tr('Provider профилей', 'Profile provider'),
      TZaryaTr.Tr(
        'Совместимых профилей не найдено. Raw-конфигурации требуют тот же диалект.',
        'No compatible profiles were found. Raw configurations require the same dialect.'),
      mtInformation, [mbOK], 0);
    Exit;
  end;
  if MessageDlg(TZaryaTr.Tr('Provider профилей', 'Profile provider'), Format(
    TZaryaTr.Tr('Назначить %s для совместимых профилей (%d)?',
      'Assign %s to compatible profiles (%d)?'),
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
  DataDirectory := ExtractFileDir(FProfileService.FileName);
  BackupDirectory := IncludeTrailingPathDelimiter(
    ExtractFileDir(DataDirectory)) + 'backups';
  ForceDirectories(BackupDirectory);
  Dialog := TSaveDialog.Create(Self);
  try
    Dialog.Title := TZaryaTr.Tr('Создать резервную копию Zarya',
      'Create a Zarya backup');
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
      MessageDlg('Backup', TZaryaTr.Tr('Не удалось создать backup:',
        'Could not create the backup:') + LineEnding +
        ErrorMessage, mtError, [mbOK], 0);
      Exit;
    end;
    AppendLog('Verified Zarya backup created.');
    MessageDlg('Backup', TZaryaTr.Tr(
      'Проверяемая резервная копия создана:',
      'Verified backup created:') + LineEnding +
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
  if FRuntimeCoordinator.State <> rsStopped then
  begin
    MessageDlg(TZaryaTr.Tr('Восстановление backup', 'Restore backup'),
      TZaryaTr.Tr('Сначала остановите активный runtime.',
        'Stop the active runtime first.'), mtWarning, [mbOK], 0);
    Exit;
  end;
  Dialog := TOpenDialog.Create(Self);
  try
    Dialog.Title := TZaryaTr.Tr('Восстановить резервную копию Zarya',
      'Restore a Zarya backup');
    Dialog.Filter := 'Zarya backup (*.zarya-backup.zip)|*.zarya-backup.zip|' +
      'ZIP archives (*.zip)|*.zip';
    Dialog.Options := Dialog.Options + [ofFileMustExist, ofPathMustExist];
    if not Dialog.Execute then
      Exit;
    if MessageDlg(TZaryaTr.Tr('Восстановление backup', 'Restore backup'),
      TZaryaTr.Tr(
      'Текущие файлы данных будут заменены после staging-проверки.' +
      LineEnding +
      'Credentials, raw config и пути к внешним EXE не входят в backup; ' +
      'профили после восстановления отключены.' +
      LineEnding + LineEnding + 'Продолжить?',
      'Current data files will be replaced after staging validation.' +
      LineEnding +
      'Credentials, raw configuration, and external EXE paths are omitted; ' +
      'restored profiles are disabled.' +
      LineEnding + LineEnding + 'Continue?'), mtConfirmation,
      [mbYes, mbNo], 0) <> mrYes then
      Exit;
    DataDirectory := ExtractFileDir(FProfileService.FileName);
    if not RestoreZaryaBackup(Dialog.FileName, DataDirectory,
      PreRestoreBackup, ErrorMessage) then
    begin
      MessageDlg(TZaryaTr.Tr('Восстановление backup', 'Restore backup'),
        TZaryaTr.Tr('Данные не восстановлены:', 'Data was not restored:') +
        LineEnding + ErrorMessage,
        mtError, [mbOK], 0);
      Exit;
    end;
    if not FCoreRegistry.Load(ErrorMessage) then
      MessageDlg(TZaryaTr.Tr('Ядра'), TZaryaTr.Tr(
        'Backup восстановлен, но providers.json не прочитан:',
        'The backup was restored, but providers.json could not be read:') +
        LineEnding + ErrorMessage, mtWarning, [mbOK], 0);
    FCoreRegistry.SetEmbeddedState(ProviderEmbeddedXray,
      FEmbeddedXray.Version, FEmbeddedXray.Available,
      FEmbeddedXray.LoadStatus);
    LoadAppSettings;
    LoadProfiles;
    ApplyCurrentTheme;
    UpdateRuntimeSurface;
    AppendLog('Verified Zarya backup restored.');
    MessageDlg(TZaryaTr.Tr('Восстановление backup', 'Restore backup'),
      TZaryaTr.Tr('Данные восстановлены и перечитаны.',
        'Data was restored and reloaded.') + LineEnding +
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
    Dialog.Title := TZaryaTr.Tr('Сохранить безопасный diagnostics bundle',
      'Save a safe diagnostics bundle');
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
      MessageDlg(TZaryaTr.Tr('Диагностика', 'Diagnostics'), TZaryaTr.Tr(
        'Не удалось создать bundle:', 'Could not create the bundle:') + LineEnding +
        ErrorMessage, mtError, [mbOK], 0);
      Exit;
    end;
    AppendLog('Redacted diagnostics bundle created.');
    MessageDlg(TZaryaTr.Tr('Диагностика', 'Diagnostics'), TZaryaTr.Tr(
      'Bundle создан без runtime-логов, raw config, credentials и путей EXE.',
      'The bundle excludes runtime logs, raw configuration, credentials, and EXE paths.') +
      LineEnding + Dialog.FileName, mtInformation, [mbOK], 0);
  finally
    Dialog.Free;
  end;
end;

procedure TMainForm.AboutClick(Sender: TObject);
begin
  MessageDlg(TZaryaTr.Tr('О Zarya', 'About Zarya'),
    'Zarya · FPC/LCL' + LineEnding + LineEnding +
    TZaryaTr.Tr('Локальные профили, подписки, share links, runtime-конфигурации, ' +
    'встроенные и внешние providers.' + LineEnding +
    'Production-сборка статически линкует Xray через общий zarya-xray ABI. ' +
    'Системный прокси включается только после подтверждённой готовности ' +
    'объявленного локального endpoint.',
    'Local profiles, subscriptions, share links, runtime configurations, ' +
    'and embedded or external providers.' + LineEnding +
    'The production build statically links Xray through the shared zarya-xray ABI. ' +
    'The system proxy is enabled only after the declared local endpoint is ready.'),
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
  LayoutToolbar;
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
