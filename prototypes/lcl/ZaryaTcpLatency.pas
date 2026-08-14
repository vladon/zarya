unit ZaryaTcpLatency;

{$mode objfpc}{$H+}

interface

function MeasureTcpLatency(const AHost: string; const APort,
  ATimeoutMs: Integer; out ALatencyMs: Integer; out AError: string): Boolean;

implementation

uses
  SysUtils
  {$IFDEF WINDOWS}, Sockets, WinSock2, Windows{$ENDIF};

{$IFDEF WINDOWS}
function MeasureOneAddress(const AFamily: Integer;
  const AAddress: WinSock2.PSockAddr; const AAddressLength,
  ATimeoutMs: Integer; out ALatencyMs: Integer;
  out AErrorCode: Integer): Boolean;
var
  SocketHandle: WinSock2.TSocket;
  NonBlocking: WinSock2.u_long;
  IoctlCommand: LongInt;
  WriteSet: WinSock2.TFDSet;
  ExceptSet: WinSock2.TFDSet;
  Timeout: WinSock2.TTimeVal;
  StartedAt: QWord;
  ConnectResult: Integer;
  SelectResult: Integer;
  SocketErrorValue: Integer;
  SocketErrorLength: Integer;
begin
  Result := False;
  ALatencyMs := -1;
  AErrorCode := 0;
  SocketHandle := WinSock2.socket(AFamily, WinSock2.SOCK_STREAM,
    WinSock2.IPPROTO_TCP);
  if SocketHandle = WinSock2.INVALID_SOCKET then
  begin
    AErrorCode := WinSock2.WSAGetLastError;
    Exit;
  end;
  try
    NonBlocking := 1;
    IoctlCommand := LongInt(LongWord(WinSock2.FIONBIO));
    if WinSock2.ioctlsocket(SocketHandle, IoctlCommand,
      NonBlocking) <> 0 then
    begin
      AErrorCode := WinSock2.WSAGetLastError;
      Exit;
    end;
    StartedAt := GetTickCount64;
    ConnectResult := WinSock2.connect(SocketHandle, AAddress,
      AAddressLength);
    if ConnectResult = 0 then
    begin
      ALatencyMs := Integer(GetTickCount64 - StartedAt);
      Exit(True);
    end;
    AErrorCode := WinSock2.WSAGetLastError;
    if (AErrorCode <> WinSock2.WSAEWOULDBLOCK) and
      (AErrorCode <> WinSock2.WSAEINPROGRESS) then Exit;
    WinSock2.FD_ZERO(WriteSet);
    WinSock2.FD_ZERO(ExceptSet);
    WinSock2.FD_SET(SocketHandle, WriteSet);
    WinSock2.FD_SET(SocketHandle, ExceptSet);
    Timeout.tv_sec := ATimeoutMs div 1000;
    Timeout.tv_usec := (ATimeoutMs mod 1000) * 1000;
    SelectResult := WinSock2.select(0, nil, @WriteSet, @ExceptSet, @Timeout);
    if SelectResult = 0 then
    begin
      AErrorCode := WinSock2.WSAETIMEDOUT;
      Exit;
    end;
    if SelectResult = WinSock2.SOCKET_ERROR then
    begin
      AErrorCode := WinSock2.WSAGetLastError;
      Exit;
    end;
    SocketErrorValue := 0;
    SocketErrorLength := SizeOf(SocketErrorValue);
    if WinSock2.getsockopt(SocketHandle, WinSock2.SOL_SOCKET,
      WinSock2.SO_ERROR, PChar(@SocketErrorValue), SocketErrorLength) <> 0 then
    begin
      AErrorCode := WinSock2.WSAGetLastError;
      Exit;
    end;
    if SocketErrorValue <> 0 then
    begin
      AErrorCode := SocketErrorValue;
      Exit;
    end;
    ALatencyMs := Integer(GetTickCount64 - StartedAt);
    Result := True;
  finally
    WinSock2.closesocket(SocketHandle);
  end;
end;

function MeasureTcpLatency(const AHost: string; const APort,
  ATimeoutMs: Integer; out ALatencyMs: Integer; out AError: string): Boolean;
var
  HostEntry: WinSock2.PHostEnt;
  HostAnsi: AnsiString;
  Address4: WinSock2.TSockAddrIn;
  ErrorCode: Integer;
  TimeoutMs: Integer;
begin
  Result := False;
  ALatencyMs := -1;
  AError := '';
  if Trim(AHost) = '' then
  begin
    AError := 'Пустое имя сервера.';
    Exit;
  end;
  if (APort < 1) or (APort > 65535) then
  begin
    AError := 'Некорректный TCP port.';
    Exit;
  end;
  TimeoutMs := ATimeoutMs;
  if TimeoutMs < 1 then TimeoutMs := 1;
  ErrorCode := 0;
  HostAnsi := AnsiString(Trim(AHost));
  HostEntry := WinSock2.gethostbyname(PAnsiChar(HostAnsi));
  if (HostEntry = nil) or (HostEntry^.h_addr_list = nil) or
    (HostEntry^.h_addr_list^ = nil) then
  begin
    AError := 'DNS lookup failed.';
    Exit;
  end;
  FillChar(Address4, SizeOf(Address4), 0);
  Address4.sin_family := WinSock2.AF_INET;
  Address4.sin_port := WinSock2.htons(APort);
  Move(HostEntry^.h_addr_list^^, Address4.sin_addr,
    SizeOf(Address4.sin_addr));
  if MeasureOneAddress(WinSock2.AF_INET, @Address4, SizeOf(Address4),
    TimeoutMs, ALatencyMs, ErrorCode) then Exit(True);
  if ErrorCode = WinSock2.WSAETIMEDOUT then
    AError := 'TCP connect timeout.'
  else
    AError := 'TCP connect failed (' + IntToStr(ErrorCode) + ').';
end;
{$ELSE}
function MeasureTcpLatency(const AHost: string; const APort,
  ATimeoutMs: Integer; out ALatencyMs: Integer; out AError: string): Boolean;
begin
  ALatencyMs := -1;
  AError := 'TCP latency measurement is not implemented on this platform.';
  Result := False;
end;
{$ENDIF}

end.
