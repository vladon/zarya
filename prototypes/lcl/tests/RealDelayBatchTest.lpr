program RealDelayBatchTest;

{$mode objfpc}{$H+}

uses
  SysUtils, ZaryaRealDelay;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then raise Exception.Create(AMessage);
end;

var
  Items: TZaryaRealDelayWorkItems;
  Batch: TZaryaRealDelayBatchThread;
  I: Integer;
begin
  SetLength(Items, 7);
  for I := 0 to High(Items) do
  begin
    Items[I].ProfileId := 'profile-' + IntToStr(I);
    Items[I].ProfileName := 'Profile ' + IntToStr(I);
    Items[I].Prepared := False;
    Items[I].PreparationError := 'expected preparation failure';
  end;
  Batch := TZaryaRealDelayBatchThread.Create(Items, 3);
  try
    Batch.WaitFor;
    Check(Batch.IsDone, 'Batch did not report completion.');
    Check(Batch.CompletedCount = Length(Items), 'Batch completion count is wrong.');
    for I := 0 to High(Batch.Results) do
    begin
      Check(Batch.Results[I].Tested, 'Prepared failure was not recorded.');
      Check(not Batch.Results[I].Success, 'Prepared failure became success.');
      Check(Batch.Results[I].ErrorCode = 'preparation_failed',
        'Prepared failure code is wrong.');
    end;
  finally
    Batch.Free;
  end;
  WriteLn('Real delay bounded batch: PASS');
end.
