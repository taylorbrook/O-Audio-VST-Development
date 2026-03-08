# Gain Staging Plugin Market Research

**Date:** 2026-03-07
**Purpose:** Comprehensive market analysis for O-Gain plugin development

---

## 1. Popular Gain Staging Plugins — Complete Landscape

The gain staging plugin market falls into four distinct categories:

### Category A: Static Gain / Trim Utilities

These plugins provide manual gain adjustment with metering — the digital equivalent of a console trim knob. They are inserted first in the chain (or between plugins) to set levels.

| Plugin | Developer | Price | Gain Range | Metering | Phase Inv | M/S | Channels | Notes |
|--------|-----------|-------|------------|----------|-----------|-----|----------|-------|
| **Klanghelm VUMT** | Klanghelm | 14 EUR | Trim knob | VU, PPM, K-12/14/20 RMS | No (std) | No (std) | Stereo | Best-in-class VU. Deluxe adds polarity, mono-maker, HP/LP, DynEQ |
| **Klanghelm VUMT Deluxe** | Klanghelm | 22 EUR | Trim knob | VU, PPM, BBC, DIN, Nordic, K-scales | Yes | Yes (mid/side trim) | Stereo | Full channel tool. The gold standard for VU-based gain staging |
| **Blue Cat Gain Suite** | Blue Cat Audio | Free | Not specified | Peak | Yes (stereo) | Yes (mid/side mode) | Mono + Stereo | MIDI learn, multi-instance linking, zero CPU on idle |
| **Sonalksis FreeG** | Sonalksis | Free | Fader-style | Peak + RMS overlay | Yes | No | Mono + Stereo | Pan control, mute, fine mode. Elegant but discontinued-feel |
| **Channel Robot Gainer** | Channel Robot | Free | -100 to +24 dB | Input/Output level | Yes | No | Stereo | Also includes inverted gate/compressor for noise reduction |
| **TBProAudio TBPAVolume** | TBProAudio | Free | +/- 48 dB | Input/Output meters | No | No | Stereo | Click-free 64-bit processing, sample-accurate automation (VST3/CLAP). Released May 2025 |
| **Airwindows PurestGain** | Airwindows | Free (Patreon) | dB control + fade | None | No | No | Stereo | Long-double precision processing, noise-shaped output. Zero-compromise audio quality |
| **Airwindows BitShiftGain** | Airwindows | Free (Patreon) | 6 dB steps only | None | No | No | Stereo | Bit-perfect gain in 6 dB increments. Mathematically lossless |
| **Dotec-Audio DeeGain** | Dotec-Audio | Free | -20 to +20 dB | None | No | No | Stereo | Ultra-simple trim control |

### Category B: VU Meter + Gain Staging Combos (Measure + Trim)

These combine metering and trim in one plugin. They are the primary "gain staging" tools used by mixing engineers.

| Plugin | Developer | Price | Auto-Gain | Metering | Calibration | Grouping | Key Differentiator |
|--------|-----------|-------|-----------|----------|-------------|----------|-------------------|
| **Klanghelm VUMT / Deluxe** | Klanghelm | 14-22 EUR | No | VU, PPM, BBC, DIN, Nordic, K-scales | 0VU = configurable dBFS | No | Analog-modeled VU ballistics with overshoot parameter |
| **HoRNet VU Meter MK4** | HoRNet | 5.99 EUR | **Yes** — auto-sets gain to hit target VU level | VU with peak hold | 0VU = configurable (default -18 dBFS) | **Yes** — group instances, sync settings | Auto-gain to target VU + max peak ceiling. Can auto-stage an entire session via groups |
| **Waves VU Meter** | Waves | Free | No | VU | Adjustable headroom | No | Industry name recognition. No trim control (users request this) |
| **GainStage Pro** | Hertz Instruments | $19 (often free promos) | **Metering only** — no gain control | Algorithmic signal analysis | Internal | No | Visual "too hot / perfect / too cold" indicator. Zero-latency. Does NOT process audio, only measures |

### Category C: Auto-Gain / Normalization (Analyze + Set)

These plugins measure audio and automatically apply gain to reach a target level. This is the "learn and set" category.

| Plugin | Developer | Price | Target Modes | Learn/Analyze | Continuous vs Static | Latency | Key Differentiator |
|--------|-----------|-------|-------------|---------------|---------------------|---------|-------------------|
| **HoRNet TheNormalizer** | HoRNet | 9.99 EUR | dBFS, VU, RMS, LUFS (integrated/short-term/momentary) | **Yes** — analyzes then sets gain | Both: peak (static) + continuous modes | Minimal once set | Most flexible target modes. Group function for batch normalization. Very low CPU after analysis |
| **iZotope Relay** (within Neutron ecosystem) | iZotope | Included with Neutron/Music Production Suite | Machine-learning based | **Yes** — ML analyzes all tracks with Relay and sets trim gain based on "Focus" | Static (set once via Mix Assistant) | Low | AI/ML-driven. Requires iZotope ecosystem. Sets gain staging for entire session at once |
| **LetiMix GainMatch** | LetiMix | $19 ($9 on sale) | RMS or Peak target | **Yes** — measures before/after for A/B matching, or matches to target | Static (A/B comparison) | Auto delay compensation | Primarily designed for A/B loudness-matched plugin comparisons. Secondary use as target level matcher |

### Category D: Continuous Gain Riders (Real-Time Level Automation)

These continuously ride gain in real-time, like an engineer riding a fader. They are NOT "set once" tools — they dynamically adjust throughout playback.

| Plugin | Developer | Price | Detection | Sidechain | Automation Write | Max Range | Key Differentiator |
|--------|-----------|-------|-----------|-----------|-----------------|-----------|-------------------|
| **Waves Vocal Rider** | Waves | ~$29-35 | RMS-based | Yes (music bus input) | Yes (writes to DAW) | +/- 12 dB | Industry standard for vocal riding. Sidechain allows vocal to track music level |
| **HoRNet AutoGain Pro MK2** | HoRNet | ~15 EUR | RMS/Peak, internal or external reference | Yes | **Yes** — writes gain automation to DAW host | Configurable min/max | Two detectors + two processors. Can write automation then switch to read mode |
| **TBProAudio GainRider 3** | TBProAudio | ~39 EUR | Loudness-based | Yes (VST3/AAX/AU) | Yes | Configurable min/max/idle | 3 ride modes: leveling, ducking, sidechain leveling. Flexible pre-delay with zero-delay option |
| **Melda MAutoVolume** | Melda | $71 | 4 modes: Simple, Target, Loudness, Perceptual | Yes | Via automation | Configurable | Most processing modes. Sidechain filtering, transient processing. Up to 16 channels surround |
| **W.A. Production Outlaw** | W.A. Production | $39.90 (Lite: free) | RMS target | No (full) / No (lite) | No | Configurable max up/down | HPF/LPF/Slope on detection signal. Lite version is very simple: Gate, Target, Mix, Gain |
| **Quiet Art WaveRider Tg** | Quiet Art | $59-129 | Level-based | No | **Yes** — writes to Pro Tools volume lane | Configurable | Ride/duck/park/trace modes. Originally Pro Tools only; Tg version supports all formats |

### Category E: Loudness Metering (Measure Only, No Gain)

These are pure metering tools — critical for understanding levels but they do not apply gain.

| Plugin | Developer | Price | LUFS | RMS | True Peak | History/Graph | Key Differentiator |
|--------|-----------|-------|------|-----|-----------|---------------|-------------------|
| **Youlean Loudness Meter** | Youlean | Free (Pro: $49) | Yes (Momentary/Short/Integrated) | No | Yes | Yes (loudness histogram) | Streaming platform presets (Spotify, YouTube, Apple, etc.). KVR 2016 winner |
| **MeterPlugs LCAST** | MeterPlugs | ~$79 (stereo) | Yes | No | Yes | Yes (24hr history) | Broadcast standards: ATSC A/85, ITU BS.1770, EBU R128. CSV export (surround version) |
| **TBProAudio dpMeter5** | TBProAudio | Free | Yes (EBU R128) | Yes | Yes (ITU BS.1770-4) | No | RMS + EBU R128 + Dialog-gated modes. Up to 5.1 surround. Click-free 64-bit |

---

## 2. Feature Comparison Matrix

### Core Features Across All Categories

| Feature | VUMT Dlx | HoRNet VU MK4 | TheNormalizer | PurestGain | TBPAVolume | Gainer | GainMatch | Blue Cat |
|---------|----------|---------------|---------------|------------|------------|--------|-----------|----------|
| **Manual Gain** | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Yes |
| **Gain Range** | Trim knob | Trim knob | Auto-set | dB + fade | +/- 48 dB | -100/+24 dB | Auto + manual | Not specified |
| **Auto-Gain to Target** | No | **Yes (VU)** | **Yes (multi)** | No | No | No | **Yes (RMS/Peak)** | No |
| **Learn/Analyze Mode** | No | **Yes** | **Yes** | No | No | No | **Yes** | No |
| **VU Metering** | Yes | Yes | Target mode | No | No | No | No | No |
| **RMS Metering** | Yes (K-scales) | No | Target mode | No | No | No | Yes | No |
| **LUFS Metering** | No | No | **Yes (all 3)** | No | No | No | No | No |
| **Peak Metering** | Yes | Yes (hold) | Yes | No | Yes | Yes | Yes | Yes |
| **True Peak** | No | No | No | No | No | No | No | No |
| **Phase Inversion** | Yes (Dlx) | No | No | No | No | Yes | No | Yes |
| **M/S Processing** | Yes (Dlx) | No | No | No | No | No | Yes | Yes |
| **Channel Swap** | No | No | No | No | No | No | No | No |
| **Mono/Stereo** | Stereo | Stereo | Stereo | Stereo | Stereo | Stereo | Stereo | Both |
| **Grouping** | No | **Yes** | **Yes** | No | No | No | No | **Yes (link)** |
| **Zero Latency** | Yes | Yes | Near-zero | Yes | Yes | Yes | No (delay comp) | Yes |
| **Price** | 22 EUR | 5.99 EUR | 9.99 EUR | Free | Free | Free | $19 | Free |

### Key Observations from the Matrix

1. **No single plugin combines all of these:** auto-learn to target level + VU/RMS/LUFS metering + phase inversion + M/S + manual trim in one clean interface.

2. **HoRNet VU Meter MK4** comes closest to a "learn and set" VU-based gain stager at an absurdly low price (5.99 EUR), but it only targets VU levels, not RMS or LUFS.

3. **HoRNet TheNormalizer** has the most flexible target modes (dBFS, VU, RMS, LUFS) but lacks utility features like phase inversion or M/S.

4. **Klanghelm VUMT Deluxe** is the most feature-rich single plugin (VU + trim + phase + M/S + HP/LP + DynEQ) but has NO auto-gain.

5. **No plugin offers a clean "analyze for N seconds, then set static gain" workflow** that targets a specific RMS/LUFS/VU with a simple one-button learn. TheNormalizer is closest but its interface is more focused on normalization than "gain staging as a workflow."

---

## 3. Gaps in the Market — What O-Gain Could Do Better

### Gap 1: The "Analyze Once, Set Static Gain" Workflow is Underserved

**The problem:** Most auto-gain plugins fall into two camps:
- **Continuous riders** (Vocal Rider, AutoGain Pro, MAutoVolume) that constantly adjust gain in real-time. These are great for vocals but are NOT what mixing engineers want for basic gain staging at the top of a chain.
- **Normalizers** (TheNormalizer) that can set a static level but are conceptually framed as "normalization" tools, not gain-staging workflow tools.

**What users actually want** (from KVR/Gearspace forum analysis):
> "A plugin that would detect the maximum peak of the channel when in 'capture'-mode and by hitting an 'adjust'-button would raise or lower the gain."

**The opportunity for O-Gain:** A clean, purpose-built "learn and set" gain staging plugin:
1. Press "Learn" (or it auto-starts on playback)
2. Play audio for a few seconds / bars
3. Plugin measures the level (user chooses: Peak, RMS, VU, or LUFS)
4. Plugin calculates and applies a static gain offset to hit the target level (e.g., -18 dBFS RMS, 0 VU, -23 LUFS)
5. Done. The learn mode disengages. The gain is now a fixed static value.
6. User can fine-tune manually if desired.

No existing plugin does this with a truly clean, single-purpose UX.

### Gap 2: Combined Metering + Trim + Auto-Learn in a Compact UI

**The problem:** To get VU metering + trim + auto-gain today, you need:
- VUMT Deluxe (metering + trim, no auto) OR
- HoRNet VU MK4 (VU meter + auto-gain, but VU-only target) OR
- TheNormalizer (auto-gain + multi-target, but limited metering display)

**The opportunity for O-Gain:** One plugin that shows:
- A clear meter (switchable: Peak, RMS, VU, LUFS)
- A gain knob/fader
- A "Learn" button that measures and auto-sets the gain
- Target level selector (preset targets: -18 dBFS, -16 dBFS, 0 VU, -14 LUFS, custom)

### Gap 3: Phase Inversion + Channel Swap as Standard Utility Features

**The problem:** Phase inversion is scattered across plugins inconsistently. Channel swap (L/R flip) is almost never included. Mid/Side encoding is only in the more expensive tools.

**The opportunity for O-Gain:** Include as standard:
- Per-channel phase inversion (L and R independently)
- Channel swap (L<->R)
- Mono summing option
- M/S encoding/decoding mode
- These are "always-need" utilities that engineers want accessible on every channel

### Gap 4: No Clean "Gain Staging Dashboard" Concept

**The problem:** Engineers must open each instance of their gain plugin individually to check/adjust levels. HoRNet VU MK4 has grouping but limited remote monitoring.

**The opportunity for O-Gain:** Consider a linked-instance overview mode where you can see all instances across the session at a glance (levels, gain offsets). This is a "nice to have" differentiator rather than a core feature.

### Gap 5: True Peak Awareness in a Gain Staging Context

**The problem:** Most gain staging tools use simple peak or RMS measurement. True peak (inter-sample peak) detection is only found in loudness meters (Youlean, LCAST, dpMeter5), not in gain-setting tools. When auto-setting gain, you could overshoot true peak even if digital peak looks fine.

**The opportunity for O-Gain:** Include a true peak ceiling option during auto-learn. When the plugin calculates gain to hit a target RMS/LUFS, it also checks that the resulting true peak does not exceed a user-defined ceiling (e.g., -1 dBTP).

### Gap 6: Modern Format Support

**The problem:** Many older plugins still lack CLAP support. Some (Sonalksis FreeG, original WaveRider) are VST2/RTAS only.

**The opportunity for O-Gain:** Ship day one with VST3, AU, AAX, and optionally CLAP. Full Apple Silicon native. Full Windows support.

---

## 4. User Workflows — How Engineers Actually Use Gain Staging Plugins

### Workflow 1: Pre-Mix Gain Staging (Most Common)

**When:** Before any mixing begins, after tracking/editing is complete.
**Where in chain:** First insert on every channel, before any processing.
**Goal:** Get all tracks to a consistent "0 VU" or "-18 dBFS" starting point so that:
- Analog-modeled plugins (compressors, EQs, saturators) receive signal at their intended operating level
- Faders all start near unity (0 dB)
- Headroom is maintained on the mix bus

**Typical workflow:**
1. Insert a VU meter or gain plugin on every track
2. Play through the loudest section of each track
3. Adjust trim until the meter reads approximately 0 VU (= -18 dBFS) or the engineer's preferred reference
4. Remove or bypass the meter (some leave it for ongoing reference)

**Target levels commonly used:**
- **-18 dBFS RMS / 0 VU** — The most widely recommended reference. This is where most analog-modeled plugins have their "sweet spot." Recommended by Bob Katz, used by most professional mixing engineers.
- **-20 dBFS** — Used by some engineers who want extra headroom. Common in film/post-production.
- **-16 dBFS** — Used by some EDM/pop producers who prefer to push levels hotter into saturation plugins.
- **-14 dBFS** — Uncommon for individual tracks but sometimes used for louder genres.

### Workflow 2: Inter-Plugin Gain Staging

**When:** During mixing, as plugins are added to the chain.
**Where in chain:** Between plugins (e.g., after a compressor that added 6 dB of makeup gain, before an EQ).
**Goal:** Ensure each plugin in the chain receives signal at the intended level.

**Typical workflow:**
1. Add a compressor, adjust settings
2. Notice output is now +6 dB hotter than input
3. Insert a trim plugin after the compressor (or use the compressor's output gain)
4. Match output level to input level
5. Continue to next plugin

**This is where LetiMix GainMatch excels** — it specifically measures before/after levels for plugin A/B comparison.

### Workflow 3: Loudness Targeting for Delivery

**When:** At the mastering stage or final mix check.
**Where in chain:** On the master bus, after all processing.
**Goal:** Hit a specific loudness target for streaming platforms.

**Target levels:**
- **Spotify:** -14 LUFS integrated
- **Apple Music:** -16 LUFS integrated (with -1 dBTP ceiling)
- **YouTube:** -14 LUFS integrated
- **Broadcast (EBU R128):** -23 LUFS integrated
- **Broadcast (ATSC A/85):** -24 LKFS integrated

**This is metering territory** (Youlean, LCAST) — not really a gain-staging plugin's job, but there is overlap.

### Workflow 4: Recording Level Optimization

**When:** During tracking, before recording.
**Where in chain:** On the input channel, monitoring incoming signal.
**Goal:** Ensure recording levels are optimal — not too hot (clipping), not too cold (noisy).

**This is where GainStage Pro lives** — a pure measurement tool that tells you "too hot / just right / too cold."

### Engineer Preferences: VU vs RMS vs LUFS for Gain Staging

Based on forum discussions and professional recommendations:

| Context | Preferred Metering | Typical Target | Why |
|---------|-------------------|----------------|-----|
| **Pre-mix gain staging** | **VU** | 0 VU (= -18 dBFS) | VU ballistics match human loudness perception. Engineers are trained on VU. Analog plugins calibrated to 0 VU. |
| **Inter-plugin matching** | **RMS or Peak** | Match in = out | Quick visual comparison. Peak catches transients VU misses. |
| **Vocal/instrument riding** | **RMS (continuous)** | Varies | RMS tracks perceived loudness in real-time for dynamic sources. |
| **Master bus targeting** | **LUFS** | -14 to -16 LUFS | Industry standard for streaming delivery. Frequency-weighted = more accurate perceived loudness than RMS. |
| **Broadcast/post** | **LUFS** | -23 to -24 LUFS | Regulatory requirement (EBU R128, ATSC A/85). |

**Key insight:** For the primary gain staging use case (Workflow 1), **VU is still king** among professional mixing engineers. But younger engineers and producers increasingly think in LUFS. A gain staging plugin that supports both paradigms has the widest appeal.

---

## 5. Competitive Pricing Analysis

| Tier | Price Range | Examples | Notes |
|------|------------|----------|-------|
| **Free** | $0 | PurestGain, TBPAVolume, Gainer, Blue Cat Gain Suite, Sonalksis FreeG, DeeGain, dpMeter5 | Many solid options. Hard to compete on price alone. Must differentiate on features/UX |
| **Budget** | $5-15 | HoRNet VU MK4 (6 EUR), HoRNet TheNormalizer (10 EUR), Klanghelm VUMT (14 EUR) | Sweet spot for utility plugins. Impulse purchase territory |
| **Mid** | $19-40 | VUMT Deluxe (22 EUR), GainStage Pro ($19), GainMatch ($19), Outlaw ($40), Waves Vocal Rider ($29-35) | Needs clear value proposition at this level |
| **Premium** | $50-150 | MAutoVolume ($71), WaveRider Tg ($59-129), LCAST ($79+) | Advanced feature sets, professional workflows |

**Recommended price positioning for O-Gain:** $15-25 range. This positions it:
- Above the HoRNet budget tier (justifiable if the feature set is more complete)
- Below MAutoVolume and WaveRider (different category — O-Gain is "set once" not "ride continuously")
- Competitive with VUMT Deluxe and GainMatch

---

## 6. Summary: The O-Gain Opportunity

The ideal O-Gain plugin would occupy a currently **empty niche**: a purpose-built gain staging utility that combines:

1. **Clean manual gain control** (+/- 48 dB or similar wide range, fine resolution)
2. **Multi-mode metering** in one view (switchable: Peak, RMS, VU, LUFS)
3. **One-button "Learn" mode** — analyze audio for N seconds, auto-calculate and apply static gain to hit a user-defined target level
4. **Flexible target presets** (-18 dBFS, -16 dBFS, 0 VU, -14 LUFS, -23 LUFS, custom)
5. **True peak ceiling** awareness during auto-learn
6. **Essential utilities**: per-channel phase inversion, L/R channel swap, mono sum, M/S mode
7. **Minimal CPU / zero latency** (or near-zero)
8. **Clean, compact UI** that stays out of the way on every channel

No single existing plugin offers all of these in one package. The closest competitors each cover only a subset:
- VUMT Deluxe = best metering + trim + utilities, but no auto-learn
- HoRNet VU MK4 = best auto-gain + VU, but VU-only and no utilities
- TheNormalizer = best auto-target flexibility, but no metering display or utilities
- GainMatch = good auto-matching, but designed for A/B comparison not gain staging workflow

**O-Gain's tagline opportunity:** "Measure. Learn. Set. Done."

---

## Sources

- [HoRNet TheNormalizer](https://www.hornetplugins.com/plugins/hornet-thenormalizer/)
- [HoRNet VU Meter MK4](https://www.hornetplugins.com/plugins/hornet-vu-meter-mk4/)
- [HoRNet AutoGain Pro MK2](https://www.hornetplugins.com/plugins/hornet-autogain-pro-mk2/)
- [Klanghelm VUMT / VUMT Deluxe](https://klanghelm.com/contents/products/VUMT)
- [GainStage Pro — Hertz Instruments](https://hertzinstruments.com/plugins/gainstage-pro/)
- [TBProAudio TBPAVolume](https://www.tbproaudio.de/products/tbpavolume)
- [TBProAudio GainRider 3](https://www.tbproaudio.de/products/gainrider)
- [TBProAudio dpMeter5](https://www.tbproaudio.de/products/dpmeter)
- [Airwindows PurestGain](https://www.airwindows.com/purestgain-vst/)
- [Blue Cat Gain Suite](https://www.bluecataudio.com/Products/Product_GainSuite/)
- [Channel Robot Gainer](https://channelrobot.com/product/gainer/)
- [Sonalksis FreeG](https://www.sonalksis.com/freeg.html)
- [Dotec-Audio DeeGain](https://dotec-audio.com/deegain.html)
- [LetiMix GainMatch](https://letimix.com/products/gainmatch)
- [Youlean Loudness Meter](https://youlean.co/youlean-loudness-meter/)
- [MeterPlugs LCAST](https://www.meterplugs.com/lcast)
- [Melda MAutoVolume](https://www.meldaproduction.com/MAutoVolume)
- [Waves Vocal Rider](https://www.waves.com/riding-vocals-with-vocal-rider)
- [W.A. Production Outlaw](https://www.pluginboutique.com/products/5059)
- [Quiet Art WaveRider Tg](https://www.kvraudio.com/product/waverider-tg-by-quiet-art)
- [iZotope Relay](https://www.izotope.com/en/products/mix/insight/features-and-comparison/relay.html)
- [TDR VOS SlickEQ (GainMatch feature)](https://www.tokyodawn.net/tdr-vos-slickeq/)
- [Gain Staging Best Practices — iZotope](https://www.izotope.com/en/learn/gain-staging-what-it-is-and-how-to-do-it)
- [Gain Staging Workflow — Waves](https://www.waves.com/gain-staging-in-your-daw-better-mix)
- [VU, RMS, and LUFS Guide](https://www.mixedbyma.com/post/digital-audio-v-vu-rms-and-lufs)
- [Loudness Standards — Sweetwater](https://www.sweetwater.com/insync/loudness-standards-lufs-peaks-and-streaming-limits/)
- [KVR Forum: Auto Gain Staging Discussion](https://www.kvraudio.com/forum/viewtopic.php?t=421278)
- [KVR Forum: Does This Type of Auto Gain Plugin Exist?](https://www.kvraudio.com/forum/viewtopic.php?t=454001)
- [Gearspace: Auto-Gainstaging Plugins](https://gearspace.com/board/music-computers/1421416-auto-gainstaging-plugins.html)
- [Gearspace: Best Gain Automation Plugins](https://gearspace.com/board/music-computers/1285294-best-gain-automation-plugs-vocal-rider-waverider-mautovoume-auto-gain-etc.html)
- [Gain Staging Explained 2026 — MixingMonster](https://mixingmonster.com/gain-staging/)
