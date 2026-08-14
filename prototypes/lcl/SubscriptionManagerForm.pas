unit SubscriptionManagerForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, StdCtrls, ExtCtrls, Grids, Dialogs,
  ZaryaProfile, ZaryaProfileStore, ZaryaSubscription,
  FpcSubscriptionStore, WindowsSubscriptionDownloader;

type
  TSubscriptionDownloadThread = class(TThread)
  private
    FUrl: string;
    FUserAgent: string;
    FCancelFlag: LongInt;
    FDoneFlag: LongInt;
    FDownloaded: Int64;
    FTotal: Int64;
    FResult: TZaryaSubscriptionDownloadResult;
    procedure Progress(const ADownloaded, ATotal: Int64);
    function CancelRequested: Boolean;
  protected
    procedure Execute; override;
  public
    constructor Create(const AUrl, AUserAgent: string);
    procedure RequestCancel;
    function IsDone: Boolean;
    property Downloaded: Int64 read FDownloaded;
    property Total: Int64 read FTotal;
    property DownloadResult: TZaryaSubscriptionDownloadResult read FResult;
  end;

  TSubscriptionManagerDialog = class(TForm)
  private
    FProfiles: TZaryaProfiles;
    FSubscriptions: TZaryaSubscriptions;
    FProfileStore: IZaryaProfileStore;
    FSubscriptionStore: TFpcSubscriptionStore;
    FGrid: TStringGrid;
    FSummary: TLabel;
    FProgress: TLabel;
    FAddButton: TButton;
    FEditButton: TButton;
    FDeleteButton: TButton;
    FUpdateButton: TButton;
    FUpdateAllButton: TButton;
    FCancelButton: TButton;
    FCloseButton: TButton;
    FTimer: TTimer;
    FWorker: TSubscriptionDownloadThread;
    FCurrentIndex: Integer;
    FQueue: array of Integer;
    FQueuePosition: Integer;
    FProfilesChanged: Boolean;
    FLogCallback: TGetStrProc;
    procedure AddClick(Sender: TObject);
    procedure EditClick(Sender: TObject);
    procedure DeleteClick(Sender: TObject);
    procedure UpdateClick(Sender: TObject);
    procedure UpdateAllClick(Sender: TObject);
    procedure CancelClick(Sender: TObject);
    procedure CloseClick(Sender: TObject);
    procedure TimerTick(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
    function SelectedIndex: Integer;
    procedure RefreshGrid(const ASelectIndex: Integer = -1);
    procedure UpdateControls;
    procedure StartNextDownload;
    procedure FinishDownload;
    function SaveSubscriptions: Boolean;
    function SaveTransaction(const ASubscriptions: TZaryaSubscriptions;
      const AProfiles: TZaryaProfiles; out AError: string): Boolean;
    procedure WriteLog(const AText: string);
  public
    constructor Create(AOwner: TComponent; const AProfiles: TZaryaProfiles;
      const AProfileStore: IZaryaProfileStore; const ADataDirectory: string;
      const ALogCallback: TGetStrProc); reintroduce;
    destructor Destroy; override;
    procedure CopyProfiles(out AProfiles: TZaryaProfiles);
    property ProfilesChanged: Boolean read FProfilesChanged;
  end;

implementation

uses
  DateUtils, SubscriptionEditForm, ZaryaSubscriptionParser,
  ZaryaSubscriptionService;

function CloneProfiles(const ASource: TZaryaProfiles): TZaryaProfiles;
var
  I: Integer;
begin
  Result := nil;
  SetLength(Result, Length(ASource));
  for I := 0 to High(ASource) do Result[I] := ASource[I];
end;

function CloneSubscriptions(const ASource: TZaryaSubscriptions): TZaryaSubscriptions;
var
  I: Integer;
begin
  Result := nil;
  SetLength(Result, Length(ASource));
  for I := 0 to High(ASource) do Result[I] := ASource[I];
end;

constructor TSubscriptionDownloadThread.Create(const AUrl,
  AUserAgent: string);
begin
  inherited Create(True);
  FreeOnTerminate := False;
  FUrl := AUrl;
  FUserAgent := AUserAgent;
  FCancelFlag := 0;
  FDoneFlag := 0;
  Start;
end;

procedure TSubscriptionDownloadThread.Progress(const ADownloaded,
  ATotal: Int64);
begin
  FDownloaded := ADownloaded;
  FTotal := ATotal;
end;

function TSubscriptionDownloadThread.CancelRequested: Boolean;
begin
  Result := InterlockedCompareExchange(FCancelFlag, 0, 0) <> 0;
end;

procedure TSubscriptionDownloadThread.Execute;
begin
  try
    FResult := DownloadSubscriptionWinHttp(FUrl, FUserAgent, 20000,
      @Progress, @CancelRequested);
  except
    on E: Exception do
    begin
      FResult := Default(TZaryaSubscriptionDownloadResult);
      FResult.ErrorMessage := E.Message;
    end;
  end;
  InterlockedExchange(FDoneFlag, 1);
end;

procedure TSubscriptionDownloadThread.RequestCancel;
begin
  InterlockedExchange(FCancelFlag, 1);
end;

function TSubscriptionDownloadThread.IsDone: Boolean;
begin
  Result := InterlockedCompareExchange(FDoneFlag, 0, 0) <> 0;
end;

constructor TSubscriptionManagerDialog.Create(AOwner: TComponent;
  const AProfiles: TZaryaProfiles; const AProfileStore: IZaryaProfileStore;
  const ADataDirectory: string; const ALogCallback: TGetStrProc);
var
  ErrorMessage: string;
begin
  inherited CreateNew(AOwner, 1);
  Caption := 'Подписки';
  Position := poOwnerFormCenter;
  ClientWidth := 920;
  ClientHeight := 520;
  Constraints.MinWidth := 760;
  Constraints.MinHeight := 420;
  OnCloseQuery := @FormCloseQuery;
  FProfiles := CloneProfiles(AProfiles);
  FProfileStore := AProfileStore;
  FLogCallback := ALogCallback;
  FCurrentIndex := -1;
  FSubscriptionStore := TFpcSubscriptionStore.Create(
    IncludeTrailingPathDelimiter(ADataDirectory) + 'subscriptions.json');
  if not FSubscriptionStore.Load(FSubscriptions, ErrorMessage) then
    MessageDlg('Подписки', 'Не удалось прочитать subscriptions.json:' +
      LineEnding + ErrorMessage, mtError, [mbOK], 0);

  FGrid := TStringGrid.Create(Self);
  FGrid.Parent := Self;
  FGrid.Align := alTop;
  FGrid.Height := 340;
  FGrid.FixedRows := 1;
  FGrid.FixedCols := 0;
  FGrid.ColCount := 5;
  FGrid.RowCount := 2;
  FGrid.Options := FGrid.Options + [goRowSelect];
  FGrid.Cells[0, 0] := 'Название';
  FGrid.Cells[1, 0] := 'Состояние';
  FGrid.Cells[2, 0] := 'Профили';
  FGrid.Cells[3, 0] := 'Обновлена';
  FGrid.Cells[4, 0] := 'Включена';
  FGrid.ColWidths[0] := 270;
  FGrid.ColWidths[1] := 130;
  FGrid.ColWidths[2] := 80;
  FGrid.ColWidths[3] := 230;
  FGrid.ColWidths[4] := 90;

  FSummary := TLabel.Create(Self);
  FSummary.Parent := Self;
  FSummary.SetBounds(20, 354, 880, 24);
  FProgress := TLabel.Create(Self);
  FProgress.Parent := Self;
  FProgress.SetBounds(20, 382, 880, 24);

  FAddButton := TButton.Create(Self);
  FAddButton.Parent := Self;
  FAddButton.Caption := 'Добавить';
  FAddButton.OnClick := @AddClick;
  FAddButton.SetBounds(20, 430, 100, 32);
  FEditButton := TButton.Create(Self);
  FEditButton.Parent := Self;
  FEditButton.Caption := 'Изменить';
  FEditButton.OnClick := @EditClick;
  FEditButton.SetBounds(130, 430, 100, 32);
  FDeleteButton := TButton.Create(Self);
  FDeleteButton.Parent := Self;
  FDeleteButton.Caption := 'Удалить';
  FDeleteButton.OnClick := @DeleteClick;
  FDeleteButton.SetBounds(240, 430, 100, 32);
  FUpdateButton := TButton.Create(Self);
  FUpdateButton.Parent := Self;
  FUpdateButton.Caption := 'Обновить';
  FUpdateButton.OnClick := @UpdateClick;
  FUpdateButton.SetBounds(420, 430, 100, 32);
  FUpdateAllButton := TButton.Create(Self);
  FUpdateAllButton.Parent := Self;
  FUpdateAllButton.Caption := 'Обновить все';
  FUpdateAllButton.OnClick := @UpdateAllClick;
  FUpdateAllButton.SetBounds(530, 430, 110, 32);
  FCancelButton := TButton.Create(Self);
  FCancelButton.Parent := Self;
  FCancelButton.Caption := 'Отменить';
  FCancelButton.OnClick := @CancelClick;
  FCancelButton.SetBounds(650, 430, 100, 32);
  FCloseButton := TButton.Create(Self);
  FCloseButton.Parent := Self;
  FCloseButton.Caption := 'Закрыть';
  FCloseButton.Cancel := True;
  FCloseButton.OnClick := @CloseClick;
  FCloseButton.SetBounds(800, 430, 100, 32);
  FTimer := TTimer.Create(Self);
  FTimer.Enabled := False;
  FTimer.Interval := 100;
  FTimer.OnTimer := @TimerTick;
  RefreshGrid;
  UpdateControls;
end;

destructor TSubscriptionManagerDialog.Destroy;
begin
  if Assigned(FWorker) then
  begin
    FWorker.RequestCancel;
    FWorker.WaitFor;
    FreeAndNil(FWorker);
  end;
  FSubscriptionStore.Free;
  inherited Destroy;
end;

procedure TSubscriptionManagerDialog.CopyProfiles(out AProfiles: TZaryaProfiles);
begin
  AProfiles := CloneProfiles(FProfiles);
end;

procedure TSubscriptionManagerDialog.WriteLog(const AText: string);
begin
  if Assigned(FLogCallback) then FLogCallback(AText);
end;

function TSubscriptionManagerDialog.SelectedIndex: Integer;
begin
  Result := FGrid.Row - 1;
  if (Result < 0) or (Result > High(FSubscriptions)) then Result := -1;
end;

procedure TSubscriptionManagerDialog.RefreshGrid(const ASelectIndex: Integer);
var
  I: Integer;
  SelectIndex: Integer;
begin
  FGrid.RowCount := Length(FSubscriptions) + 1;
  if FGrid.RowCount < 2 then FGrid.RowCount := 2;
  for I := 1 to FGrid.RowCount - 1 do
  begin
    FGrid.Rows[I].Clear;
    if I - 1 > High(FSubscriptions) then Continue;
    FGrid.Cells[0, I] := FSubscriptions[I - 1].Name;
    FGrid.Cells[1, I] := SubscriptionStatusDisplayName(
      FSubscriptions[I - 1].LastStatus);
    FGrid.Cells[2, I] := IntToStr(FSubscriptions[I - 1].ProfileCount);
    FGrid.Cells[3, I] := FSubscriptions[I - 1].LastUpdatedAt;
    if FSubscriptions[I - 1].Enabled then
      FGrid.Cells[4, I] := 'Да'
    else
      FGrid.Cells[4, I] := 'Нет';
  end;
  FSummary.Caption := Format('Подписок: %d. URL не показываются в таблице и журнале.',
    [Length(FSubscriptions)]);
  SelectIndex := ASelectIndex;
  if SelectIndex < 0 then SelectIndex := SelectedIndex;
  if (SelectIndex >= 0) and (SelectIndex <= High(FSubscriptions)) then
    FGrid.Row := SelectIndex + 1;
end;

procedure TSubscriptionManagerDialog.UpdateControls;
var
  Busy: Boolean;
begin
  Busy := Assigned(FWorker);
  FAddButton.Enabled := not Busy;
  FEditButton.Enabled := not Busy;
  FDeleteButton.Enabled := not Busy;
  FUpdateButton.Enabled := not Busy;
  FUpdateAllButton.Enabled := not Busy;
  FCancelButton.Enabled := Busy;
  FCloseButton.Enabled := not Busy;
end;

function TSubscriptionManagerDialog.SaveSubscriptions: Boolean;
var
  ErrorMessage: string;
begin
  Result := FSubscriptionStore.Save(FSubscriptions, ErrorMessage);
  if not Result then
    MessageDlg('Подписки', 'Не удалось сохранить subscriptions.json:' +
      LineEnding + ErrorMessage, mtError, [mbOK], 0);
end;

function TSubscriptionManagerDialog.SaveTransaction(
  const ASubscriptions: TZaryaSubscriptions; const AProfiles: TZaryaProfiles;
  out AError: string): Boolean;
var
  RollbackError: string;
begin
  if not FProfileStore.Save(AProfiles, AError) then Exit(False);
  if FSubscriptionStore.Save(ASubscriptions, AError) then Exit(True);
  if not FProfileStore.Save(FProfiles, RollbackError) then
    AError := AError + LineEnding + 'Не удалось откатить profiles.json: ' +
      RollbackError;
  Result := False;
end;

procedure TSubscriptionManagerDialog.AddClick(Sender: TObject);
var
  Subscription: TZaryaSubscription;
  Index: Integer;
begin
  Subscription := CreateEmptySubscription;
  if not TSubscriptionEditDialog.Edit(Self, Subscription) then Exit;
  Index := Length(FSubscriptions);
  SetLength(FSubscriptions, Index + 1);
  FSubscriptions[Index] := Subscription;
  if not SaveSubscriptions then SetLength(FSubscriptions, Index);
  RefreshGrid(Index);
end;

procedure TSubscriptionManagerDialog.EditClick(Sender: TObject);
var
  Index: Integer;
  Original: TZaryaSubscription;
begin
  Index := SelectedIndex;
  if Index < 0 then
  begin
    MessageDlg('Подписки', 'Сначала выберите подписку.', mtInformation,
      [mbOK], 0);
    Exit;
  end;
  Original := FSubscriptions[Index];
  if not TSubscriptionEditDialog.Edit(Self, FSubscriptions[Index]) then Exit;
  if not SaveSubscriptions then FSubscriptions[Index] := Original;
  RefreshGrid(Index);
end;

procedure TSubscriptionManagerDialog.DeleteClick(Sender: TObject);
var
  Index: Integer;
  I: Integer;
  NewIndex: Integer;
  Answer: TModalResult;
  Subscription: TZaryaSubscription;
  CandidateProfiles: TZaryaProfiles;
  CandidateSubscriptions: TZaryaSubscriptions;
  ErrorMessage: string;
begin
  Index := SelectedIndex;
  if Index < 0 then
  begin
    MessageDlg('Подписки', 'Сначала выберите подписку.', mtInformation,
      [mbOK], 0);
    Exit;
  end;
  Subscription := FSubscriptions[Index];
  Answer := MessageDlg('Удалить подписку',
    'Удалить подписку «' + Subscription.Name + '»?' + LineEnding +
    'Да — вместе с профилями; Нет — оставить профили как ручные.',
    mtConfirmation, [mbYes, mbNo, mbCancel], 0);
  if Answer = mrCancel then Exit;
  CandidateProfiles := CloneProfiles(FProfiles);
  NewIndex := 0;
  for I := 0 to High(CandidateProfiles) do
    if CandidateProfiles[I].SubscriptionId = Subscription.Id then
    begin
      if Answer = mrNo then
      begin
        CandidateProfiles[I].SourceType := 'manual';
        CandidateProfiles[I].Source := 'Вручную';
        CandidateProfiles[I].SubscriptionId := '';
        CandidateProfiles[I].SubscriptionName := '';
        CandidateProfiles[I].DeletedBySubscriptionUpdate := False;
        CandidateProfiles[NewIndex] := CandidateProfiles[I];
        Inc(NewIndex);
      end;
    end
    else
    begin
      CandidateProfiles[NewIndex] := CandidateProfiles[I];
      Inc(NewIndex);
    end;
  SetLength(CandidateProfiles, NewIndex);
  SetLength(CandidateSubscriptions, Length(FSubscriptions) - 1);
  NewIndex := 0;
  for I := 0 to High(FSubscriptions) do
    if I <> Index then
    begin
      CandidateSubscriptions[NewIndex] := FSubscriptions[I];
      Inc(NewIndex);
    end;
  if not SaveTransaction(CandidateSubscriptions, CandidateProfiles,
    ErrorMessage) then
  begin
    MessageDlg('Подписки', 'Не удалось сохранить удаление:' + LineEnding +
      ErrorMessage, mtError, [mbOK], 0);
    Exit;
  end;
  FSubscriptions := CandidateSubscriptions;
  FProfiles := CandidateProfiles;
  FProfilesChanged := True;
  RefreshGrid;
end;

procedure TSubscriptionManagerDialog.UpdateClick(Sender: TObject);
var
  Index: Integer;
begin
  Index := SelectedIndex;
  if Index < 0 then
  begin
    MessageDlg('Подписки', 'Сначала выберите подписку.', mtInformation,
      [mbOK], 0);
    Exit;
  end;
  SetLength(FQueue, 1);
  FQueue[0] := Index;
  FQueuePosition := 0;
  StartNextDownload;
end;

procedure TSubscriptionManagerDialog.UpdateAllClick(Sender: TObject);
var
  I: Integer;
  Count: Integer;
begin
  Count := 0;
  SetLength(FQueue, Length(FSubscriptions));
  for I := 0 to High(FSubscriptions) do
    if FSubscriptions[I].Enabled then
    begin
      FQueue[Count] := I;
      Inc(Count);
    end
    else
      FSubscriptions[I].LastStatus := ssDisabled;
  SetLength(FQueue, Count);
  if Count = 0 then
  begin
    MessageDlg('Подписки', 'Нет включённых подписок.', mtInformation,
      [mbOK], 0);
    Exit;
  end;
  FQueuePosition := 0;
  StartNextDownload;
end;

procedure TSubscriptionManagerDialog.StartNextDownload;
var
  UserAgent: string;
begin
  if FQueuePosition > High(FQueue) then
  begin
    SetLength(FQueue, 0);
    FProgress.Caption := 'Обновление подписок завершено.';
    UpdateControls;
    Exit;
  end;
  FCurrentIndex := FQueue[FQueuePosition];
  Inc(FQueuePosition);
  if not FSubscriptions[FCurrentIndex].Enabled then
  begin
    FSubscriptions[FCurrentIndex].LastStatus := ssDisabled;
    StartNextDownload;
    Exit;
  end;
  UserAgent := Trim(FSubscriptions[FCurrentIndex].UserAgent);
  if UserAgent = '' then UserAgent := 'Zarya-LCL/1.0';
  FSubscriptions[FCurrentIndex].LastStatus := ssUpdating;
  FSubscriptions[FCurrentIndex].LastError := '';
  RefreshGrid(FCurrentIndex);
  FProgress.Caption := 'Загрузка «' + FSubscriptions[FCurrentIndex].Name + '»…';
  WriteLog('Subscription update started: ' +
    FSubscriptions[FCurrentIndex].Name);
  FWorker := TSubscriptionDownloadThread.Create(
    FSubscriptions[FCurrentIndex].Url, UserAgent);
  FTimer.Enabled := True;
  UpdateControls;
end;

procedure TSubscriptionManagerDialog.FinishDownload;
var
  Download: TZaryaSubscriptionDownloadResult;
  Parsed: TZaryaSubscriptionParseResult;
  CandidateProfiles: TZaryaProfiles;
  CandidateSubscriptions: TZaryaSubscriptions;
  Stats: TZaryaSubscriptionUpdateStats;
  ErrorMessage: string;
  Index: Integer;
begin
  FTimer.Enabled := False;
  FWorker.WaitFor;
  Download := FWorker.DownloadResult;
  FreeAndNil(FWorker);
  Index := FCurrentIndex;
  if not Download.Success then
  begin
    FSubscriptions[Index].LastStatus := ssFailed;
    FSubscriptions[Index].LastError := Download.ErrorMessage;
    SaveSubscriptions;
    FProgress.Caption := 'Ошибка «' + FSubscriptions[Index].Name + '»: ' +
      Download.ErrorMessage;
    WriteLog('Subscription update failed: ' + Download.ErrorMessage);
    RefreshGrid(Index);
    StartNextDownload;
    Exit;
  end;
  Parsed := ParseSubscriptionContent(Download.Body);
  if not Parsed.Success then
  begin
    FSubscriptions[Index].LastStatus := ssFailed;
    FSubscriptions[Index].LastError := Parsed.ErrorMessage;
    SaveSubscriptions;
    FProgress.Caption := 'Ошибка «' + FSubscriptions[Index].Name + '»: ' +
      Parsed.ErrorMessage;
    WriteLog('Subscription parse failed: ' + Parsed.ErrorMessage);
    RefreshGrid(Index);
    StartNextDownload;
    Exit;
  end;
  CandidateProfiles := CloneProfiles(FProfiles);
  CandidateSubscriptions := CloneSubscriptions(FSubscriptions);
  if not MergeSubscriptionProfiles(CandidateSubscriptions[Index],
    Parsed.Profiles, CandidateProfiles, Stats, ErrorMessage) then
  begin
    FSubscriptions[Index].LastStatus := ssFailed;
    FSubscriptions[Index].LastError := ErrorMessage;
    SaveSubscriptions;
    FProgress.Caption := 'Ошибка merge: ' + ErrorMessage;
    WriteLog('Subscription merge failed: ' + ErrorMessage);
    RefreshGrid(Index);
    StartNextDownload;
    Exit;
  end;
  Stats.SkippedLines := Parsed.SkippedLines;
  if not SaveTransaction(CandidateSubscriptions, CandidateProfiles,
    ErrorMessage) then
  begin
    FSubscriptions[Index].LastStatus := ssFailed;
    FSubscriptions[Index].LastError := ErrorMessage;
    SaveSubscriptions;
    FProgress.Caption := 'Ошибка сохранения: ' + ErrorMessage;
    WriteLog('Subscription save failed: ' + ErrorMessage);
    RefreshGrid(Index);
    StartNextDownload;
    Exit;
  end;
  FSubscriptions := CandidateSubscriptions;
  FProfiles := CandidateProfiles;
  FProfilesChanged := True;
  FProgress.Caption := Format(
    '«%s»: добавлено %d, обновлено %d, отсутствует %d, пропущено %d.',
    [FSubscriptions[Index].Name, Stats.AddedProfiles,
     Stats.UpdatedProfiles, Stats.MarkedMissingProfiles,
     Stats.SkippedLines]);
  WriteLog(Format('Subscription update success: added=%d updated=%d missing=%d skipped=%d',
    [Stats.AddedProfiles, Stats.UpdatedProfiles,
     Stats.MarkedMissingProfiles, Stats.SkippedLines]));
  RefreshGrid(Index);
  StartNextDownload;
end;

procedure TSubscriptionManagerDialog.TimerTick(Sender: TObject);
var
  TotalText: string;
begin
  if not Assigned(FWorker) then Exit;
  if FWorker.IsDone then
  begin
    FinishDownload;
    Exit;
  end;
  if FWorker.Total > 0 then
    TotalText := ' / ' + IntToStr(FWorker.Total)
  else
    TotalText := '';
  FProgress.Caption := Format('Загрузка «%s»: %d%s байт',
    [FSubscriptions[FCurrentIndex].Name, FWorker.Downloaded, TotalText]);
end;

procedure TSubscriptionManagerDialog.CancelClick(Sender: TObject);
begin
  SetLength(FQueue, 0);
  FQueuePosition := 0;
  if Assigned(FWorker) then
  begin
    FWorker.RequestCancel;
    FProgress.Caption := 'Отмена текущей загрузки…';
  end;
end;

procedure TSubscriptionManagerDialog.CloseClick(Sender: TObject);
begin
  ModalResult := mrClose;
end;

procedure TSubscriptionManagerDialog.FormCloseQuery(Sender: TObject;
  var CanClose: Boolean);
begin
  CanClose := not Assigned(FWorker);
  if not CanClose then
    MessageDlg('Подписки', 'Сначала отмените или дождитесь текущего обновления.',
      mtInformation, [mbOK], 0);
end;

end.
