unit ZaryaProfileService;

{$mode objfpc}{$H+}

interface

uses
  ZaryaProfile, ZaryaProfileStore;

type
  TZaryaProfileService = class
  private
    FStore: IZaryaProfileStore;
  public
    constructor Create(const AStore: IZaryaProfileStore);
    function Load(out AProfiles: TZaryaProfiles; out AError: string): Boolean;
    function Save(const AProfiles: TZaryaProfiles; out AError: string): Boolean;
    function FindById(const AProfiles: TZaryaProfiles;
      const AProfileId: string): Integer;
    function FindRunnableById(const AProfiles: TZaryaProfiles;
      const AProfileId: string): Integer;
    function FileName: string;
    function Store: IZaryaProfileStore;
  end;

implementation

uses
  SysUtils;

constructor TZaryaProfileService.Create(const AStore: IZaryaProfileStore);
begin
  inherited Create;
  FStore := AStore;
end;

function TZaryaProfileService.Load(out AProfiles: TZaryaProfiles;
  out AError: string): Boolean;
begin
  if not Assigned(FStore) then
  begin
    AProfiles := nil;
    AError := 'Profile store is unavailable.';
    Exit(False);
  end;
  Result := FStore.Load(AProfiles, AError);
end;

function TZaryaProfileService.Save(const AProfiles: TZaryaProfiles;
  out AError: string): Boolean;
begin
  if not Assigned(FStore) then
  begin
    AError := 'Profile store is unavailable.';
    Exit(False);
  end;
  Result := FStore.Save(AProfiles, AError);
end;

function TZaryaProfileService.FindById(const AProfiles: TZaryaProfiles;
  const AProfileId: string): Integer;
var
  I: Integer;
begin
  for I := 0 to High(AProfiles) do
    if SameText(AProfiles[I].Id, AProfileId) then
      Exit(I);
  Result := -1;
end;

function TZaryaProfileService.FindRunnableById(
  const AProfiles: TZaryaProfiles; const AProfileId: string): Integer;
begin
  Result := FindById(AProfiles, AProfileId);
  if (Result >= 0) and (not AProfiles[Result].Enabled or
    AProfiles[Result].DeletedBySubscriptionUpdate) then
    Result := -1;
end;

function TZaryaProfileService.FileName: string;
begin
  if Assigned(FStore) then
    Result := FStore.FileName
  else
    Result := '';
end;

function TZaryaProfileService.Store: IZaryaProfileStore;
begin
  Result := FStore;
end;

end.
