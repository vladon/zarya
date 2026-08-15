unit GeoDataManagerForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ComCtrls, ButtonPanel, ZaryaGeoData, ZaryaThemes;

type
  TGeoDataManagerDialog = class(TForm)
  private
    FManager: IGeoDataManager;
    FSourceCombo: TComboBox;
    FStatusList: TListBox;
    FProgress: TProgressBar;
    FUpdateButton: TButton;
    FCancelButton: TButton;
    FTimer: TTimer;
    FThread: TThread;
    FSelectedSourceId: string;
    procedure BuildInterface;
    procedure RefreshStatus;
    procedure UpdateClick(Sender: TObject);
    procedure CancelClick(Sender: TObject);
    procedure TimerTick(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
  public
    constructor Create(AOwner: TComponent; const AManager: IGeoDataManager;
      const ASelectedSourceId: string; const ADarkTheme: Boolean); reintroduce;
    destructor Destroy; override;
    class function Execute(AOwner: TComponent; const AManager: IGeoDataManager;
      var ASelectedSourceId: string; const ADarkTheme: Boolean): Boolean;
  end;

implementation

uses
  ZaryaTr;

type
  TGeoUpdateThread = class(TThread)
  private
    FManager: IGeoDataManager;
    FSourceId: string;
    FCancelFlag: LongInt;
    FDoneFlag: LongInt;
    FDownloaded: Int64;
    FTotal: Int64;
    FSuccess: Boolean;
    FErrorMessage: string;
    procedure Progress(const ADownloaded, ATotal: Int64);
    function CancelRequested: Boolean;
  protected
    procedure Execute; override;
  public
    constructor Create(const AManager: IGeoDataManager;
      const ASourceId: string);
    procedure RequestCancel;
    function IsDone: Boolean;
    property Downloaded: Int64 read FDownloaded;
    property Total: Int64 read FTotal;
    property Success: Boolean read FSuccess;
    property ErrorMessage: string read FErrorMessage;
  end;

constructor TGeoUpdateThread.Create(const AManager: IGeoDataManager;
  const ASourceId: string);
begin
  inherited Create(True);
  FreeOnTerminate := False;
  FManager := AManager;
  FSourceId := ASourceId;
  Start;
end;

procedure TGeoUpdateThread.Progress(const ADownloaded, ATotal: Int64);
begin
  FDownloaded := ADownloaded;
  FTotal := ATotal;
end;

function TGeoUpdateThread.CancelRequested: Boolean;
begin
  Result := InterlockedCompareExchange(FCancelFlag, 0, 0) <> 0;
end;

procedure TGeoUpdateThread.Execute;
begin
  try
    FSuccess := FManager.UpdateAll(FSourceId, @Progress, @CancelRequested,
      FErrorMessage);
  finally
    InterlockedExchange(FDoneFlag, 1);
  end;
end;

procedure TGeoUpdateThread.RequestCancel;
begin
  InterlockedExchange(FCancelFlag, 1);
end;

function TGeoUpdateThread.IsDone: Boolean;
begin
  Result := InterlockedCompareExchange(FDoneFlag, 0, 0) <> 0;
end;

constructor TGeoDataManagerDialog.Create(AOwner: TComponent;
  const AManager: IGeoDataManager; const ASelectedSourceId: string;
  const ADarkTheme: Boolean);
var
  Theme: TZaryaTheme;
begin
  inherited CreateNew(AOwner, 1);
  Caption := 'Geo Data Manager';
  Position := poOwnerFormCenter;
  BorderStyle := bsSizeable;
  ClientWidth := 680;
  ClientHeight := 430;
  Constraints.MinWidth := 560;
  Constraints.MinHeight := 380;
  OnCloseQuery := @FormCloseQuery;
  FManager := AManager;
  FSelectedSourceId := ASelectedSourceId;
  BuildInterface;
  RefreshStatus;
  if ADarkTheme then Theme := ZaryaThemes.DarkTheme
  else Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
end;

destructor TGeoDataManagerDialog.Destroy;
begin
  if Assigned(FThread) then
  begin
    TGeoUpdateThread(FThread).RequestCancel;
    FThread.WaitFor;
    FreeAndNil(FThread);
  end;
  FManager := nil;
  inherited Destroy;
end;

procedure TGeoDataManagerDialog.BuildInterface;
var
  LabelControl: TLabel;
  Buttons: TButtonPanel;
  Source: TZaryaGeoDataSource;
begin
  Buttons := TButtonPanel.Create(Self);
  Buttons.Parent := Self;
  Buttons.Align := alBottom;
  Buttons.ShowButtons := [pbClose];
  Buttons.ShowGlyphs := [];
  Buttons.CloseButton.Caption := TZaryaTr.Tr('Закрыть');
  LabelControl := TLabel.Create(Self);
  LabelControl.Parent := Self;
  LabelControl.Caption := TZaryaTr.Tr('Источник', 'Source');
  LabelControl.SetBounds(16, 18, 90, 24);
  FSourceCombo := TComboBox.Create(Self);
  FSourceCombo.Parent := Self;
  FSourceCombo.Style := csDropDownList;
  FSourceCombo.SetBounds(110, 14, 390, 28);
  for Source in BuiltInGeoDataSources do
  begin
    FSourceCombo.Items.Add(Source.Name);
    if SameText(Source.Id, FSelectedSourceId) then
      FSourceCombo.ItemIndex := FSourceCombo.Items.Count - 1;
  end;
  if FSourceCombo.ItemIndex < 0 then FSourceCombo.ItemIndex := 0;
  FUpdateButton := TButton.Create(Self);
  FUpdateButton.Parent := Self;
  FUpdateButton.Caption := TZaryaTr.Tr('Обновить всё', 'Update all');
  FUpdateButton.SetBounds(510, 13, 145, 30);
  FUpdateButton.OnClick := @UpdateClick;
  FStatusList := TListBox.Create(Self);
  FStatusList.Parent := Self;
  FStatusList.SetBounds(16, 58, ClientWidth - 32, 250);
  FStatusList.Anchors := [akTop, akLeft, akRight, akBottom];
  FProgress := TProgressBar.Create(Self);
  FProgress.Parent := Self;
  FProgress.SetBounds(16, 320, ClientWidth - 190, 24);
  FProgress.Anchors := [akLeft, akRight, akBottom];
  FCancelButton := TButton.Create(Self);
  FCancelButton.Parent := Self;
  FCancelButton.Caption := TZaryaTr.Tr('Отмена');
  FCancelButton.Enabled := False;
  FCancelButton.SetBounds(ClientWidth - 158, 316, 142, 30);
  FCancelButton.Anchors := [akRight, akBottom];
  FCancelButton.OnClick := @CancelClick;
  FTimer := TTimer.Create(Self);
  FTimer.Enabled := False;
  FTimer.Interval := 100;
  FTimer.OnTimer := @TimerTick;
end;

procedure TGeoDataManagerDialog.RefreshStatus;
var
  Statuses: TZaryaGeoDataStatuses;
  Status: TZaryaGeoDataStatus;
  ErrorMessage, Line: string;
begin
  FStatusList.Clear;
  FStatusList.Items.Add(TZaryaTr.Tr('Каталог: ', 'Directory: ') +
    FManager.TargetDirectory);
  if not FManager.CheckAll(Statuses, ErrorMessage) then
  begin
    FStatusList.Items.Add(TZaryaTr.Tr('Ошибка: ', 'Error: ') + ErrorMessage);
    Exit;
  end;
  for Status in Statuses do
  begin
    Line := Status.FileName + ': ';
    if not Status.Exists then Line := Line + TZaryaTr.Tr('отсутствует', 'missing')
    else if Status.Verified then Line := Line + TZaryaTr.Tr('проверен', 'verified')
    else Line := Line + TZaryaTr.Tr('присутствует, checksum не подтверждён',
      'present, checksum not verified');
    if Status.SizeBytes > 0 then Line := Line +
      Format(' (%d bytes)', [Status.SizeBytes]);
    FStatusList.Items.Add(Line);
  end;
end;

procedure TGeoDataManagerDialog.UpdateClick(Sender: TObject);
var
  Sources: TZaryaGeoDataSources;
begin
  if Assigned(FThread) then Exit;
  Sources := BuiltInGeoDataSources;
  if (FSourceCombo.ItemIndex < 0) or
    (FSourceCombo.ItemIndex > High(Sources)) then Exit;
  FSelectedSourceId := Sources[FSourceCombo.ItemIndex].Id;
  FProgress.Position := 0;
  FUpdateButton.Enabled := False;
  FCancelButton.Enabled := True;
  FThread := TGeoUpdateThread.Create(FManager, FSelectedSourceId);
  FTimer.Enabled := True;
end;

procedure TGeoDataManagerDialog.CancelClick(Sender: TObject);
begin
  if Assigned(FThread) then TGeoUpdateThread(FThread).RequestCancel;
  FCancelButton.Enabled := False;
end;

procedure TGeoDataManagerDialog.TimerTick(Sender: TObject);
var
  Thread: TGeoUpdateThread;
  Percent: Int64;
  Success: Boolean;
  ErrorMessage: string;
begin
  if not Assigned(FThread) then Exit;
  Thread := TGeoUpdateThread(FThread);
  if Thread.Total > 0 then
  begin
    Percent := Thread.Downloaded * 100 div Thread.Total;
    if Percent > 100 then Percent := 100;
    FProgress.Position := Percent;
  end;
  if not Thread.IsDone then Exit;
  FTimer.Enabled := False;
  Thread.WaitFor;
  Success := Thread.Success;
  ErrorMessage := Thread.ErrorMessage;
  FreeAndNil(FThread);
  FUpdateButton.Enabled := True;
  FCancelButton.Enabled := False;
  RefreshStatus;
  if Success then
    MessageDlg('Geo data', TZaryaTr.Tr('Geo data обновлены и проверены.',
      'Geo data was updated and verified.'),
      mtInformation, [mbOK], 0)
  else
    MessageDlg('Geo data', TZaryaTr.Tr('Обновление не выполнено:',
      'The update did not complete:') + LineEnding +
      ErrorMessage, mtWarning, [mbOK], 0);
end;

procedure TGeoDataManagerDialog.FormCloseQuery(Sender: TObject;
  var CanClose: Boolean);
begin
  CanClose := not Assigned(FThread);
  if not CanClose then
  begin
    TGeoUpdateThread(FThread).RequestCancel;
    MessageDlg('Geo data',
      TZaryaTr.Tr(
        'Отмена отправлена. Дождитесь завершения текущей сетевой операции.',
        'Cancellation was requested. Wait for the current network operation to finish.'),
      mtInformation, [mbOK], 0);
  end;
end;

class function TGeoDataManagerDialog.Execute(AOwner: TComponent;
  const AManager: IGeoDataManager; var ASelectedSourceId: string;
  const ADarkTheme: Boolean): Boolean;
var
  Dialog: TGeoDataManagerDialog;
begin
  Dialog := TGeoDataManagerDialog.Create(AOwner, AManager,
    ASelectedSourceId, ADarkTheme);
  try
    Result := Dialog.ShowModal = mrClose;
    ASelectedSourceId := Dialog.FSelectedSourceId;
  finally
    Dialog.Free;
  end;
end;

end.
