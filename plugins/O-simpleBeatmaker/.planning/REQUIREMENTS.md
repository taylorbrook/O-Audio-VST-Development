# O-simpleBeatmaker - Requirements

---
version: 1.0.0
plugin: O-simpleBeatmaker
created: 2026-06-25
lastUpdated: 2026-06-25
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 24
**Coverage:** must: 15 | should: 7 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | 16-step grid drum machine: rows = synthesized drum voices, columns = sixteenth-note steps; toggling a cell programs a hit at that step | must | pending | stage-2 |
| FUNC-02 | Per-step velocity (0–127) with quick ghost / normal / accent states (click-again-to-accent), audibly changing the hit's dynamics | must | pending | stage-2 |
| FUNC-03 | Internal sequencer runs in sync with the host transport (DAW tempo + play position); playhead reflects the current step | must | pending | stage-2 |
| FUNC-04 | Voices are MIDI-playable: incoming MIDI notes (GM drum mapping) trigger the same voices, so the pattern can also be programmed from the DAW piano roll | must | pending | stage-2 |
| FUNC-05 | Concept-isolating factory pattern presets (straight/no-feel, backbeat + accents, ghost notes, swing, humanized, + a couple genre grooves) | should | pending | stage-4 |
| FUNC-06 | Per-voice mute / solo for isolating parts while teaching | should | pending | stage-2 |
| FUNC-07 | Pattern length selectable (8 / 16 / 32 steps) | nice | pending | stage-2 |
| FUNC-08 | Playable/musical enough to double as a simple 808/909-style drum instrument in real projects | nice | pending | stage-4 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Synthesized 808/909-lineage drum voices (kick, snare, clap, closed hat, open hat, tom — final roster confirmed in research), no samples | must | pending | stage-2 |
| DSP-02 | Swing: delays the off-beat 16ths into long-short pairs by a controllable amount (0–75%) | must | pending | stage-2 |
| DSP-03 | Humanize: adds small random per-hit timing + velocity offsets so repeated hits are not identical | must | pending | stage-2 |
| DSP-04 | Quantize strength: pulls the humanized timing deviation back toward the grid (100% = dead tight, lower = keeps looseness); does not remove intentional swing | must | pending | stage-2 |
| DSP-05 | Timing-feel offsets resolve to a per-hit sub-step Δt and trigger voices sample-accurately within the processing block (not snapped to block boundaries) | must | pending | stage-2 |
| DSP-06 | Per-voice shaping: tune, decay, tone/snap, level (minimal, transparent — confirmed per voice in research) | should | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | 16-step grid as the dominant control surface, with a live playhead sweeping in sync with the host | must | pending | stage-3 |
| UI-02 | Per-step velocity made visible (cell height/brightness encodes velocity; accent vs ghost obvious at a glance) | must | pending | stage-3 |
| UI-03 | Timing / groove lane that shows each hit's actual offset from its grid line as swing/humanize push off and quantize-strength pulls back | must | pending | stage-3 |
| UI-04 | Live MIDI message readout printing note-on (note#, velocity) events as steps fire | should | pending | stage-3 |
| UI-05 | On-hover pedagogical tooltips on every control (plain-language explanation + concrete example) | should | pending | stage-3 |
| UI-06 | Single-page, projector-readable, uncluttered layout consistent with the simple family | should | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations/locks/file-I/O in processBlock; humanize RNG pre-seeded); visualization data crosses to UI via lock-free handoff | must | pending | stage-2 |
| PERF-02 | All voices render in time at typical buffer sizes without dropouts; timing offsets stay sample-accurate | should | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | must | pending | stage-1 |
| COMPAT-02 | Windows WebView2 configured correctly (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`); UI renders on Windows | should | pending | stage-3 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts (clicks, zipper, aliasing) at normal parameter ranges across the voice set | must | pending | stage-2 |
| QUAL-02 | The timing lane, playhead, velocity display, and MIDI readout accurately reflect what is heard (visual matches audio) | must | pending | stage-3 |

## Acceptance Criteria Details

### FUNC-01: Step-grid drum machine

**Description:** A 16-step grid where rows are synthesized drum voices and columns are sixteenth-note steps; toggling a cell programs a hit.

**Acceptance Criteria:**
- [ ] Toggling any cell on/off adds/removes that voice's hit at that step
- [ ] When the host plays, lit steps trigger their voice in time with the transport
- [ ] The playhead position matches the step currently sounding

### FUNC-02: Per-step velocity

**Description:** Each lit step carries an editable velocity with ghost / normal / accent quick-states.

**Acceptance Criteria:**
- [ ] A step's velocity is editable (0–127) and audibly changes that hit's loudness/timbre
- [ ] Accent (loud) and ghost (quiet) states are reachable quickly (e.g. click-again / drag) and reflected in the cell display
- [ ] A flat all-equal-velocity pattern sounds mechanical; setting accents + ghosts gives audible dynamics

### FUNC-03: Host-synced sequencer

**Acceptance Criteria:**
- [ ] The pattern plays in sync with the DAW tempo and bar position
- [ ] Starting/stopping the host transport starts/stops the sequencer
- [ ] Changing host tempo changes the beat's speed correctly

### FUNC-04: MIDI-playable voices

**Acceptance Criteria:**
- [ ] Incoming MIDI notes (GM drum mapping) trigger the corresponding voices
- [ ] Note-on velocity drives the triggered hit's velocity
- [ ] The same pattern programmed in the DAW piano roll produces the same instrument sound as the internal grid

### DSP-02: Swing

**Acceptance Criteria:**
- [ ] At 0% the off-beat 16ths fall evenly on the grid; raising swing delays them into audible long-short pairs
- [ ] The delay is visible in the timing lane and the groove audibly shuffles

### DSP-03: Humanize

**Acceptance Criteria:**
- [ ] At 0% repeated hits are identical; raising humanize introduces small random timing + velocity variation
- [ ] The scatter is visible in the timing lane and audible as a looser feel

### DSP-04: Quantize strength

**Acceptance Criteria:**
- [ ] With humanize > 0, raising quantize strength pulls hits back toward the grid (100% = dead tight)
- [ ] Lowering quantize strength keeps more of the human looseness
- [ ] Intentional swing is preserved (not removed) as quantize strength rises

### DSP-05: Sample-accurate timing offsets

**Acceptance Criteria:**
- [ ] Swing/humanize/quantize resolve to per-hit sub-step offsets, not block-boundary snapping
- [ ] Triggers fire at the correct sample within the buffer (verified in an offline render harness)

### UI-03: Timing / groove lane

**Acceptance Criteria:**
- [ ] Each scheduled hit is drawn at its actual offset from the grid line
- [ ] Swing and humanize visibly push hits off the line; quantize strength visibly pulls them back
- [ ] The displayed offset matches the Δt actually applied to audio (QUAL-02)

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [ ] No allocations, locks, or file I/O in processBlock; humanize RNG is pre-seeded/real-time safe
- [ ] Playhead/velocity/timing/MIDI visualization data reaches the UI via a lock-free FIFO/atomic handoff

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01/02/03/04/06/07, DSP-*, PERF-*, QUAL-01 |
| stage-3 | UI-*, COMPAT-02, QUAL-02 |
| stage-4 | FUNC-05/08, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Bass / melodic lanes | Drums-only keeps the groove/velocity/swing/quantize focus tight; A3 bassline done with a separate DAW instrument | v1.1+ |
| Sample loading / sampled kits | Voices are synthesized + transparent; sampling is O-MicrotonalSampler / DrumRoulette territory | out |
| CC / orchestration-realism teaching (velocity layers, CC1/CC11, keyswitches) | Belongs to a deep sample library, not a minimal beat instrument | out |
| Song mode / pattern chaining (A/B/…) | v1 is a single pattern | v1.1+ |
| Per-step probability, ratchets/rolls, parameter locks | Out of pedagogical core | v1.1+ |
| Per-row step lengths / polymeter | Out of pedagogical core | future |
| Effects beyond master output | Keep signal path transparent for teaching | out |

---
*Generated from BRIEF.md on 2026-06-25*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
