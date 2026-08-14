unit FpcSubscriptionStore;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, ZaryaSubscription;

type
  TFpcSubscriptionStore = class
  private
    FFileName: string;
  public
    constructor Create(const AFileName: string);
    function Load(out ASubscriptions: TZaryaSubscriptions;
      out AError: string): Boolean;
    function Save(const ASubscriptions: TZaryaSubscriptions;
      out AError: string): Boolean;
    property FileName: string read FFileName;
  end;

implementation

uses
  fpjson, jsonparser;

constructor TFpcSubscriptionStore.Create(const AFileName: string);
begin
  inherited Create;
  FFileName := AFileName;
end;

function TFpcSubscriptionStore.Load(out ASubscriptions: TZaryaSubscriptions;
  out AError: string): Boolean;
var
  Stream: TFileStream;
  RootData: TJSONData;
  Root: TJSONObject;
  Items: TJSONArray;
  Item: TJSONObject;
  I: Integer;
begin
  SetLength(ASubscriptions, 0);
  AError := '';
  if not FileExists(FFileName) then
    Exit(True);
  RootData := nil;
  try
    Stream := TFileStream.Create(FFileName, fmOpenRead or fmShareDenyWrite);
    try
      RootData := GetJSON(Stream);
    finally
      Stream.Free;
    end;
    if RootData.JSONType <> jtObject then
      raise Exception.Create('Корневой элемент subscriptions.json должен быть объектом.');
    Root := TJSONObject(RootData);
    Items := Root.Arrays['subscriptions'];
    SetLength(ASubscriptions, Items.Count);
    for I := 0 to Items.Count - 1 do
    begin
      if Items.Items[I].JSONType <> jtObject then
        raise Exception.CreateFmt('Подписка #%d должна быть объектом.', [I + 1]);
      Item := Items.Objects[I];
      ASubscriptions[I] := CreateEmptySubscription;
      ASubscriptions[I].Id := Item.Get('id', ASubscriptions[I].Id);
      ASubscriptions[I].Name := Item.Get('name', '');
      ASubscriptions[I].Url := Item.Get('url', '');
      ASubscriptions[I].Enabled := Item.Get('enabled', True);
      ASubscriptions[I].LastUpdatedAt := Item.Get('lastUpdatedAt', '');
      ASubscriptions[I].LastStatus := SubscriptionStatusFromString(
        Item.Get('lastStatus', 'never_updated'));
      ASubscriptions[I].LastError := Item.Get('lastError', '');
      ASubscriptions[I].ProfileCount := Item.Get('profileCount', 0);
      ASubscriptions[I].UserAgent := Item.Get('userAgent', '');
      ASubscriptions[I].Remarks := Item.Get('remarks', '');
      if not ASubscriptions[I].Enabled then
        ASubscriptions[I].LastStatus := ssDisabled;
    end;
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      SetLength(ASubscriptions, 0);
      Result := False;
    end;
  end;
  RootData.Free;
end;

function TFpcSubscriptionStore.Save(
  const ASubscriptions: TZaryaSubscriptions; out AError: string): Boolean;
var
  Root: TJSONObject;
  Items: TJSONArray;
  Item: TJSONObject;
  Json: UTF8String;
  Stream: TFileStream;
  DirectoryName: string;
  TempFile: string;
  BackupFile: string;
  I: Integer;
begin
  AError := '';
  Root := nil;
  TempFile := FFileName + '.tmp';
  BackupFile := FFileName + '.bak';
  try
    DirectoryName := ExtractFileDir(FFileName);
    if (DirectoryName <> '') and (not ForceDirectories(DirectoryName)) then
      raise Exception.Create('Не удалось создать каталог данных: ' + DirectoryName);
    Root := TJSONObject.Create;
    Root.Add('schemaVersion', 1);
    Root.Add('version', 1);
    Items := TJSONArray.Create;
    Root.Add('subscriptions', Items);
    for I := 0 to High(ASubscriptions) do
    begin
      Item := TJSONObject.Create;
      Item.Add('id', ASubscriptions[I].Id);
      Item.Add('name', ASubscriptions[I].Name);
      Item.Add('url', ASubscriptions[I].Url);
      Item.Add('enabled', ASubscriptions[I].Enabled);
      if ASubscriptions[I].LastUpdatedAt <> '' then
        Item.Add('lastUpdatedAt', ASubscriptions[I].LastUpdatedAt);
      Item.Add('lastStatus', SubscriptionStatusToString(
        ASubscriptions[I].LastStatus));
      Item.Add('lastError', ASubscriptions[I].LastError);
      Item.Add('profileCount', ASubscriptions[I].ProfileCount);
      if ASubscriptions[I].UserAgent <> '' then
        Item.Add('userAgent', ASubscriptions[I].UserAgent);
      if ASubscriptions[I].Remarks <> '' then
        Item.Add('remarks', ASubscriptions[I].Remarks);
      Items.Add(Item);
    end;
    Json := UTF8String(Root.FormatJSON);
    Stream := TFileStream.Create(TempFile, fmCreate or fmShareExclusive);
    try
      if Length(Json) > 0 then
        Stream.WriteBuffer(Json[1], Length(Json));
    finally
      Stream.Free;
    end;
    if FileExists(BackupFile) then
      DeleteFile(BackupFile);
    if FileExists(FFileName) and (not RenameFile(FFileName, BackupFile)) then
      raise Exception.Create('Не удалось подготовить атомарную запись subscriptions.json.');
    if not RenameFile(TempFile, FFileName) then
    begin
      if FileExists(BackupFile) then
        RenameFile(BackupFile, FFileName);
      raise Exception.Create('Не удалось установить новый subscriptions.json.');
    end;
    if FileExists(BackupFile) then
      DeleteFile(BackupFile);
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  if FileExists(TempFile) then
    DeleteFile(TempFile);
  Root.Free;
end;

end.
