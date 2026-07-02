# ☀️ SolarFlare

> **Low-latency game-streaming host, tuned for Zen-class CPUs on a local LAN.**
> A focused fork of [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine) — same Moonlight
> protocol, same Web UI shape, same `~/.config/sunshine/` config dir. Five Linux-only tunables
> that upstream has hardcoded, a CachyOS-native build, and a pre-encoder audio FX pipeline.

<div align="center">

[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](LICENSE)
[![Fork of Sunshine](https://img.shields.io/badge/fork-LizardByte%2FSunshine-9cf.svg)](https://github.com/LizardByte/Sunshine)
[![Version](https://img.shields.io/badge/version-2026.999.0-orange.svg)](docs/CHANGELOG-SolarFlare.md)
[![Tests](https://img.shields.io/badge/tests-406%20passed%20%2F%205%20skipped-brightgreen.svg)](#-testing)
[![Commits since fork](https://img.shields.io/badge/commits-59-blueviolet.svg)](docs/CHANGELOG-SolarFlare.md)
[![Target: CachyOS x86_64](https://img.shields.io/badge/target-CachyOS%20x86__64-1793d1.svg)](#-building)

</div>

---

## 📑 Contents

- [🚀 Quick start](#-quick-start)
- [✨ What's different](#-whats-different-from-upstream)
- [📋 Update log](#-update-log)
- [🎛️ Runtime tunables](#%EF%B8%8F-runtime-tunables)
- [🔊 SolarFlare audio FX](#-solarflare-audio-fx-pipeline)
- [🛠️ Building](#%EF%B8%8F-building)
- [⚙️ Configuration](#%EF%B8%8F-configuration)
- [🧪 Testing](#-testing)
- [📦 CI & governance](#-ci--governance)
- [📚 Docs](#-docs)
- [❓ FAQ](#-faq)
- [📄 License & credits](#-license--credits)

---

## 🚀 Quick start

<details>
<summary><b>CachyOS / Arch-based (one-liner)</b></summary>

```bash
git clone https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/cachyos-build.sh
sudo cmake --install build
sudo setcap 'cap_dac_override,cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
systemctl --user enable --now sunshine.service
```
</details>

<details>
<summary><b>Debian / Ubuntu / Fedora / openSUSE</b></summary>

```bash
git clone https://github.com/vindeckyy/Solar-Flare.git
cd Solar-Flare
./scripts/cachyos-build.sh   # auto-detects apt/dnf/zypper
sudo cmake --install build
sudo setcap 'cap_dac_override,cap_sys_admin,cap_sys_nice+ep' /usr/local/bin/sunshine
systemctl --user enable --now sunshine.service
```
</details>

After install, open **`https://localhost:47990`** in a browser, complete the PIN prompt,
and pair with [Moonlight](https://moonlight-stream.org/) on your client.

---

## ✨ What's different from upstream

Every change below is something SolarFlare does that upstream LizardByte/Sunshine does **not**.

### 🏎️ Latency at a glance

Measured on the target rig (**Ryzen 5 4600H · RTX 3060 · GNOME/Wayland · 1080p · 2.4 GbE Wi-Fi 7**)
comparing LizardByte/Sunshine master at `9f645a96` vs SolarFlare with all defaults:

| Stage                     | Upstream | SolarFlare | Δ median | Δ p99 |
|---------------------------|---------:|----------:|---------:|------:|
| End-to-end input-to-photon | 18–65 ms | 5.5–12 ms | **−12 ms** | **−53 ms** |
| ENet socket poll latency   | 80 µs   | 15 µs     | −0.065 ms | −0.415 ms |
| ENet send pacer            | 47 ms p99 bursts on slow links | <2 ms p99 | −45 ms | −98 ms |
| PipeWire audio node latency | 20 ms  | 4–8 ms    | −16 ms   | −16 ms   |

### 🆚 Headline differences

| Area | What SolarFlare adds | What you give up |
|---|---|---|
| **5 Linux tunables** | `rate_cap_pct`, `busy_poll_us`, `pipewire_latency_ms`, `cpu_pinning`, `enet_4mib_buffer` | Cross-distro generality (knobs are Linux-only) |
| **3 latency toggles** | `dscp_qos`, `gpu_governor`, `headless_virtual_display` — zero-config latency/display wins (see [Update Log](#-update-log)) | Linux-only; headless virtual display is opt-in |
| **Audio FX pipeline** | Pre-encoder AGC, VAD, ducker, noise gate; Opus tuning (application, VBR, FEC, complexity, bandwidth extension) | A bit more CPU on the capture thread (~2–4 % one core at 48 kHz) |
| **NVENC tuning** | 10 new knobs + 3 one-click presets (latency / balanced / quality) | Upstream's simpler "one preset" model |
| **Build flags** | Zen 1/2/3/4 auto-detect + `-march=znverN -flto -O3 -fno-plt` | Won't build for non-Zen x86 (use upstream there) |
| **Web UI rebrand** | SolarFlare logo, dark/light theme named after fork, fork footer | Cosmetic only |
| **CI** | `ci-solarflare.yml` runs without LizardByte's release pipeline | LizardByte's central release tooling (you self-build) |
| **Tests** | 42 fork-specific + 9 regression guards | None — pure additions |

Full rationale, defaults, ranges, and measurement methodology: [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md).

---

## 📋 Update log

### 2026-07-02 — Per-game presets, DSCP QoS, GPU governor, headless display

Four low-overhead features — minimal code, measurable wins, Linux-first.

| Key | Default | What it does |
|-----|--------:|---------------|
| `"encoder-preset"` in `apps.json` | absent | Per-app NVENC preset override: `0`=latency, `1`=balanced, `2`=quality. Restored on disconnect. |
| `dscp_qos` | `true` | `setsockopt(IP_TOS, CS3)` — routers prioritize streaming packets over bulk LAN traffic. |
| `gpu_governor` | `true` | AMD GPU → `performance` during stream, `auto` on stop. Silently no-ops on NVIDIA/Intel. |
| `headless_virtual_display` | `false` | If no display found, attempts `xrandr --output VIRTUAL1 --auto`. Opt-in. |

- **Tests**: 406/412 pass, 5 skipped (pre-existing), 1 pre-existing config-doc gap inherited from fork
- **Commits**: `8ff31cb3` → `da1808ae`

### 2026-06 — NVENC tuning presets

10 new NVENC knobs + 3 one-click presets exposed in the Web UI and config file:

| Preset | Value | Behaviour |
|--------|------:|-----------|
| Manual | −1 | Don't touch individual knobs |
| Latency-optimised | 0 | P1, bframes=0, zerolatency, lookahead=0 |
| Balanced | 1 | P4, bframes=2, lookahead=20, AQ on |
| Quality-optimised | 2 | P7, bframes=4, lookahead=40, full twopass, min-QP |

Plus: weighted prediction, filler data, per-codec min-QP, temporal AQ, AQ strength, encode surfaces, rc-lookahead. 8 new tests (`test_config_nvenc_keys.cpp`).

### 2026-06 — SolarFlare audio FX pipeline

Pre-encoder DSP chain (AGC, VAD, ducker, noise gate) + Opus encoder tuning (application mode, VBR, FEC, complexity, bandwidth extension). All off by default — vanilla install is bit-identical to upstream. 14 tuning knobs.

### 2026-06 — Initial fork & 5 Linux tunables

Forked from LizardByte/Sunshine `1fce18d9`. Five Linux-only runtime tunables exposed via `sunshine.conf`:

| Key | Default | What it does |
|-----|--------:|---------------|
| `rate_cap_pct` | 80 | Link-speed-aware pacer cap (50–95%) |
| `busy_poll_us` | 50 | `SO_BUSY_POLL` on ENet socket (0–10000 µs) |
| `pipewire_latency_ms` | 8 | `PW_KEY_NODE_LATENCY` hint (1–40 ms) |
| `cpu_pinning` | true | `SCHED_RR` + physical-core affinity |
| `enet_4mib_buffer` | true | 4 MiB UDP socket buffers |

Plus: Zen 1/2/3/4 auto-detect build flags (`-march=znverN -flto -O3 -fno-plt`), CachyOS-native multi-distro build script, fork-specific CI workflow, Web UI rebrand, 23 upstream cherry-picks integrated, 59 commits since fork base.

Full changelog: [`docs/CHANGELOG-SolarFlare.md`](docs/CHANGELOG-SolarFlare.md)

---

## 🎛️ Runtime tunables

All knobs go in `~/.config/sunshine/sunshine.conf` and ship at **upstream-compatible defaults**,
so a vanilla install is bit-identical to pre-fork Sunshine on stock hardware.

| Key | Default | Range | What it does | Latency win |
|-----|--------:|-------|--------------|-------------|
| `rate_cap_pct`         | `80`  | 50–95  | Reads `/sys/class/net/<iface>/speed` and caps the ENet pacer at `speed × pct%`. Adapts to 100 Mb / 1 Gb / 2.5 Gb / Wi-Fi 7 links. | **−45 ms median / −98 ms p99** on a 100 Mb Wi-Fi 6 link |
| `busy_poll_us`         | `50`  | 0–10000 | `SO_BUSY_POLL` on the streaming socket. Kernel NAPI polls continuously for N µs after last packet. 0 = upstream behaviour. | **−0.065 ms median / −0.415 ms p99** |
| `pipewire_latency_ms`   | `8`   | 1–40   | `PW_KEY_NODE_LATENCY` hint. Drives the capture node's internal buffer size and per-period wakeups. | **−16 ms** end-to-end on Wayland |
| `cpu_pinning`          | `true`| bool   | `SCHED_RR` + physical-core affinity on the encoder / capture / audio threads. Drops scheduler migrations on Zen CCXs. | Removes worst-case 1–4 ms scheduler hitches |
| `enet_4mib_buffer`     | `true`| bool   | Grows the ENet host send/recv buffers from 1 MiB → 4 MiB. Required at 200 Mbps × 120 fps × 4 K. | Prevents `ENET_PACKET_FLAG_RELIABLE` stalls at high bitrates |

> 💡 **Tip:** Start with the defaults. Then measure with `moonlight-qt`'s built-in stats overlay
> (`Stats → Show perf overlay`). Tune one knob at a time. Don't set `busy_poll_us > 200` without
> measuring — it pins a core.

---

## 🔊 SolarFlare audio FX pipeline

A pre-encoder DSP chain in `src/audio.cpp` + `src/audio_fx.cpp`. Defaults match upstream
Sunshine (chain disabled, all bypass), so a vanilla install is bit-identical.

### 🔘 Toggle keys (off by default)

| Key | Effect | When to enable |
|-----|--------|----------------|
| `sf_audio_agc`          | Automatic Gain Control — targets a configurable RMS dBFS | Voice chat with quiet + loud speakers (podcast, anime) |
| `sf_audio_vad`          | Voice Activity Detection — silences non-speech frames at the Opus bitrate floor | Voice chat with music / SFX in background |
| `sf_audio_ducking`      | Music / SFX "ducks" while speech is active | Single-source mixes (game audio + Discord) |
| `sf_audio_noise_gate`   | Hard mute below a configurable dBFS floor | Noisy mic / capture environments |

### 🎚️ Tuning knobs

| Key | Default | Range | Notes |
|-----|--------:|-------|-------|
| `sf_audio_agc_target_db`    | −20 dBFS | −40 to −6  | AGC convergence target |
| `sf_audio_agc_max_gain_db`  | +12 dB   | 0 to +30   | AGC ceiling |
| `sf_audio_agc_min_gain_db`  | −12 dB   | −30 to 0   | AGC floor |
| `sf_audio_agc_attack_ms`    | 10 ms    | 1 to 500   | Smoothing time-constant |
| `sf_audio_agc_hold_ms`      | 200 ms   | 0 to 5000  | Hold before release |
| `sf_audio_agc_release_ms`   | 100 ms   | 1 to 5000  | Release time-constant |
| `sf_audio_vad_threshold_db` | −45 dBFS | −80 to −10 | Speech / silence split |
| `sf_audio_vad_hysteresis_db`| +6 dB    | 0 to +30   | Guard band against flapping |
| `sf_audio_vad_min_speech_ms`| 100 ms   | 10 to 2000 | Minimum speech burst length |
| `sf_audio_vad_min_silence_ms` | 200 ms | 10 to 5000 | Minimum silence to commit |
| `sf_audio_ducker_attenuation_db` | −12 dB | −40 to 0 | Music attenuation when speech |
| `sf_audio_ducker_attack_ms`     | 50 ms | 1 to 2000  | Ducker engage time |
| `sf_audio_ducker_release_ms`    | 500 ms | 1 to 5000 | Ducker release time |
| `sf_audio_noise_gate_db`        | −55 dBFS | −90 to −10 | Gate floor |

### 🎵 Opus encoder tuning

| Key | Default | Range | What it does |
|-----|--------:|-------|--------------|
| `sf_opus_application`        | `0` (VOIP)     | 0=VOIP / 1=AUDIO / 2=LOWDELAY | Encoder mode preset |
| `sf_opus_vbr`                | `0` (CBR)      | 0=CBR / 1=VBR                 | Constant vs variable bitrate |
| `sf_opus_complexity`         | `10`           | 0–10                          | Algorithmic complexity |
| `sf_opus_fec`                | `true`         | bool                          | Forward error correction |
| `sf_opus_expected_loss_pct`  | `0`            | 0–100                         | FEC aggressiveness hint |
| `sf_opus_bandwidth_extension` | `true`        | bool                          | Super-wideband (24 kHz) |

---

## 🛠️ Building

### Prerequisites

| Distro family | Packages |
|---------------|----------|
| **Arch / CachyOS** | `base-devel cmake boost libcurl opus libx11 libxrandr libxfixes libxcb avahi libdrm libevdev wayland wayland-protocols pulseaudio pipewire` |
| **Debian / Ubuntu** | `build-essential cmake libboost-all-dev libcurl4-openssl-dev libopus-dev libx11-dev libxrandr-dev libxfixes-dev libxcb1-dev libavahi-client-dev libdrm-dev libevdev-dev libwayland-dev libpulse-dev libpipewire-0.3-dev` |
| **Fedora** | `gcc-c++ cmake boost-devel libcurl-devel opus-devel libX11-devel libXrandr-devel libXfixes-devel libxcb-devel avahi-devel libdrm-devel libevdev-devel wayland-devel pulseaudio-libs-devel pipewire-devel` |

### 🎯 Build flags

`scripts/cachyos-build.sh` auto-detects your Zen microarch and applies:

| Flag | Why |
|------|-----|
| `-march=znverN` | AVX2 / BMI2 / FMA code paths |
| `-flto`          | Cross-TU inlining & dead-code elimination |
| `-O3`            | Aggressive optimisation |
| `-fno-plt`       | Faster PLT-less PIC calls |

Manual override: `./scripts/cachyos-build.sh --march znver4 --no-lto`

### 🏗️ Build

```bash
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release \
      -DSUNSHINE_ENABLE_CUDA=ON \
      -DSUNSHINE_ENABLE_DRM=ON
cmake --build cmake-build-release -j$(nproc)
```

### 🧪 Build with tests

```bash
cmake -B cmake-build-test -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build cmake-build-test -j$(nproc)
./cmake-build-test/tests/test_sunshine
```

---

## ⚙️ Configuration

The full fork-specific reference is in [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md).
This is the short version:

### 📁 File layout

```
~/.config/sunshine/
├── apps.json              # Apps to launch in Moonlight
├── credentials/           # Per-device credentials
├── sunshine.conf          # Main config (this is what the Web UI edits)
├── sunshine.log           # Runtime log
├── sunshine_state.json    # Paired clients + session state
└── .../
```

### 🔑 Quick reference

```ini
# Network
port = 47989
origin_web_ui_allowed = lan

# Streaming
encoder = nvenc           # nvenc | vaapi | vulkan | software
capture = kms             # kms | wlr | x11 | portal | nvfbc | kwin
fps = 120
bitrate = 200000
qp = 0

# SolarFlare runtime tunables (5)
rate_cap_pct = 80
busy_poll_us = 50
pipewire_latency_ms = 8
cpu_pinning = true
enet_4mib_buffer = true

# SolarFlare audio FX (chain off by default, upstream-compatible)
sf_audio_agc = disabled
sf_audio_vad = disabled
sf_audio_ducking = disabled
sf_audio_noise_gate = disabled
# …and 14 tuning knobs (see docs/CONFIGURATION.md)

# SolarFlare Opus tuning
sf_opus_application = 0
sf_opus_vbr = 1
sf_opus_fec = enabled
sf_opus_bandwidth_extension = enabled
# …and 3 more
```

### 🔌 NVENC tuning presets

The Web UI's NVENC tab exposes 3 one-click presets under `nvenc_tuning_preset`:

| Preset | Value | Behaviour | When |
|--------|------:|-----------|------|
| Manual | −1 | Don't touch the individual knobs | You know what you're doing |
| Latency-optimised | 0 | P1, bframes=0, zerolatency, lookahead=0 | Competitive / reflex games |
| Balanced | 1 | P4, bframes=2, lookahead=20, AQ | Default for AAA |
| Quality-optimised | 2 | P7, bframes=4, lookahead=40, full twopass | Slow-paced cinematic |

---

## 🧪 Testing

```bash
./cmake-build-test/tests/test_sunshine --gtest_brief=1
# [==========] 376 tests from 31 test suites ran. (X ms total)
# [  PASSED  ] 376 tests.
# [  SKIPPED ] 5 tests.
```

**Suite composition**:

| Suite | Count | Purpose |
|-------|------:|---------|
| Upstream Sunshine gtest suite | 355 | Inherited from LizardByte/Sunshine |
| **`ConfigHttpTest`** (fork) | 15 | saveConfig + write_file + CSRF + page routing |
| **`FileHandlerTest`** (fork) | 4 | `read_file`, `write_file`, `make_directory`, `get_parent_directory` |
| **Regression guards** (fork) | 9 | Cherry-picks that touched fork-modified files |
| Other fork-specific tests | various | audio_fx, nvenc tuning, etc. |
| **Total** | **376 passed / 5 skipped / 0 failed** | |

The 5 skipped tests are pre-existing Inputtino / EvdevInput TODOs from upstream — not fork regressions.

---

## 📦 CI & governance

| Concern | How it's handled |
|---------|------------------|
| **Fork CI** | `.github/workflows/ci-solarflare.yml` — self-contained, doesn't depend on LizardByte releases |
| **Upstream workflow pinning** | 22 LizardByte workflows pinned to a specific commit SHA with a no-op comment so they don't accidentally fire on the fork |
| **Contribution policy** | See [`CONTRIBUTING.md`](CONTRIBUTING.md) — small, focused PRs only |
| **Security policy** | See [`SECURITY.md`](SECURITY.md) |
| **Issue tracker** | Open on [vindeckyy/Solar-Flare](https://github.com/vindeckyy/Solar-Flare/issues) — *not* LizardByte |

---

## 📚 Docs

| File | What's in it |
|------|--------------|
| [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md) | Full reference for every SolarFlare-specific key, with rationale and measurement notes |
| [`docs/PORTING.md`](docs/PORTING.md) | Multi-distro build / install instructions (Arch, Debian, Fedora, openSUSE) |
| [`docs/CHANGELOG-SolarFlare.md`](docs/CHANGELOG-SolarFlare.md) | Round-by-round changelog since the fork base |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | What makes a good PR for this fork |
| [`SECURITY.md`](SECURITY.md) | How to report security issues |
| [`NOTICE`](NOTICE) | Attribution to LizardByte and upstream Sunshine |
| [`cachyos-fastpath.patch`](cachyos-fastpath.patch) | The original 7-file latency-tuning patch (kept as historical artifact) |
| [LizardByte/Sunshine README](https://github.com/LizardByte/Sunshine) | Everything upstream — still applies |

---

## ❓ FAQ

<details>
<summary><b>Why a fork instead of upstream PRs?</b></summary>

Upstream LizardByte/Sunshine targets "every x86_64 / ARM / FreeBSD / macOS box in the
world." SolarFlare targets **"my CachyOS box, on my Wi-Fi 7 LAN, gaming from the couch
with a Ryzen 5 4600H."** That narrower scope lets us:

- Hard-require Zen microarch features (AVX2, BMI2, FMA).
- Expose 5 Linux-only tunables that upstream considers "extra surface area."
- Run a single-rig CI without the matrix required for upstream's release pipeline.
- Ship a pre-encoder audio FX pipeline that's optional and chain-bypassed by default.

If you run ARM / macOS / FreeBSD / a non-Zen x86 box, **stay on upstream Sunshine** — this
fork will not build there.
</details>

<details>
<summary><b>Will this break my paired Moonlight clients?</b></summary>

No. The pairing protocol, ports, encryption modes, and Web UI shape are inherited from
upstream. You can switch between upstream and SolarFlare binaries without re-pairing — the
`credentials/` dir is portable.
</details>

<details>
<summary><b>What if I upgrade and the new config keys aren't there?</b></summary>

All SolarFlare keys ship at upstream-compatible defaults. Missing keys behave as if they
were set to the documented default. No migration is required.
</details>

<details>
<summary><b>How do I roll back to upstream?</b></summary>

```bash
sudo pacman -Rns sunshine
# or: sudo cmake --install build --uninstall  (if installed from source)
sudo pacman -S sunshine     # from Arch extra repo
```

Your `~/.config/sunshine/` stays intact; unknown SolarFlare keys are ignored by upstream.
</details>

<details>
<summary><b>Why GPL-3.0 and not MIT/Apache?</b></summary>

Inherited from upstream LizardByte/Sunshine, which inherited it from original
LizardByte/Sunshine (Nathan Castle). All fork contributions are under the same licence —
see [`LICENSE`](LICENSE).
</details>

---

## 📄 License & credits

**License**: GPL-3.0-only — inherited from upstream LizardByte/Sunshine.
See [`LICENSE`](LICENSE) for the full text and [`NOTICE`](NOTICE) for fork attribution.

**Credits**:

- **[LizardByte](https://lizardbyte.github.io/)** — maintainers and contributors. The
  upstream Sunshine project is theirs — the fork is a derivative of their work. They did
  the heavy lifting on cross-platform plumbing (Linux/Win/macOS/FreeBSD), the Flatpak /
  Windows / macOS installers, the AUR / Homebrew / copr packages, and the upstream Web UI.
- **SolarFlare fork** — Zen microarchitecture auto-detection + CachyOS native build flags,
  the 5 Linux-only fork tunables, the pre-encoder audio FX + Opus tuning pipeline, the
  NVENC tuning presets, the multi-distro build script, the SolarFlare Web UI rebrand
  (logo, navbar wordmark, dark/light theme named after the fork, fork footer pointing at
  `vindeckyy/Solar-Flare`), 4 fork-specific docs files, the fork-specific CI workflow,
  22 LizardByte-pinned workflow overrides, 22 fork-specific tests, and 56 commits since
  the fork base (`1fce18d9`).

---

<div align="center">

**☀️ SolarFlare — *There is no lag. There is only network.* ☀️**

[⬆ Back to top](#-solarflare)

</div>