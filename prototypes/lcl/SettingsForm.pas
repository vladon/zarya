unit SettingsForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ComCtrls, ButtonPanel, ZaryaThemes;

type
  TSettingsDialog = class(TForm)
  private
    FThemeCombo: TComboBox;
    FLanguageCombo: TComboBox;
    FPortEdit: TEdit;
    FAutoProxyCheck: TCheckBox;
    FRestoreProxyCheck: TCheckBox;
    FMinimizeToTrayCheck: TCheckBox;
    procedure BuildInterface;
    procedure AcceptClick(Sender: TObject);
    procedure AddLabel(AParent: TWinControl; const ACaption: string;
      const ALeft, ATop, AWidth: Integer);
  public
    constructor Create(AOwner: TComponent; const ADarkTheme,
      AMinimizeToTray: Boolean; const AMixedPort: Integer;
      const AAutoEnableSystemProxy, ARestoreSystemProxy: Boolean); reintroduce;
    function DarkThemeSelected: Boolean;
    function MinimizeToTraySelected: Boolean;
    function MixedPortSelected: Integer;
    function AutoEnableSystemProxySelected: Boolean;
    function RestoreSystemProxySelected: Boolean;
  end;

implementation

constructor TSettingsDialog.Create(AOwner: TComponent; const ADarkTheme,
  AMinimizeToTray: Boolean; const AMixedPort: Integer;
  const AAutoEnableSystemProxy, ARestoreSystemProxy: Boolean);
var
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  Caption := 'Настройки — LCL-прототип';
  BorderStyle := bsDialog;
  Position := poOwnerFormCenter;
  ClientWidth := 540;
  ClientHeight := 430;
  Constraints.MinWidth := 500;
  Constraints.MinHeight := 400;
  BuildInterface;
  FThemeCombo.ItemIndex := Ord(ADarkTheme);
  FMinimizeToTrayCheck.Checked := AMinimizeToTray;
  FPortEdit.Text := IntToStr(AMixedPort);
  FAutoProxyCheck.Checked := AAutoEnableSystemProxy;
  FRestoreProxyCheck.Checked := ARestoreSystemProxy;
  if ADarkTheme then
    Theme := ZaryaThemes.DarkTheme
  else
    Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

procedure TSettingsDialog.AddLabel(AParent: TWinControl; const ACaption: string;
  const ALeft, ATop, AWidth: Integer);
var
  TextLabel: TLabel;
begin
  TextLabel := TLabel.Create(Self);
  TextLabel.Parent := AParent;
  TextLabel.Caption := ACaption;
  TextLabel.Left := ALeft;
  TextLabel.Top := ATop;
  TextLabel.Width := AWidth;
end;

procedure TSettingsDialog.BuildInterface;
var
  Pages: TPageControl;
  GeneralTab: TTabSheet;
  ProxyTab: TTabSheet;
  Buttons: TButtonPanel;
  HintLabel: TLabel;
begin
  Buttons := TButtonPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.ShowButtons := [pbOK, pbCancel];
  Buttons.ShowGlyphs := [];
  Buttons.OKButton.Caption := 'Применить';
  Buttons.OKButton.ModalResult := mrNone;
  Buttons.OKButton.OnClick := @AcceptClick;
  Buttons.CancelButton.Caption := 'Отмена';

  Pages := TPageControl.Create(Self);
  Pages.Parent := Self;
  Pages.Align := alClient;
  Pages.BorderSpacing.Around := 12;

  GeneralTab := TTabSheet.Create(Self);
  GeneralTab.PageControl := Pages;
  GeneralTab.Caption := 'Интерфейс';

  AddLabel(GeneralTab, 'Тема', 24, 28, 180);
  FThemeCombo := TComboBox.Create(Self);
  FThemeCombo.Parent := GeneralTab;
  FThemeCombo.Style := csDropDownList;
  FThemeCombo.Items.Add('Светлая');
  FThemeCombo.Items.Add('Тёмная');
  FThemeCombo.SetBounds(220, 23, 250, 30);

  AddLabel(GeneralTab, 'Язык', 24, 76, 180);
  FLanguageCombo := TComboBox.Create(Self);
  FLanguageCombo.Parent := GeneralTab;
  FLanguageCombo.Style := csDropDownList;
  FLanguageCombo.Items.Add('Русский');
  FLanguageCombo.Items.Add('English');
  FLanguageCombo.ItemIndex := 0;
  FLanguageCombo.SetBounds(220, 71, 250, 30);

  FMinimizeToTrayCheck := TCheckBox.Create(Self);
  FMinimizeToTrayCheck.Parent := GeneralTab;
  FMinimizeToTrayCheck.Caption := 'Сворачивать в tray при закрытии окна';
  FMinimizeToTrayCheck.SetBounds(24, 130, 420, 26);

  HintLabel := TLabel.Create(Self);
  HintLabel.Parent := GeneralTab;
  HintLabel.Caption :=
    'Переключение темы применяется к стандартным LCL-контролам. ' +
    'Это позволяет сразу увидеть ограничения Win32 widgetset.';
  HintLabel.AutoSize := False;
  HintLabel.WordWrap := True;
  HintLabel.Font.Color := clGrayText;
  HintLabel.SetBounds(24, 184, 450, 60);

  ProxyTab := TTabSheet.Create(Self);
  ProxyTab.PageControl := Pages;
  ProxyTab.Caption := 'Прокси';

  AddLabel(ProxyTab, 'Локальный mixed-порт', 24, 28, 180);
  FPortEdit := TEdit.Create(Self);
  FPortEdit.Parent := ProxyTab;
  FPortEdit.Text := '10808';
  FPortEdit.NumbersOnly := True;
  FPortEdit.SetBounds(220, 23, 120, 30);

  FAutoProxyCheck := TCheckBox.Create(Self);
  FAutoProxyCheck.Parent := ProxyTab;
  FAutoProxyCheck.Caption := 'Включать системный прокси после готовности порта';
  FAutoProxyCheck.Checked := True;
  FAutoProxyCheck.SetBounds(24, 80, 450, 26);

  FRestoreProxyCheck := TCheckBox.Create(Self);
  FRestoreProxyCheck.Parent := ProxyTab;
  FRestoreProxyCheck.Caption := 'Восстанавливать системный прокси при остановке';
  FRestoreProxyCheck.Checked := True;
  FRestoreProxyCheck.SetBounds(24, 116, 450, 26);

  HintLabel := TLabel.Create(Self);
  HintLabel.Parent := ProxyTab;
  HintLabel.Caption :=
    'Прототип ничего не записывает в системные настройки и не запускает Xray.';
  HintLabel.AutoSize := False;
  HintLabel.WordWrap := True;
  HintLabel.Font.Color := clGrayText;
  HintLabel.SetBounds(24, 174, 450, 45);
end;

procedure TSettingsDialog.AcceptClick(Sender: TObject);
var
  Port: Integer;
begin
  Port := MixedPortSelected;
  if (Port < 1) or (Port > 65535) then
  begin
    MessageDlg('Настройки', 'Mixed-порт должен быть в диапазоне от 1 до 65535.',
      mtWarning, [mbOK], 0);
    Exit;
  end;
  ModalResult := mrOk;
end;

function TSettingsDialog.DarkThemeSelected: Boolean;
begin
  Result := FThemeCombo.ItemIndex = 1;
end;

function TSettingsDialog.MinimizeToTraySelected: Boolean;
begin
  Result := FMinimizeToTrayCheck.Checked;
end;

function TSettingsDialog.MixedPortSelected: Integer;
begin
  Result := StrToIntDef(FPortEdit.Text, 0);
end;

function TSettingsDialog.AutoEnableSystemProxySelected: Boolean;
begin
  Result := FAutoProxyCheck.Checked;
end;

function TSettingsDialog.RestoreSystemProxySelected: Boolean;
begin
  Result := FRestoreProxyCheck.Checked;
end;

end.
