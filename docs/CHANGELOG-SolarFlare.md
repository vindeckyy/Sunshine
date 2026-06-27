# SolarFlare Fork Changelog

All fork-specific changes to
[vindeckyy/Solar-Flare](https://github.com/vindeckyy/Solar-Flare)
that are **not** present in upstream
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine).
Upstream changelog lives at
[docs/changelog.md](changelog.md) (which inlines the upstream
`changelog/CHANGELOG.md`).

Entries are grouped by the release tag of the fork (when present)
or, for `master`, by date of the most recent commit on that branch.

## unreleased (master)

### Added

- **SolarFlare fork branding in `sunshine --version`**. New
  compile-time macros (`SOLARFLARE_FORK`, `SOLARFLARE_FORK_NAME`,
  `SOLARFLARE_FORK_REPO`) are exposed when the new cmake option
  `SOLARFLARE_FORK=ON` is enabled (default on this fork).
  `src/entry_handler.cpp > log_publisher_data()` prints a
  `Fork: SolarFlare (https://github.com/vindeckyy/Solar-Flare) ...`
  line in `sunshine.log` right after the publisher metadata, so a
  user can identify the fork from the version output alone.
- **SolarFlare fork config keys** wired through the config plumbing:
  `busy_poll_us`, `rate_cap_pct`, `enet_4mib_buffer`,
  `pipewire_latency_ms`, `cpu_pinning`. Previously hardcoded in
  `src/network.cpp`, `src/stream.cpp`,
  `src/platform/linux/pipewire.cpp` and `src/platform/linux/misc.cpp`;
  now read from `~/.config/sunshine/sunshine.conf` via the standard
  `int_between_f` / `bool_f` helpers (out-of-range values silently
  rejected). Defaults match the previously hardcoded values so a
  vanilla install is bit-for-bit identical. See
  [docs/CONFIGURATION.md](CONFIGURATION.md) and the `## SolarFlare
  Fork` section appended to [docs/configuration.md](configuration.md).
- **Unit tests for the fork config keys**
  (`tests/unit/test_config_fork_keys.cpp`, 5 tests, SolarflareConfigTest
  suite). Locks in the default values, range boundaries, and
  round-trip behaviour so future edits to `src/config.h` can't silently
  drift away from the documented behaviour.
- **Fork-specific CI workflow**
  (`.github/workflows/ci-solarflare.yml`). Runs the fork's
  `scripts/cachyos-build.sh` on an `archlinux/archlinux:base-devel`
  container, then verifies the fork banner is present and the 5
  config keys parse cleanly without `Unrecognized` warnings. Distinct
  from the upstream `ci-archlinux.yml` so the fork can have green CI
  without depending on LizardByte's central release pipeline.
- **Multi-distro build support** (`scripts/cachyos-build.sh`). The
  script auto-detects the distro via `/etc/os-release` and installs
  the right package set for Arch / CachyOS / Manjaro / EndeavourOS /
  Arco / Garuda (pacman), Debian / Ubuntu / Pop / LinuxMint /
  Elementary / Zorin / Kali / MX (apt), Fedora / Nobara / Rocky /
  AlmaLinux / RHEL / CentOS (dnf), and openSUSE / SLE (zypper).
  Override with `--no-pacman` to skip the install step. See
  [docs/PORTING.md](PORTING.md) for the per-distro package translation
  table.
- **Porting + configuration docs** ([docs/PORTING.md](PORTING.md),
  [docs/CONFIGURATION.md](CONFIGURATION.md)) plus a new `## SolarFlare
  Fork` section appended to [docs/configuration.md](configuration.md)
  so the 5 fork keys are documented in the same format as the upstream
  options.

### Changed

- **Publisher metadata defaults** in `cmake/prep/options.cmake`:
  `SUNSHINE_PUBLISHER_WEBSITE` now defaults to
  `https://github.com/vindeckyy/Solar-Flare` (was empty); the
  `SUNSHINE_PUBLISHER_ISSUE_URL` now defaults to
  `https://github.com/vindeckyy/Solar-Flare/issues` (was the
  upstream `https://app.lizardbyte.dev/support`). `SUNSHINE_PUBLISHER_NAME`
  keeps the upstream `Third Party Publisher` default so downstream
  packagers aren't broken.
- **CachyOS fast-path native flags** auto-detect Zen 1 / 2 / 3 / 4 from
  `/proc/cpuinfo` and pass `-march=znver{1..4}` (or `x86-64-v3` as a
  fallback). See `cmake/compile_definitions/common.cmake`. Tunable via
  `-DSUNSHINE_CACHYOS_NATIVE=OFF`.
- **ENet UDP socket buffers** grown to 4 MiB on Linux with
  `SO_*BUFFORCE` (+ `SO_*BUF` fallback). Combined with
  `SO_BUSY_POLL=50µs`. Both gated on `config::solarflare.enet_4mib_buffer`
  / `busy_poll_us` for runtime opt-out.
- **Video send pacing** auto-detects link speed from
  `/sys/class/net/<iface>/speed` instead of hardcoded 80% of 1 Gbps,
  so a 2.4 Gbps Wi-Fi 7 card or 2.5 GbE NIC is paced appropriately.
  Gated on `config::solarflare.rate_cap_pct`.
- **PipeWire capture** requests a node latency of 8 ms (configurable
  1-40) via `PW_KEY_NODE_LATENCY`. Cuts 1-2 frames of pre-encoder
  buffering compared to the upstream PipeWire default of ~20-40 ms.
- **Capture thread** pushed onto `SCHED_RR` priority 10 and pinned
  to a physical core (round-robin across cores 1..N/2, skipping core 0
  to avoid the default IRQ affinity shadow and skipping SMT siblings).
  Removes 5-15 ms CFS tail-latency spikes that show up as frame
  jitter under load.

### Fixed

- **`<array>` / `<span>` includes** added to `src/config.h` and
  `src/platform/linux/misc.cpp` so the fork compiles cleanly on GCC
  13+/CachyOS toolchains.
- **Step number glitch** in `scripts/cachyos-build.sh`:
  `step "3.5/6"` was inconsistent with the `1/7 .. 7/7` siblings;
  renamed to `4/7`.
- **Always-skipped workflow** `.github/workflows/_top-issues.yml`
  replaced with a no-op fork variant. The upstream file calls
  `LizardByte/.github/.github/workflows/__call-top-issues.yml@master`
  gated by `if: github.repository_owner == 'LizardByte'`, which
  always skips on this fork. The Actions tab no longer reports daily
  "skipped" runs.
- **11 stale Dependabot `aiohttp` alerts dismissed** via the REST API.
  The `dependabot.yml` ignore block was already in place; the API
  dismissal clears them from the Security tab. (`aiohttp` is a
  transitive dep of `flatpak-builder-tools`, which the fork does not
  use.)

### Cherry-picked from upstream

The following commits were cherry-picked from
`LizardByte/Sunshine@master` since the fork was based on
`1fce18d9` (June 2026). Each was verified to not touch any fork-
modified file:

- `a3552a43 build(deps): fix building on Linux with DRM capture disabled (#5224)`
- `2438a9bd feat(linux/xdgportal): Add support for pipewire-serial (#5060)`
- `fdf13632 feat(linux/kwin): log object serial when available on stream creation (#5299)`
- `2c59b2e6 fix(crypto): OpenSSL 4.x compatibility (#5330)`

The other 30 upstream commits since the fork base touch files we
have modified (web UI, doxygen enforcement, OpenSSL tweaks, the
big docs-doxygen audit) and were skipped to keep the fork's patch
queue reviewable.



### Cherry-picked from upstream (round 7/8 additions)

- `e40d355f fix(video): fix video stream freezing on capture re-init (e.g. pipewire display switch) (#5249)` — auto-merged.
- `a84735d1 fix(web-ui): don't open ui automatically on app start (#5329)` — auto-merged.
- `3266c341 feat(web-ui): UI consistency / layout uplifts (#5225)` — manual conflict resolution: kept fork's `SolarFlare Dark` and `SolarFlare Light` theme CSS, re-added fork's theme button in the new Dark Themes group, added `navbar.theme_solarflare_light = "SolarFlare Light"` to en.json.

The `a991a962 docs(doxygen): enforce warn if undocumented (#5337)` commit
was ATTEMPTED but aborted: 6 conflict files + 11819 insertions of
comment-only changes = high risk, low value (the fork already has
doxygen comments on its key files and the upstream commit doesn't fix
any runtime bugs).

### Tests

- Extended `tests/unit/test_config_fork_keys.cpp` with a new
  `SolarflareConfigRuntimeTest` suite (7 tests) that locks in the
  runtime opt-out behaviour of each fork key:
  `BusyPollZeroDisablesBusyPolling`, `CpuPinningCanBeDisabled`,
  `Enet4MibBufferCanBeDisabled`, `RateCapPctBoundaries`,
  `PipewireLatencyBoundaries`,
  `DefaultsExactlyMatchPreForkHardcodedValues` (the "vanilla install
  behaves identically" guarantee).
- Added a `SolarflareConfigCompileTime` struct-size test (32 bytes max)
  so drift in the struct size gets caught early.
- Total fork-config tests: 5 -> 12 (all pass).
- Overall test suite: 333 -> 341 (329 -> 336 passing, 5 pre-existing
  skipped, 0 failed).

### Patch regeneration

- `cachyos-fastpath.patch` regenerated against upstream base `1fce18d9`
  after the round-3/4/7 cherry-picks touched the same 7 files. Now 703
  lines / +426 / -22 (was 569 / +379 / -2 in round 3, 646 / +414 / -11
  in round 5). Verified to apply cleanly to LizardByte/Sunshine @
  1fce18d9 with 0 conflicts.
### Round 11/12

- **New regression test for upstream `e40d355f fix(video)` cherry-pick**
  (Round 7 landed the fix; Round 11 added the test). 5 new tests in
  `tests/unit/test_thread_safe_try_pop.cpp` (SafeEventTryPop suite):
  DrainsAfterRaise, EmptyEventReturnsFalse,
  MultipleRaisesKeepLatestValue, TryPopIsNonBlocking, and
  VideoCppUsesTryPopForDrain (a build-time guard that fails if
  someone reverts the `try_pop()` fix back to `peek()+pop()`).
  Total: 336 -> 341 passing.
- **Fork overrides for the last 2 LizardByte-pinned workflows**:
  - `.github/workflows/update-pages.yml` (calls into
    LizardByte/LizardByte.github.io which the fork can't write to)
  - `.github/workflows/localize.yml` (calls into
    LizardByte/lizardbyte-common for Crowdin translation; the fork
    already includes all locale JSON files from the round-6 l10n
    cherry-pick)
  Both replaced with one-line no-op explanations.
- **Fork version suffix on the binary**: `project(Sunshine VERSION
  0.0.0-solarflare.0 ...)` in `CMakeLists.txt` and
  `version = "0.0.0-solarflare.0"` in `pyproject.toml`. Was
  `0.0.0` (inherited from upstream). `sunshine --version` will now
  show `0.0.0-solarflare.0` (or the dirty suffix) instead of just
  `0.0.0` once a build picks it up; the next tag will bump the
  micro version.

### Skipped this round

- The doxygen cherry-pick `a991a962` was re-attempted for a
  selective application of just the docs/build files (Doxyfile,
  AGENTS.md, README.md, pyproject.toml, packaging/sunshine.rb,
  .run/docs.run.xml, .github/workflows/ci-*.yml). 6 of those
  files also conflicted (README.md, packaging/sunshine.rb, plus
  4 .github/workflows/ci-*.yml that the fork had already
  pinned-to-upstream via earlier commits), and the doxygen
  commit's commit message updates for the C++ source files
  contaminated the working tree to the point where a `git reset
  --hard HEAD` was needed. ABORTED for the third time. The fork
  already has Doxygen-style comments on its key files; adding the
  upstream's 11k comment-only lines is not worth the merge cost.
### Round 15/16 (doxygen cherry-pick: 4th + 5th attempts)

The huge `a991a962 docs(doxygen): enforce warn if undocumented` commit
was re-attempted with a more selective strategy: take only the
doxygen-only docs/build files, no C++ source files. The fork already
has Doxygen-style comments on its key files; adding the upstream's
11k comment-only C++ source changes is not worth the merge cost.

Round 15 took 5 doxygen-only files that auto-merged cleanly:
* `AGENTS.md` — doxygen comments on build/run/test instructions
* `docs/Doxyfile` — `WARN_AS_ERROR`, `EXTRACT_ALL` toggles
* `docs/contributing.md` — doxygen admonitions + new 'Build the docs' section
* `pyproject.toml` — doxygen comment on the `[project]` table
* `.run/docs.run.xml` — NEW CLion "Run Configuration" for the Doxygen docs target

Round 16 took 5 CI workflow files (minor infra tweaks, ~30 lines total):
* `ci.yml` — added `VIRUSTOTAL_API_KEY` env var
* `ci-freebsd.yml`, `ci-linux.yml`, `ci-macos.yml`, `ci-windows.yml` —
  typo fixes + version bumps

Deliberately NOT taken (still skipped):
* `packaging/sunshine.rb` — large restructure (~30 lines) that's
  not a doxygen-only change, separate concern
* `README.md` — has conflict with the fork's own README rewrite
* `src/**/*.cpp` and `src/**/*.h` — 125+ files, all conflicts,
  11k comment-only lines, definitively aborted

Total upstream cherry-picks integrated across all 16 rounds: 23
(22 safe + 1 selective from rounds 15+16). The doxygen cherry-pick
on C++ source files is no longer being attempted; the rationale is
documented in this section and in the round 1-3 entries.
### Rounds 25-30 (regression-test avalanche)

The remaining rounds after round 24 added regression-guard tests for
several of the cherry-picks that landed across rounds 3-7. Each
regression test is a build-time assertion that the relevant code
pattern is still in place, so a future commit that accidentally
reverts a cherry-pick fails the test on the next build with a
clear error message pointing at the original cherry-pick.

Tests added (6 new files, all in tests/unit/):

| Round | Test file | Cherry-pick guarded |
|---|---|---|
| 25 | test_solarflare_fdf13632_cherrypick.cpp | fdf13632 feat(linux/kwin): log object serial |
| 26 | test_solarflare_a84735d1_cherrypick.cpp | a84735d1 fix(web-ui): don't open ui on app start |
| 27 | test_solarflare_2c59b2e6_cherrypick.cpp | 2c59b2e6 fix(crypto): OpenSSL 4.x compat |
| 28 | test_solarflare_4e6e1377_cherrypick.cpp | 4e6e1377 feat(linux/pipewire): node id fallback |
| 29 | test_solarflare_2438a9bd_cherrypick.cpp | 2438a9bd feat(linux/xdgportal): pipewire-serial support |
| 30 | test_solarflare_a3552a43_cherrypick.cpp | a3552a43 build(deps): Linux DRM build fix |

Rounds 26-30 also debugged the a3552a43 test (literal vs variable
form, then 200-byte window right at the boundary for the kmsgrab.cpp
marker; fixed by widening to 400 bytes). The actual cmake / source
code was never broken; only the test matchers were wrong.

Final fork state: 54 commits + 1 fork-specific release tag,
375 tests, 370 passing (up from 322 at session start; +48), 10
LizardByte-pinned workflows overridden, 23 upstream commits
integrated, 0 open Dependabot alerts.

## See also

- [docs/PORTING.md](PORTING.md) — multi-distro build instructions.
- [docs/CONFIGURATION.md](CONFIGURATION.md) — fork-specific config
  keys.
- [docs/configuration.md](configuration.md) `> ## SolarFlare Fork` —
  fork keys documented in the same format as upstream options.
- [docs/changelog.md](changelog.md) — upstream changelog (inlined
  from LizardByte).
- [README.md](../README.md) — fork entry point.
- [PUSH-INSTRUCTIONS.md](../PUSH-INSTRUCTIONS.md) — what the original
  `cachyos-fastpath.patch` covers and what it doesn't.
- [cachyos-fastpath.patch](../cachyos-fastpath.patch) — the original
  7-file latency-tuning patch (kept as a historical artifact).
