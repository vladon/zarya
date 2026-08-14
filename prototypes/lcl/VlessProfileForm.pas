unit VlessProfileForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, StdCtrls, ComCtrls,
  ButtonPanel, ZaryaProfile, ZaryaThemes;

type
  TVlessProfileDialog = class(TForm)
  private
    FNameEdit: TEdit;
    FProtocolCombo: TComboBox;
    FHostEdit: TEdit;
    FPortEdit: TEdit;
    FUuidEdit: TEdit;
    FPasswordEdit: TEdit;
    FEncryptionEdit: TEdit;
    FMethodEdit: TEdit;
    FSecurityCipherEdit: TEdit;
    FAlterIdEdit: TEdit;
    FFlowEdit: TEdit;
    FObfsEdit: TEdit;
    FObfsPasswordEdit: TEdit;
    FLocalAddressEdit: TEdit;
    FAllowedIpsEdit: TEdit;
    FPreSharedKeyEdit: TEdit;
    FReservedEdit: TEdit;
    FMtuEdit: TEdit;
    FKeepAliveEdit: TEdit;
    FSourceEdit: TEdit;
    FEnabledCheck: TCheckBox;
    FProviderCombo: TComboBox;
    FNetworkCombo: TComboBox;
    FTransportHostEdit: TEdit;
    FPathEdit: TEdit;
    FHeaderTypeEdit: TEdit;
    FServiceNameEdit: TEdit;
    FSecurityCombo: TComboBox;
    FServerNameEdit: TEdit;
    FFingerprintEdit: TEdit;
    FPublicKeyEdit: TEdit;
    FShortIdEdit: TEdit;
    FSpiderXEdit: TEdit;
    FAlpnEdit: TEdit;
    FAllowInsecureCheck: TCheckBox;
    FRawFormatCombo: TComboBox;
    FRawConfigMemo: TMemo;
    FReadinessHostEdit: TEdit;
    FReadinessPortEdit: TEdit;
    FSystemProxyKindCombo: TComboBox;
    FProfile: TZaryaProfile;
    procedure BuildInterface;
    procedure AddLabel(AParent: TWinControl; const ACaption: string;
      const ATop: Integer);
    function AddEdit(AParent: TWinControl; const ATop: Integer;
      const AWidth: Integer = 430): TEdit;
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

uses
  ZaryaCoreProvider;

constructor TVlessProfileDialog.Create(AOwner: TComponent;
  const AProfile: TZaryaProfile; const ADarkTheme: Boolean);
var
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  FProfile := AProfile;
  Caption := 'Профиль — Zarya LCL';
  BorderStyle := bsSizeable;
  Position := poOwnerFormCenter;
  ClientWidth := 720;
  ClientHeight := 590;
  Constraints.MinWidth := 620;
  Constraints.MinHeight := 500;
  BuildInterface;
  LoadProfile;
  if ADarkTheme then
    Theme := ZaryaThemes.DarkTheme
  else
    Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

procedure TVlessProfileDialog.AddLabel(AParent: TWinControl;
  const ACaption: string; const ATop: Integer);
var
  TextLabel: TLabel;
begin
  TextLabel := TLabel.Create(Self);
  TextLabel.Parent := AParent;
  TextLabel.Caption := ACaption;
  TextLabel.SetBounds(20, ATop + 5, 190, 24);
end;

function TVlessProfileDialog.AddEdit(AParent: TWinControl;
  const ATop, AWidth: Integer): TEdit;
begin
  Result := TEdit.Create(Self);
  Result.Parent := AParent;
  Result.SetBounds(220, ATop, AWidth, 30);
  Result.Anchors := [akLeft, akTop, akRight];
end;

procedure SelectComboValue(ACombo: TComboBox; const AValue: string;
  const ADefaultIndex: Integer);
var
  Index: Integer;
begin
  Index := ACombo.Items.IndexOf(LowerCase(Trim(AValue)));
  if Index < 0 then
    Index := ADefaultIndex;
  ACombo.ItemIndex := Index;
end;

procedure TVlessProfileDialog.BuildInterface;
var
  Buttons: TButtonPanel;
  Pages: TPageControl;
  GeneralTab: TTabSheet;
  ProtocolTab: TTabSheet;
  ProtocolScroll: TScrollBox;
  TransportTab: TTabSheet;
  SecurityTab: TTabSheet;
  RawTab: TTabSheet;
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

  Pages := TPageControl.Create(Self);
  Pages.Parent := Self;
  Pages.Align := alClient;
  Pages.BorderSpacing.Around := 12;

  GeneralTab := TTabSheet.Create(Self);
  GeneralTab.PageControl := Pages;
  GeneralTab.Caption := 'Основное';
  AddLabel(GeneralTab, 'Название', 20);
  FNameEdit := AddEdit(GeneralTab, 20);
  AddLabel(GeneralTab, 'Протокол', 64);
  FProtocolCombo := TComboBox.Create(Self);
  FProtocolCombo.Parent := GeneralTab;
  FProtocolCombo.Style := csDropDownList;
  FProtocolCombo.Items.Add('VLESS');
  FProtocolCombo.Items.Add('VMess');
  FProtocolCombo.Items.Add('Trojan');
  FProtocolCombo.Items.Add('Shadowsocks');
  FProtocolCombo.Items.Add('SOCKS');
  FProtocolCombo.Items.Add('Hysteria2');
  FProtocolCombo.Items.Add('WireGuard');
  FProtocolCombo.SetBounds(220, 64, 190, 30);
  AddLabel(GeneralTab, 'Сервер', 108);
  FHostEdit := AddEdit(GeneralTab, 108);
  AddLabel(GeneralTab, 'Порт', 152);
  FPortEdit := AddEdit(GeneralTab, 152, 130);
  FPortEdit.NumbersOnly := True;
  AddLabel(GeneralTab, 'UUID / пароль', 196);
  FUuidEdit := AddEdit(GeneralTab, 196);
  AddLabel(GeneralTab, 'Encryption', 240);
  FEncryptionEdit := AddEdit(GeneralTab, 240, 190);
  AddLabel(GeneralTab, 'Flow', 284);
  FFlowEdit := AddEdit(GeneralTab, 284);
  AddLabel(GeneralTab, 'Источник', 328);
  FSourceEdit := AddEdit(GeneralTab, 328);
  AddLabel(GeneralTab, 'Runtime provider', 372);
  FProviderCombo := TComboBox.Create(Self);
  FProviderCombo.Parent := GeneralTab;
  FProviderCombo.Style := csDropDown;
  FProviderCombo.Items.Add(ProviderEmbeddedXray);
  FProviderCombo.Items.Add(ProviderEmbeddedSingBox);
  FProviderCombo.Items.Add(ProviderExternalXray);
  FProviderCombo.Items.Add(ProviderExternalSingBox);
  FProviderCombo.Items.Add(ProviderExternalV2Ray);
  FProviderCombo.Items.Add(ProviderExternalMihomo);
  FProviderCombo.Items.Add(ProviderExternalNekoBoxCore);
  FProviderCombo.Items.Add(ProviderExternalHysteria2);
  FProviderCombo.SetBounds(220, 372, 300, 30);
  FEnabledCheck := TCheckBox.Create(Self);
  FEnabledCheck.Parent := GeneralTab;
  FEnabledCheck.Caption := 'Профиль включён';
  FEnabledCheck.SetBounds(220, 416, 260, 26);

  ProtocolTab := TTabSheet.Create(Self);
  ProtocolTab.PageControl := Pages;
  ProtocolTab.Caption := 'Протокол';
  ProtocolScroll := TScrollBox.Create(Self);
  ProtocolScroll.Parent := ProtocolTab;
  ProtocolScroll.Align := alClient;
  ProtocolScroll.VertScrollBar.Tracking := True;
  AddLabel(ProtocolScroll, 'Password / auth / private key', 20);
  FPasswordEdit := AddEdit(ProtocolScroll, 20);
  AddLabel(ProtocolScroll, 'Shadowsocks method', 64);
  FMethodEdit := AddEdit(ProtocolScroll, 64);
  AddLabel(ProtocolScroll, 'VMess security cipher', 108);
  FSecurityCipherEdit := AddEdit(ProtocolScroll, 108);
  AddLabel(ProtocolScroll, 'VMess alterId', 152);
  FAlterIdEdit := AddEdit(ProtocolScroll, 152, 130);
  FAlterIdEdit.NumbersOnly := True;
  AddLabel(ProtocolScroll, 'Hysteria2 obfs', 196);
  FObfsEdit := AddEdit(ProtocolScroll, 196);
  AddLabel(ProtocolScroll, 'Hysteria2 obfs password', 240);
  FObfsPasswordEdit := AddEdit(ProtocolScroll, 240);
  AddLabel(ProtocolScroll, 'WireGuard local address', 284);
  FLocalAddressEdit := AddEdit(ProtocolScroll, 284);
  AddLabel(ProtocolScroll, 'WireGuard allowed IPs', 328);
  FAllowedIpsEdit := AddEdit(ProtocolScroll, 328);
  AddLabel(ProtocolScroll, 'WireGuard preshared key', 372);
  FPreSharedKeyEdit := AddEdit(ProtocolScroll, 372);
  AddLabel(ProtocolScroll, 'WireGuard reserved bytes', 416);
  FReservedEdit := AddEdit(ProtocolScroll, 416);
  AddLabel(ProtocolScroll, 'WireGuard MTU', 460);
  FMtuEdit := AddEdit(ProtocolScroll, 460, 130);
  FMtuEdit.NumbersOnly := True;
  AddLabel(ProtocolScroll, 'WireGuard keepalive', 504);
  FKeepAliveEdit := AddEdit(ProtocolScroll, 504, 130);
  FKeepAliveEdit.NumbersOnly := True;
  ProtocolScroll.VertScrollBar.Range := 554;

  TransportTab := TTabSheet.Create(Self);
  TransportTab.PageControl := Pages;
  TransportTab.Caption := 'Transport';
  AddLabel(TransportTab, 'Network', 20);
  FNetworkCombo := TComboBox.Create(Self);
  FNetworkCombo.Parent := TransportTab;
  FNetworkCombo.Style := csDropDownList;
  FNetworkCombo.Items.Add('tcp');
  FNetworkCombo.Items.Add('ws');
  FNetworkCombo.Items.Add('grpc');
  FNetworkCombo.SetBounds(220, 20, 190, 30);
  AddLabel(TransportTab, 'WebSocket Host', 64);
  FTransportHostEdit := AddEdit(TransportTab, 64);
  AddLabel(TransportTab, 'Path', 108);
  FPathEdit := AddEdit(TransportTab, 108);
  AddLabel(TransportTab, 'Header type', 152);
  FHeaderTypeEdit := AddEdit(TransportTab, 152, 190);
  AddLabel(TransportTab, 'gRPC service name', 196);
  FServiceNameEdit := AddEdit(TransportTab, 196);

  SecurityTab := TTabSheet.Create(Self);
  SecurityTab.PageControl := Pages;
  SecurityTab.Caption := 'TLS / REALITY';
  AddLabel(SecurityTab, 'Security', 20);
  FSecurityCombo := TComboBox.Create(Self);
  FSecurityCombo.Parent := SecurityTab;
  FSecurityCombo.Style := csDropDownList;
  FSecurityCombo.Items.Add('none');
  FSecurityCombo.Items.Add('tls');
  FSecurityCombo.Items.Add('reality');
  FSecurityCombo.SetBounds(220, 20, 190, 30);
  AddLabel(SecurityTab, 'Server name / SNI', 64);
  FServerNameEdit := AddEdit(SecurityTab, 64);
  AddLabel(SecurityTab, 'Fingerprint', 108);
  FFingerprintEdit := AddEdit(SecurityTab, 108, 190);
  AddLabel(SecurityTab, 'Public key', 152);
  FPublicKeyEdit := AddEdit(SecurityTab, 152);
  AddLabel(SecurityTab, 'Short ID', 196);
  FShortIdEdit := AddEdit(SecurityTab, 196);
  AddLabel(SecurityTab, 'Spider X', 240);
  FSpiderXEdit := AddEdit(SecurityTab, 240);
  AddLabel(SecurityTab, 'ALPN (через запятую)', 284);
  FAlpnEdit := AddEdit(SecurityTab, 284);
  FAllowInsecureCheck := TCheckBox.Create(Self);
  FAllowInsecureCheck.Parent := SecurityTab;
  FAllowInsecureCheck.Caption := 'Разрешить недоверенный TLS-сертификат';
  FAllowInsecureCheck.SetBounds(220, 330, 390, 26);

  RawTab := TTabSheet.Create(Self);
  RawTab.PageControl := Pages;
  RawTab.Caption := 'Raw config';
  AddLabel(RawTab, 'Диалект', 20);
  FRawFormatCombo := TComboBox.Create(Self);
  FRawFormatCombo.Parent := RawTab;
  FRawFormatCombo.Style := csDropDownList;
  FRawFormatCombo.Items.Add('xray-json');
  FRawFormatCombo.Items.Add('v2ray-json');
  FRawFormatCombo.Items.Add('sing-box-json');
  FRawFormatCombo.Items.Add('mihomo-yaml');
  FRawFormatCombo.Items.Add('hysteria-yaml');
  FRawFormatCombo.Items.Add('raw');
  FRawFormatCombo.SetBounds(220, 20, 220, 30);
  AddLabel(RawTab, 'Readiness host', 64);
  FReadinessHostEdit := AddEdit(RawTab, 64, 220);
  AddLabel(RawTab, 'Readiness port', 108);
  FReadinessPortEdit := AddEdit(RawTab, 108, 130);
  FReadinessPortEdit.NumbersOnly := True;
  AddLabel(RawTab, 'Системный прокси', 152);
  FSystemProxyKindCombo := TComboBox.Create(Self);
  FSystemProxyKindCombo.Parent := RawTab;
  FSystemProxyKindCombo.Style := csDropDownList;
  FSystemProxyKindCombo.Items.Add('mixed');
  FSystemProxyKindCombo.Items.Add('http');
  FSystemProxyKindCombo.Items.Add('socks');
  FSystemProxyKindCombo.Items.Add('none');
  FSystemProxyKindCombo.SetBounds(220, 152, 220, 30);
  AddLabel(RawTab, 'JSON / YAML', 196);
  FRawConfigMemo := TMemo.Create(Self);
  FRawConfigMemo.Parent := RawTab;
  FRawConfigMemo.SetBounds(220, 196, 430, 240);
  FRawConfigMemo.Anchors := [akLeft, akTop, akRight, akBottom];
  FRawConfigMemo.ScrollBars := ssAutoBoth;
  FRawConfigMemo.Font.Name := 'Consolas';
  FRawConfigMemo.Font.Size := 9;
end;

procedure TVlessProfileDialog.LoadProfile;
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
  FPasswordEdit.Text := FProfile.Password;
  FEncryptionEdit.Text := FProfile.Encryption;
  FMethodEdit.Text := FProfile.Method;
  FSecurityCipherEdit.Text := FProfile.SecurityCipher;
  FAlterIdEdit.Text := IntToStr(FProfile.AlterId);
  FFlowEdit.Text := FProfile.Flow;
  FObfsEdit.Text := FProfile.Obfs;
  FObfsPasswordEdit.Text := FProfile.ObfsPassword;
  FLocalAddressEdit.Text := FProfile.LocalAddress;
  FAllowedIpsEdit.Text := FProfile.AllowedIps;
  FPreSharedKeyEdit.Text := FProfile.PreSharedKey;
  FReservedEdit.Text := FProfile.Reserved;
  FMtuEdit.Text := IntToStr(FProfile.Mtu);
  FKeepAliveEdit.Text := IntToStr(FProfile.KeepAlive);
  FSourceEdit.Text := FProfile.Source;
  FProviderCombo.Text := FProfile.PreferredProviderId;
  if Trim(FProviderCombo.Text) = '' then
    FProviderCombo.Text := DefaultProviderForProtocol(FProfile.ProtocolName);
  FEnabledCheck.Checked := FProfile.Enabled;
  SelectComboValue(FNetworkCombo, NormalizedNetwork(FProfile), 0);
  FTransportHostEdit.Text := FProfile.TransportHost;
  FPathEdit.Text := FProfile.Path;
  FHeaderTypeEdit.Text := FProfile.HeaderType;
  FServiceNameEdit.Text := FProfile.ServiceName;
  SelectComboValue(FSecurityCombo, FProfile.Security, 0);
  FServerNameEdit.Text := EffectiveServerName(FProfile);
  FFingerprintEdit.Text := FProfile.Fingerprint;
  FPublicKeyEdit.Text := FProfile.PublicKey;
  FShortIdEdit.Text := FProfile.ShortId;
  FSpiderXEdit.Text := FProfile.SpiderX;
  FAlpnEdit.Text := FProfile.Alpn;
  FAllowInsecureCheck.Checked := FProfile.AllowInsecure;
  Index := FRawFormatCombo.Items.IndexOf(FProfile.RawConfigFormat);
  if Index < 0 then
    Index := 0;
  FRawFormatCombo.ItemIndex := Index;
  FRawConfigMemo.Text := FProfile.RawConfig;
  FReadinessHostEdit.Text := FProfile.ReadinessHost;
  FReadinessPortEdit.Text := IntToStr(FProfile.ReadinessPort);
  Index := FSystemProxyKindCombo.Items.IndexOf(FProfile.SystemProxyKind);
  if Index < 0 then
    Index := 0;
  FSystemProxyKindCombo.ItemIndex := Index;
end;

procedure TVlessProfileDialog.StoreProfile;
begin
  FProfile.Name := Trim(FNameEdit.Text);
  FProfile.ProtocolName := FProtocolCombo.Text;
  FProfile.Host := Trim(FHostEdit.Text);
  FProfile.Port := StrToIntDef(FPortEdit.Text, 0);
  FProfile.Uuid := Trim(FUuidEdit.Text);
  FProfile.Password := Trim(FPasswordEdit.Text);
  FProfile.Encryption := Trim(FEncryptionEdit.Text);
  if FProfile.Encryption = '' then
    FProfile.Encryption := 'none';
  FProfile.Method := Trim(FMethodEdit.Text);
  FProfile.SecurityCipher := Trim(FSecurityCipherEdit.Text);
  FProfile.AlterId := StrToIntDef(FAlterIdEdit.Text, 0);
  FProfile.Flow := Trim(FFlowEdit.Text);
  FProfile.Obfs := Trim(FObfsEdit.Text);
  FProfile.ObfsPassword := Trim(FObfsPasswordEdit.Text);
  FProfile.LocalAddress := Trim(FLocalAddressEdit.Text);
  FProfile.AllowedIps := Trim(FAllowedIpsEdit.Text);
  FProfile.PreSharedKey := Trim(FPreSharedKeyEdit.Text);
  FProfile.Reserved := Trim(FReservedEdit.Text);
  FProfile.Mtu := StrToIntDef(FMtuEdit.Text, 0);
  FProfile.KeepAlive := StrToIntDef(FKeepAliveEdit.Text, 0);
  FProfile.Source := Trim(FSourceEdit.Text);
  if FProfile.Source = '' then
    FProfile.Source := 'Вручную';
  FProfile.Enabled := FEnabledCheck.Checked;
  FProfile.PreferredProviderId := Trim(FProviderCombo.Text);
  if FProfile.PreferredProviderId = '' then
    FProfile.PreferredProviderId := DefaultProviderForProtocol(
      FProfile.ProtocolName);
  FProfile.Network := FNetworkCombo.Text;
  FProfile.TransportHost := Trim(FTransportHostEdit.Text);
  FProfile.Path := Trim(FPathEdit.Text);
  FProfile.HeaderType := Trim(FHeaderTypeEdit.Text);
  FProfile.ServiceName := Trim(FServiceNameEdit.Text);
  FProfile.Security := FSecurityCombo.Text;
  if SameText(FProfile.Security, 'none') then
    FProfile.Security := '';
  FProfile.ServerName := Trim(FServerNameEdit.Text);
  FProfile.Sni := FProfile.ServerName;
  FProfile.Fingerprint := Trim(FFingerprintEdit.Text);
  FProfile.PublicKey := Trim(FPublicKeyEdit.Text);
  FProfile.ShortId := Trim(FShortIdEdit.Text);
  FProfile.SpiderX := Trim(FSpiderXEdit.Text);
  FProfile.Alpn := Trim(FAlpnEdit.Text);
  FProfile.AllowInsecure := FAllowInsecureCheck.Checked;
  FProfile.RawConfigFormat := FRawFormatCombo.Text;
  FProfile.RawConfig := FRawConfigMemo.Text;
  FProfile.ReadinessHost := Trim(FReadinessHostEdit.Text);
  if FProfile.ReadinessHost = '' then
    FProfile.ReadinessHost := '127.0.0.1';
  FProfile.ReadinessPort := StrToIntDef(FReadinessPortEdit.Text, 0);
  FProfile.SystemProxyKind := FSystemProxyKindCombo.Text;
end;

procedure TVlessProfileDialog.AcceptClick(Sender: TObject);
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

class function TVlessProfileDialog.Execute(AOwner: TComponent;
  var AProfile: TZaryaProfile; const ADarkTheme: Boolean): Boolean;
var
  Dialog: TVlessProfileDialog;
begin
  Dialog := TVlessProfileDialog.Create(AOwner, AProfile, ADarkTheme);
  try
    Result := Dialog.ShowModal = mrOk;
    if Result then
      AProfile := Dialog.FProfile;
  finally
    Dialog.Free;
  end;
end;

end.
