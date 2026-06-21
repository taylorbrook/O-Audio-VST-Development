# O-simpleFM Notes

## Status
- **Current Status:** 🚧 Stage 0 (Research & Planning complete)
- **Version:** N/A
- **Type:** Synth (Pedagogical 2-Operator FM/PM)

## Lifecycle Timeline

- **2026-06-20:** Ideated — creative brief + requirements (pedagogical 2-op FM synth).
- **2026-06-20 (Stage 0):** Research & Planning complete — ARCHITECTURE.md + ROADMAP.md documented. Complexity 5.0 (raw 11.0), staged implementation.

## Known Issues

None (no implementation yet).

## Additional Notes

Deliberately simple 2-operator **phase-modulation** synth built for teaching FM. One carrier, one modulator, C:M ratio, raw radian modulation index (0–20), DX7-style self-feedback, independent mod + amp ADSRs (mod env → index is the headline feature). 16-voice polyphony. WebView UI with first-class teaching visuals: live spectrum analyzer (4096/Blackman-Harris FFT), oscilloscope, operator routing diagram, per-parameter hover tooltips, educational preset tour.

Key Stage 0 decisions: PM not true FM; raw radian index exposed; 16 voices; anti-aliasing via sine LUT + key-tracked index ceiling + 2× polyphase-IIR oversampling (4× + band-limited wavetables only for opt-in non-sine operators). Real-time-safe lock-free `AbstractFifo` audio→UI tap; FFT on message-thread Timer.

Architecture references: O-Bassoon (voice skeleton), O-Marimba (scope FIFO + native fn), O-Prism (APVTS/WebView relays/FFT), O-AnalogEQ (cross-platform WebView2 CMake).

See `.planning/research/ARCHITECTURE.md` and `.planning/ROADMAP.md` for full detail; upstream research at `research/fm-phase-modulation-synthesis-o-simplefm.md`.
