# O-GranulatorInstrument Stage 1 (Foundation): Research

**Researched:** 2026-11-07
**Domain:** JUCE 8.0.15 instrument shell. 261-param APVTS, an instrument with an aux sidechain input (VST3 `kAux`, AU `aumu`), a silent shell, an int64-seed state scaffold, and a render-harness scaffold
**Confidence:** HIGH. Every claim below was either read in-repo or in `~/JUCE` (8.0.15) this session, or comes from a command run this session. Each claim is tagged **VERIFIED** (read or run) or **INFERRED**.
**Inputs read:** CONTEXT.md (D8–D10, gate list), parameter-spec.md (LOCKED), ROADMAP § Stage 1, research/ARCHITECTURE.md (§1, D5/D6, Risks), mockups/v1-CMakeLists.txt, v1-integration-checklist.md, v1-PluginEditor.{h,cpp} (processor API used), the parent's Stage 1 RESEARCH/PLAN/SUMMARY/VERIFICATION, its CMakeLists, Source and harness, MEMORY.md plus the linked files, `troubleshooting/patterns/juce8-critical-patterns.md` §4 and the troubleshooting doc it cites.

<user_constraints>
## User Constraints (CONTEXT.md, binding; not reopened)

- **D8.** `createEditor()` returns `GenericAudioProcessorEditor` until 3.1. Stage 1 wires the WebView plumbing: `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`, `juce_generate_juce_header` and the module lines. **No UI files are copied into `Source/ui/`.** Either the binary data stays consistent with what is served, or it is deferred to 3.1. The harness uses `JUCE_WEB_BROWSER=0` and a guarded `createEditor`.
- **D9.** `withInput ("Sidechain", stereo, true)` + `withOutput ("Output", stereo, true)` on both VST3 and AU. Input layouts: disabled, mono or stereo. Output: stereo only. VST3 uses a note-expression `VST3Extensions` subclass with `getPluginHasMainInput() = false`. **The bus is dropped on VST3 only, and only if pluginval 10 or auval rejects it.** The drop and the failing output are recorded. Never pre-emptively.
- **D10.** Keep the mockup's `polyphony` wording. A note is allowed only in the spec **header**; the body hash must not move.
- **Gate:** VST3 + AU + Standalone build. auval PASS. pluginval 10 on VST3 + AU, with the sidechain enabled **and** disabled. Sidechain-only render (stereo + mono, MIDI present) → bitwise zero. 261 params in the exact order, split 176 / 70 / 15. Golden tests for the 126 destinations and the 18 sources. Range, default and skew checks, including every `setSkewForCentre` row and every interval-1 row. State round trip (int64 seed > INT32_MAX, `<Engine>`, empty `<Slots>`). NE configure check.
- **Out of scope:** DSP / fork (2.1), WebView editor (3.1), factory content / presets / i18n on a live UI (Stage 4).
</user_constraints>

## Project Constraints (CLAUDE.md + memory)
- Trunk on `main`. Commits are path-scoped (`git commit -- plugins/O-GranulatorInstrument PLUGINS.md modules/registry.yaml`). Re-check `git branch --show-current && git status --short` right before every commit (temp-index pattern, `index_git_shared_checkout`). Two foreign changes are present and must **not** be swept into a commit: the untracked `plugins/O-GrainScatter/.planning/improvements/adaptive-grain-length/nielsen-dafx26.txt` and the modified `.claude/agent-memory/*` files. **VERIFIED** (gitStatus at session start).
- Build and install with `./scripts/build-and-install.sh O-GranulatorInstrument`. It clears the AU cache and sweeps both variants. Never tag. Every long command (configure, build, install, pluginval, cold auval) runs with `run_in_background` and logs to the scratchpad (`pattern_executor_watchdog_stall_run_long_commands_in_background`).
- Stage end: a two-step handoff: `/clear`, then `/plugin-plan O-GranulatorInstrument`.

## Summary

1. **D9 is low-risk on the gates that exist.** JUCE's wrapper types the first input bus `kAux` exactly when `getPluginHasMainInput()` returns false (`juce_audio_plugin_client_VST3.cpp:3108-3113`, virtual at `juce_VST3ClientExtensions.h:139`). **VERIFIED.** No repo plugin overrides it yet, and the NE module's `VST3Extensions` is not `final`. So it takes a 3-line plugin-local subclass. **VERIFIED.** The closest precedent, O-Texture (`aumu`, a Sidechain input that is disabled by default), passed `auval -v aumu OuTx OuDv` and **pluginval 10 on VST3** in runs made today. The pluginval run included "Enabling all buses", which turns the input on. **VERIFIED.**
2. **The local gates cannot see `kAux`.** pluginval hosts through JUCE, and JUCE's VST3 hosting code never reads `busType`: 0 hits in all `juce_VST3PluginFormat*` files. The Steinberg validator is not installed, and pluginval printed "Skipping vst3 validator". **VERIFIED.** So the D9 fallback trigger, "pluginval/auval rejects the bus", is very unlikely to fire. The `kAux` typing itself is proven only by the code path, plus the Stage 4 Live/Reaper pass. **INFERRED.**
3. **D8 safe shape: defer the whole UI `juce_add_binary_data` block and its link line to 3.1.** Commented out, they are inert. check-i18n only discovers a plugin that has `Source/ui/public/js/i18n.js` (`check-i18n.js:517-526`). Without it, rule [8] cannot fire. `node scripts/check-i18n.js --plugin O-GranulatorInstrument` → "0 plugins localized", rc 0. **VERIFIED.** Keep a placeholder `Source/PluginEditor.cpp` so `target_sources` stays as the mockup has it.
4. **The CMake mockup lands nearly verbatim.** `OuGI` is unused across all `plugins/*/CMakeLists.txt` and the mockups (only this plugin's own planning files mention it). The 47 shipped codes have no duplicates. The mockup's comments do not trip `resolve-target.sh`: simulated awk → `OuariconGranulatorInstrument`. Every module path exists. **VERIFIED.** Three edits are needed: add the render-harness console block, defer the UI binary data, and add the gate-only sidechain-default option (§6).
5. **Params: reuse the parent's `Def` + `registrationOrder()` shape,** add an `interval` field, and keep the 126-entry dest table as a **literal**. A probe of the spec run today found 261 rows, 176/70/15, indices 0..260 contiguous, the dest table equal to the `b = 2 + 28·(N−1)` formula, 0 host-name mismatches against the Full ID list, and every destination a Float. All 19 `setSkewForCentre` rows reproduce the spec's skew to 5e-5. **VERIFIED.**
6. **State:** port the parent's codec (`copyState()` copy, strip custom children, int64 seed string, `isVoid()` guards). Children are `<Engine>` and `<Slots>`, with no `id` attribute. Test seeds `0x0000000080000001` (> INT32_MAX) and `0xF00DFACECAFEBEEF` (> 2^63). **VERIFIED** pattern (parent `PluginProcessor.cpp:472-541`).
7. **Harness:** copy the parent's console block, `readTsv`/goldens, `MockPlayHead` and state helpers. Add a MIDI script with absolute sample times (O-Strata `main.cpp:359-420` shape). Add `OGI_TEST_HOOKS`-gated `loadSlotSync` stubs. The sidechain-zero render needs a vacuity guard.
8. **The shared build needs an explicit re-configure** before the new folder exists to ninja. The root uses `file(GLOB …/plugins/*)` without `CONFIGURE_DEPENDS` (`CMakeLists.txt:47-58`), and `build-and-install.sh` skips configure when `build/` exists (`:262-271`). **VERIFIED.**

---

## Q1. Instrument with an aux input bus (D9)

### 1a. The JUCE seam
- `virtual bool getPluginHasMainInput() const { return true; }` lives in `struct VST3ClientExtensions` at `~/JUCE/modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h:133-139`. Doc: "check whether the first input bus should be designated as kMain or kAux… All other input buses will always be designated kAux." **VERIFIED.**
- Wrapper `getBusInfo` is at `juce_audio_plugin_client_VST3.cpp:3086-3134`. For `dir == kInput` and index 0, it calls `pluginInstance->getVST3ClientExtensions()` and returns `getPluginHasMainInput() ? kMain : kAux` (`:3108-3113`). With `JucePlugin_IsSynth`, every **output** bus is `kMain` (`:3120-3121`). `flags = kDefaultActive` comes from `isEnabledByDefault()` (`:3128-3129`). **VERIFIED.**
- `virtual VST3ClientExtensions* getVST3ClientExtensions();` is declared in `juce_audio_processors_headless/processors/juce_AudioProcessor.h:1291` with no format guard, so it compiles in the processor console too. **VERIFIED** (O-Strata's render-test with NE is built: `build/plugins/O-Strata/*render-test_artefacts`).
- Module class: `class VST3Extensions : public juce::VST3ClientExtensions` (`modules/tuning/note-expression/cpp/NoteExpression.h:178-241`). It is not `final`, has a virtual dtor, and overrides only `queryIEditController` and `onVst3RawEvent`. Nothing in the module overrides `getPluginHasMainInput`. **VERIFIED.**
- Consumers hold the module class directly. None subclasses it: O-Lyrica `PluginProcessor.h:134` (`getVST3ClientExtensions() override { return &vst3Extensions; }`) and `:241` (member). O-Bells `:111`/`:161`, O-Strata `:117`/`:356`, and 9 more (`grep -l getVST3ClientExtensions plugins/*/Source`). O-Lyrica drains at the top of `processBlock` (`PluginProcessor.cpp:813` `vst3Extensions.drainAndUpdate()`). **VERIFIED.**

**Recommended shape (plugin-local, `PluginProcessor.h`):**
```cpp
#include "NoteExpression.h"   // modules/tuning/note-expression (ouaricon_add_module)
// D9: the only input bus is the Record-in sidechain → VST3 kAux (juce_VST3ClientExtensions.h:139).
struct GiVst3Extensions final : Ouaricon::NoteExpression::VST3Extensions
{
    bool getPluginHasMainInput() const override { return false; }
};
// in the processor:
juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
GiVst3Extensions vst3Extensions;
```
Call `vst3Extensions.drainAndUpdate()` at the top of the Stage 1 `processBlock`. That keeps the raw-event queue cycling, as O-Lyrica does. The queue is reserve-capped at 64 and drops overflow (`NoteExpression.h:186-198`), so skipping the call would not grow memory either. **VERIFIED** (code). The parity rationale is **INFERRED**.

### 1b. O-Texture precedent (the only IS_SYNTH plugin with `withInput`)
- `BusesProperties().withInput ("Sidechain", stereo(), false).withOutput ("Output", stereo(), true)` (`plugins/O-Texture/Source/PluginProcessor.cpp:64-66`). Note that it is **disabled** by default. The predicate is "stereo out && (in disabled || in stereo)" (`:392-397`). There is no VST3ClientExtensions, so its VST3 input is `kMain`. **VERIFIED.**
- History: the `aumu` MIDI input is load-bearing. Without it auval fails `MusicDeviceMIDIEventList` -4 (`CHANGELOG.md:490-494`). Earlier releases ran pluginval at strictness 5 only (`:498`). The CR-02 fix added the predicate to stop >stereo OOB (`:519-523`). **VERIFIED.**
- **Run today:** `auval -v aumu OuTx OuDv` → AU VALIDATION SUCCEEDED. It reports an input-scope bus "Sidechain" with a 2 ch default format, even though it is disabled by default in JUCE. Channel capabilities `[2, 2]`. Render tests use Input Format 2 ch. "Test MIDI" PASS. Log at `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/cdae8202-5c25-4d9d-8a1f-5e18f9b43b11/scratchpad/auval-OuTx.txt`. **VERIFIED.**
- **Run today:** pluginval 1.0.4 `--strictness-level 10 --skip-gui-tests` on `O-Texture-dev.vst3` → SUCCESS. Bus tests: "Listing available buses" (Inputs: Stereo; main in 0 ch, out 2 ch), "Enabling all buses", "Disabling non-main busses", "Restoring default layout". Log at `…/scratchpad/pv-OuTx-vst3.txt`. **VERIFIED.**

### 1c. AU `aumu` with an input bus
- The wrapper derives from `ausdk::MusicDeviceBase` with input/output bus counts from `getBusCountForWrapper` (`juce_audio_plugin_client_AU_1.mm:131-154`). Under `JucePlugin_IsSynth`, the input bus count is not host-settable (`:301-303`). **VERIFIED.**
- `AudioUnitHelpers::getAUChannelInfo` (`juce_audio_processors_headless/format_types/juce_AU_Shared.h:395-528`) probes `checkBusesLayoutSupported` over every 1..N-channel set for input × output. It cannot express "input disabled". **VERIFIED.** For the D9 predicate, expect reported capabilities `[1, 2] [2, 2]` (the parent's analogous effect reported `[1, 1] [1, 2] [2, 2]`). **INFERRED.** Record the real value at the first auval and freeze it.
- **Logic caches AU I/O per VERSION** (`critical_logic_caches_au_io_config_per_version`). The AU input set must be final before Logic first sees 0.1.0. D9's fallback touches VST3 only, which preserves that. **VERIFIED** (memory). Logic offering a Side Chain menu on an *instrument* remains a Stage 4 DAW item (ARCHITECTURE:617 "developer reports conflict"). **INFERRED.**
- An unrouted AU sidechain arrives as silence, not as a disabled bus (`critical_au_hands_unrouted_sidechain_silence`). That is irrelevant to the Stage 1 shell, which clears everything. It is binding for 2.x Record-in (D6). **VERIFIED** (memory).

### 1d. The "missing input" warning in juce8-critical-patterns §4 is mis-attributed
`troubleshooting/patterns/juce8-critical-patterns.md:93-111` says "Adding input bus to instruments causes 'missing input' errors". The source doc it cites (`troubleshooting/runtime-issues/vst3-bus-config-effect-missing-input-Shake-20251106.md`) is about an **effect** that had no input bus and no MIDI event bus ("plugin has instrument category, but no valid event input bus"). `NEEDS_MIDI_INPUT TRUE` already covers that case. **VERIFIED** (read). Treat §4 as low-relevance for this plugin. **INFERRED.**

### 1e. Concrete gate risks and how a D9 fallback is detected
| Risk | Gate | Likelihood | Evidence |
|---|---|---|---|
| pluginval "Listing / Enabling / Disabling non-main / Restoring default layout" with a default-enabled input | pluginval 10 VST3 + AU | Low | O-Texture's bus tests pass; JUCE host ignores busType (§Summary 2). **INFERRED** |
| Audio-processing tests feed a 2-ch input; output must be finite | pluginval 10 | Nil for a shell that clears all channels | **INFERRED** |
| auval "Input/Output Channel Handling" with 1-2 | auval | Low (O-Granulator passed 1-2 as `aufx`) | **INFERRED** |
| `kAux` mistyped (e.g. extensions not returned) | **none locally** | Undetected | no Steinberg validator. **VERIFIED** |
| Live/Reaper/Cubase reject or mis-route an instrument aux input | Stage 4 DAW pass | Unknown | ROADMAP:86. **VERIFIED** (plan) |

**Fallback detection rule (attribution):** a D9 drop is justified only when a failing pluginval or auval row names a bus or layout test, or fails in audio processing with the input active, **and** the same run passes on an A/B build with the bus removed (compile-time switch, §6). Paste both outputs into SUMMARY-1 and CONTEXT. A failure that also reproduces without the bus is not a D9 trigger. **INFERRED** (method).

**Fallback code seam (do not enable pre-emptively):** build the buses in a static `makeBuses()`. The wrapper type is already known when the constructor runs: `createPluginFilterOfType` sets `PluginHostType::jucePlugInClientCurrentWrapperType` before `::createPluginFilter()` (`juce_audio_plugin_client/detail/juce_CreatePluginFilter.h:40-45`; `PluginHostType::getPluginLoadedAs()` at `juce_audio_processors/utilities/juce_PluginHostType.h:250`). So the fallback is a single `if (getPluginLoadedAs() == wrapperType_VST3)` inside `makeBuses()`. **VERIFIED.**

---

## Q2. CMake

### 2a. Mockup vs parent (`v1-CMakeLists.txt` vs `plugins/O-Granulator/CMakeLists.txt`)
| Item | Mockup | Parent | Verdict |
|---|---|---|---|
| Target / code | `OuariconGranulatorInstrument`, `OuGI` (`:41-44`) | `OuariconGranulator`, `OuGn` (`:28-31`) | OK. `grep -rIil ougi` hits only this plugin's CONTEXT, checklist and mockup; the 47 `PLUGIN_CODE`s have no duplicates. **VERIFIED.** AU triple will be `aumu OuGI OuDv`. **INFERRED** (IS_SYNTH → aumu, as O-Texture) |
| IS_SYNTH / MIDI | `TRUE` / `NEEDS_MIDI_INPUT TRUE` (`:48-49`) | FALSE / FALSE | Required: an aumu must accept MIDI (O-Texture CHANGELOG:490-494). **VERIFIED** |
| Modules | +note-expression, +webview-drop-streaming (`:75-98`) | 5 modules (`:66-81`) | Paths and versions are below |
| `juce_add_binary_data` UI | `:131-150`, needs `Source/ui/public/{index.html, js/i18n.js, js/juce/*.js}` | same, files present | **Defer to 3.1** (2c) |
| Factory-source binary data | commented (`:152-164`) | — | Leave commented (Stage 4) |
| Test block | param-dump only (`:179-184`) | param-dump + render-harness + `GOLDEN_DIR` + `OGRAN_TEST_HOOKS` (`:149-168`) | **Add the harness block** (Q5) |
| Comment trap | header avoids the paren form (`:9-11`); the only paren match is the real call `:41` | — | Simulated `resolve-target.sh` awk on the mockup → `OuariconGranulatorInstrument`. **VERIFIED** |

### 2b. Module registry + on-disk versions
| Module | registry.yaml | module.yaml | Notes |
|---|---|---|---|
| grain-engine | 1.1.0 (`:475-477`) | 1.1.0 | header-only, `synthesis/` |
| pitch-collections | 1.0.0 (`:418-420`) | 1.0.0 | header-only |
| reverb-unit | 1.2.0 (`:615-617`) | 1.2.0 | do **not** call `ReverbUnit::addParameters` (it adds `rev_*`, `ReverbUnit.h:55-61`), or the count ≠ 261 |
| preset-manager | 1.1.1 (`:127-129`) | 1.1.1 | not instantiated in Stage 1 |
| eb-garamond | 1.0.0 (`:654-656`) | 1.0.0 | no cpp/js, tracking only |
| note-expression | **1.1.1** (`:373-375`) | **1.1.2** | registry drift (non-blocking); the build uses the on-disk 1.1.2 |
| webview-drop-streaming | 1.0.0 (`:62-64`) | 1.0.0 | header uses `WebBrowserComponent::NativeFunction` (`WebViewDropStreaming.h:199-202`), so it must be **editor-only** and never included by the processor (the console has `JUCE_WEB_BROWSER=0`) |
All **VERIFIED.** `ouaricon_add_module` searches `core persistence metering tuning modulation synthesis effects ui` (`modules/cmake/OuariconModules.cmake:34-48`). It adds `cpp/*.{cpp,h}` to SharedCode, `cpp/<fmt>/` to the format target only (`:57-101`; NE's `cpp/vst3/NoteExpression_VST3.cpp` → `_VST3`), copies `js/*.js` into `Source/ui/public/modules/` (`:104-118`; gitignored at `.gitignore:27`), and includes `module.cmake` (`:121-124`). **VERIFIED.**

### 2c. NE configure check (`JUCE-NE-PATCH`)
`modules/tuning/note-expression/module.cmake`: it resolves the JUCE root (`$ENV{JUCE_DIR}` → `/Users/taylorbrook/JUCE`, `:13-19`). It prefers the pre-8.0.14 header path and otherwise uses the headless path (`:34-40`; 8.0.15 has only the headless one). For both `juce_VST3ClientExtensions.h` and `juce_audio_plugin_client_VST3.cpp`, it does `file(READ)` + `string(FIND "JUCE-NE-PATCH")`, with a `FATAL_ERROR` if missing (`:43-57`). It prints `[note-expression] JUCE-NE-PATCH markers verified in …` (`:59`) and adds a per-consumer `install(SCRIPT …)` for the Dorico library (`:75-81`). Markers are present at `juce_VST3ClientExtensions.h:64` and `juce_audio_plugin_client_VST3.cpp:3592`. JUCE is `project(JUCE VERSION 8.0.15)` and the pin is `.github/juce-version.txt` = 8.0.15. **VERIFIED.** Gate evidence: the configure succeeds (a missing marker is fatal), and the configure log contains the `:59` line after `[Ouaricon] Adding module 'note-expression'` for this target.

### 2d. D8 WebView plumbing without UI files: the exact safe shape
Keep everything from the mockup **except** `:131-150`:
- Keep `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE` (`:52-53`), `juce::juce_gui_extra` (`:114`), `juce_generate_juce_header` (`:121`), `JUCE_WEB_BROWSER=1` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (`:166-174`), and all 7 `ouaricon_add_module` lines.
- Keep `target_sources … Source/PluginEditor.cpp` (`:61-65`) with a **placeholder TU** (comments only, or `#include "PluginProcessor.h"` and nothing else). Do **not** add `Source/PluginEditor.h` with a class.
- **Comment out** the UI `juce_add_binary_data(…_UIResources …)` block and its `target_link_libraries(… _UIResources)` (`:131-150`) under a `# Phase 3.1:` marker. Write the comment text so it never contains the plugin-target call followed by `(`. Commented lines are invisible to both gates: check-i18n strips `#` lines before matching (`check-i18n.js:1173-1175`), and serve-ui does the same (`serve-ui.js:108-112`). **VERIFIED.** Without the defer, the build fails: the four `Source/ui/public/*` SOURCES do not exist (parent RESEARCH §3, "will not configure or build until …"). **VERIFIED** (parent record).
- Why rule [8] cannot fire: `discover()` only includes a plugin whose `<uiRoot>/js/i18n.js` exists (`check-i18n.js:517-526`). The configure-time copies land in `Source/ui/public/modules/`, not `js/`. Today: `--plugin O-GranulatorInstrument` → "0 plugins localized", rc 0. Repo-wide: ALL CHECKS PASS, 47 plugins, in 1.1 s. **VERIFIED.**
- Side effects: `boot-all-uis` lists every plugin folder that has a CMakeLists (`serve-ui.js:92-99`). For this plugin it will record "no UI root" (`serve-ui.js:391`). That is **report-only**: exit 0 when the run completes (`boot-all-uis.js:30-40`). **VERIFIED.** Treat it as expected until 3.1.
- **Rejected alternative:** a binary-data target holding only the module-sourced assets (preset-manager.js, webview-drop-streaming.js, eb-garamond css/woff2). They exist and would not trigger [8], but nothing consumes them and boot-all-uis would flag "unmapped embed". It adds no value. **INFERRED.**

### 2e. Configure blast radius
The root auto-discovers `plugins/*` that contain a CMakeLists, with no `CONFIGURE_DEPENDS` (`CMakeLists.txt:47-58`). A broken plugin CMake therefore breaks configure for every concurrent session, and a new folder needs an explicit `cmake -S . -B build`. **VERIFIED.** Land `CMakeLists.txt` and every file it names (`Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp`, `tests/render-harness/main.cpp`, goldens) in the **same** commit, and run the configure before committing. `ouaricon_add_processor_console` FATAL_ERRORs if `main.cpp` is missing (`ParamDump.cmake:75-78`). **VERIFIED.**

---

## Q3. Parameter header (`Source/GiParamIds.h`)

**Copy the parent's shape** (`plugins/O-Granulator/Source/GranulatorParamIds.h`): the `Def` struct (`:47-57`), choice-list functions, per-group `std::array<Def,N>`, `registrationOrder()` (`:227-264`), `allIdsInOrder()`, a literal dest table (`:282-315`) and `modDestChoices()` built from the registration names (`:318-330`). The layout loop is `createParameterLayout()` (`PluginProcessor.cpp:191-230`), with `ParameterID{id,1}`, `NormalisableRange (start,end,interval,skew)` + `setSkewForCentre` when `centre > 0`, and the `s1_enable` special case. **VERIFIED.** Changes for this plugin:
- **Add `float interval = 0.0f` to `Def`.** It is 1 for `polyphony`, `pitch_bend_range` and `sN_root_note` (×4) = 6 rows. **VERIFIED** (spec probe). All six have start ≥ 0, so the stepped-zero trap (`pattern_stepped_range_snap_makes_zero_default_inexact`) cannot bite. With interval 1.0f the snap `start + floor(v−start+0.5)` returns exact integers, so `get()` at the defaults 8 / 2 / 48 is exact. **INFERRED** (formula).
- **Groups in contract order** (spec `:357-621`): global 9 → amp 5 → `menvN_` 4×2 → `sN_` 38×4 → rec 7 → feedback 5 → reverb 3 → `lfoN_` 5×4 → `macroN` ×4 → `modM_` 3×16 = 261. Host names come from the Full ID list's Name column. The parent's naming convention reproduces them (`"S" + String(n) + " · " + name`, `"Mod Env " …`, `"LFO " …`, `"Mod " …`). **VERIFIED** (spec).
- **Type split 176 / 70 / 15** (spec probe). Floats include the 6 interval-1 rows. Choices: 3 global + 5×4 stream + 3 rec + 3×4 LFO + 2×16 mod = 70. Bools: `mpe_enable`, `freeze`, `sN_enable/mute/solo` ×4, `rec_arm` = 15. Only `s1_enable` defaults On. **VERIFIED.**
- **Dest table: a literal `kModDestIds[126]` + `kModDestTableVersion = 1`.** Index 0 `None`, 1 `output_gain`, per stream `b = 2 + 28·(N−1)` = parent's 24 suffixes then `scan_rate, loop_start, loop_end, key_track`, 114–121 fb+reverb, 122–125 `lfoN_rate`. The spec probe found the formula equal to the table, 0 host-name mismatches, and all 125 non-None entries Float. **VERIFIED.** The harness also cross-checks the literal against the formula, as a third leg.
- **Source table: a literal `kModSources[18]`** (`None, LFO 1–4, Mod Env 1–2, Velocity, Key, Note Random, Mod Wheel, Pressure, Slide, Bend, Macro 1–4`; spec `:224-245`). Append-only. Optionally carry the per-source class (per-voice / global / lfo-mode) beside it for 2.5. **VERIFIED** (spec).
- **No lambda ranges** (`critical_lambda_normalisable_range_invisible_to_webview_slider_frontend`). Non-ASCII host names are built with `+` / `<<`, never `juce::String (const char*)` on a `·` literal (`critical_juce_string_char_ctor_is_ascii_only`; the parent's concatenation passed G3). Never use an ID-derived C++ identifier named `begin` / `end`. Every choice has ≥ 2 entries. **VERIFIED** (memory + parent).

**Trap rows (spec probe output):**
| Class | Rows (expanded count) | What the harness checks |
|---|---|---|
| `setSkewForCentre` (19) | amp_attack/decay (300), amp_release (500), menvN_attack/decay (300) ×2, menvN_release (500) ×2, sN_filter_cutoff (632.4555) ×4, sN_ring_freq (316.2278) ×4, fb_low_cut (200), fb_high_cut (4472.136) | `range.skew` vs the spec skew (0.19767 / 0.18790 / 0.19889 / 0.24562 / 0.28906 / 0.40781) ±1e-4. All 19 reproduced to 5e-5 today. **VERIFIED** |
| raw skew (14) | glide 0.3, sN_size 0.3 ×4, sN_density 0.3 ×4, rec_length 0.5, lfoN_rate 0.3 ×4 | `range.skew == spec` exactly |
| interval 1 (6) | polyphony 1…16 (8), pitch_bend_range 0…48 (2), sN_root_note 0…127 (48) ×4 | `interval == 1`, `get() == default` exact |
| negative start, interval 0 (43) | output_gain, sN_level/pan/scan_rate/root_fine/pitch/window_skew ×4, rec_threshold, rec_gain, modM_depth ×16 | `interval == 0` (keeps 0 defaults exact) |
| inexact skewed defaults | every skewed row whose default is not an endpoint | normalised default == `convertTo0to1(spec)` exactly; engineering value within 1e-6 of span (parent G4, worst 5.1e-8). Never derive unity gain from `get()` (parent R4) |
| bools | `s1_enable` On, the other 14 Off | `getDefaultValue() > 0.5` == spec |

**Golden strategy (spec-derived, never code-derived).** Adapt `plugins/O-Granulator/tests/golden/extract_spec.py`:
- The spec is the only input. Names are in the Full ID list, so there is no YAML dependency.
- Replace U+2212 with `-`. The spec contains 13 of them, plus 359 U+00B7 middle dots, 0 CR and 0 NBSP. **VERIFIED.**
- Widen the row regex to `Float(?: \(interval 1\))?`, add the `menvN_` template, apply `+28·(N−1)` to the per-stream Mod-dest column, and skip `alias →` rows.
- Write `params.tsv` (add an `interval` column), `mod-dests.tsv` (idx, id, host name, class) and `mod-sources.tsv` (idx, name, class), with `newline='\n'` as the parent does (`extract_spec.py:169-176`).
- Exit 1 on any cross-check failure.

A byte-compared dump written from C++ must use `replaceWithText (t, false, false, "\n")` (`pattern_juce_replacewithtext_defaults_crlf`). The repo has no `.gitattributes` eol rule (only `PLUGINS.md merge=union`). **VERIFIED.**

**Spec hash check (read-only; run before and after Stage 1):** `shasum -a 256 parameter-spec.md` → `b6bc70e3…751b`. The body after the `LOCKED BODY` marker line hashes to `c5553e0c…3191`. Both match STATUS `contract_checksums`. **VERIFIED.** If planning adds the D10 header note, the **whole-file** hash in STATUS changes and must be re-recorded by whoever owns STATUS. The body hash must stay the same.

---

## Q4. State scaffold

Port the parent's codec (`plugins/O-Granulator/Source/PluginProcessor.cpp:440-541`). **VERIFIED.**
- `getStateInformation`: `auto state = parameters.copyState();` → strip the custom children (`Engine`, `Slots`; reserve `CustomScale` and `DegreeMasks` in the strip list for 2.2) → set the `uiLanguage` code string → append `<Engine seed="String((int64) seed)" stateVersion="1" modDestTableVersion="1"/>` and `<Slots/>` → `copyXmlToBinary`. Never mutate the live tree.
- `setStateInformation`: check `hasTagName (parameters.state.getType())`. Read `Engine` and `Slots` before stripping, strip them from the tree, then `replaceState`. Reset non-param state to defaults (the seed is kept, because there is no default seed). Then parse with `isVoid()` guards. Seed: `containsOnly ("-0123456789")` → `(uint64) getLargeIntValue()` (parent `:552-555`). The ints come back as strings (`critical_valuetree_xml_roundtrip_loses_type`).
- **No `id` attribute on `<Engine>` or `<Slots>`.** `updateParameterConnectionsToChildTrees` calls `setNewState` on every **top-level** child and looks up `getProperty (idPropertyID)` as a param ID (`juce_AudioProcessorValueTreeState.cpp:405-437`). `<Slot>` children (2.1) use `index`, never `id`. **VERIFIED.**
- A param with no `PARAM` child keeps its current value (`:425-436` appends a fresh child). That doesn't matter at 0.1.0; Stage 4 presets reset to defaults first. **VERIFIED.**
- Seed member: `std::atomic<uint64_t>`, seeded in the ctor from `Random::getSystemRandom().nextInt64()`; public `getSeed()` / `setSeed (uint64_t)` (parent `:36`). **VERIFIED.**
- **`uiLanguage` placement:** the spec ("properties of the APVTS state tree"), the integration checklist ("a non-parameter `uiLanguage` property on `parameters.state`", `v1-integration-checklist.md:52`) and the parent (root attribute) all put it on the **root**. CONTEXT lists it under `<Engine>`. Recommend the root, to match the Stage 3 contract. The round-trip test covers it either way (Open Q1). Names to match the editor template: `std::atomic<int> uiLanguage`, `static languageCode/languageIndex`, public `parameters` (`grep processorRef.` in `v1-PluginEditor.cpp`). **VERIFIED.**

**Round-trip test design (harness rows S1–S5):**
- **S1a.** Seed `0x0000000080000001` (> INT32_MAX, < 2^63: catches an int32 truncation). Set non-defaults through the host-normalised path on one param per class: `polyphony = 12` (interval 1), `amp_release` (setSkewForCentre), `glide` (raw skew), `mod16_dest = 125` (last dest), `s4_enable = On`, `modM_depth = −37`. Fresh instance → `setState` → `getSeed()` equal, all 261 `getValue()` bitwise equal, `uiLanguage` equal.
- **S1b.** Seed `0xF00DFACECAFEBEEF` (> 2^63). The XML attribute is the negative int64 string, and it reads back equal.
- **S2.** XML shape: exactly one `<Engine>` with `stateVersion="1"` and `modDestTableVersion="1"`, exactly one `<Slots>` with 0 children, neither carrying `id`, `uiLanguage="zh-Hans"` on the root, and 261 `PARAM` children.
- **S3.** Two reopens with a change between (A → B → change → C): still one of each child, nothing stale (`critical_preset_manager_stale_customstate_child`).
- **S4.** A blob without `<Engine>`: the seed is kept and uiLanguage is unchanged. Tolerant uiLanguage: `"1"` → fr.
- **S5.** A wrong root tag or a garbage seed (`"12ab"`, `""`) leaves state unchanged.

---

## Q5. Render harness

**Copy from the parent:**
- The CMake block (`plugins/O-Granulator/CMakeLists.txt:149-168`): `ouaricon_add_processor_console (… tests/render-harness/main.cpp render-harness)` → `O-GranulatorInstrument-render-harness`, plus `target_compile_features (… cxx_std_20)` (the console does not inherit it) and `GOLDEN_DIR` + `OGI_TEST_HOOKS=1`.
- What the console does (`scripts/param-dump/ParamDump.cmake:67-271`): it derives every `JucePlugin_*` macro from the target, including `JucePlugin_IsSynth=1` and `WantsMidiInput=1` (`:116-133`, `:197-215`). It excludes `PluginEditor*` TUs (`:156-159`), sets `JUCE_WEB_BROWSER=0` (`:200`) and drops `*UIResources` links (`:226`). NE's SharedCode `NoteExpression.cpp` is compiled in, as O-Strata already does. **VERIFIED.**
- The `#ifndef GOLDEN_DIR #error` guard, `row()`/Tally, `readTsv()`/`goldenParams()` (`main.cpp:67-145`), `MockPlayHead` verbatim, including the v2 segment script and `advance()` (`:151-300`), `setAllViaHostNormalised` (`:319`), and the state helpers `stateOf`/`load`/`xmlOf`/`countChildren` (`:589-614`). **VERIFIED.**
- `createEditor()`: `#if JUCE_WEB_BROWSER` (Stage 3: `new GranulatorInstrumentEditor`) `#else` Generic. In Stage 1 both branches return Generic (`pattern_render_harness_breaks_on_webview_editor`; parent `:337-344`). **VERIFIED.**

**Add (new for an instrument):**
- **MIDI script:** `struct MidiEv { juce::int64 t; juce::MidiMessage m; }`. Sort by absolute sample time, and per host block add `e.t − blockStart` for `blockStart ≤ t < blockStart + n` (O-Strata `main.cpp:359-420` shape). The time must be absolute, never block-relative (`pattern_block_relative_scheduling_drops_cross_block_events`). Builders: `chord`, `noteOff`, `bend`, `cc (74|64|1)`, `channelPressure`, `polyAT`, `mpeMember (ch 2–16)`, `allNotesOff`. Pre-size the `MidiBuffer` (`ensureSize`). **VERIFIED** (precedent) / **INFERRED** (builder set).
- **Seed injection:** `setSeed()` before `prepareToPlay` (a public API that the codec uses too). **INFERRED.**
- **`loadSlotSync` stubs** (declared under `#if OGI_TEST_HOOKS`, so the plugin and param-dump compile them out):
  ```cpp
  // Harness only. Decode/resample + build the tape for slot 0…3 on the CALLING thread and publish it
  // as if the loader worker had finished. Stage 1 stub: returns false, error = "loadSlotSync: stub until 2.1".
  bool loadSlotSync (int slot, const juce::File& file, juce::String& error);
  bool loadSlotSync (int slot, const juce::AudioBuffer<float>& pcm, double sourceRate, const juce::String& name, juce::String& error);
  ```
  The harness asserts the stub returns `false` with a non-empty error, which proves the symbol links. **INFERRED** (design; slot base 0 matches the `rec_target` choice index; the page and state use 1…4, see Open Q5).
- **NE raw-event injection** (2.5) can call `vst3Extensions.onVst3RawEvent(...)` directly. There is no Stage 1 row.

**Stage 1 rows** (all must PASS → exit 0):
- **G1–G6** (count, order, types 176/70/15, hint 1, names, ranges with interval/skew, defaults, choices, bools).
- **D1–D4 + D5**: literal == golden (126), all Float, `modM_dest` choices == `None` + names, version 1, literal == the `b = 2 + 28(N−1)` formula.
- **SRC1** `modM_source` choices == golden (18) on all 16 slots.
- **B1** layouts: accept {disabled, mono, stereo} → stereo; reject out mono / disabled / 5.1, in 5.1 / LCR. Default: one input "Sidechain" enabled stereo, one output stereo.
- **B3** latency 0 after prepare.
- **R1** sidechain-zero render (below).
- **S1–S5**.
- **H1** `loadSlotSync` stub.
- **H2** `getVST3ClientExtensions() != nullptr && ! getPluginHasMainInput()`. This is the only automated `kAux` evidence available.

**R1: sidechain-only render, output bitwise zero:**
- Layouts: (stereo SC → stereo), (mono SC → stereo), and the control (SC disabled → stereo). Every layout has a 2-ch buffer, so the buffer width says nothing (`pattern_stereo_in_mono_out_buffer_width_lies`).
- Stimulus: seeded noise at 0.7 plus a full-scale 997 Hz sine on every input channel. For mono SC, write `12345.0f` into ch 1 as host garbage (the parent's `main.cpp:517` trick). Write through `copyFrom` / `setSample` so that `isClear` is false (`AudioBuffer::clear()` is a no-op when `isClear`, `juce_AudioSampleBuffer.h:587-600`). **VERIFIED.**
- MIDI script: a 4-note chord at t=0, notes on block boundaries ±1, bend, CC74, channel pressure, sustain on/off, MPE member notes with `mpe_enable` On, then note-offs.
- Blocks {1, 32, 441, 512, 4096, seeded variable} × {no playhead, playing `MockPlayHead`} × {defaults, every param host-normalised random (seeded)}.
- **Verdict:** every sample of every `buffer.getNumChannels()` channel equals `+0.0f` bitwise (`memcmp` against a zeroed buffer, so a `-0.0f` fails), and every sample is finite.
- **Vacuity guards:** (1) assert that the input RMS before `processBlock` is > −20 dBFS on each SC channel; (2) the comparator must FAIL when it is run on an unprocessed copy (`pattern_gate_stimulus_below_threshold_is_vacuous`).
- Keep R1 as a permanent row. From 2.1 it holds with empty slots, and it is the QUAL-02 "sidechain never reaches output" check.

**Stage 1 `processBlock`:**
```cpp
juce::ScopedNoDenormals noDenormals;
vst3Extensions.drainAndUpdate();       // NE queue (no-op outside VST3)
// MIDI is consumed: no action in Stage 1 (the wrapper discards output MIDI; producesMidi() == false).
for (int ch = 0; ch < buffer.getNumChannels(); ++ch)   // outputs AND input-only channels; never assume the width
    buffer.clear (ch, 0, buffer.getNumSamples());
```
Stage 2 reads the SC via `getBusBuffer (buffer, true, 0)` **before** clearing, with its channel count decided in `prepareToPlay` from the negotiated layout (`critical_audiochannelset_is_a_bitset_not_an_order`, `pattern_standalone_canonical_channelset_oob`). `setLatencySamples (0)` goes in `prepareToPlay` only. `getTailLengthSeconds()` returns 0 for now. **INFERRED** (shape); the `clear` semantics are **VERIFIED**.

---

## Q6. Gate commands (repo root; BG = run with `run_in_background` and log to the scratchpad)

| # | Command | BG? |
|---|---|---|
| 0 | `shasum -a 256 plugins/O-GranulatorInstrument/.planning/parameter-spec.md` (== STATUS) ; `grep -h PLUGIN_CODE plugins/*/CMakeLists.txt \| grep -c OuGI` (== 0 before landing) | no |
| 1 | `python3 plugins/O-GranulatorInstrument/tests/golden/extract_spec.py` | no |
| 2 | `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release > $LOG 2>&1` then `grep -A4 "Adding module 'note-expression'" $LOG \| grep "JUCE-NE-PATCH markers verified"` | **BG** |
| 3 | `ninja -C build OuariconGranulatorInstrument_VST3 OuariconGranulatorInstrument_AU OuariconGranulatorInstrument_Standalone O-GranulatorInstrument-render-harness O-GranulatorInstrument-param-dump` | **BG** |
| 4 | `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness` (exit 0) | no (< 1 min) |
| 5 | `…/O-GranulatorInstrument-param-dump_artefacts/Release/O-GranulatorInstrument-param-dump > plugins/O-GranulatorInstrument/.planning/params.tsv` | no |
| 6 | `./scripts/build-and-install.sh O-GranulatorInstrument` (VST3 + AU only, `:280`; Standalone stays stale, `pattern_build_install_skips_standalone_stale_ui`) | **BG** |
| 7 | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-GranulatorInstrument-dev.vst3` ×2–3 (the fuzz reseeds) | **BG** each |
| 8 | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate-in-process ~/Library/Audio/Plug-Ins/Components/O-GranulatorInstrument-dev.component` | **BG** |
| 9 | `auval -v aumu OuGI OuDv` (the first run after an install is a cold rescan: 83 s – 17 min) + negative control `auval -v aumu ZZZZ OuDv` (rc ≠ 0). Never `auval -a` (SIGABRTs here). Record "Reported Channel Capabilities" | **BG** (cold) |
| 10 | Standalone smoke: `bash -c 'A=build/plugins/O-GranulatorInstrument/OuariconGranulatorInstrument_artefacts/Release/Standalone/O-GranulatorInstrument-dev.app/Contents/MacOS/O-GranulatorInstrument-dev; "$A" & p=$!; sleep 6; kill -0 $p && kill $p && echo ALIVE'` | **BG** |
| 11 | `node scripts/check-i18n.js` → "ALL CHECKS PASS — 47" (unchanged) | no (~1 s) |
| 12 | `node scripts/boot-all-uis.js --plugin O-GranulatorInstrument`: informational only; expect "no UI root" until 3.1 (report-only, exit 0) | **BG** (Playwright) |
| 13 | Grep gates: `grep -rn setValueNotifyingHost plugins/O-GranulatorInstrument/Source` (0); `grep -n "juce_add_plugin *(" plugins/O-GranulatorInstrument/CMakeLists.txt` (exactly 1, the real call); `grep -n WebViewDropStreaming plugins/O-GranulatorInstrument/Source/PluginProcessor.*` (0) | no |
| 14 | **Disabled-SC arm** (§6 below): `cmake -S . -B build -DOGI_GATE_SC_DEFAULT_OFF=ON` → step 6 → steps 7–9 → `-DOGI_GATE_SC_DEFAULT_OFF=OFF` → step 6 → warm auval | **BG** each |

All artefact paths follow the parent's layout (`build/plugins/O-Granulator/{OuariconGranulator,O-Granulator-render-harness}_artefacts/Release/…`). **VERIFIED.** `OUARICON_BUILD_TESTS:BOOL=ON` is already set in `build/CMakeCache.txt:631`. **VERIFIED.** Tools on this machine: cmake 4.2.1, ninja 1.13.2, node 24.19.0, python 3.14.2, Xcode 26.3, pluginval 1.0.4, auval 1.10.0, a Playwright chromium cache. **VERIFIED.**

### 6. Running pluginval with the sidechain "disabled"
pluginval cannot pick a layout from the CLI, and its bus tests only *enable* buses or disable *non-main* ones. JUCE's host treats input 0 as main. **VERIFIED** (O-Texture log + help). Recommended approach: a gate-only CMake option `OGI_GATE_SC_DEFAULT_OFF` (default OFF) → compile definition `OGI_SC_DEFAULT_ACTIVE=0`, used as the `withInput` default-active flag. The shipping binary is unchanged. Run steps 7–9 on the variant: its audio tests run with the input disabled, and "Enabling all buses" re-enables it, so both states are covered. Then rebuild and reinstall the shipping build and confirm it with a warm auval. **Do not open Logic while the variant is installed**, because Logic caches I/O per VERSION. An env-var switch read in the ctor would avoid the rebuild, but it puts hidden behaviour in the release binary, so it is rejected. A second option `OGI_GATE_NO_SIDECHAIN` gives the A/B for D9 attribution (§1e). Add it only if a gate fails. **INFERRED** (method).

---

## Pitfalls

| Pitfall | Mitigation | Source |
|---|---|---|
| New folder invisible to ninja | Explicit `cmake -S . -B build`; land CMake + sources together; configure before commit | root `CMakeLists.txt:47-58`; `build-and-install.sh:262-271` |
| UI binary-data SOURCES missing → build fails / rule [8] if UI copied | Defer `:131-150` to 3.1; never create `Source/ui/public/js/i18n.js` in Stage 1 | `check-i18n.js:517-526,1170-1190`; `pattern_check_i18n_rule8_fails_stage1_placeholder_editor` |
| `juce_add_plugin(` in a comment → garbage target | Reword; grep gate 13 | `resolve-target.sh:37-48`; `pattern_resolve_target_matches_juce_add_plugin_in_comment` |
| Console builds at gnu++17 | `target_compile_features (O-GranulatorInstrument-<suffix> PRIVATE cxx_std_20)` after every console | parent `CMakeLists.txt:146-161` |
| Processor includes WebView headers → console fails | `WebViewDropStreaming.h` / `PluginEditor.h` only under `#if JUCE_WEB_BROWSER` | `WebViewDropStreaming.h:199`; `ParamDump.cmake:200` |
| `ReverbUnit::addParameters` adds `rev_*` | Never call it | `ReverbUnit.h:55-61` |
| Buffer width used as layout | Clear all `buffer.getNumChannels()`; SC channels from the negotiated bus | `critical_audiochannelset_is_a_bitset_not_an_order`; `pattern_stereo_in_mono_out_buffer_width_lies` |
| AU I/O cached by Logic per VERSION | Freeze buses before first install; D9 drop is VST3-only; no Logic during the variant arm | `critical_logic_caches_au_io_config_per_version` |
| Unrouted AU SC = silence, not disabled | Stage 2 decides on signal presence (D6) | `critical_au_hands_unrouted_sidechain_silence` |
| kAux has no local verifier | Harness H2 + code-path citation; Stage 4 Live/Reaper | `VST3.cpp:3108-3113`; pluginval log |
| uint64 seed clamps via `getLargeIntValue` | Write `String((int64) seed)`; test > INT32_MAX and > 2^63 | `critical_juce_getlargeintvalue_clamps_unsigned_64` |
| XML round trip returns strings | `isVoid()` + `(int) var` / `getLargeIntValue`; never `isBool()` | `critical_valuetree_xml_roundtrip_loses_type` |
| `id` attribute on a custom child hijacked as a param | `<Engine>`, `<Slots>`, `<Slot index>`: no `id` | `juce_AudioProcessorValueTreeState.cpp:405-437` |
| Duplicate children on reopen | Strip before `replaceState` and on the copy; two-reopen row | parent `:459-475,518-520`; `critical_preset_manager_stale_customstate_child` |
| Stepped range with negative start snaps 0 | Interval 0 everywhere except the 6 spec interval-1 rows (all start ≥ 0) | `pattern_stepped_range_snap_makes_zero_default_inexact` |
| Lambda range invisible to the WebView | Only start/end/interval/skew/`setSkewForCentre` | `critical_lambda_normalisable_range_invisible_to_webview_slider_frontend` |
| `·` names mangled by `String(const char*)` | Build with `+` / `<<` / `CharPointer_UTF8` | `critical_juce_string_char_ctor_is_ascii_only` |
| Golden written CRLF | Python `newline='\n'`; C++ `replaceWithText (…, false, false, "\n")` | `pattern_juce_replacewithtext_defaults_crlf`; parent `extract_spec.py:169-176` |
| Golden derived from code (self-referential) | Extract from the spec only; negative control (perturb 4 rows → FAIL) | parent SUMMARY "Negative control" |
| Param written from a listener / audio thread | None in Stage 1; grep gate 13 | `critical_vst3_nested_param_write_in_listener_never_reaches_host` |
| pluginval NaN fuzz | Shell clears → silent; keep `ScopedNoDenormals`; 2–3 runs | `pattern_ci_pluginval10_catches_latent_nan` |
| Vacuous zero-output gate | Assert input RMS > −20 dBFS; comparator fails on unprocessed copy | `pattern_gate_stimulus_below_threshold_is_vacuous` |
| `AudioBuffer::clear()` skipped when `isClear` | Fill the harness SC via `copyFrom` / `setSample` | `juce_AudioSampleBuffer.h:587-600` |
| 600 s watchdog | BG every configure/build/install/pluginval/cold auval | `pattern_executor_watchdog_stall_run_long_commands_in_background`; `pattern_cold_auval_after_install_rescans_registry` |
| D10 header note moves the whole-file hash | Re-record `contract_checksums.parameter_spec` only; body hash must equal `c5553e0c…` | STATUS frontmatter; spec `:5,11` |

## Recommended task skeleton (for PLAN, not the plan)

1. **Pre-flight.** Spec hash == STATUS; `OuGI` unused; `git status` noting the foreign files.
2. **Goldens.** Write `tests/golden/extract_spec.py`, then `params.tsv` (with an interval column), `mod-dests.tsv` (126) and `mod-sources.tsv` (18). LF. Exit 1 on a cross-check failure.
3. **Param header.** Write `Source/GiParamIds.h`: Defs with interval, `registrationOrder()`, `allIdsInOrder()`, literal `kModDestIds[126]`, `kModSources[18]`, `kModDestTableVersion = 1`, `kStateVersion = 1`.
4. **Processor shell** (`GranulatorInstrumentProcessor`): `makeBuses()` (SC active per `OGI_SC_DEFAULT_ACTIVE`), predicate, `GiVst3Extensions`, `createParameterLayout`, `prepareToPlay` (latency 0, negotiated SC channel count), the silent `processBlock`, seed and `uiLanguage` API, the state codec, the guarded `createEditor`, and the `OGI_TEST_HOOKS` `loadSlotSync` stubs. Add a placeholder `Source/PluginEditor.cpp`.
5. **CMakeLists.txt** from the mockup. Defer the UI binary data, add the harness console block and the gate options, then configure (BG) and check the NE marker line.
6. **Render harness.** G/D/SRC/B/B3/R1/S/H rows, `MidiScript`, `MockPlayHead`. Build (BG), run → exit 0. Add a negative control: perturbed goldens make it fail.
7. **Install + COMPAT-01.** Install, build the Standalone, launch smoke, pluginval 10 VST3 ×2–3 + AU, cold auval + negative control; record the capabilities.
8. **Disabled-SC arm** (§6): variant install → pluginval/auval → restore shipping → warm auval. D9 fallback only on an attributable failure (§1e), recorded.
9. **Bookkeeping.** `.planning/params.tsv`, `PLUGINS.md` row, `scripts/regen-registry-used-by.sh`, SUMMARY-1, path-scoped temp-index commit (after `/plugin-execute`; this research does not commit).

## Open questions (none blocks planning)

1. **Where does `uiLanguage` live?** CONTEXT says `<Engine>`; the spec, the checklist and the parent say the root attribute. Recommend the root (the Stage 3 contract); the round-trip row covers it either way.
2. **`customScale` / `degreeMasks` codec:** Stage 1 or 2.2? They are not in CONTEXT's scaffold list. Recommend 2.2 (the scale quantiser), with the strip list reserving the child names now.
3. **`kAux` verification depth.** Is H2 plus the code-path citation enough for Stage 1, or should a scratch `IComponent::getBusInfo` probe (VST3 SDK headers from JUCE) be added? Recommend H2 now and the DAW check in Stage 4.
4. **D10 header note.** Add it (this needs the whole-file checksum re-recorded in STATUS) or skip it (the wording is already correct)? Recommend skipping; CONTEXT records the resolution.
5. **`loadSlotSync` slot base.** Recommend 0…3 internally (= `rec_target` index); the state and page map to 1…4.
6. **Registry drift:** note-expression is 1.1.1 in `registry.yaml` and 1.1.2 on disk. Let `regen-registry-used-by.sh` or a manual bump fix it when the consumer is registered.

## Sources
- In-repo (read this session): the files cited inline with line numbers. JUCE 8.0.15 at `/Users/taylorbrook/JUCE/modules`.
- Commands run this session: the spec probes (scratchpad `scripts/specprobe.py`, `specrows.py`), the resolve-target awk simulation, `auval -v aumu OuTx OuDv`, pluginval 10 on `O-Texture-dev.vst3`, check-i18n (scoped + repo), the spec sha256 checks and the tool-version probes.
- No external web or Context7 sources were needed. The whole domain is in-repo or in JUCE source.
