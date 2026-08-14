unit ZaryaRuntimeConfigFile;

{$mode objfpc}{$H+}

interface

uses
  SysUtils, ZaryaCoreProvider;

function WriteRuntimeConfig(const ADataDirectory: string;
  const AProvider: TZaryaCoreProvider; const AContent: string;
  out AFileName, AError: string): Boolean;
procedure DeleteRuntimeConfig(const AFileName: string);

implementation

uses
  Classes;

function SafeFilePart(const AValue: string): string;
var
  I: Integer;
begin
  Result := AValue;
  for I := 1 to Length(Result) do
    if not (Result[I] in ['a'..'z', 'A'..'Z', '0'..'9', '-', '_']) then
      Result[I] := '-';
end;

function WriteRuntimeConfig(const ADataDirectory: string;
  const AProvider: TZaryaCoreProvider; const AContent: string;
  out AFileName, AError: string): Boolean;
var
  RunDirectory: string;
  TemporaryName: string;
  Extension: string;
  Stream: TFileStream;
  Bytes: UTF8String;
begin
  Result := False;
  AFileName := '';
  AError := '';
  RunDirectory := IncludeTrailingPathDelimiter(ADataDirectory) + 'run';
  if not ForceDirectories(RunDirectory) then
  begin
    AError := 'Не удалось создать каталог временной конфигурации.';
    Exit;
  end;
  Extension := AProvider.ConfigExtension;
  if Extension = '' then
    Extension := '.conf';
  AFileName := IncludeTrailingPathDelimiter(RunDirectory) + 'active-' +
    SafeFilePart(AProvider.ProviderId) + Extension;
  TemporaryName := AFileName + '.tmp';
  Bytes := UTF8String(AContent);
  try
    Stream := TFileStream.Create(TemporaryName, fmCreate or fmShareExclusive);
    try
      if Length(Bytes) > 0 then
        Stream.WriteBuffer(Bytes[1], Length(Bytes));
    finally
      Stream.Free;
    end;
    if FileExists(AFileName) and (not DeleteFile(AFileName)) then
      raise Exception.Create('Не удалось заменить предыдущую runtime-конфигурацию.');
    if not RenameFile(TemporaryName, AFileName) then
      raise Exception.Create('Не удалось установить runtime-конфигурацию.');
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      if FileExists(TemporaryName) then
        DeleteFile(TemporaryName);
      AFileName := '';
    end;
  end;
end;

procedure DeleteRuntimeConfig(const AFileName: string);
begin
  if (AFileName <> '') and FileExists(AFileName) then
    DeleteFile(AFileName);
end;

end.
