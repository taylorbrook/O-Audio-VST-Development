---
title: "O-GranulatorInstrument Stage 2 (DSP) — Phase 2.5a Execution Plan"
created: 2026-11-09
juce_version: "8.0.15"
phase: "2.5a"
summary: "Eight waves: (W0) pre-flight + baselines (harness 179/0, renders-2.4 / legacy re-dump, fresh parent build + parity 133/0 + GV 2/0) and C25w re-measured alone on the UNMODIFIED 2.4 engine; (W1) plumbing with no behaviour: ControlGrid gains the 89 mod / sync rows + `using GridParams = GiGridParams`, ModSources / TempoTracker compiled VERBATIM, FORK-MANIFEST reclass (drift 0/13), readSet25a re-scopes N2 / N2s / FN3, processor reads the playhead into a TransportSnapshot; (W2) global path: sources (global LFO verbatim, macros, wheel, MIDI source state, GiModEnv), GiModMatrix.h port (126 × 18) with the parent's slot / residual / shadow logic, G-dest application; parity PL / PM / PE rows vs 82406fce with no parent edits → commit C1; (W3) per-voice pass: GiVoiceMod, per-pair views into quotas / law / spawn, pair mixer mode for level / pan, newest-voice rule, lfoN_mode crossfade, UI atomics; (W4) tempo: GiTempoClock (parent absolute-time grid), R45 effective values, power-of-two thinning on the division, note-on burst then grid (USER), per-voice Tempo LFO at tempo rate from note-on; (W5) gates eg25a / eg25b / pg25a, harness 179 → 231, parity 133 → 168; (W6) --cpu25a: C25m ≤ 40 % + C25tm ≤ 12 % with witnesses, lever L0 then the D21 ladder; (W7) install, validators, HANDS-ON-2.5a (deferred), SUMMARY / STATUS, commit C2. Checkpoint (renders-2.4 + legacy byte-identical) after every engine task. No tag; version 0.1.0; no new params; CMakeLists.txt not edited."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-GranulatorInstrument — Stage 2 / Phase 2.5a Execution Plan (Mod sources, per-voice matrix, newest-voice rule, Tempo)

## Goal

DSP-06: every 2.5a source routes through the 16-slot matrix to a continuous destination with bipolar depth, and per-voice sources modulate each voice independently. Tempo sync is live. The final PERF-02 worst-case verdict includes the matrix (D28).

- **Sources (18-entry frozen table, 17 live + None):** LFO 1–4 (Global / Per-voice), Mod Env 1–2, velocity, key, note random, mod wheel, non-MPE pressure / slide / bend, macro 1–4.
- **Matrix:** a line-for-line port of the parent `ModMatrix` sized 126 × 18 (`GiModMatrix.h`), plus a sparse per-voice pass for per-voice sources.
- **Newest-voice rule (D4):** a per-voice source routed to a global destination follows the newest counted voice through a 20 ms ramp from the current value, held when silent.
- **Tempo:** the forked `TempoTracker` (VERBATIM), Tempo divisions, Tempo-mode power-of-two thinning on the division, LFO Tempo sync (ppq phase, global; tempo-rate free run from note-on, per-voice), and a **burst onset at note-on, then the grid** (user decision 2026-11-08).

Proof: parity nulls vs O-Granulator @ `82406fce` on the shared global path (D27 parity arm), the law arm for everything per-voice, route-edit click gates, Tempo grid rows, and the D28 CPU verdict.

**Out of scope** (CONTEXT-2.5a): Mono / Legato / glide, sustain, MPE member routing, RPN 0/6, NE (2.5b); Record-in, `<Slots>`, factory sources (2.6); matrix UI, mod rings, LFO phase UI (Stage 3); the typical verdict on factory presets (Stage 4).

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/CONTEXT-2.5a.md` | D26–D28, Defaults, the gate list. **Binding, not reopened.** D11–D25 still bind. |
| `stages/2-dsp/RESEARCH-2.5a.md` | §1–§11 (cited **R§x**), Summary 1–13, Pitfalls **P1–P18**. Authoritative over ARCHITECTURE where they differ ([correction] tags: ModMatrix port, ARCH §9 thinning, ARCH §16 per-voice level / pan, ARCH §20 reverb dests). This plan's PDs decide every R "Open items" entry. |
| `stages/2-dsp/{SUMMARY,VERIFICATION}-2.4.md`, `PLAN-2.4.md` | What exists: harness 179/0, `--parity` 133/0 + GV 2/0, PB0 −152.0 dB, PN −143.0…−151.3 dB, `--cpu25` C25w 26.59 % / C25t 5.51 %, `ref_ns` (cpu-calib-2.3.txt), the PB enable-at-T_e alignment, local κ (S6), the PD20 checkpoint pattern, hook names. |
| `research/ARCHITECTURE.md` | §2 (MPE off), §4 (Mod ADSR), §9 (Tempo), §19–§21, A5 (D4), Algorithm §1, CPU model, Stage-2 Gate Catalogue. |
| `parameter-spec.md` (LOCKED) + `tests/golden/{params,mod-dests,mod-sources}.tsv` | The 89 params read for the first time; 126-dest / 18-source order. **Not edited.** |
| Parent at the pin (`PIN=82406fce`, `git show "${PIN}:plugins/O-Granulator/<path>"`, brace every variable) | `Source/dsp/ModMatrix.h` (whole file — the port source), `Source/dsp/Engine.cpp` `:92-101` (prepare init), `:222-235`, `:698-794` (G-dest substitutes), `:925-930` (macro ramps), `:983-996` (gp overwrite), `:1013-1033` (P50 Tempo effective values), `:1155-1182` (spawn re-evaluation), `Source/dsp/Engine.h` `:259-284` (`traceDest`, hooks), `Source/dsp/StreamScheduler.h:139-188` (`processTempo`), `tests/render-harness/engine_gates_26b.h:443-480` (MX3 trajectory bound), `.planning/stages/2-dsp/SUMMARY-2.5a.md` (TS1, deviations 3/4/6/10), `SUMMARY-2.6.md:63` (MF8n 2.09×). **Never the parent working tree.** |
| This plugin | `Source/dsp/{Engine.h,Engine.cpp,VoiceAllocator.h,VoiceQuotas.h,AmpEnv.h,GiFxLatch.h,GiScale.h,ScanPlayhead.h,GiConstants.h}`, `Source/dsp/forked/{ControlGrid.h,ModMatrix.h,ModSources.h,TempoTracker.h,EngineConstants.h,OutputLaw.h,FORK-MANIFEST.md}`, `Source/GiParamIds.h`, `Source/PluginProcessor.{h,cpp}`, `tests/parity/{run.sh,parent_render.cpp,ParityRows.h}`, `tests/render-harness/{main.cpp,engine_gates_21.h…24b.h,parity_gates_22.h…24.h,render_hashes.h,cpu_calib.h}`, `tests/golden/{renders-2.1,renders-2.2,renders-2.4}.sha256`, `tests/golden/cpu-calib-2.3.txt`, `tests/fork-drift-check.sh`. |

Plan-time facts (verified 2026-11-09): last code commit under `Source/ tests/ CMakeLists.txt` = `3d68135b`; `Engine.cpp` 2038 lines (`prepare:81`, `process:502`, `gridTick:553`, `updateQuotas:877`, `applyEvent:988`, `startVoice:1052`, `endVoice:1146`, `spawn:1364`, `mixPiece:1869`); `EngineTestHooks` at `Engine.h:202`; processor calls `engine->process` at `PluginProcessor.cpp:209`; `readSet24()` at `engine_gates_21.h:276` (162 bound / 99 unread); ControlGrid sha256 `faa04ef3…` = FORK-MANIFEST col 5; `kModRampMs` 20, `kLfoStream 0x0A1F0000`, `kSchedStream 0x0A5C0000`, `kBurstBit 1<<63` (`EngineConstants.h:163, 242, 240, 232`); the Standalone phony target `OuariconGranulatorInstrument_Standalone` exists.

---

## Plan-phase decisions (resolve RESEARCH open items)

- **PD1 [USER 2026-11-08] Tempo note-on = burst, then grid.**
  - A Tempo-mode stream emits **one** onset at the note-on sample on the reserved burst counter `kBurstBit | n` (`EngineConstants.h:232`, n = per-pair burst index), then the bar-anchored grid from the next slot ≥ note-on (+1 sample — a grid slot that lands exactly on the note-on sample is not doubled: the burst takes it, and the grid starts at the following slot).
  - The burst uses the pair's Tempo-effective size / density and the normal draw slots keyed on its counter, so it is block-size invariant.
  - TS1–TS3 exclude onsets by `counter & kBurstBit` and count them. **TS6** proves the burst (one onset at the note-on sample, then grid). Hook `tempoGridOnly` (parent R41: grid only) is TS6's control and is also the PL / TS parity path where the parent is compared (the parent has no burst).
  - Free mode is unchanged (its existing onset-at-note-on behaviour).
- **PD2 [R Summary 3 adopted] Leaves compiled VERBATIM through an alias.**
  - `ControlGrid.h` ADAPTED region gains `struct Lfo { int shape; float rateHz; float rateSync; float phaseDeg; }` inside `GiGridParams` (exactly the parent field names and types, parent `ControlGrid.h:326-331`) and, at namespace scope after `GiGridParams`, `using GridParams = GiGridParams;`.
  - `ModSources.h` and `TempoTracker.h` are then `#include`d by `Engine.h` unchanged. If either fails to compile against the alias → **S8** (a leaf edit is a manifest decision, never silent).
  - FORK-MANIFEST: `ModSources.h` "ADAPTED (pending 2.5)" → **VERBATIM (rename only; first compiled 2.5a)**; `ModMatrix.h` → **VERBATIM, reference only (ported to `GiModMatrix.h`, not compiled)**; `TempoTracker.h` col 8 → "first compiled 2.5a". `EngineConstants.h` is **not edited**: `kModSources 12` / `kModDests 113` stay at parent values (the leaves depend on them); instrument sizes come from `GiParamIds::kModDests 126` / `kNumModSources 18`. Drift stays 0 / 13 (only ControlGrid.h re-hashed).
- **PD3 [R§2.2 adopted] `GiModMatrix.h` = a port, not a wrapper.**
  - New header `Source/dsp/GiModMatrix.h` (`ogi::GiModMatrix`), a line-for-line port of the parent `ModMatrix::{prepare,tick,apply,offsetAt,writeTelemetry}` sized `GiParamIds::kModDests` × `kNumModSources`, with the parent's statement order: slot compare → residual `reset (current + lastContrib)` → 0 over `rampCells`; depth restarts at 0; depth `setTarget` + `tick`; sum before clamp; non-finite → 0; canonical +0 (a `−0` sum = unrouted); `apply = convertFrom0to1 (clamp (convertTo0to1 (clamp (raw)) + off, 0, 1))`; `offsetAt` lerp.
  - A `touched` list (≤ 16 slot dests + live residuals) so only routed dests roll. Hooks keep the parent names: `noResidual`, `noDepthRamp`, `fullRoundTrip`.
  - Ranges: `NormalisableRange<float> ranges[126]` copied once from the processor's `AudioParameterFloat::getNormalisableRange()` on the message thread (`Engine::setDestRanges`, called from the processor constructor and `prepareToPlay`), never on the audio thread (P15). Choice / bool dests are not in the 126 table.
  - The dest index table comes from `GiParamIds` (`GiParamIds.h:445-478`); the source order from `mod-sources.tsv`. A static self-check row (MX0, inside DR1) asserts every `mod-dests.tsv` id resolves to a float param with the frozen index.
- **PD4 [R§3.1 adopted] Per-voice data layout.**
  - `ogi::GiVoiceMod` (new header `Source/dsp/GiVoiceMod.h`), one per voice slot (×20), reset in `startVoice`: `src[18]` (cell values), `Lfo lfo[4]` (verbatim class), `lfoRateHz[4]`, `lfoRateOff[4]`, `GiModEnv env[2]`, `offV[2][126]` (prev / cur), `lastContribV[16]`, `resV[126] {value, step, remaining}` + live list, `pvTouched[≤ 32]`, and the 2.5b seam fields `pressure`, `slide`, `bendSrc`.
  - Dense-stored, sparsely-touched; fixed arrays; ≈ 3.5 KB × 20 inside the heap-held Engine. No allocation.
  - Per pair (k, s): `bool pairRouted[k][s]`, `bool pairRoutedPrev[k][s]`, `Stream view[k][s]` (built at the tick only when routed), `CellLerp lvlGL[k][s], lvlGR[k][s]` (PD7), `PitchSel sel[k][s]` (only when pitch / pitch_rand / key_track is per-voice routed).
- **PD5 [R§3.2 adopted] Tick order** (`gridTick`, `Engine.cpp:553`): (1) `gp.latch`, Tempo effective values from the block's anchor; (2) `tickSources` — global LFO ×4, macros, wheel, per non-free voice: Mod Env sample, per-voice LFOs (Per-voice or `xf_N ≠ 0`), MIDI sources; then newest-voice mono values; then `xf_N` ramps; (3) `matrixTick` — global pass (port), per-voice pass, global-routed tick dests overwrite `gp` fields (parent pattern, `gp` is re-latched every tick), per-pair views, next-cell LFO rates, telemetry atomics; (4) the 2.4 order unchanged in sequence, with G-dest substitutes at their existing sites (`outGainRamp`, stream level / pan ramps or pair mode, `writeReverbAtomics`, chain controls + `fbRamp`), then `updateHeads` → `takeTapes` → … → `updateRegion` → `enforcePolyphony` → `updateScale` (+ per-pair windows) → `updateQuotas` (per-pair) → `updateScan` (per-voice scan rate) → `writeHeads`.
  - **Views always start from `rawBase`** (the APVTS raw), with `offG + offV_k` summed **before** the single clamp (P3). Never from a `gp` field the global pass overwrote.
  - The 2.3 FX latch and the 2.4 piece loop are untouched: `spawn` builds `local` (a `Stream` copy) **before any draw** and passes `&local` where `&gp.s[s]` was; draws keep their fixed slots.
  - `pairRouted` / pair-mode flags are derived from `routed || routedPrev` in **both** exits of the tick, before the quota dirty-check (P8, memory `pattern_run_flag_past_dirty_check_latches`).
- **PD6 [R§4 adopted; decides R open items] Destination classes and consumers.**
  - 112 V / 9 G / 4 G-V per `mod-dests.tsv`; consumers per R§4 table. Tick-consumer V dests (density, size, size_rand, jitter, pitch, pitch_rand, key_track, reverse, spray, window, skew, spread, scan_rate, loop) read `view (k, s)` = `pairRouted ? view[k][s] : gp.s[s]` in `updateQuotas`, `pairCoherence`, `pairLaw`, `pairFbGain`, `fillLawKey`, `updateScale`, `updateScan`.
  - **Loop region [DECIDED]:** global routes move `region[s]` (reads + record head, as today). Per-voice routes move only that voice's **read** region at spawn; the record head keeps the global view (P17). The clamp's `circular` test already falls back to absolute distance.
  - **Reverb dests** are tick-rate atomics (`writeReverbAtomics` reads the modulated values); R54 cold-tank restart at mix ≤ 0.001 is INFO (HANDS-ON).
  - **`lfoN_rate`** is G while LFO N is Global, V while Per-voice (each voice's LFO rate), routed by the PD9 path rule.
  - **Optional parity family [DECIDED: include]** — PLs1 (LFO1 → `s1_size` ±10 %) proves the spawn `offsetAt` latch in parity; position / pitch stay excluded.
- **PD7 [R§3.4 adopted; corrects ARCH §16] Per-voice level / pan = pair mixer mode.**
  - A stream with any pair routed to level or pan (now or in the previous cell): the k-loop multiplies `bl · gL_kS (c)`, `br · gR_kS (c)` with per-pair `CellLerp` gains (`level_kS · pan_kS`), and the y statement uses `aud · vgain` only. The tap uses per-pair `min (level_kS, 1)` in its own statements (2.4 P11 separation kept).
  - Unrouted streams keep the 2.4 statements verbatim → true bypass. Pair mode exits only after the last residual lands (`pairRoutedPrev` false).
- **PD8 [R§5 adopted] Newest-voice rule.**
  - Newest = `argmax serial` over `isCounted()` voices (gated or releasing; never `State::stolen` spares), evaluated at the tick (≤ 1 cell lag, block-size invariant).
  - Per per-voice source p: `mono_p = from_p + (src_p (newest) − from_p) · w`, one shared `ExactRamp w` 0 → 1 over `rampCells20`. Handover: `from_p := mono_p` (incl. a ramp in flight), `w.reset (0); w.setTarget (1)` — never stacked. No counted voice: `from_p := mono_p`, held. **Initial value 0 at prepare; the first note ramps from 0 [DECIDED]** (no special case).
  - Telemetry counters: `newestSerial`, `handovers`, `newestWasStolen` (must stay 0). Hooks `newestNoRamp` (NV1n), `newestStack` (NV4 control).
- **PD9 [R§4.2 adopted] `lfoN_mode` switch.**
  - Per LFO an `ExactRamp xf_N` (20 ms) 0 = Global, 1 = Per-voice. `src_k = (1 − xf)·g_N + xf·pv_{N,k}`. Per-voice LFO N ticks while mode = Per-voice or `xf_N ≠ 0`; LFO N's routes run on the per-voice path in that window.
  - **Global → Per-voice [DECIDED]:** at the switch tick every non-free voice `prepare`s its LFO N at `lfoN_phase` (deterministic). Voices started later reset at note-on as usual.
  - At `xf = 0` the per-voice path yields exactly `depth · g`, so the return to the global path is a rounding-level step at most. A switch mid-ramp retargets from current. Hook `modeSwitchSnap` (LM1n).
- **PD10 [R§6.1 adopted; decides R open items] Sources, ranges, MIDI.**
  - Ranges per R§6.1: velocity, note random, pressure, wheel, macro, Mod Env unipolar; key `clamp ((note − 60) / 64, −1, 1)`; slide `(cc74 − 64) / 64` clamped; bend = the existing −1..1 normalisation; LFOs bipolar.
  - Note Random = `Draw (seed, kNoteRandStream, serial).nextFloat (0)`; new constants in `GiConstants.h`: `kNoteRandStream`, `kVoiceLfoStream`, disjoint from `0x0A1F / 0x0A5C / 0x0A6E`. Per-voice S&H seed `mix (seed ^ kVoiceLfoStream ^ mix (serial))`, cycle counted from note-on.
  - Mod wheel 14-bit `(cc1·128 + cc33) / 16383`; a CC1 write clears the LSB.
  - **[DECIDED]** pressure / slide are initialised at note-on from the last channel value; channel AT → every voice; poly AT → voices on that note; CC74 any channel → every voice; last write wins. **CC121** resets wheel 0, pressure 0, slide 64, bend 0.
  - `voiceSemis` keeps the global `bend` (`Engine.cpp:227`) until 2.5b (P16). The 2.5b seam is the per-voice fields; the `mpe_enable` branch is not written in 2.5a.
- **PD11 [R§6.1 adopted] `GiModEnv`** (in `GiVoiceMod.h`): AmpEnv math (RC 1.2 overshoot attack, −60 dB decay / release, 2 ms floors, times latched at stage entry; `AmpEnv.h:27-41, 181-200`), never ends a voice, `retrigger()` continues from the current level (unit-gated now, used by 2.5b). Advanced per segment beside `v.env.process` with the same splits; sampled at the tick. Hook `envLiveTimes` (ME3 control).
- **PD12 [R§7 adopted + PD1] Tempo.**
  - **Processor:** read `getPlayHead()->getPosition()` once per block into a `TransportSnapshot` (`TempoTracker.h:47-56`); `Engine::process (…, const TransportSnapshot& = {})`; `tracker.update (s, absPos)` before the first piece. No playhead → the tracker's own default (parent behaviour); renders unchanged in Free.
  - **Effective values per pair** (parent P50): `e = clamp (floor (4·offD + 0.5), ceil (log2 (0.5 / d0)), floor (log2 (200 / d0)))`, `density = d0·2^e`, `tempoDiv = divBeats·2^(−e)`; `size = clamp (divBeats·60000 / bpm · 2^(4·offS), 5, 2000)`, evaluated at the spawn sample.
  - **Clock:** new `Source/dsp/GiTempoClock.h` per (k, s) = port of the parent `processTempo` (absolute-time grid point, emitted at `ceil (t)`, `frac = smp − t`). Slot re-derived only at a re-lock, enable, mode switch or div change (P12). Jitter draws keyed `(serial << 24) | (epoch + tIndex)` on `kSchedStream + s`. Free ↔ Tempo folds the epoch (P14 pattern).
  - **Thinning (corrects ARCH §9):** `I = tempoDiv / pps`; `j = min { j ≥ 0 : I·2^j ≥ thinnedPeriod (…) }` (integer loop); `div = tempoDiv·2^j`; `periodT = I·2^j`; nEff uses that period. Water-fill unchanged; `rescheduleNext` not used in Tempo. Hook `tempoContinuousThin` (TS2n), `ceilPerTick` (TS1n).
  - **LFO Tempo:** global = the parent hybrid via `tick (…, tempo = true, tracker, …)`; **per-voice [DECIDED]** = `tempo = false` with `rateHz = fs·pps / divBeats · 2^(4·off)` (tempo-rate free run from the note-on phase; P5).
- **PD13 [R§8 adopted] Parity plumbing, no parent edits.**
  - `ParityRows.h`: `Route {slot, srcName, destId, depth}`, timed `Event {at, id, value}`, `Transport {bpm, ppq0, playing, steps, loop}` (the mock playhead math lives here; each driver wraps it in a `juce::AudioPlayHead`); new `Kind::modNull`. `parent_render.cpp`: route / event / transport columns appended at the **end** of each line; the parent's `traceDest` ring is drained per block. Every 2.2 / 2.3 / 2.4 parent `.f32` stays byte-identical to the W0 copy (`cmp`).
  - **Name mapping (P2):** each side maps choice names to its own index via `AudioParameterChoice::choices.indexOf`; the landed index is printed per row and self-checked.
  - **Alignment (P4):** our prepare mirrors the parent's LFO / macro / matrix init (`82406fce:…/Engine.cpp:92-101`); silent pre-roll to T_e, note-on and enable at T_e (2.4 PB precedent). **PL0t** (s1_level trace ≤ 1e-7 abs per tick) is the precondition for every PL / PM / PE row.
  - New `tests/render-harness/parity_gates_25a.h` (`pg25a`), reusing pg22–pg24 helpers. Rows (route on `s1_level` unless noted; base −12 dB; depth ±50; peak < kKneeT):

    | ID | Stimulus | Gate |
    |---|---|---|
    | PL0 | PN01 geometry, 16 slots None / 0, all sources busy | null ≤ −130 dB; sets `g_PL` |
    | PL0t | same, trace of s1_level | ≤ 1e-7 abs per tick |
    | PL1–6 | LFO1 × 6 shapes, Free 3.7 Hz | ≤ −90 dB, \|g/g_PL − 1\| ≤ 1e-4 |
    | PL7–9 | LFO2 / 3 / 4 (sine, saw, S&H), depth −50 | same |
    | PL10 | LFO1 phase 90 → 359 → 1 edits | same |
    | PL11–13 | Tempo: sine 1/4 @ 120; S&H 1/8T @ 133; saw 1 bar @ 97.3 + bpm step + loop jump | same |
    | PL14 | LFO2 → `lfo1_rate` + LFO1 → s1_level | same |
    | PL15 | LFO1 → `output_gain` | same |
    | PLs1 | LFO1 → `s1_size` ±10 % (spawn latch) | same |
    | PM1–4 | Macro 1–4: 0 → 70 → 20 | same |
    | PE1–6 | depth +60 → −60; 0 → 60; src swap LFO1 → LFO2; dest swap s1_level → output_gain; src → None; 4 stacked slots to the clamp edge | same |
    | PLc1–3 | ours: `lfo1_phase` +1°; seed + 1 (S&H); mock bpm +0.1 | each must FAIL −90 dB |
    | PMc1 | `macroRampZero` | FAIL |
    | PEc1–3 | `noDepthRamp`; `noResidual` (src swap); depth −59 | FAIL |

    35 rows → `--parity` **133 → 168**. Tempo PL rows run our side under `tempoGridOnly` only if a stream is in Tempo; PL11–13 route LFOs only (streams Free), so no hook is needed there.
- **PD14 [R§9 adopted] Law arm + click gates** — rows in T13 / T14. Thresholds frozen: trajectory 1.05 × the parent bound + 1e-6; κ ≤ 2.0× vs same-motion, hard controls > 2.0× with `ctrl FIRED` asserted (P7); edits at the LFO peak; RT trajectory arm carried from the parent MX3 (deterministic, 13.9–29.8× control margin).
- **PD15 [R Summary 11] `readSet25a`.** `eg21::readSet25a()` = `readSet24()` + `sync_mode`, `sN_size_sync` / `sN_density_sync` ×4, `menv1/2_{attack,decay,sustain,release}`, `lfoN_{shape,rate,rate_sync,phase,mode}` ×4, `macro1–4`, `modM_{source,dest,depth}` ×16 = **251 bound / 10 unread** (`voice_mode`, `glide`, `mpe_enable`, `rec_target`, `rec_arm`, `rec_trigger`, `rec_threshold`, `rec_length`, `rec_length_sync`, `rec_gain`). N2 / N2s / FN3 switch to it with literals **251 / 10**; titles say `readSet25a`. Any other row needing a re-scope → **S8**.
- **PD16 [CONTEXT Defaults] UI telemetry, never APVTS.** Engine-owned `std::atomic<float>`: `destTel[126]` (G from the global pass; V from the newest voice — `v01` routed, `base01` when the raw moved, parent rule), `srcTel[17]`, `lfoPhaseTel[4]`; written at the tick, relaxed. Static row **ST1**: `grep -rE 'setValueNotifyingHost|beginChangeGesture|setValue\(' Source/dsp` = 0 (excluding comments), control: the same grep on `Source/PluginProcessor.cpp` finds the preset path.
- **PD17 [CONTEXT Defaults] True bypass + checkpoints.**
  - Unrouted (16 slots None or depth 0): `offCur ≡ +0`, no pair routed, spawn uses `&gp.s[s]`, mixer / ramps / quotas / law follow the 2.4 statements.
  - **Checkpoint after every engine task (T3–T12):** build the harness (BG); `$H --dump-render-hashes "$S/ck-<T>.sha256"` byte-identical to `renders-2.4.sha256`; `$H --dump-render-hashes-legacy "$S/ck-<T>-legacy.sha256"` byte-identical to `renders-2.2.sha256`. A move → **S3** attributed to that task before the next starts. renders-2.1 / 2.2 / 2.4 are **never re-pinned** in 2.5a.
  - **TB3** (sources busy, unrouted: macros moving, LFO1 Per-voice, CC / AT / bend events) and **TB4** (NZ1 neutral live routes on the renders-2.4 recipes) → byte-identical.
- **PD18 [D28 + R§11] `--cpu25a`.**
  - Same method as `--cpu25`: run alone, `CLOCK_THREAD_CPUTIME_ID`, warm-up + 7 × 10 s, live ≥ 240, duty ≥ 0.9, calibration witness vs `tests/golden/cpu-calib-2.3.txt`.
  - **C25w** re-run in the same session (the delta is measured). **C25m** = C25w + LFO1–4 Per-voice (1.3 / 2.1 / 3.7 / 5.3 Hz) + Mod Env 1–2 (A 5 / D 300 / S 50 %, retriggered by the C25w note pattern) + 16 routes: s1–s4 density ±30 % (quota every tick), s1–s4 level + s1 / s2 pan (pair mixer), s1 / s3 pitch + s2 / s4 filter_cutoff (spawn + coherence), s1 spray (law), s2 scan_rate. **Verdict ≤ 40 %.** Witnesses: routed per-voice dests ≥ 16 every tick; quota recomputes ≥ 0.9 × ticks; pair mode active on 4 streams; live ≥ 240 → else INVALID = FAIL (one stimulus fix allowed, recorded).
  - **C25tm** = C25t + 4 routes (global LFO1 → s1_position, velocity → s1_level, Mod Env 1 → s1_size, Macro 1 → s1_filter_cutoff). **Verdict ≤ 12 %**, live ∈ [60, 90].
  - INFO: C25t, Δ = C25m − C25w (projection +2.5–5.5, C25m ≈ 29–32 %).
- **PD19 [D21 + R§11] Lever ladder** (T19 only on a valid-witness FAIL): **L0** per-pair dirty bits so `pairLaw` / `pairFbGain` / coherence recompute only for pairs whose view changed (bitwise-neutral: the full harness + parity must be unchanged, else revert) — automatic. Then the ARCH ladder: lever 1 (Hermite) **never automatic**; levers 2 → 3 automatic, kept only if TB0 / TB3 / TB4 + legacy dump + BN1 + every PN / PF / PB / PR / PL residual are unchanged. Still FAIL → **S2** (the `grain_limit` 128 fallback always asks).
- **PD20 Test hooks** (all under `OGI_TEST_HOOKS`, in `EngineTestHooks`):
  - Switches: `noResidual`, `noDepthRamp`, `fullRoundTrip`, `macroRampZero`, `pvSharedSource`, `pvLfoNoReset`, `slotKeyedSeed`, `envLiveTimes`, `newestNoRamp`, `newestStack`, `modeSwitchSnap`, `tempoContinuousThin`, `ceilPerTick`, `tempoGridOnly`.
  - Probes: `traceDest {destId, k, float* buf, len}` (per-tick ramped value / per-spawn latched value, parent semantics), `onsetLog` extended with `counter`.
  - Counters: `routedPvDests`, `quotaRecomputes`, `pairModeStreams`, `newestSerial`, `handovers`, `newestWasStolen`, `tempoOnsets`, `burstOnsets`, `relocks`. `resetCounters()` zeroes them.
  - Accessors: `getVoiceModForTest (k)`, `getMatrixForTest()`, `getTrackerForTest()`.
- **PD21 Housekeeping.**
  - Version stays **0.1.0** (I/O unchanged); **no tag**. No new params, no `<Engine>` keys; param-dump byte-identical.
  - `CMakeLists.txt` **not edited** (new sources are headers: `GiModMatrix.h`, `GiVoiceMod.h`, `GiTempoClock.h`; every test file is a header).
  - Not edited: `ModMatrix.h`, `ModSources.h`, `TempoTracker.h`, `EngineConstants.h`, `FeedbackChain.h`, `OutputLaw.h`, `GrainFx.h`, `Grain.h`, `GiRenderer.h` (unless T19 fires), `cpu_calib.h`, `run.sh`, pg22–pg24. `ControlGrid.h` re-hashed (cols 5 / 7).
  - **Two commits**, path-scoped: **C1** after W2 (global path + parity PL / PM / PE green) and **C2** at the end.
  - HANDS-ON-2.5a written and deferred post-Stage-3 with 2.1–2.4.
  - Standalone rebuilt explicitly (`OuariconGranulatorInstrument_Standalone`) before any Standalone check (P18).

---

## Tasks

**Roles.** **[agent]** = dsp-agent writes files (no Bash). **[run]** = build / gate runner: builds, gates, validators, hash refreshes, commits. **BG** = `run_in_background`, log to `$S/<name>.log` (`$S` = the executing session's scratchpad). `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`. Paths are relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.

**Audio-path rules (every engine task).** No locks (except the 2.2 scale try-lock), no allocation, no `shared_ptr`, no vector growth, no `NormalisableRange` copy, no `std::function` call on the audio thread; fixed arrays only. `ScopedNoDenormals` per block. No listener writes a parameter. Every random draw keys on the note serial, never the voice slot. Every ramp is read per sample at its cell position.

### Wave 0 — pre-flight, baselines, CPU reference on the unmodified engine

**T1. Pre-flight + baselines** [run]
- Files: none. Output `$S/preflight.txt`.
- Done when (any mismatch → STOP and ask):
  - **Late changes:** `git log --oneline 3d68135b..HEAD -- plugins/O-GranulatorInstrument/Source plugins/O-GranulatorInstrument/tests plugins/O-GranulatorInstrument/CMakeLists.txt` empty. If not, map: `Engine.*` → T5–T12; `PluginProcessor.*` → T4; `forked/*`, `FORK-MANIFEST.md` → T3; `tests/parity/*` → T7; `tests/render-harness/*` → T3, T8, T13–T15.
  - **Git:** branch `main`; record `git status --short`; foreign paths never staged (`modules/registry.yaml`, `.claude/*`, `plugins/O-GrainScatter/*`, any other session's files).
  - **Contracts and pin:** `parameter-spec.md` sha256 (file + body) = STATUS `contract_checksums`; `git cat-file -e 82406fce`; `git diff --quiet 82406fce HEAD -- plugins/O-Granulator/Source/dsp modules/synthesis/grain-engine modules/tuning/pitch-collections modules/effects/reverb-unit scripts/param-dump`; `shasum -a 256 Source/dsp/forked/*.h` = FORK-MANIFEST col 5.
  - **Baseline (BG):** `ninja -C build O-GranulatorInstrument-render-harness O-GranulatorInstrument-param-dump`; `$H` = **179 PASS / 0 FAIL**, 0 NOT FIRED → `$S/h-24.log`, duplicate-ID set → `$S/dup-24.txt`; param-dump → `$S/pd-24.txt` == `.planning/params.tsv`; `$H --dump-render-hashes "$S/r24-base.sha256"` == `tests/golden/renders-2.4.sha256`; `--dump-render-hashes-legacy` == `renders-2.2.sha256`; `tests/fork-drift-check.sh` 0 / 13.
  - **Parity (BG):** `tests/parity/run.sh "$S/parity"` rc 0; `$H --parity "$S/parity/out"` **133/0** → `$S/par-24.log`; `--rows GV` **2/0**; PN / PF / PB / PR residual lines → `$S/res-base.txt`; `cp -R "$S/parity/out" "$S/parity-base-out"`.
  - Standalone phony target present (`ninja -C build -t targets all | grep -c '^OuariconGranulatorInstrument_Standalone: phony'` = 1).

**T2. C25w on the unmodified 2.4 engine (reference)** [run]
- Depends on: T1 with every BG job finished. **No `Source/` edit before this task is done.**
- Run alone: `$H --golden-dir tests/golden --cpu25` (BG) → `$S/cpu25-base.log`. Valid witness, no NOVERDICT (≤ 3 tries; a third → STOP, environment). Copy C25w / C25t + calib into `$S/preflight.txt` (VERIFICATION-2.4: 26.59 % / 5.51 %).

### Wave 1 — plumbing, no behaviour

**T3. ControlGrid mod / sync rows + alias + manifest + readSet25a (PD2, PD15, P1, P6)** [agent; runner re-hashes]
- Files: `Source/dsp/forked/ControlGrid.h`, `Source/dsp/forked/FORK-MANIFEST.md`, `tests/render-harness/engine_gates_21.h`, `engine_gates_22.h`, `engine_gates_23.h`.
- Depends on: T2.
- `ControlGrid.h`, ADAPTED region only: `GiParamPointers` binds the 89 IDs (PD15 list); `GiGridParams` gains `syncMode`, `s[n].sizeSync / densitySync` (choice index + `divBeats` from the spec's division table), `struct Lfo` (PD2) `lfo[4]` + `lfoMode[4]`, `menv[2] {a, d, s, r}` (ms / 0..1, spec ranges read through the skewed `NormalisableRange`, never inverted by hand), `macro[4]` (0..1), `mod[16] {src, dest, depth}` (choice indices; depth −1..1); spec defaults; `latch` reads them. `using GridParams = GiGridParams;` after the struct. Header comment: never bound after 2.5a = voice mode, glide, MPE, record.
- FORK-MANIFEST: ControlGrid col 7 += "; 2.5a: 89 mod / sync rows, `struct Lfo`, `using GridParams`"; the PD2 reclass of ModSources / ModMatrix / TempoTracker. Runner refreshes col 5.
- `readSet25a()` after `readSet24()`; N2 / N2s / FN3 → 251 / 10 (PD15).
- Done when the checkpoint passes, drift 0 / 13 after the col-5 refresh, and `$H` = 179 / 0 (N2 / N2s / FN3 green on the new literals).

**T4. Transport plumbing + leaf includes (PD12 processor half)** [agent]
- Files: `Source/PluginProcessor.cpp`, `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on: T3.
- `Engine.h` includes `forked/ModSources.h` and `forked/TempoTracker.h`; member `TempoTracker tracker`; `process (…, const TransportSnapshot& tp = {})` calls `tracker.update (tp, absPos)` before the first piece. Processor fills the snapshot from `getPlayHead()->getPosition()` once per block (null-safe). `Engine::setDestRanges` (PD3) declared and called from the processor (message thread). Nothing reads the tracker yet.
- Done when the checkpoint passes and the leaves compile unchanged (else **S8**).

### Wave 2 — sources + global matrix + parity (commit C1)

**T5. Sources (PD10, PD11; global LFO, macros, MIDI, per-voice fields)** [agent]
- Files: `Source/dsp/GiVoiceMod.h` (new), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`, `Source/dsp/GiConstants.h`.
- Depends on: T4.
- Global `Lfo lfos[4]` (verbatim class), `lfoRateHz[4]`, macro `ExactRamp`s, wheel, channel pressure / slide state, all initialised in `prepare` **mirroring `82406fce:…/Engine.cpp:92-101`** (P4). `GiVoiceMod vm[kVoiceSlots]` reset in `startVoice` (velocity, key, note random, pressure / slide / bend copied from channel state, per-voice LFOs `prepare`d at `lfoN_phase`, Mod Envs `noteOn`). `applyEvent` parses CC1 / CC33 / CC74 / CC121, channel AT, poly AT (PD10). Mod Envs advance per segment beside `v.env.process`. `tickSources` at the tick (PD5 step 2, without the newest-voice part yet).
- Done when the checkpoint passes (no source reaches audio).

**T6. `GiModMatrix.h` + global pass + G-dest application (PD3, PD5, PD6)** [agent]
- Files: `Source/dsp/GiModMatrix.h` (new), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on: T5.
- Port per PD3; `matrixTick` global pass; global-routed tick dests overwrite `gp` fields (parent `:983-996`); spawn builds `local` and applies `offsetAt` for spawn-latched dests (parent `:1155-1182`), passing `&local` to every consumer incl. `planFx`; G dests via the parent substitutes (`outGainRamp`, stream level / pan ramps, `writeReverbAtomics`, fb λ / chain controls; parent `:698-794`); `lfoN_rate` modulation (R50.5 next-cell rates). Hooks `noResidual`, `noDepthRamp`, `fullRoundTrip`, `macroRampZero`, `traceDest`.
- Done when the checkpoint passes (unrouted defaults) and the agent reports the before / after of the `spawn` head where `local` is built.

**T7. Parity side (PD13)** [agent; written alongside T5–T6]
- Files: `tests/parity/ParityRows.h`, `tests/parity/parent_render.cpp`, `tests/render-harness/parity_gates_25a.h` (new; PL / PM / PE rows), `tests/render-harness/main.cpp` (`pg25a::runAll()` in `--parity` after pg24).
- Depends on: T1 (writes); the run waits for T8.
- Done when written; `run.sh` unchanged.

**T8. Build, parity run, commit C1** [run; agent fixes]
- Depends on: T6, T7.
- BG: build harness + param-dump; `$H` = 179 / 0; checkpoint; `tests/parity/run.sh "$S/parity"`; every 2.2–2.4 parent `.f32` `cmp`-identical to `$S/parity-base-out`; `$H --parity "$S/parity/out" --rows PL0` → PL0 ≤ −130 dB and PL0t ≤ 1e-7 (else **S1** before any row counts); full `--parity` → **168 / 0** (carried 133 residuals == `$S/res-base.txt` to 0.1 dB), `--rows GV` 2 / 0.
- **Commit C1:** `git branch --show-current` (= `main`) + `git status --short` immediately before; `git status --short -- plugins/O-GranulatorInstrument` lists only T3–T7 files; `git add` the new paths (`GiVoiceMod.h`, `GiModMatrix.h`, `parity_gates_25a.h`); `git commit -- plugins/O-GranulatorInstrument` with `phase: O-GranulatorInstrument 2-dsp/2.5a global matrix (GiModMatrix port 126x18, global LFO verbatim, macros, MIDI sources, G-dest substitutes; readSet25a 251/10; parity 133->168/0, PL0 <x> dB; harness 179/0; renders-2.4 + legacy byte-identical)`. `git log -1 --stat` only plugin paths. **No tag.**

### Wave 3 — per-voice path (checkpoint after each task)

**T9. Per-voice pass + pair views (PD4–PD6, P3, P8, P17)** [agent]
- Files: `Source/dsp/GiVoiceMod.h`, `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on: T8.
- Per-voice routes into `offV[k][cur][d]` per non-free voice with residuals; pair views from `rawBase` + `offG + offV_k`, one clamp; `pairRouted` / `pairRoutedPrev` in both tick exits; `updateQuotas` / `pairCoherence` / `pairLaw` / `pairFbGain` / `fillLawKey` / `updateScale` / `updateScan` read `view (k, s)`; spawn re-evaluates per-voice spawn dests into `local`; per-voice loop region read-only (PD6). Hooks `pvSharedSource`, `pvLfoNoReset`, `slotKeyedSeed`, `envLiveTimes`; counters `routedPvDests`, `quotaRecomputes`.
- Done when the checkpoint passes.

**T10. Pair mixer mode for level / pan (PD7)** [agent]
- Files: `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on: T9.
- `mixPiece` k-loop gains the pair branch **only** for streams in pair mode; unrouted streams keep the 2.4 statements byte-for-byte. Counter `pairModeStreams`.
- Done when the checkpoint passes and the agent reports the before / after of the k-loop.

**T11. Newest-voice rule + lfoN_mode crossfade + telemetry (PD8, PD9, PD16, P9)** [agent]
- Files: `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`, `Source/dsp/GiVoiceMod.h`.
- Depends on: T10.
- Newest from `isCounted()` at the tick; shared `w` ramp; held when silent; per-voice source → G dest uses `mono_p`. `xf_N` ramps; Global → Per-voice re-prepare at the switch tick. `destTel` / `srcTel` / `lfoPhaseTel` atomics (+ processor accessors for Stage 3). Hooks `newestNoRamp`, `newestStack`, `modeSwitchSnap`; counters `newestSerial`, `handovers`, `newestWasStolen`.
- Done when the checkpoint passes.

### Wave 4 — tempo

**T12. Tempo clock, effective values, thinning, burst, LFO tempo (PD1, PD12, P5, P11, P12)** [agent]
- Files: `Source/dsp/GiTempoClock.h` (new), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`, `Source/dsp/VoiceQuotas.h` (only if the thinned-period helper needs a pure accessor; no behaviour change in Free).
- Depends on: T11.
- `GiTempoClock` per (k, s); R45 effective values; division thinning `div·2^j`; Free ↔ Tempo epoch fold; burst at note-on on `kBurstBit | n`, grid from the next slot (PD1); global LFO `tempo = true`, per-voice LFO tempo-rate free run. Hooks `tempoContinuousThin`, `ceilPerTick`, `tempoGridOnly`; counters `tempoOnsets`, `burstOnsets`, `relocks`.
- Done when the checkpoint passes (Free mode everywhere in the recipes) and `$H` = 179 / 0.

### Wave 5 — gates, build, parity

**T13. `engine_gates_25a.h` — law arm (19 rows)** [agent]
- Files: `tests/render-harness/engine_gates_25a.h` (new, `eg25a::runAll()`).
- Depends on: T12.

  | ID | Stimulus | Pass | Control |
  |---|---|---|---|
  | PV1 | 3 held voices 48 / 60 / 72, vel 0.3 / 0.6 / 0.9; vel → s1_size, key → s1_filter_cutoff, note random → s1_pan, per-voice LFO1 → s1_spray | each logged spawn value == `apply (raw, offV_k (at))` ≤ 1 ulp; spread > 0 | `pvSharedSource` → spread 0 → fails |
  | VL1 | per-voice LFO saw → s1_level, 2 notes 0.31 s apart | each pair trace starts at `lfo1_phase` | `pvLfoNoReset` → fails |
  | VL2 | per-voice S&H + note random, slot offset 0 vs 7 | renders bitwise | `slotKeyedSeed` → differs |
  | VL3 | 64 notes with steals, S&H | per-serial values == voice-free recompute | — |
  | ME1 | menv1 A 50 / D 200 / S 40 % / R 300 → trace | ≤ 1e-6 vs closed form | — |
  | ME2 | A = R = 0 | 2 ms floors (n = 96 @ 48 k) | — |
  | ME3 | decay edited mid-decay | segment unchanged until next stage, bitwise | `envLiveTimes` → differs |
  | ME4 | unit `retrigger()` at 0.5 | continues from 0.5 | — |
  | RG1 | unit table (vel / key / slide / bend / pressure / wheel 14-bit / macro) | exact (PD10) | — |
  | NZ1 | routes ±100 from neutral controllers | bitwise == unrouted | macro 0.1 % → differs |
  | CP1 | 16 × Macro1 = 1 at +100 → s1_level; then −100 | +6 dB exact / gain 0; 0 NaN / wrap | — |
  | NV1 | vel 0.2 then 0.9 → output_gain | per cell \|Δ\| ≤ 1.05 × bound + 1e-6 | `newestNoRamp` ≥ 10× |
  | NV2 | all voices end | held, bitwise constant 2 s | — |
  | NV3 | poly 1, 8 notes @ 50 ms | bound holds; in hull; `newestWasStolen` 0 | — |
  | NV4 | 2nd handover 10 ms into the ramp | bound, no overshoot | `newestStack` → overshoot |
  | SD1 | each of 17 sources → s1_level ±60 | 17 / 17 move in the expected direction | — |
  | SD2 | per-voice sources → `reverb_mix` (mono) | all move via the newest voice | — |
  | DR1 | Macro1 +50 → each of 125 dests (+ MX0 table self-check) | its consumer moves (trace / spawn log / atomics), others constant | class gate (P10) |
  | MD1 | MIDI script CC1 / 33, CC74, channel AT, poly AT, bend, CC121 | source values exact at next tick | — |

**T14. `engine_gates_25b.h` — edits, tempo, invariance, bypass (33 rows)** [agent]
- Files: `tests/render-harness/engine_gates_25b.h` (new, `eg25b::runAll()`).
- Depends on: T13.
  - **RTa–g + RTn** (8): MX3 trajectory, 7 edit kinds (add, src → None, dest → None, depth → 0, +60 → −60, dest swap, src swap) × {global LFO, per-voice LFO, Mod Env} on 16 dests; per-cell \|Δv01\| ≤ 1.05 × parent bound, 0 cross-talk; RTn `noResidual` / `noDepthRamp` > 2×.
  - **REa–g + REn** (8): same kinds, audio local κ vs held references at pre- and post-edit states ≤ 2.0×; REn `noDepthRamp` at the LFO peak > 2.0× with `ctrl FIRED` asserted.
  - **LM1 + LM1n** (2): `lfo1_mode` G ↔ PV, 4 voices, LFO → s1_level: trajectory + κ ≤ 2.0×; `modeSwitchSnap` > 2.0×.
  - **TS1, TS1n, TS2, TS2n, TS3, TS4, TS5, TS6, TT1** (9): per R§7.3 (TS1 \|Δ\| ≤ 1 sample @ 44.1 / 48 / 96 k, 4 divs, 3 tempi, bpm step + loop jump, burst onsets excluded and counted; TS1n `ceilPerTick` @ 133 bpm 1/16T drops slots; TS2 cap 32 / 16 voices power-of-two lattice, TS2n `tempoContinuousThin` off-grid; TS3 Free < 10 % on grid; TS4 global LFO Tempo bitwise vs `shape (frac (…) + φ)`; TS5 R45 steps; **TS6** one burst onset at the note-on sample then grid, control `tempoGridOnly` → first onset ≠ note-on; TT1 grid ppq bitwise across block sizes).
  - **BN1 + BN1n** (2): every source live, 16 routes incl. edits, Tempo with bpm step + loop jump, at 32 / 441 / 512 / 4096 / var → 0 diffs; seed + 1 differs.
  - **FA5** (1): 0 allocations across route add / remove / swap, `lfoN_mode` switches with voices, bpm step + loop, steals with 16 routes, CC storms, macro sweeps; each event counter > 0.
  - **TB3, TB4** (2): PD17.
  - **ST1** (1): PD16 static gate.

**T15. `main.cpp` wiring + `--cpu25a` (PD18)** [agent]
- Files: `tests/render-harness/main.cpp`, `tests/render-harness/engine_gates_25b.h` (cpu25a lives here).
- Depends on: T14.
- `#include` eg25a / eg25b after eg24b; `runAll()` calls in order; `--cpu25a` (rc = FAIL count) with C25w, C25m, C25tm, C25t lines and witnesses; banner + usage updated.

**T16. Build + full harness** [run; agent fixes]
- Depends on: T15.
- BG build; `$H` → **231 PASS / 0 FAIL**, 0 NOT FIRED (179 + 19 + 33) → `$S/h-25a.log`; duplicate-ID set == `$S/dup-24.txt`; TB0 14/14, TB3 / TB4, RB1 7/7, FR1 14/14 (legacy) — else **S3**; param-dump == `$S/pd-24.txt`; drift 0 / 13 + NC-a / NC-b fire; X1 (compdb) 0 parent refs.
- A law-arm failure → **S5**; RT / RE / LM → **S6**; TS → **S7**. **Never loosen a threshold or re-pin a render.**

**T17. Full parity re-run** [run]
- Depends on: T16.
- BG: `tests/parity/run.sh` (incremental), `$H --parity` **168 / 0**, GV 2 / 0; carried residuals == `$S/res-base.txt` to 0.1 dB; the PL / PM / PE residual + gain-band table → SUMMARY. A band miss → **S4**.

### Wave 6 — CPU verdict

**T18. `--cpu25a` (D28)** [run]
- Depends on: T17 (nothing else running; Logic closed).
- `$H --golden-dir tests/golden --cpu25a` alone (BG) → `$S/cpu25a.log`.
- Done when: **C25m ≤ 40 %** with every PD18 witness; **C25tm ≤ 12 %**; C25w / C25t (same session) and Δ printed. NOVERDICT = FAIL (≤ 3 tries, then STOP). Valid-witness FAIL → T19.

**T19. Lever ladder — ONLY on a valid-witness FAIL (PD19)** [agent + run]
- Files (if fired): `Source/dsp/Engine.cpp` (L0), `Source/dsp/GiRenderer.h` (levers 2–3).
- L0 first, then levers 2 → 3, each kept only under PD19's conditions; full harness + parity + `--cpu25a` alone after each. Still FAIL → **S2**.

### Wave 7 — install, validators, hands-on, bookkeeping

**T20. Install + Standalone + validators + X2** [run]
- `./scripts/build-and-install.sh O-GranulatorInstrument` (BG; it clears the AU cache and sweeps both variants). `ninja -C build OuariconGranulatorInstrument_Standalone` (BG); the `.app` newer than T16's harness and alive after 6 s. `auval -v aumu OuGI OuDv` PASS + ZZZZ control. pluginval 10 VST3 + AU, one at a time, BG, `--strictness-level 10 --timeout-ms 600000`, 2 ch in, Logic closed. X2: `nm -C` on installed binaries + `$H` → 0 `ogran::`; control: the parity binary > 0.

**T21. `HANDS-ON-2.5a.md` (deferred)** [agent]
- Files: `stages/2-dsp/HANDS-ON-2.5a.md`, HANDS-ON-2.4 layout, "Deferred with HANDS-ON-2.1–2.4" banner. Rows (Standalone + Logic AU):
  1. LFO 1–4 each shape → s1_position / level / pitch: feel, no zipper, S&H steps clean.
  2. Per-voice LFO on a chord → audible per-note spread; Global ↔ Per-voice switch while holding: no click.
  3. Velocity / key / note random → size / cutoff / pan on chords: independent per note.
  4. Mod Env 1–2 → size / filter: shapes match the knobs; retrigger feel.
  5. Mod wheel, channel AT, CC74, bend from a controller → expected moves; CC121 resets.
  6. Macros 1–4 swept by hand and by DAW automation: smooth.
  7. Newest-voice: velocity → reverb_mix while playing legato phrases: smooth handover, holds after release.
  8. Route edits live (add / remove / swap / sign flip): no clicks.
  9. Tempo: density / size sync in a DAW loop at 3 tempi, tempo change and loop jump: locked; note-on burst feels immediate; thinning under heavy load stays on the grid.
  10. LFO Tempo sync locked to the bar; per-voice tempo LFO restarts per note.
  11. LFO → reverb_mix crossing 0: cold-tank restarts (R54) — acceptable or not (listening call).
  12. 2.1–2.4 patches with no routes sound unchanged.
- Not self-certified.

**T22. SUMMARY, STATUS, commit C2** [agent writes, run commits]
- Files: `stages/2-dsp/SUMMARY-2.5a.md`, `.planning/STATUS.md` (`stage_2_phase_2_5a_executed_hands_on_pending`; next = 2.5a verify).
- SUMMARY: every row with margin + control; counts vs targets (harness 231, parity 168 + GV 2); PL0 / PL0t, `g_PL`, band table; law-arm numbers (NV bound ratios, ME error, RT / RE / LM κ with controls, TS max \|Δ\|, burst count); C25w / C25m / C25tm / C25t + calib + Δ; lever outcome; PD1–PD21 as decided; every deviation with its reason.
- **Commit C2:** branch + status immediately before; plugin status lists only this plan's files (anything else → STOP); `git add` new paths (`GiTempoClock.h`, `engine_gates_25a.h`, `engine_gates_25b.h`, `SUMMARY-2.5a.md`, `HANDS-ON-2.5a.md`); `git commit -- plugins/O-GranulatorInstrument`. Never `-A` / `-a`. `git log -1 --stat` only plugin paths. **No tag.**

---

## Dependency graph

```
T1 → T2 → T3 → T4 → T5 → T6 ─┬→ T8 (C1) → T9 → T10 → T11 → T12 → T13 → T14 → T15 → T16 → T17 → T18 → [T19 on FAIL] → T20 → T21 → T22 (C2)
           (checkpoint after each of T3–T12)
T1 ─→ T7 (written alongside T5–T6) ─┘
```

- T2 finishes before any `Source/` edit (C25w measures the 2.4 engine).
- T3 → T12 strictly sequential (shared `Engine.{h,cpp}`); each followed by its checkpoint.
- C1 lands only with PL0 / PL0t and all 35 new parity rows green.
- T18 runs alone after every build and parity job.

## Execution notes

- **Agent split.** dsp-agent writes T3–T7, T9–T15, T19 code, T21, T22's SUMMARY. The runner does every build, checkpoint, gate, validator, hash refresh and both commits.
- **Background:** ninja, `run.sh`, `build-and-install.sh`, the Standalone link, pluginval, cold auval, `--cpu25`, `--cpu25a`, the full harness (600 s watchdog). BN1 renders 5 block sizes × every source; the harness gets longer.
- **Parent reads** via `git show "${PIN}:…"` (braced) or the run.sh archive only.
- **Handoff.** After execute: `/clear` → `/plugin-verify O-GranulatorInstrument 2-dsp`.

## Success criteria (CONTEXT-2.5a gate)

| Criterion | Proven by | Target |
|---|---|---|
| Parity ≤ −90 dB on global LFO / macro / ramps / shadow crossfade, band held, controls fail (D27) | PL0, PL0t, PL1–15, PLs1, PM1–4, PE1–6, PLc1–3, PMc1, PEc1–3 | `--parity` **133 → 168 / 0**; PL0 ≤ −130 dB; band ≤ 1e-4 |
| Each source routes with bipolar depth; per-voice independence with collapse control (DSP-06) | SD1, SD2, DR1, PV1 | 17 / 17, 125 / 125, PV1n fires |
| Newest-voice handover within the ramp bound; held when silent; un-ramped control fails (D4) | NV1–NV4 | ≤ 1.05 × bound; NV1n ≥ 10× |
| Mod Env golden; per-voice LFO reset; allocation-order invariance | ME1–4, VL1–3 | ≤ 1e-6; bitwise |
| Neutral controllers add 0; 16-route stack clamps without NaN | NZ1, CP1, RG1, MD1 | bitwise; exact |
| Route-edit + `lfoN_mode` click gates vs same-motion controls (QUAL-01) | RTa–g, RTn, REa–g, REn, LM1, LM1n | κ ≤ 2.0×; controls > 2.0× |
| Tempo onsets ≤ 1 sample; power-of-two thinning; Free control; note-on burst (PD1) | TS1–TS6, TS1n, TS2n, TT1 | ≤ 1 sample; < 10 %; burst at note-on |
| Block-size null with every source live; seed control | BN1, BN1n | 0 diffs / differs |
| Allocation gate across edits / mode switches / tempo / steals with 16 routes (PERF-01) | FA5 | 0 allocations |
| True bypass: renders-2.4 + legacy byte-identical; param-dump identical; drift 0; engine never writes APVTS | TB0, TB3, TB4, RB1, FR1, checkpoints, ST1, drift + NC | 14/14, 7/7, identical, 0 / 13 |
| CPU worst case with 16 per-voice routes ≤ 40 %; typical + 4 routes ≤ 12 % (D28) | C25m, C25tm (+ C25w same session) | **≤ 40 %**, **≤ 12 %** |
| Carried gates | 179 harness, 133 parity, GV 2 | harness **179 → 231 / 0**, 0 NOT FIRED |
| auval PASS; pluginval 10 VST3 + AU; parent code excluded | T20, X1, X2 | all PASS |
| HANDS-ON-2.5a written, deferred | T21 | 12 rows |

**Requirements this phase moves** (verify sets the final status): **DSP-06** pending → automated arm complete (feel = HANDS-ON-2.5a); **PERF-02** final worst case with matrix in; **PERF-01** matrix / tempo arms in (file swap stays 2.6); **QUAL-01** route-edit / mode-switch click gates in.

## Pitfalls → task + gate (RESEARCH P1–P18)

| # | Pitfall | Task | Gate |
|---|---|---|---|
| P1 | Compiling `ModMatrix.h` for 126 dests / bumping `kModDests` | T3, T6 (port) | drift 0 / 13; PL / PE rows |
| P2 | Route indices differ per side | T7 (name mapping) | landed-index self-check per row |
| P3 | View built from an overwritten `gp` field | T9 (`rawBase` + one clamp) | PV1 ≤ 1 ulp, DR1 |
| P4 | Prepare init ≠ parent → phase error | T5 | PL0t precondition |
| P5 | Per-voice Tempo LFO re-locks to bar | T12 (`tempo = false`) | VL1 in Tempo, TS4 (global) |
| P6 | N2 / N2s / FN3 randomise mod params | T3 (`readSet25a`) | 251 / 10 |
| P7 | Vacuous route-edit κ control | T14 | REn / LM1n `ctrl FIRED` + RT trajectory arm |
| P8 | Pair flag past the dirty-check skip latches | T9 | BN1, TB3 after a route removal |
| P9 | Spare chosen as newest; ramps stacking | T11 | NV3, NV4 |
| P10 | One dest per path gated | T13 | DR1 (125), SD1 (17) |
| P11 | Continuous thinning period → off-grid | T12 | TS2 + TS2n |
| P12 | Slot re-derived per tick → drops | T12 | TS1n @ 133 bpm |
| P13 | CPU row with idle routes | T15 | C25m witnesses |
| P14 | `−0` counted as routed | T6 (canonical +0) | NZ1, TB4 |
| P15 | `NormalisableRange` copied on audio thread | T4 / T6 (`setDestRanges`) | FA5 |
| P16 | `voiceSemis` switched to per-voice bend | T5 (keep global) | TB0, TB3 |
| P17 | Per-voice loop region moves the record head | T9 | TB0, HC1 carried |
| P18 | Standalone stale | T20 | explicit target, `.app` newer than T16 |

## Stop-and-ask conditions (execute; never pre-decided)

- **S1 — parity precondition.** PL0 > −130 dB or PL0t > 1e-7, or a PL / PM / PE row fails while PL0 passes. STOP: (a) find the init / tick-order / mapping difference (prepare mirror, slot snap, rate update cell, name map); (b) re-specify the family's metric (amends D27).
- **S2 — CPU over budget** after L0 + levers 2 → 3. STOP: lever 1 (Hermite; moves renders + PN, re-pin only with consent) or `grain_limit` 256 → 128 + "heavy" note (locked-spec change).
- **S3 — bypass broken.** A checkpoint, TB0 / TB3 / TB4, RB1 / FR1 or a lever moves a hash. STOP and find the statement-shape change. **Never re-baseline.**
- **S4 — gain band** \|g / g_PL − 1\| > 1e-4 on a non-control row. Diagnose the scalar; never widen.
- **S5 — law arm** (PV / VL / ME / NV / CP / NZ / BN / FA5). Structural; STOP with the diagnosis. Never loosen.
- **S6 — clicks.** An RT / RE / LM row over bound, or a hard control that cannot fire after one stimulus fix. STOP: longer slot ramp (a D27 change) or edit-shape fix — consent required.
- **S7 — tempo.** TS1 > 1 sample, TS2 off-grid, TS3 locks, TS6 burst missing / doubled. STOP with the clock trace.
- **S8 — fork / scope.** `ModSources.h` / `TempoTracker.h` don't compile through the alias, or a carried row other than N2 / N2s / FN3 needs a re-scope. STOP: a leaf edit or row change is the user's call.
- **Inherited:** T1 contract / pin / late-change mismatch; INVALID after its one stimulus step; three NOVERDICTs; unexpected paths at either commit.

## Risks

- **R1 Per-voice cost.** 13.4-pt headroom; the dominant term is per-tick quota / law recompute (R§11). C25m witnesses force the worst path; L0 is bitwise-neutral and first.
- **R2 Bypass shape.** Ten engine tasks touch the audio path; per-task checkpoints attribute any move.
- **R3 Parity phase.** Every PL row depends on the prepare mirror; PL0t catches it before any row counts.
- **R4 Tempo burst vs grid.** The burst is novel (no parent equivalent); TS6 + `tempoGridOnly` isolate it, and parity never sees it.
- **R5 Vacuous click controls.** RT trajectory arm is deterministic; RE / LM assert `ctrl FIRED`.
- **R6 2.5b compatibility.** Per-voice pressure / slide / bend fields + global `bend` in `voiceSemis` keep every 2.5a golden stable under `mpe_enable` off.
- **R7 Concurrent sessions.** Foreign dirty paths exist; both commits path-scoped with branch + status re-checked immediately before.
- **R8 Test hooks in shipping.** All under `OGI_TEST_HOOKS`; X2 and validators run on the shipping build.
