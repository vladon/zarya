unit ZaryaSubscriptionParser;

{$mode objfpc}{$H+}

interface

uses
  ZaryaProfile;

type
  TZaryaSubscriptionParseResult = record
    Success: Boolean;
    Profiles: TZaryaProfiles;
    SkippedLines: Integer;
    Warnings: array of string;
    ErrorMessage: string;
  end;

function ParseSubscriptionContent(const AContent: RawByteString):
  TZaryaSubscriptionParseResult;

implementation

uses
  Classes, SysUtils, base64, ZaryaShareLink;

function SplitContent(const AContent: RawByteString): TStringList;
begin
  Result := TStringList.Create;
  Result.Text := string(AContent);
end;

function ContainsShareLink(const ALines: TStrings): Boolean;
var
  I: Integer;
  Line: string;
begin
  for I := 0 to ALines.Count - 1 do
  begin
    Line := Trim(ALines[I]);
    if (Line <> '') and (Line[1] <> '#') and IsSupportedShareLink(Line) then
      Exit(True);
  end;
  Result := False;
end;

function DecodeFlexibleBase64(const AContent: RawByteString): RawByteString;
var
  Normalized: string;
  I: Integer;
begin
  Normalized := '';
  for I := 1 to Length(AContent) do
    if not (AContent[I] in [' ', #9, #10, #13]) then
      Normalized := Normalized + Char(AContent[I]);
  Normalized := StringReplace(Normalized, '-', '+', [rfReplaceAll]);
  Normalized := StringReplace(Normalized, '_', '/', [rfReplaceAll]);
  while (Length(Normalized) mod 4) <> 0 do
    Normalized := Normalized + '=';
  try
    Result := RawByteString(DecodeStringBase64(Normalized));
  except
    Result := '';
  end;
end;

procedure AddWarning(var AResult: TZaryaSubscriptionParseResult;
  const AWarning: string);
var
  Index: Integer;
begin
  Index := Length(AResult.Warnings);
  SetLength(AResult.Warnings, Index + 1);
  AResult.Warnings[Index] := AWarning;
end;

function ParseLines(const ALines: TStrings): TZaryaSubscriptionParseResult;
var
  I: Integer;
  Index: Integer;
  Line: string;
  ErrorMessage: string;
  Profile: TZaryaProfile;
begin
  Result := Default(TZaryaSubscriptionParseResult);
  Result.Success := True;
  for I := 0 to ALines.Count - 1 do
  begin
    Line := Trim(ALines[I]);
    if (Line = '') or (Line[1] = '#') then
      Continue;
    if not IsSupportedShareLink(Line) then
    begin
      Inc(Result.SkippedLines);
      AddWarning(Result, 'Пропущена неподдерживаемая строка подписки.');
      Continue;
    end;
    if not ParseShareLink(Line, Profile, ErrorMessage) then
    begin
      Inc(Result.SkippedLines);
      AddWarning(Result, ErrorMessage);
      Continue;
    end;
    Index := Length(Result.Profiles);
    SetLength(Result.Profiles, Index + 1);
    Result.Profiles[Index] := Profile;
  end;
  if Length(Result.Profiles) = 0 then
  begin
    Result.Success := False;
    Result.ErrorMessage := 'В подписке нет поддерживаемых share links.';
  end;
end;

function ParseSubscriptionContent(const AContent: RawByteString):
  TZaryaSubscriptionParseResult;
var
  Lines: TStringList;
  Decoded: RawByteString;
begin
  Result := Default(TZaryaSubscriptionParseResult);
  if Length(AContent) = 0 then
  begin
    Result.ErrorMessage := 'Ответ подписки пуст.';
    Exit;
  end;
  Lines := SplitContent(AContent);
  try
    if ContainsShareLink(Lines) then
      Exit(ParseLines(Lines));
  finally
    Lines.Free;
  end;
  Decoded := DecodeFlexibleBase64(AContent);
  if Decoded = '' then
  begin
    Result.ErrorMessage :=
      'Подписка не содержит share links и не декодируется как base64.';
    Exit;
  end;
  Lines := SplitContent(Decoded);
  try
    if ContainsShareLink(Lines) then
      Exit(ParseLines(Lines));
  finally
    Lines.Free;
  end;
  Result.ErrorMessage :=
    'Формат подписки не распознан: ожидаются share links или base64.';
end;

end.
