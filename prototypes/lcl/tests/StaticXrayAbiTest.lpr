program StaticXrayAbiTest;

{$mode objfpc}{$H+}
{$ifdef ZARYA_FPC_INTERNAL_LINKER_TEST}
  {$linklib zarya_xray_static}
  {$linklib mingwex}
  {$linklib mingw32}
  {$linklib msvcrt}
  {$linklib kernel32}
  {$linklib gcc}
  {$linklib ws2_32}
  {$linklib winmm}
  {$linklib ntdll}
  {$linklib bcrypt}
  {$linklib iphlpapi}
  {$linklib secur32}
  {$linklib advapi32}
  {$linklib userenv}
  {$linklib shell32}
  {$linklib ole32}
  {$linklib oleaut32}
  {$linklib uuid}
{$endif}

uses
  SysUtils;

function ZaryaXrayAbiVersion: LongInt; cdecl; external;
function ZaryaXrayVersion: PAnsiChar; cdecl; external;
procedure ZaryaXrayFree(AValue: Pointer); cdecl; external;
procedure ZaryaXrayRuntimeInit; cdecl; external name '_rt0_amd64_windows_lib';

var
  Value: PAnsiChar;
begin
  ZaryaXrayRuntimeInit;
  if ZaryaXrayAbiVersion <> 1 then
    raise Exception.Create('Static Xray ABI mismatch.');
  Value := ZaryaXrayVersion;
  try
    if (Value = nil) or (Value^ = #0) then
      raise Exception.Create('Static Xray version is empty.');
    WriteLn('Static Xray ABI: PASS (', string(UTF8String(Value)), ')');
  finally
    ZaryaXrayFree(Value);
  end;
end.
