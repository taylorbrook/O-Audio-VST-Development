# O-simpleBeatmaker — Parameter Specification (FINAL)

> **Status:** FINAL. Promoted from `parameter-spec-draft.md` and reconciled with
> the immutable `research/ARCHITECTURE.md` → *Parameter Mapping* (lines 162–188),
> which resolved every draft open question during Stage 0. This file is the
> APVTS contract for Stage 1 (Foundation) and is referenced by Stages 2–3.
> **Date finalized:** 2026-06-25 (Stage 1 discuss phase).

Plugin type: **Synth (Pedagogical Step-Sequencer Drum Machine)** — MIDI-in → stereo-out, host-synced + MIDI-playable, WebView UI (Stage 3).
Voice roster (RESOLVED): **Kick, Snare, Clap, Closed Hat, Open Hat, Tom** (6 voices). GM map **36 / 38 / 39 / 42 / 46 / 45**.

---

## Storage conventions (foundation-locking — do not change after Stage 1)

These choices are baked into the APVTS layout. Changing a range/ID later breaks
saved DAW sessions, so they are locked here.

1. **Percent params stored normalized `0–1`** (`swing`, `humanize`, `quantizeStrength`,
   per-voice `tone`, per-voice `decay`). The Stage-3 UI scales for display. This
   matches the O-simpleSubtractive/O-simpleFM convention ("store 0–1, UI ×100").
   - `swing01` maps **0–1 → 0–75 %** display. DSP swing ratio is then literal:
     `s = 0.5 + swing01 / 3` (0→0.5 straight, 1→0.75 MPC-max). *(ARCHITECTURE §2)*
   - `humanize01`, `quantize01` map **0–1 → 0–100 %** display.
2. **Voice Tune = ±12 semitones** (LOCKED by user decision, 2026-06-25). Float
   `−12…+12 st`, default 0, consistent across all 6 voices. *(Resolves the
   draft/ARCHITECTURE "semitones OR Hz offset" ambiguity.)*
3. **Voice Decay stored normalized `0–1`** (0 = short, 1 = long); the **per-voice
   ms mapping** (kick boom vs. closed-hat tick differ by an order of magnitude)
   lives in the Stage-2 `DrumVoiceEngine`, not in the param range. Default 0.5 (mid).
4. **dB levels stored `−60…0 dB`**, where `−60 dB` represents `−inf` (silence).
5. **The 6×32 step grid is NOT APVTS** — it is custom `std::atomic<uint8_t>` state
   persisted in a `ValueTree "PATTERN"` child. See *Custom State* below.

---

## A. Sequencer / Timing-Feel (the pedagogical core) — 5 params

| Parameter | ID | Type | Range (stored) | Display | Default | DSP component |
|-----------|----|------|----------------|---------|---------|---------------|
| Swing | `swing` | Float | 0–1 | 0–75 % | 0.0 | TimingFeelEngine — `s = 0.5 + swing01/3`; delays off-beat 16ths; **NOT** scaled by quantize |
| Humanize | `humanize` | Float | 0–1 | 0–100 % | 0.0 | TimingFeelEngine — random ±30 ms timing + ±24 velocity per hit |
| Quantize Strength | `quantizeStrength` | Float | 0–1 | 0–100 % | **1.0** | TimingFeelEngine — `q`; humanize×(1−q); **swing preserved** |
| Pattern Length | `patternLength` | Choice | {8, 16, 32} | steps | **16** (idx 1) | SequencerClock — active steps/bar |
| Tempo | `tempo` | Float | 40–240 | BPM | 120 | SequencerClock — **free-run only** (used when host not playing) |

---

## B. Per Drum Voice (6 voices × 6 params = 36)

Applied identically to each voice in `{kick, snare, clap, closedHat, openHat, tom}`.
IDs are `{voice}` + suffix, e.g. `kickTune`, `closedHatDecay`, `openHatSolo`.

| Parameter | ID suffix | Type | Range (stored) | Display | Default | DSP component |
|-----------|-----------|------|----------------|---------|---------|---------------|
| Tune | `…Tune` | Float | −12…+12 | st | 0 | DrumVoiceEngine — base pitch (±1 octave) |
| Decay | `…Decay` | Float | 0–1 | 0–100 % | 0.5 | DrumVoiceEngine — amp/tail decay (per-voice ms mapped in DSP) |
| Tone / Snap | `…Tone` | Float | 0–1 | 0–100 % | 0.5 | DrumVoiceEngine — snap / body↔noise / brightness (per voice) |
| Level | `…Level` | Float | −60…0 | dB | 0 | Mixer — per-voice level (−60 = silence) |
| Mute | `…Mute` | Bool | 0/1 | — | false | Router/Mixer — exclude from output |
| Solo | `…Solo` | Bool | 0/1 | — | false | Router/Mixer — solo mutes others |

Full ID list (36): `kickTune kickDecay kickTone kickLevel kickMute kickSolo` ·
`snareTune snareDecay snareTone snareLevel snareMute snareSolo` ·
`clapTune clapDecay clapTone clapLevel clapMute clapSolo` ·
`closedHatTune closedHatDecay closedHatTone closedHatLevel closedHatMute closedHatSolo` ·
`openHatTune openHatDecay openHatTone openHatLevel openHatMute openHatSolo` ·
`tomTune tomDecay tomTone tomLevel tomMute tomSolo`.

---

## C. Master — 1 param

| Parameter | ID | Type | Range (stored) | Display | Default | DSP component |
|-----------|----|------|----------------|---------|---------|---------------|
| Output Level | `outputLevel` | Float | −60…0 | dB | 0 | Master trim (`SmoothedValue`) |

**Total APVTS parameters: 5 + 36 + 1 = 42.** ✓ (matches ARCHITECTURE.md)

---

## Custom State (NOT parameters) — the step grid

| State | Representation | Access | Persistence |
|-------|----------------|--------|-------------|
| Step grid + per-step velocity | `std::atomic<uint8_t>[6 × 32]` (0 = off, 1–127 = on@velocity) | UI writes via native fn (`toggleStep`, `setStepVelocity`) on the message thread; audio thread reads via atomic `load` | child `ValueTree "PATTERN"` inside the APVTS state, written in `getStateInformation` / restored in `setStateInformation` |

- Voice row order: **0 Kick, 1 Snare, 2 Clap, 3 Closed Hat, 4 Open Hat, 5 Tom**.
- `patternLength` (8/16/32) selects how many of the 32 columns are active; off-grid
  columns are **retained** (not cleared) so shrinking then re-growing round-trips.
- Restore of invalid/missing PATTERN → empty grid (all zero).
- Rationale (ARCHITECTURE §State Persistence): 384 automatable params would bloat
  the host param list and misrepresent pattern data as knobs. Pattern data ≠ knobs.

---

## Resolved (was "open" in the draft)

- **808 vs 909 flavor per voice** → RESOLVED in ARCHITECTURE §3 voice table
  (Kick 808, Snare 808/909 hybrid, Clap 808, Hats band-passed noise, Tom 808).
- **Swing curve** → 16th-note swing, `s = 0.5 + (swing/75)/3` (MPC-canonical).
- **8th vs 16th swing** → 16th in v1.0; 8th-toggle deferred to v1.1.
- **Free-run tempo** → exposed as `tempo` (40–240), used only when host not playing.
- **Per-voice tone mapping** → one knob per voice, meaning per ARCHITECTURE table.
- **Per-voice pan** → deferred to v1.1 (single stereo bus in v1.0).
