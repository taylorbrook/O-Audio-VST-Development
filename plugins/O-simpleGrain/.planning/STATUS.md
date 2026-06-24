---
plugin: O-simpleGrain
stage: ideation
status: creative_brief_complete
last_updated: 2026-06-24 00:00:00
---

# Resume Point

## Current State: Creative Brief Complete

Creative brief has been finalized for O-simpleGrain. Ready to proceed to UI mockup or implementation.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined — pedagogical granular synth, third sibling to O-simpleFM / O-simpleAdditive
- Parameters specified (15 core + research confirmations)
- UI vision captured (four live visuals + CPU readout + preset tour + tooltips)
- Use cases identified (classroom demo, wk08 activity, self-directed, creative instrument)
- Requirements extracted with acceptance criteria (24 requirements)

## Next Steps

1. Create UI mockup to visualize design (recommended)
2. Start planning / DSP research (`/plan O-simpleGrain`)
3. Research similar plugins for inspiration

## Context to Preserve

**Key Decisions (from ideation Q&A):**
- Plugin type: Synth (Pedagogical Granular Synthesizer)
- Grain source: **Built-in curated samples + load-your-own** (drag-drop / picker)
- Playback model: **MIDI instrument + Freeze mode** (pitched, polyphonic, plus freeze/sustain)
- v1.0 scope: **Granular only** — spectral STFT (freeze/blur/filter) deferred to a future sibling O-simpleSpectral
- Live visuals (all four): grain cloud scatter, source waveform + playheads, grain-envelope inset, output scope/spectrum
- Tied to MUSC319 wk08 granular & spectral session; class ref: `/Users/taylorbrook/Documents/UBC/Courses/MUSC319/2026 term 1/out/wk08-mon-granular-spectral.html`

**Open items for Stage 0 research:**
- Density exposure (grains/sec vs grain period vs overlap factor) + live overlap readout
- MIDI-key → grain-pitch coupling (key-tracked resample vs gate-only)
- Anti-aliasing on upward transposition
- Polyphony confirmation (proposing 8) + grain budget / CPU cap
- macOS WebView load-your-own via content-streaming drag-drop (juce::Base64, NOT MemoryBlock::fromBase64Encoding)

**Files Created:**
- plugins/O-simpleGrain/.planning/BRIEF.md
- plugins/O-simpleGrain/.planning/REQUIREMENTS.md
- plugins/O-simpleGrain/.planning/STATUS.md
