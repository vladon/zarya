# Zarya FPC/LCL

Windows-реализация Zarya на Free Pascal/Lazarus LCL. Qt-версия пока остаётся в
репозитории как эталон поведения до прохождения release gate и cutover.
Формы создаются программно и используют стандартные нативные Win32-контролы.

Что можно проверить:

- главное окно с operational status, таблицей профилей и журналом;
- Win32 tray: двойной клик, контекстное меню, скрытие окна при закрытии;
- светлую и тёмную палитру через `Инструменты → Настройки`;
- поведение стандартных контролов при Windows scaling 100%, 150% и 200%;
- клавиатурную навигацию и визуальную плотность LCL;
- реальный переход `остановлен → ожидание mixed-порта → подключён`;
- production-сборка одного `Zarya.exe` со статически встроенным Xray через Go
  `c-archive`;
- изолированная проверка embedded-конфига тем же `Zarya.exe`, запуск и контроль
  состояния runtime без совместного process-wide состояния Xray;
- строковый реестр runtime providers и нативный Core Manager;
- регистрация скачанного пользователем EXE с проверкой PE x64, SHA-256,
  version probe и повторным подтверждением изменившегося файла;
- штатные определения внешних Xray, sing-box, V2Ray, Mihomo,
  `nekobox_core.exe`, Hysteria 2 и настраиваемый custom provider;
- прямой запуск внешнего EXE без shell, безопасные placeholders, асинхронный
  stdout/stderr и Windows Job Object с `KILL_ON_JOB_CLOSE`;
- raw JSON/YAML-профили с фиксированным диалектом и локальным readiness
  endpoint;
- явный выбор compatible provider «один раз» или с сохранением в профиль —
  автоматического незаметного fallback нет;
- включение WinINet-прокси только после успешного TCP readiness probe;
- сохранение предыдущего состояния системного прокси и crash recovery;
- реальное добавление, изменение и удаление локальных профилей;
- настоящий асинхронный TCP ping профилей с сохранением задержки и результата,
  progress и отменой между соединениями;
- импорт одной или нескольких ссылок `vless://`, `vmess://`, `trojan://`,
  `ss://`, `socks://`, `hysteria2://`/`hy2://` и
  `wireguard://`/`wg://`;
- редактор общей модели протоколов, transport/TLS/REALITY, Hysteria2 и
  WireGuard; предпросмотр и сохранение runtime-конфига;
- нативный менеджер подписок: CRUD, фоновый WinHTTP download с текущим
  элементом/progress/cancel, plain/base64 share-link parsing и merge без
  изменения старых профилей при ошибке загрузки или разбора;
- JSON-хранилище полной Qt-модели профиля schema v4 и отдельное хранилище
  providers;
- автоматическую first-run миграцию обычного Qt-хранилища через checksum ZIP,
  staging-проверку связей и атомарную установку; исходные Qt-файлы остаются
  неизменными;
- ручной `.zarya-backup.zip` v1 и restore через whitelist/staging/SHA-256 с
  автоматическим pre-restore backup; определения providers сохраняются без
  EXE, абсолютных путей, версии и machine hash;
- redacted diagnostics ZIP с provider id/version/сокращённым hash и
  агрегатами профилей, но без runtime-логов, raw config, credentials,
  subscription URLs и внешних путей;
- относительные пути для пользовательских ядер внутри каталога приложения и
  абсолютные пути для остальных файлов.
- versioned routing/DNS stores и нативный редактор built-in/custom правил;
- capability validation без молчаливого отбрасывания routing/DNS-настроек;
- Geo Data Manager с WinHTTP progress/cancel, SHA-256 и атомарной заменой пары
  `geoip.dat`/`geosite.dat` в пользовательском data-каталоге;
- `Real delay` через `Zarya.exe --core-test-worker`: динамические TCP/UDP-порты,
  максимум три параллельных worker и гарантированное завершение Job Object;
- first-run flow, autostart через HKCU Run, встроенные EN/RU-таблицы и выбор
  `system|ru|en` с применением после перезапуска.

При первом обычном запуске LCL-версия читает Qt-данные из
`%LOCALAPPDATA%\Zarya\Zarya`, сначала создаёт
`.zarya-backup.zip` с SHA-256, затем проверяет преобразование во временном
каталоге и устанавливает результат в `%LOCALAPPDATA%\Zarya\LCL`. Исходные
Qt-файлы не изменяются; definitions внешних providers и пути к EXE не
переносятся. Для `--portable` и `--data-dir` этот импорт отключён.

UUID, ключи и subscription URLs остаются только в рабочем локальном хранилище и
не записываются в журнал или переносимый backup. В backup профили и подписки
восстанавливаются отключёнными, поскольку credentials, raw config и URL
намеренно исключены. URL также не показывается в таблице подписок. При включённой опции
автоматического прокси приложение изменяет WinINet только после того, как
объявленный mixed/HTTP/SOCKS endpoint принял соединение.
Snapshot лежит рядом с `profiles.json` и удаляется после успешного
восстановления.

## Сборка

Закреплённый toolchain: Lazarus 4.8, FPC 3.2.2, Go 1.26.5 и WinLibs
MinGW-w64 GCC 16.1.0. Для CI или чистой машины Lazarus/FPC и MinGW можно
установить с проверкой SHA-256 и подписи:

```powershell
.\bootstrap-toolchain.ps1
```

Скрипт сборки использует исходники
общего Go bridge из `src\runtime\embedded\xray\bridge`, собирает `c-archive`,
полностью пересобирает Pascal units со статическим ABI и выполняет embedded
self-test:

```powershell
cd D:\projects\zarya\prototypes\lcl
.\build.ps1
.\bin\Zarya.exe
```

Изолированный или portable-запуск:

```powershell
.\bin\Zarya.exe --data-dir .\scratch-data
.\bin\Zarya.exe --portable
```

`--portable` хранит данные в `bin\data`; обычный запуск использует
`%LOCALAPPDATA%\Zarya\LCL`.

Результат — только `bin\Zarya.exe` и `bin\Zarya.exe.sha256`; build-сценарий
удаляет свои временные linker-файлы и прежние development artifacts. Runtime DLL
не нужна.
Для конфигураций с `geoip:`/`geosite:` development-сборка использует данные из
`build\Release\cores\xray`; в пакете они должны находиться в `bin\cores\xray`.
MinGW по умолчанию ищется в `build\tools\winlibs`; путь можно переопределить
переменной `CC`.

Проект можно открыть в Lazarus через `zarya_lcl.lpi` для навигации и работы с
Pascal-кодом. Production EXE следует собирать через `build.ps1`: этот сценарий
добавляет статический Go archive и выполняет финальный линк, которого нет в
обычном F9 build-mode. Pascal build-mode оптимизирован (`-O2`) и не включает
heap tracing или отладочные символы.

Проверки хранилищ профилей/подписок, parser/atomic subscription merge,
backup/restore и diagnostics redaction,
реестра providers, matrix генераторов, безопасных
аргументов, fake-core supervisor (quoting, timeout, crash и Job Object),
VLESS/Xray JSON, настоящего TCP connect latency и реального embedded Xray
validation/start/readiness/stop на loopback:

```powershell
.\test.ps1
```

`known-cores.json` закрепляет официальные x64-выпуски и неизменяемые URL/SHA.
Следующая команда скачивает их только в `generated`, проверяет PE/SHA/version и
запускает validation/start/readiness/cleanup matrix; файлы никогда не попадают
в release ZIP:

```powershell
.\test-known-cores.ps1
```

Release-пакет с точным составом `Zarya.exe` + `Zarya.exe.sha256` создаётся так:

```powershell
.\package.ps1
```

С версии 1.5.13 Windows CI/release используют именно этот контракт:
`scripts/verify-release-artifacts.py --windows-lcl-single-exe` требует ровно
два файла в корне ZIP без DLL и вложенных каталогов; подпись через SignPath (только
`Zarya.exe`) опциональна и для релиза не обязательна.

## Чек-лист сравнения

1. Проверить размеры, шрифты и таблицу при 100%, 150% и 200% scaling.
2. Переключить светлую/тёмную тему и отметить нативные контролы, которые
   выбиваются из палитры.
3. Закрыть окно, вернуть его двойным кликом по tray icon, затем выйти через
   контекстное меню tray.
4. Выбрать рабочий VLESS-профиль и нажать `Запустить`: системный прокси должен
   оставаться выключенным до готовности mixed-порта.
5. Пройти интерфейс только клавиатурой.
6. Импортировать share link каждого поддерживаемого протокола, выбрать профиль,
   открыть `Runtime config…` и сохранить конфигурацию в отдельный файл.
7. Открыть `Core → Менеджер ядер…`, подключить доверенный внешний EXE и
   проверить обнаруженные версию, архитектуру и сокращённый SHA-256.

## Текущая граница provider adapters

Из общей модели генерируются отдельные конфигурации:

- Xray JSON: VLESS, VMess, Trojan, Shadowsocks, SOCKS, Hysteria2 и WireGuard;
- V2Ray JSON: VLESS, VMess, Trojan, Shadowsocks и SOCKS; REALITY отклоняется;
- sing-box JSON и NekoBox core: VLESS, VMess, Trojan, Shadowsocks, SOCKS и
  Hysteria2; WireGuard блокируется до versioned endpoint adapter;
- Mihomo YAML: VLESS, VMess, Trojan, Shadowsocks, SOCKS, Hysteria2 и WireGuard;
- Hysteria 2 YAML: Hysteria2 с локальными HTTP и SOCKS5 listeners.

Локальные external Xray и sing-box дополнительно проходят настоящий
validation/start/readiness/Job-stop integration smoke. Windows CI скачивает по
`known-cores.json` и проверяет также V2Ray, Mihomo, NekoBox core и Hysteria 2;
скачанные EXE используются только как test fixtures. Неподдерживаемые сочетания
блокируются capability matrix; raw-конфиг
можно переключить только на provider того же диалекта и после подтверждения.

Текущий статус и оставшиеся срезы описаны в `MIGRATION.md`.
