unit ZaryaRuntimeProcess;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Process, ZaryaCoreProvider;

type
  TZaryaProcessContext = record
    ConfigPath: string;
    DataDirectory: string;
    AssetDirectory: string;
    MixedPort: Integer;
    HttpPort: Integer;
    SocksPort: Integer;
    LogLevel: string;
  end;

  IZaryaRuntimeProcess = interface
    ['{9EA41714-CB67-45E5-A94C-C8A1C803BDFB}']
    function Start(const AExecutable, AWorkingDirectory: string;
      const AArguments: TZaryaStringArray; out AError: string): Boolean;
    procedure Stop;
    function IsRunning: Boolean;
    function ExitCode: Integer;
    function DrainOutput: string;
  end;

  TZaryaExternalProcess = class(TInterfacedObject, IZaryaRuntimeProcess)
  private
    FProcess: TProcess;
    FJobHandle: THandle;
    FOutput: UTF8String;
    FExitCode: Integer;
    procedure ReadAvailableOutput;
    procedure CloseJob;
  public
    constructor Create;
    destructor Destroy; override;
    function Start(const AExecutable, AWorkingDirectory: string;
      const AArguments: TZaryaStringArray; out AError: string): Boolean;
    procedure Stop;
    function IsRunning: Boolean;
    function ExitCode: Integer;
    function DrainOutput: string;
  end;

function ExpandProviderArguments(const ATemplates: TZaryaStringArray;
  const AContext: TZaryaProcessContext; out AArguments: TZaryaStringArray;
  out AError: string): Boolean;
function RunProcessProbe(const AExecutable, AWorkingDirectory: string;
  const AArguments: TZaryaStringArray; const ATimeoutMs: Cardinal;
  out AOutput: string; out AExitCode: Integer; out AError: string): Boolean;

implementation

{$IFDEF WINDOWS}
uses
  Windows;

type
  TZaryaIoCounters = record
    ReadOperationCount: QWord;
    WriteOperationCount: QWord;
    OtherOperationCount: QWord;
    ReadTransferCount: QWord;
    WriteTransferCount: QWord;
    OtherTransferCount: QWord;
  end;

  TZaryaJobBasicLimitInformation = record
    PerProcessUserTimeLimit: Int64;
    PerJobUserTimeLimit: Int64;
    LimitFlags: DWORD;
    MinimumWorkingSetSize: NativeUInt;
    MaximumWorkingSetSize: NativeUInt;
    ActiveProcessLimit: DWORD;
    Affinity: NativeUInt;
    PriorityClass: DWORD;
    SchedulingClass: DWORD;
  end;

  TZaryaJobExtendedLimitInformation = record
    BasicLimitInformation: TZaryaJobBasicLimitInformation;
    IoInfo: TZaryaIoCounters;
    ProcessMemoryLimit: NativeUInt;
    JobMemoryLimit: NativeUInt;
    PeakProcessMemoryUsed: NativeUInt;
    PeakJobMemoryUsed: NativeUInt;
  end;

const
  JobObjectExtendedLimitInformation = 9;
  JobObjectLimitKillOnJobClose = $00002000;

function CreateJobObjectW(Attributes: Pointer; Name: PWideChar): THandle;
  stdcall; external 'kernel32.dll' name 'CreateJobObjectW';
function SetInformationJobObject(Job: THandle; InfoClass: Integer;
  Info: Pointer; InfoLength: DWORD): BOOL;
  stdcall; external 'kernel32.dll' name 'SetInformationJobObject';
function AssignProcessToJobObject(Job, ProcessHandle: THandle): BOOL;
  stdcall; external 'kernel32.dll' name 'AssignProcessToJobObject';
{$ENDIF}

function ReplacePlaceholder(const AValue, AName, AReplacement: string): string;
begin
  Result := StringReplace(AValue, '{' + AName + '}', AReplacement,
    [rfReplaceAll]);
end;

function ExpandProviderArguments(const ATemplates: TZaryaStringArray;
  const AContext: TZaryaProcessContext; out AArguments: TZaryaStringArray;
  out AError: string): Boolean;
var
  I: Integer;
  Value: string;
begin
  AError := '';
  SetLength(AArguments, Length(ATemplates));
  for I := 0 to High(ATemplates) do
  begin
    Value := ATemplates[I];
    Value := ReplacePlaceholder(Value, 'config', AContext.ConfigPath);
    Value := ReplacePlaceholder(Value, 'dataDir', AContext.DataDirectory);
    Value := ReplacePlaceholder(Value, 'assetDir', AContext.AssetDirectory);
    Value := ReplacePlaceholder(Value, 'mixedPort', IntToStr(AContext.MixedPort));
    Value := ReplacePlaceholder(Value, 'httpPort', IntToStr(AContext.HttpPort));
    Value := ReplacePlaceholder(Value, 'socksPort', IntToStr(AContext.SocksPort));
    Value := ReplacePlaceholder(Value, 'logLevel', AContext.LogLevel);
    if (Pos('{', Value) > 0) or (Pos('}', Value) > 0) then
    begin
      AError := 'Неизвестный placeholder в аргументе: ' + ATemplates[I];
      SetLength(AArguments, 0);
      Exit(False);
    end;
    AArguments[I] := Value;
  end;
  Result := True;
end;

constructor TZaryaExternalProcess.Create;
begin
  inherited Create;
  FProcess := TProcess.Create(nil);
  FExitCode := -1;
end;

procedure TZaryaExternalProcess.CloseJob;
begin
  {$IFDEF WINDOWS}
  if FJobHandle <> 0 then
  begin
    Windows.CloseHandle(FJobHandle);
    FJobHandle := 0;
  end;
  {$ENDIF}
end;

destructor TZaryaExternalProcess.Destroy;
begin
  Stop;
  FProcess.Free;
  inherited Destroy;
end;

procedure TZaryaExternalProcess.ReadAvailableOutput;
var
  Buffer: array[0..8191] of Byte;
  Count: Integer;
  Available: LongInt;
  Chunk: UTF8String;
begin
  if not Assigned(FProcess.Output) then
    Exit;
  repeat
    Available := FProcess.Output.NumBytesAvailable;
    if Available <= 0 then
      Break;
    if Available > SizeOf(Buffer) then
      Available := SizeOf(Buffer);
    Count := FProcess.Output.Read(Buffer, Available);
    if Count > 0 then
    begin
      SetString(Chunk, PAnsiChar(@Buffer[0]), Count);
      FOutput := FOutput + Chunk;
    end;
  until Count <= 0;
end;

function TZaryaExternalProcess.Start(const AExecutable,
  AWorkingDirectory: string; const AArguments: TZaryaStringArray;
  out AError: string): Boolean;
var
  I: Integer;
  JobInfo: TZaryaJobExtendedLimitInformation;
begin
  AError := '';
  Result := False;
  if IsRunning then
  begin
    AError := 'Уже запущен другой внешний runtime.';
    Exit;
  end;
  if not FileExists(AExecutable) then
  begin
    AError := 'Файл ядра не найден: ' + AExecutable;
    Exit;
  end;
  FOutput := '';
  FExitCode := -1;
  FProcess.Executable := AExecutable;
  FProcess.Parameters.Clear;
  for I := 0 to High(AArguments) do
    FProcess.Parameters.Add(AArguments[I]);
  if Trim(AWorkingDirectory) <> '' then
    FProcess.CurrentDirectory := AWorkingDirectory
  else
    FProcess.CurrentDirectory := ExtractFileDir(AExecutable);
  FProcess.Options := [poUsePipes, poStderrToOutput, poNoConsole,
    poRunSuspended];
  try
    FProcess.Execute;
    {$IFDEF WINDOWS}
    FJobHandle := CreateJobObjectW(nil, nil);
    if FJobHandle = 0 then
      raise Exception.Create('Не удалось создать Windows Job Object.');
    FillChar(JobInfo, SizeOf(JobInfo), 0);
    JobInfo.BasicLimitInformation.LimitFlags := JobObjectLimitKillOnJobClose;
    if not SetInformationJobObject(FJobHandle,
      JobObjectExtendedLimitInformation, @JobInfo, SizeOf(JobInfo)) then
      raise Exception.Create('Не удалось включить KILL_ON_JOB_CLOSE.');
    if not AssignProcessToJobObject(FJobHandle, FProcess.ProcessHandle) then
      raise Exception.Create('Не удалось поместить процесс ядра в Job Object.');
    {$ENDIF}
    if FProcess.Resume = -1 then
      raise Exception.Create('Не удалось продолжить процесс ядра.');
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      if FProcess.Running then
        FProcess.Terminate(1);
      CloseJob;
    end;
  end;
end;

procedure TZaryaExternalProcess.Stop;
begin
  if not Assigned(FProcess) then
    Exit;
  ReadAvailableOutput;
  CloseJob;
  if FProcess.Running then
  begin
    if not FProcess.WaitOnExit(3000) then
    begin
      FProcess.Terminate(1);
      FProcess.WaitOnExit(1000);
    end;
  end;
  ReadAvailableOutput;
  if not FProcess.Running then
    FExitCode := FProcess.ExitStatus;
end;

function TZaryaExternalProcess.IsRunning: Boolean;
begin
  Result := Assigned(FProcess) and FProcess.Running;
  if Assigned(FProcess) then
  begin
    ReadAvailableOutput;
    if not Result then
      FExitCode := FProcess.ExitStatus;
  end;
end;

function TZaryaExternalProcess.ExitCode: Integer;
begin
  IsRunning;
  Result := FExitCode;
end;

function TZaryaExternalProcess.DrainOutput: string;
begin
  ReadAvailableOutput;
  Result := string(FOutput);
  FOutput := '';
end;

function RunProcessProbe(const AExecutable, AWorkingDirectory: string;
  const AArguments: TZaryaStringArray; const ATimeoutMs: Cardinal;
  out AOutput: string; out AExitCode: Integer; out AError: string): Boolean;
var
  Runtime: IZaryaRuntimeProcess;
  Deadline: QWord;
begin
  AOutput := '';
  AExitCode := -1;
  AError := '';
  Runtime := TZaryaExternalProcess.Create;
  if not Runtime.Start(AExecutable, AWorkingDirectory, AArguments, AError) then
    Exit(False);
  Deadline := GetTickCount64 + ATimeoutMs;
  while Runtime.IsRunning and (GetTickCount64 < Deadline) do
  begin
    AOutput := AOutput + Runtime.DrainOutput;
    Sleep(10);
  end;
  if Runtime.IsRunning then
  begin
    Runtime.Stop;
    AOutput := AOutput + Runtime.DrainOutput;
    AError := 'Процесс проверки превысил допустимое время.';
    Exit(False);
  end;
  AOutput := AOutput + Runtime.DrainOutput;
  AExitCode := Runtime.ExitCode;
  Result := AExitCode = 0;
  if not Result then
    AError := Format('Процесс проверки завершился с кодом %d.', [AExitCode]);
end;

end.
