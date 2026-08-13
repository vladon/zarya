unit SettingsForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ComCtrls, ZaryaThemes;

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
    procedure AddLabel(AParent: TWinControl; const ACaption: string;
      const ALeft, ATop, AWidth: Integer);
  public
    constructor Create(AOwner: TComponent; const ADarkTheme,
      AMinimizeToTray: Boolean); reintroduce;
    function DarkThemeSelected: Boolean;
    function MinimizeToTraySelected: Boolean;
  end;

implementation

constructor TSettingsDialog.Create(AOwner: TComponent; const ADarkTheme,
  AMinimizeToTray: Boolean);
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
  BottomPanel: TPanel;
  OkButton: TButton;
  CancelButton: TButton;
  HintLabel: TLabel;
begin
  BottomPanel := TPanel.Create(Self);
  BottomPanel.Parent := Self;
  BottomPanel.Align := alBottom;
  BottomPanel.Height := 58;
  BottomPanel.BevelOuter := bvNone;

  CancelButton := TButton.Create(Self);
  CancelButton.Parent := BottomPanel;
  CancelButton.Caption := 'Отмена';
  CancelButton.ModalResult := mrCancel;
  CancelButton.SetBounds(340, 13, 88, 32);

  OkButton := TButton.Create(Self);
  OkButton.Parent := BottomPanel;
  OkButton.Caption := 'Применить';
  OkButton.ModalResult := mrOk;
  OkButton.Default := True;
  OkButton.SetBounds(434, 13, 92, 32);

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

function TSettingsDialog.DarkThemeSelected: Boolean;
begin
  Result := FThemeCombo.ItemIndex = 1;
end;

function TSettingsDialog.MinimizeToTraySelected: Boolean;
begin
  Result := FMinimizeToTrayCheck.Checked;
end;

end.
