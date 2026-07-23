# AGENTS.md — `src/platform/`

OS abstractions for system proxy, autostart, and privilege detection.

## Layout

- `windows/`, `macos/`, `linux/` — real implementations
- `stub/` — unsupported / no-op fallbacks
- Factories select the implementation at runtime

## Rules

- Stable path sets **OS HTTP(S) proxy** to the local Xray inbound — this is not TUN.
- Keep GUI calling only the platform interfaces; no `#ifdef` sprawl in `ui/` or `app/` when a factory exists.
- Windows system proxy tests: `zarya_system_proxy_test` (Windows-only).
- Autostart and proxy restore are part of safe shutdown / recovery; coordinate with `AppController` and kill-switch recovery exceptions.
- When adding an OS behavior, implement real + stub and document platform gaps in `docs/known-limitations.md` or stable docs as appropriate.
