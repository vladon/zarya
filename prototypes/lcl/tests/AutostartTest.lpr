program AutostartTest;

{$mode objfpc}{$H+}

uses
  SysUtils, WindowsAutostart;

procedure Require(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then raise Exception.Create(AMessage);
end;

var
  CommandLine: string;
begin
  Require(QuoteWindowsCommandLineArgument('plain') = 'plain',
    'Plain argument was unnecessarily changed.');
  Require(QuoteWindowsCommandLineArgument('two words') = '"two words"',
    'Space quoting failed.');
  Require(QuoteWindowsCommandLineArgument('C:\path with space\') =
    '"C:\path with space\\"', 'Trailing slash quoting failed.');
  CommandLine := BuildWindowsCommandLine('C:\Program Files\Zarya\Zarya.exe',
    ['--minimized', '--data-dir', 'C:\data path']);
  Require(Pos('"C:\Program Files\Zarya\Zarya.exe"', CommandLine) = 1,
    'Executable quoting failed.');
  Require(Pos('"C:\data path"', CommandLine) > 0,
    'Argument array quoting failed.');
  WriteLn('Windows autostart command-line quoting: PASS');
end.
