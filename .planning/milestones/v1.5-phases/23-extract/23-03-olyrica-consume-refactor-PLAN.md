---
phase: 23-extract
plan: 03
type: execute
wave: 2
depends_on: ["23-01", "23-02"]
files_modified:
  - plugins/O-Lyrica/CMakeLists.txt
  - plugins/O-Lyrica/Source/PluginProcessor.h
  - plugins/O-Lyrica/Source/PluginProcessor.cpp
  - plugins/O-Lyrica/Source/HarpSynthVoice.h
  - plugins/O-Lyrica/Source/HarpSynthVoice.cpp
  - plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h
autonomous: true
requirements:
  - LYR-01
  - LYR-02
tags: [vst3, note-expression, o-lyrica, refactor, spike-removal]

must_haves:
  truths:
    - "O-Lyrica's CMakeLists.txt consumes the shared module via ouaricon_add_module(OLyrica note-expression)."
    - "plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h no longer exists (D-16)."
    - "PluginProcessor.h has zero references to pendingTuningSemis, rawEventScratch, or OLyrica::LyricaVST3Extensions; instead declares Ouaricon::NoteExpression::VST3Extensions vst3Extensions."
    - "PluginProcessor.cpp's NE drain+correlate block (lines 705-763) collapses to a single vst3Extensions.drainAndUpdate() call."
    - "HarpSynthVoice.cpp's startNote block composes TuningEngine.getFrequency FIRST, then calls Ouaricon::NoteExpression::applyPendingTuning — no raw std::pow(2.0, semis / 12.0) remains in voice code (LYR-02)."
    - "Zero detail::neTrace, OLyrica::detail, or <fstream> references in any O-Lyrica source file after refactor (D-18)."
    - "ninja OLyrica_VST3 OLyrica_AU builds cleanly (binding integration gate for Plans 01+02)."
  artifacts:
    - path: "plugins/O-Lyrica/CMakeLists.txt"
      provides: "Module consumption via ouaricon_add_module(OLyrica note-expression)"
    - path: "plugins/O-Lyrica/Source/PluginProcessor.h"
      provides: "Module-typed VST3Extensions member; no plugin-local 128-slot table; no rawEventScratch; no getPendingTuningSource accessor"
    - path: "plugins/O-Lyrica/Source/PluginProcessor.cpp"
      provides: "Voice wiring points at module's pending table; drainAndUpdate() replaces drain+correlate block; zero neTrace references"
    - path: "plugins/O-Lyrica/Source/HarpSynthVoice.h"
      provides: "pendingTuningSource pointer retyped to Ouaricon::NoteExpression::PendingTuningTable*"
    - path: "plugins/O-Lyrica/Source/HarpSynthVoice.cpp"
      provides: "Composition: TuningEngine first, then applyPendingTuning; no raw pow in voice; no detail::neTrace calls"
  key_links:
    - from: "plugins/O-Lyrica/CMakeLists.txt"
      to: "modules/tuning/note-expression/"
      via: "ouaricon_add_module"
      pattern: "ouaricon_add_module\\(OLyrica note-expression\\)"
    - from: "plugins/O-Lyrica/Source/PluginProcessor.h"
      to: "modules/tuning/note-expression/cpp/NoteExpression.h"
      via: "#include NoteExpression.h"
      pattern: '#include "NoteExpression\\.h"'
    - from: "plugins/O-Lyrica/Source/HarpSynthVoice.cpp startNote"
      to: "Ouaricon::NoteExpression::applyPendingTuning"
      via: "single helper call after TuningEngine.getFrequency"
      pattern: "Ouaricon::NoteExpression::applyPendingTuning"
    - from: "plugins/O-Lyrica/Source/PluginProcessor.cpp processBlock"
      to: "Ouaricon::NoteExpression::VST3Extensions::drainAndUpdate"
      via: "single call replacing drain+correlate block"
      pattern: "vst3Extensions\\.drainAndUpdate"
---

<objective>
Make O-Lyrica consume the shared `note-expression` module and prove the module works end-to-end by building O-Lyrica cleanly. This plan refactors 4 O-Lyrica source files, deletes 1 spike header, and adds 1 CMakeLists line. Result: O-Lyrica's source contains zero plugin-local NE support code (LYR-01) and composes cleanly with the existing `TuningEngine` (LYR-02) — no raw `std::pow` in voice code.

Purpose: LYR-01 (module consumption, existing spike code replaced — not duplicated), LYR-02 (TuningEngine composition, no raw pow bypass). Reference shape for Phase 24's 7 downstream plugins.

Output:
- 1-line CMakeLists addition
- Surgical edits to 4 source files
- Deletion of `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h`
- `ninja OLyrica_VST3 OLyrica_AU` builds clean
</objective>

<execution_context>
@.claude/get-shit-done/workflows/execute-plan.md
@.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/phases/23-extract/23-CONTEXT.md
@.planning/phases/23-extract/23-PATTERNS.md
@.planning/phases/23-extract/23-01-module-scaffolding-PLAN.md
@.planning/phases/23-extract/23-02-juce-patch-tooling-PLAN.md
@.claude/skills/spike-findings-VST-development/sources/shared-code/processor-drain.cpp
@.claude/skills/spike-findings-VST-development/sources/shared-code/voice-startNote.cpp
@plugins/O-Lyrica/CMakeLists.txt
@plugins/O-Lyrica/Source/PluginProcessor.h
@plugins/O-Lyrica/Source/PluginProcessor.cpp
@plugins/O-Lyrica/Source/HarpSynthVoice.h
@plugins/O-Lyrica/Source/HarpSynthVoice.cpp

<interfaces>
Module API from Plan 01 (modules/tuning/note-expression/cpp/NoteExpression.h):
```cpp
namespace Ouaricon::NoteExpression {
    using PendingTuningTable = std::array<std::atomic<double>, 128>;
    inline double applyPendingTuning(PendingTuningTable&, int midi, double freq);
    class Controller : public Steinberg::Vst::INoteExpressionController { ... };
    class VST3Extensions : public juce::VST3ClientExtensions {
    public:
        int32_t queryIEditController(const Steinberg::TUID, void**) override;
        void onVst3RawEvent(const Vst3RawEvent&) override;
        void drainBlockEvents(std::vector<Vst3RawEvent>&);
        void drainAndUpdate();                         // drain + correlate
        PendingTuningTable& getPendingTable() noexcept;
    };
}
```

TuningEngine signature (plugins/O-Lyrica/Source/DSP/TuningEngine.h line 260, unchanged):
```cpp
double TuningEngine::getFrequency(int midiNote, int midiChannel = 0);
```
</interfaces>
</context>

<threat_model>
No new trust boundaries — refactor moves logic across files within plugin+module. Threats:
- T-23-07 (Tampering — stale frequency after refactor): mitigate via preserved composition order (TuningEngine first, then applyPendingTuning per D-10). Verified by grep ordering + Plan 04 Dorico smoke test.
- T-23-08 (Denial — build regression): mitigate via `ninja OLyrica_VST3 OLyrica_AU` exit 0 acceptance.
- T-23-09 (Information disclosure — audio-thread file I/O): mitigate via D-18 requiring all `neTrace` + `<fstream>` removed; grep audits confirm.
</threat_model>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Wire module into CMakeLists and delete spike header</name>
  <files>
    plugins/O-Lyrica/CMakeLists.txt,
    plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h
  </files>
  <read_first>
    - plugins/O-Lyrica/CMakeLists.txt (entire file; target name is OLyrica; insertion point is after line 75 `juce_generate_juce_header(OLyrica)`)
    - .planning/phases/23-extract/23-CONTEXT.md (D-16 deletion mandate, D-17 module consumption pattern)
    - .planning/phases/23-extract/23-PATTERNS.md §plugins/O-Lyrica/CMakeLists.txt (MODIFY) and §VST3/NoteExpressionSupport.h (DELETE)
  </read_first>
  <action>
    **Edit 1** — Insert between line 75 (`juce_generate_juce_header(OLyrica)`) and line 77 (`# Licensing module ...`) in `plugins/O-Lyrica/CMakeLists.txt`:
    ```cmake

    # Phase 23: VST3 Note Expression microtonal support (Dorico)
    # Header-only module; its module.cmake auto-verifies the JUCE-NE-PATCH marker.
    ouaricon_add_module(OLyrica note-expression)
    ```
    Do NOT modify anything else in this file. Do NOT touch `target_include_directories` at lines 46-50 — `ouaricon_add_module` handles the include automatically via `OuariconModules.cmake` line 61 (`target_include_directories(${TARGET_NAME} PRIVATE "${MODULE_DIR}/cpp")`). The VERSION bump is owned by Plan 04.

    **Edit 2** — Delete the spike header (D-16):
    ```bash
    rm plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h
    rmdir plugins/O-Lyrica/Source/VST3 2>/dev/null || true
    ```
    `rmdir` is best-effort: succeeds only if empty. If it fails (other files present), leave the directory and continue.
  </action>
  <verify>
    <automated>grep -c "ouaricon_add_module(OLyrica note-expression)" plugins/O-Lyrica/CMakeLists.txt | grep -q '^1$' && test ! -f plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h</automated>
  </verify>
  <acceptance_criteria>
    - `grep -c "ouaricon_add_module(OLyrica note-expression)" plugins/O-Lyrica/CMakeLists.txt` returns `1`
    - `test ! -f plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` exits 0 (D-16)
    - `grep -rn "NoteExpressionSupport" plugins/O-Lyrica/ | wc -l` returns `0` (no stale refs)
    - CMakeLists edit is additive: `git diff plugins/O-Lyrica/CMakeLists.txt | grep -c '^-[^-]'` returns `0` (zero deletions)
  </acceptance_criteria>
  <done>CMakeLists consumes module; spike header deleted; parent dir removed if empty.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Refactor PluginProcessor.{h,cpp} — typed VST3Extensions, drainAndUpdate collapse</name>
  <files>
    plugins/O-Lyrica/Source/PluginProcessor.h,
    plugins/O-Lyrica/Source/PluginProcessor.cpp
  </files>
  <read_first>
    - plugins/O-Lyrica/Source/PluginProcessor.h (line 22 include, lines 115-128 public block, lines 208-211 private members)
    - plugins/O-Lyrica/Source/PluginProcessor.cpp (line 506 voice init, lines 703-763 drain+correlate block)
    - .planning/phases/23-extract/23-CONTEXT.md (D-09 ownership, D-17 instantiation, D-18 strip audit)
    - .planning/phases/23-extract/23-PATTERNS.md §PluginProcessor.h (MODIFY) and §PluginProcessor.cpp (MODIFY — two touchpoints) — contains exact replacement snippets
  </read_first>
  <action>
    Apply 5 surgical edits. Read each target range before editing; match exact text to avoid ambiguity.

    **Edit 1** — `PluginProcessor.h` line 22: Replace `#include "VST3/NoteExpressionSupport.h"` with `#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)`.

    **Edit 2** — `PluginProcessor.h` lines 115-128 (public block): Replace the entire block containing `getVST3ClientExtensions` + `getPendingTuningSource` with:
    ```cpp
        //==============================================================================
        // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
        // Backed by note-expression module v1.0.0 (modules/tuning/note-expression).
        // Requires local JUCE patch (see scripts/apply-juce-patches.sh).
        juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
    ```
    `getPendingTuningSource()` is deleted entirely (D-09 — table ownership moves to the module's VST3Extensions).

    **Edit 3** — `PluginProcessor.h` lines 208-211 (private members): Replace the 3-line block starting with `// Spike 001: VST3 Note Expression support` and containing `OLyrica::LyricaVST3Extensions vst3Extensions;` + `std::array<std::atomic<double>, 128> pendingTuningSemis {};` + `std::vector<juce::VST3ClientExtensions::Vst3RawEvent> rawEventScratch;` with:
    ```cpp
        // VST3 Note Expression support (module-owned table + raw-event scratch)
        Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
    ```
    Both `pendingTuningSemis` and `rawEventScratch` are deleted (D-09 — they now live inside the module's VST3Extensions).

    **Edit 4** — `PluginProcessor.cpp` line 506 (voice init loop): Replace `voice->setPendingTuningSource(&pendingTuningSemis); // Spike 001: VST3 NE tuning` with `voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // D-09: module-owned table`.

    **Edit 5** — `PluginProcessor.cpp` lines 705-763 (drain + correlate block). Replace the 58-line block — starts at line 705 with comment `// Spike 001 (2026-04-22): Drain VST3 Note Expression events ...`, opens a `{` scope at line 709, contains the `vst3Extensions.drainBlockEvents(rawEventScratch)` call + the guarded `if (! rawEventScratch.empty())` block with two passes and several `OLyrica::detail::neTrace(...)` calls, and closes with a matching `}` at line 763 — with:
    ```cpp
        // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
        // correlate tuning deltas to their NoteOn's MIDI pitch. Pending table
        // is consumed atomically by each voice in startNote().
        vst3Extensions.drainAndUpdate();
    ```
    Before editing, read lines 700-770 to verify the exact boundaries: the block is a single outer `{ ... }` scope preceded by `buffer.clear();` at line 703 and followed by `// v1.18.3: Removed redundant null checks` at line 765. The closing `}` on line 763 matches the opening `{` on line 709.

    **Post-edit audit:**
    ```bash
    grep -c "neTrace\|OLyrica::detail\|OLyrica::LyricaVST3Extensions\|pendingTuningSemis\|rawEventScratch" \
         plugins/O-Lyrica/Source/PluginProcessor.cpp plugins/O-Lyrica/Source/PluginProcessor.h
    # Expected: 0
    ```
  </action>
  <verify>
    <automated>[ "$(grep -c 'neTrace\|OLyrica::detail\|OLyrica::LyricaVST3Extensions\|pendingTuningSemis\|rawEventScratch' plugins/O-Lyrica/Source/PluginProcessor.cpp plugins/O-Lyrica/Source/PluginProcessor.h)" = "0" ] && grep -q 'Ouaricon::NoteExpression::VST3Extensions vst3Extensions' plugins/O-Lyrica/Source/PluginProcessor.h && grep -q 'vst3Extensions.drainAndUpdate()' plugins/O-Lyrica/Source/PluginProcessor.cpp</automated>
  </verify>
  <acceptance_criteria>
    - `grep -c '#include "NoteExpression.h"' plugins/O-Lyrica/Source/PluginProcessor.h` returns `1`
    - `grep -c '#include "VST3/NoteExpressionSupport.h"' plugins/O-Lyrica/Source/PluginProcessor.h` returns `0`
    - `grep -c "Ouaricon::NoteExpression::VST3Extensions vst3Extensions" plugins/O-Lyrica/Source/PluginProcessor.h` returns `1`
    - `grep -c "pendingTuningSemis" plugins/O-Lyrica/Source/PluginProcessor.h plugins/O-Lyrica/Source/PluginProcessor.cpp` returns `0` (D-09 removal)
    - `grep -c "rawEventScratch" plugins/O-Lyrica/Source/PluginProcessor.h plugins/O-Lyrica/Source/PluginProcessor.cpp` returns `0`
    - `grep -c "OLyrica::LyricaVST3Extensions" plugins/O-Lyrica/Source/PluginProcessor.h plugins/O-Lyrica/Source/PluginProcessor.cpp` returns `0`
    - `grep -c "getPendingTuningSource" plugins/O-Lyrica/Source/PluginProcessor.h` returns `0` (accessor deleted per D-09)
    - `grep -c "vst3Extensions.drainAndUpdate" plugins/O-Lyrica/Source/PluginProcessor.cpp` returns `1` (one-line replacement for the 58-line block)
    - `grep -c "vst3Extensions.getPendingTable" plugins/O-Lyrica/Source/PluginProcessor.cpp` returns `1` (voice wiring)
    - `grep -c "detail::neTrace" plugins/O-Lyrica/Source/PluginProcessor.cpp plugins/O-Lyrica/Source/PluginProcessor.h` returns `0` (D-18)
    - `grep -c "noteIdToPitch" plugins/O-Lyrica/Source/PluginProcessor.cpp` returns `0` (correlation logic moved to module's updatePendingFromEvents)
  </acceptance_criteria>
  <done>PluginProcessor.{h,cpp} use module types; drain+correlate collapsed to drainAndUpdate(); all spike diagnostics stripped.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Refactor HarpSynthVoice.{h,cpp} — TuningEngine first, then applyPendingTuning</name>
  <files>
    plugins/O-Lyrica/Source/HarpSynthVoice.h,
    plugins/O-Lyrica/Source/HarpSynthVoice.cpp
  </files>
  <read_first>
    - plugins/O-Lyrica/Source/HarpSynthVoice.h line 128 (pendingTuningSource pointer)
    - plugins/O-Lyrica/Source/HarpSynthVoice.cpp line 13 (spike include), lines 85-88 (setPendingTuningSource), lines 106-157 (startNote with NE block at 139-157)
    - .planning/phases/23-extract/23-CONTEXT.md (D-10 composition order: TuningEngine FIRST, then applyPendingTuning — no raw pow in voice; D-18 strip audit)
    - .planning/phases/23-extract/23-PATTERNS.md §HarpSynthVoice.h (MODIFY — one line) and §HarpSynthVoice.cpp (MODIFY — three touchpoints)
    - .claude/skills/spike-findings-VST-development/sources/shared-code/voice-startNote.cpp (target shape)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md landmine 4 (composition order is load-bearing: first output sample must size to tuned frequency, no zipper)
  </read_first>
  <action>
    Apply 4 surgical edits. IMPORTANT: the existing TuningEngine lookup at lines 114-122 and humanize block at 126-137 MUST stay byte-identical — they produce `currentFrequency` which `applyPendingTuning` then composes with (D-10 composition order).

    **Edit 1** — `HarpSynthVoice.h` line 128: Replace `std::array<std::atomic<double>, 128>* pendingTuningSource = nullptr;` with:
    ```cpp
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
    ```
    Keep the preceding comment and all other members unchanged.

    **Edit 2** — `HarpSynthVoice.cpp` line 13: DELETE the line `#include "VST3/NoteExpressionSupport.h"  // Spike 002: for detail::neTrace`. The voice gets the `PendingTuningTable` type transitively via `PluginProcessor.h` (which now `#include "NoteExpression.h"`), but HarpSynthVoice.cpp does not include PluginProcessor.h directly. Check HarpSynthVoice.h — if the type is not visible at line 128 after removing the .cpp include, add `#include "NoteExpression.h"` near the top of HarpSynthVoice.h (e.g. after line 1's JuceHeader include block). The module's cpp/ dir is in the plugin's include path (via ouaricon_add_module), so a bare `#include "NoteExpression.h"` resolves.

    **Edit 3** — `HarpSynthVoice.cpp` lines 85-88 (setter): Replace:
    ```cpp
    void HarpSynthVoice::setPendingTuningSource(std::array<std::atomic<double>, 128>* source)
    {
        pendingTuningSource = source;
    }
    ```
    with:
    ```cpp
    void HarpSynthVoice::setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source)
    {
        pendingTuningSource = source;
    }
    ```

    **Edit 4** — `HarpSynthVoice.cpp` lines 139-157 (startNote NE block). This is the LYR-02 / D-10 critical edit. Before editing, re-verify lines 106-137 are UNCHANGED (TuningEngine.getFrequency at 116, MidiMessage fallback at 121, humanize block at 124-137). Then replace lines 139-157 — the 19-line block starting with `// Spike 001 (2026-04-22): Apply VST3 Note Expression tuning delta` and ending with the else-branch `else { OLyrica::detail::neTrace(...) }` — with:
    ```cpp
        // VST3 Note Expression tuning delta (Dorico microtonal).
        // Composes multiplicatively with the frequency already set by
        // TuningEngine / humanize above (D-10): base tuning * NE semitone offset.
        // Helper uses exchange(0.0) internally so retriggered notes at the same
        // pitch in a later block don't inherit a stale offset.
        if (pendingTuningSource != nullptr)
        {
            currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
                                   *pendingTuningSource, midiNoteNumber, currentFrequency);
        }
    ```

    After this edit: voice code contains exactly ONE NE call site. No `std::pow`, no `exchange(0.0)`, no `detail::neTrace`, no else-branch diagnostic. The helper encapsulates all of that (LYR-02 — no raw pow bypass).

    **Post-edit audit:**
    ```bash
    grep -c "neTrace\|OLyrica::detail\|std::pow.*semis.*12\.0\|exchange (0\.0" \
         plugins/O-Lyrica/Source/HarpSynthVoice.cpp
    # Expected: 0 (all NE mechanics live in the module helper)
    ```
  </action>
  <verify>
    <automated>[ "$(grep -c 'neTrace\|OLyrica::detail\|exchange (0\.0, std' plugins/O-Lyrica/Source/HarpSynthVoice.cpp)" = "0" ] && grep -q 'Ouaricon::NoteExpression::PendingTuningTable\* pendingTuningSource' plugins/O-Lyrica/Source/HarpSynthVoice.h && grep -q 'Ouaricon::NoteExpression::applyPendingTuning' plugins/O-Lyrica/Source/HarpSynthVoice.cpp</automated>
  </verify>
  <acceptance_criteria>
    - `grep -c "Ouaricon::NoteExpression::PendingTuningTable\* pendingTuningSource" plugins/O-Lyrica/Source/HarpSynthVoice.h` returns `1`
    - `grep -c "std::array<std::atomic<double>, 128>\*" plugins/O-Lyrica/Source/HarpSynthVoice.h plugins/O-Lyrica/Source/HarpSynthVoice.cpp` returns `0` (old type removed everywhere)
    - `grep -c '#include "VST3/NoteExpressionSupport.h"' plugins/O-Lyrica/Source/HarpSynthVoice.cpp` returns `0`
    - `grep -c "Ouaricon::NoteExpression::applyPendingTuning" plugins/O-Lyrica/Source/HarpSynthVoice.cpp` returns `1` (single call site)
    - `grep -c "detail::neTrace" plugins/O-Lyrica/Source/HarpSynthVoice.cpp` returns `0` (D-18)
    - `grep -c "OLyrica::detail" plugins/O-Lyrica/Source/HarpSynthVoice.cpp` returns `0`
    - `grep -cE 'std::pow\s*\(.*semis.*12\.0' plugins/O-Lyrica/Source/HarpSynthVoice.cpp` returns `0` (LYR-02 — no raw pow for NE in voice)
    - `grep -cE 'exchange\s*\(\s*0\.0' plugins/O-Lyrica/Source/HarpSynthVoice.cpp` returns `0` (mechanics live in the module helper)
    - Composition order verified by grep: `awk '/HarpSynthVoice::startNote/,/^}/' plugins/O-Lyrica/Source/HarpSynthVoice.cpp | grep -nE "getFrequency|applyPendingTuning"` — the `getFrequency` line number MUST be smaller than the `applyPendingTuning` line number (D-10: TuningEngine first)
    - Humanize block preserved: `grep -c "humanizeAmount" plugins/O-Lyrica/Source/HarpSynthVoice.cpp` returns at least `2` (unchanged)
  </acceptance_criteria>
  <done>HarpSynthVoice uses module type and helper; composition is TuningEngine FIRST then applyPendingTuning; voice has zero raw NE mechanics.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 4: Build O-Lyrica clean (binding integration gate for Plans 01+02)</name>
  <files>build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/OLyrica.vst3, build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/OLyrica.component</files>
  <read_first>
    - CLAUDE.md "Build Targets" section (ninja OLyrica_VST3 OLyrica_AU on macOS)
    - .planning/phases/23-extract/23-CONTEXT.md (build cleanness is success criterion 2)
  </read_first>
  <action>
    Run the clean build of O-Lyrica. This is the binding integration gate for Plans 01 (module header) and 02 (JUCE patch + CMake marker check) — nothing proves the refactor works until O-Lyrica links against the module and compiles.

    **Step 1 — Configure CMake** (if build/ is missing or CMakeLists changed):
    ```bash
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    ```
    Expect to see in configure output: `[Ouaricon]   Including note-expression/module.cmake` (from Plan 02's hook) AND `[note-expression] JUCE-NE-PATCH markers verified in /Users/taylorbrook/JUCE`. If either is missing, STOP — Plan 02's hook is not firing; do not proceed.

    **Step 2 — Build both formats:**
    ```bash
    cmake --build build --target OLyrica_VST3 OLyrica_AU --parallel
    # Or with ninja directly (per CLAUDE.md):
    # ninja -C build OLyrica_VST3 OLyrica_AU
    ```

    **Step 3 — If build fails:** read the first compile error. Most likely causes in priority order:
    - "undefined reference to Ouaricon::NoteExpression::..." → Plan 01's header didn't ship properly; re-verify Plan 01 acceptance greps.
    - "`NoteExpression.h`: No such file" → `ouaricon_add_module` didn't add the include path; verify Task 1 inserted the line inside the module-aware section of CMakeLists.txt (after `include(OuariconModules.cmake)` at line 3).
    - "`Ouaricon::NoteExpression::PendingTuningTable` not declared" in HarpSynthVoice.h → add `#include "NoteExpression.h"` to HarpSynthVoice.h per Task 3 Edit 2 note.
    - FATAL_ERROR from module.cmake about missing JUCE-NE-PATCH → Plan 02's idempotent apply-juce-patches.sh hasn't been run OR JUCE fork has been reverted; run `./scripts/apply-juce-patches.sh` and re-configure.
    - Any neTrace / detail::iidToHex symbol error → leftover spike reference; re-audit Task 2/3 acceptance greps.

    **Step 4 — Verify artifacts exist:**
    ```bash
    test -d build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/OLyrica.vst3
    test -d build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/OLyrica.component
    ```

    Do NOT install to system plugin folders in this plan — Plan 04 owns the install+test sequence with the AU cache clear. Leave artifacts in build/ untouched for Plan 04.
  </action>
  <verify>
    <automated>test -d build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/OLyrica.vst3 && test -d build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/OLyrica.component</automated>
  </verify>
  <acceptance_criteria>
    - `cmake --build build --target OLyrica_VST3 OLyrica_AU` exits 0
    - `test -d build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/OLyrica.vst3` exits 0
    - `test -d build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/OLyrica.component` exits 0
    - Configure-time log shows `Including note-expression/module.cmake` (Plan 02's hook fires)
    - Configure-time log shows `JUCE-NE-PATCH markers verified` (marker check passes)
    - No compiler warnings of the form `OLyrica::detail` / `neTrace` / `LyricaVST3Extensions` anywhere in build output
    - Final full-tree strip audit: `grep -rE "neTrace|OLyrica::detail|LyricaVST3Extensions" plugins/O-Lyrica/ modules/tuning/note-expression/ | wc -l` returns `0`
    - Final fstream audit: `grep -rE "^#include <fstream>" plugins/O-Lyrica/Source/ modules/tuning/note-expression/ | wc -l` returns `0`
  </acceptance_criteria>
  <done>Both VST3 and AU bundles built clean; Plan 02 hook confirmed firing; zero spike residue anywhere in O-Lyrica or module source.</done>
</task>

</tasks>

<verification>
1. Module consumption wired: `grep -c "ouaricon_add_module(OLyrica note-expression)" plugins/O-Lyrica/CMakeLists.txt` returns `1`.
2. Spike header deleted: `test ! -f plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h`.
3. Zero spike references across O-Lyrica + module:
   `grep -rE "neTrace|OLyrica::detail|LyricaVST3Extensions|pendingTuningSemis|rawEventScratch" plugins/O-Lyrica/ modules/tuning/note-expression/ | wc -l` returns `0`.
4. Zero stray fstream: `grep -rE "^#include <fstream>" plugins/O-Lyrica/Source/ modules/tuning/note-expression/ | wc -l` returns `0`.
5. Composition order preserved in voice: in HarpSynthVoice::startNote, line number of `getFrequency` call is SMALLER than line number of `applyPendingTuning` call.
6. Build succeeds: `cmake --build build --target OLyrica_VST3 OLyrica_AU` exits 0; both .vst3 and .component bundles exist under `build/plugins/O-Lyrica/OLyrica_artefacts/Release/`.
</verification>

<success_criteria>
- LYR-01: O-Lyrica consumes the shared module via `ouaricon_add_module(OLyrica note-expression)`; the plugin-local spike header is deleted (no duplication); all NE logic lives in the module.
- LYR-02: NE tuning composes with `TuningEngine` — voice computes `currentFrequency = tuningEngine->getFrequency(midi)` first, then passes it through `Ouaricon::NoteExpression::applyPendingTuning(...)`. Zero `std::pow(2.0, semis / 12.0)` in voice code.
- D-16 honored: `Source/VST3/NoteExpressionSupport.h` deleted.
- D-18 honored: grep audits for neTrace / detail / fstream return 0 across O-Lyrica + module.
- Build clean: `ninja OLyrica_VST3 OLyrica_AU` succeeds, proving Plan 01 and Plan 02 integrate correctly.
</success_criteria>

<output>
After completion, create `.planning/phases/23-extract/23-03-SUMMARY.md` describing:
- Files modified with change summary (CMakeLists +1 block; PluginProcessor.h lines 22/115-128/208-211 replaced; PluginProcessor.cpp lines 506/705-763 replaced; HarpSynthVoice.h line 128 retyped; HarpSynthVoice.cpp lines 13/85-88/139-157 refactored).
- Files deleted (`Source/VST3/NoteExpressionSupport.h`; `Source/VST3/` if empty).
- Grep audit results (0 for neTrace/iidToHex/fstream/OLyrica::detail/LyricaVST3Extensions/pendingTuningSemis/rawEventScratch across O-Lyrica + module).
- Build results (OLyrica_VST3 + OLyrica_AU bundles at build/plugins/O-Lyrica/OLyrica_artefacts/Release/).
- What Plan 04 consumes: the built bundles (not yet installed — Plan 04 handles cache clear + fresh install + Dorico smoke test); CHANGELOG.md is still at 2.2.2 pending Plan 04's version bump to 2.3.0.
</output>
