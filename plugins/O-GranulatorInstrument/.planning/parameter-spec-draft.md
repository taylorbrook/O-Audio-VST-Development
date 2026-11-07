# Parameter Specification (DRAFT): O-GranulatorInstrument

**Status: DRAFT, for Stage 0 planning only.** Derived on 2026-11-07 from `plugins/O-Granulator/.planning/parameter-spec.md` (LOCKED, 220 params) plus the parameter deltas in `BRIEF.md`. The full `parameter-spec.md` comes from mockup finalization (`/start O-GranulatorInstrument` → option 3) and is required before Stage 1.

**Convention carried over:** inherited IDs keep O-Granulator's exact snake_case spelling, ranges, defaults, skews and choice orders. The BRIEF's camelCase names (`ampAttack`, `sN_scanRate`, `recTarget` …) are mapped to snake_case here (`amp_attack`, `sN_scan_rate`, `rec_target`). The O-Granulator "Conventions (binding for Stage 1)" section applies unchanged: `ParameterID {id, 1}`, no lambda ranges, `setSkewForCentre` for log-like rows, no parameter writes another parameter from a listener.

## Total Parameter Count (provisional)

**Total: 261**

| Group | Count | vs O-Granulator |
|-------|-------|-----------------|
| Global / voice | 9 | was 8 (−input_gain, −buffer_length, −mix, −scale_root; +polyphony, voice_mode, glide, pitch_bend_range, mpe_enable) |
| Amp envelope | 5 | new |
| Mod envelopes (2 × 4) | 8 | new |
| Per stream (38 × 4) | 152 | was 128 (−onset_trigger; +7 per stream) |
| Record-in | 7 | new |
| Feedback network | 5 | unchanged |
| Output reverb | 3 | unchanged |
| LFOs (5 × 4) | 20 | was 16 (+lfoN_mode) |
| Macros | 4 | unchanged |
| Modulation matrix (3 × 16) | 48 | unchanged shape; new source and destination lists |
| ~~Env / pitch / onset followers~~ | 0 | −8 (removed) |
| **Total** | **261** | |

## Parameter Definitions

### Global / voice (9)

| ID | Type | Range / choices | Default | Skew | Description |
|----|------|-----------------|---------|------|-------------|
| `polyphony` | Float (interval 1) | 1 … 16 voices | 8 | linear | Most notes that sound at once. A new note past the limit steals the oldest or quietest voice with a fade. |
| `voice_mode` | Choice | 3: Poly, Mono, Legato | 0 (Poly) | — | Mono and Legato play one cloud that glides between notes. Legato does not retrigger the envelopes on overlapping notes. |
| `glide` | Float | 0 … 2000 ms | 0 | 0.3 | Portamento time. Applies in Mono/Legato, and to poly voices when non-zero. |
| `pitch_bend_range` | Float (interval 1) | 0 … 48 st | 2 | linear | Pitch bend range. In MPE mode it is the per-note bend range. |
| `mpe_enable` | Bool | Off / On | Off | — | Treats MIDI channels 2–16 as per-note channels (MPE lower zone): per-note bend, pressure and slide. |
| `grain_limit` | Choice | 5: 32, 64, 128, 256, 512 | 3 (256) | — | Most grains that can sound at once, shared by all voices and streams. **Changed** from O-Granulator's 4-entry list (32–128). |
| `freeze` | Bool | Off / On | Off | — | Stops feedback writes into the working copies. **Changed** meaning: O-Granulator froze the live buffer. |
| `sync_mode` | Choice | 2: Free, Tempo | 0 (Free) | — | Inherited. Also selects `rec_length` vs `rec_length_sync`. |
| `output_gain` | Float | -24 … 12 dB | 0 | linear | Inherited. |

**Removed:** `input_gain`, `mix` (an instrument has no dry path), `buffer_length` (replaced by the per-slot file length and the per-stream loop region), `scale_root` (it only served Pitch Follow; the played key is never quantised).

### Amp envelope, per voice (5)

| ID | Type | Range | Default | Skew | Description |
|----|------|-------|---------|------|-------------|
| `amp_attack` | Float | 0 … 10000 ms | 20 | log-like (centre ≈ 300 ms) | Voice fade-in. |
| `amp_decay` | Float | 0 … 10000 ms | 300 | log-like (centre ≈ 300 ms) | |
| `amp_sustain` | Float | 0 … 100 % | 100 | linear | |
| `amp_release` | Float | 0 … 20000 ms | 1500 | log-like (centre ≈ 500 ms) | Voice tail. Grains keep spawning during release. |
| `vel_to_amp` | Float | 0 … 100 % | 70 | linear | Velocity sensitivity of the voice level. |

A 0 ms floor must still get a minimum de-click ramp in the DSP (a few ms), not a step.

### Mod envelopes — template `menvN_` (N = 1, 2; 4 × 2 = 8)

| ID | Type | Range | Default | Skew | Description |
|----|------|-------|---------|------|-------------|
| `menvN_attack` | Float | 0 … 10000 ms | 10 | as `amp_attack` | Per-voice modulation envelope attack. |
| `menvN_decay` | Float | 0 … 10000 ms | 500 | as `amp_decay` | |
| `menvN_sustain` | Float | 0 … 100 % | 0 | linear | |
| `menvN_release` | Float | 0 … 20000 ms | 500 | as `amp_release` | |

### Per stream — template `sN_` (38 per stream × 4 = 152)

Inherited rows keep O-Granulator's definitions **verbatim** (see its spec): `sN_enable` (s1 On, s2–s4 Off), `sN_mute`, `sN_solo`, `sN_level`, `sN_pan`, `sN_spray`, `sN_size`, `sN_size_sync`, `sN_size_rand`, `sN_density`, `sN_density_sync`, `sN_jitter`, `sN_pitch`, `sN_pitch_rand`, `sN_scale` (32 entries, same order), `sN_reverse_prob`, `sN_window`, `sN_window_skew`, `sN_stereo_spread`, `sN_gain_rand`, `sN_filter_type`, `sN_filter_cutoff`, `sN_filter_rand`, `sN_filter_res`, `sN_drive`, `sN_crush`, `sN_crush_prob`, `sN_ring_freq`, `sN_ring_rand`, `sN_ring_mix` (30 rows).

Changed and new rows:

| ID | Type | Range / choices | Default | Skew | Description |
|----|------|-----------------|---------|------|-------------|
| `sN_position` | Float | 0 … 100 % | 0 | linear | **Changed:** start point within the loop region (O-Granulator: distance behind the write head). |
| `sN_scan_rate` | Float | -400 … 400 % | 0 | linear | **New.** Playhead speed through the file. 0 = static, 100 = real time, negative = backwards. |
| `sN_scan_mode` | Choice | 3: Free-run, Retrigger, Ping-pong | 1 (Retrigger) | — | **New.** Retrigger restarts each note's scan at Position. Free-run shares one running playhead across notes. Ping-pong bounces between the loop points. |
| `sN_loop_start` | Float | 0 … 100 % | 0 | linear | **New.** Start of the region the scan and spray stay inside. |
| `sN_loop_end` | Float | 0 … 100 % | 100 | linear | **New.** End of that region. Start > End is ordered in the DSP, not by writing the other parameter. |
| `sN_root_note` | Float (interval 1) | 0 … 127 (C-1 … G9) | 48 (C3) | linear | **New.** The key at which the file plays untransposed. Displayed as a note name. |
| `sN_root_fine` | Float | -100 … 100 ct | 0 | linear | **New.** Fine tuning of the root, for samples that are not at concert pitch. |
| `sN_key_track` | Float | 0 … 100 % | 100 | linear | **New.** How much the played key transposes the grains. 0 = fixed pitch (drone). |

**Removed:** `sN_onset_trigger` (no live input to follow). `sN_pitch` is now an offset on top of key tracking. `sN_scale` quantises only the Pitch Random spread, counted from unison; the key itself is never quantised.

### Record-in, sidechain (7)

| ID | Type | Range / choices | Default | Skew | Description |
|----|------|-----------------|---------|------|-------------|
| `rec_target` | Choice | 4: Slot 1, Slot 2, Slot 3, Slot 4 | 0 | — | The slot that captures. |
| `rec_arm` | Bool | Off / On | Off | — | Arms capture. The run state (Armed → Recording → Done) is engine state, not a parameter. |
| `rec_trigger` | Choice | 3: Manual, Threshold, Next Note | 0 (Manual) | — | When an armed capture starts: immediately, when the sidechain crosses the threshold, or on the next note-on. |
| `rec_threshold` | Float | -60 … 0 dB | -30 | linear | Level that starts a Threshold capture. |
| `rec_length` | Float | 0.5 … 30 s | 4 | 0.5 | Capture length (Free sync mode). |
| `rec_length_sync` | Choice | 6: 1 bar, 2 bars, 4 bars, 8 bars, 16 bars, 32 bars | 2 (4 bars) | — | Capture length in bars (Tempo sync mode). Capped at the slot maximum. |
| `rec_gain` | Float | -24 … 12 dB | 0 | linear | Sidechain input level. |

### Feedback network (5) and output reverb (3)

Inherited **verbatim**: `feedback`, `fb_low_cut`, `fb_high_cut`, `fb_saturation`, `fb_diffusion`, `reverb_mix`, `reverb_size`, `reverb_damp`.

**Changed semantics:** feedback writes into each stream's *working copy* at that stream's current scan position, not into a live ring. The voice sum feeds back, so the loop gain needs a voice-count-aware safety limit (Stage 0 to specify; O-Granulator Phase 3.3 D34 "live loop gain g_knob × AGC" is the starting reference).

### LFOs — template `lfoN_` (5 × 4 = 20)

Inherited verbatim: `lfoN_shape`, `lfoN_rate`, `lfoN_rate_sync`, `lfoN_phase`. New:

| ID | Type | Range / choices | Default | Description |
|----|------|-----------------|---------|-------------|
| `lfoN_mode` | Choice | 2: Global, Per-voice | 0 (Global) | Per-voice LFOs restart at `lfoN_phase` on each note-on. Global LFOs run free (transport-restarted, as O-Granulator). |

### Macros (4)

Inherited verbatim: `macro1` … `macro4`.

### Modulation matrix — template `modM_` (3 × 16 = 48)

Same shape as O-Granulator (`modM_source`, `modM_dest`, `modM_depth`, bipolar depth, `base + depth × source` on the normalised value, never written back to the APVTS).

**`modM_source` (18 entries, new list):** 0 None, 1–4 LFO 1–4, 5 Mod Env 1, 6 Mod Env 2, 7 Velocity, 8 Key, 9 Note Random, 10 Mod Wheel, 11 Pressure (channel AT, poly AT or MPE pressure, whichever the note receives), 12 Slide (CC74), 13 Bend, 14–17 Macro 1–4.

Per-voice sources: Per-voice LFOs, Mod Env 1–2, Velocity, Key, Note Random, Pressure, Slide, Bend. Global sources: Global LFOs, Mod Wheel, Macros.

**`modM_dest` (provisional, 126 entries, append-only once finalized):** this is a new table. It does not reuse O-Granulator's indices, because its input_gain / mix / buffer_length rows no longer exist.
- 0 `None`, 1 `output_gain`
- Per stream, 28 each (s1 = 2…29, s2 = 30…57, s3 = 58…85, s4 = 86…113): the 24 O-Granulator per-stream destinations in their original order (`level` … `ring_mix`), then `scan_rate`, `loop_start`, `loop_end`, `key_track`
- 114–118 feedback ×5, 119–121 reverb ×3, 122–125 `lfo1_rate` … `lfo4_rate`

**Open (Stage 0):** how a per-voice source drives a global destination (feedback, reverb, output, global LFO rate). The options are: last-triggered voice, loudest voice, or disallowed (dimmed in the UI).

## Non-parameter state (ValueTree)

Inherited unchanged: `customScale.*`, `degreeMasks[4][32]` (16-hex strings), `seed` (int64), `stateVersion`, `modDestTableVersion` (= 1 for the new table), `uiLanguage`.

New:

| Key | Type | Notes |
|-----|------|-------|
| `slotN.source` | String | `factory:<id>`, or an absolute path plus a path relative to the session/preset. Missing → the UI shows "relink". |
| `slotN.embedded` | base64 FLAC blob (optional) | Only when the user opts in, and only up to a size cap (Stage 0 to set it). Recorded-in slots have no file, so they need this or a saved-to-disk path. |
| `slotN.name` | String | Display name. |
| `slotN.workingCopyDirty` | — | **Not saved.** On reload the working copy restarts from the pristine file. |

**Actions (not parameters):** Reset working copies (all, or per slot), Load / Relink / Clear slot, Save recording to file. These are UI → native-function calls. They are not automatable Bool parameters, which avoids the parameter-writes-parameter trap and host-automation surprises.

## Open questions for Stage 0

1. Share the O-Granulator core as a module, or fork it (BRIEF Technical Notes; O-Granulator is mid Stage 3).
2. Maximum slot length and sample rate. Memory is pristine + working copy, × 4 slots, stereo float.
3. CPU budget at 16 voices × 4 streams, and whether `grain_limit` 512 is affordable or the list should stop at 256.
4. Per-voice source → global destination rule (above).
5. Whether to add Dorico microtonal note expression (O-Lyrica pattern) in v1.
6. AU instrument + sidechain support per host. Record-in must degrade cleanly when the sidechain bus is unrouted (it delivers silence, not an error).
