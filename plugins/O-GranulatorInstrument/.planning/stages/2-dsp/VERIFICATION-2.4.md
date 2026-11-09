# Stage 2 — DSP · Phase 2.4 (Working-copy feedback, Freeze, Reset, reverb + knee) — Verification

## Verification Date

2026-11-08

I re-ran every automated check below myself in the verify session, against `3d68135b`. The plugin tree was clean beforehand.
- I touched `Engine.cpp` and `PluginProcessor.cpp` and rebuilt the harness and param-dump (rc 0, 0 warnings).
- I re-extracted and re-built the parity reference from the pin into a fresh work dir (`tests/parity/run.sh`, scratchpad, never in the repo).
- I reinstalled with `build-and-install.sh` and relinked the Standalone explicitly.
- `--cpu25` ran alone, after pluginval finished.

No automated result here is copied from SUMMARY-2.4.md.

HANDS-ON-2.1 (13 rows), 2.2 (10), 2.3 (9) and 2.4 (11) are human gates. On 2026-11-08 the user deferred all of them until after Stage 3 (D14 override). They are **not** self-certified here.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.4 / ROADMAP Phase 2.4)

1. **D22 leaf nulls.** `FeedbackChain` and `ReverbUnit` + knee null vs the parent @ `82406fce` at ≤ −90 dB, gain band held, every control family fails the null.
2. **D22 law arm (DSP-04).** 60 s fb-100 % stress at 16 voices: tape peak ≤ max(pristine, 0.7), all finite. Runaway guard (1 vs 16 voices, sum-tap control). Freeze / Reset / fb-0 hashes. Block-size null with feedback + freeze.
3. **D22 head clamp.** 0 forced crossings when feasible, no reads in the unsettled mip margin, forced-crossing control reports.
4. **D23 ceiling.** Knee output ≤ +6 dBFS before `output_gain` on the stress render and the carried FZ1 stimulus (44.1 / 48 / 96 k).
5. **D24 click gates.** Freeze, feedback step, head stop / start, Reset swap vs same-motion controls.
6. **DSP-05.** Reverb audible; tails decay on mix → 0; cold re-enable clean.
7. **RT safety.** 0 allocations across Reset / Freeze / fb toggles / SR take / steals; host-idle bounded memory; SR rebuild with parked heads.
8. **True bypass.** fb 0 + reverb 0 is byte-identical; the knee adoption is attributed (legacy == renders-2.2); param-dump identical; fork drift 0.
9. **D25.** Final PERF-02 worst case with 4 chains + reverb ≤ 40 %; typical proxy ≤ 12 %.
10. **Validators.** auval and pluginval 10 (VST3 + AU) pass, and parent code stays out of our binaries.

### Deliverables (SUMMARY-2.4.md)

- **C1 `3d71bebe`:** `Engine::mixPiece` uses the parent's `wetKnee`; `legacyWetTanh` restores the old statements; `renders-2.4.sha256` pinned.
- **C2 `3d68135b`:**
  - `Engine.{h,cpp}`: mean tap with per-(k,s) `g_fb`, 4 VERBATIM `FeedbackChain`s at unity knob, reverb → R96 wet guard → knee → `output_gain`, Freeze, Reset swap rule, SR park.
  - New `GiRecordHead.h` (whole-cell write at the tick, writer clamp ±0.7 + convex envelope, NaN scrub, 20 ms fb / run ramps, settle at wrap and park) and `GiHeadClamp.h` (g_a 40, modular coordinates, telemetry).
  - `MipTape::settleRun`, `SlotLoader::requestReset / resetSync`, processor `requestReset`.
  - Gates: `engine_gates_24.h`, `engine_gates_24b.h`, `parity_gates_24.h`, `--cpu25`.
  - `HANDS-ON-2.4.md` (11 rows, deferred).

Code read in verify: `GiRecordHead.h:175-199` applies `λ = kFbKnob·fb·run`, clamps the return to ±kAgcCeil (counted), bounds the write to [min(wc, rw), max(wc, rw)] and scrubs a non-finite write back to `wc`. This is the structural basis of the FL1 / FL1s bound.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. Leaf nulls | ✅ Achieved | Fresh parent build: `run.sh` rc 0, 0 self-check failures, 247 outputs. `--parity` **133 PASS / 0 FAIL**; `--rows GV` **2 / 0**.<br>• **PB0** tap −152.0 dB, g_PB 1.0000000.<br>• **PB1–12** −116.3 (PB12) … −141.8 dB; \|g/g_PB − 1\| ≤ 1.0e-8; PB9 env max 0.766 (AGC engaged); landed fb raws identical.<br>• **PBd:** ours vs parent lag 0; written layer vs return lag 32.<br>• **PBc1–5** fire: −41.3 dB; −35.7 dB with band −0.757; −36.2; −28.4; −1.8 dB (lag 31).<br>• **PR1–12, PRk** −96.8 (PR12) … −112.0 dB, band ≤ 1e-4 held; PRk parent peak 0.632 (knee engaged).<br>• **PRc1** −0.0 dB (size 51), **PRc2** −47.3 dB / band +5e-3 (legacyWetTanh) fire.<br>• Carried: PN01–26 −143.0 … −151.3 dB; PF0 −149.3 dB; PF rows worst −126.1 dB; 30 parity controls FIRED, 0 NOT FIRED. |
| 2. Law arm | ✅ Achieved | **FL1:** 0 bound violations (written max 0.989 / 0.969 / … vs bound 1.000), all finite. **FL1s:** 0 violations vs 0.7f exactly; control rClampOff + injectReturn → 0.9025. **FL2:** tap RMS 1 voice −80.0 dB, 16 voices −104.3 dB (−24.3); sumTap control fires. **FL3:** freeze hashes equal over 10 s, writes 963 712 → 963 712. **FL4:** 4/4 == pristine after 60 s, 0 writes; fb 0.1 % control → 4/4 differ. **FL5:** 441 / 512 / 4096 / var → 0 differing samples, tapes equal. **FL6:** scrub 1, tapes finite, s1/s3/s4 unaffected. **FL7:** 99 unread params, 162/162 bound. **FL8:** worst tap 0.4944 ≤ 0.5. **MS1:** 0 mismatching samples vs an offline mip build (44 settles, 4 parks). |
| 3. Head clamp | ✅ Achieved | **HC1:** 0 forced crossings, 0 scan violations over 4 014 976 segments; headClampOff control fires. **HC2:** 5088 forced crossings counted on the 30 ms region; the 2 s region gives 0. **HC3:** 1.71 M boundary calls, feasible at W + 1, forced at W. |
| 4. Ceiling | ✅ Achieved | **CL1:** knee peak 1.96515 ≤ 1.99526 (pre-knee 3.928); wetSafetyOff control → 3.928. **CL2** (FZ1): 1.6464 / 1.8154 / 1.8326 at 44.1 / 48 / 96 k (pre 2.01 / 2.55 / 2.63). **TB2:** identity ≤ kKneeT bitwise on N1 / P2. |
| 5. Click gates | ⚠️ Partial (by user decision) | Local κ (5 ms frames, user decision S6). **CK1** freeze 0.609× (hard stop 20.5×). **CK4** Reset swap 0.378× (hard swap fires). **CK2** feedback step 0.523× and **CK3** head stop / start 2.458× are **INFO**: their hard controls cannot fire by construction (CK2 hard 0.757×; CK3 hard == subject). User moved both to HANDS-ON-2.4 rows 3–4 (2026-11-08). |
| 6. Reverb | ✅ Achieved | **RV1:** wet − dry −24.8 dBFS, RT60 0.224 s. **RV2:** mix 50 → 0 κ 0.955×, residual −400 dBFS after 60 ms, then bitwise the mix-0 render. **RV3:** cold re-enable κ 1.139×; +0.1 step control → 18.0×. |
| 7. RT safety | ✅ Achieved | **FA4:** 0 allocations / 333 blocks (taken 10, xfades 6, parks 17, settles 38, steals 20). **RS1 / RS2:** Reset == pristine (all while frozen, still frozen 2 s later; s1 while parked, s2–s4 unchanged). **RS3:** 100 requests with no processBlock → pending max 1, 204 published / 204 freed, 0 deferred. **RS4:** B queues behind A (A + 992 smp ≥ 960). **SR1:** 0 writes before the land, 0.000000 cent rate error, resume at region lo. |
| 8. Bypass / drift | ✅ Achieved | `--dump-render-hashes` is **byte-identical** to `renders-2.4.sha256`. `--dump-render-hashes-legacy` is **byte-identical** to `renders-2.2.sha256`, and its 7 `-pinned` lines equal `renders-2.1.sha256`. renders-2.2 → 2.4 moved 12/14 (CH, E1, H1, N1, P2, Q4 × default/pinned; V3 unmoved, it hashes the grain log). **TB0** 14/14, **TB1** shipping 14/14 + legacy 14/14 with every 2.4 control moved. param-dump byte-identical (`cmp`). Drift **0 / 13**; clean pin-archive parent root rc 0; **NC-a** (byte on `Grain.h`, parent root) rc 1; **NC-b** (byte on `FeedbackChain.h`, local root) rc 1. |
| 9. D25 CPU | ✅ Achieved | **C25w 26.59 %** (26.03 … 27.18) ≤ 40, live 243.2, duty ≥ 0.997. **C25t 5.51 %** ≤ 12. See Automated Checks. |
| 10. Validators | ✅ Achieved | See Automated Checks. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.4 of 6). A requirement closes only when **all** of its acceptance criteria are covered, including human rows where the criterion asks for them.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-04: Feedback | must | ✅ Complete (was pending) | "Feedback 100 % with 16 voices never exceeds the limiter ceiling or produces NaN": FL1 + CL1 (+ CL2 on FZ1), FL6. "Reset restores the pristine file; Freeze halts writes": RS1 / RS2, FL3. Runaway guard (description): FL2. Leaf parity: PB. Both criteria are engine-level and automated; HANDS-ON-2.4 rows 1–6 are listening confirmations, not criteria. |
| DSP-05: Reverb | should | ✅ Complete (was pending) | "Inherited": PR1–12 / PRk nulls vs the parent ≤ −96.8 dB with controls. Audible + tails (CONTEXT): RV1–RV3. HANDS-ON-2.4 row 7 is a listening confirmation. |
| PERF-02: CPU budget | must | ⚠️ Partial | **Now covered:** the final worst case, C25w 26.59 % ≤ 40 % with chains + reverb.<br>**Open:** matrix cost (2.5) re-checked against 40 %; typical verdict on factory presets (Stage 4, D20). |
| PERF-01: RT safety | must | ⚠️ Partial | **Now covered:** FA4 (Reset, Freeze, fb toggles, SR take, steals with feedback).<br>**Open:** matrix (2.5), file swap / Record-in (2.6). |
| PERF-03: Off-thread + reaper | must | ⚠️ Partial | **Now covered:** RS3 host-idle bounded Reset memory, RS4 queueing, SR1.<br>**Open:** 2.6 file swap / Record-in; HANDS-ON-2.1 rows. |
| QUAL-01: No artifacts | must | ⚠️ Partial | **Now covered:** reset + freeze toggle (CK1, CK4), reverb transitions (RV2 / RV3).<br>**Open:** CK2 / CK3 (HANDS-ON-2.4 rows 3–4), file swap (2.6), the HANDS-ON-2.1–2.3 rows. |
| DSP-01 | must | ✅ Complete (unchanged) | PN residuals −143.0 … −151.3 dB at HEAD (improved by the knee adoption). |
| FUNC-01–04, FUNC-09, DSP-02, DSP-03, DSP-07 | must | ⚠️ Partial | Unchanged; their HANDS-ON rows are pending. |

Still pending (later phases): FUNC-05, FUNC-06, FUNC-07, DSP-06.

**Requirements Summary (after this phase):**
- ✅ Complete: 3 (DSP-01, DSP-04, DSP-05)
- ⚠️ Partial: 12 (FUNC-01–04, FUNC-09, DSP-02, DSP-03, DSP-07, PERF-01–03, QUAL-01)
- ⏸️ Deferred (later phase or stage): 13
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness + param-dump) | ✅ Pass | rc 0 after the touch-rebuild; 0 warnings. |
| build-and-install.sh | ✅ Pass | rc 0, 0 warnings. Only `O-GranulatorInstrument-dev.{vst3,component}` on disk. |
| Standalone | ✅ Pass | Relinked (`OuariconGranulatorInstrument_Standalone`, rc 0); binary 21:30 newer than the harness 21:27. Alive after 6 s. |
| Render harness (default) | ✅ Pass | **179 PASS / 0 FAIL**, rc 0. **99** `ctrl FIRED`, **0** `NOT FIRED`. CK2 / CK3 print as INFO. Duplicate-ID set (B1 G1–G4 H1 H2 M1 R1 R1v S2) is the carried main.cpp / eg21 overlap; no new 2.4 ID collides. |
| Parity (`--parity`, fresh parent build) | ✅ Pass | `run.sh` rc 0, 0 self-check failures. **133 / 0**; `--rows GV` **2 / 0** (GV0 +0.0202 dB INFO, `kVoiceGainMeasuredDb` not re-measured, as SUMMARY says). |
| Render goldens | ✅ Pass | Shipping == `renders-2.4.sha256`; legacy == `renders-2.2.sha256`; legacy pinned == `renders-2.1.sha256` 7/7. |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv`. |
| Fork drift | ✅ Pass | 0 / 13, rc 0. NC-a / NC-b re-made independently → rc 1 each. |
| X1 compdb exclusion | ✅ Pass | `ninja -t compdb`: 151 O-GranulatorInstrument entries → **0** matches of `O-Granulator/Source\|ogi-parity\|82406fce`. Control: the parity compdb has 92 `O-Granulator/Source` refs. |
| X2 symbol exclusion | ✅ Pass | `nm -C` finds **0** `ogran::` in the installed VST3, the installed AU and the harness. Control: `proj-parity` has 76. |
| auval `aumu OuGI OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`, `[1, 2] [2, 2]`. `ZZZZ` control rc 2. |
| pluginval 10 VST3 (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0, `Main bus num input channels: 2`, 0 `!!!` lines. |
| pluginval 10 AU (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0, 2 ch in. Only `!!!` is the informational "Current program is -1". |
| `--cpu25` (run alone) | ✅ Pass | **2 PASS / 0 FAIL**, rc 0. Witness `ref_ns` 40 523 708; calib-before 0.939–0.950 × ref, after/before ≤ 1.017, duty ≥ 0.977.<br>• Bypass 0.55 %.<br>• C23w@2.4 (fb 0, reverb 0) 25.13 %.<br>• **C25w 26.59 %** ≤ 40 (margin 13.4 points).<br>• **C25t 5.51 %** ≤ 12.<br>• C25r (Reset every 200 ms) 27.36 %, 374 fades (INFO).<br>• Δ chains + reverb **+1.46 points** (projection +2–4).<br>• C25s **+0.27 points** (SUMMARY's +1.03 FLAG did not reproduce; below the +0.6 flag here).<br>• C25w96 54.16 % (INFO).<br>Agrees with SUMMARY (26.92 / 5.54) within 0.33 points. |
| Git | ✅ Pass | No uncommitted plugin changes before verify. No `O-GranulatorInstrument*` tag. |

## Human Verification

All pending; deferred by the user on 2026-11-08 until the Stage 3 UI is in place, run together then:
- [ ] HANDS-ON-2.1 rows 1–13
- [ ] HANDS-ON-2.2 rows 1–10 (incl. row 6, voice-gain ear trim)
- [ ] HANDS-ON-2.3 rows 1–9 (R22 crush threshold row 4, R19 resonance vs wet safety row 6)
- [ ] HANDS-ON-2.4 rows 1–11. Rows 3–4 carry the CK2 / CK3 click judgements (user decision S6). Row 5 (Reset) needs the Stage 3 Reset button. Listening notes: saturation character (row 1), chord `1/sqrt(Σa)` fallback (row 6), knee limiting character (row 8).

## Issues Found

- **None blocking** in the automated scope.
- **CK2 / CK3 not automated (accepted).** The local-κ hard controls cannot separate from the subject for a feedback step (r ≈ wc) or a head restart (parks only when no voice reads the point). Covered by HANDS-ON-2.4 rows 3–4 per the user's S6 decision; not self-certified.
- **C25s FLAG did not reproduce.** SUMMARY recorded +1.03 points (FLAG > +0.6) for the fb-0 restructure cost; this session measured +0.27. INFO either way; no lever needed (13.4 points of margin).
- **FZ1 ceiling carried and closed.** The 2.3 stress (pre-knee 1.53–1.73 then; 2.01–2.63 now with the full stimulus) is held ≤ +6 dBFS by the knee (CL2).
- **GV0 drift (INFO).** After the knee, GV0 reads +0.0202 dB; `kVoiceGainMeasuredDb` stays 0.02. Within the ±0.5 dB D16 tolerance.
- **CPU-run environment.** Logic was closed; only its QuickLook / Thumbnail extensions were resident (idle). Duty ≥ 0.977 and the calib brackets confirm an uncontended run.
- **Plan deviations reviewed and accepted** (SUMMARY-2.4 §Deviations 1–6): S6 local κ + CK2/CK3 INFO (user), PB enable-at-T_e + pre-roll (diagnosed row construction, nothing tuned), TB2 `≤`, HC3 absolute coordinates, FZ1 control retarget to the wet guard, settle span 2048 clipped to the flushed span (MS1 bitwise), renders moved 12/14 (V3 log hash), shortened control renders. Each keeps a firing control; no threshold was loosened. X2 = 0 confirms test hooks are absent from shipping binaries.
- **Carried, by design:** modulation of feedback / reverb / output destinations and the matrix CPU re-check (2.5); Record-in, `<Slots>`, factory sources, file swap (2.6); Reset / Freeze UI and record-head visuals (Stage 3); typical CPU on factory presets (Stage 4); `DevLoaderEditor` removed in 3.1.
- Foreign working-tree changes (`modules/registry.yaml`, `.claude/agent-memory/*`, `.claude/compaction-snapshot.md`, O-GrainScatter) are not touched.

## Phase Verdict

**Status:** ⚠️ PARTIAL. Every automated gate is independently verified green, including the final PERF-02 worst case (C25w 26.59 % ≤ 40 %). DSP-04 and DSP-05 close. HANDS-ON-2.1–2.4 (human) are pending, deferred by the user until after Stage 3; CK2 / CK3 live there.

**Ready for next phase:** Yes, under the user's D14 override. Phase 2.5 adds modulation, voice modes, MPE + note expression and tempo; the matrix cost must be re-checked against the 40 % budget (13.4 points of headroom today). Any ❌ in the hands-on sheets after Stage 3 goes back to `/plugin-execute O-GranulatorInstrument 2-dsp` for the owning phase.

**Blockers:** none for 2.5. Open human gates: HANDS-ON-2.1, 2.2, 2.3, 2.4 (deferred).
