unit ZaryaTr;

{$mode objfpc}{$H+}

interface

uses
  SysUtils;

type
  TZaryaLanguage = (zlRussian, zlEnglish);

  TZaryaTr = class sealed
  public
    class procedure SetLanguage(const AValue: string); static;
    class function Language: TZaryaLanguage; static;
    class function LanguageCode: string; static;
    class function Tr(const ARussian, AEnglish: string): string; static;
    class function Tr(const ASource: string): string; static;
  end;

implementation

{$IFDEF WINDOWS}
uses
  Windows;
{$ENDIF}

const
  EnglishTable: array[0..47] of string = (
    'Сохранить', 'Save',
    'Применить', 'Apply',
    'Отмена', 'Cancel',
    'Закрыть', 'Close',
    'Добавить', 'Add',
    'Изменить', 'Edit',
    'Удалить', 'Delete',
    'Запустить', 'Start',
    'Остановить', 'Stop',
    'Настройки', 'Settings',
    'Профили', 'Profiles',
    'Подписки', 'Subscriptions',
    'Ядра', 'Cores',
    'Инструменты', 'Tools',
    'Справка', 'Help',
    'Выход', 'Exit',
    'Журнал', 'Log',
    'Очистить', 'Clear',
    'Остановлено', 'Stopped',
    'Подключено', 'Connected',
    'Проверка готовности', 'Readiness check',
    'Ошибка', 'Error',
    'Русский', 'Russian',
    'Системный', 'System'
  );

var
  CurrentLanguage: TZaryaLanguage = zlEnglish;

function SystemLanguage: TZaryaLanguage;
{$IFDEF WINDOWS}
var
  LanguageId: LangId;
{$ENDIF}
begin
  {$IFDEF WINDOWS}
  LanguageId := GetUserDefaultLangID;
  if (LanguageId and $3FF) = $19 then
    Exit(zlRussian);
  {$ELSE}
  if Pos('ru', LowerCase(GetEnvironmentVariable('LANG'))) = 1 then
    Exit(zlRussian);
  {$ENDIF}
  Result := zlEnglish;
end;

class procedure TZaryaTr.SetLanguage(const AValue: string);
begin
  if SameText(Trim(AValue), 'ru') then
    CurrentLanguage := zlRussian
  else if SameText(Trim(AValue), 'en') then
    CurrentLanguage := zlEnglish
  else
    CurrentLanguage := SystemLanguage;
end;

class function TZaryaTr.Language: TZaryaLanguage;
begin
  Result := CurrentLanguage;
end;

class function TZaryaTr.LanguageCode: string;
begin
  if CurrentLanguage = zlRussian then Result := 'ru' else Result := 'en';
end;

class function TZaryaTr.Tr(const ARussian, AEnglish: string): string;
begin
  if CurrentLanguage = zlRussian then Result := ARussian else Result := AEnglish;
end;

class function TZaryaTr.Tr(const ASource: string): string;
var
  I: Integer;
begin
  if CurrentLanguage = zlRussian then Exit(ASource);
  I := Low(EnglishTable);
  while I <= High(EnglishTable) do
  begin
    if ASource = EnglishTable[I] then Exit(EnglishTable[I + 1]);
    Inc(I, 2);
  end;
  Result := ASource;
end;

initialization
  TZaryaTr.SetLanguage('system');

end.
