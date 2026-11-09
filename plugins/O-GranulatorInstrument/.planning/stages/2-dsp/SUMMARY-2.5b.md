---
plugin: O-GranulatorInstrument
stage: 2
phase: "2.5b"
title: "Mono / Legato / glide, sustain, MPE lower zone, VST3 note expression"
executed: 2026-11-09
version: 0.1.0 (unchanged, PD20)
fork_pin: 82406fce
commits: C1 e528c48a (engine: voice modes + sustain + glide + D32 + MPE + NE path) + C2 (gates, hands-on, this summary)
result: executed — every automated gate green; S8 (FL7 re-scope) hit and resolved by the user; HANDS-ON-2.5b written and deferred with 2.1–2.5a
---

# O-GranulatorInstrument — Stage 2 / Phase 2.5b SUMMARY

The instrument now plays Poly / Mono / Legato with glide, honours the sustain pedal, speaks MPE (lower zone) and takes VST3 per-note tuning by note ID, live and sample-stamped. With Poly, glide 0, MPE off and no pedal, every 2.5a artefact is unchanged bit for bit.

There is no parent equivalent (O-Granulator is an effect), so the proof is law gates, same-motion click gates, the block-size null, the allocation gate and true bypass.

## What was built

- **Grid (T3).** `ControlGrid.h` (ADAPTED) binds `voice_mode`, `glide` (raw ms through the skewed range) and `mpe_enable`, latched at the tick. FORK-MANIFEST col 5 re-hashed (`558cb26a…`), col 7 extended. `readSet25b` = 254 bound / 7 unread (rec_* only).
- **Plumbing (T4).**
  - `Voice` gains `keyDown` (physical key; `isKeyDown()` reads it), `noteId`, `neSemis`, `mpeSemis`, `serialNote`, and the closed-form glide state `{glFrom, glTo, glT0, glN}`.
  - `AmpEnv::retrigger (p, fs, newGain, ms)`: attack from the current level, velocity gain on a sample-counted linear ramp (exactly `newGain` at N).
  - `GiNoteStack.h` (new, cap 128, `{channel, note, noteId, neSemis}`, push-to-top, drop-oldest counted).
  - `voiceSemis (v, sp, t)`: the 2.5a key, **then** `+ g(t) + mpeSemis + neSemis`, each added only when non-zero (bitwise in bypass). Every caller passes a sample (`tAbs` at spawn, `nowT` at the tick / event).
  - `fillLawKey` appends `pvSemis[20]` = glide + member bend + NE + (note − serialNote) for counted voices; `kLawKeySize += 20`.
  - Before / after: `voiceSemis` was `note − root − fine/100 + bend·range`; it now appends the three terms. `fillLawKey` had 4 × 16 + bend + range + 3 hooks; it now has the 20 per-voice terms before the hooks.
- **Sustain (T5).** CC64 ≥ 64 holds the gate; a note-off under the pedal clears `keyDown` only. Pedal-up releases every `gated && ! keyDown` voice at that sample. PD9 (USER) Poly re-strike = release-with-tail of the pedal-held voice on the same (channel, note). CC121 lifts the pedal; CC123 releases every open gate ignoring the pedal and clears the stack; CC120 forces and clears.
- **Voice modes (T6).** `noteOnMono` / `noteOffMono` per R§2.1 + PD2 / PD5. `retriggerVoice` (new serial, envs + Mod Envs from the current level, 5 ms gain glide, scan / scheduler / Tempo restart, per-voice LFOs at `lfoN_phase`, NR / velocity / Key, PD4 level / pan residuals, `voicePass (k, true)` + `buildPairViews`) — never `startVoice`. `moveCloudPitch` (Legato / D30): glide retarget, note / channel / noteId / NE follow the key, Key source follows (PD3). Glide is the closed form from absolute time (PD13), including Poly glide-from-last.
- **D32 (T7).** At the tick where the latched mode changes: Poly → Mono / Legato keeps the max-serial counted voice, force-releases the rest (5 ms), rebuilds the stack from physical keys in serial order, sets `detached` (PD5). Other switches: no voice action.
- **MPE (T9).** `GiMpe.h` (new): per-channel bend / pressure / slide (written always, read only with MPE on), `memberRange` (48), `memberCount` (15), `juce::MidiRPNDetector` policy (RPN 0 member → range with cents; RPN 0 on ch 1 ignored; RPN 6 on ch 1 → MCM + range 48; NRPN / null ignored). Routing per PD8 / D31; the 2.5a write paths are untouched with MPE off.
- **NE stamped path (T10).** `GiVst3Extensions::onVst3RawEvent` calls the module first, then copies into a fixed 64-entry array (drop counted). `processBlock` converts to `ogi::GiNeEvent` and passes them to `Engine::process`; a new `processBlockBypassed` override drops a bypassed block's events (P6). The engine pre-registers raw NoteOns for correlation, merges NE values with MIDI by clamped offset (MIDI first at a tie), and writes `semis = 240·(value − 0.5)` (`kTuningTypeID` literal 2, P7) to every voice and stack entry with that noteId. A running glide keeps its start and moves its target with the tuning. Module untouched (`git diff --quiet -- modules/tuning/note-expression`).
- **Threading fact (T10, S8 check).** `onVst3RawEvent` runs on the audio thread inside the same `process (ProcessData&)` call, before `processAudio → processBlock / processBlockBypassed` (`juce_audio_plugin_client_VST3.cpp:3588-3640 → :3779`, patched 8.0.15). Not an S8.
- **PD16 tie order.** `MidiCursor` is unchanged: off-then-on at one sample is a retrigger, on-then-off is legato. Deterministic and block-size invariant (FK3).

## Results

| Gate | Result |
|---|---|
| Build | harness, param-dump, VST3, AU, Standalone rc 0; **0 warnings** |
| Checkpoints | renders-2.4 **byte-identical** and legacy == renders-2.2 at T4, T7, T10, T14. Full harness 231/0 at T10 (with FL7 re-scoped). Nothing re-pinned |
| Default harness | **300 PASS / 0 FAIL**, **0 NOT FIRED** (231 + 33 eg25c + 36 eg25d). Duplicate-ID set == the 2.5a set (no new duplicates after the FK / VS rename) |
| `--parity` | **168 / 0**, `--rows GV` **2 / 0**; every residual line identical to T1 |
| param-dump | byte-identical to T1 |
| Drift | 0 / 13; NC-a (byte on Grain.h, parent root) rc 1, NC-b (byte on FeedbackChain.h, local root) rc 1 |
| X1 | 0 parent references in the compdb (332 entries) |
| `--cpu25a` (alone) | **C25m 28.04 %** (≤ 40, witnesses valid: PV dests 16 / tick, recomputes 100 %, pair-mode 4, live 243.2); **C25tm 6.63 %** (≤ 12, live 80.2). C25w 26.79 %, C25t 5.70 %. Δ vs T2 (29.60 / 6.96 / 28.04 / 5.91): −1.56 / −0.33 / −1.25 / −0.21 points (machine noise; no regression) |
| `--cpu25b` (alone) | **C25mpe 13.28 % INFO** (16 notes on 15 member channels, member bends at 0.5 Hz, Poly glide 300 ms re-struck every 500 ms); witness law recomputes 100 % of ticks, 300 400 member writes |
| T17 | `build-and-install.sh` rc 0 (dual-variant sweep). Standalone rebuilt explicitly (newer than the harness), alive after 6 s. **auval PASS** `[1, 2] [2, 2]` (ZZZZ control rc 2). **pluginval 10 VST3 SUCCESS** (0 `!!!`) + **AU SUCCESS** (1 `!!!`: pluginval's generic "current program is -1" warning). **X2** 0 `ogran::` in the installed VST3, AU and the harness |

### eg25c (33 / 0) — voice modes, glide, switch, sustain
- **VM1** serial 1 → 2, |Δ amp| 1.3e-4, |Δ menv| 1.3e-4, σ 0 at t, LFO 1 == the note-on phase value, NR redrawn, velocity 0.8976. **VM1n** ampResetZero Δ 0.4998.
- **VM2** gain linear to g_v (0.9) within 3.0e-8 over 272 samples (step 0.451). **VM2n** gainSnap 0.449.
- **VM3** Legato: serial / env / gain / NR / velocity / LFO / scan bitwise == a single-note render; note 64, Key 0.0625. **VM3n** legatoAsMono differs.
- **VM4** 4 / 4 (tail retrigger on the same slot from the current level; fresh voice from 0; both modes).
- **VM5** lawGain / gFbLerp bitwise across the retrigger. **VM5n** retrigViaStartVoice snaps 0.153 → 0.354.
- **VM6** max per-cell gain step 0.084 ≤ 1.05 · B (B 0.119; natural 0.075). **VM6n** noPvResidual 1.32.
- **FK1** both modes: serial kept, note 60, stage unchanged, fallbacks 1. **FK1n** fallbackRetrig mints a serial. **FK2** 3 / 3. **FK3** off-first retrigger / on-first legato, 32 vs 4096 0 diffs.
- **GL1** worst 0 st over 196 grains (≤ 1e-9). **GL1n** 24 st. **GL2** N 14400.003 both. **GL3** continuity 0 st, new t0 / N. **GL4** second voice starts at 60. **GL5** glide state untouched, exact jump. **GL6** lawTarget == fresh pairLaw (rel 0) over 3099 ticks, 852 target changes. **GL6n** pvKeyOff 1.31.
- **VS1** counted 1 (max serial), 15 forced, 1 left after 1 s, stack 16. **VS2** 0 diffs before the first new note, 4 counted after. **VS3** 0 diffs. **VS4** 3 / 3 (detached; held-key off keeps the cloud; next note-on retriggers its slot).
- **SU1** release starts at the pedal-up sample. **SU2** victim = the sustained voice. **SU3** old voice releasing, new voice fresh, restrikeReleases 1, ch 2 untouched. **SU4** 3 / 3. **SU5** 3 / 3. **SU6** no stuck voice, no NaN.

### eg25d (36 / 0) — MPE, NE, clicks, invariance, bypass
- **MP1** ch 3 worst 0 st over 98 grains; ch 2 / 4 grain logs bitwise. **MP1n** mpeBroadcast moves them. **MP2** 7 / 7 (48 → 12 → 12.5; NRPN / null / ch 1 ignored; 2 RPN messages). **MP3** 0 st error (49.500122070). **MP4** ignored under MPE, moved with MPE off. **MP5** initial values exact. **MP6** 3 / 3. **MP7** 3 / 3. **MP8** 3 / 3.
- **MR1** 5 / 5. **MR2** ch 2 offset 0 → 0.5, ch 3 bitwise. **MR3** 0.236 → 0.787. **MR4** slot offset 0 vs 7: 0 diffs.
- **NE1** worst 0.000 c over 48 grains. **NE1c** no NE / noteId 99: 50.000 c away. **NE2** 441 / 4096: 0.000 c before and after a mid-block change. **NE2n** neModuleTable: 70 c off after the change; one-pitch collision 80 / 30 c. **NE3** 0.000 c each. **NE4** start continuous at 59.70, end 64.25, fallback 59.70. **NE5** 64 applied, 6 dropped, 0 allocs, 0 replayed after a bypassed block.
- **Click gates (local κ vs same-motion pre / post references):**

| Row | κ ratio | Hard control |
|---|---|---|
| CK5a Mono retrigger | 1.285 | CK5an ampResetZero 337× |
| CK5b Legato glide | 0.673 | none constructible (bound only) |
| CK5c D30 fallback | 1.012 | none constructible (bound only) |
| CK5d D32 switch | 1.000 | CK5dn modeSwitchHard 1279× |
| CK5e pedal-up | 1.028 | CK5en pedalUpHard 511× |
| CK5f member bend sweep | 0.585 | none constructible (bound only) |
| CK5g Mono velocity change | 1.542 | CK5gn gainSnap 210× |

- **BS1** 441 / 512 / 4096 / var: 0 diffs vs 32. **BS1n** seed + 1: 78 998 differ.
- **AL1** 0 allocations over 104 blocks (retriggers 132, legato 1, fallbacks 3, switches 4, deferred 6, restrikes 1, RPN 2, member writes 180, NE 67 applied / 6 dropped, stack dropped 2); liveness 1.
- **TB5** 14 / 14 + legacy 14 / 14. **TB6** 14 / 14 with RPN / NRPN traffic on all 16 channels; any-channel source rule 2 / 2.

## Plan decisions as executed
PD1–PD20 as written, except the deviations below.

## Deviations (each with its reason)
1. **FL7 re-scoped (S8, user 2026-11-09).** eg24's FL7 still randomised `voice_mode` / `glide` / `mpe_enable` (readSet25a) and failed once the engine read them (287 998 samples). Moved to readSet25b 254 / 7 like N2 / N2s / FN3.
2. **Checkpoint grouping.** Checkpoints ran at T4 (T3 + T4), T7 (T5–T7, + full harness) and T10 (T9 + T10, + full harness), not after every task. All held, so nothing needed bisecting.
3. **C1 scope.** C1 (`e528c48a`) covers T3–T10 (the MPE and NE engine path were written while the T7 run was in flight), not T3–T7. It was checkpointed (renders byte-identical) and the full harness was 231 / 0 before the commit.
4. **T2 ordering.** The T3 ControlGrid edit was written before `--cpu25a` ran, but the measured binary was the unmodified 2.5a build (copied aside, built at `98836951`); no rebuild happened before T2 finished.
5. **Row IDs renamed.** Plan FB1–FB3 / FB1n → **FK1–FK3 / FK1n** and MS1–MS4 → **VS1–VS4**: FB is the 2.3 FX-state family and MS1 the 2.4 mip-settle row (the plan forbids new duplicate IDs).
6. **CK5 references.** Same-motion = the max of the held pre / post renders (the 2.5a RE form), not a static pre-only render: a pitch move alone raises a tone's local κ by f² (static-only read 2.1–6.1× on CK5a/b/d/f with no click).
7. **GL6 stimulus.** Glide 47 → 49 over 2 s, not 2 octaves: the coherence pitch term is non-zero only within 0.5 st of the scan pitch (`kCoherP0St`), so a 2-octave glide crosses it in ~9 ticks. VM5 uses 48 → 49 for the same reason.
8. **MP1 sweep ±2000** (|b| ≤ 0.244): ±8000 drove 16 + 48 b past the ±48 st spawn clamp.
9. **NE glide target.** An NE value on a gliding voice shifts `glTo` with the tuning (start continuous, ends on the tuned pitch). Not in PD6's text; needed for NE4 "glides to the tuned pitch".
10. **Test-only probes** added under `OGI_TEST_HOOKS`: `getVoiceSemisForTest`, `getLawTargetForTest`, `getLawFreshForTest`, `getLawLerpForTest`, `getPairGainForTest`, `getStackForTest`, `getMpeForTest`, `getSustainForTest`, `getDetachedForTest`, `getMonoCloudForTest`.
11. **NE flush / suspended blocks.** A host parameter flush (`numSamples == 0`) or a suspended processor skips `processBlock`, so raw events captured in that call carry into the next block at clamped offsets. Rare; documented, not gated.
12. **Bypass-safe allNotesOff.** CC123 now releases every `gated` voice (was `isKeyDown`); identical without CC64.

## Requirements moved (verify sets the final status)
FUNC-05 automated arm complete (feel = HANDS-ON-2.5b). DSP-06 MPE arm in (automated arm complete). PERF-01 MIDI / MPE / NE allocation arm (AL1, NE5). QUAL-01 voice-mode / sustain / MPE click gates (CK5). COMPAT-02 stays Stage 4.

## HANDS-ON-2.5b
Written (11 rows), deferred post-Stage-3 with 2.1–2.5a. Not self-certified.
