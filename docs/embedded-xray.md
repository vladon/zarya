# Embedded Xray runtime

Zarya vendors Xray-core `v26.3.27` (`d2758a023cd7f4174a5a5fa4ff66e487d4342ba0`)
under `third_party/xray-core/`. The source manifest is
`third_party/xray-core.zarya.json`; update it with
`scripts/update-vendored-xray.ps1` rather than editing upstream files in place.

## Runtime layout

- Windows: `zarya-xray.dll` is loaded from the absolute application directory.
- Linux: `libzarya-xray.so` is loaded from the absolute application directory.
- macOS: the Go `c-archive` is linked into the app and is not dynamically unloaded.
- `cores/xray/` remains the mutable asset directory for `geoip.dat`,
  `geosite.dat`, and matcher cache data. It no longer contains the runtime.
- Generated Xray JSON is passed to the bridge in memory. It is not written to a
  runtime config file and never appears in the command line.

The Zarya-owned ABI is versioned independently of Xray. The loader rejects a
missing symbol set or ABI mismatch and reports repair/reinstall guidance without
including the generated config in errors.

## Lifecycle and readiness

The bridge serializes calls, allows one active Xray instance, makes stop
idempotent, bounds its log queue, and recovers panics at every exported function.
Zarya enables the system proxy only after the local mixed port accepts a
connection. A readiness timeout stops Xray and leaves the system proxy disabled.

Real-delay and diagnostic validation that may overlap the main instance run in
`zarya-core-test-worker`. The worker receives config through stdin, loads the same
bridge, is attached to kill-on-close process handling, and is canceled with its
test job.

## Hard cut

Xray has no executable fallback. Existing `cores/xrayPath` values are intentionally inert:
Zarya never reads, changes, exports, or imports them. A missing or incompatible embedded runtime
requires repair or reinstallation of the application.

## Future sing-box placement

`ICoreRuntimeHost` separates runtime placement from UI/backend policy. A future
embedded sing-box implementation uses the same launch/result/state contract.
Privileged TUN execution belongs in `zarya-helper`; the GUI will proxy the
runtime contract over authenticated IPC and will not pass an executable path.
