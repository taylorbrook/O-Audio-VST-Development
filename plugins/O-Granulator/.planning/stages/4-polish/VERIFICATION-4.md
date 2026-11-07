# Stage 4: Polish / Validation - Verification

**Plugin:** O-Granulator 1.0.0 · **Stage:** 4-polish · **GSD phase:** verify
**Inputs:** CONTEXT-4.md (D42–D47), PLAN-4.md (P143–P162), SUMMARY-4.md, HANDS-ON-4.md, DAW-4.md, REQUIREMENTS.md

## Verification Date

2026-11-07

## Goal-Backward Analysis

### Original Goals (from CONTEXT-4.md)

1. **D43:** make the loop AGC engage at non-zero saturation, with no pumping at moderate feedback and no change to the ∞-tail / freeze behaviour.
2. **D47 (a)–(e):** close the carried housekeeping without changing audio (Env pin, event-aware `renderPool`, C7 calibration, W2o rule, `uiSnap` clear).
3. **FUNC-09 / D42:** ship 16 factory presets (4 × ambient / harmoniser / rhythmic / cascade). None of them may set `output_gain`, and each value must be converted with its skew.
4. **COMPAT-02:** round-trip the full state on a fresh instance, through both the session codec and preset files, including a second reopen.
5. **COMPAT-03:** gate the mono→mono, mono→stereo and stereo→stereo layouts. The Windows VST3 build is proven by release CI.
6. **UI-04:** keep the EN / FR / zh-Hans gates green at the widest language.
7. **Release:** version 1.0.0, CHANGELOG with a `## [1.0.0]` heading, auval, pluginval 10 on VST3 and AU, installed. **No tag.**
8. **QUAL-04:** a hands-on pass in Logic (AU) and Reaper (VST3), plus the Standalone preset-menu checks.

### Deliverables (from SUMMARY-4.md + code inspection)

1. `FeedbackChain::stepAgc` runs its follower on `d·|y|` (`Source/dsp/FeedbackChain.h:184`). The pre-D43 follower is still available as a test hook (`agcLegacy`). Gate results:
   - AG1: min g = 0.745 on the LP-res corner.
   - AG1n: the legacy follower gives g ≡ 1, so it fails AG1 as a negative control must.
   - AG2: growth after the hold is at most +0.0056 dB/gen.
   - AG3: 51 of 51 sat-0 renders are bitwise identical to the legacy follower.
   - AG4: 0.0 dB peak-to-peak of AGC modulation at fb 50 %.
   - AG5: the Z7d tail is unchanged (295.77 s, same as legacy).
   - AG6: env ≤ 1.
2. D47 housekeeping:
   - (a) Env pin: `Source/dsp/ModSources.h:269`, gated by Z7h3e / Z7h3n.
   - (e) `uiSnap` clear: `Source/dsp/Engine.cpp:151`, gated by US1 / US1n.
   - (b) Event-aware `renderPool`: Z6a's freeze toggles now engage (3).
   - (c) C7 calibration: 0.786 ns/op, max ratio 1.006.
   - (d) W2o rule written into the harness notes.

   Two rows moved and both are attributed: Z6a (by design) and M7c (non-finite host input).
3. `Source/FactoryPresets.h` holds the spec table and is wired at `PluginProcessor.cpp:193`, after `setExcludedParameters({"output_gain"})` and `setValidatedApply(true)`. Results:
   - PR0–PR5 + PR1n pass. PR4 is 240/240 bitwise.
   - Loudness spread is 5.94 dB.
4. State round-trip: ST1–ST3 + ST2n pass in the harness, and ST4 passes 12/12 on the installed VST3.
5. Layouts: LY1 accepts all three layouts at 44.1 / 48 / 96 k. LY1n refuses stereo→mono, 5.1 and a disabled input. Windows is still pending CI.
6. UI: all UI gates return rc 0. PF1 measures the widest name at 150 / 172 px in all three languages, and PF1n overflows as a negative control must.
7. `CMakeLists.txt` VERSION is 1.0.0, and `CHANGELOG.md` has a single `## [1.0.0] - 2026-11-07` heading. The build was installed with auval and pluginval 10 run in execute, and re-run independently below.
8. HANDS-ON-4.md (H1–H6) and DAW-4.md (L1–L7, R1–R5) were run by Taylor on 2026-11-07: **all 18 items PASS.**

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| D43 AGC restage | ✅ Achieved (automated) | AG1–AG6 + AG1n in SUMMARY-4. Code at `FeedbackChain.h:184-197` matches: `peak = d·max\|x\|`, and the 0.7 ceiling is unchanged. DAW-4 L2 / L3 still have to confirm it by ear |
| D47 housekeeping | ✅ Achieved | Z7h3e/n, US1/n, Z6a engages 3, C7 cal, W2o. Movers are attributed in the C → D → final ledger |
| FUNC-09 presets | ✅ Achieved (automated) | Installed `~/Library/O-Granulator/Presets/Factory/`: 16 files, `.factory-version` = 1.0.0, 0 files contain `output_gain` (re-checked at verify). H1 / H5 / H6 and DAW L1 / R2 still have to be run |
| COMPAT-02 state | ✅ Achieved | ST1–ST4 + ST2n. DAW L6 / R4 still have to be run |
| COMPAT-03 layouts + Windows | ⚠️ Partial (by design) | LY1 / LY1n pass on macOS. Windows VST3 is deferred to release CI at `/publish` (sign-off 4) |
| UI-04 i18n | ✅ Achieved | Gates rc 0, PF1 / PF1n. H2–H4 still have to be run |
| Release 1.0.0 | ✅ Achieved | See Automated Checks (independent re-run) |
| QUAL-04 hands-on | ✅ Achieved | HANDS-ON-4 H1–H6 PASS; DAW-4 Logic L1–L7 + Reaper R1–R5 PASS (Taylor, 2026-11-07). L2 / L3 confirm D43 by ear |

## Requirements Verification

**Stage:** stage-4
**Requirements for this stage:** 5 total (1 must, 2 should, 2 nice)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-09: Factory presets covering 4 use cases | should | ✅ Complete | 16 presets, 4/4/4/4, PR0–PR5 |
| COMPAT-02: State round-trips all params + routings | must | ✅ Complete | All params, all 16 routes and the seed restored on a fresh instance (ST1–ST4) |
| COMPAT-03: Mono/stereo I/O; Windows VST3 | should | ⚠️ Complete-pending-CI | Mono/stereo pass (LY1). Windows is built at `/publish` |
| UI-04: EN/FR/zh-Hans | nice | ✅ Complete | Lint, fit and labels green; strings stay `reviewed: false` (D45) |
| QUAL-04: Hands-on DAW pass | nice | ✅ Complete | HANDS-ON-4 PASS (6 / 6) + DAW-4 PASS (12 / 12) |

**Requirements Summary:**
- ✅ Complete: 4
- ⚠️ Partial: 1 (COMPAT-03 waits for Windows CI at `/publish`)
- ⏸️ Deferred: 0
- ❌ Failed: 0

All 30 requirements across stages 1–4: 29 complete, COMPAT-03 complete-pending-CI.

## Automated Checks (independent re-run at verify)

| Check | Result | Notes |
|-------|--------|-------|
| Source tree vs installed binary | ✅ Pass | No uncommitted `Source/` diff. Installed binaries were built at 16:38; the last source/CMake commit is 9866e37a (16:40, version-only change, built before it was committed). The installed Info.plist says 1.0.0 |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED (rc 0). There are 4 known warnings that LFO 1–4 Rate "did not retain default value": the range is skewed and the retrieved value is identical. They have been unchanged since Stage 1 (VERIFICATION-2.6). Installed `O-Granulator-dev.component` |
| pluginval strictness 10 — VST3 | ✅ Pass | SUCCESS, rc 0, 0 failed tests (--validate-in-process).  Installed `O-Granulator-dev.vst3` |
| pluginval strictness 10 — AU | ✅ Pass | SUCCESS, rc 0, 0 failed tests.  Installed `O-Granulator-dev.component` |
| Installed factory bank | ✅ Pass | 16 files, sentinel 1.0.0, none sets `output_gain` |
| CHANGELOG heading | ✅ Pass | Exactly one `## [1.0.0] - 2026-11-07` (bracket form) |
| Harness 547/547, N3/N2/N1/N0 --strict, R12, UI gates | ✅ Pass (execute) | Taken from SUMMARY-4 (T3/T6 logs); not re-run at verify |
| No release tag | ✅ Pass | `git tag -l "O-Granulator*"` returns nothing |

## Human Verification

Run by Taylor, 2026-11-07. Verdicts recorded in HANDS-ON-4.md and DAW-4.md (§Results).

- [x] HANDS-ON-4 H1–H6 (Standalone preset menu, widest name, long user name, language switch with the menu open, ◀ ▶ through all 16, Output Gain kept)
- [x] DAW-4 Part 1, Logic AU, L1–L7 (16 presets by intent, AGC on Rust Spiral, no pumping at fb 50 %, freeze re-listen, A → B while playing, save / reopen ×2, mono + stereo insert)
- [x] DAW-4 Part 2, Reaper VST3, R1–R5 (load, 4 categories incl. 93 bpm sync, automation, save / reopen, mono track)

## Issues Found

- None blocking in the automated bar. 14 execute deviations are recorded and justified in SUMMARY-4 §Deviations. No threshold was loosened, and the moved rows are attributed (FB2c / FB4 at C → D; Z6a / M7c at D → final).
- Watch item (carried to `/publish`): the `·` separator in preset file names survives the sanitiser on macOS. Confirm it on Windows when CI runs.

## Stage Verdict

**Status:** ✅ VERIFIED (2026-11-07). The automated bar is fully green and the human gates HANDS-ON-4 + DAW-4 PASS (18 / 18).

**Ready for next stage:** Yes — `/install-plugin O-Granulator`.

**Blockers:** None.

**Carried (not blocking install):**
- COMPAT-03 (Windows VST3): release CI at `/publish` (sign-off 4). Confirm the `·` preset-file-name watch item there.
- No tag. `/publish O-Granulator 1.0.0` is a separate decision.
