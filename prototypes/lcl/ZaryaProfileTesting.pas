unit ZaryaProfileTesting;

{$mode objfpc}{$H+}

interface

uses
  Classes, ZaryaProfile;

type
  TZaryaTcpTestResult = record
    Tested: Boolean;
    Success: Boolean;
    LatencyMs: Integer;
    ErrorMessage: string;
  end;

  TZaryaTcpTestResults = array of TZaryaTcpTestResult;

  TProfileTcpTestThread = class(TThread)
  private
    FProfiles: TZaryaProfiles;
    FResults: TZaryaTcpTestResults;
    FCancelFlag: LongInt;
    FDoneFlag: LongInt;
    FCurrentIndex: Integer;
  protected
    procedure Execute; override;
  public
    constructor Create(const AProfiles: TZaryaProfiles);
    procedure RequestCancel;
    function IsDone: Boolean;
    property CurrentIndex: Integer read FCurrentIndex;
    property Results: TZaryaTcpTestResults read FResults;
  end;

implementation

uses
  ZaryaTcpLatency;

constructor TProfileTcpTestThread.Create(const AProfiles: TZaryaProfiles);
var
  I: Integer;
begin
  inherited Create(True);
  FreeOnTerminate := False;
  SetLength(FProfiles, Length(AProfiles));
  for I := 0 to High(AProfiles) do FProfiles[I] := AProfiles[I];
  SetLength(FResults, Length(AProfiles));
  FCurrentIndex := -1;
  Start;
end;

procedure TProfileTcpTestThread.Execute;
var
  I: Integer;
begin
  try
    for I := 0 to High(FProfiles) do
    begin
      if InterlockedCompareExchange(FCancelFlag, 0, 0) <> 0 then Break;
      FCurrentIndex := I;
      if not FProfiles[I].Enabled or FProfiles[I].DeletedBySubscriptionUpdate then
        Continue;
      FResults[I].Tested := True;
      FResults[I].Success := MeasureTcpLatency(FProfiles[I].Host,
        FProfiles[I].Port, 3000, FResults[I].LatencyMs,
        FResults[I].ErrorMessage);
    end;
  finally
    FCurrentIndex := -1;
    InterlockedExchange(FDoneFlag, 1);
  end;
end;

procedure TProfileTcpTestThread.RequestCancel;
begin
  InterlockedExchange(FCancelFlag, 1);
end;

function TProfileTcpTestThread.IsDone: Boolean;
begin
  Result := InterlockedCompareExchange(FDoneFlag, 0, 0) <> 0;
end;

end.
