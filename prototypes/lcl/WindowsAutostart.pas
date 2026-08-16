unit WindowsAutostart;

{$mode objfpc}{$H+}

interface

uses
  SysUtils;

type
  IAutostartManager = interface
    ['{94095A2D-A1B0-4D42-8D76-49938A2D2F26}']
    function IsEnabled(out AEnabled: Boolean; out AError: string): Boolean;
    function SetEnabled(const AEnabled: Boolean;
      const AExecutable: string; const AArguments: array of string;
      out AError: string): Boolean;
  end;

  TWindowsAutostartManager = class(TInterfacedObject, IAutostartManager)
  public
    function IsEnabled(out AEnabled: Boolean; out AError: string): Boolean;
    function SetEnabled(const AEnabled: Boolean;
      const AExecutable: string; const AArguments: array of string;
      out AError: string): Boolean;
  end;

function QuoteWindowsCommandLineArgument(const AValue: string): string;
function BuildWindowsCommandLine(const AExecutable: string;
  const AArguments: array of string): string;

implementation

{$IFDEF MSWINDOWS}
uses
  Registry;
{$ENDIF}

function QuoteWindowsCommandLineArgument(const AValue: string): string;
var
  I, Backslashes: Integer;
  NeedsQuotes: Boolean;
begin
  NeedsQuotes := (AValue = '') or (Pos(' ', AValue) > 0) or
    (Pos(#9, AValue) > 0) or (Pos('"', AValue) > 0);
  if not NeedsQuotes then Exit(AValue);
  Result := '"';
  Backslashes := 0;
  for I := 1 to Length(AValue) do
  begin
    if AValue[I] = '\' then
    begin
      Inc(Backslashes);
      Continue;
    end;
    if AValue[I] = '"' then
    begin
      Result := Result + StringOfChar('\', Backslashes * 2 + 1) + '"';
      Backslashes := 0;
      Continue;
    end;
    if Backslashes > 0 then
    begin
      Result := Result + StringOfChar('\', Backslashes);
      Backslashes := 0;
    end;
    Result := Result + AValue[I];
  end;
  if Backslashes > 0 then
    Result := Result + StringOfChar('\', Backslashes * 2);
  Result := Result + '"';
end;

function BuildWindowsCommandLine(const AExecutable: string;
  const AArguments: array of string): string;
var
  Argument: string;
begin
  Result := QuoteWindowsCommandLineArgument(AExecutable);
  for Argument in AArguments do
    Result := Result + ' ' + QuoteWindowsCommandLineArgument(Argument);
end;

{$IFDEF MSWINDOWS}
function OpenRunKey(const ACreate: Boolean; out ARegistry: TRegistry;
  out AError: string): Boolean;
begin
  Result := False;
  ARegistry := TRegistry.Create;
  ARegistry.RootKey := QWord($80000001);
  if ACreate then
    Result := ARegistry.OpenKey(
      '\Software\Microsoft\Windows\CurrentVersion\Run', True)
  else
    Result := ARegistry.OpenKeyReadOnly(
      '\Software\Microsoft\Windows\CurrentVersion\Run');
  if not Result then
  begin
    AError := 'Cannot open the current-user Windows Run registry key.';
    FreeAndNil(ARegistry);
  end;
end;

function TWindowsAutostartManager.IsEnabled(out AEnabled: Boolean;
  out AError: string): Boolean;
var
  Registry: TRegistry;
begin
  AEnabled := False;
  AError := '';
  if not OpenRunKey(False, Registry, AError) then Exit(False);
  try
    AEnabled := Registry.ValueExists('Zarya');
    Result := True;
  finally
    Registry.Free;
  end;
end;

function TWindowsAutostartManager.SetEnabled(const AEnabled: Boolean;
  const AExecutable: string; const AArguments: array of string;
  out AError: string): Boolean;
var
  Registry: TRegistry;
begin
  AError := '';
  { Deleting a value also needs a writable handle. The per-user Run key is
    safe to create when it is missing and does not require elevation. }
  if not OpenRunKey(True, Registry, AError) then Exit(False);
  try
    if AEnabled then
      Registry.WriteString('Zarya', BuildWindowsCommandLine(AExecutable,
        AArguments))
    else if Registry.ValueExists('Zarya') then
      Registry.DeleteValue('Zarya');
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Registry.Free;
end;
{$ELSE}
function TWindowsAutostartManager.IsEnabled(out AEnabled: Boolean;
  out AError: string): Boolean;
begin
  AEnabled := False;
  AError := 'Windows autostart is unavailable on this platform.';
  Result := False;
end;

function TWindowsAutostartManager.SetEnabled(const AEnabled: Boolean;
  const AExecutable: string; const AArguments: array of string;
  out AError: string): Boolean;
begin
  AError := 'Windows autostart is unavailable on this platform.';
  Result := False;
end;
{$ENDIF}

end.
