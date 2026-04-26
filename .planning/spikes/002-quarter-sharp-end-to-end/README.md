---
spike: 002
name: quarter-sharp-end-to-end
validates: "Given a Dorico quarter-sharp accidental, when the note plays through O-Lyrica-dev, then emitted pitch is +50¢ ±5¢ above 12-TET reference"
verdict: VALIDATED
related: [001-patch-build-load, 003-attack-transient-check]
tags: [dorico, microtonal, end-to-end]
---

# Spike 002: quarter-sharp-end-to-end

## What This Validates

**Given** a Dorico score with a quarter-sharp accidental applied to a note, routed to O-Lyrica-dev via VST3,
**when** the note plays,
**then** the emitted pitch is **+50¢ (±5¢) above the unaltered reference pitch**.

A successful result simultaneously proves:
- Dorico discovered `INoteExpressionController` on O-Lyrica-dev (if it hadn't, it would've fallen back to pitch bend, which O-Lyrica ignores → no offset at all, or wrong offset).
- The JUCE patch is correctly forwarding `kNoteExpressionValueEvent` events to `VST3ClientExtensions::onVst3RawEvent`.
- The processor's NE → noteId → pitch correlation works.
- The voice-side `pendingTuningSemis.exchange` applies the delta before the first sample renders.

## How to Run

### Setup — configure Dorico tonality system with quarter-tones

1. Open a new Dorico project: `File → New from Template → Solo → Piano`.
2. Go to **Play mode**, switch the Piano track's VST to **Ouaricon Audio → O-Lyrica-dev**.
3. Back in **Write mode**, select the whole bar (or the first beat).
4. Open the tonality system panel: `Library → Tonality Systems…`
5. Add the preset **24-EDO Equal Temperament** to the current project (or pick an existing preset that includes quarter-tone accidentals, e.g. "24-EDO").
6. In the Key Signatures panel (right side), set the key signature for the current flow to a 24-EDO key (C major 24-EDO is fine).

### Enter the test notes

Two notes back-to-back on the piano staff:

- **Bar 1, beat 1:** Plain C4 quarter note. This is your 12-TET reference.
- **Bar 1, beat 2:** C4 quarter note with a **quarter-sharp** accidental (Write → Accidentals panel → "¼♯" or `Shift+Alt+=` for +¼-tone, depending on your key map).

Both notes should appear on the same line (C4) — the second one has the ¼♯ glyph in front.

### Measure the pitch offset

Pick one of these three methods:

**Method A — Ear + tuner plugin (fastest, least precise)**
1. Load a free tuner in Dorico's effect chain (VST3 Effects → pick any chromatic tuner — e.g., GTune, Voxengo Tuner, MTuner).
2. Solo the track, play each note, read the cents offset on the tuner.
3. C4 plain should read ~0¢; C4 quarter-sharp should read ~+50¢.

**Method B — Export audio + spectrum analysis (most rigorous)**
1. `File → Export → Audio…` → export the 2-note snippet as a WAV.
2. Save it at: `/Users/taylorbrook/Dev/VST-development/.planning/spikes/002-quarter-sharp-end-to-end/test-output.wav`
3. Tell me the path is ready — I'll run FFT-based pitch analysis and report the measured offsets.

**Method C — DAW record + tuner (hybrid)**
1. Set up a DAW (Logic, Reaper, Live) that can host VST3 synths AND receive MIDI from Dorico (via IAC Bus or virtual MIDI routing).
2. Load O-Lyrica-dev in the DAW, route Dorico's MIDI output to it, put a tuner on the DAW track.
3. Play from Dorico, read tuner in DAW.

**Recommended: Method B** — gives a precise number, removes operator error, and the wav file becomes spike evidence.

## What to Expect

- Plain C4 (MIDI note 60): fundamental at ~261.63 Hz (A440 tuning).
- Quarter-sharp C4: fundamental at ~269.29 Hz (261.63 × 2^(0.5/12) = 2^(50/1200) ratio).
- Measured offset: **+50.0¢ ±5¢**.

| Measured offset | Verdict |
|---|---|
| +45¢ to +55¢ | ✓ VALIDATED — pipeline works end-to-end |
| 0¢ (no offset) | ✗ Dorico chose pitch bend (NEC not discovered) or our queue isn't receiving events — debug needed |
| Non-zero but wrong magnitude | ⚠ PARTIAL — data path works but value math is wrong (normalization, semis vs. cents, etc.) |
| Audible wrong pitch, can't measure | Same as "non-zero" — need the actual number |

## Results

### Measured (2026-04-23, fresh trace after expression-map fix)

| Event | Value |
|---|---|
| Plain C4 NoteOn | `noteId=3 pitch=60` — no NE event — 12-TET baseline |
| Quarter-sharp C4 NoteOn | `noteId=4 pitch=61` (Dorico picked the *upper* semitone) |
| NE event | `typeId=2 (kTuningTypeID) noteId=4 value=0.497917` |
| Semitones applied | `240 × (0.497917 - 0.5) = -0.5000` |
| Voice frequency transform | `277.183 Hz → 269.292 Hz` |
| Expected (C4 + 50¢) | `261.626 × 2^(50/1200) = 269.292 Hz` |
| **Match** | **Exact** (within float precision) |

### Surprise 1 — Dorico chooses pitch representation

For a quarter-sharp C4, Dorico did *not* send `pitch=60, NE=+50¢`. It sent `pitch=61, NE=-50¢`. Final frequency is correct either way, but this means:
- Plugins must not infer accidental direction from NE sign.
- `noteId`-based correlation (our approach) is robust; MIDI-pitch-based correlation would need to handle Dorico's neighbor-semitone choice.
- Test matrix for real build needs to cover quarter-flat (likely `pitch-1, NE=+50¢`), ¾-sharp, ¾-flat etc.

### Surprise 2 — Dorico does not query INoteExpressionController

Across 2103 log lines and thousands of `queryInterface` calls, Dorico never queried for the NEC IID. NE events flow purely based on the expression map's "VST3 Note Expression" setting — the NEC advertisement is apparently ignored by Dorico 6.

Implication: **end-user UX friction.** Users must manually duplicate their expression map and set Microtonality to "VST3 Note Expression" before O-Lyrica (and any future Ouaricon microtonal plugin) produces quarter-tones. Dorico's "Auto" mode routes non-Steinberg VST3s to pitch bend silently. Real build must ship clear setup instructions, ideally with a Dorico expression map file in the installer.

Our `TuningNoteExpressionController` is dead code for Dorico but should stay — other VST3 hosts may use it, and it's ~80 LOC with no runtime cost.

### Verdict

**VALIDATED.** The JUCE patch + NEC + voice-side apply pipeline produces correct microtonal pitch offsets from Dorico. End-to-end latency is one block (NE arrives at `sampleOffset=0` same as NoteOn, voice picks it up in `startNote`). Ready for Spike 003 (attack transient check).
