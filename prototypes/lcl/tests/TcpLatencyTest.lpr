program TcpLatencyTest;

{$mode objfpc}{$H+}

uses
  SysUtils, Process, ZaryaTcpLatency;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then raise Exception.Create(AMessage);
end;

var
  FakeCorePath: string;
  Fixture: TProcess;
  Port: Integer;
  LatencyMs: Integer;
  ErrorMessage: string;
  Success: Boolean;
  Deadline: QWord;
begin
  Randomize;
  FakeCorePath := IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
    'FakeCore.exe';
  Check(FileExists(FakeCorePath), 'FakeCore.exe is missing.');
  Port := 20000 + Random(20000);
  Fixture := TProcess.Create(nil);
  try
    Fixture.Executable := FakeCorePath;
    Fixture.Parameters.Add('listen-once');
    Fixture.Parameters.Add(IntToStr(Port));
    Fixture.Options := [poNoConsole];
    Fixture.Execute;
    Deadline := GetTickCount64 + 3000;
    Success := False;
    repeat
      Sleep(25);
      Success := MeasureTcpLatency('127.0.0.1', Port, 250,
        LatencyMs, ErrorMessage);
    until Success or (not Fixture.Running) or (GetTickCount64 >= Deadline);
    Check(Success, 'TCP latency fixture failed: ' + ErrorMessage);
    Check((LatencyMs >= 0) and (LatencyMs <= 1000),
      'TCP latency result is outside the expected local range.');
    Fixture.WaitOnExit;
    Check(Fixture.ExitStatus = 0, 'TCP fixture exited with an error.');
  finally
    if Fixture.Running then Fixture.Terminate(1);
    Fixture.Free;
  end;
  WriteLn('Real TCP latency measurement: PASS');
end.
