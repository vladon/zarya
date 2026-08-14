# LCL migration status

Qt-код не удаляется до функционального паритета и используется как executable
specification. При первом обычном запуске LCL-версия автоматически обнаруживает
Qt-хранилище, создаёт checksum-backup, переносит данные через staging и только
после полной проверки атомарно устанавливает отдельное LCL-хранилище. Legacy-
файлы не изменяются. `--portable` и `--data-dir` автоматическую миграцию не
запускают.

## Реализовано

- нативный Win32 LCL shell, tray, светлая/тёмная тема и DPI scaling;
- локальное schema-v4 хранилище полной Qt-модели профиля и настройки INI;
- share-link import для VLESS, VMess, Trojan, Shadowsocks, SOCKS, Hysteria2 и
  WireGuard;
- редактор общей protocol/transport/TLS/REALITY/Hysteria2/WireGuard модели;
- production target `Zarya.exe` и воспроизводимый статический линк Go
  `c-archive` с embedded Xray ABI v1;
- embedded validation в изолированной worker-инстанции того же EXE,
  start/state/log drain/stop в основной инстанции;
- публичные runtime-контракты, строковый provider registry и JSON store;
- внешние presets Xray/sing-box/V2Ray/Mihomo/NekoBox core/Hysteria 2/custom;
- Xray generator и embedded validation matrix для VLESS, VMess, Trojan,
  Shadowsocks, SOCKS, Hysteria2 и WireGuard;
- отдельные capability-aware generators V2Ray, sing-box/NekoBox core, Mihomo
  и standalone Hysteria2;
- Core Manager: trust confirmation, PE/SHA-256/version detection, check,
  change/remove/open/use и блокировка изменившегося EXE;
- внешний process supervisor без shell, с массивом аргументов, ограниченным
  набором placeholders, pipes и Job Object `KILL_ON_JOB_CLOSE`;
- raw JSON/YAML profiles, dialect-safe совместимость и явный fallback;
- first-run Qt migration для profiles/subscriptions/routing/DNS: полный
  pre-import ZIP со SHA-256, staging parse/reload/relation validation,
  атомарная установка и сохранение legacy-файлов;
- mixed/HTTP/SOCKS readiness gate: 100 мс, timeout 5 секунд;
- WinINet snapshot/apply/restore и startup crash recovery;
- subscriptions CRUD, atomic JSON store, фоновые WinHTTP download/progress/
  cancel, plain/base64 parser и non-destructive source-key merge;
- асинхронный TCP ping профилей с progress/cancel и сохранением результата;
- `.zarya-backup.zip` v1: sanitized provider definitions, SHA-256 manifest,
  ZIP whitelist, staging validation, atomic restore и pre-restore backup;
- diagnostics ZIP с provider id/version/short hash и агрегатами без raw config,
  credentials, subscription URL, runtime logs и внешних путей;
- headless tests, fake-core process matrix, реальные embedded validation и
  loopback runtime smoke, а также external Xray/sing-box integration smoke.

## Следующие стабильные срезы

1. Добавить pinned real-core matrix для V2Ray, Hysteria2, Mihomo и NekoBox core;
   версионировать особенности схем и команд известных выпусков.
2. Routing и DNS: перенос моделей/валидаторов/генераторов и выбор активных
   профилей в основном окне.
3. Real delay через выбранный provider и изолированный локальный endpoint с
   ограничением concurrency; базовый provider-independent TCP ping уже работает.
4. Geo data manager и autostart; расширить уже работающие backup/restore и
   first-run migrator на будущие версии схем.
5. Экспериментальный sing-box TUN
   за теми же feature gates, что в Qt-версии.
6. EN/RU resources, accessibility tests, packaging/signing и переключение
   основного build/release entrypoint на LCL.

## Инварианты

- системный прокси меняется только после подтверждённой готовности runtime;
- предыдущие WinINet-значения восстанавливаются точно, включая отсутствие
  registry values;
- UUID, ключи и subscription URLs не записываются в журнал;
- GUI остаётся непривилегированным;
- `--portable` хранит mutable state рядом с приложением.
