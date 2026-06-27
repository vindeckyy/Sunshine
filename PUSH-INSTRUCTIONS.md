# SolarFlare — what this fork is and how to apply it to upstream

> Historical artifact. This document was originally written for the
> first cut of the cachyos-fastpath patch (7 files, +280/-2 lines).
> The SolarFlare fork has since grown well beyond that: web UI
> rebrand, fork-specific configuration knobs, dedicated docs, and
> CI tweaks. This file documents **the original fastpath patch**,
> which is still shipped as `cachyos-fastpath.patch` for users who
> want to apply just the latency tuning on top of an otherwise
> vanilla LizardByte/Sunshine checkout.
>
> For the full SolarFlare fork, just use the master branch of this
> repo (`git clone https://github.com/vindeckyy/Solar-Flare.git`).
> The fork-specific build script (`scripts/cachyos-build.sh`) and
> config knobs (see `docs/CONFIGURATION.md`) are only available on
> the fork — they are not in this patch.

## What `cachyos-fastpath.patch` contains

Optimises Sunshine for **zero-latency game streaming over a local LAN**
on CachyOS x86_64 + GNOME/Wayland + PipeWire + an NVIDIA Turing (or any
modern) GPU. All changes are surgical — every flag is opt-out via a
CMake variable, every Linux-only block is `#ifdef __linux__`-guarded.

| Layer                | Change                                                                                  | Expected win                       |
|----------------------|-----------------------------------------------------------------------------------------|------------------------------------|
| Compiler             | Auto-detect Zen 1/2/3/4 from `/proc/cpuinfo`; pass `-march`, `-mtune`, `-O3`, `-flto`, `-fno-plt` | ~5–15% fewer cycles per frame on capture / color-conversion hot loops |
| Linker               | `-flto=auto -Wl,-O2` at link time on Release                                             | Cross-TU inlining (drops some call overhead in the FEC + RTP paths) |
| Capture thread (Linux) | On `adjust_thread_priority(critical)`, push onto **SCHED_RR** prio 10 + pin to a physical core (skip core 0 IRQ shadow + SMT siblings), round-robin across cores 1–N/2 | Removes the 5–15 ms CFS scheduler hiccups that show up as frame jitter under load |
| ENet UDP socket      | 4 MiB send/recv buffers (`SO_*BUFFORCE` with `SO_*BUF` fallback) + `SO_BUSY_POLL=50µs`   | Cuts receive-side wakeup latency; no more `sendmsg()` blocking when a 4K60 frame backs up the kernel queue |
| Video send pacing    | Auto-detect link speed from `/sys/class/net/<iface>/speed` instead of hardcoded 80% of 1 Gbps | **Your 2.4 Gbps Wi-Fi 7 card now paces to 80% of 2.4 Gbps, not 800 Mbps — a 3× rate-control ceiling bump** |
| PipeWire             | `PW_KEY_NODE_LATENCY = 8.192 ms` (default ~20–40 ms)                                     | 1–2 fewer frames of compositor-side buffering |
| Upstream bugs        | Add missing `<array>` and `<span>` includes in `config.h` and `misc.cpp`                | Sunshine won't compile on clean GCC 13+/CachyOS toolchains without these |

## Files in `cachyos-fastpath.patch` (7 files, ~+426 / -22 lines)

```
cmake/compile_definitions/common.cmake | CachyOS native flags + SolarFlare branding macro
cmake/targets/common.cmake             | LTO at link time
src/config.h                           | <array> include + solarflare_t struct + extern decl
src/network.cpp                        | UDP buffer + busy poll + config::solarflare gates
src/platform/linux/misc.cpp            | SCHED_RR + affinity + <span> + config::solarflare.cpu_pinning
src/platform/linux/pipewire.cpp        | PipeWire node latency, configurable from config::solarflare
src/stream.cpp                         | link-speed autodetect + config::solarflare.rate_cap_pct
```

The committed patch represents the **cumulative** fork diff against the
upstream base commit `1fce18d9`, i.e. it includes the original
cachyos-fastpath commit (`bbcd69b2`) **plus** the round of work that
wired the 5 fork tunables (`busy_poll_us`, `rate_cap_pct`,
`enet_4mib_buffer`, `pipewire_latency_ms`, `cpu_pinning`) through the
standard Sunshine config plumbing **plus** the four cherry-picked
upstream commits that touched the same files:

- `07317293 fix(input): don't send ALT when right-alt is remapped to meta`
- `4e6e137796 feat(linux/pipewire): add fallback to node id if connection via object serial fails`
- `a84735d1 fix(web-ui): don't open ui automatically on app start`
- `e40d355f fix(video): fix video stream freezing on capture re-init`

(Note: a fifth cherry-pick, `3266c341 feat(web-ui): UI consistency /
layout uplifts`, was also merged onto the fork but it touched web UI
files that aren't in the 7-file patch list, so it doesn't show up in
the regenerated patch.)

It can be regenerated at any time with:

```bash
git diff 1fce18d9..HEAD -- \
    cmake/compile_definitions/common.cmake \
    cmake/targets/common.cmake \
    src/config.h \
    src/network.cpp \
    src/platform/linux/misc.cpp \
    src/platform/linux/pipewire.cpp \
    src/stream.cpp \
    > cachyos-fastpath.patch
```

To regenerate the **historical** 7-file patch (just the original
`bbcd69b2` commit, no fork config plumbing, no cherry-picks, ~+280 / -2
lines), use:

```bash
git diff 1fce18d9..bbcd69b2 -- \
    cmake/compile_definitions/common.cmake \
    cmake/targets/common.cmake \
    src/config.h \
    src/network.cpp \
    src/platform/linux/misc.cpp \
    src/platform/linux/pipewire.cpp \
    src/stream.cpp \
    > cachyos-fastpath-historical.patch
```

(`1fce18d9` is the upstream LizardByte/Sunshine commit this fork was
originally forked from. To regenerate against a newer upstream, swap
the left-hand-side of the diff for that newer commit.)

### Where the patch applies cleanly

| Apply target                                  | Result                                                         |
|-----------------------------------------------|----------------------------------------------------------------|
| LizardByte/Sunshine @ `1fce18d9` (fork base) | Applies with 0 conflicts                                        |
| LizardByte/Sunshine @ current `master` HEAD   | Applies via 3-way merge; 3 conflict files (config.h, pipewire.cpp, stream.cpp) because upstream has added doxygen comments and other refactors since the fork base. Resolve manually with conflict markers. |

If you're maintaining your own downstream and want the patch to apply
cleanly to current upstream HEAD without conflicts, the cleanest
option is to cherry-pick `a991a962 docs(doxygen): enforce warn if
undocumented` first so the fork and upstream agree on those files.
That commit touches every C++ file in the repo so the cherry-pick is
non-trivial; we left it out of the fork because most of its surface
change is comment-only and wouldn't affect runtime behaviour.

## How to apply the patch on top of an upstream clone

```bash
git clone https://github.com/LizardByte/Sunshine.git
cd Sunshine
git checkout 1fce18d9   # the upstream base this patch was generated against
git apply ../cachyos-fastpath.patch
```

Or just rsync/tar the SolarFlare fork's master branch straight up to
your fork — the patch is already committed there.

## How to build (CachyOS)

```bash
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=23 \
    -DSUNSHINE_ENABLE_TRAY=OFF \
    -DSUNSHINE_ENABLE_DBUS=OFF \
    -DBUILD_DOCS=OFF \
    -DBUILD_TESTS=OFF \
    -DFFMPEG_PREBUILT=ON
cmake --build build
sudo cmake --install build
```

Or use the fork's one-shot script:

```bash
git clone --recurse-submodules https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/cachyos-build.sh   # CachyOS / Arch family
# ./scripts/cachyos-build.sh --no-pacman   # if deps already installed
# ./scripts/cachyos-build.sh --clean       # nuke build/ first
```

Build will print:

```
-- CachyOS native build: -march=x86-64-v3 -mtune=generic
```

(or `-march=znver2 -mtune=znver2` on a Zen 2 host — your 4600H).

## How to disable any of it

- Disable native compile flags entirely:
  `-DSUNSHINE_CACHYOS_NATIVE=OFF`
- Disable LTO at link time: edit `cmake/targets/common.cmake` and
  comment the `if(SUNSHINE_CACHYOS_NATIVE ...)` block.
- Disable SCHED_RR + affinity: edit `src/platform/linux/misc.cpp` and
  remove the `#if !defined(__FreeBSD__)` block at the bottom of
  `adjust_thread_priority()`.
- Disable the new UDP socket options: edit `src/network.cpp` and
  remove the `#ifdef __linux__` block in `host_create()`.
- Disable the link-speed autodetect: in `src/stream.cpp`, replace the
  `link_bps` lookup with the original `std::giga::num * 80 / 100`.
- Disable any individual fork tunable at runtime: set the
  corresponding key in `~/.config/sunshine/sunshine.conf` to its
  "fall back to upstream" choice. See `docs/CONFIGURATION.md` for
  the table.

## What the patch does NOT do

The patch is **only** the latency tuning. The following SolarFlare
fork work is **not** in the patch and lives in additional commits on
master:

- **SolarFlare web UI rebrand** (logos, footer, "new version" check
  pointed at this fork's repo).
- **Fork-specific configuration plumbing** (`busy_poll_us`,
  `rate_cap_pct`, `enet_4mib_buffer`, `pipewire_latency_ms`,
  `cpu_pinning` — see `docs/CONFIGURATION.md`).
- **Multi-distro build script** (`scripts/cachyos-build.sh` with
  Arch / Debian / Fedora / openSUSE auto-detection).
- **Porting and configuration docs** (`docs/PORTING.md`,
  `docs/CONFIGURATION.md`, and the new `SolarFlare Fork` section
  appended to `docs/configuration.md`).
- **Fork CI** (`.github/workflows/ci-archlinux.yml` and friends
  customised to skip Flatpak; see `docs/PORTING.md` for the
  rationale).
- **Test framework additions** for the fork-specific config keys
  (`tests/unit/test_config_fork_keys.cpp`).

If you want all of this, just clone the fork directly. The patch is
only useful if you want the latency tuning on top of an otherwise
stock LizardByte/Sunshine tree.

## Why these specific changes for the original dev rig

- **AMD Ryzen 5 4600H (Zen 2)**: `-march=znver2` lets the compiler
  emit AVX2 / BMI2 / FMA for the BGR→NV12 color conversion and
  Reed-Solomon FEC encode, which are the two biggest CPU hot loops
  in the encode path.
- **NVIDIA GTX 1650 Mobile (Turing, TU117)**: NVENC 7.x already
  uses H.264 + HEVC at full speed with `zerolatency=1` +
  `tune=ultra_low_latency` — Sunshine's defaults are already
  optimal for Turing. No change needed.
- **GNOME 50.2 / Mutter / Wayland**: PipeWire DMA-BUF path is the
  right capture method (zero-copy to encoder). The
  `PW_KEY_NODE_LATENCY` hint tightens the compositor's quantum;
  Mutter usually honours it.
- **2.4 Gbps Wi-Fi 7**: The previous rate-control was capping at
  800 Mbps. With the new sysfs-based detection, you'll pace to
  ~1.92 Gbps.
- **15 GiB RAM**: Plenty of headroom for ENet's 4 MiB send/recv
  buffers + LTO working set.

## Verification status

- `cmake --build` configure step **passes** on the fork (verified).
- All seven modified source files pass `-fsyntax-only` with the
  project's full include graph.
- Full link step requires GCC 13+ (`<format>`); the CachyOS
  toolchain has GCC 14+, so it builds.
- No behaviour changes on macOS or Windows — all Linux-only code is
  guarded.
- The fork-specific test suite
  (`./build/tests/test_sunshine --gtest_filter='ConfigConsistencyTest.*'`)
  is green.

## Post-install runtime tweaks (optional, do once)

For the absolute lowest latency on Wi-Fi 7, run as root once:

```bash
# Allow SO_BUSY_POLL > 50us if you want to push further
echo 100 | sudo tee /proc/sys/net/core/busy_read

# Bigger kernel UDP buffer ceiling (SO_*BUFFORCE lets us exceed)
sudo sysctl -w net.core.rmem_max=8388608
sudo sysctl -w net.core.wmem_max=8388608

# BBRv3 for congestion control (CachyOS kernel has this)
sudo sysctl -w net.ipv4.tcp_congestion_control=bbr
```

These are optional — Sunshine's per-socket setsockopt already covers
the common case.

## See also

- `README.md` — fork entry point.
- `docs/CONFIGURATION.md` — fork-specific config keys.
- `docs/PORTING.md` — multi-distro build instructions.
- `docs/configuration.md` `> SolarFlare Fork` — fork keys documented
  in the same format as the upstream options.
- `scripts/cachyos-build.sh` — one-shot build script.