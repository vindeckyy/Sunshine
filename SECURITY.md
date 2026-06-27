# Security

This document covers the SolarFlare fork's security policy. For Sunshine
itself, see upstream's
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine) and the
[upstream security policy](https://github.com/LizardByte/.github/blob/master/SECURITY.md)
(if one is published).

## Reporting a vulnerability in the fork

If you find a security issue that is **specific to the SolarFlare fork** --
the 5 fork config keys, the CachyOS native build flags, the multi-distro
build script, the SolarFlare web UI rebrand, the post-install verification
block, or any other code that lives only on this fork -- please report
it via one of:

* **GitHub Security Advisories** (preferred):
  https://github.com/vindeckyy/Solar-Flare/security/advisories/new
* **GitHub issue** tagged `security`: open a public issue and the
  maintainer (`@vindeckyy`) will DM you to coordinate a private
  disclosure. We don't publish a security@ email because the fork is
  a single-maintainer hobby project; the public issue + DM flow is
  faster in practice.

For **upstream Sunshine security issues** (anything that would also
affect a stock LizardByte/Sunshine build), please report them at
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine/security/advisories/new)
instead. Fixes are cherry-picked onto the fork once upstream ships
them.

## Scope

The fork's security surface is small. The interesting bits are:

| Component                  | Security impact                                    |
|----------------------------|----------------------------------------------------|
| 5 fork config keys in `src/config.h`  | None (config is read from `~/.config/sunshine/sunshine.conf`, not a network surface) |
| `scripts/cachyos-build.sh` post-install verification | None (reads binary version, writes a temp file in /tmp) |
| Fork-specific web UI branding (logo, navbar, theme) | None (purely cosmetic) |
| CachyOS native flags (-march=znverN, -flto, -O3) | None (compiler flags) |
| fork-publisher metadata (`vindeckyy/Solar-Flare` URL) | Low (the URL is logged on startup, no auth tokens) |

Everything else on the fork is a direct copy of upstream code and any
vulnerability found there is an upstream issue.

## Supported versions

Only the `master` branch of this fork is maintained. There is no LTS
branch and no `release/X` long-term-support branches. If you need a
patched build, pull the latest master and run `./scripts/cachyos-build.sh`.

## Update policy

The fork follows upstream's release cadence. Critical security fixes
are cherry-picked onto the fork as soon as upstream ships them (see
[docs/CHANGELOG-SolarFlare.md](CHANGELOG-SolarFlare.md) for the
cherry-pick log). The fork does **not** maintain its own security
advisories; if you need CVE-tracked notifications, watch upstream.

## What to expect when you report

1. A maintainer acknowledgement within ~7 days (the fork is a
   single-maintainer project, so response time is variable).
2. A reproducer (we'll ask you for one if you didn't provide it).
3. A fix committed to `master` and cherry-picked from upstream if
   it's a Sunshine issue.
4. A note in `docs/CHANGELOG-SolarFlare.md` mentioning the fix.

The fork does **not** ship backports to a separate LTS branch and does
**not** publish security advisory records to GitHub Security Advisories
for fork-specific bugs (a small project that ships to one user's
CachyOS box doesn't need a CVE database).
