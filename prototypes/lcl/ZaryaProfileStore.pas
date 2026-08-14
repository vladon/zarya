unit ZaryaProfileStore;

{$IFDEF FPC}
{$mode delphi}{$H+}
{$ENDIF}

interface

uses
  ZaryaProfile;

type
  IZaryaProfileStore = interface
    ['{B1F2ACFD-776A-4D1C-87BD-8409F2F36F8D}']
    function GetFileName: string;
    function Load(out AProfiles: TZaryaProfiles; out AError: string): Boolean;
    function Save(const AProfiles: TZaryaProfiles; out AError: string): Boolean;
    property FileName: string read GetFileName;
  end;

implementation

end.
