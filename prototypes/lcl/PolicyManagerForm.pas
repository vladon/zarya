unit PolicyManagerForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ComCtrls, Grids, ButtonPanel, ZaryaRouting, ZaryaDns, ZaryaThemes, ZaryaTr;

type
  TPolicyManagerDialog = class(TForm)
  private
    FRoutingProfiles: TZaryaRoutingProfiles;
    FDnsProfiles: TZaryaDnsProfiles;
    FSelectedRoutingId: string;
    FSelectedDnsId: string;
    FDarkTheme: Boolean;
    FRoutingList: TListBox;
    FDnsList: TListBox;
    FRoutingActiveLabel: TLabel;
    FDnsActiveLabel: TLabel;
    procedure BuildInterface;
    procedure RefreshRouting;
    procedure RefreshDns;
    procedure UseRoutingClick(Sender: TObject);
    procedure UseDnsClick(Sender: TObject);
    procedure AddRoutingClick(Sender: TObject);
    procedure EditRoutingClick(Sender: TObject);
    procedure DeleteRoutingClick(Sender: TObject);
    procedure AddDnsClick(Sender: TObject);
    procedure EditDnsClick(Sender: TObject);
    procedure DeleteDnsClick(Sender: TObject);
    function RoutingIndex: Integer;
    function DnsIndex: Integer;
  public
    constructor Create(AOwner: TComponent;
      const ARoutingProfiles: TZaryaRoutingProfiles;
      const ADnsProfiles: TZaryaDnsProfiles;
      const ASelectedRoutingId, ASelectedDnsId: string;
      const ADarkTheme: Boolean); reintroduce;
    class function Execute(AOwner: TComponent;
      var ARoutingProfiles: TZaryaRoutingProfiles;
      var ADnsProfiles: TZaryaDnsProfiles;
      var ASelectedRoutingId, ASelectedDnsId: string;
      const ADarkTheme: Boolean): Boolean;
  end;

implementation

type
  TRoutingProfileEditor = class(TForm)
  private
    FNameEdit: TEdit;
    FModeCombo: TComboBox;
    FStrategyCombo: TComboBox;
    FGrid: TStringGrid;
    procedure AddRuleClick(Sender: TObject);
    procedure DeleteRuleClick(Sender: TObject);
    procedure AcceptClick(Sender: TObject);
  public
    constructor Create(AOwner: TComponent;
      const AProfile: TZaryaRoutingProfile;
      const ADarkTheme: Boolean); reintroduce;
    function CopyProfile(const AOriginal: TZaryaRoutingProfile;
      out AProfile: TZaryaRoutingProfile; out AError: string): Boolean;
  end;

  TDnsProfileEditor = class(TForm)
  private
    FNameEdit: TEdit;
    FModeCombo: TComboBox;
    FStrategyCombo: TComboBox;
    FDisableCache: TCheckBox;
    FDisableFallback: TCheckBox;
    FDisableFallbackIfMatch: TCheckBox;
    FHostsMemo: TMemo;
    FGrid: TStringGrid;
    procedure AddServerClick(Sender: TObject);
    procedure DeleteServerClick(Sender: TObject);
    procedure AcceptClick(Sender: TObject);
  public
    constructor Create(AOwner: TComponent; const AProfile: TZaryaDnsProfile;
      const ADarkTheme: Boolean); reintroduce;
    function CopyProfile(const AOriginal: TZaryaDnsProfile;
      out AProfile: TZaryaDnsProfile; out AError: string): Boolean;
  end;

function NewPolicyId(const APrefix: string): string;
begin
  Result := APrefix + '-' + FormatDateTime('yyyymmddhhnnsszzz', Now) + '-' +
    IntToHex(Random(MaxInt), 8);
end;

function SplitValues(const AValue: string): TZaryaStringArray;
var
  Values: TStringList;
  I: Integer;
begin
  Result := nil;
  Values := TStringList.Create;
  try
    Values.Delimiter := ',';
    Values.StrictDelimiter := True;
    Values.DelimitedText := AValue;
    SetLength(Result, Values.Count);
    for I := 0 to Values.Count - 1 do Result[I] := Trim(Values[I]);
  finally
    Values.Free;
  end;
end;

function SplitDnsValues(const AValue: string): TZaryaDnsStringArray;
var
  Generic: TZaryaStringArray;
  I: Integer;
begin
  Result := nil;
  Generic := SplitValues(AValue);
  SetLength(Result, Length(Generic));
  for I := 0 to High(Generic) do Result[I] := Generic[I];
end;

function JoinValues(const AValues: TZaryaStringArray): string;
var
  I: Integer;
begin
  Result := '';
  for I := 0 to High(AValues) do
  begin
    if I > 0 then Result := Result + ', ';
    Result := Result + AValues[I];
  end;
end;

function JoinDnsValues(const AValues: TZaryaDnsStringArray): string;
var
  I: Integer;
begin
  Result := '';
  for I := 0 to High(AValues) do
  begin
    if I > 0 then Result := Result + ', ';
    Result := Result + AValues[I];
  end;
end;

function BoolText(const AValue: Boolean): string;
begin
  if AValue then Result := 'true' else Result := 'false';
end;

procedure AddCaption(AOwner: TComponent; AParent: TWinControl;
  const AText: string; const ALeft, ATop, AWidth: Integer);
var
  LabelControl: TLabel;
begin
  LabelControl := TLabel.Create(AOwner);
  LabelControl.Parent := AParent;
  LabelControl.Caption := AText;
  LabelControl.SetBounds(ALeft, ATop, AWidth, 24);
end;

constructor TRoutingProfileEditor.Create(AOwner: TComponent;
  const AProfile: TZaryaRoutingProfile; const ADarkTheme: Boolean);
var
  Buttons: TButtonPanel;
  AddButton, DeleteButton: TButton;
  Rule: TZaryaRoutingRule;
  Row: Integer;
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  Caption := 'Routing profile';
  Position := poOwnerFormCenter;
  BorderStyle := bsSizeable;
  ClientWidth := 850;
  ClientHeight := 560;
  Constraints.MinWidth := 720;
  Constraints.MinHeight := 480;

  Buttons := TButtonPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.ShowButtons := [pbOK, pbCancel];
  Buttons.ShowGlyphs := [];
  Buttons.OKButton.Caption := TZaryaTr.Tr('Сохранить');
  Buttons.OKButton.ModalResult := mrNone;
  Buttons.OKButton.OnClick := @AcceptClick;
  Buttons.CancelButton.Caption := TZaryaTr.Tr('Отмена');

  AddCaption(Self, Self, TZaryaTr.Tr('Название', 'Name'), 16, 18, 100);
  FNameEdit := TEdit.Create(Self);
  FNameEdit.Parent := Self;
  FNameEdit.SetBounds(120, 14, 260, 28);
  FNameEdit.Text := AProfile.Name;
  AddCaption(Self, Self, TZaryaTr.Tr('Режим', 'Mode'), 410, 18, 70);
  FModeCombo := TComboBox.Create(Self);
  FModeCombo.Parent := Self;
  FModeCombo.Style := csDropDownList;
  FModeCombo.Items.AddStrings(['proxy_all', 'bypass_lan', 'bypass_ru',
    'bypass_lan_and_ru', 'custom']);
  FModeCombo.ItemIndex := FModeCombo.Items.IndexOf(
    RoutingModeToString(AProfile.Mode));
  FModeCombo.SetBounds(480, 14, 170, 28);
  AddCaption(Self, Self, 'Domain strategy', 16, 58, 120);
  FStrategyCombo := TComboBox.Create(Self);
  FStrategyCombo.Parent := Self;
  FStrategyCombo.Style := csDropDownList;
  FStrategyCombo.Items.AddStrings(['AsIs', 'IPIfNonMatch', 'IPOnDemand']);
  FStrategyCombo.ItemIndex := FStrategyCombo.Items.IndexOf(AProfile.DomainStrategy);
  if FStrategyCombo.ItemIndex < 0 then FStrategyCombo.ItemIndex := 0;
  FStrategyCombo.SetBounds(140, 54, 180, 28);

  AddButton := TButton.Create(Self);
  AddButton.Parent := Self;
  AddButton.Caption := TZaryaTr.Tr('Добавить правило', 'Add rule');
  AddButton.SetBounds(16, 98, 150, 30);
  AddButton.OnClick := @AddRuleClick;
  DeleteButton := TButton.Create(Self);
  DeleteButton.Parent := Self;
  DeleteButton.Caption := TZaryaTr.Tr('Удалить правило', 'Delete rule');
  DeleteButton.SetBounds(174, 98, 150, 30);
  DeleteButton.OnClick := @DeleteRuleClick;

  FGrid := TStringGrid.Create(Self);
  FGrid.Parent := Self;
  FGrid.AnchorSideTop.Control := AddButton;
  FGrid.AnchorSideTop.Side := asrBottom;
  FGrid.AnchorSideLeft.Control := Self;
  FGrid.AnchorSideRight.Control := Self;
  FGrid.AnchorSideRight.Side := asrBottom;
  FGrid.AnchorSideBottom.Control := Buttons;
  FGrid.Anchors := [akTop, akLeft, akRight, akBottom];
  FGrid.SetBounds(16, 138, ClientWidth - 32, ClientHeight - 202);
  FGrid.ColCount := 5;
  FGrid.FixedRows := 1;
  FGrid.RowCount := Length(AProfile.Rules) + 1;
  if FGrid.RowCount < 2 then FGrid.RowCount := 2;
  FGrid.Options := (FGrid.Options + [goEditing, goColSizing]) - [goRowSelect];
  FGrid.Cells[0, 0] := 'Enabled';
  FGrid.Cells[1, 0] := 'Type';
  FGrid.Cells[2, 0] := 'Action';
  FGrid.Cells[3, 0] := 'Values (comma-separated)';
  FGrid.Cells[4, 0] := 'Note';
  FGrid.ColWidths[0] := 70;
  FGrid.ColWidths[1] := 90;
  FGrid.ColWidths[2] := 90;
  FGrid.ColWidths[3] := 340;
  FGrid.ColWidths[4] := 180;
  Row := 1;
  for Rule in AProfile.Rules do
  begin
    FGrid.Cells[0, Row] := BoolText(Rule.Enabled);
    FGrid.Cells[1, Row] := RoutingRuleTypeToString(Rule.RuleType);
    FGrid.Cells[2, Row] := RoutingActionToString(Rule.Action);
    FGrid.Cells[3, Row] := JoinValues(Rule.Values);
    FGrid.Cells[4, Row] := Rule.Note;
    Inc(Row);
  end;
  if ADarkTheme then Theme := ZaryaThemes.DarkTheme
  else Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

procedure TRoutingProfileEditor.AddRuleClick(Sender: TObject);
begin
  FGrid.RowCount := FGrid.RowCount + 1;
  FGrid.Row := FGrid.RowCount - 1;
  FGrid.Cells[0, FGrid.Row] := 'true';
  FGrid.Cells[1, FGrid.Row] := 'domain';
  FGrid.Cells[2, FGrid.Row] := 'proxy';
end;

procedure TRoutingProfileEditor.DeleteRuleClick(Sender: TObject);
var
  Row, Col: Integer;
begin
  if (FGrid.Row <= 0) or (FGrid.RowCount <= 2) then
  begin
    for Col := 0 to FGrid.ColCount - 1 do FGrid.Cells[Col, 1] := '';
    Exit;
  end;
  for Row := FGrid.Row to FGrid.RowCount - 2 do
    for Col := 0 to FGrid.ColCount - 1 do
      FGrid.Cells[Col, Row] := FGrid.Cells[Col, Row + 1];
  FGrid.RowCount := FGrid.RowCount - 1;
end;

procedure TRoutingProfileEditor.AcceptClick(Sender: TObject);
var
  ErrorMessage: string;
  Profile: TZaryaRoutingProfile;
begin
  if not CopyProfile(Default(TZaryaRoutingProfile), Profile, ErrorMessage) then
  begin
    MessageDlg('Routing', ErrorMessage, mtWarning, [mbOK], 0);
    Exit;
  end;
  ModalResult := mrOk;
end;

function TRoutingProfileEditor.CopyProfile(
  const AOriginal: TZaryaRoutingProfile; out AProfile: TZaryaRoutingProfile;
  out AError: string): Boolean;
var
  Row, OutIndex: Integer;
  Rule: TZaryaRoutingRule;
begin
  AProfile := AOriginal;
  if AProfile.Id = '' then AProfile.Id := NewPolicyId('routing');
  AProfile.Name := Trim(FNameEdit.Text);
  AProfile.Mode := RoutingModeFromString(FModeCombo.Text);
  AProfile.DomainStrategy := FStrategyCombo.Text;
  AProfile.Enabled := True;
  AProfile.IsBuiltIn := False;
  AProfile.UpdatedAt := FormatDateTime('yyyy-mm-dd"T"hh:nn:ss', Now);
  if AProfile.CreatedAt = '' then AProfile.CreatedAt := AProfile.UpdatedAt;
  SetLength(AProfile.Rules, FGrid.RowCount - 1);
  OutIndex := 0;
  for Row := 1 to FGrid.RowCount - 1 do
  begin
    if Trim(FGrid.Cells[3, Row]) = '' then Continue;
    Rule := NewRoutingRule;
    Rule.Enabled := not SameText(Trim(FGrid.Cells[0, Row]), 'false');
    Rule.RuleType := RoutingRuleTypeFromString(FGrid.Cells[1, Row]);
    Rule.Action := RoutingActionFromString(FGrid.Cells[2, Row]);
    Rule.Values := SplitValues(FGrid.Cells[3, Row]);
    Rule.Note := Trim(FGrid.Cells[4, Row]);
    AProfile.Rules[OutIndex] := Rule;
    Inc(OutIndex);
  end;
  SetLength(AProfile.Rules, OutIndex);
  Result := ValidateRoutingProfile(AProfile, AError);
end;

constructor TDnsProfileEditor.Create(AOwner: TComponent;
  const AProfile: TZaryaDnsProfile; const ADarkTheme: Boolean);
var
  Buttons: TButtonPanel;
  AddButton, DeleteButton: TButton;
  Server: TZaryaDnsServer;
  Host: TZaryaDnsHost;
  Row: Integer;
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  Caption := 'DNS profile';
  Position := poOwnerFormCenter;
  BorderStyle := bsSizeable;
  ClientWidth := 920;
  ClientHeight := 650;
  Constraints.MinWidth := 760;
  Constraints.MinHeight := 540;
  Buttons := TButtonPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.ShowButtons := [pbOK, pbCancel];
  Buttons.ShowGlyphs := [];
  Buttons.OKButton.Caption := TZaryaTr.Tr('Сохранить');
  Buttons.OKButton.ModalResult := mrNone;
  Buttons.OKButton.OnClick := @AcceptClick;
  Buttons.CancelButton.Caption := TZaryaTr.Tr('Отмена');

  AddCaption(Self, Self, TZaryaTr.Tr('Название', 'Name'), 16, 18, 100);
  FNameEdit := TEdit.Create(Self);
  FNameEdit.Parent := Self;
  FNameEdit.SetBounds(120, 14, 260, 28);
  FNameEdit.Text := AProfile.Name;
  AddCaption(Self, Self, TZaryaTr.Tr('Режим', 'Mode'), 400, 18, 60);
  FModeCombo := TComboBox.Create(Self);
  FModeCombo.Parent := Self;
  FModeCombo.Style := csDropDownList;
  FModeCombo.Items.AddStrings(['system', 'secure-remote',
    'china-direct-global-remote', 'custom']);
  FModeCombo.ItemIndex := FModeCombo.Items.IndexOf(DnsModeToString(AProfile.Mode));
  FModeCombo.SetBounds(460, 14, 230, 28);
  AddCaption(Self, Self, 'Query strategy', 16, 58, 110);
  FStrategyCombo := TComboBox.Create(Self);
  FStrategyCombo.Parent := Self;
  FStrategyCombo.Style := csDropDownList;
  FStrategyCombo.Items.AddStrings(['system-default', 'use-ip', 'use-ipv4',
    'use-ipv6']);
  FStrategyCombo.ItemIndex := FStrategyCombo.Items.IndexOf(
    DnsQueryStrategyToString(AProfile.QueryStrategy));
  FStrategyCombo.SetBounds(130, 54, 170, 28);
  FDisableCache := TCheckBox.Create(Self);
  FDisableCache.Parent := Self;
  FDisableCache.Caption := 'Disable cache';
  FDisableCache.Checked := AProfile.DisableCache;
  FDisableCache.SetBounds(320, 54, 130, 26);
  FDisableFallback := TCheckBox.Create(Self);
  FDisableFallback.Parent := Self;
  FDisableFallback.Caption := 'Disable fallback';
  FDisableFallback.Checked := AProfile.DisableFallback;
  FDisableFallback.SetBounds(460, 54, 140, 26);
  FDisableFallbackIfMatch := TCheckBox.Create(Self);
  FDisableFallbackIfMatch.Parent := Self;
  FDisableFallbackIfMatch.Caption := 'Disable fallback if match';
  FDisableFallbackIfMatch.Checked := AProfile.DisableFallbackIfMatch;
  FDisableFallbackIfMatch.SetBounds(610, 54, 210, 26);

  AddCaption(Self, Self, TZaryaTr.Tr(
    'Hosts (host=address, по одному в строке)',
    'Hosts (host=address, one per line)'), 16, 92, 360);
  FHostsMemo := TMemo.Create(Self);
  FHostsMemo.Parent := Self;
  FHostsMemo.ScrollBars := ssVertical;
  FHostsMemo.SetBounds(16, 116, ClientWidth - 32, 90);
  for Host in AProfile.Hosts do
    FHostsMemo.Lines.Add(Host.Host + '=' + Host.Address);

  AddButton := TButton.Create(Self);
  AddButton.Parent := Self;
  AddButton.Caption := TZaryaTr.Tr('Добавить сервер', 'Add server');
  AddButton.SetBounds(16, 216, 150, 30);
  AddButton.OnClick := @AddServerClick;
  DeleteButton := TButton.Create(Self);
  DeleteButton.Parent := Self;
  DeleteButton.Caption := TZaryaTr.Tr('Удалить сервер', 'Delete server');
  DeleteButton.SetBounds(174, 216, 150, 30);
  DeleteButton.OnClick := @DeleteServerClick;

  FGrid := TStringGrid.Create(Self);
  FGrid.Parent := Self;
  FGrid.SetBounds(16, 256, ClientWidth - 32, ClientHeight - 320);
  FGrid.Anchors := [akTop, akLeft, akRight, akBottom];
  FGrid.ColCount := 8;
  FGrid.FixedRows := 1;
  FGrid.RowCount := Length(AProfile.Servers) + 1;
  if FGrid.RowCount < 2 then FGrid.RowCount := 2;
  FGrid.Options := (FGrid.Options + [goEditing, goColSizing]) - [goRowSelect];
  FGrid.Cells[0, 0] := 'Enabled'; FGrid.Cells[1, 0] := 'Kind';
  FGrid.Cells[2, 0] := 'Address'; FGrid.Cells[3, 0] := 'Port';
  FGrid.Cells[4, 0] := 'Domains'; FGrid.Cells[5, 0] := 'Expect IPs';
  FGrid.Cells[6, 0] := 'Strategy'; FGrid.Cells[7, 0] := 'Tag';
  FGrid.ColWidths[0] := 65; FGrid.ColWidths[1] := 70;
  FGrid.ColWidths[2] := 230; FGrid.ColWidths[3] := 55;
  FGrid.ColWidths[4] := 150; FGrid.ColWidths[5] := 130;
  FGrid.ColWidths[6] := 90; FGrid.ColWidths[7] := 80;
  Row := 1;
  for Server in AProfile.Servers do
  begin
    FGrid.Cells[0, Row] := BoolText(Server.Enabled);
    FGrid.Cells[1, Row] := DnsServerKindToString(Server.Kind);
    FGrid.Cells[2, Row] := Server.Address;
    FGrid.Cells[3, Row] := IntToStr(Server.Port);
    FGrid.Cells[4, Row] := JoinDnsValues(Server.Domains);
    FGrid.Cells[5, Row] := JoinDnsValues(Server.ExpectIps);
    FGrid.Cells[6, Row] := Server.QueryStrategy;
    FGrid.Cells[7, Row] := Server.Tag;
    Inc(Row);
  end;
  if ADarkTheme then Theme := ZaryaThemes.DarkTheme
  else Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

procedure TDnsProfileEditor.AddServerClick(Sender: TObject);
begin
  FGrid.RowCount := FGrid.RowCount + 1;
  FGrid.Row := FGrid.RowCount - 1;
  FGrid.Cells[0, FGrid.Row] := 'true';
  FGrid.Cells[1, FGrid.Row] := 'plain';
end;

procedure TDnsProfileEditor.DeleteServerClick(Sender: TObject);
var
  Row, Col: Integer;
begin
  if (FGrid.Row <= 0) or (FGrid.RowCount <= 2) then
  begin
    for Col := 0 to FGrid.ColCount - 1 do FGrid.Cells[Col, 1] := '';
    Exit;
  end;
  for Row := FGrid.Row to FGrid.RowCount - 2 do
    for Col := 0 to FGrid.ColCount - 1 do
      FGrid.Cells[Col, Row] := FGrid.Cells[Col, Row + 1];
  FGrid.RowCount := FGrid.RowCount - 1;
end;

procedure TDnsProfileEditor.AcceptClick(Sender: TObject);
var
  ErrorMessage: string;
  Profile: TZaryaDnsProfile;
begin
  if not CopyProfile(Default(TZaryaDnsProfile), Profile, ErrorMessage) then
  begin
    MessageDlg('DNS', ErrorMessage, mtWarning, [mbOK], 0);
    Exit;
  end;
  ModalResult := mrOk;
end;

function TDnsProfileEditor.CopyProfile(const AOriginal: TZaryaDnsProfile;
  out AProfile: TZaryaDnsProfile; out AError: string): Boolean;
var
  Row, OutIndex, Marker: Integer;
  Server: TZaryaDnsServer;
  Line: string;
begin
  AProfile := AOriginal;
  if AProfile.Id = '' then AProfile.Id := NewPolicyId('dns');
  AProfile.Name := Trim(FNameEdit.Text);
  AProfile.Mode := DnsModeFromString(FModeCombo.Text);
  AProfile.QueryStrategy := DnsQueryStrategyFromString(FStrategyCombo.Text);
  AProfile.DisableCache := FDisableCache.Checked;
  AProfile.DisableFallback := FDisableFallback.Checked;
  AProfile.DisableFallbackIfMatch := FDisableFallbackIfMatch.Checked;
  AProfile.Enabled := True;
  AProfile.IsBuiltIn := False;
  AProfile.UpdatedAt := FormatDateTime('yyyy-mm-dd"T"hh:nn:ss', Now);
  if AProfile.CreatedAt = '' then AProfile.CreatedAt := AProfile.UpdatedAt;
  SetLength(AProfile.Hosts, FHostsMemo.Lines.Count);
  OutIndex := 0;
  for Row := 0 to FHostsMemo.Lines.Count - 1 do
  begin
    Line := Trim(FHostsMemo.Lines[Row]);
    if Line = '' then Continue;
    Marker := Pos('=', Line);
    if Marker <= 1 then
    begin
      AError := 'Hosts line must use host=address: ' + Line;
      Exit(False);
    end;
    AProfile.Hosts[OutIndex].Host := Trim(Copy(Line, 1, Marker - 1));
    AProfile.Hosts[OutIndex].Address := Trim(Copy(Line, Marker + 1, MaxInt));
    Inc(OutIndex);
  end;
  SetLength(AProfile.Hosts, OutIndex);
  SetLength(AProfile.Servers, FGrid.RowCount - 1);
  OutIndex := 0;
  for Row := 1 to FGrid.RowCount - 1 do
  begin
    if Trim(FGrid.Cells[2, Row]) = '' then Continue;
    Server := NewDnsServer;
    Server.Enabled := not SameText(Trim(FGrid.Cells[0, Row]), 'false');
    Server.Kind := DnsServerKindFromString(FGrid.Cells[1, Row]);
    Server.Address := Trim(FGrid.Cells[2, Row]);
    Server.Port := StrToIntDef(Trim(FGrid.Cells[3, Row]), 0);
    Server.Domains := SplitDnsValues(FGrid.Cells[4, Row]);
    Server.ExpectIps := SplitDnsValues(FGrid.Cells[5, Row]);
    Server.QueryStrategy := Trim(FGrid.Cells[6, Row]);
    Server.Tag := Trim(FGrid.Cells[7, Row]);
    AProfile.Servers[OutIndex] := Server;
    Inc(OutIndex);
  end;
  SetLength(AProfile.Servers, OutIndex);
  Result := ValidateDnsProfile(AProfile, AError);
end;

constructor TPolicyManagerDialog.Create(AOwner: TComponent;
  const ARoutingProfiles: TZaryaRoutingProfiles;
  const ADnsProfiles: TZaryaDnsProfiles;
  const ASelectedRoutingId, ASelectedDnsId: string;
  const ADarkTheme: Boolean);
var
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  Caption := TZaryaTr.Tr('Routing и DNS', 'Routing and DNS');
  Position := poOwnerFormCenter;
  BorderStyle := bsSizeable;
  ClientWidth := 760;
  ClientHeight := 500;
  Constraints.MinWidth := 640;
  Constraints.MinHeight := 420;
  FRoutingProfiles := Copy(ARoutingProfiles);
  FDnsProfiles := Copy(ADnsProfiles);
  FSelectedRoutingId := ASelectedRoutingId;
  FSelectedDnsId := ASelectedDnsId;
  FDarkTheme := ADarkTheme;
  BuildInterface;
  RefreshRouting;
  RefreshDns;
  if ADarkTheme then Theme := ZaryaThemes.DarkTheme
  else Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

procedure TPolicyManagerDialog.BuildInterface;
var
  Pages: TPageControl;
  RoutingTab, DnsTab: TTabSheet;
  Buttons: TButtonPanel;
  Button: TButton;
begin
  Buttons := TButtonPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.ShowButtons := [pbOK, pbCancel];
  Buttons.ShowGlyphs := [];
  Buttons.OKButton.Caption := TZaryaTr.Tr('Применить');
  Buttons.CancelButton.Caption := TZaryaTr.Tr('Отмена');
  Pages := TPageControl.Create(Self);
  Pages.Parent := Self;
  Pages.Align := alClient;
  Pages.BorderSpacing.Around := 12;

  RoutingTab := TTabSheet.Create(Self);
  RoutingTab.PageControl := Pages;
  RoutingTab.Caption := 'Routing';
  FRoutingActiveLabel := TLabel.Create(Self);
  FRoutingActiveLabel.Parent := RoutingTab;
  FRoutingActiveLabel.SetBounds(16, 14, 650, 24);
  FRoutingList := TListBox.Create(Self);
  FRoutingList.Parent := RoutingTab;
  FRoutingList.SetBounds(16, 44, 520, 340);
  FRoutingList.Anchors := [akTop, akLeft, akRight, akBottom];
  Button := TButton.Create(Self); Button.Parent := RoutingTab;
  Button.Caption := TZaryaTr.Tr('Использовать', 'Use'); Button.SetBounds(552, 44, 150, 30);
  Button.OnClick := @UseRoutingClick;
  Button := TButton.Create(Self); Button.Parent := RoutingTab;
  Button.Caption := TZaryaTr.Tr('Добавить'); Button.SetBounds(552, 82, 150, 30);
  Button.OnClick := @AddRoutingClick;
  Button := TButton.Create(Self); Button.Parent := RoutingTab;
  Button.Caption := TZaryaTr.Tr('Изменить'); Button.SetBounds(552, 120, 150, 30);
  Button.OnClick := @EditRoutingClick;
  Button := TButton.Create(Self); Button.Parent := RoutingTab;
  Button.Caption := TZaryaTr.Tr('Удалить'); Button.SetBounds(552, 158, 150, 30);
  Button.OnClick := @DeleteRoutingClick;

  DnsTab := TTabSheet.Create(Self);
  DnsTab.PageControl := Pages;
  DnsTab.Caption := 'DNS';
  FDnsActiveLabel := TLabel.Create(Self);
  FDnsActiveLabel.Parent := DnsTab;
  FDnsActiveLabel.SetBounds(16, 14, 650, 24);
  FDnsList := TListBox.Create(Self);
  FDnsList.Parent := DnsTab;
  FDnsList.SetBounds(16, 44, 520, 340);
  FDnsList.Anchors := [akTop, akLeft, akRight, akBottom];
  Button := TButton.Create(Self); Button.Parent := DnsTab;
  Button.Caption := TZaryaTr.Tr('Использовать', 'Use'); Button.SetBounds(552, 44, 150, 30);
  Button.OnClick := @UseDnsClick;
  Button := TButton.Create(Self); Button.Parent := DnsTab;
  Button.Caption := TZaryaTr.Tr('Добавить'); Button.SetBounds(552, 82, 150, 30);
  Button.OnClick := @AddDnsClick;
  Button := TButton.Create(Self); Button.Parent := DnsTab;
  Button.Caption := TZaryaTr.Tr('Изменить'); Button.SetBounds(552, 120, 150, 30);
  Button.OnClick := @EditDnsClick;
  Button := TButton.Create(Self); Button.Parent := DnsTab;
  Button.Caption := TZaryaTr.Tr('Удалить'); Button.SetBounds(552, 158, 150, 30);
  Button.OnClick := @DeleteDnsClick;
end;

function TPolicyManagerDialog.RoutingIndex: Integer;
begin
  Result := FRoutingList.ItemIndex;
  if (Result < 0) or (Result > High(FRoutingProfiles)) then Result := -1;
end;

function TPolicyManagerDialog.DnsIndex: Integer;
begin
  Result := FDnsList.ItemIndex;
  if (Result < 0) or (Result > High(FDnsProfiles)) then Result := -1;
end;

procedure TPolicyManagerDialog.RefreshRouting;
var
  I: Integer;
begin
  FRoutingList.Items.BeginUpdate;
  try
    FRoutingList.Clear;
    for I := 0 to High(FRoutingProfiles) do
    begin
      FRoutingList.Items.Add(FRoutingProfiles[I].Name + ' · ' +
        RoutingModeToString(FRoutingProfiles[I].Mode));
      if SameText(FRoutingProfiles[I].Id, FSelectedRoutingId) then
        FRoutingList.ItemIndex := I;
    end;
  finally
    FRoutingList.Items.EndUpdate;
  end;
  FRoutingActiveLabel.Caption := TZaryaTr.Tr('Активный routing: ',
    'Active routing: ') + FSelectedRoutingId;
end;

procedure TPolicyManagerDialog.RefreshDns;
var
  I: Integer;
begin
  FDnsList.Items.BeginUpdate;
  try
    FDnsList.Clear;
    for I := 0 to High(FDnsProfiles) do
    begin
      FDnsList.Items.Add(FDnsProfiles[I].Name + ' · ' +
        DnsModeToString(FDnsProfiles[I].Mode));
      if SameText(FDnsProfiles[I].Id, FSelectedDnsId) then FDnsList.ItemIndex := I;
    end;
  finally
    FDnsList.Items.EndUpdate;
  end;
  FDnsActiveLabel.Caption := TZaryaTr.Tr('Активный DNS: ',
    'Active DNS: ') + FSelectedDnsId;
end;

procedure TPolicyManagerDialog.UseRoutingClick(Sender: TObject);
var Index: Integer;
begin
  Index := RoutingIndex;
  if Index < 0 then Exit;
  FSelectedRoutingId := FRoutingProfiles[Index].Id;
  RefreshRouting;
end;

procedure TPolicyManagerDialog.UseDnsClick(Sender: TObject);
var Index: Integer;
begin
  Index := DnsIndex;
  if Index < 0 then Exit;
  FSelectedDnsId := FDnsProfiles[Index].Id;
  RefreshDns;
end;

procedure TPolicyManagerDialog.AddRoutingClick(Sender: TObject);
var
  Editor: TRoutingProfileEditor;
  Profile, Empty: TZaryaRoutingProfile;
  ErrorMessage: string;
begin
  Empty := BuiltInProxyAllRouting;
  Empty.Id := '';
  Empty.Name := 'Custom routing';
  Empty.Mode := rmCustom;
  Empty.IsBuiltIn := False;
  Editor := TRoutingProfileEditor.Create(Self, Empty, FDarkTheme);
  try
    if (Editor.ShowModal = mrOk) and Editor.CopyProfile(Empty, Profile,
      ErrorMessage) then
    begin
      SetLength(FRoutingProfiles, Length(FRoutingProfiles) + 1);
      FRoutingProfiles[High(FRoutingProfiles)] := Profile;
      FSelectedRoutingId := Profile.Id;
      RefreshRouting;
    end;
  finally
    Editor.Free;
  end;
end;

procedure TPolicyManagerDialog.EditRoutingClick(Sender: TObject);
var
  Editor: TRoutingProfileEditor;
  Profile: TZaryaRoutingProfile;
  ErrorMessage: string;
  Index: Integer;
begin
  Index := RoutingIndex;
  if Index < 0 then Exit;
  if FRoutingProfiles[Index].IsBuiltIn then
  begin
    MessageDlg('Routing', 'Built-in profile is read-only. Add a custom copy.',
      mtInformation, [mbOK], 0);
    Exit;
  end;
  Editor := TRoutingProfileEditor.Create(Self, FRoutingProfiles[Index], FDarkTheme);
  try
    if (Editor.ShowModal = mrOk) and Editor.CopyProfile(FRoutingProfiles[Index],
      Profile, ErrorMessage) then
    begin
      FRoutingProfiles[Index] := Profile;
      RefreshRouting;
    end;
  finally
    Editor.Free;
  end;
end;

procedure TPolicyManagerDialog.DeleteRoutingClick(Sender: TObject);
var I, Index: Integer;
begin
  Index := RoutingIndex;
  if (Index < 0) or FRoutingProfiles[Index].IsBuiltIn then Exit;
  if SameText(FSelectedRoutingId, FRoutingProfiles[Index].Id) then
    FSelectedRoutingId := RoutingProxyAllId;
  for I := Index to High(FRoutingProfiles) - 1 do
    FRoutingProfiles[I] := FRoutingProfiles[I + 1];
  SetLength(FRoutingProfiles, Length(FRoutingProfiles) - 1);
  RefreshRouting;
end;

procedure TPolicyManagerDialog.AddDnsClick(Sender: TObject);
var
  Editor: TDnsProfileEditor;
  Profile, Empty: TZaryaDnsProfile;
  ErrorMessage: string;
begin
  Empty := BuiltInSystemDns;
  Empty.Id := '';
  Empty.Name := 'Custom DNS';
  Empty.Mode := dmCustom;
  Empty.IsBuiltIn := False;
  Editor := TDnsProfileEditor.Create(Self, Empty, FDarkTheme);
  try
    if (Editor.ShowModal = mrOk) and Editor.CopyProfile(Empty, Profile,
      ErrorMessage) then
    begin
      SetLength(FDnsProfiles, Length(FDnsProfiles) + 1);
      FDnsProfiles[High(FDnsProfiles)] := Profile;
      FSelectedDnsId := Profile.Id;
      RefreshDns;
    end;
  finally
    Editor.Free;
  end;
end;

procedure TPolicyManagerDialog.EditDnsClick(Sender: TObject);
var
  Editor: TDnsProfileEditor;
  Profile: TZaryaDnsProfile;
  ErrorMessage: string;
  Index: Integer;
begin
  Index := DnsIndex;
  if Index < 0 then Exit;
  if FDnsProfiles[Index].IsBuiltIn then
  begin
    MessageDlg('DNS', 'Built-in profile is read-only. Add a custom copy.',
      mtInformation, [mbOK], 0);
    Exit;
  end;
  Editor := TDnsProfileEditor.Create(Self, FDnsProfiles[Index], FDarkTheme);
  try
    if (Editor.ShowModal = mrOk) and Editor.CopyProfile(FDnsProfiles[Index],
      Profile, ErrorMessage) then
    begin
      FDnsProfiles[Index] := Profile;
      RefreshDns;
    end;
  finally
    Editor.Free;
  end;
end;

procedure TPolicyManagerDialog.DeleteDnsClick(Sender: TObject);
var I, Index: Integer;
begin
  Index := DnsIndex;
  if (Index < 0) or FDnsProfiles[Index].IsBuiltIn then Exit;
  if SameText(FSelectedDnsId, FDnsProfiles[Index].Id) then FSelectedDnsId := DnsSystemId;
  for I := Index to High(FDnsProfiles) - 1 do FDnsProfiles[I] := FDnsProfiles[I + 1];
  SetLength(FDnsProfiles, Length(FDnsProfiles) - 1);
  RefreshDns;
end;

class function TPolicyManagerDialog.Execute(AOwner: TComponent;
  var ARoutingProfiles: TZaryaRoutingProfiles;
  var ADnsProfiles: TZaryaDnsProfiles;
  var ASelectedRoutingId, ASelectedDnsId: string;
  const ADarkTheme: Boolean): Boolean;
var
  Dialog: TPolicyManagerDialog;
begin
  Dialog := TPolicyManagerDialog.Create(AOwner, ARoutingProfiles, ADnsProfiles,
    ASelectedRoutingId, ASelectedDnsId, ADarkTheme);
  try
    Result := Dialog.ShowModal = mrOk;
    if Result then
    begin
      ARoutingProfiles := Copy(Dialog.FRoutingProfiles);
      ADnsProfiles := Copy(Dialog.FDnsProfiles);
      ASelectedRoutingId := Dialog.FSelectedRoutingId;
      ASelectedDnsId := Dialog.FSelectedDnsId;
    end;
  finally
    Dialog.Free;
  end;
end;

end.
