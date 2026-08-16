unit ZaryaSettingsForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ComCtrls, ButtonPanel, ZaryaThemes, ZaryaAppSettings;

type
  TZaryaSettingsDialog = class(TForm)
  private
    FOriginal: TZaryaAppSettings;
    FThemeCombo: TComboBox;
    FLanguageCombo: TComboBox;
    FMinimizeToTrayCheck: TCheckBox;
    FPortEdit: TEdit;
    FAutoProxyCheck: TCheckBox;
    FRestoreProxyCheck: TCheckBox;
    FStartAtLoginCheck: TCheckBox;
    FStartMinimizedCheck: TCheckBox;
    FAutoStartProfileCheck: TCheckBox;
    FAutoStartProxyCheck: TCheckBox;
    FAutoStartDelayEdit: TEdit;
    FConcurrencyEdit: TEdit;
    FTimeoutEdit: TEdit;
    FTestUrlEdit: TEdit;
    FGeoSourceCombo: TComboBox;
    FGeoAutoCheck: TCheckBox;
    FGeoWarnCheck: TCheckBox;
    procedure BuildInterface;
    procedure AcceptClick(Sender: TObject);
    procedure AddLabel(AParent: TWinControl; const ACaption: string;
      const ALeft, ATop, AWidth: Integer);
  public
    constructor Create(AOwner: TComponent;
      const ASettings: TZaryaAppSettings); reintroduce;
    procedure ApplyTo(var ASettings: TZaryaAppSettings);
  end;

implementation

uses
  ZaryaTr;

procedure TZaryaSettingsDialog.AddLabel(AParent: TWinControl;
  const ACaption: string; const ALeft, ATop, AWidth: Integer);
var
  LabelControl: TLabel;
begin
  LabelControl := TLabel.Create(Self);
  LabelControl.Parent := AParent;
  LabelControl.Caption := ACaption;
  LabelControl.SetBounds(ALeft, ATop, AWidth, 24);
end;

constructor TZaryaSettingsDialog.Create(AOwner: TComponent;
  const ASettings: TZaryaAppSettings);
var
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  FOriginal := ASettings;
  Caption := TZaryaTr.Tr('Настройки Zarya', 'Zarya Settings');
  BorderStyle := bsDialog;
  Position := poOwnerFormCenter;
  ClientWidth := 620;
  ClientHeight := 520;
  Constraints.MinWidth := 580;
  Constraints.MinHeight := 480;
  BuildInterface;
  FThemeCombo.ItemIndex := Ord(ASettings.DarkTheme);
  if ASettings.Language = 'ru' then FLanguageCombo.ItemIndex := 1
  else if ASettings.Language = 'en' then FLanguageCombo.ItemIndex := 2
  else FLanguageCombo.ItemIndex := 0;
  FMinimizeToTrayCheck.Checked := ASettings.MinimizeToTray;
  FPortEdit.Text := IntToStr(ASettings.MixedPort);
  FAutoProxyCheck.Checked := ASettings.AutoEnableSystemProxy;
  FRestoreProxyCheck.Checked := ASettings.RestoreSystemProxy;
  FStartAtLoginCheck.Checked := ASettings.StartAtLogin;
  FStartMinimizedCheck.Checked := ASettings.StartMinimizedToTray;
  FAutoStartProfileCheck.Checked := ASettings.AutoStartLastProfile;
  FAutoStartProxyCheck.Checked := ASettings.AutoEnableSystemProxyAfterAutoStart;
  FAutoStartDelayEdit.Text := IntToStr(ASettings.AutoStartDelaySeconds);
  FConcurrencyEdit.Text := IntToStr(ASettings.RealDelayConcurrency);
  FTimeoutEdit.Text := IntToStr(ASettings.RealDelayTimeoutSeconds);
  FTestUrlEdit.Text := ASettings.RealDelayTestUrl;
  if SameText(ASettings.GeoSourceId, 'loyalsoldier') then
    FGeoSourceCombo.ItemIndex := 1
  else if SameText(ASettings.GeoSourceId, 'chocolate4u') then
    FGeoSourceCombo.ItemIndex := 2
  else
    FGeoSourceCombo.ItemIndex := 0;
  FGeoAutoCheck.Checked := ASettings.GeoAutoCheckOnStartup;
  FGeoWarnCheck.Checked := ASettings.GeoWarnIfMissing;
  if ASettings.DarkTheme then Theme := ZaryaThemes.DarkTheme
  else Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

procedure TZaryaSettingsDialog.BuildInterface;
var
  Pages: TPageControl;
  InterfaceTab, ProxyTab, StartupTab, TestingTab: TTabSheet;
  Buttons: TButtonPanel;
begin
  Buttons := TButtonPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.ShowButtons := [pbOK, pbCancel];
  Buttons.ShowGlyphs := [];
  Buttons.OKButton.Caption := TZaryaTr.Tr('Применить');
  Buttons.OKButton.ModalResult := mrNone;
  Buttons.OKButton.OnClick := @AcceptClick;
  Buttons.CancelButton.Caption := TZaryaTr.Tr('Отмена');
  Pages := TPageControl.Create(Self);
  Pages.Parent := Self;
  Pages.Align := alClient;
  Pages.BorderSpacing.Around := 12;

  InterfaceTab := TTabSheet.Create(Self);
  InterfaceTab.PageControl := Pages;
  InterfaceTab.Caption := TZaryaTr.Tr('Интерфейс', 'Interface');
  AddLabel(InterfaceTab, TZaryaTr.Tr('Тема', 'Theme'), 24, 28, 180);
  FThemeCombo := TComboBox.Create(Self);
  FThemeCombo.Parent := InterfaceTab;
  FThemeCombo.Style := csDropDownList;
  FThemeCombo.Items.AddStrings([
    TZaryaTr.Tr('Светлая', 'Light'), TZaryaTr.Tr('Тёмная', 'Dark')]);
  FThemeCombo.SetBounds(220, 23, 280, 30);
  AddLabel(InterfaceTab, TZaryaTr.Tr('Язык', 'Language'), 24, 76, 180);
  FLanguageCombo := TComboBox.Create(Self);
  FLanguageCombo.Parent := InterfaceTab;
  FLanguageCombo.Style := csDropDownList;
  FLanguageCombo.Items.AddStrings([
    TZaryaTr.Tr('Системный'), TZaryaTr.Tr('Русский'), 'English']);
  FLanguageCombo.SetBounds(220, 71, 280, 30);
  FMinimizeToTrayCheck := TCheckBox.Create(Self);
  FMinimizeToTrayCheck.Parent := InterfaceTab;
  FMinimizeToTrayCheck.Caption := TZaryaTr.Tr(
    'Сворачивать в tray при закрытии окна',
    'Minimize to tray when closing the window');
  FMinimizeToTrayCheck.SetBounds(24, 130, 460, 26);
  AddLabel(InterfaceTab,
    TZaryaTr.Tr('Изменение языка применяется после перезапуска.',
      'Language changes take effect after restart.'), 24, 184, 500);

  ProxyTab := TTabSheet.Create(Self);
  ProxyTab.PageControl := Pages;
  ProxyTab.Caption := TZaryaTr.Tr('Прокси', 'Proxy');
  AddLabel(ProxyTab, TZaryaTr.Tr('Локальный mixed-порт',
    'Local mixed port'), 24, 28, 180);
  FPortEdit := TEdit.Create(Self);
  FPortEdit.Parent := ProxyTab;
  FPortEdit.NumbersOnly := True;
  FPortEdit.SetBounds(220, 23, 120, 30);
  FAutoProxyCheck := TCheckBox.Create(Self);
  FAutoProxyCheck.Parent := ProxyTab;
  FAutoProxyCheck.Caption := TZaryaTr.Tr(
    'Включать системный прокси после готовности порта',
    'Enable the system proxy after the local port is ready');
  FAutoProxyCheck.SetBounds(24, 80, 500, 26);
  FRestoreProxyCheck := TCheckBox.Create(Self);
  FRestoreProxyCheck.Parent := ProxyTab;
  FRestoreProxyCheck.Caption := TZaryaTr.Tr(
    'Восстанавливать системный прокси при остановке',
    'Restore the previous system proxy when stopping');
  FRestoreProxyCheck.SetBounds(24, 116, 500, 26);
  AddLabel(ProxyTab,
    TZaryaTr.Tr(
      'Проверка readiness выполняется каждые 100 мс, максимум 5 секунд.',
      'Readiness is checked every 100 ms for up to 5 seconds.'),
    24, 174, 520);

  StartupTab := TTabSheet.Create(Self);
  StartupTab.PageControl := Pages;
  StartupTab.Caption := TZaryaTr.Tr('Автозапуск', 'Startup');
  FStartAtLoginCheck := TCheckBox.Create(Self);
  FStartAtLoginCheck.Parent := StartupTab;
  FStartAtLoginCheck.Caption := TZaryaTr.Tr(
    'Запускать Zarya при входе в Windows', 'Start Zarya at Windows sign-in');
  FStartAtLoginCheck.SetBounds(24, 28, 500, 26);
  FStartMinimizedCheck := TCheckBox.Create(Self);
  FStartMinimizedCheck.Parent := StartupTab;
  FStartMinimizedCheck.Caption := TZaryaTr.Tr(
    'Запускать свёрнутой в tray', 'Start minimized to tray');
  FStartMinimizedCheck.SetBounds(24, 64, 500, 26);
  FAutoStartProfileCheck := TCheckBox.Create(Self);
  FAutoStartProfileCheck.Parent := StartupTab;
  FAutoStartProfileCheck.Caption := TZaryaTr.Tr(
    'Подключать последний использованный профиль',
    'Connect the last used profile');
  FAutoStartProfileCheck.SetBounds(24, 100, 500, 26);
  FAutoStartProxyCheck := TCheckBox.Create(Self);
  FAutoStartProxyCheck.Parent := StartupTab;
  FAutoStartProxyCheck.Caption :=
    TZaryaTr.Tr('После автоподключения включать системный прокси',
      'Enable the system proxy after automatic connection');
  FAutoStartProxyCheck.SetBounds(24, 136, 520, 26);
  AddLabel(StartupTab, TZaryaTr.Tr('Задержка подключения, секунд',
    'Connection delay, seconds'), 24, 184, 240);
  FAutoStartDelayEdit := TEdit.Create(Self);
  FAutoStartDelayEdit.Parent := StartupTab;
  FAutoStartDelayEdit.NumbersOnly := True;
  FAutoStartDelayEdit.SetBounds(270, 179, 100, 30);

  TestingTab := TTabSheet.Create(Self);
  TestingTab.PageControl := Pages;
  TestingTab.Caption := TZaryaTr.Tr('Тесты и данные', 'Tests and data');
  AddLabel(TestingTab, TZaryaTr.Tr('Параллельных Real delay тестов',
    'Parallel Real delay tests'), 24, 28, 250);
  FConcurrencyEdit := TEdit.Create(Self);
  FConcurrencyEdit.Parent := TestingTab;
  FConcurrencyEdit.NumbersOnly := True;
  FConcurrencyEdit.SetBounds(300, 23, 90, 30);
  AddLabel(TestingTab, TZaryaTr.Tr('Timeout Real delay, секунд',
    'Real delay timeout, seconds'), 24, 72, 250);
  FTimeoutEdit := TEdit.Create(Self);
  FTimeoutEdit.Parent := TestingTab;
  FTimeoutEdit.NumbersOnly := True;
  FTimeoutEdit.SetBounds(300, 67, 90, 30);
  AddLabel(TestingTab, TZaryaTr.Tr('URL проверки', 'Probe URL'),
    24, 116, 250);
  FTestUrlEdit := TEdit.Create(Self);
  FTestUrlEdit.Parent := TestingTab;
  FTestUrlEdit.SetBounds(24, 142, 540, 30);
  AddLabel(TestingTab, TZaryaTr.Tr('Источник geo data', 'Geo data source'),
    24, 190, 250);
  FGeoSourceCombo := TComboBox.Create(Self);
  FGeoSourceCombo.Parent := TestingTab;
  FGeoSourceCombo.Style := csDropDownList;
  FGeoSourceCombo.Items.AddStrings(['runetfreedom', 'Loyalsoldier',
    'Chocolate4U']);
  FGeoSourceCombo.SetBounds(300, 185, 220, 30);
  FGeoAutoCheck := TCheckBox.Create(Self);
  FGeoAutoCheck.Parent := TestingTab;
  FGeoAutoCheck.Caption := TZaryaTr.Tr(
    'Проверять наличие geo data при запуске',
    'Check for missing geo data at startup');
  FGeoAutoCheck.SetBounds(24, 236, 500, 26);
  FGeoWarnCheck := TCheckBox.Create(Self);
  FGeoWarnCheck.Parent := TestingTab;
  FGeoWarnCheck.Caption := TZaryaTr.Tr(
    'Предупреждать о недостающих geo data', 'Warn about missing geo data');
  FGeoWarnCheck.SetBounds(24, 270, 500, 26);
end;

procedure TZaryaSettingsDialog.AcceptClick(Sender: TObject);
var
  Value: Integer;
begin
  Value := StrToIntDef(FPortEdit.Text, 0);
  if (Value < 1) or (Value > 65535) then
  begin
    MessageDlg(TZaryaTr.Tr('Настройки'), TZaryaTr.Tr(
      'Mixed-порт должен быть от 1 до 65535.',
      'The mixed port must be between 1 and 65535.'),
      mtWarning, [mbOK], 0);
    Exit;
  end;
  Value := StrToIntDef(FAutoStartDelayEdit.Text, -1);
  if (Value < 0) or (Value > 120) then
  begin
    MessageDlg(TZaryaTr.Tr('Настройки'), TZaryaTr.Tr(
      'Задержка автозапуска должна быть от 0 до 120 секунд.',
      'The startup delay must be between 0 and 120 seconds.'),
      mtWarning, [mbOK], 0);
    Exit;
  end;
  Value := StrToIntDef(FConcurrencyEdit.Text, 0);
  if (Value < 1) or (Value > 10) then
  begin
    MessageDlg(TZaryaTr.Tr('Настройки'), TZaryaTr.Tr(
      'Concurrency должна быть от 1 до 10.',
      'Concurrency must be between 1 and 10.'),
      mtWarning, [mbOK], 0);
    Exit;
  end;
  Value := StrToIntDef(FTimeoutEdit.Text, 0);
  if (Value < 1) or (Value > 60) then
  begin
    MessageDlg(TZaryaTr.Tr('Настройки'), TZaryaTr.Tr(
      'Timeout должен быть от 1 до 60 секунд.',
      'The timeout must be between 1 and 60 seconds.'),
      mtWarning, [mbOK], 0);
    Exit;
  end;
  if (Pos('https://', LowerCase(Trim(FTestUrlEdit.Text))) <> 1) and
    (Pos('http://', LowerCase(Trim(FTestUrlEdit.Text))) <> 1) then
  begin
    MessageDlg(TZaryaTr.Tr('Настройки'), TZaryaTr.Tr(
      'URL проверки должен использовать HTTPS или HTTP.',
      'The probe URL must use HTTPS or HTTP.'),
      mtWarning, [mbOK], 0);
    Exit;
  end;
  ModalResult := mrOk;
end;

procedure TZaryaSettingsDialog.ApplyTo(var ASettings: TZaryaAppSettings);
begin
  ASettings := FOriginal;
  ASettings.DarkTheme := FThemeCombo.ItemIndex = 1;
  case FLanguageCombo.ItemIndex of
    1: ASettings.Language := 'ru';
    2: ASettings.Language := 'en';
  else
    ASettings.Language := 'system';
  end;
  ASettings.MinimizeToTray := FMinimizeToTrayCheck.Checked;
  ASettings.MixedPort := StrToIntDef(FPortEdit.Text, 10808);
  ASettings.AutoEnableSystemProxy := FAutoProxyCheck.Checked;
  ASettings.RestoreSystemProxy := FRestoreProxyCheck.Checked;
  ASettings.StartAtLogin := FStartAtLoginCheck.Checked;
  ASettings.StartMinimizedToTray := FStartMinimizedCheck.Checked;
  ASettings.AutoStartLastProfile := FAutoStartProfileCheck.Checked;
  ASettings.AutoEnableSystemProxyAfterAutoStart := FAutoStartProxyCheck.Checked;
  ASettings.AutoStartDelaySeconds := StrToIntDef(FAutoStartDelayEdit.Text, 3);
  ASettings.RealDelayConcurrency := StrToIntDef(FConcurrencyEdit.Text, 3);
  ASettings.RealDelayTimeoutSeconds := StrToIntDef(FTimeoutEdit.Text, 10);
  ASettings.RealDelayTestUrl := Trim(FTestUrlEdit.Text);
  case FGeoSourceCombo.ItemIndex of
    1: ASettings.GeoSourceId := 'loyalsoldier';
    2: ASettings.GeoSourceId := 'chocolate4u';
  else
    ASettings.GeoSourceId := 'runetfreedom';
  end;
  ASettings.GeoAutoCheckOnStartup := FGeoAutoCheck.Checked;
  ASettings.GeoWarnIfMissing := FGeoWarnCheck.Checked;
end;

end.
