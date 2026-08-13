unit MainForm;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls, StdCtrls,
  ComCtrls, Grids, Menus, ZaryaThemes, SettingsForm;

type
  TRuntimeState = (rsStopped, rsConnecting, rsRunning);

  TMainForm = class(TForm)
  private
    FRuntimeState: TRuntimeState;
    FDarkTheme: Boolean;
    FMinimizeToTray: Boolean;
    FQuitting: Boolean;
    FStatusPanel: TPanel;
    FStateTitle: TLabel;
    FStateBadge: TPanel;
    FStateDetail: TLabel;
    FStartButton: TButton;
    FStopButton: TButton;
    FSettingsButton: TButton;
    FToolbar: TPanel;
    FGrid: TStringGrid;
    FLogPanel: TPanel;
    FLogToolbar: TPanel;
    FLogFilter: TComboBox;
    FLogMemo: TMemo;
    FStatusBar: TStatusBar;
    FReadyTimer: TTimer;
    FTrayIcon: TTrayIcon;
    FTrayMenu: TPopupMenu;
    FStartMenuItem: TMenuItem;
    FStopMenuItem: TMenuItem;
    procedure BuildMenus;
    procedure BuildInterface;
    procedure BuildTray;
    procedure SeedProfiles;
    procedure UpdateRuntimeSurface;
    procedure ApplyCurrentTheme;
    procedure AppendLog(const AText: string);
    function SelectedProfileName: string;
    procedure StartClick(Sender: TObject);
    procedure StopClick(Sender: TObject);
    procedure ReadyTimerTimer(Sender: TObject);
    procedure TestClick(Sender: TObject);
    procedure AddClick(Sender: TObject);
    procedure ImportClick(Sender: TObject);
    procedure SubscriptionsClick(Sender: TObject);
    procedure SettingsClick(Sender: TObject);
    procedure AboutClick(Sender: TObject);
    procedure ExitClick(Sender: TObject);
    procedure TrayShowClick(Sender: TObject);
    procedure ClearLogClick(Sender: TObject);
    procedure GridClick(Sender: TObject);
    procedure FormResize(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
  public
    constructor Create(AOwner: TComponent); override;
  end;

implementation

function NewMenuItem(AOwner: TComponent; const ACaption: string;
  AHandler: TNotifyEvent): TMenuItem;
begin
  Result := TMenuItem.Create(AOwner);
  Result.Caption := ACaption;
  Result.OnClick := AHandler;
end;

constructor TMainForm.Create(AOwner: TComponent);
begin
  inherited CreateNew(AOwner, 1);
  Caption := 'Zarya — LCL/VCL feasibility prototype';
  Position := poScreenCenter;
  ClientWidth := 1040;
  ClientHeight := 720;
  Constraints.MinWidth := 800;
  Constraints.MinHeight := 560;
  Scaled := True;
  FRuntimeState := rsStopped;
  FMinimizeToTray := True;
  OnResize := @FormResize;
  OnCloseQuery := @FormCloseQuery;
  BuildMenus;
  BuildInterface;
  BuildTray;
  SeedProfiles;
  ApplyCurrentTheme;
  UpdateRuntimeSurface;
  AppendLog('Zarya LCL prototype started. No core or system proxy actions are performed.');
  AppendLog('Win32 widgetset: standard controls, tray, DPI and theme smoke test.');
end;

procedure TMainForm.BuildMenus;
var
  RootItem: TMenuItem;
begin
  Menu := TMainMenu.Create(Self);

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Файл';
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, 'Скрыть в &tray', @TrayShowClick));
  RootItem.Add(NewMenuItem(Menu, '-', nil));
  RootItem.Add(NewMenuItem(Menu, 'Вы&ход', @ExitClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Профили';
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, '&Добавить…', @AddClick));
  RootItem.Add(NewMenuItem(Menu, '&Импортировать…', @ImportClick));
  RootItem.Add(NewMenuItem(Menu, 'Под&писки…', @SubscriptionsClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Core';
  Menu.Items.Add(RootItem);
  FStartMenuItem := NewMenuItem(Menu, '&Запустить', @StartClick);
  FStopMenuItem := NewMenuItem(Menu, '&Остановить', @StopClick);
  RootItem.Add(FStartMenuItem);
  RootItem.Add(FStopMenuItem);

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Инструменты';
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, '&Настройки…', @SettingsClick));

  RootItem := TMenuItem.Create(Menu);
  RootItem.Caption := '&Справка';
  Menu.Items.Add(RootItem);
  RootItem.Add(NewMenuItem(Menu, '&О прототипе', @AboutClick));
end;

procedure TMainForm.BuildInterface;
var
  AddButton: TButton;
  ImportButton: TButton;
  SubscriptionsButton: TButton;
  TestButton: TButton;
  ClearButton: TButton;
  LogLabel: TLabel;
  Splitter: TSplitter;
begin
  FStatusBar := TStatusBar.Create(Self);
  FStatusBar.Parent := Self;
  FStatusBar.Align := alBottom;
  FStatusBar.SimplePanel := True;
  FStatusBar.SimpleText := 'Готово · UI-only prototype';

  FStatusPanel := TPanel.Create(Self);
  FStatusPanel.Parent := Self;
  FStatusPanel.Align := alTop;
  FStatusPanel.Height := 174;
  FStatusPanel.BevelOuter := bvNone;

  FStateTitle := TLabel.Create(Self);
  FStateTitle.Parent := FStatusPanel;
  FStateTitle.SetBounds(20, 16, 650, 26);
  FStateTitle.Font.Size := 12;
  FStateTitle.Font.Style := [fsBold];

  FStateBadge := TPanel.Create(Self);
  FStateBadge.Parent := FStatusPanel;
  FStateBadge.BevelOuter := bvNone;
  FStateBadge.SetBounds(20, 48, 184, 28);
  FStateBadge.Caption := 'Остановлено';

  FStateDetail := TLabel.Create(Self);
  FStateDetail.Parent := FStatusPanel;
  FStateDetail.WordWrap := True;
  FStateDetail.SetBounds(20, 88, 650, 70);

  FStartButton := TButton.Create(Self);
  FStartButton.Parent := FStatusPanel;
  FStartButton.Caption := 'Запустить';
  FStartButton.OnClick := @StartClick;
  FStartButton.Default := True;
  FStartButton.SetBounds(708, 30, 132, 36);

  FStopButton := TButton.Create(Self);
  FStopButton.Parent := FStatusPanel;
  FStopButton.Caption := 'Остановить';
  FStopButton.OnClick := @StopClick;
  FStopButton.SetBounds(850, 30, 132, 36);

  FSettingsButton := TButton.Create(Self);
  FSettingsButton.Parent := FStatusPanel;
  FSettingsButton.Caption := 'Настройки…';
  FSettingsButton.OnClick := @SettingsClick;
  FSettingsButton.SetBounds(850, 76, 132, 32);

  FToolbar := TPanel.Create(Self);
  FToolbar.Parent := Self;
  FToolbar.Align := alTop;
  FToolbar.Top := FStatusPanel.Height;
  FToolbar.Height := 48;
  FToolbar.BevelOuter := bvNone;

  AddButton := TButton.Create(Self);
  AddButton.Parent := FToolbar;
  AddButton.Caption := 'Добавить';
  AddButton.OnClick := @AddClick;
  AddButton.SetBounds(12, 8, 92, 32);

  ImportButton := TButton.Create(Self);
  ImportButton.Parent := FToolbar;
  ImportButton.Caption := 'Импорт';
  ImportButton.OnClick := @ImportClick;
  ImportButton.SetBounds(110, 8, 92, 32);

  SubscriptionsButton := TButton.Create(Self);
  SubscriptionsButton.Parent := FToolbar;
  SubscriptionsButton.Caption := 'Подписки';
  SubscriptionsButton.OnClick := @SubscriptionsClick;
  SubscriptionsButton.SetBounds(208, 8, 100, 32);

  TestButton := TButton.Create(Self);
  TestButton.Parent := FToolbar;
  TestButton.Caption := 'Проверить';
  TestButton.OnClick := @TestClick;
  TestButton.SetBounds(320, 8, 100, 32);

  FLogPanel := TPanel.Create(Self);
  FLogPanel.Parent := Self;
  FLogPanel.Align := alBottom;
  FLogPanel.Height := 220;
  FLogPanel.BevelOuter := bvNone;

  FLogToolbar := TPanel.Create(Self);
  FLogToolbar.Parent := FLogPanel;
  FLogToolbar.Align := alTop;
  FLogToolbar.Height := 40;
  FLogToolbar.BevelOuter := bvNone;

  LogLabel := TLabel.Create(Self);
  LogLabel.Parent := FLogToolbar;
  LogLabel.Caption := 'Журнал';
  LogLabel.Font.Style := [fsBold];
  LogLabel.SetBounds(12, 12, 70, 20);

  FLogFilter := TComboBox.Create(Self);
  FLogFilter.Parent := FLogToolbar;
  FLogFilter.Style := csDropDownList;
  FLogFilter.Items.Add('Все сообщения');
  FLogFilter.Items.Add('Ошибки');
  FLogFilter.Items.Add('Runtime');
  FLogFilter.ItemIndex := 0;
  FLogFilter.SetBounds(92, 6, 160, 30);

  ClearButton := TButton.Create(Self);
  ClearButton.Parent := FLogToolbar;
  ClearButton.Caption := 'Очистить';
  ClearButton.OnClick := @ClearLogClick;
  ClearButton.SetBounds(262, 6, 88, 30);

  FLogMemo := TMemo.Create(Self);
  FLogMemo.Parent := FLogPanel;
  FLogMemo.Align := alClient;
  FLogMemo.ReadOnly := True;
  FLogMemo.ScrollBars := ssAutoVertical;
  FLogMemo.Font.Name := 'Consolas';
  FLogMemo.Font.Size := 9;

  Splitter := TSplitter.Create(Self);
  Splitter.Parent := Self;
  Splitter.Align := alBottom;
  Splitter.Height := 6;
  Splitter.Top := FLogPanel.Top - 6;

  FGrid := TStringGrid.Create(Self);
  FGrid.Parent := Self;
  FGrid.Align := alClient;
  FGrid.BorderSpacing.Around := 12;
  FGrid.FixedCols := 0;
  FGrid.FixedRows := 1;
  FGrid.ColCount := 6;
  FGrid.RowCount := 5;
  FGrid.Row := 1;
  FGrid.Options := FGrid.Options + [goRowSelect, goColSizing, goDblClickAutoSize];
  FGrid.OnClick := @GridClick;
  FGrid.Cells[0, 0] := 'Профиль';
  FGrid.Cells[1, 0] := 'Протокол';
  FGrid.Cells[2, 0] := 'Сервер';
  FGrid.Cells[3, 0] := 'Задержка';
  FGrid.Cells[4, 0] := 'Источник';
  FGrid.Cells[5, 0] := 'Статус';
  FGrid.ColWidths[0] := 190;
  FGrid.ColWidths[1] := 90;
  FGrid.ColWidths[2] := 230;
  FGrid.ColWidths[3] := 90;
  FGrid.ColWidths[4] := 130;
  FGrid.ColWidths[5] := 90;

  FReadyTimer := TTimer.Create(Self);
  FReadyTimer.Enabled := False;
  FReadyTimer.Interval := 1100;
  FReadyTimer.OnTimer := @ReadyTimerTimer;
end;

procedure TMainForm.BuildTray;
var
  Item: TMenuItem;
begin
  FTrayMenu := TPopupMenu.Create(Self);
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, 'Показать Zarya', @TrayShowClick));
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, 'Запустить', @StartClick));
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, 'Остановить', @StopClick));
  Item := NewMenuItem(FTrayMenu, '-', nil);
  FTrayMenu.Items.Add(Item);
  FTrayMenu.Items.Add(NewMenuItem(FTrayMenu, 'Выход', @ExitClick));

  FTrayIcon := TTrayIcon.Create(Self);
  FTrayIcon.Hint := 'Zarya LCL Prototype — остановлено';
  FTrayIcon.PopUpMenu := FTrayMenu;
  FTrayIcon.OnDblClick := @TrayShowClick;
  if not Application.Icon.Empty then
    FTrayIcon.Icon.Assign(Application.Icon);
  FTrayIcon.Visible := True;
end;

procedure TMainForm.SeedProfiles;
begin
  FGrid.Cells[0, 1] := 'Амстердам · Demo';
  FGrid.Cells[1, 1] := 'VLESS';
  FGrid.Cells[2, 1] := 'nl.example.invalid:443';
  FGrid.Cells[3, 1] := '42 мс';
  FGrid.Cells[4, 1] := 'Вручную';
  FGrid.Cells[5, 1] := 'Готов';

  FGrid.Cells[0, 2] := 'Хельсинки · Demo';
  FGrid.Cells[1, 2] := 'VLESS';
  FGrid.Cells[2, 2] := 'fi.example.invalid:443';
  FGrid.Cells[3, 2] := '58 мс';
  FGrid.Cells[4, 2] := 'Nord Demo';
  FGrid.Cells[5, 2] := 'Готов';

  FGrid.Cells[0, 3] := 'Сингапур · Demo';
  FGrid.Cells[1, 3] := 'Trojan';
  FGrid.Cells[2, 3] := 'sg.example.invalid:443';
  FGrid.Cells[3, 3] := '184 мс';
  FGrid.Cells[4, 3] := 'Asia Demo';
  FGrid.Cells[5, 3] := 'Готов';

  FGrid.Cells[0, 4] := 'Локальная проверка';
  FGrid.Cells[1, 4] := 'SOCKS';
  FGrid.Cells[2, 4] := '127.0.0.1:10808';
  FGrid.Cells[3, 4] := '—';
  FGrid.Cells[4, 4] := 'Вручную';
  FGrid.Cells[5, 4] := 'Выключен';
end;

function TMainForm.SelectedProfileName: string;
begin
  if (FGrid.Row > 0) and (FGrid.Row < FGrid.RowCount) then
    Result := FGrid.Cells[0, FGrid.Row]
  else
    Result := '—';
end;

procedure TMainForm.AppendLog(const AText: string);
begin
  FLogMemo.Lines.Add(FormatDateTime('hh:nn:ss', Now) + '  ' + AText);
  FLogMemo.SelStart := Length(FLogMemo.Text);
end;

procedure TMainForm.UpdateRuntimeSurface;
var
  Theme: TZaryaTheme;
begin
  if FDarkTheme then
    Theme := ZaryaThemes.DarkTheme
  else
    Theme := ZaryaThemes.LightTheme;

  case FRuntimeState of
    rsStopped:
      begin
        FStateTitle.Caption := 'Runtime: остановлен — Xray system proxy';
        FStateBadge.Caption := 'Остановлено';
        FStateBadge.Color := Theme.Panel;
        FStateBadge.Font.Color := Theme.Muted;
        FStateDetail.Caption :=
          'Выбранный профиль: ' + SelectedProfileName + LineEnding +
          'Routing: Default     DNS: Default     Системный прокси: выключен';
        FStartButton.Enabled := True;
        FStopButton.Enabled := False;
        FStartMenuItem.Enabled := True;
        FStopMenuItem.Enabled := False;
        FStatusBar.SimpleText := 'Готово · системный прокси выключен · UI-only prototype';
        FTrayIcon.Hint := 'Zarya LCL Prototype — остановлено';
      end;
    rsConnecting:
      begin
        FStateTitle.Caption := 'Runtime: подключение — ожидание mixed-порта';
        FStateBadge.Caption := 'Проверка готовности';
        FStateBadge.Color := Theme.WarningSurface;
        FStateBadge.Font.Color := Theme.Warning;
        FStateDetail.Caption :=
          'Профиль: ' + SelectedProfileName + LineEnding +
          'Local proxy: 127.0.0.1:10808     Системный прокси: пока выключен';
        FStartButton.Enabled := False;
        FStopButton.Enabled := True;
        FStartMenuItem.Enabled := False;
        FStopMenuItem.Enabled := True;
        FStatusBar.SimpleText := 'Ожидание готовности 127.0.0.1:10808 (симуляция)…';
        FTrayIcon.Hint := 'Zarya LCL Prototype — подключение';
      end;
    rsRunning:
      begin
        FStateTitle.Caption := 'Runtime: работает — Xray system proxy';
        FStateBadge.Caption := 'Подключено';
        FStateBadge.Color := Theme.SuccessSurface;
        FStateBadge.Font.Color := Theme.Success;
        FStateDetail.Caption :=
          'Профиль: ' + SelectedProfileName + LineEnding +
          'Local proxy: 127.0.0.1:10808     Системный прокси: включён (симуляция)';
        FStartButton.Enabled := False;
        FStopButton.Enabled := True;
        FStartMenuItem.Enabled := False;
        FStopMenuItem.Enabled := True;
        FStatusBar.SimpleText := 'Подключено · mixed 127.0.0.1:10808 · UI-only prototype';
        FTrayIcon.Hint := 'Zarya LCL Prototype — подключено';
      end;
  end;
end;

procedure TMainForm.ApplyCurrentTheme;
var
  Theme: TZaryaTheme;
begin
  if FDarkTheme then
    Theme := ZaryaThemes.DarkTheme
  else
    Theme := ZaryaThemes.LightTheme;
  ApplyZaryaTheme(Self, Theme);
  FStatusPanel.Color := Theme.Surface;
  FToolbar.Color := Theme.Panel;
  FLogToolbar.Color := Theme.Panel;
  FLogPanel.Color := Theme.Surface;
  FStateTitle.Font.Size := 12;
  FStateTitle.Font.Style := [fsBold];
  UpdateRuntimeSurface;
end;

procedure TMainForm.StartClick(Sender: TObject);
begin
  if FRuntimeState <> rsStopped then
    Exit;
  FRuntimeState := rsConnecting;
  AppendLog('Core process started (simulation).');
  AppendLog('Waiting for local mixed port 127.0.0.1:10808 before enabling system proxy…');
  FReadyTimer.Enabled := True;
  UpdateRuntimeSurface;
end;

procedure TMainForm.StopClick(Sender: TObject);
begin
  if FRuntimeState = rsStopped then
    Exit;
  FReadyTimer.Enabled := False;
  AppendLog('System proxy restored; core stopped (simulation).');
  FRuntimeState := rsStopped;
  UpdateRuntimeSurface;
end;

procedure TMainForm.ReadyTimerTimer(Sender: TObject);
begin
  FReadyTimer.Enabled := False;
  if FRuntimeState <> rsConnecting then
    Exit;
  FRuntimeState := rsRunning;
  AppendLog('Mixed port accepted a connection (simulation).');
  AppendLog('System proxy enabled only after readiness (simulation).');
  UpdateRuntimeSurface;
end;

procedure TMainForm.TestClick(Sender: TObject);
const
  DemoLatency: array[1..4] of string = ('39 мс', '61 мс', '179 мс', '1 мс');
var
  I: Integer;
begin
  for I := 1 to 4 do
    FGrid.Cells[3, I] := DemoLatency[I];
  AppendLog('Profile latency test completed (simulation).');
  FStatusBar.SimpleText := 'Проверено 4 профиля · UI-only prototype';
end;

procedure TMainForm.AddClick(Sender: TObject);
var
  ProfileName: string;
begin
  ProfileName := 'Новый профиль';
  if not InputQuery('Добавить профиль', 'Название:', ProfileName) then
    Exit;
  FGrid.RowCount := FGrid.RowCount + 1;
  FGrid.Cells[0, FGrid.RowCount - 1] := ProfileName;
  FGrid.Cells[1, FGrid.RowCount - 1] := 'VLESS';
  FGrid.Cells[2, FGrid.RowCount - 1] := 'server.example.invalid:443';
  FGrid.Cells[3, FGrid.RowCount - 1] := '—';
  FGrid.Cells[4, FGrid.RowCount - 1] := 'Вручную';
  FGrid.Cells[5, FGrid.RowCount - 1] := 'Готов';
  FGrid.Row := FGrid.RowCount - 1;
  AppendLog('Added a local demo profile: ' + ProfileName);
  UpdateRuntimeSurface;
end;

procedure TMainForm.ImportClick(Sender: TObject);
begin
  MessageDlg('Импорт профилей',
    'В прототипе импорт отключён: здесь проверяется только UI LCL.',
    mtInformation, [mbOK], 0);
end;

procedure TMainForm.SubscriptionsClick(Sender: TObject);
begin
  MessageDlg('Подписки',
    'Список подписок будет отдельной формой. Сетевые запросы прототип не выполняет.',
    mtInformation, [mbOK], 0);
end;

procedure TMainForm.SettingsClick(Sender: TObject);
var
  Dialog: TSettingsDialog;
begin
  Dialog := TSettingsDialog.Create(Self, FDarkTheme, FMinimizeToTray);
  try
    if Dialog.ShowModal = mrOk then
    begin
      FDarkTheme := Dialog.DarkThemeSelected;
      FMinimizeToTray := Dialog.MinimizeToTraySelected;
      ApplyCurrentTheme;
      AppendLog('Interface settings applied.');
    end;
  finally
    Dialog.Free;
  end;
end;

procedure TMainForm.AboutClick(Sender: TObject);
begin
  MessageDlg('О прототипе',
    'Zarya LCL/VCL feasibility prototype' + LineEnding + LineEnding +
    'Только интерфейс: стандартные LCL-контролы, Win32 tray, темы и DPI.' +
    LineEnding + 'Xray не запускается, системный прокси не изменяется.',
    mtInformation, [mbOK], 0);
end;

procedure TMainForm.ExitClick(Sender: TObject);
begin
  FQuitting := True;
  FTrayIcon.Visible := False;
  Close;
end;

procedure TMainForm.TrayShowClick(Sender: TObject);
begin
  if Visible then
    Hide
  else
  begin
    Show;
    WindowState := wsNormal;
    BringToFront;
  end;
end;

procedure TMainForm.ClearLogClick(Sender: TObject);
begin
  FLogMemo.Clear;
end;

procedure TMainForm.GridClick(Sender: TObject);
begin
  UpdateRuntimeSurface;
end;

procedure TMainForm.FormResize(Sender: TObject);
begin
  if not Assigned(FStartButton) then
    Exit;
  FStartButton.Left := ClientWidth - 318;
  FStopButton.Left := ClientWidth - 176;
  FSettingsButton.Left := ClientWidth - 176;
  FStateDetail.Width := ClientWidth - 390;
end;

procedure TMainForm.FormCloseQuery(Sender: TObject; var CanClose: Boolean);
begin
  if FMinimizeToTray and (not FQuitting) then
  begin
    Hide;
    CanClose := False;
    AppendLog('Main window hidden to tray.');
  end
  else
    CanClose := True;
end;

end.
