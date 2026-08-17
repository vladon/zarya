program VersionContractTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, Process, ZaryaVersion;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then raise Exception.Create(AMessage);
end;

var
  ZaryaPath, CMakePath, LpiPath, OutputText: string;
  Child: TProcess;
  Lines: TStringList;
begin
  ZaryaPath := ExpandFileName(IncludeTrailingPathDelimiter(
    ExtractFileDir(ParamStr(0))) + '..' + PathDelim + '..' + PathDelim +
    'bin' + PathDelim + 'Zarya.exe');
  Check(FileExists(ZaryaPath), 'Production Zarya.exe is missing.');
  Child := TProcess.Create(nil);
  Lines := TStringList.Create;
  try
    Child.Executable := ZaryaPath;
    Child.Parameters.Add('--version');
    Child.Options := [poUsePipes, poWaitOnExit, poNoConsole];
    Child.Execute;
    Lines.LoadFromStream(Child.Output);
    OutputText := Trim(Lines.Text);
    Check(Child.ExitStatus = 0, 'Zarya --version returned an error.');
    Check(OutputText = 'Zarya ' + ZaryaVersionString,
      'Zarya --version drifted: ' + OutputText);
  finally
    Lines.Free;
    Child.Free;
  end;

  CMakePath := ExpandFileName(IncludeTrailingPathDelimiter(
    ExtractFileDir(ParamStr(0))) + '..' + PathDelim + '..' + PathDelim +
    '..' + PathDelim + '..' + PathDelim + 'cmake' + PathDelim +
    'ZaryaVersion.cmake');
  Check(FileExists(CMakePath), 'CMake version source is missing.');
  Lines := TStringList.Create;
  try
    Lines.LoadFromFile(CMakePath);
    Check(Pos('set(ZARYA_VERSION_STRING "' + ZaryaVersionString + '")',
      Lines.Text) > 0, 'Pascal and CMake versions drifted.');
  finally
    Lines.Free;
  end;

  LpiPath := ExpandFileName(IncludeTrailingPathDelimiter(
    ExtractFileDir(ParamStr(0))) + '..' + PathDelim + '..' + PathDelim +
    'zarya_lcl.lpi');
  Check(FileExists(LpiPath), 'LPI project file is missing.');
  Lines := TStringList.Create;
  try
    Lines.LoadFromFile(LpiPath);
    Check(Pos('<MajorVersionNr Value="' + IntToStr(ZaryaVersionMajor) + '"/>',
      Lines.Text) > 0, 'LPI MajorVersionNr drifted.');
    Check(Pos('<MinorVersionNr Value="' + IntToStr(ZaryaVersionMinor) + '"/>',
      Lines.Text) > 0, 'LPI MinorVersionNr drifted.');
    Check(Pos('<RevisionNr Value="' + IntToStr(ZaryaVersionPatch) + '"/>',
      Lines.Text) > 0, 'LPI RevisionNr drifted.');
    Check(Pos('ProductVersion="' + ZaryaVersionString + '"',
      Lines.Text) > 0, 'LPI ProductVersion drifted.');
  finally
    Lines.Free;
  end;
  WriteLn('CLI, Pascal and CMake version contract: PASS');
end.
