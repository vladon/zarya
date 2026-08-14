unit ZaryaTcpProbe;

{$mode objfpc}{$H+}

interface

function CanConnectLocalhost(const APort: Integer): Boolean;

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

end.
