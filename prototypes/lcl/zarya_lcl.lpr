program zarya_lcl;

{$mode objfpc}{$H+}

uses
  Interfaces,
  Classes,
  SysUtils,
  Forms,
  Dialogs,
  MainForm,
  FpcProfileStore,
  ZaryaDataMigration,
  ZaryaEmbeddedXray,
  ZaryaTcpProbe,
  ZaryaNodeTestWorker;

var
  Window: TMainForm;
  WorkerExitCode: Integer;
  MigrationResult: TZaryaMigrationResult;
  MigrationBackup: string;
  MigrationError: string;
  ProfileStorePath: string;

function ReadUtf8File(const AFileName: string): string;
var
  Stream: TFileStream;
  Bytes: UTF8String;
begin
  Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyWrite);
  try
    SetLength(Bytes, Stream.Size);
    if Length(Bytes) > 0 then
      Stream.ReadBuffer(Bytes[1], Length(Bytes));
  finally
    Stream.Free;
  end;
  Result := string(Bytes);
end;

procedure WriteUtf8File(const AFileName, AContent: string);
var
  Stream: TFileStream;
  Bytes: UTF8String;
begin
  if AFileName = '' then
    Exit;
  Bytes := UTF8String(AContent);
  Stream := TFileStream.Create(AFileName, fmCreate or fmShareExclusive);
  try
    if Length(Bytes) > 0 then
      Stream.WriteBuffer(Bytes[1], Length(Bytes));
  finally
    Stream.Free;
  end;
end;

function RunWorkerMode(out AExitCode: Integer): Boolean;
var
  Runtime: TZaryaEmbeddedXray;
  Config: string;
  ErrorMessage: string;
  ErrorFile: string;
  Port: Integer;
  Attempt: Integer;
  Ready: Boolean;
  StopError: string;
begin
  Result := False;
  AExitCode := 0;
  if ParamCount < 1 then
    Exit;
  if SameText(ParamStr(1), '--embedded-abi-test') then
  begin
    Result := True;
    Runtime := TZaryaEmbeddedXray.Create(
      IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
      'zarya-xray.dll');
    try
      if Runtime.Available then
      begin
        AExitCode := 0;
      end
      else
      begin
        AExitCode := 2;
      end;
    finally
      Runtime.Free;
    end;
    Exit;
  end;
  if SameText(ParamStr(1), '--embedded-validate') then
  begin
    Result := True;
    if ParamCount < 4 then
    begin
      AExitCode := 2;
      Exit;
    end;
    ErrorFile := ParamStr(4);
    if FileExists(ErrorFile) then
      DeleteFile(ErrorFile);
    try
      Config := ReadUtf8File(ParamStr(2));
      Runtime := TZaryaEmbeddedXray.Create(
        IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
        'zarya-xray.dll');
      try
        if not Runtime.Available then
        begin
          WriteUtf8File(ErrorFile, Runtime.LoadStatus);
          AExitCode := 2;
        end
        else if Runtime.Validate(Config, ParamStr(3), ErrorMessage) then
          AExitCode := 0
        else
        begin
          WriteUtf8File(ErrorFile, ErrorMessage);
          AExitCode := 3;
        end;
      finally
        Runtime.Free;
      end;
    except
      on E: Exception do
      begin
        WriteUtf8File(ErrorFile, E.Message);
        AExitCode := 4;
      end;
    end;
    Exit;
  end;
  if SameText(ParamStr(1), '--embedded-runtime-smoke') then
  begin
    Result := True;
    if ParamCount < 4 then
    begin
      AExitCode := 2;
      Exit;
    end;
    ErrorFile := ParamStr(4);
    if FileExists(ErrorFile) then
      DeleteFile(ErrorFile);
    Port := StrToIntDef(ParamStr(3), 0);
    if (Port < 1) or (Port > 65535) then
    begin
      WriteUtf8File(ErrorFile, 'Invalid runtime smoke port.');
      AExitCode := 2;
      Exit;
    end;
    Config := '{"log":{"loglevel":"none"},"inbounds":[' +
      '{"listen":"127.0.0.1","port":' + IntToStr(Port) +
      ',"protocol":"mixed","tag":"mixed-in","settings":{"udp":false}}],' +
      '"outbounds":[{"protocol":"freedom","tag":"direct"}]}';
    Runtime := TZaryaEmbeddedXray.Create(
      IncludeTrailingPathDelimiter(ExtractFileDir(ParamStr(0))) +
      'zarya-xray.dll');
    try
      if not Runtime.Available then
      begin
        WriteUtf8File(ErrorFile, Runtime.LoadStatus);
        AExitCode := 2;
        Exit;
      end;
      if not Runtime.Start(Config, ParamStr(2), ErrorMessage) then
      begin
        WriteUtf8File(ErrorFile, ErrorMessage);
        AExitCode := 3;
        Exit;
      end;
      try
        Ready := False;
        for Attempt := 1 to 50 do
        begin
          if CanConnectLocalhost(Port) then
          begin
            Ready := True;
            Break;
          end;
          Sleep(100);
        end;
        if not Ready then
        begin
          WriteUtf8File(ErrorFile,
            'Embedded Xray mixed endpoint did not become ready.');
          AExitCode := 4;
        end
        else
          AExitCode := 0;
      finally
        if not Runtime.Stop(StopError) then
        begin
          WriteUtf8File(ErrorFile, StopError);
          AExitCode := 5;
        end;
      end;
    finally
      Runtime.Free;
    end;
  end;
end;

begin
  if RunCoreTestWorkerMode(WorkerExitCode) then
    Halt(WorkerExitCode);
  if RunWorkerMode(WorkerExitCode) then
    Halt(WorkerExitCode);
  RequireDerivedFormResource := False;
  Application.Title := 'Zarya';
  Application.Scaled := True;
  Application.Initialize;
  ProfileStorePath := ProfileStorePathFromCommandLine;
  if SameText(ExcludeTrailingPathDelimiter(ExtractFileDir(ProfileStorePath)),
    ExcludeTrailingPathDelimiter(DefaultLclDataDirectory)) then
  begin
    MigrationResult := EnsureFirstRunQtMigration(DefaultLclDataDirectory,
      MigrationBackup, MigrationError);
    if MigrationResult = mrFailed then
    begin
      MessageDlg('Миграция данных Zarya',
        'Автоматическая миграция Qt-данных остановлена: ' + MigrationError +
        LineEnding + LineEnding +
        'Legacy-файлы не изменены. Исправьте причину и запустите Zarya снова.',
        mtError, [mbOK], 0);
      Halt(2);
    end;
    if MigrationResult = mrMigrated then
      MessageDlg('Миграция данных Zarya',
        'Профили и настройки перенесены после полной staging-проверки.' +
        LineEnding + 'Резервная копия: ' + MigrationBackup,
        mtInformation, [mbOK], 0);
  end;
  Application.CreateForm(TMainForm, Window);
  Application.Run;
end.
