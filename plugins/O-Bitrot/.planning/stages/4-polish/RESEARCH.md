# Stage 4: Polish — Research

**Date:** 2026-08-15
**Scope:** preset-manager v1.0.5 integration (band placement, native fns, factory bank), CHANGELOG + 1.0.0, automated validation gate, local install.
**Confidence:** HIGH — every claim verified against module source, O-Bitrot source, or the O-Tapestop stage-4 execution completed this week (same module version, same workflow, ALL GATES GREEN).

<user_constraints>
## User Constraints (from CONTEXT.md — binding, do not re-litigate)

- preset-manager **v1.0.5** via /module-add — fresh include-path integration, no vendored copy
- **~8 factory presets** covering the 6 families, sync + free clocking, extreme/combo patches
- Factory presets in **engineering units + convertTo0to1**; choices/bools as engineering values, not indices-as-floats
- applyPresetJson resets all 31 params first; AsyncUpdater cancelPendingUpdate() guard; FileChooser SafePointer / never `complete(false)` after teardown; "/" sanitization
- Harness **44/44** after ANY processor edit; native-fn grep-diff parity re-check
- Fixed **900×620** editor — band must not resize or crowd the 3×2 grid + Tab. VII strip
- Validation **automated only**: pluginval s10 VST3+AU ×2–3, auval, harness. Manual DAW checklist stays open, non-gating
- CHANGELOG.md; `VERSION 0.1.0 → 1.0.0` **at verify**. Local install only via build-and-install.sh. EB Garamond bundling declined
</user_constraints>

---

## Findings

### F1. Module surface — preset-manager v1.0.5 (VERIFIED against module source)

- **Files:** one header-only C++ class + one JS module. `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`, `modules/persistence/preset-manager/js/preset-manager.js` (`module.yaml:62-66`). Version 1.0.5 confirmed at `module.yaml:5` and `modules/registry.yaml:140`.
- **C++ API** (`OuariconPresetManager.h`): ctor `{apvts, pluginName}` (:87); `savePreset` (:354), `savePresetToFile` (:386, honors caller path), `loadPreset` (:406), `loadPresetFromFile` (:438), `deletePreset` (:460), `getPresetList` (:487), `getCurrentPresetName`/`setCurrentPresetName` (:137-140), `isFactoryPreset` (:264), `getNextPreset`/`getPreviousPreset` (:517/:536), `getUserPresetsDirectory` (:259), `initializeFactoryPresets` (:609), `setCustomStateCallbacks` (:100). Requires only `juce_core` + `juce_audio_processors` + `juce_data_structures` (`module.yaml:55-58`) — harness-safe, no WebView types.
- **Preset location:** `~/Library/{pluginName}/Presets/{Factory,User}/*.json` (:245-262). Pass a **literal** name so dev/release variants share one library — use `"Ouaricon Bitrot"` (O-Tapestop precedent: `plugins/O-Tapestop/Source/PluginProcessor.h:102` uses `"Ouaricon Tapestop"`).
- **Native functions the JS requests — 10, not the 9 in module.yaml:** `savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile, getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset, deletePreset, isFactoryPreset` (`preset-manager.js:108-117`). `savePresetWithDialog` is undocumented in the README example but is what the Save button calls (`preset-manager.js:130-131 → :273`). All 10 must be registered or the bridge-gap pattern fires.
- **JS constructor options** (`preset-manager.js:57-72`): `displayElement, prevButton, nextButton, saveButton, loadButton, deleteButton, onConfirmDelete, getNativeFunction`. **`getNativeFunction: Juce.getNativeFunction` is mandatory** — omitted → single console error and a silently inert band (`:119`).

### F2. Choice/Bool serialization — native, no adapter code needed (VERIFIED)

Preset JSON stores **normalized 0..1 floats for every parameter type**: `createPresetJson()` writes `paramWithID->getValue()` for each `RangedAudioParameter` (`OuariconPresetManager.h:276-283`) and `applyPresetJson()` restores via `setValueNotifyingHost(prop.value)` (`:334-341`). `AudioParameterChoice` (5 in O-Bitrot) round-trips as `index/(N−1)`; `AudioParameterBool` (7) as 0.0/1.0; `AudioParameterInt` SEED as `value/9999`. No adapter needed anywhere.

The CONTEXT constraint "store engineering values" is satisfied at the **authoring layer**: `FactoryPresetDef::parameters` holds normalized values verbatim (`:189` — "Parameter ID -> normalized value"), so factory defs are authored in engineering units and batch-converted once — see F6. User saves are always correct automatically (they serialize live normalized values).

### F3. CMake / binary-data wiring — module JS lands exactly where O-Bitrot's UI root is (VERIFIED)

- `ouaricon_add_module(OBitrot preset-manager)` adds the module `cpp/` dir to the target include path and **configure-copies the JS to `Source/ui/public/modules/`** on every configure (`modules/cmake/OuariconModules.cmake:104-116`). O-Bitrot's UI root **is** `Source/ui/public/` (find: index.html, img/, js/juce/ all under `public/`) — so the auto-copy location is canonical; do **not** relocate the file (O-Tapestop's plan tried and reverted: `O-Tapestop/.planning/stages/4-polish/SUMMARY.md` Task 1 deviation — a relocated copy is re-created every configure).
- `include(OuariconModules.cmake)` already exists at `plugins/O-Bitrot/CMakeLists.txt:2`. Place `ouaricon_add_module` after `juce_generate_juce_header(OBitrot)` (:92), before the binary-data target (O-Tapestop precedent `CMakeLists.txt:61`).
- **No second `juce_add_binary_data` target.** Add `Source/ui/public/modules/preset-manager.js` to the existing `OBitrot_UIResources` target (`CMakeLists.txt:95-102`). That target has no `NAMESPACE` argument → default namespace `BinaryData`. Hyphen strips: the symbol is **`BinaryData::presetmanager_js`** (no collision with the existing `index_js`/`check_native_interop_js` symbols). Dual-target NAMESPACE collision trap thereby avoided by construction.
- **Resource provider route:** `getResource()` matches **bare paths** (`PluginEditor.cpp:224-272`, comment at :227-228). Add one route: `if (url == "/modules/preset-manager.js") return { makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize), "application/javascript" };` — mirrors O-Tapestop SUMMARY Task 5.
- **Registry:** after wiring, run `scripts/regen-registry-used-by.sh` (exists, executable; O-Tapestop used it — SUMMARY Task 1) rather than hand-editing `registry.yaml`.

### F4. Preset-band placement in the 900×620 frame — header center, zero vertical cost (MEASURED)

Unlike O-Tapestop (whose stage 3 pre-authored a disabled band), **O-Bitrot's index.html has no band markup** — stage 4 must add it. Height budget (all from `Source/ui/public/index.html`):

| Element | px | Evidence |
|---|---|---|
| frame border | 3 top + 3 bottom | `.plugin` :45 |
| `.inner` padding | 14 top + 14 bottom | :78 |
| header | 46 + 8 margin | `.hdr` :86-87, :94 |
| 3×2 grid | flex 1 1 auto (**≈ 422 → panels ≈ 206/row**) | `.grid` :127-131 |
| global strip | 100 + 10 margin | `.global` :341-342 |

The grid has **no slack** — inserting a Tapestop-style band row (32 px + 12 px) would shave ~22 px off each panel row and crowd the 58 px knobs. Instead: the header is `display:flex; justify-content:space-between; align-items:flex-end` (:88-90) with only the wordmark (~190 px) left and `.hdr-right` (~230 px) right — **≈ 400 px of empty center at 866 px interior width**. Place a compact band (~26-28 px tall, six elements: `◀ ▶ [name] Save Load Del`) as a third flex child centered in the header. Zero change to grid/global geometry; frame stays 900×620.

Markup/IDs: copy O-Tapestop's band verbatim in structure (`O-Tapestop/Source/ui/public/index.html:56-66`) — IDs `preset-prev`, `preset-next`, `preset-name`, `preset-save`, `preset-load`, `preset-delete`; controls ship `disabled`; button copy in `data-label`/`data-confirm` attributes, never JS literals; `#preset-name` stays childless (`_updateDisplay()` writes `textContent`). Style in the Naturalist idiom (brown borders, small-caps). The README's dropdown CSS (`README.md:209-264`) is **optional** — O-Tapestop shipped without a dropdown; recommend the same (prev/next + dialogs cover browse).

### F5. Editor integration — 0 → 10 native functions (VERIFIED against current source)

O-Bitrot's editor currently registers **zero** native functions (`PluginEditor.cpp:66` comment "NO native functions in this plugin"; stage-3 parity gate 0↔0 at `3-gui/VERIFICATION.md:73`). Stage 4 adds exactly the 10 module fns — **copy O-Tapestop `PluginEditor.cpp:231-366` in shape verbatim** (fresh, gate-green this week):

- Simple fns (`savePreset`, `loadPreset`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`): thin `complete(juce::var(...))` wrappers.
- Dialog fns (`savePresetWithDialog`, `loadPresetFromFile`): `std::shared_ptr<juce::FileChooser>` captured into its own callback; `juce::Component::SafePointer<OBitrotAudioProcessorEditor> safeThis(this)` **hoisted to a local** (MSVC init-capture trap); on dead editor `return` — **never `complete(false)`** (UAF on the dead WebView impl); completion is a **`{success, name}` DynamicObject** — the JS reads `result.success`/`result.name` (`preset-manager.js:273-275, :296-297`); a bare bool reads as failure. `savePresetWithDialog` writes via `savePresetToFile()` (honors chosen path — the O-DigiDelay bug, comment at Tapestop `:261-263`).
- Options chaining: `.withNativeFunction(...)` calls fold into the existing `Options{}` chain before/among the 31 `.withOptionsFrom` calls (order irrelevant; Tapestop builds `options` incrementally).

Parity gate widens: grep-diff `withNativeFunction` (C++) ↔ `getNativeFunction("...")` (JS, excluding the vendored `js/juce/index.js` library per stage-3 note `VERIFICATION.md:86`) must be **10↔10 both directions**.

### F6. Factory-preset authoring pipeline (VERIFIED, O-Tapestop-proven)

Author in **engineering units**, then batch-convert through each param's own `NormalisableRange` once, in the processor constructor, before `initializeFactoryPresets` — O-Tapestop `PluginProcessor.cpp:232-242`:

```cpp
for (auto& preset : factoryPresets)
    for (auto& [paramId, value] : preset.parameters)
        if (auto* p = apvts.getParameter(paramId))
            value = p->convertTo0to1(value);
presetManager.initializeFactoryPresets(factoryPresets);
```

- Skewed ranges this protects: `CLOCK_FREE_RATE` (`setSkewForCentre(1.414f)`, `PluginProcessor.cpp:62-63`) and `CRUSH_RATE` (`setSkewForCentre(3162.0f)`, `:271-272`). O-Tapestop verified on disk that the skew is honored (4000 ms → 0.7842, not 0.5 — SUMMARY Task 3).
- Choices authored as the **index in engineering units** (`convertTo0to1(index)` over the 0..N−1 range); bools as 0/1; SEED as the integer 0-9999.
- **Every preset lists all 31 param IDs** (defense in depth; module WR-01 reset covers omissions but explicit is auditable).
- **No `customState`** — O-Bitrot has no custom state: SEED is an APVTS param, session state is plain APVTS XML (`PluginProcessor.cpp:653-666`), and `setCustomStateCallbacks` is simply not called. The Tapestop "every preset must carry customState" rule does **not** apply here. `FactoryPresetDef::customState = juce::var()` throughout.
- Sentinel gating (`.factory-version`, `OuariconPresetManager.h:614-623`): factory files (re)write only when `JucePlugin_VersionString` changes. The 0.1.0 → 1.0.0 bump at verify regenerates the bank automatically. **Dev note for plan:** while iterating on factory defs at 0.1.0, delete `~/Library/Ouaricon Bitrot/Presets/Factory/.factory-version` (or the whole Factory dir) to force a rewrite.
- Preset version stamp is `JucePlugin_VersionString` (IN-02, `:651`) — correct automatically post-bump.

### F7. Processor-side integration is minimal; session state untouched (VERIFIED)

- Add `#include <OuariconPresetManager.h>` + public member `OuariconPresetManager presetManager { apvts, "Ouaricon Bitrot" };` declared **after** `apvts` (`PluginProcessor.h:77` — apvts is public; member init order requirement, Tapestop precedent `PluginProcessor.h:102`).
- `initializeFactoryPresets(...)` once at the end of the constructor (after param-pointer caching, `PluginProcessor.cpp:315-360`).
- **Keep `getStateInformation`/`setStateInformation` exactly as-is** (`PluginProcessor.cpp:653-666`). Do not adopt the module's `getStateAsXml`/`setStateFromXml` — changes session format for zero benefit; module preset fns are independent of its session fns (Tapestop decision, RESEARCH + SUMMARY Task 2, round-trip proven byte-identical).
- **Seed determinism contract holds with zero new code:** preset apply is `setValueNotifyingHost` on the message thread; `processBlock` detects the SEED change at the block boundary and reseeds all 8 RNG streams (`PluginProcessor.cpp:461-468`; `lastSeed` at `PluginProcessor.h:165`). Prepare also reseeds (`:400-401`). Harness must re-confirm bit-identity per seed after the edits (existing 44 probes cover this).

### F8. WebView/JS wiring — inline script, not app.js (VERIFIED)

O-Bitrot's UI JS is one inline `<script type="module">` in index.html (:729-931) that already does `import * as Juce from "./js/juce/index.js"` (:732). Integration:

- `import { PresetManager } from "./modules/preset-manager.js"` at the top of the inline module script.
- Construct with **explicit DOM refs for all six elements** + `getNativeFunction: Juce.getNativeFunction`. **Never `createPresetBar()`** — it `innerHTML`-wipes the container and creates no delete button (Tapestop RESEARCH Q2 corollary).
- Call `initialize()` at the end of the existing init flow (after bindings, view visible) — the module awaits every native call with no timeout, and completions are **dropped while the view is hidden** (repo memory) — init-at-visible avoids the hang. `_waitForNative()` polls `window.__JUCE__.backend` (bounded 100×50 ms, IN-04) — already populated (:917 uses it for `ledUpdate`).
- Ship the six controls `disabled`; un-disable only after `initialize()` resolves (Tapestop refinement, SUMMARY Task 5 — a dead bridge leaves the band honestly disabled).
- Delete confirm via **`onConfirmDelete`** returning the two-click `data-armed` pattern (2.5 s auto-disarm) — never the module's `window.confirm` fallback (unreliable in JUCE WebView; module aborts fail-safe, button appears dead).

### F9. Harness compatibility pre-verified (VERIFIED)

`tests/render-harness/CMakeLists.txt` compiles `../../Source/PluginProcessor.cpp` (:23), inherits `$<TARGET_PROPERTY:OBitrot,INCLUDE_DIRECTORIES>` (:34) — so the module include dir added by `ouaricon_add_module` flows to the harness automatically — and defines `JucePlugin_VersionString` (:81), which the module header references in `createPresetJson`/sentinel. `createEditor` is already `#if JUCE_WEB_BROWSER`-guarded with a Generic fallback (`PluginProcessor.cpp:644-650`). Expected harness delta: none in DSP; re-run required because the processor TU gains the member + factory bank.

### F10. Version / CHANGELOG mechanics (VERIFIED)

- Version lives at `plugins/O-Bitrot/CMakeLists.txt:11` — `VERSION 0.1.0` inside `juce_add_plugin` (the correct keyword; `PLUGIN_VERSION` is the known ignored-keyword trap). Bump to `1.0.0` at verify; this also regenerates the factory sentinel + preset version stamps (F6).
- CHANGELOG template: `plugins/O-Tapestop/CHANGELOG.md` — `# Changelog — O-Bitrot`, `## [1.0.0] — <date>`, "Initial release.", `### Added` bullets per feature family (DSP families, seeded determinism, WebView UI, preset system, factory bank). Date finalized at verify.

---

## Module Reuse

| What | Source | How |
|---|---|---|
| preset-manager v1.0.5 | `modules/persistence/preset-manager/` | `ouaricon_add_module(OBitrot preset-manager)` — include path, never vendored |
| Native-fn block (all 10) | `O-Tapestop/Source/PluginEditor.cpp:231-366` | copy shape verbatim, rename types |
| convertTo0to1 batch loop | `O-Tapestop/Source/PluginProcessor.cpp:232-242` | copy |
| Band markup + disabled-until-init pattern | `O-Tapestop/Source/ui/public/index.html:56-66` + SUMMARY Task 5 | adapt IDs verbatim, restyle for O-Bitrot header |
| Two-click delete (`onConfirmDelete` + `data-armed`) | O-Tapestop app.js `armedConfirmDelete()` (SUMMARY Task 5) | adapt into inline script |
| Registry regen | `scripts/regen-registry-used-by.sh` | run after wiring |
| CHANGELOG format | `plugins/O-Tapestop/CHANGELOG.md` | template |

## Pitfalls / Gates (each verified against source this session)

| # | Gate | Status in O-Bitrot context | Evidence |
|---|---|---|---|
| G1 | applyPresetJson resets all params to defaults first | **Already module-side** (WR-01, v1.0.3; meta-first CR-03 v1.0.5 — no-op here, no meta params). Gate = use the registry module at 1.0.5, never a stale vendored copy | `OuariconPresetManager.h:315-341`; `module.yaml:74-86` |
| G2 | AsyncUpdater `cancelPendingUpdate()` | **N/A — verified absent**: zero `AsyncUpdater` matches in `plugins/O-Bitrot/Source/`. Record as checked, no task | grep this session |
| G3 | FileChooser: shared_ptr chooser + hoisted SafePointer + bare `return` on dead editor (never `complete(false)`) | Must implement in the 2 dialog fns | Tapestop `PluginEditor.cpp:246-254, :293-301` |
| G4 | "/" in preset names | **Module-side** `sanitizePresetName` (`/ \ :` → `_`) applied at save/load/delete/factory (WR-04 + IN-17). Residual gate: **no factory name may contain "/"** — e.g. never name a preset "33 1/3 Locked Groove" (it would silently become `33 1_3 …` on disk) | `OuariconPresetManager.h:218-221, :629-632` |
| G5 | Engineering units + convertTo0to1 (skewed CLOCK_FREE_RATE, CRUSH_RATE) | Batch loop in constructor; verify one skewed value on disk post-build (Tapestop did: skew honored) | F6; `PluginProcessor.cpp:62-63, :271-272` |
| G6 | Harness 44/44 after ANY processor edit | Re-run after the member/bank lands AND again at completion; harness inherits include dirs + VersionString already | F9 |
| G7 | Native-fn parity | 10↔10 both directions, excluding vendored `js/juce/index.js` matches | F5; `3-gui/VERIFICATION.md:73, :86` |
| G8 | Binary-data hyphen strip / dual-NAMESPACE collision | Single existing target `OBitrot_UIResources` (default `BinaryData` namespace); symbol `presetmanager_js`; **no second target** | F3; `CMakeLists.txt:95-104` |
| G9 | `createPresetBar()` forbidden | innerHTML-wipes container, no delete button | `preset-manager.js:419` region; Tapestop RESEARCH Q2 |
| G10 | Dialog fns return `{success, name}` objects | Bare bool reads as failure | `preset-manager.js:273-275, :296-297` |
| G11 | `getNativeFunction: Juce.getNativeFunction` in JS options | Omitted → silently inert band | `preset-manager.js:107-119` |
| G12 | Initialize when visible; buttons disabled until `initialize()` resolves | Completions dropped while hidden (repo memory); Tapestop refinement | F8 |
| G13 | Factory sentinel during dev | Editing factory defs at 0.1.0 needs sentinel/Factory-dir delete to rewrite; 1.0.0 bump regenerates | `OuariconPresetManager.h:614-623` |
| G14 | pluginval s10 ×2–3 VST3+AU, auval | Latent-NaN pattern; run after preset-apply path exists | CONTEXT; Tapestop gate table |
| G15 | Layout: band must not steal grid height | Header-center placement, zero vertical cost (F4). If any grid change becomes necessary, that is a plan-phase red flag | F4 measurements |

## Draft Factory Preset Bank (8 presets, engineering units — names slash-free per G4)

All 31 IDs authored per preset; table shows deltas from defaults (defaults: Sync · 1/8 · 2.0 Hz · SEED 0 · Hard Edges Off · MIX 100 · Tape/CD/Vinyl **On** 25/…, Packet/Codec/Crush **Off**). Enables not listed are set explicitly to the value shown in the Families column.

| # | Name | Character | Clock | Key settings (engineering units) |
|---|---|---|---|---|
| 1 | Worn Cassette | Tape showcase — wow, drag, occasional full stop | **Free 1.2 Hz** | Tape ON: TAPE_PROB 45, TAPE_STOP_PROB 12, TAPE_RAMP 260 ms · CD/Vinyl/Packet/Codec/Crush OFF · SEED 1111 |
| 2 | Skipping Disc | CD showcase — machine-gun buffer loops, restart chirps | **Sync 1/16** | CD ON: CD_PROB 55, CD_SEVERITY 0.8, CD_SEGMENT 45 ms · all others OFF · SEED 2222 |
| 3 | Locked Groove | Vinyl showcase — revolution jumps, heavy pops, locked-groove holds | **Free 0.4 Hz** | Vinyl ON: VINYL_PROB 40, VINYL_RPM 33 1/3 (index 0), VINYL_POP 70 · all others OFF · SEED 3333 |
| 4 | Dropped Call | Packet showcase — bursty robotic loss | **Sync 1/8** | Packet ON: PACKET_LOSS 45, PACKET_BURST 65, PACKET_CONCEAL Repeat (index 1) · all others OFF · SEED 4444 |
| 5 | Cellphone 1998 | Codec showcase — GSM crunch with a whiff of loss | **Sync 1/4** | Codec ON: CODEC_MODE GSM (index 1), CODEC_MIX 100 · Packet ON light: PACKET_LOSS 12, PACKET_BURST 40, PACKET_CONCEAL Decay (index 2) · Tape/CD/Vinyl/Crush OFF · SEED 5555 |
| 6 | Eight-Bit Ruin | Crush showcase — quantize + SRR + jitter, ducking envelope | Sync 1/8 (clock moot) | Crush ON: CRUSH_BITS 6.0, CRUSH_RATE 11025 Hz, CRUSH_JITTER 15, CRUSH_ENV_AMT −35, CRUSH_DITHER 0.6 LSB · all others OFF · SEED 6666 |
| 7 | Total Media Failure | Extreme combo — everything failing at once, hard splices | **Sync 1/16 · HARD_EDGES On** | ALL six ON: TAPE 60/25/80 ms · CD 50/0.9/25 ms · VINYL 50/33 1/3/85 · PACKET 55/70/Silence (0) · CODEC Mu-law (0)/100 · CRUSH 4.0 bits/6000 Hz/40/0/1.0 LSB · SEED 7777 |
| 8 | Gentle Rot | Subtle physical-media patina — mixable default-plus | **Free 0.7 Hz · MIX 85** | Tape ON 12/8/300 ms · CD ON 10/0.25/120 ms · Vinyl ON 15/33 1/3/35 · Packet/Codec/Crush OFF · SEED 8888 |

Coverage check: 6 family showcases (1-6) ✓ · sync-clocked (2,4,5,7) + free-clocked (1,3,8) ✓ · extreme combo (7) + subtle combo (8) ✓ · HARD_EDGES exercised (7) ✓ · both CODEC modes across bank (5 GSM, 7 Mu-law) ✓ · skewed params exercised (CLOCK_FREE_RATE in 1/3/8, CRUSH_RATE in 6/7) ✓. Exact values are plan/execute-tunable by ear; names and family coverage are the contract.

## Open Items for Plan Phase

1. **Band visual design** — exact Naturalist styling of the header-center band (border treatment, fleuron separators, name-field width ~140 px). Functional contract (six IDs, disabled-until-init, data-attr copy) is fixed by this research.
2. **Preset-apply UX during audio** — a load fires 31 `setValueNotifyingHost` calls; family enables flip and the clock re-rolls next tick. Accept as-is (Tapestop precedent) unless the harness/listen check shows a click — HARD_EDGES-off crossfades already cover jumps.
3. **Factory value tuning by ear** at execute (table values are informed drafts; keep coverage matrix invariant).
4. **CHANGELOG bullet list** — draft at execute from BRIEF/stage summaries; date + 1.0.0 at verify.
5. **Order of operations** — recommended: (T1) CMake wiring + registry → (T2) processor member + bank → (T3) editor 10 fns + resource route → (T4) band markup/CSS/JS → (T5) harness + parity + probes → (T6) CHANGELOG → (T7 verify) version bump, pluginval ×3, auval, install.
