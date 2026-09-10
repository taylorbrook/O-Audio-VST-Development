# Stage 1 — Foundation, second pass (re-parameterise the verified fork): CONTEXT

**Plugin:** O-Strata
**Stage:** 1 of 4 — Foundation + Shell, **second pass** (single phase; ROADMAP "Stage 1: Foundation — second pass")
**Phase:** discuss ✓
**Date:** 2026-09-10
**Mode:** manual (interactive — two decisions put to Taylor, answers recorded below)
**Branch:** `main` (trunk-based; path-scoped commits under `plugins/O-Strata`)
**First pass:** `first-pass-baked/` (CONTEXT D1–D6, RESEARCH, PLAN, SUMMARY, VERIFICATION of the baked-parameter fork, 2026-09-07). Every reference elsewhere to `stages/1-foundation/VERIFICATION.md` dated 2026-09-07 means `first-pass-baked/VERIFICATION.md`. `smoke/` is shared by both passes and is updated, not replaced.

---

## Goal

The existing, verified O-Strata binary (O-Prism v1.24.0 fork, `OuSt`, `VERSION 1.0.0`) with the 48 baked-geometry parameters replaced by the 34 live-oscillator parameters of the locked `parameter-spec.md` v2, the mod-destination list grown from 26 to 46, the state child renamed, the relay plumbing sized for the v2 UI, and COMPAT-01 re-verified. Both oscillators still play the sine wavetable placeholder (deleted in Phase 2.1) so the plugin sounds during validation.

Requirement verified: **COMPAT-01** (pluginval strictness 10 on VST3 + AU, auval pass — re-verified after the re-parameterise). FUNC-09 (ID parity) and FUNC-05 (list acceptance) get their Stage 1 evidence here; formal verification is Stage 2.

## Inherited contracts — NOT re-opened

Settled by Stage 0 v2, the finalised mockup v2 and the locked `parameter-spec.md` v2 (sha256 in STATUS `contract_checksums.parameter_spec`). Stage 1 implements them verbatim; where ROADMAP / ARCHITECTURE and the spec disagree, **the spec wins** (it was locked after both).

| Contract | Source | Value |
|---|---|---|
| Parameter count / order | spec §Total, §APVTS order, §Notes | **205** = 171 inherited + 34 new; `createOscParameters("oscA")` then `("oscB")`, **28 per oscillator** in spec rows 1–28 (11 carried + `Terrain, TerFreq, TerModX, TerModY, TerTrack, TerSat, TerBlur, TerEdge, Orbit, OrbAspect, OrbRot, OrbCX, OrbCY, OrbMod, OrbFeedback, OrbFbDamp, Quality`), then the inherited sections in O-Prism's `createParameterLayout()` order |
| Removed | spec §APVTS order note | the 24 baked suffixes per oscillator (`GeoSource … TerSweepRange`, incl. `TerOrbit TerCX TerCY TerAspect TerRot TerSweepAxis TerSweepRange`); `Terrain`, `TerBlur`, `TerEdge` **keep their IDs** with the v2 meaning and lists |
| In-place changes | spec §Inherited parameters changed in place | `osc?Pos` default 0.0 → **0.5**, host name `Osc ? Position` → `Osc ? Orbit Size`; `osc?Unison` `AudioParameterInt (1, 4, 1)` (was 1–8); `modSlot{0..15}Dst` list 26 → **46** |
| Choice lists (verbatim, ≥ 2 entries each) | spec rows 12 / 19 / 20 / 28 | Terrain 7 (`Sine Product / Radial Rings / Saddle / Ridged Cosines / Mitsuhashi / Cosine Wells / Imported…`, `Imported…` **last**); TerEdge 2 (`Mirror / Window`); Orbit 11 (`Ellipse / Superellipse / Limaçon / Epitrochoid 3 / Epitrochoid 5 / Epitrochoid 7 / Hypocycloid 3 / Hypocycloid 5 / Hypocycloid 7 / Butterfly / Squarcle`); Quality 3 (`Bandlimited / 2× / 4×`, default **2×** = index 1). Non-ASCII glyphs (`…`, `ç`, `×`) through `CharPointer_UTF8` hex escapes as the fork already does for `Imported…` (memory `critical_juce_string_char_ctor_is_ascii_only`) |
| `osc?TerFreq` | spec row 13 + §Draft reconciliation | 0.25–8.0, **exact log** `NormalisableRange<float>` from lambdas (`from0to1: 0.25·32^n`, `to0to1: log(v/0.25)/log 32`, snap identity), interval 0, default 1.0 (norm 0.400000). Not a skew factor |
| Other new floats | spec rows 14–18, 21–27 | `TerModX/Y` 0–1 / 0.5; `TerTrack` 0–1 / 1.0; `TerSat` 0–1 / 0.0; `TerBlur` 0–1 / 0.2; `OrbAspect` 0.1–1 / 0.7; `OrbRot` 0–360 step 0.1 / 0; `OrbCX` −1–1 / 0.13; `OrbCY` −1–1 / 0.21; `OrbMod` 0–1 / 0.5; `OrbFeedback` 0–1 / 0.0; `OrbFbDamp` 0–1 / 0.5; step 0.001 unless stated |
| Host names | spec §semantics | `"Osc A " + <Label>` (`Osc A Terrain Freq`, `Osc A Orbit Centre X`, `Osc A Quality`, …) — not part of the contract; IDs, ranges, defaults, lists are |
| Mod destinations | spec §Mod-matrix destinations | `ModDest` enum + `getModDestNames()` = 46: indices 0–25 unchanged **except** 1 / 2 relabelled `"OscA Orbit Size"` / `"OscB Orbit Size"` (enum names `OscAPos` / `OscBPos` unchanged); 26–45 appended **all A then all B**: `OscA Orbit Aspect, OscA Orbit Rot, OscA Orbit CX, OscA Orbit CY, OscA Orbit Mod, OscA Terrain Freq, OscA Terrain Mod X, OscA Terrain Mod Y, OscA Feedback, OscA Saturation`, then the OscB ten. ROADMAP's shorter `"OscA Size"` is superseded by the spec (Stage 0 open question 4 closed) |
| `StrataParamIds` | spec §APVTS order note; checklist A.2 | `oscIds(prefix)` = rows 1–11 + the 13 Float rows 13–18, 21–27 (**24** slider IDs per oscillator, that order); **new** `oscComboIds(prefix)` = `Terrain, TerEdge, Orbit, Quality`; **new** `allComboIds()` = 8; `allSliderIds()` = **166** (24 × 2 + 118 incl. first-pass D4 `delayDivision`) |
| Relays | spec §Relay / attachment plumbing; template `mockups/v2-PluginEditor-TEMPLATE.*` | 166 `WebSliderRelay` + **8 `WebComboBoxRelay`** (new group, `.withOptionsFrom()`, order relays → webView → attachments) + 30 toggle relays = 204 for 205 params (`stereoWidth` unbound, as in O-Prism) |
| State child | spec §Non-parameter state | `geometryImports` → **`terrainImports`** (`PluginProcessor.cpp` ~1081 / ~1129), still written empty and read tolerantly; content is Stage 4 |
| Placeholder oscillator | Stage 0 D6 | `WavetableOscillator` + sine table stay through this pass; `kMaxUnison = 8` in the oscillator is left alone (deleted with the class in Phase 2.1) — the parameter clamps at 4, `setUnison` still accepts ≤ 8 |
| Version | first-pass CONTEXT | `VERSION 1.0.0` unchanged — this pass is pre-release |
| Editor guard, binary data, root CMake, param-dump target | first-pass CONTEXT | unchanged (`#if JUCE_WEB_BROWSER` guard, one `O-Strata_UIResources`, `ouaricon_add_param_dump(O-Strata …)`) |
| Fork base drift | O-Prism CHANGELOG 1.25.0 / 1.26.0 | **none to pull**: 1.25.0 = Geometry wavetable bank (wavetable-only), 1.26.0 = markup/CSS only ("no parameter, DSP or state-format change"). The UI shell re-fork from v1.26.0 is Stage 3 (checklist B.0) |

**Read-only in this stage:** `parameter-spec.md` (locked v2), `BRIEF.md`, `REQUIREMENTS.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` (checksummed contracts — count corrections below are recorded here, not edited in), everything under `plugins/O-Prism/`.

## Decisions taken this phase

### D1 — UI scope: relay plumbing only, page untouched

Taylor: *"Relays only, page untouched."*

`PluginEditor.h/.cpp` gain the `comboRelays` / `comboAttachments` group (8 IDs, template order) and the slider group is regenerated from the new `allSliderIds()` (166). **`Source/ui/public/index.html` and `js/i18n.js` are not edited** — they stay the stripped first-pass v1.24.0 page. No placeholder panels, no new `data-i18n` keys.

**Why:** Stage 3 re-forks the page shell from O-Prism v1.26.0 and splices `mockups/v2-ui.html` over it (checklist B.0 / B.1); anything added to the v1.24.0 page now is discarded there. The first pass validated its 48 inert parameters the same way (first-pass D3) and COMPAT-01 does not need a WebView control.

**Consequences:**
- ROADMAP Stage 1 criteria "every new knob moves its parameter" and "`data-i18n` keys added with en / fr / zh-Hans strings" **move to Phase 3.1**. The 34 new parameters are exercised through the host automation list, `GenericAudioProcessorEditor` (`JUCE_WEB_BROWSER=0`), param-dump, pluginval and the smoke harness.
- Relays and attachments need no matching DOM node (proven in the first pass at 188). `jassert (sliderAttachments.size() == sliderRelays.size())` holds at 166; add the same assert for the combo group at 8.
- The page's hard-coded 26-entry `destNames` fallback (`index.html` ~3189) is only used when the native `getModDestNames` call fails (browser gates); inside the plugin the dropdown receives 46 from C++. The fallback is replaced by `v2-ui.html`'s 46-entry list in Stage 3. The Synth-tab knob still captioned "Position" reads the `osc?Pos` value (now defaulting to 0.5) correctly; relabel is Stage 3.
- UI gates (`check-i18n`, fr / zh lint, `check-ui-labels`, `boot-all-uis`, tip render) are re-run as a **regression check only** — the page is byte-identical, so the expected result is the first-pass baseline (0 DEAD / 0 late, 2799 tip checks).

### D2 — Factory presets: start from scratch, no O-Prism presets

Taylor: *"start from scratch for presets — don't bring in O-Prism presets."*

`FactoryPresets.cpp` is **rewritten**: the inherited O-Prism bank (≈ 20 presets naming `osc?Unison` 5–7 and `osc?Pos` 0.0, plus the `DST_*` index constants) is removed. Stage 1 ships **one factory preset, `Init`, at the 205-parameter defaults** (the preset manager needs a non-empty bank to initialise; an Init preset is also what O-Strata's first user click should load). O-Strata's real bank is authored in Phase 4.1 from the live oscillator.

**Consequences:**
- `FactoryPresets::build()` keeps its signature (`std::vector<FactoryPresetDef>` from the APVTS); the helper scaffolding (category / name / value map → normalised) may stay as the 4.1 seed, but no preset content from O-Prism survives. The `oscATable` / `oscBTable` strip from the first pass becomes moot.
- **On-disk regeneration trap:** the constructor regenerates the factory bank only when `getFactoryPresetsVersion() != JucePlugin_VersionString`, and the version stays `1.0.0`. The first-pass dev install already wrote the O-Prism-derived bank under `~/Library/…/O-Strata/Factory/` stamped `1.0.0`, so on Taylor's machine it would **not** regenerate. The execute phase deletes that folder before the first launch (dev machine only — nothing has shipped, no user is affected) and the smoke harness or verify step confirms the bank on disk is exactly `Init`. Research locates the exact path (`OuariconPresetManager::getPresetsDirectory()`, `~/Library/Application Support/…`) and checks whether `initializeFactoryPresets` clears stale files itself.
- Preset-manager v1.0.6's choice-param migration hook is untouched; with no shipped preset there is nothing to migrate.
- CHANGELOG records the `osc?Pos` default and `osc?Unison` range change as the **last free range change** and the bank reset.

### D3 — The 20 new mod-destination offsets are not read by the voice yet (orchestrator decision)

ROADMAP says the voice "reads the 20 new offsets and (for now) discards them". `ModulationMatrix::destOffsets` is a `std::array<float, kNumDests>` that grows with the enum, and the matrix's slot loop already bounds-checks `dstIdx < destOffsets.size()`, so a slot routed to index 26–45 accumulates into a valid cell that nobody reads. **`StrataVoice.cpp` is not edited in this pass.** Reading-and-discarding is dead code that Phase 2.1 would rewrite anyway.

**Consequence:** with a slot routed to a new destination the sine placeholder is unaffected — expected, and the smoke harness asserts it (route LFO1 → `OscA Terrain Freq`, render, output equals the unrouted render).

### D4 — Count corrections recorded, not edited in (orchestrator)

ROADMAP Stage 1's test criterion "diff against O-Prism v1.24.0's `params.tsv` = exactly 2 removed + 34 added + 2 changed in place" undercounts the in-place changes: the 16 `modSlot?Dst` rows change too (`numSteps` 26 → 46, `textAtMax` `OscB Warp` → `OscB Saturation`). The spec §Notes states it correctly. The verify criterion below uses the full figure: **−2, +34, 4 + 16 = 20 rows changed in place**, every other row byte-identical. ROADMAP and ARCHITECTURE are checksummed contracts and are not edited for this.

## Constraints carried into implementation

- **Choice params need ≥ 2 entries** (memory `critical_choice_param_needs_two_choices`): TerEdge is exactly 2 — never trim it.
- **Param-ID identifiers must not shadow `juce::` free functions** (memory `critical_paramid_shadows_juce_free_function`): the 17 new suffixes were checked at spec lock; re-grep at execute (`Orbit`, `Quality`, `Terrain` are the likeliest collisions if `p…` raw-value caches are added — none are needed in Stage 1).
- **`juce::String (const char*)` is ASCII-only** (memory `critical_juce_string_char_ctor_is_ascii_only`): `Imported…`, `Limaçon`, `2×`, `4×` go through `CharPointer_UTF8`.
- **Exact-log range:** `NormalisableRange<float>` with conversion lambdas has no `interval`; param-dump must report `numSteps 2147483647` for `osc?TerFreq` and `defaultNorm 0.400000` to six places. The lambdas must be pure (no captures of processor state) — the range is copied into the parameter.
- **Append-only lists after v1.0.0** — this pass is the last free change to any range or list.
- **`getLatencySamples()` is non-virtual** — no latency change in this pass (the +1 sample report is Core 5, Stage 2).
- **Standalone stays stale after `build-and-install.sh`** (memory `pattern_build_install_skips_standalone_stale_ui`): build `O-Strata_Standalone` explicitly if it is used for a visual check.
- **Cold auval rescans** (memory `pattern_cold_auval_after_install_rescans_registry`): run `auval -a | grep -i strata` in the background at the end of the verify batch, 2-minute budget.
- **O-Prism isolation:** `git status --short plugins/O-Prism` and `git diff --stat HEAD -- plugins/O-Prism` empty at commit time.
- **Commit discipline:** `git branch --show-current` + `git status --short` immediately before `git commit -- plugins/O-Strata PLUGINS.md`; new files need `git add` first (memory `pattern_git_commit_pathspec_takes_only_tracked_files`); never `-a` / `-A`; **no tag** (memory `feedback_never_tag_unless_publish`).
- **Two sessions share the checkout** — path-scope everything.

## Stage 1 second-pass test criteria (ROADMAP Stage 1 as amended by D1–D4)

- [ ] `ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone` clean (no new warnings vs the first pass); `./scripts/build-and-install.sh O-Strata` installs with the dual-variant sweep
- [ ] pluginval strictness 10 passes VST3 and AU; `auval -v aumu OuSt OuDv` passes; reported version `1.0.0` (**COMPAT-01**)
- [ ] Param-dump = **205** rows, byte-identical to the regenerated `.planning/params.tsv`; diff vs O-Prism v1.24.0 `params.tsv` = −2 (`osc?Table`) +34 (spec rows 12–28 × A/B, in spec order inside each oscillator block) + 20 rows changed in place (`osc?Pos` defaultNorm 0.5 / host name, `osc?Unison` max 4 / numSteps 4, 16 × `modSlot?Dst` numSteps 46 / textAtMax); every other row byte-identical (FUNC-09 IDs)
- [ ] `modSlot0Dst` exposes 46 choices; indices 0–25 equal O-Prism's strings except 1 / 2 = `OscA Orbit Size` / `OscB Orbit Size`; 26–45 equal the spec table verbatim (FUNC-05 list)
- [ ] Choice lists verbatim per the spec (Terrain 7 with `Imported…` last, TerEdge 2, Orbit 11, Quality 3 default `2×`); `osc?TerFreq` numSteps continuous, defaultNorm 0.400000, 0.25 / 8.0 ends
- [ ] `allSliderIds().size() == 166`; `allComboIds().size() == 8`; `sliderAttachments.size() == sliderRelays.size()`; `comboAttachments.size() == comboRelays.size() == 8`; 30 toggle relays unchanged
- [ ] Removal grep: `grep -rn "GeoSource\|GeoFrames\|GeoDrive\|MeshTilt\|VolField\|VolOrbit\|TerSweep\|TerOrbit\|geometryImports" plugins/O-Strata/Source/` returns nothing
- [ ] Smoke harness (`smoke/`) updated and green: notes sound on A and B (sine placeholder), Scala load, **205**-param randomised state round-trip into a fresh processor with 0 mismatches, `<terrainImports/>` present and empty, a slot routed to a new destination leaves the placeholder output unchanged (D3)
- [ ] Factory bank on disk after a fresh launch = exactly `Init` (D2); the stale first-pass bank folder removed on the dev machine
- [ ] UI gates unchanged from the first-pass baseline (`check-i18n`, fr / zh lint, `check-ui-labels`, `boot-all-uis --strict-tips`, tip render) — regression only, page untouched (D1)
- [ ] `plugins/O-Prism` untouched; `git tag | grep -i strata` empty
- [ ] `CHANGELOG.md` `## v1.0.0 (unreleased)` second-pass entry (205 params, 46 destinations, `osc?Pos` / `osc?Unison` last free range change, factory bank reset); `NOTES.md` entry; `STATUS.md` Stage 1 second pass complete; PLUGINS.md row → 🚧 Stage 1
- [ ] Commit `feat(O-Strata): Stage 1 second pass — 205 live-oscillator parameters, 46 mod destinations, COMPAT-01 re-verified` touching only `plugins/O-Strata/**` and `PLUGINS.md`

## Open for the research phase

1. **Preset folder:** exact `getPresetsDirectory()` path on macOS and whether `initializeFactoryPresets` deletes files it did not write (if not, the execute step removes `…/O-Strata/Factory/` by hand); confirm that `factoryPresetsExist()` is false after the removal so the constructor regenerates.
2. **`createOscParameters` current shape** (`PluginProcessor.cpp:73-135`): confirm the 24 baked pushes are a contiguous block and whether any `p…` raw-value cache or `addParameterListener` in the processor / voice names a removed ID (the first pass said none; re-verify after the fork's later edits).
3. **`WebComboBoxRelay` / `WebComboBoxParameterAttachment` in JUCE 8.0.14:** constructor signatures, `.withOptionsFrom()` placement in `WebBrowserComponent::Options`, and whether an attachment for a Choice parameter with no bound DOM node logs or asserts (the slider relays are silent; confirm for combo).
4. **Exact-log `NormalisableRange`:** the `AudioParameterFloat` constructor that takes a `NormalisableRange` plus `AudioParameterFloatAttributes`, and how param-dump derives `numSteps` / `defaultNorm` for a lambda range (must print continuous / 0.400000).
5. **`FactoryPresets.cpp` minimal shape:** what `FactoryPresetDef` needs for a single `Init` preset (category string, an explicit value map or an empty map meaning "defaults") and whether `Init` must list all 205 IDs or may rely on reset-to-defaults-first (memory `pattern_preset_apply_needs_reset_to_defaults`).
6. **Smoke harness link:** `smoke/build.sh` relinks the `O-Strata-param-dump` objects — confirm the object list still matches after `FactoryPresets.cpp` shrinks and no editor TU is pulled in.
7. **`params.tsv` diff procedure:** the O-Prism v1.24.0 baseline is `plugins/O-Prism/.planning/params.tsv` at 173 rows (spec says unchanged through v1.26.0) — confirm the current file is still that registry before diffing.

## Requirements traceability

| ID | Stage 1 second-pass evidence |
|---|---|
| COMPAT-01 | pluginval strictness 10 VST3 + AU pass, auval pass, on the 205-parameter binary |
| FUNC-09 (IDs) | param-dump diff vs O-Prism v1.24.0: −2 +34, 20 in-place rows, rest byte-identical |
| FUNC-05 (list) | `modSlot0Dst` 46 choices, strings verbatim; new destinations accepted by the matrix (harness routes one) |
| FUNC-10 (smoke) | Scala load + tuning round-trip in the smoke harness (formal verification Phase 2.1) |
