unit CoreManagerForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, StdCtrls, ExtCtrls,
  Grids, ZaryaThemes, ZaryaCoreProvider, ZaryaCoreProviderRegistry;

type
  TCoreManagerDialog = class(TForm)
  private
    FRegistry: TZaryaCoreProviderRegistry;
    FDarkTheme: Boolean;
    FGrid: TStringGrid;
    FDetails: TMemo;
    FAddButton: TButton;
    FCheckButton: TButton;
    FChangeButton: TButton;
    FConfigureButton: TButton;
    FConfirmButton: TButton;
    FRemoveButton: TButton;
    FOpenButton: TButton;
    FUseButton: TButton;
    FSelectedProviderId: string;
    procedure BuildInterface;
    procedure RefreshGrid(const ASelectedProviderId: string = '');
    function SelectedIndex: Integer;
    function SelectedProvider(out AProvider: TZaryaCoreProvider): Boolean;
    procedure GridClick(Sender: TObject);
    procedure AddClick(Sender: TObject);
    procedure CheckClick(Sender: TObject);
    procedure ChangeClick(Sender: TObject);
    procedure ConfigureClick(Sender: TObject);
    procedure ConfirmClick(Sender: TObject);
    procedure RemoveClick(Sender: TObject);
    procedure OpenClick(Sender: TObject);
    procedure UseClick(Sender: TObject);
    function SelectTrustedExecutable(out AFileName: string): Boolean;
  public
    constructor Create(AOwner: TComponent;
      const ARegistry: TZaryaCoreProviderRegistry;
      const ADarkTheme: Boolean); reintroduce;
    class function Execute(AOwner: TComponent;
      const ARegistry: TZaryaCoreProviderRegistry; const ADarkTheme: Boolean;
      out AUseProviderId: string): Boolean;
  end;

implementation

uses
  LCLIntf, CustomProviderForm, ZaryaTr;

constructor TCoreManagerDialog.Create(AOwner: TComponent;
  const ARegistry: TZaryaCoreProviderRegistry; const ADarkTheme: Boolean);
var
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  FRegistry := ARegistry;
  FDarkTheme := ADarkTheme;
  Caption := TZaryaTr.Tr('Ядра — Zarya', 'Cores — Zarya');
  Position := poOwnerFormCenter;
  BorderStyle := bsSizeable;
  ClientWidth := 1060;
  ClientHeight := 610;
  Constraints.MinWidth := 860;
  Constraints.MinHeight := 480;
  BuildInterface;
  RefreshGrid;
  if ADarkTheme then
    Theme := ZaryaThemes.DarkTheme
  else
    Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

procedure TCoreManagerDialog.BuildInterface;
var
  Buttons: TPanel;
  CloseButton: TButton;
begin
  Buttons := TPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.Height := 92;
  Buttons.BevelOuter := bvNone;

  FAddButton := TButton.Create(Self);
  FAddButton.Parent := Buttons;
  FAddButton.Caption := TZaryaTr.Tr('Добавить EXE…', 'Add EXE…');
  FAddButton.SetBounds(12, 10, 128, 32);
  FAddButton.OnClick := @AddClick;

  FCheckButton := TButton.Create(Self);
  FCheckButton.Parent := Buttons;
  FCheckButton.Caption := TZaryaTr.Tr('Проверить', 'Check');
  FCheckButton.SetBounds(148, 10, 104, 32);
  FCheckButton.OnClick := @CheckClick;

  FChangeButton := TButton.Create(Self);
  FChangeButton.Parent := Buttons;
  FChangeButton.Caption := TZaryaTr.Tr('Изменить путь…', 'Change path…');
  FChangeButton.SetBounds(260, 10, 136, 32);
  FChangeButton.OnClick := @ChangeClick;

  FConfigureButton := TButton.Create(Self);
  FConfigureButton.Parent := Buttons;
  FConfigureButton.Caption := TZaryaTr.Tr('Настроить custom…',
    'Configure custom…');
  FConfigureButton.SetBounds(404, 10, 152, 32);
  FConfigureButton.OnClick := @ConfigureClick;

  FConfirmButton := TButton.Create(Self);
  FConfirmButton.Parent := Buttons;
  FConfirmButton.Caption := TZaryaTr.Tr('Подтвердить замену',
    'Confirm replacement');
  FConfirmButton.SetBounds(564, 10, 152, 32);
  FConfirmButton.OnClick := @ConfirmClick;

  FRemoveButton := TButton.Create(Self);
  FRemoveButton.Parent := Buttons;
  FRemoveButton.Caption := TZaryaTr.Tr('Удалить регистрацию',
    'Remove registration');
  FRemoveButton.SetBounds(724, 10, 158, 32);
  FRemoveButton.OnClick := @RemoveClick;

  FOpenButton := TButton.Create(Self);
  FOpenButton.Parent := Buttons;
  FOpenButton.Caption := TZaryaTr.Tr('Открыть каталог', 'Open directory');
  FOpenButton.SetBounds(890, 10, 150, 32);
  FOpenButton.OnClick := @OpenClick;

  FUseButton := TButton.Create(Self);
  FUseButton.Parent := Buttons;
  FUseButton.Caption := TZaryaTr.Tr('Для совместимых профилей',
    'Use for compatible profiles');
  FUseButton.SetBounds(12, 50, 210, 32);
  FUseButton.OnClick := @UseClick;

  CloseButton := TButton.Create(Self);
  CloseButton.Parent := Buttons;
  CloseButton.Caption := TZaryaTr.Tr('Закрыть');
  CloseButton.ModalResult := mrCancel;
  CloseButton.SetBounds(920, 50, 120, 32);
  CloseButton.Anchors := [akRight, akBottom];

  FDetails := TMemo.Create(Self);
  FDetails.Parent := Self;
  FDetails.Align := alBottom;
  FDetails.Height := 126;
  FDetails.ReadOnly := True;
  FDetails.ScrollBars := ssAutoVertical;
  FDetails.Font.Name := 'Consolas';
  FDetails.Font.Size := 9;

  FGrid := TStringGrid.Create(Self);
  FGrid.Parent := Self;
  FGrid.Align := alClient;
  FGrid.BorderSpacing.Around := 12;
  FGrid.FixedCols := 0;
  FGrid.FixedRows := 1;
  FGrid.ColCount := 7;
  FGrid.RowCount := 2;
  FGrid.Options := FGrid.Options + [goRowSelect, goColSizing];
  FGrid.OnClick := @GridClick;
  FGrid.Cells[0, 0] := 'Provider ID';
  FGrid.Cells[1, 0] := TZaryaTr.Tr('Ядро', 'Core');
  FGrid.Cells[2, 0] := TZaryaTr.Tr('Поставка', 'Distribution');
  FGrid.Cells[3, 0] := TZaryaTr.Tr('Формат', 'Format');
  FGrid.Cells[4, 0] := TZaryaTr.Tr('Версия', 'Version');
  FGrid.Cells[5, 0] := 'Архитектура / SHA-256';
  FGrid.Cells[6, 0] := TZaryaTr.Tr('Состояние', 'State');
  FGrid.ColWidths[0] := 170;
  FGrid.ColWidths[1] := 170;
  FGrid.ColWidths[2] := 90;
  FGrid.ColWidths[3] := 120;
  FGrid.ColWidths[4] := 160;
  FGrid.ColWidths[5] := 170;
  FGrid.ColWidths[6] := 100;
end;

function TCoreManagerDialog.SelectedIndex: Integer;
begin
  Result := FGrid.Row - 1;
  if (Result < 0) or (Result >= FRegistry.Count) then
    Result := -1;
end;

function TCoreManagerDialog.SelectedProvider(
  out AProvider: TZaryaCoreProvider): Boolean;
var
  Index: Integer;
begin
  Index := SelectedIndex;
  Result := Index >= 0;
  if Result then
    AProvider := FRegistry.ProviderAt(Index)
  else
    AProvider := Default(TZaryaCoreProvider);
end;

procedure TCoreManagerDialog.RefreshGrid(const ASelectedProviderId: string);
var
  I: Integer;
  SelectedRow: Integer;
  Provider: TZaryaCoreProvider;
begin
  FRegistry.RefreshLocalState;
  FGrid.RowCount := FRegistry.Count + 1;
  SelectedRow := 1;
  for I := 0 to FRegistry.Count - 1 do
  begin
    Provider := FRegistry.ProviderAt(I);
    FGrid.Cells[0, I + 1] := Provider.ProviderId;
    FGrid.Cells[1, I + 1] := Provider.DisplayName;
    FGrid.Cells[2, I + 1] := DistributionToString(Provider.Distribution);
    FGrid.Cells[3, I + 1] := ConfigFormatToString(Provider.ConfigFormat);
    FGrid.Cells[4, I + 1] := Provider.Version;
    FGrid.Cells[5, I + 1] := Provider.Architecture;
    if Provider.Sha256 <> '' then
      FGrid.Cells[5, I + 1] := Trim(FGrid.Cells[5, I + 1] + ' ' +
        Copy(Provider.Sha256, 1, 12));
    FGrid.Cells[6, I + 1] := StateToString(Provider.State);
    if SameText(Provider.ProviderId, ASelectedProviderId) then
      SelectedRow := I + 1;
  end;
  if FRegistry.Count > 0 then
    FGrid.Row := SelectedRow;
  GridClick(nil);
end;

procedure TCoreManagerDialog.GridClick(Sender: TObject);
var
  Provider: TZaryaCoreProvider;
  IsExternal: Boolean;
begin
  if not SelectedProvider(Provider) then
  begin
    FDetails.Clear;
    Exit;
  end;
  FDetails.Lines.Text :=
    'Provider: ' + Provider.ProviderId + LineEnding +
    'Adapter: ' + Provider.AdapterId + ' / ' +
      ConfigFormatToString(Provider.ConfigFormat) + LineEnding +
    TZaryaTr.Tr('Протоколы: ', 'Protocols: ') +
      Provider.SupportedProtocols + LineEnding +
    TZaryaTr.Tr('Путь: ', 'Path: ') + Provider.ExecutablePath + LineEnding +
    'SHA-256: ' + Provider.Sha256 + LineEnding +
    TZaryaTr.Tr('Ошибка: ', 'Error: ') + Provider.LastError;
  IsExternal := Provider.Distribution = pdExternal;
  FChangeButton.Enabled := IsExternal;
  FRemoveButton.Enabled := IsExternal;
  FOpenButton.Enabled := IsExternal and (Provider.ExecutablePath <> '');
  FConfirmButton.Enabled := IsExternal and (Provider.State = psChanged);
  FConfigureButton.Enabled := IsExternal and
    (Pos('external.custom.', LowerCase(Provider.ProviderId)) = 1);
  FUseButton.Enabled := Provider.State = psAvailable;
end;

function TCoreManagerDialog.SelectTrustedExecutable(
  out AFileName: string): Boolean;
var
  OpenDialog: TOpenDialog;
begin
  AFileName := '';
  OpenDialog := TOpenDialog.Create(Self);
  try
    OpenDialog.Title := TZaryaTr.Tr('Выберите исполняемый файл ядра',
      'Select a core executable');
    OpenDialog.Filter := TZaryaTr.Tr(
      'Исполняемые файлы Windows (*.exe)|*.exe',
      'Windows executables (*.exe)|*.exe');
    OpenDialog.Options := [ofFileMustExist, ofPathMustExist, ofEnableSizing];
    Result := OpenDialog.Execute;
    if Result then
      AFileName := OpenDialog.FileName;
  finally
    OpenDialog.Free;
  end;
  if Result then
    Result := MessageDlg(TZaryaTr.Tr('Доверие внешнему ядру',
      'Trust external core'), TZaryaTr.Tr(
      'Zarya запустит выбранный EXE без повышения привилегий. ' +
      'Подтвердите, что доверяете источнику файла:',
      'Zarya will run the selected EXE without elevation. ' +
      'Confirm that you trust the source of this file:') + LineEnding +
      AFileName, mtWarning, [mbYes, mbNo], 0) = mrYes;
end;

procedure TCoreManagerDialog.AddClick(Sender: TObject);
var
  FileName: string;
  Provider: TZaryaCoreProvider;
  ErrorMessage: string;
begin
  if not SelectTrustedExecutable(FileName) then
    Exit;
  if not FRegistry.RegisterExecutable(FileName, Provider, ErrorMessage) then
  begin
    MessageDlg(TZaryaTr.Tr('Внешнее ядро', 'External core'), ErrorMessage,
      mtError, [mbOK], 0);
    Exit;
  end;
  if (Pos('external.custom.', LowerCase(Provider.ProviderId)) = 1) and
    TCustomProviderDialog.Execute(Self, Provider, FDarkTheme) then
    if not FRegistry.UpdateProvider(Provider, ErrorMessage) then
      MessageDlg('Custom provider', ErrorMessage, mtError, [mbOK], 0);
  RefreshGrid(Provider.ProviderId);
end;

procedure TCoreManagerDialog.CheckClick(Sender: TObject);
var
  Provider: TZaryaCoreProvider;
  ErrorMessage: string;
begin
  if not SelectedProvider(Provider) then
    Exit;
  if not FRegistry.CheckProvider(Provider.ProviderId, Provider,
    ErrorMessage) then
    MessageDlg(TZaryaTr.Tr('Проверка ядра', 'Core check'), ErrorMessage,
      mtWarning, [mbOK], 0)
  else
    MessageDlg(TZaryaTr.Tr('Проверка ядра', 'Core check'),
      TZaryaTr.Tr('Ядро доступно: ', 'Core is available: ') + Provider.Version,
      mtInformation, [mbOK], 0);
  RefreshGrid(Provider.ProviderId);
end;

procedure TCoreManagerDialog.ChangeClick(Sender: TObject);
var
  Current: TZaryaCoreProvider;
  UpdatedProvider: TZaryaCoreProvider;
  FileName: string;
  ErrorMessage: string;
begin
  if not SelectedProvider(Current) or
    (Current.Distribution <> pdExternal) then
    Exit;
  if not SelectTrustedExecutable(FileName) then
    Exit;
  if not FRegistry.ChangeExecutable(Current.ProviderId, FileName, UpdatedProvider,
    ErrorMessage) then
  begin
    MessageDlg(TZaryaTr.Tr('Изменить путь', 'Change path'), ErrorMessage,
      mtError, [mbOK], 0);
    Exit;
  end;
  RefreshGrid(UpdatedProvider.ProviderId);
end;

procedure TCoreManagerDialog.ConfigureClick(Sender: TObject);
var
  Provider: TZaryaCoreProvider;
  ErrorMessage: string;
begin
  if not SelectedProvider(Provider) or
    (Pos('external.custom.', LowerCase(Provider.ProviderId)) <> 1) then
    Exit;
  if not TCustomProviderDialog.Execute(Self, Provider, FDarkTheme) then
    Exit;
  if not FRegistry.UpdateProvider(Provider, ErrorMessage) then
  begin
    MessageDlg('Custom provider', ErrorMessage, mtError, [mbOK], 0);
    Exit;
  end;
  RefreshGrid(Provider.ProviderId);
end;

procedure TCoreManagerDialog.ConfirmClick(Sender: TObject);
var
  Provider: TZaryaCoreProvider;
  ErrorMessage: string;
begin
  if not SelectedProvider(Provider) then
    Exit;
  if MessageDlg(TZaryaTr.Tr('EXE изменился', 'EXE changed'), TZaryaTr.Tr(
    'SHA-256 файла отличается от подтверждённого. Повторно доверять этому EXE?',
    'The file SHA-256 differs from the confirmed hash. Trust this EXE again?'),
    mtWarning, [mbYes, mbNo], 0) <> mrYes then
    Exit;
  if not FRegistry.ConfirmChanged(Provider.ProviderId, ErrorMessage) then
    MessageDlg(TZaryaTr.Tr('Подтверждение EXE', 'Confirm EXE'), ErrorMessage,
      mtError, [mbOK], 0);
  RefreshGrid(Provider.ProviderId);
end;

procedure TCoreManagerDialog.RemoveClick(Sender: TObject);
var
  Provider: TZaryaCoreProvider;
  ErrorMessage: string;
begin
  if not SelectedProvider(Provider) or
    (Provider.Distribution <> pdExternal) then
    Exit;
  if MessageDlg(TZaryaTr.Tr('Удалить регистрацию', 'Remove registration'),
    TZaryaTr.Tr('Удалить регистрацию «', 'Remove registration “') +
    Provider.DisplayName + TZaryaTr.Tr('»? Сам EXE не удаляется.',
      '”? The EXE itself will not be deleted.'),
    mtConfirmation, [mbYes, mbNo], 0) <> mrYes then
    Exit;
  if not FRegistry.Remove(Provider.ProviderId, ErrorMessage) then
    MessageDlg(TZaryaTr.Tr('Удалить регистрацию', 'Remove registration'),
      ErrorMessage, mtError, [mbOK], 0);
  RefreshGrid;
end;

procedure TCoreManagerDialog.OpenClick(Sender: TObject);
var
  Provider: TZaryaCoreProvider;
begin
  if SelectedProvider(Provider) and (Provider.ExecutablePath <> '') then
    OpenDocument(ExtractFileDir(Provider.ExecutablePath));
end;

procedure TCoreManagerDialog.UseClick(Sender: TObject);
var
  Provider: TZaryaCoreProvider;
begin
  if not SelectedProvider(Provider) or (Provider.State <> psAvailable) then
    Exit;
  FSelectedProviderId := Provider.ProviderId;
  ModalResult := mrOk;
end;

class function TCoreManagerDialog.Execute(AOwner: TComponent;
  const ARegistry: TZaryaCoreProviderRegistry; const ADarkTheme: Boolean;
  out AUseProviderId: string): Boolean;
var
  Dialog: TCoreManagerDialog;
begin
  AUseProviderId := '';
  Dialog := TCoreManagerDialog.Create(AOwner, ARegistry, ADarkTheme);
  try
    Result := Dialog.ShowModal = mrOk;
    if Result then
      AUseProviderId := Dialog.FSelectedProviderId;
  finally
    Dialog.Free;
  end;
end;

end.
