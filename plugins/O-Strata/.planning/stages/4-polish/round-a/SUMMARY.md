# Stage 4 — Polish / Validation, Round A (Phase 4.1): SUMMARY

**Plugin:** O-Strata · **Stage:** 4 of 4 · **Round:** A = ROADMAP Phase 4.1 (CONTEXT D1) · **Phase:** execute ✓
**Date:** 2026-09-12 · **Mode:** manual · **Branch:** `main` (trunk-based, path-scoped commits)
**Plan:** `stages/4-polish/PLAN.md` (12 tasks / 4 waves / 3 commits, Decisions 1–26) · **Research:** `stages/4-polish/RESEARCH.md`
**Baseline:** `9e5f6f85` (plan) / code `5b34b5cf` (Stage 3 verdict: `--gate all` 139 / 139, labels 1767 / 0 over default + 41, tip gate 3163 at 120, 33 natives)
**Commits:** `f2fe26ed` preset-manager v1.0.7 (`modules/persistence/preset-manager`, `modules/registry.yaml`) · `1a1ff54e` Phase 4.1a (`plugins/O-Strata`) · **4.1 = the commit carrying this file** (`plugins/O-Strata`, `PLUGINS.md`)

---

## Outcome

Round A delivers every item of the PLAN goal:

- **Persistence (FUNC-08):** the `terrainImports` state child and the preset-manager `customState` carry one `<slot osc form name sha256 size data|path/>` per oscillator holding an import — raw PNG bytes as standard base64 when ≤ 2 MiB, else `path` + `sha256` + `name` — written and read by one processor-owned adapter pair; a restore re-imports through the Stage 2 API before the `stateGeneration` bump (inline on the message thread, else `callAsync` under a weak alive token). H10 (a)–(e) pin it: bytes-form state round trip, the cap form present / deleted / rewritten, the preset JSON round trip through the public file loader, the no-`customState` clear, two 1.9 MB slots published in ≈ 60 ms, the UI-04 ordering.
- **Fallback (QUAL-03):** an `Imported…` oscillator with no image plays **Sine Product** on every quality path (RESEARCH C1: it was silent), `copyHeightmap` mirrors it, `sourceMissing` reaches the status push through an atomic mirror, the notice carries a **Locate…** button whose native re-links only a SHA-equal file and refuses a different one with `label.locateMismatch`. H10 (b) pins the fallback: rms 0.2420, SHA equal to a Sine Product instance at the same patch.
- **Bank (FUNC-11):** `FactoryPresets::build` is the 18-preset bank of RESEARCH §5.1 (six terrains, all eleven orbits, feedback × 3, Bandlimited × 3, the five BRIEF use cases), authored in engineering units through `RawMap → completeBase() → convertTo0to1`, stamped `1.0.0+18c17735821c`; the constructor sweeps `Factory/` on a stamp mismatch (the Stage 1 `Init`-only orphan is gone: nine folders, 18 files, 198 keys each, no `customState`). H2 gates all 20 rows (18 A + 2 B) with Coarse / Fine honoured.
- **Harness:** H10 grows five row groups, smoke [5] asserts the bank + stamp, the H2 preset loop honours Coarse / Fine, adds the osc-B row, bypasses the FX chain and prints preset-centred neighbours; `--real id=eng` + `--gate h2cli` upstreamed; the layout gate gains a `locate` section and two new source greps.

`--gate all`: **159 / 159, 0 failures, 115.9 s** from the repo root (139 baseline + 18 H10 rows + 2 H2 rows … see A1); **139 / 139 in 117.3 s from `/`** at the 4.1a checkpoint (before the H10 rows landed).

---

## Deliverables per task

| Task | Files | What landed |
|---|---|---|
| 1 Processor | `Source/PluginProcessor.h/.cpp` | `kMaxImportBytes` (2 MiB) + `kMaxImportFileBytes` (8 MiB) public constants; `ImportSlot { …, path, sourceMissing, sourceSize }`; `sourceMissingFlag[2]` atomics read by `getTerrainStatus`; `restoreAlive` token; `saveTerrainImportsTree/Var`, `loadTerrainImportsTree/Var`, `clearImportSlot` (retires the published image through `retire()`, `revision` monotonic, `importRevision` → 0, `imageGeneration` bump, `imageFit` → 0); the `SlotRecord` adapters (`collectSlotRecords`, `recordFromTree/Var`, `applySlotRecord`); `importTerrainImageFromPath` (the record-keeping core: `importTerrainFile` records `getFullPathName()`, a drop / bytes restore records none); `getStateInformation` **replaces** any restored child (`removeChild` + `appendChild`); `setStateInformation` Decision 9 guard; constructor `setCustomStateCallbacks` after `excludedParameterIds`; destructor resets `restoreAlive` before `releasePublishedImages()` |
| 2 Fallback | `Source/dsp/TerrainOscillator.cpp/.h`, `Source/TerrainViewFeed.cpp` | the Imported branch → `terrain (TerrainKind::SineProduct, px, py, terrainFreq * rTrack, terrainModX, terrainModY)` when `image == nullptr`; header comment; `copyHeightmap` mirror |
| 3 preset-manager v1.0.7 | `modules/persistence/preset-manager/{cpp/OuariconPresetManager.h, module.yaml, README.md}`, `modules/registry.yaml`, `Source/OuariconPresetManager.h` | `if (customLoad) customLoad (hasProperty ("customState") ? getProperty (…) : juce::var())`; module version + changelog + README line; registry `1.0.7` + O-Strata `used_by` row; the O-Strata copy carries the identical two-line change |
| 4 Harness H2 | `tests/render-harness/main.cpp`, `README.md` | `--real id=eng`, `cleanPatchB()`, `bypassFx()`, `h2Render (…, f0Mul)`, the preset loop rewrite (Coarse / Fine, `(B)` rows, preset-centred neighbour print), `gateH2Cli()` under `wantsExact ("h2cli")`, `makeNoisePng` / `renderShaOf` / `stateXml` helpers (smoke [3] uses `stateXml`) |
| 5 Bank | `Source/FactoryPresets.h/.cpp`, `Source/PluginProcessor.cpp` (ctor) | 18 `RawMap` presets + `completeBase()` + `normalize` (jassert + DBG on an unknown key) + `choiceIndex (apvts, id, label)` for the division names; `stamp (defs)` = `JucePlugin_VersionString + "+" + sha256 (category, name, (id, IEEE float)…)[0:12]`; the constructor's `deleteRecursively()` + `initializeFactoryPresets (defs, stamp)` block |
| 6 Editor | `Source/PluginEditor.h/.cpp` | `launchPngChooser (osc, title, onFile)` = the one `launchAsync` site (hoisted `SafePointer`, bare return on the dead path); `chooseTerrainImage` on it at `kMaxImportFileBytes` (`tooLargeFile`); `importTerrainImageData` on the processor's `kMaxImportBytes` (reason `tooLarge` unchanged); new `locateTerrainImage (osc)` native (cancel / `tooLargeFile` / `undecodable` / `hashMismatch` untouched / `importTerrainFile` + `repushPending`); the editor's `kMaxImportBytes` deleted |
| 7 Page + i18n | `Source/ui/public/index.html`, `js/i18n.js`, `tests/i18n-states.json`, `tests/ui-stub/generic-overrides.json`, `tests/ui_tip_render_check.js` | notice = `<span id="terrain-notice-text">` + `<button id="btn-terLocate">`, `.transient` hides the button, `setLabel` on the span, two literal whitelist branches, `handleImportResult` maps `hashMismatch` / `tooLargeFile`, the `#btn-terLocate` listener → `locateTerrainImage (ACTIVE_OSC)`, `'Init'` first in `CATEGORY_ORDER`, `#btn-terLocate { min-width: 93.41px }` (en 75.67 / fr 93.41 / zh 54.67); five keys (`label.locate`, `label.locateMismatch`, `label.importTooLargeFile`, `tip.terImport`, `tip.terLocate`; fr `reviewed: false`, zh `'mt'` → 53) + two `TIP_BINDINGS` rows; state `terrain-locate-mismatch`; stub override `locateTerrainImage → {ok:false, reason:'cancelled'}`; tip literal 120 → 122 |
| 8 H10 + smoke [5] | `tests/render-harness/main.cpp`, `README.md` | H10 (a)–(e) (18 new rows), smoke [5] (seven rows over the bank + stamp), README rows |
| 9 Layout `locate` | `tests/ui_layout_check.js` | the `locate` section (button inside the notice in en / fr / zh-Hans, text left of it, transient hides it, click → `locateTerrainImage ('A')` then `('B')`, mismatch transient + restore, sourceMissing false hides), the `locateTerrainImage` native grep, the editor-cap grep; the `drop` section's `notice()` helper reads the span |
| 10 Gates | — | A1–A16 below |
| 11 Install + validate | — | `build-and-install.sh O-Strata` (no orphan sweep: Stage 3 installed `-dev` only), auval ×2, pluginval ×2, the on-disk bank, no tag |
| 12 Docs | `research/ARCHITECTURE.md` (State Persistence), `CHANGELOG.md`, `NOTES.md`, `STATUS.md`, `PLUGINS.md`, this file | as written |

---

## Gate run A1–A16 (repo root, the 4.1 tree)

| # | Gate | Result |
|---|---|---|
| A1 | `$H --gate all` | **159 checks / 0 failures / 115.9 s** (repo root); `presets: 18`; `[H2] factory presets: 20 / 20 pass` |
| A2 | `$H --gate H2` | 66 / 66 analytic, 66 / 66 Bandlimited (worst −2.3 dB Radial Rings × Ellipse), **20 / 20 preset rows**: Init 0.0, Breathing Pad −1.9 (B row 0.0 at (−0.17, 0.24), f0 × 1.003), Chatter Lead 0.0, Pierce Bell 0.0, Your Terrain Here 0.0, PD Organ −3.3, Radial Keys 0.0, Folded Bass 0.0 (f0 × 0.500), Wells Drone 0.0 (B row 0.0, f0 × 0.500), Saddle Pluck 0.0, Butterfly Choir −4.5, Limacon Lead 0.0, Squarcle Storm −3.8 at (0.23, 0.21), Glass Rings 0.0, Mitsuhashi Organ 0.0 at (0, 0), Superellipse Bass 0.0 (f0 × 0.500), Epi Keys 0.0, Saddle Sweep 0.0; every row 100 % of windows; negative control −109.0 dB |
| A3 | `$H --gate H10` | **24 / 24**: the Stage 2 pair; (a) one bytes slot, blob 21 098 B, restore SHA equal; (b) fixture 3 652 762 B in (2 MiB, 8 MiB], path form blob 8 917 B, present → SHA equal, deleted → `sourceMissing` on slot + status, `imagePtr` null, **rms 0.2420 and SHA equal to Sine Product**, rewritten → mismatch → `sourceMissing`; (c) var round trip SHA equal, file loader SHA equal to the same preset + direct import, no-`customState` → both slots cleared; (d) fixtures 1 908 942 / 1 860 715 B, **both slots published in 62 ms** (restore 51 + jobs 11, blob 5.0 MB; < 1000); (e) `stateGeneration` 0 → 1 with `importRevision` 1; preset path moves `importRevision` without a bump, `notifyStateChanged()` bumps |
| A4 | `$H --gate smoke --with-disk` | 61 / 61 incl. [5]: 18 JSONs, `Init/Init.json`, `.factory-version` = `1.0.0+18c17735821c` = `FactoryPresets::stamp`, 19 files, nine folders / 0 stray, 198 keys / no `customState` |
| A5 | `--gate H8 / storm / import` | inside A1 (0 allocations; unchanged rows) |
| A6 | `check-i18n` | exit 0 |
| A7 | `i18n-fr-lint` / `i18n-zh-lint` | exit 0 / exit 0, **53 `'mt'`** |
| A8 | `check-ui-labels` | `states: default + 42`, **1809 PASS / 0 FAIL, ALL CHECKS PASSED** (re-run on the 4.1 tree) |
| A9 | `boot-all-uis --strict-tips` | clean 1 / 1, DEAD 0, late 0, no `stub invented:` (re-run on the 4.1 tree) (the script prints no natives count; the 34-native parity is the layout gate's grep list + the editor's 34 `withNativeFunction` sites) |
| A10 | `ui_layout_check.js` | **ALL PASS — 607 / 0** with `webgl` / `fallback` / `interaction` / `drop` / `locate` (the first run after the `locate` section read 604 / 3: the three `drop` rows read the notice div's dataset — helper fixed to the span); liveness `--viewport 1200x780` **FAILS as required: 526 / 81** (78 at baseline + the three `locate` rows whose button box moves with the shorter frame) |
| A11 | `ui_tip_render_check.js` | **122 bindings, ALL CHECKS PASSED (3215)** on the 4.1 tree |
| A12 | `ui_shell_diff_check.js`; `measure-ui.js` | shell diff **ALL CHECKS PASSED** (self-diff 0 px; the planted 1 px edit reads 1576 px — the probe is live); measure-ui exit 0 (the full census; `#btn-terLocate` reads en 75.67 / fr 93.41 / zh 54.67 px, pinned at 93.41); the CDP font probe is the verify's row (not re-run here) |
| A13 | `--gate orbits` + `orbit-golden.mjs` | **44 / 44 within 1e-4 (worst 2.93e-6)** |
| A14 | param dump vs `.planning/params.tsv` | **diff empty** (205 rows) |
| A15 | warning census | 0 new: every warning line in the rebuilt TUs sits on unchanged code (`TerrainOscillator.cpp` 175 / 321 / 497 float-equal + switch-enum; `PluginProcessor.cpp` 802–803 float-equal; `PluginEditor.cpp` 378–417 unused lambda capture; harness 678 / 963 sign-conversion, 1767 deprecated `createWriterFor`, 1878 float-equal, `maxStep` / `mean` unused); `FactoryPresets.cpp` and `TerrainViewFeed.cpp` compile silent |
| A16 | MSVC greps | `= juce::Component::SafePointer` 0; `[…] constexpr` in lambdas 0; non-ASCII inside a C++ string literal 0 (one em-dash in a new `DBG` literal found and made ASCII) |
| — | install / validators | `build-and-install.sh O-Strata` exit 0 (Phase 4 sweep found no alternate variant); `auval -a` ×2 lists `aumu OuSt OuDv`; `auval -v aumu OuSt OuDv` **AU VALIDATION SUCCEEDED**; pluginval strictness 10 (`/Applications/pluginval.app/Contents/MacOS/pluginval`): VST3 **SUCCESS**, AU **SUCCESS** (on the re-installed 4.1 build); `git tag -l '*Strata*'` empty; the installed VST3 binary is sha256-identical to the build tree's (`798d103f…`) |
| — | on-disk bank | `~/Library/O-Strata/Presets/Factory/`: Bass (2) Drone (1) FX (2) Init (1) Keys (6) Lead (2) Pads (2) Pluck (1) Sequence (1) = 18 + `.factory-version` `1.0.0+18c17735821c`; no `User/` existed on this Mac |

---

## Deviations from the plan (all recorded, none re-opens a contract)

1. **H2 preset rows bypass the FX chain** (`Instance::bypassFx()`, preset rows only). The pre-filter tap is a *voice* tap: the processor's reverb / chorus / delay / distortion still ran on it, so Breathing Pad read −11.7 dB / 62 % and PD Organ −8.7 dB / 62 % (FAIL) while Glass Rings read −5.7 dB through its reverb tail — the same patches FX-free read the research figures (−1.9 / −3.3 / 0.0). The research's scratch rows never set an effect, so the bank was measured FX-free; H2 is the oscillator's symmetry contract. `cleanPatch()` itself is untouched (its renders feed the export golden). Isolated with the new `--gate h2cli` (`GR +reverb` −5.7 vs base 0.0; `BP +reverb` −7.1 / 62 %).
2. **Squarcle Storm reads −3.8 dB, not the research's 0.0 at (0.23, 0.21)**: the shipped def carries the LFO1 S&H 4 Hz → Centre Y ± 0.05 route the scratch line omitted. 100 % of windows; within the −6 dB rule, outside the plan's "± 1 dB of the §5.1 column" wording for this one row.
3. **`SafePointer<…> safeThis (this)` count stays 6, not 7** — the chooser's own site moved *into* `launchPngChooser`; every site is still the hoisted form and `= juce::Component::SafePointer` is 0.
4. **The O-Strata `OuariconPresetManager.h` is a diverged fork of the module** (categories + exclusion, braces, `<map>`), so Task 3's "identical apart from the licence header" diff cannot hold; the identical two-line v1.0.7 change was made in both. `scripts/regen-registry-used-by.sh` would have rewritten 75 / 61 lines across unrelated modules (repo-wide version drift) and was reverted; the registry got the version bump and the O-Strata `used_by` row by hand.
5. **The notice is a flex row** (`.terrain-notice.visible { display: flex; align-items: center; gap: 8px }`, `#terrain-notice-text { line-height: 12px }`) instead of the plan's `margin-left` / `vertical-align` button: the inline form made `#terrain-notice` 3 px taller under zh-Hans (Han fallback face on a `normal` line box) and failed check-ui-labels [7] in three states.
6. **The tip gate learnt the new anchors**: its `TAB_OF` map knew `#knob-ter*` only, and `#btn-terLocate` is hidden until `sourceMissing` — `ui_tip_render_check.js` gained the `#btn-ter*` → Terrain mapping and a `setSourceMissing (on)` push around the two buttons (120 of 122 anchors reached otherwise).
7. **H10 (c)'s file-loader row compares against a reference that went through the same preset apply** (`loadPresetFromFile (presetWithout)` + a direct `importTerrainImage`): a normalised JSON round trip stores `convertFrom0to1 (convertTo0to1 (x))`, which moves a skewed-range parameter by one ulp — measured max |Δ| 4.5e-8 from sample 0 against the live patch, with identical slots, fit and published image, and two file-loaded instances bit-identical. Preset storage, not the image path; recorded in CHANGELOG "Known limits".
8. **H10 (d)'s wait is on the published `imagePtr`s, not `imageGeneration`**: `clearImportSlot` bumps the generation before the import lands, which made the first draft of the row vacuous (0 ms). Fixtures are RGB noise at 740² / 730² (JUCE writes the alpha plane, ≈ 3.5 B / pixel) so they sit in (1.5 MiB, 2 MiB].
9. **The `drop` section's `notice()` helper reads the span** (`#terrain-notice-text`) for key and text — the Stage 3 rows themselves are unchanged; the div now also holds the button's text.
10. **`juce::Process::getProcessId()` does not exist in JUCE 8.0.14** — the H10 temp directory uses `getpid()` (`<unistd.h>`, the TU is macOS-only anyway: `posix_memalign`).
11. **A1 from `/`** was run at the 4.1a checkpoint (139 / 139, 117.3 s, before the H10 rows); the 159-row run is from the repo root. Both cwd forms were exercised, not both on the final row count.

---

## Hands-on checklist for the Round A verify (Decision 25 — not run in this session)

Use `build/plugins/O-Strata/O-Strata_artefacts/Release/Standalone/O-Strata-dev.app` (the install script does not install the Standalone — memory `pattern_build_install_skips_standalone_stale_ui`) and Logic for row 2.

1. **Locate… by hand:** Import… a 3–8 MiB PNG (`makeNoisePng`-class, or any photo saved as PNG), save a User preset, move the file, reload the preset → the "Source missing" notice with **Locate…**, the oscillator sounds Sine Product; Locate… the moved file → map restored, notice gone; Locate… a *different* PNG → "Different file — hash does not match the preset" for 4 s, the sticky notice returns.
2. **Logic (bytes form):** a User preset with an embedded ≤ 2 MiB PNG survives save / close / reopen of the project and the map re-pushes within one push interval.
3. **Live restore (UI-04):** save a Standalone session with an imported PNG, quit, relaunch → the map shows the picture within one interval; load `Init` → the map returns to Sine Product and the notice is hidden.
4. **Dropdown:** `Init` first, then Pads … FX; ◀ / ▶ walk all 18 alphabetically.
5. **First listen** to the 18 presets for gross faults only — the sign-off is Round B's LISTENING.md. Pierce Bell wants the Tuning tab at Bohlen-Pierce.

**Carried unchanged:** Stage 3's six human rows (`stages/3-gui/VERIFICATION.md` §Human) and the Round A rows 3–10 of `stages/3-gui/round-a/VERIFICATION.md`; fr read of the five new rows (`reviewed: false`); zh flips (53 `'mt'`).

---

## Handoff to verify

`/plugin-verify O-Strata 4-polish` re-measures A1–A16 from the 4.1 commit, annotates FUNC-08 / FUNC-11 / QUAL-03 in `REQUIREMENTS.md` (untouched this round), moves this PLAN / SUMMARY / VERIFICATION to `stages/4-polish/round-a/`, and hands to `/plugin-plan O-Strata 4-polish` (Round B, Decisions 27+: the D4 padded Chebyshev evaluator, `ci-tests.yml` O-Strata dispatch, the listening pass, CHANGELOG v1.0.0, the release install) unless a contract re-opens.
