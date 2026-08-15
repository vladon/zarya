unit ZaryaPolicyStore;

{$mode objfpc}{$H+}

interface

uses
  ZaryaRouting, ZaryaDns;

type
  IRoutingProfileStore = interface
    ['{4ED385F5-A4BA-4B83-97DC-5B61CDA43C4C}']
    function Load(out AProfiles: TZaryaRoutingProfiles;
      out AError: string): Boolean;
    function Save(const AProfiles: TZaryaRoutingProfiles;
      out AError: string): Boolean;
    function FileName: string;
  end;

  IDnsProfileStore = interface
    ['{39AD1CC8-3EA6-4C05-8A31-C8C3152442BE}']
    function Load(out AProfiles: TZaryaDnsProfiles;
      out AError: string): Boolean;
    function Save(const AProfiles: TZaryaDnsProfiles;
      out AError: string): Boolean;
    function FileName: string;
  end;

implementation

end.
