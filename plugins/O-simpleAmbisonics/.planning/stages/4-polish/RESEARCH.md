# Stage 4: Polish — Research

**Date:** 2026-10-14 · **Inputs:** `CONTEXT.md` (this folder), three parallel investigations (preset-manager integration; UI fit + nits; CI / release / critic / validation). Sibling model: **O-simpleWavetable Stage 4** (not O-simpleFM — older pattern, sets `outputLevel`, writes the real Factory folder from the harness).

## 0. Decisions taken during research (user, 2026-10-14)

| # | Decision | Choice |
|---|----------|--------|
| R-D1 | Preset bar placement | **Retire `#chipLatency`**; FM-style one-row bar in the header strip (◀ name ▶ Save Delete) |
| R-D2 | Lesson semantics | **Full reset, as today** — a lesson = loading its factory preset (defaults + recipe). Supersedes CONTEXT's "partial delta" wording. ST6 stays valid |
| R-D3 | `inputMode` | **User-owned** like `outputLevel`: never reset, set, or counted in "modified" by lessons / presets / user JSON |
| R-D4 | "Order 1 → 2 → 3" lesson | **Steps 1→2→3→1 on repeated clicks**; the factory preset of the same name sets order 1 |

R-D4 detail for the planner: the step continues only while the scene is still the unmodified `orderStep` scene (current preset id == `orderStep` and no other param moved since); otherwise a click starts at order 1. Each step is still a full reset + recipe with the next order index, so the order-3 headroom rule holds (distance resets to 1.0 m).

## 1. Preset manager — integration recipe

**Module:** `modules/persistence/preset-manager/` **v1.0.9** (`module.yaml:6`; registry natives list is incomplete — trust module.yaml). Hook: `ouaricon_add_module(O-simpleAmbisonics preset-manager)` → adds `cpp/` include + copies `preset-manager.js` to `Source/ui/public/modules/` (gitignored, `.gitignore:26`).

**Use the module narrowly (Wavetable pattern, `O-simpleWavetable/.planning/stages/4-polish/PLAN.md:36-44`):** folders, `savePreset` (factory-name guard), `deletePreset`, `initializeFactoryPresets` + `.factory-version` sentinel, the JS. **Never** call `setStateFromXml`/`getStateAsXml` (stale `<CustomState>` child), `applyPresetJson` (resets + sets `outputLevel`, `:358-374`), `savePresetToFile` (no factory guard), module prev/next. Static grep gate (Wavetable S10) keeps those at 0.

**Storage — in code, no BinaryData XML.** New `Source/PresetRecipes.h` (namespace `ambpresets`): stable `id`, display key, raw engineering-unit entries. Bank = **Init + 8 lessons** in table order; Init = one-entry recipe equal to defaults. `kLessons` (`PluginProcessor.cpp:431-440`) moves here; lesson buttons and factory bank read the same rows. Disk copies exist only so the module can list/guard them; factory loads read the table, never disk.

**One apply core** (`applyNormalisedTargets`, Wavetable `PluginProcessor.cpp:1332`): for every param except `outputLevel` and `inputMode` → target = recipe value (via that param's own `convertTo0to1` — skew-correct for `distance` 0.2–20 c2, `orbitRate` 0.02–2 c0.2; choices by index) else default; clamp, snap, skip within 1e-6, begin/set/end per change. Lesson buttons, factory load and user load all route here.

**User JSON:** strict own parser (numbers + bools only, unknown ids ignored, `outputLevel`/`inputMode` ignored even if present). Save via module `savePreset(name)`.

**Lazy factory write:** `ensureFactoryBankOnDisk()` on the first preset native only — never constructor / harness (harness stamps the real version, `render-harness/CMakeLists.txt:81` → would write the user's real Factory folder). auval/pluginval stay I/O-free.

**State:** `currentPreset` root property in our own `getStateInformation`; restore with the `isVoid()` + string read used for `uiLanguage` (`:410`); absent → `""`. Preset name is not an APVTS value → revision-counter `presetState` event (name, id, factory, modified) pushed from the editor timer (one-shot push goes stale on lesson click / session restore).

**Natives** (registry comment `PluginEditor.cpp:181` grows 5 → ~16): the 10 module names (`savePreset`, `savePresetWithDialog`*, `loadPreset`, `loadPresetFromFile`*, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`) + `getPresetCatalog`; * = refusal stubs. prev/next return the neighbour name without loading. Keep `applyFactoryPreset` for lesson buttons. JS: add `Source/ui/public/modules/preset-manager.js` to the **existing** `O-simpleAmbisonics_UIResources` target (symbol `presetmanager_js`) + `/modules/preset-manager.js` branch in `getResource`; lazy `import()`. Harness include path gets `modules/persistence/preset-manager/cpp`.

**v1.0.6 migration hook:** not needed at 1.0.0 (note for future range changes).

## 2. UI

### 2a. Preset bar (R-D1)
- Frame 1180×780, content 1142×750, vertical slack only 14 px → no new row. Lesson row ruled out (fr slack 74 px, floor 8, `tests/ui-probes/lesson-row-budget.mjs:64`).
- Header: `.title-block` 425 pinned, `.header-strip` ≈ 655 px, chips 442 (`#chipFormat` 236 + `#chipLatency` 196). Retiring `#chipLatency` → ≈ 419 px free.
- Bar per O-simpleFM (`index.html:41-62`, `css/styles.css:791-830`): ◀ 22, name button (min 116, opens list), ▶ 22, Save, Delete, gap 5; Save/Delete min-width pinned at fr width (measure "ENREG." / "SUPPR."-class captions). Markup + CSS hand-written, localized; do **not** use module `createPresetBar()` (unlocalized innerHTML).
- Knock-ons of retiring the chip: delete the TIP_BINDINGS entry (`js/i18n.js:732`); move the latency tooltip copy to `#decWrap` / decoder line (`label.decLineBin` already says "64-sample latency", :830); remove keys `chipLatency`, `label.latencyBody`, `label.latencyDetail` (else check-i18n orphans). New anchors → TIP_BINDINGS as static markup (strict-tips). Add a preset anchor to `tooltip-clamp-probe.mjs` (header right edge).
- Factory option labels via i18n keys (Wavetable `FACTORY_LABELERS` map), ids stable.

### 2b. `label.encReadN` (`js/i18n.js:663`)
- Box: `.enc-read` 144 px (166 − 20 pad − 2 border), nowrap + ellipsis; en overflows 8.0 px (order 3), 4.4 px (order 2).
- New copy: en `({n}+1)² = {ch} ch` · fr `({n}+1)² = {ch} can.` · zh `({n}+1)² = {ch} 声道` (matches `label.chipOrder` / `stEncoder` / `bandCh`).
- **Gate blind spot:** check-ui-labels [4] skips non-replaced inline boxes (`scripts/check-ui-labels.js:798-810`) → add an explicit assertion: `#encReadN` parent `scrollWidth <= clientWidth` at order 3 in en/fr/zh (state exists in `tests/i18n-states.json`), with a negative control (old copy must fail).

### 2c. Chevron / puck (`drawTop()` `index.html:1929`)
- Cause: chevron = `azToScreen(heading, R−40≈96)`; puck = `azToScreen(az, r(d))`, r ≈ 96 at d ≈ 4.4 m → same ray when az ≈ yaw; collide d ≈ 3–6 m within ±9°; elevation halo (8+10·alt) + brass ▲ glyph push outward; puck drawn after chevron covers it; amber vs brass ▲ ambiguous.
- Fix: (1) draw chevron after `drawPuck` (:2043) with re-applied `translate`/`rotate(-heading)` + 3 px `--well-bot` backing stroke; (2) proximity-aware tangential slide away from the puck when rose-frame distance < 7 + (8+10·alt) + 6.
- Rejected: outside rim (collides with sage rim tick at `azRot`, default front case); radius change (D10 already moved R−30 → R−40 to stop the "Y").
- Gate: new frame-probe **A14** via `__probe.setParams` (az = yaw = 20, pitch 40, el 30, d ≈ 4.4) — ≥ 4 px gap amber vs brass, chevron present; exclude the beam's low-alpha amber lobe. `--negative-control chevron` (old draw) must FAIL. Do **not** add a fixture (harness PL4 pins fixture count at 15).

## 3. Critic pass
- First real review for this plugin (Stage 2 gate's dsp-critic was a smoke stub, `2-dsp/gate-report.json:8,18`; Stage 3 had none).
- `critic-orchestrator` (`.claude/agents/critic-orchestrator.md`) → critic-dsp / -ui / -architecture / -foundation, merged by `.claude/hooks/merge-critic-reports.py`. At Stage 4 run **all critics** (earlier-stage files will change).
- Triage per Wavetable: fix / accept-by-design / research-decides; each DSP fix gets a gate + negative control; everything logged in **`plugins/O-simpleAmbisonics/CODE_REVIEW.md`** (canonical path; WR-xx / IN-xx + "Upstream" section; `status: resolved`, `open_findings: none` at close).
- Run the critic **before** the preset work lands is cheaper (smaller diff) — recommend Part 1 opens with it.

## 4. Carried test fix
- `tests/pedalboard/verify_2_1.py:79` `for dec in ("Stereo", "Binaural")` → Stereo only (25 → 22 checks); header note (`:1-5`) points Binaural onset to `verify_2_3.py` V2 (`:157-172`: onset = 100 + `kBinauralOnsetFrontSamples[order]` = 184 / 150, same 64-sample latency) + V1 (`:140-155`). Also relabel V1 "Binaural placeholder" (`:47,50`).

## 5. Windows CI (COMPAT-02)
- `.github/workflows/build-and-release.yml:44-59` `workflow_dispatch` (`plugin_name`, `version`, `validate_only=true`): builds only the target VST3 on Windows + pluginval s10 (`:614-620`), uploads `pluginval-windows-log`, skips release (`:703-706`); macOS job also runs (sign + notarize).
- **Needs a push first** (dispatch checks out origin `main`). `origin/main..main` = 45 commits (35 O-simpleAmbisonics, 10 O-Octagon); origin has **0** O-simpleAmbisonics files → the push publishes the whole plugin source (public AGPL repo) and fires `ui-static-gates.yml`. **Blocking go-ahead checkpoint** (Wavetable Task 22): show `git log origin/main..main`, side effects, then on explicit yes: `git push origin main` → `gh workflow run build-and-release.yml --ref main -f plugin_name=O-simpleAmbisonics -f version=1.0.0-validate -f validate_only=true` → `gh run watch <id> --exit-status` (background) → `gh run download <id> -n pluginval-windows-log`. Pass = 0 FAILED + job success (log has no SUCCESS line). Re-dispatch needs a fresh yes.
- Risk scan: WebView2 flags + `withUserDataFolder` present (`CMakeLists.txt:29-30,117`; `PluginEditor.cpp:255-260`); no M_PI, no C3493/SafePointer traps; `BinauralFilterData.h` 277 KB of `inline constexpr float` (≈ 58 KB data, 4 short literals) — low risk, only MSVC compile time unproven; manual C3615 look at `ShRotator.h:105,111`, `ShEncoder.h:141`. Re-grep MSVC traps after preset code lands.

## 6. Release docs
- `CMakeLists.txt:24` already `VERSION "1.0.0"`.
- **CHANGELOG.md** (new): exactly one `## [1.0.0] - YYYY-MM-DD`, `###` subsections only. Checks: awk extract `> 0` lines; `grep -cE '^## \['` = 1; `grep -cE '^## v?[0-9]|^## Version'` = 0.
- **CODE_REVIEW.md** at plugin root (from §3).
- **PLUGINS.md** line 69 → `📦 Installed | 1.0.0` (model: O-simpleWavetable row 63). Working tree has other sessions' unstaged PLUGINS.md edits → build the blob from `git show HEAD:PLUGINS.md` with only this row swapped, `git update-index --cacheinfo` (Wavetable PLAN.md:265-269).
- REQUIREMENTS.md statuses (FUNC-08/09, COMPAT-02, QUAL-01/02), ROADMAP/ARCHITECTURE amendments (chip retired, lesson stepping, inputMode ownership).
- **No tag** (future `/publish` tag `O-simpleAmbisonics-v1.0.0`).

## 7. Gates to add (render-harness ST7+, ported from Wavetable `tests/viz-check/main.cpp:676-3467`)

| Gate | Asserts | Negative control |
|------|---------|------------------|
| G-LESSON | lesson = defaults + recipe; `outputLevel` + `inputMode` untouched; orderStep walks 1→2→3→1 only while unmodified, restarts at 1 otherwise | naive reset loop incl. `outputLevel`/`inputMode` |
| G-FACTORY [table/init/apply/defs] | bank = Init + 8, ids stable, defs normalised via each param's `convertTo0to1` | skew NC on `distance` / `orbitRate` (linear normalise must fail) |
| G-ORDER3-DIST | no recipe (and no orderStep step) yields order index 2 with distance < 1 m | injected bad row |
| G-PRESET-USER [roundtrip/malformed/missing/clamp/refuse] | strict parser, factory-name save refused | JSON carrying `outputLevel`/`inputMode` must not apply |
| G-PRESET-NAME / WALK / STATE | prev/next return neighbours without loading; `currentPreset` survives two reopens, absent → `""` | — |
| S10 grep | 0 calls to `setStateFromXml`, `getStateAsXml`, `applyPresetJson`, `savePresetToFile`, module prev/next | — |
| R-BRIDGE | native count == registry list; every page `getNativeFunction` name registered | — |
| encReadN fit | parent `scrollWidth <= clientWidth`, order 3, en/fr/zh | old copy fails |
| frame-probe A14 | chevron/puck ≥ 4 px apart, chevron present | `--negative-control chevron` |

Extend `verify_3_state.py` for `currentPreset`; ST6 stays (R-D2).

## 8. Validation sweep (exact commands, from repo root)

```
ninja -C build O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics_Standalone O-simpleAmbisonics-render-test   # 0 "warning:"
build/plugins/O-simpleAmbisonics/tests/render-harness/O-simpleAmbisonics-render-test_artefacts/Release/O-simpleAmbisonics-render-test   # ×2; digests 2.1 9189d911a1fe98d5 / 2.2 7f35bc9f91e321b9 PASS, 2.3 edd1acfd21f54bcd reproduced; logs identical except PERF-02
./scripts/build-and-install.sh O-simpleAmbisonics && ninja -C build O-simpleAmbisonics_Standalone   # installed cmp-identical
auval -v aufx OSAm OuDv
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-simpleAmbisonics-dev.vst3        # ×2
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/Components/O-simpleAmbisonics-dev.component  # ×2
for v in 2_1 2_2 2_3; do uv run --no-project --python 3.12 --with pedalboard --with numpy --with scipy python plugins/O-simpleAmbisonics/tests/pedalboard/verify_$v.py; done
uv run --no-project --python 3.12 --with pedalboard python plugins/O-simpleAmbisonics/tests/pedalboard/verify_3_state.py
node scripts/check-i18n.js --plugin O-simpleAmbisonics
node scripts/i18n-fr-lint.js --plugin O-simpleAmbisonics
node scripts/i18n-zh-lint.js --plugin O-simpleAmbisonics
node scripts/check-ui-labels.js --plugin O-simpleAmbisonics          # banner seed=overrides
node scripts/boot-all-uis.js --plugin O-simpleAmbisonics --strict-tips
node plugins/O-simpleAmbisonics/tests/ui-probes/frame-probe.mjs      # 14/14 after A14 + NCs fuma/pole/idle/chevron FAIL
node plugins/O-simpleAmbisonics/tests/ui-probes/lesson-row-budget.mjs
node plugins/O-simpleAmbisonics/tests/ui-probes/tooltip-clamp-probe.mjs
node plugins/O-simpleAmbisonics/scripts/generate_sh_icons.mjs --check
```
Exit 77 = Playwright missing = not a pass. Suite-wide fr-lint exits 2 on main (O-DigiDelay) — compare to baseline.

## 9. Human gates (Taylor)
COMPAT-02 Logic (mono + stereo tracks, no I/O config) + Ableton; carried Logic R1–R3 + general list, Standalone list (each lesson moves exactly its recipe, never Output/inputMode; orderStep walk; Overhead order 3 no clip), D16 reflow eye-check, R9 report; preset-bar visual checkpoint; QUAL-02 headphone listening (front/back/left/right/above at order 3, +90° louder left); FUNC-09 orbit-in-a-mix sanity.

## 10. Suggested cadence
- **Part 1:** critic pass + triage → carried fixes (verify_2_1, encReadN, chevron) → PresetRecipes + apply core (R-D2/3/4) → preset-manager + natives + header bar (chip retired) → gates → build/install → **preset-bar visual checkpoint**.
- **Part 2:** full validation sweep → Windows CI (**go-ahead checkpoint for push + dispatch**) → CHANGELOG / CODE_REVIEW / PLUGINS / REQUIREMENTS → human hands-on + QUAL-02 sign-off.

## Risks
- Retiring the chip touches header layout → re-measure `.title-block` pin and chip body at order 3 in fr/zh.
- orderStep "unmodified" detection must ignore `outputLevel`/`inputMode` moves (consistent with R-D3).
- The push publishes the plugin publicly and carries 10 unrelated O-Octagon commits — user must accept both.
