# O-simpleFM Notes

## Status
- **Current Status:** ✅ Working
- **Version:** 1.1.0
- **Type:** Synth (Pedagogical 2-Operator FM/PM)

## Lifecycle Timeline

- **2026-06-20:** Ideated — creative brief + requirements (pedagogical 2-op FM synth).
- **2026-06-20 (Stage 0):** Research & Planning complete — ARCHITECTURE.md + ROADMAP.md documented. Complexity 5.0 (raw 11.0), staged implementation.
- **2026-06-20 (Stages 1–4):** Implemented & shipped v1.0.0 — DSP voice, WebView field-guide UI, preset manager.
- **2026-06-20 (v1.0.1):** UI fix — frame made scrollable, sections de-overlapped (`.controls` no longer shrinks below content), editor grown 760×720 → 760×860.
- **2026-06-20 (v1.0.2):** Added a dedicated tooltip for the Signal Path readout (`C:M ratio` + modulation index `I`); fixed `focusin` bubbling so nested `[data-tip]` tips aren't overridden by their ancestor.
- **2026-06-21 (v1.1.0):** Teaching + cleanup release. Added an on-screen keyboard (mouse + computer keyboard, MIDI injected via `MidiMessageCollector`) so the Standalone build makes sound without external MIDI; spectrum frequency-axis labels (100/1k/10k via new `getSampleRate` native fn); live harmonic/inharmonic + Carson sideband-count readout. Fixed the dead/self-contradicting index-ceiling code in `FMVoice` (single `computeIndexCeiling()` armed per block + snapped on note-on). De-duplicated the Lesson Presets (now load the factory presets by name; captions only in JS). Window grown 760×860 → 760×980. Validation: harness 7/7, auval + pluginval L10 green.

## Known Issues

None.

## Additional Notes

Deliberately simple 2-operator **phase-modulation** synth built for teaching FM. One carrier, one modulator, C:M ratio, raw radian modulation index (0–20), DX7-style self-feedback, independent mod + amp ADSRs (mod env → index is the headline feature). 16-voice polyphony. WebView UI with first-class teaching visuals: live spectrum analyzer (4096/Blackman-Harris FFT), oscilloscope, operator routing diagram, per-parameter hover tooltips, educational preset tour.

Key Stage 0 decisions: PM not true FM; raw radian index exposed; 16 voices; anti-aliasing via sine LUT + key-tracked index ceiling + 2× polyphase-IIR oversampling (4× + band-limited wavetables only for opt-in non-sine operators). Real-time-safe lock-free `AbstractFifo` audio→UI tap; FFT on message-thread Timer.

Architecture references: O-Bassoon (voice skeleton), O-Marimba (scope FIFO + native fn), O-Prism (APVTS/WebView relays/FFT), O-AnalogEQ (cross-platform WebView2 CMake).

See `.planning/research/ARCHITECTURE.md` and `.planning/ROADMAP.md` for full detail; upstream research at `research/fm-phase-modulation-synthesis-o-simplefm.md`.
