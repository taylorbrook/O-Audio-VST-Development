# O-simpleBeatmaker — Parameter Specification (DRAFT)

> **Status:** Draft, extracted from `BRIEF.md` for Stage 0 planning.
> Ranges/tapers/roster are starting proposals — Stage 0 research confirms the
> exact voice roster, swing/humanize/quantize formulations, and per-voice tone
> mappings. A full `parameter-spec.md` is required before Stage 1 (mockup
> finalization).

Plugin type: **Synth (Pedagogical Step-Sequencer Drum Machine)**
Voice roster (proposed, confirm in research): **Kick, Snare, Clap, Closed Hat, Open Hat, Tom** (~6 voices)

---

## A. Sequencer / Timing-Feel (the pedagogical core)

| Parameter | ID (proposed) | Range | Default | Taper | Notes |
|-----------|---------------|-------|---------|-------|-------|
| Step On/Off (per voice × 16 steps) | `step_{voice}_{n}` | on/off | off | — | The grid. Per-voice × 16-step boolean matrix. Stored as pattern state (likely non-APVTS / custom state, confirm in research). |
| Step Velocity (per lit step) | `vel_{voice}_{n}` | 0–127 (ghost / normal / accent quick-states) | normal (~100) | linear | Per-hit dynamics. THE velocity lesson. Cell height/brightness encodes it. |
| Swing | `swing` | 0–75% | 0% | linear | Delays off-beat 16ths into long-short pairs. Confirm exact curve + 8th vs 16th swing in research. |
| Humanize | `humanize` | 0–100% | 0% | linear | Random per-hit timing + velocity offset. Confirm timing/velocity split + distribution in research. |
| Quantize Strength | `quantize` | 0–100% | 100% | linear | Pulls humanized deviation back toward grid. 100% = dead tight. THE quantize lesson. Does NOT remove swing. |
| Pattern Length | `pattern_length` | 8 / 16 / 32 steps | 16 | discrete | Steps per loop. Confirm range in research. |
| Tempo | (host) | follows host transport | host | — | Synced to DAW. Confirm whether to expose internal free-run tempo for standalone. |

## B. Per Drum Voice (intentionally minimal — transparent, not a black box)

Applied to each voice in the roster.

| Parameter | ID (proposed) | Range | Default | Taper | Notes |
|-----------|---------------|-------|---------|-------|-------|
| Voice Tune | `{voice}_tune` | -12 to +12 semitones (or per-voice musical range) | 0 | linear | Pitch of synthesized voice. |
| Voice Decay | `{voice}_decay` | short–long (per-voice) | per-voice | linear/skewed | Tail length (kick boom, open-hat sustain). |
| Voice Tone / Snap | `{voice}_tone` | 0–100% | per-voice | linear | One timbral knob per voice (kick click/punch, snare tone↔snap, hat brightness). Exact per-voice mapping in research. |
| Voice Level | `{voice}_level` | -inf–0 dB | 0 dB | dB/skewed | Per-voice mixer level. |
| Voice Mute | `{voice}_mute` | on/off | off | — | Standard mixer mute. |
| Voice Solo | `{voice}_solo` | on/off | off | — | Standard mixer solo. |

## C. Master

| Parameter | ID (proposed) | Range | Default | Taper | Notes |
|-----------|---------------|-------|---------|-------|-------|
| Output Level | `output_level` | -inf–0 dB | 0 dB | dB/skewed | Master output gain. |

---

## Open questions for Stage 0 research

- 808-vs-909 synthesis flavor per voice.
- Exact swing curve (e.g. % delay of even 16ths); 8th-note vs 16th-note swing.
- Humanize distribution + how it splits timing vs velocity; how quantize-strength composes with swing.
- Sub-step timing-offset scheduling (sample-accurate triggering within `processBlock`).
- GM drum note map for MIDI-play.
- Per-voice polyphony/tail handling (monophonic-retrigger + tail).
- Final voice count/roster (≈6; possible Rimshot / Cowbell additions).
- Per-voice pan (nice-to-have).
- Whether pattern presets ship as factory data.
- Pattern/velocity matrix state representation (APVTS vs custom serialized state).

---

*Extracted from `BRIEF.md` (Parameters section) on 2026-06-25 to satisfy Stage 0 precondition. Supersede with full `parameter-spec.md` after mockup finalization.*
