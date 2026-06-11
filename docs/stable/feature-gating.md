# Feature Gating

Zarya gates experimental features by **release channel** and the **Show experimental features** setting.

## Release channel

Settings → **Release channel**:

| Channel | Experimental UI default |
|---------|------------------------|
| Stable | Hidden |
| Beta | Visible (with warnings) |
| Dev | Visible |

## Settings key

- `release/channel` — `stable`, `beta`, or `dev`
- `release/showExperimentalFeatures` — user override

## Runtime behavior

- `configuredRuntimeMode` — value stored in settings
- `effectiveRuntimeMode` — value used at runtime after gating

When experimental features are hidden:

- TUN, helper, and kill switch controls are hidden from Settings
- sing-box rule sets and TUN preview are hidden from Tools
- Effective runtime falls back to **Xray system-proxy**
- Configured TUN settings are preserved, not deleted

## Recovery exception

Kill switch and system proxy **recovery** remains available even when experimental UI is hidden.

## Code

- `src/features/FeatureGate.h` — visibility and enablement checks
- `src/features/FeaturePolicy.h` — channel defaults
