# O-MicrotonalSampler Simplification Audit (v1.16.6)

## Summary
- Files audited: 14 (8 C++ source/header, 2 JS, 1 HTML, 2 CSS skim, plus minor headers)
- Candidates found: 22 (HIGH: 6, MEDIUM: 9, LOW: 7)
- Estimated LOC reduction if all applied: ~250–350 LOC

The codebase is in good shape overall — visible evidence of multiple cleanup passes (v1.12.4 native-fn registry extraction, v1.13.0 drop-streaming module extraction, v1.12.3 RR-counter constant hoist). The biggest remaining wins are in the C++ atomic-shared-ptr `#if/#else` boilerplate (11 sites in PluginProcessor.cpp) and a handful of JSON-building / dialog-cleanup duplications that have outlived their context.

### Phase progress
- **Phase 1 (v1.16.7, commit `a6e0a3b`):** HIGH-01, HIGH-03, HIGH-04, HIGH-05 — atomic-shared-ptr / JSON / bool-arg helpers extracted.
- **Phase 2 (v1.16.8, commit `023cd4a`):** HIGH-02, HIGH-06 — loopMode helpers + JS-driven knob render. (See `## Phase 2 Applied` below.)
- **Phase 3 (v1.16.9, commit `9293470`):** Batch A only — LOW-01, LOW-03, LOW-05, LOW-07. LOW-04 + LOW-06 re-verified as false positives. Batches B (MEDIUM/LOW-risk dedups) and C (MEDIUM-04 dialog modal helper) deferred. (See `## Phase 3 Applied` and `## Phase 3 Skipped` below.)
- **Auto-resolved by Phase 1 side-effect:** MEDIUM-01 (the `__cpp_lib_atomic_shared_ptr >= 202002L` dead-code check at former lines 510–516). Folded into the `atomicLoad` helper extraction.

Remaining open candidates: MEDIUM-02, MEDIUM-03, MEDIUM-04, MEDIUM-05, MEDIUM-06, MEDIUM-07, MEDIUM-08, MEDIUM-09 (all in the Candidates section below). Estimated LOC saved if all applied: ~280 LOC.

---

## Candidates

### [HIGH-01] Collapse 11 atomic-shared-ptr `#if/#else` blocks behind a helper
- **File:** `Source/PluginProcessor.cpp` (lines 383–388, 855–859, 960–964, 1324–1328, 1433–1437, 1515–1519, 1616–1620, 1648–1652, 1656–1660, plus 511–516 and 514–516 for CC/PC tables)
- **Risk:** LOW
- **Type:** duplication / verbose-pattern
- **Current:** Every `currentSampleMap` / `currentCcMapping` / `currentPcMapping` access is wrapped in:
  ```cpp
  #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
      auto x = std::atomic_load (&currentSampleMap);
  #else
      auto x = currentSampleMap;
  #endif
  ```
  Both branches actually compile cleanly under JUCE 8 + C++20 — the `else` branch is a non-atomic fallback that the project never exercises. The CC/PC version (lines 510–516) even has identical content in both branches, betraying the dead conditional.
- **Proposed:** Add two file-local helpers in an anonymous namespace at the top of `PluginProcessor.cpp`:
  ```cpp
  template <class T> std::shared_ptr<T> atomicLoad (std::shared_ptr<T>& slot)
                  { return std::atomic_load (&slot); }
  template <class T> void atomicStore (std::shared_ptr<T>& slot, std::shared_ptr<T> v)
                  { std::atomic_store (&slot, std::move (v)); }
  ```
  Replace every `#if/#else/#endif` site with one-line calls. Removes ~50 lines and eliminates a branch readers always have to mentally evaluate.
- **Rationale:** The `__cpp_lib_atomic_shared_ptr` branch is a no-op safety net — both arms produce equivalent compiled code on the toolchains this project ships against. Removing the conditional documents the actual contract (shared_ptr atomics) without changing behaviour.
- **Test impact:** Render-harness identity test, state save/load round-trip, single-cell replace test — all should pass unchanged.

### [HIGH-03] Extract repeated `notes/freqs` JSON builder
- **File:** `Source/PluginEditor.cpp:344–356` (timer callback) and `Source/PluginEditor.cpp:715–725` (`getHeldNotesJson` native fn)
- **Risk:** LOW
- **Type:** duplication
- **Current:** Two essentially identical 12-line blocks build `{"notes":[...],"freqs":[...]}` from a parallel pair of `std::vector<int>`/`std::vector<double>`.
- **Proposed:** Add a file-local helper:
  ```cpp
  static juce::String buildNotesFreqsJson (const std::vector<int>& notes,
                                           const std::vector<double>& freqs)
  {
      juce::String n = "[", f = "[";
      for (size_t i = 0; i < notes.size(); ++i) {
          if (i > 0) { n += ","; f += ","; }
          n += juce::String (notes[i]);
          f += juce::String (freqs[i], 4);
      }
      n += "]"; f += "]";
      return "{\"notes\":" + n + ",\"freqs\":" + f + "}";
  }
  ```
  Both call sites collapse to one line.
- **Rationale:** Same data, same shape, two places. Saves ~16 lines and locks the format in one spot.
- **Test impact:** TuningPanel TrueKeys / Circle / Polar visualisations should keep working; payload format is unchanged.

### [HIGH-04] Extract intervals-to-JSON builder used 4× in native-fn registry
- **File:** `Source/PluginEditor.cpp:651–668` (getTuningIntervals), `1505–1525` (generateEDO), `1529–1550` (generateHarmonicSeries), `1552–1575` (generateRank2)
- **Risk:** LOW
- **Type:** duplication
- **Current:** Each of these four native-fn handlers contains the same shape:
  ```cpp
  juce::String json = "[";
  for (size_t i = 0; i < intervals.size(); ++i)
  { if (i > 0) json += ","; json += juce::String (intervals[i], 6); }
  json += "]";
  complete (juce::var (json));
  ```
- **Proposed:** Anonymous-namespace helper:
  ```cpp
  static juce::String centsArrayToJson (const std::vector<double>& v)
  { /* ... loop with comma separator ... */ }
  ```
  Each native fn becomes 2–3 lines of arg parsing + one helper call.
- **Rationale:** Saves ~30 lines and ensures all 4 endpoints emit byte-identical formatting (the 6-digit precision is currently load-bearing — tested in JS via `parseFloat` so any drift is invisible until cents diverge).
- **Test impact:** Tuning panel intervals readout, scale-generator preview tabs.

### [HIGH-05] Extract trivial setValueNotifyingHost-bool wrapper
- **File:** `Source/PluginEditor.cpp:1911–1925` (setKeyswitchEnabled), `2013–2027` (setCcEnabled), `2063–2078` (setPcEnabled)
- **Risk:** LOW
- **Type:** duplication
- **Current:** Three native fns all contain:
  ```cpp
  if (args.size() < 1) { complete (juce::var (false)); return; }
  auto& apvts = processorRef.getAPVTS();
  if (auto* p = apvts.getParameter ("ks_enabled"))
  { p->setValueNotifyingHost (static_cast<bool>(args[0]) ? 1.0f : 0.0f);
    complete (juce::var (true)); return; }
  complete (juce::var (false));
  ```
- **Proposed:** Add a file-local helper accepting an APVTS reference, a parameter id, and the args, returning `bool`. Each native fn body collapses to ~3 lines.
- **Rationale:** ~30 LOC saved. Centralises the "bool-arg → param" idiom — future params (hypothetical e.g. `lfo_enabled`) get one-line handlers.
- **Test impact:** KS/CC/PC enable toggles in UI.

### [MEDIUM-01] Drop the dead `__cpp_lib_atomic_shared_ptr >= 202002L` check
- **File:** `Source/PluginProcessor.cpp:510–516`
- **Risk:** LOW
- **Type:** dead-code
- **Current:**
  ```cpp
  #if (defined(__cplusplus) && __cplusplus >= 202002L)
      auto ccTable = std::atomic_load (&currentCcMapping);
      auto pcTable = std::atomic_load (&currentPcMapping);
  #else
      auto ccTable = std::atomic_load (&currentCcMapping);
      auto pcTable = std::atomic_load (&currentPcMapping);
  #endif
  ```
  Both arms are identical (look closely — same code, no fallback distinction). The `#if` is purely scaffolding.
- **Proposed:** Drop the conditional entirely:
  ```cpp
  auto ccTable = std::atomic_load (&currentCcMapping);
  auto pcTable = std::atomic_load (&currentPcMapping);
  ```
- **Rationale:** Either this was meant to use the C++20 atomic-ref overload in one branch and got mid-refactored, or it's pure boilerplate. Either way the current code is misleading.
- **Test impact:** None — compiler emits the same code.

### [MEDIUM-02] Hoist the RR counter index packing into a single helper
- **File:** `Source/PluginProcessor.cpp:896` (in ReplaceLayer wipe), `915–917` (folder-load apply), `1445–1447` (single-sample load), and `MicrotonalSamplerVoice.cpp:218–220` (selectVariantIndex)
- **Risk:** LOW
- **Type:** duplication
- **Current:** Four sites compute the 4096-entry counter index with the literal `midi * 4 * 8 + layer * 8 + tech` (or variants of it). The `8` is the `kMaxTechniques` constant from SampleMap.h, but it's hard-coded as `8` in three of four sites; only `MicrotonalSamplerVoice::selectVariantIndex` uses a named `kMaxTech = 8` local.
- **Proposed:** Add a `static constexpr` helper next to `kRrCounterSize`:
  ```cpp
  static constexpr int packRrCounterIndex (int midi, int layer, int tech) noexcept
  { return midi * 4 * 8 + layer * 8 + tech; }
  ```
  in `MicrotonalSamplerVoice.h`. All four sites become `juce::jlimit (0, kRrCounterSize - 1, packRrCounterIndex(...))`.
- **Rationale:** The layout coupling to `kRrCounterSize` is currently expressed as a numeric coincidence in four places. Hoisting fixes the drift risk + makes any future technique-axis growth a one-line change.
- **Test impact:** Round-robin variant selection across folder load, single-cell load, and ReplaceLayer wipe paths. Render-harness identity test should still pass.

### [MEDIUM-03] Replace 8 indexed `for (size_t i = 0; i < .size(); ++i)` JSON loops with range-for + first-flag
- **File:** `Source/PluginEditor.cpp:660`, `716`, `734`, `755`, `1149`, `1515`, `1539`, `1564`; `Source/PluginProcessor.cpp:2367`
- **Risk:** LOW
- **Type:** verbose-pattern
- **Current:** Each loop indexes for the comma-skip on element 0. The HIGH-04 helper covers the intervals loops; the others (`getEmbeddedTuningList`, `getEmbeddedTuningCategories`, `getSkippedFiles`, the captureTuningValueTree CSV) all have the same structure.
- **Proposed:** A small `joinJsonArray` helper or two more dedicated builders. Each remaining loop gets a `bool first = true; for (const auto& x : v) { if (!first) out += ","; first = false; ... }` form, or just one helper that takes a callable for the per-element formatter.
- **Rationale:** The shape is repeated 9× across these two files. Extraction puts the comma logic in one place.
- **Test impact:** Embedded-tunings list, embedded-tuning categories, skipped-files toast, intervals CSV in saved state.

### [MEDIUM-04] Hoist 7 dialog-modal cleanup/key-handler scaffolds into a `bindModal` helper
- **File:** `Resources/ui/js/sampler-app.js:1303`, `1365`, `1416`, `1454`, `1519`, `2215`, `2315`
- **Risk:** MEDIUM
- **Type:** duplication
- **Current:** Seven dialogs (`folder-load-options`, `embed-size-confirm`, `per-cell-merge`, `confirm`, `diagnostic`, `missing-folder`, `rr-confirm`) each define a near-identical `cleanup`/`onKey`/`addEventListener`/`removeEventListener` lifecycle around their Yes/No buttons.
- **Proposed:** Extract a `bindModal({ dialog, buttons: { yes, no, cancel? }, onKey, focus })` helper that returns a Promise resolving to the chosen action. Each dialog becomes 10–15 lines of payload-specific DOM munging + one `bindModal` call.
- **Rationale:** ~80–100 LOC saved, and the "forgot to remove a listener" bug class becomes structurally impossible. Defaults (Esc=cancel, Enter=confirm) live in one place.
- **Test impact:** All seven modals — manual smoke test required (folder load, embed confirm, per-cell merge, clear-samples confirm, diagnostic dialog, missing-folder, ambiguous-RR confirm).

### [MEDIUM-05] Extract `Number.isFinite(x) ? x : default` helper
- **File:** `Resources/ui/js/sampler-app.js:1717`, `1718`, `1721`, `1722`, `1974`, `1975`, `2422`–`2426`, `2671`, `2676`–`2685`
- **Risk:** LOW
- **Type:** verbose-pattern
- **Current:** ~15 sites use `Number.isFinite(x) ? x : default` to coerce a possibly-null/string field from a parsed JSON payload.
- **Proposed:** One module-level helper:
  ```js
  const num = (v, fallback = 0) => Number.isFinite(v) ? v : fallback;
  ```
  Usage shrinks to e.g. `editorState.loopStart = num(snap.loopStart);`.
- **Rationale:** Saves ~25 LOC; readability win — the intent is "guarded number with default" and the helper says exactly that.
- **Test impact:** Anywhere snapshots are deserialised — loop editor, technique state, trigger state, sample-map snapshot.

### [MEDIUM-06] DRY the `if (!window.__JUCE__) return; try { ... } catch (e) { console.warn }` native-fn invocation pattern
- **File:** `Resources/ui/js/sampler-app.js` — 25+ sites (lines 446, 885, 1079, 1558, 1702, 2119, 2134, 2336, 2371, 2412, 2447, 2602, 2609, 2662, 2695, 2815, 2826, 2840, 2868, 2893, 2904 + more)
- **Risk:** LOW
- **Type:** verbose-pattern
- **Current:** Almost every async-button handler has the same prologue/epilogue:
  ```js
  if (!window.__JUCE__) return;
  try {
    const fn = Juce.getNativeFunction('xxxx');
    await fn(...);
  } catch (err) { console.warn('[sampler-app] xxxx failed', err); }
  ```
- **Proposed:** A thin wrapper:
  ```js
  async function invokeNative(name, ...args) {
    if (!window.__JUCE__) return undefined;
    try { return await Juce.getNativeFunction(name)(...args); }
    catch (err) { console.warn(`[sampler-app] ${name} failed`, err); return undefined; }
  }
  ```
  Most call sites collapse to `await invokeNative('setKeyswitchEnabled', !!ksToggle.checked);`.
- **Rationale:** Cuts ~80 LOC and standardises error logging tag. Some current sites use `console.error`, some use `console.warn`, some are silent — tightening to one consistent path also fixes that drift.
- **Test impact:** All native-fn-driven UI actions; test that the silent error class for some rarely-fired paths still resolves (compare with current behaviour).

### [MEDIUM-07] Collapse identical CC/PC mapping setter scaffolds in `setCcMappingSlot`/`setPcMappingSlot`
- **File:** `Source/PluginProcessor.cpp:2012–2056`
- **Risk:** LOW
- **Type:** duplication
- **Current:** `setCcMappingSlot` (45 lines) and `setPcMappingSlot` (19 lines) follow the same template: bounds-check slot, atomic_load → make_shared (or default), mutate, atomic_store, fire callback. The CC version has a few extra fields (rangeLow/High swap), but the COW boilerplate is identical.
- **Proposed:** Extract a generic `template <class T, class Mut> void mutateMappingSlot(...)` that takes the slot, the shared_ptr slot pointer, the default factory, and a mutator lambda. Each setter becomes ~6 lines.
- **Rationale:** ~30 LOC saved; a future "add a third mapping table" (notes-to-X say) becomes a 6-line setter instead of 30.
- **Test impact:** CC/PC mapping slot edits via UI; trigger-state-updated callback should still fire.

### [MEDIUM-08] `kickNextReplayOp` — extract repeated "first-missing-folder publish" block
- **File:** `Source/PluginProcessor.cpp:1112–1124`, `1127–1145`
- **Risk:** LOW
- **Type:** duplication
- **Current:** Two near-identical blocks check `pendingMissingFolderPath.isEmpty() && pendingMissingFolderName.isEmpty()` then populate the three pending-missing fields and fire the callback. One for drag-drop (kind="drag-drop", empty path) and one for filesystem (kind="filesystem", real path).
- **Proposed:** Extract `publishMissingFolderIfNew(kind, path, displayName)` that does the empty-check + assignment + callback fire. Each call site becomes one line.
- **Rationale:** Two paths, one logic. Saves ~15 LOC and makes "first-missing-only" semantics explicit in one place.
- **Test impact:** Project reopen with missing/relocated folder; drag-drop temp-dir-gone path.

### [MEDIUM-09] Drop the `MicrotonalSamplerSound` empty constructor
- **File:** `Source/MicrotonalSamplerSound.h:18`
- **Risk:** LOW
- **Type:** stale-comment / verbose-pattern
- **Current:**
  ```cpp
  MicrotonalSamplerSound() {}
  ```
- **Proposed:** Remove — the implicitly-declared default constructor is fine (no user code depends on the explicit definition).
- **Rationale:** Trivial readability nit. The class becomes pure interface overrides.
- **Test impact:** None.

---

### [LOW-02] `Source/PluginProcessor.cpp:1707–1716` and `1833–1842` — duplicate `loopModeToString` lambdas
Already covered by HIGH-02; mentioning here just so the LOW count includes it as already-counted.

---

## Skipped (false-positive checks)

- **`MicrotonalSamplerVoice` round-robin selection logic** (`selectVariantIndex`, dual-cell crossfade): touched across v1.8–v1.11.3 with multi-bug fixes; on the DO-NOT-TOUCH list; no obvious dead code, only cleanups would risk regression.
- **WKWebView drag-drop content-streaming functions** (`dropSession*` in shared module + `bindWebViewFileDrop`): explicitly load-bearing per CLAUDE.md memory; left alone.
- **Resource provider URL handling** (`getResource` direct path comparisons): correct as-is; do NOT switch to URL parsing.
- **`Juce` namespace vs `window.__JUCE__`**: the `tuningPanelInstance = new TuningPanel(container, Juce)` and `bindWebViewFileDrop({ juce: Juce, ... })` calls are deliberately the ES-module namespace — left alone.
- **Microtonal note-expression top-level fields** in PluginProcessor / Dorico XML emission: regression-sensitive; not flagged.
- **WebView2 `#if JUCE_WINDOWS` guards / `withUserDataFolder`**: Windows DAW-host-specific; not flagged.
- **`jlimit(0, kMaxTechniques - 1, technique)` clamp at the top of every overrideLoopPoints / loadSingleSample / snapshotWaveformPeaks** (lines 1256, 1513, 1825): could be hoisted to a helper, but the redundancy is defensive and message-thread cheap; the consistency of the pattern at every entry-point is a feature, not a bug.
- **The duplicated "tail-buf zero-fill on early-exit" loop in `MicrotonalSamplerVoice::renderTailRamp`** (lines 283–287, 301–305): ~5 LOC; touching renderTailRamp risks DSP regressions that are hard to test for.

---

## Phase 2 Applied (v1.16.8)

Commit: `023cd4a` — refactor(O-MicrotonalSampler): v1.16.8 — Phase 2 simplification

### [HIGH-02] De-duplicate `loopModeToString` (3 copies, 2 inconsistent) — APPLIED
- **File:** `Source/PluginProcessor.cpp` (formerly lines 1707–1716, 1833–1842, 2158–2167; pre-commit lines 1680–1689, 1802–1811, 2127–2135)
- **Risk:** MEDIUM
- **Type:** duplication
- **Resolution:** Two canonical helpers added to the top anonymous namespace:
  - `loopModeToJsonString()` → hyphenated form (`"one-shot"`, `"auto"`, `"manual"`) — used by `snapshotSampleMapJson()`, `snapshotWaveformPeaks()`, and the `loopMode` property setter on the waveform-peaks return object.
  - `loopModeToXmlString()` → underscored form (`"one_shot"`, `"auto"`, `"manual"`) — used by `buildEmbeddedAudioTree`. Inverse parser `loopModeFromString` left in place (was never duplicated).
- **Verification:** Hyphen literal now confined to `loopModeToJsonString()` definition; underscore literal in `loopModeToXmlString()` writer + `loopModeFromString()` parser. Build + auval + format-stability greps all pass.

### [HIGH-06] Consolidate 8 `<div class="ouaricon-knob">` blocks via JS-driven render — APPLIED
- **Files:** `Resources/ui/index.html` (lines 484–583), `Resources/ui/js/sampler-app.js` (lines 38–47, plus new `renderControlStrip()` and updated `bindSliders()`)
- **Risk:** MEDIUM
- **Type:** duplication
- **Resolution:** `<footer id="control-strip">` is now empty in HTML. `SLIDER_BINDINGS` extended with `label` and optional `tooltip` fields. `renderControlStrip()` runs at boot before `bindOneKnob` (inside `bindSliders`, before the `__JUCE__` gate, so DOM exists even outside plugin host). Expression knob's CC 11 tooltip preserved via the `tooltip` field on its binding entry.
- **Verification:** CSS positional-selector check (`:nth-child` / `:first-child` / `:last-child` / `>` direct-child) returned zero hits against `.ouaricon-knob`. Visual smoke confirmed via standalone — all 8 knobs render in correct order, drag/value/hover/dragging styles work, expression tooltip preserved.

---

## Phase 3 Applied (v1.16.9)

Commit: `9293470` — refactor(O-MicrotonalSampler): v1.16.9 — Phase 3 sweep (4 candidates from LOW tier)

User selected Batch A only (LOW-severity items). Batches B (MEDIUM-severity, LOW-risk: MEDIUM-02/03/05/06/07/08/09) and C (MEDIUM-04, MEDIUM-risk dialog modal helper) were skipped — they remain in Candidates above for a future Phase 3 pass.

### Batch A — LOW severity (4 applied)

#### [LOW-01] Extract `resetAllRrCounters()` helper — APPLIED
- **File:** `Source/PluginProcessor.cpp` (post-commit lines 263, 914, 1692; helper definition at line 309 in `.cpp`, declaration at line 541 in `.h`)
- **Risk:** LOW
- **Type:** duplication
- **Resolution:** Three identical `for (auto& c : rrCounters) c.store ((uint8_t) 0xFFu, std::memory_order_relaxed);` reset loops collapsed into a single `OMicrotonalSamplerAudioProcessor::resetAllRrCounters() noexcept` member helper. Memory order (relaxed), sentinel value (0xFFu), noexcept contract preserved. The 0xFFu literal is now confined to the helper definition + 3 unrelated single-cell store sites in CC/PC counter-index paths (NOT reset-all loops).
- **Verification:** Build + auval + grep. The compiler likely inlines the helper back to the original code.

#### [LOW-03] Replace `lastWidthBucket` string with `lastIsNarrow` boolean — APPLIED
- **File:** `Resources/ui/js/sampler-app.js:1070-1083`
- **Risk:** LOW
- **Type:** verbose-pattern
- **Resolution:** `let lastWidthBucket = null; // 'wide' | 'narrow'` → `let lastIsNarrow = null;`. Loop body uses `isNarrow = w < NARROW_BREAKPOINT_PX` directly. Initial `null` preserved so first invocation still fires (boolean compared against `null` is `!==`, matching old behaviour where first run compares string against `null`).
- **Verification:** `lastWidthBucket` zero hits; `lastIsNarrow` 3 hits (declaration + comparison + assignment). Visual smoke deferred to user-side UAT (window resize across 900px boundary).

#### [LOW-05] Move `makeVector` lambda inside `getResource` — APPLIED
- **File:** `Source/PluginEditor.cpp` (lambda definition moved from anonymous namespace at former lines 36-41 to inside `getResource()` body at post-commit line 420)
- **Risk:** LOW
- **Type:** verbose-pattern
- **Resolution:** Lambda signature/body byte-identical; only scope/location changed. All 12 call sites (former lines 429-508) remain unchanged. The other anonymous-namespace helpers (`buildNotesFreqsJson`, `centsArrayToJson`, `setBoolParamFromArgs`) stay where they are — they're called from multiple member functions.
- **Verification:** All `makeVector` references in `PluginEditor.cpp` now inside `getResource()`. Build green.

#### [LOW-07] Use ternary for `startTechnique` init — APPLIED
- **File:** `Source/MicrotonalSamplerVoice.cpp:432-435`
- **Risk:** LOW
- **Type:** verbose-pattern
- **Resolution:** `startTechnique = 0; if (pendingTechniqueSource != nullptr) startTechnique = juce::jlimit (...);` → single ternary `startTechnique = (pendingTechniqueSource != nullptr) ? juce::jlimit (..., load(memory_order_acquire)) : 0;`. memory_order_acquire and the jlimit clamp preserved (RT-safety contract from v1.14.0).
- **Verification:** `startTechnique = 0` line zero hits. Generated code expected to be identical to the previous form.

---

## Phase 3 Skipped (re-verified false-positive)

These LOW items from the original audit were re-verified during Phase 3 and found to be false positives in current code (post-Phase-1/2). No source change needed. Documented here for audit completeness.

### [LOW-04] `Source/PluginProcessor.h:38–43` — comment lists "Append" before "MergeRR" but enum order is `Append=2, MergeRR=3` — FALSE POSITIVE
- **Re-verification:** The audit's claim contradicts itself. The doc comment lists `ReplaceAll → ReplaceLayer → Append → MergeRR`, which already matches the enum's value order (`0,1,2,3`). "Append before MergeRR" matches "Append=2 before MergeRR=3". No change needed.

### [LOW-06] `Resources/ui/js/sampler-app.js:825` — comment header about "250 ms double-click discrimination" duplicates inline note at line 843 — FALSE POSITIVE
- **Re-verification:** Current code has the canonical "250 ms double-click discrimination" explanation at one site (`sampler-app.js:854`, above `bindGridInteractions`). The earlier reference at line 659 inside `renderGrid` is a load-bearing cross-reference (it explains why we clear the pending click timer when the grid re-renders mid-defer — see v1.12.2 FE-03 fix), NOT a duplicate header. Both stay as-is.
