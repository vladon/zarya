program zarya_lcl;

{$mode objfpc}{$H+}

uses
  Interfaces,
  Forms,
  MainForm;

var
  Window: TMainForm;

begin
  RequireDerivedFormResource := False;
  Application.Scaled := True;
  Application.Initialize;
  Application.CreateForm(TMainForm, Window);
  Application.Run;
end.
