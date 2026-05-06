# O-Prism Simplification Audit (v1.17.0)

## Summary
- Files audited: 56 (C++: 23 .cpp + 24 .h; WebView: 1 HTML + 3 JS + 2 CSS; CMakeLists.txt + dsp/MathConstants.h spot-checked)
- Candidates found: 19 (HIGH: 7, MEDIUM: 7, LOW: 5)
- Estimated LOC reduction if all applied: ~2200–2700 LOC

The codebase is in good shape overall — `FactoryPresets.cpp` is already well-factored (archetype + merge pattern), `PrismParamIds.h` consolidates ID strings, and `OuariconPresetManager.h` is a shared module. The two concentrated trouble spots are: (1) **the WebView resources** — three completely-bundled-but-unreferenced files (`tuning-panel.js` 919 LOC, `tuning-panel.css` 612 LOC, `modules/preset-manager.js` 387 LOC) totaling ~1900 dead LOC, plus 64 inline copies of the SVG knob HTML in `index.html`; (2) **`PluginEditor.cpp`** — 5 nearly-identical `setValueNotifyingHost` on `tuningPreset = Custom` copies, ad-hoc JSON-array building in 4 places that already have the `toJsonArray` helper, and 5 `name.replace("\"", "\\\"")` JSON-string-escape duplications. `WavetableFactory.cpp` is also a clear extraction target (20 generators with identical allocate/fill/normalize/mipmap scaffolding), but it's RT-irrelevant (build-time only) so it stays at HIGH severity but Phase 2 because of the per-generator caveats.

### Phase progress

- **Phase 1 (applied v1.17.1, commit c646d0f):** HIGH-01, HIGH-02, HIGH-03 — dead-WebView-files purge + `syncTuningPresetToCustom` helper + `juce::JSON::toString` name escaping. All Risk:LOW + verified by build + auval + visual smoke of the tuning tab.
- **Phase 2 (applied v1.17.2, commit 1c5f5d0):** HIGH-04 (knob HTML scaffold consolidation), HIGH-05 (effect-block param-cache + `runEffect` helper), HIGH-06 (WavetableFactory `buildTable` extraction across 17 of 20 generators — Bitcrush/FM/ChurchBell kept as-is per audit caveat), HIGH-07 (LFO toggle-relay loop consolidation via 3 file-static helpers). Verified by clean release build, AU validation PASSED, fresh AU cache install.
- **Phase 3 (applied v1.17.3, commit a90a5bf):** MEDIUM-01, MEDIUM-02, MEDIUM-07, LOW-01, LOW-02 (covers LOW-05). MEDIUM-03 was already resolved as a side effect of HIGH-05 in Phase 2; MEDIUM-04 subsumed by MEDIUM-01; MEDIUM-05, MEDIUM-06, LOW-03 are explicit "keep" no-ops; LOW-04 skipped (audit underestimated `PrismSound` forward-declaration coupling — see Phase 3 Skipped).

---

## Phase 2 Applied (v1.17.2)

Commit: `1c5f5d0 refactor(O-Prism): v1.17.2 — Phase 2 simplification (HIGH-04/05/06/07)`

| ID | Result |
|----|--------|
| HIGH-04 | 64 inline SVG knob blocks → `data-knob` placeholders + `expandKnobMarkup()` pass. Large refPitch + 2 small footer knobs untouched. ~30 KB binary reduction. |
| HIGH-05 | 25 FX param atomics cached as `OPrismAudioProcessor` members. `runEffect()` template extracted. Per-FX `mix > 0.001f` short-circuit preserved in each configure lambda (load-bearing). |
| HIGH-06 | 17 generators (16 public + `generateFormantTable` static helper) collapsed to per-frame lambdas via `buildTable()`. `generateBitcrush`, `generateFM`, `generateChurchBell` kept in current shape per audit caveat. `rng` + `phaseDist` captured by reference into the lambda for `generateBreath` / `generateWind` / `generateFilteredNoise` so the cross-frame draw sequence stays deterministic. |
| HIGH-07 | 4 LFO sync/free-run loops consolidated via `createToggleRelays` + `addRelayOptions` + `attachToggleRelays` file-static helpers. `bypassRelays` / `modSlotToggleRelays` / `delaySyncRelay` follow the same shape and could fold in here in a future pass — out of scope for this commit. Member-declaration destruction order in `PluginEditor.h:30-35` preserved. |

---

## Phase 3 Applied (v1.17.3)

Commit: `a90a5bf refactor(O-Prism): v1.17.3 — Phase 3 sweep (5 candidates from MEDIUM/LOW tier)`

### Batch A — LOW severity (default-bulk-approved)

| ID | Result |
|----|--------|
| LOW-01 | Updated stale `60 Hz is plenty` comment in `Source/PluginEditor.cpp` to match the actual `startTimerHz(30)` call. |
| LOW-02 | Deleted `currentPitchWheel = 8192;` member from `Source/PrismVoice.h:157` and the two assignment sites in `PrismVoice.cpp` (`startNote` line 162, `pitchWheelMoved` line 658). The JUCE-mandated `currentPitchWheelPosition` parameter on the `startNote` override stays in the signature (renamed to `/*currentPitchWheelPosition*/` to silence unused-param warning). Covers LOW-05. |

### Batch B — MEDIUM severity, LOW risk (default-bulk-approved)

| ID | Result |
|----|--------|
| MEDIUM-01 | Collapsed 4 ad-hoc JSON-array build loops to the existing `toJsonArray` helper. Sites: `startWavetableEditor` harmonics, `getFrameHarmonics`, `getAllEditorFrameWaveforms` (composed nested call), `getPresetListWithCategories` inner names array. Outer object structure of `getPresetListWithCategories` (per-category `firstCat` flag) preserved — emits `{"cat":[..]}` not a simple array. Covers MEDIUM-04. |
| MEDIUM-02 | `getEmbeddedTuningList` JSON now emits `,"period":N` (using `juce::String (t.period, 1)` for one decimal place) after `noteCount`. The dead JS branch at `Source/ui/public/index.html:3304` (`tuning.period && tuning.period !== 1200 ? ...`) is now reachable; non-octave tunings (Bohlen-Pierce, Carlos α/β/γ) display "(NNNN¢ period)". |

### Batch C — MEDIUM severity, MEDIUM risk (per-candidate gate)

| ID | Result |
|----|--------|
| MEDIUM-07 | Extracted `OPrismAudioProcessor::resolveActiveTable (int oscIndex) const` private helper. `updateWavetableAssignments` (audio-thread voice-assignment path) and `getActiveOscTable` (public accessor, also called from `saveEditedWavetable`) now share the single helper for "user pointer takes priority over factory index" lookup. `std::memory_order_relaxed` on `userTablePtrA/B` load preserved verbatim per audit's "Skipped" caveat on factoryTables atomic ownership. `juce::jlimit` clamping bounds (`0` to `factoryTables.size() - 1`) preserved verbatim. |

### Phase 3 Skipped (audit-suggested but not applied)

| ID | Reason |
|----|--------|
| LOW-04 | Audit underestimated this candidate. `PrismSound` is forward-declared in `PrismVoice.h:23`; inlining `canPlaySound` body in the header requires the full type for `dynamic_cast<PrismSound*>`, which would force a new `#include "PrismSound.h"` in `PrismVoice.h` and tighten compile coupling for a 4-line cosmetic refactor. Reverted; `canPlaySound` remains in `PrismVoice.cpp:142-145`. |

### Phase 3 No-Ops (audit candidates flagged "keep as-is")

| ID | Reason |
|----|--------|
| MEDIUM-03 | Already resolved as a side effect of HIGH-05 in Phase 2 (the `runEffect` extraction caches all FX param atomics as members; per Phase 2 Applied table — "25 FX param atomics cached"). |
| MEDIUM-04 | Subsumed by MEDIUM-01 (the nested 2D case in `getAllEditorFrameWaveforms` is handled by composing two `toJsonArray` calls). |
| MEDIUM-05 | Audit explicitly recommended "keep as-is" — `isNoteOn`/`isNoteOff`/etc. else-if chain is the canonical JUCE pattern. |
| MEDIUM-06 | Audit explicitly recommended "already well-factored". |
| LOW-03 | Audit explicitly recommended "keep" — load-bearing destruction-order documentation in `PluginEditor.h`. |
| LOW-05 | Subsumed by LOW-02 (single member-removal covers both entries). |

### Verification

- Clean Release build (macOS VST3 + AU): `ninja O-Prism_VST3 O-Prism_AU` — no new warnings introduced.
- AU validation: `auval -v aumu OuPr OuDv` → PASSED.
- AU cache cleared and fresh binaries installed to `~/Library/Audio/Plug-Ins/{VST3,Components}/` per project CLAUDE.md.
- Spot-check greps:
  - `grep -rn "currentPitchWheel\b"` — only `currentPitchWheelPosition` parameter occurrences remain; zero bare-member references.
  - `grep -n "60 Hz is plenty"` — zero matches.
  - 4 ad-hoc `if (i > 0) json` / `if (f > 0) json` / `if (s > 0) json` / `if (! firstCat) json` patterns reduced from 9 occurrences to 3 (helper-internal at lines 111/124, plus outer object iteration at 908 in `getPresetListWithCategories`).

---

## Candidates

### [HIGH-01] Three completely-unused WebView resource files bundled into binary
- **File:** `Source/ui/public/js/tuning-panel.js` (919 LOC), `Source/ui/public/css/tuning-panel.css` (612 LOC), `Source/ui/public/modules/preset-manager.js` (387 LOC); referenced in `CMakeLists.txt:92-93` and `Source/PluginEditor.cpp:46-52`.
- **Risk:** LOW
- **Type:** dead-code
- **Current:** `index.html` (lines 3406-3407) only loads `wavetable-editor.css/js`; `tuning-panel.js` and `tuning-panel.css` are bundled by `juce_add_binary_data` and exposed via the `getResource` provider, but **never `<link>`-ed or `<script>`-ed by `index.html`**. The entire tuning UI is implemented inline in `index.html` (lines 2705-3394). Likewise, `modules/preset-manager.js` is not in the binary-data list at all and never imported by anyone.
  ```cmake
  # CMakeLists.txt:92-93 (currently bundled but unused at runtime)
  Source/ui/public/js/tuning-panel.js
  Source/ui/public/css/tuning-panel.css
  ```
  ```cpp
  // PluginEditor.cpp:46-52 (resource handlers for unreferenced files)
  if (url == "/js/tuning-panel.js")
      return makeBinaryResource (BinaryData::tuningpanel_js, ...);
  if (url == "/css/tuning-panel.css")
      return makeBinaryResource (BinaryData::tuningpanel_css, ...);
  ```
  Verification: `grep -rn "tuning-panel\.js" plugins/O-Prism/` shows references only in CMakeLists.txt, the resource-handler block in PluginEditor.cpp, and self-references inside the dead files' own docstrings. `index.html` has exactly two `<script>` tags (`<script type="module">` inline + `wavetable-editor.js`) and one `<link>` (`wavetable-editor.css`).
- **Proposed:** Delete `Source/ui/public/js/tuning-panel.js`, `Source/ui/public/css/tuning-panel.css`, and `Source/ui/public/modules/preset-manager.js` from disk. Remove the two `juce_add_binary_data` SOURCES lines in `CMakeLists.txt:92-93`. Remove the two `if (url == "/js/tuning-panel.js")` / `if (url == "/css/tuning-panel.css")` blocks in `PluginEditor.cpp:46-52`.
- **Rationale:** ~1918 LOC of source eliminated (and ~30 KB out of the plugin binary). Eliminates a refactor footgun: anyone editing `tuning-panel.js` to "fix the tuning panel" would be silently editing dead code while the inline copy in `index.html` is what actually runs. The `tuning-panel.js` even still contains the buggy docstring `window.__JUCE__` reference that the project memory flagged as a recurring trap.
- **Test impact:** Verify the tuning tab still renders, knobs work, library loads, generators work (visual smoke). Render-harness will have no diffs since these files are not used at runtime.

### [HIGH-02] `setValueNotifyingHost(... kCustomTuningPresetIndex ...)` repeated 5x verbatim
- **File:** `Source/PluginEditor.cpp:122-123, 144-145, 247-248, 402-403, 424-425`
- **Risk:** LOW
- **Type:** duplication
- **Current:** Same 2-line block appears at every native-function that mutates the active tuning, so the APVTS choice param is forced to "Custom" for persistence:
  ```cpp
  if (auto* param = processorRef.getAPVTS().getParameter ("tuningPreset"))
      param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (PrismParamIds::kCustomTuningPresetIndex)));
  ```
  Sites: `setTuningIntervals` (l122-123), `setSingleInterval` (l144-145), `loadScalaFile` success branch (l247-248), `loadEmbeddedTuning` (l402-403), `applyGeneratedScale` (l424-425).
- **Proposed:** Extract a single helper at the top of the file:
  ```cpp
  static void syncTuningPresetToCustom (juce::AudioProcessorValueTreeState& apvts)
  {
      if (auto* param = apvts.getParameter ("tuningPreset"))
          param->setValueNotifyingHost (param->convertTo0to1 (
              static_cast<float> (PrismParamIds::kCustomTuningPresetIndex)));
  }
  ```
  Replace each of the 5 sites with `syncTuningPresetToCustom (processorRef.getAPVTS());`.
- **Rationale:** ~10 LOC saved, but the bigger win is eliminating a "one-of-N copies drifts" risk: if the kCustomTuningPresetIndex semantics ever change (e.g., custom slot moves to a different index), all 5 sites must be updated together.
- **Test impact:** Identical APVTS state after each tuning mutation; existing render-harness tests should pass unchanged.

### [HIGH-03] `name.replace("\"", "\\\"")` JSON-string-escape duplicated in 5 sites
- **File:** `Source/PluginEditor.cpp:525, 549, 583, 647, 859`
- **Risk:** LOW
- **Type:** duplication
- **Current:** Same JSON-string-escape pattern at every native-function that returns a name in JSON:
  ```cpp
  // l525 (getUserWavetableList)
  return "\"" + s.replace ("\"", "\\\"") + "\"";
  // l549 (importUserWavetable success)
  complete ("{\"success\":true,\"name\":\"" + name.replace ("\"", "\\\"") + "\"}");
  // l583 (importUserWavetableData success)
  complete ("{\"success\":true,\"name\":\"" + name.replace ("\"", "\\\"") + "\"}");
  // l647 (getActiveOscInfo isUser branch)
  complete ("{\"isUser\":true,\"name\":\"" + name.replace ("\"", "\\\"") ...);
  // l859 (saveEditedWavetable)
  complete ("{\"success\":true,\"name\":\"" + name.replace ("\"", "\\\"") + "\"}");
  ```
  Note: this naive escaping doesn't handle backslashes or control characters — `juce::JSON::toString` does. Sites at `891-908` (preset list) already use `juce::JSON::toString` for that reason.
- **Proposed:** Replace all 5 sites with `juce::JSON::toString (name)` (which produces `"escaped name"` including the surrounding quotes):
  ```cpp
  // l549 example
  complete ("{\"success\":true,\"name\":" + juce::JSON::toString (name) + "}");
  ```
- **Rationale:** Unified format — one of the 5 sites is bug-prone if a user wavetable name ever contains a backslash, tab, or newline. The `juce::JSON::toString` path handles all RFC-7159 escapes. **Mild semantic change** (proper escaping of backslash etc.) that is strictly safer than today.
- **Test impact:** Visual smoke: import a user wavetable, verify name displays correctly in the dropdown. A wavetable named `My"Quote".wav` should render as `My"Quote"` in the UI without breaking JSON parse.

### [HIGH-04] 64 inline copies of the SVG knob HTML scaffold (~30 KB)
- **File:** `Source/ui/public/index.html:986-994, 1001, 1040-1048, 1055, 1082, 1094, 1100, 1107, 1123-1126, 1146-1149, 1163-1167, 1185-1188, 1212-1215, 1229-1230, 1238, 1245, 1268-1273, 1280-1285, 1425-1426, 1437, 1445-1447, 1461-1462, 1470-1475, 1483-1486, 1563, 1568` (every `class="knob-container"` line, 64 total)
- **Risk:** MEDIUM
- **Type:** verbose-pattern
- **Current:** Each of the 64 standard knobs is an identical 1-line block (~250 chars) that varies only in 5 places: param ID, label, default-value text, optional `small` modifier, and (for footer/refPitch) the dasharray length:
  ```html
  <div class="knob-container"><div class="knob" id="knob-oscAPos"><div class="knob-visual"><svg viewBox="0 0 52 52"><circle class="knob-track" cx="26" cy="26" r="22"/><circle class="knob-vine" id="vine-oscAPos" cx="26" cy="26" r="22" stroke-dasharray="103.67" stroke-dashoffset="103.67"/></svg></div><span class="knob-label">Position</span><span class="knob-value" id="val-oscAPos">0%</span></div></div>
  ```
  60 are size=standard, 2 are size=small (footer), 2 are referenced in special positions (`vine-refPitch` is its own large `<svg viewBox="0 0 64 64">` block at l1351-1360 — keep that one separate).
- **Proposed:** Replace each `<div class="knob-container">…</div>` with a thin custom-element placeholder that JS expands at startup, e.g.:
  ```html
  <div class="knob-container" data-knob="oscAPos" data-label="Position" data-initial="0%"></div>
  ```
  Add a single `expandKnobMarkup()` pass at the top of the script (right before the existing knob-binding loop) that walks `[data-knob]` and emits the SVG scaffold. Keeps the existing `bindKnob('oscAPos', pct, 0.0)` call sites unchanged.
- **Rationale:** ~30 KB out of the index.html binary blob, ~15× DOM verbosity reduction. Any future visual change to the knob (track stroke, vine curve, value-tick marks) requires editing 64 lines today vs. 1 helper.
- **Test impact:** Visual smoke is mandatory — open the plugin, verify all 64 knobs render with correct labels and initial values. The refPitch knob (l1351, large size, distinct svg viewBox) should be left untouched. The `vine-` and `val-` IDs must remain identical so the existing `bindKnob` / `valueChangedEvent` handlers continue to work.

### [HIGH-05] Effect-block parameter-fetch + setter pattern repeated 5x in `processBlock`
- **File:** `Source/PluginProcessor.cpp:629-707`
- **Risk:** MEDIUM
- **Type:** verbose-pattern
- **Current:** Each FX block (Distortion, Chorus, Delay, Reverb, EQ) follows the same 8-12 line shape:
  ```cpp
  bool distBypassed = parameters.getRawParameterValue ("distBypass")->load() > 0.5f;
  if (! distBypassed)
  {
      int distType = static_cast<int> (parameters.getRawParameterValue ("distType")->load());
      float distDrive = parameters.getRawParameterValue ("distDrive")->load();
      float distMix = parameters.getRawParameterValue ("distMix")->load();
      distortion.setType (distType);
      distortion.setDrive (distDrive);
      distortion.setMix (distMix);
      if (distMix > 0.001f)
          distortion.process (block);
  }
  ```
  Sites: l629-641 (Distortion), l643-655 (Chorus), l657-671 (Delay), l673-691 (Reverb), l693-707 (EQ).
- **Proposed:** Cache the bypass param atomic pointers as members in the processor and extract a small helper for the common bypass-and-process pattern:
  ```cpp
  template <typename FX, typename ConfigureFn>
  static void runEffect (std::atomic<float>* pBypass, FX& fx,
                         juce::dsp::AudioBlock<float>& block,
                         ConfigureFn&& configure)
  {
      if (pBypass->load() > 0.5f) return;
      configure (fx);
      fx.process (block);
  }
  ```
- **Rationale:** ~30 LOC saved. Eliminates 5 hash-map lookups per block (currently 30+ probes); caching aligns with the existing voice-side caching pattern in `PrismVoice.h:70-152`.
- **Test impact:** Render-harness identity expected — same params read, same process calls in same order. **Caveat:** the per-FX `mix > 0.001f` gate currently skips `process()` entirely when mix is ~0. The proposed helper drops this short-circuit. Verify the FX `process()` paths are RT-safe at mix=0; if not, keep the gate inside the `configure` callback. **Risk-tagged MEDIUM because of this.**

### [HIGH-06] WavetableFactory: 20 generator functions share 5-line allocate/fill/normalize/mipmap scaffolding
- **File:** `Source/dsp/WavetableFactory.cpp:145-917` (all 20 `generate*` functions)
- **Risk:** LOW (per-generator caveats below — defer to Phase 2 for safety despite the Risk:LOW tag)
- **Type:** verbose-pattern
- **Current:** Every generator follows this skeleton:
  ```cpp
  std::unique_ptr<WavetableData> WavetableFactory::generateXxx (int numFrames)
  {
      auto table = std::make_unique<WavetableData>();
      table->allocate (numFrames);
      for (int f = 0; f < numFrames; ++f)
      {
          float* buf = table->getFrameData (0, f);
          std::fill (buf, buf + WavetableData::kTableSize, 0.0f);  // 17 of 20 do this
          // ...per-frame body that fills `buf`...
          normalizeFrame (buf, WavetableData::kTableSize);  // 20 of 20
      }
      WavetableGenerator::generateMipmaps (*table);  // 20 of 20
      return table;
  }
  ```
- **Proposed:** Extract a frame-loop helper that takes a per-frame lambda:
  ```cpp
  template <typename Fn>
  static std::unique_ptr<WavetableData> buildTable (int numFrames, Fn&& fillFrame)
  {
      auto table = std::make_unique<WavetableData>();
      table->allocate (numFrames);
      for (int f = 0; f < numFrames; ++f)
      {
          float* buf = table->getFrameData (0, f);
          std::fill (buf, buf + WavetableData::kTableSize, 0.0f);
          fillFrame (f, buf);
          normalizeFrame (buf, WavetableData::kTableSize);
      }
      WavetableGenerator::generateMipmaps (*table);
      return table;
  }
  ```
  Each generator collapses to its per-frame body.
- **Rationale:** ~60-80 LOC saved across 20 functions. Build-time only (called once at processor construction) so RT-safety is irrelevant.
- **Test impact:** Render-harness identity required — every preset plays the same. The 3 generators that *don't* use the standard pre-fill (`generateBitcrush` builds a `sawBuf` source first, `generateFM` writes directly into `buf`, `generateChurchBell` uses inharmonic ratios) need careful inspection — `generateFM` doesn't `std::fill` because every sample is written; it can keep its current shape or use a `buildTableNoPrefill` overload.

### [HIGH-07] LFO sync + free-run toggle relay/attachment loops are duplicated 4x
- **File:** `Source/PluginEditor.cpp:1006-1020, 1038-1053, 1092-1115`; declarations in `Source/PluginEditor.h:40-43, 51-54`
- **Risk:** LOW
- **Type:** duplication
- **Current:** Two parallel toggle-relay vectors (`lfoSyncRelays`, `lfoFreeRunRelays`) and two parallel attachment vectors are populated by 4 identical loops:
  ```cpp
  // Relay creation (l1007-1012)
  for (int i = 1; i <= 4; ++i)
      lfoSyncRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> ("lfo" + juce::String (i) + "Sync"));
  for (int i = 1; i <= 4; ++i)
      lfoFreeRunRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> ("lfo" + juce::String (i) + "FreeRun"));

  // withOptionsFrom (l1039-1044) — same shape
  // Attachment creation (l1093-1115) — identical except for vector + paramId suffix
  ```
  The same 3-step pattern (relay → options-from → attachment) also exists for `bypassRelays`, `modSlotToggleRelays`, and `delaySyncRelay`.
- **Proposed:** Build all toggle-button relays/attachments through a single helper that takes a `StringArray` of param IDs:
  ```cpp
  static void registerToggles (juce::AudioProcessorValueTreeState& apvts,
                               const juce::StringArray& ids,
                               std::vector<std::unique_ptr<juce::WebToggleButtonRelay>>& relays,
                               std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>>& attachments,
                               juce::WebBrowserComponent::Options& options) { ... }
  ```
- **Rationale:** ~25-30 LOC saved across the 4 loops + the 2 `lfoSyncRelays` / `lfoFreeRunRelays` vector members can collapse into the existing `bypassRelays`/`modSlotToggleRelays` pattern.
- **Test impact:** Visual smoke for LFO sync + free-run buttons (toggle them, verify state syncs to APVTS). Render-harness identity expected.

### [MEDIUM-01] `toJsonArray` helper exists but ad-hoc JSON-array building used in 4 sites
- **File:** `Source/PluginEditor.cpp:714-722, 766-772, 866-887, 1192-1196`
- **Risk:** LOW
- **Type:** verbose-pattern
- **Current:** A `toJsonArray` template helper (l69-82) and `toJsonFloatArray` helper (l84-94) already exist, but 4 sites still build the array by hand:
  ```cpp
  // l714-722 (startWavetableEditor harmonics)
  juce::String harmJson = "[";
  for (size_t i = 0; i < harmonics.size(); ++i)
  {
      if (i > 0) harmJson += ",";
      harmJson += juce::String (harmonics[i], 4);
  }
  harmJson += "]";
  // l766-772 (getFrameHarmonics) — same 7 lines verbatim
  // l866-887 (getAllEditorFrameWaveforms) — nested 2D array using same shape
  // l1192-1196 (timer callback) — uses toJsonArray correctly already
  ```
- **Proposed:** Replace the 3 ad-hoc loops (l714-722, l766-772, l873-883) with `toJsonArray` calls:
  ```cpp
  juce::String harmJson = toJsonArray (harmonics, [] (float v) { return juce::String (v, 4); });
  ```
- **Rationale:** ~15 LOC saved. The helper exists for exactly this purpose.
- **Test impact:** Open the wavetable editor tab, verify harmonic bars render. Identical JSON output expected.

### [MEDIUM-02] Stale `tuning.period` reference in `renderLibraryList` — C++ JSON doesn't emit `period`
- **File:** `Source/ui/public/index.html:3279`; relevant C++ at `Source/PluginEditor.cpp:369-379` (`getEmbeddedTuningList` native fn)
- **Risk:** LOW
- **Type:** stale-comment / dead-code-path
- **Current:** JS tries to display the period for non-octave tunings:
  ```js
  // index.html:3279
  const periodNote = tuning.period && tuning.period !== 1200 ? ' (' + tuning.period.toFixed(0) + '¢ period)' : '';
  ```
  But the C++ JSON emit at `PluginEditor.cpp:373-377` only includes `id, name, category, noteCount` — never `period`. So `tuning.period` is always `undefined` and Bohlen-Pierce / Carlos tunings always fall back to the empty string instead of showing "(1902¢ period)".
- **Proposed:** Add `period` to the JSON output:
  ```cpp
  return "{\"id\":\"" + juce::String (t.id)
       + "\",\"name\":\"" + juce::String (t.name)
       + "\",\"category\":\"" + juce::String (t.category)
       + "\",\"noteCount\":" + juce::String (static_cast<int> (t.intervals.size()))
       + ",\"period\":" + juce::String (t.period, 1)
       + "}";
  ```
  (Or remove the dead JS branch — but emitting period seems to be the original intent.)
- **Rationale:** Either the JS or the C++ is wrong; today the feature is silently broken. **Mild semantic change** — Bohlen-Pierce will now show its period in the library list, which is correct.
- **Test impact:** Open the tuning library, scroll to a non-octave tuning (e.g., Bohlen-Pierce in the "Non-Octave" filter), verify it shows "(1902¢ period)". No render-harness impact.

### [MEDIUM-03] Per-FX param atomics fetched via hash-lookup in `processBlock` instead of cached pointers
- **File:** `Source/PluginProcessor.cpp:580-714` (multiple sites)
- **Risk:** MEDIUM
- **Type:** verbose-pattern (RT-perf adjacent)
- **Current:** `processBlock` calls `parameters.getRawParameterValue (id)->load()` ~30 times per block (5 master/tuning params + 4-7 per FX × 5 FX). Each lookup is a hash-map probe. PrismVoice already caches all its param pointers in `setAPVTS()`; the processor should follow the same pattern.
- **Proposed:** Add cached `std::atomic<float>*` members to `OPrismAudioProcessor` (next to the existing `parameters` member) for the 30+ params read in `processBlock`. Initialize them in the constructor.
- **Rationale:** ~30 hash-map lookups per block eliminated. Source code becomes shorter and matches the established voice-side pattern. **Risk-tagged MEDIUM** because it touches `processBlock` and any pointer-init bug becomes a null-deref during host bring-up.
- **Test impact:** auval clean, render-harness identity. Verify state restoration via `setStateInformation` still works (cached pointers stay valid since APVTS doesn't reallocate parameter memory after construction).

### [MEDIUM-04] `if i == 0 / i > 0 ? add comma else ""` JSON-comma loop pattern repeated 4x in PluginEditor
- **File:** `Source/PluginEditor.cpp:715-720, 767-771, 873-883, 894-908, 938-944`
- **Risk:** LOW
- **Type:** verbose-pattern
- **Current:** This `if (! first) json += ","; first = false;` (or `if (i > 0) json += ",";`) pattern appears in 4 places that *don't* go through `toJsonArray`:
  ```cpp
  // l873-883 (nested 2D wavetable JSON)
  juce::String json = "[";
  for (size_t f = 0; f < allWaveforms.size(); ++f)
  {
      if (f > 0) json += ",";
      json += "[";
      for (size_t s = 0; s < allWaveforms[f].size(); ++s)
      {
          if (s > 0) json += ",";
          json += juce::String (allWaveforms[f][s], 3);
      }
      json += "]";
  }
  json += "]";
  ```
- **Proposed:** Add a 2D overload of `toJsonArray` (or use nested calls) — see MEDIUM-01.
- **Rationale:** Subsumed by MEDIUM-01; tagged separately because the nested-loop case is its own readability win.
- **Test impact:** Same as MEDIUM-01.

### [MEDIUM-05] `processBlock` MIDI-state extraction uses 5 separate else-if branches; could be flattened
- **File:** `Source/PluginProcessor.cpp:605-618`
- **Risk:** LOW
- **Type:** verbose-pattern
- **Current:**
  ```cpp
  for (const auto metadata : midiMessages)
  {
      auto msg = metadata.getMessage();
      if (msg.isNoteOn())                          /* noteStates store true  */
      else if (msg.isNoteOff())                    /* noteStates store false */
      else if (msg.isAllNotesOff() || ...)         /* clear all */
      else if (msg.isController() && cc==1)        /* modWheel store */
      else if (msg.isChannelPressure())            /* aftertouch store */
  }
  ```
- **Proposed:** **Keep as-is.** This is correctly clear code — the chain of `isNoteOn` / `isNoteOff` / etc. is the canonical JUCE pattern. Listing it for completeness so the audit captures that it was reviewed and is intentional.
- **Rationale:** No-op recommendation — listing for false-positive transparency.
- **Test impact:** N/A — no change.

### [MEDIUM-06] `Source/PluginProcessor.cpp` create*Parameters helpers could share a builder, but already well-structured
- **File:** `Source/PluginProcessor.cpp:22-403`
- **Risk:** LOW
- **Type:** verbose-pattern
- **Current:** `createOscParameters`, `createSubNoiseParameters`, `createAmpEnvelopeParameters`, etc. — 13 helper functions of 5-30 LOC each.
- **Proposed:** **Already well-factored.** The grouping is informative and the current `addSection` lambda in `createParameterLayout` is clean. Listing for completeness.
- **Rationale:** No-op. Reducing further would obscure semantically-grouped param defaults.
- **Test impact:** N/A.

### [MEDIUM-07] `getActiveOscTable` and `updateWavetableAssignments` duplicate the user-vs-factory selection logic
- **File:** `Source/PluginProcessor.cpp:742-790, 828-848`
- **Risk:** MEDIUM
- **Type:** duplication
- **Current:** Both functions re-implement the same "user-pointer takes priority over factory index" lookup:
  ```cpp
  // updateWavetableAssignments (l747-773)
  auto* currentUserA = userTablePtrA.load (std::memory_order_relaxed);
  if (currentUserA != nullptr) targetA = currentUserA;
  else { int oscATable = juce::jlimit (0, numFactoryTables - 1,
            static_cast<int> (parameters.getRawParameterValue ("oscATable")->load()));
         targetA = factoryTables[static_cast<size_t> (oscATable)].get(); }

  // getActiveOscTable (l830-847) — same pattern, separate copy for index 0 / 1
  ```
- **Proposed:** Extract a private helper `const WavetableData* resolveActiveTable (int oscIndex) const` that does the user-or-factory lookup, then have `updateWavetableAssignments` call it for both A and B.
- **Rationale:** ~15 LOC saved. Single source of truth for the user-takes-priority rule. **Risk-tagged MEDIUM** because the helper participates in the audio-thread voice-assignment path.
- **Test impact:** Render-harness identity required. Visual smoke: A/B tabs of every dropdown, including user-table override.

### [LOW-01] Stale comment "60 Hz is plenty for UI updates" but timer is set to 30 Hz
- **File:** `Source/PluginEditor.cpp:1150-1151`
- **Risk:** LOW
- **Type:** stale-comment
- **Current:**
  ```cpp
  // Start polling for active MIDI notes (60 Hz is plenty for UI updates)
  startTimerHz (30);
  ```
- **Proposed:** Update comment to match the actual 30 Hz timer.
- **Rationale:** ~1 LOC churn. Avoids confusing future reader who greps for "60 Hz" expecting a 60 Hz timer.
- **Test impact:** None.

### [LOW-02] `currentPitchWheel` member of `PrismVoice` is set but never read
- **File:** `Source/PrismVoice.h:157`, `Source/PrismVoice.cpp:158, 162, 658`
- **Risk:** LOW
- **Type:** dead-code
- **Current:** `currentPitchWheel` is initialized at l157 (header), assigned at `startNote` (l162) and `pitchWheelMoved` (l658), but never read anywhere in the file.
- **Proposed:** Delete the `currentPitchWheel` member declaration and the two assignment sites. Leave the `currentPitchWheelPosition` parameter to `startNote` (JUCE override signature).
- **Rationale:** ~3 LOC dead-code removed. Misleading — suggests the voice tracks pitch-wheel state when it actually doesn't.
- **Test impact:** None — pitch-wheel still works.

### [LOW-03] `PluginEditor.h` member-order comment block is informative but oversized
- **File:** `Source/PluginEditor.h:30-35`
- **Risk:** LOW
- **Type:** verbose-pattern
- **Current:**
  ```cpp
  // ═══════════════════════════════════════════════════════════════════
  // CRITICAL: Member declaration order (C++ destroys in reverse)
  // 1. Relays destroyed LAST
  // 2. WebView destroyed SECOND
  // 3. Attachments destroyed FIRST (WebView still alive — safe)
  // ═══════════════════════════════════════════════════════════════════
  ```
- **Proposed:** **Keep the comment** — it explains a load-bearing constraint. Listing only to confirm it was reviewed and intentional.
- **Rationale:** No-op. False-positive check.
- **Test impact:** N/A.

### [LOW-04] `PrismVoice::canPlaySound` could be inlined into the header
- **File:** `Source/PrismVoice.cpp:142-145`
- **Risk:** LOW
- **Type:** verbose-pattern
- **Current:**
  ```cpp
  bool PrismVoice::canPlaySound (juce::SynthesiserSound* sound)
  {
      return dynamic_cast<PrismSound*> (sound) != nullptr;
  }
  ```
- **Proposed:** Move the one-liner into the header. Trivial refactor that reduces .cpp clutter.
- **Rationale:** ~4 LOC moved. Minor.
- **Test impact:** None.

### [LOW-05] PluginProcessor.cpp `currentPitchWheelPosition` plumbing
- **File:** `Source/PrismVoice.cpp:158-162` (already covered by LOW-02 — listing here only because the JUCE override signature requires it)
- **Risk:** LOW
- **Type:** stale-comment
- **Current:** Same as LOW-02 — the `currentPitchWheelPosition` parameter is part of the JUCE `SynthesiserVoice::startNote` override and **must** stay in the signature. The stored `currentPitchWheel` member is the dead part.
- **Proposed:** Subsumed by LOW-02. Listing for completeness.
- **Rationale:** N/A.
- **Test impact:** N/A.

---

## Skipped (false-positive checks)

- **`#if/#else atomic_load/atomic_store`:** No instances found in `Source/`. (The HIGH-01 from the prior O-MicrotonalSampler audit doesn't apply here.)
- **`std::tanh` soft-clipping in `NoiseGenerator.cpp:73, 77, 124, 125, 147, 148`:** Intentional per Stage 4 Task 1 — do not flag as "could use a faster approx."
- **`window.__JUCE__` vs `Juce` namespace** in tuning-panel.js: file is dead code (HIGH-01); skip the namespace docstring concern.
- **WebView2 `#if JUCE_WINDOWS` guard + `withUserDataFolder`** at PluginEditor.cpp:1057-1063: Windows DAW-host-specific, load-bearing per project memory.
- **`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`** in CMakeLists.txt:17, 108: load-bearing per project memory.
- **PrismVoice modulation matrix per-sample loop** at `PrismVoice.cpp:451-653`: Intentional — RT hot path, every line is justified by the 16-slot mod matrix design. Helper extraction here would obscure the audio chain.
- **`pendingTuningSource` / `applyPendingTuning` in `PrismVoice::startNote`** (l187-195): VST3 Note Expression / Dorico microtonal — load-bearing per Phase 24 propagation memory; do not touch.
- **CSS `:nth-child(even)` on `.mod-row`** at `index.html:265`: striped-row visual; harmless because mod-rows are dynamically created by JS in a single sequence (no positional fragility).
- **CSS `:last-child border-bottom: none`** patterns at `index.html:203, 855` and `tuning-panel.css:108`: standard "last item gets no border" idiom; not a refactor blocker.
- **`startTimerHz(30)`** at `PluginEditor.cpp:1151`: tagged as LOW-01 (stale comment); the timer rate itself is intentional.
- **`PluginProcessor::getTailLengthSeconds() = 5.0`** at `PluginProcessor.h:50`: voice-allocator/tail-buffer sensitive; do not touch.
- **`setLatencySamples(static_cast<int>(distortion.getLatencyInSamples()))`** at `PluginProcessor.cpp:555`: per project memory, `getLatencySamples()` is non-virtual in JUCE 8 — `setLatencySamples` is correct. Do not "improve."
- **`factoryTables` ownership / `userTablePtrA/B` atomic swap** in `OPrismAudioProcessor`: deliberately atomic for RT safety; the load/store-relaxed pattern at `PluginProcessor.cpp:747-748, 805-810` etc. is intentional.
