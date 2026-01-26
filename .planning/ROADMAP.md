# Roadmap: O-Bass v1.0

## Overview

O-Bass v1.0 delivers a psychoacoustic bass enhancement plugin with Clean and Colored modes, minimal 4-control WebView UI, and full Ouaricon suite integration. The roadmap progresses from DSP foundation through algorithm implementation, control refinement, UI development, and finally format/preset integration. Each phase delivers a coherent, testable capability that builds on the previous phase.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Core DSP Foundation** - Crossover, mono bass processing, parameter framework, latency management
- [x] **Phase 2: Clean Mode** - Psychoacoustic harmonic generation with oversampling
- [x] **Phase 3: Colored Mode** - Analog saturation character with mode switching
- [x] **Phase 4: Controls & Refinement** - All 4 controls with intensity limiting and parameter tuning
- [x] **Phase 5: WebView UI** - Visual interface matching Ouaricon suite language
- [x] **Phase 6: Formats & Integration** - VST3/AU builds and preset system
- [x] **Phase 7: Oversampling & Adaptive Harmonics** - Tech debt closure (DSP-04)
- [ ] **Phase 8: Sound & UI Polish** - Final tuning and visual refinements

## Phase Details

### Phase 1: Core DSP Foundation
**Goal**: Establish the audio processing architecture that all enhancement algorithms depend on
**Depends on**: Nothing (first phase)
**Requirements**: DSP-02, DSP-03, DSP-05
**Success Criteria** (what must be TRUE):
  1. Audio passes through plugin with unity gain when enhancement is bypassed
  2. Crossover filter splits signal at configurable frequency (40-200Hz range)
  3. Bass frequencies below crossover are summed to mono before processing
  4. Plugin reports accurate latency to host (under 5ms at 44.1kHz)
  5. No allocations occur in processBlock (pre-allocated buffers only)
**Plans**: 6 plans in 5 waves

Plans:
- [x] 01-01-PLAN.md — Plugin scaffold with APVTS and pass-through
- [x] 01-02-PLAN.md — Dual-mode crossover filter (IIR + FIR)
- [x] 01-03-PLAN.md — Mono summing and stereo expansion
- [x] 01-04-PLAN.md — Signal path integration and latency reporting
- [x] 01-05-PLAN.md — Gap closure: RT-safe FIR coefficient bank (eliminates allocations in processBlock)
- [x] 01-06-PLAN.md — Gap closure: RT-safe mode switching (atomic flag, dual filter preparation)

### Phase 2: Clean Mode
**Goal**: Implement psychoacoustic harmonic generation that creates perceived bass on limited playback systems
**Depends on**: Phase 1
**Requirements**: DSP-01, DSP-04, MODE-01
**Success Criteria** (what must be TRUE):
  1. Low-frequency content generates audible harmonics in 100-400Hz range
  2. Enhancement is transparent with no audible aliasing artifacts
  3. Harmonics translate to perceived bass weight on laptop/phone speakers
  4. Processing uses 4x oversampling to prevent aliasing
  5. Original transient character is preserved (no smearing on attack)
**Plans**: 4 plans in 3 waves

Plans:
- [x] 02-01-PLAN.md — Envelope follower and pitch tracker components
- [x] 02-02-PLAN.md — Harmonic generator with Chebyshev waveshaping and 4x oversampling
- [x] 02-03-PLAN.md — CleanModeProcessor orchestration with transient ducking
- [x] 02-04-PLAN.md — Plugin integration and perceptual verification

### Phase 3: Colored Mode
**Goal**: Add analog-style saturation character as alternative to transparent Clean mode
**Depends on**: Phase 2
**Requirements**: MODE-02, MODE-03
**Success Criteria** (what must be TRUE):
  1. Colored mode produces audibly warmer character than Clean mode
  2. Mode switch toggles between Clean and Colored processing paths
  3. Enhancement intensity behaves consistently across both modes
  4. No clicks or artifacts when switching modes during playback
**Plans**: 2 plans in 2 waves

Plans:
- [x] 03-01-PLAN.md — ColoredModeProcessor with asymmetric tanh saturation
- [x] 03-02-PLAN.md — Plugin integration with mode switching and crossfade

### Phase 4: Controls & Refinement
**Goal**: All 4 parameters function with musical behavior and auto-limiting to prevent over-processing
**Depends on**: Phase 3
**Requirements**: CTRL-01, CTRL-02, CTRL-03, CTRL-04
**Success Criteria** (what must be TRUE):
  1. Frequency knob smoothly adjusts crossover from 40Hz to 200Hz
  2. Enhance knob applies intensity with diminishing returns curve (prevents boomy sound)
  3. Output knob provides +/- 18dB gain compensation
  4. Mode toggle switches between Clean and Colored with smooth transition
  5. Extreme Enhance settings are auto-limited to prevent artifacts
**Plans**: 3 plans in 2 waves

Plans:
- [x] 04-01-PLAN.md — Intensity tuning (ColoredMode boost, frequency-dependent scaling, bandpass extension)
- [x] 04-02-PLAN.md — Output gain control with soft clipping
- [x] 04-03-PLAN.md — Limit indicator metering and human verification

### Phase 5: WebView UI
**Goal**: Visual interface that matches Ouaricon suite and exposes all 4 controls
**Depends on**: Phase 4
**Requirements**: UI-01, UI-02, UI-03
**Success Criteria** (what must be TRUE):
  1. WebView displays 4 controls: Frequency, Enhance, Output, Mode toggle
  2. UI matches Ouaricon visual language (paper texture, botanical style)
  3. Knob movements update DSP parameters in real-time without glitches
  4. Parameter changes from host (automation) reflect in UI immediately
  5. UI is responsive and renders correctly at default plugin size
**Plans**: 3 plans in 3 waves

Plans:
- [x] 05-01-PLAN.md — UI assets, HTML/CSS/JS interface, CMake BinaryData
- [x] 05-02-PLAN.md — PluginEditor WebView implementation with parameter binding
- [x] 05-03-PLAN.md — Build verification and human UI approval

### Phase 6: Formats & Integration
**Goal**: Plugin builds in all formats with functional preset system
**Depends on**: Phase 5
**Requirements**: FMT-01, FMT-02, INT-01
**Success Criteria** (what must be TRUE):
  1. Plugin builds and loads as VST3 in compatible DAW (Logic, Ableton, etc.)
  2. Plugin builds and loads as AU in Logic Pro
  3. OuariconPresetManager loads and saves presets correctly
  4. Factory presets demonstrate Clean and Colored modes on different source types
  5. pluginval passes at strictness 5+, auval passes validation
**Plans**: 2 plans in 2 waves

Plans:
- [x] 06-01-PLAN.md — Preset manager integration and factory preset definitions
- [x] 06-02-PLAN.md — Build, validation (pluginval + auval), and human DAW verification

### Phase 7: Oversampling & Adaptive Harmonics
**Goal**: Wire 4x oversampling and pitch-adaptive harmonics that were planned but bypassed during Phase 2
**Depends on**: Phase 6
**Requirements**: DSP-04
**Gap Closure**: Closes tech debt from v1.0 audit (dead code, orphaned components)
**Success Criteria** (what must be TRUE):
  1. HarmonicGenerator uses 4x oversampling in process() path
  2. PitchTracker.detectPitch() called and drives adaptive harmonic count
  3. No dead code paths (processOversampled either integrated or removed)
  4. Documentation matches implementation (STATE.md decisions accurate)
  5. pluginval still passes at strictness 10 after changes
**Plans**: 3 plans in 3 waves

Plans:
- [x] 07-01-PLAN.md — Wire 4x oversampling into HarmonicGenerator
- [x] 07-02-PLAN.md — Wire PitchTracker for adaptive harmonics
- [x] 07-03-PLAN.md — Dead code cleanup and documentation update

### Phase 8: Sound & UI Polish
**Goal**: Final tuning pass on effect character and visual presentation before release
**Depends on**: Phase 7
**Requirements**: MODE-02
**Gap Closure**: Ensures Colored mode has distinct analog warmth character
**Success Criteria** (what must be TRUE):
  1. Colored mode is audibly warmer/more saturated than Clean mode in A/B test
  2. Both modes sound musical across the full Enhance range (0-100%)
  3. UI visual polish complete (animations smooth, layout balanced)
  4. Human listening test approves final sound quality
  5. Human visual inspection approves final UI appearance
**Plans**: TBD

Plans:
- [ ] 08-01-PLAN.md — Sound tuning and Colored mode differentiation
- [ ] 08-02-PLAN.md — UI visual polish and refinements
- [ ] 08-03-PLAN.md — Final human verification (sound + UI approval)

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Core DSP Foundation | 6/6 | ✓ Complete | 2026-01-23 |
| 2. Clean Mode | 4/4 | ✓ Complete | 2026-01-24 |
| 3. Colored Mode | 2/2 | ✓ Complete | 2026-01-25 |
| 4. Controls & Refinement | 3/3 | ✓ Complete | 2026-01-24 |
| 5. WebView UI | 3/3 | ✓ Complete | 2026-01-25 |
| 6. Formats & Integration | 2/2 | ✓ Complete | 2026-01-26 |
| 7. Oversampling & Adaptive Harmonics | 3/3 | ✓ Complete | 2026-01-26 |
| 8. Sound & UI Polish | 0/3 | Pending | — |

---
*Roadmap created: 2026-01-22*
*Milestone: v1.0 Initial Release*
