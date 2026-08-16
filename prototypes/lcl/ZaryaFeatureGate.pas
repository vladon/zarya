unit ZaryaFeatureGate;

{$mode objfpc}{$H+}

interface

type
  TZaryaFeatureGate = class sealed
  public
    class function ExperimentalEnabled: Boolean; static;
    class function EmbeddedSingBoxVisible: Boolean; static;
    class function PrivilegedTunVisible: Boolean; static;
  end;

implementation

uses
  SysUtils;

class function TZaryaFeatureGate.ExperimentalEnabled: Boolean;
var
  I: Integer;
begin
  for I := 1 to ParamCount do
    if SameText(ParamStr(I), '--experimental') then
      Exit(True);
  Result := False;
end;

class function TZaryaFeatureGate.EmbeddedSingBoxVisible: Boolean;
begin
  Result := ExperimentalEnabled;
end;

class function TZaryaFeatureGate.PrivilegedTunVisible: Boolean;
begin
  Result := ExperimentalEnabled;
end;

end.
