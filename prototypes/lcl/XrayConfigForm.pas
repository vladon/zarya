unit XrayConfigForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ButtonPanel, ZaryaThemes;

type
  TXrayConfigDialog = class(TForm)
  private
    FJsonMemo: TMemo;
    FConfigExtension: string;
    procedure SaveClick(Sender: TObject);
  public
    constructor Create(AOwner: TComponent; const AProfileName, AConfig,
      AFormatName, AExtension: string; const ADarkTheme: Boolean); reintroduce;
    class procedure Execute(AOwner: TComponent; const AProfileName, AConfig,
      AFormatName, AExtension: string; const ADarkTheme: Boolean);
  end;

implementation

constructor TXrayConfigDialog.Create(AOwner: TComponent;
  const AProfileName, AConfig, AFormatName, AExtension: string;
  const ADarkTheme: Boolean);
var
  Buttons: TButtonPanel;
  IntroLabel: TLabel;
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  Caption := 'Runtime config — ' + AProfileName;
  FConfigExtension := AExtension;
  if FConfigExtension = '' then FConfigExtension := '.conf';
  Position := poOwnerFormCenter;
  ClientWidth := 820;
  ClientHeight := 590;
  Constraints.MinWidth := 620;
  Constraints.MinHeight := 420;

  Buttons := TButtonPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.ShowButtons := [pbOK, pbClose];
  Buttons.ShowGlyphs := [];
  Buttons.OKButton.Caption := 'Сохранить…';
  Buttons.OKButton.ModalResult := mrNone;
  Buttons.OKButton.OnClick := @SaveClick;
  Buttons.CloseButton.Caption := 'Закрыть';

  IntroLabel := TLabel.Create(Self);
  IntroLabel.Parent := Self;
  IntroLabel.Caption :=
    'Формат: ' + AFormatName + '. Core не запускается.';
  IntroLabel.SetBounds(16, 14, 780, 24);

  FJsonMemo := TMemo.Create(Self);
  FJsonMemo.Parent := Self;
  FJsonMemo.ReadOnly := True;
  FJsonMemo.ScrollBars := ssAutoBoth;
  FJsonMemo.WordWrap := False;
  FJsonMemo.Font.Name := 'Consolas';
  FJsonMemo.Font.Size := 9;
  FJsonMemo.Text := AConfig;
  FJsonMemo.SetBounds(16, 44, 788, 478);
  FJsonMemo.Anchors := [akLeft, akTop, akRight, akBottom];

  if ADarkTheme then
    Theme := ZaryaThemes.DarkTheme
  else
    Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

procedure TXrayConfigDialog.SaveClick(Sender: TObject);
var
  SaveDialog: TSaveDialog;
  Stream: TFileStream;
  Json: UTF8String;
begin
  SaveDialog := TSaveDialog.Create(Self);
  try
  SaveDialog.Title := 'Сохранить runtime config';
  SaveDialog.Filter := 'Конфигурация (*' + FConfigExtension + ')|*' +
    FConfigExtension + '|Все файлы (*.*)|*.*';
  SaveDialog.DefaultExt := Copy(FConfigExtension, 2, MaxInt);
  SaveDialog.FileName := 'config' + FConfigExtension;
    if not SaveDialog.Execute then
      Exit;
    try
      Json := UTF8String(FJsonMemo.Text);
      Stream := TFileStream.Create(SaveDialog.FileName, fmCreate);
      try
        if Length(Json) > 0 then
          Stream.WriteBuffer(Json[1], Length(Json));
      finally
        Stream.Free;
      end;
      MessageDlg('Runtime config', 'Конфигурация сохранена.', mtInformation,
        [mbOK], 0);
    except
      on E: Exception do
        MessageDlg('Runtime config', 'Не удалось сохранить конфигурацию:' +
          LineEnding + E.Message, mtError, [mbOK], 0);
    end;
  finally
    SaveDialog.Free;
  end;
end;

class procedure TXrayConfigDialog.Execute(AOwner: TComponent;
  const AProfileName, AConfig, AFormatName, AExtension: string;
  const ADarkTheme: Boolean);
var
  Dialog: TXrayConfigDialog;
begin
  Dialog := TXrayConfigDialog.Create(AOwner, AProfileName, AConfig,
    AFormatName, AExtension, ADarkTheme);
  try
    Dialog.ShowModal;
  finally
    Dialog.Free;
  end;
end;

end.
