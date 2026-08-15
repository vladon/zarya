program GeoDataTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, ZaryaGeoData, ZaryaFileIntegrity;

procedure Require(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then raise Exception.Create(AMessage);
end;

procedure WriteText(const AFileName, AText: string);
var
  Stream: TFileStream;
  Bytes: UTF8String;
begin
  Bytes := UTF8String(AText);
  Stream := TFileStream.Create(AFileName, fmCreate or fmShareExclusive);
  try
    if Length(Bytes) > 0 then Stream.WriteBuffer(Bytes[1], Length(Bytes));
  finally
    Stream.Free;
  end;
end;

var
  DirectoryName, DownloadFile, FinalFile, Hash, ErrorMessage: string;
  DownloadGeoIp, DownloadGeoSite, FinalGeoIp, FinalGeoSite: string;
  GeoIpHash, GeoSiteHash: string;
begin
  Randomize;
  DirectoryName := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-geodata-' + IntToHex(Random(MaxInt), 8);
  ForceDirectories(DirectoryName);
  DownloadFile := IncludeTrailingPathDelimiter(DirectoryName) + 'geoip.download';
  FinalFile := IncludeTrailingPathDelimiter(DirectoryName) + 'geoip.dat';
  try
    WriteText(DownloadFile, 'verified-content');
    Require(Sha256File(DownloadFile, Hash, ErrorMessage), ErrorMessage);
    Require(ParseSha256Sum(Hash + '  geoip.dat', 'geoip.dat') = Hash,
      'SHA sum parser rejected a valid checksum.');
    Require(InstallVerifiedGeoDataFile(DownloadFile, FinalFile, Hash,
      ErrorMessage), ErrorMessage);
    Require(FileExists(FinalFile), 'Verified geodata was not installed.');
    WriteText(DownloadFile, 'corrupt-content');
    Require(not InstallVerifiedGeoDataFile(DownloadFile, FinalFile,
      StringOfChar('0', 64), ErrorMessage),
      'Invalid geodata hash was accepted.');
    Require(FileExists(FinalFile),
      'Existing geodata was lost after verification failure.');
    DownloadGeoIp := IncludeTrailingPathDelimiter(DirectoryName) +
      'pair-geoip.download';
    DownloadGeoSite := IncludeTrailingPathDelimiter(DirectoryName) +
      'pair-geosite.download';
    FinalGeoIp := IncludeTrailingPathDelimiter(DirectoryName) +
      'pair-geoip.dat';
    FinalGeoSite := IncludeTrailingPathDelimiter(DirectoryName) +
      'pair-geosite.dat';
    WriteText(FinalGeoIp, 'old-ip');
    WriteText(FinalGeoSite, 'old-site');
    WriteText(DownloadGeoIp, 'new-ip');
    WriteText(DownloadGeoSite, 'new-site');
    Require(Sha256File(DownloadGeoIp, GeoIpHash, ErrorMessage), ErrorMessage);
    Require(Sha256File(DownloadGeoSite, GeoSiteHash, ErrorMessage), ErrorMessage);
    Require(InstallVerifiedGeoDataPair(DownloadGeoIp, DownloadGeoSite,
      FinalGeoIp, FinalGeoSite, GeoIpHash, GeoSiteHash, ErrorMessage),
      ErrorMessage);
    Require(Sha256File(FinalGeoIp, Hash, ErrorMessage) and
      SameText(Hash, GeoIpHash), 'Pair install did not replace geoip.dat.');
    Require(Sha256File(FinalGeoSite, Hash, ErrorMessage) and
      SameText(Hash, GeoSiteHash), 'Pair install did not replace geosite.dat.');
    WriteText(DownloadGeoIp, 'bad-new-ip');
    WriteText(DownloadGeoSite, 'bad-new-site');
    Require(not InstallVerifiedGeoDataPair(DownloadGeoIp, DownloadGeoSite,
      FinalGeoIp, FinalGeoSite, StringOfChar('0', 64),
      StringOfChar('0', 64), ErrorMessage),
      'Pair install accepted invalid checksums.');
    Require(Sha256File(FinalGeoIp, Hash, ErrorMessage) and
      SameText(Hash, GeoIpHash), 'Failed pair update lost previous geoip.dat.');
    Require(Sha256File(FinalGeoSite, Hash, ErrorMessage) and
      SameText(Hash, GeoSiteHash),
      'Failed pair update lost previous geosite.dat.');
    WriteLn('Geodata checksum and atomic install: PASS');
  finally
    if FileExists(DownloadFile) then DeleteFile(DownloadFile);
    if FileExists(FinalFile) then DeleteFile(FinalFile);
    if FileExists(FinalFile + '.bak') then DeleteFile(FinalFile + '.bak');
    if FileExists(DownloadGeoIp) then DeleteFile(DownloadGeoIp);
    if FileExists(DownloadGeoSite) then DeleteFile(DownloadGeoSite);
    if FileExists(FinalGeoIp) then DeleteFile(FinalGeoIp);
    if FileExists(FinalGeoSite) then DeleteFile(FinalGeoSite);
    if FileExists(FinalGeoIp + '.bak') then DeleteFile(FinalGeoIp + '.bak');
    if FileExists(FinalGeoSite + '.bak') then DeleteFile(FinalGeoSite + '.bak');
    RemoveDir(DirectoryName);
  end;
end.
