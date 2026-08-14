unit ZaryaCoreProviderStore;

{$mode objfpc}{$H+}

interface

uses
  ZaryaCoreProvider;

type
  IZaryaCoreProviderStore = interface
    ['{54E9839B-851A-430E-824D-E4C2FF18C532}']
    function GetFileName: string;
    function Load(out AProviders: TZaryaCoreProviders;
      out AError: string): Boolean;
    function Save(const AProviders: TZaryaCoreProviders;
      out AError: string): Boolean;
    property FileName: string read GetFileName;
  end;

implementation

end.
