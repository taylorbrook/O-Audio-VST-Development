# Stage 2 — DSP · Phase 2.6 (Record-in capture, `<Slots>` state / resolve / relink / embed, factory sources, telemetry) — Verification

## Verification Date

2026-11-09

I re-ran every automated check below myself in the verify session, against `e4987f8a`. The plugin tree was clean beforehand.
- I touched `Engine.cpp` and `PluginProcessor.cpp` and rebuilt the harness and param-dump (rc 0, 0 warnings).
- I re-extracted and re-built the parity reference from the pin into a fresh work dir (`tests/parity/run.sh`, scratchpad, never in the repo).
- I reinstalled with `build-and-install.sh` and relinked the Standalone explicitly.
- Logic was open at the start of verify. The user quit it before `--cpu26` and pluginval, so both ran with Logic closed (execute ran them with Logic open). `--cpu26` ran alone.

No automated result here is copied from SUMMARY-2.6.md.

HANDS-ON-2.1 (13 rows), 2.2 (10), 2.3 (9), 2.4 (11), 2.5a (12), 2.5b (11) and 2.6 (13) are human gates. On 2026-11-08 the user deferred all of them until after Stage 3 (D14 override). They are **not** self-certified here.

There is no parent equivalent for 2.6 (O-Granulator has no Record-in). The proof is law gates, leak gates with hard controls, click gates, the block-size null, the allocation gate, the host-idle bound and true bypass. The parity arm is carried, not extended.

This is the last Stage 2 phase. The Stage 2 roll-up is at the end.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.6 / ROADMAP Phase 2.6)

1. **FUNC-07 capture.** Record the sidechain into any target slot by Manual / Threshold / Next Note. The captured slot plays polyphonically, and its PCM equals sc × gain over the expected span.
2. **D35 Threshold pre-roll.** Start 10 ms before the crossing, clipped at the arm sample. Manual and Next Note are sample-exact.
3. **D34 one-shot arm.** Arm still On after a take stays Idle. Off while Armed cancels. Off while Recording keeps ≥ 0.5 s and discards a shorter take.
4. **Length.** Bars under Tempo + playing, seconds otherwise; the cap holds at `F_max`.
5. **QUAL-02 engine path.** A silent sidechain is discarded with the slot unchanged. With no bus, arm is inert. The output never contains sidechain audio in any state, including host bypass.
6. **FUNC-06 state.** Save / reopen restores file, rec, factory and embedded sources, loop regions and the scale. A missing file gives `missing` + relink, keeps its reference on re-save, and the plugin stays stable.
7. **D36 embedding.** The blob decodes to the 24-bit-quantised pristine bit for bit, and save → load → save is byte-identical. A blob over 16 MB is omitted and the slot falls down the resolve order.
8. **Resolve order.** Every rung is exercised, including the Recordings hash search and a hash-mismatch relink.
9. **D33 factory.** `factory:` stubs load with no disk access.
10. **PERF-01 / PERF-03.** 0 allocations across commit, discard, swap, state save / load, factory and missing resolves, and an SR change mid-record. The host-idle bound holds. Capture spans + PCM are block-size invariant.
11. **QUAL-01.** Click gates on the commit swap and the state-load swap against held references, with a firing hard control.
12. **True bypass.** renders-2.4 + legacy byte-identical; harness 300 carried; parity 168 / 0 + GV 2 / 0; param-dump identical; drift 0 / 13.
13. **CPU regression.** C25m ≤ 40 %, C25tm ≤ 12 %.
14. **Validators.** auval PASS; pluginval 10 VST3 + AU with SC active (2 ch) and inactive; parent code stays out of our binaries; the real Recordings folder is untouched.

### Deliverables (SUMMARY-2.6.md)

- **C1 `a8669ebc`:** the bypass leak fix (`processBlockBypassed` clears everything and never calls the JUCE base); the per-piece SC scratch + output zero inside `Engine::process`; `GiRecorder.h` (Idle / Armed / Recording / Committing + reasons, one-shot arm, pre-roll ring, grow-only capture buffer); the Timer / watcher copy-out and the worker commit (hash → float32 WAV → tape → publish → FLAC-24 blob); the clear sentinel; `GiHash.h` (XXH64 + own 24-bit quantiser); the loader's resolve / relink / embed paths; the `<Slots>` codec; the `FactorySourceData` target + 2 generated stubs; `GiTelemetry.h`.
- **C2 `e4987f8a`:** `engine_gates_26a.h` (28) + `engine_gates_26b.h` (29), `--cpu26`, `--gen-factory`, the swapKillHard hook, `HANDS-ON-2.6.md` (13 rows, deferred).

Code read in verify:
- `PluginProcessor.cpp:335-355` `processBlockBypassed`: NE drop → `buffer.clear()` → `engine->bypassedBlock()`. The base call exists only under `OGI_TEST_HOOKS` (LK2n); X2 below confirms the hooks are absent from the shipping binaries.
- `PluginProcessor.cpp:258-332` `processBlock`: channels ≥ 2 cleared; `scL` / `scR` passed only for the negotiated sidechain channel count (`scCh`); the whole buffer is cleared when the engine is not called.
- `PluginProcessor.cpp:592-657` `setStateInformation`: `<Slots>` parsed before the strip; `restoreSlots` only writes records and queues jobs (no decode on the host thread).
- `GiRecorder.h:151-189` `prepare`: the capture buffer grows only under `captureLock` and only while no copy-out is pending; `captureLock` is never taken by the audio-thread path (`beginBlock` onward reads `ready` atomically).

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. Capture | ✅ Achieved | **RC1–RC3** Manual / Threshold / Next Note × slots 0–3 at +6 dB: tape == sc × g, 0 diffs over 48 000 × 2. **RC1n** startOffset fires. **RC4** 4 rates over 200 grains on the recorded slot; mono SC → 1-ch pristine. |
| 2. Pre-roll | ✅ Achieved | **TH1** start = crossing − 480; **TH1n** prerollOff fires. **TH2** crossing inside 10 ms → start == arm. **TH3** Manual +0, Next Note == note sample. |
| 3. One-shot | ✅ Achieved | **OS1** 1 commit after 2 s held On; **OS1n** armLevelTrigger → 2 arms. **OS2** new edge takes again. **OS3** Cancelled. **OS4** 0.50 s kept, 0.49 s TooShort. **OS5** held arm during Committing → Armed after the copy-out. |
| 4. Length | ✅ Achieved | **LN1–LN3** seconds / 2 bars @ 120 (tempo change ignored) / stopped → seconds. **LN4** cap holds; **LN4n** capOff overruns. **MN1** Mono retrigger counts, Legato move does not; **MN1n** fires. |
| 5. QUAL-02 engine | ✅ Achieved | **QD1–QD3** silent SC discarded (NoSignal, tape + hash unchanged); −89 commits, −91 discards; disabled bus → SidechainUnavailable, 0 arms. **LK1** 12 / 12 (3 layouts × 4 states) output == SC-silent render bitwise; **LK1n** scNoClear leaks. **LK2** bypassed → +0.0f on 3 layouts; **LK2n** baseBypass (pre-2.6) leaks. |
| 6. FUNC-06 state | ✅ Achieved (engine) | **SS1** file / rec / factory / embedded-only restore via rungs 1 / 1 / 4 / 2, all sounding; loop 10–80 and `edo 19` restored. **RL6** missing → sentinel clear, stable 2 s, reference re-saved identical. **RL7** relink match / mismatch flagged. **SS6 / SS7** identical-source no-op; empty-source clear. Relink *UI* = Stage 3.2. |
| 7. D36 embed | ✅ Achieved | **SS2** blob == deq (Q (pristine)), 0 / 96 000; **SS2n** float path fires. **SS3** save → load → save identical before / after resolve and a second hop (298 450 bytes); **SS3n** re-encode fires. **SS4** 15 s @ 192 k → tooLarge, `embed="1"` without `embedded`, restores via rung 1. **SS5** codec rejects malformed input; **SS5n** fires. |
| 8. Resolve order | ✅ Achieved | **RL1–RL5** path / size mismatch → blob / Recordings exact name / renamed found by size + hash / factory. **RL8** 1100 same-size files: 17 scanned, 16 hashed; **RL8n** unbounded fires. |
| 9. Factory | ✅ Achieved | **FA1** both stubs decode to the pinned hashes. **FA2** fsTouches 0; **FA2n** `file:` → 2. Checked-in FLACs match `factory-stubs.sha256`, and a fresh `--gen-factory` is **byte-identical** (tone 124 716 B `6d117c4cc9d31ec3`, noise 777 546 B `e6c6444b102af6b8`). |
| 10. RT / host idle / block size | ✅ Achieved | **AL26** 0 allocations (arms 5, starts 4, commits 3, discards 1, cancels 1, published 2, missing 1, factory 1, clears 1). **CM3** host idle mid-commit / mid-swap: pending ≤ 1, retiring drains; **CM3n** fires. **CM4** prepare mid-take 44.1 → 96 k. **BS26** 24 / 24 start / stop / PCM bitwise; **BS26n** fires. **HS1–HS3** hash vectors + 2²⁴ quantiser round trip. |
| 11. Clicks | ✅ Achieved (S6) | **CK6a** commit swap \|d2\| 1.202× (κ 3.58× INFO); **CK6b** state-load swap 1.202× (κ 3.83× INFO); **CK6n** swapKillHard fires. Judged on raw \|d2\| by user decision S6 (2026-11-09); the κ excess is the 2.1 swap-rule level dip → HANDS-ON-2.6 row 13. |
| 12. True bypass | ✅ Achieved | `--dump-render-hashes` **byte-identical** to `renders-2.4.sha256`; legacy **byte-identical** to `renders-2.2.sha256`, its 7 `-pinned` lines == `renders-2.1.sha256`. **TB7** 14 / 14 + legacy 14 / 14 with the SC bus present and the recorder idle. Parity, param-dump, drift: see below. |
| 13. CPU | ✅ Achieved | **C25m 27.85 %** ≤ 40; **C25tm 6.55 %** ≤ 12. See Automated Checks. |
| 14. Validators | ✅ Achieved | See Automated Checks. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.6 of 2.1–2.6). A requirement closes only when **all** of its acceptance criteria are covered.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-06: File state | must | ⚠️ Partial (was pending) | **Covered:** save / reopen restores 4 sources, loop regions and the scale (SS1); a missing file is flagged, stable, relinkable and keeps its reference (RL6, RL7).<br>**Open:** "shows relink" is the Stage 3.2 UI; DAW reopen = HANDS-ON-2.6 rows 7–8. |
| FUNC-07: Record-in | should | ⚠️ Partial (was pending) | **Covered:** all three triggers into all four slots, pre-roll, one-shot, length (RC / TH / OS / LN / MN rows).<br>**Open:** real-host sidechain routing (Logic AU, Ableton / Reaper VST3) and threshold feel = HANDS-ON-2.6 rows 1–6 (FUNC-05 precedent). |
| PERF-01: RT safety | must | ✅ Complete (was partial) | **Now covered:** file swap / Record-in / state paths (AL26), on top of note on / off, steal, reset, MIDI / MPE / NE (FA5, AL1, NE5). The alloc criterion is fully met. |
| PERF-03: Off-thread + reaper | must | ⚠️ Partial | **Now covered:** commit / state-load off the audio thread, host-idle bound mid-commit and mid-swap (CM3), capture block-size null (BS26).<br>**Open:** HANDS-ON-2.1. |
| QUAL-01: No artifacts | must | ⚠️ Partial | **Now covered:** file swap (commit + state-load, CK6a / b with CK6n firing; S6).<br>**Open:** HANDS-ON-2.1–2.6 listening (incl. CK5b / c / f bound-only and the CK6 swap dip, row 13). |
| QUAL-02: SC degrades cleanly | nice | ⚠️ Partial (was pending) | Engine path covered: QD1–QD3, LK1 12 / 12, LK2 + LK2n, pluginval with the SC inactive. Cross-host = HANDS-ON-2.6 rows 2–3 + Stage 4 (its verifiedAt). |
| FUNC-08: Factory set | should | ⏸️ Pending (stage-4) | Groundwork in: `factory:` path + 2 pinned stubs (FA1, FA2). The curated set + presets are Stage 4. |
| PERF-02: CPU budget | must | ⚠️ Partial | Regression held: C25m 27.85 % ≤ 40. Open: typical on factory presets (Stage 4). |
| DSP-01, DSP-04, DSP-05, DSP-06 | must / should | ✅ Complete (unchanged) | Parity 168 / 0 carried. |
| FUNC-01–05, FUNC-09, DSP-02, DSP-03, DSP-07 | must / should | ⚠️ Partial | Unchanged; their HANDS-ON rows are pending. |

**Requirements Summary (after this phase):**
- ✅ Complete: 6 (DSP-01, DSP-04, DSP-05, DSP-06, PERF-01; COMPAT-01 from Stage 1)
- ⚠️ Partial: 15 (FUNC-01–07, FUNC-09, DSP-02, DSP-03, DSP-07, PERF-02, PERF-03, QUAL-01, QUAL-02)
- ⏸️ Pending (later stage): 4 (UI-01–02 stage 3; FUNC-08, COMPAT-02 stage 4)
- ❌ Failed: 0

Total 25, matching REQUIREMENTS.md. (VERIFICATION-2.5b's "24" left QUAL-02 out of its pending list; corrected here.)

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness + param-dump) | ✅ Pass | rc 0 after the touch-rebuild; 0 warnings. |
| build-and-install.sh | ✅ Pass | rc 0, 0 warnings (shipping, SC-off variant, and the restore). Only `O-GranulatorInstrument-dev.{vst3,component}` on disk. |
| Standalone | ✅ Pass | Relinked (`OuariconGranulatorInstrument_Standalone`, rc 0); binary 22:41:23 newer than the harness 22:07:30. Alive after 6 s. |
| Render harness (default) | ✅ Pass | **357 PASS / 0 FAIL** (300 + eg26a 28 + eg26b 29). **159** `ctrl FIRED`, **0** `NOT FIRED`. Duplicate-ID set (B1 G1–G4 H1 H2 M1 R1 R1v S2) == the 2.5b set; the SS / RL / AL26 / BS26 renames added no duplicates. |
| Parity (`--parity`, fresh parent build) | ✅ Pass | `run.sh` rc 0, 0 self-check failures, 332 outputs. **168 / 0**; `--rows GV` **2 / 0**. |
| Render goldens | ✅ Pass | Shipping == `renders-2.4.sha256`; legacy == `renders-2.2.sha256`; legacy pinned == `renders-2.1.sha256` 7 / 7. |
| Factory stubs | ✅ Pass | `shasum -c factory-stubs.sha256` OK ×2; `--gen-factory` to scratch → `cmp`-identical to `Resources/factory/*.flac`. |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv`. |
| Fork drift | ✅ Pass | 0 / 13. Clean pin-archive parent root → 0. **NC-a** (byte on `Grain.h`, parent root) rc 1, 1 drift; **NC-b** (byte on `FeedbackChain.h`, local root) rc 1, 1 drift. |
| NE module untouched | ✅ Pass | `git diff --quiet 98836951 HEAD -- modules/tuning/note-expression scripts/juce-patches`. |
| X1 compdb exclusion | ✅ Pass | `ninja -t compdb`: 164 entries referencing O-GranulatorInstrument → **0** matches of `O-Granulator/Source\|ogi-parity\|82406fce`. |
| X2 symbol exclusion | ✅ Pass | `nm -C` finds **0** `ogran::` in the installed VST3, the installed AU and the harness. Control: `proj-parity` has 76. |
| auval `aumu OuGI OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`, `[1, 2] [2, 2]`, and again after the shipping reinstall. `ZZZZ` control rc 2. |
| pluginval 10 VST3, SC active | ✅ Pass | `SUCCESS`, rc 0, 2 ch in, 0 `!!!`. |
| pluginval 10 AU, SC active | ✅ Pass | `SUCCESS`, rc 0, 2 ch in. Only `!!!` is the informational "Current program is -1". |
| pluginval 10 VST3, SC inactive (`OGI_GATE_SC_DEFAULT_OFF=ON`) | ✅ Pass | `SUCCESS`, rc 0, **0 ch in**, 0 `!!!`. |
| pluginval 10 AU, SC inactive | ✅ Pass | `SUCCESS`, rc 0, same single `!!!`. Reports 2 ch in: the AU wrapper negotiates the input bus itself, so the AU arm never sees 0 ch (INFO; the AU unrouted case is QD1 / HANDS-ON-2.6 row 2). Cache restored to `OFF`, shipping build reinstalled. |
| `--cpu26` (run alone, Logic closed) | ✅ Pass | **2 PASS / 0 FAIL**, rc 0. Witness `ref_ns` 40 523 708; calib 0.927–0.976 × ref, after/before ≤ 1.031, duty ≥ 0.949.<br>• **C25m 27.85 %** (26.84 … 28.52) ≤ 40, live 243.2, PV dests 16 / tick, recomputes 100 %, pair-mode 4 — margin 12.15 points.<br>• **C25tm 6.55 %** ≤ 12, live 80.2.<br>• C25w 26.38 %, C25t 5.66 %; Δ C25m − C25w +1.47.<br>• **C26rec 27.91 % INFO** (C25m with a Manual take in flight). |
| Recordings guard (GD1 + manual) | ✅ Pass | `~/Music/Ouaricon/O-GranulatorInstrument/Recordings` absent before the harness and after every validator. |
| Git | ✅ Pass | No uncommitted plugin changes before verify. No `O-GranulatorInstrument*` tag. |

## Human Verification

All pending; deferred by the user on 2026-11-08 until the Stage 3 UI is in place, run together then:
- [ ] HANDS-ON-2.1 rows 1–13
- [ ] HANDS-ON-2.2 rows 1–10
- [ ] HANDS-ON-2.3 rows 1–9
- [ ] HANDS-ON-2.4 rows 1–11
- [ ] HANDS-ON-2.5a rows 1–12
- [ ] HANDS-ON-2.5b rows 1–11
- [ ] HANDS-ON-2.6 rows 1–13. Key calls: Logic AU capture routed / unrouted (rows 1–2), VST3 sidechain + bypass bleed (row 3), threshold feel (row 4), reopen / relink (rows 7–8), save time with 4 large blobs (row 11), the swap dip (row 13).

## Issues Found

- **None blocking** in the automated scope.
- **CK6 judged on |d2|, not κ (S6, user decision).** Local κ reads 3.58× / 3.83× because the 2.1 swap rule leaves a short level dip that shrinks κ's RMS denominator; raw curvature is 1.20× the held references and the hard control fires at 57×. The swap rule is unchanged. HANDS-ON-2.6 row 13 decides whether the dip is audible.
- **CPU spread.** C25m 27.85 % here vs 29.26 % in execute (Logic open then) and 27.62 % at 2.5b verify. Witnesses valid on all runs. Recording adds no measurable cost (C26rec 27.91 %).
- **AU SC-inactive variant still reports 2 input channels** (INFO). Not a regression; the gate variant only removes the default bus where the format lets it (VST3 shows 0 ch). The AU unrouted path is covered by QD1 (silence → NoSignal) and LK1.
- **Plan deviations reviewed and accepted** (SUMMARY-2.6 §Deviations 1–13): S6 CK6 metric (user), commit / checkpoint grouping, T2 ordering, row renames (no new duplicate IDs — confirmed), the HS1 XXH64 vector correction (reference-library value), SS3 excluding the pre-existing `<CustomScale version>` counter, empty-source restore clearing only sourced slots, N2 / N2s / FN3 forcing rec_arm On, stimulus fixes, test-only hooks (X2 = 0 confirms they are absent from shipping binaries), the dual SC peak telemetry (Stage 3 picks one), and the "Done / Discarded" → Idle + reason mapping (reconcile in Stage 3). No threshold was loosened and nothing was re-pinned.
- **Carried, by design:** relink / load / record / embed UI and the lanes display (Stage 3.2); the curated factory set + presets (FUNC-08, Stage 4); cross-host sidechain (QUAL-02, Stage 4); `DevLoaderEditor` removed in 3.1.
- Foreign working-tree changes (`modules/registry.yaml`, `.claude/agent-memory/*`, `.claude/compaction-snapshot.md`, `.planning/STATE.md`, O-GrainScatter) are not touched.

## Phase Verdict

**Status:** ⚠️ PARTIAL. Every automated gate is independently verified green (harness 357 / 0, 159 controls fire, 0 NOT FIRED; parity 168 / 0 + GV 2 / 0; true bypass byte-identical; C25m 27.85 % ≤ 40 %; pluginval 10 VST3 + AU with SC on and off). PERF-01 moves partial → complete. FUNC-06, FUNC-07 and QUAL-02 move pending → partial. HANDS-ON-2.1–2.6 (human) are pending, deferred by the user until after Stage 3.

## Stage 2 Roll-up (Phases 2.1–2.6)

All six phases are verified ⚠️ PARTIAL with every automated gate green and the human sheets deferred (D14 override). No requirement has failed. Stage 2's automated scope is complete:
- Parity with O-Granulator: DSP-01 / DSP-04 / DSP-05 complete; 168 / 0 rows.
- Mod matrix incl. MPE / NE: DSP-06 complete.
- RT safety: PERF-01 complete.
- CPU: worst case 27.85 % of the 40 % budget.

**Ready for next stage:** Yes, under the user's D14 override. Stage 3 (GUI) builds the UI the deferred hands-on sheets need. Any ❌ in HANDS-ON-2.1–2.6 after Stage 3 goes back to `/plugin-execute O-GranulatorInstrument 2-dsp` for the owning phase.

**Blockers:** none for Stage 3. Open human gates: HANDS-ON-2.1, 2.2, 2.3, 2.4, 2.5a, 2.5b, 2.6 (deferred).
