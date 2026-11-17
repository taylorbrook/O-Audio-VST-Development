# Stage 4: Polish — Phase 4.1 (build: factory bank, host text, page fixes, i18n, G10 mode, Windows prep) - Verification

## Verification Date

2026-11-16 (verify session). Everything below ran against `e3ee9546` (0.10.0). Before the run, `git status` showed no changes under `plugins/O-ParametricEQ`.

What the verify session did:

- **Independent harness re-run** (`verify-4.1/run-harness.sh`). The harness objects were deleted and recompiled. Then, in order: session dump, SHA list, designer, render default with `--lid-dir` on this session's dump, `--dump-params` against the golden, probe N1e, and render `--long`. Tallies are in `verify-4.1/harness-tallies.txt`.
- **G9 (b)** and **meter-ballistics** re-run.
- **Code read** of the full `4acfc9c1..e3ee9546` diff:
  - `FactoryPresets.h` and `ParamText.h`, in full;
  - `PluginProcessor.{h,cpp}` (`initFactoryBank`, float attributes) and `PeakHold.h`;
  - in `index.html`: `deleteBand`, the point-tip fade path (`hideTooltip`, `schedulePointTipRemoval`, `cancelPointTipRemoval`, `clearPointTip`) and the 800 px media rule;
  - `i18n.js`, `CMakeLists.txt` and `ci-tests.yml`.
- **Independent gates:**
  - the five UI gates, PN1 / PN1n and the WebView2 static gate;
  - `diff -rq` of the installed bundles against the artefacts;
  - a `build.ninja` check that `OPEQ_TEST_HOOKS` is defined only on the harness;
  - a decode of the real `Factory/*.json`.
- **Validators** on the installed bundles: pluginval strictness 10 (VST3, AU) and `auval -v aufx OuPQ OuDv`. The real `Factory/` listing was hashed before and after.
- **Execute's committed bench JSON re-scored:**
  - `verdict41.py` over `rel-1.json` and `dbg.json`: ALL-PASS for both.
  - `verdict33.py` over `rel-1.json`: ALL-PASS.
  - `dbg.log`: 0 assertions.
- **Verify mutation run** (`verify-4.1/mutants41v.py`, `mutants41v-results.json`):
  - 13 one-line mutants, none of them among execute's 14.
  - 8 C++ mutants are scored against a named `--rows41` row.
  - 3 are probes (is there any row at all?).
  - 2 page / gate mutants are scored by PN1 and by `check-ui-labels` on a scratch root.

Nothing in the repo source changed. Scratch builds and the mutable source copies are in the session scratchpad.

Scope: Phase 4.1. Stage 4 stays open (4.2: the binding battery, `--g10`, the push and the Windows dispatch, and Taylor's sitting).

## Goal-Backward Analysis

### Original Goals (CONTEXT P1 – P6 as split by RESEARCH; PLAN-4.1 Goal)

1. **Factory bank (FUNC-07).**
   - 13 task presets, written in engineering units.
   - A bank hash, so that a re-voice or a rename reaches `~/Library` at an unchanged version, with no orphan and no `User/` touch.
   - The E16 / R2 rows.
2. **Host text** in units, parsed back. No audio change.
3. **Carried page fixes**, each with a row that kills its regression: Delete ends listen at once; the point tip keeps its size through its fade; strip `aria-pressed`; the meter hold under jitter.
4. **Localisation, build half (UI-05).** The glossaries cover the EQ vocabulary, fr and zh are revised, and the five UI gates read 0.
5. **G10 mechanism (PERF-02).** The gated `--g10` mode exists. 4.2 takes the binding reading.
6. **Windows prep (COMPAT-02).** The WebView2 static gate, and `ci-tests.yml` able to dispatch O-ParametricEQ.
7. **Audio frozen.** 0.10.0 builds, installs and validates.

### Deliverables (from SUMMARY-4.1.md, confirmed in the tree)

1. **Factory bank:**
   - `Source/FactoryPresets.h`: 13 specs, `addBand`, `makeFactoryPresets` (one `convertTo0to1` per value through the live parameter), XXH64 `bankHash`, and the `OPEQ_TEST_HOOKS` spec edit.
   - `initFactoryBank()` at the end of the constructor.
2. **Host text:** `Source/ParamText.h`, plus `attributesFor (kindForId (id))` on every float.
3. **Page fixes:**
   - `deleteBand` → `endListenOn(b)`.
   - Point-tip removal deferred to the opacity `transitionend`, with a 160 ms fallback timer.
   - `PeakHold::jitterMargin` 1.3.
   - The 800 px header rule. Pre pinned at 39 px.
4. **Localisation:** 18 glossary entries (fr, zh and the prose companion), plus the `i18n.js` revisions.
5. **G10:** `--g10` and `g10SelfCheckRow`.
6. **Windows prep:** `tests/windows-webview2-gate.sh`, and the `ci-tests.yml` option line.
7. **Build:** 0.10.0. CHANGELOG `## [0.10.0] - 2026-11-16` (bracket form).

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 Factory bank | ✅ Achieved (listening pass in 4.2) | **FP1** 2535 / 2535 at defaults, 152 named values back in units. **FP1o**, **FP2** (a) – (e), **FP3**, **FP4**: all PASS on this session's rebuild.<br>• **Real `Factory/`:** 13 JSON + `.factory-version` (0.10.0) + `.bank-hash` (`2dca9fb8ba6a7172`). `User/` is absent.<br>• **Choice values decoded by hand:** Mono Low End placement 1.0 (Side = 4 / 4); slope s24 3 / 7; Telephone s48 5 / 7, Band Pass 6 / 7, High Cut 4 / 7. `Default.json` carries `{}`.<br>• **pluginval 10 (VST3 and AU) and auval** constructed the processor many times under the real home, and the `Factory/` listing (names, sizes, mtimes) hashed identical before and after. The stamp short-cut holds outside the harness.<br>• **Verify mutants caught:**<br>– V6, delete + rewrite on every construct → FP2 (b);<br>– V7, mismatch keeps `.factory-version`, so the bank is empty → FP2 and FP3;<br>– V8, `bankHash` ignores values → FP2 (c).<br>• **Probes with no row** (Issue 2): V9 (placement dropped from every def), V10 (Rumble Cut 96 → 6 dB/oct), V11 (stamp written after a failed write). |
| 2 Host text | ✅ Achieved in-process; host display by hand | **HT1** and **HT1g** PASS; `--dump-params` is byte-equal to the golden (17 634 bytes).<br>• **Verify mutants caught by HT1:** V2 (the "-0.0" guard removed), V3 (the Hz / kHz threshold on the raw value), V4 (the leading minus dropped), V5 (`k` ignored by the parser).<br>• **Code read:** a comma is read as a decimal point, so "1,200 Hz" parses as 1.2 Hz (Issue 3).<br>• Logic and Reaper display is a 4.2 hands-on row. |
| 3 Page fixes | ✅ Achieved | **E-del-listen, E-listen33-aria, E-tipfade, E-tip-lang, E-tip-v13, E-preset-listen**: execute's committed `rel-1` and `dbg` re-score ALL-PASS (`verdict41.py`), and the 3.3 rows ALL-PASS (`verdict33.py`).<br>• Execute's M1 – M3 are caught by those rows.<br>• **M1j** PASS on this rebuild (1024 / 44.1 k H 3 and 2048 / 44.1 k H 5: 0 empty reads; the H − 1 control fires).<br>• Verify V1 (jitter margin back to 1.0) is caught by M1j: 2048 / 44.1 k H 4 gives 4 empty reads.<br>• **Code read:** a re-show calls `cancelPointTipRemoval`. The immediate path (`showTooltip`) still calls `clearPointTip`. The reduced-motion case has the timer fallback. |
| 4 Localisation (build half) | ✅ Achieved; fr sign-off in 4.2 | **Five UI gates at 0:**<br>• `check-i18n`: ALL PASS, 49 plugins.<br>• `i18n-fr-lint`: O-ParametricEQ 74 rows, 0 findings.<br>• `i18n-zh-lint`: 74 rows, 52 `mt`, 0 findings.<br>• `check-ui-labels`: ALL PASSED, at the widest language.<br>• `boot-all-uis --strict-tips`: 0 DEAD.<br>**PN1** PASS for en / fr / zh-Hans (widest 109 / 135 px), and PN1n fires. Verify mutants:<br>• V13 (Pre back to 35 px) is caught by `check-ui-labels`: zh 处理前 36.8 > 34.0.<br>• V12 (the 800 px rule keeps the Analyzer caption) is caught by PN1: the box shrinks to 63 px and 8 names fail in each language. This needed a scratch-root rerun: see Issue 4. |
| 5 G10 mechanism | ✅ Mechanism present; the binding reading is 4.2 | `--g10` and `G10d0` exist, and execute's reading was 5 / 0. This session did not run `--g10`, because 4.2 is binding. **G10d0 is load-sensitive** (Issue 1). |
| 6 Windows prep | ✅ Achieved | WebView2 static gate PASS on this session's run. Execute's 7 / 7 negative controls are recorded. `ci-tests.yml` lists `- O-ParametricEQ` under `options:`; `default:` is unchanged. |
| 7 Audio frozen, validates | ✅ Achieved | **Sessions:** 16 / 16 SHA-256-equal to `verify-3.3/sessions-090.sha`, which are the 0.5.0 hashes.<br>• **Probe N1e:** 16 / 16, hash `6274ff9b5b0e488a`.<br>• **Designer:** 150 / 0 / 18. **Render default:** 183 / 0 (this session's dump). **Render `--long`:** 183 / 0.<br>• **G9 (b):** PASS, worst 5.16e-9 dB at 44.1 k. Meter-ballistics PASS.<br>• No `processBlock` change in the diff.<br>• Installed `-dev` VST3 and AU are `diff -rq` identical to the artefacts, version 0.10.0, with no unsuffixed orphan.<br>• **Validators:** pluginval 10 VST3 SUCCESS, AU SUCCESS; `AU VALIDATION SUCCEEDED`. |

## Requirements Verification

**Stage:** stage-4. Phase 4.1 builds what the stage-4 requirements need. Their binding criteria (the listening pass, G10, hosts and Windows) are 4.2.

| Requirement | Priority | Status | Acceptance criteria |
|-------------|----------|--------|---------------------|
| FUNC-07 Presets via preset-manager, with a factory set | should | ⚠️ Partial | 13-preset bank built and gated (FP1 – FP5i, E16 / R2). Owed in 4.2: loading in Logic and Reaper, and the listening pass. |
| UI-05 House style, fr / zh-Hans | should | ⚠️ Partial | Glossary and revision done, gates at 0. Owed: Taylor's French spot read, then the fr flip. zh stays `'mt'` by suite convention. |
| PERF-02 CPU budget | must | ⏸️ Pending (4.2) | Mechanism in place. Execute's reading of 5 / 0 is not binding. |
| COMPAT-02 Layouts, rates, platforms | must | ⏸️ Pending (4.2) | FP5 renders all 13 presets at five rates, mono and stereo (criterion 2's preset half). Logic, Reaper and the Windows CI run are 4.2. |
| QUAL-01 Transparency | must | ⚠️ Partial (unchanged) | The listening pass across the factory presets is 4.2. |

**Requirements Summary:** Complete 0, Partial 3, Pending (4.2) 2, Failed 0. This is expected for a build phase. The statuses in REQUIREMENTS.md are updated to match.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Harness rebuild | ✅ | `build rc 0` after the objects were deleted |
| Designer | ✅ 150 / 0 / 18 | |
| Render default, `--lid-dir` | ✅ 183 / 0 | 171 + FP1, FP1o, FP2, FP3, FP4, FP5, FP5k, FP5r, FP5i, HT1, HT1g, M1j |
| Render `--long` | ✅ 183 / 0 | same dump |
| Sessions vs 0.5.0 | ✅ 16 / 16 | `verify-4.1/sessions-0100.sha` |
| `--dump-params` vs golden | ✅ byte-equal | |
| Probe N1e | ✅ 16 / 16, `6274ff9b5b0e488a` | |
| G9 (b) | ✅ PASS | worst 5.16e-9 dB (44.1 k), all controls fire |
| Meter-ballistics | ✅ PASS | |
| UI gates ×5 | ✅ all 0 | logs in `verify-4.1/gates/` |
| PN1 / PN1n | ✅ PASS / fires | |
| WebView2 static gate | ✅ PASS | |
| `OPEQ_TEST_HOOKS` | ✅ harness only | 27 `build.ninja` lines, all on targets that define `GOLDEN_DIR` (the harness) |
| Install | ✅ identical | 0.10.0, `-dev` only |
| pluginval 10 VST3 / AU | ✅ SUCCESS / SUCCESS | |
| auval | ✅ | |
| Real `Factory/` across the validators | ✅ unchanged | listing hash equal before and after |
| Committed bench JSON re-scored | ✅ ALL-PASS | `verdict41` rel-1 and dbg; `verdict33` rel-1; dbg 0 assertions |
| Verify mutation run | ✅ 10 / 10 scored caught | 3 probes with no row; the control is in Issue 1 |

### Mutation run (verify)

| Mutant | Target | Result |
|---|---|---|
| control (harness) | all `--rows41` rows PASS | **FAIL on G10d0 only** while `render --long` and pluginval ran alongside: c 7.78, 1 band 8.25, 24 bands 9.54 ms. The same control built and run alone a few minutes later: all 9 rows PASS (c 4.90, 24 bands 7.75 ms). See Issue 1. FP / HT / M1j rows PASS in both runs |
| V1 `jitterMargin` 1.3 → 1.0 | M1j | caught (2048 / 44.1 k H 4: 4 empty) |
| V2 dbToText "-0.0" guard removed | HT1 | caught |
| V3 Hz / kHz threshold on the raw value | HT1 | caught |
| V4 parser drops a leading minus | HT1 | caught |
| V5 textToFreq ignores `k` | HT1 | caught |
| V6 delete + rewrite on every construct | FP2 | caught |
| V7 mismatch keeps `.factory-version` (empty bank) | FP2 | caught (FP2 and FP3) |
| V8 `bankHash` ignores values | FP2 | caught |
| *probe* V9 placement dropped from every def | — | **no row** (Issue 2) |
| *probe* V10 Rumble Cut slope 96 → 6 dB/oct | — | **no row** (Issue 2) |
| *probe* V11 stamp written after a failed bank write | — | no row: not reachable without a write fault (Issue 2) |
| control (PN1) / V12 caption kept below 960 px | PN1 | control PASS; V12 **caught** on the scratch-root rerun: box 63 px, 8 names fail in each language (Issue 4) |
| control (labels) / V13 Pre width 35 px | `check-ui-labels` | control PASS; V13 caught: zh 处理前 36.8 > 34.0 |

## Human Verification

For Taylor, in 4.2. This is the SUMMARY-4.1 checklist, unchanged, plus one line from Issue 3:

- [ ] Analyzer modes Off / Pre / Post / Pre+Post, in Logic and Reaper.
- [ ] Meter against Logic's at a 2048 buffer: steady, no dips (H = 5).
- [ ] Listen by ear: latch, drag, and Delete ends it at once.
- [ ] Standalone with live input.
- [ ] Meter look at 800 × 500 and 1600 × 1000. At 800: the header has no Analyzer caption, and the preset names show in full.
- [ ] Every factory preset in the listening pass. Re-voice as needed; the bank hash carries a re-voice, but a reinstall is needed.
- [ ] Mono Low End on a mono track: the whole signal is cut below 120 Hz (C2).
- [ ] Host text in Logic (AU) and Reaper (VST3): `1.20 kHz`, `+3.0 dB`. Typing `1k` and `-3 dB`. Typing `1,200` gives 1.2 Hz → clamped to 10 Hz (Issue 3): decide whether that matters.
- [ ] French spot read in the Standalone, including "signal central" and the preset bar. Then flip fr to `reviewed: true`.
- [ ] zh at 800 px: 处理前 / 处理后 / 处理前+后 in the analyzer segment.

## Issues Found

1. **G10d0 is load-sensitive (minor; a 4.2 run-order note).**
   - The self-check times a synthetic cost that is quadratic in the band count. With another harness and pluginval on the machine, the quadratic term was buried in noise (1 band 8.25 ms vs 24 bands 9.54 ms). G10d then passed, so the self-check failed.
   - The same thing showed as a collateral G10d0 FAIL on mutants V1 and V4. Neither is scored on it.
   - Run alone, it passes. It is the same "busy machine reads as noise first" rule as VERIFICATION-3.3 Issue 4.
   - Recommendation: in 4.2, run `--g10` (which includes G10d0) with nothing else running, and allow one re-run.
2. **The bank's content is not gated, only its mechanics (minor; 4.2 plan).**
   - FP1 reads every preset back against the same spec table it was built from. A wrong value in the table is invisible: V9 drops every placement, so Mono Low End, Side Air and L-R Pair Match turn into plain stereo, and V10 makes Rumble Cut 6 dB/oct.
   - FP5i prints the Mono Low End mono-vs-stereo level, but as INFO.
   - By design, the listening pass is the content check. But a re-voice in 4.2 can also break a placement or a slope by accident.
   - Recommendation for 4.2: a content row that pins either the spec table or the bank hash. An intended re-voice updates the pin in the same edit, so an accident shows as a FAIL. Also gate FP5i's Mono Low End (mono cut > stereo cut).
   - V11 (the stamp written after a failed write) needs a filesystem fault to show. Code only, no row.
3. **A comma is read as a decimal point in host text (minor; hands-on decision).**
   - `splitNumber` maps `,` to `.`, which suits a French user typing "0,7". But "1,200 Hz" reads 1.2 Hz and clamps to 10 Hz, and "1e3" reads 1.
   - JUCE's default parser has the same comma behaviour, and the page's own entry accepts exponents.
   - Low impact. Recorded for Taylor's host-text row.
4. **PN1 cannot be pointed at a scratch tree (info; tooling).**
   - `preset_name_fit.js` calls `S.buildRoot('O-ParametricEQ')` without `repoRoot`, so it always serves the real repo page. The first run of V12 therefore measured the unmutated page.
   - With `{ repoRoot }` passed in a scratch copy, V12 is caught. In-repo use is unaffected.
   - Recommendation: pass `{ repoRoot }` (one token) the next time the file is touched, so mutation runs work.
5. **Construction now does file I/O (info).**
   - Every processor reads `Factory/.bank-hash`, and writes the bank only when the stamp is stale. The `presetManager` header comment is updated to say so.
   - The pluginval runs (many constructions) left `Factory/` untouched.

Carried forward unchanged: the 4.2 scope in SUMMARY-4.1 "For 4.2" (C1 push with Taylor's go-ahead at that moment, Windows dispatch, the binding battery with `--g10` and `--long`, the sitting, the fr flip).

No blocker. No scored mutant survived.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 4.1). Every automated criterion in PLAN-4.1's success list is met and was reproduced on an independent rebuild. The audio path is frozen (16 / 16 sessions, N1e hash). Stage 4 stays open.

**Ready for next phase:** Yes. 4.2 (validation): the fresh binding battery with `--g10` (run alone, Issue 1) and `--long`; the C1 push and the `ci-tests.yml` Windows dispatch; Taylor's sitting; re-voice and the fr flip. Fold Issue 2's content pin into the 4.2 plan.

**Blockers:** none.
