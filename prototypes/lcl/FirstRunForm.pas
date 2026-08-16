unit FirstRunForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, StdCtrls, ExtCtrls, Dialogs,
  ZaryaAppSettings;

type
  TZaryaFirstRunDialog = class(TForm)
  private
    FSettings: TZaryaAppSettings;
    FWelcomeLabel: TLabel;
    FDescription: TMemo;
    FLanguageLabel: TLabel;
    FLanguageCombo: TComboBox;
    FPrivacyLabel: TLabel;
    FContinueButton: TButton;
    FExitButton: TButton;
    procedure LanguageChanged(Sender: TObject);
    procedure ContinueClick(Sender: TObject);
    procedure Localize;
  public
    constructor Create(AOwner: TComponent;
      const ASettings: TZaryaAppSettings); reintroduce;
    procedure ApplyTo(var ASettings: TZaryaAppSettings);
  end;

function RunFirstRunDialog(AOwner: TComponent;
  var ASettings: TZaryaAppSettings): Boolean;

implementation

uses
  ZaryaTr;

constructor TZaryaFirstRunDialog.Create(AOwner: TComponent;
  const ASettings: TZaryaAppSettings);
var
  ButtonPanel: TPanel;
begin
  inherited CreateNew(AOwner, 1);
  FSettings := ASettings;
  Position := poScreenCenter;
  BorderStyle := bsDialog;
  ClientWidth := 640;
  ClientHeight := 430;
  Constraints.MinWidth := 560;
  Constraints.MinHeight := 400;
  Scaled := True;

  FWelcomeLabel := TLabel.Create(Self);
  FWelcomeLabel.Parent := Self;
  FWelcomeLabel.Font.Size := 18;
  FWelcomeLabel.Font.Style := [fsBold];
  FWelcomeLabel.SetBounds(28, 24, 584, 34);

  FDescription := TMemo.Create(Self);
  FDescription.Parent := Self;
  FDescription.ReadOnly := True;
  FDescription.ScrollBars := ssAutoVertical;
  FDescription.TabStop := False;
  FDescription.SetBounds(28, 72, 584, 205);

  FLanguageLabel := TLabel.Create(Self);
  FLanguageLabel.Parent := Self;
  FLanguageLabel.SetBounds(28, 298, 170, 24);

  FLanguageCombo := TComboBox.Create(Self);
  FLanguageCombo.Parent := Self;
  FLanguageCombo.Style := csDropDownList;
  FLanguageCombo.Items.Add('System');
  FLanguageCombo.Items.Add('Русский');
  FLanguageCombo.Items.Add('English');
  if SameText(FSettings.Language, 'ru') then FLanguageCombo.ItemIndex := 1
  else if SameText(FSettings.Language, 'en') then FLanguageCombo.ItemIndex := 2
  else FLanguageCombo.ItemIndex := 0;
  FLanguageCombo.SetBounds(210, 294, 200, 30);
  FLanguageCombo.OnChange := @LanguageChanged;

  FPrivacyLabel := TLabel.Create(Self);
  FPrivacyLabel.Parent := Self;
  FPrivacyLabel.WordWrap := True;
  FPrivacyLabel.SetBounds(28, 337, 584, 42);

  ButtonPanel := TPanel.Create(Self);
  ButtonPanel.Parent := Self;
  ButtonPanel.Align := alBottom;
  ButtonPanel.Height := 56;
  ButtonPanel.BevelOuter := bvNone;

  FContinueButton := TButton.Create(Self);
  FContinueButton.Parent := ButtonPanel;
  FContinueButton.Default := True;
  FContinueButton.SetBounds(376, 12, 128, 32);
  FContinueButton.OnClick := @ContinueClick;

  FExitButton := TButton.Create(Self);
  FExitButton.Parent := ButtonPanel;
  FExitButton.Cancel := True;
  FExitButton.ModalResult := mrCancel;
  FExitButton.SetBounds(512, 12, 100, 32);
  Localize;
end;

procedure TZaryaFirstRunDialog.Localize;
begin
  Caption := TZaryaTr.Tr('Первый запуск Zarya', 'Welcome to Zarya');
  FWelcomeLabel.Caption := TZaryaTr.Tr('Добро пожаловать в Zarya',
    'Welcome to Zarya');
  FDescription.Text := TZaryaTr.Tr(
    'Stable runtime — встроенный Xray.' + LineEnding + LineEnding +
    'Системный прокси включается только после готовности локального endpoint.' +
    LineEnding + LineEnding +
    'Zarya не загружает и не обновляет внешние EXE. Подключайте только ядра, ' +
    'которым доверяете.' + LineEnding + LineEnding +
    'TUN, helper, kill switch и встроенный sing-box пока скрыты feature gate.',
    'The stable runtime is embedded Xray.' + LineEnding + LineEnding +
    'The system proxy is enabled only after the local endpoint is ready.' +
    LineEnding + LineEnding +
    'Zarya does not download or update external EXE files. Register only ' +
    'cores you trust.' + LineEnding + LineEnding +
    'TUN, helper, kill switch, and embedded sing-box remain feature-gated.');
  FLanguageLabel.Caption := TZaryaTr.Tr('Язык интерфейса', 'Interface language');
  FPrivacyLabel.Caption := TZaryaTr.Tr(
    'Профили и настройки хранятся локально. Диагностика по умолчанию не ' +
    'содержит credentials, raw config и runtime-логи.',
    'Profiles and settings are stored locally. Diagnostics exclude credentials, ' +
    'raw configuration, and runtime logs by default.');
  FContinueButton.Caption := TZaryaTr.Tr('Продолжить', 'Continue');
  FExitButton.Caption := TZaryaTr.Tr('Выход', 'Exit');
end;

procedure TZaryaFirstRunDialog.LanguageChanged(Sender: TObject);
begin
  case FLanguageCombo.ItemIndex of
    1: FSettings.Language := 'ru';
    2: FSettings.Language := 'en';
  else
    FSettings.Language := 'system';
  end;
  TZaryaTr.SetLanguage(FSettings.Language);
  Localize;
end;

procedure TZaryaFirstRunDialog.ContinueClick(Sender: TObject);
begin
  LanguageChanged(Sender);
  FSettings.FirstRunCompleted := True;
  ModalResult := mrOk;
end;

procedure TZaryaFirstRunDialog.ApplyTo(var ASettings: TZaryaAppSettings);
begin
  ASettings := FSettings;
end;

function RunFirstRunDialog(AOwner: TComponent;
  var ASettings: TZaryaAppSettings): Boolean;
var
  Dialog: TZaryaFirstRunDialog;
begin
  Dialog := TZaryaFirstRunDialog.Create(AOwner, ASettings);
  try
    Result := Dialog.ShowModal = mrOk;
    if Result then Dialog.ApplyTo(ASettings);
  finally
    Dialog.Free;
  end;
end;

end.
