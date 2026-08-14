unit ProfileForm;

{$mode objfpc}{$H+}

{$IFNDEF ZARYA_LEGACY_PROFILE_FORM}

interface

uses
  VlessProfileForm;

type
  TProfileDialog = TVlessProfileDialog;

implementation

end.

{$ELSE}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ButtonPanel, ZaryaProfile, ZaryaThemes;

type
  TProfileDialog = class(TForm)
  private
    FNameEdit: TEdit;
    FProtocolCombo: TComboBox;
    FHostEdit: TEdit;
    FPortEdit: TEdit;
    FUuidEdit: TEdit;
    FSourceEdit: TEdit;
    FEnabledCheck: TCheckBox;
    FProfile: TZaryaProfile;
    procedure BuildInterface;
    procedure AddLabel(const ACaption: string; const ATop: Integer);
    procedure AcceptClick(Sender: TObject);
    procedure LoadProfile;
    procedure StoreProfile;
  public
    constructor Create(AOwner: TComponent; const AProfile: TZaryaProfile;
      const ADarkTheme: Boolean); reintroduce;
    class function Execute(AOwner: TComponent; var AProfile: TZaryaProfile;
      const ADarkTheme: Boolean): Boolean;
  end;

implementation

constructor TProfileDialog.Create(AOwner: TComponent; const AProfile: TZaryaProfile;
  const ADarkTheme: Boolean);
var
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  FProfile := AProfile;
  Caption := 'Профиль — Zarya LCL';
  BorderStyle := bsDialog;
  Position := poOwnerFormCenter;
  ClientWidth := 520;
  ClientHeight := 420;
  Constraints.MinWidth := 500;
  Constraints.MinHeight := 410;
  BuildInterface;
  LoadProfile;
  if ADarkTheme then
    Theme := ZaryaThemes.DarkTheme
  else
    Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

procedure TProfileDialog.AddLabel(const ACaption: string; const ATop: Integer);
var
  TextLabel: TLabel;
begin
  TextLabel := TLabel.Create(Self);
  TextLabel.Parent := Self;
  TextLabel.Caption := ACaption;
  TextLabel.SetBounds(24, ATop + 5, 160, 24);
end;

procedure TProfileDialog.BuildInterface;
var
  Buttons: TButtonPanel;
begin
  Buttons := TButtonPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.ShowButtons := [pbOK, pbCancel];
  Buttons.ShowGlyphs := [];
  Buttons.OKButton.Caption := 'Сохранить';
  Buttons.OKButton.ModalResult := mrNone;
  Buttons.OKButton.OnClick := @AcceptClick;
  Buttons.CancelButton.Caption := 'Отмена';

  AddLabel('Название', 24);
  FNameEdit := TEdit.Create(Self);
  FNameEdit.Parent := Self;
  FNameEdit.SetBounds(190, 24, 294, 30);

  AddLabel('Протокол', 70);
  FProtocolCombo := TComboBox.Create(Self);
  FProtocolCombo.Parent := Self;
  FProtocolCombo.Style := csDropDownList;
  FProtocolCombo.Items.Add('VLESS');
  FProtocolCombo.Items.Add('VMess');
  FProtocolCombo.Items.Add('Trojan');
  FProtocolCombo.Items.Add('Shadowsocks');
  FProtocolCombo.Items.Add('SOCKS');
  FProtocolCombo.SetBounds(190, 70, 180, 30);

  AddLabel('Сервер', 116);
  FHostEdit := TEdit.Create(Self);
  FHostEdit.Parent := Self;
  FHostEdit.SetBounds(190, 116, 294, 30);

  AddLabel('Порт', 162);
  FPortEdit := TEdit.Create(Self);
  FPortEdit.Parent := Self;
  FPortEdit.NumbersOnly := True;
  FPortEdit.SetBounds(190, 162, 120, 30);

  AddLabel('UUID (для VLESS)', 208);
  FUuidEdit := TEdit.Create(Self);
  FUuidEdit.Parent := Self;
  FUuidEdit.SetBounds(190, 208, 294, 30);

  AddLabel('Источник', 254);
  FSourceEdit := TEdit.Create(Self);
  FSourceEdit.Parent := Self;
  FSourceEdit.SetBounds(190, 254, 294, 30);

  FEnabledCheck := TCheckBox.Create(Self);
  FEnabledCheck.Parent := Self;
  FEnabledCheck.Caption := 'Профиль включён';
  FEnabledCheck.SetBounds(190, 298, 260, 26);
end;

procedure TProfileDialog.LoadProfile;
var
  Index: Integer;
begin
  FNameEdit.Text := FProfile.Name;
  Index := FProtocolCombo.Items.IndexOf(FProfile.ProtocolName);
  if Index < 0 then
    Index := 0;
  FProtocolCombo.ItemIndex := Index;
  FHostEdit.Text := FProfile.Host;
  FPortEdit.Text := IntToStr(FProfile.Port);
  FUuidEdit.Text := FProfile.Uuid;
  FSourceEdit.Text := FProfile.Source;
  FEnabledCheck.Checked := FProfile.Enabled;
end;

procedure TProfileDialog.StoreProfile;
begin
  FProfile.Name := Trim(FNameEdit.Text);
  FProfile.ProtocolName := FProtocolCombo.Text;
  FProfile.Host := Trim(FHostEdit.Text);
  FProfile.Port := StrToIntDef(FPortEdit.Text, 0);
  FProfile.Uuid := Trim(FUuidEdit.Text);
  FProfile.Source := Trim(FSourceEdit.Text);
  if FProfile.Source = '' then
    FProfile.Source := 'Вручную';
  FProfile.Enabled := FEnabledCheck.Checked;
end;

procedure TProfileDialog.AcceptClick(Sender: TObject);
var
  ErrorMessage: string;
begin
  StoreProfile;
  if not ValidateProfile(FProfile, ErrorMessage) then
  begin
    MessageDlg('Профиль', ErrorMessage, mtWarning, [mbOK], 0);
    Exit;
  end;
  ModalResult := mrOk;
end;

class function TProfileDialog.Execute(AOwner: TComponent;
  var AProfile: TZaryaProfile; const ADarkTheme: Boolean): Boolean;
var
  Dialog: TProfileDialog;
begin
  Dialog := TProfileDialog.Create(AOwner, AProfile, ADarkTheme);
  try
    Result := Dialog.ShowModal = mrOk;
    if Result then
      AProfile := Dialog.FProfile;
  finally
    Dialog.Free;
  end;
end;

end.

{$ENDIF}
