# O-simpleGrain - Requirements

---
version: 1.0.0
plugin: O-simpleGrain
created: 2026-06-24
lastUpdated: 2026-06-24
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 24
**Coverage:** must: 16 | should: 6 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Granulate a source buffer into overlapping windowed grains summed via overlap-add into a continuous sound | must | pending | stage-2 |
| FUNC-02 | MIDI-playable, polyphonic instrument — each held note transposes the grain cloud to that pitch | must | pending | stage-2 |
| FUNC-03 | Freeze pins the read head on one instant and sustains it indefinitely (held pad from one moment) | must | pending | stage-2 |
| FUNC-04 | Ship curated built-in source samples (incl. a "fire" sound for the class's worked example) selectable from the UI | must | pending | stage-2 |
| FUNC-05 | Load-your-own source: user can supply a short sound via drag-drop and/or file picker | should | pending | stage-2 |
| FUNC-06 | Concept-isolating preset tour (single grain, pitched buzz, fragments, smooth cloud, frozen pad, asynchronous cloud, granular fire) | should | pending | stage-3 |
| FUNC-07 | On-hover pedagogical tooltips on every control (plain-language, class-grounded) | should | pending | stage-3 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Grain Size control crosses the buzz↔fragments continuum (audibly pitched buzz at a few ms, recognizable fragments at tens of ms) | must | pending | stage-2 |
| DSP-02 | Density control sets overlap depth; raising it fuses discrete grains into a continuous cloud | must | pending | stage-2 |
| DSP-03 | Five selectable window shapes (rectangular, triangular, Welch, Gaussian, Hann); rectangular audibly clicks, smooth windows do not | must | pending | stage-2 |
| DSP-04 | Pitch spray and position spray apply independent per-grain randomization (texture shimmers, no two grains identical) | must | pending | stage-2 |
| DSP-05 | Scatter (grain-period randomness) moves the result from synchronous (discrete sidebands/pitched) to asynchronous (noisy cloud) | must | pending | stage-2 |
| DSP-06 | Scan / Time-Stretch moves the read head through the source faster/slower than realtime, including reverse and hold | should | pending | stage-2 |
| DSP-07 | Per-voice amplitude ADSR envelope | must | pending | stage-2 |
| DSP-08 | Upward transposition is band-limited / anti-aliased so high pitch-spray grains stay clean | should | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Grain cloud scatter — accumulating grains plotted over the source (read-position × time) | must | pending | stage-3 |
| UI-02 | Source waveform + live playheads, freeze point, and shaded position-spray range | must | pending | stage-3 |
| UI-03 | Grain-envelope inset — live plot of the selected window shape on one grain | must | pending | stage-3 |
| UI-04 | Output scope / spectrum showing synchronous (sidebands) vs asynchronous (noise) | should | pending | stage-3 |
| UI-05 | Live grain-count / CPU readout connecting density × grain size × polyphony to CPU cost | nice | pending | stage-3 |
| UI-06 | Single clear projector-readable page, no deep menus, consistent with O-simpleFM / O-simpleAdditive | must | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing — no allocations or locks in processBlock (preallocated grain pool) | must | pending | stage-2 |
| PERF-02 | Bounded grain count with graceful voice/grain-stealing so high density × size × polyphony cannot xrun | nice | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | pending | stage-1 |
| COMPAT-02 | Windows WebView2 flags set (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) | must | pending | stage-1 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No unintended audio artifacts (no zipper on param moves, clean freeze/unfreeze, no clicks except the intentional rectangular-window teaching artifact) | must | pending | stage-2 |

## Acceptance Criteria Details

### FUNC-01: Granular overlap-add engine

**Description:** Grains are read from the source buffer, windowed, and summed via overlap-add into a continuous sound.

**Acceptance Criteria:**
- [ ] With Density high enough that grain period < grain size, output is continuous (no gaps between grains)
- [ ] Lowering Density below overlap produces audibly separated grains
- [ ] Engine is allocation-free and lock-free in processBlock

### FUNC-02: MIDI-playable polyphonic instrument

**Acceptance Criteria:**
- [ ] Holding different MIDI notes produces grain clouds transposed to those pitches
- [ ] At least the proposed polyphony (8 voices, confirm in research) sounds simultaneously without dropouts

### FUNC-03: Freeze

**Acceptance Criteria:**
- [ ] Engaging Freeze pins the read position; the sustained texture does not drift through the source
- [ ] Freeze→unfreeze transition is click-free

### DSP-01: Grain Size buzz↔fragments continuum

**Acceptance Criteria:**
- [ ] At a few ms the grain stream reads as a pitched buzz
- [ ] At tens of ms recognizable source fragments are heard
- [ ] The grain-cloud scatter visibly reflects the change

### DSP-03: Window shapes

**Acceptance Criteria:**
- [ ] All five window shapes selectable and applied per grain
- [ ] Rectangular window adds an audible broadband click per grain; Hann/Gaussian do not
- [ ] Grain-envelope inset redraws to match the selected shape

### DSP-05: Synchronous → asynchronous via Scatter

**Acceptance Criteria:**
- [ ] At Scatter 0% the output shows discrete sidebands (pitched) on the spectrum view
- [ ] At high Scatter the spectrum smears toward noise
- [ ] No allocation/locks introduced by the per-grain RNG

### UI-01: Grain cloud scatter

**Acceptance Criteria:**
- [ ] Grains appear as dots positioned by source read-position over time
- [ ] Raising density visibly thickens the cloud; spray visibly widens it
- [ ] Driven by a lock-free FIFO from the audio thread (no audio-thread UI work)

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [ ] No heap allocation in processBlock (verified by inspection / instrumentation)
- [ ] No locks on the audio thread
- [ ] Passes pluginval strictness without real-time violations

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01, COMPAT-02 |
| stage-2 | FUNC-01..05, DSP-*, PERF-*, QUAL-01 |
| stage-3 | FUNC-06, FUNC-07, UI-* |
| stage-4 | COMPAT-*, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Spectral STFT processing (freeze/blur/filter) | The spectral half of wk08; preserve one-concept discipline | O-simpleSpectral (sibling) |
| Phase-vocoder / tempo-locked time-stretch | Granular Scan + Freeze covers the time-stretch/freeze key term | v1.1+ |
| Recording live input into the buffer | Built-ins + load-your-own suffice | v1.1+ |
| Effects (reverb/delay/chorus) | Keep signal path transparent for teaching | — |
| Deep modulation matrix / LFO networks | Spray + scatter provide the taught randomness | v1.1+ (auto-scan LFO) |
| Multi-sample / multi-layer sources | One source buffer keeps it simple | v1.1+ |

---
*Generated from BRIEF.md on 2026-06-24*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
