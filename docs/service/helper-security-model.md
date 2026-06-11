# Helper Security Model

## Principles

- IPC is local-only (`QLocalServer` / named pipe)
- requests require session token
- helper validates allowed runtime/core paths
- service mode may restrict client identity (`--allowed-client-sid` / UID)

## 0.28 tightening

- token auth is still required
- optional client UID check on Linux (`SO_PEERCRED`)
- Windows peer SID verification is limited; token + path restrictions remain primary

## Not weakened

Existing token and path restrictions remain in place for all helper modes.
