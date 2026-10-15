# Research: O-simple family retrofit

**Plugin:** O-SimpleReverb (becomes O-simpleReverb)
**Milestone:** family-retrofit
**Created:** 2026-10-15
**Phase:** Research
**Inputs:** `plugins/O-SimpleReverb/improvements/family-retrofit.md` (the brief), `CONTEXT.md` (the discuss answers). The template is O-simpleAmbisonics v1.0.1, with O-simpleWavetable as the secondary reference.

Paths are relative to `plugins/O-SimpleReverb/` unless they say otherwise. "Amb" means `plugins/O-simpleAmbisonics/`.

---

## Context Summary

- **Core requirement:** rename the plugin to O-simpleReverb with a case-only change. Rebuild the UI as an Ambisonics-style family page at 1180×780: four wells, sigpath stations, two lesson rows and the 48 utility presets in the browser. DSP, parameter IDs, ranges and state stay byte-compatible with v2.0.0.
- **Scope:** rename, editor and page rewrite, MeterFrame payload, PresetRecipes.h, family natives, test tree and docs. No DSP, parameter or factory-bank change.
- **Success:** a v2.0.0 session reopens with the same values, the render nulls against v2.0.0, the wells track the real DSP, and the family gates plus render-check (126 + 30 mutants) are green.

---

## Key decisions (TL;DR)

| # | Question | Decision |
|---|----------|----------|
| D1 | Preset-folder migration | **No migration needed.** The path is `~/Library/{pluginName}/Presets`. macOS APFS and Windows NTFS are case-insensitive, so the old folder is the new folder. No Linux build ships. Optional: an 8-line guarded rename for case-sensitive volumes (§2.4). |
| D2 | VST3 / AU identity | It derives from `PLUGIN_CODE OuSr` and the manufacturer only. The product name is not part of it (`JUCE_VST3_CAN_REPLACE_VST2=0`; §2.5). |
| D3 | ER tap source | A message-thread **model instance of `osr::EarlyReflections`** owned by the editor, read through its existing public `tapSamples()` / `tapGain()`. Native `getEarlyReflectionTaps(type, size)`. No tables in JS, no DSP edit. |
| D4 | Tone curve | **C++ computes it.** Native `getToneCurve(type, character, lowCutHz, lowCutOn)` returns 128 log bins of the post-engine tone stage: type EQ × Character × Low Cut. JS only draws. The in-loop absorption is not drawn (§4). |
| D5 | ModulationFx.h | This is the octave-up shimmer **inside the Plate tank only**, always on and with no parameter. It gets **no station of its own**: it becomes a sub-line of the Engine station when TYPE = Plate. |
| D6 | Payload | `MeterFrame { inDb, wetDb, outPeakL, outPeakR, spanMs, live }` goes through `TripleBuffer`, is hash-gated and emitted at 30 Hz. It replaces `outputPeak`. The brass RT60 slope is drawn from `getReverbModel()` baseT60 × DECAY. |
| D7 | Preset plumbing | **Keep `OuariconPresetManager` as the state/XML authority**: the state format is unchanged and the 48 presets stay on disk. Port only Ambisonics' *surface*: catalog, presetState, applyFactoryPreset. **Lessons never go to disk**, so render-check §9 "8 per type" holds. |
| D8 | State additions | `editorScale` as one additive root property, written and read the Ambisonics way. Nothing else. |

---

## 1. Ambisonics → Reverb file map

### 1.1 Source

| Amb file / item | Reverb action | Notes |
|---|---|---|
| `Source/dsp/TripleBuffer.h` (79 l) | **CREATE** `Source/TripleBuffer.h` (or `Source/ui/`), copied verbatim, namespace `osr` | Not a registered module (Amb `TripleBuffer.h:28-31`). Reverb would be the 4th copy, after GrainScatter, simpleGrain and Ambisonics. Keep it out of `Source/dsp/` so the "engines frozen" boundary stays a directory. |
| `Source/dsp/MeterFrame.h` (90 l) | **CREATE** `Source/MeterFrame.h` with Reverb fields (§6) | Trivially copyable, `static_assert`, JUCE-free. |
| `Source/MeterFramePayload.h` (252 l) | **PORT** with new fields | Keep `scaled` / `Writer` / `fnvInt` / `LiveTracker` / `sampleFrame` unchanged (Amb `:72-252`). Change only `QuantisedFrame`, `quantise`, `hash` and `format`. |
| `Source/PresetRecipes.h` (101 l) | **CREATE** `Source/PresetRecipes.h`, namespace `srpresets`, Init + 10 lessons (§8) | `Entry entries[N]` with N = 6 (the longest recipe). `findById` / `findByName` as Amb `:79-101`. |
| `PluginProcessor.h` editorScale (Amb `:136-145`) | **ADD** `std::atomic<float> editorScale{0}` | Read/write as Amb `PluginProcessor.cpp:464-471, 492-501`. Reverb writes on `parameters.state` before `presetManager.getStateAsXml()` (same as uiLanguage at Reverb `PluginProcessor.cpp:840-842`) and reads after `setStateFromXml` (`:875-878`). |
| Amb `meterFrames` + `outPeakAcc*` (Amb `PluginProcessor.cpp:330-334, 427-431`) | **ADD** `TripleBuffer<MeterFrame> meterFrames` plus accumulators. **REMOVE** `outputPeak` (Reverb `PluginProcessor.h:182`, `.cpp:636-646`) | render-check gate 4 must be ported (§9.3). |
| Amb preset core (`PluginProcessor.h:147-263`: Targets, applyNormalisedTargets, walk order, sanitise, save/delete, catalog) | **PORT a reduced core** (§7.3). Do **not** port Amb's custom state path (`currentPreset` root property and the user-JSON parser) | Reverb's state is the module's XML (`getStateAsXml` / `setStateFromXml`). Replacing it is a state-format change, which is out of scope. |
| `PluginEditor.h/.cpp` (585 l) | **REWRITE** Reverb editor on the Amb shape | Natives (§1.3), gesture tracking (Amb `PluginEditor.h:90-100`, `.cpp:461-475`), resize (Amb `.cpp:426-449, 563, 568-585`), `emitFrame` / `emitPresetState` (Amb `.cpp:481-532`), timer (Amb `.cpp:539-545`), `#if JUCE_WINDOWS` backend block (Amb `.cpp:380-397`), `withKeepPageLoadedWhenBrowserIsHidden` (Amb `:184`). |
| Relays | 8: `TYPE` combo; `CHARACTER`, `WET`, `DRY`, `DECAY`, `SIZE`, `LPFREQ` slider; **`LPON` → `WebToggleButtonRelay`** | `WebToggleButtonParameterAttachment` takes any `RangedAudioParameter&` (`~/JUCE/modules/juce_gui_extra/misc/juce_WebControlRelays.h:330`). LPON is a 0..1 float with step 1. Optionally keep the slider relay, but that is non-family. |
| `Source/ui/public/img/insects.png` | **COPY** from Amb | **DELETE** `img/paper.jpg` and `img/flora.png`, with their CMake and `getResource` lines (Reverb `CMakeLists.txt:66-67`, `PluginEditor.cpp:304-316`). |
| `Source/ui/public/modules/preset-manager.js` | **EMBED + SERVE**. It is configure-copied today (gitignored, `.gitignore:26`) but **not embedded and not served** by Reverb | Add it to `juce_add_binary_data` and add the `/modules/preset-manager.js` branch (Amb `PluginEditor.cpp:143-144`). |
| `Source/ui/public/js/shicons.js` | none | Ambisonics-only. |
| `Source/ui/public/index.html` (2211 l) | **REWRITE** from the signed-off mockup | Single file plus inline module, as Amb. |
| `Source/ui/public/js/i18n.js` (1008 l) | **REWRITE** keys, keeping the canon block byte-identical | fr `reviewed:false`, zh `reviewed:'mt'` (Amb `i18n.js:40-73`). |
| `Source/ModulationFx.h` | **KEEP** (comment banner rename only) | §5. |
| `Source/dsp/*.h` | **FROZEN**. Only the line 2 / line 23 banner comment changes | The null test proves no code changed. |

### 1.2 CMake (Reverb `CMakeLists.txt`)

| Line | Today | v3.0.0 |
|---|---|---|
| 7 | `juce_add_plugin(O-SimpleReverb` | `O-simpleReverb` |
| 8 | `VERSION 2.0.0` | `VERSION "3.0.0"` (quoted, Amb `:24`) |
| 13 | `PRODUCT_NAME "O-SimpleReverb${…}"` | `"O-simpleReverb${OUARICON_DEV_SUFFIX}"` |
| 9-16 | — | add `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT FALSE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`, `VST3_CATEGORIES "Fx" "Reverb"` (Amb `:25-34`) |
| 19-29, 54, 57, 76-79, 100, 107 | target name | `O-simpleReverb`. List the new headers in `target_sources` (Amb `:41-62`) |
| 60 | `O-SimpleReverb_UIResources` | `O-simpleReverb_UIResources`. Sources: index.html, i18n.js, juce/index.js, juce/check_native_interop.js, **modules/preset-manager.js**, **img/insects.png**, 4 × eb-garamond |
| 111 | `O-SimpleReverb-render-check` | `O-simpleReverb-render-check`. The `OSIMPLEREVERB_TEST_HOOKS` macro keeps its name |

The root `CMakeLists.txt:48-57` globs `plugins/*`, so it needs **no edit** (the brief's "root add_subdirectory" item is moot). `SKIP_PLUGINS` (`:45`) compares names case-sensitively: any local cache that lists `O-SimpleReverb` needs the new spelling.

### 1.3 Native-function census (bridge parity: every name the page or preset-manager.js calls)

| Name | Source | Reverb implementation |
|---|---|---|
| `getParameterDefaults` | Amb `:219-227` | Walk an 8-ID `kAll` (`TYPE, CHARACTER, WET, DRY, DECAY, SIZE, LPFREQ, LPON`). |
| `getUiLanguage` / `setUiLanguage` | Reverb `:135-153` (exists) | Keep. |
| `uiReady` | Amb `:258-267` | `emitFrame(true)` + `emitPresetState(true)`. |
| `applyFactoryPreset(id)` | Amb `:275-282` | Lessons from `PresetRecipes.h` (§7.3). |
| `getPresetCatalog` | Amb `:367-373` | `{ lessons:[{name,id}×11], factory:[name×48 in TYPE order], user:[…] }`. |
| `getPluginVersion` | Reverb `:159-161` (exists) | **Keep.** Not in Amb, but it fixed Stage-O item 47. Show it in the gear popover or the console banner. |
| `getReverbModel` | **new** | Once at init: `types[6]: {engine, baseT60, preDelayMs, earlySpanMs, earlyLevel, sizeLo, sizeHi}` from `typePresets` (`PluginProcessor.cpp:58-160`). |
| `getEarlyReflectionTaps(type, size)` | **new** | §3. |
| `getToneCurve(type, character, lowCutHz, lowCutOn)` | **new** | §4. |
| `savePreset`, `deletePreset`, `isFactoryPreset`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `loadPreset` | preset-manager.js (10 names) | Wrap the module (§7.3). |
| `savePresetWithDialog`, `loadPresetFromFile` | preset-manager.js | Register as refused (Amb `refusedDialog()`, `:107-114`) **or** keep Reverb's file choosers (`:95-131, 162-191`). **Open question Q2.** |

Total: 20 names. An unregistered name means a dead control while build, auval and pluginval still pass (memory `pattern_webview_native_fn_bridge_gap`).

### 1.4 Tests and docs

| Amb | Reverb |
|---|---|
| `tests/ui-probes/frame-probe.mjs` | PORT. Feed `tests/ui-fixtures/frames.json`; assert the decay-well and out-meter states. |
| `tests/ui-probes/header-fit-probe.mjs` | PORT (`NAME='O-simpleReverb'`). |
| `tests/ui-probes/lesson-row-budget.mjs` | PORT for **two rows** (6 + 5), each pinned at fr. |
| `tests/ui-probes/tooltip-clamp-probe.mjs` | PORT. |
| `tests/ui-stub/generic-overrides.json` | CREATE: the real ranges from §7.1. `natives`: getParameterDefaults, getPresetCatalog (11 + 48 + 2 user), getPresetList, getReverbModel, plus **static** getEarlyReflectionTaps (Room 50) and getToneCurve (Room flat). The stub's natives are values, not functions (`scripts/ui-stub/README.md:110-126`). |
| `tests/ui-fixtures/frames.json` | CREATE from a new `render-check --write-frames` mode. It must use `MeterFramePayload.h` (C++-producible states: memory `pattern_ui_fixture_state_must_be_producible_by_cpp`). |
| `tests/pedalboard/verify_*.py` | CREATE `verify_3_null.py` (v3 vs v2.0.0, §9.4) and `verify_3_state.py` (port Amb's decoder; v2.0.0 state → v3 values by name, plus the editorScale / uiLanguage round-trip). |
| `tests/i18n-states.json` | REWRITE for the new page states (LPON out, Plate/Spring taps, preset popover). |
| `tests/ui_tip_render_check.js` | **RETIRE.** It targets the 500×350 page; the tooltip-clamp probe supersedes it. |
| `tests/render-check/*` | KEEP. Rename strings at `main.cpp:2,8,11,411-412` and `measure.h:2`. Leave the fixture `state-v1.14.0.tsv:1` header alone (historical). Add gates (§9.3). |
| `.planning/` | Write BRIEF, REQUIREMENTS, ROADMAP and STATUS (with YAML frontmatter, as Amb `.planning/`). STATUS.md frontmatter is stale at `stage: 1`, `2026-01-13`. Regenerate `params.tsv` through param-dump (its header says `O-SimpleReverb`). |

### 1.5 Shared modules

| Module | Amb | Reverb today | Action |
|---|---|---|---|
| preset-manager (registry v1.0.9, `modules/registry.yaml:138-140`) | used *narrowly* (folders, save/delete, sentinel; JS embedded) | used *fully*: XML state, loads and factory bank. **JS copied but not embedded** | Keep full use; embed and serve the JS. `used_by` lists Reverb as `version: 1.5.7` (`registry.yaml:217-218`, stale) and **omits Ambisonics entirely**. Run `scripts/regen-registry-used-by.sh` after the rename. |
| eb-garamond | direct embed | direct embed (`CMakeLists.txt:70-73`, `PluginEditor.cpp:322-352`) | Keep (the registry `used_by` drift is known: memory). |
| triple buffer | verbatim copy | none | Copy (§1.1). |
| i18n canon | canon block | canon block | Keep it byte-identical. |

---

## 2. Rename

### 2.1 Live hits (must change)

| File:line | Change |
|---|---|
| `CMakeLists.txt` (15 hits) | §1.2 |
| `Source/PluginProcessor.h:161` | `getName()` → `"O-simpleReverb"`. Keep it a **literal**: `JucePlugin_Name` would carry `-dev` on dev builds, which v2 never did. |
| `Source/PluginProcessor.cpp:245` | `presetManager(parameters, "O-simpleReverb")` |
| `Source/PluginEditor.cpp:57` | WebView2 data folder `"O-simpleReverb_WebView"`, inside an `#if JUCE_WINDOWS` block as Amb `:380-397` |
| Banner comments: `Source/*.h/.cpp:2,23`, `Source/dsp/*.h:2,23`, `ModulationFx.h:2,23`, `ui/public/index.html:3`, `js/i18n.js:2,21,852` | Text only |
| `tests/render-check/main.cpp:2,8,11,411,412`, `measure.h:2` | Text; the target name is in the usage comment |
| `PLUGINS.md:36` | The row name. **PLUGINS.md is dirty in the shared checkout** (git status): path-scope the commit and do not stage other sessions' rows |
| `modules/registry.yaml:217` | Via `regen-registry-used-by.sh` |
| `README.md:52` | Change the display text to `O-simpleReverb`. **Keep the URL** `…/ouaricon-simple-reverb` (a website slug) |
| `.claude/scripts/canary-test.py:4,146-147` | `run_canary("O-simpleReverb", …)`. This is the system canary, so it breaks silently if missed |
| `.claude/critics/critic-architecture.md:83,201`, `critic-foundation.md:225` | Example text. Optional, low priority |
| `tests/ui_tip_render_check.js:2,24,34,92,110` | Retired (§1.4) |
| `NOTES.md` (7), `CHANGELOG.md` | NOTES: update. CHANGELOG: **append** a 3.0.0 entry and leave old entries alone |

### 2.2 Historical (leave alone)

`.planning/**` at the repo root (canary history and the quick-task ledgers: ~70 files), `plugins/O-SimpleReverb/.planning/improvements/reverb-engine-rewrite/**`, `CODE_REVIEW.md` (24, a dated review), and other plugins' comments that cite "O-SimpleReverb" as a past event: `O-Chorus/…/i18n.js:356`, `O-Tremolo/…/i18n.js:430,584,631,634`, `O-Comp/…/index.html:1921`, `O-GrainScatter/…/index.html:605`, `O-Bells`/`O-MicrotonalSampler`/`O-TextureForge` `tests/ui_tip_render_check.js`, `O-Detune/CHANGELOG.md`, `O-SpectralShaper/CODE_REVIEW.md`, `research/*.md`, `docs/codebase/STRUCTURE.md:62`, and `modules/persistence/preset-manager/module.yaml:101-102` (changelog provenance). Old git tags `O-SimpleReverb-v*` stay. The next release tag is `O-simpleReverb-v3.0.0` (the CI derives `PLUGIN_NAME="${TAG%-v*}"`, `build-and-release.yml:122`; a case mismatch breaks the build path on the Ubuntu parse job and on the macOS paths).

No gate script keys on the name. `check-i18n`, `boot-all-uis`, `i18n-fr-lint` and `i18n-zh-lint` discover plugins from `plugins/*`, and `check-ui-labels` takes `--plugin`. **No registration is needed. Change only the `--plugin` argument in the plans.**

### 2.3 Precedent

- v1.5.0 (`CHANGELOG.md:841-861`) claims "Existing presets will be migrated automatically". **That claim is false.** No migration code exists in git history (`git log -S "Ouaricon Simple Reverb"` finds only i18n and doc commits), and `~/Library/Ouaricon Simple Reverb/` is still on this machine. Do not cite it as a working pattern.
- There is no prior **case-only** rename in the repo. Other renames (O-Bass `9e7eb33b`, O-Lyrica `c6300f13`, …) changed spelling and the code.
- `git config core.ignorecase` = `true`. Use the two-step: `git mv plugins/O-SimpleReverb plugins/O-simpleReverb.tmp && git mv plugins/O-simpleReverb.tmp plugins/O-simpleReverb`. It moves the gitignored `Source/ui/public/modules/` and the untracked `family-retrofit/` dirs with it, because the directory itself is renamed. Then `git status` must show only `R` entries. Do it **in its own commit, first**, with path-scoped `git commit -- plugins/O-SimpleReverb plugins/O-simpleReverb`. Other sessions share the checkout and the index (CLAUDE.md "Commit discipline").
- `build/plugins/O-SimpleReverb/` resolves to the same directory on APFS. Re-run `cmake` (the target name changed) and expect `O-simpleReverb_artefacts/`.

### 2.4 User-preset folder: no migration needed

The module builds `userHomeDirectory/Library/{pluginName}/Presets/{Factory,User}` (`modules/persistence/preset-manager/cpp/OuariconPresetManager.h:274-291`). Every lookup is a `getChildFile` on that path. There is no string compare of the folder name, and a preset's `"plugin"` field is written (`:323`, `:689`) but never checked on load (`:330-388`).

| Platform | FS | Result |
|---|---|---|
| macOS (shipping) | APFS, case-insensitive by default. Verified: `ls -d ~/Library/O-simpleReverb` resolves to the existing `O-SimpleReverb` dir, which holds `User/a little plate.json` | Same folder. The on-disk name keeps its old case (cosmetic). |
| Windows (shipping) | NTFS, case-insensitive (per-directory case sensitivity is off by default) | `C:\Users\…\Library\O-simpleReverb` is the same folder. |
| Linux | case-sensitive | **Not shipped**: `build-and-release.yml` builds on macos-14 and windows-latest only; its Ubuntu jobs parse and publish. |
| macOS case-sensitive APFS volume | rare | The new folder would be empty and user presets orphaned. |

The factory bank rewrites itself anyway: the `.factory-version` sentinel ≠ `3.0.0` (`OuariconPresetManager.h:657-660`), and Reverb's stale-file sweep runs first (`PluginProcessor.cpp:974-982`).

**Recommendation:** no migration is required. If the user wants belt-and-braces, add this in the processor ctor before `initializeFactoryPresets()` (≤ 8 lines; a no-op on case-insensitive volumes because `newDir.isDirectory()` is already true):

```cpp
const auto lib = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library");
const auto oldDir = lib.getChildFile ("O-SimpleReverb"), newDir = lib.getChildFile ("O-simpleReverb");
if (! newDir.isDirectory() && oldDir.isDirectory())
    oldDir.moveFileTo (newDir);   // case-sensitive volumes only
```

The same reasoning covers the WebView2 temp folder, the Standalone `.settings` file and the VST3 host preset folders (`…/VST3 Presets/Ouaricon Audio/<name>`). All of them sit on case-insensitive volumes.

### 2.5 Plugin identity

- VST3: `getVST3InterfaceId()` returns `vst2PluginId(VSTUniqueID, JucePlugin_Name)` **only if `JUCE_VST3_CAN_REPLACE_VST2`**. Otherwise it returns `jucePluginId(JucePlugin_ManufacturerCode, JucePlugin_PluginCode, type)` (`~/JUCE/modules/juce_audio_plugin_client/VST3/juce_VST3ModuleInfo.h:54-62`, JUCE 8.0.15). Reverb sets `JUCE_VST3_CAN_REPLACE_VST2=0` (`CMakeLists.txt:84`), so the FUID = f(`OuAu`/`OuDv`, `OuSr`). The name is not involved.
- AU: the component is `aufx` / `OuSr` / manufacturer. The name is only the display string, so Logic shows "O-simpleReverb" after the cache sweep.
- PKG identifier: `io.oaudio.$(lowercase PLUGIN)` (`build-and-release.yml:382`) is **unchanged** (`io.oaudio.o-simplereverb`).
- `CFBundleIdentifier` (JUCE default `com.<company>.<target>`) changes case. Nothing keys on it for an effect. Note it only.
- Installed today: `O-SimpleReverb{,-dev}.{vst3,component}`. The release and dev bundles have **different manufacturer codes** (`OuAu` vs `OuDv`, root `CMakeLists.txt:25,30`). `build-and-install.sh` Phase 4 sweeps both, and on APFS `rm -rf O-simpleReverb.vst3` removes `O-SimpleReverb.vst3`. An end-user PKG install over a v2 install writes into the existing old-case bundle dir. That works but leaves the old-case folder name (cosmetic).

---

## 3. Early-reflection taps

**Structure** (`Source/dsp/EarlyReflections.h`): one static table of 8 L + 8 R tap *fractions of a span* (`:54-55`), **shared by all types**. A type only sets the span (`setType(spanMs)`, `:83`), and SIZE scales it (`setSize(scale)`, `:86-92`). Each tap = `max(2, round(frac × spanMs × scale × fs/1000))` whole samples (`:124-132`). Gains are `±t0/t_k` (alternating sign), normalised to unit energy per side (`:68-77`), and do not depend on span or type. The ER line is fed **after** the pre-delay (`PluginProcessor.cpp:573-587`), and its output is scaled by `earlyLevel` at the slot output.

Per type (`PluginProcessor.cpp:58-160`): `earlySpanMs` / `earlyLevel` / `preDelayMs`. Booth 6.9 / 0.6 / 3. Room 23 / 0.4 / 15. Hall 46 / 0.3 / 50. Ambient 57.5 / 0.25 / 35. **Plate and Spring have no taps** (span 0, level 0; `useEarly` false, `:536`). Plate has an 8 ms pre-delay. Spring has 0 pre-delay; its first output is its own echo at 33 / 37 / 41 ms × the size scale (`SpringEngine.h:97-100`, scale 0.75..1.333).

`sizeScale = sizeLo × (sizeHi/sizeLo)^(SIZE/100)` (`PluginProcessor.h:112-115`). For the FDN types that is 0.5..2.0, and **1.0 at SIZE 50**.

**Recommendation: a native `getEarlyReflectionTaps(type, sizePercent)` backed by a model instance.** The editor owns an `osr::EarlyReflections erModel` and prepares it once at `processor.getSampleRate()` (falling back to 48000) with the max span. Per call it runs `setType(span)`, `setSize(sizeScale(preset,size))` and `land()`, then reads `tapSamples(side,k)` and `tapGain(side,k)`. These are existing public accessors (`EarlyReflections.h:117-119`). The same class, the same rounding and the same rate mean no duplicated table or formula in JS and no edit to the frozen header. The call returns:

```json
{ "type":2, "engine":"fdn", "preDelayMs":50.0, "earlyLevel":0.3,
  "left":[{"ms":5.06,"g":0.7993},…8], "right":[…8],
  "springEchoesMs":[] }
```

For Plate it returns `left/right: []` with `preDelayMs: 8`. For Spring it returns `springEchoesMs: [33×s, 37×s, 41×s]` from `SpringEngine::kSpring` (public constexpr). The well draws Spring's arrivals as a distinct glyph labelled "the spring's own echoes".

- **Why not the payload:** the taps are a pure function of (type, size, fs) and change only on a gesture. Putting them in the 30 Hz frame would defeat the hash gate, because live peaks change every frame.
- **Why not reading `slots[].early`:** that object belongs to the audio thread, so a read from the message thread is a data race.
- **Latency:** the JS calls it on a TYPE or SIZE change, latest-wins with one call in flight. A message-thread round-trip is well under a frame.

The full tap tables are in **Appendix A**.

---

## 4. Tone curve

**The wet path after the engines** (all outside every feedback loop):

| Stage | Where | Filter | Parameter dependence |
|---|---|---|---|
| Type EQ | `renderSlot` → `slot.eq` (`PluginProcessor.cpp:611-613`), coefficients from `setTypeEq` (`:445-475`) | One RBJ biquad via `ArrayCoefficients`: Booth HPF 150 Hz Q 0.707. Room none. Hall high shelf 3 kHz −2 dB Q 0.5. Spring peak 800 Hz +4 dB Q 2.5. Plate high shelf 5 kHz +3 dB Q 0.707. Ambient high shelf 2.5 kHz −3 dB Q 0.5 | TYPE |
| CHARACTER Warm | `warmFilter` TPT SVF low-pass, R = 1/√2 (`:275-277, 488-498`) | For c < −0.5: fc = min(2000 + 18000·(c+100)/99, 0.45 fs). From −0.5 to 0 it glides to 0.49 fs. For c ≥ 0 it is parked at 0.49 fs (flat in band) | CHARACTER |
| CHARACTER Bright | `brightFilter` RBJ high shelf 4 kHz Q 0.707, gain = 6 dB × clamp(c/100, 0, 1) (`:504-506`) | | CHARACTER |
| LOW CUT | `lowCutFilter` TPT SVF high-pass, R = 1/√2, fc = LPFREQ (`:285-293, 778-800`) | crossfaded by LPON over 20 ms | LPFREQ, LPON |

**CHARACTER is not absorption.** It is a post-tank tilt: a low-pass on the dark side and a shelf on the bright side, applied to the whole wet signal equally. It does not change how fast the highs decay. Real absorption is **in-loop and fixed per type**:

- FDN: two-band decay, `gHi` from `t60 × hfRatio` with a one-pole shelf at `crossoverHz`. Booth 0.55 / 6 kHz, Room 0.50 / 5 kHz, Hall 0.45 / 4 kHz, Ambient 0.60 / 3.5 kHz (`FdnEngine.h:296-305`; hfRatio is "nominal … reaches it only well above the crossover", `:37-39`).
- Plate: a 10 kHz damping one-pole in the loop (`PlateEngine.h:99`).
- Spring: a 6th-order low-pass at about 4.4 kHz and a 100 Hz high-pass in the loop (`SpringEngine.h:23-31`).

The tank's own "frequency response" is a time-varying decay-vs-frequency surface, not a magnitude curve, so it is ill-defined as a single line.

**What is honest to draw:** the exact steady-state magnitude of the *tone stage* (type EQ × Warm × Bright × Low Cut), labelled "tone after the tank". Add one static annotation for Spring ("the springs pass little above ~4.5 kHz", shaded) and nothing for the FDN hfRatio (nominal, not calibrated). The CHARACTER tooltip and the lesson must say "darkens or brightens the whole tail", not "absorption" (see Q3).

**Recommendation: C++ computes it** with a native `getToneCurve(type, character, lowCutHz, lowCutOn)` that returns `{ hz:[128], db:[128] }`, log-spaced from 20 Hz to 20 kHz (or `0.49·fs` when that is lower).

- Extract the maps as pure static functions so the DSP and the curve share one source. These are a **pure refactor in PluginProcessor.cpp**, not in `Source/dsp/`:
  - `static float warmCutoffHz(float c, double fs)` from `:488-497`
  - `static float brightGainDb(float c)` from `:504`
  - `static std::array<float,6> typeEqCoefficients(int type, double fs)` from `:450-473`
  - The null test proves them bit-identical; keep the float expression order exactly.
- Magnitude: evaluate each biquad `|H(e^{jω})|` from its 6 coefficients in double. The TPT SVF with R = 1/√2 is a bilinear Butterworth prewarped at fc, so its magnitude equals the RBJ `makeLowPass` / `makeHighPass` (Q = 1/√2) at the same fc. Build those `ArrayCoefficients` and evaluate them the same way.
- Low Cut OUT: return the curve with Low Cut and a separate `lowCutDb[]`, so the page can dim it ("knob dims while OUT").
- **Drift reasoning:** a JS port would duplicate the piecewise cutoff map (including the −0.5..0 glide and the 0.45 / 0.49·fs clamps), the six-row EQ table and fs. The cost of the native route is one message-thread call per gesture.
- **Anti-drift gate:** in render-check, run real `StateVariableTPTFilter` / `IIR::Filter` objects on sines, configured through the same public statics, and assert they match `getToneCurve` within 0.1 dB at 12 frequencies × 5 CHARACTER values × 6 types.

---

## 5. ModulationFx.h

`Source/ModulationFx.h` holds `OctaveUpShifter` only: two 40 ms grains with sin²/cos² windows (`:38-98`). `FlutterDelay` was removed in v2.0.0 (`reverb-engine-rewrite/SUMMARY.md:230`).

- **Only caller:** `PlateEngine.h:68` (include), `:329` (`cross()`), `:399` (`shifterA`, `shifterB`). It sits inside the plate's loop as a convex mix whose share is `kShimmerPerLoopSecond = 0.121` per second of loop time (`PlateEngine.h:92`), set in `startSlot` (`PluginProcessor.cpp:363`).
- It is **always on for Plate**, never for the other types, and has no parameter. The processor no longer includes it (`PluginProcessor.h:37-41`).
- `processBlock` does not see it. It is part of the Plate engine.
- **No station.** The Engine station sub-line switches by type:
  - FDN: "16 lines · Hadamard". For Room, Hall and Ambient add "· slow modulation" (`modMs > 0`, `PluginProcessor.cpp:80,96,149`).
  - Plate: "Dattorro figure-8 · octave shimmer in the loop".
  - Spring: "3 dispersive springs".
- A Plate lesson caption can point at the shimmer, and render-check §7 already gates the bloom.

---

## 6. Live decay well payload

### 6.1 Fields

| Field | Source / tap point | Quantisation |
|---|---|---|
| `inDb` | max \|x\| of `dryBuffer` (pre-everything) per chunk (`processChunk`, after `:684`), peak-since-read | dB × 10, int, floor −1200 (−120 dB), ceiling +240 |
| `wetDb` | **mean-square** of `wetBuffer` after Low Cut, **before** the WET gain (`:800`), Σx² over channels and samples since the last read, divided by n·ch → 10·log10 | dB × 10, int, same clamp. Pre-WET-gain, so the tail is visible at any mix; the out-meter shows the post-mix level |
| `outPeakL`, `outPeakR` | `buffer` after all chunks (replaces `:636-646`); on a mono bus R = L | linear × 1000 (Amb rule, reuse its out-meter JS) |
| `spanMs` | samples accumulated since the last read × 1000 / fs | int ms. It lets the page place points in real audio time when timer ticks jitter |
| `live` | `LiveTracker` (Amb `MeterFramePayload.h:218-238`) | 0/1 |

Not in the frame:

- The **T60 slope**. The page computes `baseT60[type] × DECAY` from `getReverbModel()` and the relay values. It keeps responding while the host idles the audio thread, which a frame cannot do (memory `pattern_generation_reaper_freezes_when_host_idles_audio_thread`). Note this is the DSP's own law (`driveSlot`, `PluginProcessor.cpp:416`): a multiplication, not a table.
- **ER taps and the tone curve** (natives, §3/§4).

### 6.2 Calibration (is the brass slope honest?)

DECAY is a multiplier on the type's **mid-band (1 kHz) RT60**: Booth 0.40 s, Room 1.1, Hall 3.0, Spring 2.5, Plate 2.5, Ambient 7.0 (`PluginProcessor.cpp:62-147`, `driveSlot :416`). The engines turn it into loop gains from the current lengths, so it holds across SIZE (`FdnEngine.h:299-305`, `PlateEngine.h:353-364`). render-check §11/§12 gate "RT60 = base × DECAY within 10 % and holding across SIZE" at 44.1, 48 and 96 kHz. The slope is −60 dB per T60 seconds, starting after `preDelayMs`.

The broadband `wetDb` decays close to the mid-band slope on pink-ish material. HF dies faster on the FDN types, so a slightly steeper measured tail is expected and true. Add the render-check gate "frame-sampled wetDb slope within ±20 % of base × DECAY" (Success criterion 3).

### 6.3 RT safety and protocol

- Accumulators are plain audio-thread members: `float inPk`, `double wetSumSq`, `int64 wetN`, `float outPkL/R`. Reset them only when `!meterFrames.hasNewData()` (Amb `PluginProcessor.cpp:328-334`). Publish **once per processBlock** after the chunk loop.
- While no editor reads, the sums grow without bound. Store them in double and cap at about 1 s worth of samples: when `wetN > fs`, halve both sums. The editor drains one `read()` at construction (Amb `PluginEditor.cpp:451-455`).
- Everything is allocation-free and covered by the existing §12 "no allocation in processBlock" gate.
- **Hash gate:** a steady silent input gives a constant floor and so no emission. The page must **scroll by wall clock** (rAF) and hold the last value. On the live→idle edge it freezes the plot.
- Payload buffer: about 120 bytes. A 256-byte member is enough. Keep the 1024-byte habit if preferred.

---

## 7. Parameters, factory bank, browser

### 7.1 Parameters (`PluginProcessor.cpp:162-238`). Frozen.

| ID | Name | Type | Range | Interval | Skew | Default | Unit / notes |
|---|---|---|---|---|---|---|---|
| `TYPE` | Type | Choice | Booth, Room, Hall, Spring, Plate, Ambient | — | — | 1 (Room) | index = round(norm × 5) (render-check §1) |
| `CHARACTER` | Character | Float | −100 … 100 | 0.1 | 1 | 0 | % dark ↔ bright |
| `WET` | Wet | Float | 0 … 100 | 0.1 | 1 | 25 | % |
| `DRY` | Dry | Float | 0 … 100 | 0.1 | 1 | 100 | % |
| `DECAY` | Decay | Float | 0.5 … 2.0 | 0.01 | 0.6309 (1.0× at centre) | 1.0 | × the type's RT60 |
| `SIZE` | Size | Float | 0 … 100 | 0.1 | 1 | 50 | % (length scale, not time) |
| `LPFREQ` | Low Cut Freq | Float | 20 … 400 | 1 | 1 (linear) | 200 | Hz. High-pass; the ID is historical |
| `LPON` | Low Cut On | Float | 0 … 1 | 1 | 1 | 0 | off/on |

There is **no output-gain parameter**, so the memory rule "presets never set outputGain" is vacuous. Lessons are still level-gated (§9.3).

### 7.2 The 48 utility presets

- Defined in `initializeFactoryPresets()` (`PluginProcessor.cpp:881-985`) as `Def bank[48]` in engineering units, 8 per type in TYPE order, `convertTo0to1`.
- Written by the module to `~/Library/O-SimpleReverb/Presets/Factory/*.json` when the sentinel ≠ version, after a stale-file sweep.
- Loaded through `presetManager.loadPreset(name)`: factory first, then user, then `applyPresetJson`, which resets all parameters to default first (`OuariconPresetManager.h:349-364`).
- The ctor runs it on every construction; it is sentinel-gated.
- **Unchanged in v3.**

### 7.3 Lessons coexisting with the bank (Ambisonics surface, Reverb core)

| Concern | Recommendation |
|---|---|
| Apply | `bool applyFactoryPreset(id)` on the processor, message thread: (1) every parameter goes to its default with begin / `setValueNotifyingHost` / end, skipped within 1e-6 (Amb `applyNormalisedTargets`); (2) the recipe goes through each parameter's own `convertTo0to1`; (3) `presetManager.setCurrentPresetName(recipe.fileName)` (`OuariconPresetManager.h:168`); (4) the identity is `{id, factory:true}`, `revision++`, and the targets are snapshotted. |
| Disk | **Lessons are never written to Factory/.** render-check §9 counts factory files as 8 per type plus one Send each (`main.cpp:785-797, 1993`). Lesson files there would break it, and the bank is out of scope. |
| Catalog | `getPresetCatalog → { lessons:[{name,id}], factory:[48 names in bank (TYPE) order], user:[stems sorted case-insensitively] }`. The page groups factory by the type prefix (the existing names are "Room - Studio A", …). |
| Walk (◀/▶) | v2 used the module's alphabetical merged list (`OuariconPresetManager.h:525-588`): Ambient first, with user presets interleaved. **Recommend one order**, the Amb principle: bank order, then user (sorted). Implement `selectNext`/`selectPrevious` in the editor from a processor `getPresetWalkOrder()` instead of the module's `getNextPreset`. This is a UI change only, not a state change. |
| presetState | `{ name, id, factory, modified }` (Amb `PluginEditor.cpp:509-532`). `id` = lesson id or "". `factory` = lesson or `presetManager.isFactoryPreset(name)`. `modified` = any parameter more than 1e-4 from the **snapshot taken right after the load/apply** (simpler than Amb's JSON re-parse). A restored session has unknown targets, so it is unmodified (Amb rule). A restored lesson name is found by `findByName` and its targets are rebuilt from the table, which keeps the lesson lit. |
| Save / Delete | Use the module's `savePreset` / `deletePreset` (factory-guarded, `:393-399, 497-505`) behind Amb's `sanitisePresetName` and the factory-name refusal. Also refuse lesson file names. |
| Name persistence | No change: `currentPreset` is already an XML attribute of the module state (`:611, 643`). |

---

## 8. Lesson recipes (engineering units; Init = defaults; every lesson = full reset + recipe)

Defaults: TYPE Room, CHARACTER 0, WET 25, DRY 100, DECAY 1.0, SIZE 50, LPFREQ 200, LPON 0.

| Row | id | fileName | Recipe (only non-default keys) | Tail (s) | What the wells show |
|---|---|---|---|---|---|
| 2 | `init` | Init | — | 1.1 | Room baseline |
| 1 | `booth` | Lesson - Booth | TYPE 0, WET 30 | 0.40 | ER cluster inside 7 ms after a 3 ms pre-delay; steep slope |
| 1 | `room` | Lesson - Room | TYPE 1, WET 30 | 1.1 | ER spread to 23 ms after 15 ms |
| 1 | `hall` | Lesson - Hall | TYPE 2, SIZE 70, WET 30 | 3.0 | 50 ms pre-delay gap, then ER out to about 60 ms; long slope; −2 dB shelf |
| 1 | `spring` | Lesson - Spring | TYPE 3, WET 35 | 2.5 | No ER; 3 echo markers at 33/37/41 ms; +4 dB peak at 800 Hz; >4.5 kHz shaded |
| 1 | `plate` | Lesson - Plate | TYPE 4, CHARACTER 20, WET 30 | 2.5 | No ER, 8 ms pre-delay; +3 dB shelf at 5 kHz plus Bright; shimmer sub-line |
| 1 | `ambient` | Lesson - Ambient | TYPE 5, SIZE 75, WET 45, DRY 80 | 7.0 | Wide ER (out to about 85 ms); very shallow slope |
| 2 | `sizeDecay` | Lesson - Size vs Decay | TYPE 1, SIZE 0, DECAY 2.0, WET 35 | 2.2 | ER packed into 11.5 ms (small) while the slope is long. Caption: "drag SIZE: the echoes spread, the tail time holds" |
| 2 | `character` | Lesson - Character | TYPE 2, CHARACTER −80, WET 35 | 3.0 | Warm LP at about 5.6 kHz. Caption: "sweep to +80: a 4 kHz shelf lifts; the tail length does not change" |
| 2 | `lowCut` | Lesson - Low Cut | TYPE 2, DECAY 1.3, WET 40, LPON 1, LPFREQ 250 | 3.9 | HPF slope in the tone well. Caption: "toggle OUT to hear the mud return" |
| 2 | `insertSend` | Lesson - Insert vs Send | TYPE 4, WET 100, DRY 0, LPON 1, LPFREQ 150 | 2.5 | Equals `Plate - Send`. Caption: "100 % wet, 0 % dry is for an aux bus; on an insert you hear only the reverb" |

Notes:

- ASCII `fileName`s are `createLegalFileName`-stable. They must not collide with the bank (none do) and are never on disk.
- Insert lessons must sit at or below +5 dB re input (K-weighted pink), using the §9 measurer. `insertSend` is exempt as a Send.
- WET/DRY values sit within the bank's own envelope ("Pad Wash" 45/80, "Dark Hall" 40/95), which already passes.
- Max 6 entries per recipe, so `Entry entries[6]`.

---

## 9. Gates

### 9.1 Suite gates

| Gate | Location | Change |
|---|---|---|
| check-i18n | `scripts/check-i18n.js` (CI `ui-static-gates.yml`) | Auto-discovers. The new `i18n.js` and embedded-and-served pairing (assertion 8) must hold for `modules/preset-manager.js` too. |
| check-ui-labels | `scripts/check-ui-labels.js --plugin O-simpleReverb` | Parses the **last numeric `setSize(W,H)`** in PluginEditor.cpp (`:485`), so keep `setSize (1180, 780)` literal and last (Amb `PluginEditor.cpp:419-423`). It reads `tests/i18n-states.json`. |
| i18n-fr-lint / i18n-zh-lint | `scripts/` (CI) | Auto. fr `reviewed:false`, zh `'mt'`. |
| boot-all-uis | `scripts/boot-all-uis.js [--plugin O-simpleReverb]` | Auto; the generic stub plus overrides (§1.4). |
| param-dump | `scripts/param-dump` via `OUARICON_BUILD_TESTS` | Regenerate `.planning/params.tsv`. |

### 9.2 render-check

Location: `tests/render-check/{main.cpp,measure.h}`, target `O-simpleReverb-render-check` (`ouaricon_add_processor_console`, `CMakeLists.txt:107-111`). Build it with `-DOUARICON_BUILD_TESTS=ON`. `main()` deletes the factory sentinel and reads the **installed** Factory folder (`main.cpp:56-58`), which resolves case-insensitively after the rename.

### 9.3 New or ported render-check gates

1. **§4 port.** "Peak held until read": read through `meterFrames` (`hasNewData` / `read`) under `#if __has_include("MeterFramePayload.h")`, so the v1.14.0 negative-control build still compiles (the file already uses `__has_include("dsp/FdnEngine.h")`, `main.cpp:107`).
2. **Payload.** Format is deterministic, hash-equal means bytes-equal, a fuzzed NaN/Inf frame still parses, and the idle edge emits one zero frame (Amb harness PL1 pattern).
3. **Decay well tracks the tail.** Run a burst, sample frames every 1600 samples, and fit the `wetDb` slope against base × DECAY, ±20 %, for 6 types × DECAY {0.5, 2.0}.
4. **Tone curve = DSP filters** (§4 anti-drift).
5. **ER native = engine taps.** `getEarlyReflectionTaps` agrees with an independently prepared `EarlyReflections` and with the §12 measured first arrivals.
6. **Lessons.** 11 ids resolve; each apply resets first (it is idempotent after a dirty state); insert lessons are ≤ +5 dB.
7. **Mutants.** Add at least `frameNoReset` (peaks never reset) and `toneCurveStale` (the curve ignores CHARACTER). The 30 existing mutants are unchanged.
8. `--write-frames` writes `tests/ui-fixtures/frames.json`.

### 9.4 Null v3 against v2.0.0

The DSP is deterministic: no `Random`, fixed modulation phases (`FdnEngine.h:31-32`, `PlateEngine.h:52-53`, `SpringEngine.h:41-43`). **Check this first** by rendering v2.0.0 twice and running `cmp`.

1. `git archive O-SimpleReverb-v2.0.0 CMakeLists.txt modules plugins/O-SimpleReverb scripts | tar -x` into the scratchpad. Copy `Source/ui/public/modules/*.js` in (memory `pattern_git_archive_ui_tree_misses_gitignored_module_copies`).
2. Build it out of tree. **Never** check out or stash in the shared checkout.
3. Render each VST3 **in its own process** (they share a class ID): `uv run --with pedalboard`. Set parameters by `raw_value` and match by name (`type`, `character`, `wet`, `dry`, `decay`, `size`, `low_cut_freq`, `low_cut_on`). Write `.npy`, then compare bit-exact.
4. Stimuli: pink burst + silence, sine sweep, impulse. Sweep 48 presets × 44.1 / 96 kHz, and run an automation run with mid-block TYPE, SIZE and DECAY moves.
5. **Positive control:** v3 with LPON flipped must differ.
6. The meter refactor (moving `outputPeak` into the frame) does not touch the audio path. The CHARACTER / type-EQ static extraction is the one audio-path edit, and the null covers it.

State: decode a v2.0.0 `raw_state` (Amb `verify_3_state.py` decoder), load it into v3, and confirm all 8 decoded values plus `uiLanguage`. The existing `tests/fixtures/state-v1.14.0.bin` gate (§10) carries over.

---

## 10. Complexity, risks, plan shape

### Complexity: High

About 20 files touched or created; DSP none (one pure refactor in the processor's tone-map code); UI full rewrite; parameters unchanged; breaking: name only.

### Risks

| # | Risk | Mitigation |
|---|---|---|
| R1 | A case-only `git mv` in a shared checkout picks up other sessions' staging, or leaves a half-renamed tree | Two-step mv in its own path-scoped commit, first. Re-check `git branch` and `git status --short` right before committing. |
| R2 | A stale CMake cache, `SKIP_PLUGINS` or the canary script still names `O-SimpleReverb` | Reconfigure; update `canary-test.py`; grep for zero live hits as a plan acceptance check. |
| R3 | The Logic AU cache shows the old name, or both release and dev variants stay installed | Use `build-and-install.sh` (sweeps both variants and the cache); verify with `auval -a \| grep -i simplereverb` (one entry per variant). |
| R4 | A bridge gap: one of the 20 natives is unregistered and its control is silently dead | Census in the plan: grep `getNativeFunction` in index.html and preset-manager.js against `withNativeFunction` in PluginEditor.cpp, set-equal. |
| R5 | The tone curve or ER drift from the DSP | Native-only data; render-check gates 4 and 5. |
| R6 | The walk-order change surprises users | It is UI-only. Note it in the CHANGELOG. |
| R7 | Meter accumulators overflow when no editor is open | Double sums with a halving cap (§6.3). |
| R8 | French lesson-button width: 6 + 5 buttons at 1180 px | `lesson-row-budget` probe per row; pin at fr (memory `pattern_parallel_localizers_need_glossary_first`, `pattern_composed_readout_needs_setlabel_tokens_and_tour_row_budget`). |
| R9 | "Character (absorption)" copy is false (§4) | Rename the lesson and tooltip copy (Q3). |
| R10 | PLUGINS.md is dirty in the shared checkout | Edit only the Reverb row; path-scope the commit. |
| R11 | Removing `outputPeak` breaks the v1.14.0 negative-control build of render-check | `__has_include` guard (§9.3.1). |

### Domain

| Domain | Score | Keywords |
|---|---|---|
| GUI | 9 | WebView, wells, lessons, i18n, resize, probes |
| DSP | 3 | payload taps, tone-curve statics, null |
| Polish | 2 | rename, docs |

**Detected:** mixed. **Execute agent:** `general-purpose`. The gui-agent lacks Bash for the gates, and the C++ payload and natives need render-check.

### Suggested plan waves

1. **W0 Rename.** Two-step mv; CMake; literals; registry regen; PLUGINS.md; README text; canary script; banner comments. Build, auval, render-check green. **Build the null harness now** and null the renamed build against v2.0.0 before any other change.
2. **W1 C++ plumbing.** TripleBuffer, MeterFrame and Payload; processor accumulators and publish; editorScale; PresetRecipes and the lesson core; catalog, walk and presetState; tone-map static refactor; the three model natives; editor rewrite (relays, gestures, resize, timer). Render-check gates 1-8. Null again.
3. **W2 Page.** Build `index.html` from the signed-off mockup; i18n.js en/fr/zh (glossary first); insects.png; CMake binary data. Delete paper.jpg and flora.png.
4. **W3 Tests.** ui-stub overrides; frames.json; 4 probes; pedalboard verify_3_null / verify_3_state; i18n-states.json; retire ui_tip_render_check.js. Full suite gates at 0.6×, 1.0× and 1.5× in en, fr and zh-Hans.
5. **W4 Docs and release prep.** `.planning` BRIEF, REQUIREMENTS, ROADMAP and STATUS; CHANGELOG 3.0.0 (rename, migration note: none needed and why, walk-order note); NOTES; params.tsv. No tag.

### Version: MAJOR (3.0.0)

The rename is user-visible. Parameter IDs, ranges, preset format and state are unchanged (plus the additive `editorScale` root property).

---

## Open questions

1. **Q1 (blocking for sign-off):** the mockup (`mockup/index.html`) is the other research deliverable and **is not produced by this pass**. Plan stays gated on it.
2. **Q2:** keep **Load from file… / Save to file…** (v2's file-chooser natives), or adopt the family popover Save + Delete only and register the dialogs as refused? Dropping them removes import of a preset from outside the User folder. Recommend: the family bar, plus a single "Load file…" entry in the gear popover.
3. **Q3:** rename the "Character (absorption)" lesson and copy to "Character: dark ↔ bright"? Recommend yes; it is a post-tank tilt (§4).
4. **Q4:** add the optional 8-line case-sensitive-volume folder rename (§2.4)? Recommend yes; it is free on normal volumes.
5. **Q5:** accept the ◀/▶ walk-order change (alphabetical merged → bank TYPE order, then user)? Recommend yes.

---

## Appendix A: early-reflection tap tables

Times are ms after the **pre-delayed** input, at 48 kHz, rounded to whole samples exactly as `applyLengths()` does. Add `preDelayMs` for the arrival time after the dry signal. The level at the output is `earlyLevel × g` (before the slot trim).

Gains (all types, all sizes):

- **L:** +0.7993, −0.4289, +0.2765, −0.2031, +0.1564, −0.1267, +0.1061, −0.0879
- **R:** +0.7349, −0.4255, +0.3767, −0.2231, +0.2009, −0.1418, +0.1204, −0.1105

Fractions (`EarlyReflections.h:54-55`):

- **L:** .110 .205 .318 .433 .562 .694 .829 1.000
- **R:** .143 .247 .279 .471 .523 .741 .873 .951

| Type (pre-delay, earlyLevel) | SIZE | scale | L taps (ms) | R taps (ms) |
|---|---|---|---|---|
| Booth (3 ms, 0.6) | 0 | 0.5 | 0.38 0.71 1.10 1.50 1.94 2.40 2.85 3.46 | 0.50 0.85 0.96 1.62 1.81 2.56 3.02 3.27 |
| | 50 | 1.0 | 0.75 1.42 2.19 2.98 3.88 4.79 5.73 6.90 | 0.98 1.71 1.92 3.25 3.60 5.10 6.02 6.56 |
| | 100 | 2.0 | 1.52 2.83 4.40 5.98 7.75 9.58 11.44 13.79 | 1.98 3.42 3.85 6.50 7.21 10.23 12.04 13.12 |
| Room (15 ms, 0.4) | 0 | 0.5 | 1.27 2.35 3.67 4.98 6.46 7.98 9.54 11.50 | 1.65 2.83 3.21 5.42 6.02 8.52 10.04 10.94 |
| | 50 | 1.0 | 2.52 4.71 7.31 9.96 12.92 15.96 19.06 23.00 | 3.29 5.69 6.42 10.83 12.02 17.04 20.08 21.88 |
| | 100 | 2.0 | 5.06 9.44 14.62 19.92 25.85 31.92 38.12 46.00 | 6.58 11.35 12.83 21.67 24.06 34.08 40.17 43.75 |
| Hall (50 ms, 0.3) | 0 | 0.5 | 2.52 4.71 7.31 9.96 12.92 15.96 19.06 23.00 | 3.29 5.69 6.42 10.83 12.02 17.04 20.08 21.88 |
| | 50 | 1.0 | 5.06 9.44 14.62 19.92 25.85 31.92 38.12 46.00 | 6.58 11.35 12.83 21.67 24.06 34.08 40.17 43.75 |
| | 100 | 2.0 | 10.12 18.85 29.25 39.83 51.71 63.85 76.27 92.00 | 13.15 22.73 25.67 43.33 48.12 68.17 80.31 87.50 |
| Ambient (35 ms, 0.25) | 0 | 0.5 | 3.17 5.90 9.15 12.46 16.17 19.96 23.83 28.75 | 4.10 7.10 8.02 13.54 15.04 21.31 25.10 27.33 |
| | 50 | 1.0 | 6.33 11.79 18.29 24.90 32.31 39.90 47.67 57.50 | 8.23 14.21 16.04 27.08 30.06 42.60 50.19 54.69 |
| | 100 | 2.0 | 12.65 23.58 36.56 49.79 64.62 79.81 95.33 115.00 | 16.44 28.40 32.08 54.17 60.15 85.21 100.40 109.38 |
| Plate (8 ms, —) | any | 0.40..0.80 | none (diffuse from the first sample) | none |
| Spring (0 ms, —) | 0 / 50 / 100 | 0.75 / 1.0 / 1.333 | spring echoes: 24.8/27.8/30.8 · 33/37/41 · 44/49.3/54.7 ms (L feeds spring 1, R spring 2, both feed 3) | |

Teaching fact for **Size ≠ Decay**: Room at SIZE 100 has exactly Hall-at-50's tap pattern, and Hall at SIZE 0 has Room-at-50's. The shape is the same; the decay times (1.1 s vs 3.0 s) are not.

## Appendix B: tone-stage reference values (for the mockup)

- **CHARACTER Warm cutoff:** −100 → 2.0 kHz; −80 → 5.6 kHz; −50 → 11.1 kHz; −20 → 16.5 kHz; ≥ 0 parked at 0.49·fs.
- **CHARACTER Bright:** high shelf at 4 kHz, +0 … +6 dB linear in 0 … +100.
- **Type EQ:** Booth HPF 150 Hz (Q 0.707); Room flat; Hall shelf 3 kHz −2 dB (Q 0.5); Spring peak 800 Hz +4 dB (Q 2.5); Plate shelf 5 kHz +3 dB (Q 0.707); Ambient shelf 2.5 kHz −3 dB (Q 0.5).
- **Low Cut:** 12 dB/oct Butterworth high-pass, 20–400 Hz, default 200 Hz, OUT by default.

---

*Generated by improve-milestone research phase*

---

## Mockup Sign-off (2026-10-15)

`mockup/index.html` signed off by the user, with these amendments. Plan and execute treat them as requirements.

| # | Decision |
|---|----------|
| M1 | **Knob baselines:** lower the Tone and Mix knobs to the Engine knob baseline so all six knobs line up. The IN/OUT pill uses the space above. The mockup still shows the older offset and is not re-rendered. |
| M2 | **Type names** (buttons + the six type lessons) stay in English in fr and zh-Hans, because they are the TYPE choice strings. |
| M3 | **Low Cut pill words:** en IN/OUT, fr ACT./DÉS., zh 开/关 (the v2 convention). |
| M4 | **Live decay well** shows absolute dBFS, not normalised to the input level. |
| M5 | The spare space (right end of the Ideas row, the extra line in the Engine note) is left empty. |
| Q2 | **Preset bar:** the family Save + Delete bar, plus a "Load file…" entry in the gear menu. The v2 dialogs are dropped. |
| Q3 | **Character lesson:** "Character: dark ↔ bright". The word "absorption" is removed from all copy. |
| Q4 | **Preset folder:** add the optional guarded case-only rename (it does nothing on case-insensitive volumes). |
| Q5 | **◀/▶ order:** type order, then user presets. |

**Interim note:** the tone well in the mockup computes its curve in JS. The build uses the native `getToneCurve` (§4).
