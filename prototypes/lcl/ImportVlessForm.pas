unit ImportVlessForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ButtonPanel, ZaryaThemes;

type
  TImportVlessDialog = class(TForm)
  private
    FLinksMemo: TMemo;
  public
    constructor Create(AOwner: TComponent; const ADarkTheme: Boolean); reintroduce;
    class function Execute(AOwner: TComponent; const ADarkTheme: Boolean;
      out ALinks: string): Boolean;
  end;

implementation

uses
  ZaryaTr;

constructor TImportVlessDialog.Create(AOwner: TComponent;
  const ADarkTheme: Boolean);
var
  IntroLabel: TLabel;
  HintLabel: TLabel;
  Buttons: TButtonPanel;
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  Caption := TZaryaTr.Tr('Импорт share links', 'Import share links');
  BorderStyle := bsSizeable;
  Position := poOwnerFormCenter;
  ClientWidth := 680;
  ClientHeight := 390;
  Constraints.MinWidth := 560;
  Constraints.MinHeight := 320;

  Buttons := TButtonPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.ShowButtons := [pbOK, pbCancel];
  Buttons.ShowGlyphs := [];
  Buttons.OKButton.Caption := TZaryaTr.Tr('Импортировать', 'Import');
  Buttons.CancelButton.Caption := TZaryaTr.Tr('Отмена');

  IntroLabel := TLabel.Create(Self);
  IntroLabel.Parent := Self;
  IntroLabel.Caption := TZaryaTr.Tr(
    'Вставьте share links — по одной в строке (VLESS, VMess, Trojan, SS, SOCKS, Hysteria2, WireGuard).',
    'Paste share links, one per line (VLESS, VMess, Trojan, SS, SOCKS, Hysteria2, WireGuard).');
  IntroLabel.SetBounds(18, 16, 640, 24);

  HintLabel := TLabel.Create(Self);
  HintLabel.Parent := Self;
  HintLabel.Caption := TZaryaTr.Tr(
    'Разбор и сохранение выполняются локально; ссылка не попадает в журнал.',
    'Parsing and storage are local; links are never written to the log.');
  HintLabel.SetBounds(18, 42, 640, 22);

  FLinksMemo := TMemo.Create(Self);
  FLinksMemo.Parent := Self;
  FLinksMemo.ScrollBars := ssAutoBoth;
  FLinksMemo.WordWrap := False;
  FLinksMemo.Font.Name := 'Consolas';
  FLinksMemo.Font.Size := 9;
  FLinksMemo.SetBounds(18, 72, 644, 248);
  FLinksMemo.Anchors := [akLeft, akTop, akRight, akBottom];

  if ADarkTheme then
    Theme := ZaryaThemes.DarkTheme
  else
    Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
  ActiveControl := FLinksMemo;
end;

class function TImportVlessDialog.Execute(AOwner: TComponent;
  const ADarkTheme: Boolean; out ALinks: string): Boolean;
var
  Dialog: TImportVlessDialog;
begin
  ALinks := '';
  Dialog := TImportVlessDialog.Create(AOwner, ADarkTheme);
  try
    Result := Dialog.ShowModal = mrOk;
    if Result then
      ALinks := Dialog.FLinksMemo.Text;
  finally
    Dialog.Free;
  end;
end;

end.
