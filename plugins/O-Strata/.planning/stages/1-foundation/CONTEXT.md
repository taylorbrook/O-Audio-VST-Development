# Stage 1 — Foundation: CONTEXT

**Plugin:** O-Strata
**Stage:** 1 of 4 — Foundation + Shell (single phase)
**Phase:** discuss ✓
**Date:** 2026-09-07
**Mode:** manual (interactive — four decisions put to Taylor, answers recorded below)
**Branch:** `main` (trunk-based; path-scoped commits under `plugins/O-Strata`)

---

## Goal

A buildable, validating O-Strata that is a **complete, independent copy** of O-Prism v1.24.0 with the
wavetable library removed and 48 inert geometry parameters added. No generator code; both oscillators
play `WavetableGenerator`'s sine placeholder through the unchanged voice/FX path. Requirement verified:
**COMPAT-01** (pluginval strictness 10 on VST3 + AU, auval pass).

## Inherited contracts — NOT re-opened

Settled by Stage 0, the finalized v1 mockup and the locked `parameter-spec.md`. Stage 1 implements them verbatim.

| Contract | Source | Value |
|---|---|---|
| Fork base | ROADMAP Stage 1 | `plugins/O-Prism/` at v1.24.0 (working tree = HEAD, clean) |
| Target / folder | ROADMAP, CMake snippet hunk 1 | `O-Strata`, `plugins/O-Strata/` |
| Product name | CMake snippet hunk 1 | `"O-Strata${OUARICON_DEV_SUFFIX}"` |
| Plugin code | ROADMAP | `OuSt` — verified unused across `plugins/*/CMakeLists.txt` at Stage 0 |
| Version keyword | memory `critical_plugin_version_keyword_ignored_by_juce` | **`VERSION 1.0.0`** inside `juce_add_plugin`; never `PLUGIN_VERSION` |
| Formats | CMake snippet | VST3 AU Standalone; `IS_SYNTH TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE` |
| Modules | CMake snippet hunk 3 | O-Prism's 13 + **`juce::juce_cryptography`** |
| Parameter count | parameter-spec.md v1 (locked) | **219** = 171 inherited + 48 geometry (24 × {A, B}); `oscATable`/`oscBTable` removed |
| Parameter order | parameter-spec.md "Notes" | O-Prism's `createParameterLayout()` minus the two Table params, 24 geometry params appended **inside each oscillator's block** in the "all 48 IDs" table order |
| Geometry IDs | parameter-spec.md | camelCase `oscA…`/`oscB…` + `GeoSource GeoFrames GeoDrive Mesh MeshTiltX MeshTiltY MeshPhi MeshUnwrap MeshLoop VolField VolOrbit VolSweepAxis VolSweepRange VolDetail Terrain TerOrbit TerCX TerCY TerAspect TerRot TerSweepAxis TerSweepRange TerBlur TerEdge` |
| Choice lists | parameter-spec.md | English display strings exactly as the spec tables; `Imported…` is always the **last** index; Terrain list is the mockup's **3** entries (`Sine Product / Radial Rings / Imported…`) — NOT ARCHITECTURE's six. Stage 2.4 may insert analytic terrains before `Imported…` with a spec bump |
| Bake params | ARCHITECTURE Decision 2 | plain `AudioParameterFloat` / `AudioParameterChoice`; **not** added to `modSlot?Dst` (stays at O-Prism's 26 entries); no APVTS listener |
| `osc?Pos` | parameter-spec.md | ID, range, default unchanged; still a mod destination |
| Relay generation | parameter-spec.md "Relay / attachment plumbing" | `oscIds()` drops `"Table"`, appends the 24 suffixes → 35 per oscillator; `allSliderIds()` 141 → **187** (+1 for D4 below = **188**) |
| Editor guard | memory `pattern_render_harness_breaks_on_webview_editor` | `createEditor()` wrapped in `#if JUCE_WEB_BROWSER … #else GenericAudioProcessorEditor` |
| Binary data | memory `critical_dual_binary_data_namespace_collision` | exactly one `juce_add_binary_data` target: `O-Strata_UIResources` |
| State children | parameter-spec.md "Non-parameter state" | `geometryImports` child **stubbed empty** in get/setStateInformation; `tuningEngine`, `uiLanguage` inherited |
| Removal set | ARCHITECTURE Decision 5 | `WavetableFactory`, `UserWavetableManager`, `WavetableImporter`, `WavetableEditor` (.h/.cpp), `wavetable-editor.js`, `wavetable-editor.css`, `oscATable`/`oscBTable`, `pOscATable`/`pOscBTable`, the user-wavetable ValueTree child, `resolveActiveTable` → `oscTablePtr[osc]`, 14 native functions, their i18n rows |
| Kept | ARCHITECTURE Decision 5 | `WavetableGenerator` (+ `WavetableData.h`): `generateMipmaps`, `generateProceduralTable(Sine)` as the initial table for both oscillators |
| Root CMake | root `CMakeLists.txt:48` | auto-discovers `plugins/*` via `file(GLOB)` — **no root edit needed** |
| Param-dump | `scripts/param-dump/ParamDump.cmake` | `ouaricon_add_param_dump(O-Strata …)` behind `OUARICON_BUILD_TESTS`; expect 219 rows, diff vs O-Prism = −2 +48 |

**Read-only in this stage:** `parameter-spec.md` (locked v1), `BRIEF.md`, `REQUIREMENTS.md`, everything under `plugins/O-Prism/`.

## Decisions taken this phase

### D1 — Full rename: O-Strata is a new plugin, O-Prism stays untouched

Taylor: *"strata should be a completely different and new plugin — keep prism as is and separate."*

Every `Prism`/`OPrism` identifier in the copied source becomes its `Strata` counterpart, applied everywhere in one pass:

| O-Prism | O-Strata | Occurrences (O-Prism source) |
|---|---|---|
| `OPrismAudioProcessor` | `OStrataAudioProcessor` | 41 |
| `OPrismAudioProcessorEditor` | `OStrataAudioProcessorEditor` | 20 |
| `PrismVoice` (+ `PrismVoice.h/.cpp`) | `StrataVoice` (+ files) | 27 |
| `PrismSound` (+ `PrismSound.h`) | `StrataSound` (+ file) | 7 |
| `PrismParamIds` (+ `PrismParamIds.h`) | `StrataParamIds` (+ file) | 9 |
| `"OPrismParameters"` (APVTS state Identifier) | `"OStrataParameters"` | 1 |
| `"O-Prism"` in `getName()`, `presetManager(parameters, "O-Prism")`, tuning HTML export title, file-header comments | `"O-Strata"` | — |
| `"OPrism_WebView"` (WebView2 user-data folder) | `"OStrata_WebView"` | 1 |
| `O-Prism_UIResources` | `O-Strata_UIResources` | 2 |

**Consequences:**
- **No file under `plugins/O-Prism/` is created, modified or deleted** by Stage 1. `git status --short plugins/O-Prism` must stay empty at the Stage 1 commit. No shared-module extraction, no symlinks, no `#include "../../O-Prism/..."`.
- Every literal `Prism` token remaining in `plugins/O-Strata/Source/` after the rename is a defect to explain or remove (the grep gate below). The only tolerated survivors are historical comments that name O-Prism as the fork origin (e.g. the CHANGELOG line and the file-header "forked from O-Prism v1.24.0" note).
- Presets: `presetManager (parameters, "O-Strata")` gives O-Strata its own user-preset folder; O-Prism presets are not visible in O-Strata. `FactoryPresets.cpp` is carried over with `oscATable`/`oscBTable` entries stripped (a preset naming a removed ID would otherwise be silently ignored by the preset manager, but the diff must be explicit). Presets are re-authored in Stage 4 regardless.
- The APVTS state Identifier change means an O-Prism session state cannot load into O-Strata — intended; they are different plugins with different `PLUGIN_CODE`.

### D2 — GeometryBakeScheduler skeleton deferred to Phase 2.1

Stage 1 adds **no** `Source/geometry/` directory and no bake code. The oscillators read `oscTablePtr[osc]` which is initialised once, in the processor constructor, to the sine placeholder table. The thread contract (timer → ThreadPool → `callAsync` publish → two-generation reaper) is proven in Phase 2.1 where it is mandatory. The ROADMAP's "(Optional here; mandatory in 2.1)" resolves to *not here*.

**Consequence:** the CMake snippet's hunk 2 `Source/geometry/*.cpp` lines are **not** applied in Stage 1; only the three removals in that hunk are. Hunk 4's `js/geometry-view.js` and the shell PNG are likewise Stage 3 — `O-Strata_UIResources` in Stage 1 lists exactly `index.html`, `js/i18n.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`.

### D3 — UI: strip only

`index.html` loses the wavetable tab/editor/selection UI, the `wavetable-editor.js` / `.css` references, and the JS that called the 14 removed native functions. **No geometry controls are added**; the Geometry panel arrives in Phase 3.1 from `mockups/v1-ui.html`. The two oscillator panels keep their remaining 11 controls each (Pos, Level, Pan, Coarse, Fine, Phase, Unison, Detune, Width, WarpType, WarpAmt).

**Consequences:**
- The 48 geometry parameters are exercised in Stage 1 only through the host (automation list, generic editor when `JUCE_WEB_BROWSER=0`, param-dump, state round-trip) — not through the WebView. That is sufficient for COMPAT-01.
- Relays and attachments **are** still generated for all 188 slider IDs by the unchanged constructor loop (they need no matching DOM node); `jassert (sliderAttachments.size() == sliderRelays.size())` holds at 188.
- `i18n.js`: remove the wavetable-tab keys (en/fr/zh-Hans rows together — a key is removed from all three languages or none). No new keys in Stage 1. `check-i18n`, `check-ui-labels` and `boot-all-uis` must pass on the stripped page; `boot-all-uis` must report no dead tip bindings left behind by removed markup.
- The 14 removed native functions are `getUserWavetableList importUserWavetable importUserWavetableData selectUserWavetable clearUserWavetableOverride deleteUserWavetable startWavetableEditor stopWavetableEditor getEditorFrameWaveform getFrameHarmonics setFrameHarmonics applyFrameOperation saveEditedWavetable getAllEditorFrameWaveforms` (16 wavetable-related minus 2). **Kept:** `getActiveOscInfo`, `getActiveOscFrame` — they read the published table and serve the ≋ view in Stage 3. O-Prism's 45 → O-Strata's 31 native functions.
- `timerCallback`'s `evaluateJavascript` push (held notes) is **left as-is** in Stage 1; ARCHITECTURE Decision 7 (event push) is a Stage 3 change.

### D4 — `delayDivision` added to `allSliderIds()`

One-line fix in the fork: `delayDivision` joins the delay group in `allSliderIds()`. It is bound in `index.html` via `getSliderState("delayDivision")` in O-Prism but has no relay, so its knob has never followed host automation. **Relay/attachment total becomes 188 sliders + 30 toggles = 218 for 219 parameters** (only `stereoWidth`, which has no UI, remains unbound). `parameter-spec.md` and `v1-integration-checklist.md` say 187 — that figure predates this decision; the spec is not edited (locked), this CONTEXT is the record. `stereoWidth` stays as-is.

### D5 — Count corrections propagate to ROADMAP / ARCHITECTURE (taken by the orchestrator, no question needed)

`parameter-spec.md` "Draft reconciliation" establishes 48 / 219 / 141 → 187 and states that ROADMAP.md and ARCHITECTURE.md "inherit the same undercount and should read 48 / 219 / 141 → 187". Stage 1 applies that correction to both documents (24 stale occurrences found by grep: `217`, `46`, `23 ×`, `126 → 170`, `170`) and to the Stage 1 test criteria (`217` → `219`). STATUS.md's contract checksums for `roadmap` and `architecture` are re-recorded after the edit. The `allSliderIds()` trailing `// 126` comment is corrected in the fork to the real count.

### D6 — Fork mechanics (routine, recorded for the plan)

Copy **only**: `Source/` (all of it, then delete the removal set), `CMakeLists.txt`, `tests/` (`i18n-states.json`, `ui_tip_render_check.js`, `ui-stub/generic-overrides.json` — the UI-gate fixtures the strip must keep green). **Do not copy:** `backups/`, `BUG-tuning-tab-cutoff.md`, `CHANGELOG.md` (new stub written instead), `NOTES.md` (O-Strata already has its own). Fresh `CHANGELOG.md` with `## v1.0.0 (unreleased)` naming the fork base. `plugins/O-Strata/NOTES.md` (exists) gets a Stage 1 entry, not a rewrite.

## Constraints carried into implementation

- **Two-way grep gate on the removal set** (Stage 1 test criterion, extended): `grep -rn "WavetableFactory\|UserWavetableManager\|WavetableImporter\|WavetableEditor\|oscATable\|oscBTable\|pOscATable\|pOscBTable\|resolveActiveTable" plugins/O-Strata/Source/` returns nothing; `grep -rln "Prism" plugins/O-Strata/Source/` returns only files whose hits are fork-origin comments (list them in SUMMARY.md).
- **O-Prism isolation gate:** `git status --short plugins/O-Prism` empty; `git diff --stat HEAD -- plugins/O-Prism` empty at commit time.
- **Choice params need ≥ 2 choices** (memory `critical_choice_param_needs_two_choices`) — every geometry Choice list in the spec has ≥ 3; assert at layout time.
- **Param-ID identifiers shadow juce free functions** (memory `critical_paramid_shadows_juce_free_function`) — the 24 new suffixes were checked: none collides with a `juce::` free function; keep the `p…` cache naming convention if raw-value caches are added (they are **not** needed in Stage 1 — bake params are read by the Stage 2 scheduler via `getRawParameterValue` at bake time).
- **Version:** `VERSION 1.0.0`; `JucePlugin_VersionString` must print `1.0.0` in pluginval's header (proves the keyword was honoured).
- **Standalone stays stale after `build-and-install.sh`** (memory `pattern_build_install_skips_standalone_stale_ui`) — the Stage 1 build criterion names `O-Strata_Standalone` explicitly.
- **Cold auval rescans** (memory `pattern_cold_auval_after_install_rescans_registry`) — run `auval -a | grep -i strata` in the background at the end of the verify batch, budget 2 min.
- **Commit discipline:** `git commit -- plugins/O-Strata PLUGINS.md` after `git branch --show-current` / `git status --short`; never `-a`/`-A`. Untracked new files need `git add` first (memory `pattern_git_commit_pathspec_takes_only_tracked_files`).
- **No tags** (memory `feedback_never_tag_unless_publish`).

## Stage 1 test criteria (as amended by D1–D5)

- [ ] `ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone` builds clean (no new warnings vs O-Prism); `./scripts/build-and-install.sh O-Strata` installs with the dual-variant AU sweep
- [ ] `auval -a | grep -i strata` lists the AU; `auval -v aumu OuSt <mfr>` passes
- [ ] pluginval strictness 10 passes VST3 and AU (COMPAT-01); reported version `1.0.0`
- [ ] Host automation list shows **219** parameters; `oscATable`/`oscBTable` absent
- [ ] Param-dump (`OUARICON_BUILD_TESTS=ON`) prints **219** IDs in the exact order and with the ranges/defaults/choice strings of `parameter-spec.md`; diff vs O-Prism's dump = −2 +48
- [ ] `allSliderIds().size() == 188`; `sliderAttachments.size() == sliderRelays.size()`; 30 toggle relays unchanged
- [ ] Removal-set grep and `Prism` grep gates (above) pass; `plugins/O-Prism` unchanged
- [ ] Stripped `index.html` passes `check-ui-labels`, `check-i18n`, `boot-all-uis`; fr lint exit 0; zh lint Z5 pass; no console errors in Standalone; no 404s in the resource provider
- [ ] Notes play the sine placeholder on both oscillators through the unchanged voice/FX path; tuning tab loads a Scala file
- [ ] State save/reload round-trips all 219 parameters + tuning + uiLanguage; an empty `geometryImports` child is written and read without error
- [ ] `CHANGELOG.md` `## v1.0.0 (unreleased)`; PLUGINS.md row → 🚧 Stage 1; ROADMAP/ARCHITECTURE counts read 48 / 219

## Open for the research phase

1. Exact list of `index.html` regions and `i18n.js` keys that belong to the wavetable UI (the 51 `wavetable|table|wtEd` matches in `i18n.js` need triage: some are legitimate, e.g. tuning "table" rows).
2. Whether `FactoryPresets.cpp` references `oscATable`/`oscBTable` by ID (strip) and whether any preset's audible identity depended on a non-sine table (note for Stage 4 re-authoring; not fixed here).
3. Where `resolveActiveTable` / `updateWavetableAssignments` sit in `PluginProcessor.cpp:946-1000` and the minimal edit that leaves `oscTablePtr[osc]` + retire/reaper intact for Phase 2.1.
4. Whether O-Prism's `tests/ui_tip_render_check.js` and `tests/i18n-states.json` encode wavetable-tab states that must be removed alongside the markup.
5. The concrete `sed`/rename procedure that is safe for the `Prism` → `Strata` sweep (case variants `Prism`, `PRISM`, `prism`; must not touch the i18n canon region byte-identity that check-i18n assertion 6 checks — confirm whether that region contains the word).

## Requirements traceability

| ID | Stage 1 evidence |
|---|---|
| COMPAT-01 | pluginval strictness 10 VST3 + AU pass, auval pass |
| FUNC-09 / FUNC-10 (smoke only) | sine placeholder plays; Scala file loads — formally verified in Phase 2.1 |
