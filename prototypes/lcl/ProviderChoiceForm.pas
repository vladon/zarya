unit ProviderChoiceForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, StdCtrls, ExtCtrls, Dialogs,
  ZaryaCoreProvider, ZaryaCoreProviderRegistry, ZaryaThemes;

type
  TProviderChoiceDialog = class(TForm)
  private
    FRegistry: TZaryaCoreProviderRegistry;
    FProviderIds: TZaryaStringArray;
    FList: TListBox;
    FSelectedProviderId: string;
    FSaveToProfile: Boolean;
    procedure OneTimeClick(Sender: TObject);
    procedure SaveClick(Sender: TObject);
    function CaptureSelection: Boolean;
  public
    constructor Create(AOwner: TComponent;
      const ARegistry: TZaryaCoreProviderRegistry;
      const AProviderIds: TZaryaStringArray; const APreferredProviderId: string;
      const ADarkTheme: Boolean); reintroduce;
    class function Execute(AOwner: TComponent;
      const ARegistry: TZaryaCoreProviderRegistry;
      const AProviderIds: TZaryaStringArray; const APreferredProviderId: string;
      const ADarkTheme: Boolean; out AProviderId: string;
      out ASaveToProfile: Boolean): Boolean;
  end;

implementation

uses
  ZaryaTr;

constructor TProviderChoiceDialog.Create(AOwner: TComponent;
  const ARegistry: TZaryaCoreProviderRegistry;
  const AProviderIds: TZaryaStringArray; const APreferredProviderId: string;
  const ADarkTheme: Boolean);
var
  I: Integer;
  Provider: TZaryaCoreProvider;
  Buttons: TPanel;
  OneTimeButton: TButton;
  SaveButton: TButton;
  CancelButton: TButton;
  Info: TLabel;
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  FRegistry := ARegistry;
  FProviderIds := Copy(AProviderIds);
  Caption := TZaryaTr.Tr('Выбор runtime provider', 'Choose runtime provider');
  Position := poOwnerFormCenter;
  BorderStyle := bsDialog;
  ClientWidth := 660;
  ClientHeight := 350;

  Info := TLabel.Create(Self);
  Info.Parent := Self;
  Info.WordWrap := True;
  Info.Caption := TZaryaTr.Tr('Предпочтительный provider «',
    'The preferred provider “') + APreferredProviderId +
    TZaryaTr.Tr('» недоступен или несовместим. Автоматическое переключение запрещено; выберите явное действие.',
      '” is unavailable or incompatible. Automatic switching is disabled; choose an explicit action.');
  Info.SetBounds(16, 14, 628, 52);

  FList := TListBox.Create(Self);
  FList.Parent := Self;
  FList.SetBounds(16, 74, 628, 196);
  for I := 0 to High(FProviderIds) do
    if FRegistry.TryGet(FProviderIds[I], Provider) then
      FList.Items.Add(Provider.DisplayName + '  [' + Provider.ProviderId + ']');
  if FList.Count > 0 then
    FList.ItemIndex := 0;

  Buttons := TPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.Height := 64;
  Buttons.BevelOuter := bvNone;
  OneTimeButton := TButton.Create(Self);
  OneTimeButton.Parent := Buttons;
  OneTimeButton.Caption := TZaryaTr.Tr('Использовать один раз', 'Use once');
  OneTimeButton.SetBounds(16, 16, 176, 32);
  OneTimeButton.OnClick := @OneTimeClick;
  SaveButton := TButton.Create(Self);
  SaveButton.Parent := Buttons;
  SaveButton.Caption := TZaryaTr.Tr('Сменить provider профиля',
    'Change profile provider');
  SaveButton.SetBounds(202, 16, 194, 32);
  SaveButton.OnClick := @SaveClick;
  CancelButton := TButton.Create(Self);
  CancelButton.Parent := Buttons;
  CancelButton.Caption := TZaryaTr.Tr('Отмена');
  CancelButton.ModalResult := mrCancel;
  CancelButton.SetBounds(532, 16, 112, 32);

  if ADarkTheme then Theme := ZaryaThemes.DarkTheme
  else Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

function TProviderChoiceDialog.CaptureSelection: Boolean;
begin
  Result := (FList.ItemIndex >= 0) and
    (FList.ItemIndex <= High(FProviderIds));
  if not Result then
  begin
    MessageDlg('Runtime provider', TZaryaTr.Tr('Выберите provider.',
      'Select a provider.'), mtInformation,
      [mbOK], 0);
    Exit;
  end;
  FSelectedProviderId := FProviderIds[FList.ItemIndex];
end;

procedure TProviderChoiceDialog.OneTimeClick(Sender: TObject);
begin
  if not CaptureSelection then Exit;
  FSaveToProfile := False;
  ModalResult := mrOk;
end;

procedure TProviderChoiceDialog.SaveClick(Sender: TObject);
begin
  if not CaptureSelection then Exit;
  FSaveToProfile := True;
  ModalResult := mrOk;
end;

class function TProviderChoiceDialog.Execute(AOwner: TComponent;
  const ARegistry: TZaryaCoreProviderRegistry;
  const AProviderIds: TZaryaStringArray; const APreferredProviderId: string;
  const ADarkTheme: Boolean; out AProviderId: string;
  out ASaveToProfile: Boolean): Boolean;
var
  Dialog: TProviderChoiceDialog;
begin
  AProviderId := '';
  ASaveToProfile := False;
  Dialog := TProviderChoiceDialog.Create(AOwner, ARegistry, AProviderIds,
    APreferredProviderId, ADarkTheme);
  try
    Result := Dialog.ShowModal = mrOk;
    if Result then
    begin
      AProviderId := Dialog.FSelectedProviderId;
      ASaveToProfile := Dialog.FSaveToProfile;
    end;
  finally
    Dialog.Free;
  end;
end;

end.
