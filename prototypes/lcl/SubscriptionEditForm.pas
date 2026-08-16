unit SubscriptionEditForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, StdCtrls, Dialogs, ZaryaSubscription;

type
  TSubscriptionEditDialog = class(TForm)
  private
    FNameEdit: TEdit;
    FUrlEdit: TEdit;
    FEnabledCheck: TCheckBox;
    FUserAgentEdit: TEdit;
    FRemarksMemo: TMemo;
    procedure SaveClick(Sender: TObject);
    procedure AddLabel(const ACaption: string; const ATop: Integer);
  public
    constructor Create(AOwner: TComponent); override;
    class function Edit(AOwner: TComponent;
      var ASubscription: TZaryaSubscription): Boolean;
  end;

implementation

uses
  ZaryaTr;

constructor TSubscriptionEditDialog.Create(AOwner: TComponent);
var
  SaveButton: TButton;
  CancelButton: TButton;
begin
  inherited CreateNew(AOwner, 1);
  Caption := TZaryaTr.Tr('Подписка', 'Subscription');
  Position := poOwnerFormCenter;
  BorderStyle := bsDialog;
  ClientWidth := 620;
  ClientHeight := 390;
  Constraints.MinWidth := 520;
  AddLabel(TZaryaTr.Tr('Название', 'Name'), 18);
  FNameEdit := TEdit.Create(Self);
  FNameEdit.Parent := Self;
  FNameEdit.SetBounds(20, 42, 580, 28);
  AddLabel('HTTPS/HTTP URL', 82);
  FUrlEdit := TEdit.Create(Self);
  FUrlEdit.Parent := Self;
  FUrlEdit.SetBounds(20, 106, 580, 28);
  FEnabledCheck := TCheckBox.Create(Self);
  FEnabledCheck.Parent := Self;
  FEnabledCheck.Caption := TZaryaTr.Tr('Подписка включена',
    'Subscription enabled');
  FEnabledCheck.SetBounds(20, 146, 220, 28);
  AddLabel(TZaryaTr.Tr('User-Agent (необязательно)',
    'User-Agent (optional)'), 184);
  FUserAgentEdit := TEdit.Create(Self);
  FUserAgentEdit.Parent := Self;
  FUserAgentEdit.SetBounds(20, 208, 580, 28);
  AddLabel(TZaryaTr.Tr('Примечание', 'Notes'), 248);
  FRemarksMemo := TMemo.Create(Self);
  FRemarksMemo.Parent := Self;
  FRemarksMemo.ScrollBars := ssVertical;
  FRemarksMemo.SetBounds(20, 272, 580, 58);
  SaveButton := TButton.Create(Self);
  SaveButton.Parent := Self;
  SaveButton.Caption := TZaryaTr.Tr('Сохранить');
  SaveButton.Default := True;
  SaveButton.OnClick := @SaveClick;
  SaveButton.SetBounds(390, 344, 100, 32);
  CancelButton := TButton.Create(Self);
  CancelButton.Parent := Self;
  CancelButton.Caption := TZaryaTr.Tr('Отмена');
  CancelButton.Cancel := True;
  CancelButton.ModalResult := mrCancel;
  CancelButton.SetBounds(500, 344, 100, 32);
end;

procedure TSubscriptionEditDialog.AddLabel(const ACaption: string;
  const ATop: Integer);
var
  LabelControl: TLabel;
begin
  LabelControl := TLabel.Create(Self);
  LabelControl.Parent := Self;
  LabelControl.Caption := ACaption;
  LabelControl.SetBounds(20, ATop, 580, 20);
end;

procedure TSubscriptionEditDialog.SaveClick(Sender: TObject);
var
  Url: string;
begin
  if Trim(FNameEdit.Text) = '' then
  begin
    MessageDlg(TZaryaTr.Tr('Подписка', 'Subscription'), TZaryaTr.Tr(
      'Введите название.', 'Enter a name.'), mtWarning, [mbOK], 0);
    FNameEdit.SetFocus;
    Exit;
  end;
  Url := LowerCase(Trim(FUrlEdit.Text));
  if (Pos('https://', Url) <> 1) and (Pos('http://', Url) <> 1) then
  begin
    MessageDlg(TZaryaTr.Tr('Подписка', 'Subscription'), TZaryaTr.Tr(
      'URL должен начинаться с https:// или http://.',
      'The URL must start with https:// or http://.'),
      mtWarning, [mbOK], 0);
    FUrlEdit.SetFocus;
    Exit;
  end;
  ModalResult := mrOk;
end;

class function TSubscriptionEditDialog.Edit(AOwner: TComponent;
  var ASubscription: TZaryaSubscription): Boolean;
var
  Dialog: TSubscriptionEditDialog;
begin
  Dialog := TSubscriptionEditDialog.Create(AOwner);
  try
    Dialog.FNameEdit.Text := ASubscription.Name;
    Dialog.FUrlEdit.Text := ASubscription.Url;
    Dialog.FEnabledCheck.Checked := ASubscription.Enabled;
    Dialog.FUserAgentEdit.Text := ASubscription.UserAgent;
    Dialog.FRemarksMemo.Text := ASubscription.Remarks;
    Result := Dialog.ShowModal = mrOk;
    if Result then
    begin
      ASubscription.Name := Trim(Dialog.FNameEdit.Text);
      ASubscription.Url := Trim(Dialog.FUrlEdit.Text);
      ASubscription.Enabled := Dialog.FEnabledCheck.Checked;
      ASubscription.UserAgent := Trim(Dialog.FUserAgentEdit.Text);
      ASubscription.Remarks := Trim(Dialog.FRemarksMemo.Text);
      if not ASubscription.Enabled then
      begin
        ASubscription.LastStatus := ssDisabled;
        ASubscription.LastError := '';
      end
      else if ASubscription.LastStatus = ssDisabled then
        ASubscription.LastStatus := ssNeverUpdated;
    end;
  finally
    Dialog.Free;
  end;
end;

end.
