unit ZaryaThemes;

{$mode objfpc}{$H+}

interface

uses
  Classes, Controls, Forms, Graphics, StdCtrls, ExtCtrls, ComCtrls, Grids;

type
  TZaryaTheme = record
    Canvas: TColor;
    Surface: TColor;
    Panel: TColor;
    Ink: TColor;
    Muted: TColor;
    Border: TColor;
    Accent: TColor;
    AccentText: TColor;
    Success: TColor;
    SuccessSurface: TColor;
    Warning: TColor;
    WarningSurface: TColor;
  end;

function LightTheme: TZaryaTheme;
function DarkTheme: TZaryaTheme;
procedure ApplyZaryaTheme(AControl: TControl; const ATheme: TZaryaTheme);

implementation

function RgbColor(const R, G, B: Byte): TColor;
begin
  Result := TColor(R or (G shl 8) or (B shl 16));
end;

function LightTheme: TZaryaTheme;
begin
  Result.Canvas := RgbColor($F4, $F6, $F8);
  Result.Surface := clWhite;
  Result.Panel := RgbColor($EE, $F1, $F4);
  Result.Ink := RgbColor($1B, $1F, $24);
  Result.Muted := RgbColor($5B, $65, $70);
  Result.Border := RgbColor($D0, $D7, $DE);
  Result.Accent := RgbColor($1F, $6F, $EB);
  Result.AccentText := clWhite;
  Result.Success := RgbColor($1A, $7F, $37);
  Result.SuccessSurface := RgbColor($DA, $FB, $E1);
  Result.Warning := RgbColor($94, $62, $00);
  Result.WarningSurface := RgbColor($FF, $F8, $C5);
end;

function DarkTheme: TZaryaTheme;
begin
  Result.Canvas := RgbColor($0D, $11, $17);
  Result.Surface := RgbColor($16, $1B, $22);
  Result.Panel := RgbColor($21, $26, $2D);
  Result.Ink := RgbColor($E6, $ED, $F3);
  Result.Muted := RgbColor($8B, $94, $9E);
  Result.Border := RgbColor($30, $36, $3D);
  Result.Accent := RgbColor($1F, $6F, $EB);
  Result.AccentText := clWhite;
  Result.Success := RgbColor($3F, $B9, $50);
  Result.SuccessSurface := RgbColor($12, $26, $1E);
  Result.Warning := RgbColor($D2, $99, $22);
  Result.WarningSurface := RgbColor($3D, $2E, $00);
end;

procedure ApplyZaryaTheme(AControl: TControl; const ATheme: TZaryaTheme);
var
  I: Integer;
begin
  AControl.Font.Name := 'Segoe UI';
  AControl.Font.Size := 9;
  AControl.Font.Color := ATheme.Ink;

  if AControl is TCustomForm then
    TCustomForm(AControl).Color := ATheme.Canvas
  else if AControl is TPanel then
  begin
    TPanel(AControl).ParentColor := False;
    TPanel(AControl).Color := ATheme.Surface;
  end
  else if AControl is TMemo then
  begin
    TMemo(AControl).ParentColor := False;
    TMemo(AControl).Color := ATheme.Surface;
  end
  else if AControl is TEdit then
  begin
    TEdit(AControl).ParentColor := False;
    TEdit(AControl).Color := ATheme.Surface;
  end
  else if AControl is TComboBox then
  begin
    TComboBox(AControl).ParentColor := False;
    TComboBox(AControl).Color := ATheme.Surface;
  end
  else if AControl is TStringGrid then
  begin
    TStringGrid(AControl).Color := ATheme.Surface;
    TStringGrid(AControl).FixedColor := ATheme.Panel;
    TStringGrid(AControl).GridLineColor := ATheme.Border;
  end
  else if AControl is TStatusBar then
    TStatusBar(AControl).Color := ATheme.Panel
  else if AControl is TPageControl then
    TPageControl(AControl).Color := ATheme.Canvas
  else if AControl is TTabSheet then
    TTabSheet(AControl).Color := ATheme.Canvas;

  if AControl is TWinControl then
    for I := 0 to TWinControl(AControl).ControlCount - 1 do
      ApplyZaryaTheme(TWinControl(AControl).Controls[I], ATheme);
end;

end.
