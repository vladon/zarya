# Zarya Beta Release Checklist

## Build

- [ ] Windows portable ZIP builds
- [ ] macOS app bundle builds
- [ ] Linux tarball builds

## First run

- [ ] starts with empty config
- [ ] tray works
- [ ] settings open
- [ ] no crash without cores

## Core manager

- [ ] installs Xray
- [ ] installs sing-box
- [ ] blocks update while running
- [ ] rollback works

## Basic proxy mode

- [ ] import VLESS link
- [ ] start Xray
- [ ] system proxy enabled
- [ ] stop restores proxy

## Subscriptions

- [ ] add subscription
- [ ] update subscription
- [ ] merge behavior works

## Routing/DNS

- [ ] Proxy All
- [ ] Bypass LAN
- [ ] Secure Remote DNS
- [ ] validation OK

## TUN experimental

- [ ] disabled by default
- [ ] warning shown
- [ ] helper mode works
- [ ] sing-box check runs

## Kill switch experimental

- [ ] disabled by default
- [ ] Linux nft PoC recovery works
- [ ] Windows WFP recovery works

## Backup/diagnostics

- [ ] export backup
- [ ] import backup
- [ ] diagnostics bundle redacted

## Shutdown/recovery

- [ ] close to tray
- [ ] tray exit
- [ ] unclean shutdown recovery

## Automated smoke tests

```powershell
cmake --build build --config Release --target zarya_smoke_test
.\build\Release\zarya_smoke_test.exe
```
