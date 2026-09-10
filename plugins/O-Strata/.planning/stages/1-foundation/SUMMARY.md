# Stage 1 — Foundation, second pass (re-parameterise the verified fork): SUMMARY

**Plugin:** O-Strata · **Stage:** 1 of 4 (Foundation + Shell, **second pass**) · **Phase:** execute ✓
**Date:** 2026-09-10 · **Mode:** manual · **Branch:** `main` (path-scoped commit under `plugins/O-Strata` + `PLUGINS.md`)
**Inputs:** `CONTEXT.md` (D1–D4), `RESEARCH.md` (§2.1–2.7), `PLAN.md` (14 tasks / 5 waves, Decisions 1–12), `parameter-spec.md` v2 (locked).
**First pass:** `first-pass-baked/` (fork + rename + strip, verified 2026-09-07). `smoke/` was updated, not replaced.
**Template:** summary-complex (14 tasks, 18 files, harness work).

---

## Outcome

The verified O-Strata binary (`OuSt`, `VERSION 1.0.0`, sine wavetable placeholder) now exposes the **205** parameters of `parameter-spec.md` v2: the 48 baked-geometry parameters are gone, the 34 live-oscillator parameters are in (17 per oscillator, spec rows 12–28, in spec order after `WarpAmt`), `osc?Pos` / `osc?Unison` changed in place, the mod-destination list is **46**, the state child is `terrainImports`, the editor carries 166 slider + **8 combo** + 30 toggle relays, and the factory bank is a single self-describing `Init`. **COMPAT-01 re-verified**: pluginval strictness 10 SUCCESS on VST3 and AU, `auval -v aumu OuSt OuDv` SUCCEEDED. The page (`Source/ui/**`) and the voice (`StrataVoice.*`) are untouched (D1, D3). Both oscillators still sound the sine placeholder.

Every number below was measured in this session (memory `pattern_review_recomputes_instead_of_measuring`).

## Tasks executed (PLAN.md 1–14)

| # | Task | Result |
|---|---|---|
| 1 | `StrataParamIds.h` | `oscIds()` = 24 suffixes (rows 1–11 + 13 floats); new `oscComboIds()` (4) / `allComboIds()` (8); `allSliderIds()` comments 24 / 24 / 166 |
| 2 | `PluginProcessor.cpp` `createOscParameters` | `Pos` → `Osc ? Orbit Size`, default 0.5; `Unison` 1–4; baked block deleted; 17 pushes added (4 `choice` + 12 `knob` + explicit `TerFreq` with the 3-arg exact-log lambdas); `jassert (params.size() == 28)`; layout comments `// 28`. Shadow re-grep: none of the 17 suffixes collides with a `juce::` free function |
| 3 | `dsp/ModulationMatrix.h` | `ModDest` + 20 (A block then B block); `getModDestNames()` 46 with `[1]/[2]` = `OscA/OscB Orbit Size`; `static_assert (NumDests == 46)` |
| 4 | `terrainImports` | both literals + both comments renamed; `grep -rn geometryImports Source/` empty |
| 5 | `PluginEditor.h/.cpp` | `comboRelays` / `comboAttachments` (declared relays → webView → attachments); Step 1 / 2 / 3 loops; `jassert (comboAttachments.size() == comboRelays.size())`; 31 native functions unchanged; `Source/ui/**` no diff |
| 6 | `FactoryPresets.cpp/.h` | 2354 → 49 lines; single `Init` built from `getDefaultValue()` per `RangedAudioParameter`; no `makePreset` / `DST_*` / `SRC_*` / `completeBase` |
| 7 | Build | `cmake … -DOUARICON_BUILD_TESTS=ON` configure OK; `ninja O-Strata-param-dump` OK; `ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone` exit 0, no O-Strata warnings (the only CMake warnings are the pre-existing `JUCE_BUNDLE_ID contains spaces` notices on other plugins) |
| 8 | `params.tsv` + diff | header `# params 205`; see **FUNC-09 evidence** below |
| 9 | Stale bank + smoke | `rm -rf ~/Library/O-Strata/Presets` (192 JSON, `User/` absent); harness rebuilt and run → **ALL SMOKE CHECKS PASSED — 0 failure(s)**, 62 PASS lines, checks [1]–[6]; bank on disk = exactly `Factory/.factory-version` + `Factory/Init/Init.json` |
| 10 | Install + validate | `build-and-install.sh O-Strata` (dual-variant sweep); pluginval 1.0.4 strictness 10 **SUCCESS** ×2; auval **SUCCEEDED**; bank still the same two files afterwards |
| 11 | UI gates | regression only — all at the first-pass baseline (table below) |
| 12 | Docs / comments / registry | CHANGELOG second-pass block; NOTES timeline + Known Issues + Type; comment-only refresh in `CMakeLists.txt`, `tests/ui_tip_render_check.js`, `PluginEditor.cpp`; banner subtitle → "Microtonal Wave-Terrain Synthesizer" in `PluginProcessor.h/.cpp`, `PluginEditor.h/.cpp` (voice files left alone); PLUGINS.md row → 🚧 Stage 1 (dup check empty); STATUS.md execute ✓ |
| 13 | SUMMARY.md | this file |
| 14 | Commit | `feat(O-Strata): Stage 1 second pass — 205 live-oscillator parameters, 46 mod destinations, COMPAT-01 re-verified`, path-scoped, no tag |

## FUNC-09 evidence — `params.tsv` ID-keyed diff vs O-Prism v1.24.0 (`git show a774d6d4^:plugins/O-Prism/.planning/params.tsv`)

```
removed: oscATable oscBTable
added:   34
changed in place: 20
```

Changed IDs (20): `oscAPos oscBPos oscAUnison oscBUnison modSlot0Dst … modSlot15Dst`. Field deltas:

| Row | v1.24.0 | now |
|---|---|---|
| `osc?Pos` | `Osc ? Position`, defaultNorm 0.000000, defaultText 0.000 | `Osc ? Orbit Size`, defaultNorm **0.500000**, defaultText 0.500 |
| `osc?Unison` | numSteps 8, textAtMax 8 | numSteps **4**, textAtMax 4 |
| `modSlot?Dst` ×16 | numSteps 26, textAtMax `OscB Warp` | numSteps **46**, textAtMax `OscB Saturation` |

Every other inherited row is byte-identical. The 17 new `oscA*` rows sit at file rows 16–32, between `oscAWarpAmt` (15) and `oscBPos` (33), in spec order: `Terrain, TerFreq, TerModX, TerModY, TerTrack, TerSat, TerBlur, TerEdge, Orbit, OrbAspect, OrbRot, OrbCX, OrbCY, OrbMod, OrbFeedback, OrbFbDamp, Quality`.

Spot rows:
```
oscATerrain   Osc A Terrain        7           Sine Product  Imported…        0.000000  Sine Product  automatable,discrete
oscATerFreq   Osc A Terrain Freq   2147483647  0.2500000     8.0000000        0.400000  1.0000000     automatable
oscATerEdge   Osc A Terrain Edge   2           Mirror        Window           0.000000  Mirror        automatable,discrete
oscAOrbit     Osc A Orbit          11          Ellipse       Squarcle         0.000000  Ellipse       automatable,discrete
oscAQuality   Osc A Quality        3           Bandlimited   4×               0.500000  2×            automatable,discrete
modSlot0Dst   Mod 1 Dest           46          None          OscB Saturation  0.000000  None          automatable,discrete
```

**Host names (Decision 8, not contractual):** `Osc ? ` + `Terrain, Terrain Freq, Terrain Mod X, Terrain Mod Y, Pitch Track, Saturation, Terrain Blur, Terrain Edge, Orbit, Orbit Aspect, Orbit Rotation, Orbit Centre X, Orbit Centre Y, Orbit Mod, Feedback, Feedback Damp, Quality`; `osc?Pos` → `Osc ? Orbit Size`.
**Steps (Decision 9):** 0.001 on every new float except `OrbRot` 0.1 and `TerFreq` 0 (continuous, exact-log; text prints 7 decimals — Decision 2).

## COMPAT-01 evidence

| Check | Result |
|---|---|
| `pluginval --strictness-level 10 --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/O-Strata-dev.vst3` | `Testing plugin: VST3-O-Strata-dev-e64a00ba-3d10af48` → **SUCCESS** |
| `pluginval --strictness-level 10 --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/Components/O-Strata-dev.component` | `Testing plugin: AudioUnit-O-Strata-dev-ec8c5705-61757a77` → **SUCCESS** (same `Current program is -1` warning as the first pass) |
| `auval -v aumu OuSt OuDv` | `Component Version: 1.0.0 (0x10000)` … **AU VALIDATION SUCCEEDED.** |
| `auval -a \| grep -i strata` (cold rescan, background) | `aumu OuSt OuDv  -  Ouaricon Audio Development: O-Strata-dev` |
| pluginval 1.0.4 on the lambda-range `osc?TerFreq` (RESEARCH 2.4 `[ASSUMPTION]`) | no finding — the assumption holds |

## Smoke harness (`smoke/smoke-output.log`, 62 PASS / 0 FAIL)

| Check | Evidence |
|---|---|
| [1] notes sound | rms A=0.1799, A(muted)=0.0000, B=0.1798, B(muted)=0.0000; no NaN |
| [2] Scala | `just-major.scl` loads; 62/60 = 1.25, 64/60 = 1.5, 67/60 = 2.0 |
| [3] round-trip | 205 PARAM nodes == live `getParameters().size()`; 0 mismatches; 182 of 205 differ from default; `<terrainImports/>` present and empty; uiLanguage + Scala tuning restored |
| [4] D3 route (FUNC-05) | `getModDestNames().size() == 46 == ModDest::NumDests`; `[1]/[2]` Orbit Size, `[26]` OscA Orbit Aspect, `[31]` OscA Terrain Freq, `[45]` OscB Saturation; `modSlot0Dst` exposes the same 46; two unrouted renders identical; **LFO1 → OscA Terrain Freq max\|Δ\| = 0.000e+00**; positive control **LFO1 → Pitch max\|Δ\| = 7.126e-01** (40 960 samples) |
| [5] D2 bank | `~/Library/O-Strata/Presets`: json=1, files=2 (`Factory/Init/Init.json`, `Factory/.factory-version` = 1.0.0); `Init.json` 198 keys == live 205 − 7 excluded; none of the 7 tuning IDs; `oscAPos` 0.5, `oscAUnison` 0.0, `oscAQuality` 0.5, `oscATerFreq` ≈ 0.4; category `Init`, plugin `O-Strata`, factory true, version 1.0.0 (the preset **name is the file name** — the JSON has no `name` key) |
| [6] lists / range | Terrain 7 (`Imported…` last), TerEdge 2, Orbit 11 (`Limaçon` at [2]), Quality 3 default index 1 (`2×`); `osc?TerFreq` start 0.25 / end 8.0 / `getDefaultValue()` 0.400000 / `getNumSteps()` 2147483647 / norm 0 → 0.25, 0.4 → 1.0, 1 → 8.0; `allSliderIds() == 166`, `allComboIds() == 8`, all 174 relay IDs resolve |

**Harness finding (new):** the placeholder render is **not** deterministic across instances at the defaults — `StrataVoice` calls `resetWithRandomPhases()` when `osc?Phase == 0`, and that seeds from the oscillator's address. Check [4] pins `oscAPhase = oscBPhase = 0.25` on every instance; with that, two unrouted renders are sample-identical and the D3 comparison is meaningful. RESEARCH 2.6's "the placeholder path is deterministic" was wrong as stated. Stage 2 harness gates that compare renders sample-for-sample need the same pin (or a seeded phase).

## UI gates (regression only — page byte-identical, `git diff --stat HEAD -- Source/ui tests/i18n-states.json tests/ui-stub` empty)

| Gate | First-pass baseline | This run |
|---|---|---|
| `check-i18n --plugin O-Strata` | ALL CHECKS PASS | **ALL CHECKS PASS — 1 localized plugin(s)** |
| `i18n-fr-lint --plugin O-Strata --strict` | CLEAN exit 0 | **CLEAN — exit 0** |
| `i18n-zh-lint --plugin O-Strata` | GATE PASSED exit 0 | **GATE PASSED — exit 0. 0 findings** |
| `check-ui-labels --plugin O-Strata` (20 states) | ALL CHECKS PASSED | **ALL CHECKS PASSED** (no page error, every resource served) |
| `boot-all-uis --plugin O-Strata --strict-tips` | 0 DEAD / 0 late | **DEAD bindings: 0, late bindings: 0**, no page errors, no i18n runtime diagnostics (text 859, aria 6, title 0, i18n 166) |
| `tests/ui_tip_render_check.js` | ALL CHECKS PASSED (2799, 106 bindings) | **ALL CHECKS PASSED (2799 passed)** |

Executor note: running these from a zsh `for` loop with the flags inside the loop variable hands Node a file name with the flag glued on (`MODULE_NOT_FOUND`) — zsh does not word-split (memory `pattern_zsh_no_word_split_backup_loop_strays`). The two affected gates were re-run with explicit arguments.

## RESEARCH corrections, now facts

1. Preset folder is `~/Library/O-Strata/Presets/`; the initializer never deletes — the 192-file stale bank was removed by hand and the harness's first construction wrote exactly `Init`.
2. The fork's `OuariconPresetManager.h` has no v1.0.6 migration hook; nothing was preserved or needed.
3. `ValueRemapFunction` is `(start, end, v)`; the continuous-range text columns print 7 decimals (`0.2500000 / 8.0000000 / 1.0000000`).
4. The working-tree O-Prism `params.tsv` is the v1.25.0 registry; the diff was taken against `a774d6d4^` so "−2 (`osc?Table`)" is literal.

## Files

**Modified (18):** `Source/StrataParamIds.h`, `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h` (banner), `Source/dsp/ModulationMatrix.h`, `Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/FactoryPresets.cpp`, `Source/FactoryPresets.h`, `CMakeLists.txt` (comment), `tests/ui_tip_render_check.js` (comment), `CHANGELOG.md`, `NOTES.md`, `.planning/params.tsv` (219 → 205 rows), `.planning/STATUS.md`, `.planning/stages/1-foundation/smoke/main.cpp`, `…/smoke/smoke-output.log`, `.planning/stages/0-ideation/gate-report.json` (gate run), `PLUGINS.md`
**Created:** this file
**Deleted on disk (dev machine only):** `~/Library/O-Strata/Presets/**` (192 stale JSON) → regenerated as `Factory/Init/Init.json` + `.factory-version`
**Not touched:** `Source/ui/public/**`, `Source/StrataVoice.*`, `Source/StrataSound.h`, `Source/dsp/WavetableOscillator.*`, `tests/i18n-states.json`, `tests/ui-stub/**`, `parameter-spec.md`, `ROADMAP.md`, `research/ARCHITECTURE.md`, `BRIEF.md`, `REQUIREMENTS.md`, `plugins/O-Prism/**`, root `CMakeLists.txt`
**Untracked, not committed:** `smoke/strata-smoke` (harness binary; `main.o` is gitignored via `*.o`)

## Deviations from PLAN.md

- Task 12 banner subtitle: applied to `PluginProcessor.h/.cpp` and `PluginEditor.h/.cpp` only; the same comment in `StrataVoice.h/.cpp` and `StrataSound.h` was reverted so the success criterion "`StrataVoice.cpp` has no diff" stays literal.
- Smoke [4] gained a determinism pre-check and the `osc?Phase` pin (finding above); smoke [5] checks `category` + file name instead of a non-existent `name` key.
- `grep -c "comboRelays\|comboAttachments" PluginEditor.cpp` = 5 lines, not the plan's "≥ 6" (the count was a guess; all five sites — relay loop, options loop, attachment loop ×2, assert — are present).
- The gate script (`run-gate.sh 0-ideation → 1-foundation`) **PASSED** without `--force` this time (memory `pattern_gate_0_to_1_always_needs_force` did not apply — the Stage 0 artifacts now satisfy the schema).

## Notes for later stages

- **Phase 2.1:** wire `ModDest` 26–45 in the voice (indices per spec §Mod-matrix destinations: 26 OscA Orbit Aspect … 35 OscA Saturation, 36–45 the OscB ten); delete `WavetableOscillator` / `WavetableData` / the placeholder; the random-phase seed (`resetWithRandomPhases` from `this`) goes with them — give `TerrainOscillator` a seedable phase so harness gates can pin it.
- **Phase 3.1:** re-fork the page from O-Prism v1.26.0 and splice `mockups/v2-ui.html`; bind the 8 combo relays (`bindCombo()` in `v2-ui.html`); the ROADMAP Stage 1 criteria "every new knob moves its parameter" and the `data-i18n` keys (en / fr / zh-Hans) live there (CONTEXT D1). The page's 26-entry `destNames` fallback is replaced by the 46-entry list.
- **Phase 4.1:** replace `Init` with the real bank under a new `.factory-version` stamp **and delete orphans itself** — the preset manager never does (RESEARCH 2.1). If any range or list changes after v1.0.0, port the v1.0.6 migration hook from O-Prism `PluginProcessor.cpp:525-547`.
- **Verify phase:** COMPAT-01 evidence is above; FUNC-09 / FUNC-05 Stage 1 evidence is the diff + smoke [4]; formal verification of both is Stage 2.
