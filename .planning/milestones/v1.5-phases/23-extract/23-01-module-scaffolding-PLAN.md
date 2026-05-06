---
phase: 23-extract
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - modules/tuning/note-expression/module.yaml
  - modules/tuning/note-expression/README.md
  - modules/tuning/note-expression/cpp/NoteExpression.h
  - modules/registry.yaml
autonomous: true
requirements:
  - MOD-01
  - MOD-02
  - MOD-03
  - MOD-04
  - MOD-06
  - MOD-08
tags: [vst3, note-expression, module-extraction, header-only, microtonal]

must_haves:
  truths:
    - "A directory modules/tuning/note-expression/ exists and is a valid Ouaricon module."
    - "The module advertises Ouaricon::NoteExpression::Controller (NEC for kTuningTypeID) and Ouaricon::NoteExpression::VST3Extensions (raw-event dispatch, owns PendingTuningTable)."
    - "Ouaricon::NoteExpression::applyPendingTuning and Ouaricon::NoteExpression::updatePendingFromEvents exist as header-only helpers so a consuming voice needs exactly one call to apply NE tuning."
    - "The module source contains zero diagnostic spike code — no OLyrica::detail::neTrace, no detail::iidToHex, no #include <fstream>."
    - "The module is registered with semver 1.0.0 in modules/registry.yaml under the tuning category and will be discoverable via /module-list and /module-info."
  artifacts:
    - path: "modules/tuning/note-expression/module.yaml"
      provides: "Module metadata (name, version 1.0.0, category tuning, provides, dependencies=[], used_by OLyrica 2.3.0, juce_patch requirement)."
    - path: "modules/tuning/note-expression/README.md"
      provides: "Skeleton README (full content written in Plan 04). Plan 01 ships a short stub so /module-info has something to display."
    - path: "modules/tuning/note-expression/cpp/NoteExpression.h"
      provides: "Header-only module: Controller + VST3Extensions + PendingTuningTable + applyPendingTuning + updatePendingFromEvents under namespace Ouaricon::NoteExpression, stripped of all spike diagnostics."
    - path: "modules/registry.yaml"
      provides: "New registry entry in tuning section, appended before EFFECTS MODULES divider."
  key_links:
    - from: "modules/tuning/note-expression/module.yaml"
      to: "modules/tuning/note-expression/cpp/NoteExpression.h"
      via: "sources.cpp list"
      pattern: "cpp/NoteExpression.h"
    - from: "modules/registry.yaml"
      to: "modules/tuning/note-expression/"
      via: "path: tuning/note-expression"
      pattern: "path: tuning/note-expression"
    - from: "modules/tuning/note-expression/cpp/NoteExpression.h"
      to: "<juce_audio_processors> VST3ClientExtensions + Vst3RawEvent"
      via: "subclass of juce::VST3ClientExtensions, consumes Vst3RawEvent"
      pattern: "public juce::VST3ClientExtensions"
---

<objective>
Create the `note-expression` shared module at `modules/tuning/note-expression/` — metadata, stub README, stripped-and-renamespaced header-only C++ source — and register it in `modules/registry.yaml`. This plan produces the artifact that O-Lyrica (Plan 03) and all 7 Phase 24 plugins will consume. No consumer wiring and no JUCE patch tooling live here (Plan 02 owns the patch tooling; Plan 03 owns O-Lyrica wiring; Plan 04 owns the full README).

Purpose: MOD-01 (module exists), MOD-02 (Controller exposed), MOD-03 (VST3Extensions with raw-event queue + queryIEditController), MOD-04 (header-only voice helper), MOD-06 (diagnostic code stripped), MOD-08 (registered with semver).

Output:
- `modules/tuning/note-expression/module.yaml`
- `modules/tuning/note-expression/README.md` (stub; fleshed out in Plan 04)
- `modules/tuning/note-expression/cpp/NoteExpression.h`
- Appended entry in `modules/registry.yaml`
</objective>

<execution_context>
@.claude/get-shit-done/workflows/execute-plan.md
@.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/phases/23-extract/23-CONTEXT.md
@.planning/phases/23-extract/23-PATTERNS.md
@.claude/skills/spike-findings-VST-development/SKILL.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@.claude/skills/spike-findings-VST-development/sources/shared-code/NoteExpressionSupport.spike.h
@.claude/skills/spike-findings-VST-development/sources/shared-code/processor-drain.cpp
@.claude/skills/spike-findings-VST-development/sources/shared-code/voice-startNote.cpp
@modules/tuning/scala-tuning-engine/module.yaml
@modules/tuning/scala-tuning-engine/README.md
@modules/registry.yaml
@modules/cmake/OuariconModules.cmake

<interfaces>
<!-- JUCE-side types the module header consumes. These come from a patched JUCE
     (the patch Plan 02 ships) and must be referenced exactly as shown so Plan 03
     executors can wire against them without guessing. -->

From /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h
(post-patch; marker "JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)"):
```cpp
struct juce::VST3ClientExtensions::Vst3RawEvent
{
    enum class Kind : uint8_t { NoteOn, NoteOff, NoteExpressionValue };
    Kind     kind{};
    int32_t  sampleOffset = 0;
    int32_t  noteId       = -1;
    int16_t  pitch        = -1;
    int16_t  channel      = -1;
    uint32_t typeId       = 0;
    double   value        = 0.0;
};

virtual void juce::VST3ClientExtensions::onVst3RawEvent (const Vst3RawEvent&) {} // no-op default
virtual int32_t juce::VST3ClientExtensions::queryIEditController (const Steinberg::TUID, void**);
```

From <pluginterfaces/vst/ivstnoteexpression.h>:
```cpp
namespace Steinberg::Vst {
    using NoteExpressionTypeID = uint32_t;
    constexpr NoteExpressionTypeID kTuningTypeID; // (VST3 SDK reserved constant)
    class INoteExpressionController; // COM interface the Controller implements
}
```
</interfaces>
</context>

<threat_model>
## Trust Boundaries

No new trust boundaries introduced by this plan. Module consumes already-validated `Vst3RawEvent` structs produced in-process by the patched JUCE wrapper (trusted host → trusted plugin).

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-23-01 | T (tampering — stale data across retriggers) | `applyPendingTuning` | mitigate | Use `exchange(0.0, std::memory_order_acq_rel)` to atomically consume the slot so a retriggered note cannot inherit a stale semitone delta. |
| T-23-02 | D (denial — audio-thread allocation) | `VST3Extensions::onVst3RawEvent` | mitigate | Pre-reserve `blockEvents` to 64 slots in constructor (mirrors spike) so push_back is amortized O(1) without allocation on the hot path for typical event counts. |
| T-23-03 | I (information disclosure via debug artifacts) | Module header | mitigate | Strip all `neTrace`/`iidToHex`/`<fstream>`/`<mutex>` — MOD-06 enforces zero diagnostic code reaches the module. |
</threat_model>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Create module directory, module.yaml, and registry entry</name>
  <files>
    modules/tuning/note-expression/module.yaml,
    modules/registry.yaml,
    modules/tuning/note-expression/README.md
  </files>
  <read_first>
    - .planning/phases/23-extract/23-CONTEXT.md (D-01, D-02, D-03, D-04, D-05, D-06, D-07, D-08, D-11)
    - .planning/phases/23-extract/23-PATTERNS.md §`modules/tuning/note-expression/module.yaml (CREATE)` and §`modules/registry.yaml (MODIFY — add new entry)`
    - modules/tuning/scala-tuning-engine/module.yaml (shape reference)
    - modules/registry.yaml lines 190–253 (tuning section + EFFECTS divider — for exact insert location)
    - modules/persistence/preset-manager/cpp/ listing (confirms header-only precedent)
  </read_first>
  <action>
    Create `modules/tuning/note-expression/` as a new module directory.

    **File 1 — `modules/tuning/note-expression/module.yaml`:** Write a module metadata file per D-01 (path `modules/tuning/note-expression`), D-02 (registry name `note-expression`), D-03 (version `1.0.0`), D-04 (public namespace `Ouaricon::NoteExpression`), D-11 (no dependency on `scala-tuning-engine`). Use the shape from 23-PATTERNS.md §module.yaml. The file must contain, at minimum, these top-level keys and values:
    - `name: note-expression`
    - `version: 1.0.0`
    - `description:` multi-line block explaining the module owns the NEC, the VST3Extensions subclass, the 128-slot PendingTuningTable, a drain+correlate helper, and a voice-side `applyPendingTuning` helper. State it requires the local JUCE patch (`scripts/juce-patches/note-expression-juce-8.0.4.patch`).
    - `category: tuning`
    - `author: Ouaricon Audio`
    - `provides.cpp-classes: [Ouaricon::NoteExpression::Controller, Ouaricon::NoteExpression::VST3Extensions]`
    - `provides.cpp-free-functions: [Ouaricon::NoteExpression::applyPendingTuning, Ouaricon::NoteExpression::updatePendingFromEvents]`
    - `provides.cpp-types: [Ouaricon::NoteExpression::PendingTuningTable]`
    - `dependencies: []` (D-11)
    - `requirements.juce_modules: [juce_audio_processors, juce_core]`
    - `requirements.cpp_standard: 20`
    - `requirements.juce_patch.marker: "JUCE-NE-PATCH"` (verbatim; do not rename — load-bearing per D-15)
    - `requirements.juce_patch.file: "scripts/juce-patches/note-expression-juce-8.0.4.patch"`
    - `requirements.juce_patch.juce_version: "8.0.4"`
    - `sources.cpp: [cpp/NoteExpression.h]` (single header-only file)
    - `used_by: [{plugin: OLyrica, version: 2.3.0}]` (D-19 anticipates the bump; Plan 04 delivers it)
    - `changelog:` one 1.0.0 entry dated 2026-04-24 with bullets covering initial extraction from spike 001/002/003, header-only public API under `Ouaricon::NoteExpression`, ownership of PendingTuningTable, CMake-time JUCE-NE-PATCH marker verification.

    **File 2 — `modules/tuning/note-expression/README.md`:** Write a short stub (10–20 lines). Plan 04 writes the comprehensive README (MOD-05). The stub must contain at minimum: an H1 `# note-expression v1.0.0`, a one-line description, and a sentence "Consumer integration, JUCE patch procedure, and Dorico setup documented in Plan 04 / Phase 23 final." The stub exists only so `/module-info note-expression` has content to display — no user should consume this intermediate stub.

    **File 3 — Edit `modules/registry.yaml`:** Append a new entry inside the `# TUNING MODULES` section. Exact insert point: after the end of the `scala-tuning-engine` entry (its `used_by: []` line) and before the `# EFFECTS MODULES` divider. Use the shape from 23-PATTERNS.md §registry.yaml with these fields:
    - `name: note-expression`
    - `path: tuning/note-expression`
    - `version: 1.0.0`
    - `description:` multi-line block mirroring module.yaml's description
    - `category: tuning`
    - `provides:` list with `cpp-class: Ouaricon::NoteExpression::Controller`, `cpp-class: Ouaricon::NoteExpression::VST3Extensions`, `cpp-type: Ouaricon::NoteExpression::PendingTuningTable`, `cpp-free-function: Ouaricon::NoteExpression::applyPendingTuning`, `cpp-free-function: Ouaricon::NoteExpression::updatePendingFromEvents`
    - `dependencies: []`
    - `tags: [vst3, note-expression, dorico, microtuning, per-note-pitch, header-only]`
    - `reuse_score: 10`
    - `used_by: [{plugin: OLyrica, version: 2.3.0}]`

    Do NOT modify any other registry entry. Do NOT reformat the file.
  </action>
  <verify>
    <automated>test -f modules/tuning/note-expression/module.yaml &amp;&amp; test -f modules/tuning/note-expression/README.md &amp;&amp; grep -q "^  - name: note-expression$" modules/registry.yaml &amp;&amp; grep -q "path: tuning/note-expression" modules/registry.yaml &amp;&amp; grep -q "JUCE-NE-PATCH" modules/tuning/note-expression/module.yaml</automated>
  </verify>
  <acceptance_criteria>
    - `test -f modules/tuning/note-expression/module.yaml` exits 0
    - `test -f modules/tuning/note-expression/README.md` exits 0
    - `grep -c "^name: note-expression$" modules/tuning/note-expression/module.yaml` returns `1`
    - `grep -c "^version: 1.0.0$" modules/tuning/note-expression/module.yaml` returns `1`
    - `grep -c "^category: tuning$" modules/tuning/note-expression/module.yaml` returns `1`
    - `grep -c "Ouaricon::NoteExpression::Controller" modules/tuning/note-expression/module.yaml` returns at least `1`
    - `grep -c "JUCE-NE-PATCH" modules/tuning/note-expression/module.yaml` returns `1` (verbatim marker string)
    - `grep -c "^  - name: note-expression$" modules/registry.yaml` returns `1`
    - `grep -c "path: tuning/note-expression" modules/registry.yaml` returns `1`
    - Running `awk '/^  - name: note-expression$/,/^$/' modules/registry.yaml` shows `used_by:` containing `plugin: OLyrica` and `version: 2.3.0`
    - The scala-tuning-engine entry is unchanged: `git diff modules/registry.yaml -- | grep -E '^[-+]\s+- name: scala-tuning-engine'` returns no lines
  </acceptance_criteria>
  <done>
    module.yaml and README stub exist; registry.yaml appended; all acceptance greps pass.
  </done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Create cpp/NoteExpression.h (header-only, fully stripped, renamespaced)</name>
  <files>modules/tuning/note-expression/cpp/NoteExpression.h</files>
  <read_first>
    - .claude/skills/spike-findings-VST-development/sources/shared-code/NoteExpressionSupport.spike.h (entire file — source of truth for Controller + Extensions bodies)
    - .claude/skills/spike-findings-VST-development/sources/shared-code/processor-drain.cpp (two-pass correlation logic → `updatePendingFromEvents`)
    - .claude/skills/spike-findings-VST-development/sources/shared-code/voice-startNote.cpp (voice-side `exchange(0.0)` → `applyPendingTuning`)
    - .planning/phases/23-extract/23-CONTEXT.md (D-04 namespace, D-05 class names, D-06 PendingTuningTable alias, D-07 applyPendingTuning signature, D-08 updatePendingFromEvents signature, D-09 ownership on VST3Extensions, D-18 strip-verification)
    - .planning/phases/23-extract/23-PATTERNS.md §`cpp/NoteExpression.h (CREATE — header-only)` (has the exact replacement code blocks to paste)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md landmines 1–5 (why the patterns matter)
  </read_first>
  <action>
    Create a single header file `modules/tuning/note-expression/cpp/NoteExpression.h`. Start from the spike source `NoteExpressionSupport.spike.h` and apply the following transformations. The resulting file must satisfy MOD-02, MOD-03, MOD-04, MOD-06 in one shot — no follow-up strip pass.

    **File header block (top of file).** Replace the spike's `OLyrica` header comment with:
    ```cpp
    /*
      ==============================================================================
        NoteExpression.h — note-expression module v1.0.0
        VST3 Note Expression (kTuningTypeID) support for Dorico microtonal playback.
        Header-only. Public API under namespace Ouaricon::NoteExpression.
        Requires a local JUCE patch (see scripts/apply-juce-patches.sh).
      ==============================================================================
    */
    ```

    **Include block.** Keep: `<JuceHeader.h>`, `<pluginterfaces/vst/ivstnoteexpression.h>`, `<pluginterfaces/base/ibstream.h>`, `<pluginterfaces/base/ustring.h>`, `<public.sdk/source/common/pluginview.h>`, `<array>`, `<atomic>`, `<cmath>`, `<cstdint>`, `<map>`, `<vector>`. DELETE: `<fstream>` (MOD-06 / D-18), `<mutex>` (only needed for neTrace). Add `<map>` (used by `updatePendingFromEvents`). DO NOT add any other includes.

    **Namespace.** Change `namespace OLyrica { ... }` to `namespace Ouaricon::NoteExpression { ... }` (C++17 nested namespace syntax per D-04).

    **Delete entirely (MOD-06 / D-18):**
    - The entire `namespace detail { ... }` block (spike lines 35–57) containing `neTrace` and `iidToHex`.
    - Every call site of `detail::neTrace(...)` inside `Controller::getNoteExpressionCount`, `Controller::getNoteExpressionInfo`, `VST3Extensions::queryIEditController`, `VST3Extensions::onVst3RawEvent` (spike lines 72, 82, 186, 207–211).
    - Every `detail::iidToHex(...)` call site (spike line 187 inside the `neTrace` call — removed with the enclosing line).

    **Rename classes (D-05):**
    - `TuningNoteExpressionController` → `Controller`.
    - `LyricaVST3Extensions` → `VST3Extensions`.
    Keep member names and public methods otherwise identical, except for the additions below.

    **Add public type alias (D-06):** Inside the namespace, BEFORE the `Controller` class, declare:
    ```cpp
    using PendingTuningTable = std::array<std::atomic<double>, 128>;
    ```

    **Add voice helper (D-07, MOD-04)** — inline free function inside the namespace, after `PendingTuningTable` alias, before `Controller`:
    ```cpp
    /** Consumes the pending NE tuning delta for midiNoteNumber and returns the
        new frequency. exchange(0.0) ensures a retriggered note at the same pitch
        in a later block does not inherit a stale offset. Encapsulates std::pow
        so voice code never calls pow directly. Composes multiplicatively with
        the caller's base frequency — caller should pass the frequency AFTER any
        TuningEngine / humanize lookup (D-10). */
    inline double applyPendingTuning (PendingTuningTable& table,
                                      int                 midiNoteNumber,
                                      double              currentFrequency)
    {
        if (midiNoteNumber < 0 || midiNoteNumber >= 128)
            return currentFrequency;

        const double semis = table[(size_t) midiNoteNumber]
                                 .exchange (0.0, std::memory_order_acq_rel);
        if (semis == 0.0)
            return currentFrequency;

        return currentFrequency * std::pow (2.0, semis / 12.0);
    }
    ```

    **Add drain+correlate helper (D-08)** — inline free function inside the namespace, after `applyPendingTuning`, before `Controller`:
    ```cpp
    /** Two-pass correlation: (1) build noteId -> midi-pitch map from NoteOns in
        the block; (2) for each kTuningTypeID NE, compute 240*(value-0.5) semitones
        and store into table[pitch]. Caller must call
        VST3Extensions::drainBlockEvents(events) immediately before this. */
    inline void updatePendingFromEvents (
        const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>& events,
        PendingTuningTable&                                          table)
    {
        if (events.empty()) return;

        using Kind = juce::VST3ClientExtensions::Vst3RawEvent::Kind;

        std::map<int32_t, int> noteIdToPitch;
        for (const auto& e : events)
            if (e.kind == Kind::NoteOn)
                noteIdToPitch[e.noteId] = (int) e.pitch;

        for (const auto& e : events)
        {
            if (e.kind != Kind::NoteExpressionValue)        continue;
            if (e.typeId != Steinberg::Vst::kTuningTypeID)  continue;

            auto it = noteIdToPitch.find (e.noteId);
            if (it == noteIdToPitch.end())                  continue;

            const int pitch = it->second;
            if (pitch < 0 || pitch >= 128)                  continue;

            // VST3 kTuningTypeID: norm [0,1] -> plain semitones [-120, +120].
            const double semitones = 240.0 * (e.value - 0.5);
            table[(size_t) pitch].store (semitones, std::memory_order_release);
        }
    }
    ```

    **Refactor `VST3Extensions` (D-09):** Move ownership of `PendingTuningTable` and a `Vst3RawEvent` scratch buffer INTO the class (they were on the plugin in the spike). Exact public API after refactor:
    ```cpp
    class VST3Extensions : public juce::VST3ClientExtensions
    {
    public:
        VST3Extensions() { blockEvents.reserve (64); rawEventScratch.reserve (64); }

        // Keep exactly the spike's queryIEditController body — MINUS the
        // detail::neTrace line — returning the Controller for INoteExpressionController.
        int32_t queryIEditController (const Steinberg::TUID targetIID, void** obj) override;

        // Keep exactly the spike's onVst3RawEvent body — MINUS the detail::neTrace
        // line — just push_back onto blockEvents.
        void onVst3RawEvent (const Vst3RawEvent& e) override;

        // Existing spike method: move blockEvents into `out` scratch.
        void drainBlockEvents (std::vector<Vst3RawEvent>& out);

        /** Convenience: drain + correlate in one call. Plugins use this from processBlock. */
        void drainAndUpdate()
        {
            drainBlockEvents (rawEventScratch);
            updatePendingFromEvents (rawEventScratch, pendingTable);
        }

        /** Voice wiring entry point. */
        PendingTuningTable& getPendingTable() noexcept { return pendingTable; }

    private:
        Controller                  nec;
        std::vector<Vst3RawEvent>   blockEvents;
        std::vector<Vst3RawEvent>   rawEventScratch;
        PendingTuningTable          pendingTable {};
    };
    ```
    `queryIEditController`, `onVst3RawEvent`, and `drainBlockEvents` should be defined inline in the class body (header-only) with bodies copied verbatim from the spike MINUS every `detail::neTrace(...)` call. Do not introduce new logic. Do not change the `addRef`/`release`/`queryInterface` plumbing on `Controller` — keep it exactly as in the spike.

    **Controller class:** copy the spike's entire `TuningNoteExpressionController` body under the new name `Controller`, deleting ONLY the two `detail::neTrace(...)` lines at the tops of `getNoteExpressionCount` and `getNoteExpressionInfo`. All Steinberg::UString assignments, `kIsBipolar | kIsAbsolute` flags, `240.0 * (valueNormalized - 0.5)` semitone math, ASCII widening for Dorico display, and the `FUnknown`/`queryInterface`/`addRef`/`release` plumbing must remain byte-identical to the spike.

    **Namespace close.** End with `} // namespace Ouaricon::NoteExpression`.

    **No `#pragma once` required — the module's CMake glue adds it to the include path once; keep `#pragma once` at line 2 of the file** (exactly as in the spike).
  </action>
  <verify>
    <automated>test -f modules/tuning/note-expression/cpp/NoteExpression.h &amp;&amp; grep -c "namespace Ouaricon::NoteExpression" modules/tuning/note-expression/cpp/NoteExpression.h | grep -q "^[12]$" &amp;&amp; [ "$(grep -c 'detail::neTrace' modules/tuning/note-expression/cpp/NoteExpression.h)" = "0" ] &amp;&amp; [ "$(grep -c 'detail::iidToHex' modules/tuning/note-expression/cpp/NoteExpression.h)" = "0" ] &amp;&amp; [ "$(grep -c '#include <fstream>' modules/tuning/note-expression/cpp/NoteExpression.h)" = "0" ] &amp;&amp; grep -q "class Controller" modules/tuning/note-expression/cpp/NoteExpression.h &amp;&amp; grep -q "class VST3Extensions" modules/tuning/note-expression/cpp/NoteExpression.h &amp;&amp; grep -q "using PendingTuningTable" modules/tuning/note-expression/cpp/NoteExpression.h &amp;&amp; grep -q "applyPendingTuning" modules/tuning/note-expression/cpp/NoteExpression.h &amp;&amp; grep -q "updatePendingFromEvents" modules/tuning/note-expression/cpp/NoteExpression.h</automated>
  </verify>
  <acceptance_criteria>
    - `test -f modules/tuning/note-expression/cpp/NoteExpression.h` exits 0
    - `grep -c "namespace Ouaricon::NoteExpression" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` or `2` (open + close variant) — NOT `0`
    - `grep -c "namespace OLyrica" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0` (no residual spike namespace)
    - `grep -c "detail::neTrace" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0` (MOD-06)
    - `grep -c "detail::iidToHex" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0` (MOD-06)
    - `grep -c "#include <fstream>" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0` (MOD-06 / D-18)
    - `grep -c "#include <mutex>" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0`
    - `grep -c "class Controller" modules/tuning/note-expression/cpp/NoteExpression.h` returns at least `1` (MOD-02)
    - `grep -c "class VST3Extensions" modules/tuning/note-expression/cpp/NoteExpression.h` returns at least `1` (MOD-03)
    - `grep -c "using PendingTuningTable" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (D-06)
    - `grep -c "inline double applyPendingTuning" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (MOD-04 / D-07)
    - `grep -c "inline void updatePendingFromEvents" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (D-08)
    - `grep -c "void drainAndUpdate" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (D-09 convenience API)
    - `grep -c "PendingTuningTable& getPendingTable" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (D-09 voice accessor)
    - `grep -c "240.0 \* (valueNormalized - 0.5)" modules/tuning/note-expression/cpp/NoteExpression.h` returns at least `1` (Dorico formula preserved — landmine 2)
    - `grep -c "exchange (0.0, std::memory_order_acq_rel)" modules/tuning/note-expression/cpp/NoteExpression.h` returns at least `1` (retrigger safety — landmine 4)
    - `grep -c "kTuningTypeID" modules/tuning/note-expression/cpp/NoteExpression.h` returns at least `2` (Controller advertisement + updatePendingFromEvents filter)
    - `grep -c "std::pow" modules/tuning/note-expression/cpp/NoteExpression.h` returns exactly `1` (only inside `applyPendingTuning` — MOD-04 / D-07 / LYR-02 no-raw-pow-in-voice)
    - Header passes a syntax check when preprocessed with JUCE available: running `(cd build && cmake --build . --target clean && cmake --build . --target OLyrica_VST3)` after Plan 03's integration succeeds is the integration-level proof; in isolation a minimal compile-only check is sufficient: `echo '#include "modules/tuning/note-expression/cpp/NoteExpression.h"' | c++ -std=c++20 -fsyntax-only -I/Users/taylorbrook/JUCE/modules -xc++ -` — this command will fail (JUCE needs more than include paths) and is NOT a required acceptance gate; the binding gate is Plan 03's build.
  </acceptance_criteria>
  <done>
    NoteExpression.h exists, renamespaced, stripped of spike diagnostics, contains Controller + VST3Extensions + PendingTuningTable + applyPendingTuning + updatePendingFromEvents; grep audit passes with zero hits for neTrace/iidToHex/fstream/mutex/OLyrica. Final build-time verification is deferred to Plan 03 (which links the header into O-Lyrica).
  </done>
</task>

</tasks>

<verification>
1. Module directory exists with required files:
   - `test -d modules/tuning/note-expression/cpp && test -f modules/tuning/note-expression/module.yaml && test -f modules/tuning/note-expression/README.md && test -f modules/tuning/note-expression/cpp/NoteExpression.h`
2. Module source is fully stripped of spike diagnostics:
   - `grep -rE "neTrace|iidToHex|<fstream>|<mutex>|OLyrica::detail" modules/tuning/note-expression/ | wc -l` returns `0`
3. Module is registered:
   - `grep -A 2 "^  - name: note-expression$" modules/registry.yaml | grep -q "path: tuning/note-expression"`
4. Public API symbols present in header:
   - `grep -cE "class Controller|class VST3Extensions|using PendingTuningTable|applyPendingTuning|updatePendingFromEvents" modules/tuning/note-expression/cpp/NoteExpression.h` returns at least `5`
</verification>

<success_criteria>
- MOD-01: `modules/tuning/note-expression/` exists; name confirmed against `/module-list` registry (registry entry shipped in Task 1).
- MOD-02: `Ouaricon::NoteExpression::Controller` exists, advertises `kTuningTypeID` (Controller body copied from spike with `info.typeId = Steinberg::Vst::kTuningTypeID` preserved).
- MOD-03: `Ouaricon::NoteExpression::VST3Extensions` subclass of `juce::VST3ClientExtensions` with `queryIEditController` + raw-event queue (`blockEvents`/`drainBlockEvents`) preserved.
- MOD-04: Header-only `applyPendingTuning(table, midi, freq) -> double` present; encapsulates `std::pow(2.0, semis / 12.0)` so voice code needs one line.
- MOD-06: Zero `neTrace`, zero `iidToHex`, zero `#include <fstream>` in the module source.
- MOD-08: Entry in `modules/registry.yaml` with `version: 1.0.0` and `path: tuning/note-expression`.
</success_criteria>

<output>
After completion, create `.planning/phases/23-extract/23-01-SUMMARY.md` describing:
- Files created (module.yaml, README.md stub, cpp/NoteExpression.h) and registry diff.
- Symbols exported (namespace, classes, type alias, free functions).
- Stripping audit result (grep counts = 0 for neTrace/iidToHex/fstream/mutex/OLyrica).
- Note that the header is not yet compile-tested — Plan 03 is the integration gate.
- What Plan 03 consumes from this plan (the `Ouaricon::NoteExpression::VST3Extensions` type, the `applyPendingTuning` helper, the `#include "NoteExpression.h"` path made available by `ouaricon_add_module`).
</output>
