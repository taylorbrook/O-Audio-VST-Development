---
plugin: O-simpleSampler
stage: ideation
status: creative_brief_complete
last_updated: 2026-06-25 00:00:00
---

# Resume Point

## Current State: Creative Brief Complete

Creative brief has been finalized for O-simpleSampler. Ready to proceed to UI mockup or planning.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (pedagogical classic keyboard sampler — wk05 only)
- Parameters specified (source/region/pitch/vintage/filter/amp)
- UI vision captured (waveform editor as the headline surface)
- Use cases identified (wk05 demo + in-class activity)
- Requirements extracted with acceptance criteria

## Next Steps

1. Create UI mockup to visualize the waveform editor (recommended)
2. Start planning / DSP research (`/plan O-simpleSampler`)
3. Research the stretch algorithm choice (granular/PSOLA vs phase vocoder)

## Context to Preserve

**Key Decisions (from ideation):**
- Plugin type: Synth (Pedagogical Sampler Instrument)
- Scope: **Classic keyboard-melodic sampler only** — wk09/beatmaking framing dropped; accompanies ONLY MUSC319 wk05-wed sampling
- Slicing-into-hits and any internal sequencer are OUT OF SCOPE (pure instrument played by the DAW)
- **Repitch ↔ Stretch toggle** is the headline pitch/time-independence lesson
- **Vintage knob** (rate decimation + bit crush) = SP-1200 lo-fi character
- Curated embedded found-sounds + load-your-own drag-drop (in-class activity)
- Sibling-consistent: WebView UI, tooltips, concept-isolating presets, ~16 voices

**Open questions for Stage 0:**
- Stretch algorithm: reuse O-simpleGrain granular scheduler vs phase vocoder
- Anti-aliasing on upward repitch; loop-crossfade + zero-crossing snap
- Built-in sample set + per-sample default root key

**Files Created:**
- plugins/O-simpleSampler/.planning/BRIEF.md
- plugins/O-simpleSampler/.planning/REQUIREMENTS.md
- plugins/O-simpleSampler/.planning/STATUS.md
