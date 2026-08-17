# LCL migration status

Qt-код не удаляется до прохождения stable release matrix и используется как
executable specification. LCL-хранилище устанавливается только после checksum
backup, staging-преобразования и полной проверки. Legacy-файлы не изменяются;
`--portable` и `--data-dir` автоматическую миграцию не запускают.

## Реализовано

- production target `Zarya.exe` со статическим Go `c-archive`, embedded Xray
  ABI v2 и HTTP/mixed/SOCKS URL probe без изменения системного прокси;
- типизированный `TZaryaRuntimeRequest`, provider/process/config/readiness,
  routing/DNS/settings/geodata и node-worker contracts;
- profile schema v4, routing/DNS schema v1 и migration journal schema v2;
- routing `Proxy All`, `Bypass LAN`, `Bypass RU`, `Bypass LAN and RU`, custom;
- DNS System, Secure Remote, China direct/global remote и Custom;
- capability-aware Xray, V2Ray, sing-box/NekoBox, Mihomo и Hysteria2 generators;
  unsupported routing/DNS блокирует запуск, а raw config не переписывается;
- embedded и внешние Xray/sing-box/V2Ray/Hysteria2/Mihomo/NekoBox/custom
  providers, безопасные argv placeholders, stdout/stderr pipes и Windows Job;
- `known-cores.json` с закреплёнными x64 URL/SHA/version/commands и отдельная
  CI integration matrix, не включающая скачанные EXE в артефакт;
- first-run Qt migration profiles/subscriptions/routing/DNS и разрешённых
  QSettings с fallback на Bypass LAN/System DNS и фиксацией причины;
- Geo Data Manager: user data directory, WinHTTP progress/cancel, checksum,
  temporary files и атомарная парная замена geoip/geosite;
- provider-independent TCP ping и Real delay через отдельный
  `Zarya.exe --core-test-worker`, dynamic TCP/UDP ports, concurrency 3,
  timeout 10 секунд и typed final stdout JSON;
- mixed/HTTP/SOCKS readiness каждые 100 мс до 5 секунд перед WinINet proxy;
- точный WinINet snapshot/restore, crash recovery, tray, safe shutdown и
  корректно quoted HKCU Run autostart;
- compiled-in EN/RU, first-run language choice и нативные формы routing/DNS,
  geodata, providers, subscriptions, backup и diagnostics;
- portable `.zarya-backup.zip` без raw config, credentials, subscription URL,
  external EXE/полных путей; diagnostics дополнительно исключает runtime logs;
- закреплённая Windows-сборка Lazarus 4.8/FPC 3.2.2/Go 1.26.5/MinGW-w64,
  hosted Windows CI и self-hosted Windows 10/11 release gate;
- release packaging, отклоняющий любой состав кроме `Zarya.exe` и
  `Zarya.exe.sha256`.

## До stable cutover

1. Оформить накопленные изменения короткими PR с частичным staging; коммиты и
   Git-операции выполняются только после отдельного разрешения пользователя.
2. Пройти реальную matrix всех закреплённых внешних ядер на hosted Windows и
   release gate на чистых Windows 10/11 x64.
3. Завершить ручную DPI 100/150/200%, tab order, keyboard navigation и
   screen-reader проверку нативных форм.
4. Поставить контрольный тег последней Windows Qt-версии
   (`windows-qt-final-1.5.12`), отдельным PR переключить Windows packaging на
   LCL и ещё одним удалить Windows Qt/lib_ui targets. Linux/macOS Qt пока
   оставить. С версии 1.5.13 Windows CI/release и SignPath уже работают по
   LCL-контракту: ровно `Zarya.exe` + `Zarya.exe.sha256` в корне ZIP
   (`--windows-lcl-single-exe`), подписывается только `Zarya.exe`.
5. После cutover отдельно развивать gated embedded sing-box, TUN,
   helper/service, kill switch и self-update.

## Инварианты

- stable рекомендует только embedded Xray system-proxy;
- системный прокси меняется только после подтверждённой готовности runtime;
- provider не переключается автоматически, raw config не меняет диалект;
- GUI не повышает привилегии, внешние EXE не скачиваются приложением;
- предыдущие WinINet-значения восстанавливаются, включая отсутствовавшие;
- UUID, ключи, raw config и subscription URLs не попадают в логи/backup;
- `--portable` хранит mutable state рядом с приложением;
- TUN/helper/kill switch/embedded sing-box не влияют на stable без opt-in.
