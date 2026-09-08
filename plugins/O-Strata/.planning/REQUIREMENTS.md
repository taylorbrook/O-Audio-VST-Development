# O-Strata - Requirements

---
version: 2.0.0
plugin: O-Strata
created: 2026-09-07
lastUpdated: 2026-09-08
supersedes: superseded-baked-v1/REQUIREMENTS.md (v1.0.0, baked-geometry design)
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 28
**Coverage:** must: 19 | should: 7 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Each oscillator (A, B) is a live wave-terrain oscillator: a closed orbit at the note frequency scans a 2D terrain per sample; θ is the phase accumulator so Sync / Bend / Window warps, FM, unison and Phase apply unchanged | must | pending | stage-2 |
| FUNC-02 | Analytic terrain library ≥ 6 (Sine Product, Radial Rings, Saddle, Ridged Cosines, Mitsuhashi, Cosine Wells), each with Terrain Freq and two documented shape inputs (Mod X / Mod Y), clean-room formulas | must | pending | stage-2 |
| FUNC-03 | Orbit library ≥ 11 (Ellipse, Superellipse, Limaçon, Epitrochoid 3/5/7, Hypocycloid 3/5/7, Butterfly, Squarcle) with Size, Aspect, Rotation, Centre X/Y and a per-orbit shape input (Orbit Mod) | must | pending | stage-2 |
| FUNC-04 | Trajectory feedback: previous output displaces the next orbit point, with Feedback amount and Feedback Damp; bounded for every setting | must | pending | stage-2 |
| FUNC-05 | Audio-rate modulation: Orbit Size (= `osc?Pos`), Aspect, Rotation, Centre X, Centre Y, Orbit Mod, Terrain Freq, Terrain Mod X/Y, Feedback and Saturation are mod-matrix destinations (10 new per oscillator, appended after O-Prism's 26), smoothed per sample | must | pending | stage-2 |
| FUNC-06 | Per-oscillator Quality: Bandlimited (Chebyshev, 1×) / 2× / 4× | must | pending | stage-2 |
| FUNC-07 | User greyscale PNG terrain import via file chooser and drag-and-drop, with Image Blur and Edge Mode; decode and pre-blur off the audio thread | must | pending | stage-3 |
| FUNC-08 | Imported PNG persists in plugin state and presets (raw bytes ≤ 2 MB; path + SHA-256 above the cap); presets regenerate on load | must | pending | stage-4 |
| FUNC-09 | All O-Prism v1.24.0 non-oscillator sections (sub/noise, envelopes, dual filters, LFOs, mod matrix, FX rack, global) carry over unchanged | must | pending | stage-2 |
| FUNC-10 | Full microtonal tuning engine (scala-tuning-engine v3.0.1: factory tunings, Scala/KBM import, EDO / harmonic / rank-2 generators, tuning tab) | must | pending | stage-2 |
| FUNC-11 | Factory presets ≥ 15 covering terrains, orbits, feedback and Bandlimited mode, every one passing the symmetry gate (DSP-06) | should | pending | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Pitch-tracked terrain spatial frequency ("terrain mip"): with Pitch Track = 1 the harmonic count of a patch is constant across the keyboard; Pitch Track = 0 keeps the spatial frequency fixed | must | pending | stage-2 |
| DSP-02 | Bandlimited mode: terrain in the Chebyshev basis, total degree ≤ 16, coefficients truncated per pitch so the maximum harmonic (n+m)·K stays below Nyquist; coefficient sets computed off the audio thread and swapped atomically; PNG terrains projected onto the basis at import | must | pending | stage-2 |
| DSP-03 | Oversampling is per oscillator (halfband polyphase IIR 2×, 4× HQ), parameters held over sub-samples; the voice loop, filters, LFOs and mod matrix run at base rate | must | pending | stage-2 |
| DSP-04 | Orbit (including feedback displacement) clamped inside [−1,1]²; PNG terrains pre-blurred with Mirror / Window edge handling; every sampled value `isfinite`-guarded | must | pending | stage-2 |
| DSP-05 | No `pow`, `std::function` or allocation in the per-sample path; enum `switch` dispatch; fast tanh; `SmoothedValue` per modulated parameter; a Quality change never allocates on the audio thread | must | pending | stage-2 |
| DSP-06 | Symmetry rule: default orbit centre, aspect and every library / factory default produce harmonic 1 as the strongest partial or within 6 dB of it (a centred orbit over an even terrain plays an octave up) | must | pending | stage-2 |
| DSP-07 | Saturation applies tanh to the scanned value before the oscillator output; identity at 0 | should | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Per-oscillator 3D terrain view (displaced 64×64 wireframe + orbit + scan point + feedback trail) with a 30 Hz playhead, hand-rolled WebGL2 | should | pending | stage-3 |
| UI-02 | Canvas 2D fallback in the same view when WebGL2 is unavailable; localised placeholder in en/fr/zh-Hans | must | pending | stage-3 |
| UI-03 | Drag on the view edits Orbit Centre X/Y, wheel edits Orbit Size, alt-drag edits Rotation, through the slider relay with drag-start/drag-end | should | pending | stage-3 |
| UI-04 | 3D view and playhead re-push after every preset apply and session restore | must | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations, locks, file I/O or coefficient projection in processBlock) | must | pending | stage-2 |
| PERF-02 | CPU: 16 voices × 2 oscillators, unison 1, Quality 2×, default patch ≤ 12 % of one Apple M-series core; unison capped at 4 for the live oscillator | must | pending | stage-2 |
| PERF-03 | 3D view costs ≤ 2 ms per frame in WKWebView and WebView2 at DPR 2 | should | pending | stage-3 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) at strictness 10, and auval — re-verified after the re-parameterise pass (the baked fork passed on 2026-09-07) | must | pending | stage-1 |
| COMPAT-02 | Windows VST3 build via CI (WebView2, static linking) | must | pending | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | Aliasing: every library terrain × orbit at defaults measures ≤ −60 dB non-harmonic energy at C6 at Quality 2× with Pitch Track 1; ≤ −90 dB in Bandlimited mode with a trig-polynomial orbit | must | pending | stage-2 |
| QUAL-02 | No zipper noise when any mod destination of FUNC-05 is stepped by host automation or the mod matrix (sample-step detector on the rendered output) | must | pending | stage-2 |
| QUAL-03 | Loading a preset whose imported PNG is missing degrades to the library fallback with a visible notice, never silence or a crash | nice | pending | stage-4 |
| QUAL-04 | Listening pass on the factory presets and on a terrain × orbit grid rendered by the harness | nice | pending | stage-4 |

## Acceptance Criteria Details

### FUNC-01: Live oscillator

- [ ] Rendering C4 with Sine Product / Ellipse at defaults gives a periodic waveform at 261.6 Hz ± 1 cent whose spectrum changes when Orbit Size changes, with no re-bake and no table
- [ ] Sync, Bend and Window warps, unison detune/width and Phase produce the same effect on the terrain oscillator as on O-Prism's wavetable oscillator (θ parity)

### FUNC-02 / FUNC-03: Libraries

- [ ] Each analytic terrain's Mod X / Mod Y are documented and audibly change the spectrum; each orbit's Orbit Mod is documented and audibly changes the spectrum (Ellipse: inert, documented)
- [ ] Orbit Size 0.05 → 1.0 over Sine Product gives a monotonically non-decreasing spectral centroid

### FUNC-04: Feedback

- [ ] Grid Feedback × Damp × 6 terrains × 11 orbits at C2/C4/C6: bounded output (|y| ≤ 1), no NaN, no DC runaway; Feedback 0 is bit-identical to the feedback path removed

### FUNC-05: Audio-rate modulation

- [ ] An LFO at 40 Hz on Orbit Centre X produces sidebands at ±40 Hz around each harmonic (proof the destination is per-sample, not per-block)
- [ ] `modSlot?Dst` lists 46 entries: O-Prism's 26 followed by the 10 new per oscillator

### FUNC-06 / DSP-02 / DSP-03: Quality

- [ ] Bandlimited mode with Ellipse: the maximum harmonic in the output equals the truncation bound at each of C2 / C4 / C6 and non-harmonic energy ≤ −90 dB
- [ ] Switching Quality while notes are held does not allocate (debug allocator) and does not click beyond the crossfade
- [ ] Oversampling latency reported through `setLatencySamples` from `prepareToPlay`

### FUNC-07 / FUNC-08: PNG import and persistence

- [ ] PNG loads via chooser and via drag-and-drop on macOS and Windows; Blur and Edge Mode are audible and visible in the 3D view
- [ ] Save/reload a preset with an embedded PNG: identical rendered output (SHA-256 of a 1 s render matches); above the cap: path + SHA; missing file → fallback + notice (QUAL-03)

### FUNC-09 / FUNC-10: Inherited sections

- [ ] Parameter IDs for sub/noise, envelope, filter, LFO, mod matrix, FX, tuning and global sections match O-Prism v1.24.0
- [ ] The tuning tab loads a Scala file and a 31-EDO generator; rendered fundamental equals the TuningEngine frequency within 0.5 cent on ≥ 5 keys

### DSP-01: Pitch tracking

- [ ] With Pitch Track 1, the number of partials above −40 dB at C2, C4 and C6 differs by ≤ 2; with Pitch Track 0, non-harmonic energy at C6 rises (negative control)

### DSP-05 / PERF-01 / PERF-02: Real-time

- [ ] pluginval strictness 10 passes; zero allocations in processBlock under a debug allocator across all Quality settings
- [ ] CPU harness: 16 voices × 2 osc, unison 1, 2×, default patch, 48 kHz → ≤ 12 % of one core (renders 10 s, measures wall time ÷ audio time)

### DSP-06: Symmetry gate

- [ ] For every library terrain × orbit at defaults and every factory preset: harmonic 1 is the strongest partial or within 6 dB in ≥ 95 % of 1 s at C4; negative control: Centre (0,0), Aspect 1 over Sine Product fails the gate (octave up)

### UI-01 through UI-04: 3D view

- [ ] WebGL2 view renders in the Standalone, in Logic (WKWebView) and in a Windows host (WebView2); forcing `getContext('webgl2')` to null shows the Canvas 2D fallback and the localised placeholder in all three languages; `WEBGL_lose_context` recovers
- [ ] Playhead advances with θ at 30 Hz; the feedback trail is visible when Feedback > 0; unchanged state sends no event
- [ ] Dragging the view moves Orbit Centre X/Y through the relay with drag-start/drag-end (one host undo step); after a preset apply the view shows the new terrain within one push interval

### QUAL-01 / QUAL-02: Artifacts

- [ ] Offline render harness sweeps every library terrain × orbit at C2, C4, C6 at 2× with Pitch Track 1: non-harmonic energy ≤ −60 dB, no NaN
- [ ] Stepping each FUNC-05 destination by 10 % every 100 ms at C4: no sample-to-sample step > 0.1 in the rendered output

### COMPAT-01 / COMPAT-02: Validation

- [ ] pluginval strictness 10 VST3 + AU pass; auval pass — after the re-parameterise pass
- [ ] CI Windows build green; WebView2 static linking and `withUserDataFolder()` in place

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 (re-parameterise pass: 205 parameters, 46 mod destinations) |
| stage-2 | FUNC-01..06, FUNC-09, FUNC-10, DSP-*, PERF-01, PERF-02, QUAL-01, QUAL-02 |
| stage-3 | FUNC-07, UI-*, PERF-03 |
| stage-4 | FUNC-08, FUNC-11, COMPAT-02, QUAL-03, QUAL-04, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Baked geometry sources (mesh slicing, volume orbits, terrain sweeps → wavetable) | Ships as an O-Prism factory bank; full design preserved in `superseded-baked-v1/` | v1.1 |
| Wavetable oscillator mode | O-Prism's job | none |
| RGB-channel terrain morphing | Extension of PNG import | v1.1 |
| Dual displaced-orbit stereo | Per-voice CPU doubling | v1.x |
| Scanned synthesis | Stability + block-rate simulation | v1.2+ |
| ADAA for terrains | Unsound for composite paths | none |
| three.js / lit rendering | No bundler; ESM-only | v1.x |

---
*Regenerated from BRIEF.md on 2026-09-08 (re-plan)*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
