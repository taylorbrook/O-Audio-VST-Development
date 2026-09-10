# Stage 1 — Foundation, second pass (re-parameterise the verified fork): RESEARCH

**Plugin:** O-Strata
**Stage:** 1 of 4 — Foundation + Shell, **second pass** (ROADMAP "Stage 1: Foundation — second pass")
**Phase:** research ✓
**Date:** 2026-09-10
**Input:** `stages/1-foundation/CONTEXT.md` (second pass, D1–D4, seven open items)
**Method:** every claim below was read from the fork's `Source/`, the JUCE 8.0.14 tree at `/Users/taylorbrook/JUCE`, the repo scripts, git history, or the dev machine's disk in this session. Nothing is copied from the first-pass RESEARCH without re-reading. `[VERIFIED: path:line]` marks a fact read from source; `[ASSUMPTION]` marks the two things that could not be read.

---

## 1. Summary

The seven open items all resolve in favour of the CONTEXT's plan, with four corrections that the planner must carry:

1. **Preset folder is `~/Library/O-Strata/Presets/`, not Application Support**, and the initializer never deletes — the execute step must `rm -rf` the folder by hand. The stale bank on disk is **192** JSON files in 9 categories (not "≈ 20"), and the fork's `FactoryPresets.cpp` actually builds 192 presets (its "96" header comment and `jassert (out.size() == 96)` are stale; the assert is inert in Release).
2. **The fork's `OuariconPresetManager.h` has no v1.0.6 migration hook** — the fork is from O-Prism v1.24.0 and the hook was ported at v1.25.0. There is nothing to "leave untouched"; nothing is needed in Stage 1.
3. **`ValueRemapFunction` takes three arguments** `(start, end, v)`, not one. Default text for a continuous range prints **7 decimals** (`0.2500000` / `8.0000000` / `1.0000000`) — the verify criterion "0.25 / 8.0 ends" is about values, not text.
4. **`plugins/O-Prism/.planning/params.tsv` is the v1.25.0 registry** (commit `a774d6d4`); the only change from v1.24.0 is the two `osc?Table` rows, which the diff removes anyway. Diff against `git show a774d6d4^:…` to make "−2" literal.

Everything else is confirmed as the CONTEXT expects: the 48 baked pushes are one contiguous block with reusable `choice`/`knob` lambdas; no raw-value cache or listener names a removed ID; `WebComboBoxRelay` is an `OptionsBuilder` and its attachment is silent with no page; `getNumSteps()` returns `0x7fffffff` for a lambda range and `getDefaultValue()` gives 0.400000; the smoke harness `build.sh` derives its object list from `build.ninja` at run time.

---

## 2. Open items resolved

### 2.1 Item 1 — Preset folder, deletion, regeneration

**Path** `[VERIFIED: Source/OuariconPresetManager.h:161-177]`:
```
~/Library/O-Strata/Presets/Factory/<Category>/<Name>.json
~/Library/O-Strata/Presets/Factory/.factory-version      ← "1.0.0" today
~/Library/O-Strata/Presets/User/<Name>.json
```
`getPresetsDirectory()` = `userHomeDirectory/Library/<pluginName>/Presets`. The module header (`modules/persistence/preset-manager/cpp/OuariconPresetManager.h:45`) warns "Do not switch to Application Support without a one-time migration" — the CONTEXT's guess of Application Support is wrong.

**Deletion semantics** `[VERIFIED: OuariconPresetManager.h:562-605]`: `initializeFactoryPresets()` only `createDirectory()`s and `replaceWithText()`s the presets it is given, then writes `.factory-version`. It never enumerates or removes existing files. A shrinking bank therefore leaves orphans forever (module-level gap; irrelevant for Stage 1 since nothing has shipped, but worth a Stage 4 note when the real bank lands).

**Regeneration gate** `[VERIFIED: PluginProcessor.cpp:568-572; OuariconPresetManager.h:179-191]`:
```cpp
if (! presetManager.factoryPresetsExist()
    || presetManager.getFactoryPresetsVersion() != factoryVersion)   // "1.0.0"
    presetManager.initializeFactoryPresets (FactoryPresets::build (parameters), factoryVersion);
```
`factoryPresetsExist()` = the `Factory` dir exists **and** some category subdir holds ≥ 1 `*.json`. After `rm -rf ~/Library/O-Strata/Presets/Factory` it returns false, so the constructor regenerates regardless of the version stamp. Nothing is loaded at construction (`currentPresetName` starts as `"Default"`); the bank is only written.

**Dev-machine state today** (read 2026-09-10): `Factory/` holds **192** JSON files (Bass 20, Drone 22, FX 20, Keys 20, Lead 24, Pads 26, Percussion 18, Pluck 20, Sequence 22), `.factory-version` = `1.0.0`, `User/` absent. Every file carries `"plugin": "O-Strata"`, `oscAUnison` values up to 0.714 (= 6 of 1–8) and `oscAPos` 0.0–0.3 — the O-Prism-derived content D2 removes.

**Ordering trap:** any binary whose constructor runs after the deletion regenerates a bank — the *old* installed O-Strata-dev (Logic re-scan, a stale Standalone) would put the 192 back. Delete immediately before the first run of the **new** build, and let the smoke harness (which constructs the processor through `createPluginFilter()`) be that first run and assert the folder contents. Recommended sequence: build → `rm -rf ~/Library/O-Strata/Presets` (User is empty, so the whole tree is safe) → `smoke/build.sh` + run → `find ~/Library/O-Strata/Presets/Factory -name '*.json'` = exactly `Init/Init.json` → then install/pluginval/auval (which construct again but find the bank present at 1.0.0 and skip).

### 2.2 Item 2 — `createOscParameters` shape and removed-ID references

**Block layout** `[VERIFIED: PluginProcessor.cpp:73-170]`:
- 73–116: signature, `label`, `levelDefault`, the **11 carried pushes** (`Pos` … `WarpAmt`). `osc?Pos` is `NormalisableRange (0,1,0.001), 0.0f` with name `label + " Position"` (line 82-84); `osc?Unison` is `AudioParameterInt (…, 1, 8, 1)` (line 101-102).
- 118–167: the baked block — comment, three UTF-8 strings (`imported`, `epitroch`, `hypocycl`), the `choice` / `knob` lambdas (with the `jassert (choices.size() >= 2)` guard), then the 24 `choice(...)`/`knob(...)` calls from `GeoSource` (line 143) to `TerEdge` (line 167). **Contiguous**; `return params;` at 169. The lambdas and `imported` are exactly what the new 17 pushes need; `epitroch` / `hypocycl` go.
- `createParameterLayout()` (519-549) is untouched — the section comments `// 10` are already stale and harmless.

**Removed-ID references outside the block** — full grep of `Source/` for every baked suffix plus `geometryImports` `[VERIFIED]`:

| Site | What | Action |
|---|---|---|
| `StrataParamIds.h:57-61` | the 24 suffixes in `oscIds()` | replace with rows 1–11 + the 13 Float suffixes (24 IDs); add `oscComboIds()` / `allComboIds()`; fix `allSliderIds()` comment counts (35 → 24 per osc; 188 → 166) |
| `PluginProcessor.cpp:1081` | `state.getOrCreateChildWithName ("geometryImports", nullptr)` | → `"terrainImports"` |
| `PluginProcessor.cpp:1127-1129` | comment + `ignoreUnused (state.getChildWithName ("geometryImports"))` | → `"terrainImports"` |
| `smoke/main.cpp:8, 157-160` | harness asserts the child | → `terrainImports` (see 2.6) |
| `tests/ui_tip_render_check.js:1002-1007` | **comment only** ("103 of O-Strata's 219 parameters … the 48 geometry parameters") | optional wording refresh; the gate is `TIP_BINDINGS.length === 106`, unaffected |

**Raw-value caches / listeners:** every `getRawParameterValue` and `addParameterListener` string in `Source/*.cpp|h` was enumerated; the set is the inherited O-Prism IDs only (`osc?Pos/Level/Pan/Coarse/Fine/Phase/Unison/Detune/Width/WarpType/WarpAmt`, sub/noise, filters, FX, LFOs, tuning, global). **None names a baked ID; none names a new ID.** `StrataVoice.cpp:54-69` caches `pOsc?Unison` / `pOsc?Pos`; lines 257/280/473/477 cast the raw Unison to `int` into `setUnison`, which accepts ≤ `kMaxUnison = 8` (`dsp/WavetableOscillator.h:76`) — a 1–4 parameter is a strict subset; nothing to change (CONTEXT D3 / placeholder row).

**Shadow re-grep (execute step):** none of the 17 suffixes is a `juce::` free function; the only way to collide is a `pTerrain`-style cache, which Stage 1 does not add. Command for the executor:
```bash
for s in Terrain TerFreq TerModX TerModY TerTrack TerSat TerBlur TerEdge Orbit OrbAspect OrbRot OrbCX OrbCY OrbMod OrbFeedback OrbFbDamp Quality; do
  grep -rIn --include='*.h' -E "^\s*(inline|static|constexpr|JUCE_API)?\s*[A-Za-z_:<>]+\s+$s\s*\(" /Users/taylorbrook/JUCE/modules/juce_core /Users/taylorbrook/JUCE/modules/juce_audio_basics && echo "SHADOW: $s"; done; echo done
```

**Order of the 17 new pushes** (spec rows 12–28, after `WarpAmt`): `Terrain`(C7) `TerFreq`(F, log) `TerModX` `TerModY` `TerTrack` `TerSat` `TerBlur` `TerEdge`(C2) `Orbit`(C11) `OrbAspect` `OrbRot` `OrbCX` `OrbCY` `OrbMod` `OrbFeedback` `OrbFbDamp` `Quality`(C3, default index 1). Host names `label + " Terrain Freq"` etc. per spec §semantics (not contractual).

**Mod destinations** `[VERIFIED: dsp/ModulationMatrix.h:54-101; PluginProcessor.cpp:466-482]`: `ModDest` ends at `OscBWarpAmt, NumDests`; `getModDestNames()` is a 26-entry initialiser list; `createModMatrixParameters()` builds every `modSlot?Dst` from `getModDestNames()`, so the 46-entry list propagates with no other edit. `destOffsets` is `std::array<float, kNumDests>` (`:154`) and the slot loop bounds-checks against it (`ModulationMatrix.cpp:84`) — D3 stands. Index of `OscA Terrain Freq` = **31** (spec §Mod-matrix destinations; 26 + 5). The `DST_*` enum in `FactoryPresets.cpp:95-102` mirrors the old 26 and disappears with the file (2.5).

### 2.3 Item 3 — `WebComboBoxRelay` / `WebComboBoxParameterAttachment` (JUCE 8.0.14)

**Relay** `[VERIFIED: juce_gui_extra/misc/juce_WebControlRelays.h:225-271; .cpp:212-262]`:
```cpp
class WebComboBoxRelay : public OptionsBuilder<WebBrowserComponent::Options>, private WebViewLifetimeListener
{ public: WebComboBoxRelay (StringRef nameIn); … };
```
- It is an `OptionsBuilder`, so `options = options.withOptionsFrom (*relay);` is the same call the fork already makes for slider and toggle relays (`PluginEditor.cpp:59`, template `:222-223`).
- `buildOptions()` registers the event listener `__juce__comboBox<name>`, `withInitialisationData ("__juce__comboBoxes", name)` and a lifetime listener. `browser` is `nullptr` until `webViewConstructed()`; `emitEvent()` is a no-op while it is null and otherwise `emitEventIfBrowserIsVisible()` (`juce_WebBrowserComponent.cpp:607-611`).

**Attachment** `[VERIFIED: juce_audio_processors/utilities/juce_ParameterAttachments.h:366-396; .cpp:385-439]`:
```cpp
WebComboBoxParameterAttachment (RangedAudioParameter& parameterIn, WebComboBoxRelay& combo, UndoManager* undoManager = nullptr);
```
- Constructor: `sendInitialUpdate()` (emits a `propertiesChanged` event carrying `name`, `parameterIndex` and, for an `AudioParameterChoice`, `choices`) then `relay.addListener (this)`. Parameter → page is `relay.setValue (convertTo0to1 (v))` (normalised); page → parameter is `setValueAsCompleteGesture (convertFrom0to1 (v))`.
- **No DOM node, no page, or WebView not yet constructed: silent.** No log, no assert. The only `jassertfalse` (`:429`) is re-entrancy during `ignoreCallbacks`, which cannot fire without a page. Same behaviour as the slider attachments proven at 188 in the first pass.
- Declared inside the same `#if` block as the web slider attachment (`.h:398 #endif`); the fork's `#if JUCE_WEB_BROWSER` editor guard already covers it, and the param-dump / smoke build excludes the editor TU (2.6).

**Order** (template `v2-PluginEditor-TEMPLATE.cpp:196-241`, matching the fork's slider group): construct relays → `withOptionsFrom` every relay → construct the `WebBrowserComponent` → construct attachments → `jassert (comboAttachments.size() == comboRelays.size())`. Declaration order in the header (relays first, attachments last) gives the required destruction order (`PluginEditor.h:51-73`).

### 2.4 Item 4 — Exact-log `NormalisableRange` and param-dump columns

**Constructor** `[VERIFIED: juce_core/maths/juce_NormalisableRange.h:104-124]`:
```cpp
using ValueRemapFunction = std::function<ValueType (ValueType rangeStart, ValueType rangeEnd, ValueType valueToRemap)>;
NormalisableRange (ValueType rangeStart, ValueType rangeEnd,
                   ValueRemapFunction convertFrom0To1Func, ValueRemapFunction convertTo0To1Func,
                   ValueRemapFunction snapToLegalValueFunc = {}) noexcept;
```
Three arguments, not one — write the lambdas in terms of `start`/`end` so they are pure and capture nothing:
```cpp
juce::NormalisableRange<float> terFreqRange (0.25f, 8.0f,
    [] (float s, float e, float n) { return s * std::pow (e / s, n); },                 // 0.25 · 32^n
    [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });    // log32 (v / 0.25)
```
`interval` stays 0 (`:232`); `skew` is ignored when lambdas are present (`:205-206, 243-244`); `convertTo0to1` clamps the lambda's output and `convertFrom0to1` clamps its input (`:136-166`), so out-of-range inputs cannot produce NaN. No `snapToLegalValue` needed.

**Parameter constructor** `[VERIFIED: juce_audio_processors_headless/utilities/juce_AudioParameterFloat.h:76-80]`: `AudioParameterFloat (const ParameterID&, const String&, NormalisableRange<float>, float defaultValue, const AudioParameterFloatAttributes& = {})` — the same overload every other float in the file uses; no attributes required.

**param-dump derivation** `[VERIFIED: scripts/param-dump/main.cpp:139-149; juce_RangedAudioParameter.cpp:38-46]`:

| Column | Source | Value for `osc?TerFreq` |
|---|---|---|
| `numSteps` | `RangedAudioParameter::getNumSteps()`: `interval > 0 ? …+1 : getDefaultNumParameterSteps()` | **2147483647** ✓ |
| `defaultNorm` | `String (p->getDefaultValue(), 6)` = `convertTo0to1 (1.0)` = log 4 / log 32 = 0.4 (float 0.40000001) | **0.400000** ✓ |
| `textAtMin` / `textAtMax` / `defaultText` | `AudioParameterFloat::getText` → default `stringFromValueFunction` with `numDecimalPlacesToDisplay` = **7** when `interval == 0` (`juce_AudioParameterFloat.cpp:52-77`) | `0.2500000` / `8.0000000` / `1.0000000` |

So the row reads `osc?TerFreq  Osc ? Terrain Freq  <label>  2147483647  0.2500000  8.0000000  0.400000  1.0000000  automatable`. The CONTEXT criterion "0.25 / 8.0 ends" is satisfied as values. **Recommendation:** keep the default text function — `PluginProcessor.cpp` has zero `AudioParameterFloatAttributes` / `withStringFromValueFunction` uses, so adding one just for this parameter would be the file's only exception, and the `×` unit belongs to the UI readout (spec §semantics). If the planner wants a tidier host lane, `AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2); })` gives `0.25` / `8.00`; the default `valueFromString` is `getFloatValue()` either way.

`[ASSUMPTION]` pluginval strictness 10 exercises `setValue`/`getValue` and text round-trips on a lambda range without demanding bit-exact identity; float `log`/`pow` round-trip to ≈ 1e-7. JUCE's own examples use this exact log pattern for frequency ranges, and the pluginval binary is not readable locally, so this is stated rather than verified — the COMPAT-01 run is the check.

### 2.5 Item 5 — `FactoryPresets.cpp` minimal shape for a single `Init`

**Types** `[VERIFIED: OuariconPresetManager.h:97-107]`: `FactoryPresetDef { juce::String category; juce::String name; std::map<juce::String, float> parameters; juce::var customState; }`; `initializeFactoryPresets (const std::vector<FactoryPresetDef>&, const juce::String& versionStamp)`. Values in `parameters` are **normalised** (`:571-580` writes them verbatim; `loadPreset` applies them with `setValueNotifyingHost`, `:266-268`). `excludedParameterIds` (the 7 tuning IDs) are dropped at write time (`:576-577`).

**Reset-first is real** `[VERIFIED: OuariconPresetManager.h:250-261]`: `loadPreset` resets every non-excluded `RangedAudioParameter` to `getDefaultValue()` **before** applying the map, so an `Init` with an empty map is functionally a reset (memory `pattern_preset_apply_needs_reset_to_defaults` is already honoured by this copy). `initializeFactoryPresets` writes `"parameters": {}` for an empty map and `loadPreset` only needs `hasProperty ("parameters")`, so even that degenerate form loads.

**Recommended shape** — self-describing, no hand-typed IDs, no O-Prism content:
```cpp
std::vector<Preset> FactoryPresets::build (juce::AudioProcessorValueTreeState& apvts)
{
    Preset init;
    init.category = "Init";
    init.name     = "Init";
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            init.parameters[rp->getParameterID()] = rp->getDefaultValue();   // already normalised — do NOT pass through a raw→norm helper
    return { init };
}
```
Yields `Factory/Init/Init.json` with 198 keys (205 − 7 excluded), `"category": "Init"`, `"factory": true`, version `1.0.0`. It tracks any future parameter change automatically and is exactly the 4.1 seed (4.1 adds real presets alongside it). The alternative — re-typing a `completeBase()` map of 205 IDs — is a drift hazard for no benefit. `getPresetList()` (`:404-433`) sorts names across categories; with one preset, next/previous return `Init` (`:480-496`); `getPresetListWithCategories()` groups by the category directory name, so the page's dropdown shows one group "Init".

**What is deleted:** `FactoryPresets.cpp:37-2331` — helpers (`merge`, `normalize`, `makePreset`, `modSlot`), the `SRC_*`/`DST_*` enums, `completeBase()`, the archetypes, and the nine `add*` functions (193 `makePreset` call sites, **192** presets — the header comment "96 presets" and `jassert (out.size() == 96)` at `:2352` have been wrong since the fork; Release never fired it). `FactoryPresets.h` keeps its declaration; its "96 presets across 9 categories" comment should be reworded. Result ≈ 60 lines.

**Migration hook — correction to CONTEXT D2:** the fork's `OuariconPresetManager.h` is the **pre-v1.0.6** copy — `diff plugins/O-Prism/Source/OuariconPresetManager.h plugins/O-Strata/Source/OuariconPresetManager.h` shows only the name lines and the **absence** of `MigrationCallback` / `setMigrationCallback` in O-Strata (O-Prism added them for 1.25.0, after the fork point; `modules/registry.yaml:138-140` lists the module at 1.0.6). There is no hook to leave untouched. Nothing is needed now (no O-Strata preset has shipped; ROADMAP's "no preset-migration gate needed" stands). **Stage 4 note:** if a list or range ever widens after v1.0.0, port the hook from O-Prism `PluginProcessor.cpp:525-547` / module v1.0.6.

### 2.6 Item 6 — Smoke harness link after the shrink

**`build.sh` is self-updating** `[VERIFIED: smoke/build.sh]`: it greps the `O-Strata-param-dump` link rule out of `build/build.ninja` at run time, takes every `.o` except `param-dump/main.cpp.o`, and reuses that rule's `LINK_LIBRARIES`, plus the `DEFINES`/`INCLUDES` of the param-dump `main.cpp.o` compile rule. Today's rule lists 20 plugin objects — `PluginProcessor, FactoryPresets, StrataVoice, TuningEngine, ScaleGenerator, EmbeddedTunings, TuningExporter` and 12 `dsp/*` — plus `NoteExpression.cpp.o` and the JUCE module objects; **no `PluginEditor.cpp.o`**. A smaller `FactoryPresets.cpp` changes nothing in the rule. Two preconditions the planner must state:
1. `ninja O-Strata-param-dump` **before** `build.sh` — the script links existing objects and does not rebuild them; stale objects give a stale harness that still passes with 219.
2. The harness constructs the processor through `createPluginFilter()` at `JUCE_WEB_BROWSER=0`, so the constructor's factory-bank write runs inside it (2.1 ordering).

**`main.cpp` edits** `[VERIFIED: smoke/main.cpp]`:
- Header comment lines 7–8; `219` at lines 7, 167, 175, 190, 191 → `205`; the `214 randomised` note at line 192 → recompute from the harness's own exclusion list (it is the parameter count minus the IDs the test leaves alone).
- `geometryImports` → `terrainImports` at lines 8, 157, 158, 160.
- New check **[4] (D3)**: two fresh processors with identical settings (or one processor, `prepareToPlay` + `reset` between renders); in the second, `modSlot0Src` = LFO1 (1/10), `modSlot0Dst` = **31/45** (`OscA Terrain Freq`), `modSlot0Amt` = 1.0 (norm 1.0), `modSlot0On` = 1; render the same held note; assert the two buffers are sample-identical (`memcmp` or max |Δ| == 0 — the placeholder path is deterministic: noise level 0, LFO free-run off). A positive control in the same check: route the same slot to index 1 (`OscA Orbit Size` = `osc?Pos`) and assert the buffers **differ**, so the "unchanged" verdict is not vacuous (memory `pattern_zipper_sweep_probe_needs_liveness_gate`).
- New check **[5] (D2)**: after construction, `~/Library/O-Strata/Presets/Factory` contains exactly one `*.json`, at `Init/Init.json`, and `.factory-version` reads `1.0.0`; parse the JSON and assert `parameters` has 198 keys, none of the 7 excluded IDs, and `oscAPos` = 0.5, `oscAUnison` = 0.0, `oscAQuality` = 0.5 (index 1 of 3).
- Optional check on the new combos: `oscATerrain` choices = 7 with `Imported…` last; `oscAQuality` default index 1; `modSlot0Dst` choices = 46 with `[1]` = `OscA Orbit Size`, `[31]` = `OscA Terrain Freq`, `[45]` = `OscB Saturation` — cheap to add here and makes FUNC-05 evidence machine-checked rather than read off the TSV.

### 2.7 Item 7 — `params.tsv` baseline and diff procedure

**Provenance** `[VERIFIED: git log / git show]`: `plugins/O-Prism/.planning/params.tsv` = 177 lines = 4 header lines (`# plugin`, `# params 173`, `# note`, `#id …`) + 173 rows. Last touched by `a774d6d4` (v1.25.0); that commit changed **only** the two `osc?Table` rows (`numSteps 28 → 36`, `textAtMax 27 → 35`). v1.26.0 did not touch it (CHANGELOG: markup/CSS only). There is no `O-Prism-v1.24.0` tag (tags present: `O-Prism-v1.20.0`, `O-Prism-v1.26.0`, older version-first ones), so the literal v1.24.0 registry is `git show a774d6d4^:plugins/O-Prism/.planning/params.tsv`. Either file gives the same result for the 171 inherited rows; use the `a774d6d4^` version so the "−2 (`osc?Table`)" figure is literal rather than "−2 rows that also differ".

**Procedure** (ID-keyed, so the 34 insertions in the middle of the osc blocks do not smear the hunks):
```bash
cd /Users/taylorbrook/Dev/VST-development
git show a774d6d4^:plugins/O-Prism/.planning/params.tsv | grep -v '^#' | sort > /tmp/prism.rows
grep -v '^#' plugins/O-Strata/.planning/params.tsv                  | sort > /tmp/strata.rows
cut -f1 /tmp/prism.rows  | sort > /tmp/prism.ids
cut -f1 /tmp/strata.rows | sort > /tmp/strata.ids
echo "removed: $(comm -23 /tmp/prism.ids /tmp/strata.ids | tr '\n' ' ')"        # expect oscATable oscBTable
echo "added:   $(comm -13 /tmp/prism.ids /tmp/strata.ids | wc -l)"              # expect 34
join -t $'\t' -j1 /tmp/prism.rows /tmp/strata.rows | awk -F'\t' '{ n=(NF-1)/2; a=""; b=""; for(i=2;i<=n+1;i++){a=a"\t"$i; b=b"\t"$(i+n)}; if (a!=b) print $1 }' | sort > /tmp/changed.ids
echo "changed in place: $(wc -l < /tmp/changed.ids)"                              # expect 20
cat /tmp/changed.ids   # expect oscAPos oscBPos oscAUnison oscBUnison modSlot0Dst … modSlot15Dst
```
Expected in-place field deltas: `osc?Pos` name (`Osc ? Position` → `Osc ? Orbit Size`), `defaultNorm 0.000000 → 0.500000`, `defaultText 0.000 → 0.500`; `osc?Unison` `numSteps 8 → 4`, `textAtMax 8 → 4`; `modSlot?Dst` `numSteps 26 → 46`, `textAtMax OscB Warp → OscB Saturation`. The 34 added rows must sit between `osc?WarpAmt` and the next block in file order (the unsorted diff or `grep -n` confirms placement). Regenerate with `ninja O-Strata-param-dump && $(find build -name 'O-Strata-param-dump' -perm +111) > plugins/O-Strata/.planning/params.tsv` (README `scripts/param-dump/README.md:42-46`); the header will read `# params 205`.

---

## 3. Pitfalls checklist (memory → step it guards)

| Memory | Where it bites in this pass |
|---|---|
| `critical_choice_param_needs_two_choices` | `TerEdge` = 2 entries; the `choice` lambda's `jassert` stays |
| `critical_juce_string_char_ctor_is_ascii_only` | `Imported…` (existing), `Limaçon` (`Lima\xC3\xA7on`), `2×` / `4×` (`\xC3\x97`) via `CharPointer_UTF8` |
| `critical_paramid_shadows_juce_free_function` | re-grep in 2.2; no caches added |
| `critical_apvts_denormalised_vs_preset_normalised` | `Init` stores `getDefaultValue()` (normalised) directly; never route it through the deleted `normalize()` |
| `pattern_preset_apply_needs_reset_to_defaults` | already in `loadPreset` (2.5) — no change |
| `pattern_zipper_sweep_probe_needs_liveness_gate` / `pattern_probe_must_target_the_branch_the_fix_changed` | smoke [4] needs the positive-control route to `osc?Pos` (2.6) |
| `pattern_build_install_skips_standalone_stale_ui` | build `O-Strata_Standalone` explicitly if used |
| `pattern_cold_auval_after_install_rescans_registry` | `auval -a` in the background at batch end, 2-min budget |
| `pattern_git_commit_pathspec_takes_only_tracked_files` | `RESEARCH.md` (this file) and any new smoke files need `git add` |
| `pattern_rsync_restore_defeats_ninja_mtime` (cousin) | `smoke/build.sh` links existing objects — run `ninja O-Strata-param-dump` first (2.6) |
| `pattern_test_fixture_mirrors_drift_silently` | the harness's `205` / `198` / `31` literals mirror the spec — assert them against the live `getParameters().size()` and `getModDestNames().size()` too, so a later change fails loudly |

---

## 4. Recommended task ordering (each independently checkable)

1. `StrataParamIds.h` — new `oscIds()` (24), `oscComboIds()`, `allComboIds()`, comment counts → `allSliderIds().size() == 166`, `allComboIds().size() == 8` (a one-line `static_assert`-free check in the smoke harness or the editor `jassert`s).
2. `PluginProcessor.cpp:73-170` — `osc?Pos` default/name, `osc?Unison` 1–4, delete lines 118–167, add the 17 pushes (log range per 2.4) → build the param-dump target, 205 rows.
3. `dsp/ModulationMatrix.h` — enum + 46 names (spec strings, A block then B block; relabel `[1]`/`[2]`) → `modSlot0Dst` numSteps 46.
4. `PluginProcessor.cpp:1081/1129` — `terrainImports`.
5. `PluginEditor.h/.cpp` — combo relay/attachment group + `jassert` (relays only, D1).
6. `FactoryPresets.cpp/.h` — rewrite per 2.5.
7. Regenerate `params.tsv`; run the 2.7 diff (−2 / +34 / 20).
8. `rm -rf ~/Library/O-Strata/Presets`; update and run the smoke harness (checks [1]–[5]).
9. `build-and-install.sh O-Strata`; pluginval strictness 10 VST3 + AU (`.claude/skills/plugin-testing/references/pluginval-guide.md:50-57` shape: `pluginval --validate <bundle> --strictness-level 10`); `auval -v aumu OuSt OuDv`; background `auval -a | grep -i strata`.
10. UI gates as regression only (page byte-identical) — expect the first-pass baseline (0 DEAD / 0 late, 106 tip bindings, 2799 tip checks, 20 states).
11. CHANGELOG / NOTES / STATUS / PLUGINS.md; O-Prism isolation check; path-scoped commit, no tag.

---

## 5. Contradictions and gaps to flag (not silently resolved)

- **CONTEXT D2 "≈ 20 presets" / "v1.0.6 hook untouched"** — 192 presets on disk and in code; no hook exists in the fork (2.1, 2.5). Both are corrections of description, not of the decision.
- **CONTEXT item 1 "Application Support"** — the path is `~/Library/O-Strata/Presets` (2.1).
- **CONTEXT §Exact-log "from0to1: 0.25·32^n"** — correct maths, but the lambda signature is `(start, end, v)` (2.4).
- **CONTEXT criterion "`osc?TerFreq` … 0.25 / 8.0 ends"** — text columns will read `0.2500000` / `8.0000000`; verify against values or accept the 7-decimal strings (2.4).
- **ROADMAP tasks superseded by CONTEXT D1/D2** (UI knobs + i18n keys → Phase 3.1; "existing preset set re-pointed" → single `Init`), **ARCHITECTURE Core 9 short strings** (`OscA Aspect`, `OscA Fdbk`, `OscA CtrX`) and **ROADMAP `"OscA Size"`** — the locked spec's strings win (`OscA Orbit Aspect`, `OscA Feedback`, `OscA Orbit CX`, `OscA Orbit Size`); checksummed files are not edited.
- **`FactoryPresets.h` comment "96 presets across 9 categories"** and the `.cpp` header — stale since the fork; both go with the rewrite.
- **`tests/ui_tip_render_check.js:1002-1007`** — stale comment (219 / 48 geometry); gate value 106 unaffected. Refresh the wording or leave; either way the gate result is the baseline.
- **`verify-suite-battery.sh`** runs pluginval at strictness 8 with `--skip-gui-tests` — not the COMPAT-01 bar; run pluginval directly at 10 as the first pass did.
- **Preset-manager orphan gap** (initializer never deletes) — no Stage 1 impact; note for Stage 4 when the real bank replaces `Init` under a new version stamp.

---

## 6. Sources

- Fork: `plugins/O-Strata/Source/{PluginProcessor.cpp, PluginEditor.h/.cpp, StrataParamIds.h, StrataVoice.cpp, FactoryPresets.h/.cpp, OuariconPresetManager.h, dsp/ModulationMatrix.h/.cpp, dsp/WavetableOscillator.h}`; `tests/ui_tip_render_check.js`; `.planning/{parameter-spec.md, ROADMAP.md, research/ARCHITECTURE.md, params.tsv, mockups/v2-PluginEditor-TEMPLATE.h/.cpp, stages/1-foundation/{CONTEXT.md, smoke/build.sh, smoke/main.cpp, smoke/smoke-output.log, first-pass-baked/VERIFICATION.md}}`
- O-Prism: `Source/OuariconPresetManager.h`, `Source/PluginProcessor.cpp:525-547`, `.planning/params.tsv` (+ `git show a774d6d4`, `a774d6d4^`), `CMakeLists.txt` (VERSION 1.26.0)
- JUCE 8.0.14: `juce_gui_extra/misc/juce_WebControlRelays.h/.cpp`, `juce_gui_extra/misc/juce_WebBrowserComponent.cpp:607`, `juce_audio_processors/utilities/juce_ParameterAttachments.h/.cpp`, `juce_core/maths/juce_NormalisableRange.h`, `juce_audio_processors_headless/utilities/{juce_RangedAudioParameter.cpp, juce_AudioParameterFloat.h/.cpp}`
- Repo: `scripts/param-dump/{main.cpp, ParamDump.cmake, README.md}`, `build/build.ninja` (param-dump link rule), `modules/registry.yaml:138-140`, `modules/persistence/preset-manager/cpp/OuariconPresetManager.h:45`, `.claude/skills/plugin-testing/references/pluginval-guide.md:50-57`
- Disk: `~/Library/O-Strata/Presets/Factory/**` (192 JSON, `.factory-version` 1.0.0)

## Metadata

- Complexity: 5.0 (from STATUS); research path: single researcher (items were code lookups, no algorithm debate)
- Unverified: the pluginval lambda-range assumption in 2.4 (checked by the COMPAT-01 run itself)
- Next: `/plugin-plan O-Strata 1-foundation`
