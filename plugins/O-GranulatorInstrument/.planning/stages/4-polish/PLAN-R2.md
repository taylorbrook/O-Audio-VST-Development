---
title: "O-GranulatorInstrument Stage 4 (Polish / Validation) — Execution Plan, Rework R2 (feedback holds the lane level)"
created: 2026-11-13
juce_version: "8.0.15"
phase: "4-polish"
summary: "Five waves: (W0) pre-flight and baseline RB on the unmodified R1 tree, commit C1; (W1) the level-hold makeup in the record head (pristine-energy table, leveller in `writeCell`, stop-writing latch), the LH rows with controls, repair of CK1 / CK4 / CK7n; (W2) bank: level table R1 vs R2, `sN_level` trims on the level outliers, preset comparison rows, CPU, commit C2 (W1 + W2 land together, so `main` never carries a red FP2); (W3) build / install / validators / final regression; (W4) CHANGELOG, ARCHITECTURE addendum, HANDS-ON-SESSION-4 rewording + retest reset, SUMMARY-R2, STATUS, commit C3. VERSION stays 1.0.0. No tag. No push. No parameter, destination, page, string, module, `scripts/` or forked-leaf change. Render goldens never re-pinned."
stages: [4]
agents: [dsp-agent, polish-agent, build-gate-runner]
signoffs: "2026-11-13 Taylor: RESEARCH-R2 sign-offs 1–4 as recommended — 1 cap +40 dB (after dividing out κ); 2 stop-writing rule; 3 clean peak limit; 4 D72 outlier = more than 3 dB louder over the 10 s audition render or centroid moved more than 0.5 octave, level outliers trimmed with `sN_level` to within 1.5 dB of their R1 loudness, centroid outliers a listening call. Defaults taken in research without a question: gain never below 1, mean-level reference (not a contour), stereo-linked."
---

# O-GranulatorInstrument — Stage 4 Execution Plan, Rework R2 (feedback holds the lane level)

## Goal

Hands-on finding F3 is fixed before 1.0.0:
- **D70 / D71:** a lane under feedback keeps its loudness for as long as feedback runs. The lane still evolves (grain smear, filters, diffusion, saturation).
- **Sign-off 2:** when the keys are up and the return can no longer be held, the head stops writing instead of erasing the lane.
- **D75:** harness rows bound the lane level from below, each with a control that fails.
- **D72:** the 13 presets that change are measured before / after; level outliers are trimmed; the rest is a listening call in Pass 2.

**What must not move.**
- `renders-2.1 / 2.2 / 2.4.sha256` (42 lines) byte-identical at every step. Parity 168 / 0 + GV 2 / 0.
- Feedback 0, the toggle Off and Freeze write nothing; the tape stays bitwise pristine.
- With the makeup switched off by its test hook, the engine is bitwise the R1 engine: 512 rows with identical text, 64 preset renders bitwise.
- 51 presets render bitwise as on R1 with the makeup on.
- 293 parameters, 150 destinations, table version 2, `params.tsv`, param-dump, destination goldens, the FX page, every string.
- `forked/` drift 0 / 13.
- The loop bound: tape peak ≤ max (pristine peak, 0.7). FL1, FL1s, FL1p, FL1sp, FL2 / FL2p, CL1 pass unchanged.

**Out of scope (CONTEXT-R2 § Out of scope).** A sustain / decay control; keeping the fading loop as an option; any surface change; a version bump or tag; Windows hands-on, Reaper, Dorico, MPE; running the human session (verify's job); REQUIREMENTS.md edits (verify's job).

**Scope fence (the only paths R2 may change).**
- `Source/dsp/{GiRecordHead.h, SlotTape.h, SlotLoader.cpp, Engine.h, Engine.cpp}`
- `Source/FactoryPresets.h` (`sN_level` values only)
- `tests/render-harness/{engine_gates_r2.h (new), main.cpp, preset_gates_4.h, engine_gates_24b.h, engine_gates_r1.h}`
- `.planning/**`, `CHANGELOG.md`, `../../PLUGINS.md` (date cell)

Not touched: `Source/dsp/forked/**`, `GiParamIds.h`, `PluginProcessor.*`, `PluginEditor.*`, `Source/ui/**`, `CMakeLists.txt`, `tests/parity/**`, `tests/*.js`, `tests/golden/**`, `modules/**`, `scripts/**`, the mockups. A need outside the fence = STOP and ask.

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/4-polish/CONTEXT-R2.md` | D70–D77. Binding, except as amended by RESEARCH-R2's "Corrections" table and the sign-offs above. |
| `stages/4-polish/RESEARCH-R2.md` | The design (Q2–Q4), the measured results, gate design (Q7), pitfalls 1–10. Where this plan's "Research corrections" disagree, this plan wins. |
| `stages/4-polish/research-r2/prototype.patch` | The prototype. **A reference for the algorithm, not code to apply**: PD7 lists what the shipped form changes. |
| `stages/4-polish/research-r2/{r2probe.h, probe-final-*.txt, probe-r1-*.txt, cpu.txt}` | The probe's stimuli and the numbers the LH rows must reproduce. |
| `stages/4-polish/{PLAN-R1.md, SUMMARY-R1.md, VERIFICATION-R1.md}` | Carried procedure: PD3 (extract comparison), PD4 (regression set), PD29 (preset compare), PD33 (CPU), conventions, commit discipline. PLAN-R1 is executed and not edited. |
| `stages/4-polish/HANDS-ON-SESSION-4.md` | The sheet T15 edits; F3 in the triage table. |
| `.planning/{STATUS.md, REQUIREMENTS.md}`, `.planning/research/ARCHITECTURE.md` (D7 `:27`, write `:200`, bound `:417`, R1 addendum `:661`) | Contracts and checksums. |
| Root `CLAUDE.md` | Build / install / cache clearing, commit discipline. |

**Plan-time facts** (2026-11-13, HEAD `6be24716`; every anchor grepped by the planner; nothing built or run). Last commit under `Source/` or `tests/`: **`4c89247c`** (R1 C6).

| File | Facts |
|---|---|
| `Source/dsp/GiRecordHead.h` (212) | header comment `:29-49`; `RecordHeadFlags :62`; `RecordHeadCounters :69` (`rClamps :72`); `RecordHead :79` (`returnPk :84`); `writeCell :140`; λ `:175`; writer clamp `:181-186`; convex write `:187`; scrub `:188-195`; envelope `:198` |
| `Source/dsp/SlotTape.h` (53) | "Built and destroyed OFF the audio thread" `:29`; struct `:37-52` |
| `Source/dsp/SlotLoader.cpp` (1838) | `buildTape :453`; `flushTail :504`; callers `:185, :665, :968` |
| `Source/dsp/Engine.cpp` (4486) | prepare: `resetTailHold :214`, `pkRel :268`, heads `:269-280`; `chainKnobParity` read `:3836`; `updateHeads :3939` (`returnPk :3998`, `voiceTerm :4003`, `resetTailHold :4010-4012`, run target `:4013-4014`, `parkedNow :4033`); `writeHeads :4041` (parked skip `:4055`, `writeCell :4077`) |
| `Source/dsp/Engine.h` (1062) | `EngineTestHooks :227` (`chainKnobParity :268`, `rClampOff :310`, `injectReturn :314`, `preHoldOff :334`); `head[] :914`; `pkRel :917`; `resetTailHold :1052` |
| `Source/dsp/VoiceAllocator.h` | `gated :61` ("held by CC64 after key-up"); `isCounted :93` (gated **or** releasing) |
| `Source/FactoryPresets.h` (1093) | `sN_level` in dB (for example `:132`); `bankHash :1075` (computed, not pinned) |
| `tests/render-harness/engine_gates_24b.h` | `ckChain :261`; `ckRender :266`; `ckRow :281`; `rowCK1 :301`; `rowCK4 :349` |
| `tests/render-harness/engine_gates_r1.h` | `rowCK7 :2219`; `infoPreTail :2495` |
| `tests/render-harness/preset_gates_4.h` | FP2 `:714`; `kShowcase :1319`; compare loop `:1396-1530` |
| `tests/render-harness/main.cpp` | `--egr1 :1823`; `--cmp-preset-renders :1843-1850`; `rowHM1() :1857` |
| `CHANGELOG.md` | one heading `## [1.0.0] - 2026-11-12 :3`; "Feedback and space" `:13` |
| `HANDS-ON-SESSION-4.md` | 2.4-1 – 2.4-7 `:77-83`; 3.2-10 / 11 `:126-127`; 3.2-24 `:135`; section 1d `:165`; R1-PRE1 `:176`; F3 `:433`; Results `:435-447` |
| `build/plugins/O-GranulatorInstrument/` | `r1-b0/`, `r1-b1a/` present (R1 references; not R2's) |
| Working tree | Dirty under the plugin: `.planning/STATUS.md`, `CONTEXT-R2.md`, `RESEARCH-R2.md`, `research-r2/`, this file; untracked `stages/3-gui/gate-report.json` (never staged) |

---

## Research corrections (checked at plan time)

| # | RESEARCH-R2 says | Plan |
|---|---|---|
| 1 | The prototype is the design | It has a latent defect: one non-finite return sample makes the running means B, C and the held detectors non-finite, and they stay so until the head parks (`std::max (NaN, x)` returns NaN). The gain then sits at 1 and the lane fades again. PD6 adds the scrub; LH12 gates it. |
| 2 | Parity: makeup off in `ParityRows.h` / `parity_gates_24.h` | The engine derives it: makeup off = `hooks.makeupOff \|\| hooks.chainKnobParity`. No parity file changes (`tests/parity/**` stays outside the fence). The parity run, not measured in research, is measured at T4. |
| 3 | Full-harness result 508 / 4 | Run at X = +6 dB; the shipped value is +9 dB. The moved-row list is therefore re-established at T7, under PD3. |
| 4 | PT rows: "re-dump the baseline for the 13" | The R1 references (`r1-b0`) predate the showcase re-voices. R2 takes its own reference `$RB` from the unmodified tree at T2 and retires the `r1-b0` comparison (PD9). |
| 5 | LH8 "default patch and one preset" | The Pre worst case (Mix 100 / Size 100: −5.4 dB) is outside a −3 dB band by measurement. It is an INFO line plus a hands-on row, not a gate (OS2). |
| 6 | 44.1 / 96 kHz not measured | LH13 measures both. Every constant is in seconds or dB per second (PD7). |
| 7 | Files: `Engine.h` hooks list of six | Eight (PD8): two more for the corrections above. |
| 8 | 14 presets carry feedback, 13 change | T1 re-counts from `FactoryPresets.h`; the 13 names become a constant in the harness (PD9). |

## Open sign-offs

Defaults are taken so execute is not blocked.

| # | Question | Default taken | Alternative |
|---|---|---|---|
| **OS1** | Which streams take the level trim (sign-off 4 names `sN_level`, not the streams) | **Every enabled stream of the preset, by the same amount.** The balance the preset was authored with at note-on is kept. | Trim only the streams that carry feedback: the other streams keep their R1 level, the feedback stream starts quieter. |
| **OS2** | Latch threshold. The approved rule was measured at 6 dB / 50 ms; the Pre long-tail case loses 5.4 dB at note-off | Ship **6 dB**. T6 also measures 3 dB through a test hook and prints it as INFO (LH8i). Decided at hands-on row R2-3. | Ship 3 dB if LH8i shows the Pre case inside −3 dB and no other LH8 arm gets worse. Changing the shipped value = stop and ask. |
| **OS3** | A sparse lane (hits with gaps) becomes a steady cloud at its mean level (research default, not asked) | Ship it; hands-on row R2-9 asks. | A contour reference. Not prototyped; would be a new research pass. |
| **OS4** | CK1 / CK4 / CK7n cannot be made to fire again by the PD10 ladder | T7 **stops and asks**. | Candidate then: run the three rows' subject, static and control arms under `makeupOff` (worst-case splice, hook-only), recorded as such. |

---

## Plan-phase decisions

### Order, baselines, gates

**PD1 Landing order.**

| Step | Task | What lands | Compared with |
|---|---|---|---|
| RB | T2 | Nothing (unmodified tree) | It is the reference |
| M1 | T3 | Pristine-energy table on the tape; nothing reads it | RB: extract text identical, 64 preset renders bitwise |
| M2 | T4 | Makeup gain in `writeCell` | Makeup off: RB bitwise. Makeup on: fb-0 goldens, 51 presets bitwise, 13 differ |
| M3 | T5 | Stop-writing latch | As M2 |
| M4 | T6, T7 | LH rows; CK repairs | — |
| M5 | T9, T10 | `sN_level` trims | 51 presets still bitwise against RB |

One task at a time (shared build directory; a render move is attributed to one task).

**PD2 Baseline storage.**
- `$RB` = `build/plugins/O-GranulatorInstrument/r2-b0/` (gitignored): 64 raw `.f32` renders of the unmodified tree + sha256 list. Do not delete `build/` during R2. If `$RB` is lost → STOP and ask.
- `$E` = `.planning/stages/4-polish/evidence-r2/` (committed, text only): `B.ids.txt`, `B.full.txt`, `rb-preset-renders.sha256`, `rb-parity.txt`, `rb-cpu.txt`, `rb-levels.txt`, then one notes file per wave and the logs worth keeping. Evidence, not goldens.

**PD3 Extract comparison** (PLAN-R1 PD3, restated for R2).
- `ids` (PASS / FAIL + row id, sorted) for the 512 carried rows equals `B.ids.txt` at every step, with one planned exception: FP2 may read FAIL between T4 and T10.
- **Makeup off** (PD8 switch): the `full` extract equals `B.full.txt` exactly.
- **Makeup on:** a carried row's text may move only if its stimulus has a feedback Amount above 0 on a running head. For every moved row the executor records old → new and names the feedback setting that explains it. A moved row whose stimulus has feedback 0, the toggle Off or Freeze from load = STOP (the bitwise no-op broke).
- Rows expected to change verdict: CK1, CK4, CK7n (control stops firing; repaired at T7), FP2 (repaired by the trims). Any other verdict change = STOP and attribute.
- Expected text moves that are not verdict changes (research): FL1 / FL1s / CL1 printed peaks and margins (CL1's margin falls to about 0.0002: the knee is an asymptote), INFO `PreTail` 9.7 s → about 1.8 s.

**PD4 Carried regression set.** PLAN-R1 PD4 **R-core** unchanged, run at the end of W1 + W2 and on the final tree: builds with 0 warnings in `Source/`; full harness under PD3; render hashes + legacy + pinned, `ctrl FIRED`; `tests/parity/run.sh` 168 / 0 + GV 2 / 0 with PB / PR residual lines equal to `rb-parity.txt`; param-dump == `.planning/params.tsv`; fork drift 0 / 13; `--dump-ui-overrides` byte-identical; `ui_frontend_check.js` ALL PASS + `--nc` 4 / 4; real-home listings unchanged.
- **R-ui** is run once, at T13, as a no-change proof (no page, string or parameter file is in the fence).
- **R-val** at T13.
- Exit 77 anywhere = not run = not a pass.

### DSP

**PD5 Reference table.**
- `SlotTape` gains the cumulative sum of L² + R² per 256 level-0 frames (`double`), built in `SlotLoader::buildTape` from the pristine PCM after `flushTail`, on the worker thread. About 90 KB at the 2.88 M-frame cap.
- `refMeanSq (lo, hi)`: mean square per sample and channel over the bins that cover `[lo, hi)`, O(1), `noexcept`, 0 for an empty table.
- The head reads it with its **latched** `lo / hi`, so a region change is picked up at the next latch.
- The table is owned by the tape, so it is freed where the tape is freed (off the audio thread, `SlotTape.h:29`).

**PD6 Makeup law** (RESEARCH-R2 Q2–Q4; per 32-sample cell, at the tick, before the write).
1. Per-cell sums over the frames written: A = wc², B = wc·r, C = r² (both channels, r **before** the makeup), and the return's cell peak and cell mean square.
2. **Scrub (correction 1):** if any return sample of the cell is non-finite, the cell updates none of the running values and writes with the previous gain. The existing write scrub still replaces a non-finite `w` by `wc`.
3. Running one-pole means (τ = 1 s) of A, B, C and of the constant 1 (u). Target `T = ref · u`.
4. Solve `λ²C·g² + 2λ(1−λ)B·g + (1−λ)²A = T` for g, λ taken at mid-cell from the two existing ramps.
5. Limits, in this order: cap `g ≤ 100 · κ(λ)`, κ = √((2−λ)/λ); peak `g ≤ 0.7 / pk` (pk = held return peak, 50 ms release); short-term ceiling `g ≤ κ · √(X · ref / Cf)` (Cf = cell mean square held with a 50 ms release, X = +9 dB as an energy ratio); floor `g ≥ 1`; a non-finite g becomes 1.
6. The gain may fall to its new value inside one cell. It rises at no more than 375 dB / s (0.25 dB per cell at 48 kHz). It is applied as a linear ramp across the cell.
7. `rw = clamp (g · r, ±0.7)`, then the convex write, scrub and envelope exactly as today.
8. A parked head resets the makeup state.

Fixed constants (sign-offs; changing one = stop and ask): cap +40 dB, X +9 dB, τ 1 s, peak and Cf release 50 ms, rise 375 dB / s, floor 1, stereo-linked.

**PD7 Shipped form (what differs from the prototype).**
- No process-global configuration. Constants are `inline constexpr` in `GiRecordHead.h`; rate-dependent coefficients live in one small struct formed once in `Engine::prepare` (the only place `exp` / `pow` run) and passed to `writeCell` by reference.
- At the tick: arithmetic and `sqrt` only.
- Every time constant is in seconds, the rise limit in dB per second, the latch time in seconds: 44.1 and 96 kHz behave the same (LH13).
- With the makeup off the write path executes the same floating-point statements as today: the multiply by the gain is inside the `if`, not a multiply by 1. `rw` is clamped between the multiply and the convex write, so the write statement keeps its shape.
- Gain statistics (mean, min, max, counts of capped / peak-limited / ceiling-limited cells) sit behind `OGI_TEST_HOOKS`. The one value the latch needs (the count of consecutive short cells) is always present.
- The header comment of `GiRecordHead.h` and the run-condition comment above `updateHeads` are rewritten to the R2 law in the same task (they are compiled files).

**PD8 Hooks** (all inside `EngineTestHooks`; X2 must still find 0 hook symbols in the shipping binaries).

| Hook | Effect | Used by |
|---|---|---|
| `makeupOff` | g = 1, no latch: the R1 engine | Neutrality proof, LH1n, PT3n. Also implied by `chainKnobParity` |
| `mkAttackSlew` | The gain falls at the rise limit too | LH4n |
| `mkLatchOff` | No stop-writing latch | LH8n |
| `mkPeakOff` | No peak limit | LH10n |
| `mkCapOff` | No cap | LH7n |
| `mkWholeTapeRef` | Reference = whole tape, not the latched loop | LH6n |
| `mkStatScrubOff` | Non-finite return cells enter the running values | LH12n |
| `mkLatchOnCounted` | The latch clears on any counted voice (a releasing one included) | LH14n |
| `mkLatchDb` | Latch threshold override (default 6) | LH8i INFO only |

A harness-wide switch (`--makeup-off`, or an environment variable read by the harness only) sets `makeupOff` on every session, so the whole run can be compared with RB. Test code only.

**PD9 Stop-writing latch** (in `Engine::updateHeads`, beside `resetTailHold`).
- Per stream. **Set** when no gated voice drives the stream (enabled, audible, `isCounted() && gated`) and the levelled return has been more than 6 dB short of the κ-normalised target for 50 ms of written cells.
- **Cleared** by the next gated voice on the stream. A releasing voice does not clear it (pitfall 7). A voice held by the sustain pedal is gated, so the head keeps writing under the pedal.
- While set, the run target is false: 20 ms run ramp, park, as for Freeze. It is a second, independent term next to `resetTailHold`.
- Recomputed at every tick; it does not sit behind a dirty check (repo memory: a run flag past a dirty check latches).
- `prepare` clears it.

### Rows

**PD10 Click-row repair (CK1, CK4, CK7n).** These rows erase a 440 Hz sine lane through an empty chain pass band so that a hard stop leaves the worst-case splice. The makeup now refills the lane, so the hard controls read under 2.0×. Ladder, in order, stopping at the first step where all three controls fire and all three subjects pass:
1. Lower the tape's sine in `ckRender` (220 → 110 → 55 Hz) until the chain loss exceeds the cap. Validity clause added to each row: the makeup is cap-limited on more than 90 % of cells in the static render, and the lane at the stop point is at least 20 dB under pristine. An INVALID is not a pass.
2. Move the event time or the measurement window.
3. STOP and ask (OS4).

Fixed: the 2.0× threshold; the subject always runs the shipping path (never `makeupOff`); `ckRender` is shared by other CK rows, so every row that uses it is re-read and its old → new values recorded (CK2 / CK3 stay INFO).

**PD11 New rows.** New header `tests/render-harness/engine_gates_r2.h`, namespace `egr2`, flag `--egr2`, in the full run after `egr1::runAll()` and before `rowHM1()`. Every row asserts its stimulus fired before its result; every `n` control runs the same predicate on a broken input and must FAIL it. Lane level = RMS over the head's latched loop at tape level 0, in dB re its value at t = 0.

Base stimulus **S0** (LH1–LH3): default patch, stream 1, 4 s stereo noise tape at 0.1 peak (−24.8 dBFS RMS), chord 48 / 55 / 60 held, 60 s, readings at t = 0 and at every lap from 8 s.

| Row | Stimulus | Pass | Control |
|---|---|---|---|
| **LH1** | S0, Amount 100 %, Saturate 0 / 20 / 100, Post | every reading within −3 / +2 dB | `makeupOff` fails, and falls more than 20 dB (stimulus validity) |
| **LH1p** | LH1 in Pre (Mix 50) | −3 / +2 dB | `makeupOff` |
| **LH2** | S0, Amount 50 / 25 % | −3 / +2 dB | `makeupOff` |
| **LH3** | S0, Pre, Mix 100, Size 100, Saturate 20 / 100 | −3 / +2 dB | `makeupOff` |
| **LH4** | −55 dBFS noise tape, 60 s; second arm: a 10 s Freeze pulse mid-hold; third arm: toggle Off / On | max reading ≤ +3 dB in every arm | `mkAttackSlew` exceeds +3 dB |
| **LH5** | Zero tape; 1e-5 tape | zero stays bitwise zero; 1e-5 never reads above +2 dB | — (bound row) |
| **LH6** | Two-level tape (first half 20 dB down), loop on the quiet half | within −3 / +2 dB of that half | `mkWholeTapeRef` fails high |
| **LH6u** | Unit: `refMeanSq` against a direct sum, 3 tapes (one of a length that is not a multiple of 256, one shorter than a bin), 40 regions each | bin-aligned regions exact to 1e-12 relative; any region on stationary noise within 0.5 dB | an off-by-one bin index fails |
| **LH7** | 55 Hz sine tape under Low Cut 2000 Hz, Amount 100 % | lane fades more than 20 dB in 60 s (D71: filtering to nothing still fades) | `mkCapOff` fades less than 20 dB |
| **LH8** | Default patch (Post) and Drones · Tape Halo: 20 s phrase, release, read 20 s later; third arm: sustain pedal down at release | lane within −3 dB of its level at release; under the pedal the write counter keeps growing | `mkLatchOff` falls below −3 dB |
| **LH9** | One Post and one Pre makeup render at block 512 / 32 / 1 / seeded variable, and a repeat | output and tape hashes all equal | — |
| **LH10** | The 13 presets, held chord 60 s | return samples clamped ≤ 1 % of written samples on every feedback stream | `mkPeakOff` exceeds 1 % on at least one |
| **LH12** | S0 at Saturate 20; one non-finite return sample injected at 10 s (`injectReturn`) | readings from 12 s on within −3 / +2 dB; tape finite | `mkStatScrubOff` falls out of the band |
| **LH13** | S0 at Saturate 20, sessions at 44.1 and 96 kHz; LH8's default arm at 96 kHz | same bands | — |
| **LH14** | LH8's default arm; after the latch sets, (a) a note releases into the parked head, (b) a new note | (a) 0 writes during the release; (b) writes resume within the 20 ms ramp + 1 cell | `mkLatchOnCounted` writes during (a) |
| **LHB** | The 13 presets, held chord 60 s, **after the trims** | every feedback stream's lane within −3 / +2 dB at 60 s and never below −4 dB at the marks (2, 5, 10, 20, 30, 45, 60 s) | `makeupOff`: at least 11 of the 15 streams end more than 9 dB down |
| **LHF** | S0 for 20 s, then Reset, then a Record-in commit, heads writing throughout | 0 allocations and 0 frees on the audio thread (FA4's counter) | FA4's control |

INFO lines (not rows): **LH8i** (LH8's arms + the Pre Mix 100 / Size 100 arm at latch thresholds 6 and 3 dB); **LH11** (600 s drift on four presets; behind `--egr2-long`, not in the full run); **LHg** (mean / max makeup gain and capped-cell share per preset stream).

- LH7's stimulus was not run in research. If it does not fade by 20 dB the stimulus may be made harder (lower sine, lower High Cut); the predicate and control stay. Recorded as a deviation.
- LH10 exists because the writer clamp hides a makeup that overdrives (repo memory: a downstream guard masks the defect; gate the clamp target).
- A 0.25-peak noise tape is not used for LH1: it measures the peak limit, not the hold.
- Names grepped at plan time, 0 hits in `tests/` and `Source/`: `egr2`, LH1–LH14, LHB, LHF, PT3. T1 greps them again.

**PD12 Preset comparison** (extends PLAN-R1 PD29 in `preset_gates_4.h`).
- A constant `kFeedbackMoved` lists the 13 names (Iron Sky, Deep Well, Organ Loft, Hollow Wind, Low Orbit, Tape Halo, Night Engine, Feedback Loop, Four Lanes, Fifth Bloom, Slow Tide, Dotted Echo, Broken Radio).
- `--cmp-preset-renders $RB --r2`:
  - **PT0r:** the other 51 are bitwise equal to `$RB`.
  - **PT3:** each of the 13 differs from `$RB` (`differ > 0`).
  - **PT3n:** the same compare under `makeupOff` → the 13 are bitwise, so PT3's predicate fails.
- `--cmp-preset-renders $RB --all-bitwise` with the harness-wide makeup-off switch: PT0 64 / 64 (T3 without the switch; T4, T5 with it).
- The `r1-b0` compare (PT1 38 / PT2 16) is retired: six of its members now carry the makeup. SUMMARY-R2 says so.
- After the trims, `--dump-preset-renders` into a scratch directory twice: sha lists equal → `$E/r2-preset-renders.sha256` (verify re-dumps and compares).

**PD13 Level table (D72, sign-off 4).** New mode `--preset-level-table <refDir>` in `preset_gates_4.h`: per preset, RMS over 0–10 s and 6–10 s and the magnitude-weighted spectral centroid of the mono sum over 2–10 s, for the reference raw and the current render, with the differences in dB and octaves and an outlier flag. It prints; it is not a row.
- Sanity check at T9: before any trim, the table agrees with RESEARCH-R2's preset table within 0.5 dB and 0.15 octave on the 13 (the shipped law is the prototype's law). A larger gap = STOP and find the difference.

**PD14 Trims.**
- **Level outlier:** more than 3.0 dB louder than `$RB` over 0–10 s, by T9's table. Expected about nine (research: Iron Sky, Deep Well, Hollow Wind, Tape Halo, Night Engine, Feedback Loop, Broken Radio; Low Orbit and Slow Tide at the line).
- Trim: `sN_level` on every enabled stream of that preset, same amount (OS1), until the 0–10 s level is within 1.5 dB of `$RB`. At most three passes per preset. Never `output_gain`.
- **Centroid outlier** (more than 0.5 octave): no value changes in execute. Listed in the sheet for Pass 2 (R2-6).
- After the trims: FP0–FP6 with controls, FP2 spread ≤ 9 dB, LHB, PT0r / PT3.
- A preset that cannot meet FP2 by level alone, or needs any other value changed = STOP and ask.
- The bank hash is computed (`bankHash`), so the Factory folder is rewritten on the first run of the new build (checked at T13).

**PD15 CPU** (PLAN-R1 PD33, unchanged limits). Run alone, in the background, compared by the session factor: `--cpu25` (C25w ≤ 40, C25t ≤ 12), `--cpu4` (C25m ≤ 40, C25tm ≤ 12, INFO C25mPre, every preset T ≤ 12 % / W ≤ 40 %). Research: no measurable cost. Over a limit = STOP and ask.

### Release and docs

**PD16 Docs.**
- `CHANGELOG.md`: still exactly one `## [1.0.0] - <date>` heading, date = the day T14 runs. The "Feedback and space" bullet gains two plain sentences: a lane keeps its level while its stream's feedback rewrites it; and with all keys released the lane is left as it is. No "Fixed" section (nothing was released).
- `.planning/research/ARCHITECTURE.md`: one R2 addendum paragraph after the R1 one (the write law with the makeup, κ, the four limits, the pristine reference, the stop-writing latch replacing "the head writes for the tail's length", the unchanged bound). Checksum re-pinned in STATUS.
- `PLUGINS.md:76`: the date cell only.
- **REQUIREMENTS.md is not edited in execute.** SUMMARY-R2 lists what verify rewrites: DSP-04 (LH rows, lower bound), FUNC-08 (bank trims), PERF-02 (CPU lines).
- STATUS: a one-line progress note after every task (last task, next task), so a new session resumes from the file.

**PD17 HANDS-ON-SESSION-4** (edited in place; every carried row ID still present exactly once).
- **Reworded:** 2.4-1 (the lane evolves **and keeps its level** at Saturate 0 / 20 / 100); 2.4-4 (after release the lane is left as it is; no splice when playing resumes); 2.4-6 (a chord no longer thins the lane; the listening call about the chord fallback is closed); R1-PRE1 (after note-off the head stops within about 2 s instead of following the tail).
- **Reset to not-run** (the F3 retest list): 2.4-1 – 2.4-7, 3.2-10, 3.2-11, 3.2-24, 2.4-9, every row of section 1d, and the Pass 2 rows of the 14 presets with a Feedback Amount above 0. Each trimmed preset's row says "level trimmed at R2".
- **New rows (section 1e):**
  - R2-1 hold for 60 s at Saturate 0 / 20 / 100.
  - R2-2 release, wait 20 s, the lane is still there.
  - R2-3 Pre with a long tail: a dip of a few dB at note-off is expected; acceptable? (OS2).
  - R2-4 a loud dense file at Amount 25 %: settles lower, no distortion (sign-off 3).
  - R2-5 hold 60 s on Dotted Echo, Feedback Loop, Tape Halo, Low Orbit: the lane walks brighter; acceptable, or re-voice the cuts?
  - R2-6 the centroid outliers named by T9: character still right?
  - R2-7 a quiet lane and an empty lane are not lifted; no noise floor comes up.
  - R2-8 Record-in take, then feedback: holds at the take's level.
  - R2-9 a sparse lane (hits with gaps) becomes a steady cloud; acceptable? (OS3).
  - R2-10 crossed Low Cut / High Cut no longer fades a broadband lane; a low drone under a 2 kHz Low Cut still does.
  - R2-11 change the loop region while feedback runs: holds at the new region's level after one lap.
  - R2-12 hold the sustain pedal, release the keys: the lane keeps evolving; release the pedal: it stops.
- "What automation already covers" gains the LH rows. F3 in the triage table → resolved-pending-retest. Results table re-tallied.

**PD18 Release rules (carried).** VERSION 1.0.0. No tag. No push (D54's CI arm stays pending `/publish`).

---

## Tasks

**Roles.** **[dsp]** = dsp-agent writes engine and harness code. **[agent]** = polish-agent writes presets and docs. **[run]** = builds, gates, validators, commits.

**Conventions** (PLAN-R1's, unchanged).
- **BG** = `run_in_background`, logging to `$S/<name>.log` (`$S` = the session scratchpad). Anything over about 5 min runs BG (600 s watchdog): the full harness, `--cpu*`, pluginval.
- `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`.
- Paths are relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.
- **Commits.** Trunk-based on `main`. `git branch --show-current` and `git status --short` immediately before each. New files staged by explicit path; never `git add -A`, never `git commit -a`. Form: `git commit -- plugins/O-GranulatorInstrument PLUGINS.md`. Never staged: `modules/registry.yaml`, `.claude/*`, `.planning/STATE.md` (repo root), `plugins/O-GrainScatter/*`, `stages/3-gui/gate-report.json`.
- **Sessions.** A later session reads STATUS's progress note, re-runs T1's cheap checks (branch, hashes, `$RB` present) and continues.

### Wave 0 — pre-flight, baseline RB

1. **T1 — pre-flight** [run]. Output: `$S/preflight.txt`.
   - Contracts re-hashed against the STATUS frontmatter: 7 / 7.
   - Branch `main`; `git log 4c89247c..HEAD -- plugins/O-GranulatorInstrument/{Source,tests,CMakeLists.txt}` is empty.
   - Every Plan-time fact re-grepped; drift recorded.
   - PD11 / PD12 names grepped across `tests/` and `Source/` (excluding `ui/public/modules`): 0 hits.
   - The feedback presets re-counted from `FactoryPresets.h` (expect 14 with an Amount above 0, Frozen Field with Freeze on).
   - Real-home listings snapshotted (`~/Library/O-GranulatorInstrument`, `~/Music/Ouaricon`). Logic and Live not running.
   - Any mismatch → STOP and ask.
   - Depends on: none
2. **T2 — baseline RB + commit C1** [run].
   - Build the harness on the tree as it is. Full run (BG) → `B.ids.txt`, `B.full.txt` (expect **512 / 0**, 0 NOT FIRED).
   - `$H --dump-preset-renders` into `$RB` and into a second directory: the 64 sha lines equal → `rb-preset-renders.sha256`. `--cmp-preset-renders $RB --all-bitwise`: PT0 64 / 64.
   - Render hashes + legacy == goldens (`ctrl FIRED`); parity 168 / 0 + GV 2 / 0 → `rb-parity.txt`; param-dump; drift 0 / 13; overrides diff-clean.
   - CPU baseline, BG, alone: `--cpu25`, then `--cpu4` → `rb-cpu.txt` with the session factor.
   - Commit C1: `CONTEXT-R2.md`, `RESEARCH-R2.md`, `research-r2/`, `PLAN-R2.md`, `evidence-r2/`, STATUS. `git diff --stat -- Source tests CMakeLists.txt` is empty.
   - Files: `.planning/stages/4-polish/evidence-r2/*`
   - Depends on: T1

### Wave 1 — the makeup

3. **T3 — pristine-energy table (M1)** [dsp + run].
   - PD5 in `SlotTape.h` and `SlotLoader.cpp`. Nothing reads it yet.
   - Proof: harness builds with 0 warnings in `Source/`; full extract == `B.full.txt`; PT0 64 / 64 against `$RB`; render hashes.
   - Files: `Source/dsp/SlotTape.h`, `Source/dsp/SlotLoader.cpp`
   - Depends on: T2
4. **T4 — makeup gain (M2)** [dsp + run].
   - PD6, PD7, PD8 (every hook except the two latch hooks) and the harness-wide makeup-off switch.
   - Proof, makeup **off**: full extract == `B.full.txt`; PT0 64 / 64; parity 168 / 0 with residual lines == `rb-parity.txt` (the first measured parity run with the makeup in the tree).
   - Proof, makeup **on**: render hashes + legacy + pinned byte-identical; a dump of the 64 preset renders shows 51 sha lines equal to RB and 13 different, and the 13 are PD12's names.
   - Code read before going on: no `exp` / `pow` / `log` at the tick; no allocation; the makeup-off path has no added floating-point statement.
   - Files: `Source/dsp/GiRecordHead.h`, `Source/dsp/Engine.{h,cpp}`, `tests/render-harness/main.cpp`
   - Depends on: T3
5. **T5 — stop-writing latch (M3)** [dsp + run].
   - PD9 and its two hooks.
   - Proof: T4's makeup-off proofs again (the latch is off with the makeup); makeup on: render hashes; RS2p passes (the Reset hold and the latch are independent terms).
   - Files: `Source/dsp/Engine.{h,cpp}`
   - Depends on: T4
6. **T6 — LH rows (M4)** [dsp + run].
   - PD11 and PD12's `--r2` mode. `--egr2` first, then each row's control. Each row PASSES, each control FAILS its predicate, each validity clause holds. INFO lines LH8i, LHg recorded; LH11 run once with `--egr2-long` (BG).
   - LHB is written now and run on the untrimmed bank (expected to pass on level; it is re-run at T10).
   - A row that fails for a DSP reason: fix inside PD6 / PD9 without changing a fixed constant, then repeat T4's and T5's proofs. A fix that needs a constant changed = STOP and ask.
   - Compare with research: LH1 −1.3 … +0.8, LH1p −0.4 … +0.6, LH2 −0.4 / −0.8, LH3 −0.4 … +1.1, LH4 +0.6, LH8 −0.1 … −0.7. A reading more than 1 dB from these is explained before going on.
   - Files: `tests/render-harness/{engine_gates_r2.h, main.cpp, preset_gates_4.h}`, `Source/dsp/Engine.{h,cpp}` (hooks; fixes only if a row demands one)
   - Depends on: T5
7. **T7 — click-row repair + moved-row list** [dsp + run].
   - Full harness (BG), makeup on. Build the PD3 moved-row list (old → new, with the feedback setting that explains each).
   - Repair CK1, CK4, CK7n by the PD10 ladder. Record the step that worked and every `ckRender` user's old → new values.
   - Expected end state: every carried row PASS except FP2; every control fires; 0 NOT FIRED; 0 INVALID.
   - Files: `tests/render-harness/{engine_gates_24b.h, engine_gates_r1.h}`
   - Depends on: T6

### Wave 2 — bank, CPU, commit

8. **T8 — level-table mode** [dsp + run].
   - PD13. Proof of the mode itself: under the harness-wide makeup-off switch every difference against `$RB` prints 0.0; with the makeup on, the 51 unchanged presets print 0.0 and the 13 do not.
   - Files: `tests/render-harness/{preset_gates_4.h, main.cpp}`
   - Depends on: T7
9. **T9 — before / after table** [run].
   - `--preset-level-table $RB` → `$E/rb-levels.txt`. PD13's sanity check against the research table.
   - Output: the level-outlier list (trimmed at T10) and the centroid-outlier list (named in the sheet at T15).
   - Depends on: T8
10. **T10 — trims** (PD14) [agent + run].
    - One preset at a time: trim, rebuild, re-read its table line. Record the dB applied per stream.
    - Then: FP0–FP6 with controls (FP2 spread ≤ 9 dB); LHB; `--cmp-preset-renders $RB --r2` (PT0r 51 / 51, PT3 13 / 13, PT3n fails); the final table → `$E/r2-levels.txt`; `r2-preset-renders.sha256` from two equal dumps.
    - Files: `Source/FactoryPresets.h`
    - Depends on: T9
11. **T11 — CPU verdicts** (PD15) [run, **BG, alone**].
    - `--cpu25`, then `--cpu4`. Record the session factor, C25w, C25m, C25mPre, C25tm, bank T median / p90 / max, W max, each of the 13 presets' T against `rb-cpu.txt`.
    - Depends on: T10
12. **T12 — wave proof + commit C2** [run].
    - R-core (PD4) on the tree: full harness all PASS (FP2 included), `ids` for the 512 carried rows == `B.ids.txt`, new rows counted by name; the makeup-off run == `B.full.txt` except the rows whose stimulus T7 changed (CK1, CK4, CK7 and any `ckRender` user), which are listed.
    - Shipping targets (VST3, AU, Standalone) build with 0 warnings in `Source/`.
    - `git diff --stat C1..` (working tree) lists nothing outside the fence; `git diff -- Source/FactoryPresets.h` shows only `sN_level` values.
    - Commit C2.
    - Depends on: T11

### Wave 3 — build, install, validators

13. **T13 — build / install / validate / final regression** [run].
    - Logic and Live quit. `./scripts/build-and-install.sh O-GranulatorInstrument` (cache clear, dual-variant sweep). Installed binaries `cmp`-identical to the build; `auval -a | grep -i granulatorinstrument` shows one `-dev` line.
    - auval `aumu OuGI OuDv`: PASS with exactly the 4 LFO-rate warnings.
    - pluginval 10 VST3 + AU, sidechain on, then off (`OGI_GATE_SC_DEFAULT_OFF=ON`); shipping build restored and reinstalled.
    - X2: 0 hook symbols and 0 `stub_` strings in the shipping VST3 / AU / Standalone; harness control non-zero.
    - First run rewrote `~/Library/O-GranulatorInstrument/Presets/Factory` (new `.bank-hash` == the harness's `bankHash`; 64 `.json` + 2 sentinels; none carries `"output_gain"`).
    - R-ui once (no-change proof): overrides → `ui_frontend_check.js` → `ui_binding_probe.js` (+ `--nc`) → check-i18n → fr-lint → zh-lint → check-ui-labels → boot-all-uis `--strict-tips`; `tests/preset_name_fit.js`; the three v2 pairs `cmp` clean; `tests/windows-webview2-gate.sh` exit 0.
    - If no file under `Source/`, `tests/` or `CMakeLists.txt` changed since T12's build, T12 is the final-tree regression: say so and name the commit. Otherwise re-run R-core.
    - Depends on: T12

### Wave 4 — docs, sheet, summary

14. **T14 — docs and metadata** (PD16) [agent].
    - CHANGELOG (`grep -c '^## \['` == 1; the CI awk extract prints a non-empty body); ARCHITECTURE addendum; PLUGINS.md date.
    - Files: `CHANGELOG.md`, `.planning/research/ARCHITECTURE.md`, `../../PLUGINS.md`
    - Depends on: T13
15. **T15 — HANDS-ON-SESSION-4** (PD17) [agent].
    - Row IDs counted before and after; removed and added IDs listed. The trimmed presets and the centroid outliers come from T9 / T10, not from the research table.
    - Files: `.planning/stages/4-polish/HANDS-ON-SESSION-4.md`
    - Depends on: T14
16. **T16 — SUMMARY-R2, STATUS, commit C3** [agent + run].
    - `SUMMARY-R2.md`: what was built; gate table on the final tree; LH rows with measured values and control results; the PD3 moved-row list; the CK repair; the level tables and trims; CPU table; deviations; residual limits (hot-lane plateau, peaks above 0.7, Pre note-off dip, brightness walk); OS1–OS4 as taken; what verify must re-run and rewrite.
    - STATUS → `stage_4_rework_r2_execute_complete`; `architecture` checksum re-hashed; `requirements` and the other five unchanged (assert); next: `/plugin-verify O-GranulatorInstrument 4-polish`.
    - Commit C3. Then the two-step handoff.
    - Files: `.planning/stages/4-polish/SUMMARY-R2.md`, `.planning/STATUS.md`
    - Depends on: T15

---

## Dependency graph

```
T1 ── T2 ── T3 ── T4 ── T5 ── T6 ── T7 ── T8 ── T9 ── T10 ── T11 ── T12 ── T13 ── T14 ── T15 ── T16
     (C1)                                                         (C2)                       (C3)
```

Strictly serial: one build directory, and every proof compares with the step before it.

## Commit split

| Commit | Content | Tasks |
|---|---|---|
| C1 | Planning docs (CONTEXT-R2, RESEARCH-R2, `research-r2/`, PLAN-R2), `evidence-r2/` baseline, STATUS. No source. | T2 |
| C2 | Energy table, makeup, latch, hooks; `engine_gates_r2.h`, `--r2` compare, level-table mode, CK repairs; `sN_level` trims; evidence | T3–T12 |
| C3 | CHANGELOG, ARCHITECTURE addendum, PLUGINS.md date, HANDS-ON-SESSION-4, SUMMARY-R2, STATUS | T14–T16 |

W1 and W2 share one commit because FP2 is red between T4 and T10.

---

## Gate criteria (R2 execute)

| Gate | Pass |
|---|---|
| Renders | `renders-2.4`, legacy `renders-2.2`, `renders-2.1` pinned lines byte-identical at T3, T4, T5, T12 and on the final tree. **Zero re-pins.** |
| Neutrality | Makeup off: full extract == `B.full.txt` (T3–T5 exactly; T12 except the listed CK stimulus rows); PT0 64 / 64 against `$RB` |
| Harness | 512 carried rows: same IDs, all PASS; text moves only under PD3, each recorded. LH rows PASS; every `n` control FAILS its predicate; no INVALID; 0 NOT FIRED; HM1 green |
| Lower bound | LH1, LH1p, LH2, LH3 within −3 / +2 dB; `makeupOff` falls more than 20 dB on LH1 |
| Upper bound | FL1, FL1s, FL1p, FL1sp, FL2 / FL2p, CL1 pass; LH4 ≤ +3 dB; LH10 ≤ 1 % clamped |
| Note-off | LH8 within −3 dB; LH14 (a) 0 writes, (b) resumes |
| Invariance | LH9 hashes equal across block sizes and a repeat; LH13 at 44.1 and 96 kHz |
| Parity | 168 / 0 + GV 2 / 0; PB / PR residual lines equal to RB digit for digit |
| Preset renders | PT0r 51 / 51 bitwise; PT3 13 / 13 differ; PT3n fails; two final dumps equal |
| Presets | 64; FP0–FP6 PASS with controls; FP2 spread ≤ 9 dB; level outliers within 1.5 dB of RB; only `sN_level` values changed; no preset sets `output_gain` |
| Contract | param-dump == `.planning/params.tsv`; 293 / 150 / version 2 untouched; six of seven checksums unchanged, `architecture` re-pinned |
| Fork drift | 0 / 13 |
| CPU | C25w ≤ 40, C25t ≤ 12, C25m ≤ 40, C25tm ≤ 12; every preset T ≤ 12 %, W ≤ 40 %; C25mPre recorded |
| Build | VST3 + AU + Standalone + consoles; 0 warnings in `Source/`; VERSION 1.0.0 |
| Validators | auval PASS with exactly the 4 LFO-rate warnings; pluginval 10 VST3 + AU SUCCESS, sidechain on and off; X2 0 |
| UI (no-change proof) | R-ui chain green; three v2 pairs `cmp` clean |
| Release | one `## [1.0.0]` heading; PLUGINS.md `🚧 Stage 4 \| 1.0.0`; no tag; no push |
| Scope | Nothing outside the fence in C1..C3 |

Exit 77 anywhere = not run = not a pass.

## Risks / watch items

- **A NaN in the running means is permanent** until the head parks (correction 1; repo memory: `SmoothedValue::reset` does not clear a NaN). PD6 step 2, LH12.
- **Do not slew the gain downward.** The prototype's first form swung a quiet lane to +26 dB. LH4n.
- **The reference must come from the pristine.** Anything measured from the working copy drifts. LH6, LH11.
- **The writer clamp masks an overdriving makeup.** LH10.
- **Vacuous lower-bound rows.** LH1's control must fall more than 20 dB or the stimulus proves nothing (repo memory: stimulus below threshold).
- **Statement shape.** A multiply by 1 on the makeup-off path, or a reordered write, changes contraction and breaks neutrality. T4's makeup-off proofs catch it at the task that caused it.
- **The latch and a releasing voice** (pitfall 7). LH14.
- **CK repair changes shared stimulus code.** `ckRender` serves several rows; all are re-read at T7.
- **The shipped law is not bit-equal to the prototype** (coefficients formed once, a scrub added). The LH bands and T9's sanity check bound the difference; neither is a bitwise claim.
- **CL1's printed margin is thin** (about 0.0002). It cannot cross: the knee is an asymptote. Recorded, not acted on.
- **Presets were levelled against a fading lane.** Trims are done on the final DSP (T10 after T7); a DSP fix after T10 re-runs T9 and T10.
- **The sound changes more than the level** (research summary 10). Execute fixes level only; character is Taylor's call in Pass 2.
- **`$RB` lives under `build/`.** A clean rebuild that deletes `build/` loses the reference.
- **Limits that remain by design:** a dense lane hotter than about −16 dBFS RMS settles on a plateau (lower at low Amounts); peaks above 0.7 cannot be held. In the sheet (R2-4), not gated.

## Deviations protocol

Record every departure in SUMMARY-R2 (what, why, evidence, which gate). The CK stimulus change (PD10), an LH7 stimulus change and the retired `r1-b0` compare are planned and recorded as such.

Stop and ask only for:
- a render-hash move, or neutrality (makeup off) not bitwise;
- a carried row with feedback 0 moving, or a verdict change outside PD3's list;
- a fixed constant of PD6 or the shipped latch threshold changing;
- the PD10 ladder ending without all three controls firing (OS4);
- a preset needing more than an `sN_level` change, or FP2 not reachable by level;
- T9's table disagreeing with research beyond PD13's tolerance;
- a CPU limit exceeded;
- a path outside the fence, a forked-leaf edit, any parameter / destination / page / string change;
- a lost `$RB`;
- any push or tag.

## Carried (not R2)

- D54 Windows CI arm, pending `/publish` (the standing answer is hold).
- COMPAT-02 DAW pass (MPE, Dorico) → 1.0.x. Reaper. Windows hands-on.
- Everything in PLAN-R1 § Carried.

## What verify must re-run

On the final tree, without copying from SUMMARY-R2:
- R-core in full; `--egr2` and the full harness; the count of LH rows by name and each control's failure.
- The makeup-off full run against `B.full.txt`.
- `--cmp-preset-renders $RB --r2` while `$RB` exists; each `$RB` raw's sha against `rb-preset-renders.sha256`; a fresh dump against `r2-preset-renders.sha256`.
- `--preset-level-table $RB` against `r2-levels.txt`.
- `--cpu25`, `--cpu4` alone in the background.
- R-val; X2.
- The 7 contract hashes against STATUS (`architecture` moved in execute; `requirements` moves at verify itself).
- A code read: no transcendental call at the tick; the makeup-off path unchanged in shape; the latch recomputed every tick and cleared only by a gated voice; the table built in `buildTape` only; `git diff 4c89247c -- Source/dsp/forked` empty.

Verify then updates REQUIREMENTS.md (PD16 list) and runs the human retest on the edited sheet: the reset rows and R2-1 – R2-12.

## Success criteria (gate for verify)

- [ ] A lane under feedback holds its level for 60 s at Amount 100 / 50 / 25 %, Post and Pre, within −3 / +2 dB (LH1–LH3), and the same rows fail with the makeup off
- [ ] No overshoot, no lift of a silent or near-silent lane, the reference follows the loop region, the cap still lets a filtered-out lane fade (LH4–LH7)
- [ ] After note-off the lane is left within 3 dB; a releasing voice does not re-open the head; the pedal keeps it writing (LH8, LH14)
- [ ] Bound rows unchanged and passing; return clamping ≤ 1 % on the presets (LH10)
- [ ] Block-size invariant and deterministic; 44.1 and 96 kHz inside the same bands (LH9, LH13)
- [ ] Feedback 0, the toggle Off and Freeze bitwise; render goldens byte-identical; parity 168 / 0; makeup off == the R1 engine
- [ ] CK1, CK4, CK7n pass with controls that fire again, on the shipping path
- [ ] 51 presets bitwise; level outliers trimmed to within 1.5 dB with `sN_level` only; FP2 spread ≤ 9 dB; centroid outliers listed for Pass 2
- [ ] C25m ≤ 40 %, every preset T ≤ 12 % / W ≤ 40 %
- [ ] 293 parameters, 150 destinations, page, strings, `params.tsv`, forked leaves untouched
- [ ] Build, install, auval (4 LFO-rate warnings exactly), pluginval 10 sidechain on / off, X2
- [ ] CHANGELOG `## [1.0.0]` amended, ARCHITECTURE R2 addendum + checksum, PLUGINS.md date, HANDS-ON-SESSION-4 reworded with the retest rows reset and R2-1 – R2-12 added, SUMMARY-R2 written; VERSION 1.0.0, no tag, no push
