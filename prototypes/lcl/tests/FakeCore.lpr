program FakeCore;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, Process, Sockets;

procedure SleepFor(const AMilliseconds: Integer);
begin
  Sleep(AMilliseconds);
end;

procedure SpawnChild(const APidFile: string);
var
  Child: TProcess;
  PidText: TStringList;
begin
  Child := TProcess.Create(nil);
  try
    Child.Executable := ParamStr(0);
    Child.Parameters.Add('child-sleep');
    Child.Options := [poNoConsole];
    Child.Execute;
    PidText := TStringList.Create;
    try
      PidText.Text := IntToStr(Child.ProcessID);
      PidText.SaveToFile(APidFile);
    finally
      PidText.Free;
    end;
    WriteLn('child=', Child.ProcessID);
    Flush(Output);
    while Child.Running do
      Sleep(50);
  finally
    Child.Free;
  end;
end;

procedure ListenOnce(const APort: Integer);
var
  ServerSocket: LongInt;
  ClientSocket: LongInt;
  Address: TInetSockAddr;
  ClientAddress: TInetSockAddr;
  ClientLength: TSockLen;
begin
  ServerSocket := fpSocket(AF_INET, SOCK_STREAM, 0);
  if ServerSocket < 0 then Halt(31);
  try
    FillChar(Address, SizeOf(Address), 0);
    Address.sin_family := AF_INET;
    Address.sin_port := htons(APort);
    Address.sin_addr.s_addr := htonl($7F000001);
    if fpBind(ServerSocket, @Address, SizeOf(Address)) <> 0 then Halt(32);
    if fpListen(ServerSocket, 1) <> 0 then Halt(33);
    WriteLn('ready');
    Flush(Output);
    ClientLength := SizeOf(ClientAddress);
    ClientSocket := fpAccept(ServerSocket, @ClientAddress, @ClientLength);
    if ClientSocket < 0 then Halt(34);
    CloseSocket(ClientSocket);
  finally
    CloseSocket(ServerSocket);
  end;
end;

var
  I: Integer;
begin
  if ParamCount = 0 then Halt(0);
  if ParamStr(1) = 'version' then
  begin
    WriteLn('Zarya fake core 1.0');
    Halt(0);
  end;
  if ParamStr(1) = 'echo-args' then
  begin
    for I := 2 to ParamCount do
      WriteLn(IntToStr(I - 2) + '=' + ParamStr(I));
    WriteLn(StdErr, 'stderr-line');
    Halt(0);
  end;
  if ParamStr(1) = 'sleep' then
  begin
    SleepFor(StrToIntDef(ParamStr(2), 1000));
    Halt(0);
  end;
  if ParamStr(1) = 'child-sleep' then
  begin
    SleepFor(30000);
    Halt(0);
  end;
  if ParamStr(1) = 'spawn-child' then
  begin
    SpawnChild(ParamStr(2));
    Halt(0);
  end;
  if ParamStr(1) = 'listen-once' then
  begin
    ListenOnce(StrToIntDef(ParamStr(2), 0));
    Halt(0);
  end;
  if ParamStr(1) = 'crash' then Halt(23);
  Halt(2);
end.
