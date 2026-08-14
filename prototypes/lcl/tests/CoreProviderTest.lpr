program CoreProviderTest;

{$mode objfpc}{$H+}

uses
  Classes, SysUtils, ZaryaCoreProvider, ZaryaCoreProviderStore,
  FpcCoreProviderStore, ZaryaCoreProviderRegistry, ZaryaFileIntegrity,
  ZaryaRuntimeProcess;

procedure Check(const ACondition: Boolean; const AMessage: string);
begin
  if not ACondition then
    raise Exception.Create(AMessage);
end;

var
  TempDirectory: string;
  HashFile: string;
  ProviderFile: string;
  Stream: TFileStream;
  Bytes: UTF8String;
  Digest: string;
  ErrorMessage: string;
  Store: IZaryaCoreProviderStore;
  Providers: TZaryaCoreProviders;
  Loaded: TZaryaCoreProviders;
  Provider: TZaryaCoreProvider;
  Context: TZaryaProcessContext;
  Arguments: TZaryaStringArray;
begin
  Randomize;
  TempDirectory := IncludeTrailingPathDelimiter(GetTempDir(False)) +
    'zarya-provider-test-' + IntToHex(Random(MaxInt), 8);
  Check(ForceDirectories(TempDirectory), 'Could not create temp directory.');
  HashFile := IncludeTrailingPathDelimiter(TempDirectory) + 'abc.txt';
  Bytes := 'abc';
  Stream := TFileStream.Create(HashFile, fmCreate);
  try
    Stream.WriteBuffer(Bytes[1], Length(Bytes));
  finally
    Stream.Free;
  end;
  Check(Sha256File(HashFile, Digest, ErrorMessage),
    'SHA-256 failed: ' + ErrorMessage);
  Check(Digest =
    'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad',
    'SHA-256 vector mismatch: ' + Digest);

  Provider := CreateProviderPreset(ProviderExternalMihomo);
  Check(Provider.ConfigFormat = cfMihomoYaml, 'Wrong Mihomo config format.');
  Check(ProviderSupportsProtocol(Provider, 'VLESS'),
    'Mihomo should support VLESS.');
  Check(not ProviderSupportsProtocol(
    CreateProviderPreset(ProviderExternalHysteria2), 'VMess'),
    'Hysteria provider unexpectedly supports VMess.');

  Context.ConfigPath := 'C:\runtime data\profile.yaml';
  Context.DataDirectory := 'C:\runtime data';
  Context.MixedPort := 10808;
  Context.HttpPort := 10809;
  Context.SocksPort := 10810;
  Context.LogLevel := 'warning';
  Check(ExpandProviderArguments(Provider.RunArguments, Context, Arguments,
    ErrorMessage), 'Argument expansion failed: ' + ErrorMessage);
  Check(Length(Arguments) = 4, 'Mihomo argument count mismatch.');
  Check(Arguments[1] = 'C:\runtime data\profile.yaml',
    'Config path was not kept as one argument.');

  ProviderFile := IncludeTrailingPathDelimiter(TempDirectory) +
    'core-providers.json';
  Store := TFpcCoreProviderStore.Create(ProviderFile);
  SetLength(Providers, 1);
  Provider.ExecutablePath := 'C:\external cores\mihomo.exe';
  Provider.WorkingDirectory := 'C:\external cores';
  Provider.Architecture := 'x86_64';
  Provider.Sha256 := Digest;
  Provider.ConfirmedSha256 := Digest;
  Providers[0] := Provider;
  Check(Store.Save(Providers, ErrorMessage),
    'Provider save failed: ' + ErrorMessage);
  Check(Store.Load(Loaded, ErrorMessage),
    'Provider load failed: ' + ErrorMessage);
  Check(Length(Loaded) = 1, 'Provider count mismatch.');
  Check(Loaded[0].ProviderId = ProviderExternalMihomo,
    'Provider id mismatch.');
  Check(Loaded[0].RunArguments[1] = '{config}',
    'Provider command template mismatch.');

  Store := nil;
  DeleteFile(ProviderFile);
  DeleteFile(HashFile);
  RemoveDir(TempDirectory);
  WriteLn('Core provider contracts: PASS');
end.
