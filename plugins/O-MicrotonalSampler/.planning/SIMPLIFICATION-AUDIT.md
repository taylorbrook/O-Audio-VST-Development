# O-MicrotonalSampler Simplification Audit (v1.16.6)

## Summary
- Files audited: 14 (8 C++ source/header, 2 JS, 1 HTML, 2 CSS skim, plus minor headers)
- Candidates found: 22 (HIGH: 6, MEDIUM: 9, LOW: 7)
- Estimated LOC reduction if all applied: ~250–350 LOC

The codebase is in good shape overall — visible evidence of multiple cleanup passes (v1.12.4 native-fn registry extraction, v1.13.0 drop-streaming module extraction, v1.12.3 RR-counter constant hoist). The biggest remaining wins are in the C++ atomic-shared-ptr `#if/#else` boilerplate (11 sites in PluginProcessor.cpp) and a handful of JSON-building / dialog-cleanup duplications that have outlived their context.

### Phase progress
- **Phase 1 (v1.16.7, commit `a6e0a3b`):** HIGH-01, HIGH-03, HIGH-04, HIGH-05 — atomic-shared-ptr / JSON / bool-arg helpers extracted.
- **Phase 2 (v1.16.8, commit `023cd4a`):** HIGH-02, HIGH-06 — loopMode helpers + JS-driven knob render. (See `## Phase 2 Applied` below.)
- **Phase 3 Batch A (v1.16.9, commit `9293470`):** LOW-01, LOW-03, LOW-05, LOW-07. LOW-04 + LOW-06 re-verified as false positives. (See `## Phase 3 Applied (v1.16.9)` and `## Phase 3 Skipped` below.)
- **Phase 3 Batch B (v1.16.10, commit `52711dd`):** MEDIUM-02, MEDIUM-03, MEDIUM-05, MEDIUM-06, MEDIUM-07, MEDIUM-08, MEDIUM-09 — 7 LOW-risk MEDIUM-severity candidates applied. (See `## Phase 3 Applied (v1.16.10)` below.)
- **Auto-resolved by Phase 1 side-effect:** MEDIUM-01 (the `__cpp_lib_atomic_shared_ptr >= 202002L` dead-code check at former lines 510–516). Folded into the `atomicLoad` helper extraction.

Remaining open candidates: MEDIUM-04 only (Batch C — dialog modal `bindModal` helper across 7 dialogs in `sampler-app.js`). Estimated LOC saved if applied: ~80–100 LOC. Deferred because of MEDIUM-risk gate and the 7-modal manual smoke surface.

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

### [MEDIUM-04] Hoist 7 dialog-modal cleanup/key-handler scaffolds into a `bindModal` helper
- **File:** `Resources/ui/js/sampler-app.js:1303`, `1365`, `1416`, `1454`, `1519`, `2215`, `2315`
- **Risk:** MEDIUM
- **Type:** duplication
- **Current:** Seven dialogs (`folder-load-options`, `embed-size-confirm`, `per-cell-merge`, `confirm`, `diagnostic`, `missing-folder`, `rr-confirm`) each define a near-identical `cleanup`/`onKey`/`addEventListener`/`removeEventListener` lifecycle around their Yes/No buttons.
- **Proposed:** Extract a `bindModal({ dialog, buttons: { yes, no, cancel? }, onKey, focus })` helper that returns a Promise resolving to the chosen action. Each dialog becomes 10–15 lines of payload-specific DOM munging + one `bindModal` call.
- **Rationale:** ~80–100 LOC saved, and the "forgot to remove a listener" bug class becomes structurally impossible. Defaults (Esc=cancel, Enter=confirm) live in one place.
- **Test impact:** All seven modals — manual smoke test required (folder load, embed confirm, per-cell merge, clear-samples confirm, diagnostic dialog, missing-folder, ambiguous-RR confirm).

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

## Phase 3 Applied (v1.16.10)

Commit: `52711dd` — refactor(O-MicrotonalSampler): v1.16.10 — Phase 3 sweep (7 candidates from MEDIUM tier, Batch B)

User selected Batch B only (MEDIUM-severity, LOW-risk dedups). Batch C (MEDIUM-04, dialog modal `bindModal` helper) remains in Candidates above for a future Phase 3 pass — separate run required for the MEDIUM-risk gate and 7-modal manual smoke surface.

### Batch B — MEDIUM severity, LOW risk (7 applied)

#### [MEDIUM-02] Hoist RR counter index packing into a constexpr helper — APPLIED
- **File:** `Source/MicrotonalSamplerVoice.h:62` (helper definition); call sites at `Source/PluginProcessor.cpp:935` (ReplaceLayer wipe), `956` (folder-load apply, jlimit-wrapped), `1474` (single-sample load, jlimit-wrapped); `Source/MicrotonalSamplerVoice.cpp:219` (selectVariantIndex).
- **Risk:** LOW
- **Type:** duplication
- **Resolution:** Added `static constexpr int packRrCounterIndex (int midi, int layer, int tech) noexcept` to `MicrotonalSamplerVoice` next to `kRrCounterSize`. Helper uses the named `kMaxTechniques` constant (was the literal `8` in 3 of 4 sites, plus a redundant local `constexpr int kMaxTech = 8` in voice.cpp). Also rewrote `kRrCounterSize` from `128 * 4 * 8` to `128 * 4 * kMaxTechniques` for the same reason. The 3 PluginProcessor.cpp sites use `MicrotonalSamplerVoice::packRrCounterIndex (...)` (cross-class scope), the voice.cpp site uses unqualified `packRrCounterIndex (...)` (own class). The `juce::jlimit (0, kRrCounterSize - 1, ...)` clamp + `std::memory_order_relaxed` on the `.store()` are preserved at every site.
- **Verification:** `* 4 * 8 +` literal zero non-helper hits. 4 `packRrCounterIndex` call sites + 1 helper definition.

#### [MEDIUM-03] joinJsonArray helper for indexed JSON-build loops — APPLIED
- **File:** `Source/PluginEditor.cpp:88` (helper definition); call sites at `Source/PluginEditor.cpp:772` (`getEmbeddedTuningList`), `790` (`getEmbeddedTuningCategories`), `1181` (`getSkippedFiles`).
- **Risk:** LOW
- **Type:** duplication
- **Resolution:** Added a `template <class Vec, class Formatter> joinJsonArray (const Vec& v, Formatter&& fmt)` helper to the anonymous namespace next to `centsArrayToJson`/`buildNotesFreqsJson`. Helper wraps the per-element formatter callable in `[…]` brackets with comma separation via a `bool first = true` flag. Three native-fn handlers refactored to one-liners that pass per-element JSON shape as a lambda. The CSV intervals loop in `captureTuningValueTree` (`PluginProcessor.cpp:2353`) was deliberately left as-is — it emits a comma-separated string with no `[…]` wrapping (different output shape).
- **Verification:** 3 `joinJsonArray` call sites + 1 helper definition. Per-element JSON format byte-identical to the original loops (id/name/category quoted strings, integer noteCount, etc.).

#### [MEDIUM-05] num() JS helper for Number.isFinite ternary fallbacks — APPLIED
- **File:** `Resources/ui/js/sampler-app.js:33` (helper definition); 18 call sites across snapshot-deserialise paths (loop editor at 1755-56 + 2013-14, technique state at 2455-56 + 2458-59, trigger state at 2704 + 2709-11 + 2716-17, sample-map snapshot at 691 + 2502, variantIndex/Count at 1759-60).
- **Risk:** LOW
- **Type:** verbose-pattern
- **Resolution:** Added `const num = (v, fallback = 0) => (Number.isFinite(v) ? v : fallback);` at the top of the module next to other helpers. Replaced 18 `Number.isFinite(x) ? x : default` ternaries. Sites that use `Number.isFinite` as a control-flow guard (`if (!Number.isFinite(x)) return`) are preserved — different intent.
- **Verification:** `Number.isFinite` count drops from 33 → 18. The 18 remaining are: 3 helper-related lines (definition + 2 doc-comment lines), 15 preserved guards (early-return / branching guards, plus one compound condition at line 933 with extra `&& existingCount > 0`).

#### [MEDIUM-06] invokeNative wrapper for native-fn invocation pattern — APPLIED
- **File:** `Resources/ui/js/sampler-app.js:41` (helper definition); 14 call sites: `setCcEnabled`, `setCcNumber`, `setPcEnabled`, `setCcMapping`, `setPcMapping`, `resetTriggerMappings`, `setTechniqueName`, `addTechniqueSlot`, `removeTechniqueSlot`, `setKeyswitchEnabled`, `setKeyswitchRange` (commitKsRange), `clearSampleMap`, `dismissMissingFolder`, `confirmRoundRobinLoad`.
- **Risk:** LOW
- **Type:** verbose-pattern
- **Resolution:** Added `async function invokeNative(name, ...args)` that returns the resolved value on success or `undefined` on missing-host / exception (matches the prior silent-catch behaviour). Standardised log tag to `[sampler-app] {name} failed`. Preserved sites: backend `addEventListener` subscriptions (~10 sites — those are not native-fn invocations); sites with user-visible toast on catch (`saveCurrentPreset` / `loadPreset` / `locateMissingFolder` — toasts a "failed" message that invokeNative would swallow); `setActiveTechnique` (has an `else` branch that runs only when host is missing — converting would change semantics for the throw case); `pullTechniqueState` / `pullTriggerState` / `getPendingMissingFolder` / multi-step load paths (multiple sequential native fns + return-value processing — leave to a separate, more careful pass).
- **Verification:** `window.__JUCE__` count drops from 49 → 37. The 37 remaining are: 4 comments, 1 helper-definition body, 1 module-load warning at line 292 (different message: "running outside plugin host"), 16 backend event-listener subscriptions, ~15 preserved native-fn sites with semantic dependencies.

#### [MEDIUM-07] mutateMappingSlot template for setCcMappingSlot/setPcMappingSlot — APPLIED
- **File:** `Source/PluginProcessor.cpp:50` (template definition in anonymous namespace next to `atomicLoad`/`atomicStore`); call sites at `Source/PluginProcessor.cpp:2020` (`setCcMappingSlot`), `2040` (`setPcMappingSlot`).
- **Risk:** LOW
- **Type:** duplication
- **Resolution:** Added `template <class T, class DefaultFactory, class Mutator> mutateMappingSlot (slot, slotIndex, makeDefault, mut)` that bounds-checks the index, runs the COW dance (atomic load → make_shared from current-or-default → mutate via callback), then atomic-stores. The `triggerStateDirty.store + triggerAsyncUpdate()` notification stays at each call site — intentionally outside the helper so the helper is purely structural. `OMtsTrigger::defaultCcMapping (1)` vs `OMtsTrigger::defaultPcMapping()` arity asymmetry absorbed by the DefaultFactory lambda. Helper uses the existing v1.16.7 `atomicLoad`/`atomicStore` wrappers for consistency.
- **Verification:** 2 `mutateMappingSlot` call sites + 1 template definition. CC setter went from 28 lines → 18 lines; PC setter went from 19 lines → 14 lines (savings net of helper definition: small, but the COW boilerplate is now expressed in one place).

#### [MEDIUM-08] publishMissingFolderIfNew helper in kickNextReplayOp — APPLIED
- **File:** `Source/PluginProcessor.h:672` (declaration); `Source/PluginProcessor.cpp:1133` (definition); call sites at `Source/PluginProcessor.cpp:1187` (drag-drop case) and `1201` (filesystem case).
- **Risk:** LOW
- **Type:** duplication
- **Resolution:** Added `void OMicrotonalSamplerAudioProcessor::publishMissingFolderIfNew (kind, path, displayName)` member helper. Bails if either pending field is non-empty (preserves "first-missing-only" semantics: subsequent missing folders silently skipped within a replay queue), otherwise assigns all three pending-missing fields and fires the `missingFolderCallback`. Drag-drop call site passes empty `juce::String{}` for path; filesystem fallback (`op.displayName.isNotEmpty() ? op.displayName : f.getFileName()`) computed at the call site, NOT inside the helper (which has no `juce::File` to derive from).
- **Verification:** 2 helper call sites + 1 declaration + 1 definition. Two near-identical 7-line blocks each collapsed to a one-line helper call (filesystem case has one preceding statement to compute the displayName fallback).

#### [MEDIUM-09] Drop empty MicrotonalSamplerSound default ctor — APPLIED
- **File:** `Source/MicrotonalSamplerSound.h:18` (deleted line)
- **Risk:** LOW
- **Type:** stale-comment / verbose-pattern
- **Resolution:** Deleted `MicrotonalSamplerSound() {}`. Class is now pure interface overrides; the implicitly-declared default constructor is sufficient.
- **Verification:** `MicrotonalSamplerSound() {}` zero hits. Build clean (no `= delete` overload anywhere required the explicit definition).

---

## Phase 3 Skipped (re-verified false-positive)

These LOW items from the original audit were re-verified during Phase 3 and found to be false positives in current code (post-Phase-1/2). No source change needed. Documented here for audit completeness.

### [LOW-04] `Source/PluginProcessor.h:38–43` — comment lists "Append" before "MergeRR" but enum order is `Append=2, MergeRR=3` — FALSE POSITIVE
- **Re-verification:** The audit's claim contradicts itself. The doc comment lists `ReplaceAll → ReplaceLayer → Append → MergeRR`, which already matches the enum's value order (`0,1,2,3`). "Append before MergeRR" matches "Append=2 before MergeRR=3". No change needed.

### [LOW-06] `Resources/ui/js/sampler-app.js:825` — comment header about "250 ms double-click discrimination" duplicates inline note at line 843 — FALSE POSITIVE
- **Re-verification:** Current code has the canonical "250 ms double-click discrimination" explanation at one site (`sampler-app.js:854`, above `bindGridInteractions`). The earlier reference at line 659 inside `renderGrid` is a load-bearing cross-reference (it explains why we clear the pending click timer when the grid re-renders mid-defer — see v1.12.2 FE-03 fix), NOT a duplicate header. Both stay as-is.
