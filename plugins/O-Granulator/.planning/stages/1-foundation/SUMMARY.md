---
plugin: O-Granulator
stage: 1-foundation
phase: execute
date: 2026-11-04
status: complete
commits:
  - f7398147  W1 pitch-collections 1.0.0
  - a480ac41  W2 O-GrainScatter 3.6.0
  - (W3+W4)   O-Granulator 0.1.0 shell — see git log
---

# O-Granulator — Stage 1 (Foundation) — SUMMARY

## Result

All four waves landed in plan order (module → GS → Granulator → validation). Every
Stage 1 success criterion is met; see the table at the end.

## W1 — `modules/tuning/pitch-collections` 1.0.0 (f7398147)

- `cpp/PitchCollections.h`: GS 3.5.1 `PitchEngine.h:41-305` moved **verbatim** under
  `ouaricon::tuning::PitchCollections` (diff-checked: lines 42-236 and 253-304 are
  byte-identical; only the 3-arg `tableFromText` became a wrapper). juce_core only,
  builds at C++17.
- Additions: `edo N [period]` (regex + error order per RESEARCH §1d, built with
  `equalDivision` so `edo 19` ≡ Edo19 and `edo 13 3/1` ≡ Bohlen-Pierce bitwise),
  4-arg `tableFromText` with `badToken`, `toScala` + `scalaDescription` (P2).
- Tests 229/229: U1 builtIns digests == v3.5.1 for all 32 Ids; U2 edo equivalences;
  U3 edo errors + order; U4 offending token, 3-arg ≡ 4-arg; U5 v3.5.1 corpus (34 texts
  + 7 `.scl`, captured by a scratch console compiled against `0e1117b6` BEFORE the
  move, `tests/expected_v351.inc`); U6 toScala bitwise round-trip.
- Registry entry, `version` 1.2.18 → 1.3.0 (→ 1.3.1 after `regen-registry-used-by.sh`).

**Found, not fixed (pre-existing, frozen):** `JustSevenLimit` lists 12/7 after 7/4,
so that built-in table is NOT ascending. GS's `PitchEngine::rebuild` walks degrees
in table order and comments "so the table is sorted" — its active table for 7-Limit
Just is out of order (ladders step 7/4 → 12/7 backwards). Fixing it changes GS audio,
so it is outside a bit-identical move. Flagged for a GS `/improve`; Granulator Stage 2
must sort its own active table.

## W2 — O-GrainScatter 3.6.0 (a480ac41)

- `PitchEngine.h`: namespace deleted, module include + alias; CMake `VERSION 3.6.0` +
  `ouaricon_add_module(... pitch-collections)`; `badEdo` in `app.js` + i18n en / fr
  (`reviewed:false`) / zh-Hans (`reviewed:'mt'`) (P4 minimum); CHANGELOG `## [3.6.0]`;
  STATUS + PLUGINS.md. No tag.
- New `tests/pitch-null` + `pitch-null-v351` twin (`git archive 0e1117b6`):
  **415/415** — 190/190 renders byte-identical (R1 32 Ids × 3 seeds, R2 72 ladder
  rows, R3 12 masks, R4 7 texts + 3 `.scl`), parse corpus byte-identical, positive
  control (edo) differs, seed liveness 32/32, quietest render −17.9 dBFS.
  Harness control: the same run on the UNMODIFIED tree reports the positive control
  FAILED (so the harness can tell). Harness bug fixed on the way: `replaceWithText`
  defaults to CRLF.
- param-dump diff empty. `ninja -t deps`: all four old-tree twins resolve their own
  `dsp/PitchEngine.h`; the new tree resolves the module header.
- Existing gates: blocksize-null 21, density-gain 25 (compoff ref), alias 11
  (v320 control fails rows 1/2/4), skip-tilt 31 (v330), adapt-gate 48 (v340; fixed-K
  control fails M1) — ALL PASS.
- check-i18n ALL PASS, fr lint CLEAN, zh lint 0 findings, check-ui-labels ALL PASSED.
- pluginval 10 VST3 + AU (in-process) SUCCESS; `auval -v aufx OuGS OuDv` PASS (3.6.0).

## W3 — O-Granulator shell

| File | Content |
|---|---|
| `CMakeLists.txt` | from `v2-CMakeLists.txt`; pitch-collections on; `cxx_std_20` on every console; render-harness console with `GOLDEN_DIR` |
| `Source/GranulatorParamIds.h` | frozen Def tables (global 8, stream 32, fb+rev 8, LFO 4, env 3, onset, macro, mod 3), `registrationOrder()` / `allIdsInOrder()`, literal `kModDestIds[113]`, `kModDestTableVersion = 1`, `scaleChoiceToId` + static_asserts |
| `Source/PluginProcessor.{h,cpp}` | 220 params (`ParameterID{id,1}`, interval 0, skew / `setSkewForCentre`, no lambdas); 3 bus layouts; bitwise pass-through (mono→stereo copies ch 0 → 1; loops bounded by the buffer); `setLatencySamples(0)`; seed (`std::atomic<uint64_t>`, random per instance); `builtIns()` warmed in ctor; scale-state API matching the v2 editor scaffold (`getCollectionTable`, `getCustomScale`, `setCustomScale(name,text,badToken)`, `get/setDegreeMask`, `getScaleStateRevision`, `uiLanguage`); state codec per RESEARCH §6 |
| `Source/PluginEditor.{h,cpp}` | No editor class yet (D2: `GenericAudioProcessorEditor`). Holds `granulatorUiResource()` — the scaffold's resource map over `UIBinaryData` — see deviation 1 |
| `Source/ui/public/` | `index.html` (v2-ui.html), `js/i18n.js` (v2-i18n.js), `js/juce/{index,check_native_interop}.js` (JUCE 8.0.15, identical to GS) |
| `tests/golden/extract_spec.py` → `params.tsv`, `mod-dests.tsv` | spec + YAML derived goldens; P1 names (user-approved) |
| `tests/render-harness/main.cpp` | rows G1-G6, D1-D4, B1-B3, S1-S4, P1 + MockPlayHead |
| `.planning/params.tsv` | from `O-Granulator-param-dump` (220) |

State codec: `<Engine seed (int64 string) stateVersion modDestTableVersion>`,
`<CustomScale name text period version>`, `<DegreeMasks><Mask stream collection bits/>`
(16 hex, non-default only, canonical); no `id` attributes; written on the
`copyState()` copy (old children stripped first, so reopens never duplicate);
restore resets non-param state first, then reads with `isVoid()` guards; uiLanguage
as the code string, read tolerantly (P3, spec line amended).

**Render harness: 47/47 PASS.** G1 220 / ID order, G2 140/59/21, G3 hint 1 + names,
G4 ranges + normalised defaults exact (worst engineering default error 5.1e-8 of
span), G5 59 choice lists, G6 bools; D1-D4; B1 layouts; B2 bitwise in 3 layouts ×
{defaults, mix 0 %} × {no playhead, playing playhead} × blocks {1, 32, 441, 512, 4096,
variable}; B3 latency 0; S1 seed 0xF00DFACECAFEBEEF, edo 19 / edo 64 + raw
0x8000000000000001 mask, uiLanguage code + int form; S2 absent state → defaults; S3
two reopens → exactly one of each child, 220 PARAM; S4 no Mask children; P1 map.
**Negative control:** perturbing the goldens (swapped IDs, one default, one choice
string, two dest rows) fails 7 rows; restored → ALL PASS.

Grep gate: zero `setValueNotifyingHost` in `Source/`.
UI gates with the copied page: check-i18n ALL PASS (47 plugins), fr lint CLEAN, zh lint 0.

## W4 — install + COMPAT-01

- `./scripts/build-and-install.sh O-Granulator` → `O-Granulator-dev.{vst3,component}`
  (after deviation 2 below).
- pluginval `--strictness-level 10 --timeout-ms 600000` on the VST3: **3 runs, 3 × SUCCESS**
  (220 params, Generic editor, no `--skip-gui-tests`).
- pluginval 10 `--validate-in-process` on the AU: **SUCCESS**.
- `auval -v aufx OuGn OuDv`: **AU VALIDATION SUCCEEDED**, component 0.1.0, channel
  capabilities exactly `[1, 1] [1, 2] [2, 2]` (D3 frozen before first install).
  Negative control `auval -v aufx ZZZZ OuDv`: "didn't find the component" (rc 2).
- Standalone: launches, stays up 6 s, exits on SIGTERM.

## Success criteria

| Criterion | Result |
|---|---|
| VST3 + AU + Standalone build; auval PASS; pluginval 10 VST3 + AU | ✓ |
| B2 bitwise pass-through, 3 layouts, defaults + mix 0 % | ✓ 12 rows × 6 block patterns |
| D1-D4 113-entry dest table, frozen order, all Float | ✓ |
| G1-G6 220 params, ID order, 140/59/21, hint 1, ranges/defaults/choices | ✓ |
| S1-S4 seed > 2^63, high-bit masks, custom scale, uiLanguage, two reopens | ✓ |
| pitch-collections U1-U6 | ✓ 229/229 |
| GS 3.6.0: pitch-null + control, 5 gates, param-dump, i18n gates, pluginval + auval | ✓ |
| No `setValueNotifyingHost` in Source; no tag; foreign untracked files untouched | ✓ (`nielsen-dafx26.txt` never staged) |

## Deviations from PLAN.md

1. **`PluginEditor.cpp` is not comment-only.** Copying the UI resources made
   repo-wide `check-i18n` fail rule [8] ("PluginEditor.cpp serves js/i18n.js"). Rather
   than ship a red repo gate, the scaffold's resource map moved in now as a free
   function `granulatorUiResource (url)` over `UIBinaryData` (all 9 resources). No
   editor class, no WebView: Stage 3 adds `GranulatorEditor` and points
   `withResourceProvider()` at it.
2. **`CMakeLists.txt` header comment reworded.** The mockup's comment contained the
   text `juce_add_plugin()`; `scripts/resolve-target.sh` matches that inside a
   comment and resolved the target as `cmake_minimum_required(VERSION`, so
   `build-and-install.sh` failed. **Tooling bug (unfixed, shared script):** the awk
   should strip comments before matching `juce_add_plugin(`.
3. **check-ui-labels on Granulator: deferred to Stage 3** — it needs the WebView
   editor's `setSize` frame (per-plugin gate, not in CI).
4. U1 asserts "degrees inside the period", not "ascending" (see the 7-Limit Just
   finding above).

## Notes for Stage 2

- Skewed defaults are not bit-exact through APVTS (fb_high_cut reads 12000.00098 if
  the golden is nudged; worst real error 5e-8 of span) — never derive unity gain
  from `get()` on a skewed range (RESEARCH R4).
- Sort the active pitch table (7-Limit Just is unsorted at the source).
- The audio thread needs its own copy of the custom table (`getCollectionTable (31)`
  returns the message-thread member).
- `MockPlayHead` (bpm, playing, timeInSamples, ppq, `advance`) is in the harness.
