program TranslationTest;

{$mode objfpc}{$H+}

uses
  SysUtils, ZaryaTr;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then raise Exception.Create(AMessage);
end;

begin
  TZaryaTr.SetLanguage('ru');
  Check(TZaryaTr.Tr('Сохранить') = 'Сохранить', 'Russian table failed.');
  TZaryaTr.SetLanguage('en');
  Check(TZaryaTr.Tr('Сохранить') = 'Save', 'English table failed.');
  Check(TZaryaTr.Tr('Xray') = 'Xray', 'Core name was translated.');
  Check(TZaryaTr.Tr('Проверка', 'Check') = 'Check',
    'Two-language translation failed.');
  WriteLn('Compiled-in EN/RU translation table: PASS');
end.
