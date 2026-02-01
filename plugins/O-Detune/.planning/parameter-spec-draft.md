# O-Detune Parameter Specification (Draft)

**Status:** Draft - extracted from BRIEF.md
**Created:** 2026-02-01
**Finalize before:** Stage 1 (Foundation)

---

## Mode Selection

| Parameter | ID | Type | Range | Default | Notes |
|-----------|-----|------|-------|---------|-------|
| Mode Blend | blend | Float | 0.0-1.0 | 0.5 | 0=Wobble only, 1=Unison only |

---

## Wobble Engine

| Parameter | ID | Type | Range | Default | Notes |
|-----------|-----|------|-------|---------|-------|
| Era | wobble_era | Choice | 60s/70s/80s | 70s | Ampex/Teac/Cassette character |
| Rate | wobble_rate | Float | 0.1-10.0 Hz | 2.0 | Modulation speed |
| Depth | wobble_depth | Float | 0-100 cents | 25 | Pitch deviation amount |
| Shape | wobble_shape | Choice | Sine/Triangle/Random | Sine | LFO waveform |
| Tempo Sync | wobble_sync | Bool | Off/On | Off | Lock rate to DAW BPM |

---

## Unison Engine

| Parameter | ID | Type | Range | Default | Notes |
|-----------|-----|------|-------|---------|-------|
| Voices | unison_voices | Choice | 2/3/4/5/7 | 3 | Number of parallel voices |
| Detune | unison_detune | Float | 0-50 cents | 15 | Total spread across voices |
| Distribution | unison_dist | Choice | Linear/Exp/Random | Linear | Voice frequency spacing |
| Stereo Spread | unison_spread | Float | 0-100% | 75 | Voice panning width |

---

## Character Section

| Parameter | ID | Type | Range | Default | Notes |
|-----------|-----|------|-------|---------|-------|
| Drive | drive | Float | 0-100% | 20 | Saturation intensity |
| Color | color | Float | -100 to +100 | 0 | Dark (-) to Bright (+) |
| Age | age | Float | 0-100% | 0 | Combined degradation |

---

## Output Section

| Parameter | ID | Type | Range | Default | Notes |
|-----------|-----|------|-------|---------|-------|
| Width | width | Float | 0-200% | 100 | 0=Mono, 100=Stereo, 200=Extra Wide |
| Mix | mix | Float | 0-100% | 50 | Wet/dry blend |
| Focus Low | focus_low | Float | 20-500 Hz | 20 | Processing frequency low bound |
| Focus High | focus_high | Float | 1k-20k Hz | 20000 | Processing frequency high bound |
| Mono-Safe | mono_safe | Bool | Off/On | On | Guarantees mono compatibility |

---

## Advanced (Expandable)

| Parameter | ID | Type | Range | Default | Notes |
|-----------|-----|------|-------|---------|-------|
| Pre-Delay | delay | Float | 0-50 ms | 0 | Spatial depth |
| Feedback | feedback | Float | 0-80% | 0 | Recirculation amount |
| Randomization | random_amt | Float | 0-100% | 15 | Per-voice variation |

---

## Parameter Count Summary

- **Mode:** 1
- **Wobble:** 5
- **Unison:** 4
- **Character:** 3
- **Output:** 5
- **Advanced:** 3

**Total:** 21 parameters

---

## Complexity Indicators

- **Dual engine architecture** (Wobble + Unison with blend)
- **Mono-safe requirement** (phase-coherent processing)
- **Multi-voice pitch shifting** (up to 7 parallel delay lines)
- **Tape wow/flutter modeling** (non-repeating LFO patterns)
- **Character processing** (saturation + filtering + noise)

**Estimated Complexity:** Medium-High (delay-based pitch shifting is well-documented, but dual-engine + mono-safe adds integration complexity)

---

*Draft extracted from BRIEF.md - finalize with UI mockup before Stage 1*
