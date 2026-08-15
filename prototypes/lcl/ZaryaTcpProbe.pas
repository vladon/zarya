unit ZaryaTcpProbe;

{$mode objfpc}{$H+}

interface

function CanConnectLocalhost(const APort: Integer): Boolean;
function AllocateLocalTcpPort(out APort: Integer; out AError: string): Boolean;
function AllocateLocalTcpUdpPort(out APort: Integer; out AError: string): Boolean;

implementation

uses
  Sockets;

function CanConnectLocalhost(const APort: Integer): Boolean;
var
  SocketHandle: LongInt;
  Address: TInetSockAddr;
begin
  Result := False;
  if (APort < 1) or (APort > 65535) then
    Exit;
  SocketHandle := fpSocket(AF_INET, SOCK_STREAM, 0);
  if SocketHandle < 0 then
    Exit;
  try
    FillChar(Address, SizeOf(Address), 0);
    Address.sin_family := AF_INET;
    Address.sin_port := htons(APort);
    Address.sin_addr.s_addr := htonl($7F000001);
    Result := fpConnect(SocketHandle, @Address, SizeOf(Address)) = 0;
  finally
    CloseSocket(SocketHandle);
  end;
end;

function AllocateLocalTcpPort(out APort: Integer; out AError: string): Boolean;
var
  SocketHandle: LongInt;
  Address: TInetSockAddr;
  AddressLength: TSockLen;
begin
  Result := False;
  APort := 0;
  AError := '';
  SocketHandle := fpSocket(AF_INET, SOCK_STREAM, 0);
  if SocketHandle < 0 then
  begin
    AError := 'Cannot create a socket for dynamic port allocation.';
    Exit;
  end;
  try
    FillChar(Address, SizeOf(Address), 0);
    Address.sin_family := AF_INET;
    Address.sin_port := 0;
    Address.sin_addr.s_addr := htonl($7F000001);
    if fpBind(SocketHandle, @Address, SizeOf(Address)) <> 0 then
    begin
      AError := 'Cannot bind a dynamic local port.';
      Exit;
    end;
    AddressLength := SizeOf(Address);
    if fpGetSockName(SocketHandle, @Address, @AddressLength) <> 0 then
    begin
      AError := 'Cannot query the allocated local port.';
      Exit;
    end;
    APort := ntohs(Address.sin_port);
    Result := APort > 0;
  finally
    CloseSocket(SocketHandle);
  end;
end;

function AllocateLocalTcpUdpPort(out APort: Integer; out AError: string): Boolean;
const
  MaxAttempts = 32;
var
  TcpSocket, UdpSocket: LongInt;
  Address: TInetSockAddr;
  AddressLength: TSockLen;
  Attempt: Integer;
begin
  Result := False;
  APort := 0;
  AError := '';
  for Attempt := 1 to MaxAttempts do
  begin
    TcpSocket := fpSocket(AF_INET, SOCK_STREAM, 0);
    if TcpSocket < 0 then
    begin
      AError := 'Cannot create a TCP socket for dynamic port allocation.';
      Exit;
    end;
    try
      FillChar(Address, SizeOf(Address), 0);
      Address.sin_family := AF_INET;
      Address.sin_port := 0;
      Address.sin_addr.s_addr := htonl($7F000001);
      if fpBind(TcpSocket, @Address, SizeOf(Address)) <> 0 then
        Continue;
      AddressLength := SizeOf(Address);
      if fpGetSockName(TcpSocket, @Address, @AddressLength) <> 0 then
        Continue;
      APort := ntohs(Address.sin_port);

      UdpSocket := fpSocket(AF_INET, SOCK_DGRAM, 0);
      if UdpSocket < 0 then
      begin
        AError := 'Cannot create a UDP socket for dynamic port allocation.';
        Exit;
      end;
      try
        if fpBind(UdpSocket, @Address, SizeOf(Address)) = 0 then
        begin
          Result := True;
          Exit;
        end;
      finally
        CloseSocket(UdpSocket);
      end;
    finally
      CloseSocket(TcpSocket);
    end;
  end;
  APort := 0;
  AError := 'Cannot allocate a local port available for both TCP and UDP.';
end;

end.
