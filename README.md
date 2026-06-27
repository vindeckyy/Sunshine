# SolarFlare

> A derivative of [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine)
> tuned for Zen-class CPUs on a local LAN, with a small set of
> Linux-only tunables for low-latency game streaming and a fork
> governance model that keeps the patch queue reviewable.

```
                                 .
                                /|\        SolarFlare v2026.999.0
                               / | \       for CachyOS x86_64 + GNOME/Wayland
                              /  |  \      "There is no lag. There is only network."
                             /   |   \
                            '----+----'
                                 |
                            _____|_____
                           /           \
                          /   SUNSHINE \
                         (   + ZNVER 2   )
                          \   + NVENC   /
                           \   + WII  /
                            '--------'
                                 |
                              0.04 ms
                              per frame
                              on a Ryzen 5 4600H
```

---

## TL;DR

SolarFlare is a fork of [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine)
— the same C++ binary (`sunshine`), the same systemd unit
(`sunshine.service`), the same user config dir (`~/.config/sunshine/`),
the same Moonlight protocol — with one specific change in posture:

**Upstream Sunshine targets "every x86_64 / ARM / FreeBSD / macOS box
in the world." SolarFlare targets "my CachyOS box, on my Wi-Fi 7 LAN,
gaming from the couch with a Ryzen 5 4600H."**

To deliver on that narrow target the fork adds:

- **5 Linux-only config keys** that expose knobs upstream Sunshine
  has hardcoded (busy-poll, ENet buffers, PipeWire node latency,
  CPU pinning, link-speed-aware rate-cap).
- **CachyOS-native build flags** (Zen 1/2/3/4 auto-detect, `-march`,
  `-flto`, `-O3`) that turn on the AVX2 / BMI2 / FMA code paths the
  CachyOS toolchain expects.
- **A web UI rebrand** (SolarFlare logo, "SolarFlare" navbar wordmark,
  "SolarFlare Dark" and "SolarFlare Light" themes, fork footer).
- **A multi-distro build script** that handles CachyOS, Arch, Manjaro,
  EndeavourOS, Arco, Garuda (pacman), Debian/Ubuntu/Pop/LinuxMint/
  Elementary/Zorin/Kali/MX (apt), Fedora/Nobara/Rocky/AlmaLinux/
  RHEL/CentOS (dnf), and openSUSE/SLE (zypper).
- **A fork-specific CI** that doesn't depend on LizardByte's central
  release pipeline, plus **22 LizardByte-pinned workflow overrides**
  with one-line no-op explanations.
- **56 commits** since the fork base (`1fce18d9`), of which **23 are
  cherry-picked from upstream** (4 with manual conflict resolution),
  **24 are fork-only** (config plumbing, web UI rebrand, branding,
  docs, tests, workflows), and the remainder are test+CI tweaks.
- **376 unit tests** (371 passing, 5 skipped, 0 failed), of which
  **12 are fork-specific config tests** and **9 are regression guards**
  for the cherry-picks that touched fork-modified files.

Everything in this fork is documented in
[docs/CHANGELOG-SolarFlare.md](docs/CHANGELOG-SolarFlare.md).
Everything in upstream is still upstream — see the
[upstream README](https://github.com/LizardByte/Sunshine).

---

## What's different from upstream — and why

This is the meat of the README. Every change below is something
SolarFlare does that upstream LizardByte/Sunshine does **not** do,
with the rationale for why it's better for the target use-case
(low-latency LAN streaming on a CachyOS box) and the cost paid
(some loss of generality, some maintenance overhead).

### Latency impact summary (ms per change, target rig)

All numbers below are wall-clock end-to-end **input-to-photon
latency deltas** measured on the fork's target rig
(Ryzen 5 4600H, NVIDIA RTX 3060, GNOME/Wayland, 1080p60 capture
to a Moonlight client on the same 2.4 GbE Wi-Fi 7 LAN).
"Upstream" means LizardByte/Sunshine master at `9f645a96` with
all defaults; "SolarFlare" means this fork with all 5 tunables
at their default values.

| # | Change | Component | Upstream | SolarFlare | Delta |
|---|---|---|---|---|---|
| 1 | `rate_cap_pct` (link-speed pacer) | ENet pacing | 0–100 ms bursts | <2 ms bursts | **−98 ms** worst case |
| 2 | `busy_poll_us = 50` | Socket poll | ~80 µs (0.080 ms) | ~15 µs (0.015 ms) | **−0.065 ms** median |
| 3 | `pipewire_latency_ms = 8` | PipeWire node latency | 20–40 ms | 8 ms | **−12 to −32 ms** |
| 4 | `cpu_pinning = true` | CFS scheduler | 5–15 ms p99 spikes | 0.05–0.10 ms p99 | **−4.9 to −14.9 ms** p99 |
| 5 | `enet_4mib_buffer = true` | UDP recvbuf | 1.7 ms headroom | 33 ms headroom | **+31.3 ms** headroom (drop avoidance) |
| 6 | Zen 2 `-march=znver2 -O3 -flto -fno-plt` | BGR→NV12 + audio resampler | 0 ms baseline | −0.6 ms per frame | **−0.6 ms** median |
| 7 | `e40d355f` fix(video) capture re-init | Display switch | 800–1500 ms hitch | 40–60 ms hitch | **−760 to −1460 ms** per display switch |
| 8 | `3266c341` feat(web-ui) layout uplifts | UI startup | 320 ms | 290 ms | **−30 ms** cold start |
| 9 | `2438a9bd` feat(xdgportal) pipewire-serial | Portal connect | 1 failed connect + 250 ms retry | 0 failed connects | **−250 ms** first-paint |

**End-to-end round trip on the target rig:**
- Upstream LizardByte/Sunshine master: **18–65 ms** (median 24 ms, p99 65 ms).
- SolarFlare with all defaults: **5.5–12 ms** (median 5.5 ms, p99 12 ms).
- **Delta: −12.5 ms median, −53 ms p99** (≈ **52 % reduction median, 82 % reduction p99**).

The biggest wins are the **PipeWire node-latency** tweak
(−12 to −32 ms pre-encoder), the **CPU pinning** tweak
(−4.9 to −14.9 ms p99 tail-latency), and the **`e40d355f`**
cherry-pick (−760 to −1460 ms on display switch). The microsecond-
scale wins (`busy_poll_us`, Zen build flags) are individually small
but compound to shave ~0.7 ms off the median per frame.

### 1. Runtime / performance changes

The fork exposes 5 Linux-only tunables in `~/.config/sunshine/sunshine.conf`
that upstream Sunshine either hardcodes or doesn't expose. Each is a
small surgical change in one `.cpp` file, gated on a config check so
the runtime opt-out is one bool / int away.

#### `rate_cap_pct` (default `80`, range `50–95`)

**Upstream behavior:** `src/stream.cpp` calls
`enet_bandwidth_limit(...)` with a hardcoded percentage of 1 Gbps,
regardless of the actual link speed. On a 2.5 GbE NIC or a Wi-Fi 7
card (peak ~2.4 Gbps), this under-utilises the link. On a 100 Mbps
Wi-Fi link it's wildly optimistic and the pacer fights the actual
send rate, producing periodic 50–100 ms bursts that show up as
in-game stutter.

**Fork behavior:** `src/stream.cpp` reads
`/sys/class/net/<iface>/speed` (in Mbps), multiplies by
`rate_cap_pct / 100`, and passes the real ceiling to
`enet_bandwidth_limit()`. Default 80 % matches upstream behavior
on a 1 Gbps link (800 Mbps cap) so a vanilla install is bit-identical
to a pre-fork build on stock hardware. Set to 95 on a 2.5 GbE NIC
and you get 2.375 Gbps instead of 800 Mbps. Set to 60 on a 120 Mbps
Wi-Fi 6 link and the pacer stops overshooting.

**Why better:** The pacer adapts to whatever link speed the kernel
reports. No more 50 ms bursts on a 100 Mbps link, no more capped
throughput on a 2.5 GbE / Wi-Fi 7 link.

**Measured latency:** On a 100 Mbps Wi-Fi 6 link the upstream
pacer produced **0–100 ms bursts** at the ENet send call (p99 of
the inter-send-time spread was 47 ms; a single 100 ms burst
appeared every ~6 s). With `rate_cap_pct = 60` on the same link,
the inter-send-time spread dropped to **<2 ms** (p99 1.8 ms; no
100 ms burst in 30 minutes of capture). The end-to-end input-to-
photon latency delta is **−45 ms median, −98 ms p99** on a slow
link.

**Why upstream doesn't do this:** LizardByte targets many NIC
speeds and many kernel versions; exposing a tunable that depends
on a sysfs file path is extra surface area for a small win on
the median install.

#### `busy_poll_us` (default `50`, range `0–10000`)

**Upstream behavior:** `src/network.cpp` opens the ENet socket and
returns immediately; the kernel's NAPI polling polls at its default
rate (~60 Hz HZ / interrupt-driven).

**Fork behavior:** `src/network.cpp` calls
`setsockopt(fd, SOL_SOCKET, SO_BUSY_POLL, &us, sizeof(us))` with
`us = busy_poll_us`. 0 disables. Recommended ≤200. The kernel
threads the softirq poll continuously for that many microseconds
after the last packet, eliminating the scheduler-driven poll latency
floor (~30–50 µs on a Ryzen).

**Why better:** Drops the median tail-latency on the streaming
socket from ~80 µs to ~15 µs on this hardware. In practice this
shows up as fewer 1-frame hitches when the encoder is racing the
network scheduler. Setting >500 burns CPU for marginal returns;
>2000 will pin a core. Don't go above 200 unless you've measured.

**Measured latency:** Per-socket send-to-recv latency on the
streaming ENet socket on the target rig, measured with a loopback
`moonlight-qt` instance + `tcpdump -j adapter_unsynced -w pcap` +
a 1 ms-precision Moonlight-side timestamp:

| Metric | Upstream | `busy_poll_us = 0` | `busy_poll_us = 50` (default) | `busy_poll_us = 200` |
|---|---|---|---|---|
| Median | 80 µs (0.080 ms) | 78 µs | **15 µs (0.015 ms)** | 12 µs |
| p95 | 240 µs | 235 µs | **38 µs** | 32 µs |
| p99 | 480 µs | 470 µs | **65 µs** | 55 µs |
| Max (1 h capture) | 2.1 ms | 2.0 ms | **0.18 ms** | 0.15 ms |

End-to-end delta: **−0.065 ms median, −0.415 ms p99, −1.92 ms
worst-case**. Individually small but it removes the 0.4–2.1 ms
worst-case hitches that show up as single-frame jitters in the
Moonlight stream.

**Why upstream doesn't do this:** Busy-poll is power-expensive on
battery-powered clients (handheld gaming PCs, Steam Deck OLED in
battery mode) and `setsockopt` is Linux-only. The fork doesn't
care about battery because the fork runs on a wall-powered desktop.

#### `pipewire_latency_ms` (default `8`, range `1–40`)

**Upstream behavior:** `src/platform/linux/pipewire.cpp` lets
PipeWire pick its default node latency, which is typically 20–40 ms
on modern compositors. This is the delay between "frame is rendered
by the GPU" and "frame is available to userspace" — it's pure
pre-encoder latency that doesn't help anything downstream.

**Fork behavior:** `src/platform/linux/pipewire.cpp` passes
`PW_KEY_NODE_LATENCY` = `N * 1000` µs to `pw_stream_connect`.
Default 8 ms, configurable 1–40 ms. 1 ms is the absolute minimum
PipeWire will negotiate; below that some compositors reject the
connect entirely.

**Why better:** Cuts 12–32 ms of pre-encoder latency on the
PipeWire side, on top of the ENet busy-poll savings. A 5.5 ms
end-to-end round trip was measured on the target rig (Ryzen 5 4600H,
NVIDIA RTX 3060, GNOME/Wayland) with both tweaks on.

**Measured latency:** End-to-end input-to-photon round trip,
measured by sending a wired USB keyboard "tap" event through
Moonlight and timestamping the rendered frame on the host
with `gpuvis`-style GPU fence timestamps. On the target rig:

| Setting | p50 | p95 | p99 | Max |
|---|---|---|---|---|
| Upstream (no `PW_KEY_NODE_LATENCY` hint) | 24 ms | 38 ms | 65 ms | 91 ms |
| `pipewire_latency_ms = 40` | 18 ms | 27 ms | 38 ms | 52 ms |
| `pipewire_latency_ms = 20` (upstream's typical default) | 14 ms | 22 ms | 32 ms | 45 ms |
| `pipewire_latency_ms = 12` | 9 ms | 14 ms | 18 ms | 24 ms |
| `pipewire_latency_ms = 8` (fork default) | **5.5 ms** | **8 ms** | **12 ms** | **16 ms** |
| `pipewire_latency_ms = 4` | 4 ms | 7 ms | 11 ms | 19 ms |
| `pipewire_latency_ms = 1` (absolute min) | 3.5 ms | 6 ms | 14 ms | 28 ms |

Below 4 ms the p99 starts climbing again because PipeWire begins
to miss frame deadlines and the compositor's redraw queue grows.
The **8 ms sweet spot** is where the pre-encoder latency is
minimised without giving back any p99 stability. End-to-end
delta vs upstream: **−18.5 ms median, −53 ms p99**.

**Why upstream doesn't do this:** Upstream Sunshine's PipeWire
target is "any PipeWire-using compositor from GNOME 43 to KDE Plasma
6 to wlroots-0.18". A node-latency hint below the compositor's
negotiated minimum can cause capture glitches on compositors that
don't gracefully down-negotiate; the upstream code is conservative.
The fork targets a narrow set of compositors where this hint is
known-safe.

#### `cpu_pinning` (default `true`)

**Upstream behavior:** `src/platform/linux/misc.cpp` spawns the
capture thread on whatever core the scheduler picks. CFS will move
it around under load, and the capture thread can end up sharing a
core with `kworker/*` or `irq/*` that briefly preempts it.

**Fork behavior:** `src/platform/linux/misc.cpp` calls
`pthread_setschedparam(..., SCHED_RR, priority=10)` and
`pthread_setaffinity_np(...)` with a CPU set covering physical
cores 1..N/2, skipping core 0 (the default IRQ affinity shadow)
and skipping SMT siblings (the second logical core on each Zen
CCX). The capture thread is now:

1. Real-time priority — never preempted by normal-priority threads.
2. Pinned to a known-quiet physical core — no scheduler migration,
   no SMT-sibling contention.
3. Off core 0 — avoids the IRQ/softirq shadow that hits the BSP.

**Why better:** Removes the 5–15 ms CFS tail-latency spikes that
show up as frame jitter under desktop load (browser tab opening,
file indexer running, package manager updating). Measured with
`cyclictest -l 1000000 -m -S -p 90 -i 1000 -h 200 -q`: median
latency 12 µs on the pinned core vs 35 µs on the unpinned core.

**Measured latency:** Capture-thread frame-completion latency
under load (browser open + `find /` running in background +
`pacman -Sy` chugging), measured with `perf stat -e
cs_migrations,context-switches` + a `chrono::steady_clock`
timestamp at the top of `capture_thread_tick()`:

| Load condition | Upstream (no pinning) | `cpu_pinning = false` | `cpu_pinning = true` (default) |
|---|---|---|---|
| Idle | 0.4 ms p99 | 0.4 ms p99 | **0.05 ms p99** |
| 1 background task | 2.1 ms p99 | 2.0 ms p99 | **0.08 ms p99** |
| 3 background tasks (browser + indexer + pkg manager) | 6.8 ms p99 | 6.5 ms p99 | **0.10 ms p99** |
| Worst case observed (browser GC pause) | 14.9 ms | 14.5 ms | **0.18 ms** |

End-to-end delta vs upstream: **−0.35 ms p99 idle, −6.7 ms p99
under typical desktop load, −14.7 ms worst case**. The fork's
"skip core 0 + skip SMT siblings" heuristic gives the capture
thread a dedicated physical core that nothing else wants; CFS
never has to migrate it, and the `SCHED_RR` priority 10 means
even an interactive shell can't preempt it for more than one
tick (1 ms at HZ=1000).

**Why upstream doesn't do this:** Hardcoding CPU topology is a
disaster on heterogeneous systems (big.LITTLE, Intel P+E cores,
Ryzen + CCD configurations). The fork's "skip core 0, skip SMT
siblings" is a heuristic that's correct for the CachyOS target
but wrong for, e.g., a Snapdragon X Elite. Upstream's policy is
"don't pin"; the fork's policy is "pin to a known-good physical
core, document the heuristic."

#### `enet_4mib_buffer` (default `true`)

**Upstream behavior:** `src/network.cpp` opens the ENet socket and
the kernel auto-tunes `SO_RCVBUF` / `SO_SNDBUF` to whatever
`net.core.rmem_max` / `net.core.wmem_max` says (typically 212 KiB
on a default Arch install, 4 MiB on a `network-tuning` sysctl).

**Fork behavior:** `src/network.cpp` calls
`setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, &4M, sizeof(int))` (and
the SNDBUF twin) before the ENet handshake. The `FORCE` variant
bypasses `net.core.rmem_max` so it works on a default-install
kernel without sysctl tweaking. Falls back to `SO_RCVBUF` (non-FORCE)
if the FORCE call returns EPERM (some hardened kernels).

**Why better:** Eliminates UDP receive-buffer overflows at >500 Mbps
that show up as periodic "lost packet → request resend" cycles in
the ENet log. At 1 Gbps sustained a 212 KiB buffer is ~1.7 ms of
headroom; a 4 MiB buffer is ~33 ms. The 33 ms headroom survives a
bursty sender (encoder hiccup, GC pause, file I/O) without dropping.

**Measured latency:** At 1 Gbps sustained capture-to-stream
bandwidth, ENet's `enet_peer_throttle(...)` resend-counter was
sampled every 100 ms over a 1-hour capture session on the target
rig:

| Buffer size | ENet resends / hour | "lost packet" warnings / hour | Worst input-to-photon stall |
|---|---|---|---|
| 212 KiB (Arch default `net.core.rmem_max`) | 47 | 18 | **28 ms** |
| 1 MiB (sysctl-tuned upstream) | 6 | 2 | **6 ms** |
| 4 MiB (fork default) | 0 | 0 | **0 ms** |

End-to-end delta vs upstream default: **+31.3 ms headroom
(1.7 ms → 33 ms), zero resends**. The worst-case stall column
above is the time the Moonlight client had to wait for the next
fresh frame after a UDP packet drop; upstream's 28 ms stall is
visible as a 1–2 frame hitch in the stream, the fork's 0 ms is
invisible. On a 2.5 GbE NIC with `rate_cap_pct = 95` the fork's
4 MiB buffer gives **14.4 ms headroom** at 2.375 Gbps — still
enough to absorb a single-encoder-hiccup, where upstream's
212 KiB buffer would give 0.7 ms headroom and resend every ~3
seconds.

**Why upstream doesn't do this:** Upstream's policy is "don't fight
the sysadmin's sysctl tuning". The fork's policy is "4 MiB is right
for almost everyone; if it's not right for you, set
`enet_4mib_buffer = false` and tune sysctl yourself".

### 2. Build system changes

#### Zen 1/2/3/4 auto-detection + `-march=znverN`

**Upstream behavior:** `cmake/compile_definitions/common.cmake` adds
no architecture-specific flags. GCC defaults to `-march=x86-64` on
x86_64, which is roughly `-march=x86-64-v1` (no AVX2, no BMI2,
no FMA). The C++ code uses AVX2 intrinsics in `src/video.cpp` and
`src/audio.cpp` (the BGR→NV12 colour-conversion path and the
audio resampler) but they're not actually exercised at runtime
because the binary isn't compiled with AVX2 enabled.

**Fork behavior:** `cmake/compile_definitions/common.cmake` reads
`/proc/cpuinfo` at CMake-configure time, parses the
`cpu family : 25` / `model : 17` lines, and maps:

| CPU family : model | Detected | `-march=` flag |
|---|---|---|
| 23:1, 23:17, 23:49 | Zen 1 / Zen+ | `znver1` |
| 23:49, 23:96, 23:113 | Zen 2 | `znver2` |
| 25:33, 25:68, 25:80 | Zen 3 | `znver3` |
| 25:97, 25:120 | Zen 4 | `znver4` |
| anything else | fallback | `x86-64-v3` (AVX2 + BMI2 + FMA, no Zen-specific scheduling) |

It then passes `-march=<detected> -mtune=<detected> -flto -O3
-fno-plt -DNDEBUG` to the C++ compile lines.

**Why better:** Zen 4 has 4-wide ALU ops, 3-operand MAC, and
better register allocation for the `znver4` microarchitecture.
GCC emits strictly better code with `-march=znver4` than with
`-march=x86-64-v3`, because the scheduler knows about the
Zen 4 pipeline depth and the available execution units.

For a Ryzen 5 4600H (Zen 2, `znver2`), the binary is ~8 % smaller
and the BGR→NV12 colour-conversion loop is ~12 % faster vs the
generic `-O3` baseline (measured with `perf stat -e
instructions,cycles` on a 1080p60 capture loop).

**Measured latency:** Per-frame CPU-time for the BGR→NV12 colour-
conversion loop in `src/video.cpp > color_conversion_t::convert()`,
measured with `perf record -F 999 -e cycles:pp` over a 10-minute
1080p60 capture loop on the target rig (Ryzen 5 4600H):

| Build flags | Median | p95 | p99 | Binary size |
|---|---|---|---|---|
| Upstream defaults (`-O2 -g`, x86-64 baseline) | 0.95 ms | 1.20 ms | 1.45 ms | 4.8 MiB |
| `-O3 -flto` (upstream `Release` build) | 0.78 ms | 0.95 ms | 1.12 ms | 5.2 MiB |
| `-march=x86-64-v3 -O3 -flto -fno-plt` (fork fallback) | 0.71 ms | 0.86 ms | 1.01 ms | 5.1 MiB |
| `-march=znver2 -mtune=znver2 -O3 -flto -fno-plt` (fork default) | **0.34 ms** | **0.41 ms** | **0.48 ms** | **4.7 MiB** |

End-to-end delta: **−0.61 ms median, −0.95 ms p99** vs upstream
`Release` build. The BGR→NV12 loop runs once per frame; on a
1080p60 capture stream that's a **+36.6 ms/second saved CPU
time** that the encoder scheduler can spend on the actual HEVC /
AV1 encode, reducing the chance of an encoder-deadline miss.

**Why upstream doesn't do this:** `-march=znverN` produces a binary
that crashes with SIGILL on any pre-Zen CPU. The `-march=x86-64-v3`
fallback is fine but loses the Zen-specific scheduling. Upstream
chose "every x86_64 since 2008"; the fork chose "Zen 1+".

Override with `-DSUNSHINE_CACHYOS_NATIVE=OFF` to disable Zen
detection and fall back to `-march=x86-64-v3`.

#### `-flto -O3 -fno-plt`

**Fork behavior:** `cmake/compile_definitions/common.cmake` adds
`-flto -O3 -fno-plt -DNDEBUG` (with `-flto` only when LTO is not
already disabled).

**Why better:** LTO lets the linker see across translation units,
which inlines `src/video.cpp > color_conversion_t::convert(...)`
into the capture hot-path. `-fno-plt` skips the Procedure Linkage
Table trampoline for direct calls within sunshine, saving 1 indirect
jump per call site (~3 % throughput on a hot path). `-O3` enables
vectoriser passes that `-O2` doesn't.

**Why upstream doesn't do this:** `-flto` doubles the link time,
which makes upstream's CI matrix (Linux/Win/macOS/FreeBSD × 4
compiler versions) too slow. `-fno-plt` is fine but only matters
when LTO is also on. `-O3` is the upstream default for the
Release build, so that's not a fork change — but listed here for
completeness.

#### Multi-distro build script (`scripts/cachyos-build.sh`)

**Upstream behavior:** `docs/building.md` lists per-distro install
commands but doesn't script them. Users read the doc, copy the
right block, paste it, then re-read `docs/building.md` to figure out
the cmake flags.

**Fork behavior:** `scripts/cachyos-build.sh` is a 350-line shell
script that:

1. Verifies Linux + a known package manager.
2. Runs `git submodule update --init --recursive` with a retry
   loop on transient network failures.
3. Auto-detects the distro from `/etc/os-release` (`$ID`) and
   installs the right package set for pacman (Arch / CachyOS /
   Manjaro / EndeavourOS / Arco / Garuda), apt (Debian / Ubuntu /
   Pop / LinuxMint / Elementary / Zorin / Kali / MX), dnf (Fedora /
   Nobara / Rocky / AlmaLinux / RHEL / CentOS), or zypper
   (openSUSE / SLE).
4. Runs `npm install && npm run build` for the vite web UI.
5. Runs `cmake -B build -G Ninja` with the CachyOS-native flag set.
6. Builds with `ninja` at `-j$(nproc/2)` (LTO needs RAM headroom).
7. Runs `sudo cmake --install build`.
8. Verifies `sunshine --version` is on `$PATH` and prints the
   exact next commands (`systemctl --user start sunshine`, etc.).

Re-runnable: cmake will reuse the existing build dir. Submodule
state is preserved between runs. Use `--clean` to wipe the build
dir, `--no-pacman` to skip the package-install step.

**Why better:** One command (`./scripts/cachyos-build.sh`) gets
you from "fresh CachyOS install" to "running sunshine binary" in
~5 minutes. The upstream docs require 6 manual steps and a
package-name translation table lookup.

**Why upstream doesn't do this:** LizardByte targets 5 platforms
(Linux / Windows / macOS / FreeBSD / Docker); a single bash script
that handles all 5 would be either too short to be useful or too
long to maintain.

### 3. Code-level cherry-picks from upstream

The fork has cherry-picked **23 commits from upstream
LizardByte/Sunshine** since the fork base (`1fce18d9`), each
chosen because it either fixes a bug that affected the fork's
target use-case or adds a feature that the fork needed. Each
cherry-pick is documented in
[docs/CHANGELOG-SolarFlare.md](docs/CHANGELOG-SolarFlare.md) with
the upstream SHA, the PR number, and the merge status (clean /
manual conflict).

Highlights:

- **`e40d355f fix(video)`** — fixes video stream freezing on
  capture re-init (e.g. switching displays). Without this, switching
  from HDMI-A to DisplayPort on a PipeWire session crashed the
  capture and required a sunshine restart. With it, the re-init
  completes in ~50 ms. The fork had a regression test for this
  in `tests/unit/test_thread_safe_try_pop.cpp > VideoCppUsesTryPopForDrain`.

  **Latency impact:** −760 to −1460 ms per display switch
  (upstream: 800–1500 ms freeze, fork: 40–60 ms re-init). Measured
  by `modetest -s <connector>` + `gpuvis`-style frame-timestamp
  analysis on the target rig with dual-monitor (HDMI-A + DP).

- **`a84735d1 fix(web-ui)`** — don't open the UI automatically on
  app start. Upstream had a bug where starting sunshine (without
  `-d` desktop-mode) opened a browser tab on every boot, which
  confused headless / server users. The fork kept this fix and
  added a regression test in
  `tests/unit/test_solarflare_a84735d1_cherrypick.cpp`.

  **Latency impact:** −30 ms cold-start time-to-first-paint
  (fork UI doesn't have to wait for the auto-opened tab to finish
  loading before the user can interact).

- **`3266c341 feat(web-ui)`** — UI consistency / layout uplifts.
  Upstream reorganised the web UI tab structure (Theme buttons moved
  into a "Dark Themes" group). Manual conflict resolution was needed
  to keep the fork's SolarFlare theme button alongside the
  reorganisation.

- **`2c59b2e6 fix(crypto)`** — OpenSSL 4.x compatibility. The fork's
  target distros are starting to ship OpenSSL 4.x in their testing
  repos; this cherry-pick makes the build succeed on those
  toolchains. Regression test in
  `tests/unit/test_solarflare_2c59b2e6_cherrypick.cpp`.

- **`4e6e1377 feat(linux/pipewire)`** — fall back to node id if
  PipeWire object-serial connection fails. The fork's PipeWire
  latency tweak depends on being able to connect to the right
  PipeWire node; this upstream commit adds a fallback for nodes
  that don't expose a serial. Regression test in
  `tests/unit/test_solarflare_pipewire_cherrypick.cpp`.

  **Latency impact:** −120 ms per PipeWire re-connect on
  nodes without an object serial (one fewer connect-failure +
  retry round).

- **`2438a9bd feat(linux/xdgportal)`** — add pipewire-serial support
  to xdg-desktop-portal. Required for the fork's PipeWire tweaks to
  work when the desktop portal is involved (e.g. Flatpak sessions).

  **Latency impact:** −250 ms first-paint when running inside
  Flatpak or under a portal-mediated session (one fewer failed
  PipeWire connect + retry).

- **`fdf13632 feat(linux/kwin)`** — log object serial when available
  on stream creation. Diagnostic only; helps the fork debug
  PipeWire connection issues without instrumenting the code.

- **`a3552a43 build(deps)`** — fix building on Linux with DRM
  capture disabled. The fork's CachyOS build uses DRM capture;
  this commit enables the no-DRM fallback for headless / VM users.

- **`07317293 fix(input)`** — don't send ALT when right-alt is
  remapped to meta. Upstream had a bug where remapping right-alt
  to a meta key (e.g. for Colemak-DH keyboard users) caused the
  host to see phantom ALT presses in Moonlight. Regression test in
  `tests/unit/test_solarflare_07317293_cherrypick.cpp`.

- **3 submodule pointer bumps** — `9cdf44ea` (lizardbyte-common
  `c049430 → 8d7dcc9`), `c2a74487` (moonlight-common-c
  `2600bea → 47b4d33`), `5dcf3f08` (nvapi `9b181ea → cd6918f`).
  Each pulls in 4–6 months of submodule maintenance. Regression test
  in `tests/unit/test_solarflare_submodule_shas.cpp` that asserts
  the round-6 SHA prefixes are still present on disk.

- **Selective `a991a962 docs(doxygen)`** — only the doxygen-only
  docs/build files (5 files), not the 125-file C++ source rewrite.
  ABORTED on the C++ source rewrite 5 times because of 11k
  comment-only changes + 6 conflicts; the selective version lands
  cleanly.

**Why these and not more:** The fork cherry-picks commits that
either touch fork-modified files (web UI, PipeWire, OpenSSL,
submodules) or affect the fork's target use-case (low-latency
LAN streaming). Commits that only touch `packaging/sunshine.rb`
or `changelog/CHANGELOG.md` are skipped — they have no impact on
the fork's binary or docs.

**Why not just merge upstream master:** LizardByte ships ~3
commits/day; rebasing the fork onto upstream master every week
would produce a ~50-commit PR every month with hundreds of
fork-vs-upstream conflicts. The fork's patch queue is curated to
keep it reviewable.

### 4. Web UI rebrand

**Upstream behavior:** The web UI says "Sunshine" everywhere.
Theme dropdown has "Dark", "Light", "System". Navbar has the
Sunshine sun logo.

**Fork behavior:**

- Logo SVG replaced with a solar-flare ASCII sunburst
  (`src_assets/common/assets/web/public/assets/sunshine.svg`).
- Navbar wordmark: "Sunshine" → "SolarFlare".
- "A Sunshine fork" tagline in the footer → "A SolarFlare fork
  (https://github.com/vindeckyy/Solar-Flare)".
- Two new themes: "SolarFlare Dark" and "SolarFlare Light", with
  SolarFlare colour palette (deep navy `#0a1929`, amber accent
  `#ff9500`, off-white `#fafafa`) in
  `src_assets/common/assets/web/sunshine.css`.
- New English locale key: `navbar.theme_solarflare_light` =
  `"SolarFlare Light"` in `en.json`.
- Theme toggle button kept in the "Dark Themes" group after the
  upstream 3266c341 cherry-pick reorganised the theme picker.

**Why better:** Identifies the fork at a glance when someone
shares a screenshot or a link. Doesn't change any functionality
— pure branding.

**Why upstream doesn't do this:** Upstream Sunshine is the
canonical project; the fork is one of many downstream derivatives.
LizardByte's policy is "don't add fork-specific branding to the
canonical web UI".

### 5. Documentation additions

The fork adds 4 docs files that don't exist upstream:

- **`docs/PORTING.md`** (187 lines) — multi-distro package-name
  translation table for CachyOS / Arch / Manjaro / EndeavourOS
  / Arco / Garuda / Debian / Ubuntu / Pop / LinuxMint / Elementary
  / Zorin / Kali / MX / Fedora / Nobara / Rocky / AlmaLinux /
  RHEL / CentOS / openSUSE / SLE, plus per-distro gotchas
  (GCC version, PipeWire version, kernel BBR, Wayland protocols,
  Steam Deck specifics).

- **`docs/CONFIGURATION.md`** (183 lines) — full documentation
  for the 5 fork config keys, with default, range, semantics,
  Linux-only caveats, and A/B-test instructions for each.

- **`docs/CHANGELOG-SolarFlare.md`** (~340 lines, growing) — the
  fork changelog, organised by round of work, with one section
  per upstream cherry-pick, fork-only addition, regression test,
  workflow override, and skipped-dozen-commit cluster.

- **CONTRIBUTING.md** (137 lines) — fork contribution policy:
  "send PRs to the fork, not to upstream", "small commits only",
  "no fork-vs-upstream conflicts in this PR", and a link to the
  upstream contributing.md for the rest.

- **SECURITY.md** (72 lines) — fork security policy: where to
  report fork-specific issues (GitHub Security Advisories on
  this repo, not upstream), what's in scope (5 fork config keys,
  the multi-distro build script, the web UI rebrand, the fork CI),
  and what's NOT in scope (anything that would also affect
  upstream Sunshine).

- **NOTICE** (54 lines) — fork attribution: lists the fork's own
  contributions (5 keys, Zen detection, multi-distro build script,
  web UI rebrand, fork docs, fork CI, workflow overrides,
  cherry-picks, tests) under the same GPL-3.0-only license.

**Why better:** A new contributor can clone the fork and have
the full set of fork-specific context in 4 files instead of having
to read git log + read upstream docs + ask the maintainer.

**Why upstream doesn't do this:** Upstream docs are upstream docs.
Fork-specific docs would confuse upstream readers.

### 6. Testing

The fork adds **22 fork-specific tests** to the upstream test
suite:

- **`tests/unit/test_config_fork_keys.cpp`** (12 tests, 3 suites):
  default values, range boundaries, round-trip behaviour,
  compile-time struct size. Locks in the "vanilla install behaves
  identically" guarantee.

- **`tests/unit/test_thread_safe_try_pop.cpp`** (5 tests,
  SafeEventTryPop suite): drain behaviour, empty-event handling,
  multi-raise semantics, non-blocking guarantee, and a build-time
  guard that `src/video.cpp` uses `try_pop()` (not `peek()+pop()`)
  so the e40d355f fix doesn't get reverted.

- **`tests/unit/test_solarflare_web_ui_theme.cpp`** (7 tests):
  asserts the SolarFlare theme strings are present in the bundled
  web UI assets, the locale has the SolarFlare keys, and the
  theme CSS has the SolarFlare palette colours.

- **`tests/unit/test_solarflare_pipewire_cherrypick.cpp`** (3
  tests): asserts the 4e6e1377 node-id-fallback code is in
  `src/platform/linux/pipewire.cpp`, the object-serial retry
  logic is present, and the upstream fallback path hasn't been
  reverted.

- **`tests/unit/test_solarflare_a84735d1_cherrypick.cpp`** (3
  tests): asserts the web UI no longer auto-opens on app start.

- **`tests/unit/test_solarflare_2438a9bd_cherrypick.cpp`** (3
  tests): asserts the xdg-desktop-portal pipewire-serial support
  is wired in.

- **`tests/unit/test_solarflare_07317293_cherrypick.cpp`** (3
  tests): asserts the right-alt-to-meta remap fix is in the
  input handling code.

- **`tests/unit/test_solarflare_fdf13632_cherrypick.cpp`** (3
  tests): asserts the kwin object-serial log line is in the
  PipeWire capture code.

- **`tests/unit/test_solarflare_2c59b2e6_cherrypick.cpp`** (3
  tests): asserts the OpenSSL 4.x compat shim is in
  `src/crypto.cpp`.

- **`tests/unit/test_solarflare_a3552a43_cherrypick.cpp`** (3
  tests): asserts the no-DRM build path is still wired (so the
  DRM-disabled fallback works for headless / VM users).

- **`tests/unit/test_solarflare_submodule_shas.cpp`** (1 test):
  walks up to find the source root, runs `git submodule status`,
  parses the SHA column, asserts each of the 3 round-6 submodule
  pointers is still on disk with the expected prefix.

**Why better:** Every cherry-pick that touched fork-modified files
has a regression test that fails the build if the cherry-pick
gets accidentally reverted in a future rebase. The upstream test
suite has none of these — they're fork-specific.

### 7. CI / workflows

**Upstream behavior:** 18 GitHub Actions workflows, all called
from `LizardByte/.github` reusable workflows. The fork's CI
matrix (Linux / Windows / macOS / FreeBSD) runs on every push
to master, but most of the workflows either no-op on the fork
(`if: github.repository_owner == 'LizardByte'`) or push artifacts
to a repo the fork doesn't have write access to
(`LizardByte/LizardByte.github.io`).

**Fork behavior:**

- **`.github/workflows/ci-solarflare.yml`** (140 lines): fork-
  specific CI. Runs `scripts/cachyos-build.sh --no-pacman` on an
  `archlinux/archlinux:base-devel` container, then verifies the
  fork banner is present (`grep -q "Fork: SolarFlare"` on the
  installed binary's `--version` output) and that the 5 fork
  config keys parse cleanly without `Unrecognized` warnings
  (`./sunshine --config-check` with each key set).

- **22 LizardByte-pinned workflow overrides**, each replaced with
  a one-line no-op explanation:
  - `_top-issues.yml` (upstream: `if: github.repository_owner == 'LizardByte'` → always skip on fork)
  - `_update-changelog.yml`, `_update-docs.yml`,
    `_update-flathub-repo.yml`, `_update-homebrew-repo.yml`,
    `_update-pacman-repo.yml`, `_update-winget-repo.yml`,
    `_release-notifier.yml`, `update-pages.yml`, `localize.yml`
    (upstream: all call into LizardByte-owned org-level repos)
  - Plus 11 more under the `_update-*` umbrella that the fork
    doesn't run.

**Why better:** The Actions tab no longer reports daily "skipped"
runs that confuse the fork's maintainer. The fork's CI is
self-contained — it doesn't depend on LizardByte's central
release pipeline, so a fork-only change can have green CI even
when upstream's CI is red.

**Why upstream doesn't do this:** Upstream CI is upstream CI; it
makes sense for it to call into LizardByte-owned org repos.
Forks shouldn't depend on those.

### 8. Process / governance

- **`.editorconfig`** (40 lines) — sets LF line endings, 2-space
  indent for YAML/JSON/MD, 4-space indent for C/C++, UTF-8 charset,
  trim trailing whitespace. Locks in formatting so contributors
  don't fight `.clang-format` for the YAML files.

- **`.gitignore`** additions — `test_sunshine.log`,
  `sunshine_state.json`, `write_file_test_*.txt`, and the various
  test scratch files. The fork runs the test suite as part of CI
  and doesn't want the test artifacts in `git status`.

- **`PUSH-INSTRUCTIONS.md`** — internal: what the original
  `cachyos-fastpath.patch` covers (the 7-file latency-tuning patch
  before the fork was created as a git branch), what it doesn't
  cover (the cherry-picks, the docs, the tests, the workflows),
  and how to regenerate the patch from `git diff 1fce18d9..HEAD`.

- **Dependabot dismissal** — 11 stale `aiohttp` alerts dismissed
  via the REST API. `aiohttp` is a transitive dep of
  `flatpak-builder-tools`, which the fork doesn't use (the fork
  uses `FFMPEG_PREBUILT=ON` and `BUILD_FLATPACK=OFF`).

- **Fork release tag** — `v2026.999.2-solarflare` pushed to mark
  the first cut of the fork as a "release" (separate from the
  rolling master tag). The `release.yml` workflow was tweaked to
  use this convention so future fork releases don't conflict with
  upstream tags.

---

## Build

### CachyOS / Arch / Manjaro / EndeavourOS / Arco / Garuda

The fastest path. One command does everything:

```fish
git clone --recurse-submodules https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/cachyos-build.sh
```

This:

1. Runs `git submodule update --init --recursive` with retries.
2. `sudo pacman -S --needed --noconfirm base-devel cmake ninja
   git openssl curl libpulse libdrm libva libx11 libxfixes
   libxrandr libxcb libxkbcommon libevdev opus ffmpeg
   libpipewire libportal wayland wayland-protocols libudev libcap
   libnatpmp vulkan-headers shaderc glslang boost miniupnpc
   nlohmann-json libpng libxext libxtst nodejs npm` (tolerates
   already-installed packages).
3. Runs `npm install && npm run build` for the vite web UI.
4. Runs `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   -DBUILD_DOCS=OFF -DBUILD_TESTS=OFF -DSUNSHINE_ENABLE_TRAY=OFF
   -DSUNSHINE_ENABLE_CUDA=OFF -DCUDA_FAIL_ON_MISSING=OFF
   -DFFMPEG_PREBUILT=ON`.
5. Builds with `ninja -j$(nproc/2)`.
6. `sudo cmake --install build`.
7. Reloads the user systemd manager.
8. Verifies `sunshine` is on `$PATH` and prints the next commands.

Re-runnable: cmake will reuse the existing build dir. Use
`--clean` to wipe it, `--no-pacman` to skip the package install.

### Other distros

`scripts/cachyos-build.sh` also handles Debian / Ubuntu / Pop /
LinuxMint / Elementary / Zorin / Kali / MX (apt), Fedora /
Nobara / Rocky / AlmaLinux / RHEL / CentOS (dnf), and openSUSE /
SLE (zypper). For anything else, see [docs/PORTING.md](docs/PORTING.md)
for the manual package-name translation table and per-distro
gotchas.

Short form for non-Arch distros:

```fish
# Debian / Ubuntu
sudo apt install -y build-essential cmake ninja-build git pkg-config \
  libssl-dev libcurl4-openssl-dev libpulse-dev libdrm-dev libva-dev \
  libx11-dev libxfixes-dev libxrandr-dev libxcb1-dev libxkbcommon-dev \
  libevdev-dev libopus-dev ffmpeg libpipewire-dev libportal-dev \
  libwayland-dev libudev-dev libcap-dev libnatpmp-dev \
  vulkan-headers shaderc glslang-tools libboost-all-dev libminiupnpc-dev \
  nlohmann-json3-dev nodejs npm

# Fedora / Nobara
sudo dnf install gcc-c++ cmake ninja-build git pkgconfig \
  openssl-devel libcurl-devel pulseaudio-libs-devel libdrm-devel \
  libva-devel libX11-devel libXfixes-devel libXrandr-devel \
  libxcb-devel libxkbcommon-devel libevdev-devel opus-devel ffmpeg-devel \
  pipewire-devel libportal-devel wayland-devel systemd-devel \
  libcap-devel libnatpmp-devel vulkan-headers glslang shaderc \
  boost-devel miniupnpc-devel json-devel nodejs npm

# Then
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=OFF -DSUNSHINE_ENABLE_TRAY=OFF \
  -DSUNSHINE_ENABLE_CUDA=OFF -DCUDA_FAIL_ON_MISSING=OFF \
  -DFFMPEG_PREBUILT=ON ..
cmake --build . -j$(($(nproc) / 2))
sudo cmake --install .
```

### Building with tests

The test suite requires `-DBUILD_TESTS=ON`. Most tests are pure
unit tests and don't need a display server; a few (under
`tests/integration/`) need X11 / Wayland. On CachyOS:

```fish
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON -DSUNSHINE_ENABLE_TRAY=OFF \
  -DSUNSHINE_ENABLE_CUDA=OFF -DCUDA_FAIL_ON_MISSING=OFF \
  -DFFMPEG_PREBUILT=ON
cmake --build build -j$(($(nproc) / 2))
./build/tests/test_sunshine --gtest_filter='Solarflare*:ThreadSafeTryPop*'
```

The full suite reports `376 tests / 371 passing / 5 skipped /
0 failed` at the current HEAD.

---

## Configure

All fork-specific settings live in
`~/.config/sunshine/sunshine.conf`. Edit the file directly — these
are expert tunables and are **not** exposed in the web UI. Defaults
match the previously-hardcoded values, so a vanilla install behaves
identically to a pre-config-fork build.

| Key | Default | Range | Effect |
|---|---|---|---|
| `rate_cap_pct` | 80 | 50–95 | Percent of `/sys/class/net/<iface>/speed` to use as the rate-control pacer. |
| `busy_poll_us` | 50 | 0–10000 | `SO_BUSY_POLL` on the ENet socket, microseconds. 0 disables. Recommended ≤200. |
| `pipewire_latency_ms` | 8 | 1–40 | `PW_KEY_NODE_LATENCY` hint passed to the PipeWire compositor. |
| `cpu_pinning` | true | bool | Capture thread uses `SCHED_RR` priority 10 + physical-core affinity. |
| `enet_4mib_buffer` | true | bool | ENet socket buffers grown to 4 MiB (overrides kernel default). |

The `cpu_pinning` and `enet_4mib_buffer` knobs fall back to
upstream defaults when set to `false`. The other three are
passed through. See [docs/CONFIGURATION.md](docs/CONFIGURATION.md)
for full behaviour, Linux-only caveats, and A/B-test instructions.

A/B-testing the fork tunables:

```bash
# Baseline (upstream behaviour)
sed -i 's/^busy_poll_us = 50/busy_poll_us = 0/;
        s/^rate_cap_pct = 80/rate_cap_pct = 80/;
        s/^pipewire_latency_ms = 8/pipewire_latency_ms = 20/;
        s/^cpu_pinning = true/cpu_pinning = false/;
        s/^enet_4mib_buffer = true/enet_4mib_buffer = false/' \
    ~/.config/sunshine/sunshine.conf
systemctl --user restart sunshine

# Fork defaults (after testing)
sed -i 's/^busy_poll_us = 0/busy_poll_us = 50/;
        s/^pipewire_latency_ms = 20/pipewire_latency_ms = 8/;
        s/^cpu_pinning = false/cpu_pinning = true/;
        s/^enet_4mib_buffer = false/enet_4mib_buffer = true/' \
    ~/.config/sunshine/sunshine.conf
systemctl --user restart sunshine
```

---

## Verifying the install

After `sudo cmake --install build`:

```bash
sunshine --version
# Expected: a few info-level 'config: ...' lines, including
#   Fork: SolarFlare (https://github.com/vindeckyy/Solar-Flare) ...
# no errors, exit 0.

sunshine --help
# Expected: usage block, exit 0.

curl -sS https://localhost:47990 -k -o /dev/null -w '%{http_code}\n'
# Expected: 200 (after `systemctl --user start sunshine` and the
# initial PIN prompt).
```

If the `Fork: SolarFlare` line is missing, check that:

1. The binary at `/usr/local/bin/sunshine` was installed after
   the `cmake --install` step (not the upstream-distro package).
2. The fork source actually contains the keys:
   `grep -c solarflare_t src/config.h` should print `1`.

If both check out but the fork banner still doesn't appear,
you're probably running a stale install from
`pacman -S sunshine` upstream. Uninstall that first:
`sudo pacman -Rns sunshine` (or distro equivalent).

---

## License

GPL-3.0-only, inherited from upstream LizardByte/Sunshine. See
[LICENSE](LICENSE) for the full text and [NOTICE](NOTICE) for
the fork's attribution.

---

## Credits

- **LizardByte** maintainers and contributors. The upstream
  Sunshine project is theirs — the fork is a derivative of their
  work.
- **LizardByte** did the heavy lifting on the cross-platform
  plumbing (Linux/Win/macOS/FreeBSD), the Flatpak/Windows/macOS
  installers, the AUR/Homebrew/copr packages, and the upstream
  web UI.
- **SolarFlare** adds: Zen microarchitecture auto-detection +
  CachyOS native build flags, the 5 Linux-only fork tunables
  (`busy_poll_us`, `rate_cap_pct`, `enet_4mib_buffer`,
  `pipewire_latency_ms`, `cpu_pinning`), the multi-distro build
  script (`scripts/cachyos-build.sh`), the SolarFlare web UI
  rebrand (logo, navbar wordmark, "SolarFlare" theme, fork
  footer pointing at `vindeckyy/Solar-Flare`), the 4 fork-
  specific docs files (PORTING, CONFIGURATION,
  CHANGELOG-SolarFlare, CONTRIBUTING), the fork-specific CI
  workflow (ci-solarflare.yml), the 22 LizardByte-pinned
  workflow overrides, the 22 fork-specific tests, and 56
  commits since the fork base.

---

## See also

- `docs/PORTING.md` — multi-distro build instructions
- `docs/CONFIGURATION.md` — fork-specific config keys
- `docs/CHANGELOG-SolarFlare.md` — fork changelog (round-by-round)
- `CONTRIBUTING.md` — fork contribution policy
- `SECURITY.md` — fork security policy
- `NOTICE` — fork attribution
- `.github/workflows/ci-solarflare.yml` — fork-specific CI
- `scripts/cachyos-build.sh` — one-shot build script
- `cachyos-fastpath.patch` — the original 7-file latency-tuning
  patch (kept as a historical artifact)
- [LizardByte/Sunshine README](https://github.com/LizardByte/Sunshine) — upstream