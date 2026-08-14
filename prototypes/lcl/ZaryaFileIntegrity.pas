unit ZaryaFileIntegrity;

{$mode objfpc}{$H+}
{$R-}
{$Q-}

interface

uses
  Classes, SysUtils;

function Sha256File(const AFileName: string; out ADigest: string;
  out AError: string): Boolean;
function PeArchitecture(const AFileName: string; out AArchitecture: string;
  out AError: string): Boolean;

implementation

type
  TSha256State = record
    H: array[0..7] of Cardinal;
    Buffer: array[0..63] of Byte;
    BufferSize: Integer;
    TotalBytes: QWord;
  end;

const
  K: array[0..63] of Cardinal = (
    $428A2F98,$71374491,$B5C0FBCF,$E9B5DBA5,$3956C25B,$59F111F1,$923F82A4,$AB1C5ED5,
    $D807AA98,$12835B01,$243185BE,$550C7DC3,$72BE5D74,$80DEB1FE,$9BDC06A7,$C19BF174,
    $E49B69C1,$EFBE4786,$0FC19DC6,$240CA1CC,$2DE92C6F,$4A7484AA,$5CB0A9DC,$76F988DA,
    $983E5152,$A831C66D,$B00327C8,$BF597FC7,$C6E00BF3,$D5A79147,$06CA6351,$14292967,
    $27B70A85,$2E1B2138,$4D2C6DFC,$53380D13,$650A7354,$766A0ABB,$81C2C92E,$92722C85,
    $A2BFE8A1,$A81A664B,$C24B8B70,$C76C51A3,$D192E819,$D6990624,$F40E3585,$106AA070,
    $19A4C116,$1E376C08,$2748774C,$34B0BCB5,$391C0CB3,$4ED8AA4A,$5B9CCA4F,$682E6FF3,
    $748F82EE,$78A5636F,$84C87814,$8CC70208,$90BEFFFA,$A4506CEB,$BEF9A3F7,$C67178F2);

function RotateRight(const AValue: Cardinal; const ACount: Byte): Cardinal; inline;
begin
  Result := (AValue shr ACount) or (AValue shl (32 - ACount));
end;

procedure Sha256Transform(var AState: TSha256State);
var
  W: array[0..63] of Cardinal;
  A, B, C, D, E, F, G, Hh, T1, T2, S0, S1, Ch, Maj: Cardinal;
  I: Integer;
begin
  for I := 0 to 15 do
    W[I] := (Cardinal(AState.Buffer[I * 4]) shl 24) or
      (Cardinal(AState.Buffer[I * 4 + 1]) shl 16) or
      (Cardinal(AState.Buffer[I * 4 + 2]) shl 8) or
      Cardinal(AState.Buffer[I * 4 + 3]);
  for I := 16 to 63 do
  begin
    S0 := RotateRight(W[I - 15], 7) xor RotateRight(W[I - 15], 18) xor
      (W[I - 15] shr 3);
    S1 := RotateRight(W[I - 2], 17) xor RotateRight(W[I - 2], 19) xor
      (W[I - 2] shr 10);
    W[I] := W[I - 16] + S0 + W[I - 7] + S1;
  end;

  A := AState.H[0]; B := AState.H[1]; C := AState.H[2]; D := AState.H[3];
  E := AState.H[4]; F := AState.H[5]; G := AState.H[6]; Hh := AState.H[7];
  for I := 0 to 63 do
  begin
    S1 := RotateRight(E, 6) xor RotateRight(E, 11) xor RotateRight(E, 25);
    Ch := (E and F) xor ((not E) and G);
    T1 := Hh + S1 + Ch + K[I] + W[I];
    S0 := RotateRight(A, 2) xor RotateRight(A, 13) xor RotateRight(A, 22);
    Maj := (A and B) xor (A and C) xor (B and C);
    T2 := S0 + Maj;
    Hh := G; G := F; F := E; E := D + T1;
    D := C; C := B; B := A; A := T1 + T2;
  end;
  Inc(AState.H[0], A); Inc(AState.H[1], B); Inc(AState.H[2], C);
  Inc(AState.H[3], D); Inc(AState.H[4], E); Inc(AState.H[5], F);
  Inc(AState.H[6], G); Inc(AState.H[7], Hh);
  AState.BufferSize := 0;
end;

procedure Sha256Init(var AState: TSha256State);
begin
  FillChar(AState, SizeOf(AState), 0);
  AState.H[0] := $6A09E667; AState.H[1] := $BB67AE85;
  AState.H[2] := $3C6EF372; AState.H[3] := $A54FF53A;
  AState.H[4] := $510E527F; AState.H[5] := $9B05688C;
  AState.H[6] := $1F83D9AB; AState.H[7] := $5BE0CD19;
end;

procedure Sha256Update(var AState: TSha256State; const AData; ACount: Integer);
var
  P: PByte;
  ToCopy: Integer;
begin
  P := @AData;
  Inc(AState.TotalBytes, QWord(ACount));
  while ACount > 0 do
  begin
    ToCopy := 64 - AState.BufferSize;
    if ToCopy > ACount then
      ToCopy := ACount;
    Move(P^, AState.Buffer[AState.BufferSize], ToCopy);
    Inc(P, ToCopy);
    Inc(AState.BufferSize, ToCopy);
    Dec(ACount, ToCopy);
    if AState.BufferSize = 64 then
      Sha256Transform(AState);
  end;
end;

function Sha256Final(var AState: TSha256State): string;
var
  BitLength: QWord;
  I: Integer;
begin
  BitLength := AState.TotalBytes * 8;
  AState.Buffer[AState.BufferSize] := $80;
  Inc(AState.BufferSize);
  if AState.BufferSize > 56 then
  begin
    while AState.BufferSize < 64 do
    begin
      AState.Buffer[AState.BufferSize] := 0;
      Inc(AState.BufferSize);
    end;
    Sha256Transform(AState);
  end;
  while AState.BufferSize < 56 do
  begin
    AState.Buffer[AState.BufferSize] := 0;
    Inc(AState.BufferSize);
  end;
  for I := 0 to 7 do
    AState.Buffer[63 - I] := Byte(BitLength shr (I * 8));
  AState.BufferSize := 64;
  Sha256Transform(AState);
  Result := '';
  for I := 0 to 7 do
    Result := Result + LowerCase(IntToHex(AState.H[I], 8));
end;

function Sha256File(const AFileName: string; out ADigest: string;
  out AError: string): Boolean;
var
  Stream: TFileStream;
  Buffer: array[0..65535] of Byte;
  Count: Integer;
  State: TSha256State;
begin
  ADigest := '';
  AError := '';
  Stream := nil;
  try
    Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyWrite);
    Sha256Init(State);
    repeat
      Count := Stream.Read(Buffer, SizeOf(Buffer));
      if Count > 0 then
        Sha256Update(State, Buffer, Count);
    until Count = 0;
    ADigest := Sha256Final(State);
    Result := True;
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Stream.Free;
end;

function PeArchitecture(const AFileName: string; out AArchitecture: string;
  out AError: string): Boolean;
var
  Stream: TFileStream;
  DosMagic: Word;
  PeOffset: LongWord;
  Signature: LongWord;
  Machine: Word;
begin
  AArchitecture := '';
  AError := '';
  Stream := nil;
  try
    Stream := TFileStream.Create(AFileName, fmOpenRead or fmShareDenyWrite);
    if Stream.Size < 64 then
      raise Exception.Create('Файл слишком мал для Windows PE.');
    Stream.ReadBuffer(DosMagic, SizeOf(DosMagic));
    if DosMagic <> $5A4D then
      raise Exception.Create('Файл не является Windows PE executable.');
    Stream.Position := $3C;
    Stream.ReadBuffer(PeOffset, SizeOf(PeOffset));
    if (PeOffset > LongWord(Stream.Size - 6)) then
      raise Exception.Create('Повреждён заголовок Windows PE.');
    Stream.Position := PeOffset;
    Stream.ReadBuffer(Signature, SizeOf(Signature));
    if Signature <> $00004550 then
      raise Exception.Create('Не найдена сигнатура Windows PE.');
    Stream.ReadBuffer(Machine, SizeOf(Machine));
    case Machine of
      $014C: AArchitecture := 'x86';
      $8664: AArchitecture := 'x86_64';
      $AA64: AArchitecture := 'arm64';
    else
      AArchitecture := 'unknown-' + IntToHex(Machine, 4);
    end;
    Result := AArchitecture = 'x86_64';
    if not Result then
      AError := 'Требуется x86_64 EXE; обнаружено: ' + AArchitecture + '.';
  except
    on E: Exception do
    begin
      AError := E.Message;
      Result := False;
    end;
  end;
  Stream.Free;
end;

end.
