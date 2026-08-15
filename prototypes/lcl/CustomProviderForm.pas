unit CustomProviderForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, StdCtrls, ComCtrls, ButtonPanel,
  Dialogs, ZaryaCoreProvider, ZaryaThemes;

type
  TCustomProviderDialog = class(TForm)
  private
    FProvider: TZaryaCoreProvider;
    FNameEdit: TEdit;
    FAdapterCombo: TComboBox;
    FFormatCombo: TComboBox;
    FProtocolsEdit: TEdit;
    FAssetEdit: TEdit;
    FVersionMemo: TMemo;
    FValidateMemo: TMemo;
    FRunMemo: TMemo;
    FReadinessCombo: TComboBox;
    FSystemProxyCheck: TCheckBox;
    FRoutingCheck: TCheckBox;
    FDnsCheck: TCheckBox;
    FTunCheck: TCheckBox;
    procedure AcceptClick(Sender: TObject);
    procedure LoadProvider;
    procedure StoreProvider;
    function ArgumentsFromMemo(AMemo: TMemo): TZaryaStringArray;
    procedure ArgumentsToMemo(AMemo: TMemo;
      const AArguments: TZaryaStringArray);
  public
    constructor Create(AOwner: TComponent;
      const AProvider: TZaryaCoreProvider;
      const ADarkTheme: Boolean); reintroduce;
    class function Execute(AOwner: TComponent;
      var AProvider: TZaryaCoreProvider;
      const ADarkTheme: Boolean): Boolean;
  end;

implementation

uses
  ZaryaRuntimeProcess, ZaryaTr;

procedure AddLabel(AOwner: TComponent; AParent: TWinControl;
  const ACaption: string; const ALeft, ATop, AWidth: Integer);
var
  LabelControl: TLabel;
begin
  LabelControl := TLabel.Create(AOwner);
  LabelControl.Parent := AParent;
  LabelControl.Caption := ACaption;
  LabelControl.SetBounds(ALeft, ATop, AWidth, 22);
end;

constructor TCustomProviderDialog.Create(AOwner: TComponent;
  const AProvider: TZaryaCoreProvider; const ADarkTheme: Boolean);
var
  Pages: TPageControl;
  GeneralTab: TTabSheet;
  CommandsTab: TTabSheet;
  Buttons: TButtonPanel;
  HintLabel: TLabel;
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  FProvider := AProvider;
  Caption := 'Custom provider — Zarya';
  Position := poOwnerFormCenter;
  BorderStyle := bsSizeable;
  ClientWidth := 760;
  ClientHeight := 610;
  Constraints.MinWidth := 680;
  Constraints.MinHeight := 520;

  Buttons := TButtonPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.ShowButtons := [pbOK, pbCancel];
  Buttons.ShowGlyphs := [];
  Buttons.OKButton.Caption := TZaryaTr.Tr('Сохранить');
  Buttons.OKButton.ModalResult := mrNone;
  Buttons.OKButton.OnClick := @AcceptClick;
  Buttons.CancelButton.Caption := TZaryaTr.Tr('Отмена');

  Pages := TPageControl.Create(Self);
  Pages.Parent := Self;
  Pages.Align := alClient;
  Pages.BorderSpacing.Around := 12;

  GeneralTab := TTabSheet.Create(Self);
  GeneralTab.PageControl := Pages;
  GeneralTab.Caption := TZaryaTr.Tr('Основное', 'General');
  AddLabel(Self, GeneralTab, TZaryaTr.Tr('Название', 'Name'), 20, 24, 190);
  FNameEdit := TEdit.Create(Self);
  FNameEdit.Parent := GeneralTab;
  FNameEdit.SetBounds(220, 20, 450, 30);
  AddLabel(Self, GeneralTab, 'Adapter ID', 20, 68, 190);
  FAdapterCombo := TComboBox.Create(Self);
  FAdapterCombo.Parent := GeneralTab;
  FAdapterCombo.Style := csDropDownList;
  FAdapterCombo.Items.Add('xray');
  FAdapterCombo.Items.Add('v2ray');
  FAdapterCombo.Items.Add('sing-box');
  FAdapterCombo.Items.Add('mihomo');
  FAdapterCombo.Items.Add('hysteria2');
  FAdapterCombo.Items.Add('raw');
  FAdapterCombo.SetBounds(220, 64, 220, 30);
  AddLabel(Self, GeneralTab, TZaryaTr.Tr('Формат config', 'Config format'),
    20, 112, 190);
  FFormatCombo := TComboBox.Create(Self);
  FFormatCombo.Parent := GeneralTab;
  FFormatCombo.Style := csDropDownList;
  FFormatCombo.Items.Add('xray-json');
  FFormatCombo.Items.Add('v2ray-json');
  FFormatCombo.Items.Add('sing-box-json');
  FFormatCombo.Items.Add('mihomo-yaml');
  FFormatCombo.Items.Add('hysteria-yaml');
  FFormatCombo.Items.Add('raw');
  FFormatCombo.SetBounds(220, 108, 220, 30);
  AddLabel(Self, GeneralTab, TZaryaTr.Tr('Протоколы CSV / *',
    'Protocols CSV / *'), 20, 156, 190);
  FProtocolsEdit := TEdit.Create(Self);
  FProtocolsEdit.Parent := GeneralTab;
  FProtocolsEdit.SetBounds(220, 152, 450, 30);
  AddLabel(Self, GeneralTab, TZaryaTr.Tr('Каталог ресурсов',
    'Asset directory'), 20, 200, 190);
  FAssetEdit := TEdit.Create(Self);
  FAssetEdit.Parent := GeneralTab;
  FAssetEdit.SetBounds(220, 196, 450, 30);
  AddLabel(Self, GeneralTab, 'Readiness', 20, 244, 190);
  FReadinessCombo := TComboBox.Create(Self);
  FReadinessCombo.Parent := GeneralTab;
  FReadinessCombo.Style := csDropDownList;
  FReadinessCombo.Items.Add('mixed-tcp');
  FReadinessCombo.Items.Add('http-tcp');
  FReadinessCombo.Items.Add('socks-tcp');
  FReadinessCombo.Items.Add('custom-tcp');
  FReadinessCombo.SetBounds(220, 240, 220, 30);
  FSystemProxyCheck := TCheckBox.Create(Self);
  FSystemProxyCheck.Parent := GeneralTab;
  FSystemProxyCheck.Caption := 'System proxy';
  FSystemProxyCheck.SetBounds(220, 292, 150, 26);
  FRoutingCheck := TCheckBox.Create(Self);
  FRoutingCheck.Parent := GeneralTab;
  FRoutingCheck.Caption := 'Routing';
  FRoutingCheck.SetBounds(380, 292, 120, 26);
  FDnsCheck := TCheckBox.Create(Self);
  FDnsCheck.Parent := GeneralTab;
  FDnsCheck.Caption := 'DNS';
  FDnsCheck.SetBounds(510, 292, 90, 26);
  FTunCheck := TCheckBox.Create(Self);
  FTunCheck.Parent := GeneralTab;
  FTunCheck.Caption := 'TUN (gated)';
  FTunCheck.SetBounds(220, 328, 160, 26);

  CommandsTab := TTabSheet.Create(Self);
  CommandsTab.PageControl := Pages;
  CommandsTab.Caption := TZaryaTr.Tr('Аргументы', 'Arguments');
  HintLabel := TLabel.Create(Self);
  HintLabel.Parent := CommandsTab;
  HintLabel.WordWrap := True;
  HintLabel.Caption := TZaryaTr.Tr(
    'Одна строка — один аргумент. Shell не используется. ' +
    'Разрешены placeholders: {config}, {dataDir}, {assetDir}, {mixedPort}, ' +
    '{httpPort}, {socksPort}, {logLevel}.',
    'One line per argument. No shell is used. Allowed placeholders: ' +
    '{config}, {dataDir}, {assetDir}, {mixedPort}, {httpPort}, {socksPort}, ' +
    '{logLevel}.');
  HintLabel.SetBounds(16, 12, 690, 44);
  AddLabel(Self, CommandsTab, 'Version / help', 16, 66, 150);
  FVersionMemo := TMemo.Create(Self);
  FVersionMemo.Parent := CommandsTab;
  FVersionMemo.SetBounds(176, 62, 520, 90);
  FVersionMemo.ScrollBars := ssAutoVertical;
  AddLabel(Self, CommandsTab, 'Validation', 16, 166, 150);
  FValidateMemo := TMemo.Create(Self);
  FValidateMemo.Parent := CommandsTab;
  FValidateMemo.SetBounds(176, 162, 520, 110);
  FValidateMemo.ScrollBars := ssAutoVertical;
  AddLabel(Self, CommandsTab, 'Run', 16, 286, 150);
  FRunMemo := TMemo.Create(Self);
  FRunMemo.Parent := CommandsTab;
  FRunMemo.SetBounds(176, 282, 520, 150);
  FRunMemo.ScrollBars := ssAutoVertical;

  LoadProvider;
  if ADarkTheme then Theme := ZaryaThemes.DarkTheme
  else Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

procedure TCustomProviderDialog.ArgumentsToMemo(AMemo: TMemo;
  const AArguments: TZaryaStringArray);
var
  I: Integer;
begin
  AMemo.Clear;
  for I := 0 to High(AArguments) do
    AMemo.Lines.Add(AArguments[I]);
end;

function TCustomProviderDialog.ArgumentsFromMemo(
  AMemo: TMemo): TZaryaStringArray;
var
  I: Integer;
  CountFound: Integer;
begin
  Result := nil;
  CountFound := 0;
  for I := 0 to AMemo.Lines.Count - 1 do
  begin
    if AMemo.Lines[I] = '' then Continue;
    SetLength(Result, CountFound + 1);
    Result[CountFound] := AMemo.Lines[I];
    Inc(CountFound);
  end;
end;

procedure TCustomProviderDialog.LoadProvider;
begin
  FNameEdit.Text := FProvider.DisplayName;
  FAdapterCombo.ItemIndex := FAdapterCombo.Items.IndexOf(FProvider.AdapterId);
  if FAdapterCombo.ItemIndex < 0 then FAdapterCombo.ItemIndex := 5;
  FFormatCombo.ItemIndex := FFormatCombo.Items.IndexOf(
    ConfigFormatToString(FProvider.ConfigFormat));
  if FFormatCombo.ItemIndex < 0 then FFormatCombo.ItemIndex := 5;
  FProtocolsEdit.Text := FProvider.SupportedProtocols;
  FAssetEdit.Text := FProvider.AssetDirectory;
  FReadinessCombo.ItemIndex := FReadinessCombo.Items.IndexOf(
    ReadinessKindToString(FProvider.ReadinessKind));
  if FReadinessCombo.ItemIndex < 0 then FReadinessCombo.ItemIndex := 0;
  FSystemProxyCheck.Checked := FProvider.SupportsSystemProxy;
  FRoutingCheck.Checked := FProvider.SupportsRouting;
  FDnsCheck.Checked := FProvider.SupportsDns;
  FTunCheck.Checked := FProvider.SupportsTun;
  ArgumentsToMemo(FVersionMemo, FProvider.VersionArguments);
  ArgumentsToMemo(FValidateMemo, FProvider.ValidateArguments);
  ArgumentsToMemo(FRunMemo, FProvider.RunArguments);
end;

procedure TCustomProviderDialog.StoreProvider;
begin
  FProvider.DisplayName := Trim(FNameEdit.Text);
  FProvider.AdapterId := FAdapterCombo.Text;
  FProvider.ConfigFormat := ConfigFormatFromString(FFormatCombo.Text);
  case FProvider.ConfigFormat of
    cfXrayJson, cfV2RayJson, cfSingBoxJson: FProvider.ConfigExtension := '.json';
    cfMihomoYaml, cfHysteriaYaml: FProvider.ConfigExtension := '.yaml';
  else
    FProvider.ConfigExtension := '.conf';
  end;
  FProvider.SupportedProtocols := Trim(FProtocolsEdit.Text);
  FProvider.AssetDirectory := Trim(FAssetEdit.Text);
  FProvider.ReadinessKind := ReadinessKindFromString(FReadinessCombo.Text);
  FProvider.SupportsSystemProxy := FSystemProxyCheck.Checked;
  FProvider.SupportsRouting := FRoutingCheck.Checked;
  FProvider.SupportsDns := FDnsCheck.Checked;
  FProvider.SupportsTun := FTunCheck.Checked;
  FProvider.VersionArguments := ArgumentsFromMemo(FVersionMemo);
  FProvider.ValidateArguments := ArgumentsFromMemo(FValidateMemo);
  FProvider.RunArguments := ArgumentsFromMemo(FRunMemo);
end;

procedure TCustomProviderDialog.AcceptClick(Sender: TObject);
var
  Context: TZaryaProcessContext;
  Expanded: TZaryaStringArray;
  ErrorMessage: string;
begin
  StoreProvider;
  if FProvider.DisplayName = '' then
  begin
    MessageDlg('Custom provider', TZaryaTr.Tr('Введите название.',
      'Enter a name.'), mtWarning, [mbOK], 0);
    Exit;
  end;
  if FProvider.SupportedProtocols = '' then
  begin
    MessageDlg('Custom provider', TZaryaTr.Tr('Укажите протоколы или *.',
      'Specify protocols or *.'), mtWarning,
      [mbOK], 0);
    Exit;
  end;
  if Length(FProvider.RunArguments) = 0 then
  begin
    MessageDlg('Custom provider', TZaryaTr.Tr(
      'Команда Run не может быть пустой.', 'The Run command cannot be empty.'),
      mtWarning, [mbOK], 0);
    Exit;
  end;
  Context.ConfigPath := 'config';
  Context.DataDirectory := 'data';
  Context.AssetDirectory := 'assets';
  Context.MixedPort := 10000;
  Context.HttpPort := 10001;
  Context.SocksPort := 10002;
  Context.LogLevel := 'warning';
  if not ExpandProviderArguments(FProvider.VersionArguments, Context,
    Expanded, ErrorMessage) or
    not ExpandProviderArguments(FProvider.ValidateArguments, Context,
    Expanded, ErrorMessage) or
    not ExpandProviderArguments(FProvider.RunArguments, Context,
    Expanded, ErrorMessage) then
  begin
    MessageDlg('Custom provider', ErrorMessage, mtWarning, [mbOK], 0);
    Exit;
  end;
  ModalResult := mrOk;
end;

class function TCustomProviderDialog.Execute(AOwner: TComponent;
  var AProvider: TZaryaCoreProvider; const ADarkTheme: Boolean): Boolean;
var
  Dialog: TCustomProviderDialog;
begin
  Dialog := TCustomProviderDialog.Create(AOwner, AProvider, ADarkTheme);
  try
    Result := Dialog.ShowModal = mrOk;
    if Result then
      AProvider := Dialog.FProvider;
  finally
    Dialog.Free;
  end;
end;

end.
