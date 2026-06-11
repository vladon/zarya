# Risk Register

| Risk | Severity | Area | Mitigation | 1.0 status |
|------|---------:|------|------------|------------|
| System proxy not restored | Critical | runtime | startup recovery + safe shutdown | must fix |
| Diagnostics leak secrets | Critical | diagnostics | redaction audit | must fix |
| Core Manager selects wrong asset | High | core updates | asset selector tests | must fix |
| macOS unsigned warning | Medium | packaging | docs, signing later | acceptable |
| TUN breaks networking | Critical | experimental | hidden in stable by default | acceptable if gated |
| Kill switch blocks networking | Critical | experimental | hidden in stable + recovery visible | acceptable if gated |
| App self-update without verification | Critical | updater | download-only in beta; no auto-install | acceptable if gated |
| Helper privilege escalation | Critical | experimental | optional install + warnings | acceptable if gated |

Use this register for go/no-go decisions before 1.0.
