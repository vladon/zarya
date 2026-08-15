unit FpcPolicyStore;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, ZaryaRouting, ZaryaDns, ZaryaPolicyStore;

type
  TFpcRoutingProfileStore = class(TInterfacedObject, IRoutingProfileStore)
  private
    FFileName: string;
  public
    constructor Create(const AFileName: string);
    function Load(out AProfiles: TZaryaRoutingProfiles;
      out AError: string): Boolean;
    function Save(const AProfiles: TZaryaRoutingProfiles;
      out AError: string): Boolean;
    function FileName: string;
  end;

  TFpcDnsProfileStore = class(TInterfacedObject, IDnsProfileStore)
  private
    FFileName: string;
  public
    constructor Create(const AFileName: string);
    function Load(out AProfiles: TZaryaDnsProfiles;
      out AError: string): Boolean;
    function Save(const AProfiles: TZaryaDnsProfiles;
      out AError: string): Boolean;
    function FileName: string;
  end;

implementation

uses
  fpjson, jsonparser;

procedure AtomicWriteJson(const AFileName: string; const ARoot: TJSONData);
var
  DirectoryName: string;
  TempFile: string;
  BackupFile: string;
  Stream: TFileStream;
  Json: UTF8String;
begin
  DirectoryName := ExtractFileDir(AFileName);
  if (DirectoryName <> '') and not ForceDirectories(DirectoryName) then
    raise Exception.Create('Cannot create policy data directory: ' + DirectoryName);
  TempFile := AFileName + '.tmp';
  BackupFile := AFileName + '.bak';
  Json := UTF8String(ARoot.FormatJSON);
  Stream := TFileStream.Create(TempFile, fmCreate or fmShareExclusive);
  try
    if Length(Json) > 0 then Stream.WriteBuffer(Json[1], Length(Json));
  finally
    Stream.Free;
  end;
  if FileExists(BackupFile) then DeleteFile(BackupFile);
  if FileExists(AFileName) and not RenameFile(AFileName, BackupFile) then
  begin
    DeleteFile(TempFile);
    raise Exception.Create('Cannot prepare atomic policy file update.');
  end;
  if not RenameFile(TempFile, AFileName) then
  begin
    if FileExists(BackupFile) then RenameFile(BackupFile, AFileName);
    DeleteFile(TempFile);
    raise Exception.Create('Cannot install updated policy file.');
  end;
  if FileExists(BackupFile) then DeleteFile(BackupFile);
end;

function IsRoutingBuiltInId(const AId: string): Boolean;
begin
  Result := SameText(AId, RoutingProxyAllId) or
    SameText(AId, RoutingBypassLanId) or SameText(AId, RoutingBypassRuId) or
    SameText(AId, RoutingBypassLanAndRuId) or
    SameText(AId, RoutingCustomTemplateId);
end;

function IsDnsBuiltInId(const AId: string): Boolean;
begin
  Result := SameText(AId, DnsSystemId) or SameText(AId, DnsSecureRemoteId) or
    SameText(AId, DnsChinaDirectGlobalRemoteId) or
    SameText(AId, DnsCustomTemplateId);
end;

function JsonStrings(const AArray: TJSONArray): TZaryaStringArray;
var
  I: Integer;
begin
  Result := nil;
  SetLength(Result, AArray.Count);
  for I := 0 to AArray.Count - 1 do Result[I] := AArray.Strings[I];
end;

function DnsJsonStrings(const AArray: TJSONArray): TZaryaDnsStringArray;
var
  I: Integer;
begin
  Result := nil;
  SetLength(Result, AArray.Count);
  for I := 0 to AArray.Count - 1 do Result[I] := AArray.Strings[I];
end;

function StringsToJson(const AValues: TZaryaStringArray): TJSONArray;
var
  Value: string;
begin
  Result := TJSONArray.Create;
  for Value in AValues do Result.Add(Value);
end;

function DnsStringsToJson(const AValues: TZaryaDnsStringArray): TJSONArray;
var
  Value: string;
begin
  Result := TJSONArray.Create;
  for Value in AValues do Result.Add(Value);
end;

constructor TFpcRoutingProfileStore.Create(const AFileName: string);
begin
  inherited Create;
  FFileName := AFileName;
end;

function TFpcRoutingProfileStore.FileName: string;
begin
  Result := FFileName;
end;

function TFpcRoutingProfileStore.Load(out AProfiles: TZaryaRoutingProfiles;
  out AError: string): Boolean;
var
  Data: TJSONData;
  Root, Item, RuleObject: TJSONObject;
  Items, Rules: TJSONArray;
  Stream: TFileStream;
  BuiltIns: TZaryaRoutingProfiles;
  Candidate: TZaryaRoutingProfile;
  I, J, OutIndex, K: Integer;
begin
  AError := '';
  BuiltIns := CreateBuiltInRoutingProfiles;
  AProfiles := BuiltIns;
  if not FileExists(FFileName) then Exit(True);
  Data := nil;
  try
    Stream := TFileStream.Create(FFileName, fmOpenRead or fmShareDenyWrite);
    try
      Data := GetJSON(Stream);
    finally
      Stream.Free;
    end;
    if Data.JSONType <> jtObject then
      raise Exception.Create('routing.json root must be an object.');
    Root := TJSONObject(Data);
    if Root.Get('version', Root.Get('schemaVersion', 1)) <> 1 then
      raise Exception.Create('Unsupported routing.json schema version.');
    Items := Root.Arrays['profiles'];
    SetLength(AProfiles, Length(BuiltIns) + Items.Count);
    for I := 0 to High(BuiltIns) do AProfiles[I] := BuiltIns[I];
    OutIndex := Length(BuiltIns);
    for I := 0 to Items.Count - 1 do
    begin
      if Items.Items[I].JSONType <> jtObject then
        raise Exception.CreateFmt('Routing profile #%d must be an object.', [I + 1]);
      Item := Items.Objects[I];
      Candidate := Default(TZaryaRoutingProfile);
      Candidate.Id := Item.Get('id', '');
      if IsRoutingBuiltInId(Candidate.Id) then Continue;
      Candidate.Name := Item.Get('name', '');
      Candidate.Mode := RoutingModeFromString(Item.Get('mode', 'custom'));
      Candidate.Enabled := Item.Get('enabled', True);
      Candidate.DomainStrategy := Item.Get('domainStrategy', 'AsIs');
      Candidate.IsBuiltIn := False;
      Candidate.CreatedAt := Item.Get('createdAt', '');
      Candidate.UpdatedAt := Item.Get('updatedAt', '');
      Rules := Item.Arrays['rules'];
      SetLength(Candidate.Rules, Rules.Count);
      for J := 0 to Rules.Count - 1 do
      begin
        if Rules.Items[J].JSONType <> jtObject then
          raise Exception.CreateFmt('Routing rule #%d must be an object.', [J + 1]);
        RuleObject := Rules.Objects[J];
        Candidate.Rules[J].Id := RuleObject.Get('id', '');
        Candidate.Rules[J].Enabled := RuleObject.Get('enabled', True);
        Candidate.Rules[J].RuleType := RoutingRuleTypeFromString(
          RuleObject.Get('type', 'domain'));
        Candidate.Rules[J].Action := RoutingActionFromString(
          RuleObject.Get('action', 'proxy'));
        Candidate.Rules[J].Note := RuleObject.Get('note', '');
        Candidate.Rules[J].Values := JsonStrings(RuleObject.Arrays['values']);
      end;
      if not ValidateRoutingProfile(Candidate, AError) then Exit(False);
      for K := 0 to OutIndex - 1 do
        if SameText(AProfiles[K].Id, Candidate.Id) then
          raise Exception.Create('Duplicate routing profile id: ' + Candidate.Id);
      AProfiles[OutIndex] := Candidate;
      Inc(OutIndex);
    end;
    SetLength(AProfiles, OutIndex);
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      SetLength(AProfiles, 0);
      Result := False;
    end;
  end;
  Data.Free;
end;

function TFpcRoutingProfileStore.Save(const AProfiles: TZaryaRoutingProfiles;
  out AError: string): Boolean;
var
  Root, Item, RuleObject: TJSONObject;
  Items, Rules: TJSONArray;
  Profile: TZaryaRoutingProfile;
  Rule: TZaryaRoutingRule;
  I, J: Integer;
begin
  AError := '';
  Root := nil;
  try
    for I := 0 to High(AProfiles) do
    begin
      if not ValidateRoutingProfile(AProfiles[I], AError) then Exit(False);
      for J := 0 to I - 1 do
        if SameText(AProfiles[I].Id, AProfiles[J].Id) then
          raise Exception.Create('Duplicate routing profile id: ' + AProfiles[I].Id);
    end;
    Root := TJSONObject.Create;
    Root.Add('schemaVersion', 1);
    Root.Add('version', 1);
    Items := TJSONArray.Create;
    Root.Add('profiles', Items);
    for Profile in AProfiles do
    begin
      Item := TJSONObject.Create;
      Item.Add('id', Profile.Id);
      Item.Add('name', Profile.Name);
      Item.Add('mode', RoutingModeToString(Profile.Mode));
      Item.Add('enabled', Profile.Enabled);
      Item.Add('domainStrategy', Profile.DomainStrategy);
      Item.Add('isBuiltIn', Profile.IsBuiltIn);
      Item.Add('createdAt', Profile.CreatedAt);
      Item.Add('updatedAt', Profile.UpdatedAt);
      Rules := TJSONArray.Create;
      Item.Add('rules', Rules);
      for Rule in Profile.Rules do
      begin
        RuleObject := TJSONObject.Create;
        RuleObject.Add('id', Rule.Id);
        RuleObject.Add('enabled', Rule.Enabled);
        RuleObject.Add('type', RoutingRuleTypeToString(Rule.RuleType));
        RuleObject.Add('action', RoutingActionToString(Rule.Action));
        RuleObject.Add('note', Rule.Note);
        RuleObject.Add('values', StringsToJson(Rule.Values));
        Rules.Add(RuleObject);
      end;
      Items.Add(Item);
    end;
    AtomicWriteJson(FFileName, Root);
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Root.Free;
end;

constructor TFpcDnsProfileStore.Create(const AFileName: string);
begin
  inherited Create;
  FFileName := AFileName;
end;

function TFpcDnsProfileStore.FileName: string;
begin
  Result := FFileName;
end;

function TFpcDnsProfileStore.Load(out AProfiles: TZaryaDnsProfiles;
  out AError: string): Boolean;
var
  Data: TJSONData;
  Root, Item, Hosts, ServerObject: TJSONObject;
  Items, Servers: TJSONArray;
  Stream: TFileStream;
  BuiltIns: TZaryaDnsProfiles;
  Candidate: TZaryaDnsProfile;
  I, J, OutIndex, K: Integer;
begin
  AError := '';
  BuiltIns := CreateBuiltInDnsProfiles;
  AProfiles := BuiltIns;
  if not FileExists(FFileName) then Exit(True);
  Data := nil;
  try
    Stream := TFileStream.Create(FFileName, fmOpenRead or fmShareDenyWrite);
    try
      Data := GetJSON(Stream);
    finally
      Stream.Free;
    end;
    if Data.JSONType <> jtObject then
      raise Exception.Create('dns.json root must be an object.');
    Root := TJSONObject(Data);
    if Root.Get('version', Root.Get('schemaVersion', 1)) <> 1 then
      raise Exception.Create('Unsupported dns.json schema version.');
    Items := Root.Arrays['profiles'];
    SetLength(AProfiles, Length(BuiltIns) + Items.Count);
    for I := 0 to High(BuiltIns) do AProfiles[I] := BuiltIns[I];
    OutIndex := Length(BuiltIns);
    for I := 0 to Items.Count - 1 do
    begin
      if Items.Items[I].JSONType <> jtObject then
        raise Exception.CreateFmt('DNS profile #%d must be an object.', [I + 1]);
      Item := Items.Objects[I];
      Candidate := Default(TZaryaDnsProfile);
      Candidate.Id := Item.Get('id', '');
      if IsDnsBuiltInId(Candidate.Id) then Continue;
      Candidate.Name := Item.Get('name', '');
      Candidate.Mode := DnsModeFromString(Item.Get('mode', 'custom'));
      Candidate.Enabled := Item.Get('enabled', True);
      Candidate.IsBuiltIn := False;
      Candidate.QueryStrategy := DnsQueryStrategyFromString(
        Item.Get('queryStrategy', 'system-default'));
      Candidate.DisableCache := Item.Get('disableCache', False);
      Candidate.DisableFallback := Item.Get('disableFallback', False);
      Candidate.DisableFallbackIfMatch := Item.Get('disableFallbackIfMatch', False);
      Candidate.CreatedAt := Item.Get('createdAt', '');
      Candidate.UpdatedAt := Item.Get('updatedAt', '');
      Hosts := Item.Objects['hosts'];
      SetLength(Candidate.Hosts, Hosts.Count);
      for J := 0 to Hosts.Count - 1 do
      begin
        Candidate.Hosts[J].Host := Hosts.Names[J];
        Candidate.Hosts[J].Address := Hosts.Strings[Hosts.Names[J]];
      end;
      Servers := Item.Arrays['servers'];
      SetLength(Candidate.Servers, Servers.Count);
      for J := 0 to Servers.Count - 1 do
      begin
        if Servers.Items[J].JSONType <> jtObject then
          raise Exception.CreateFmt('DNS server #%d must be an object.', [J + 1]);
        ServerObject := Servers.Objects[J];
        Candidate.Servers[J].Id := ServerObject.Get('id', '');
        Candidate.Servers[J].Enabled := ServerObject.Get('enabled', True);
        Candidate.Servers[J].Kind := DnsServerKindFromString(
          ServerObject.Get('kind', 'plain'));
        Candidate.Servers[J].Address := ServerObject.Get('address', '');
        Candidate.Servers[J].Port := ServerObject.Get('port', 0);
        Candidate.Servers[J].QueryStrategy := ServerObject.Get('queryStrategy', '');
        Candidate.Servers[J].TimeoutMs := ServerObject.Get('timeoutMs', 0);
        Candidate.Servers[J].Tag := ServerObject.Get('tag', '');
        Candidate.Servers[J].SkipFallback := ServerObject.Get('skipFallback', False);
        Candidate.Servers[J].Note := ServerObject.Get('note', '');
        Candidate.Servers[J].Domains := DnsJsonStrings(ServerObject.Arrays['domains']);
        Candidate.Servers[J].ExpectIps := DnsJsonStrings(ServerObject.Arrays['expectIPs']);
      end;
      if not ValidateDnsProfile(Candidate, AError) then Exit(False);
      for K := 0 to OutIndex - 1 do
        if SameText(AProfiles[K].Id, Candidate.Id) then
          raise Exception.Create('Duplicate DNS profile id: ' + Candidate.Id);
      AProfiles[OutIndex] := Candidate;
      Inc(OutIndex);
    end;
    SetLength(AProfiles, OutIndex);
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      SetLength(AProfiles, 0);
      Result := False;
    end;
  end;
  Data.Free;
end;

function TFpcDnsProfileStore.Save(const AProfiles: TZaryaDnsProfiles;
  out AError: string): Boolean;
var
  Root, Item, Hosts, ServerObject: TJSONObject;
  Items, Servers: TJSONArray;
  Profile: TZaryaDnsProfile;
  Host: TZaryaDnsHost;
  Server: TZaryaDnsServer;
  I, J: Integer;
begin
  AError := '';
  Root := nil;
  try
    for I := 0 to High(AProfiles) do
    begin
      if not ValidateDnsProfile(AProfiles[I], AError) then Exit(False);
      for J := 0 to I - 1 do
        if SameText(AProfiles[I].Id, AProfiles[J].Id) then
          raise Exception.Create('Duplicate DNS profile id: ' + AProfiles[I].Id);
    end;
    Root := TJSONObject.Create;
    Root.Add('schemaVersion', 1);
    Root.Add('version', 1);
    Items := TJSONArray.Create;
    Root.Add('profiles', Items);
    for Profile in AProfiles do
    begin
      Item := TJSONObject.Create;
      Item.Add('id', Profile.Id);
      Item.Add('name', Profile.Name);
      Item.Add('mode', DnsModeToString(Profile.Mode));
      Item.Add('enabled', Profile.Enabled);
      Item.Add('isBuiltIn', Profile.IsBuiltIn);
      Item.Add('queryStrategy', DnsQueryStrategyToString(Profile.QueryStrategy));
      Item.Add('disableCache', Profile.DisableCache);
      Item.Add('disableFallback', Profile.DisableFallback);
      Item.Add('disableFallbackIfMatch', Profile.DisableFallbackIfMatch);
      Item.Add('createdAt', Profile.CreatedAt);
      Item.Add('updatedAt', Profile.UpdatedAt);
      Hosts := TJSONObject.Create;
      for Host in Profile.Hosts do Hosts.Add(Host.Host, Host.Address);
      Item.Add('hosts', Hosts);
      Servers := TJSONArray.Create;
      Item.Add('servers', Servers);
      for Server in Profile.Servers do
      begin
        ServerObject := TJSONObject.Create;
        ServerObject.Add('id', Server.Id);
        ServerObject.Add('enabled', Server.Enabled);
        ServerObject.Add('kind', DnsServerKindToString(Server.Kind));
        ServerObject.Add('address', Server.Address);
        ServerObject.Add('port', Server.Port);
        ServerObject.Add('queryStrategy', Server.QueryStrategy);
        ServerObject.Add('timeoutMs', Server.TimeoutMs);
        ServerObject.Add('tag', Server.Tag);
        ServerObject.Add('skipFallback', Server.SkipFallback);
        ServerObject.Add('note', Server.Note);
        ServerObject.Add('domains', DnsStringsToJson(Server.Domains));
        ServerObject.Add('expectIPs', DnsStringsToJson(Server.ExpectIps));
        Servers.Add(ServerObject);
      end;
      Items.Add(Item);
    end;
    AtomicWriteJson(FFileName, Root);
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Root.Free;
end;

end.
