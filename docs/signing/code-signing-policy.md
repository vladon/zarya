# Code Signing Policy

Free code signing provided by [SignPath.io](https://signpath.io/), certificate by
[SignPath Foundation](https://signpath.org/).

## Scope

Official Windows release artifacts built from this repository may contain this
Zarya-authored signed binary:

- `Zarya.exe`

Helper, updater, core-test-worker, and Xray bridge binaries have not been part
of the Windows package since the 1.5.13 LCL single-EXE cutover. Signing is
optional and currently inactive: stable releases are published unsigned with
mandatory SHA-256 checksums until SignPath is activated.

Downloaded or bundled third-party cores, including Xray and sing-box, are not
signed as Zarya-authored code. Their versions and checksums are recorded by the
release packaging process.

## Build and approval policy

- Signing requests originate only from the public `vladon/zarya` repository.
- Release binaries are built on GitHub-hosted runners from a `v*` tag.
- Pull-request and ordinary branch builds do not receive signing credentials.
- SignPath origin verification must succeed before a signing request is accepted.
- Every release signing request requires explicit approval.
- The returned artifact must pass Authenticode, timestamp, package, checksum, and
  secret-audit verification before publication.

## Project roles

- Committers and reviewers: [Vladislav Yaroslavlev (`@vladon`)](https://github.com/vladon)
- Signing approvers: [Vladislav Yaroslavlev (`@vladon`)](https://github.com/vladon)

All project members with repository or signing access must use multi-factor
authentication.

## Privacy

Zarya communicates with network services only as required by user-configured proxy,
subscription, core, geo-data, diagnostics, and update workflows. Secrets and raw
proxy configuration are excluded from diagnostics by default. See
[Privacy and Diagnostics](../public-beta/privacy-and-diagnostics.md) and the
[Security Model](../security-model.md).

## Verification and incident response

Users can verify the Authenticode publisher and SHA-256 checksum by following
[Release Verification](release-verification.md). Suspected misuse or a compromised
release must be reported through the repository security contact or issue tracker;
the signing policy will be disabled while the incident is investigated and the
certificate will be revoked when required.
