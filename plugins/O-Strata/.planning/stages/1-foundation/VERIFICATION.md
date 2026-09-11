# Stage 1: Foundation (second pass) - Verification

## Verification Date

2026-09-10

**Plugin:** O-Strata · **Stage:** 1 of 4 (Foundation + Shell, **second pass** — re-parameterise the verified fork) · **Mode:** manual
**Inputs:** `CONTEXT.md` (D1–D4, test criteria), `PLAN.md` (14 tasks, Decisions 1–12), `SUMMARY.md` (execute report, commit `0330f18e`), `ROADMAP.md` "Stage 1: Foundation — second pass", `REQUIREMENTS.md` v2.0.0
**First pass:** `first-pass-baked/VERIFICATION.md` (baked parameter set, 2026-09-07) — the fork, rename and strip verified there are not re-verified here beyond the regression gates.
**Method:** every SUMMARY claim was re-measured in this session from the committed tree at `0330f18e` — build, param-dump, ID-keyed diff, pluginval ×2, auval, smoke harness, i18n / UI gates, on-disk bank, bundle hashes. Nothing below is copied from SUMMARY without a fresh run.

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / ROADMAP.md Stage 1 second pass)

1. The 48 baked-geometry parameters replaced by the 34 live-oscillator parameters of the locked `parameter-spec.md` v2: **205** parameters in spec order, `osc?Pos` / `osc?Unison` changed in place, choice lists verbatim, `osc?TerFreq` exact-log (FUNC-09 ID parity evidence).
2. Mod-destination list 26 → **46**, indices 1 / 2 relabelled `Osc? Orbit Size`, 26–45 appended all-A then all-B; the voice untouched, a slot routed to a new destination leaves the placeholder output unchanged (D3; FUNC-05 list evidence).
3. State child `geometryImports` → `terrainImports`, still empty; 205-parameter state round-trips into a fresh processor.
4. Editor relay plumbing sized for the v2 UI — 166 slider + **8 combo** + 30 toggle relays — with the page untouched (D1); UI gates at the first-pass baseline (regression only).
5. Factory bank reset to a single self-describing `Init`; the stale first-pass bank removed from the dev machine (D2).
6. **COMPAT-01 re-verified**: pluginval strictness 10 VST3 + AU, auval, version 1.0.0.
7. Docs and registry: CHANGELOG second-pass entry (last free range change), NOTES, PLUGINS.md 🚧 Stage 1, STATUS; one path-scoped commit, no tag, `plugins/O-Prism` untouched.

### Deliverables (from SUMMARY.md)

1. `StrataParamIds.h` (24 slider + 4 combo suffixes per oscillator), `PluginProcessor.cpp` `createOscParameters` (28 pushes, `jassert (params.size() == 28)`), `params.tsv` 205 rows.
2. `dsp/ModulationMatrix.h` `ModDest` + 20, `getModDestNames()` 46, `static_assert (NumDests == 46)`; `StrataVoice.*` no diff.
3. `terrainImports` literals + comments; smoke check [3].
4. `PluginEditor.h/.cpp` `comboRelays` / `comboAttachments` + assert at 8; `Source/ui/**` no diff.
5. `FactoryPresets.cpp` 2354 → 49 lines, `Init` from `getDefaultValue()`; `~/Library/O-Strata/Presets` regenerated.
6. pluginval SUCCESS ×2, auval SUCCEEDED, `build-and-install.sh` dual-variant sweep.
7. CHANGELOG / NOTES / PLUGINS.md / STATUS; commit `0330f18e`.

### Goal Achievement

| Goal | Status | Evidence (re-measured this session) |
|------|--------|----------|
| 1. 205-parameter contract | ✅ Achieved | `O-Strata-param-dump` → 205 rows, **byte-identical** to committed `.planning/params.tsv` (`# params 205`). ID-keyed diff vs O-Prism v1.24.0 (`git show a774d6d4^:plugins/O-Prism/.planning/params.tsv`, 173 rows): removed 2 (`oscATable`, `oscBTable`), added 34, changed in place 20 (`oscAPos oscBPos oscAUnison oscBUnison modSlot0..15Dst`), **151 rows byte-identical**. Spot rows: `oscATerrain` 7 choices `Sine Product … Imported…`; `oscATerFreq` numSteps 2147483647, ends 0.2500000 / 8.0000000, defaultNorm 0.400000; `oscATerEdge` 2 `Mirror / Window`; `oscAOrbit` 11 `Ellipse … Squarcle`; `oscAQuality` 3 `Bandlimited … 4×` default `2×` (norm 0.5); `oscAPos` `Osc A Orbit Size` default 0.500000; `oscAUnison` numSteps 4. Removal grep (`GeoSource … geometryImports`) over `Source/` returns nothing |
| 2. 46 mod destinations, voice untouched | ✅ Achieved | `ModulationMatrix.h:91` `static_assert (NumDests == 46)`; `modSlot0Dst` row numSteps 46, textAtMax `OscB Saturation`; smoke [4]: `getModDestNames().size() == 46`, `[1]/[2]` Orbit Size, `[26]` OscA Orbit Aspect, `[31]` OscA Terrain Freq, `[45]` OscB Saturation; LFO1 → OscA Terrain Freq max\|Δ\| = 0.000e+00 with positive control LFO1 → Pitch max\|Δ\| = 7.126e-01 (40 960 samples). `git diff --stat HEAD~1 HEAD -- Source/StrataVoice.*` empty |
| 3. `terrainImports` + round-trip | ✅ Achieved | smoke [3]: 205 PARAM nodes == live count, 0 mismatches, `<terrainImports/>` present and empty, uiLanguage + Scala tuning restored |
| 4. Relays sized, page untouched | ✅ Achieved | `PluginEditor.cpp:734` slider assert, `:747` combo assert `// 8`; smoke [6]: `allSliderIds() == 166`, `allComboIds() == 8`, all 174 relay IDs resolve. `git diff --stat 0e838512 HEAD -- Source/ui tests/` = only a 4-line comment refresh in `ui_tip_render_check.js` (219 → 205 counts); the page is byte-identical to the first-pass verified page. Gates: see Automated Checks |
| 5. `Init` bank | ✅ Achieved | `~/Library/O-Strata/Presets` holds exactly `Factory/.factory-version` (= `1.0.0`) and `Factory/Init/Init.json`, before and after the smoke re-run; smoke [5] 198 keys == 205 − 7 excluded tuning IDs, `oscAPos` 0.5, `oscAQuality` 0.5, `oscATerFreq` ≈ 0.4, category `Init`, factory true |
| 6. COMPAT-01 | ✅ Achieved | see Requirements Verification |
| 7. Docs / commit hygiene | ✅ Achieved | `CHANGELOG.md:27` "Stage 1, second pass" block (205 params, 46 dests, `Init` bank); commit `0330f18e` is the last commit touching `plugins/O-Strata`; `git status --short plugins/O-Prism` and `git diff --stat HEAD -- plugins/O-Prism` empty; `git tag \| grep -i strata` empty; only untracked file is the harness binary `smoke/strata-smoke` |

## Requirements Verification

**Stage:** 1-foundation (second pass)
**Requirements for this stage:** 1 total (1 must) — COMPAT-01. FUNC-09 and FUNC-05 receive Stage 1 *evidence* only; their formal verification stays at stage-2 per REQUIREMENTS §Traceability.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| COMPAT-01: pluginval strictness 10 VST3 + AU, auval — re-verified after the re-parameterise pass | must | ✅ Complete | pluginval 1.0.4 `--strictness-level 10 --skip-gui-tests`: `VST3-O-Strata-dev-e64a00ba-3d10af48` **SUCCESS** exit 0; `AudioUnit-O-Strata-dev-ec8c5705-61757a77` **SUCCESS** exit 0 (one warning line, the inherited `Current program is -1`). `auval -v aumu OuSt OuDv`: `Component Version: 1.0.0 (0x10000)` … **AU VALIDATION SUCCEEDED** (one inherited warning: multi-channel output without a channel layout — same as the first pass and O-Prism). Installed `-dev` bundles hash-identical to the build artefacts (VST3 `4dfad9be…`, AU `dd7079cf…`) |
| FUNC-09: inherited sections carry over (IDs) | must | ⏸️ Deferred (stage-2) | Stage 1 evidence: 151 inherited rows byte-identical, −2 +34 +20 exactly as CONTEXT D4 |
| FUNC-05: audio-rate mod destinations (list) | must | ⏸️ Deferred (stage-2) | Stage 1 evidence: `modSlot?Dst` lists 46 = O-Prism's 26 (1 / 2 relabelled) + 10 per oscillator; matrix accepts a new destination without affecting the placeholder |
| FUNC-10: tuning engine (smoke) | must | ⏸️ Deferred (stage-2) | Stage 1 evidence: smoke [2] `just-major.scl` loads, 62/60 = 1.25, 64/60 = 1.5, 67/60 = 2.0 |

**Requirements Summary:**
- ✅ Complete: 1 (COMPAT-01)
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 27
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (`ninja O-Strata_VST3 O-Strata_AU O-Strata-param-dump`) | ✅ Pass | `ninja: no work to do` on the committed tree — artefacts current; installed bundles hash-identical |
| pluginval strictness 10 VST3 | ✅ Pass | SUCCESS, exit 0, 0 warnings |
| pluginval strictness 10 AU | ✅ Pass | SUCCESS, exit 0, 1 inherited warning |
| auval `aumu OuSt OuDv` | ✅ Pass | SUCCEEDED, version 1.0.0 |
| param-dump vs `params.tsv` | ✅ Pass | byte-identical, 205 rows |
| ID-keyed diff vs O-Prism v1.24.0 | ✅ Pass | −2 / +34 / 20 in place / 151 identical (CONTEXT D4 figure) |
| Removal grep (baked suffixes, `geometryImports`) | ✅ Pass | no hits in `Source/` |
| Smoke harness `smoke/strata-smoke` (checks [1]–[6]) | ✅ Pass | **62 PASS / 0 FAIL** when run from the repo root (see Issues Found for the cwd trap) |
| Factory bank on disk | ✅ Pass | exactly `Init` + `.factory-version 1.0.0`, unchanged by the harness re-run |
| `check-i18n --plugin O-Strata` | ✅ Pass | ALL CHECKS PASS — 1 localized plugin |
| `i18n-fr-lint --plugin O-Strata --strict` | ✅ Pass | CLEAN — exit 0 |
| `i18n-zh-lint --plugin O-Strata` | ✅ Pass | GATE PASSED — exit 0, 0 findings |
| `check-ui-labels --plugin O-Strata` | ✅ Pass | ALL CHECKS PASSED, exit 0 — no uncaught page error, every resource served (regression only, page byte-identical) |
| `boot-all-uis --plugin O-Strata --strict-tips` | ✅ Pass | text 859 / aria 6 / title 0 / i18n 166; DEAD bindings 0, late bindings 0, no i18n runtime diagnostics, exit 0 — equals the first-pass baseline |
| `tests/ui_tip_render_check.js` | ✅ Pass | ALL CHECKS PASSED (2799 passed, 106 bindings, en / fr / zh-Hans), exit 0 — equals the first-pass baseline |
| O-Prism isolation / no tag / commit scope | ✅ Pass | see Goal 7 |

## Human Verification

Non-blocking; the headless runs cover the substance. Left for the next time the Standalone or Logic is open:

- [ ] Logic: the automation list shows the 34 new `Osc A/B …` parameters and `Osc A Orbit Size` (was Position); a held note still sounds the sine placeholder on A and B
- [ ] Logic: the preset browser shows only `Init`
- [ ] Standalone (`ninja O-Strata_Standalone` first — `build-and-install.sh` leaves it stale): the Synth-tab "Position" knob reads 0.5 at Init; the mod-matrix destination dropdown lists 46 entries from C++

## Issues Found

- **Smoke harness is cwd-sensitive** (harness, not plugin): `main.cpp:116` resolves `test-tunings/just-major.scl` from the current working directory. Run from `smoke/` it reports 6 failures ([2] ×4, [3] ×2 — the Scala load never happens and the "restored tuning is Scala" check fails as a consequence); run from the repo root it passes 62/0. Not fixed here (harness file, verify phase is read-only on code). Carry to Stage 2 when the harness is reseeded as the render harness: resolve the fixture relative to the source file (`__FILE__`) or the executable, and print the resolved path on failure.
- **RESEARCH 2.6 correction stands:** the placeholder render is not deterministic across instances at `osc?Phase == 0` (random start phase seeded from the oscillator address, memory `pattern_random_start_phase_seeded_from_this_breaks_render_diff`); the harness pins `osc?Phase = 0.25`. Every Stage 2 sample-identical gate needs the same pin or a seedable phase in `TerrainOscillator`.
- None in the plugin.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes — Stage 2 (DSP), Phase 2.1 `TerrainOscillator` replaces the placeholder in place (`/plugin-discuss O-Strata 2-dsp`)

**Blockers:** none

**Carry-forward for Stage 2 (from SUMMARY "Notes for later stages" + this pass):** wire `ModDest` 26–45 in the voice (26 OscA Orbit Aspect … 35 OscA Saturation, 36–45 the OscB ten); delete `WavetableOscillator` / `WavetableData` / the placeholder and the `resetWithRandomPhases` address seed with them; give `TerrainOscillator` a seedable phase; the 20 new destinations are currently accumulated into `destOffsets` cells nobody reads (D3) — Phase 2.1 reads them; `osc?TerFreq` is an exact-log lambda range (`ValueRemapFunction (start, end, v)`), so any smoothing or modulation of it must work in the normalised or log domain, not on the 0.25–8 value with a linear offset; the smoke harness is the seed for the Stage 2 render harness (fix the cwd trap when reseeding). Phase 3.1 owns the page re-fork from O-Prism v1.26.0, the 8 combo bindings and the `data-i18n` keys (CONTEXT D1). Phase 4.1 replaces `Init` with the real bank and must delete orphans itself (the preset manager never does).
