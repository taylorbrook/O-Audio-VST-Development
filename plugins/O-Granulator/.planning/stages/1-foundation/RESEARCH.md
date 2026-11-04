# O-Granulator Stage 1 (Foundation): Research

**Researched:** 2026-11-04
**Domain:** JUCE 8.0.15 plugin shell: 220-param APVTS, buses, state, plus a shared module extraction and an O-GrainScatter migration
**Confidence:** HIGH. Almost every claim was read in-repo this session. External docs were not needed: the JUCE behaviour was read from `/Users/taylorbrook/JUCE/modules` (8.0.15).
**Inputs read:** CONTEXT.md, parameter-spec.md (LOCKED), ROADMAP § Stage 1, ARCHITECTURE.md (grep), mockups/v2-CMakeLists.txt, v2-PluginEditor.{h,cpp} (scale API), v2-ui-test.html (JS reference parser), v2-ui.yaml, v2-integration-checklist.md, the stage-1 skill reference, MEMORY.md and the linked files cited below.

<user_constraints>
## User Constraints (from CONTEXT.md, verbatim excerpts)

### Locked Decisions
**D1: PitchCollections: extract AND migrate O-GrainScatter in Stage 1**
- Create shared module `modules/tuning/pitch-collections` from `plugins/O-GrainScatter/Source/dsp/PitchEngine.h` `namespace PitchCollections` (Id enum, names, `builtIns()` tables, `Table`, `parsePitch`, `tableFromPitches`, `tableFromText`, `tableFromScala`). Max 64 degrees (`kMaxPitches`).
- Module additions: `edo N [period]` shorthand (error `badEdo`), a `tableFromText` overload reporting the offending token, `toScala (name, text)`.
- Register in `modules/registry.yaml` + `module.yaml`; consumed via `ouaricon_add_module(... pitch-collections)` (uncomment the line in v2-CMakeLists).
- **Migrate O-GrainScatter** to consume the module in the same stage — delete the in-tree namespace, include the module header. Minor bump **3.5.1 → 3.6.0** (edo shorthand is user-visible), CHANGELOG `## [3.6.0]` entry.
- **GS regression gate (mandatory):** existing GS tests (blocksize-null, alias-gate, density-gain, adapt-gate, skip-tilt) still pass; a render null vs. pre-migration GS binary (seeded out-of-tree pair via `git archive`) is bit-identical for all built-in collections + Custom; custom-scale text/Scala parse results unchanged for existing inputs. No tag (only `/publish` tags).
- O-Granulator maps choice index → Id at the parameter boundary (0 Off, 1…5 → 0…4, 6…31 → 6…31; GS "Free" Id 5 dropped). Do NOT copy the file into `plugins/O-Granulator/Source/`.

**D2: Shell editor: GenericAudioProcessorEditor (Stages 1–2)**
- `createEditor()` returns `GenericAudioProcessorEditor` for now. Stage 3 swaps in `v2-PluginEditor`.
- WebView build plumbing still wired in Stage 1 (`NEEDS_WEB_BROWSER TRUE`, `juce_generate_juce_header`, binary-data target for `v2-ui.html` + i18n + preset-manager.js + eb-garamond assets) so Stage 3 is an editor swap, not a CMake rework. Windows WebView2 block … is a Stage 3 concern — leave the CMake defines in place from the scaffold.
- Render harness guards `createEditor` under `#if JUCE_WEB_BROWSER`.

**D3: Bus layouts: all three**
- mono→mono, mono→stereo, stereo→stereo. Default stereo→stereo. Mono input feeds both ring channels (Stage 2). Channel counts read from buses, never assumed.

### Claude's Discretion
None listed in CONTEXT.md. Implementation details below are recommendations.

### Deferred / Out of scope
DSP (Stage 2), WebView editor integration (Stage 3), presets/i18n (Stage 4).

### Stage 1 gate (CONTEXT.md test criteria)
VST3 + AU + Standalone build, auval PASS, pluginval 10 on VST3 and AU · bitwise pass-through at defaults with mix 0 % in all three layouts · 113-entry dest-table golden · 220 params with exact ID order, 140 Float / 59 Choice / 21 Bool · pitch-collections unit checks (edo, badEdo, offending token, toScala round-trip) · GS 3.6.0 regression gate green.
</user_constraints>

## Project Constraints (from CLAUDE.md + memory)
- Trunk-based on `main`. **Path-scoped commits only** (`git commit -- <paths>`). Re-run `git branch --show-current && git status --short` right before every commit. Use the temp-index commit (index_git_shared_checkout). Note: `plugins/O-GrainScatter/.planning/improvements/adaptive-grain-length/nielsen-dafx26.txt` is untracked, from another session. Do NOT sweep it into the GS commit.
- Build and install with `./scripts/build-and-install.sh <Name>`. It does the AU cache clear and sweeps both variants. Run builds, pluginval and cold auval with `run_in_background` (pattern_executor_watchdog_stall_run_long_commands_in_background).
- No tags (feedback_never_tag_unless_publish). CHANGELOG headings must be `## [X.Y.Z] - date` (pattern_changelog_heading_must_be_bracket_form_and_partial_fix_is_worse).
- The stage ends with a two-step handoff: `/clear`, then `/plugin-plan O-Granulator`.

## Summary

1. **Module = a verbatim move plus a namespace alias shim.** Copy `PitchEngine.h:41-305` into `modules/tuning/pitch-collections/cpp/PitchCollections.h` with every expression unchanged. Put it under `namespace ouaricon::tuning::PitchCollections` and include `<juce_core/juce_core.h>`. In GS, `PitchEngine.h` keeps only the engine plus `namespace PitchCollections = ouaricon::tuning::PitchCollections;`. All 20 GS call sites then compile unchanged, and so do the test drivers built against old `git archive` trees. v3.5.1 did exactly this for CounterRng and got 76/76 byte-identical renders.
2. **`tuning/` needs no CMake change.** `ouaricon_add_module` already searches `tuning`. The module must compile at **gnu++17**: GS and every processor-console target build at C++17.
3. **edo cannot change any existing parse result.** Every string the edo regex matches contains the letters `edo`, and the current `parsePitch` rejects letters. So each matching input used to return `badPitch`, and every input that parses today keeps its exact result. Proof by construction, plus the null gate.
4. **GS gate = the repo's own twin pattern.** A new `tests/pitch-null` driver is compiled twice: against the current tree, and against `git archive 0e1117b6` (v3.5.1) at configure time. It covers all 32 Ids (Free and Custom included), ladders, masks, custom text and Scala, and a parse corpus. A `cmp` of every dump must match; edo inputs act as the positive control. Also re-run the 5 existing gates (with their twins), the param-dump diff, pluginval 10 and auval. **GS UI work is needed too:** `app.js` silently clears unknown error codes, so `badEdo` needs a mapping and en/fr/zh-Hans rows.
5. **The CMake mockup is close to shippable.** It will not configure or build until `Source/ui/public/{index.html, js/i18n.js, js/juce/index.js, js/juce/check_native_interop.js}` and `Source/PluginEditor.cpp` exist. `cxx_std_20` is set on the plugin target only, so console targets get gnu++17; add the feature to each console. Root CMake auto-discovers plugins, so a broken configure blocks every concurrent session.
6. **Params:** generate IDs from frozen suffix arrays, but keep the 113-entry dest table as a **literal** append-only array. The golden data is generated from `parameter-spec.md` (plus `v2-ui.yaml` names) by a script, never from the code. One processor-console `O-Granulator-render-harness` holds every Stage 1 gate.
7. **State:** write the non-parameter children onto the `copyState()` copy, never onto the live tree. Use `<Engine seed>` = `String((int64) seed)`, masks as 16-hex strings with only non-default ones written, bools and ints read through `isVoid()` + var conversion, and no `id` attribute on custom children (APVTS matches children by `id`). Test with two reopens.
8. **Spec gap:** only 112 of 220 params have a host-facing `name:` in v2-ui.yaml (the 112 mod destinations). The other 108 need a naming convention before coding (open question O1).

## Architectural Responsibility Map

| Capability | Primary tier | Secondary | Rationale |
|---|---|---|---|
| Pitch tables / parsing / toScala | Shared module (header-only, message thread) | GS + Granulator processors | One implementation, two consumers. |
| Param IDs, layout, dest table | Plugin `Source/GranulatorParamIds.h` + `PluginProcessor.cpp` | render-harness goldens | The contract lives in one header. |
| Bus negotiation / pass-through | Processor (`isBusesLayoutSupported`, `processBlock`) | — | AU channel configs derive from the predicate. |
| Session state (seed, scale, masks, uiLanguage) | Processor get/setStateInformation | preset-manager (Stage 4) | Stage 1 owns the codec. |
| Validation | Console harness (processor TUs only) + pluginval/auval on installed binaries | — | Headless: no WebView, no binary data. |

---

## 1. PitchCollections extraction

### 1a. What moves vs what stays (`plugins/O-GrainScatter/Source/dsp/PitchEngine.h`, 459 lines) [VERIFIED: read in full]

| Lines | Item | Disposition |
|---|---|---|
| 22-26 | `#include <JuceHeader.h>`, `<array>`, `<cmath>`, `<cstdint>`, `"GrainScatterRng.h"` | Stays in GS (the engine needs `GrainScatterRng.h` → `CounterRng::Draw`). The module includes `<juce_core/juce_core.h>`, `<array>`, `<cmath>`, `<cstdint>`, `<initializer_list>`, `<utility>`, `<regex>`. |
| 41-305 | `namespace PitchCollections { … }` | **Moves verbatim.** |
| 43 | `static constexpr int kMaxPitches = 64;` | moves |
| 48-60 | `enum Id { Chromatic = 0, Major, Minor, Pentatonic, WholeTone, Free, Dorian, … MaqamRast, Custom, kCount };` | moves. The order is GS's saved-session contract: `Free` = 5, `Custom` = 31, `kCount` = 32. |
| 63-74 | `names()`: GS host strings, including `"Free (unquantized)"` | moves verbatim. Document it as **frozen**, because GS's `AudioParameterChoice` uses it (`PluginProcessor.cpp:128`). |
| 76-81 | `struct Table { std::array<float, kMaxPitches> cents {}; int count = 1; float period = 1200.0f; };` | moves |
| 83-117 | `ratioToCents`, `fromCents`, `equalDivision`, `fromRatios` | move (used by `builtIns` and GS `:1345`) |
| 122-165 | `builtIns()`: function-local static, built off the audio thread | moves. GS warms it in its ctor (`PluginProcessor.cpp:86`); Granulator must do the same. |
| 179-204 | `parsePitch (juce::String token, float&)` | moves verbatim |
| 209-235 | `tableFromPitches`: errors `"noPitches"`, `"badPeriod"` (`!(period > 1.0f) \|\| period > 4800.0f`), `"tooMany"` | moves verbatim. **Do not touch it** (dedup is `addIfNotAlreadyThere`, an exact compare). |
| 238-252 | `tableFromText (text, out, error)`: tokens split on `",;\n\r\t"` → spaces, error `"badPitch"` | becomes a thin wrapper over the new 4-arg overload |
| 256-304 | `tableFromScala (scl, out, asText, description, error)`: errors `"badCount"`, `"badPitch"`, `.scl` rule (bare int → `"/1"`) | moves verbatim |
| 307-459 | `class PitchEngine` (uses `PitchCollections::Table`/`Free`/`Custom`/`builtIns`/`kCount` at 332, 341, 389, 398-401) | **stays in GS** |

JUCE dependencies used by the namespace: `juce::String`, `StringArray`, `Array<float>`, `jlimit`. That means juce_core only.

### 1b. Every GS call site [VERIFIED: grep plus reads]
- `Source/PluginProcessor.h:27` `#include "dsp/PitchEngine.h"`; `:139` `PitchCollections::Table getCustomTable() const;`; `:246` `std::array<std::atomic<uint64_t>, PitchCollections::kCount> pitchMasks;`; `:248`, `:251` `PitchCollections::Table` members.
- `Source/PluginProcessor.cpp:86` builtIns warm-up; `:128` `names()`; `:643` collection clamp; `:1206`, `:1244` mask save/load loops; `:1345` `equalDivision (12)`; `:1355-1360`, `:1371`, `:1377` mask helpers (`kMaxPitches`); `:1388-1390` `tableFromText`; `:1400` `Custom`; `:1409-1411` `tableFromScala`; `:1417` `getCustomTable`.
- `Source/PluginEditor.cpp:418-421` builtIns/Custom loop.
- Tests: **none** reference `PitchCollections` or set `"scale"`. All 5 gates run at the default collection 0, so they do NOT cover the move by themselves. That is why a new driver is needed (§2).

### 1c. Module system survey [VERIFIED]
- `modules/cmake/OuariconModules.cmake:30-145` `ouaricon_add_module`. `:34` reads `set(MODULE_CATEGORIES core persistence metering tuning modulation synthesis effects ui)`, so **`tuning/` is already searched**. A module is found when `<cat>/<name>/module.yaml` exists (`:37-43`). `cpp/*.cpp` and `cpp/*.h` are globbed into target sources, and `cpp/` goes on the PRIVATE include path (`:57-67`). `js/*.js` is copied into `Source/ui/public/modules/` (gitignored, `:104-118`). The optional `module.cmake` hook is at `:120-124`. A header-only module needs nothing else.
- Existing tuning modules: `note-expression` (namespace `Ouaricon::NoteExpression`), `scala-tuning-engine` (global classes `TuningEngine`, `ScaleGenerator`; no filename clash with `PitchCollections.h`, and no `PitchCollections.h` exists anywhere in `plugins/`).
- Header-only reference: `modules/synthesis/grain-engine/`, with `module.yaml`, `README.md`, `cpp/*.h`, `tests/CMakeLists.txt` and `tests/grain_engine_tests.cpp`. The test is a standalone `juce_add_console_app` linking only `juce::juce_core`, configured on its own with `cmake -S modules/synthesis/grain-engine/tests -B build-grain-engine-tests -G Ninja`. "Exit 0 iff every check passes." Namespace `ouaricon::grain`, includes `<juce_core/juce_core.h>`.
- Registry: `modules/registry.yaml:12-14` reads `version: 1.2.18` / `last_updated: 2026-11-03` / `schema_version: 1.0.0`, and the header comment says these "MUST be bumped on every edit". Entries follow the `grain-engine` block at `:445-492` (name, path, version, description, category, origin, provides, dependencies, tags, reuse_score, used_by). `scripts/regen-registry-used-by.sh` rebuilds `used_by` from `#include` and `ouaricon_add_module` references, so run it after both consumers are wired.

### 1d. Module design (recommended)

```
modules/tuning/pitch-collections/
  module.yaml         # name, version 1.0.0, category tuning, origin O-GrainScatter, requirements juce_core, cpp_standard 17
  README.md           # API, error codes, frozen Id/names contract, edo grammar
  cpp/PitchCollections.h
  tests/CMakeLists.txt            # clone grain-engine's; CMAKE_CXX_STANDARD 17 (lowest consumer)
  tests/pitch_collections_tests.cpp
```

The namespace is `ouaricon::tuning::PitchCollections`. The GS shim, mirroring `dsp/GrainScatterRng.h` from v3.5.1:
```cpp
// PitchEngine.h (GS 3.6.0)
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstdint>
#include "GrainScatterRng.h"
#include "PitchCollections.h"   // pitch-collections 1.0.0 (modules/tuning/pitch-collections/cpp)
namespace PitchCollections = ouaricon::tuning::PitchCollections;   // call sites + old-tree drivers unchanged
class PitchEngine { /* unchanged 307-459 */ };
```

**Additions:** append them. Leave every existing body byte-identical.

1. `tableFromText (text, out, error, badToken)`, the new primary overload:
   ```cpp
   inline bool tableFromText (const juce::String& text, Table& out, juce::String& error, juce::String& badToken)
   {
       badToken = {};
       if (tryEdo (text, out, error, badToken)) return error.isEmpty();   // returns true iff text IS edo-shaped
       /* lines 240-251 verbatim, except: on parsePitch failure set badToken = t before error = "badPitch" */
   }
   inline bool tableFromText (const juce::String& text, Table& out, juce::String& error)
   { juce::String unused; return tableFromText (text, out, error, unused); }   // GS keeps calling this
   ```
2. `edo N [period]`. Port the JS reference (`v2-ui-test.html` `tableFromText`) exactly. Granulator's page shows the same errors.
   - Grammar: `std::regex (R"(^(?:edo\s*(\d+)|(\d+)\s*-?\s*edo)(?:\s+(\S+))?$)", ECMAScript | icase)`, applied to `text.trim()`. This accepts `edo 19`, `EDO19`, `19-edo`, `19 edo`, `edo 13 3/1`. Make the regex a function-local `static const` (message thread only).
   - Order (JS parity): period token present and `parsePitch` fails → `badPitch` with `badToken = m[3]`. Then period not in (1, 4800] → `badPeriod`. Then N not in 1…64 → `badEdo`. Before converting N, treat a digit string longer than 3 characters as `badEdo` (`getIntValue` overflow).
   - Build with **`equalDivision (n, period)`**, not `tableFromPitches`. That makes `edo 19` ≡ `builtIns()[Edo19]` and `edo 13 3/1` ≡ `BohlenPierce` bitwise, which is a free unit check. The default period is `1200.0f`.
   - Non-matching text falls through to the token path, so `edo`, `edo 12.5` and `edo 19, 1200` fail with `badPitch` and token `"edo"`, matching the JS.
   - **Why existing results stay byte-identical:** the regex requires `edo`. `parsePitch` rejects any token outside `"+-.0123456789"` or a digits/digits ratio, so every matching text previously failed with `badPitch`. No previously-valid input can reach the new branch. `tableFromScala` is untouched; edo applies to text only.
3. `toScala (name, text) -> juce::String`. It returns `{}` if `text` does not parse.
   - Header: `! <File::createLegalFileName(name or "custom")>.scl`, then `!`, then the description line (`name`, with newlines → spaces and a leading `!` stripped, or `"Ouaricon custom scale"` when empty), then ` <count-1+1>` (degree lines + period), then `!`.
   - Lines: the degrees 1…count-1 in table order, then the period.
   - Per degree, emit the user's token when its folded value equals the parsed value exactly; otherwise emit cents. For the edo form, emit cents for every degree and the period token (or `2/1`).
   - A token with no `.` and no `/` (bare-integer cents) gets `.0` appended. The `.scl` rule would otherwise read it as a ratio, the very bug fixed at `PitchEngine.h:170-177`.
   - **Cents formatting must round-trip the float exactly.** `juce::String (double)` uses ostream precision 6 [VERIFIED: `juce_String.cpp:481-496`], which is not enough. Use the shortest fixed p in 1…10 where `(float) String (double (c), p).getDoubleValue() == c`. p = 10 always works for c in (0.01, 4800). Do not use `std::to_chars` (float overloads are unavailable at deployment target 10.13, root `CMakeLists.txt:5`). Do not use `snprintf` (locale-dependent decimal separator).
   - Round-trip contract (unit test): `tableFromScala (toScala (n, t))` gives a Table bitwise equal to `tableFromText (t)` (count, period bits, cents[0…count) bits), and description == the sanitized name.

**Granulator boundary mapping** (plugin code, not module):
```cpp
// −1 = Off. static_assert (PitchCollections::Custom == 31 && PitchCollections::WholeTone == 4 && PitchCollections::Free == 5);
inline int scaleChoiceToId (int c) noexcept { return c <= 0 ? -1 : (c <= 5 ? c - 1 : c); }
```
Golden cross-check: for c in 1…31, `granulatorScaleNames[c] == PitchCollections::names()[scaleChoiceToId (c)]`. All 31 match the spec list (`parameter-spec.md:162-193`).

### 1e. Module unit checks (tests/pitch_collections_tests.cpp, exit 0 iff all pass)
- U1 builtIns structure: 32 tables, cents[0] == 0, ascending, count ≤ 64, BP period == `ratioToCents (3, 1)`.
- U2 edo equivalences, bitwise: `edo 19`/`19-edo`/`EDO 19` ≡ Edo19; `edo 12` ≡ Chromatic ≡ `tableFromText ("100 200 … 1200")`; `edo 13 3/1` ≡ BohlenPierce; `edo 64` → count 64.
- U3 errors: `edo 0`, `edo 65`, `edo 99999999999` → `badEdo`; `edo 19 abc` → `badPitch` + token `abc`; `edo 19 5000` → `badPeriod`; `edo` and `edo 12.5` → `badPitch` + token `edo`.
- U4 offending token: `"100 2x0 1200"` → `badPitch`, token `2x0`. The 3-arg and 4-arg overloads give identical Table and error on a corpus.
- U5 regression corpus: expected outputs for ~30 valid and invalid texts, captured from **v3.5.1** code before the move. Simplest way: the GS pitch-null twin dumps them (§2). Paste into the test as hex literals.
- U6 toScala round-trip over the corpus: cents, ratios, folding (`1300`, `-100`), duplicates, 64 degrees, edo forms, a name with `!` and newline.

---

## 2. GS migration + regression gate

### 2a. Existing GS gates [VERIFIED: `plugins/O-GrainScatter/CMakeLists.txt:87-215`]
All are `ouaricon_add_processor_console` targets under `option(OUARICON_BUILD_TESTS …)`. There is no CTest: you build the target and run the binary, and exit 0 = all rows PASS. Binaries land in `build/plugins/O-GrainScatter/O-GrainScatter-<suffix>_artefacts/Release/O-GrainScatter-<suffix>`. The shared `build/` already has `OUARICON_BUILD_TESTS:BOOL=ON`.

| Gate | Run recipe | Twin / control |
|---|---|---|
| blocksize-null | `O-GrainScatter-blocksize-null` | scheduler baseline from `backups/O-GrainScatter/v3.0.5` if present |
| density-gain | `…-density-gain-baseline <dir> && …-density-gain <dir>` (also `-compoff`) | v3.1.0 backup |
| alias-gate | `…-alias-gate`; `…-alias-gate-v320` must FAIL rows 1, 2, 4 | `git archive 9c6c22d0` at configure (`:140-155`) |
| skip-tilt | `…-skip-tilt-v330 <dir> && …-skip-tilt <dir>` | `git archive d920bedd` |
| adapt-gate | `…-adapt-gate-v340 <dir> && …-adapt-gate <dir> && …-adapt-gate-fixedk` (fixedk = negative control) | `git archive 144a73f3` |

The old-tree twins keep compiling after the move: their own `dsp/PitchEngine.h` resolves first (quoted include, relative to the including file), and the extra module include dir is inert. Confirm with `ninja -t deps`, as v3.5.1 did.

### 2b. New gate: `tests/pitch-null` + `pitch-null-v351` twin
Precedent: the CounterRng move (CHANGELOG `## [3.5.1]`: "76 dumps are byte-identical"; CONVERGENCE.md step 5) and memory pattern_null_test_needs_seeded_out_of_tree_pair. GS is deterministic once seeded: the ctor seeds from `Random::getSystemRandom()` (`PluginProcessor.cpp:33`), and the public `setRngSeed()` (`PluginProcessor.h:111`) overrides it. Seed after any `setStateInformation`. Baseline = **`0e1117b6`** (last GS source commit, v3.5.1).

CMake, a copy of the alias-gate block:
```cmake
ouaricon_add_processor_console(OuariconGrainScatter ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/pitch-null/main.cpp pitch-null)
set(_gs_v351_root ${CMAKE_BINARY_DIR}/gs-v351-0e1117b6)
set(_gs_v351_src  ${_gs_v351_root}/plugins/O-GrainScatter/Source)
if(NOT EXISTS ${_gs_v351_src}/PluginProcessor.cpp)
    file(MAKE_DIRECTORY ${_gs_v351_root})
    execute_process(COMMAND git -C ${CMAKE_SOURCE_DIR} archive 0e1117b6 plugins/O-GrainScatter/Source
                    COMMAND tar -x -C ${_gs_v351_root})
endif()
if(EXISTS ${_gs_v351_src}/PluginProcessor.cpp)
    ouaricon_add_processor_console(OuariconGrainScatter ${_gs_v351_src}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/pitch-null/main.cpp pitch-null-v351)
    get_target_property(_gs_pn_srcs O-GrainScatter-pitch-null-v351 SOURCES)
    list(TRANSFORM _gs_pn_srcs REPLACE "^${CMAKE_CURRENT_SOURCE_DIR}/Source/" "${_gs_v351_src}/")
    set_property(TARGET O-GrainScatter-pitch-null-v351 PROPERTY SOURCES ${_gs_pn_srcs})
    target_compile_definitions(O-GrainScatter-pitch-null-v351 PRIVATE GS_PITCHNULL_V351=1)
endif()
```
Run: `O-GrainScatter-pitch-null-v351 <dir>` (dumps), then `O-GrainScatter-pitch-null <dir>` (re-renders and compares each file byte for byte; exit 0 iff all equal and the controls behave). Same shape as skip-tilt. The `git archive` extracts Source only (C++), so the UI-module-copy trap (pattern_git_archive_ui_tree_misses_gitignored_module_copies) does not apply. Add a guard anyway: the driver must not touch UI.

The driver uses only the 3.5.1 public API: `setRngSeed`, `setPitchMaskString`, `setCustomPitchesFromText`, `setCustomPitchesFromScala`, `getCustomTable`, and the params `scale`, `root_note`, `pitch_mode`, `pitch_direction`, `pitch_random`, `pitch_range`, `pitch_transpose`, `pitch_fine` [VERIFIED: ParameterID grep]. Jobs (2 s each, seeded noise plus a 220 Hz tone, stereo, blocks 512 and a seeded variable split):
- **R1:** every Id 0…31 (Free and Custom included) × 3 seeds, `pitch_random` 70, `pitch_range` 24, Random mode.
- **R2:** LadderUp/LadderDown/Pendulum × direction Up/Down × {Major, Edo53, BohlenPierce, JustSevenLimit, Harmonics8to16, Custom} × root 0/7.
- **R3:** masks: alternating, single-degree and all-zero strings on {Chromatic, Edo19, Edo53, Custom}.
- **R4:** custom text corpus: the default 12-EDO; `5/4 3/2 2/1`; `386.3 701.955 1200`; folding `100 1300 2400`; `-100 1200`; 64 degrees; period `3/1`. Plus 3 `.scl` bodies (comments, bare integer `2`, description).
- **P (parse corpus):** dump `error | count | period hex | cents hex` for valid and invalid inputs (`""`, `abc`, `100 5000`, 65 pitches, malformed scl) → must match.
- **Positive control:** `edo 19` and `19-edo` written to `parse-edo.txt`, which MUST differ (old `badPitch`, new success).
- **Seed liveness:** the 3 seeds are pairwise distinct per R1 row. Signal check: finite, RMS > −90 dBFS.

### 2c. Other GS gate items
- param-dump diff: build `O-GrainScatter-param-dump` **before** the edit and save the output to scratchpad; diff after, which must be empty. Do not compare against `.planning/params.tsv`, which is stale (41 rows; source has 44 IDs).
- pluginval 10 on VST3 and AU, plus `auval -v aufx OuGS OuDv` (subtype from `PLUGIN_CODE OuGS`, `CMakeLists.txt:8`).
- UI gates after the badEdo edit: `node scripts/check-i18n.js`, `i18n-fr-lint`, `i18n-zh-lint`, `check-ui-labels --plugin O-GrainScatter`. This is the same set as the 3.5.0 VERIFICATION.md.

### 2d. GS edits checklist
1. `Source/dsp/PitchEngine.h`: delete lines 41-305; add the module include and the alias.
2. `CMakeLists.txt`: `VERSION 3.5.1` (`:11`) → `3.6.0`; add `ouaricon_add_module(OuariconGrainScatter pitch-collections)` next to `:54`; add the pitch-null block.
3. UI: `app.js:626-633` `showError` falls through to **clearing** the error for unknown codes, so add `else if (code === 'badEdo') setLabel(errorEl, 'label.customErr.badEdo');`. In `i18n.js` near `:1296-1314`, add `label.customErr.badEdo` in en, fr (`reviewed: false`) and zh-Hans (`reviewed: 'mt'`). Optional: extend `label.customHelp` (`:1277`) and `badPitch` (`:1300`) to mention `edo 19`. Both need fr and zh too. GS does not need the offending-token overload.
4. `CHANGELOG.md`: `## [3.6.0] - 2026-11-0X`, with Added (edo shorthand + `badEdo` message), Changed (PitchCollections → module; no audio, state or parameter change), and Testing (the pitch-null counts).
5. `plugins/O-GrainScatter/.planning/STATUS.md` (`version: 3.5.1` line 7), the `PLUGINS.md:46` row, and `regen-registry-used-by.sh`.
6. MINOR bump, so no Logic AU I/O issue (I/O unchanged).

---

## 3. CMake review (`mockups/v2-CMakeLists.txt` vs shipped plugins)

| Item | Finding | Verdict |
|---|---|---|
| `PLUGIN_CODE OuGn` (`:29`) | No `OuGn` in any `plugins/*/CMakeLists.txt`; no duplicate codes repo-wide | OK. AU triple `aufx OuGn OuDv` (dev). |
| Target `OuariconGranulator` ≠ folder | `build-and-install.sh` resolves it via `scripts/resolve-target.sh`; consoles are named `O-Granulator-<suffix>` (`ParamDump.cmake:80-81`) | OK. Never assume target == folder (build_script_target_name_vs_folder). |
| `VERSION 0.1.0` | JUCE reads VERSION; PLUGIN_VERSION is ignored | OK |
| Module names/versions | grain-engine `version: 1.1.0`, reverb-unit `version: 1.1.0`, preset-manager `version: 1.1.1`, eb-garamond `version: 1.0.0` [VERIFIED: each module.yaml] | OK (the mockup's "preset-manager 1.1.x" comment is right) |
| `ouaricon_add_module(... reverb-unit)` | No plugin uses this call today (O-Marimba adds the include dir by hand). It works: `cpp/ReverbUnit.h` is header-only (`juce_audio_processors`, `juce_dsp`). It also copies `reverb-unit.js` into the gitignored `Source/ui/public/modules/` | OK. Do NOT call `ReverbUnit::addParameters` (it adds `rev_*` params → count ≠ 220). |
| `ouaricon_add_module(... preset-manager)` | Header only (`OuariconPresetManager.h`) plus a JS copy to the gitignored folder. O-ReverseDelay embeds the module JS directly instead | OK. Never commit `Source/ui/public/modules/*` (pattern_module_js_move_build_recopies_tracked_file). |
| `eb-garamond` | No cpp or js dirs → no-op except tracking; fonts `EBGaramond-{Regular,Italic,Bold}.woff2` + `css/eb-garamond.css` exist | OK |
| `pitch-collections` line (`:75`) | Commented out; FATAL_ERROR until the module's `module.yaml` exists | Uncomment only after T1 lands |
| Binary data (`:108-126`) | `NAMESPACE UIBinaryData` + `HEADER_NAME UIBinaryData.h` (critical_dual_binary_data_namespace_collision). Symbols strip hyphens: `presetmanager_js`, `ebgaramond_css` (critical_binary_data_strips_hyphens) | OK, **but the 4 local SOURCES must exist**. Copy `v2-ui.html` → `Source/ui/public/index.html` and `v2-i18n.js` → `js/i18n.js`. JUCE's `index.js` and `check_native_interop.js` are byte-identical to GS's copies (`cmp`), so copy them from `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/` |
| `target_sources … Source/PluginEditor.cpp` (`:46-50`) | Required to exist | Add a placeholder `PluginEditor.h/.cpp` (comment-only TU) so Stage 3 swaps file contents, not CMake. Consoles skip any file matching `PluginEditor` (`ParamDump.cmake:156-159`) |
| `cxx_std_20` (`:41`) | PRIVATE on the plugin target only. `build.ninja` shows GS and all its consoles at `-std=gnu++17` (5656 vs 36 C++20 objects) | **Trap:** `ouaricon_add_processor_console` does not propagate it. Add `target_compile_features(O-Granulator-<suffix> PRIVATE cxx_std_20)` after each console call. Keep the module C++17-clean. |
| `NEEDS_WEB_BROWSER TRUE` / `NEEDS_WEBVIEW2 TRUE` / `JUCE_WEB_BROWSER=1` / `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` | Matches GS and RD | OK (memory: VST3 needs NEEDS_WEB_BROWSER) |
| `Standalone` in FORMATS | GS and RD do the same | OK. Standalone with a 3–7 channel device → layout rejection; loop by `buffer.getNumChannels()`. |
| `ParamDump.cmake` include (`:139-143`) | Exists, defines both helpers | OK. Add the render-harness console in the same block. |

**Configure blast radius:** root `CMakeLists.txt:47-58` adds every `plugins/*` that has a CMakeLists. A broken O-Granulator configure, or a GS `ouaricon_add_module(pitch-collections)` added before the module exists, fails `cmake` for every concurrent session. Land the module first, then consumers, and configure before committing.

**UI files under Source/ui/public bring the plugin into repo-wide UI gates** (`check-i18n` scans plugins). The mockup was pre-verified on a scratch tree, but run `node scripts/check-i18n.js` once after copying so a red gate is not inherited by Stage 3.

---

## 4. 220 APVTS params

### 4a. ID generation (`Source/GranulatorParamIds.h`)
The repo pattern is O-Strata `Source/StrataParamIds.h` (suffix arrays + `prefix + suffix` in loops, `juce::StringArray` builders). Recommended shape:
```cpp
namespace GranulatorParamIds {
inline constexpr int kStreams = 4, kLfos = 4, kMacros = 4, kModSlots = 16, kModDests = 113;
// FROZEN registration order — parameter-spec.md "Full ID list". Never re-order; append nothing (contract).
inline constexpr const char* kGlobal[] = { "input_gain","buffer_length","freeze","sync_mode","mix","output_gain","scale_root","grain_limit" };
inline constexpr const char* kStream[32] = { "enable","mute","solo","level","pan","position","spray","size","size_sync","size_rand",
  "density","density_sync","jitter","pitch","pitch_rand","scale","reverse_prob","window","window_skew","stereo_spread","gain_rand",
  "onset_trigger","filter_type","filter_cutoff","filter_rand","filter_res","drive","crush","crush_prob","ring_freq","ring_rand","ring_mix" };
inline constexpr const char* kFeedbackReverb[] = { "feedback","fb_low_cut","fb_high_cut","fb_saturation","fb_diffusion","reverb_mix","reverb_size","reverb_damp" };
inline constexpr const char* kLfo[4] = { "shape","rate","rate_sync","phase" };
// then env_attack env_release env_gain, pitch_follow_s1..s4, onset_threshold, macro1..4, mod1..16 {source,dest,depth}
inline juce::String stream (int n, const char* s) { return "s" + juce::String (n) + "_" + s; }
inline juce::String lfo (int n, const char* s)    { return "lfo" + juce::String (n) + "_" + s; }
inline juce::String mod (int m, const char* s)    { return "mod" + juce::String (m) + "_" + s; }
inline const juce::StringArray& allIdsInOrder();   // built once; used by layout AND harness
// APPEND-ONLY. Literal on purpose: a diff shows an append, a generated table hides a re-order.
inline constexpr const char* kModDestIds[kModDests] = { "None","input_gain","mix","output_gain","buffer_length","s1_level", /* … 113 */ };
inline constexpr int kModDestTableVersion = 1;
}
```
- Counts check (derived from the spec tables): per stream 24 F / 4 C / 4 B; global 4 F / 3 C / 1 B; fb+rev 8 F; LFO 8 F / 8 C; env 3 F; pitch-follow 4 B; onset 1 F; macros 4 F; mod 16 F / 32 C. Totals **140 / 59 / 21 = 220**.
- Per-stream dest offset: `5 + 24·(N−1) + k` for the 24 modulatable suffixes in `kStream` order (skipping enable/mute/solo/size_sync/density_sync/scale/onset_trigger/filter_type). Use it only as a harness cross-check against the literal table.
- `s1_enable` defaults On and s2–s4 Off. Special-case it in the loop and cover it in the golden.
- Shadowing: no ID is `begin` or `end` (critical_paramid_shadows_juce_free_function). Avoid C++ identifiers named after juce free functions if you add `constexpr` names.
- Every choice list has ≥ 2 entries (`sync_mode` has exactly 2), so the single-choice NaN trap (critical_choice_param_needs_two_choices) cannot fire.
- Groups (optional, for DAW tree views): `getParameters()` flattens depth-first in insertion order. If grouped, add groups in spec order; the golden catches any drift.

### 4b. Ranges
- `juce::ParameterID { id, 1 }` everywhere (spec `:31`). The harness asserts `getVersionHint() == 1`; it is `AudioProcessorParameter::getVersionHint()` [VERIFIED: `juce_AudioProcessorParameter.h:293`].
- Floats: `NormalisableRange<float> (start, end, 0.0f, skew)`, **interval 0** everywhere, per spec. That sidesteps pattern_stepped_range_snap_makes_zero_default_inexact. Log-like rows call `setSkewForCentre (centre)`; the harness checks the resulting skew against the spec value (`0.19889`, `0.24562`, `0.28906`, `0.40781`) to ±1e-4.
- **No lambda ranges** (critical_lambda_normalisable_range_invisible_to_webview_slider_frontend). The harness asserts `range.convertFrom0to1Function == nullptr` style: no custom map, skew > 0.
- **Defaults are not exact through APVTS.** `ParameterAdapter` seeds `unnormalisedValue (getRange().convertFrom0to1 (parameter.getDefaultValue()))` [VERIFIED: `juce_AudioProcessorValueTreeState.cpp:81`]. A float32 simulation of the round-trip gives `s_size` 99.99997, `s_density` 9.999997, `ring` 199.99992, `lfo_rate` 0.5000002 (skewed rows). Linear rows came back exact. Harness default checks therefore compare `getDefaultValue()` (normalised) to `range.convertTo0to1 (specDefault)` exactly, and engineering values with `approximatelyEqual` + an explicit absolute tolerance. Stage 2 must not assume `get() == 0 dB` → gain 1.0 is bit-exact on skewed or non-endpoint ranges.
- Factory-preset skew trap (pattern_factory_preset_normalized_ignores_skew) is Stage 4. Author presets in engineering units + `convertTo0to1`.

### 4c. Golden tests (recommended harness pattern)
Model on `ouaricon_add_processor_console` (derives every `JucePlugin_*` macro, compiles processor TUs only, `JUCE_WEB_BROWSER=0`; `ParamDump.cmake:67-271`). It is preferable to the older hand-macro harness CMake (O-simpleAmbisonics), which mirrors codes.
- `tests/golden/extract_spec.py` parses **parameter-spec.md** (the Full ID list fenced block `:258-479`, the group tables with `sN_`/`lfoN_`/`modM_` templates expanded, choice lists, the dest table `:201-231`) and **v2-ui.yaml** (`name:` for the 112 dests → `modM_dest` choice strings). It writes `tests/golden/params.tsv` (idx, id, type, start, end, default, skew-or-centre, choices) and `tests/golden/mod-dests.tsv`. Both are committed. The golden must come from the spec, never from the code, or the test is self-referential.
- `O-Granulator-render-harness` reads them via a `GOLDEN_DIR` compile definition and checks:
  - G1: count 220 and the ID sequence equal.
  - G2: per-ID type, plus totals 140/59/21.
  - G3: version hint 1.
  - G4: float start/end/skew/interval 0, default normalised.
  - G5: choice strings, order and default index.
  - G6: bool defaults.
  - D1: `kModDestIds` == golden (113, order).
  - D2: every dest ≠ None is a registered **Float** whose spec "Mod dest" column equals its index.
  - D3: each `modM_dest` choice list == `["None"] + host names of dests`.
  - D4: `kModDestTableVersion == 1`.
- Also wire `ouaricon_add_param_dump` (already in the mockup) for a human-readable TSV in `.planning/params.tsv`.

---

## 5. Shell + buses

Reference: `plugins/O-ReverseDelay/Source/PluginProcessor.cpp:1518-1531` [VERIFIED]. It does exactly D3:
```cpp
if (in.isDisabled() || out.isDisabled()) return false;
const bool inOk  = in  == mono() || in  == stereo();
const bool outOk = out == mono() || out == stereo();
return inOk && outOk && in.size() <= out.size();   // rejects stereo→mono
```
Constructor: `BusesProperties().withInput ("Input", stereo(), true).withOutput ("Output", stereo(), true)`.

The Stage 1 `processBlock` is a pure pass-through:
- `juce::ScopedNoDenormals`.
- `numIn = getMainBusNumInputChannels()`, `numOut = getMainBusNumOutputChannels()`, `numCh = buffer.getNumChannels()`.
- Stereo→stereo and mono→mono: touch nothing.
- Mono→stereo (`numIn == 1 && numOut == 2 && numCh >= 2`): `buffer.copyFrom (1, 0, buffer, 0, 0, n)`. The spec's "mono feeds both"; channel 1 otherwise holds input-less garbage.
- Clear any channel in `[max(numIn, …), numCh)` that is not an output.
- Bound every loop by `numCh` (pattern_standalone_canonical_channelset_oob). Derive "is stereo out" from the **negotiated** set in `prepareToPlay`, not from buffer width (pattern_stereo_in_mono_out_buffer_width_lies).
- No parameter reads needed. `setLatencySamples (0)` in `prepareToPlay` (JUCE 8 `getLatencySamples` is non-virtual). `getTailLengthSeconds()` 0 for now; Stage 2 sets the real tail.
- No `setValueNotifyingHost` anywhere in Source (grep gate), because no listener writes params (critical_vst3_nested_param_write_in_listener_never_reaches_host).
- AU channel configs are derived from this predicate: {1,1}, {1,2}, {2,2}. Logic caches AU I/O per VERSION (critical_logic_caches_au_io_config_per_version), so freeze the layouts now. A later change needs a version bump.

---

## 6. State

Recommended serialization, written in `getStateInformation` **onto the copy**:
```cpp
auto state = parameters.copyState();                    // never mutate the live tree from a host thread
state.setProperty ("uiLanguage", languageCode (uiLanguage.load()), nullptr);   // "en"/"fr"/"zh-Hans" (GS/RD precedent)
juce::ValueTree eng ("Engine");
eng.setProperty ("seed", juce::String ((juce::int64) seed.load()), nullptr);   // critical_juce_getlargeintvalue_clamps_unsigned_64
eng.setProperty ("stateVersion", 1, nullptr);
eng.setProperty ("modDestTableVersion", GranulatorParamIds::kModDestTableVersion, nullptr);
state.appendChild (eng, nullptr);
juce::ValueTree cs ("CustomScale");                     // name (≤40), text (source of truth), period, version
state.appendChild (cs, nullptr);
juce::ValueTree dm ("DegreeMasks");                     // only non-default masks
for (s 1..4) for (c 1..31) if (mask != ~0ull)
    dm.appendChild (juce::ValueTree ("Mask", { { "stream", s }, { "collection", c },
                    { "bits", juce::String::toHexString (mask).paddedLeft ('0', 16) } }), nullptr);
state.appendChild (dm, nullptr);
copyXmlToBinary (*state.createXml(), destData);
```
- **No `id` attribute on custom children.** `updateParameterConnectionsToChildTrees` calls `setNewState (child)` for every child and looks up `child.getProperty ("id")` as a parameter ID [VERIFIED: `juce_AudioProcessorValueTreeState.cpp:406-428`].
- Restore:
  - `xml->hasTagName (parameters.state.getType())`, then `replaceState`.
  - **Reset all non-param state to defaults first**: seed kept; custom = 12-EDO `"100 200 … 1200"`; masks `~0ull`; uiLanguage unchanged if absent.
  - Then read: `isVoid()` guards everywhere (critical_valuetree_xml_roundtrip_loses_type: every property comes back as a String var).
  - Ints come from `(int) var` (it parses the string).
  - Seed: `(uint64) s.getLargeIntValue()` after `s.containsOnly ("-0123456789")`, the GS pattern at `PluginProcessor.cpp:1258-1261`. Absent → keep the instance seed.
  - Masks: accept only 16 hex chars, then `(uint64) s.getHexValue64()`. `getHexValue64` parses as uint64 and casts without clamping [VERIFIED: `juce_String.cpp:1973`, `HexParser<uint64>`]. It **skips** non-hex chars, hence the `containsOnly` validation. `toHexString(uint64)` is unsigned [VERIFIED: `juce_String.cpp:1921-1941`].
  - Custom text: re-parse with `tableFromText`; on failure fall back to the 12-EDO default.
- Canonicalize masks on set: `mask |= ~((1ull << count) - 1)` for count < 64. "All on" is then always `~0ull` and "write only non-defaults" is exact. GS's own convention is "degrees beyond the string stay ON" (`PluginProcessor.cpp:1372-1378`). Custom masks reset to all-on when the custom degree list changes. Index masks by **Granulator choice index** (spec: "collection index 0…31"); index 0 (Off) is never written.
- uiLanguage: the spec says `int (0 en, 1 fr, 2 zh-Hans)`, while the integration checklist says `languageCode()`/`languageIndex()` per RD. Persist the code string; read tolerantly (a digit string → int, else `languageIndex`). Open question O3.
- preset-manager (Stage 4): module 1.1.1 still leaves `<CustomState>` in the tree after `setStateFromXml` (`OuariconPresetManager.h:673-700`) [VERIFIED]. If session state ever routes through it, strip the child (critical_preset_manager_stale_customstate_child). Its JSON custom-state blob must carry uint64 masks and the seed as **strings**. Stage 1 does not instantiate it.
- Absent PARAMs: `replaceState` leaves a param with no PARAM child at its current value (comment at GS `PluginProcessor.cpp:1291-1293`). Not an issue at 0.1.0 (no legacy sessions); note for Stage 4 presets (pattern_preset_apply_needs_reset_to_defaults).

---

## 7. Render harness scaffold + validation

- **Target:** `ouaricon_add_processor_console(OuariconGranulator ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-harness/main.cpp render-harness)` → `O-Granulator-render-harness`, plus `target_compile_features(... cxx_std_20)` and `target_compile_definitions(... GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/golden")`. It already sets `JUCE_WEB_BROWSER=0`, excludes `PluginEditor*` and `*UIResources`, and depends on the plugin target for JuceHeader.
- **createEditor guard** (pattern_render_harness_breaks_on_webview_editor):
  ```cpp
  #if JUCE_WEB_BROWSER
   // #include "PluginEditor.h"   // Stage 3
  #endif
  juce::AudioProcessorEditor* GranulatorProcessor::createEditor()
  {
  #if JUCE_WEB_BROWSER
      return new juce::GenericAudioProcessorEditor (*this);   // Stage 3: new GranulatorEditor (*this)
  #else
      return new juce::GenericAudioProcessorEditor (*this);   // harness only
  #endif
  }
  ```
- **Seed hook:** a public `uint64_t getSeed() const` / `void setSeed (uint64_t)` on an `std::atomic<uint64_t>`. The ctor seeds it from `Random::getSystemRandom().nextInt64()`, as GS does (`PluginProcessor.cpp:33`).
- **Synthetic playhead:** model O-ReverseDelay `tests/render-harness/main.cpp:613-627` (`MockPlayHead : juce::AudioPlayHead`, `getPosition()` → `PositionInfo` with bpm, isPlaying, timeSig). Extend it with `timeInSamples`, `ppqPosition` and `advance (numSamples)` so Stage 2 tempo gates inherit it. Also cover a null playhead and a no-bpm row.
- **Stage 1 rows:**
  - G1-G6 and D1-D4 (§4c).
  - B1 `isBusesLayoutSupported`: accepts the 3 layouts; rejects stereo→mono, disabled buses and 5.1.
  - B2 pass-through `memcmp`-exact in each layout, at all defaults AND with mix 0 %. Blocks 1/32/441/512/4096 + seeded variable; mono→stereo checks L == R == in.
  - B3 `getLatencySamples() == 0` after prepare.
  - S1 state round trip, both directions: seed `0xF00DFACECAFEBEEF` (> 2^63), mask `0x8000000000000001`, custom `edo 19`, uiLanguage 2.
  - S2 absent-property blob keeps the defaults.
  - S3 two reopens with a change between (no stale or duplicate child).
  - S4 no `Mask` children at defaults.
  - P1 scale map + names cross-check (§1d).
- **pluginval / auval:**
  - `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-Granulator-dev.vst3` (CI parity, `build-and-release.yml:633`; no `--skip-gui-tests`, since the Generic editor is safe). Repeat 2–3× (the fuzz reseeds each run).
  - `pluginval --strictness-level 10 --validate-in-process ~/Library/Audio/Plug-Ins/Components/O-Granulator-dev.component` (O-Bassoon precedent).
  - `auval -v aufx OuGn OuDv` (not `auval -a`, which SIGABRTs on this machine: pattern_auval_a_sigabrt_native_instruments_tcc), plus the negative control `auval -v aufx ZZZZ OuDv`. The first auval after install is a cold rescan, so run it in the background.

---

## Pitfalls checklist (each → memory file)
- [ ] Module header compiles at gnu++17 (GS + consoles). Granulator consoles get `cxx_std_20` explicitly. (build.ninja evidence; no memory file. Candidate for `/doc-fix`)
- [ ] Verbatim move. Diff the namespace body old vs new with whitespace intact; prove include resolution with `ninja -t deps` → [index_dsp_gate_design.md] / CONVERGENCE step 5
- [ ] GS UI maps `badEdo` (en/fr/zh) and the i18n gates pass → [feedback_module_extraction_regression_check.md], [project_i18n_fr_lint_and_glossary.md]
- [ ] Null test seeded, with a positive control and seed liveness → [pattern_null_test_needs_seeded_out_of_tree_pair.md]
- [ ] `git archive` trees lack gitignored UI module copies (inert for a C++-only archive) → [pattern_git_archive_ui_tree_misses_gitignored_module_copies.md]
- [ ] Shared checkout: temp-index, path-scoped commits; do not sweep `nielsen-dafx26.txt` → [index_git_shared_checkout.md]
- [ ] Never commit `Source/ui/public/modules/*` → [pattern_module_js_move_build_recopies_tracked_file.md]
- [ ] Binary-data symbols strip hyphens; distinct NAMESPACE + HEADER_NAME → [critical_binary_data_strips_hyphens.md], [critical_dual_binary_data_namespace_collision.md]
- [ ] Target ≠ folder → [build_script_target_name_vs_folder.md]
- [ ] `ParameterID {id, 1}`, interval 0, skew-only ranges, no lambdas → [pattern_stepped_range_snap_makes_zero_default_inexact.md], [critical_lambda_normalisable_range_invisible_to_webview_slider_frontend.md]
- [ ] Choice lists ≥ 2 → [critical_choice_param_needs_two_choices.md]; no `begin`/`end` identifiers → [critical_paramid_shadows_juce_free_function.md]
- [ ] No param writes from listeners or the audio thread → [critical_vst3_nested_param_write_in_listener_never_reaches_host.md]
- [ ] Bus traps: loop by `buffer.getNumChannels()`, stereo decision from the negotiated set → [critical_audiochannelset_is_a_bitset_not_an_order.md], [pattern_stereo_in_mono_out_buffer_width_lies.md], [pattern_standalone_canonical_channelset_oob.md]
- [ ] Freeze the I/O set before first install → [critical_logic_caches_au_io_config_per_version.md]
- [ ] int64 seed, hex masks → [critical_juce_getlargeintvalue_clamps_unsigned_64.md]; `isVoid()` + var conversion → [critical_valuetree_xml_roundtrip_loses_type.md]
- [ ] Two-reopen state test; strip `<CustomState>` if preset-manager is ever in the session path → [critical_preset_manager_stale_customstate_child.md]
- [ ] Reset non-param state before restore → [pattern_preset_apply_needs_reset_to_defaults.md]
- [ ] createEditor guard; harness excludes the editor TU → [pattern_render_harness_breaks_on_webview_editor.md]
- [ ] Warm `builtIns()` in the ctor (function-local static must not first-run on the audio thread) → GS `PluginProcessor.cpp:83-86`
- [ ] pluginval 10, repeated runs; NaN fuzz → [pattern_ci_pluginval10_catches_latent_nan.md]
- [ ] Targeted `auval -v`; cold rescan in the background → [pattern_auval_a_sigabrt_native_instruments_tcc.md], [pattern_cold_auval_after_install_rescans_registry.md]
- [ ] Long commands in the background → [pattern_executor_watchdog_stall_run_long_commands_in_background.md]; the 0→1 gate needs `--force` → [pattern_gate_0_to_1_always_needs_force.md]
- [ ] Never tag; bracket CHANGELOG heading → [feedback_never_tag_unless_publish.md], [pattern_changelog_heading_must_be_bracket_form_and_partial_fix_is_worse.md]

## Open questions / risks
- **O1 (blocks param code): host-facing names for 108 params.** Only the 112 mod-destination controls carry `name:` in v2-ui.yaml; the spec has no Name column. Recommendation: derive them from the YAML `label` with the spec's naming pattern ("S1 · Enable", "S1 · Size Sync", "LFO 1 · Shape", "Mod 1 · Source", "Macro 1", "Pitch Follow S1", "Freeze", "Sync Mode", "Scale Root", "Grain Limit"). Commit the list as part of `tests/golden/params.tsv` and have the user confirm it. Names are not identity (IDs are), but they show in DAW lanes and are what by-name VST3 null tests match on.
- **O2: `toScala` default description and sanitization** are not in the spec. The proposal is in §1d.
- **O3: uiLanguage stored form** (int per spec vs code string per checklist/RD). Recommendation: write the code, read both.
- **O4: GS help/badPitch copy update to mention edo.** Optional, and it costs fr/zh rows plus lint. Minimum = the `badEdo` row only.
- **R1: the GS migration touches a shipped plugin.** Any non-zero `cmp` in pitch-null is a blocker. Investigate before touching code (likely a non-verbatim move or a changed include order).
- **R2: shared-build configure breakage** (see §3). Land in order: module → GS → Granulator.
- **R3: pluginval on 220 params + 113-entry combos with the Generic editor** may run long. Give it `--timeout-ms 600000` and run it in the background.
- **R4: Stage 2 inherits "bitwise at defaults".** Skewed defaults are not exact through APVTS (§4b), so Stage 2 must not derive unity gain from `get()` on skewed or non-endpoint ranges without a snap.

## Suggested plan breakdown (ordered)
1. **T1: pitch-collections module** (`modules/tuning/pitch-collections/**`, `modules/registry.yaml` entry + `version` 1.2.18 → 1.3.0 + `last_updated`). Verbatim move + edo + 4-arg overload + toScala + tests (C++17). Gate: module tests exit 0. First capture the U5 corpus expectations from the **current** GS code (scratch console against HEAD).
2. **T2: GS 3.6.0 migration (isolated, gated; its own commit).**
   - Pre-capture first: param-dump output and the pitch-null-v351 dumps (add the driver + twin before editing PitchEngine.h).
   - Then the shim, CMake, UI `badEdo` + i18n, version/CHANGELOG/STATUS/PLUGINS.md.
   - Gate: pitch-null all-equal + the positive control differs; the 5 existing gates + twins/controls green; param-dump diff empty; check-i18n / fr-lint / zh-lint / check-ui-labels; pluginval 10 VST3 + AU; `auval -v aufx OuGS OuDv`; `regen-registry-used-by.sh`.
   - Commit only `plugins/O-GrainScatter/{Source,CMakeLists.txt,CHANGELOG.md,tests/pitch-null,.planning/STATUS.md}`, `PLUGINS.md` and `modules/registry.yaml`.
3. **T3: Granulator CMake + file skeleton.** CMakeLists from the mockup (pitch-collections uncommented, `cxx_std_20` on the consoles, render-harness block). Copy the UI resources. Placeholder `PluginEditor.{h,cpp}`. Gate: configure clean; `ninja OuariconGranulator_VST3 OuariconGranulator_AU OuariconGranulator_Standalone`.
4. **T4: `GranulatorParamIds.h` + `createParameterLayout()`** (220, frozen order, literal 113-entry dest table, choice→Id map). Depends on O1.
5. **T5: processor shell.** Buses, pass-through, `setLatencySamples(0)`, seed hook, scale-state placeholders over the module, the state codec (§6), `builtIns()` warm-up.
6. **T6: golden extractor + render harness.** `extract_spec.py` → `params.tsv`/`mod-dests.tsv`; `O-Granulator-render-harness` rows G/D/B/S/P. Gate: exit 0.
7. **T7: install + COMPAT-01.** `./scripts/build-and-install.sh O-Granulator`; pluginval 10 on VST3 and AU (2–3×); `auval -v aufx OuGn OuDv` + negative control; Standalone launch smoke.
8. **T8: bookkeeping.** STATUS.md, the PLUGINS.md row, registry `used_by` regen, `.planning/params.tsv` from param-dump. Path-scoped commit. Handoff: `/clear` → `/plugin-plan O-Granulator`.

## Environment Availability
| Dependency | Available | Version |
|---|---|---|
| JUCE | ✓ `/Users/taylorbrook/JUCE` | 8.0.15 (`JUCE_BUILDNUMBER 15`) |
| cmake / ninja | ✓ | 4.2.1 / 1.13.2 |
| pluginval | ✓ `/Applications/pluginval.app` | 1.0.4 (CI pins 1.0.3 Windows) |
| auval | ✓ `/usr/bin/auval` | system |
| git (configure-time `git archive`) | ✓ | — |

## Sources
- In-repo (HIGH): files cited inline with line ranges, all read this session. JUCE 8.0.15 sources: `juce_AudioProcessorValueTreeState.cpp:81,396-460`, `juce_String.cpp:481-538,1915-1973`, `juce_String.h:1124,1396-1402`, `juce_CharacterFunctions.h:455-475`, `juce_AudioProcessorParameter.h:293`.
- Memory files (HIGH, project-validated): those linked in the Pitfalls checklist.
- Simulation (MEDIUM): numpy float32 default round-trip (§4b). It approximates JUCE's `std::pow`/`exp`/`log` path; the harness is the authority.

## Assumptions Log
| # | Claim | Risk if wrong |
|---|---|---|
| A1 | `std::regex` ECMAScript+icase in libc++ matches the JS regex semantics for these inputs | Low: U2/U3 unit rows catch any divergence |
| A2 | pluginval `--validate-in-process` on `.component` works for AU here (O-Bassoon precedent only) | Low: fall back to the battery script form |
| A3 | The `(float) getDoubleValue()` round-trip loop finds an exact decimal by p ≤ 10 for all cents in (0.01, 4800) | Low: argued from float ulp vs 1e-10; U6 enforces it |
| A4 | Proposed host names for 108 params (O1) | Medium: user-visible strings in DAWs; confirm before T4 |
