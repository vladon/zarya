unit ZaryaRealDelay;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, ZaryaRuntimeContracts;

type
  TZaryaRealDelayWorkItem = record
    ProfileId: string;
    ProfileName: string;
    Prepared: Boolean;
    PreparationError: string;
    Request: TZaryaNodeTestRequest;
  end;

  TZaryaRealDelayWorkItems = array of TZaryaRealDelayWorkItem;

  TZaryaRealDelayResult = record
    ProfileId: string;
    Tested: Boolean;
    Success: Boolean;
    DelayMs: Int64;
    ErrorCode: string;
    ErrorMessage: string;
  end;

  TZaryaRealDelayResults = array of TZaryaRealDelayResult;
  TZaryaRealDelayBatchThread = class;

  TZaryaRealDelaySlotThread = class(TThread)
  private
    FOwner: TZaryaRealDelayBatchThread;
    FSlotIndex: Integer;
  protected
    procedure Execute; override;
  public
    constructor Create(AOwner: TZaryaRealDelayBatchThread;
      const ASlotIndex: Integer);
  end;

  TZaryaRealDelayBatchThread = class(TThread)
  private
    FItems: TZaryaRealDelayWorkItems;
    FResults: TZaryaRealDelayResults;
    FSlots: array of TZaryaRealDelaySlotThread;
    FActiveWorkers: array of INodeTestWorker;
    FCurrentNames: array of string;
    FLock: TRTLCriticalSection;
    FNextIndex: Integer;
    FCompletedCount: LongInt;
    FDoneFlag: LongInt;
    FCancelFlag: LongInt;
    FConcurrency: Integer;
    function TakeNext(const ASlotIndex: Integer; out AIndex: Integer;
      out AItem: TZaryaRealDelayWorkItem): Boolean;
    procedure SetActiveWorker(const ASlotIndex: Integer;
      const AWorker: INodeTestWorker; const AProfileName: string);
    procedure StoreResult(const AIndex: Integer;
      const AResult: TZaryaNodeTestResult);
  protected
    procedure Execute; override;
  public
    constructor Create(const AItems: TZaryaRealDelayWorkItems;
      const AConcurrency: Integer);
    destructor Destroy; override;
    procedure RequestCancel;
    function IsDone: Boolean;
    function ProgressText: string;
    function CompletedCount: Integer;
    property Results: TZaryaRealDelayResults read FResults;
  end;

implementation

uses
  ZaryaNodeTestWorker;

constructor TZaryaRealDelaySlotThread.Create(
  AOwner: TZaryaRealDelayBatchThread; const ASlotIndex: Integer);
begin
  inherited Create(True);
  FreeOnTerminate := False;
  FOwner := AOwner;
  FSlotIndex := ASlotIndex;
end;

procedure TZaryaRealDelaySlotThread.Execute;
var
  Index: Integer;
  Item: TZaryaRealDelayWorkItem;
  Worker: INodeTestWorker;
  TestResult: TZaryaNodeTestResult;
  WorkerLog: string;
begin
  while FOwner.TakeNext(FSlotIndex, Index, Item) do
  begin
    if not Item.Prepared then
    begin
      TestResult := Default(TZaryaNodeTestResult);
      TestResult.Success := False;
      TestResult.ErrorCode := 'preparation_failed';
      TestResult.MessageText := Item.PreparationError;
      TestResult.DelayMs := -1;
    end
    else
    begin
      Worker := TZaryaNodeTestWorker.Create;
      FOwner.SetActiveWorker(FSlotIndex, Worker, Item.ProfileName);
      try
        Worker.Run(Item.Request, TestResult, WorkerLog);
      finally
        FOwner.SetActiveWorker(FSlotIndex, nil, '');
        Worker := nil;
      end;
    end;
    FOwner.StoreResult(Index, TestResult);
    FOwner.SetActiveWorker(FSlotIndex, nil, '');
  end;
end;

constructor TZaryaRealDelayBatchThread.Create(
  const AItems: TZaryaRealDelayWorkItems; const AConcurrency: Integer);
var
  I: Integer;
begin
  inherited Create(True);
  FreeOnTerminate := False;
  InitCriticalSection(FLock);
  FItems := Copy(AItems);
  SetLength(FResults, Length(FItems));
  for I := 0 to High(FItems) do FResults[I].ProfileId := FItems[I].ProfileId;
  FConcurrency := AConcurrency;
  if FConcurrency < 1 then FConcurrency := 1;
  if FConcurrency > 10 then FConcurrency := 10;
  if FConcurrency > Length(FItems) then FConcurrency := Length(FItems);
  SetLength(FActiveWorkers, FConcurrency);
  SetLength(FCurrentNames, FConcurrency);
  Start;
end;

destructor TZaryaRealDelayBatchThread.Destroy;
begin
  RequestCancel;
  if not Finished then WaitFor;
  DoneCriticalSection(FLock);
  inherited Destroy;
end;

function TZaryaRealDelayBatchThread.TakeNext(const ASlotIndex: Integer;
  out AIndex: Integer; out AItem: TZaryaRealDelayWorkItem): Boolean;
begin
  Result := False;
  if InterlockedCompareExchange(FCancelFlag, 0, 0) <> 0 then Exit;
  EnterCriticalSection(FLock);
  try
    if FNextIndex >= Length(FItems) then Exit;
    AIndex := FNextIndex;
    Inc(FNextIndex);
    AItem := FItems[AIndex];
    if (ASlotIndex >= 0) and (ASlotIndex < Length(FCurrentNames)) then
      FCurrentNames[ASlotIndex] := AItem.ProfileName;
    Result := True;
  finally
    LeaveCriticalSection(FLock);
  end;
end;

procedure TZaryaRealDelayBatchThread.SetActiveWorker(const ASlotIndex: Integer;
  const AWorker: INodeTestWorker; const AProfileName: string);
begin
  EnterCriticalSection(FLock);
  try
    if (ASlotIndex >= 0) and (ASlotIndex < Length(FActiveWorkers)) then
    begin
      FActiveWorkers[ASlotIndex] := AWorker;
      FCurrentNames[ASlotIndex] := AProfileName;
    end;
  finally
    LeaveCriticalSection(FLock);
  end;
end;

procedure TZaryaRealDelayBatchThread.StoreResult(const AIndex: Integer;
  const AResult: TZaryaNodeTestResult);
begin
  EnterCriticalSection(FLock);
  try
    if (AIndex >= 0) and (AIndex < Length(FResults)) then
    begin
      FResults[AIndex].Tested := True;
      FResults[AIndex].Success := AResult.Success;
      FResults[AIndex].DelayMs := AResult.DelayMs;
      FResults[AIndex].ErrorCode := AResult.ErrorCode;
      FResults[AIndex].ErrorMessage := AResult.MessageText;
    end;
  finally
    LeaveCriticalSection(FLock);
  end;
  InterlockedIncrement(FCompletedCount);
end;

procedure TZaryaRealDelayBatchThread.Execute;
var
  I: Integer;
begin
  try
    SetLength(FSlots, FConcurrency);
    for I := 0 to High(FSlots) do
    begin
      FSlots[I] := TZaryaRealDelaySlotThread.Create(Self, I);
      FSlots[I].Start;
    end;
    for I := 0 to High(FSlots) do FSlots[I].WaitFor;
    for I := 0 to High(FSlots) do FreeAndNil(FSlots[I]);
    SetLength(FSlots, 0);
  finally
    InterlockedExchange(FDoneFlag, 1);
  end;
end;

procedure TZaryaRealDelayBatchThread.RequestCancel;
var
  I: Integer;
  Workers: array of INodeTestWorker;
begin
  InterlockedExchange(FCancelFlag, 1);
  EnterCriticalSection(FLock);
  try
    SetLength(Workers, Length(FActiveWorkers));
    for I := 0 to High(FActiveWorkers) do Workers[I] := FActiveWorkers[I];
  finally
    LeaveCriticalSection(FLock);
  end;
  for I := 0 to High(Workers) do
    if Assigned(Workers[I]) then Workers[I].Cancel;
end;

function TZaryaRealDelayBatchThread.IsDone: Boolean;
begin
  Result := InterlockedCompareExchange(FDoneFlag, 0, 0) <> 0;
end;

function TZaryaRealDelayBatchThread.CompletedCount: Integer;
begin
  Result := InterlockedCompareExchange(FCompletedCount, 0, 0);
end;

function TZaryaRealDelayBatchThread.ProgressText: string;
var
  I: Integer;
begin
  Result := '';
  EnterCriticalSection(FLock);
  try
    for I := 0 to High(FCurrentNames) do
      if Trim(FCurrentNames[I]) <> '' then
      begin
        if Result <> '' then Result := Result + ', ';
        Result := Result + FCurrentNames[I];
      end;
  finally
    LeaveCriticalSection(FLock);
  end;
end;

end.
