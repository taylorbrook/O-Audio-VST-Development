# Drum808 Notes

## Status
- **Current Status:** 💡 Ideated
- **Version:** N/A
- **Type:** Synth (Drum Instrument)

## Lifecycle Timeline

- **2025-11-13:** Creative brief completed

## Known Issues

None

## Additional Notes

**Description:**
MIDI-triggered 808 drum machine clone with six authentic analog-modeled voices (kick, low tom, mid tom, clap, closed hat, open hat). Circuit-accurate synthesis with modern enhancements: individual outputs, velocity sensitivity, and per-drum tuning.

**Parameters (26 total):**

Kick (6):
- Level: 0-100%, default 80%
- Tone: 0-100%, default 50%
- Decay: 50-1000ms, default 400ms
- Tuning: ±12 st, default 0 st

Low Tom (4):
- Level: 0-100%, default 75%
- Tone: 0-100%, default 50%
- Decay: 50-1000ms, default 300ms
- Tuning: ±12 st, default 0 st

Mid Tom (4):
- Level: 0-100%, default 75%
- Tone: 0-100%, default 50%
- Decay: 50-1000ms, default 250ms
- Tuning: ±12 st, default +5 st

Clap (4):
- Level: 0-100%, default 70%
- Tone: 0-100%, default 50%
- Snap: 0-100%, default 60%
- Tuning: ±12 st, default 0 st

Closed Hat (4):
- Level: 0-100%, default 65%
- Tone: 0-100%, default 60%
- Decay: 20-200ms, default 80ms
- Tuning: ±12 st, default 0 st

Open Hat (4):
- Level: 0-100%, default 60%
- Tone: 0-100%, default 60%
- Decay: 100-1000ms, default 500ms
- Tuning: ±12 st, default 0 st

**DSP:** Circuit-accurate 808 synthesis. Bridged-T oscillators for kick/toms, filtered noise with multi-trigger envelope for clap, six-oscillator metallic synthesis for hi-hats. Closed hat chokes open hat (authentic behavior).

**GUI:** Six vertical channel strips (one per drum), each with four knobs. 808-inspired aesthetic with LED trigger indicators. Individual output routing display. Horizontal layout (~1000×500px).

**MIDI:** GM drum mapping (C1=kick, D1=clap, F1=low tom, A1=mid tom, F#1=closed hat, A#1=open hat). Velocity-sensitive. Monophonic per voice.
