# sing-box Rule Sets in Zarya

## Why rule sets are separate from Xray geo data

Xray routing uses `geoip.dat` and `geosite.dat` managed by **Geo Data Manager**.

Experimental sing-box TUN mode can use **local binary rule sets** (`.srs`) referenced from `route.rule_set` and DNS/route rules. These are different artifacts with different lifecycles. Zarya does not convert Xray `.dat` files into sing-box `.srs`.

## Storage layout

Under the Zarya data directory:

```
sing-box/
  rule-set/           # binary .srs files (e.g. geosite-ru.srs)
  rule-set-source/    # optional source JSON before compile
  rulesets.json       # custom catalog entries
```

Portable mode uses `data/sing-box/...` next to the app; non-portable uses the OS app data location.

Rule sets are referenced by **absolute path** in generated sing-box config.

## Built-in catalog

Zarya ships a catalog of known tags referenced by built-in routing/DNS profiles:

- `geosite-private`, `geoip-private`
- `geosite-ru`, `geoip-ru`
- `geosite-cn`, `geoip-cn`
- `geosite-geolocation-not-cn`
- `geosite-category-ads-all`

Built-in entries without a configured download URL show **Source missing** until you import a local `.srs` or add a custom URL.

## Custom rule sets

Custom entries can be stored in `rulesets.json` with:

- Binary `.srs` download URL
- Source JSON URL (downloaded then compiled with `sing-box rule-set compile`)
- Optional checksum URL

## Compile JSON to SRS

```bash
sing-box rule-set compile --output geosite-ru.srs geosite-ru.json
```

Zarya runs the same command when you use **Compile JSON** in Rule Set Manager.

## Mapping geosite:/geoip: to rule_set

When a local `.srs` exists for a tag (e.g. `geosite:ru` → `geosite-ru.srs`):

- `route.rule_set` lists the local binary files
- Route/DNS rules use `"rule_set": ["geosite-ru"]` instead of native `"geosite": ["ru"]`

If `.srs` is missing:

- Default: warning + fallback to native `geosite`/`geoip` fields (sing-box check decides)
- **Require local .srs** (Settings): missing required tags block TUN start

## Known limitations (0.17)

- No Clash/Surge rule provider import
- No automatic remote rule-set refresh inside sing-box
- No conversion from Xray `.dat`
- Built-in download URLs are not invented; manual import is supported

## Troubleshooting

1. Open **Tools → sing-box Rule Sets**
2. Check **Required by active TUN config**
3. Import `.srs` or compile JSON for missing tags
4. Run **Preview sing-box TUN config** and **sing-box check**
