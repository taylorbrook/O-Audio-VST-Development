---
plugin: O-GranulatorInstrument
stage: 2
phase: "2.5a"
title: "Mod sources, the 16-slot matrix with per-voice evaluation, the newest-voice rule, Tempo"
executed: 2026-11-09
version: 0.1.0 (unchanged, PD21)
fork_pin: 82406fce
commits: C1 (global matrix + per-voice pass, T3–T11) + C2 (this phase)
result: executed — every automated gate green; S1 (PLs1), S6 (RE) and S8 (FL7) hit and resolved by the user; HANDS-ON-2.5a written and deferred with 2.1–2.4
---

# O-GranulatorInstrument — Stage 2 / Phase 2.5a SUMMARY

The engine now has its modulation system: 17 sources route through a 16-slot matrix to the 125 continuous destinations. Per-voice sources modulate each voice independently, and Tempo sync is live.

**What was built**
- **Grid (T3).** `ControlGrid.h` (ADAPTED) binds the 89 mod / sync rows and gains `struct Lfo` plus `using GridParams = GiGridParams`. The parent `ModSources.h` and `TempoTracker.h` compile **VERBATIM** through the alias (no S8). FORK-MANIFEST reclasses ModSources → VERBATIM, and ModMatrix → VERBATIM (reference only, ported). Drift 0 / 13.
- **Transport (T4).** The processor reads the playhead once per block into a `TransportSnapshot`, and `Engine::process` updates the forked tracker. Destination ranges are copied on the message / host thread only (`setDestRange`, P15).
- **Sources (T5).**
  - Global LFOs use the verbatim parent class, with a prepare that mirrors 82406fce :90-101.
  - Macros ramp over 20 ms.
  - The wheel is 14-bit. CC74 / channel AT / poly AT / bend / CC121 are parsed.
  - `GiModEnv` uses the AmpEnv math with stage-entry latching, never ends a voice, and has `retrigger()`.
  - `GiVoiceMod` (×20) holds per-voice LFOs (seed keyed on the serial), velocity / key / note random, and the 2.5b seam fields.
- **Global matrix (T6).** `GiModMatrix.h` is a line-for-line port of the parent ModMatrix sized 126 × 18:
  - slot compare → residual → 0 over 20 ms; depth restarts at 0; sum before one clamp; canonical +0;
  - only the touched set is rolled;
  - ramped G destinations use parent-R47 substitutes (output gain, level / pan, feedback λ, diffusion, saturation, cutoffs); reverb uses tick-rate atomics;
  - spawn-latched fields are re-evaluated at the spawn sample into a `local` copy built before any draw.
- **Per-voice pass (T9).**
  - Per-voice slots (per-voice source → class-V destination) sum into each voice's sparse offsets, with per-voice residuals.
  - Pair views are built from the raw value + offG + offV through one clamp (P3).
  - Quotas, coherence, law, tap, scale window, scan and the spawn read the view.
  - A per-voice loop region moves only that voice's read region (P17).
  - Offsets are seeded at note-on, so the first grain is already modulated.
- **Pair mixer mode (T10).** A stream with a per-voice level / pan offset mixes per pair, while unrouted streams keep the 2.4 statements.
  - Before: `aL[c] += a * bl[c]`, then `y = acc · level · aud · vgain · pan`.
  - After (pair mode only): `aL[c] += a * (bl[c] · gL_kS(c))`, then `y = acc · aud · vgain`. The tap carries `min (level_kS, 1)` in its own statement.
- **Newest-voice rule, mode crossfade, telemetry (T11).**
  - The newest voice is the max serial among counted voices, taken at the tick.
  - One shared 20 ms ramp runs from the current blend, never stacked; the value holds when silent; it starts at 0.
  - `lfoN_mode` crosses over 20 ms, and the per-voice LFO is re-prepared at `lfoN_phase` on Global → Per-voice.
  - UI telemetry is relaxed atomics: dest v01, sources, LFO phase. Nothing writes APVTS.
- **Tempo (T12).**
  - `GiTempoClock` ports the parent `processTempo`, with the absolute-time grid point emitted at ceil(t).
  - Effective values follow R45.
  - Thinning works on the division: `div·2^j`, j = min{j ≥ 0 : I·2^j ≥ T(1 − 1e-6)}.
  - USER PD1: one burst onset at the note-on sample on `kBurstBit | n`, then the grid strictly after it.
  - Free ↔ Tempo folds the epoch.
  - The per-voice Tempo LFO free-runs at tempo rate from note-on (P5).

## Results

| Gate | Result |
|---|---|
| Build | harness, param-dump, VST3, AU and Standalone all rc 0; **0 warnings** |
| Checkpoints T3–T12 (+T8b, T14) | renders-2.4 **byte-identical**, legacy == renders-2.2, param-dump identical at **every** engine task. Nothing re-pinned |
| Default harness | **231 PASS / 0 FAIL**, **0 NOT FIRED** (179 + 19 eg25a + 33 eg25b; target 231). The duplicate-ID set equals the 2.4 log's (the plan's ST1 is renamed AP1 because eg22 owns ST1) |
| `--parity` | **168 / 0** (133 carried + 35; target 168). `--rows GV` **2 / 0**. The carried PN / PF / PB / PR residuals are identical to T1 (0.1 dB); every 2.2–2.4 parent artefact is byte-identical except manifest.tsv, which only gained the new rows |
| `--cpu25a` (alone) | **C25m 28.18 %** (≤ 40, margin 11.8): routed per-voice dests min 16 / tick, quota recomputes 100 % of ticks, pair-mode streams min 4, live 243.2. **C25tm 6.68 %** (≤ 12, live 80.2). Same session: C25w 26.75 %, C25t 5.77 %; Δ C25m − C25w **+1.43** points (projection +2.5…5.5). Levers not fired (T19 skipped). T2 reference on the unmodified 2.4 engine: C25w 25.87 % / C25t 5.45 % |
| Drift | 0 / 13 (ControlGrid col 5 re-hashed) |
| T20 | `build-and-install.sh` rc 0 (dual-variant sweep). Standalone rebuilt explicitly (binary 11:58:51 > harness 11:57:05), alive after 6 s. **auval PASS** (ZZZZ control rc 2). **pluginval 10 VST3 + AU SUCCESS**. **X1** 0 / 151 compdb entries reference the parent. **X2** 0 `ogran::` in the installed VST3, the installed AU and the harness; control: the parity binary has 76 |

### Parity (DSP-06 shared path, vs 82406fce)
| Row | Result |
|---|---|
| PL0 | **−146.0 dB**, g_PL 0.9977000 (no route, every source busy) |
| PL0t | per-tick s1_level trace **bitwise** == parent: max diff 0 over 13 891 ticks (P4 prepare mirror) |
| PL1–10, PL14, PL15 | −143.8…−146.2 dB |
| PL11–13 (Tempo) | −147.0…−148.7 dB |
| PM1–4 | −145.9 dB |
| PE1–6 | −143.0…−145.7 dB |
| Band | \|g / g_PL − 1\| ≤ 8.8e-8 on every row |
| PLs1 | **re-specified (S1, user 2026-11-09)**: the per-grain latched len equals the parent's grains.tsv **bitwise** (10 grains, len 2033–9482 samples); control lfo1_phase +1° → 1.4e-2. The audio null −25.0 dB is INFO, because the parent ramps g_out over 20 ms while ours steps the law per cell (since 2.2), and size moves the law |
| Controls | PLc1 −33.6, PLc2 −5.5, PLc3 −0.0, PMc1 −16.9, PEc1 −17.3, PEc2 −16.3, PEc3 −42.8 dB: all fire |

### Law arm (eg25a, 19 / 0)
- **PV1:** 108 grains of 3 voices; len 0 ulp vs `apply (raw, offV_k)`, cutoff rel err 0. Control pvSharedSource → 1 distinct len.
- **Per-voice LFO:**
  - VL1: each voice's first tick is −0.5 exactly (pvLfoNoReset → −0.64).
  - VL2: slot offsets 0 / 7 bitwise (slotKeyedSeed → 71 998 differ).
  - VL3: 0 / 13 172 tick values differ from a voice-free recompute over 64 serials with 59 steals.
- **Mod Env:**
  - ME1 ≤ 1e-6 vs the closed form, including the −96 dB end snap.
  - ME2: coefficients bitwise at N = 96.
  - ME3: 0 / 4000 differ (envLiveTimes → 4000).
  - ME4: retrigger continues from 0.5.
- **Source values:** RG1 20 / 20 exact; MD1 10 / 10, with global sources at the next tick.
- **Neutral and clamp:** NZ1 bitwise (macro 0.1 % fires); CP1 is +6 dB exactly, then gain 0, with no NaN.
- **Newest voice:**
  - NV1 1.000× the bound (newestNoRamp 30×).
  - NV2 holds bitwise.
  - NV3: 8 handovers, never a stolen spare.
  - NV4 1.000× with no overshoot (newestStack 234×).
- **Coverage:** SD1 17 / 17; SD2 8 / 8; DR1 moves 125 / 125 consumers, routedCount == 1 on 125 / 125, MX0 table 0 mismatches.

### Edits, Tempo, invariance, bypass (eg25b, 33 / 0)
- **RTa–g** (trajectory, parent MX3 bound): 1.00× on every kind × {global LFO, per-voice LFO, Mod Env}, with 0 violations and 0 cross-talk. RTn 29.9×.
- **REa–g:** **re-specified (S6, user 2026-11-09)** to PD14's same-motion reference — the identical gain change made by the s1_level parameter's own 20 ms ramp. Readings are 0.24–1.07× (≤ 2.0), and REn (noDepthRamp) is 25.5×. Held-reference κ for the LFO classes is INFO (1.9–5.5× on 110 Hz material: the 20 ms ramp's slope kink against the sine's own curvature).
- **LM1:** κ 1.36×, trajectory 0.59 (switch at the LFO peak); LM1n 115×.
- **Tempo:**
  - TS1: 465 grid onsets over 37 runs (44.1 / 48 / 96 k, 4 divisions incl. the R45-clamped 1 bar, 3 tempi, step + loop); max |onset − grid| 0.99 samples, 0 missing, 1 burst per run.
  - TS1n: 2 dropped. TS2: 0 off-lattice, 0 non-power-of-two intervals. TS2n: 224 off-lattice. TS3: 0 % locked.
  - TS4: 0 / 4500 bitwise across a re-lock. TS5: bitwise.
  - **TS6:** burst at 10 000, grid at 12 000 (tempoGridOnly → 12 000). TT1: 0 differ.
- **BN1:** 0 diffs at 441 / 512 / 4096 / var (BN1n 287 984 differ).
- **FA5:** 0 allocations (24 handovers, 18 steals, re-lock, 96 bursts, 3750 recomputes).
- **Bypass:** TB3 14 / 14 + legacy 14 / 14; TB4 14 / 14; AP1 0 writes in 37 dsp files (control: the processor's replaceState path).

## Plan decisions as executed
PD1–PD21 executed as written, except for the deviations below.

## Deviations (each with its reason)
1. **FL7 re-scoped (S8, user).** A 4th carried row randomised the newly read mod params; it moves to readSet25a 251 / 10 like N2 / N2s / FN3.
2. **PLs1 metric (S1, user).** Per-grain latch instead of the audio null (see Parity).
3. **RE metric (S6, user).** The same-motion reference (PD14 wording), not held references (T14 wording).
4. **C1 scope.** C1 covers T3–T11, not T3–T7: T9 was written while the T8 parity ran, so it was checkpointed and committed with C1. PL / PM / PE were re-run on the T9 and T11 binaries with identical residuals.
5. **PL11–13 + PLc3** were evaluated after T12. With `sync_mode` Tempo the parent's streams also run on the R41 grid, which needs our Tempo stream path; the plan's "streams Free" assumption did not hold.
6. **PL0t** traces the PL1 stimulus (LFO → s1_level). That is stronger than tracing an unrouted level.
7. **Tempo thinning tolerance.** Thinning compares at 1e-6 relative, not 1e-9: the float effective density rounds by ~6e-8, which at 1e-9 halved some grids. Found by TS1 / TS2 / TT1 and fixed before the full run.
8. **TB3 / TB4 exclude Bend.** Bend moves pitch by design (voiceSemis), and the N1 recipe bends. Bend-neutral routing is covered by NZ1 and Bend as a source by SD1 / MD1.
9. **C25tm stimulus fix.** One allowed fix (PD18): the Mod Env → size route depth went 20 → 5, because 20 put live at 113 (INVALID); the measurement was 9.33 % even then.
10. **Row renamed.** Plan ST1 → **AP1** (the ID collides with eg22's ST1). AP1's pattern also includes `replaceState`, so its control has a target: the processor's only parameter write.
11. **`rh::Overrides.midi`** added in `prepareJob` (the designated extension point) for TB3.
12. **Per-voice pitch_rand with a scale.** A per-pair window runs on the stream's ActiveSet, which is built at the global pitch_rand. This is an approximation, documented here.

## Requirements moved (verify sets the final status)
DSP-06 automated arm complete (the feel is HANDS-ON-2.5a). PERF-02: final worst case with the matrix, C25m 28.18 %. PERF-01: matrix / Tempo allocation arm (FA5). QUAL-01: route-edit / mode-switch gates.

## HANDS-ON-2.5a
Written (12 rows), deferred post-Stage-3 with 2.1–2.4. Not self-certified.
