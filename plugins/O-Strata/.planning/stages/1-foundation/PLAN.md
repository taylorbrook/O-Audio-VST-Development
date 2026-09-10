# Stage 1 — Foundation, second pass (re-parameterise the verified fork): PLAN

**Plugin:** O-Strata · **Stage:** 1 of 4 (Foundation + Shell, single phase, **second pass**) · **Phase:** plan
**Date:** 2026-09-10 · **Mode:** manual · **Branch:** `main` (trunk-based; path-scoped commits under `plugins/O-Strata`)
**Inputs:** `stages/1-foundation/CONTEXT.md` (D1–D4, amended criteria), `stages/1-foundation/RESEARCH.md` (§2.1–2.7 resolutions, §3 pitfalls, §4 ordering, §5 corrections), `parameter-spec.md` v2 (locked — **the spec wins** over ROADMAP / ARCHITECTURE where they disagree), `mockups/v2-PluginEditor-TEMPLATE.{h,cpp}` §0 / §4, `mockups/v2-integration-checklist.md` §A, ROADMAP "Stage 1: Foundation — second pass".
**First pass:** `first-pass-baked/` (fork + rename + strip, verified 2026-09-07). `smoke/` is shared and is **updated, not replaced**.
**Line numbers** below are the fork's at commit `056cc06f` and drift as edits land — always re-locate by the quoted token (memory `pattern_deferred_item_line_number_drifts_record_a_token`).

---

## Goal

The existing, verified O-Strata binary (`OuSt`, `VERSION 1.0.0`, sine wavetable placeholder) with its 48 baked-geometry parameters replaced by the 34 live-oscillator parameters of the locked spec v2 → **205 parameters**, the mod-destination list grown 26 → **46**, the state child renamed `terrainImports`, the relay plumbing sized for the v2 UI (166 slider + **8 combo** + 30 toggle), the O-Prism factory bank replaced by a single `Init`, and **COMPAT-01 re-verified** (pluginval strictness 10 VST3 + AU, auval). The page is untouched (D1); the voice is untouched (D3); both oscillators still sound the sine placeholder.

**Requirement verified:** COMPAT-01. Stage 1 evidence for FUNC-09 (ID parity via the `params.tsv` diff) and FUNC-05 (46-entry list accepted by the matrix); formal verification is Stage 2.

---

## Decisions resolved for the executor

These close RESEARCH §5's flags and the choices RESEARCH left to the planner. The executor applies them without asking.

| # | Question | Decision |
|---|---|---|
| 1 | Preset folder / stale bank (RESEARCH 2.1) | Folder is **`~/Library/O-Strata/Presets/`** (not Application Support). The initializer never deletes, so the executor runs `rm -rf ~/Library/O-Strata/Presets` (192 stale JSON; `User/` is absent — confirm with `ls` first) **immediately before the smoke harness run**, which is the first construction of the new binary and regenerates exactly `Factory/Init/Init.json`. No plugin host or old Standalone may launch between the `rm` and that run. |
| 2 | `osc?TerFreq` text function (RESEARCH 2.4) | **Keep the default** `stringFromValue` — the file has zero `AudioParameterFloatAttributes` uses and the `×` unit belongs to the UI readout. Text columns therefore read `0.2500000 / 8.0000000 / 1.0000000` (7 decimals); the verify criterion compares **values** (`getNormalisableRange().start/end`, `getDefaultValue()`), not text. |
| 3 | `NormalisableRange` lambdas (RESEARCH 2.4) | Three-argument `ValueRemapFunction (start, end, v)`, capture-free, written in terms of `s`/`e`: `from0to1 = s * pow (e / s, n)`, `to0to1 = log (v / s) / log (e / s)`; no snap function; `interval` 0. |
| 4 | `Init` preset shape (RESEARCH 2.5) | Self-describing: iterate `apvts.processor.getParameters()`, store `rp->getDefaultValue()` (already normalised — never through a raw→norm helper; memory `critical_apvts_denormalised_vs_preset_normalised`) under `getParameterID()`. Category `"Init"`, name `"Init"`. The 7 excluded tuning IDs are dropped by the preset manager at write time → 198 keys on disk. Delete everything else in `FactoryPresets.cpp` (helpers, `SRC_*`/`DST_*` enums, `completeBase`, archetypes, nine `add*`). No v1.0.6 migration hook exists in the fork (pre-1.25.0 preset manager) — nothing to preserve; Stage 4 note only. |
| 5 | Smoke check [4] positive control (RESEARCH 2.6) | RESEARCH proposed routing LFO1 → `OscA Orbit Size` (= `osc?Pos`). **Rejected:** the placeholder is a **single-frame** sine table, so `Pos` is inert and the control would be vacuous. Use **`Pitch` (index 23 → norm 23/45)** as the positive control; fall back to `OscA Pan` (16) if Pitch is unmodulated on the placeholder path. The negative arm routes to `OscA Terrain Freq` (index 31 → norm 31/45). |
| 6 | Harness literals (RESEARCH §3 last row) | Every `205` / `198` / `31` / `46` literal in `smoke/main.cpp` is asserted **alongside** the live value (`getParameters().size()`, `getModDestNames().size()`, `getModDestNames()[31] == "OscA Terrain Freq"`), so a later spec drift fails loudly instead of silently re-pinning (memory `pattern_test_fixture_mirrors_drift_silently`). |
| 7 | `params.tsv` baseline (RESEARCH 2.7) | Diff against **`git show a774d6d4^:plugins/O-Prism/.planning/params.tsv`** (the literal v1.24.0 registry; the working-tree file is v1.25.0 and differs only in the two `osc?Table` rows). ID-keyed procedure in Task 8. Expected: −2 / +34 / **20** in place. |
| 8 | Host names for the 17 new pushes | Not contractual (spec §semantics). Fixed here so param-dump is reproducible: `Terrain, Terrain Freq, Terrain Mod X, Terrain Mod Y, Pitch Track, Saturation, Terrain Blur, Terrain Edge, Orbit, Orbit Aspect, Orbit Rotation, Orbit Centre X, Orbit Centre Y, Orbit Mod, Feedback, Feedback Damp, Quality`, each prefixed `"Osc A " / "Osc B "`. `osc?Pos` → `Osc ? Orbit Size`. |
| 9 | Float steps | **0.001** on every new float except `OrbRot` (0.1) and `TerFreq` (0 — continuous), per spec §APVTS order ("0.001 unless stated"). `OrbCX / OrbCY` use 0.001, not the first pass's 0.01. |
| 10 | Stale-count comments in source | Refresh **comments only** where they name `219`, `188`, `48 geometry`, `96 presets` (`CMakeLists.txt` test block, `PluginEditor.cpp` jassert comment, `FactoryPresets.h`, `tests/ui_tip_render_check.js:1002-1007`). No gate value changes (tip gate stays 106). |
| 11 | UI gates | Page byte-identical (D1) → run as **regression only**; expected result is the first-pass baseline verbatim (check-i18n ALL PASS, fr lint CLEAN, zh lint GATE PASSED, check-ui-labels 20 states, boot-all-uis 0 DEAD / 0 late, tip render 2799 / 106 bindings). Any deviation is a plan defect, not a fix target. |
| 12 | ROADMAP items moved by CONTEXT | New knobs / `data-i18n` keys → Phase 3.1 (D1); "existing preset set re-pointed" → single `Init` (D2); voice "reads and discards" → not edited (D3). ROADMAP / ARCHITECTURE are checksummed and **not edited**; the count correction (20 in-place rows) lives in CONTEXT D4 and SUMMARY.md. |

---

## Tasks

Order follows RESEARCH §4. Tasks 1–6 are pure edits checkable without a build; 7–11 need the build; 12–14 close. Each task ends with the check the executor runs before moving on.

### Task 1 — `StrataParamIds.h`: slider IDs 35 → 24 per oscillator, new combo IDs
- **Modify:** `Source/StrataParamIds.h` (`oscIds()` ≈ :47-63, `allSliderIds()` ≈ :113-142)
- **`oscIds (prefix)`:** replace the suffix list with exactly (template §0):
  `Pos, Level, Pan, Coarse, Fine, Phase, Unison, Detune, Width, WarpType, WarpAmt, TerFreq, TerModX, TerModY, TerTrack, TerSat, TerBlur, OrbAspect, OrbRot, OrbCX, OrbCY, OrbMod, OrbFeedback, OrbFbDamp` (24). Replace the "Geometry (parameter-spec.md v1 order, 24)" comment with "Terrain / Orbit floats (parameter-spec.md v2 rows 13–18, 21–27)".
- **Add** after `oscIds`:
  ```cpp
  // ─── Oscillator COMBOBOX IDs — the 4 Choice params per oscillator (spec rows 12, 19, 20, 28) ───
  inline juce::StringArray oscComboIds (const juce::String& prefix)
  {
      juce::StringArray ids;
      for (const auto* s : { "Terrain", "TerEdge", "Orbit", "Quality" })
          ids.add (prefix + s);
      return ids;                                                        // 4
  }

  inline juce::StringArray allComboIds()
  {
      juce::StringArray all;
      all.addArray (oscComboIds ("oscA"));                                // 4
      all.addArray (oscComboIds ("oscB"));                                // 4
      return all;                                                         // 8
  }
  ```
- **`allSliderIds()`:** comments `// 35` → `// 24` (×2), trailing `// 188` → `// 166`. Body otherwise unchanged (`delayDivision` stays).
- **Depends on:** none
- **Verify:** hand-count `oscIds` = 24, `oscComboIds` = 4; `grep -c '"Geo\|"Mesh\|"Vol\|TerOrbit\|TerCX\|TerCY\|TerAspect\|TerRot\|TerSweep' Source/StrataParamIds.h` = 0.

### Task 2 — `PluginProcessor.cpp`: `createOscParameters` re-parameterised
- **Modify:** `Source/PluginProcessor.cpp` — `createOscParameters` (≈ :73-170), `createParameterLayout` comments (≈ :528-529)
- **In-place edits (carried block):**
  - `prefix + "Pos"` push (≈ :81-83): name `label + " Orbit Size"`, default `0.5f` (range `0, 1, 0.001` unchanged)
  - `prefix + "Unison"` push (≈ :97-98): `AudioParameterInt (…, 1, 4, 1)` (was `1, 8, 1`)
- **Delete** the baked block: from the `// ─── Geometry (Stage 1: inert bake parameters …` comment (≈ :112) through `choice ("TerEdge", …)` (≈ :167), **keeping** `imported` and the `choice` / `knob` lambdas (with their `jassert (choices.size() >= 2)`); delete `epitroch` and `hypocycl`.
- **Add** the 17 pushes in this exact order (spec rows 12–28), after `WarpAmt` and before `return params;`:
  ```cpp
  // ─── Terrain / Orbit / Quality (parameter-spec.md v2 rows 12–28; live oscillator lands in Phase 2.1) ───
  // Non-ASCII glyphs (U+2026 …, U+00E7 ç, U+00D7 ×) via CharPointer_UTF8 hex escapes — the source stays ASCII
  // (memory critical_juce_string_char_ctor_is_ascii_only).
  const juce::String limacon (juce::CharPointer_UTF8 ("Lima\xC3\xA7on"));
  const juce::String twoX    (juce::CharPointer_UTF8 ("2\xC3\x97"));
  const juce::String fourX   (juce::CharPointer_UTF8 ("4\xC3\x97"));

  // Exact-log range 0.25–8.0 (norm 0.4 = 1.0×). Three-argument ValueRemapFunction, capture-free;
  // interval 0 → getNumSteps() continuous, default text 7 decimals (RESEARCH 2.4).
  const juce::NormalisableRange<float> terFreqRange (0.25f, 8.0f,
      [] (float s, float e, float n) { return s * std::pow (e / s, n); },
      [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });

  choice ("Terrain",     " Terrain",        { "Sine Product", "Radial Rings", "Saddle", "Ridged Cosines",
                                              "Mitsuhashi", "Cosine Wells", imported }, 0);          // 7, Imported… last
  params.push_back (std::make_unique<juce::AudioParameterFloat> (
      juce::ParameterID { prefix + "TerFreq", 1 }, label + " Terrain Freq", terFreqRange, 1.0f));
  knob   ("TerModX",     " Terrain Mod X",  0.0f, 1.0f, 0.001f, 0.5f);
  knob   ("TerModY",     " Terrain Mod Y",  0.0f, 1.0f, 0.001f, 0.5f);
  knob   ("TerTrack",    " Pitch Track",    0.0f, 1.0f, 0.001f, 1.0f);
  knob   ("TerSat",      " Saturation",     0.0f, 1.0f, 0.001f, 0.0f);
  knob   ("TerBlur",     " Terrain Blur",   0.0f, 1.0f, 0.001f, 0.2f);
  choice ("TerEdge",     " Terrain Edge",   { "Mirror", "Window" }, 0);                              // exactly 2 — never trim
  choice ("Orbit",       " Orbit",          { "Ellipse", "Superellipse", limacon,
                                              "Epitrochoid 3", "Epitrochoid 5", "Epitrochoid 7",
                                              "Hypocycloid 3", "Hypocycloid 5", "Hypocycloid 7",
                                              "Butterfly", "Squarcle" }, 0);                          // 11
  knob   ("OrbAspect",   " Orbit Aspect",   0.1f, 1.0f, 0.001f, 0.7f);
  knob   ("OrbRot",      " Orbit Rotation", 0.0f, 360.0f, 0.1f, 0.0f);
  knob   ("OrbCX",       " Orbit Centre X", -1.0f, 1.0f, 0.001f, 0.13f);
  knob   ("OrbCY",       " Orbit Centre Y", -1.0f, 1.0f, 0.001f, 0.21f);
  knob   ("OrbMod",      " Orbit Mod",      0.0f, 1.0f, 0.001f, 0.5f);
  knob   ("OrbFeedback", " Feedback",       0.0f, 1.0f, 0.001f, 0.0f);
  knob   ("OrbFbDamp",   " Feedback Damp",  0.0f, 1.0f, 0.001f, 0.5f);
  choice ("Quality",     " Quality",        { "Bandlimited", twoX, fourX }, 1);                       // default 2×

  jassert (params.size() == 28);   // spec v2 rows 1–28 — StrataParamIds::oscIds (24) + oscComboIds (4) must agree
  ```
  `<cmath>` is already available through `JuceHeader.h`. The `choice` lambda's mixed `const char*` / `juce::String` brace lists compile today (the `Mesh` list used `imported`).
- **`createParameterLayout()`:** comments `// 10` → `// 28` on the two `createOscParameters` lines.
- **Shadow re-grep** (memory `critical_paramid_shadows_juce_free_function`) — run once, expect only `done`:
  ```bash
  for s in Terrain TerFreq TerModX TerModY TerTrack TerSat TerBlur TerEdge Orbit OrbAspect OrbRot OrbCX OrbCY OrbMod OrbFeedback OrbFbDamp Quality; do
    grep -rIn --include='*.h' -E "^\s*(inline|static|constexpr|JUCE_API)?\s*[A-Za-z_:<>]+\s+$s\s*\(" /Users/taylorbrook/JUCE/modules/juce_core /Users/taylorbrook/JUCE/modules/juce_audio_basics && echo "SHADOW: $s"; done; echo done
  ```
- **Depends on:** none (Task 1 independent)
- **Verify:** `grep -c 'choice ("\|knob ("' Source/PluginProcessor.cpp` = **16** (4 `choice` + 12 `knob`) — plus the explicit `TerFreq` push = 17 new parameters per oscillator; removal grep `grep -n "GeoSource\|GeoFrames\|GeoDrive\|MeshTilt\|VolField\|VolOrbit\|TerSweep\|TerOrbit\|epitroch\|hypocycl" Source/PluginProcessor.cpp` prints nothing.

### Task 3 — `ModulationMatrix.h`: `ModDest` 26 → 46
- **Modify:** `Source/dsp/ModulationMatrix.h` (`enum class ModDest` ≈ :54-82, `getModDestNames()` ≈ :92-101)
- **Enum:** after `OscBWarpAmt,` insert, before `NumDests`:
  `OscAOrbAspect, OscAOrbRot, OscAOrbCX, OscAOrbCY, OscAOrbMod, OscATerFreq, OscATerModX, OscATerModY, OscAOrbFeedback, OscATerSat, OscBOrbAspect, OscBOrbRot, OscBOrbCX, OscBOrbCY, OscBOrbMod, OscBTerFreq, OscBTerModX, OscBTerModY, OscBOrbFeedback, OscBTerSat` (enum names `OscAPos` / `OscBPos` unchanged).
- **`getModDestNames()`:** `"OscA Pos"` → `"OscA Orbit Size"`, `"OscB Pos"` → `"OscB Orbit Size"`; append after `"OscB Warp"` verbatim:
  `"OscA Orbit Aspect", "OscA Orbit Rot", "OscA Orbit CX", "OscA Orbit CY", "OscA Orbit Mod", "OscA Terrain Freq", "OscA Terrain Mod X", "OscA Terrain Mod Y", "OscA Feedback", "OscA Saturation", "OscB Orbit Aspect", "OscB Orbit Rot", "OscB Orbit CX", "OscB Orbit CY", "OscB Orbit Mod", "OscB Terrain Freq", "OscB Terrain Mod X", "OscB Terrain Mod Y", "OscB Feedback", "OscB Saturation"`.
- Add `static_assert (static_cast<int> (ModDest::NumDests) == 46, "getModDestNames() must list 46 entries");` after the enum (the `StringArray` size is checked by the smoke harness, Task 9).
- **Nothing else:** `createModMatrixParameters()` builds every `modSlot?Dst` from `getModDestNames()`; `destOffsets` is `std::array<float, kNumDests>` and the slot loop bounds-checks (D3). `StrataVoice.cpp` **not edited**.
- **Depends on:** none
- **Verify:** `grep -c '"Osc[AB] ' Source/dsp/ModulationMatrix.h` = 28 (2 relabelled + 2 Detune + 2 Pan + 2 Warp + 20 new); `grep -n '"OscA Pos"\|"OscB Pos"\|"OscA Size"' Source/dsp/ModulationMatrix.h` prints nothing.

### Task 4 — State child `geometryImports` → `terrainImports`
- **Modify:** `Source/PluginProcessor.cpp` (`getStateInformation` ≈ :1078-1081, `setStateInformation` ≈ :1127-1129)
- Both string literals → `"terrainImports"`; the two comments → "Phase 4.1 fills" / "absent or empty terrainImports child is the pre-4.1 state". Still written empty, still read with `ignoreUnused`.
- **Depends on:** none
- **Verify:** `grep -rn geometryImports plugins/O-Strata/Source/` prints nothing; `grep -c terrainImports Source/PluginProcessor.cpp` = 2 (+ comments).

### Task 5 — `PluginEditor.h/.cpp`: combo relay group (relays only — D1)
- **Modify:** `Source/PluginEditor.h` (member blocks ≈ :51-73), `Source/PluginEditor.cpp` (constructor Steps 1–3 ≈ :640-760)
- **`.h`:** after `sliderRelays` add `std::vector<std::unique_ptr<juce::WebComboBoxRelay>> comboRelays;   // 8`; after `sliderAttachments` add `std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;   // 8`. Declaration order relays → webView → attachments is what gives the destruction order; do not move anything else.
- **`.cpp` Step 1** (after the `sliderRelays` loop): `auto comboIds = StrataParamIds::allComboIds();` + `for (const auto& id : comboIds) comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));`
- **Step 2** (after the `sliderRelays` `withOptionsFrom` loop): `for (const auto& relay : comboRelays) options = options.withOptionsFrom (*relay);`
- **Step 3** (after the slider `jassert`): the template §4 attachment loop (3-argument `WebComboBoxParameterAttachment (*param, *comboRelays[i], nullptr)`) + `jassert (comboAttachments.size() == comboRelays.size());   // 8`. Update the existing slider jassert comment "188 in Stage 1" → "166 after the Stage 1 second pass".
- **Not touched:** `index.html`, `i18n.js`, native functions (`getModDestNames` grows automatically), `getResource`, `timerCallback`.
- **Depends on:** Task 1 (`allComboIds`)
- **Verify:** `grep -c "comboRelays\|comboAttachments" Source/PluginEditor.cpp` ≥ 6; `grep -c withNativeFunction Source/PluginEditor.cpp` = 31 (unchanged); `git diff --stat -- Source/ui/` empty.

### Task 6 — `FactoryPresets.cpp/.h`: single `Init` (D2, Decision 4)
- **Modify:** `Source/FactoryPresets.cpp` (2354 → ≈ 60 lines), `Source/FactoryPresets.h` (comment)
- **`.cpp`:** keep the AGPL header; replace everything from the file-description comment through the end with:
  ```cpp
  /*
    ==============================================================================
      FactoryPresets.cpp
      O-Strata - Factory preset library.
      Stage 1: a single "Init" preset at the 205-parameter defaults so the preset
      manager has a non-empty bank. The real bank is authored in Phase 4.1 from
      the live oscillator. No O-Prism preset content survives (Stage 1 CONTEXT D2).
    ==============================================================================
  */
  #include "FactoryPresets.h"

  std::vector<OuariconPresetManager::FactoryPresetDef>
  FactoryPresets::build (juce::AudioProcessorValueTreeState& apvts)
  {
      OuariconPresetManager::FactoryPresetDef init;
      init.category = "Init";
      init.name     = "Init";

      // getDefaultValue() is already normalised [0,1] — store it directly
      // (memory critical_apvts_denormalised_vs_preset_normalised). The preset
      // manager drops excludedParameterIds (7 tuning IDs) at write time → 198 keys.
      for (auto* p : apvts.processor.getParameters())
          if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
              init.parameters[rp->getParameterID()] = rp->getDefaultValue();

      return { init };
  }
  ```
- **`.h`:** comment "Factory preset library (96 presets across 9 categories)" → "Factory preset library (Stage 1: `Init` only; real bank in Phase 4.1)"; doc comment on `build` → "Values are normalised [0,1]; tuning parameters are excluded by the PresetManager at write time."
- **Depends on:** none
- **Verify:** `grep -c "makePreset\|DST_\|SRC_\|completeBase\|WT_\|osc[AB]Table" Source/FactoryPresets.cpp` = 0; `wc -l` < 80; `grep -c '"Init"' Source/FactoryPresets.cpp` = 2.

### Task 7 — Configure + build (background)
- **Commands (repo root):**
  ```bash
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DOUARICON_BUILD_TESTS=ON
  ninja -C build O-Strata-param-dump                                    # first — Task 8 + smoke link both need fresh objects
  ninja -C build O-Strata_VST3 O-Strata_AU O-Strata_Standalone          # run_in_background, one call (memory pattern_executor_watchdog_stall_run_long_commands_in_background)
  ```
- **Depends on:** Tasks 1–6
- **Verify:** param-dump binary present (`find build/plugins/O-Strata -name 'O-Strata-param-dump' -perm +111`); three artefacts under `build/plugins/O-Strata/O-Strata_artefacts/Release/{VST3,AU,Standalone}/`; no new warnings vs the first-pass build. Likely compile errors to fix in place: a `StringArray` list mixing `const char*` and `juce::String` (build the list from `juce::String` objects if the variadic ctor refuses), a stale `epitroch` reference, a missing `#include <cmath>`.

### Task 8 — `params.tsv` regenerate + ID-keyed diff (FUNC-09 evidence)
- **Create/replace:** `plugins/O-Strata/.planning/params.tsv` (219 → 205 rows)
- **Commands** (`$SP` = this session's scratchpad dir, never `/tmp`):
  ```bash
  "$(find build/plugins/O-Strata -name 'O-Strata-param-dump' -perm +111 | head -1)" > plugins/O-Strata/.planning/params.tsv
  head -2 plugins/O-Strata/.planning/params.tsv                                    # "# params 205"
  git show a774d6d4^:plugins/O-Prism/.planning/params.tsv | grep -v '^#' | sort > $SP/prism.rows
  grep -v '^#' plugins/O-Strata/.planning/params.tsv                  | sort > $SP/strata.rows
  cut -f1 $SP/prism.rows | sort > $SP/prism.ids;  cut -f1 $SP/strata.rows | sort > $SP/strata.ids
  echo "removed: $(comm -23 $SP/prism.ids $SP/strata.ids | tr '\n' ' ')"           # oscATable oscBTable
  echo "added:   $(comm -13 $SP/prism.ids $SP/strata.ids | wc -l)"                 # 34
  join -t $'\t' -j1 $SP/prism.rows $SP/strata.rows | awk -F'\t' '{ n=(NF-1)/2; a=""; b=""; for(i=2;i<=n+1;i++){a=a"\t"$i; b=b"\t"$(i+n)}; if (a!=b) print $1 }' | sort > $SP/changed.ids
  echo "changed in place: $(wc -l < $SP/changed.ids)"; cat $SP/changed.ids         # 20: osc?Pos osc?Unison modSlot{0..15}Dst
  grep -n "^oscA" plugins/O-Strata/.planning/params.tsv | cut -f1 | head -30      # 28 rows, WarpAmt then Terrain … Quality
  grep "^osc[AB]TerFreq" plugins/O-Strata/.planning/params.tsv                    # numSteps 2147483647, textAtMin 0.2500000, textAtMax 8.0000000, defaultNorm 0.400000
  grep "^modSlot0Dst" plugins/O-Strata/.planning/params.tsv                        # numSteps 46, textAtMax "OscB Saturation"
  ```
- **Depends on:** Task 7 (param-dump target)
- **Verify:** header `# params 205`; removed = exactly `oscATable oscBTable`; added = 34; changed = 20 with the expected field deltas (`osc?Pos` name / defaultNorm 0.500000 / defaultText; `osc?Unison` numSteps 4 / textAtMax 4; `modSlot?Dst` numSteps 46 / textAtMax); the 17 new `oscA*` rows sit between `oscAWarpAmt` and `oscBPos` in file order; choice text columns byte-equal to the spec (`Imported…`, `Limaçon`, `2×`). Record the three echo lines in SUMMARY.md.

### Task 9 — Stale bank removal + smoke harness update + run (D2, D3)
- **Modify:** `stages/1-foundation/smoke/main.cpp` (206 lines today); `build.sh` unchanged (self-updating from `build.ninja`)
- **`main.cpp` edits:**
  - header comment lines 7–8 and every `219` literal (≈ :167, 175, 190, 191) → `205`; the "214 randomised" note (≈ :192) → recompute from the harness's own exclusion list; `geometryImports` → `terrainImports` (≈ :8, 157, 158, 160)
  - **[3] additions:** `check (params2.size() == 205 && params2.size() == (size_t) s2->getParameters().size(), …)` — literal and live value together (Decision 6)
  - **New [4] — D3 route is inert, with positive control:** two fresh processors, identical `prepareToPlay`, same held note, same buffer length; in P-neg set `modSlot0Src` = LFO1 (norm 0.1), `modSlot0Dst` = **31/45** (`OscA Terrain Freq`), `modSlot0Amt` = 1.0, `modSlot0On` = 1; assert max |Δ| == 0 vs the unrouted render. In P-pos route the same slot to **`Pitch` (23/45)** and assert max |Δ| > 1e-3 (Decision 5; fallback `OscA Pan` 16/45). Also assert `getModDestNames().size() == 46`, `[1] == "OscA Orbit Size"`, `[31] == "OscA Terrain Freq"`, `[45] == "OscB Saturation"` (FUNC-05 machine-checked).
  - **New [5] — on-disk bank is exactly `Init`:** after the first processor construction, `find ~/Library/O-Strata/Presets/Factory -name '*.json'` (via `juce::File::findChildFiles`) = exactly one file at `Init/Init.json`; `.factory-version` reads `1.0.0`; parse the JSON: `parameters` has **198** keys, none of the 7 excluded tuning IDs, `oscAPos` = 0.5, `oscAUnison` = 0.0, `oscAQuality` = 0.5 (index 1 of 3), `oscATerFreq` ≈ 0.4.
  - **New [6] — choice lists:** `oscATerrain` 7 choices, last = `Imported…`; `oscATerEdge` 2; `oscAOrbit` 11, `[2]` = `Limaçon`; `oscAQuality` 3, default index 1, `[1]` = `2×`; `oscATerFreq` range start 0.25 / end 8.0, `getDefaultValue()` ≈ 0.400000, `getNumSteps()` = 0x7fffffff. Compare through `juce::String (juce::CharPointer_UTF8 (…))` so the harness stays ASCII.
- **Commands (exact order — Decision 1):**
  ```bash
  ls ~/Library/O-Strata/Presets/                      # expect: Factory only (no User) — if User has files, STOP and ask
  rm -rf ~/Library/O-Strata/Presets                   # 192 stale O-Prism-derived JSON; nothing has shipped
  bash plugins/O-Strata/.planning/stages/1-foundation/smoke/build.sh     # links the fresh param-dump objects from Task 7
  plugins/O-Strata/.planning/stages/1-foundation/smoke/strata-smoke | tee plugins/O-Strata/.planning/stages/1-foundation/smoke/smoke-output.log
  find ~/Library/O-Strata/Presets -type f | sort      # Factory/.factory-version, Factory/Init/Init.json — nothing else
  ```
  No DAW, `auval`, pluginval or Standalone may run between the `rm` and the smoke run (the old installed `-dev` bundle would regenerate the 192).
- **Depends on:** Tasks 7, 8
- **Verify:** `ALL SMOKE CHECKS PASSED — 0 failure(s)`; checks [1]–[6] all `PASS`; the on-disk `find` shows exactly two files. Commit `smoke-output.log` (tracked already) and the updated `main.cpp`.

### Task 10 — Install + validate (COMPAT-01)
- **Commands:**
  ```bash
  ./scripts/build-and-install.sh O-Strata      # dual-variant sweep; VST3 + AU (Standalone stays stale — memory pattern_build_install_skips_standalone_stale_ui)
  /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests --timeout-ms 60000 --validate ~/Library/Audio/Plug-Ins/VST3/O-Strata-dev.vst3
  /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests --timeout-ms 60000 --validate ~/Library/Audio/Plug-Ins/Components/O-Strata-dev.component
  auval -v aumu OuSt OuDv
  auval -a | grep -i strata                    # run_in_background at the END of the batch, 2-min budget (memory pattern_cold_auval_after_install_rescans_registry)
  find ~/Library/O-Strata/Presets -type f | sort   # still exactly two files — the bank was present at 1.0.0, so no regeneration
  ```
- **Depends on:** Tasks 7, 9 (bank asserted before any host constructs the plugin)
- **Verify:** both pluginval runs print `SUCCESS` with version `1.0.0` in the header (the lambda-range round-trip is the one `[ASSUMPTION]` RESEARCH 2.4 left — this run is its check; if strictness 10 flags `osc?TerFreq`, capture the exact message and stop); `auval -v` → `AU VALIDATION SUCCEEDED`; `auval -a` lists `aumu OuSt OuDv`. Capture the pluginval headers for SUMMARY.md.

### Task 11 — UI gates (regression only — D1, Decision 11)
- **Commands:**
  ```bash
  git diff --stat HEAD -- plugins/O-Strata/Source/ui plugins/O-Strata/tests/i18n-states.json plugins/O-Strata/tests/ui-stub   # MUST be empty
  node scripts/check-i18n.js --plugin O-Strata
  node scripts/i18n-fr-lint.js --plugin O-Strata --strict
  node scripts/i18n-zh-lint.js --plugin O-Strata
  node scripts/check-ui-labels.js --plugin O-Strata
  node scripts/boot-all-uis.js --plugin O-Strata --strict-tips
  node plugins/O-Strata/tests/ui_tip_render_check.js
  ```
  If another session is running a UI gate, the stub server port may be taken and serve **its** page (memory `pattern_ui_test_server_port_clash_serves_other_session`) — check `lsof -i :<port>` before trusting a result.
- **Depends on:** Task 7 (nothing in the page changed; run any time after the source edits)
- **Verify:** identical to the first-pass baseline: check-i18n ALL CHECKS PASS; fr lint CLEAN exit 0; zh lint GATE PASSED exit 0; check-ui-labels ALL CHECKS PASSED, 20 states; boot-all-uis 0 DEAD / 0 late / no page errors; tip render ALL CHECKS PASSED (2799, 106 bindings). Any change = stop and diagnose (the page is byte-identical, so a change means a gate reads C++ or `params.tsv`).

### Task 12 — Docs, stale-count comments, registry row
- **Modify:** `plugins/O-Strata/CHANGELOG.md` — add a second Stage 1 block under `## v1.0.0 (unreleased)`:
  "Stage 1, second pass (re-parameterise): the 48 baked-geometry parameters are replaced by the 34 live wave-terrain parameters of parameter-spec v2 (17 per oscillator: Terrain, Terrain Freq (exact-log 0.25–8×), Mod X/Y, Pitch Track, Saturation, Blur, Edge, Orbit (11 shapes), Aspect, Rotation, Centre X/Y, Orbit Mod, Feedback, Feedback Damp, Quality (Bandlimited / 2× / 4×)) — **205 parameters**. `osc?Pos` is now Orbit Size (default 0.5); `osc?Unison` is 1–4 (was 1–8) — **the last free range change**: no O-Strata preset has shipped; from v1.0.0 every range or list change needs its own migration gate. Mod-matrix destinations 26 → **46** (indices 1/2 relabelled Orbit Size; 20 new per-oscillator destinations appended, inert until Phase 2.1). State child `geometryImports` → `terrainImports`. Factory bank reset: the inherited O-Prism presets are removed; Stage 1 ships one `Init` preset; the real bank is Phase 4.1. 8 `WebComboBoxRelay`s added for the four Choice parameters per oscillator (page unchanged until Stage 3)."
- **Modify:** `plugins/O-Strata/NOTES.md` — timeline entry `2026-09-10 — Stage 1 second pass: …`; Known Issues: replace "All 96 factory presets … re-authored in Stage 4.1" with "Factory bank is `Init` only until Phase 4.1"; Additional Notes: "v1.0 is baked-only … v1.1" → "v1.0 is the live wave-terrain oscillator (design v2, 2026-09-08); baked sources are v1.1"; Type → "Synth (Microtonal Wave Terrain)".
- **Modify (comments only — Decision 10):** `plugins/O-Strata/CMakeLists.txt` test block ("219 parameter IDs … Expect 219 rows; diff … = -2 +48" → "205 … = -2 +34"); `tests/ui_tip_render_check.js` ≈ :1002-1007 ("103 of O-Strata's 219 parameters … 48 geometry" → "… 205 … 34 terrain / orbit"); `Source/PluginProcessor.h` banner subtitle if it reads "Microtonal Wavetable Synthesizer" → "Microtonal Wave-Terrain Synthesizer" (also `PluginEditor.h:23`). Gate: `grep -rn "219\|\b188\b\|48 geometry\|48 inert\|96 presets" plugins/O-Strata/Source plugins/O-Strata/CMakeLists.txt plugins/O-Strata/tests` prints nothing afterwards.
- **Modify:** `PLUGINS.md` row → `| O-Strata | 🚧 Stage 1 (second pass — 205 live-oscillator params, COMPAT-01 re-verified) | 1.0.0 | Synth (Microtonal Wave Terrain) | 2026-09-10 |`; then `grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d` must be empty.
- **Modify:** `.planning/STATUS.md` — Stage 1 table execute ✓ (verify row stays ○ for `/plugin-verify`), `status: stage_1_second_pass_execute_complete`, `next_action: "/plugin-verify O-Strata 1-foundation"`, progress bar, Current Position paragraph; **do not** touch `contract_checksums` (no contract edited).
- **Not edited:** `parameter-spec.md`, `ROADMAP.md`, `research/ARCHITECTURE.md`, `BRIEF.md`, `REQUIREMENTS.md`, anything under `plugins/O-Prism/`.
- **Depends on:** Tasks 8–11 (numbers must be measured, not predicted)
- **Verify:** the stale-count grep prints nothing; PLUGINS.md duplicate check empty; `git status --short plugins/O-Prism` empty.

### Task 13 — `SUMMARY.md`
- **Create:** `stages/1-foundation/SUMMARY.md` (template `summary-complex.md` — 14 tasks, 12+ files, harness work). Must include: the Task 8 diff echo lines (`removed / added / changed in place`) and the 20 changed IDs; the 17 host names (Decision 8) and the step convention (Decision 9); pluginval headers ×2 + auval line; smoke log summary (checks [1]–[6]); the on-disk `find` output (two files); the UI-gate baseline confirmation; RESEARCH's four corrections restated as facts; **Notes for later stages:** Phase 2.1 wires `ModDest` 26–45 in the voice (indices in spec §Mod-matrix destinations), deletes `WavetableOscillator`/`WavetableData` and the placeholder; Phase 3.1 re-forks the page from O-Prism v1.26.0 and binds the 8 combo relays (`bindCombo()` in `v2-ui.html`), moves the ROADMAP "every new knob moves its parameter" / `data-i18n` criteria there; Phase 4.1 replaces `Init` with the real bank under a new `.factory-version` stamp **and must delete orphans itself** (the preset manager never does — RESEARCH 2.1); if any range/list changes after v1.0.0, port the v1.0.6 migration hook from O-Prism `PluginProcessor.cpp:525-547`.
- **Depends on:** Task 12
- **Verify:** file exists; every number in it traces to a Task 8–11 output captured this session (memory `pattern_review_recomputes_instead_of_measuring`).

### Task 14 — Commit (path-scoped, no tag)
- **Commands:**
  ```bash
  git branch --show-current                  # main
  git status --short                         # another session's staging must NOT ride along
  git status --short plugins/O-Prism         # empty
  git diff --stat HEAD -- plugins/O-Prism    # empty
  git add plugins/O-Strata PLUGINS.md        # SUMMARY.md and any new smoke file are untracked (memory pattern_git_commit_pathspec_takes_only_tracked_files)
  git commit -m "feat(O-Strata): Stage 1 second pass — 205 live-oscillator parameters, 46 mod destinations, COMPAT-01 re-verified" -- plugins/O-Strata PLUGINS.md
  git show --stat HEAD | head -60
  git tag | grep -i strata                   # empty (memory feedback_never_tag_unless_publish)
  ```
- **Depends on:** Task 13
- **Verify:** `git show --stat` lists only `plugins/O-Strata/**` and `PLUGINS.md`; no tag.

---

## Parallelism

| Wave | Tasks | Note |
|---|---|---|
| 1 | 1 · 2 · 3 · 4 · 6 | independent edits; 5 needs 1 |
| 2 | 5 | after 1 |
| 3 | 7 | one configure, param-dump first, then the three plugin targets in the background |
| 4 | 8 → 9 → 10 · 11 | 8 needs the param-dump binary; 9's `rm` must directly precede its own run; 10 only after 9 asserted the bank; 11 any time after 7 |
| 5 | 12 → 13 → 14 | docs after the numbers are measured |

Removal grep (run after wave 1 and again before Task 14 — must print nothing):
`grep -rn "GeoSource\|GeoFrames\|GeoDrive\|MeshTilt\|VolField\|VolOrbit\|TerSweep\|TerOrbit\|geometryImports\|epitroch\|hypocycl" plugins/O-Strata/Source/ plugins/O-Strata/CMakeLists.txt plugins/O-Strata/.planning/stages/1-foundation/smoke/main.cpp`

---

## Files

**Modified:** `Source/StrataParamIds.h`, `Source/PluginProcessor.cpp`, `Source/dsp/ModulationMatrix.h`, `Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/FactoryPresets.cpp`, `Source/FactoryPresets.h`, `CMakeLists.txt` (comment), `tests/ui_tip_render_check.js` (comment), `CHANGELOG.md`, `NOTES.md`, `.planning/params.tsv`, `.planning/STATUS.md`, `.planning/stages/1-foundation/smoke/main.cpp`, `…/smoke/smoke-output.log`, `PLUGINS.md`
**Created:** `.planning/stages/1-foundation/SUMMARY.md`
**Deleted on disk (dev machine only):** `~/Library/O-Strata/Presets/**` (192 stale JSON), regenerated as `Factory/Init/Init.json` + `.factory-version`
**Not touched:** `Source/ui/public/**`, `Source/StrataVoice.*`, `Source/dsp/WavetableOscillator.*`, `tests/i18n-states.json`, `tests/ui-stub/**`, `parameter-spec.md`, `ROADMAP.md`, `research/ARCHITECTURE.md`, `BRIEF.md`, `REQUIREMENTS.md`, `plugins/O-Prism/**`, root `CMakeLists.txt`

---

## Success criteria (CONTEXT.md second-pass criteria as amended by RESEARCH §5 and Decisions 1–12)

- [ ] `ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone` clean, no new warnings vs the first pass; `./scripts/build-and-install.sh O-Strata` installs with the dual-variant sweep
- [ ] pluginval strictness 10 passes VST3 and AU; `auval -v aumu OuSt OuDv` passes; version `1.0.0` (**COMPAT-01**)
- [ ] `params.tsv` header `# params 205`; ID-keyed diff vs `a774d6d4^` O-Prism registry = removed exactly `oscATable oscBTable`, added 34, **changed in place 20** (`osc?Pos` name / default 0.5, `osc?Unison` max 4, 16 × `modSlot?Dst` numSteps 46 / textAtMax `OscB Saturation`), every other row byte-identical (FUNC-09); the 17 new `oscA*` rows sit between `oscAWarpAmt` and `oscBPos`
- [ ] `modSlot0Dst` exposes 46 choices; indices 0–25 equal O-Prism's strings except 1 / 2 = `OscA Orbit Size` / `OscB Orbit Size`; 26–45 equal the spec table verbatim; `static_assert (NumDests == 46)` compiles (FUNC-05)
- [ ] Choice lists verbatim per the spec (Terrain 7 with `Imported…` last, TerEdge 2, Orbit 11 incl. `Limaçon`, Quality 3 default `2×`); `osc?TerFreq` numSteps 2147483647, `getDefaultValue()` 0.400000, range 0.25 / 8.0 (text columns `0.2500000` / `8.0000000` accepted)
- [ ] `allSliderIds().size() == 166`; `allComboIds().size() == 8`; `sliderAttachments.size() == sliderRelays.size()`; `comboAttachments.size() == comboRelays.size() == 8`; 30 toggle relays and 31 native functions unchanged
- [ ] Removal grep prints nothing; `StrataVoice.cpp` and `Source/ui/**` have no diff
- [ ] Smoke harness green: [1] notes sound on A and B; [2] Scala load; [3] 205-param randomised round-trip into a fresh processor, 0 mismatches, `<terrainImports/>` present and empty, literal 205 == live count; [4] LFO1 → `OscA Terrain Freq` leaves the render sample-identical **and** LFO1 → `Pitch` changes it; [5] on-disk bank = exactly `Init/Init.json` (198 keys, `oscAPos` 0.5, `oscAUnison` 0.0, `oscAQuality` 0.5) + `.factory-version` 1.0.0; [6] choice lists and `TerFreq` range asserted in-process
- [ ] After install + pluginval + auval the bank on disk is still exactly the two files (no regeneration from an old binary)
- [ ] UI gates equal the first-pass baseline (check-i18n PASS, fr CLEAN, zh PASSED, check-ui-labels 20 states, boot-all-uis 0 DEAD / 0 late, tip render 2799 / 106) — page byte-identical
- [ ] `plugins/O-Prism` untouched; `git tag | grep -i strata` empty
- [ ] `CHANGELOG.md` second-pass entry (205 / 46 / last free range change / bank reset / `terrainImports`); `NOTES.md` entry; `STATUS.md` execute ✓; PLUGINS.md row → 🚧 Stage 1; stale-count comment grep empty; `SUMMARY.md` written from measured outputs
- [ ] One path-scoped commit `feat(O-Strata): Stage 1 second pass — 205 live-oscillator parameters, 46 mod destinations, COMPAT-01 re-verified` touching only `plugins/O-Strata/**` and `PLUGINS.md`

---

## Out of scope (do not do in this stage)

Any edit to `index.html` / `i18n.js` / `data-i18n` keys (→ Phase 3.1, D1) · `StrataVoice.cpp` reading the new offsets (→ Phase 2.1, D3) · `TerrainOscillator`, Chebyshev, decimator, latency report (→ Stage 2) · deleting `WavetableOscillator` / `WavetableData` / the reaper (→ Phase 2.1) · real factory presets (→ Phase 4.1) · `terrainImports` content, PNG import, SHA-256 (→ Phase 4.1) · preset-manager migration hook (→ Stage 4 if ever needed) · `stereoWidth` UI · `getLatencySamples` / `setLatencySamples` · any edit to the checksummed contracts or under `plugins/O-Prism/` · Standalone visual check (optional; if done, build `O-Strata_Standalone` explicitly and record it).

---

## Requirements traceability

| ID | Evidence produced by this plan |
|---|---|
| COMPAT-01 | Task 10 pluginval ×2 (strictness 10) + auval on the 205-parameter binary, captured in SUMMARY.md |
| FUNC-09 (IDs) | Task 8 ID-keyed diff: −2 / +34 / 20 in place, rest byte-identical |
| FUNC-05 (list) | Task 3 `static_assert` + Task 8 `modSlot0Dst` row + Task 9 check [4] (46 names, indices 1 / 31 / 45 asserted; a slot routed to index 31 accepted by the matrix) |
| FUNC-10 (smoke) | Task 9 check [2] Scala load + [3] tuning round-trip (formal verification Phase 2.1) |
