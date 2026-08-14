unit WindowsSubscriptionDownloader;

{$mode objfpc}{$H+}

interface

uses
  SysUtils;

type
  TZaryaSubscriptionDownloadProgress = procedure(const ADownloaded,
    ATotal: Int64) of object;
  TZaryaSubscriptionCancelCheck = function: Boolean of object;

  TZaryaSubscriptionDownloadResult = record
    Success: Boolean;
    Cancelled: Boolean;
    Body: RawByteString;
    HttpStatusCode: Integer;
    FinalUrl: string;
    ErrorMessage: string;
  end;

function DownloadSubscriptionWinHttp(const AUrl, AUserAgent: string;
  const ATimeoutMs: Integer; const AProgress: TZaryaSubscriptionDownloadProgress;
  const ACancelCheck: TZaryaSubscriptionCancelCheck):
  TZaryaSubscriptionDownloadResult;

implementation

uses
  Classes
  {$IFDEF WINDOWS}, Windows, WinHttp{$ENDIF};

{$IFDEF WINDOWS}
function ZaryaWinHttpSetTimeouts(hInternet: HINTERNET;
  nResolveTimeout, nConnectTimeout, nSendTimeout,
  nReceiveTimeout: Integer): WINBOOL; stdcall;
  external 'winhttp.dll' name 'WinHttpSetTimeouts';
{$ENDIF}

const
  MaxSubscriptionBytes = 16 * 1024 * 1024;

function CancelRequested(const ACancelCheck: TZaryaSubscriptionCancelCheck): Boolean;
begin
  Result := Assigned(ACancelCheck) and ACancelCheck();
end;

procedure ReportProgress(const AProgress: TZaryaSubscriptionDownloadProgress;
  const ADownloaded, ATotal: Int64);
begin
  if Assigned(AProgress) then
    AProgress(ADownloaded, ATotal);
end;

{$IFDEF WINDOWS}
function WinHttpError(const AOperation: string): string;
begin
  Result := AOperation + ': ' + SysErrorMessage(GetLastError);
end;

function DownloadSubscriptionWinHttp(const AUrl, AUserAgent: string;
  const ATimeoutMs: Integer; const AProgress: TZaryaSubscriptionDownloadProgress;
  const ACancelCheck: TZaryaSubscriptionCancelCheck):
  TZaryaSubscriptionDownloadResult;
var
  UrlText: UnicodeString;
  UserAgentText: UnicodeString;
  Verb: UnicodeString;
  Host: UnicodeString;
  ObjectName: UnicodeString;
  Components: URL_COMPONENTS;
  Session: HINTERNET;
  Connection: HINTERNET;
  Request: HINTERNET;
  Flags: DWORD;
  RedirectPolicy: DWORD;
  StatusCode: DWORD;
  ContentLength: DWORD;
  HeaderSize: DWORD;
  Available: DWORD;
  ReadCount: DWORD;
  Buffer: array of Byte;
  Stream: TMemoryStream;
  Timeout: Integer;
begin
  Result := Default(TZaryaSubscriptionDownloadResult);
  Result.FinalUrl := Trim(AUrl);
  UrlText := UnicodeString(Trim(AUrl));
  UserAgentText := UnicodeString(Trim(AUserAgent));
  if UserAgentText = '' then
    UserAgentText := 'Zarya-LCL/1.0';
  if UrlText = '' then
  begin
    Result.ErrorMessage := 'URL подписки пуст.';
    Exit;
  end;
  FillChar(Components, SizeOf(Components), 0);
  Components.dwStructSize := SizeOf(Components);
  Components.dwSchemeLength := DWORD(-1);
  Components.dwHostNameLength := DWORD(-1);
  Components.dwUrlPathLength := DWORD(-1);
  Components.dwExtraInfoLength := DWORD(-1);
  if not WinHttpCrackUrl(PWideChar(UrlText), Length(UrlText), 0,
    @Components) then
  begin
    Result.ErrorMessage := WinHttpError('Некорректный URL подписки');
    Exit;
  end;
  if (Components.nScheme <> INTERNET_SCHEME_HTTP) and
    (Components.nScheme <> INTERNET_SCHEME_HTTPS) then
  begin
    Result.ErrorMessage := 'Поддерживаются только HTTPS и HTTP subscription URL.';
    Exit;
  end;
  SetString(Host, Components.lpszHostName, Components.dwHostNameLength);
  SetString(ObjectName, Components.lpszUrlPath, Components.dwUrlPathLength);
  if Components.dwExtraInfoLength > 0 then
    SetString(Verb, Components.lpszExtraInfo, Components.dwExtraInfoLength)
  else
    Verb := '';
  ObjectName := ObjectName + Verb;
  if ObjectName = '' then
    ObjectName := '/';

  Session := nil;
  Connection := nil;
  Request := nil;
  Stream := TMemoryStream.Create;
  try
    if CancelRequested(ACancelCheck) then
    begin
      Result.Cancelled := True;
      Result.ErrorMessage := 'Обновление подписки отменено.';
      Exit;
    end;
    Session := WinHttpOpen(PWideChar(UserAgentText),
      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS, 0);
    if Session = nil then
    begin
      Result.ErrorMessage := WinHttpError('WinHttpOpen');
      Exit;
    end;
    Timeout := ATimeoutMs;
    if Timeout < 1000 then Timeout := 1000;
    if not ZaryaWinHttpSetTimeouts(Session, Timeout, Timeout, Timeout,
      Timeout) then
    begin
      Result.ErrorMessage := WinHttpError('WinHttpSetTimeouts');
      Exit;
    end;
    Connection := WinHttpConnect(Session, PWideChar(Host), Components.nPort, 0);
    if Connection = nil then
    begin
      Result.ErrorMessage := WinHttpError('WinHttpConnect');
      Exit;
    end;
    Flags := 0;
    if Components.nScheme = INTERNET_SCHEME_HTTPS then
      Flags := WINHTTP_FLAG_SECURE;
    Verb := 'GET';
    Request := WinHttpOpenRequest(Connection, PWideChar(Verb),
      PWideChar(ObjectName), nil, WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES, Flags);
    if Request = nil then
    begin
      Result.ErrorMessage := WinHttpError('WinHttpOpenRequest');
      Exit;
    end;
    RedirectPolicy := WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    if not WinHttpSetOption(Request, WINHTTP_OPTION_REDIRECT_POLICY,
      @RedirectPolicy, SizeOf(RedirectPolicy)) then
    begin
      Result.ErrorMessage := WinHttpError('WinHttpSetOption redirect policy');
      Exit;
    end;
    if CancelRequested(ACancelCheck) then
    begin
      Result.Cancelled := True;
      Result.ErrorMessage := 'Обновление подписки отменено.';
      Exit;
    end;
    if not WinHttpSendRequest(Request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
      nil, 0, 0, 0) then
    begin
      Result.ErrorMessage := WinHttpError('WinHttpSendRequest');
      Exit;
    end;
    if not WinHttpReceiveResponse(Request, nil) then
    begin
      Result.ErrorMessage := WinHttpError('WinHttpReceiveResponse');
      Exit;
    end;
    StatusCode := 0;
    HeaderSize := SizeOf(StatusCode);
    if not WinHttpQueryHeaders(Request, WINHTTP_QUERY_STATUS_CODE or
      WINHTTP_QUERY_FLAG_NUMBER, nil, @StatusCode, @HeaderSize, nil) then
    begin
      Result.ErrorMessage := WinHttpError('WinHttpQueryHeaders status');
      Exit;
    end;
    Result.HttpStatusCode := StatusCode;
    if (StatusCode < 200) or (StatusCode >= 300) then
    begin
      Result.ErrorMessage := 'HTTP error ' + IntToStr(StatusCode) + '.';
      Exit;
    end;
    ContentLength := 0;
    HeaderSize := SizeOf(ContentLength);
    if not WinHttpQueryHeaders(Request, WINHTTP_QUERY_CONTENT_LENGTH or
      WINHTTP_QUERY_FLAG_NUMBER, nil, @ContentLength, @HeaderSize, nil) then
      ContentLength := 0;
    if ContentLength > MaxSubscriptionBytes then
    begin
      Result.ErrorMessage := 'Subscription response exceeds the 16 MiB limit.';
      Exit;
    end;
    ReportProgress(AProgress, 0, ContentLength);
    repeat
      if CancelRequested(ACancelCheck) then
      begin
        Result.Cancelled := True;
        Result.ErrorMessage := 'Обновление подписки отменено.';
        Exit;
      end;
      Available := 0;
      if not WinHttpQueryDataAvailable(Request, @Available) then
      begin
        Result.ErrorMessage := WinHttpError('WinHttpQueryDataAvailable');
        Exit;
      end;
      if Available = 0 then
        Break;
      if (Stream.Size + Available) > MaxSubscriptionBytes then
      begin
        Result.ErrorMessage := 'Subscription response exceeds the 16 MiB limit.';
        Exit;
      end;
      SetLength(Buffer, Available);
      ReadCount := 0;
      if not WinHttpReadData(Request, @Buffer[0], Available, @ReadCount) then
      begin
        Result.ErrorMessage := WinHttpError('WinHttpReadData');
        Exit;
      end;
      if ReadCount > 0 then
        Stream.WriteBuffer(Buffer[0], ReadCount);
      ReportProgress(AProgress, Stream.Size, ContentLength);
    until False;
    if Stream.Size = 0 then
    begin
      Result.ErrorMessage := 'Ответ подписки пуст.';
      Exit;
    end;
    SetLength(Result.Body, Stream.Size);
    Stream.Position := 0;
    Stream.ReadBuffer(Result.Body[1], Stream.Size);
    Result.Success := True;
  finally
    if Request <> nil then WinHttpCloseHandle(Request);
    if Connection <> nil then WinHttpCloseHandle(Connection);
    if Session <> nil then WinHttpCloseHandle(Session);
    Stream.Free;
  end;
end;
{$ELSE}
function DownloadSubscriptionWinHttp(const AUrl, AUserAgent: string;
  const ATimeoutMs: Integer; const AProgress: TZaryaSubscriptionDownloadProgress;
  const ACancelCheck: TZaryaSubscriptionCancelCheck):
  TZaryaSubscriptionDownloadResult;
begin
  Result := Default(TZaryaSubscriptionDownloadResult);
  Result.ErrorMessage := 'WinHTTP доступен только в Windows-сборке.';
end;
{$ENDIF}

end.
