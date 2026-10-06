# Stage 4: Polish - Research

**Plugin:** O-simpleWavetable · **Researched:** 2026-10-06 · **Target:** v1.0.0 (no tag, no publish)
**Method:** three parallel researchers. Part A covers the preset manager and UI fixes, Part B the DSP and import fixes, Part C QUAL-04, PERF-02, COMPAT-02, the release and the regression sweep. This synthesis comes first; the three parts follow in full as §A, §B and §C.
**Confidence:** HIGH overall.
- Every DSP fix (Part B) was prototyped in a scratch copy of `Source/`, and each passes its gate while its negative control fails.
- The QUAL-04 and PERF-02 numbers (Part C) were measured on the installed Stage 3 VST3 and on a scratch Release build.
- Part A's widths were measured with a Playwright probe. Its preset-manager findings come from reading the module source.
- Nothing in `Source/`, `tests/`, the shared `build/` or the git index was touched.

**Evidence:** the probe sources and the prototype diffs are preserved in `research-probes/{A,B,C}/`:
- `B/proc.diff` and `B/w4.diff`: the fix prototypes against the shipped `Source/`
- `B/src-fix/`: the patched tree
- `B/undef/`: the `-Wundef` per-header checks
- `C/perf/`: the PERF-02 harness prototype
- `C/q4*.py`: the QUAL-04 metric probes
- `A/measure*.js` and `A/flac_gen.py`: the width and FLAC-size probes

Binaries and audio dumps were not kept. Rebuild them from the scripts.

---

## Executive summary

| # | Topic | Recommendation | Key number | Part |
|---|---|---|---|---|
| 1 | Preset manager (OQ1) | Use module **v1.0.9** for the Factory/User folders, guarded save and delete, the factory sentinel and `preset-manager.js`. **Never** use its `setStateFromXml` or its apply. The plugin keeps its own state, plus a `currentPreset` string property, and has a single apply path: defaults first, skip `output_level`, gestures. | v1.0.9 **still has** the stale `<CustomState>` child bug (`OuariconPresetManager.h:617-644`) and resets/applies `output_level` (`:360-376`) | A §1 |
| 2 | Imported × presets (OQ2) | **(a) Store the bank choice only.** Loading never touches `IMPORTED_BANK`. With nothing imported, the empty-Imported prompt shows. | (b) costs 0.24–1.31 MB of base64 per preset and breaks the hard constraint | A §2 |
| 3 | Panel placement (OQ3) | Keyboard row, right-anchored, pinned at **352 px** (lines up with Amp + Output). Two rows: `PRESETS` + Save/Delete, then ◀ select ▶. Popover 352 × 92. | Keyboard 920 → 556 px (white keys 61 → 37 px); the lesson row has no room (fr caption uses 471/492 px) | A §3 |
| 4 | One recipe table | New `Source/PresetRecipes.h` with 9 raw-unit entries (Init + §A9 + a 4-bit PPG variant). It feeds the lesson buttons, the factory loads, the factory JSON, the page catalog and the modified check. Factory presets load **from the table**, not disk. | Alias Demo → Drive bank; the old Sine→Saw recipe has **no** in-band alias at C6 (−102 dB), measured | A §4, C §1.2 |
| 5 | Stage 2 W3 | Seed voice 0 with the last wheel value at Poly→Mono (~6 lines) | Reproduced on the installed VST3: **+199.98 c**; fixed 0.00 c | B §1 |
| 6 | Stage 2 W4 (OQ5) | The frozen cycle keeps its own phase and its heard rate (`xfPhase`/`xfInc`, ~20 lines in `WtVoice.h`) | Alias in the 5 ms fade, 60→96 @ 48 kHz: **−20 / −23 dB** → analyser floor (−102 / −107 dB); exactness 7.1e-8 | B §2 |
| 7 | Stage 2 notes 3/4/8 | Note 3: byte-capped chunk fill. Note 4: a `prepared` atomic. Note 8: a shared `TestHooks.h` (7 sites in 3 headers). | Note 3: 6,000 events → 1/7 mallocs → 0. Note 4: shipped code **SIGSEGVs** before prepare. | B §3 |
| 8 | Stage 3 W2 (OQ4) | **Lower the drop cap to 16 MiB.** Pre-sizing buys nothing. Change the `tooLarge` copy to point at the Import button. | 96 MiB = 664 ms decode + 300 ms JSON on the message thread; a full 256-frame bank needs only 1–4 MiB | B §4 |
| 9 | UI critic fixes | Exact shapes are given for Stage 3 W1, W3, W4, W5, N1, N9, N10–N13 | — | A §5 |
| 10 | Fix-or-log (OQ8) | **Fix:** Stage 3 N5 (the apply clears `pendingAutoSelect` under the lock) and N8 (gesture refcount). **Log:** Stage 3 N2, N3; Stage 2 notes 5, 6, 7 (note 7's premise is wrong: decoding happens before the lock). CODE_REVIEW wording is drafted. | Full import ≈ 17–24 ms | A §5, B §5 |
| 11 | QUAL-04 (OQ6) | Add three dsp-check gates: `G-Q4-STEP` (R ≥ 10), `G-Q4-ALIAS` (Off ≥ −50 dB, contrast ≥ 40 dB), `G-Q4-BITS` (≈ 6 dB/bit, monotonic). Then Taylor's by-ear sign-off. | Measured: R = 53.9; contrast ≥ 69 dB; 6.029 dB/bit | C §1 |
| 12 | PERF-02 (OQ7) | New `tests/perf-check`: Release, no test hooks, thread-CPU per block, gated < 25 %, exit 77 if the duty cycle is low. pedalboard cross-checks the steady case only. | **0.75 %** of one core at 96 kHz, 16 voices worst case (installed VST3: 0.72 %); a crossfade every block at bs 512: 1.97 % | C §2 |
| 13 | COMPAT-02 | The config is already complete: `NEEDS_WEBVIEW2`, static linking, `withUserDataFolder`, 0 MSVC trap carriers. The validate-only dispatch exists. | See the go-ahead caveats below | C §3 |
| 14 | Release | `CMakeLists.txt:14` → `1.0.0`; new CHANGELOG (single `## [1.0.0] - date`); new CODE_REVIEW.md at the canonical path; PLUGINS.md row via a temp-index commit | — | C §4 |
| 15 | Regression | Snapshot the **installed Stage 3 VST3 before the first Stage 4 install**; it is the null baseline. Expected diffs: only `mono_legato` @ samples 28801–29039 (W4), and the G-BLOCK peak detail and G-DROP[b] char count in R-GOLD. | 7/10 render scenarios bit-identical in the prototype | B §6, C §5 |

## Decisions for Taylor (resolve at `/plugin-plan` or the first checkpoint)

1. **Drive Sweep recipe vs copy.**
   - §A9's envelope (fast attack, then relaxes toward a sine) contradicts the signed-off Stage 3 caption, "starts clean, grows gritty".
   - **Recommend:** keep §A9 and rewrite the en/fr/zh copy (A §4, A14).
2. **Continuous-knob wheel (W3b).**
   - The continuous knobs over-step on a trackpad flick the same way as W3. The critic did not flag this.
   - Fixing it changes working behaviour.
   - **Recommend:** fix it with the same accumulator as W3, but only with your OK (A §5).
3. **Keyboard at 37 px white keys** to make room for the preset panel. Confirm at the visual checkpoint (A §3).
4. **COMPAT-02 push + dispatch: outward-facing, explicit go-ahead required.** Facts to weigh:
   - About 22 unpushed commits go out. `origin/main` has 0 O-simpleWavetable files.
   - The push also triggers `ui-static-gates.yml`.
   - The validate-only dispatch also **signs and notarizes the macOS build**.
   - Both jobs upload signed binaries as artifacts. Any signed-in GitHub user can download them from the public repo (~90-day default retention).
5. **Listening pass additions.** Add a Mono legato octave jump (e.g. C4 → C7 on Drive). After the W4 fix it is a 5 ms two-pitch crossfade instead of an alias burst. This is the only audible change in Part B.

## Cross-part reconciliation

- **Gate names.** Part A's Stage 3 gates were renamed to avoid a clash with Part B's Stage 2 W-numbers: `G-S3W4-ERRCLEAR` (import error cleared on a bank change), `G-S3W5-UIHELD` (UI-held notes released), `G-S3N5` (pending auto-select vs lesson). Part B uses descriptive names (`G-MONO-WHEEL`, `G-LEGATO-XF`, `G-LEGATO-ALIAS`, `G-MIDI-FLOOD`, `G-UNPREPARED`, `R-UNDEF`, `G-DROP-CAPSYNC`, `G-VIZ-RELEASE`, `G-SANITISE`). Part C uses `G-Q4-*` and `G-PERF02-*`.
- **The W2 copy change** (`import.err.tooLarge` → "use the Import button") is Part B's decision. It joins Part A's i18n batch (A8): en + fr + zh-Hans at `mt`, fr lint, and the 240 px error pin.
- **The G-LESSON goldens** get re-transcribed from §A9 (A2). The Part C `G-Q4-ALIAS` stimulus uses the new Alias Demo recipe through `applyFactoryPreset`, so it depends on A1/A2 landing first.
- **The N5 fix** (A2's apply core clears `pendingAutoSelect`) and **the W4 status poll** (A12) both touch `PluginProcessor.cpp` near Part B's note 3/4 edits. Sequence them in one wave or give them file-ownership boundaries.
- **The `TestHooks.h` header (note 8)** should land first: every later gate hook includes it (B order).
- **The module bugs stay upstream.** The preset-manager stale-child and `output_level` defects are logged in CODE_REVIEW.md, not fixed: `modules/` is outside this stage's commit scope.

## Suggested cadence (planner may adjust)

**Part 1: fixes + preset manager → build/install → visual checkpoint**
1. Snapshot the installed Stage 3 VST3 (the null baseline).
2. B: note 8 → note 4 → note 3 → W3 → W4 → N4, N7 → W2 constants.
3. A: A1–A6 (table, apply core, module, state, natives, Imported gate), then A10–A13 (critic fixes), A7–A9 (panel, i18n, fixtures).
4. Regression sweep: the 6 drivers, viz-check, `--alloc-check`, R-GOLD with the named diffs, the null test, the 5 UI gates, auval targeted, pluginval VST3/AU s10.
5. `./scripts/build-and-install.sh O-simpleWavetable`, plus an explicit Standalone rebuild.
6. **Checkpoint (Taylor):** the preset panel, the 37 px keyboard, the lesson highlight, and the trackpad wheel.

**Part 2: gates, perf, CI, release → listening sign-off**
1. The QUAL-04 gates (C §1) and the PERF-02 `perf-check` (C §2) + pedalboard cross-check.
2. VERSION 1.0.0, CHANGELOG, CODE_REVIEW.md (resolution log: Resolved v1.0.0 / Accepted by design, with Part B's §5 wording), PLUGINS.md row.
3. Regression sweep again, then install.
4. **Checkpoint (Taylor):** the go-ahead for push + `workflow_dispatch` validate-only → a green Windows build + pluginval.
5. **Checkpoint (Taylor):** by-ear sign-off on the factory presets, plus the Mono legato octave jump.

## Consolidated planner inputs

The per-part tables are authoritative; see §A "Planner inputs" (A1–A15), §B "Planner inputs" (rows 1–10 + suggested order) and §C "Planner inputs". Gate thresholds, negative controls and file:line anchors are there.

---

## §A — Stage 4 Research, Part A: Preset Manager + UI Critic Fixes

**Researched:** 2026-10-06
**Scope:** CONTEXT Open Questions 1–3, the lesson/§A9 single table, the Stage 3 UI critic items (W1, W3, W4, W5, N1, N5, N8–N13), and the effect on the UI gates.
**Not covered here:** DSP/import fixes (W2, Stage 2 W3/W4, notes 3–8), PERF/QUAL/CI/release. Parts B and C own those.

**Confidence tags**
- **HIGH**: read in code or measured this session, with file:line.
- **MEDIUM**: derived from code read this session, but not run.
- **ASSUMED**: not verified this session. Taylor confirms before it is locked.

---

### Summary (recommendations)

1. **Integrate preset-manager v1.0.9, but not its state path or its load path.**
   - Use the module for:
     - the Factory/User directories
     - `savePreset` (it guards factory names and sanitizes "/")
     - `deletePreset` (it guards factory presets)
     - `isFactoryPreset`
     - `initializeFactoryPresets` (with its sentinel)
     - `preset-manager.js`
   - Keep the plugin's own `getStateInformation`/`setStateInformation`, and add one string property, `currentPreset`.
   - Replace the module's `applyPresetJson` with the plugin's single-pass apply. That apply skips `output_level`, resets to defaults first, and brackets every change in a gesture.
   - Why: v1.0.9 **still has** the stale-child `setStateFromXml`. Its apply also **resets and applies `output_level`**.
2. **Open Question 2: choose (a), bank choice only.**
   - A user preset stores `bank = Imported` as a normal parameter. Loading it never touches `IMPORTED_BANK`.
   - With nothing imported, the Stage 3 empty-Imported prompt appears.
   - Embedding (b) is ruled out by the hard constraint: it would replace the session bank. It also costs 0.24–1.31 MB of base64 per preset (measured) and a decode plus mip build on the message thread at every browse step.
   - Blocking the save (c) is hostile.
3. **Open Question 3: put the browser in the keyboard row, right-anchored and pinned at 352 px.**
   - At that width its left edge lines up with the Amp + Output group above it.
   - The keyboard shrinks from 920 to 556 px (white keys 61 → 37 px).
   - The lesson row has no room: its caption already uses 471 of the 492 px left for it, in fr.
   - The panel has two rows: `PRESETS` label plus Save/Delete, then ◀ select ▶.
   - Save and Delete confirmations open in a fixed-size popover above the panel.
4. **One recipe table:** a new header-only `Source/PresetRecipes.h`.
   - It holds 9 entries: Init plus the §A9 rows, with the 4-bit PPG variant as the 9th.
   - Values are in raw units, converted with `convertTo0to1` at use.
   - It feeds `applyFactoryPreset` (lesson buttons), the factory-preset load, the factory JSON materialization, the page catalog and the "modified" check.
   - Factory presets load **from the table, not from disk**, so a stale sentinel can never play stale sound.
5. **Lesson highlight (N10) and preset name come from C++.**
   - A new `presetState {name, id, factory, modified}` event is polled on the existing 30 Hz editor timer, by a revision counter and a modified check.
   - It covers lesson clicks, browser loads, host automation, session restore with the editor open, and reopen.
6. **Natives: 8 → 20.**
   - The 10 that preset-manager.js resolves. Two of them are explicit refusal stubs: `savePresetWithDialog` and `loadPresetFromFile`.
   - `getPresetCatalog`.
   - `stepKnobDrag` (N13).
   - **Events: 3 → 4** (`presetState`).
7. **Critic fixes:**
   - W1: snapshot first.
   - W3: burst-aware px accumulator.
   - W4: the processor clears a latched import error on any bank change.
   - W5: the processor tracks UI-held notes and the editor destructor releases them.
   - N1: the O-AnalogEQ member plus in-flight flag.
   - N9: an `import.err.unsupported` key that fits 240 px.
   - N11: two TIP_BINDINGS rows.
   - N12: a page-only U+2212.
   - N13: a native gesture bracket.
   - **N5: FIX** (the shared apply clears `pendingAutoSelect` under `bankStateLock`).
   - **N8: FIX** (a page-side per-parameter gesture refcount).
8. **Gates:**
   - About 30 new I18N entries, 7 new TIP_BINDINGS rows, and 6 new `i18n-states.json` states.
   - A new `tests/ui-stub/generic-overrides.json` with the preset natives.
   - Pins: panel 352, label 88, Save 74, Delete 72, Cancel 62, select 296, popover 352 × 92.
   - New C++ gates: G-FACTORY, G-PRESET-USER, G-PRESET-IMPORTED, G-S3N5 and G-S3W4-ERRCLEAR (viz-check), plus state-check P11 (`currentPreset`).

---

### 1. Preset-manager integration surface (Open Question 1)

#### Findings

**Version and location.**
- The module is `modules/persistence/preset-manager/`. `module.yaml:5` reads `version: 1.0.9`, and `registry.yaml:140` reads `version: 1.0.9` [HIGH].
- Its sources are `cpp/OuariconPresetManager.h` (700 lines, header-only) and `js/preset-manager.js` (461 lines) [HIGH].
- `ouaricon_add_module` does three things [HIGH: `modules/cmake/OuariconModules.cmake:57-66,105-118`]:
  - globs `cpp/*.h` into the target's sources
  - adds the include dir
  - `configure_file`-copies `js/*.js` into `Source/ui/public/modules/`
- That directory is gitignored [HIGH: `.gitignore:26` `plugins/*/Source/ui/public/modules/`].

**The stale `<CustomState>` child is NOT fixed in v1.0.9** [HIGH: `OuariconPresetManager.h:617-644`].
- `setStateFromXml` calls `parameters.replaceState(juce::ValueTree::fromXml(*xml))` on the whole element, with no strip.
- `getStateAsXml` (`:591-615`) starts from `copyState()` and appends a new `CustomState`.
- Memory `critical_preset_manager_stale_customstate_child` still applies.
- Worse for this plugin: `setStateFromXml` would also carry `IMPORTED_BANK` into the live APVTS tree. The plugin's state code is built to prevent exactly that [HIGH: `PluginProcessor.cpp:771-778`, "Detach BEFORE replaceState so the live APVTS tree never carries the blob"]. state-check P3, P4 and P10 gate it.

**The module apply violates two suite rules** [HIGH: `OuariconPresetManager.h:360-376`].
- The reset pass calls `setValueNotifyingHost(getDefaultValue())` on **every** parameter, including `output_level`, with no gesture.
- The apply pass applies every key in the JSON, including `output_level`, because `createPresetJson` saves all parameters (`:305-311`).
- So any load through `loadPreset`/`applyPresetData` moves `output_level`. That breaks the CONTEXT "never set `output_level`" rule and memory `feedback_presets_never_set_output_gain`.

**Save APIs** [HIGH].
- `savePreset(name)` refuses `isFactoryPreset(name)` (`:397-401`), sanitizes `/\:` (`:247-250`, `:406`) and lazily creates `User/` (`:404`).
- `savePresetToFile` has **no guard** (`:423-441`; memory `pattern_savepresettofile_has_no_factory_guard`).
- `deletePreset` refuses factory presets and resets `currentPresetName` to `"Default"` when the current preset is deleted (`:503-517`).

**Listing and navigation** [HIGH: `:524-589`].
- `getPresetList` returns factory and user presets merged and sorted case-insensitively.
- `getNextPreset`/`getPreviousPreset` walk that alphabetical list.
- So "8-bit PPG" would sort before "Init". Memory `pattern_grouping_preset_dropdown_breaks_prev_next`: prev/next only agree with a grouped dropdown if both read the same walk order.

**Factory sentinel** [HIGH: `:646-700`].
- `initializeFactoryPresets` returns early when `Factory/.factory-version` equals `JucePlugin_VersionString`.
- The console gate targets define `JucePlugin_VersionString="${_pd_version}"` from the plugin's VERSION [HIGH: `scripts/param-dump/ParamDump.cmake`, `ouaricon_add_processor_console` compile definitions].
- So a harness that constructs the processor writes to the user's real `~/Library/O-simpleWavetable/Presets/Factory` under the same sentinel (memory `pattern_factory_bank_sentinel_makes_preset_gates_read_stale_bank`).

**Programs.** The module has no program API at all (whole header read). `getNumPrograms()` stays `1` [HIGH: `PluginProcessor.h:154-158`]. Compatible.

**JS module** [HIGH: `preset-manager.js:100-144`].
- `initialize()` resolves 10 natives: `savePreset`, `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`.
- It binds only the buttons it is given.
- It calls `refresh()` = `getCurrentPreset` + `getPresetList`.
- `promptDelete` uses `options.onConfirmDelete` (`:352-380`).
- v1.0.9 has no native `title=` (`:433-441`). check-i18n [11] scans it as a served module [HIGH: `scripts/check-i18n.js:982-1060`].

**Reference integrations.**
- **O-AnalogEQ** (clean tree) is the reference for:
  - the natives
  - the FileChooser member plus `presetDialogInFlight` flag [HIGH: `O-AnalogEQ/Source/PluginEditor.h:112-132`, `PluginEditor.cpp:107-260`]
  - the location-aware `PresetSaveGuard.h`
  - serving `/modules/preset-manager.js` as `BinaryData::presetmanager_js` (the hyphen is stripped) [HIGH: `PluginEditor.cpp:460-462`]
- **O-simpleFM** (read at HEAD only; it has another session's uncommitted edits) is **not** clean for this plugin:
  - It routes state through `getStateAsXml`/`setStateFromXml` [HIGH: `git show HEAD:…/PluginProcessor.cpp` ~`:375-405`].
  - Its factory table sets `outputLevel` in every lesson preset [HIGH: `HEAD:…/FactoryPresets.cpp`, E-Piano `{ outputLevel, -3.0f }` etc.].
  - Reuse only its CMake ordering: `ouaricon_add_module(... preset-manager)` **before** `juce_add_binary_data`, and `Source/ui/public/modules/preset-manager.js` in SOURCES [HIGH: `HEAD:plugins/O-simpleFM/CMakeLists.txt:50-66`].

**This plugin's state** [HIGH].
- `getStateInformation` (`PluginProcessor.cpp:743-752`):
  1. `copyState`
  2. `stripImportedBank`
  3. `uiLanguage` code
  4. `writeImportedBank` (`:871-891`, one child from `cachedBlob`)
- `setStateInformation` (`:754-780`):
  1. reads the language with an `isVoid` gate
  2. detaches the child
  3. strips every copy
  4. `replaceState`
  5. `restoreImportedBank` (synchronous, `:788-869`)

#### Recommendation

**State.** Keep both functions as they are. Add one root property:
- `getStateInformation`: `state.setProperty ("currentPreset", getPresetName(), nullptr)`, next to the `uiLanguage` line, before `writeImportedBank`.
- `setStateInformation`: `const juce::var cp = state.getProperty ("currentPreset"); if (! cp.isVoid()) setPresetName (sanitisePresetName (cp.toString()));`. Read it from the incoming `state` before `replaceState`, the same as `uiLanguage`.
- An absent property becomes `""` (unnamed).
- **Never call `getStateAsXml`/`setStateFromXml`.** No custom-state callbacks are needed: the Imported bank is not preset content (§2).

**Processor member.** Add `OuariconPresetManager presetManager { parameters, "O-simpleWavetable" };`, declared after `parameters`. Use the literal name: dev and release builds share `~/Library/O-simpleWavetable/Presets`, which is the suite norm (module `:274-281`).

**Apply core** (one function, message thread). It replaces the body of `applyFactoryPreset` [HIGH: current body `PluginProcessor.cpp:1142-1203`]:

```cpp
// targets[i] = normalised value for ParamIDs::all[i]; NaN = "not in preset -> default".
bool OSimpleWavetableAudioProcessor::applyNormalisedTargets (const std::array<float, 21>& targets)
{
    { const juce::ScopedLock sl (bankStateLock); pendingAutoSelect = false; }   // N5 (sec. 5)
    namespace ids = OSimpleWavetable::ParamIDs;
    for (size_t i = 0; i < ids::all.size(); ++i)
    {
        if (std::strcmp (ids::all[i], ids::outputLevel) == 0) continue;           // user-owned: never reset, never set
        auto* p = parameters.getParameter (ids::all[i]);
        if (p == nullptr) continue;
        const float t = std::isfinite (targets[i]) ? juce::jlimit (0.0f, 1.0f, targets[i])
                                                   : p->getDefaultValue();        // reset-to-defaults first
        if (std::abs (p->getValue() - t) < 1.0e-6f) continue;
        p->beginChangeGesture(); p->setValueNotifyingHost (t); p->endChangeGesture();
    }
    return true;
}
```

Three callers:
1. **Recipe → targets.** `convertTo0to1(raw)` for each listed entry, NaN for the rest.
2. **User JSON → targets.** For each `ParamIDs::all` id: if `parameters` has the property and it is `isDouble()/isInt()/isInt64()/isBool()`, use `(float)(double) v`; otherwise NaN. Unknown keys are ignored, and `output_level` is ignored even when present. This is the ASVS V5 untrusted-file rule.
3. **Factory name → recipe → targets** (never the disk JSON).

**Natives** (editor). All are message-thread `withNativeFunction`.

| Native | Implementation |
|---|---|
| `getPresetList` | **The walk order**: the factory table in table order, then the files in `getUserPresetsDirectory()/*.json` sorted case-insensitively, minus any that match a factory name. One processor function builds the list (`getPresetWalkOrder()`). |
| `getCurrentPreset` | `getPresetName()` |
| `selectNextPreset` / `selectPreviousPreset` | Neighbour of the current name in the **same** walk order. An unnamed or missing name enters at the top going forward and at the bottom going back. |
| `loadPreset(name)` | `name` must be in the walk order (this also blocks path traversal). Factory → table → apply. User → `loadFileAsString` + `JSON::parse` → apply. Then set the name, store the targets, and bump `presetRevision`. |
| `savePreset(name)` | Clean the name (see Pitfalls). Refuse it if it matches a factory table name (case-insensitive). Call `presetManager.ensure…` (lazy factory materialization), then `presetManager.savePreset(clean)`. Name = `presetManager.getCurrentPresetName()`, the sanitized on-disk name. Store the targets, bump the revision, return the bool. |
| `deletePreset(name)` | Refuse a factory name. Call `presetManager.deletePreset(name)`. If it was current, name = `""` (not the module's `"Default"`). Bump the revision. |
| `isFactoryPreset(name)` | Table lookup. Not the disk (the disk can be stale). |
| `savePresetWithDialog` / `loadPresetFromFile` | **Refusal stubs**: `complete({success:false, name:""})`. The page never binds them, but registering them means no promise can hang and the bridge cross-check stays exact. |
| `getPresetCatalog` | `{ factory: [{name, id}], user: [names] }`, from the table and the user directory. |
| `stepKnobDrag` | N13 (§5) |

**Events.** Add `presetState { name, id, factory, modified }`.
- Emit it from `timerCallback` when `getPresetRevision()` changes or `isPresetModified()` flips.
- `uiReady` forces it.
- This is the push that memory `pattern_webview_one_shot_state_push_stale_on_preset_load` requires: a session restored with the editor open must reach the page.

**Factory bank on disk.**
- Materialize it lazily on the first preset native (`ensureFactoryBankOnDisk()`, message thread), not in the processor constructor. Then auval, pluginval and every console gate do no file I/O.
- Each `FactoryPresetDef` holds the full 20-parameter normalized map (defaults filled, `output_level` omitted) from `PresetRecipes.h`.
- The disk copy exists only for the module's own guards and for users who browse the folder. The sound always comes from the table.

**Serving the JS.**
- CMake: `ouaricon_add_module(O-simpleWavetable preset-manager)` next to the existing `webview-drop-streaming` line, which already precedes `juce_add_binary_data` [HIGH: `CMakeLists.txt:55,62`]. Add `Source/ui/public/modules/preset-manager.js` to SOURCES.
- `getResource`: `if (url == "/modules/preset-manager.js") return makeBinaryResource (BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize, "application/javascript; charset=utf-8");`, alongside the drop-streaming branch [HIGH: pattern at `PluginEditor.cpp:103-105`].
- Page: **dynamic** `import('./modules/preset-manager.js')` inside `initPresets(Juce)`. On failure, disable the panel. A static import that 404s blanks the whole page, the same failure mode as `i18n.js` (`PluginEditor.cpp:87-90` comment).
- Construct `new PresetManager({ getNativeFunction: Juce.getNativeFunction, onConfirmDelete, onPresetChanged, onPresetListUpdated, deleteButton? })`. Do **not** pass `displayElement` (we render localized names), `saveButton` (it would call the dialog stub) or `loadButton`. Prev/next may go to the module, because the natives walk our order.

**Thread safety.**
- `setStateInformation` can run off the message thread.
- Keep the name, the targets and `presetRevision` in processor members under a new `juce::CriticalSection presetLock`. Never take it on the audio thread, and never call `setValueNotifyingHost` while holding it.
- Module calls happen only on the message thread.

#### Pitfalls

- **Name cleaning** (`sanitisePresetName`, a new helper used for save and restore):
  1. trim
  2. remove C0, DEL, C1 (U+0080–009F), bidi controls (U+200E/F, U+202A–202E, U+2066–2069)
  3. `juce::File::createLegalFileName` (it removes `"#@,;:<>*^|?\/` [HIGH: `JUCE/modules/juce_core/files/juce_File.cpp:855-857`])
  4. strip leading dots
  5. cap at 64 characters
  6. refuse an empty result
- Do not reuse `WavetableImporter::sanitiseName`: it never returns empty [HIGH: `WavetableImporter.h:104-106`].
- **The page also refuses** names that match any factory **display** label in any language. Otherwise a user "Repliement" sits beside the factory "Repliement".
- **Do not call `savePresetToFile` anywhere.** The only save path is `savePreset(name)`.
- The module's user JSON **stores** `output_level`, because `createPresetJson` saves every parameter. Our apply ignores it. Document that it is stored but never applied.
- The module's `std::map` use (`:217`) depends on JUCE's transitive `<map>`. It compiles today in O-simpleFM [MEDIUM]. Watch the R-DBG log for module-header warnings: they sit outside `plugins/O-simpleWavetable`, so the 0-warning grep will not count them.
- **Out-of-tree / `git archive` builds** need the gitignored `Source/ui/public/modules/preset-manager.js` (memory `pattern_git_archive_ui_tree_misses_gitignored_module_copies`). `ouaricon_add_module` regenerates it only if the archive includes `modules/`.
- **Gates must not touch `~/Library`.** Test the apply through `applyUserPresetJson(const juce::var&)`, which is public and pure, and the defs through `buildFactoryPresetDefs()`, which is pure.

---

### 2. Imported bank in user presets (Open Question 2)

#### Findings

- `IMPORTED_BANK` is processor-owned:
  - `importedOwner` and `cachedBlob { filename, encoding, data, numFrames, passthrough }` live under `bankStateLock` [HIGH: `PluginProcessor.h:383-393`].
  - It is serialized only by `writeImportedBank`: `version` 1, `encoding` `"flac16"` or `"pcm16gz"`, standard base64 [HIGH: `PluginProcessor.cpp:871-891`, `:1059-1064`].
  - The restore cap is `kMaxDataChars = 4 * 1024 * 1024` [HIGH: `WavetableImporter.h:88`].
- The encoder is mono 16-bit FLAC, quality 5, 48 kHz nominal [HIGH: `WavetableImporter.cpp:191-218`].
- **Measured flac16 size** for 524,288 samples (1 MiB raw), using `flac -5` (the same libFLAC preset) on 4 synthetic sources [HIGH for these sources; MEDIUM that JUCE's writer matches within a few %]:

  | Source | FLAC | ÷ raw | base64 in JSON |
  |---|---|---|---|
  | 256-frame Sine→Saw-like wavetable | 176,951 B | 0.17 | 236 KB |
  | speech-like bursts | 291,605 B | 0.28 | 389 KB |
  | harmonic instrument + vibrato + noise | 769,388 B | 0.73 | 1.03 MB |
  | white noise (worst case) | 981,711 B | 0.94 | 1.31 MB |

- **Load cost** of an embedded bank:
  - JSON parse of up to a 1.3 MB string, base64 decode, FLAC decode and mip build. ARCHITECTURE quotes the mip build at about 50 ms worst case [CITED: `ARCHITECTURE.md` §State Persistence].
  - That all happens **on the message thread inside the `loadPreset` native**, and again on every ◀/▶ step.
  - It would also **replace the session's Imported bank**, which breaks the hard constraint.

#### Recommendation: (a) store the bank choice only

- A user preset saves `bank` as a normal parameter: normalized 1.0 = Imported.
- Loading it sets the parameter only. The apply core never touches `importedOwner`, `cachedBlob` or `importStatus`. Its only `bankStateLock` write is `pendingAutoSelect = false`.
  - With an import in the session, the preset plays that audio.
  - With none, the existing empty-Imported prompt shows: attention pulse on Import, `src.empty` [HIGH: `index.html:2195-2222`].
- The Save tooltip says plainly that the audio is not saved (§3 copy). Factory presets never use Imported (ARCHITECTURE §State Persistence).
- Optional, deferred: write an informational `"importedName"` into the user JSON, to show "made with <file>". Not v1.0.

#### Pitfalls

- **G-PRESET-IMPORTED** (viz-check):
  1. Publish a bank.
  2. Snapshot `getImportedBankSnapshot().get()` and the `IMPORTED_BANK` data string from `getStateInformation`.
  3. Apply a user JSON with `bank` = 1.0, then one with `bank` = Drive.
  4. Assert pointer and string unchanged, and bank index 5 then 4.
  - **Negative control (NC):** a deliberately wrong apply that calls `restoreImportedBank({})` must fail the pointer check.
- Do not add `customState` for this. Module v1.0.7+ calls `customLoad` with an empty var on every apply (`:385-386`). A callback wired to the bank would be one null-check away from clearing it.

---

### 3. Browser panel placement and copy (Open Question 3)

#### Findings: the fixed geometry [HIGH: `index.html`]

**Vertical.**
- Frame: 780 − 2×3 border − 2×12 padding (`:131-141`) = **750** inside.
- Rows: header 50 (`:146`), viz-band 330 (`:338`), controls 236 (`:430`), preset-tour 28 (`:651`), keyboard-panel 66 (`:676`), plus 4 gaps of 8 = **742**.
- **8 px spare. No new row fits.**

**Horizontal.**
- 1120 − 6 − 32 = **1082** inside.
- Lesson row: label 100 (`:657`) + 458 pinned buttons (`:659`) + gaps 24 + padding 8 → **492 px left for `.tour-caption`**.
- Measured caption widths (12 px italic EB Garamond, headless Chromium, scratch probe):
  - widest fr lesson caption (`tour.caption.steppedSmooth`): **470.95 px**
  - fr `tour.hint`: 396.14 px
- The lesson row is full. Taking width from it would truncate signed-off teaching copy.

**Keyboard row.** `kbd-side` 150 + gap 12 + keyboard (flex, 920 today). The keys are laid out in percentages [HIGH: `buildKeyboard` `index.html:2496-2527`, `left: calc(i*ww% - 11px)`], so a narrower keyboard reflows safely.

**Alignment target.** `.group-amp` is `flex: 0 0 352px` and right-anchored in `.controls` [HIGH: `:443`].

#### Recommendation: geometry

```
.keyboard-panel:  [kbd-side 150] 12 [keyboard 556] 12 [.preset-panel 352]   = 1082
.preset-panel (flex: 0 0 352px; column; justify-content: center; gap: 6px; position: relative)
  row 1 (h 20): [.preset-label "PRESETS" w 88]  ……flex……  [Save w 74] 6 [Delete w 72]
  row 2 (h 26): [◀ 22×22] 6 [select#preset-select w 296] 6 [▶ 22×22]
  .preset-popover (absolute; bottom: calc(100% + 6px); right: 0; width 352; height 92; z-index 40)
```

- The panel's left edge lines up with the Amp + Output group above it (both right-anchored, both 352 px).
- White keys go from 61 to 37 px; black keys stay at a fixed 22 px. Taylor confirms at the visual checkpoint [ASSUMED acceptable].
- Styles:
  - Select: reuse `select.combo` (`:193`) at 13 px, with an explicit `width: 296px` (memory `pattern_appearance_none_select_exposes_caption_width`).
  - Buttons: reuse the `.tour-btn` look (12 px), with **explicit widths**.
  - Arrows: reuse `.oct-btn`.
- **Modified marker:** a CSS 6 px brass dot (`.preset-panel.modified::before`). No text, so no i18n. The select tooltip explains it.
- **Popover** (Save, Replace? and Delete?):
  - Fixed 352 × 92, so check-ui-labels [7] sees the same rectangle in every language.
  - Rows: title line (12 px), input (24 px) + [Save]/[Cancel] or [Delete]/[Cancel], and a one-line message (12 px italic, `--warn-text`).
  - Dismiss with a capture-phase `pointerdown` outside and with Escape, the same as the settings popover (`index.html:2456-2468`; memory `pattern_pointerdown_preventdefault_kills_document_mousedown`).
  - Enter submits.
  - The keyboard handler already ignores `INPUT` targets [HIGH: `:2565-2566`], so typing a name plays no notes.
- **Options:**
  - Factory entries go through `setLabel(opt, <literal key>)` from a literal id → labeler map. A computed key fails check-i18n [13]/[15]; see the `renderBankLesson` precedent at `:2106-2120`.
  - User entries go through `setRaw` (`:1601-1607`).
  - A disabled `────` separator option sits between the groups. No optgroups: an optgroup `label=` is invisible to every gate.
- **Lesson coupling:**
  - The page stops toggling `.tour-btn.active` on click (`:2439`). `presetState` drives it: lit iff `id` is that lesson's id and `!modified`.
  - The caption follows the `presetState` id.
  - `LESSON_OCTAVE` (`:1156`) stays click-driven (a lesson button or a browser load), never a restore.

#### Recommendation: copy

Measured widths in px, EB Garamond plus the PingFang fallback, headless Chromium, `getBoundingClientRect` [HIGH]. fr is at `reviewed: false` and zh-Hans at `reviewed: 'mt'`. fr typography: U+00A0 before `? : ; !`, inside `« »`, and between a number and its unit; U+2019 apostrophes.

**Labels** (`{ t }`):

| Key | en | fr | zh-Hans | Widths en/fr/zh → pin |
|---|---|---|---|---|
| `label.presets` (10.5 px caps, ls 1.2) | Presets | Préréglages | 预设 | 50.3 / **85.0** / 23.9 → **88** |
| `preset.save` (12 px) | Save | Enregistrer | 保存 | 21.4 / **54.7** / 25.3 → **74** incl. 8+8 padding, 2 border |
| `preset.delete` | Delete | Supprimer | 删除 | 32.1 / **52.9** / 25.3 → **72** |
| `preset.cancel` | Cancel | Annuler | 取消 | 34.3 / **41.7** / 25.3 → **62** |
| `preset.saveTitle` | Save preset | Enregistrer le préréglage | 保存预设 | 49.9 / 113.1 / 49.8 |
| `preset.namePlaceholder` (`data-i18n-placeholder`) | Preset name | Nom du préréglage | 预设名称 | 56.5 / 89.4 / 49.8 |
| `preset.errFactoryName` | A factory preset has that name — choose another. | Nom d’un préréglage d’usine — choisissez-en un autre. | 该名称属于出厂预设——请换一个。 | 224.5 / **252.3** / 196.1 (≤ 328 inside the popover) |
| `preset.errEmpty` | Type a name first. | Saisissez d’abord un nom. | 请先输入名称。 | 84 / 119 / 87 |
| `preset.errSave` | Could not save the preset. | Impossible d’enregistrer le préréglage. | 无法保存预设。 | 116 / 173 / 87 |
| `preset.confirmReplace` `{name}` | Replace “{name}”? | Remplacer « {name} » ? | 替换“{name}”？ | with "Bright Pad": 104 / 133 / 99 |
| `preset.confirmDelete` `{name}` | Delete preset "{name}"? | Supprimer le préréglage « {name} » ? | 删除预设“{name}”？ | 126 / 193 / 124. The en uses straight quotes so it matches the zh glossary key `'delete preset "{name}"?'` exactly [HIGH: `scripts/i18n-zh-glossary.js:217`]. |
| `aria.presetPrev` / `aria.presetNext` | Previous preset / Next preset | Préréglage précédent / Préréglage suivant | 上一个预设 / 下一个预设 (glossary `:197-198`) | aria only |
| `import.err.unsupported` (N9, 10.5 px italic) | saved by a newer version — kept, not playable here | d’une version plus récente — conservé, non jouable ici | 由更新版本保存——已保留，此处无法播放 | 213.9 / **224.6** / 208.8 (≤ 240 `.src-meta` content: 262 − 20 − 2) |

**Factory display names** (13 px). Widest is fr 140.25, which fits the select's 255 px text box (296 − 10 − 28 − 3):

| id | file name (ASCII, no `/`) | en | fr | zh-Hans | key |
|---|---|---|---|---|---|
| `init` | `Init - Additive Build` | Init · Additive Build (105.7) | Init · construction additive (**140.3**) | 初始 · 加法叠加 (90.1) | `preset.init` (new) |
| `steppedSmooth` | `Stepped Scan` | Stepped Scan | Balayage par paliers (101.2) | 阶梯扫描 | `preset.steppedScan` (new) |
| `smoothScan` | `Smooth Scan` | Smooth Scan | Balayage fondu | 平滑扫描 | `preset.smoothScan` (new) |
| `aliasDemo` | `Alias Demo` | reuse `label.lessonAliasDemo` | Repliement | 混叠演示 | existing |
| `driveSweep` | `Drive Sweep` | reuse `label.lessonDriveSweep` | Balayage saturé | 过载扫描 | existing |
| `vowelPad` | `Vowel Pad` | reuse `label.lessonVowelPad` | Voyelles | 元音铺底 | existing |
| `pulseNarrowing` | `Pulse Narrowing` | Pulse Narrowing (88.6) | Impulsion qui rétrécit (115.9) | 脉冲收窄 | `preset.pulseNarrowing` (new) |
| `ppg8bit` | `8-bit PPG` | reuse `label.lessonPpg8bit` | PPG 8 bits | 8 bit PPG | existing |
| `ppg4bit` | `4-bit PPG` | 4-bit PPG | PPG 4 bits | 4 bit PPG | `preset.ppg4bit` (new) |

**Tooltips** (TIP_BINDINGS, `{ t, b }`):

| Selector → key | en t / b | fr t / b | zh-Hans t / b |
|---|---|---|---|
| `#preset-select` → `presetSelect` | Presets / Factory presets come first — each one shows one idea. Your own presets follow. Loading a preset never changes Output Level or the imported audio. A dot means you have changed it since loading. | Préréglages / Les préréglages d’usine d’abord — chacun montre une idée. Les vôtres suivent. Charger un préréglage ne change jamais le niveau de sortie ni l’audio importé. Un point signale une modification depuis le chargement. | 预设 / 出厂预设在前——每个演示一个概念，你自己的预设在后。载入预设不会改变输出电平，也不会改变导入的音频。圆点表示载入后你已作修改。 |
| `#preset-prev` → `presetPrev` | Previous preset / Loads the previous preset in the list. | Préréglage précédent / Charge le préréglage précédent de la liste. | 上一个预设 / 载入列表中的上一个预设。 |
| `#preset-next` → `presetNext` | Next preset / Loads the next preset in the list. | Préréglage suivant / Charge le préréglage suivant de la liste. | 下一个预设 / 载入列表中的下一个预设。 |
| `#preset-save` → `presetSave` | Save preset / Saves every setting except Output Level as one of your presets. On the Imported bank only the bank choice is saved, not the audio. | Enregistrer le préréglage / Enregistre tous les réglages sauf le niveau de sortie dans un de vos préréglages. Avec la banque Imported, seul le choix de banque est enregistré, pas l’audio. | 保存预设 / 将除输出电平外的所有设置保存为你的预设。使用 Imported 波表库时只保存所选波表库，不保存音频。 |
| `#preset-delete` → `presetDelete` | Delete preset / Deletes the selected preset of your own. Factory presets cannot be deleted. | Supprimer le préréglage / Supprime le préréglage sélectionné, s’il est à vous. Les préréglages d’usine ne peuvent pas être supprimés. | 删除预设 / 删除所选的用户预设。出厂预设无法删除。 |
| `#octDown` → `octDown` (N11) | Octave down / Moves the on-screen keyboard and the computer keys down one octave. Shortcut: Z. | Octave inférieure / Descend le clavier à l’écran et les touches de l’ordinateur d’une octave. Raccourci : Z. | 降八度 / 将屏幕键盘和电脑按键降低一个八度。快捷键：Z。 |
| `#octUp` → `octUp` (N11) | Octave up / Moves the on-screen keyboard and the computer keys up one octave. Shortcut: X. | Octave supérieure / Monte le clavier à l’écran et les touches de l’ordinateur d’une octave. Raccourci : X. | 升八度 / 将屏幕键盘和电脑按键升高一个八度。快捷键：X。 |

**Glossary compliance** [HIGH: `scripts/i18n-fr-glossary.js:81-92`, `:385-397`; `scripts/i18n-zh-glossary.js:124,137-138,191-218`]:
- fr:
  - Save → `enregistrer`, Delete → `supprimer`, Save preset → `enregistrer le préréglage`.
  - The forbidden labels `sauver`, `sauvegarder` and `lire` are not used. "non jouable" avoids `lire`.
- zh:
  - Save 保存, Delete 删除, Cancel 取消, Presets 预设, Previous/Next preset 上一个/下一个预设, Save preset 保存预设, Factory 出厂, User 用户.
- **Caveat:** the fr and zh body prose is machine-drafted, the same as Stage 3. It is not reviewed.

#### Pitfalls

- **Pin widths come from rendered measurements.** Re-measure after the CSS lands, because padding specificity can win (memory `pattern_language_width_pins_content_sized_boxes_move`). Keep the three widths as a comment beside each pin.
- The popover message box is `white-space: nowrap`, at a fixed width and height. A user name long enough to overflow ellipsizes. The gate state uses "Bright Pad", so it stays measurable.
- Measurements are headless Chromium (the gate engine). WKWebView can differ by about 1 px [ASSUMED]. The pins carry ≥ 2 px of slack.

---

### 4. Lesson buttons ↔ §A9 single recipe table

#### Findings

**Today there are three copies** [HIGH]:
1. The C++ inline recipes in `applyFactoryPreset`. Ids: `steppedSmooth`, `aliasDemo`, `driveSweep`, `vowelPad`, `ppg8bit` (`PluginProcessor.cpp:1154-1175`).
2. The page ids: `data-preset` (`index.html:1029-1033`), `CAPTION_LABELERS` (`:2428-2434`) and `LESSON_OCTAVE = { aliasDemo: 6 }` (`:1156`).
3. G-LESSON's **independent** transcription (`tests/viz-check/main.cpp:393-409`).

**§A9's 8 rows** [HIGH: `ARCHITECTURE.md:350-361`]:
- Init / Additive Build
- Stepped Scan
- Smooth Scan
- Alias Demo (Drive, Pos 100 %, Band-limit Off, C6–C8)
- Drive Sweep (Env +100 %, menv A 0.01, D 1.5, S 0)
- Vowel Pad (Tri 0.1 Hz, depth 100 %, Pos 50 %, slow amp)
- Pulse Narrowing (Env +80 %)
- 8-bit PPG ("and a 4-bit variant", Interp Off, S&H 4 Hz)

**Choice indices** [HIGH: `PluginProcessor.cpp` `createParameterLayout`]:
- bank: 0 Sine→Saw, 1 Sine→Square, 2 Pulse Width, 3 Formant, 4 Drive, 5 Imported
- lfo_shape: 0 Sine, 1 Triangle, 2 Saw, 3 Square, 4 S&H
- bit_depth: `"8"` = 9, `"4"` = 13
- Every parameter is `ParameterID{id, 1}`. Skews: `lfo_rate` 0.3, menv times 0.3, amp times 0.35.

#### Recommendation: `Source/PresetRecipes.h`

- Header-only, C++17: the build is `-std=gnu++17` for the plugin targets [HIGH: `build/build.ninja` flag census]. No `std::span`.
- Values are **raw engineering units**.
- Rule: **§A9 wins where it names a value; the Stage 3 recipe fills where §A9 is silent.** [ASSUMED: Taylor confirms.]

```cpp
namespace wtpresets {
struct Entry  { const char* paramId; float raw; };
struct Recipe { const char* id; const char* name; const Entry* entries; int count; };
template <int N> constexpr Recipe make (const char* id, const char* name, const Entry (&e)[N]) { return { id, name, e, N }; }

inline constexpr Entry kInit[]     = { { "bank", 0 } };   // == all defaults (a 1-entry array; C++ has no zero-length arrays)
inline constexpr Entry kStepped[]  = { {"bank",0}, {"position",0.5f}, {"interp",0}, {"lfo_sync",0}, {"lfo_shape",2}, {"lfo_rate",0.25f}, {"lfo_depth",1} };
inline constexpr Entry kSmooth[]   = { {"bank",0}, {"position",0.5f}, {"lfo_sync",0}, {"lfo_shape",2}, {"lfo_rate",0.25f}, {"lfo_depth",1} };      // interp default On
inline constexpr Entry kAlias[]    = { {"bank",4}, {"position",1}, {"bandlimit",0} };
inline constexpr Entry kDrive[]    = { {"bank",4}, {"position",0}, {"env_amount",1}, {"menv_attack",0.01f}, {"menv_decay",1.5f}, {"menv_sustain",0}, {"menv_release",0.8f} };
inline constexpr Entry kVowel[]    = { {"bank",3}, {"position",0.5f}, {"lfo_sync",0}, {"lfo_shape",1}, {"lfo_rate",0.1f}, {"lfo_depth",1}, {"amp_attack",0.6f}, {"amp_release",1.4f} };
inline constexpr Entry kPulse[]    = { {"bank",2}, {"env_amount",0.8f} };
inline constexpr Entry kPpg8[]     = { {"bank",1}, {"position",0.6f}, {"interp",0}, {"bit_depth",9},  {"lfo_sync",0}, {"lfo_shape",4}, {"lfo_rate",4}, {"lfo_depth",0.4f} };
inline constexpr Entry kPpg4[]     = { {"bank",1}, {"position",0.6f}, {"interp",0}, {"bit_depth",13}, {"lfo_sync",0}, {"lfo_shape",4}, {"lfo_rate",4}, {"lfo_depth",0.4f} };

inline constexpr Recipe kFactory[] = {
    make ("init", "Init - Additive Build", kInit),  make ("steppedSmooth", "Stepped Scan", kStepped),
    make ("smoothScan", "Smooth Scan", kSmooth),    make ("aliasDemo", "Alias Demo", kAlias),
    make ("driveSweep", "Drive Sweep", kDrive),     make ("vowelPad", "Vowel Pad", kVowel),
    make ("pulseNarrowing", "Pulse Narrowing", kPulse), make ("ppg8bit", "8-bit PPG", kPpg8),
    make ("ppg4bit", "4-bit PPG", kPpg4) };
}
```

**Changes from the Stage 3 lesson values.** The by-ear sign-off has to listen for each of these:

| Recipe | Change |
|---|---|
| steppedSmooth | shape Tri → **Saw**, rate 0.18 → **0.25** |
| aliasDemo | bank Sine→Saw → **Drive** (the locked change) |
| driveSweep | A 0.9 → **0.01**, D 1.6 → **1.5**, S 0.25 → **0** |
| vowelPad | Sine → **Tri**, 0.12 → **0.1**, depth 0.9 → **1.0** |
| ppg8bit | rate 3 → **4** |

**Consumers:**
- `applyFactoryPreset(id)` → table → targets → apply core. It also sets the preset name to `recipe.name` and bumps the revision.
- The `loadPreset` native for factory names, through the same function.
- `buildFactoryPresetDefs()` → the full normalized map with `output_level` omitted → `initializeFactoryPresets`.
- `getPresetCatalog` → the page.
- The modified check (targets cached at apply).
- Ids remain the `data-preset` contract: the 5 lesson buttons are a subset of the 9 ids.

#### Pitfalls

- **Drive Sweep copy conflict** [HIGH: `i18n.js` `lessonDriveSweep` and `tour.caption.driveSweep` lines 396-405 and 567].
  - The signed-off copy says each note "starts clean and grows gritty" / "starts as a sine". That describes the Stage 3 slow-attack envelope.
  - With §A9's A 0.01 / S 0, a note hits full drive within 10 ms and **relaxes back to the sine** over 1.5 s.
  - Recommended: keep §A9 and rewrite those two strings in 3 languages, e.g. "Each note strikes driven and relaxes back to a sine as the envelope decays."
  - The alternative is to amend §A9's row to the Stage 3 envelope. **Taylor decides** [ASSUMED].
- **G-LESSON must be re-transcribed** from §A9 independently (`viz-check/main.cpp:393-409`). It must not read `PresetRecipes.h`, or it would grade itself.
- **New G-FACTORY gate:**
  - For every entry:
    - `isfinite`
    - `convertFrom0to1(convertTo0to1(raw)) ≈ raw` (catches out-of-range clamps, choice values that are not integers, and the skew trap)
    - the file name is ASCII, has no `/\:`, and equals `createLegalFileName`
    - ids are unique
    - no recipe names `output_level` or `bank` = 5
  - Factory load by name and `applyFactoryPreset(id)` give identical parameter snapshots.
  - `output_level` is untouched by all 9 (set to −17.3 dB first).
  - The 5 page `data-preset` ids are a subset of the table ids. Extend the Stage 3 bridge cross-check.
  - NC: author one skewed value as a linear fraction → the round-trip arm must fail (memory `pattern_factory_preset_normalized_ignores_skew`).
- `init`'s defaults must equal the APVTS defaults. Then a fresh instance reports `name = "Init - Additive Build", modified = false`. Set the name in the processor constructor.

---

### 5. UI critic fixes: exact fix shape per item

| Item | Evidence | Fix shape | Gate |
|---|---|---|---|
| **W1** | `PluginEditor.cpp:227-237` emits, then reads the counters [HIGH] | In `uiReady`, assign `lastBankGeneration`, `lastBankIndex` and `lastImportVersion` **first**, then `emitBankUpdate(true); emitImportStatus(); forceCycleEmit = true;`. This is the order `timerCallback` uses (`:419-433`). Also emit `presetState` (forced). A transition in the window now re-sends on the next tick: a duplicate, never a loss. | Code inspection (the editor TU is not in any harness) + Standalone reopen during an import |
| **W3** | stepped knob `index.html:1477-1490` (`Math.max(WHEEL_PX_PER_NUDGE, px)` floors every event to a step); bank stack `:2349-2360` (one frame per event) [HIGH] | **Burst-aware accumulator**, the same in both:<br>• A discrete event steps exactly once. Discrete means `deltaMode !== 0`, or the first event of a burst (no wheel event in the last `WHEEL_GESTURE_MS`).<br>• Every later pixel event in the burst does `acc += dir*px`. A direction reversal zeroes `acc`.<br>• Step `trunc(acc / WHEEL_PX_PER_NUDGE)` (clamped to `WHEEL_MAX_NUDGES`) and keep the remainder.<br>• Reset `acc` 250 ms after the last event.<br>• The bank stack steps `1/(N-1)` per step. | Hands-on: a trackpad flick on bit_depth moves ≤ about 3 detents; one mouse notch = one detent. [ASSUMED: the WKWebView mouse-notch `deltaY` magnitude was not probed. Hence "first event of a burst = a step".] |
| **W3b** (adjacent, not in the critic table) | continuous knobs `:1344-1356`: `Math.max(1, px / WHEEL_PX_PER_NUDGE)`. At least 2 % per event, so a 50-event flick moves 100 %. | Same accumulator. **Flag to Taylor before changing**: this is working behaviour outside the named finding. | same |
| **W4** | page `onBankUpdate` clears `importError` (`:2250`); `uiReady` then re-emits the processor's latched error (`PluginProcessor.cpp` `importStatus` is never cleared) [HIGH] | The processor gets public `pollBankForImportStatus()` (message thread). It is called from the 250 ms `timerCallback` (`:952-956`). Its logic: `idx = getSelectedBankIndex(); if (idx != lastStatusBank) { lastStatusBank = idx; ScopedLock sl(bankStateLock); if (importStatus.state == error) { importStatus = {}; ++importStatusVersion; } }`. This mirrors the page rule "an error lives until the next bank change". It is a superset of "leaves Imported" and works with the editor closed. Consequence: a restore's `unsupported` notice also clears on a bank change, and the passthrough blob is still kept. | viz-check **G-S3W4-ERRCLEAR**: a too-short import → error; set bank 1; poll → state idle and version bumped. NC: poll with no bank change → error persists. |
| **W5** | the page's `heldNotes` only (`index.html:2477-2494`); editor destructor only `stopTimer()` (`PluginEditor.cpp:360-363`) [HIGH] | **Processor** (message thread): `std::array<bool,128> uiHeld`. `handleUiMidi` (`PluginProcessor.cpp:709-724`) sets it on note-on with velocity > 0 and clears it otherwise. `releaseUiHeldNotes()` queues a `noteOff` for each held note and clears them all. **Editor destructor:** `processorRef.releaseUiHeldNotes();` before `stopTimer()`. Belt and braces: the page calls `allNotesOff()` on `visibilitychange → hidden`. | state-check/viz-check **G-S3W5-UIHELD**: `handleUiMidi(60,true)`, render 100 ms, `releaseUiHeldNotes()`, render the release + 50 ms → `getSoundingVoiceCountForTesting()==0`. NC: without the release, the voice is still sounding. |
| **N1** | `PluginEditor.cpp:246-261` makes a fresh `make_shared<FileChooser>` per click [HIGH] | Copy O-AnalogEQ [HIGH: `PluginEditor.h:112-132`]: an editor member `std::unique_ptr<juce::FileChooser> importChooser;` plus `bool importDialogInFlight = false;`. In flight → return immediately. Otherwise set the flag, assign the member and launch. The callback clears **only the flag**, first, above every return. It never resets the chooser (that would destroy the running `std::function`). Keep the SafePointer bail. | Hands-on double-click on Import → one dialog |
| **N5** | `runImportJob` sets `pendingAutoSelect` (`:1083`) → `handleAsyncUpdate` selects Imported (`:1088-1105`) [HIGH] | **FIX.** The apply core clears `pendingAutoSelect` under `bankStateLock` before it applies (§1 code). `handleAsyncUpdate` reads and clears the flag under the lock at run time, so a queued callback becomes a no-op. A later, newer import publish re-arms it correctly. **No `cancelPendingUpdate()` is needed**: the flag, not the queue, is the authority. That is also why memory `pattern_asyncupdater_guard_flag_needs_cancel` does not bite here. `restoreImportedBank` already clears it (`:797`, `:826`, `:866`). | viz-check **G-S3N5**: `importFromMemory` (valid WAV) → poll until `done` → `applyFactoryPreset("aliasDemo")` → `handleUpdateNowIfNeeded()` → bank == 4. Sensitivity control: the same sequence without the apply → bank == 5, which proves the select was pending. |
| **N8** | the hero knob (`bindKnob`, own `wheelTimer`, `:1286-1356`) and the bank stack (own drag plus `wheelTimer`, `:2288-2360`) both call `sliderState.position.sliderDragStarted/Ended` [HIGH]. A stack wheel burst followed within 250 ms by a knob drag gives begin, begin, end (stack timer), end. `WebSliderParameterAttachment` → nested `beginChangeGesture` → a debug `jassert(! isPerformingGesture)` [HIGH: `JUCE/…/juce_AudioProcessorParameter.cpp:65-76`], and the host gesture closes mid-drag. | **FIX** (cheap). Page helpers `gestureBegin(id)` / `gestureEnd(id)` with a per-id refcount: `sliderDragStarted` only on 0→1, `sliderDragEnded` only on 1→0. Every begin/end in `bindKnob`, `nudge`, `resetToDefault` and `bindBankDrag` goes through them. | Code inspection + an optional Playwright stub probe counting `sliderDragStarted`/`Ended` on `position` (strict alternation) |
| **N9** | `restoreImportedBank` → `error "unsupported"` (`PluginProcessor.cpp:819-829`) → `importStatus` → `onImportStatus` (`index.html:2260-2266`) → `renderImportError` falls through to `import.err.generic` "import failed — the current bank is unchanged" (`:2225-2232`), although no import was attempted [HIGH] | Add `else if (importError === 'unsupported') setLabel(el, 'import.err.unsupported');` (literal key) plus the I18N entry from §3 (all three languages ≤ 240 px). | check-ui-labels state "import error unsupported"; check-i18n [15] key resolves |
| **N10** | `initLessons` toggles `.active` on click only (`index.html:2436-2441`). It never clears on a parameter edit, automation, preset load or restore [HIGH]. | `presetState` (§1). The processor caches the normalized targets at every apply or save. `isPresetModified()` = any parameter except `output_level` differs by more than 1e-4. For factory presets, targets are always rebuilt from the table; a restored user preset re-reads its file on the first editor tick (a missing file → no dot). The page lights `.tour-btn[data-preset=id]` iff `!modified`, and sets the panel dot iff `modified`. | viz-check **G-PRESET-STATE**: lesson → `{id, modified:false}`; nudge `position` → `modified:true`; re-apply → false. Restore round trip keeps the name. |
| **N11** | `#octDown` / `#octUp` carry only aria keys (`index.html:1044-1046`; `i18n.js:575-576`). They are absent from TIP_BINDINGS (`i18n.js:593-639`) [HIGH]. | Add `['#octDown','octDown'], ['#octUp','octUp']` plus the two `{t,b}` entries from §3. | boot-all-uis `--strict-tips` (bound count +2), check-i18n [2] |
| **N12** | `fmtDb` returns ASCII `'-inf'` (`index.html:1098`); EXEMPT `['-inf', …]` (`i18n.js:589`); tooltip bodies "(-inf)" en/fr/zh (`i18n.js:366-371`) [HIGH] | **Page only.** `return '−inf'`. Update the EXEMPT text to `'−inf'` with the reason "typographic minus, matching the page's −6.0 dB; the host string stays ASCII -inf". Change the three tooltip bodies to "(−inf)". **Leave the C++ host text `"-inf"` and its parser alone** (`PluginProcessor.cpp:83-97`; state-check P0 gates it). | check-i18n [14]; state-check P0 unchanged |
| **N13** | stepped knobs use `ComboBoxState.setChoiceIndex` (`index.html:1440-1446`). That fires `WebComboBoxParameterAttachment::valueChanged` → `setValueAsCompleteGesture`, one begin/end per detent [HIGH: `JUCE/…/juce_ParameterAttachments.cpp:425-434`, `:59-67`]. `ComboBoxState` has no gesture API [HIGH: `js/juce/index.js:389-450`]. Wrapping it in a native begin would nest and assert. | New native `stepKnobDrag(id, phase, index)`:<br>• `id` must be `bit_depth` or `lfo_div`.<br>• `begin`: if not open, `p->beginChangeGesture()` and mark it open.<br>• `move`: if open, `idx = jlimit(0, n-1, index)`; `norm = p->convertTo0to1((float) idx)`; set it if it differs.<br>• `end`: if open, `endChangeGesture()` and mark it closed.<br>• The editor destructor ends any open gesture.<br>Page: pointerdown → `begin`; onMove → a local `dragIdx` for the visual plus `move`; onUp/cancel/lost capture → `end`. Arrow keys, wheel and dblclick keep `setChoiceIndex`: a discrete step is one gesture, which is correct. | pluginval s10 (Debug gesture asserts) + Logic Touch-automation hands-on: one write region per drag |

---

### 6. UI gate impact

**check-i18n** (`scripts/check-i18n.js`, header `:20-120`):
- [1] / [5]: every new key has en/fr/zh-Hans with `reviewed: false` (fr) and `'mt'` (zh). TIP entries carry `t` + `b`.
- [2]: TIP_BINDINGS +7 (5 preset + 2 octave).
- [11]:
  - The new markup has no `title=`.
  - `aria-label`s use `data-i18n-aria`.
  - The input uses `data-i18n-placeholder` (`applyI18nAttributes` already handles it, `index.html:2745-2757`).
  - The served `preset-manager.js` v1.0.9 passes the title scan [HIGH: `check-i18n.js:982-1060`; `preset-manager.js:433-441`].
- [12]: option text goes through `setLabel`/`setRaw`. The served module is title-scanned only and prints a NOTE; that is expected.
- [13] / [15]: literal keys only. Factory labels use a literal id→labeler map. Every new LABELS key must be referenced, so remove any that end up unused.

**fr lint:**
- Glossary roots as in §3.
- U+00A0 before `? : ;` and inside `« »`; U+2019 apostrophes.
- No `sauver`, `sauvegarder` or `lire` [HIGH: `i18n-fr-glossary.js:385-397`].

**zh lint:** all new entries at `'mt'`, counted, not failed (Stage 3: 142 entries → about 172). Glossary terms as in §3.

**check-ui-labels** [7] (the geometry diff):
- **Pins:**
  - `.preset-panel` 352
  - `.preset-label` 88
  - Save 74, Delete 72, Cancel 62
  - `#preset-select` 296
  - arrows 22
  - popover 352 × 92, message line nowrap
- **New `tests/i18n-states.json` states.** Append them; the walk is cumulative, so close popovers with an Escape eval between states.
  1. `__stubEmit('presetState', {name:'Init - Additive Build', id:'init', factory:true, modified:false})`, which shows the widest fr name
  2. `modified:true` (the dot)
  3. click `#preset-save` (title, placeholder, Save/Cancel)
  4. type "Alias Demo" plus submit (`preset.errFactoryName`)
  5. select the user preset "Bright Pad", then click `#preset-delete` (`preset.confirmDelete`)
  6. `__stubEmit('importStatus', {state:'error', filename:'future.wav', frames:0, error:'unsupported'})` (N9)
- **New `tests/ui-stub/generic-overrides.json` `natives`** [HIGH: mechanism `scripts/ui-stub/generic-juce-stub.js:411-420`, README `:108-126`]:
  - `getPresetList`: the 9 factory names + `"Bright Pad"`
  - `getPresetCatalog`: `{factory:[{name,id}…], user:["Bright Pad"]}`
  - `getCurrentPreset`: `"Init - Additive Build"`
  - The stub's built-in preset functions (`:273-303`) cover the module names. Without these overrides the page would render the bland "Default / Preset A" and the pins would be unmeasured at real widths.

**boot-all-uis `--strict-tips`:**
- Every new TIP selector must be **static markup**, present at settle. Popover controls stay in the DOM while hidden.
- `getPresetCatalog` and `stepKnobDrag` would otherwise be reported as invented natives. The override covers the catalog; `stepKnobDrag` falls to the benign default, which is harmless.

**Bridge cross-check** (Stage 3 verify method):
- 20/20 `withNativeFunction` ↔ page/module `getNativeFunction`
- 4 events
- `data-preset` ⊂ catalog ids
- the relay census is unchanged: 13 slider / 6 combo / 2 toggle. N13 adds a native, not relays.

---

### Planner inputs

| # | Task-shaped item | Files | Depends on | Gate / proof |
|---|---|---|---|---|
| A1 | Add `Source/PresetRecipes.h` (9 recipes, raw units, §A9 + fill rule) and list it in CMake `target_sources` | `PresetRecipes.h`, `CMakeLists.txt` | Taylor: Drive Sweep copy-vs-§A9 decision | G-FACTORY (+NC) |
| A2 | Refactor `applyFactoryPreset` to table → `applyNormalisedTargets` core (output_level skip, defaults-first, gestures, `pendingAutoSelect=false` under lock); preset name/targets/revision under `presetLock` | `PluginProcessor.{h,cpp}` | A1 | G-LESSON re-transcribed from §A9; G-S3N5 (+sensitivity) |
| A3 | `OuariconPresetManager` member; `ouaricon_add_module(... preset-manager)`; `preset-manager.js` in binary data + `getResource` branch; lazy `ensureFactoryBankOnDisk()`; `buildFactoryPresetDefs()`; `applyUserPresetJson(var)`; walk order; `sanitisePresetName` | `CMakeLists.txt`, `PluginProcessor.{h,cpp}`, `PluginEditor.cpp` | A2 | G-PRESET-USER (in-memory JSON round trip, output_level ignored, unknown keys ignored, malformed values → default); check-ui-labels "every resource served" |
| A4 | State: `currentPreset` root property (save; isVoid-gated restore; absent → ""); NO getStateAsXml/setStateFromXml | `PluginProcessor.cpp` | A3 | state-check P11 (round trip both ways + absent) + existing P3/P4/P9/P10 green |
| A5 | 12 new natives (10 module names incl. 2 refusal stubs, `getPresetCatalog`) + `presetState` event in timer/uiReady | `PluginEditor.{h,cpp}` | A3 | Bridge cross-check 20/20 (with A11), boot-all-uis |
| A6 | Imported × presets = option (a); gate the hard constraint | (tests) | A3 | G-PRESET-IMPORTED (+NC) |
| A7 | Preset panel markup/CSS in keyboard row (352/556 geometry, pins from §3), popover (save/replace/delete, fixed 352×92), dynamic import of the module, factory labeler map, lesson highlight + caption from `presetState` | `index.html` | A5 | check-ui-labels, boot-all-uis `--strict-tips`, **visual checkpoint (Taylor)** |
| A8 | i18n: ~30 entries (§3 tables), 7 TIP_BINDINGS rows, EXEMPT `−inf`, Drive Sweep copy rewrite if §A9 kept | `js/i18n.js` | A7 | check-i18n, fr lint CLEAN, zh lint (mt counted) |
| A9 | Gate fixtures: 6 `i18n-states.json` states, `tests/ui-stub/generic-overrides.json` natives | `tests/` | A7, A8 | check-ui-labels ALL PASS |
| A10 | W1 snapshot-first; N1 FileChooser member + in-flight flag; W5 editor dtor → `releaseUiHeldNotes()` | `PluginEditor.{h,cpp}` | — | inspection + Standalone hands-on |
| A11 | N13 `stepKnobDrag` native (begin/move/end, dtor closes) + page drag path | `PluginEditor.{h,cpp}`, `index.html` | — | pluginval s10 Debug, Logic touch hands-on |
| A12 | W4 `pollBankForImportStatus()` in processor timer; W5 `uiHeld` tracking + `releaseUiHeldNotes()` | `PluginProcessor.{h,cpp}` | — | G-S3W4-ERRCLEAR (+NC), G-S3W5-UIHELD (+NC) |
| A13 | Page fixes: W3 burst accumulator (stepped knob + bank stack), N8 gesture refcount, N9 `unsupported` branch, N11 tips, N12 `−inf`, W5 `visibilitychange` allNotesOff | `index.html`, `js/i18n.js` | — | check-ui-labels (N9 state), check-i18n, hands-on trackpad/mouse |
| A14 | **Ask Taylor:** (1) Drive Sweep: §A9 envelope + new copy vs keep Stage 3 envelope; (2) W3b continuous-knob wheel (unflagged by critic); (3) 37 px keys acceptable | — | before A1/A7 | decision record |
| A15 | Log in CODE_REVIEW.md: N5 FIXED, N8 FIXED (with this rationale); module v1.0.9 stale-child + output_level reset remain upstream (not touched — `modules/` is outside the commit scope) | `CODE_REVIEW.md` | — | — |

### Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| X1 | The "§A9 wins, Stage 3 fills the gaps" rule, and the 4-bit variant as the 9th preset | §4 | Wrong recipe values; sign-off rework |
| X2 | Drive Sweep follows §A9 and its copy is rewritten | §4 | The lesson text contradicts the sound |
| X3 | 37 px white keys are acceptable | §3 | The layout is reworked at the visual checkpoint |
| X4 | WKWebView mouse-notch `deltaY` (hence "first event of a burst = one step") | §5 W3 | The mouse wheel feels sluggish or too quick |
| X5 | JUCE's FLAC writer size ≈ `flac -5` | §2 | Only the size table changes, not the recommendation |
| X6 | WKWebView text width ≈ Chromium ±1 px | §3 | A pin is short by a pixel; the gate catches it |
| X7 | All fr and zh strings are machine-drafted | §3 | A native review is a ship-bar item, as in Stage 3 |

---

## §B — Stage 4 Polish, Research Part B: DSP and import fixes

**Researched:** 2026-10-06
**Scope:** Stage 2 W3 and W4; Stage 2 notes 1–8; Stage 3 W2, N2, N3, N4, N6 and N7; the regression guard.
**Out of scope** (Parts A and C): the preset manager, the UI fixes (W1, W3–W5, N1, N5, N8–N13), QUAL-04, PERF-02, CI and the release docs.
**Confidence:** HIGH. Every fix shape was built in a scratch tree against the real processor, and every gate was run against both the shipped `Source/` and the fixed tree.

**Scratch evidence:** `research-probes/B/`. It contains:
- `src-fix/`: a patched copy of `Source/`, with `proc.diff` and `w4.diff` against the shipped files
- `fixprobe-{orig,fix}`: the gate prototypes, linked against the real `OSimpleWavetableAudioProcessor` with test hooks on
- `w4probe`, `b64probe`, `sizeprobe`, `w3repro.py` and `undef/`
- the outputs: `*.out`, `orig.bin` and `fix.bin`

`Source/`, `tests/`, the shared `build/` and the git index were not touched. JUCE is `/Users/taylorbrook/JUCE` 8.0.15, built `-O2`/`-O3` in scratch, on an Apple M4 Max.

**Constraints honoured (CONTEXT.md, verbatim):**
- "Every DSP fix needs a gate with a negative control (Stage 2 W3, W4; notes 3, 4). G-* goldens move only where a fix intends it, and that must be documented."
- "All 6 offline drivers + viz-check, `--alloc-check` 0, and R-GOLD must stay green."
- "Re-run the Stage 3 binary null test against `f4eea85a` (or the Stage 3 install) on the cases no Stage 4 fix touches. A case a fix does touch must show the intended difference only."

---

### Summary

- **W3 is real, and it is now reproduced on the installed VST3** (the critic had not reproduced it).
  - Scenario: Poly, wheel up, note off, wheel back to centre while idle, switch to Mono, play a note.
  - Result: the note plays **+199.98 c**. A wheel left at −1 st while idle gives **+100.00 c** (that bend is lost).
  - **Fix:** seed voice 0 with the last wheel value at the Poly→Mono switch. That is 6 lines in `PluginProcessor.cpp`.
  - Prototype gate: 0.00 c with the fix; the negative control gives +199.98 / +100.00 c.
- **W4 is confirmed.**
  - **Cause:** the frozen outgoing cycle (captured at the OLD mip level) is read at the live phase, so it plays at the NEW pitch.
  - At C7, level 4's 64 harmonics run to 134 kHz. Everything above fs/2 folds.
  - **Measured on the real 5 ms fade, 60→96 at 48 kHz:** alias RMS is **−20.0 dB** (Sine→Saw) and **−23.0 dB** (Drive) relative to the new note. The alias peak reaches **−1.8 dB**.
  - With a 2 s magnifier fade the alias reads **−24.3 / −26.0 dB**.
- **W4 fix:** the frozen cycle keeps its own phase and the rate it was heard at (`xfPhase` / `xfInc`). That is about 20 lines in `WtVoice.h`.
  - After the fix, alias is at the analyser floor (−102.0 / −107.2 dB, equal to the no-jump control).
  - The output equals `(1−w)·old-continuation + w·hard-switch` to 7.1e-8 across 5 trigger sources. The negative control is off by 0.54–1.00.
  - Every non-pitch trigger stays bit-identical, so the QUAL-03 goldens hold.
- **Note 3:**
  - **Real capacity:** `ensureSize(32768)` actually reserves 49,160 bytes (about 5,460 events); the critic's ~3,600 is low.
  - **Measured:** a 6,000-event host block allocates on the audio thread: 1 malloc in Poly, 7 in Mono, where `wheelMidi` is only 4 KB.
  - **Fix:** cap each chunk's copy at the reserved bytes, ending the chunk early (a 0-length chunk is allowed), and skip messages over 3 bytes.
  - With the fix: 0 allocations, and every event is still delivered (the pitch after the flood is correct, and the note-off still lands).
- **Note 4:** `processBlock` before `prepareToPlay` **crashes** with SIGSEGV (`knobBuf` is empty). This was measured in a forked child.
  - **Fix:** a `prepared` atomic, set at the end of `prepareToPlay`, with silence until then.
  - Prototype: exact 0 output, then normal sound after prepare. The negative control is SIGSEGV.
- **Note 8:** the class is 7 `#if OSIW_TEST_HOOKS` sites in 3 headers, not 1:
  - `WtVoice.h:104`
  - `PositionSmoother.h:51`
  - `PositionLfo.h:94,166,184,211,259`

  None fires in today's TUs; it is a latent hygiene issue. **Fix:** one `TestHooks.h` header, included first.
- **W2 (the 96 MB drop):**
  - **Measured:** 664 ms for `convertFromBase64` at 96 MiB, plus 300 ms of JSON parse and 63 ms of UTF-8 scan, all on the message thread. The WKWebView NSString conversion and the JS base64 encode come on top.
  - **Pre-sizing buys nothing** (674 ms; macOS `realloc` grows large blocks in place). The cost is JUCE's per-byte `writeByte`, about 4.9 ns per decoded byte.
  - A full 256-frame source needs only 1–4 MiB for mono/stereo files at 16/24/32f.
  - **Recommend:** lower the drop cap to 16 MiB. That is about 170 ms of measured C++ cost, 6× less. Keep the decode where it is, and change the `tooLarge` copy to point at the Import button.
- **N4 confirmed:** after `releaseResources` the display stays "sounding" (note 64, amp 1.00). **Fix:** a shared `resetDisplayState()`.
- **N7:** strip C1, the 12 Unicode `Bidi_Control` characters, LS/PS, BOM/ZWSP/WJ, lone surrogates and U+FFFE/FFFF. Keep ZWJ/ZWNJ.
- **Fix-or-log verdicts:**
  - Stage 3 N2: **log**
  - Stage 3 N3: **log**
  - Stage 2 note 5: **log**
  - Stage 2 note 6: **log**; a full import costs about 17–24 ms
  - Stage 2 note 7: **log**; the premise is false, because decoding runs *before* the lock
  - Stage 2 notes 1, 2 and Stage 3 N6: by design, wording drafted below
- **Regression check, shipped vs fixed tree (10 render scenarios):**
  - 7 scenarios are bit-identical.
  - The 3 level-crossing pitch-change scenarios differ only at samples t+1..t+239, the 5 ms fade.
  - **In the Stage 3 null test, only `mono_legato` is touched**: its 65→67 jump crosses L4→L5 at 0.6 s.

---

### 1. Stage 2 W3: Mono wheel seed after Poly→Mono

#### Defect

**The JUCE Synthesiser sends the wheel only to voices that are playing on that channel.**
- `juce_Synthesiser.cpp:403-410`: `for (auto* voice : voices) if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel)) voice->pitchWheelMoved (wheelValue);` `[VERIFIED: /Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp:403-410]`
- It still stores the value: `int lastPitchWheelValues [16];` (protected) `[VERIFIED: juce_Synthesiser.h:581]`
- Poly voices seed from that value at note-on: `pitchWheelPos = currentPitchWheelPosition;      // SEED the wheel at note-on` `[VERIFIED: WtVoice.h:272]`

**The Mono path never seeds the wheel.** `noteOnDirect` runs `noteHz = …; lastNote = midiNote; updatePitch();` with no wheel seed `[VERIFIED: WtVoice.h:452-457]`. It plays whatever `pitchWheelPos` voice 0 last saw.
- In Mono, every wheel event goes to voice 0, whether it is idle or not: `else if (m.isPitchWheel()) { v0->pitchWheelMoved (m.getPitchWheelValue()); }` `[VERIFIED: PluginProcessor.cpp:548-551]`
- So voice 0 can only be stale **after a Poly→Mono switch**, and the switch does nothing to the wheel: `synth.allNotesOff (0, false); monoStack.clear(); lastVoiceMode = mode;` `[VERIFIED: PluginProcessor.cpp:388-393]`

**Reproduced on the installed `-dev.vst3`** (`w3repro.py`, pedalboard, 48 kHz, Sine→Saw pos 0, A4 measured by zero crossings over 0.2–0.9 s):

| Arm | Stimulus | Measured | Expected | Error |
|-----|----------|----------|----------|-------|
| A (critic) | Poly A4, wheel +8191, note off, wheel back to centre while idle, then Mono, then A4 | 493.876 Hz | 440.000 | **+199.98 c** |
| B | Poly, no note, wheel −4096 (−1 st), then Mono, then A4 | 440.000 Hz | 415.305 | **+100.00 c** (bend lost) |
| C | control, no wheel | 440.000 | 440.000 | +0.00 c |

`[VERIFIED: research-probes/B/w3repro.py run on ~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3]`

#### Fix shape (prototyped in `src-fix`, `proc.diff`)

```diff
// PluginProcessor.h (audio-thread members)
+    int wheelNow = 8192;   // W3: last pitch-wheel value on ANY channel (Mono is omni)
+   #if OSIW_TEST_HOOKS
+    bool testMonoWheelSeed = true;   // + setMonoWheelSeedForTesting (bool)
+   #endif

// renderBlock, mode switch (PluginProcessor.cpp:388-393)
     if (mode != lastVoiceMode) {
         synth.allNotesOff (0, false); monoStack.clear(); lastVoiceMode = mode;
+        if (mode == 1 /* && testMonoWheelSeed */)
+            wtVoices[0]->pitchWheelMoved (wheelNow);   // voice 0 missed the wheel while idle in Poly
     }

// chunk MIDI copy loop (every event is already visited there)
+    if (meta.numBytes >= 3 && (meta.data[0] & 0xf0) == 0xe0)
+        wheelNow = (meta.data[1] & 0x7f) | ((meta.data[2] & 0x7f) << 7);
```

**Why seeding at the switch covers the whole class.**
- In Mono, voice 0 already receives every wheel event.
- In the Mono→Poly direction, the Synthesiser's `lastPitchWheelValues` is kept current by the forwarding at `PluginProcessor.cpp:507-516`.
- The mode cannot change mid-block, so updating `wheelNow` in the copy loop is safe.

**Edge:** `wheelNow` is omni, while `lastPitchWheelValues` is per channel. A multi-channel controller with a different wheel per channel is a non-goal.

**RT cost:** one int store per wheel event. No allocation.

#### Gate: `dsp-check` G-MONO-WHEEL (prototype in `fixprobe.cpp::w3`)

- **Stimulus:**
  - Rig at 48 kHz, block 512, `baseParams (sineSaw, 0)`.
  - Run 56 blocks of Poly history, then `set (voiceMode, 1)` on a block boundary.
  - Note-on A4 at +64 samples, render 1 s, measure f by zero crossings over 0.2–0.9 s.
- **Arms:** A, B and C, as above.
- **Verdict:**

  | | Arm A | Arm B | Arm C |
  |---|---|---|---|
  | Fix | \|cents\| ≤ 1 | \|cents\| ≤ 1 | \|cents\| ≤ 1 |
  | Negative control (`setMonoWheelSeedForTesting (false)`) | ≥ +150 c | ≥ +75 c | not measured |

  The ≤ 1 c threshold follows G-PITCH.

- **Measured in the prototype:**
  - Fix: A **+0.00 c**, B **−0.00 c**, C +0.00 c.
  - Negative control: A **+199.98 c**, B **+100.00 c**.
  - The shipped `Source/` build gives the same numbers as the negative control.

**Goldens:** G-PITCH, G-BEND (its wheel is set *before* the note), G-MONO, G-RETRIG and G-RETRIG-VEL have no Poly→Mono switch after a wheel move, so they are unchanged.

The `--alloc-check` scenario (`dsp-check/main.cpp:1352-1381`) does go wheel sweep → Poly→Mono, so its Mono notes are now seeded at 16128. That gate counts allocations only, so it is unaffected (the seed is one int).

**Confidence:** HIGH.

---

### 2. Stage 2 W4 (Open Question 5): upward-legato crossfade source mip

#### Defect, precisely

1. A legato move (`noteOnDirect (96, v, false)`) calls `updatePitch()`, so `inc` jumps to 96's rate before the next render `[VERIFIED: WtVoice.h:455-457, 623-629]`.
2. The next render runs `refreshReadConfig()`. Strict level is higher for 96, and "moving UP a level switches immediately" `[VERIFIED: WtVoice.h:634-644]`, so `checkConfig (oldCfg, next)` fires.
3. `checkConfig` captures the outgoing cycle from the **old** level: `wt::readSample (*ob, oldCfg.level, ph, oldCfg.interp, capPos, latchedFrame)` `[VERIFIED: WtVoice.h:714-721]`.
   - At 48 kHz, MIDI 60 is level 4: `x = 261.6·2048/48000 = 11.16 → ceil(log2) = 4`, so kmax = 64.
4. The fade then reads that frozen cycle at the **live** phase, which now advances at 96's increment: `quant.apply (readFrozen (frozen[frozenActive], phase))` `[VERIFIED: WtVoice.h:401-409]`.
   - The level-4 cycle therefore plays at 2093 Hz. Its harmonics run up to 64 × 2093 = 134 kHz, and every harmonic k ≥ 12 (k·2093 ≥ 24 kHz) folds back into the audible band.
   - That alias is weighted by `1 − w`, a 5 ms raised cosine `[VERIFIED: WtVoice.h:139, 736-739]`.

The same mechanism fires for **every** trigger that coincides with a pitch change (memory `pattern_gate_inherits_the_review_findings_scope`: gate the class):

| # | Trigger | Where |
|---|---------|-------|
| a | Mono legato up | `noteOnDirect`, retrigger = false |
| b | Mono fallback up (release the top note, return to a higher held note) | `setPitchNote` `[VERIFIED: WtVoice.h:484-489]` |
| c | Mono retrigger from a sounding release tail with a new note | `noteOnDirect`, retrigger = true, `wasSounding` (no `cfgFresh`) `[VERIFIED: WtVoice.h:459-479]` |
| d | Poly or Mono pitch wheel crossing a level boundary | `pitchWheelMoved` → `updatePitch` `[VERIFIED: WtVoice.h:315-319]` |

Downward moves read fewer harmonics at a lower pitch, so they do not alias. They do change under the fix, because the outgoing part keeps its pitch.

#### Measured alias before the fix (`w4probe`, the voice driven directly, the shipped crossfader verbatim)

**Metric A** is the analytic alias on the real 5 ms fade: `e[n] = (1−w)(F − BL_F)(φ_n)·env·0.5`, where `BL_F` is the frozen cycle with every harmonic `k·f_read ≥ fs/2` removed. Sampling folds those components at full amplitude, so `e` *is* the alias. Values are dB relative to the new note's steady RMS.

**Metric C** is the magnifier: the fade forced to 2 s, then a Kaiser-38 FFT of 2^15 points taken 0.35 s after the jump, masking ±14 bins around k·f60. Since f96 = 8·f60 exactly, the mask covers both notes.

| Bank, pos 1 | fs | capture level | A: alias peak | A: alias RMS | C: bug | C: fix | C: no-jump floor |
|---|---|---|---|---|---|---|---|
| Sine→Saw | 44.1k | L4 | −1.7 dB | **−18.8 dB** | −23.8 | −102.0 | −102.0 |
| Sine→Saw | 48k | L4 | −1.8 dB | **−20.0 dB** | −24.3 | −102.0 | −102.0 |
| Sine→Saw | 96k | L3 | −8.9 dB | −25.2 dB | −103.4\* | −102.0 | −102.0 |
| Drive | 44.1k | L4 | −6.8 dB | **−21.3 dB** | −24.4 | −107.2 | −107.2 |
| Drive | 48k | L4 | −6.3 dB | **−23.0 dB** | −26.0 | −107.2 | −107.2 |
| Drive | 96k | L3 | −11.1 dB | −28.6 dB | −71.2\* | −107.2 | −107.2 |
| Pulse | 48k | L4 | +11.6 dB | −7.2 dB | −4.8 | −78.0 | −78.0 |

With the fix, metric A is −128 to −135 dB in every row, which is numerical zero.

\* **The magnifier is blind at 96 kHz.** fs/f60 = 366.93, so the folded lines land 17 Hz from a harmonic of f60, inside the ±41 Hz mask. Run the alias gate at **48 kHz**: fs/f60 = 183.47, so folds land halfway between harmonics.

The critic's −21 dB estimate matches the 44.1/48 kHz RMS values.

`[VERIFIED: research-probes/B/w4probe.out; command: ./w4probe]`

#### Fix shape (prototyped, `w4.diff`): the frozen cycle keeps the rate it was heard at

```diff
+    double xfPhase = 0.0, xfInc = 0.0;   // the frozen cycle's own phase and rate
+    double renderedInc = 0.0;            // inc of the last rendered sample (set after the render loop)

// render loop, crossfade branch (WtVoice.h:401-409)
-   quant.apply (readFrozen (frozen[frozenActive], phase))
+   quant.apply (readFrozen (frozen[frozenActive], xfPhase))
+   xfPhase += xfInc; if (xfPhase >= 1.0) xfPhase -= 1.0;
// after the loop
+   renderedInc = inc;

// checkConfig (WtVoice.h:707-728)
+   const double curOffset = folding ? xfPhase - phase : 0.0;   // a running frozen cycle is read at xfPhase
    for i: liveOld = readSample (old cfg ...);
-          dst[i] = folding ? (1-w) * cur[i] + w * liveOld : liveOld;
+          curVal = exactlyEqual (curOffset, 0.0) ? cur[i] : readFrozen (cur, wrap01 (i/2048 + curOffset));
+          dst[i] = folding ? (1-w) * curVal + w * liveOld : liveOld;     // phase-aligned FOLD
+   xfPhase = phase;  xfInc = renderedInc;
```

**Exactness for non-pitch triggers.** Bank, interp, bandlimit and `forceLevel` triggers have `renderedInc == inc`, and `xfPhase` starts equal to `phase`. The two then follow the identical double sequence, so the output is **bit-identical** to the shipped build. Without a pitch change the fold has `curOffset == 0`, so it is also bit-identical.

**Residual in the fold case.** This is a second trigger within 5 ms of a level-crossing pitch jump, for example a bank switch right after a legato jump.
- The folded remainder takes the newest rate, which is the pre-fix behaviour for that remainder.
- Measured (Drive, 60→96, then a bank switch 128 samples later):

  | Measure | Shipped | Fixed |
  |---------|---------|-------|
  | Magnifier alias | −24.5 dB | −24.3 dB (not worse) |
  | Real-length max \|Δy\| / steady | 1.765 | **1.000** |

- A "two concurrent fades" scheme would close the residual, but it is not recommended for v1.0.0. Log the residual in CODE_REVIEW.

**Timbre note.** A level-crossing legato is now a 5 ms two-pitch crossfade: the old note fades out at its own pitch. Add a Mono legato octave jump to Taylor's QUAL-04 listening pass.

#### Gates (prototyped in `fixprobe.cpp::w4`, at 48 kHz, through the processor Rig, block 64)

**1. G-LEGATO-XF (exactness, real 5 ms).** For each arm, the reference is `(1−w_k)·y_old[t+k] + w_k·y_hard[t+k]` for k < 240, where:
- `y_old` is the same render without the pitch change (for arm c: the same note retriggered, so the envelope and velocity ramp match)
- `y_hard` is the render with `setXfadeLenOverrideForTesting (0)`

Verdict:

| | Threshold | Measured |
|---|---|---|
| Fix | max \|y − ref\| ≤ 1e-6 | 5.08e-8 … 7.10e-8 on all 5 arms |
| Negative control (`setXfadeKeepRateForTesting (false)`) | ≥ 0.1 | 0.544 / 1.001 / 1.000 / 0.998 / 0.848 |

The 5 arms are:
- legato 60→96 Sine→Saw
- legato 60→96 Drive
- fallback 60→96 Drive
- sounding retrigger 60→96 Drive
- Poly wheel F#4 +2 st Drive

The shipped build measures the same as the negative control.

**Add a confinement check to this gate:** the fixed and negative-control renders must be bit-identical before t and after t+240. Measured: 0 differing samples. This proves that only the fade window changes.

**2. G-LEGATO-ALIAS (magnifier).**
- Sine→Saw and Drive at pos 1, 60→96, `setXfadeLenOverrideForTesting (2 s)`.
- Measure metric C at t+0.35 s.

Verdict:

| | Threshold | Measured |
|---|---|---|
| Fix | ≤ no-jump control + 1 dB | −102.0 vs −102.0; −107.2 vs −107.2 |
| Negative control | ≥ control + 40 dB | −24.3 / −26.0, a margin of 77.7 / 81.2 dB |

This is a ratio verdict against the gate's own floor, so it needs no absolute slack.

**3. Optional click guard** (G-STEAL style): max |Δy| over the fade divided by the steady max |Δy|. Measured on the 60→96 Drive arm: shipped **1.532** (fails the suite's 1.5), fixed **1.000**.

#### Goldens affected

From the orig-vs-fix render compare (`cmp.py orig.bin fix.bin`):
- **Bit-identical:**
  - default chord
  - bank / interp / bandlimit switches mid-note
  - Mono legato at the same level (60→64)
  - Mono retrigger (48)
  - 17-note steal
  - Poly→Mono with no wheel
  - S&H LFO
- **Change only inside [t+1, t+239]:**
  - Mono legato crossing 65→67 (Drive): max diff 0.90
  - Poly wheel sweep crossing one level: 0.095
  - Mono 60→96: 1.00

**R-GOLD expectation:**
- All PASS/FAIL states unchanged.
- **dsp-check G-BLOCK**'s detail string `peak x.xxx` *may* move. Its Mono arm goes 60→64→67, with 67 crossing L4→L5 at 48 kHz (`dsp-check/main.cpp:1317-1318`). The gate compares two renders of the same build, so PASS holds. Name it as an allowed R-GOLD diff, with the reason.
- The import-check QUAL-03 exactness and ratio tiers have no pitch change, so they stay bit-identical.
- mod-check's wheel arm (`mod-check/main.cpp:1501`, +0.69 st) crosses no level.

**Confidence:** HIGH.

---

### 3. Stage 2 notes 3, 4, 8

#### Note 3: MIDI scratch buffer growth path

**Defect:**
- `chunkMidi.ensureSize (32768)` and `wheelMidi.ensureSize (4096)` `[VERIFIED: PluginProcessor.cpp:57-58, 302-305]`.
- `MidiBuffer::ensureSize` → `data.ensureStorageAllocated` → `setAllocatedSize ((min + min/2 + 8) & ~7)` `[VERIFIED: juce_MidiBuffer.cpp:123; juce_ArrayBase.h:240-246]`. That reserves **49,160 B** and **6,152 B**.
- Each event costs a 6-byte header plus its bytes: `numBytes + sizeof (int32) + sizeof (uint16)`, inserted with `data.insertMultiple` `[VERIFIED: juce_MidiBuffer.cpp:152-155]`.
- A 3-byte event is therefore 9 B, so the buffers grow past **5,462 events per chunk** (chunkMidi) and **683 wheel events** (Mono `wheelMidi`).
- `juce::Array` exposes no capacity getter (only `ArrayBase::capacity()`, which is internal), so the guard must count bytes against the known reserve.

**Measured** (`fixprobe`, malloc_logger, one 512-sample host block with 6,000 events at a single sample plus a wheel after it):

| Build | Poly allocs | Mono allocs |
|-------|-------------|-------------|
| Shipped | **1** | **7** |
| Fixed | **0** | **0** |
| Fixed, guard off | 1 | 2 |

In every build the wheel after the flood landed: 493.88 Hz, exactly +2 st. The note-off landed too: tail RMS 0.

**Fix shape (prototyped):**
- An iterator-based chunk fill that counts bytes. When the next event does not fit in `kMidiChunkBytes`, end the chunk at that event (`n = p − start`, which may be 0). The Synthesiser handles trailing events even for a 0-sample render `[VERIFIED: juce_Synthesiser.cpp:195-236, for_each after the loop]`, and `renderMono` with `numSamples = 0` only handles events.
- Skip events with `numBytes > 3` (sysex and meta; neither path reads them).
- Set `kWheelMidiBytes = kMidiChunkBytes`, because `wheelMidi` is a subset of a chunk.
- Set `lastChunkLen` only when `n > 0`. `lfo.lastValue (0)` already returns 0 `[VERIFIED: PositionLfo.h:204-207]`.

The normal path is bit-identical: the same events, the same order, the same chunks.

**Gates:**
- **G-MIDI-FLOOD (functional, dsp-check).**
  - Poly and Mono arms.
  - Verdict: the pitch after the flood is +2 st within 1 c, and the tail RMS after note-off is < 1e-6.
  - Negative control: the flood must exceed the reserve (6,000 > 5,462); assert `6000 * 9 > 49160` in the gate.
- **The alloc arm** runs inside `--alloc-check`.
  - Add the Poly and Mono flood blocks to `allocScenario()` (armed): 0 allocations.
  - Add a separate arm on a fresh Rig with `setMidiCapacityGuardForTesting (false)`: it must count ≥ 1 allocation (measured 1 / 2).

**Adjacent, not fixed:** `midiCollector.removeNextBlockOfMessages` inserts the UI notes into the **host-owned** `midi` buffer. That buffer's growth belongs to the host or JUCE wrapper, and the UI produces only a few events per block. Log it.

**Pitfall:** `MidiBuffer::addEvent` scans linearly for the insertion point (`findEventAfter`), so a flood is O(n²) in time. That is a CPU concern only, at about 5k events.

#### Note 4: processBlock before prepareToPlay

**Defect:**
- The constructor leaves `knobBuf`, `depthBuf` and `amtBuf` empty, with `preparedBlock = 512` `[VERIFIED: PluginProcessor.h:436, 447]`. They are sized only in `prepareToPlay` `[VERIFIED: PluginProcessor.cpp:289-291]`.
- The chunk loop writes `knobBuf[i]` for i < 512 `[VERIFIED: PluginProcessor.cpp:448-453]`, which is a write through `nullptr`.

**Measured:** the child is **killed by signal 11** (SIGSEGV), on the shipped build and on the fixed build with the guard off.

**Fix shape (prototyped):**
```diff
+    std::atomic<bool> prepared { false };
 prepareToPlay(): ... ; prepared.store (true);          // LAST
 processBlock():  buffer.clear(); const bool ready = prepared.load();
-    if (numSamples > 0 && buffer.getNumChannels() > 0)
+    if (ready && numSamples > 0 && buffer.getNumChannels() > 0)
```
- The single exit stays unchanged: `blockEntries` first, `blockGeneration` last. REG-01 holds.
- **Do not clear `prepared` in `releaseResources`**, even though the prototype did. The buffers stay valid after release, and a host that processes after release would go silent for no reason.

**Gate G-UNPREPARED (dsp-check):**
- Run the child as a **self-exec** (`posix_spawn (argv[0], "--child-unprepared [--guard-off]")`), not as a fork. A fork after other processors have started their `ThreadPool` and `Timer` threads can deadlock the child on a lock inherited from the parent; the prototype's fork worked only because nothing was held.
- The child:
  - constructs the processor and calls `setPlayConfigDetails`
  - calls `processBlock` twice on a buffer prefilled with 1.0, with a note-on
  - checks that the output is exactly 0
  - then calls `prepareToPlay` and renders, checking peak > 0.01
- **Verdict:** the fixed child exits 0. The negative control (guard off) dies by a signal. Measured: exit 0 / SIGSEGV 11.

#### Note 8: `-Wundef` and `OSIW_TEST_HOOKS`

**Defect:** only `WavetableBank.h:48-50` defines the default:
```cpp
#ifndef OSIW_TEST_HOOKS
 #define OSIW_TEST_HOOKS 0
#endif
```
`[VERIFIED: WavetableBank.h:48-50]`

**Measured** (`undef/`, each header first in an otherwise empty TU, `-Wundef`, JUCE as `-isystem`):

| Header | Warnings | Sites |
|--------|----------|-------|
| `WtVoice.h` | 2 | `:104`, plus `PositionSmoother.h:51` via its include at `:109`, which precedes `WavetableBank.h` at `:110` |
| `PositionSmoother.h` | 1 | `:51` |
| `PositionLfo.h` | 5 | `:94, 166, 184, 211, 259` |
| the other 10 headers | 0 | — |

`[VERIFIED: research-probes/B/undef]`

The plugin's warning set has no `-Wundef` (the build flags in `build/` contain 30 `-W` flags, none of them `-Wundef`), so no shipped TU warns today. The undefined macro evaluates to 0, which matches the intended default, so this is hygiene and has no behaviour change.

**Fix:**
- Add a new `Source/TestHooks.h` holding the 3-line default, and add it to `target_sources`, which lists the headers (`CMakeLists.txt:27-48`).
- Include it first in `WavetableBank.h` (replacing lines 46-50), `WtVoice.h`, `PositionSmoother.h`, `PositionLfo.h`, `MipmapBuilder.h` and `PluginProcessor.h`.

**Gate R-UNDEF** (add to the R-DBG recipe):
```bash
for h in Source/*.h; do
  printf '#include "%s"\n' "$(basename "$h")" > "$T"
  clang++ -std=gnu++17 -fsyntax-only -Wundef -Werror=undef <JUCE module -D set> -isystem ~/JUCE/modules -ISource -x objective-c++ "$T" || echo "UNDEF $h"
done
```
- **Accept:** no `UNDEF` lines.
- **Negative control:** the pre-fix tree prints the 3 headers above.

---

### 4. Stage 3 W2 (Open Question 4): the 96 MB drop freeze. Measured

#### The path today

```
page: blob.size > DROP_MAX_BYTES (96 MiB) -> tooLarge          index.html:1144, 2412
page: arrayBufferToBase64 (whole file) -> importBytesFn(name, b64)   index.html:2418-2419
WKWebView: message body NSString -> juce::var -> JSON::fromString    juce_WebBrowserComponent_mac.mm:481-495
editor native importDroppedAudio -> processor.importFromBase64       PluginEditor.cpp:267-277
importFromBase64: getNumBytesAsUTF8() cap, then convertFromBase64
  into an UNSIZED MemoryBlock (message thread)                        PluginProcessor.cpp:1109-1134
importFromMemory: 96 MiB byte cap, then the worker                     PluginProcessor.cpp:1004-1032
```

All of these are `[VERIFIED]` by reading the files this session.

- The JUCE decoder loops character by character and calls `binaryOutput.writeByte` per byte `[VERIFIED: juce_Base64.cpp:85-118]`.
- `MemoryOutputStream::prepareToWrite` grows by `min (needed/2, 1 MB) + 32` `[VERIFIED: juce_MemoryOutputStream.cpp:86-109]`.

#### Measured (`b64probe`, -O3, JUCE 8.0.15, random bytes, M4 Max)

| Payload | Base64 chars | UTF-8 length scan | Decode, unsized (shipped) | Decode, pre-sized `setSize` | `preallocate()` | `JSON::fromString` of the invoke message (bridge proxy) |
|---|---|---|---|---|---|---|
| 1 MiB | 1.40 M | 1.7 ms | 13.5 ms | 13.2 | 10.8 | 4.4 ms |
| 4 MiB | 5.59 M | 2.6 | 27.9 | 27.1 | 27.1 | 12.2 |
| 8 MiB | 11.2 M | 5.1 | 54.7 | 54.3 | 54.4 | 24.3 |
| **16 MiB** | 22.4 M | **10.6** | **110.0** | 111.1 | 109.9 | **49.3** |
| 32 MiB | 44.7 M | 21.1 | 230.1 | 221.1 | 219.5 | 100.0 |
| 64 MiB | 89.5 M | 42.2 | 440.1 | 439.4 | 436.8 | 198.1 |
| **96 MiB** | 134.2 M | **63.5** | **663.7** | 673.3 | 656.4 | **299.6** |

`[VERIFIED: research-probes/B/b64probe.out]`

- **Pre-sizing does nothing.** Growth is not quadratic on macOS: `realloc` extends large blocks in place. The decode is linear, at about 6.9 ms per MiB, and the per-byte virtual `writeByte` dominates.
- **Total measured C++ message-thread cost at 96 MiB is about 1.03 s.**
  - Still unmeasured on top of that: the NSString (UTF-16, about 256 MB) → juce::String conversion in `fromObject` and the JS-side base64 of 96 MiB.
  - The transient memory is about 600 MB: the NSString, the juce String, the JSON copy and the 96 MB block.

#### What file size actually fills the importer's cap

The importer reads at most `kMaxFrames * kFrame` = 524,288 samples: `jmin<int64> (r.lengthInSamples, (int64) kMaxFrames * kFrame)` `[VERIFIED: WavetableImporter.cpp:64-72]`. A longer file is truncated, not rejected.

From `sizeprobe` (files of exactly 524,288 frames at 44.1 kHz, then the shipped `decode`, `encodeFlac16` and `buildImportedBank`):

| Format | Ch | Bits | Bytes (MiB) | Frames | decode / FLAC encode / build (ms) |
|---|---|---|---|---|---|
| WAV | 1 | 16 | 1.000 | 256 | 3.3 / 5.1 / 12.3 |
| WAV | 2 | 16 | 2.000 | 256 | 3.1 / 4.3 / 9.8 |
| WAV | 1 / 2 | 24 | 1.500 / 3.000 | 256 | ≈ 2.8 / 4.4 / 9.5 |
| WAV | 1 / 2 | 32f | 2.000 / **4.000** | 256 | ≈ 2.6 / 4.1 / 9.6 |
| WAV | 6 | 24 | 9.000 | 256 | 4.4 / 4.2 / 10.7 |
| WAV | 8 | 32f | 16.000 + 104 B | 256 | 3.7 / 3.9 / 9.3 |
| AIFF | 2 | 16 / 24 | 2.000 / 3.000 | 256 | — |
| FLAC | 1 / 2 | 16 (tonal sweep) | 0.631 / 1.132 | 256 | ≈ 5 / 4 / 10 |
| FLAC | 2 | 24 (tonal) | 1.925 | 256 | — |
| FLAC | 2 | 16 / 24 (white noise, worst case) | 2.001 / 3.001 | 256 | — |

`[VERIFIED: research-probes/B/sizeprobe.out]`

Any mono or stereo source, at any bit depth, carries the full 256-frame bank in **≤ 4 MiB**. Everything past that is either a longer file (truncated anyway) or a multichannel file, where all channels are averaged.

#### Recommendation: lower the drop cap to 16 MiB. No pre-size, no worker decode.

- **Lower the cap:** set `WavetableImporter::kMaxMemoryBytes = 16 MiB` (currently `(std::size_t) 96 * 1024 * 1024` `[VERIFIED: WavetableImporter.h:89]`) and the page's `DROP_MAX_BYTES = 16 * 1024 * 1024` `[VERIFIED: index.html:1144]`.
  - The measured C++ cost at the cap is about **170 ms** (10.6 + 110 + 49), down from about 1.03 s. That is 6× less.
  - The JS encode, the bridge IPC and the transient memory all scale down linearly too.
  - 16 MiB is 4× the largest useful stereo file, and covers ≤ 7-channel 32f and ≤ 10-channel 24-bit files at full length.
  - (8 MiB would halve the stall again, to about 84 ms. It is the alternative if 170 ms still reads as a hitch.)
- **Pre-sizing:** measured no gain. Do not add it.
- **Worker decode:** it would remove only the decode share (110 ms at 16 MiB). The JSON parse and NSString conversion stay on the message thread. It would also break G-DROP[c]'s synchronous `false / unreadable` contract (`viz-check` `G-DROP`), which then needs async rewrites.
- **The Import button is unaffected.** `importFromFile` streams from disk on the worker and reads ≤ 524,288 samples, so it has no size cap and still handles long songs.

**UI and error mapping (hand to Part A):**
- The page refuses before base64, with `importError = 'tooLarge'` → `import.err.tooLarge` = "file too large to import" `[VERIFIED: i18n.js:469]`. That copy becomes misleading, because Import accepts the same file.
  - Change en, fr and zh-Hans to say "too large to drop — use Import", or similar.
  - Pin the width at the widest language (memory: language-width pins).
- The C++ `tooLarge` from `importFromBase64` and `importFromMemory` becomes defense in depth (it fires only if the two caps diverge).
- **Page mapping bug, for Part A:** `if (!ok) { importError = 'generic' }` `[VERIFIED: index.html:2421]` races the `importStatus` `tooLarge` event. A C++ refusal can display "generic" instead of the specific code.

**Gates:**
- G-DROP[b] already derives the cap from the constant (`viz-check/main.cpp:2018`), so it follows automatically, and its cap+1 string drops from 134 M to 22 M characters, so the test gets faster.
- import-check's "97 MB → tooLarge" (`import-check/main.cpp:1155-1159`) stays valid.
- **New: G-DROP-CAPSYNC (viz-check).** Parse `DROP_MAX_BYTES = A * 1024 * 1024` from `index.html` (the file is located via `__FILE__`, as G-IMPORT-ERR does) and require it to equal `kMaxMemoryBytes`.
  - Negative control: an in-memory copy of the page with 96 swapped in must fail.
  - Gate the whole class of copies: the C++ constant, the page constant, and the `WavetableImporter.h:89` comment "page DROP_MAX_BYTES".

**Confidence:** HIGH for the C++ costs. MEDIUM for the total user-visible stall, because the NSString conversion and the JS encode were not measured.

---

### 5. Stage 3 N4 and N7, and the fix-or-log verdicts

#### N4: clear the display state in releaseResources. **FIX**

**Defect:** `releaseResources` only hard-stops and clears the stack and `audioHeldBank` `[VERIFIED: PluginProcessor.cpp:340-345]`. The `disp*` reset lives only in `prepareToPlay` `[VERIFIED: PluginProcessor.cpp:329-335]`.

**Measured:** after a held note, then `releaseResources()`: sounding = 1, note 64, hz 329.6, amp 1.00. The editor's lamp freezes in the "sounding" state.

**Fix (prototyped):** extract `resetDisplayState (double fs)`, holding the 7 stores from 329-335, and call it from both functions. `releaseResources` passes the current `displayFs`.

**Gate G-VIZ-RELEASE (viz-check):**
- **Stimulus:** hold a note and render 0.2 s; check `isDisplaySounding()` is true (liveness).
- Then call `releaseResources()`.
- **Verdict:** sounding is false, note −1, hz 0, amp 0, and `buildCycleView` gives the silent payload (D-R: note −1, f0 0).
- **Negative control** (`setReleaseClearsDisplayForTesting (false)`): still sounding.
- **Measured:** fix 0 / −1 / 0.0 / 0.00; negative control 1 / 64 / 329.6 / 1.00.

**Confidence:** HIGH.

#### N7: strip C1 and bidi characters in sanitiseName. **FIX**

**Today:** `if (c >= 0x20 && c != 0x7f) clean += c;` `[VERIFIED: WavetableImporter.cpp:173-178]`.

The same function serves all three entry points:
- the file name: `importFromFile`, `PluginProcessor.cpp:982`
- the JS name: `importFromMemory` and `importFromBase64`, `:1006, 1115, 1129`
- the restored state: `:827, 852`

So one predicate fixes the whole class.

**Strip list:**

| Code points | Reason |
|---|---|
| U+0000–001F, U+007F | current behaviour (C0 controls, DEL) |
| **U+0080–U+009F** | C1 controls |
| **U+061C, U+200E, U+200F, U+202A–U+202E, U+2066–U+2069** | the complete set of 12 Unicode `Bidi_Control` characters (ALM, LRM, RLM, LRE, RLE, PDF, LRO, RLO, LRI, RLI, FSI, PDI). CONTEXT named 11 of them; U+061C ALM is the missing one. These are the "Trojan Source" class: an RLO can show `evil.wav` as `vaw.live`. `[CITED: unicode.org/reports/tr9 — Bidi_Control list]` |
| **U+2028, U+2029** | LINE and PARAGRAPH SEPARATOR: they break a one-line label |
| **U+200B, U+2060, U+FEFF** | ZWSP, WORD JOINER, BOM/ZWNBSP: invisible, so they make a name differ from how it looks |
| **U+D800–U+DFFF** | lone surrogates are not Unicode scalar values. A hand-edited state can carry `&#xD800;`, which the XML parser decodes, and `String += juce_wchar` would then write invalid UTF-8 into the next save `[ASSUMED: JUCE XmlDocument decodes numeric entities to the code point]` |
| **U+FFFE, U+FFFF**, and anything > U+10FFFF | U+FFFE/FFFF are excluded by the XML 1.0 `Char` production, and the name is persisted in the `IMPORTED_BANK` attribute. Values above U+10FFFF are outside Unicode. `[CITED: w3.org/TR/xml/#charsets]` |
| *Optional:* U+E0000–U+E007F | tag characters: invisible "ASCII smuggling", but legitimate in subdivision-flag emoji. Recommend strip. |
| **Keep:** U+200C ZWNJ, U+200D ZWJ | needed for emoji sequences and for Persian and Indic orthography |

```cpp
static bool isStripped (juce::juce_wchar c) noexcept
{
    return c < 0x20 || c == 0x7f || (c >= 0x80 && c <= 0x9f)
        || c == 0x061c || c == 0x200b || c == 0x200e || c == 0x200f
        || (c >= 0x202a && c <= 0x202e) || c == 0x2028 || c == 0x2029 || c == 0x2060
        || (c >= 0x2066 && c <= 0x2069) || c == 0xfeff
        || (c >= 0xd800 && c <= 0xdfff) || c == 0xfffe || c == 0xffff || c > 0x10ffff
        || (c >= 0xe0000 && c <= 0xe007f);
}
```

`juce_wchar` is `wchar_t`, which is a signed 32-bit type on macOS, so `c < 0x20` also catches negative values from malformed input. `[ASSUMED]`

**Gate G-SANITISE (viz-check, next to G-DROP):**
- A table in which every listed code point is embedded as `"a" + cp + "b.wav"`. Each must sanitise to `"ab.wav"`.
- `"a\u200Db.wav"` and the ZWNJ case are kept.
- **Per-class negative control:** a test-local copy of the legacy predicate (`c >= 0x20 && c != 0x7f`) must **keep** each stripped code point. This proves that every row exercises the new rule.
- **End-to-end, all three surfaces:**
  - `importFromBase64` with the name `"x\u202Egnp.wav"`: the status, the thumbnails and the wire all show `"xgnp.wav"`.
  - A state blob with `filename="a&#x202E;b.wav"` restores as `"ab.wav"`.

**Confidence:** HIGH for the code. MEDIUM for the XML-parser detail (tagged above).

#### Fix-or-log verdicts

| Item | Verdict | Evidence and rationale |
|---|---|---|
| **S3 N2:** torn display atomics | **LOG** | `updateDisplayFromLeadVoice` stores 8 separate relaxed atomics per block `[VERIFIED: PluginProcessor.cpp:577-595]`. `buildCycleView` loads them separately `[VERIFIED: :628-667]`. A 30 Hz read can mix two consecutive blocks for one tick: one 33 ms frame of the cycle view with the previous block's level or frame. The next tick corrects it, because every block rewrites every field, and the final block before an idle is complete. No audio effect. A seqlock would add an audio-thread write protocol (and a probabilistic, hard-to-negative-control race gate) to remove a one-frame cosmetic glitch. |
| **S3 N3:** `importedOwner` read under the lock | **LOG** (by design) | `getBankThumbnails` copies `importedOwner` and `cachedBlob.filename` in **one** `bankStateLock` scope `[VERIFIED: PluginProcessor.cpp:686-691]`. That is required by D-S and Amendment 18: "The `bankUpdate` filename comes from the cached blob read in the same `bankStateLock` scope as the imported-bank copy" `[VERIFIED: ARCHITECTURE.md:478]`. `getImportedBankSnapshot()` is the same lock and the same `shared_ptr` copy `[VERIFIED: :974-978]`, but calling it separately from the filename read would reopen the bank/name TOCTOU. |
| **S2 note 5:** Mono ignores sustain | **LOG** (v1.0.0; a candidate for `/improve`) | `renderMono` handles note-on/off, all-notes/sound-off and the wheel only, and forwards only the wheel to the Synthesiser `[VERIFIED: PluginProcessor.cpp:511-551]`. Poly honours CC64 through JUCE. Mono pedal semantics are a design choice, not a defect: "hold the sounding note" or "keep released keys in the legato stack". Fixing it is new behaviour that needs its own gate and a decision from Taylor. **Side note:** CC64 pressed in Mono never reaches the Synthesiser, so after a Mono→Poly switch with the pedal held, Poly reads the pedal as up until it is re-pressed. Log that too. |
| **S2 note 6:** no import cancel | **LOG** | Supersede already exists: each new import or restore bumps `importGen`, and the decode polls it per 8,192-sample chunk `[VERIFIED: PluginProcessor.cpp:989, 1039, 1048; WavetableImporter.cpp:79-85]`. The destructor also cancels `[VERIFIED: :222-228]`. **Measured:** a full 256-frame job costs **17–24 ms** in total (decode 2.5–6.8, FLAC encode 3.9–5.1, build 9.3–12.3 ms; `sizeprobe`). A cancel control would have nothing to cancel. Only a slow disk on the button path is longer, and a second import supersedes it. |
| **S2 note 7:** save/restore decode under `bankStateLock` | **LOG** (the premise does not match the code) | `restoreImportedBank` runs base64 → FLAC/gzip → `buildImportedBank` **before** it takes the lock `[VERIFIED: PluginProcessor.cpp:841-861]`. It locks only to publish and update the cache `[VERIFIED: :863-868]`. `writeImportedBank` holds the lock only for refcounted String/ValueTree copies `[VERIFIED: :871-891]`. **Measured** (256 frames): restore decode outside the lock = base64 3.9 ms + FLAC 1.4 ms + build 9.9 ms; freeing a 23.2 MB bank < 1 µs; 1,000 blob refcount copies 4 µs. The audio thread never takes `bankStateLock` `[VERIFIED: PluginProcessor.h:380-382; no ScopedLock in PluginProcessor.cpp:355-561]`. Exposure to the audio thread is zero; the lock is held for microseconds against the message thread. |
| **S2 note 1:** Poly↔Mono hard stop | **LOG** (by design) | See the wording below. |
| **S2 note 2:** Interp-Off frame-step ticks | **LOG** (by design) | See the wording below. |
| **S3 N6:** a lesson during Mono gets the 2 ms tail | **LOG** (by design) | `applyFactoryPreset` resets every parameter it does not list to its default, and `voice_mode` defaults to Poly `[VERIFIED: PluginProcessor.cpp:161, 1181-1201]`. So a lesson clicked in Mono is a Mono→Poly switch. |

(Stage 3 N5 and N8 belong to Part A.)

#### Draft CODE_REVIEW.md wording

> **Accepted by design**
>
> - **S2-N1: Poly↔Mono switch ends sounding notes.** Changing Voice Mode hard-stops every voice; each voice's last sample decays over a 2 ms raised-cosine tail (W1, Amendment 13), so the switch is click-free but held notes do not carry into the new mode. Accepted: the switch is user-initiated, and a carried note has no single correct owner (16 Poly voices → 1 Mono voice). Gate: G-SWITCH-TAIL 0.656× (negative control 20.0×).
> - **S2-N2: Interpolation Off steps the timbre once per cycle.** With Interpolation Off the frame index latches at each cycle wrap (ARCHITECTURE row 15), so modulated Position steps cycle by cycle; on the sine-phase built-ins the step lands at a zero crossing, on Formant and Imported frames it can tick. Accepted: the stepped sound is the lesson the switch teaches; Interpolation On removes it.
> - **S3-N6: A lesson clicked during Mono playback ends the note through the 2 ms tail.** Every lesson resets unlisted parameters to their defaults, including Voice Mode → Poly, so it is a Poly↔Mono switch (S2-N1). Accepted for the same reason.
>
> **Reviewed, not changed**
>
> - **S3-N2: Display atomics can tear for one tick.** The lead-voice snapshot is 8 independent relaxed atomics; the 30 Hz reader may combine two consecutive blocks for one 33 ms frame. The next tick corrects it; there is no audio effect. A seqlock is not worth an audio-thread protocol for a one-frame cosmetic glitch.
> - **S3-N3: `getBankThumbnails` reads `importedOwner` directly under `bankStateLock`.** Intended (Amendment 18, D-S): the bank and its filename must come from one lock scope; `getImportedBankSnapshot()` takes the same lock and returns the same `shared_ptr`.
> - **S2-N5: Mono ignores the sustain pedal.** Mono is true-legato last-note priority and handles note, all-notes and pitch-wheel messages only; Poly honours CC64 through the JUCE Synthesiser. Pedal semantics in Mono are a design choice deferred to a later version.
> - **S2-N6: Imports cannot be cancelled from the UI.** A newer import or a state restore supersedes an in-flight job, and the destructor cancels it. A full 256-frame import costs about 20 ms on the worker (measured), so there is nothing for a cancel to save.
> - **S2-N7: Save/restore and `bankStateLock`.** Restore decodes before taking the lock and locks only to publish (microseconds); the audio thread never takes this lock.
> - **W4 residual:** if a second crossfade trigger (bank, interp, band-limit or another level change) lands within 5 ms of a level-crossing pitch change, the unfinished part of the first fade continues at the newest pitch (the pre-v1.0.0 behaviour for that remainder). Measured not worse than before (magnifier −24.3 vs −24.5 dB; click ratio 1.000 vs 1.765).
> - **Note 3 adjacent:** UI keyboard notes are merged into the host-owned MIDI buffer by `MidiMessageCollector`; that buffer's capacity belongs to the host or wrapper.

---

### 6. Regression guard

#### Which drivers and gates each fix touches

| Fix | Driver(s) changed | Goldens that may move | Expected |
|---|---|---|---|
| W3 | dsp-check: + G-MONO-WHEEL | none | `--alloc-check` scenario output changes (Mono notes are now seeded), but that gate counts allocations only |
| W4 | dsp-check: + G-LEGATO-XF, + G-LEGATO-ALIAS (+ optional click guard) | **G-BLOCK detail `peak`** (its Mono arm crosses L4→L5); PASS states unchanged | QUAL-03 exactness and ratio tiers (import-check), G-MONO, G-RETRIG and G-RETRIG-VEL stay bit-identical (no pitch change) |
| Note 3 | dsp-check: + G-MIDI-FLOOD; `--alloc-check` + flood blocks + a guard-off arm | none | G-BLOCK and the block-size invariance gates are bit-identical (same events and order) |
| Note 4 | dsp-check: + G-UNPREPARED (self-exec child) | none | — |
| Note 8 | R-DBG: + R-UNDEF | none | 0 plugin warnings still required |
| W2 | viz-check: G-DROP[b] follows the cap; + G-DROP-CAPSYNC | G-DROP[b] detail char count (134,217,733 → 22,369,625) | an intended, documented diff |
| N4 | viz-check: + G-VIZ-RELEASE | none | — |
| N7 | viz-check: + G-SANITISE | none | G-DROP[d] `"x.wav"` unchanged |

**New test hooks** (in `OSIW_TEST_HOOKS` blocks only; `nm` on the shipped binary must still show 0 `ForTesting` symbols):
- `setMonoWheelSeedForTesting`
- `setXfadeKeepRateForTesting`
- `setMidiCapacityGuardForTesting`
- `setPreparedGuardForTesting`
- `setReleaseClearsDisplayForTesting`

**Warnings:** the `src-fix` prototype compiles with 0 warnings under the plugin's 30 `-W` flags, with and without hooks. Use `juce::exactlyEqual` for the `curOffset` test, to match the codebase style.

#### Re-running the Stage 3 binary null test (how Stage 3 did it)

The Stage 3 verify ran from scratchpad `d9bfa2c4…/scratchpad`:
- `git archive 850df9b9` of the **whole repo** into `base/`
- a configure with `-DSKIP_PLUGINS=<all others>` and a Release build of the VST3
- `null_render.py` (pedalboard, 48 kHz, 3.5 s, 6 cases), run once per binary, each in its own process (`run.sh`)
- the comparison against `s3.npz` / `s3b.npz` (determinism)

`[VERIFIED: /private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/d9bfa2c4-aba1-4f1f-a353-7d429b726ff5/scratchpad/{null_render.py,run.sh,base-cfg.log}]`

The configure regenerates the gitignored `webview-drop-streaming.js` (`base-cfg.log`: "Copied webview-drop-streaming.js to ui/public/modules/"). No `Source/`, CMake or test change has landed since `f4eea85a` (`git log f4eea85a..HEAD -- Source CMakeLists.txt tests` is empty), so `f4eea85a` equals the current plugin code.

**Recipe:**
```bash
S=$SCRATCH/null4; mkdir -p "$S/base"
git -C ~/Dev/VST-development archive f4eea85a | tar -x -C "$S/base"
bash -c 'cd "'"$S"'/base"; SKIP=$(for d in plugins/*/; do n=$(basename "$d"); [ -f "${d}CMakeLists.txt" ] && [ "$n" != O-simpleWavetable ] && printf "%s;" "$n"; done); \
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release "-DSKIP_PLUGINS=$SKIP"' > "$S/cfg.log" 2>&1
cmake --build "$S/base/build" --target O-simpleWavetable_VST3 > "$S/build.log" 2>&1     # run in the background
# then null_render.py (copied from the Stage 3 scratch) on the base artefact and on the installed Stage 4 -dev.vst3
```

**Expected verdict:**

| Case | Touched by | Expected |
|---|---|---|
| `default_chord`, `bank_saw_pos1_blOff`, `lfo_tri_interp`, `bits3_noInterp`, `steal17` | nothing | max abs diff **0.0**, per channel |
| `mono_legato` (60, 62, 64, 65, 67 every 0.15 s) | W4: 65→67 crosses L4→L5 at 48 kHz (`x = 16.73`); the other notes stay at L4 (`x ≤ 14.9`) | non-zero diff **only** at samples 28801–29039 (0.6 s + 1 … + 239); 0.0 elsewhere. Script: `idx = np.nonzero(a != b)[0]; assert idx.min() >= 28801 and idx.max() <= 29039` |
| new positive control `w3_poly_to_mono` (the `w3repro.py` arm A, chunked calls) | W3 | base +200 c, Stage 4 0 c |
| new positive control `legato_60_96_drive` (Mono, Drive pos 1) | W4 | diff confined to t+1 … t+239 |

- Also render the Stage 4 binary twice and require bit-identical output (determinism).
- The positive controls are what show that the null verdict could fail (memory `pattern_null_test_needs_seeded_out_of_tree_pair`).

---

### Planner inputs

| # | Item | Files (verified lines) | Fix size | New gate(s) | Negative control (measured) | Verdict threshold | Goldens / null cases | Conf. |
|---|---|---|---|---|---|---|---|---|
| 1 | S2 W3: Mono wheel seed | `PluginProcessor.{h,cpp}` 388-393, copy loop 460-466 | ~6 lines | dsp-check G-MONO-WHEEL (arms A, B, C) | seed off: +199.98 c / +100.00 c | fix \|c\| ≤ 1; neg A ≥ 150 c, B ≥ 75 c | none; null: positive control `w3_poly_to_mono` | HIGH |
| 2 | S2 W4: frozen cycle keeps its rate | `WtVoice.h` 401-409, 432-437, 686-729 + members | ~20 lines | G-LEGATO-XF (5 arms, real 5 ms); G-LEGATO-ALIAS (48 kHz, 2 s magnifier); optional click ratio | keep-rate off: resid 0.54–1.00; alias −24.3 / −26.0 dB | resid ≤ 1e-6 (meas. ≤ 7.1e-8); alias ≤ floor + 1 dB, neg ≥ floor + 40 dB; confinement: 0 diffs outside t…t+240 | G-BLOCK `peak` detail may move; null: `mono_legato` differs only at 28801–29039 | HIGH |
| 3 | S2 note 3: MIDI scratch growth | `PluginProcessor.cpp` 57-58, 302-305, 444-475 | ~25 lines (chunk loop) | G-MIDI-FLOOD (Poly + Mono); `--alloc-check` flood arm + guard-off arm | guard off: 1 / 2 allocs (shipped: 1 / 7) | 0 allocs; neg ≥ 1; pitch +2 st ± 1 c; tail < 1e-6 | none (bit-identical normal path) | HIGH |
| 4 | S2 note 4: pre-prepare guard | `PluginProcessor.{h,cpp}` 279-338, 355-377 | ~4 lines | G-UNPREPARED (posix_spawn self-exec child) | guard off: SIGSEGV 11 | child exit 0: exact 0 before prepare, > 0.01 after | none | HIGH |
| 5 | S2 note 8: `-Wundef` | new `TestHooks.h`; `WavetableBank.h` 46-50; `WtVoice.h` 104; `PositionSmoother.h` 51; `PositionLfo.h` 94/166/184/211/259; `CMakeLists.txt` 27-48 | new 5-line header + 6 includes | R-UNDEF (per-header `-Werror=undef`) | current tree: 3 headers fail | 0 failures | none | HIGH |
| 6 | S3 W2: drop cap | `WavetableImporter.h:89`; `index.html:1144`; i18n `import.err.tooLarge` (Part A) | 2 constants + copy | G-DROP-CAPSYNC; G-DROP[b] follows | page = 96 copy fails | page cap == C++ cap | G-DROP[b] char count (documented) | HIGH (C++), MED (UX total) |
| 7 | S3 N4: display reset | `PluginProcessor.cpp` 329-345 | helper + 2 calls | viz-check G-VIZ-RELEASE | hook off: sounding 1, note 64 | sounding 0, note −1, hz 0, amp 0 | none | HIGH |
| 8 | S3 N7: sanitiseName | `WavetableImporter.cpp` 167-185 | predicate | viz-check G-SANITISE (unit + 3 surfaces) | legacy predicate keeps every row | every listed cp stripped; ZWJ/ZWNJ kept | G-DROP[d] unchanged | HIGH |
| 9 | Log: S3 N2, N3; S2 notes 1, 2, 5, 6, 7; S3 N6; W4 fold residual; collector buffer | CODE_REVIEW.md | wording above | — | — | — | — | HIGH |
| 10 | Null test vs `f4eea85a` | scratch base + `null_render.py` + 2 positive controls | — | 5 bit-exact + `mono_legato` windowed + 2 positive controls | positive controls must differ | as in §6 | — | HIGH |

**Suggested order:**
1. note 8 (the header that every other hook touches)
2. note 4, then note 3 (both in the processor block path)
3. W3
4. W4
5. N4 and N7
6. the W2 constants, with the copy change in Part A

Then run R-DBG, R-RUN, R-GOLD (allowing only the named G-BLOCK and G-DROP[b] detail diffs), `--alloc-check`, the null test, auval and pluginval.

**Listening (QUAL-04 sign-off):** add a Mono legato octave jump, for example C4→C7 on Drive. It is the only audible behaviour change in Part B: a 5 ms two-pitch crossfade replaces the 5 ms alias burst.

### Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | JUCE `XmlDocument` decodes numeric entities such as `&#xD800;` to the raw code point | §5 N7 | Low: the strip stays harmless, and only the justification for one row weakens |
| A2 | `juce_wchar` is signed 32-bit on macOS | §5 N7 | Low: `c < 0x20` stays correct for valid input |
| A3 | The NSString→juce::String conversion and the JS base64 encode scale linearly and add to the measured 1.03 s | §4 | Medium: the stall at 16 MiB could be more than 170 ms; it still scales 6× down with the cap |
| A4 | The Bidi_Control list (12 characters) and the XML 1.0 `Char` exclusions are cited from memory of the specs (UAX #9, W3C XML 1.0), not fetched this session | §5 N7 | Low |

### Environment

| Dependency | Available | Version |
|---|---|---|
| JUCE | ✓ | 8.0.15 at `/Users/taylorbrook/JUCE` (`.github/juce-version.txt`) |
| clang | ✓ | Apple clang 17.0.0 |
| uv + pedalboard | ✓ | `~/.local/bin/uv`, Python 3.12 |
| Installed VST3 | ✓ | `~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3` (Stage 3, 2026-10-06 13:24) |

**Probe commands** (all in `research-probes/B`):
- `./build-juce.sh` and `./build-juce2.sh` (JUCE modules, background)
- `bash link.sh b64probe.cpp b64probe "obj/juce_core.o obj/juce_ct.o" && ./b64probe`
- `./sizeprobe`
- `./w4probe`
- `./fixprobe-orig; ./fixprobe-fix`
- `./fixprobe-{orig,fix} --fold`
- `./fixprobe-orig --dump orig.bin; ./fixprobe-fix --dump fix.bin; python3 cmp.py orig.bin fix.bin`
- `uv run --no-project --python 3.12 --with pedalboard --with numpy --with mido python w3repro.py <vst3>`

---

## §C — Stage 4 Research, Part C: QUAL-04 gates, PERF-02 harness, COMPAT-02 CI, release mechanics, regression sweep

**Researched:** 2026-10-06
**Scope:** CONTEXT Open Questions 6 and 7, COMPAT-02, release mechanics, regression-guard inventory. Part A covers the preset manager and UI fixes; Part B covers the DSP and import critic fixes. This part does not repeat their work.
**Confidence:** HIGH for the metric numbers and timing, which were measured this session on the installed Stage 3 VST3 and a scratch Release build. MEDIUM for the CI items: the workflow was read, but nothing was dispatched.

**Evidence rule:** `file:line` means the file was read this session. Numbers marked *measured* come from scratch probes were run in session scratch; the probe sources are preserved in `research-probes/C/`, and the important parts of their code are copied inline below.

### Summary

- **All three QUAL-04 contrasts were measured on the installed Stage 3 VST3** (pedalboard). Each matches the numpy model to within 0.1 dB, or within 1 % on the ratio. So each gate below has a real-binary number behind its threshold, not just a model figure.
- **(a) Stepped vs smooth: the cycle-difference energy ratio R = S_Off / S_On** is 53.9 on the installed binary and 54.3 in the model, against a threshold of ≥ 10.
  - **Trap:** if the analysis window contains the Saw LFO's reset, R collapses to **3.2**. Measured on the installed VST3, this makes the gate vacuous. The window has to exclude the LFO wrap and the amp-decay settling.
- **(b) Aliasing vs clean, on the §A9 Alias Demo (Drive 32, Band-limit Off):**
  - Off: in-band (≤ 8 kHz) aliases reach −19 … −41 dB on C7/C8 at every rate, and −37 / −39 dB at C6 44.1 / 48 kHz.
  - On: −107 … −123 dB.
  - Contrast: ≥ 69 dB on every gated cell.
  - **The old Stage 3 recipe (Sine→Saw 32) has no in-band aliasing at C6: −102 dB at 44.1 kHz, and none at all at 88.2/96 kHz.** That is the negative control. It is also the measured case for the §A9 Drive move.
- **(c) Bit depth:** SNR from 16 bits down to 3 is 95.1 → 16.5 dB, in steps of 5.70–6.27 dB, with a least-squares slope of **6.029 dB/bit**. The installed binary matches the model exactly.
- **PERF-02 is far inside budget:**
  - 16 voices with a worst-case patch, at 96 kHz: **0.75 % of one core** (median per block; p99 0.88 %). Scratch Release -O3 build, thread-CPU clock.
  - The installed LTO VST3 agrees: **0.72 %** through pedalboard.
  - With a frozen-cycle crossfade triggered **every block**, 96 kHz: 1.97 % at a 512-sample block and 7.1 % at 64.
  - Only a pathological 16-sample block with a trigger in every block reaches 24 %.
  - Recommended threshold: < 25 % (CONTEXT), with a duty-cycle witness, so timing never decides a verdict on a contended machine.
- **pedalboard cannot measure per-block automation cost.** Each `parameters[k].raw_value = v` costs about **1.5 ms**: it read 13.5 % "CPU" at 44.1 kHz with no DSP change at all. Crossfade storms have to be timed in the C++ harness.
- **COMPAT-02: the config is in place.**
  - CMake: `NEEDS_WEBVIEW2 TRUE` (`CMakeLists.txt:20`) and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (`:107`).
  - Editor: `withUserDataFolder` (`PluginEditor.cpp:301-309`).
  - Source scan: 0 MSVC C3493 / SafePointer init-capture / C3615 carriers.
  - The validate-only `workflow_dispatch` exists, needs no tag, and skips `create-release` (`build-and-release.yml:44-59, 701-706`).
  - **Caveats:**
    1. The **macOS job also runs** on a dispatch, including Developer-ID signing and notarization.
    2. Both jobs **upload the signed binaries as workflow artifacts**, which anyone signed in can download on a public repo.
    3. `origin/main` (local ref) has **0 O-simpleWavetable files**, with 22 unpushed commits.
    4. Pushing main also triggers `ui-static-gates.yml`.
- **Release mechanics:**
  - `VERSION "0.1.0"` → `"1.0.0"` at `CMakeLists.txt:14`. No code reads the version string.
  - The CHANGELOG does not exist yet. Create it with one `## [1.0.0] - YYYY-MM-DD` section.
  - CODE_REVIEW.md does not exist yet. Canonical path: `plugins/O-simpleWavetable/CODE_REVIEW.md`, using the suite frontmatter.
  - PLUGINS.md is `MM`: another session's staged blob carries version **downgrades** of O-AnalogSaturation and O-DigiDelay. Use the temp-index CAS commit, and leave their staged blob alone.
- **Regression sweep:**
  - 6 R-GOLD logs (bank, dsp, dsp-alloc, mod, import, state), plus viz-check, plus the 5 UI gates, plus host checks.
  - A null test against a **snapshot of the currently installed Stage 3 VST3**. Take it before the first Stage 4 install.
  - The R-GOLD diff must filter the new Stage 4 gate lines.

---

### 1. QUAL-04 measured contrast gates (Open Question 6)

#### 1.0 Where they live

**Recommended: add 3 gates to `tests/dsp-check/main.cpp`**, named `G-Q4-STEP`, `G-Q4-ALIAS` and `G-Q4-BITS`. dsp-check already has everything they need:
- `report()` (`tests/dsp-check/main.cpp:131`) prints `PASS|FAIL <gate> detail`
- the processor `Rig` (params set before prepare, events at absolute positions) (`:265`)
- `Q2Analyzer` (Kaiser β 38, FFT 2^17, ±14-bin harmonic mask) (`:467-530`)
- `q2Proc` (`:627-633`)
- the exact-period A4 @ 56 320 Hz trick from G-DSP01 (`:766-841`)

Wire them into `main()` after `gateFull()` (`:1475-1478`).

The gates must apply the lesson recipes through the processor's `applyFactoryPreset(id)` (`Source/PluginProcessor.cpp:1142-1203`). That way the gate tests the shipped recipe table that Part A unifies, and then overrides only the contrast parameter.
- Today's ids are `steppedSmooth`, `aliasDemo`, `driveSweep`, `vowelPad` and `ppg8bit` (`PluginProcessor.cpp:1154-1170`).
- Today's `aliasDemo` is `{ bank 0, position 1.0, bandlimit 0 }` (`:1157-1158`). That is the Stage 3 Sine→Saw recipe; §A9 wants Drive.
- **Dependency:** the G-Q4 gates use Part A's final recipe ids. Until that lands, the gate sets the §A9 values directly.

**R-GOLD consequence:** the new `PASS G-Q4-…` lines are absent from the baseline `base-dsp.log`. The R-GOLD diff has to exclude them, as shown in §5.

#### 1.1 G-Q4-STEP: stepped vs smooth (Interp Off vs On)

**Metric: pitch-synchronous cycle-difference energy.**

- Render at **fs = 56 320 Hz, A4 (MIDI 69), vel 100**. Then `inc = 440/56320 = 2^-7` exactly, so the period is P = 128 samples exactly, as G-DSP01 uses (`dsp-check:766`). Interp Off latches a new frame only at the phase wrap (`Source/WtVoice.h:419-425`), so frame steps land exactly on cycle boundaries.
- Recipe: §A9 Stepped Scan = Sine→Saw, Pos 0.5, LFO **Saw** 0.25 Hz free, depth 1.0, Interp Off. Smooth Scan is the same with Interp On.
  - With depth 1, `raw = knob + depth·0.5·lfo` (`WtVoice.h:365`) gives raw = LFO phase, so the sweep covers 0 → 1 over 4 s.
  - The LFO phase is reset to 0 in `prepare()` (`Source/PositionLfo.h:87-97`), so a note at sample 0 puts the saw reset at t = 4.000 s.
- Let `c_k` be output samples `[kP, (k+1)P)`. Then `D_k = Σ_n (c_{k+1}[n] − c_k[n])²` and `S = Σ_{k∈W} D_k`.
- **Window W = cycles with 1.0 s ≤ t_k ≤ 3.8 s:**
  - It starts after the default amp env (A 5 ms, D 0.3 s, S 0.8) has settled. The decay alone puts per-cycle energy into D_k of the same order as the On-mode morph: in the model it adds 52 spurious "steps" when the window starts at 0.2 s.
  - It ends before the LFO wrap.
  - If the recipe's LFO phase is not known (for example, a host-driven render), exclude every cycle within ±50 ms of a raw-position jump of more than 0.5.
- `R = S_Off / S_On`.

| Quantity | Model (`q4probe_c.py`) | Installed VST3 (`q4_pb_a.py`) |
|---|---|---|
| R, window [1.0, 3.8] s (LFO aligned to the note) | **54.3** | n/a (pedalboard's settle call shifts the LFO) |
| R, window [1.0, 3.4] s (reset at 3.5 s excluded) | — | **53.9** |
| R, window **including** the saw reset | **5.0** | **3.2** |
| Off frame steps counted in W | 21 | 18 (shorter window) |
| R(Off, Off), negative control | 1.000 | — |
| R at A3 (256-sample period), model | 22.5 (R scales with cycles per frame step) | — |

- **Threshold:** `R ≥ 10`. The 5.4× margin is deterministic, because the render is bit-stable (G-BLOCK). R tracks "cycles per frame step" (≈ 1760/31 at A4 over 4 s, see `q4probe.py`), so do not change the note, rate or LFO rate without re-deriving it.
- **Liveness, the anti-vacuity terms** (memory `pattern_gate_stimulus_below_threshold_is_vacuous`):
  - Off: the number of k ∈ W with `D_k > 1e-3·max D` equals the number of `latchFrame(raw)` changes inside W (expected 21 for [1.0, 3.8] s), ±1.
  - On: `S_On > 0`, and at least 90 % of the cycles in W have `D_k > 0`.
  - Both renders: RMS > 0.05.
- **Negative controls:**
  1. `R(Off, Off) = 1.0` must FAIL `R ≥ 10`.
  2. LFO depth 0: both renders are static, so the Off step count is 0 and the liveness term must FAIL. Run this as a separate NC line.
  3. Record-only: R with the window stretched across the LFO reset. This documents the pitfall in the log.
- Confidence: HIGH (model and installed binary agree to 1 %).

Reference implementation (model; the C++ is the same arithmetic on the `Rig::run` output):
```python
C = y[:(len(y)//P)*P].reshape(-1, P).astype(np.float64)
D = np.sum(np.diff(C, axis=0)**2, axis=1); t = (np.arange(len(D))+1)*P/fs
m = (t > 1.0) & (t < 3.8);  R = D_off[m].sum() / D_on[m].sum()
```

#### 1.2 G-Q4-ALIAS: aliasing vs clean (Band-limit Off vs On, Alias Demo)

**Metric:** the existing Q2Analyzer measure (`dsp-check:467-530`), with one addition: an upper band edge for the inharmonic search.
- `A_full` = max inharmonic bin / max harmonic bin, full band. This is today's `inhDb`.
- `A_8k` = the same, with inharmonic bins restricted to `b·fs/N ≤ 8000 Hz`.

Why restrict the band: Drive-32 Off's worst full-band alias sits at 15–24 kHz (fold-back just under Nyquist). At 88.2/96 kHz it is above 40 kHz, which nobody hears.

- Stimulus: the §A9 Alias Demo (Drive, Pos 1.0, Band-limit Off; LESSON_OCTAVE 6 = keys C6..C8, `Source/ui/public/index.html:1155` and `:2500` with `base = (6+1)·12 = 84`). Render one note per cell: MIDI 84 / 96 / 108 × 44.1 / 48 / 88.2 / 96 kHz × Band-limit Off / On. Use `q2Proc`-style renders: skip 50 ms, then 2^17 samples.

Measured (installed Stage 3 VST3 with the §A9 params set directly; `q4_pb.py`). Each cell is full band | ≤ 8 kHz, dB re the strongest harmonic:

| fs | C6 Off | C6 On | C7 Off | C7 On | C8 Off | C8 On |
|---|---|---|---|---|---|---|
| 44.1k | −30.1 \| **−36.9** | −110.0 \| −110.2 | −21.4 \| **−27.5** | −115.8 \| −115.8 | −17.0 \| **−19.3** | −122.9 \| −122.9 |
| 48k | −29.9 \| **−39.2** | −109.9 \| −110.4 | −23.1 \| **−28.8** | −115.9 \| −115.9 | −17.0 \| **−21.5** | −122.8 \| −122.8 |
| 88.2k | −41.4 \| −60.0 | −107.3 \| −107.3 | −30.1 \| **−39.2** | −110.0 \| −110.2 | −21.4 \| **−28.9** | −115.8 \| −118.5 |
| 96k | −43.7 \| −64.4 | −107.3 \| −107.3 | −29.9 \| **−41.3** | −109.9 \| −110.4 | −23.1 \| **−30.0** | −115.9 \| −115.9 |
| *NC: Stage 3 recipe Sine→Saw 32, 44.1k* | −26.7 \| **−101.9** | −108.1 | −20.8 \| −25.1 | −114.4 | −15.4 \| −18.9 | −120.2 |
| *NC: Sine→Saw 32, 96k* | −102.1 \| −102.1 (no alias) | −102.1 | −27.1 \| −102.0 | −108.3 | −21.7 \| −27.0 | −114.3 |

Model (`q4alias.py`): every cell is identical to ±0.1 dB.

**Gated cells:** C7 and C8 at all 4 rates, and C6 at 44.1 and 48 kHz. These are the 10 bold cells. C6 at 88.2/96 kHz is too weak to call "clearly audible" (−60 / −64 dB in band), so it is record-only. That is an honest teaching limit to note in the tooltip or the CHANGELOG: at high sample rates, play the top octave.

**Verdict terms:**
1. **Audibility (vacuity guard):** `A_8k(Off) ≥ −50 dB` on every gated cell. The worst is −41.3 (C7 @ 96k), so the margin is 8.7 dB.
2. **Contrast (ratio verdict):** `A_8k(Off) − A_8k(On) ≥ 40 dB` on every gated cell. The minimum is 69.1 dB (C7 @ 96k).
3. **Clean:** `A_full(On) ≤ −90 dB` on all 12 cells (worst −107.3). This is weaker than QUAL-02's −100 at C8, on purpose: it is a contrast gate, not a re-run of QUAL-02.
4. **Liveness:** the max harmonic bin is > −40 dBFS in every render.

**Negative controls:**
- NC1, recipe discrimination: run the same arm on the Stage 3 recipe (Sine→Saw, Pos 1) at C6 44.1 kHz. Term 1 must FAIL (−101.9 < −50). This proves the gate tells a weak demo from the §A9 one; it is the measured reason the Drive move matters.
- NC2: On vs On has a contrast of 0 dB, so term 2 must FAIL.

**Interp:** the recipe leaves Interp at its default (On). Position 1.0 → `fp = 31`, `f0 = f1 = 31` (`Source/WtRead.h:112-119`), so this is exactly frame 32. Interp Off gives the same frame.

Confidence: HIGH.

#### 1.3 G-Q4-BITS: SNR per bit setting, monotonic

**Metric:**
- `SNR(b) = 10·log10( Σ y_Full² / Σ (y_b − y_Full)² )` over t ∈ [0.25 s, 1.0 s].
- Two `Rig`s, identical except `bit_depth` (choice index i = 1..14 → bits = 17 − i, per `Source/BitQuantizer.h:50-58`), each sample-aligned to a Full reference render.
- The quantizer runs **before** the amp env and the output gain (`WtVoice.h:399`, `:416`). So `y_b − y_Full = env·vel·gain·(q(s) − s)`, and the ratio is independent of the envelope, velocity and `output_level`.
- Determinism: the S&H seed is a constant (`PositionLfo.h:75`), and G-FULL proves render identity.
- Stimulus: Sine→Saw Pos 1.0 (frame 32), A4, 48 kHz, amp sustain 1.0, no LFO. This gives regular steps.
- Optional record-only arm on the 8-bit PPG recipe (Sine→Square, Pos 0.6, Interp Off). Its steps are irregular at low bits (model: 7.27 dB for 5→4, then 2.06 dB for 4→3) but still monotonic.

Measured (installed VST3, `q4_pb.py`; the model `q4bits.py` is identical):

| bits | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| SNR dB | 95.1 | 88.9 | 83.2 | 76.9 | 70.8 | 65.0 | 59.0 | 52.8 | 46.6 | 40.9 | 34.8 | 29.0 | 22.7 | 16.5 |

The steps run 5.70–6.27 dB, and the LS slope is 6.029 dB/bit. Theory is 6.02·b + 10·log10(3·P_s): P_s = 0.248 gives 95.0 / 16.8.

**Verdict terms:**
1. Every SNR(b) is finite.
2. Each step `SNR(b) − SNR(b−1)` is in **[4.5, 7.5] dB** for b = 16..4 (measured 5.70–6.27).
3. The LS slope over 16..3 is in **6.02 ± 0.6 dB/bit**, a ratio of 0.9–1.1 (measured 6.029).
4. Audibility: `SNR(3) ≤ 25 dB` (16.5) and `SNR(16) ≥ 85 dB` (95.1).

**Negative controls (analyzer-side, cheap and deterministic):**
- (a) "Knob not reaching the voice": evaluate the verdict on 14 copies of the Full render. SNR = +inf, so term 1 must FAIL.
- (b) "Mapping reversed": evaluate it on the reversed SNR vector. Term 2 must FAIL.
- (c) Misalignment: a 1-sample offset between reference and subject puts SNR near 0 dB everywhere, so terms 2 and 4 must FAIL. This guards the alignment the metric depends on.

Confidence: HIGH.

#### 1.4 Human checkpoint (locked)

After the three gates pass, Taylor listens to the factory presets and signs off. This is a `checkpoint:human-verify` task, and it stays after the gates. The gates catch regressions; the ear judges how well the contrast teaches (CONTEXT:57-62).

---

### 2. PERF-02 harness (Open Question 7)

#### 2.1 Measured numbers (this session)

**A. Scratch Release harness** (`$SCRATCH/C/perf/`):
- Built with plain clang -O3 -DNDEBUG (memory `reference_scratch_processor_harness_recipe`), against `/Users/taylorbrook/JUCE` and the processor TUs **without** `OSIW_TEST_HOOKS`.
- Timer: `CLOCK_THREAD_CPUTIME_ID` per `processBlock`, 1 s warm-up unmeasured, then 4 s measured.
- Worst-case patch: 16 voices MIDI 84..99 at vel 100, Interp On, Band-limit On, 3-bit quantizer, LFO Saw 5 Hz depth 1, Env Amount +1, mod-env sustain 0.5, amp sustain 1.
- Machine: Apple M4 Max, load 3.0. Duty witness user/wall = **95 %** (quiet).

| Case | 44.1k | 48k | 88.2k | 96k |
|---|---|---|---|---|
| steady, bs 512, median / p99 | 0.36 / 0.41 % | 0.38 / 0.46 % | 0.69 / 0.86 % | **0.75 / 0.88 %** |
| steady, bs 64, median / p99 | 0.43 / 0.53 % | 0.38 / 0.51 % | 0.74 / 0.94 % | **0.80 / 1.02 %** |
| crossfade every block (band-limit toggled), bs 512 | 0.77 % | 0.85 % | 1.77 % | **1.97 %** (p99 2.14) |
| crossfade every block, bs 64 | 3.27 % | 3.56 % | 6.31 % | **7.06 %** (p99 8.15, max 9.05) |
| bank cycled every block, bs 512 / 64 | 0.72 / 3.28 % | 0.85 / 3.58 % | 1.78 / 6.52 % | 1.97 / 7.09 % |

At 96 kHz:
- Steady at bs 16 / 32 / 128 / 1024 / 4096: 1.08 / 0.78 / 0.77 / 0.73 / 0.74 %.
- Crossfade every block at bs 16 / 32 / 128 / 1024 / 4096: **24.4** / 12.5 / 4.19 / 1.28 / 0.88 %.
- Cost scaling: 1 voice 0.072 %, 16 voices 0.767 %, a ratio of 10.7.

**B. pedalboard on the installed `-dev` VST3** (Release + LTO, the shipped binary; `perf_pb.py`, `perf_pb2.py`):
- Steady, worst patch, bs 512, one ~4 s `process()` call, median of 5: 0.34 / 0.37 / 0.65 / **0.72 %** at 44.1 / 48 / 88.2 / 96 kHz. Duty 100 %.
- This agrees with harness A to within 0.03 percentage points, so the missing LTO in the console build is negligible.
- 16 voices really sound: RMS(16) / RMS(1) = 4.37.

**Why crossfades cost:** a trigger captures a 2048-sample frozen cycle per voice (the loop at `Source/WtVoice.h:714`). That is a fixed ~4 µs per voice per trigger. So the storm cost scales with **trigger rate**, not with block size. A trigger in every 16-sample block is not a realistic host pattern: automation is usually block-rate or slower, and mip-level changes carry D-K hysteresis.

**pedalboard trap:** each `plugin.parameters[k].raw_value = v` costs ≈ **1.5 ms**. Setting the same value every 512-sample block read 13.5 % at 44.1k and 27.5 % at 96k, against 0.35 / 0.76 % for chunked calls alone. pedalboard can therefore only cross-check the **steady** case. The storm deltas it shows (≈ +1.5 % at 96k) match the harness but sit on top of that overhead.

#### 2.2 Harness design (for the plan)

**New console target `perf-check`**:
- File: `tests/perf-check/main.cpp`.
- Add `ouaricon_add_processor_console(O-simpleWavetable … tests/perf-check/main.cpp perf-check)` inside the existing `if(OUARICON_BUILD_TESTS AND APPLE)` block (`CMakeLists.txt:112-143`).
- **Do not** add `OSIW_TEST_HOOKS`. The timed processor code is then identical to the plugin's. The hooks add branches in the voice loop (`WtVoice.h:388-397`).
- The helper compiles the plugin's own TUs into the console target (`scripts/param-dump/ParamDump.cmake:146-185`). It links `juce_recommended_config_flags` but **not** the LTO flags (`:243-254`); §2.1 B shows that difference is negligible.

**Build: a second out-of-repo tree in Release**, `$SCRATCH/build-oswt-rel`:
- Same SKIP recipe as R-DBG (`stages/3-gui/PLAN.md:142-150`), with `-DCMAKE_BUILD_TYPE=Release`.
- Never set `OUARICON_BUILD_TESTS` / `SKIP_PLUGINS` in the shared `build/` (its cache: `CMAKE_BUILD_TYPE=Release`, `OUARICON_BUILD_TESTS:BOOL=OFF` at `build/CMakeCache.txt:69,631`).
- Run it in the background: `add_dependencies` also builds the plugin target (`ParamDump.cmake:197`).

**Procedure:**
- Rates {44.1, 48, 88.2, 96} kHz × block sizes {64, 512} × cases {steady worst patch, crossfade once per block}.
- Each cell: a fresh processor, params set before `prepareToPlay`, 16 note-ons in block 0, 1 s warm-up unmeasured, then 4 s measured.
- Per block: `CLOCK_THREAD_CPUTIME_ID` delta × fs / bs, which is the fraction of that block's real-time budget on one core.
- Report median, p99 and max. Repeat each cell 3× and take the **best-of-3 median**, which cancels transient load.

**Gates:**
- `G-PERF02-LIVE` (deterministic): output finite; RMS(16 voices) / RMS(1 voice) ≥ 3 (measured 4.37).
- `G-PERF02-SCALE` (cost sanity, proves the timer is measuring voice work): cost(16) / cost(1) ≥ 4 (measured 10.7).
- `G-PERF02-STEADY`: median ≤ 25 % **and** p99 ≤ 25 % at every rate × {64, 512}. The worst measured is p99 1.02 %, a ≈ 25× margin.
- `G-PERF02-XFADE`: crossfade once per block at bs 512, median ≤ 25 % at every rate (worst 1.97 %). The bs 64 storm (7.1 %) and the bs ≤ 32 cases are **printed, not gated**.
- **Duty witness:**
  - `getrusage` user time / wall time over the run must be ≥ 80 %. If it is lower, print `SKIPPED (machine contended, duty N %)` and exit **77**. Exit 77 is never recorded as a pass; re-run on a quiet machine.
  - Timing stays out of every correctness verdict (memories `pattern_wallclock_inside_a_stability_verdict`, `pattern_absolute_slack_threshold_is_not_runner_portable`).
  - The 25 % threshold has ≥ 12× margin over the realistic worst case, so a 3× load inflation of the kind seen in suite memory cannot flip it.
- Requirement wording for VERIFICATION: "CPU per block as a fraction of the real-time budget". Print a table like §2.1 A. Run it **after** Part 1's DSP fixes (W4 and note 3/4 touch the render path).

**pedalboard cross-check** (independent code, installed binary):
```
uv run --no-project --python 3.12 --with pedalboard --with numpy --with mido python perf_pb.py
```
- Steady worst patch only: one `process()` call of ≥ 4 s per rate, buffer_size 512, `reset=False` after a 0.5 s settle with notes on.
- Pass criterion: within 2× of `perf-check`'s steady median, and < 25 %.
- Set params with `plugin.parameters[name].raw_value` (memory `pattern_verify_installed_binary_with_pedalboard_via_uv`). Parameter keys and raw values are in §2.3.

#### 2.3 Installed-VST3 parameter keys (pedalboard, read this session)

| Key | Notes |
|---|---|
| `bank` | 6 steps; Drive = 0.8 |
| `position` | |
| `interpolation` | |
| `band_limiting` | |
| `bit_depth` | 15 steps; 3 bits = 1.0 |
| `lfo_rate_hz` | skew 0.3 over 0.01–20; 0.25 Hz = 0.2654 |
| `lfo_sync` | |
| `lfo_division` | |
| `lfo_shape` | Saw = 0.5 |
| `lfo_depth` | |
| `mod_env_attack_s` | |
| `mod_env_decay_s` | |
| `mod_env_sustain` | |
| `mod_env_release_s` | |
| `env_amount` | bipolar; 0.5 = 0 |
| `amp_attack_s` | |
| `amp_decay_s` | |
| `amp_sustain` | |
| `amp_release_s` | |
| `voice_mode` | |
| `output_level_db` | |
| `bypass` | |

Ranges: `Source/PluginProcessor.cpp:137-160`.

---

### 3. COMPAT-02: Windows VST3 via CI

#### 3.1 Confirmed in source (this session)

| Item | Evidence | Status |
|---|---|---|
| `NEEDS_WEBVIEW2 TRUE` | `CMakeLists.txt:20` | ✅ |
| `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (PUBLIC) | `CMakeLists.txt:101-108` | ✅ |
| `withUserDataFolder(tempDirectory/"OsimpleWavetable_WebView")`, status bar and error page disabled, `#if JUCE_WINDOWS` | `Source/PluginEditor.cpp:301-309` | ✅ |
| `webview-drop-streaming.js` copy | `ouaricon_add_module` (`CMakeLists.txt:55`) runs `configure_file(... COPYONLY)` at **configure time** on every platform (`modules/cmake/OuariconModules.cmake:104-117`), before `juce_add_binary_data` (`CMakeLists.txt:62`). The file is gitignored (`.gitignore:26`), so a fresh CI checkout regenerates it. Its module sources are tracked (`git ls-files modules/core/webview-drop-streaming` → 5 files). | ✅ |
| Hyphenated binary-data names | `BinaryData::webviewdropstreaming_js` and `ebgaramond_css` / `EBGaramondRegular_woff2` already use the stripped form (`PluginEditor.cpp:104-119`). **Part A:** the preset-manager module adds `preset-manager.js` → `BinaryData::presetmanager_js` (memory `critical_binary_data_strips_hyphens`). | ✅ (watch Part A) |
| MSVC C3493 (non-static local `constexpr` used in a lambda) | The 2 function-scope locals, `CycleView.cpp:108` and `VizPayload.h:190`, are not inside lambdas. Every other `constexpr` hit is at namespace scope. | ✅ 0 carriers |
| MSVC SafePointer init-capture in a nested lambda | `PluginEditor.cpp:248` uses the hoisted local `SafePointer<…> safeThis (this);` captured by value (`:252`). `grep "= juce::Component::SafePointer"` gives 0. | ✅ |
| MSVC C3615 (uninitialised local in a `constexpr` fn) | Only `PositionSmoother::activePoles` and `WavetableBank::kmax` (`:58`, `:62`), both single-return | ✅ |
| Other MSVC-hostile constructs | Comment-stripped scan of `Source/*.{cpp,h}`: 0 hits for `M_PI`, `__builtin`, designated initialisers, `and`/`or`/`not` tokens, `pthread`, `unistd` and the rest | ✅ |
| Non-ASCII in C++ | 0 (Stage 2 static checks; `juce::String(const char*)` trap) | ✅ |

**Part A/B watch item:** re-run the three greps after Parts A and B land. They add FileChooser and preset code, the usual home of SafePointer and `constexpr` traps:
```
grep -rn "= juce::Component::SafePointer" plugins/O-simpleWavetable/Source
grep -nE "^\s+constexpr " plugins/O-simpleWavetable/Source/*.cpp | grep -v static
```
Then check each constexpr function body for an uninitialised local (C3615).

#### 3.2 `build-and-release.yml` validate-only path (read, not run)

- **Trigger:** `workflow_dispatch` with three inputs (`:44-59`):
  - `plugin_name`: required string
  - `version`: default `'0.0.0-validate'`
  - `validate_only`: boolean, default `true`
- On a dispatch, `parse-tag` takes the plugin and version from the inputs. **No tag is needed** (`:114-118`).
- **Discovery:** there is no allow-list. The Windows configure skips every `plugins/*` folder except the target (`:573-584`). The target resolves with `scripts/resolve-target.sh`: run this session, `cmake-target` and `product-name` both return `O-simpleWavetable`. The artefact path is `build/plugins/O-simpleWavetable/O-simpleWavetable_artefacts/Release/VST3/*.vst3` (`:613`).
- **Windows job (`:528-700`):**
  - JUCE zip from the pin
  - override provenance check
  - WebView2 NuGet 1.0.1901.177 (`:563-566`)
  - configure with `-DOUARICON_RELEASE=ON` (unsuffixed product name)
  - build `O-simpleWavetable_VST3`
  - **pluginval v1.0.3 (Windows), `--strictness-level 10 --timeout-ms 600000 --validate`, editor tests ON** (no `--skip-gui-tests`), piped through tee under pipefail (`:603-621`)
  - the log is uploaded with `if: always()` (`:622-627`)
  - Inno Setup EXE, uploaded as the `windows-build` artifact
- **`create-release` is skipped:** `if: needs.parse-tag.outputs.validate_only != 'true'` (`:701-706`). Run 30848239205 (2026-08-03, validate dispatch) shows parse-tag ✓, build-macos ✓, build-windows ✓, create-release **skipped** (`gh run view`, this session).
- **Caveat 1: macOS also runs.**
  - `build-macos` has no `if:`. Its own comment says it "Runs on validate-only dispatches too" (`:137-140`). The `create-release` comment saying "build-macos is also skipped there" is stale.
  - On a dispatch that means a universal build, **Developer-ID codesign and Apple notarization**, and a signed, stapled PKG uploaded as `macos-build`.
  - The macOS configure does **not** skip siblings (`-DSKIP_PLUGINS="O-Orbit"` only). A broken sibling plugin on `main` could fail the macOS job, unrelated to O-simpleWavetable.
  - There is **no pluginval on macOS** in this workflow: pluginval appears only in the Windows job. Local pluginval covers macOS.
- **Caveat 2: artifacts are semi-public.** "People who are signed into GitHub and have read access to a repository can download workflow artifacts" [CITED: docs.github.com/en/actions/managing-workflow-runs/downloading-workflow-artifacts]. Origin is public (memory `project_public_release_readiness_blockers`), so a dispatch makes a signed and notarized O-simpleWavetable PKG and EXE downloadable by anyone signed in, under the `version` label. Retention follows the repo default; not checked this session [ASSUMED 90 days]. Taylor's go-ahead should cover this. Suggested `version` label: `1.0.0-validate`.
- **Caveat 3: push scope.**
  - The local `origin/main` ref is 22 commits behind `main` (`git rev-list --left-right --count origin/main...main` → `0 22`; the ref was not fetched this session). It holds **0** O-simpleWavetable files.
  - The 22 commits are all O-simpleWavetable, plus `18638e14 chore(claude)` research-agent memory.
  - Pushing `main` also fires `ui-static-gates.yml` (`on: push: branches: [main]`), which runs the 3 static UI gates for every plugin.
  - `ci-tests.yml` is dispatch-only (O-Octagon / O-Strata) and is not triggered.
  - At push time, re-check `git log origin/main..main` for other sessions' commits that would ride along.
- **Dispatch command, for after Taylor's explicit go-ahead only:**
  ```bash
  git fetch origin && git log --oneline origin/main..main      # review what will be published
  git push origin main
  gh workflow run build-and-release.yml --ref main -f plugin_name=O-simpleWavetable -f version=1.0.0-validate -f validate_only=true
  gh run list --workflow build-and-release.yml -L 1             # then: gh run watch <id>; gh run view <id> --log-failed
  ```
- **Pass:** build-windows ✓; the uploaded `pluginval-windows-log` ends in SUCCESS with 0 FAILED. build-macos ✓ is incidental but must not be red. create-release is skipped.
- **Windows assumptions:**
  - The windows-latest image has the Evergreen WebView2 Runtime, so pluginval's editor test can open the page [ASSUMED]. Precedent: the dispatch on 2026-08-03 was green.
  - A cold WebView2 open is covered by the 600 s timeout (`:614-617`).
- No hands-on Windows DAW test this stage (locked).

---

### 4. Release mechanics

| Item | Fact (this session) | Action |
|---|---|---|
| VERSION | `CMakeLists.txt:14` `VERSION "0.1.0"`. Use the `VERSION` keyword; `PLUGIN_VERSION` is ignored (memory). No code reads the version string: grep for `JucePlugin_VersionString` / `0.1.0` in Source and tests gives 0. The `IMPORTED_BANK` `version` attribute is its own `1` (`PluginProcessor.cpp:885`). | Change it to `"1.0.0"`. Logic re-reads the AU I/O on a version change; that is harmless (memory `critical_logic_caches_au_io_config_per_version`). |
| CHANGELOG.md | **Does not exist** at `plugins/O-simpleWavetable/CHANGELOG.md`. The CI extractor is `awk "/^## \[${VERSION}\]/{flag=1; next} /^## \[/{flag=0} flag"` (`build-and-release.yml:723-737`). Sibling format: `## [1.4.0] — 2026-09-25` (`plugins/O-simpleAdditive/CHANGELOG.md:6`); its first release is `## [1.0.0] — 2026-06-22` (`:837`), with `###` subsections per stage. | Create it with **exactly one** bracket heading, `## [1.0.0] - YYYY-MM-DD`. Put Stage 1–4 history in `###` subsections under it, never as `##` headings: the extractor runs from the heading to the next `## [` or EOF. Dry-run with `awk '/^## \[1.0.0\]/{flag=1; next} /^## \[/{flag=0} flag' plugins/O-simpleWavetable/CHANGELOG.md \| wc -l` (> 0, and the content is only 1.0.0), and census with `grep -cE "^## v?[0-9]\|^## Version"` = 0. |
| CODE_REVIEW.md | **Does not exist.** Canonical path: `plugins/O-simpleWavetable/CODE_REVIEW.md` (memory `project_code_review_artifact_path_canon`; `/improve-review` hard-requires that glob, `.claude/commands/improve-review.md:38-40`). Exemplars: frontmatter `phase`, `reviewed`, `depth`, `files_reviewed_list`, `findings: {critical, warning, info, total}`, `status` (`plugins/O-simpleAdditive/CODE_REVIEW.md:1-20`); a resolution-log table `\| Finding \| Status \| Where \|` with "✅ Resolved in **vX**" plus `open_findings: none` (`plugins/O-Prism/CODE_REVIEW.md:30-43`). | Write it in the suite format. IDs: CR-* / WR-* / IN-*, mapped from the Stage 2 W3/W4 + notes 1–8 and Stage 3 W1–W5 + N1–N13. Add a resolution log: fixed items "✅ Resolved in v1.0.0" with their gate; the by-design ones (Stage 3 N6, Stage 2 notes 1–2, plus whatever Part B logs) as "Accepted (by design), see rationale". Set `status: resolved` and `open_findings: none` only if nothing is left open; otherwise use `issues_found` and list them, because `/improve-review` offers every WR-*. |
| PLUGINS.md | Current row: `PLUGINS.md:63` `\| O-simpleWavetable \| 🚧 Stage 3 \| 0.1.0 \| Synth (Pedagogical Wavetable) \| 2026-10-06 \|`. Finished siblings use `📦 Installed` (`:61-68`). The file is **`MM`**. The foreign **staged** blob changes O-AnalogSaturation 1.8.0 → 1.7.0 and O-DigiDelay 1.9.1 → 1.7.0 (`git diff --cached -- PLUGINS.md`, read only). That is a stale-base snapshot from another session. | Update only the O-simpleWavetable row to `📦 Installed \| 1.0.0 \| … \| <date>`. Use the temp-index **CAS** commit: read-tree HEAD → patch the row in `HEAD:PLUGINS.md` → hash-object → write-tree → commit-tree → `git update-ref refs/heads/main <new> <old>` in a retry loop. **Never** commit from the main index, and do **not** `git reset` PLUGINS.md (the staged blob is theirs). Afterwards: `git show HEAD:PLUGINS.md \| grep "^\| O-simpleWavetable"`; resync only `plugins/O-simpleWavetable` in the main index (memories `index_git_shared_checkout`, `pattern_stale_base_temp_index_commit_reverts_shared_row`, `pattern_temp_index_commit_desyncs_main_index`). Path-scope the paths; zsh does not word-split, so use bash or arrays. |
| Tag / publish | **None** (locked; memory `feedback_never_tag_unless_publish`). For the later `/publish`: CI fires only on `'*-v*'` and parses `PLUGIN_NAME="${TAG%-v*}"` (`:38-40`, `:120-124`). The tag must be `O-simpleWavetable-v1.0.0`; the recent green runs use `O-DigiDelay-v1.9.1` (`gh run list`). CLAUDE.md's `vX.Y.Z-<Plugin>` wording disagrees with CI; CI wins (memory `pattern_release_tag_must_be_plugin_first_or_ci_never_fires`). | — |
| Build / install | `./scripts/build-and-install.sh O-simpleWavetable` builds **only** `_VST3` + `_AU` (`scripts/build-and-install.sh:280`). It does the dual-variant sweep, the AU cache clear and the install. Flags: `--dry-run --no-install --verbose --reconfigure` (`:53-60`). | Afterwards run `ninja -C build O-simpleWavetable_Standalone` before any Standalone visual check, because it stays stale (memory `pattern_build_install_skips_standalone_stale_ui`). Confirm with `strings` on both binaries for a Stage 4 marker, such as a preset-panel string id. |
| auval | `auval -a` SIGABRTs on this machine (NI AU + TCC, memory). Use `bash scripts/verify-au-link.sh O-simpleWavetable` or `auval -v aumu OSiW OuDv` (expect AU VALIDATION SUCCEEDED and the 2 known skew warnings). Negative control: `auval -v aumu ZZZZ OuDv` → "didn't find". | The first call after install is a cold registry rescan, measured 83 s to 17 min. Run it in the **background** and poll. |
| pluginval | pluginval **1.0.4** at `/Applications/pluginval.app/Contents/MacOS/pluginval` (`--version`, this session) | `… --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3`, then `…/Components/O-simpleWavetable-dev.component`. Editor tests stay ON (no `--skip-gui-tests`). Run in the background (600 s watchdog). |

---

### 5. Regression-guard harness inventory (one "regression sweep" task per Part)

All of these exist today unless marked NEW. Paths and accept rules come from `stages/3-gui/PLAN.md:142-198` (R-DBG / R-RUN / R-GOLD / R-UISTATIC / R-UIPW).

| # | Check | Command | Accept |
|---|---|---|---|
| 0 | **Baseline capture, Part 1 Task 1, before any edit** | Run R-DBG + R-RUN once on the unmodified tree into `$SCRATCH/base-<x>.log`. **Also snapshot the installed Stage 3 binary:** `cp -R ~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3 "$SCRATCH/s3-baseline.vst3"`. The installed binary is byte-identical to the f4eea85a Release artefacts (`3-gui/VERIFICATION.md:12`), and the plugin Source is unmodified since. The snapshot replaces a `git archive f4eea85a` rebuild. | baseline logs present |
| 1 | R-DBG Debug build | `bash -c 'SKIP=$(for d in plugins/*/; do n=$(basename "$d"); [ -f "${d}CMakeLists.txt" ] && [ "$n" != O-simpleWavetable ] && printf "%s;" "$n"; done); cmake -S . -B "$SCRATCH/build-oswt" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOUARICON_BUILD_TESTS=ON "-DSKIP_PLUGINS=$SKIP"'`, then `cmake --build "$SCRATCH/build-oswt" --target O-simpleWavetable-{bank,dsp,mod,import,state,viz}-check O-simpleWavetable-param-dump` (background) | exit 0; `grep -c 'plugins/O-simpleWavetable/.*warning:'` = 0 |
| 2 | bank-check | `find "$SCRATCH/build-oswt" -name O-simpleWavetable-bank-check -type f -perm +111`, then run it | last line `bank-check: ALL PASS` (`tests/bank-check/main.cpp:452`) |
| 3 | dsp-check (+ NEW G-Q4-*) | same pattern | `dsp-check: ALL PASS` (`tests/dsp-check/main.cpp:1496`) |
| 4 | `dsp-check --alloc-check` | `… --alloc-check` (`:1452`) | G-ALLOC 0 allocations, liveness counted 1, G-FINITE |
| 5 | mod-check | | `mod-check: ALL PASS` (`tests/mod-check/main.cpp:1674`) |
| 6 | import-check | | `import-check: ALL PASS` (`:1462`); the soak block count varies (allowed) |
| 7 | state-check | | 11 PASS (plus any Part A preset/state probes), 0 FAIL |
| 8 | viz-check | | `viz-check: ALL PASS` (`tests/viz-check/main.cpp:2250`). G-LESSON's golden **moves intentionally** with the §A9 `aliasDemo` change (Part A); document it. |
| 9 | JUCE assertions | `grep -c 'JUCE Assertion failure' "$SCRATCH"/*.log` | 0 in every log |
| 10 | **R-GOLD** | `diff <(grep -E '^(PASS\|FAIL)' "$SCRATCH/base-<x>.log" \| grep -vE "$S4NEW") <(grep -E '^(PASS\|FAIL)' "$SCRATCH/<x>.log" \| grep -vE "$S4NEW")` for x in bank, dsp, dsp-alloc, mod, import, state, where `S4NEW='G-Q4-\|G-PERF02-\|<Part B new gate names>'` | Only allowed: bank `buildMillis`, the import soak count, and **pre-declared** golden moves from Part B fixes (W3/W4), each documented. Anything else is a regression → stop. Suggest one prefix convention for all new Stage 4 gates so `S4NEW` is a single regex. |
| 11 | R-UISTATIC (also CI on push) | `node scripts/check-i18n.js --plugin O-simpleWavetable`; `node scripts/i18n-fr-lint.js --plugin O-simpleWavetable`; `node scripts/i18n-zh-lint.js --plugin O-simpleWavetable` | exit 0, with the assertion count (16/16 today; Part A adds strings); fr CLEAN; zh output **names** the plugin and its count (exit 0 alone is vacuous) |
| 12 | R-UIPW | `node scripts/check-ui-labels.js --plugin O-simpleWavetable`; `node scripts/boot-all-uis.js --plugin O-simpleWavetable --strict-tips` | exit 0; 0 DEAD / 0 late / 0 404; **exit 77 is never a pass**; output names this plugin's page |
| 13 | NEW perf-check (Part 2) | `$SCRATCH/build-oswt-rel` Release tree (§2.2), `O-simpleWavetable-perf-check` | the §2.2 gates; exit 77 = re-run quietly |
| 14 | Host: build/install | `./scripts/build-and-install.sh O-simpleWavetable`, then `ninja -C build O-simpleWavetable_Standalone` | installed bundles `diff -rq` equal to the artefacts; `nm -gU … \| grep -c ForTesting` = 0 |
| 15 | Host: auval / pluginval | §4 rows (background) | SUCCEEDED; pluginval VST3 + AU strictness 10 exit 0, 0 FAILED |
| 16 | Host: binary null test | pedalboard, each binary in **its own process** (`3-gui/VERIFICATION.md:21`). Run `$SCRATCH/s3-baseline.vst3` against the new installed VST3 on the 6 Stage 3 cases at 48 kHz, 3.5 s stereo: default chord; Sine→Saw Pos 1 BL Off C6–C7 run; LFO full depth; 3-bit Interp Off; Mono legato; 17-note steal (`3-gui/VERIFICATION.md:137-143`). Set params by `parameters[k].raw_value`. | max abs diff 0.0 on untouched cases. Cases a Stage 4 fix touches must differ **only** where intended: Mono legato under W4 differs only in the 5 ms crossfade after an upward jump that changes mip level; W3 only after a Poly→Mono with a moved wheel. Control: different cases differ (≈ 0.43), so the test can fail. |
| 17 | NEW QUAL-04 pedalboard cross-check (Part 2) | `q4_pb.py`-style probe on the installed binary (§1) | within ±1 dB / ±10 % of the dsp-check G-Q4 values |

---

### Planner inputs

| Input | Value / decision | Source |
|---|---|---|
| QUAL-04 gate home | dsp-check: `G-Q4-STEP`, `G-Q4-ALIAS`, `G-Q4-BITS`, using the recipe via `applyFactoryPreset` | §1.0; `tests/dsp-check/main.cpp:131,265,467,627,766` |
| G-Q4-STEP | A4 @ 56 320 Hz, Stepped/Smooth Scan; R = Σ‖Δcycle‖² Off/On over **t ∈ [1.0, 3.8] s**; **R ≥ 10** (measured 54.3 / 53.9); liveness: 21 ± 1 Off steps, On ≥ 90 % non-zero; NC: Off/Off = 1, depth 0 fails liveness | §1.1 |
| G-Q4-ALIAS | Drive 32, Pos 1, BL Off/On; MIDI 84/96/108 × 4 rates; gated cells C7, C8 (all rates) + C6 (44.1/48); **A_8k(Off) ≥ −50 dB** (worst −41.3), **contrast ≥ 40 dB** (min 69.1), A_full(On) ≤ −90; NC1: Sine→Saw C6 44.1k fails (−101.9); NC2: On/On fails | §1.2 |
| G-Q4-BITS | Sine→Saw frame 32, A4 48k; SNR vs Full; steps in [4.5, 7.5] dB for 16..4, slope 6.02 ± 0.6 (6.029), SNR(3) ≤ 25, SNR(16) ≥ 85; NCs: stuck, reversed, 1-sample offset | §1.3 |
| QUAL-04 human checkpoint | after the gates: Taylor listens to the factory presets (`checkpoint:human-verify`) | CONTEXT:57-62 |
| PERF-02 harness | NEW `tests/perf-check` console **without** `OSIW_TEST_HOOKS`, out-of-repo **Release** tree `$SCRATCH/build-oswt-rel` | §2.2 |
| PERF-02 cases | worst patch (16 high voices, Interp On, 3-bit, LFO, env); rates 44.1/48/88.2/96; bs 64 + 512; steady + crossfade every block; thread-CPU per block; 1 s warm-up; best-of-3 median, p99 | §2.2 |
| PERF-02 threshold | median and p99 ≤ **25 %** steady; ≤ 25 % crossfade at bs 512; storms at bs ≤ 64 printed only. Duty ≥ 80 % else exit 77. Expected: **0.75 % / 1.97 %** at 96 kHz | §2.1–2.2 |
| PERF-02 cross-check | pedalboard on the installed VST3, **steady only** (raw_value sets cost ~1.5 ms each); expected 0.72 % at 96 kHz | §2.1 B |
| COMPAT-02 config | already complete; re-grep the MSVC traps after Parts A and B | §3.1 |
| COMPAT-02 run | push main + `gh workflow run … -f plugin_name=O-simpleWavetable -f version=1.0.0-validate -f validate_only=true`, **after Taylor's explicit go-ahead**. Tell Taylor: macOS signs and notarizes too; artifacts are downloadable by signed-in users; 22 commits get pushed; `ui-static-gates` fires | §3.2 |
| COMPAT-02 pass | build-windows ✓, pluginval-windows-log SUCCESS / 0 FAILED, create-release skipped | §3.2 |
| VERSION | `CMakeLists.txt:14` → `"1.0.0"` | §4 |
| CHANGELOG | new file, single `## [1.0.0] - date`, stage history in `###`; awk dry-run | §4 |
| CODE_REVIEW | new at `plugins/O-simpleWavetable/CODE_REVIEW.md`, suite frontmatter plus a resolution log (Resolved v1.0.0 / Accepted by design) | §4 |
| PLUGINS.md | row only → `📦 Installed \| 1.0.0`; temp-index CAS commit; leave the foreign staged blob alone | §4 |
| Null-test baseline | snapshot the installed Stage 3 VST3 into `$SCRATCH` **before the first Stage 4 install** | §5 row 0 |
| Regression sweep | rows 1–12 + 14–16 in each Part; row 13 + 17 in Part 2; R-GOLD with the `S4NEW` filter | §5 |
| Tag / publish | none; for the later `/publish`, the tag is `O-simpleWavetable-v1.0.0` | §4 |

### Assumptions log

| # | Claim | Risk if wrong |
|---|---|---|
| A1 | The windows-latest image ships the WebView2 Runtime, so pluginval's editor test opens the page | Windows pluginval editor test fails or times out; the fallback is to inspect the log, not to skip GUI tests |
| A2 | Artifact retention is the repo default (≈ 90 days) | Signed binaries stay downloadable longer or shorter than expected |
| A3 | M4 Max timing is representative: a 4–6× slower classroom CPU stays < 5 % at 96 kHz steady | None for the gate (25 % threshold); only the margin claim changes |
| A4 | The local `origin/main` ref is current (not fetched this session) | More or fewer commits to push; re-check at push time |

### Sources

- In-repo, read this session:
  - `CMakeLists.txt`
  - `Source/{PluginEditor.cpp,PluginProcessor.cpp,WtVoice.h,WtRead.h,BitQuantizer.h,PositionLfo.h,CycleView.cpp,VizPayload.h,BankFactory.cpp,WavetableImporter.h}`
  - `tests/dsp-check/main.cpp` and the other drivers' main/report lines
  - `scripts/param-dump/ParamDump.cmake`, `modules/cmake/OuariconModules.cmake`, `scripts/resolve-target.sh`
  - `.github/workflows/{build-and-release,ci-tests,ui-static-gates}.yml`
  - `PLUGINS.md`; sibling `CHANGELOG.md` / `CODE_REVIEW.md` (O-simpleAdditive, O-Prism, O-simpleFM, O-simpleGrain, O-Gain)
  - stages 2 and 3 SUMMARY / VERIFICATION / PLAN, ARCHITECTURE §3–6, §A2, §A9, amendments 1–18
- Measured: scratch probes `q4probe*.py`, `q4alias.py`, `q4bits.py` (numpy model reusing `research-probes/wt_model.py`); `q4_pb.py`, `q4_pb_a.py`, `perf_pb.py`, `perf_pb2.py` (pedalboard on the installed VST3); `perf/perfprobe` (scratch clang Release harness).
- Web: [Downloading workflow artifacts — GitHub Docs](https://docs.github.com/en/actions/managing-workflow-runs/downloading-workflow-artifacts) (who can download artifacts).
