# Stage 4 — Polish / Validation — Execute Summary, Rework R1 (per-stream feedback + reverb)

**Date:** 2026-11-12 · **Plan:** `PLAN-R1.md` (32 tasks, PD1–PD36) · **Commits:** C0 `a7ad2199`, C1 `9b00a1d4`, C2 `c50944d5`, C3 `db26325f`, T11 `1fb0e668`, C4 `082045f2`, D65 record `86689980`, C5 `1c5e7d70`, C6 `4c89247c`, C7 (this commit). VERSION 1.0.0. No tag. No push. No module edit, no `scripts/` edit, no `CMakeLists.txt` edit. One forked leaf edited (`forked/ControlGrid.h`, ADAPTED, OS1 approved).

D65 approval (Taylor, 2026-11-12): Variant B; row density and destination names as shown; OS1 yes; OS2 and OS3 defaults; the feedback switch dims with a disabled stream.

## What was built

| Area | Result |
|---|---|
| F1 | "Select a sidechain input" copy (en / fr / zh-Hans), committed on its own (C0), built and installed. |
| Per-stream feedback (D60, D63) | Each stream has Amount, Low Cut, High Cut, Saturate, Diffuse and an on / off switch. Off is a per-stream freeze of the write head (20 ms ramp, no writes, Amount kept; a routed Amount cannot lift it). The 5 global feedback parameters are gone. |
| Per-stream reverb (D60, D62) | Each stream has Mix, Size, Damp and a Pre / Post switch. Post: one audible reverb on the stream's term before the sum. Pre: a second reverb inside that stream's feedback loop, so the tail is written back. 8 `reverb-unit` 1.2.0 instances, module unedited. The 3 global reverb parameters are gone. Freeze and Reset All stay global. |
| Contract (D67, D68) | 293 parameters (200 Float / 74 Choice / 19 Bool). 150 destinations (stream stride 36, LFO rates 146–149), table version 2. The 32 new destinations are class G (one value per stream; a per-voice source follows the newest voice). Spec body rewritten once under header addendum C; extractor and goldens regenerated from the spec. |
| Page (D61, D65) | FX · Feedback page v2, Variant B, 1200 × 800: five feedback knobs in one row under a heading with the switch, a reverb row with a vertical Pre / Post segment, 13 figures and `#write-ro` removed. `index.html`, `js/i18n.js`, `tests/i18n-states.json` are `cmp`-equal to the three `mockups/v2-*` files. |
| Bank (D64) | 64 presets copied to the new parameters through `s*_` wildcard keys (`expandSpecValues()`); 10 showcase presets re-voiced with per-stream contrasts (PD28, no swap). |
| Record-in Arm (D66) | Arm returns to Off when a take ends or cannot start, written from the 20 ms message-thread poll timer (`armGen` / `endGen`). Waiting arms are left alone. State functions untouched (OS2 default). |
| Docs | CHANGELOG (one `## [1.0.0] - 2026-11-12` heading), ARCHITECTURE and ROADMAP addenda, CONTEXT-2.6 D34 note, PLUGINS.md date, HANDS-ON-SESSION-4 re-authored (227 → 243 run rows, 16 new `R1-` rows, no carried ID removed). |

## Gate results (final tree)

No file under `Source/`, `tests/` or `CMakeLists.txt` changed after C6. The harness, parity, preset and JS gates below were re-run on that tree at T28; the validators ran at T27 on the same sources.

| Gate | Result |
|---|---|
| Renders | `renders-2.4`, legacy `renders-2.2`, 7 pinned `renders-2.1` lines byte-identical at T7, T8, T9, T10, T14, T20, T24, T23 and T28. Zero re-pins. Both modes print `ctrl FIRED`. |
| Harness | **512 PASS / 0 FAIL**, 0 NOT FIRED, 0 INVALID. 454 carried rows all present and PASS; 58 new (14 AC rows, 44 DSP rows and controls). |
| Parity | 168 / 0 (GV row PASS); PB / PR residual lines equal to B0 digit for digit. |
| Preset renders vs B0 | PT1 38 / 38 bitwise; PT2 16 / 16 (worst null −132.57 dB, worst \|g − 1\| 4.66e-09, differ > 0); PT2n (Low Orbit, −9.75 dB) and PT2n2 (−6.42 dB) fire; the 10 showcase names printed as excluded. Before the showcase: PT2 26 / 26 at −131.18 dB. |
| Contract | Extractor `OK: 293 params (200 Float / 74 Choice / 19 Bool), 150 dests, 18 sources`; controls exit 1 and write nothing; `mod-sources.tsv` byte-identical. |
| param-dump | == the T21 pin. |
| Fork drift | 1 / 13 before the manifest update at T17, 0 / 13 after and at every later gate. Only `ControlGrid.h` and `FORK-MANIFEST.md` moved under `forked/`. |
| Presets | 64; FP0–FP6 PASS with controls; FP2 spread 6.32 dB (unchanged); no preset sets `output_gain`. |
| CPU (alone) | C25w 27.84 %, C25t 5.70 %, C25m 29.49 %, C25tm 6.70 %, C26rec 29.51 %, **C25mPre 30.93 %** (INFO, Pre ×4). `--cpu4` 64 / 64: T median 4.34 %, p90 6.60 %, max 9.19 % (Fifth Bloom); W max 10.29 % (Reverse River). |
| `rec_arm` | AC1–AC9 PASS with controls; no hunk in `getStateInformation` / `setStateInformation`. |
| Home hygiene | HM1; real `~/Library/O-GranulatorInstrument` and `~/Music/Ouaricon` unchanged by the harness and the param-dump console. |
| Build | VST3 + AU + Standalone + consoles, 0 warnings in `Source/`. |
| Install | `build-and-install.sh` rc 0; installed VST3 / AU identical to the build; one `-dev` AU registered. First instantiation rewrote the Factory folder (new `.bank-hash`; per-stream keys present; no bare `reverb_mix` / `feedback` / `output_gain` key in any file). |
| auval | PASS; exactly the 4 LFO-rate warnings (AU1 agrees), on the gate build and the restored build. |
| pluginval 10 | VST3 + AU SUCCESS with the sidechain on, and with `OGI_GATE_SC_DEFAULT_OFF=ON`; no `rec_arm` line. One seed each at T27 (three seeds each at T14). Cache variable back to OFF, shipping build restored and reinstalled. |
| X2 | 0 hook symbols and 0 `stub_` strings in the shipping VST3 / AU / Standalone; harness control 8 symbols / 9 strings. |
| Windows gate | `tests/windows-webview2-gate.sh` exit 0; 6 controls exit 1. |
| UI | frontend ALL PASS (`--nc` 4 / 4); probe 293 × 2, value entry 15 / 15, DM2 161 / 161 and 149 / 149, FD1, VE15, AC9, ST1 on 48 states, 8 controls live; check-i18n · fr · zh 0 · 0 · 0; check-ui-labels `param-dump (293 rows) + overrides`, default + 48; boot-all-uis 0 DEAD / 0 late; PN1. |
| `cmp` | three v2 pairs clean; `MOCKUP-ONLY` count 0. |
| Scope | `git diff --stat a7ad2199..HEAD` under this plugin lists only scope-fence paths. (Other plugins' commits by other sessions sit in the same range; none is R1's.) |

### New harness rows (58)

AC rows (T13): AC1, AC1n, AC2, AC2n, AC3, AC3n, AC4, AC5, AC5n, AC6, AC6n, AC7, AC7n, AC8; AC9 in the probe. Detail in `evidence-r1/w3-notes.txt`.

DSP rows (T24), each with its `n` control fired:

| Row | Measured | Control |
|---|---|---|
| FL1p | 0 bound violations, 0 non-finite / subnormal; chain-input peak 4.00 (stimulus live) | scanners 3 / 3 fail |
| FL1sp | written 0.7000 exactly; rClamps 1 694 441 | `rClampOff`: 0.7921 |
| FL2p | 16 voices vs 1: −19.5 / −29.5 dB | `sumTap`: +61.5 / +54.3 dB |
| FL3p | hashes equal after settle; writes frozen | edge hash differs |
| FL4p / FL4t / FL4m | 4 / 4 == pristine, 0 writes (feedback 0; switch Off in Post and Pre; switch Off under a routed Amount) | each differs / writes |
| FL5p | bound holds; untouched tapes bitwise equal to a no-switch render | tape differs |
| TG1 | guard scrubs 9600; 0 non-finite downstream | `tankGuardOff`: 286 231 non-finite |
| RS2p | slot == pristine at 200 ms and 2 s, 0 writes | `preHoldOff`: 8640 / 104 640 writes |
| MU1 | muted slot stops writing (parks at 370 ms); others continue | `gateBIgnoreAudible`: keeps writing |
| IN1 | other tapes and a solo render bitwise unchanged | 10 / 10 parameters move the changed stream |
| SI1 | null −185.5 dBFS | `tankOnSum`: −37.8 dBFS |
| TK1 | tank runs while sounding, idles after, re-entry bit-equal; 4 tanks called every piece | `tankGateSticky` |
| MZ1 | N1 hash == golden; hard-panned N1 keeps its zero signs | `addZeroTerm`: 47 298 signs flipped |
| FA4p | 0 audio-thread allocations | armed malloc counted |
| CK7 | κ 0.945× / 1.315× | `fbOnHardStop`: 14.7× / 28.0× |
| CK8 | κ on the return 1.227× / 1.400× | `preHardCut`: 10.3× / 20.1× |
| CK9 | κ 1.166× / 0.288× | `revTailOff`: 10.8× / 15.6× |
| DC1 | 0 class mismatches of 150 | `destClassLegacy`: 32 |
| MC1 | Velocity → `s2_reverb_mix` and Mod Env → `s3_feedback` move and follow the newest voice | flat |
| UHD1 | dashed head on lane 2 only | stream-1-only rule fails |

INFO: **PreTail** — in Pre (Size 100, Mix 60, feedback 100 %) the head keeps writing 9.7 s after note-off; return peak 10 s later 0.000007. **FL8p** — chain-input peak 2.1044, written tape peak 0.5000. **CK8o** — the output cannot see the Pre ↔ Post switch (1.000×). **C25mPre** 30.93 %.

### Carried rows whose text moved (PD3 / PD16)

- **B1 (T8), measured:** CL1 knee peak 1.96514666 → 1.99324679 (threshold holds); NZ1 control count 95998 → 95997; FL7 control count 283754 → 283753.
- **W3:** AP1 control count 1 → 3; OS1 and CM1 labels.
- **B2 (T20), counts and labels only:** G1–G3 (261 → 293; 176 / 70 / 15 → 200 / 74 / 19), G4 (skew rows 19 → 25), G5 (70 → 74), G6 (19 Bools, five On), D1, D2, D4, D5 (150, version 2, stride 36), S1a, S2, N2, N2s, FN3, FL7 (bound counts), AU1, FP4, DR1 (149 / 149), UMN1, UMN1n, UMB1, UPM1, UPM2, UPM2n, UPM4, UPM6, SS3 (blob size). No measured value moved on any ported row.
- Wall-clock rows that differ run to run and are not moves: CM1, H4, SR1, UCM6b, UCM6bn, USV2, USV3, USV3n.

### Re-pins

- `.planning/params.tsv`: 261 → 293 rows. 237 survivors identical; the 16 `modM_dest` rows differ in `numSteps` only (126 → 150); 8 globals gone; 32 new Float rows equal their old global row except id and name; 4 Bool + 4 Choice new.
- `tests/ui-stub/generic-overrides.json`: 8 global entries out, 36 new ids in (the file never carried toggles, so the 4 Bools have no entry), 16 destination choice lists at 150 names.
- STATUS checksums: `parameter_spec`, `parameter_spec_body` (C5); `architecture`, `roadmap` (C7). `requirements` unchanged.

## Deviations (each with its reason)

1. **T24 ran before T23.** A DSP fix at T24 would have re-run the showcase rows. Bank loudness and CPU verdicts were taken on the final DSP either way.
2. **Engine fix found by RS2p (`resetTailHold`).** After a Reset on a Pre stream with notes off, the head resumed on the residual return and wrote the old tail into the fresh copy. A Reset take of a Pre stream now drops the tail term of the head's run target until a voice drives that stream again. Never set on a Post stream. Renders, PT rows, parity and the carried extract did not move.
3. **B1 fade hold (`tankFade`, 60 ms), found by FP6.** A stream whose audible reverb gate dropped left the sum as a step; the term now stays joined while the module ramps out.
4. **FL1p / FL1sp stimulus raised.** With FL1's grain rows on noise tapes the Pre loop decays and the validity clauses cannot be reached. Key Track 0, Pitch 0, Scan Rate 100 % were added; predicates and thresholds are unchanged; the literal stimulus prints as INFO FL1lit.
5. **MZ1 second clause.** No golden job has a −0 output sample, so `addZeroTerm` cannot move N1's hash. A hard-panned N1 clause carries the control.
6. **CK9 geometry:** mute at sample 48032 with a silent second stream enabled. **UHD1n** is a harness-side model; **TK1** stops by mute; **IN1n** runs the 10 parameters separately.
7. **AC7 (b) amended:** save / restore / save differs in the `<CustomScale version>` runtime counter only (carried behaviour, not R1's); every PARAM row is identical.
8. **Showcase judgement calls.** Pre needs feedback above 0 to act, so Four Lanes S4 and Fifth Bloom S3 got feedback 25 / diffusion 30. Organ Loft S1 feedback 30 → 0. Phase Garden's 7th route uses LFO 3 (global, 0.08 Hz). Broken Radio `s1_level` sits at 5.7 of 6.0. Four descriptions are slightly longer than the previous longest (up to 112 characters).
9. **Extractor additions beyond PD15:** a class-column check (V for fields 0–27, G for 28–35) and a general per-stream Bool default grammar.
10. **Two parity gate headers edited** (`parity_gates_24.h`, `parity_gates_25a.h`) for the ours-side ID map; `parent_render.cpp` untouched.
11. **Trace samples are per stream** for the feedback and reverb destinations (DR1 149 / 149 depends on it).
12. **VE15 also checks the neighbour's readout box**, not only its text (the text-only check could not see the 56 px rule regress). ST1 asserts 48 states.
13. **T15:** `.sw.off` dims a disabled stream's feedback switch; the `stream-off-head` and `fb-off` states assert it. T5's pixel measurements were not re-run after the strip (VE15 re-measures the row-4 gap: 8.7 px).
14. **T27 pluginval ran one seed per format per sidechain setting** (three seeds each were proven at T14 on the auto-clear). **The Standalone smoke shows the Streams page only:** the window boots in the real WebView, but the FX tab could not be clicked from the shell.
15. **PD17 wording:** "253 identical" is 237 identical + 16 `modM_dest` rows with a new step count; "40 new ids" in the overrides is 36.

## Residual risks (recorded, not fixed)

- **Pre holds and tails (sign-off 5).** In Pre the head keeps writing for the tail's length after note-off (9.7 s at Size 100). Hands-on row R1-PRE1.
- **Reset in Pre with notes held** can still write roughly 30 ms of fading old tail. No row gates it.
- **Loop-tank tail survives a new file or a recording** into a Pre stream (only a Reset arms the hold). Gate B reads its inputs one cell late.
- **FL2p is green but weak** at its stimulus (the loop decays, so 16 voices sit far below 1 voice); its control fires.
- **Pre-R1 sessions and user presets re-map silently** (sign-off 11); hands-on row R1-S1.
- **French destination titles still clip** in the 98 px select (33 of 150; OS3 default).
- **Broken Radio** has almost no level trim left on S1.
- Carried: PN1 zero margin; `--cpu4` W does not force 16 voices; `~SlotLoader` join; the `<CustomScale version>` counter.

## Open items for verify

- Re-run per PLAN-R1 "What verify must re-run" (R-core, R-ui, R-val with three seeds, `--egr1`, PT rows while `build/plugins/O-GranulatorInstrument/r1-b0` exists, extractor into scratch, CPU alone, drift + a read of the `ControlGrid.h` diff, the 7 contract hashes).
- A Standalone or DAW look at the FX page in the real WebView (not captured here).
- HANDS-ON-SESSION-4 (D51): 243 run rows incl. the F1 retests and the R1 rows.
- D54 Windows CI arm: pending `/publish` (T32 standing answer: hold).

## Requirements

REQUIREMENTS.md was not edited (checksum unchanged). Verify rewrites the evidence lines of: DSP-04 (per-stream rows), DSP-05 (per-stream reverb wording; parity evidence unchanged), DSP-06 (DR1 149 / 149), FUNC-07 (auto-clear), FUNC-08, PERF-02 (four / eight tanks, C25mPre).
