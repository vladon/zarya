# Privacy and Diagnostics

Zarya diagnostics bundles are designed for troubleshooting.

## Included

- app version
- platform info
- core versions
- runtime status
- helper status
- redacted logs
- redacted config previews
- validation output
- Core Manager status (`diagnostics/core-manager-status.json`)
- subscription update summary (`diagnostics/subscription-status.json`)
- first-run status (`diagnostics/first-run-status.json`)
- packaging layout (`diagnostics/packaging-status.json`)

**Help → Copy Support Summary** copies a short redacted text summary without creating a full bundle.

## Not included

- helper token
- raw proxy links
- subscription URLs
- raw generated runtime configs
- passwords
- private keys
- packet captures

## Review before sharing

Always review the diagnostics archive before attaching it to a public issue.
