program ExternalProcessTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, ZaryaCoreProvider, ZaryaRuntimeProcess
  {$IFDEF WINDOWS}, Windows{$ENDIF};

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

{$IFDEF WINDOWS}
function ProcessExists(const APid: DWORD): Boolean;
var
  ProcessHandle: THandle;
begin
  ProcessHandle := OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
    False, APid);
  Result := ProcessHandle <> 0;
  if Result then CloseHandle(ProcessHandle);
end;
{$ENDIF}

var
  FakeCorePath: string;
  Output: string;
  ErrorMessage: string;
  ExitCode: Integer;
  Runtime: IZaryaRuntimeProcess;
  PidFile: string;
  PidLines: TStringList;
  ChildPid: Cardinal;
  Deadline: QWord;
begin
  FakeCorePath := IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
    'FakeCore.exe';
  Check(FileExists(FakeCorePath), 'FakeCore.exe is missing.');

  Check(RunProcessProbe(FakeCorePath, ExtractFileDir(FakeCorePath),
    StringArray(['echo-args', 'value with spaces', 'literal&token']), 3000,
    Output, ExitCode, ErrorMessage), 'Echo probe failed: ' + ErrorMessage);
  Check(Pos('0=value with spaces', Output) > 0,
    'Argument with spaces was split.');
  Check(Pos('1=literal&token', Output) > 0,
    'Shell metacharacter was interpreted.');
  Check(Pos('stderr-line', Output) > 0, 'stderr was not captured.');

  Check(not RunProcessProbe(FakeCorePath, ExtractFileDir(FakeCorePath),
    StringArray(['crash']), 3000, Output, ExitCode, ErrorMessage),
    'Crash probe unexpectedly succeeded.');
  Check(ExitCode = 23, 'Crash exit code was not preserved.');

  Check(not RunProcessProbe(FakeCorePath, ExtractFileDir(FakeCorePath),
    StringArray(['sleep', '30000']), 100, Output, ExitCode, ErrorMessage),
    'Timeout probe unexpectedly succeeded.');
  Check(Pos('превысил', ErrorMessage) > 0, 'Timeout error was not reported.');

  PidFile := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-fake-core-child-' + IntToHex(Random(MaxInt), 8) + '.pid';
  Runtime := TZaryaExternalProcess.Create;
  Check(Runtime.Start(FakeCorePath, ExtractFileDir(FakeCorePath),
    StringArray(['spawn-child', PidFile]), ErrorMessage),
    'Job test start failed: ' + ErrorMessage);
  Deadline := GetTickCount64 + 3000;
  while (not FileExists(PidFile)) and (GetTickCount64 < Deadline) do Sleep(20);
  Check(FileExists(PidFile), 'Fake child PID was not written.');
  PidLines := TStringList.Create;
  try
    PidLines.LoadFromFile(PidFile);
    ChildPid := StrToIntDef(Trim(PidLines.Text), 0);
  finally
    PidLines.Free;
  end;
  Check(ChildPid <> 0, 'Fake child PID is invalid.');
  Runtime.Stop;
  Runtime := nil;
  {$IFDEF WINDOWS}
  Deadline := GetTickCount64 + 3000;
  while ProcessExists(ChildPid) and (GetTickCount64 < Deadline) do Sleep(20);
  Check(not ProcessExists(ChildPid), 'Job Object left a child process running.');
  {$ENDIF}
  SysUtils.DeleteFile(PidFile);
  WriteLn('External process supervisor: PASS');
end.
