unit ZaryaGeoData;

{$mode objfpc}{$H+}

interface

uses
  SysUtils, ZaryaRouting, ZaryaDns,
  WindowsSubscriptionDownloader;

type
  TZaryaGeoDataKind = (gdkGeoIp, gdkGeoSite);
  TZaryaGeoDataStatus = record
    Kind: TZaryaGeoDataKind;
    FileName: string;
    FilePath: string;
    Exists: Boolean;
    SizeBytes: Int64;
    Sha256: string;
    ExpectedSha256: string;
    Verified: Boolean;
    ErrorMessage: string;
  end;
  TZaryaGeoDataStatuses = array of TZaryaGeoDataStatus;

  TZaryaGeoDataSource = record
    Id: string;
    Name: string;
    BaseUrl: string;
  end;
  TZaryaGeoDataSources = array of TZaryaGeoDataSource;

  IGeoDataManager = interface
    ['{4B299315-4E19-4A6A-9E51-BF989F202B00}']
    function TargetDirectory: string;
    function CheckAll(out AStatuses: TZaryaGeoDataStatuses;
      out AError: string): Boolean;
    function UpdateAll(const ASourceId: string;
      const AProgress: TZaryaSubscriptionDownloadProgress;
      const ACancelCheck: TZaryaSubscriptionCancelCheck;
      out AError: string): Boolean;
    function RequiredFilesPresent(const ARouting: TZaryaRoutingProfile;
      const ADns: TZaryaDnsProfile; out AMissing,
      AError: string): Boolean;
  end;

  TZaryaGeoDataManager = class(TInterfacedObject, IGeoDataManager)
  private
    FTargetDirectory: string;
    function MetadataFile: string;
    function DownloadOne(const ASource: TZaryaGeoDataSource;
      const AKind: TZaryaGeoDataKind;
      const AProgress: TZaryaSubscriptionDownloadProgress;
      const ACancelCheck: TZaryaSubscriptionCancelCheck;
      out ATempFile, AExpectedSha256, AError: string): Boolean;
  public
    constructor Create(const ATargetDirectory: string);
    function TargetDirectory: string;
    function CheckAll(out AStatuses: TZaryaGeoDataStatuses;
      out AError: string): Boolean;
    function UpdateAll(const ASourceId: string;
      const AProgress: TZaryaSubscriptionDownloadProgress;
      const ACancelCheck: TZaryaSubscriptionCancelCheck;
      out AError: string): Boolean;
    function RequiredFilesPresent(const ARouting: TZaryaRoutingProfile;
      const ADns: TZaryaDnsProfile; out AMissing,
      AError: string): Boolean;
  end;

function BuiltInGeoDataSources: TZaryaGeoDataSources;
function GeoDataSourceById(const AId: string): TZaryaGeoDataSource;
function GeoDataFileName(const AKind: TZaryaGeoDataKind): string;
function ParseSha256Sum(const AContent, AExpectedFileName: string): string;
function InstallVerifiedGeoDataFile(const ADownloadedFile, AFinalFile,
  AExpectedSha256: string; out AError: string): Boolean;
function InstallVerifiedGeoDataPair(const ADownloadedGeoIp,
  ADownloadedGeoSite, AFinalGeoIp, AFinalGeoSite, AExpectedGeoIp,
  AExpectedGeoSite: string; out AError: string): Boolean;

implementation

uses
  Classes, IniFiles, ZaryaFileIntegrity;

const
  MaxGeoDataBytes = Int64(128) * 1024 * 1024;

function MakeSource(const AId, AName, ARepository: string): TZaryaGeoDataSource;
begin
  Result.Id := AId;
  Result.Name := AName;
  Result.BaseUrl := 'https://github.com/' + ARepository +
    '/releases/latest/download/';
end;

function BuiltInGeoDataSources: TZaryaGeoDataSources;
begin
  Result := nil;
  SetLength(Result, 3);
  Result[0] := MakeSource('runetfreedom',
    'runetfreedom russia-v2ray-rules-dat',
    'runetfreedom/russia-v2ray-rules-dat');
  Result[1] := MakeSource('loyalsoldier',
    'Loyalsoldier v2ray-rules-dat', 'Loyalsoldier/v2ray-rules-dat');
  Result[2] := MakeSource('chocolate4u', 'Chocolate4U Iran-v2ray-rules',
    'Chocolate4U/Iran-v2ray-rules');
end;

function GeoDataSourceById(const AId: string): TZaryaGeoDataSource;
var
  Source: TZaryaGeoDataSource;
begin
  for Source in BuiltInGeoDataSources do
    if SameText(Source.Id, AId) then Exit(Source);
  Result := BuiltInGeoDataSources[0];
end;

function GeoDataFileName(const AKind: TZaryaGeoDataKind): string;
begin
  if AKind = gdkGeoIp then Result := 'geoip.dat'
  else Result := 'geosite.dat';
end;

function IsHex(const AValue: string): Boolean;
var
  Ch: Char;
begin
  if Length(AValue) <> 64 then Exit(False);
  for Ch in AValue do
    if not (Ch in ['0'..'9', 'a'..'f', 'A'..'F']) then Exit(False);
  Result := True;
end;

function ParseSha256Sum(const AContent, AExpectedFileName: string): string;
var
  Lines: TStringList;
  Line, Hash, FileName: string;
  I, Marker: Integer;
begin
  Result := '';
  Lines := TStringList.Create;
  try
    Lines.Text := AContent;
    for I := 0 to Lines.Count - 1 do
    begin
      Line := Trim(Lines[I]);
      if (Line = '') or (Line[1] = '#') then Continue;
      Marker := 1;
      while (Marker <= Length(Line)) and not (Line[Marker] in [' ', #9]) do
        Inc(Marker);
      Hash := Copy(Line, 1, Marker - 1);
      while (Marker <= Length(Line)) and (Line[Marker] in [' ', #9, '*']) do
        Inc(Marker);
      FileName := Trim(Copy(Line, Marker, MaxInt));
      FileName := ExtractFileName(StringReplace(FileName, '/', PathDelim,
        [rfReplaceAll]));
      if IsHex(Hash) and SameText(FileName, AExpectedFileName) then
        Exit(LowerCase(Hash));
    end;
  finally
    Lines.Free;
  end;
end;

function InstallVerifiedGeoDataFile(const ADownloadedFile, AFinalFile,
  AExpectedSha256: string; out AError: string): Boolean;
var
  Actual: string;
  BackupFile: string;
begin
  Result := False;
  AError := '';
  if not IsHex(Trim(AExpectedSha256)) then
  begin
    AError := 'Invalid expected SHA-256.';
    Exit;
  end;
  if not Sha256File(ADownloadedFile, Actual, AError) then Exit;
  if not SameText(Actual, Trim(AExpectedSha256)) then
  begin
    AError := 'SHA-256 mismatch.';
    Exit;
  end;
  BackupFile := AFinalFile + '.bak';
  if FileExists(BackupFile) and not DeleteFile(BackupFile) then
  begin
    AError := 'Cannot replace previous geodata backup.';
    Exit;
  end;
  if FileExists(AFinalFile) and not RenameFile(AFinalFile, BackupFile) then
  begin
    AError := 'Cannot backup existing geodata file.';
    Exit;
  end;
  if not RenameFile(ADownloadedFile, AFinalFile) then
  begin
    if FileExists(BackupFile) then RenameFile(BackupFile, AFinalFile);
    AError := 'Cannot atomically install verified geodata file.';
    Exit;
  end;
  if FileExists(BackupFile) then DeleteFile(BackupFile);
  Result := True;
end;

function VerifyDownloadedFile(const AFileName, AExpectedSha256: string;
  out AError: string): Boolean;
var
  Actual: string;
begin
  Result := False;
  if not IsHex(Trim(AExpectedSha256)) then
  begin
    AError := 'Invalid expected SHA-256.';
    Exit;
  end;
  if not Sha256File(AFileName, Actual, AError) then Exit;
  if not SameText(Actual, Trim(AExpectedSha256)) then
  begin
    AError := 'SHA-256 mismatch.';
    Exit;
  end;
  Result := True;
end;

function InstallVerifiedGeoDataPair(const ADownloadedGeoIp,
  ADownloadedGeoSite, AFinalGeoIp, AFinalGeoSite, AExpectedGeoIp,
  AExpectedGeoSite: string; out AError: string): Boolean;
var
  GeoIpBackup, GeoSiteBackup: string;
  HadGeoIp, HadGeoSite, InstalledGeoIp: Boolean;
begin
  Result := False;
  AError := '';
  if not VerifyDownloadedFile(ADownloadedGeoIp, AExpectedGeoIp, AError) or
    not VerifyDownloadedFile(ADownloadedGeoSite, AExpectedGeoSite,
      AError) then Exit;
  GeoIpBackup := AFinalGeoIp + '.bak';
  GeoSiteBackup := AFinalGeoSite + '.bak';
  if FileExists(GeoIpBackup) or FileExists(GeoSiteBackup) then
  begin
    AError := 'Unfinished geodata replacement backup exists.';
    Exit;
  end;
  HadGeoIp := FileExists(AFinalGeoIp);
  HadGeoSite := FileExists(AFinalGeoSite);
  if HadGeoIp and not RenameFile(AFinalGeoIp, GeoIpBackup) then
  begin
    AError := 'Cannot backup existing geoip.dat.';
    Exit;
  end;
  if HadGeoSite and not RenameFile(AFinalGeoSite, GeoSiteBackup) then
  begin
    if HadGeoIp then RenameFile(GeoIpBackup, AFinalGeoIp);
    AError := 'Cannot backup existing geosite.dat.';
    Exit;
  end;
  InstalledGeoIp := False;
  try
    if not RenameFile(ADownloadedGeoIp, AFinalGeoIp) then
    begin
      AError := 'Cannot install verified geoip.dat.';
      Exit;
    end;
    InstalledGeoIp := True;
    if not RenameFile(ADownloadedGeoSite, AFinalGeoSite) then
    begin
      AError := 'Cannot install verified geosite.dat.';
      Exit;
    end;
    Result := True;
  finally
    if not Result then
    begin
      if InstalledGeoIp and FileExists(AFinalGeoIp) then DeleteFile(AFinalGeoIp);
      if FileExists(AFinalGeoSite) and not HadGeoSite then DeleteFile(AFinalGeoSite);
      if HadGeoIp and FileExists(GeoIpBackup) then
        RenameFile(GeoIpBackup, AFinalGeoIp);
      if HadGeoSite and FileExists(GeoSiteBackup) then
        RenameFile(GeoSiteBackup, AFinalGeoSite);
    end
    else
    begin
      if FileExists(GeoIpBackup) then DeleteFile(GeoIpBackup);
      if FileExists(GeoSiteBackup) then DeleteFile(GeoSiteBackup);
    end;
  end;
end;

constructor TZaryaGeoDataManager.Create(const ATargetDirectory: string);
begin
  inherited Create;
  FTargetDirectory := ExcludeTrailingPathDelimiter(ExpandFileName(
    ATargetDirectory));
end;

function TZaryaGeoDataManager.TargetDirectory: string;
begin
  Result := FTargetDirectory;
end;

function TZaryaGeoDataManager.MetadataFile: string;
begin
  Result := IncludeTrailingPathDelimiter(FTargetDirectory) + 'geodata.ini';
end;

function TZaryaGeoDataManager.CheckAll(out AStatuses: TZaryaGeoDataStatuses;
  out AError: string): Boolean;
var
  Kind: TZaryaGeoDataKind;
  Status: TZaryaGeoDataStatus;
  Ini: TIniFile;
  Index: Integer;
begin
  AError := '';
  SetLength(AStatuses, 2);
  Ini := TIniFile.Create(MetadataFile);
  try
    for Kind := Low(TZaryaGeoDataKind) to High(TZaryaGeoDataKind) do
    begin
      Index := Ord(Kind);
      Status := Default(TZaryaGeoDataStatus);
      Status.Kind := Kind;
      Status.FileName := GeoDataFileName(Kind);
      Status.FilePath := IncludeTrailingPathDelimiter(FTargetDirectory) +
        Status.FileName;
      Status.Exists := FileExists(Status.FilePath);
      Status.ExpectedSha256 := Ini.ReadString('verified', Status.FileName, '');
      if Status.Exists then
      begin
        with TFileStream.Create(Status.FilePath, fmOpenRead or fmShareDenyNone) do
        try
          Status.SizeBytes := Size;
        finally
          Free;
        end;
        if not Sha256File(Status.FilePath, Status.Sha256,
          Status.ErrorMessage) then
        begin
          AStatuses[Index] := Status;
          AError := Status.ErrorMessage;
          Exit(False);
        end;
        Status.Verified := (Status.ExpectedSha256 <> '') and
          SameText(Status.Sha256, Status.ExpectedSha256);
      end;
      AStatuses[Index] := Status;
    end;
    Result := True;
  finally
    Ini.Free;
  end;
end;

function TZaryaGeoDataManager.DownloadOne(const ASource: TZaryaGeoDataSource;
  const AKind: TZaryaGeoDataKind;
  const AProgress: TZaryaSubscriptionDownloadProgress;
  const ACancelCheck: TZaryaSubscriptionCancelCheck;
  out ATempFile, AExpectedSha256, AError: string): Boolean;
var
  FileName, FinalFile: string;
  Checksum: TZaryaSubscriptionDownloadResult;
  Cancelled: Boolean;
  StatusCode: Integer;
begin
  Result := False;
  ATempFile := '';
  AExpectedSha256 := '';
  FileName := GeoDataFileName(AKind);
  FinalFile := IncludeTrailingPathDelimiter(FTargetDirectory) + FileName;
  ATempFile := FinalFile + '.download';
  if FileExists(ATempFile) and not DeleteFile(ATempFile) then
  begin
    AError := 'Cannot remove an unfinished geodata download.';
    Exit;
  end;
  Checksum := DownloadSubscriptionWinHttp(ASource.BaseUrl + FileName +
    '.sha256sum', 'Zarya/LCL', 30000, nil, ACancelCheck);
  if not Checksum.Success then
  begin
    AError := Checksum.ErrorMessage;
    Exit;
  end;
  AExpectedSha256 := ParseSha256Sum(string(Checksum.Body), FileName);
  if AExpectedSha256 = '' then
  begin
    AError := 'Cannot parse SHA-256 for ' + FileName + '.';
    Exit;
  end;
  if not DownloadFileWinHttp(ASource.BaseUrl + FileName, 'Zarya/LCL',
    ATempFile, 300000, MaxGeoDataBytes, AProgress, ACancelCheck,
    Cancelled, StatusCode, AError) then Exit;
  if not VerifyDownloadedFile(ATempFile, AExpectedSha256, AError) then
  begin
    if FileExists(ATempFile) then DeleteFile(ATempFile);
    Exit;
  end;
  Result := True;
end;

function TZaryaGeoDataManager.UpdateAll(const ASourceId: string;
  const AProgress: TZaryaSubscriptionDownloadProgress;
  const ACancelCheck: TZaryaSubscriptionCancelCheck;
  out AError: string): Boolean;
var
  Source: TZaryaGeoDataSource;
  GeoIpTemp, GeoSiteTemp, GeoIpHash, GeoSiteHash: string;
  GeoIpFinal, GeoSiteFinal: string;
  Ini: TIniFile;
begin
  AError := '';
  if not ForceDirectories(FTargetDirectory) then
  begin
    AError := 'Cannot create geodata directory: ' + FTargetDirectory;
    Exit(False);
  end;
  Source := GeoDataSourceById(ASourceId);
  GeoIpTemp := '';
  GeoSiteTemp := '';
  try
    if not DownloadOne(Source, gdkGeoIp, AProgress, ACancelCheck,
      GeoIpTemp, GeoIpHash, AError) then Exit(False);
    if not DownloadOne(Source, gdkGeoSite, AProgress, ACancelCheck,
      GeoSiteTemp, GeoSiteHash, AError) then Exit(False);
    GeoIpFinal := IncludeTrailingPathDelimiter(FTargetDirectory) + 'geoip.dat';
    GeoSiteFinal := IncludeTrailingPathDelimiter(FTargetDirectory) +
      'geosite.dat';
    if not InstallVerifiedGeoDataPair(GeoIpTemp, GeoSiteTemp, GeoIpFinal,
      GeoSiteFinal, GeoIpHash, GeoSiteHash, AError) then Exit(False);
    GeoIpTemp := '';
    GeoSiteTemp := '';
    Ini := TIniFile.Create(MetadataFile);
    try
      Ini.WriteString('verified', 'geoip.dat', GeoIpHash);
      Ini.WriteString('verified', 'geosite.dat', GeoSiteHash);
      Ini.UpdateFile;
    finally
      Ini.Free;
    end;
    Result := True;
  finally
    if (GeoIpTemp <> '') and FileExists(GeoIpTemp) then DeleteFile(GeoIpTemp);
    if (GeoSiteTemp <> '') and FileExists(GeoSiteTemp) then
      DeleteFile(GeoSiteTemp);
  end;
end;

function TZaryaGeoDataManager.RequiredFilesPresent(
  const ARouting: TZaryaRoutingProfile; const ADns: TZaryaDnsProfile;
  out AMissing, AError: string): Boolean;
var
  NeedGeo: Boolean;
  GeoIp, GeoSite: Boolean;
  Rule: TZaryaRoutingRule;
  Server: TZaryaDnsServer;
  Value, Lower: string;
begin
  AError := '';
  AMissing := '';
  GeoIp := False;
  GeoSite := False;
  NeedGeo := RoutingUsesGeoData(ARouting) or DnsUsesGeoData(ADns);
  if not NeedGeo then Exit(True);
  for Rule in ARouting.Rules do if Rule.Enabled then for Value in Rule.Values do
  begin
    Lower := LowerCase(Trim(Value));
    if (Pos('geoip:', Lower) = 1) or (Pos('ext:geoip.dat:', Lower) = 1) then GeoIp := True;
    if (Pos('geosite:', Lower) = 1) or (Pos('ext:geosite.dat:', Lower) = 1) then GeoSite := True;
  end;
  for Server in ADns.Servers do if Server.Enabled then
  begin
    for Value in Server.Domains do
    begin
      Lower := LowerCase(Trim(Value));
      if Pos('geosite:', Lower) = 1 then GeoSite := True;
      if Pos('geoip:', Lower) = 1 then GeoIp := True;
    end;
    for Value in Server.ExpectIps do
    begin
      Lower := LowerCase(Trim(Value));
      if Pos('geoip:', Lower) = 1 then GeoIp := True;
      if Pos('geosite:', Lower) = 1 then GeoSite := True;
    end;
  end;
  if GeoIp and not FileExists(IncludeTrailingPathDelimiter(FTargetDirectory) +
    'geoip.dat') then AMissing := 'geoip.dat';
  if GeoSite and not FileExists(IncludeTrailingPathDelimiter(FTargetDirectory) +
    'geosite.dat') then
  begin
    if AMissing <> '' then AMissing := AMissing + ', ';
    AMissing := AMissing + 'geosite.dat';
  end;
  Result := AMissing = '';
end;

end.
