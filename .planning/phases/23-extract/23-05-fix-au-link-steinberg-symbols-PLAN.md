---
phase: 23-extract
plan: 05
type: execute
wave: 1
depends_on: []
files_modified:
  - modules/tuning/note-expression/cpp/NoteExpression.h
  - modules/tuning/note-expression/cpp/NoteExpression.cpp
  - modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp
  - modules/cmake/OuariconModules.cmake
  - scripts/verify-au-link.sh
autonomous: false
requirements: []
tags: [defect-fix, au-link, per-format-sources, ouaricon-modules-cmake, regression-prevention, two-tu-split, custom-deleter-pimpl, dispatch-slot]

must_haves:
  truths:
    - "Clean rebuild of OLyrica targets succeeds: `ninja OLyrica_VST3 OLyrica_AU OLyrica_Standalone` produces three artefacts with NO `Undefined symbols for architecture arm64` errors on ANY link line — caught by a broadened grep that flags ANY undefined-symbol failure, not just D-23-04-A's specific Steinberg symbols (regression-proof against the planner's previous mistake of moving ctor/dtor/drainAndUpdate into the VST3-only TU and creating a NEW undefined-symbol class on `Ouaricon::NoteExpression::VST3Extensions::*`)."
    - "modules/tuning/note-expression/cpp/NoteExpression.h contains zero `<pluginterfaces/...>` includes and zero `#if JucePlugin_Build_VST3` guards (D-22 amended)."
    - "modules/tuning/note-expression/cpp/NoteExpression.cpp (SharedCode-bound TU) contains `VST3Extensions::VST3Extensions()`, `VST3Extensions::~VST3Extensions()`, and `VST3Extensions::drainAndUpdate()` definitions with ZERO `Steinberg::*` symbol references and ZERO `<pluginterfaces/...>` includes — links cleanly into AU, Standalone, and any future format wrapper (D-21, D-22 amended)."
    - "modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp (VST3-only TU) owns the full Controller class body, VST3Extensions::queryIEditController body, updatePendingFromEvents free-helper body, realControllerDelete deleter, and a static-init that registers updatePendingFromEvents into the SharedCode-bound TU's dispatch slot (D-20, D-22 amended). All `<pluginterfaces/...>` includes consolidated here."
    - "VST3Extensions::nec is declared in the header as `std::unique_ptr<Controller, void(*)(Controller*)>` (custom function-pointer deleter) with `Controller` forward-declared (D-21 amended). The deleter pattern is what allows the SharedCode-bound dtor to compile without `Controller`'s body."
    - "Static-init dispatch registration: when the VST3-only TU is linked, `updatePendingFromEvents` is registered into the SharedCode TU's `std::atomic<NEUpdateFn> g_neUpdate` slot at TU load time. When the VST3 TU is NOT linked (AU/Standalone), the slot stays `nullptr` and `drainAndUpdate` skips correlation — correct because non-VST3 hosts cannot deliver `kTuningTypeID` events anyway (D-22 amended)."
    - "modules/cmake/OuariconModules.cmake routes `cpp/<format>/` source files to per-format JUCE subtargets (`${TARGET}_VST3`, `${TARGET}_AU`, etc.) when those targets exist; SharedCode glob is non-recursive and only sweeps top-level `cpp/*.cpp` and `cpp/*.h` (D-24, D-27)."
    - "Per-format routing silently no-ops when `${TARGET_NAME}_<FORMAT>` does not exist — a plugin that excludes a format from its FORMATS list still gets a clean configure (D-28)."
    - "scripts/verify-au-link.sh accepts a plugin name, parses `PLUGIN_CODE` and `PLUGIN_MANUFACTURER_CODE` (resolving the `${OUARICON_MANUFACTURER_CODE}` variable to its dev value) from `plugins/<Plugin>/CMakeLists.txt`, and invokes `auval -v aumu <subtype> <manuf>` (or the appropriate AU 4-char type derived from `IS_SYNTH TRUE` / `PLUGIN_AU_MAIN_TYPE`); exits non-zero on failure."
    - "`auval -v aumu OLyr OuDv` (or equivalent codes auto-extracted by the script) accepts the freshly-installed `~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component` (D-30)."
    - "O-Lyrica VST3 still passes the LYR-03 Dorico quarter-sharp smoke test (5 sub-tests pass) — the refactor preserved behaviour."
    - "Module public API surface is preserved verbatim (D-23, D-32): `Ouaricon::NoteExpression::PendingTuningTable`, `applyPendingTuning(...)`, `VST3Extensions::queryIEditController`, `VST3Extensions::onVst3RawEvent`, `VST3Extensions::drainBlockEvents`, `VST3Extensions::drainAndUpdate`, `VST3Extensions::getPendingTable`, and the free `updatePendingFromEvents(...)` helper all still exist with their pre-refactor signatures."
    - "O-Lyrica consumer call-sites compile UNCHANGED: `plugins/O-Lyrica/CMakeLists.txt` line 80 (`ouaricon_add_module(OLyrica note-expression)`), `plugins/O-Lyrica/Source/PluginProcessor.{h,cpp}` (`vst3Extensions` member, `getVST3ClientExtensions`, `vst3Extensions.drainAndUpdate()` call), and `plugins/O-Lyrica/Source/HarpSynthVoice.{h,cpp}` (voice-side `applyPendingTuning` call) all build without source edits (D-23, D-32)."
    - "`JUCE-NE-PATCH` marker check in `modules/tuning/note-expression/module.cmake` still passes (D-34 — module.cmake hook contract preserved)."
  artifacts:
    - path: "modules/tuning/note-expression/cpp/NoteExpression.h"
      provides: "Steinberg-free public header (PendingTuningTable, applyPendingTuning, VST3Extensions class with forward-declared Controller + custom-deleter pimpl, free helper forward-decls, dispatch-slot register/get accessors)"
    - path: "modules/tuning/note-expression/cpp/NoteExpression.cpp"
      provides: "SharedCode-bound TU: VST3Extensions ctor/dtor/drainAndUpdate, noopControllerDelete, dispatch-slot storage + accessor. Zero Steinberg symbol references."
    - path: "modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp"
      provides: "VST3-only TU: full Controller class body, VST3Extensions::queryIEditController body (with lazy-create swap-deleter), updatePendingFromEvents body, realControllerDelete, static-init dispatch registration"
    - path: "modules/cmake/OuariconModules.cmake"
      provides: "Per-format module-source routing convention (cpp/<format>/ -> ${TARGET}_<FORMAT>)"
    - path: "scripts/verify-au-link.sh"
      provides: "Reusable AU link + auval gate keyed by plugin CMake-extracted PLUGIN_CODE / PLUGIN_MANUFACTURER_CODE"
  key_links:
    - from: "modules/tuning/note-expression/cpp/NoteExpression.h"
      to: "modules/tuning/note-expression/cpp/NoteExpression.cpp"
      via: "VST3Extensions ctor/dtor/drainAndUpdate declared in header, defined out-of-line in SharedCode-bound TU using the custom function-pointer deleter to dodge the unique_ptr<incomplete-Controller> dtor instantiation"
      pattern: "std::unique_ptr<Controller, void"
    - from: "modules/tuning/note-expression/cpp/NoteExpression.cpp dispatch slot"
      to: "modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp updatePendingFromEvents"
      via: "static-init in VST3 TU calls registerNEUpdate(&updatePendingFromEvents); SharedCode drainAndUpdate loads the atomic and dispatches if non-null"
      pattern: "registerNEUpdate"
    - from: "modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp queryIEditController"
      to: "VST3Extensions::nec member (custom-deleter unique_ptr)"
      via: "lazy-create idiom: `nec = std::unique_ptr<Controller, void(*)(Controller*)>(new Controller, &realControllerDelete);` — move-assigns a fresh unique_ptr to swap BOTH the managed pointer AND the deleter atomically"
      pattern: "realControllerDelete"
    - from: "modules/cmake/OuariconModules.cmake per-format loop"
      to: "${TARGET_NAME}_VST3 target sources"
      via: "target_sources(${TARGET_NAME}_VST3 PRIVATE ${_FMT_SOURCES})"
      pattern: "target_sources\\(\\$\\{TARGET_NAME\\}_\\$\\{_FMT_UPPER\\}"
    - from: "scripts/verify-au-link.sh"
      to: "auval -v <type> <subtype> <manuf>"
      via: "auval invocation with codes parsed from plugin CMakeLists.txt"
      pattern: "auval -v"
    - from: "plugins/O-Lyrica/CMakeLists.txt:80 ouaricon_add_module(OLyrica note-expression)"
      to: "${TARGET}_VST3 receiving cpp/vst3/ sources"
      via: "OuariconModules.cmake per-format loop"
      pattern: "ouaricon_add_module\\(OLyrica note-expression\\)"
---

<objective>
Defect-fix follow-up to Plans 23-01..04: restore clean `OLyrica_AU` and `OLyrica_Standalone` link by relocating Steinberg-touching code out of SharedCode (`libO-Lyrica-dev_SharedCode.a`) using a **two-TU split** (per amended D-21/D-22):
- `cpp/NoteExpression.cpp` (NEW, SharedCode-bound) — owns `VST3Extensions::VST3Extensions()`, `~VST3Extensions()`, `drainAndUpdate()`, plus a `noopControllerDelete` no-op deleter and a `std::atomic<NEUpdateFn>` dispatch slot. Zero `Steinberg::*` references — links cleanly into AU/Standalone/etc.
- `cpp/vst3/NoteExpression_VST3.cpp` (NEW, VST3-only via D-27 routing) — owns the full `Controller` body, `queryIEditController` body (with lazy-create swap-deleter idiom), `updatePendingFromEvents` body, `realControllerDelete`, and a static-init that registers `updatePendingFromEvents` into the SharedCode dispatch slot. All `<pluginterfaces/...>` includes consolidated here.

The header pimpl is `std::unique_ptr<Controller, void(*)(Controller*)>` — a custom function-pointer deleter which is the load-bearing trick that lets the SharedCode-bound dtor compile without seeing `Controller`'s body. Establishes the project-wide per-format module-source convention by extending `modules/cmake/OuariconModules.cmake` with per-format auto-routing, and adds a reusable `auval`-based AU verification gate so Phase 24 propagation cannot silently regress.

Purpose: Plan 23-04 surfaced D-23-04-A — the `#if JucePlugin_Build_VST3` guards added in Plan 23-03 (commit `f85ff38`) evaluate at every translation-unit compile site. SharedCode compiles with `JucePlugin_Build_VST3=1` because the plugin's FORMATS list includes VST3, so guarded Steinberg references leak into SharedCode IR. The AU/Standalone link lines do NOT link `pluginterfaces`, producing `Undefined symbols: Steinberg::Vst::INoteExpressionController::iid, Steinberg::UString::assign, Steinberg::FUnknown::iid`.

The plan-checker caught a defect in the *previous* iteration of Plan 23-05: placing `VST3Extensions::VST3Extensions()`, `~VST3Extensions()`, and `drainAndUpdate()` in the VST3-only TU would replace the original Steinberg undefined-symbol failure with a NEW `Ouaricon::NoteExpression::VST3Extensions::*` undefined-symbol failure on the same AU/Standalone link lines (because `PluginProcessor` in SharedCode references those symbols by holding `vst3Extensions` as a value member and calling `drainAndUpdate()` from `processBlock`). The amended D-21/D-22 fixes this by splitting into TWO new TUs: ctor/dtor/drainAndUpdate live in a Steinberg-free SharedCode-bound TU; only the genuinely VST3-touching code lives in the VST3-only TU. The custom-deleter pimpl (D-21) is what makes the dtor linkable without `Controller`'s body.

This plan is a defect fix on top of Plans 23-01..04's already-satisfied requirements (MOD-01..08, LYR-01..04). REQUIREMENTS.md coverage is preserved — those REQ-IDs were claimed by the earlier plans; this plan does not re-claim them. The `requirements: []` frontmatter is intentional.

Output:
- `modules/tuning/note-expression/cpp/NoteExpression.h` — Steinberg-free, `Controller` forward-declared, `std::unique_ptr<Controller, void(*)(Controller*)> nec` member with custom function-pointer deleter, ctor/dtor + `queryIEditController` + `drainAndUpdate` + `updatePendingFromEvents` declared out-of-line, dispatch-slot accessors (`registerNEUpdate`, internal slot type) declared.
- `modules/tuning/note-expression/cpp/NoteExpression.cpp` (new, SharedCode-bound) — VST3Extensions ctor/dtor/drainAndUpdate + noopControllerDelete + dispatch-slot storage. Zero `<pluginterfaces/...>` includes, zero `Steinberg::*` references.
- `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` (new, VST3-only) — single VST3-only TU owning all Steinberg-touching definitions + the `<pluginterfaces/...>` includes + the static-init that wires the dispatch slot.
- `modules/cmake/OuariconModules.cmake` — narrows SharedCode glob to `cpp/*.cpp` + `cpp/*.h` (non-recursive) and appends a per-format routing loop iterating `vst3 au standalone vst2 aax lv2 unity`.
- `scripts/verify-au-link.sh` (new) — reusable AU verification one-liner consumed by Phase 24 plans.
- Clean rebuild + fresh install of OLyrica VST3 + AU + Standalone per CLAUDE.md Plugin Cache Clearing protocol.
- `auval` accepts the AU bundle.
- LYR-03 Dorico quarter-sharp smoke test re-passes via VST3 (regression check).

**SCOPE BOUNDARIES (D-32, D-33, D-34) — what this plan does NOT do:**
- Does NOT change module public namespace (`Ouaricon::NoteExpression`), public class names, public method signatures, `module.yaml`, or `registry.yaml`.
- Does NOT bump the `note-expression` module version (stays 1.0.0; public API unchanged).
- Does NOT bump O-Lyrica's version (stays 2.3.0). Rationale: 2.3.0 was never released externally — the AU defect was caught during Plan 23-04's installation step, before any user-facing release. A defect fix internal to Phase 23 closeout does not warrant a 2.3.1 bump. CHANGELOG entry is also not amended (the [2.3.0] entry already accurately describes the user-visible feature; the AU fix is internal architecture, not user-visible behavior change).
- Does NOT regenerate `scripts/juce-patches/note-expression-juce-8.0.4.patch`, change `scripts/apply-juce-patches.sh`, or alter the `JUCE-NE-PATCH` marker check in `modules/tuning/note-expression/module.cmake`. The module.cmake hook contract is preserved.
- Does NOT introduce any CMake-time include-grep assertion (deferred per CONTEXT.md `<deferred>` Plan 23-05 section — brittle and the per-format convention + verify gate already prevent the defect).
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
@.planning/phases/23-extract/23-04-version-readme-dorico-smoketest-SUMMARY.md
@CLAUDE.md
@modules/tuning/note-expression/cpp/NoteExpression.h
@modules/tuning/note-expression/module.cmake
@modules/cmake/OuariconModules.cmake
@plugins/O-Lyrica/CMakeLists.txt
@plugins/O-Lyrica/Source/PluginProcessor.h
@plugins/O-Lyrica/Source/PluginProcessor.cpp
@.claude/skills/spike-findings-VST-development/SKILL.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md

<interfaces>
<!-- Public API surface that MUST be preserved verbatim (D-23, D-32). -->
<!-- Header declarations + the new dispatch wiring. -->

From modules/tuning/note-expression/cpp/NoteExpression.h (POST-REFACTOR, two-TU split per D-22 amended):
```cpp
namespace Ouaricon::NoteExpression
{
    // Steinberg-free, header-resident:
    using PendingTuningTable = std::array<std::atomic<double>, 128>;

    inline double applyPendingTuning (PendingTuningTable& table,
                                      int                 midiNoteNumber,
                                      double              currentFrequency);

    // Forward declaration — full body lives in cpp/vst3/NoteExpression_VST3.cpp.
    class Controller;

    // Dispatch slot wiring (Plan 23-05 amended D-22):
    // The SharedCode-bound cpp/NoteExpression.cpp owns a `std::atomic<NEUpdateFn>`
    // slot. The VST3-only cpp/vst3/NoteExpression_VST3.cpp registers its
    // updatePendingFromEvents body into that slot at TU-load time via static-init.
    // Non-VST3 builds never link the VST3 TU, so the slot stays nullptr and
    // drainAndUpdate skips correlation — correct because non-VST3 hosts can't
    // deliver kTuningTypeID events anyway.
    using NEUpdateFn = void (*) (
        const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>&,
        PendingTuningTable&);

    void registerNEUpdate (NEUpdateFn fn) noexcept;  // defined in cpp/NoteExpression.cpp

    // Free helper — declared in header, DEFINED in cpp/vst3/NoteExpression_VST3.cpp
    // (references Steinberg::Vst::kTuningTypeID, so its body cannot live in SharedCode).
    void updatePendingFromEvents (
        const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>& events,
        PendingTuningTable& table);

    class VST3Extensions : public juce::VST3ClientExtensions
    {
    public:
        VST3Extensions();                  // out-of-line in cpp/NoteExpression.cpp (SharedCode)
        ~VST3Extensions() override;        // out-of-line in cpp/NoteExpression.cpp (SharedCode)

        int32_t queryIEditController (const Steinberg::TUID targetIID, void** obj) override;  // out-of-line in cpp/vst3/NoteExpression_VST3.cpp
        void onVst3RawEvent (const Vst3RawEvent& e) override;                                  // header-resident inline (no Steinberg refs)
        void drainBlockEvents (std::vector<Vst3RawEvent>& out);                                // header-resident inline (no Steinberg refs)
        void drainAndUpdate();                                                                  // out-of-line in cpp/NoteExpression.cpp (dispatches via slot)
        PendingTuningTable& getPendingTable() noexcept;                                         // header-resident inline

    private:
        // Custom function-pointer deleter (D-21 amended). Allows the SharedCode-
        // bound dtor to compile without seeing Controller's body. The deleter is
        // initialized to noopControllerDelete (defined in cpp/NoteExpression.cpp);
        // the VST3 TU's queryIEditController swaps both the managed pointer AND
        // the deleter to realControllerDelete via move-assignment when it lazy-
        // creates the Controller (idiom in Task 2 below).
        std::unique_ptr<Controller, void(*)(Controller*)> nec;
        std::vector<Vst3RawEvent> blockEvents;
        std::vector<Vst3RawEvent> rawEventScratch;
        PendingTuningTable        pendingTable {};
    };
}
```

From modules/cmake/OuariconModules.cmake (POST-REFACTOR per-format routing):
```cmake
# SharedCode: top-level cpp/ only (NON-recursive — excludes cpp/<format>/ subdirs)
file(GLOB MODULE_CPP_SOURCES "${MODULE_DIR}/cpp/*.cpp" "${MODULE_DIR}/cpp/*.h")
target_sources(${TARGET_NAME} PRIVATE ${MODULE_CPP_SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE "${MODULE_DIR}/cpp")

# Per-format routing: cpp/<format>/ -> ${TARGET_NAME}_<FORMAT>
set(_OUA_JUCE_FORMATS vst3 au standalone vst2 aax lv2 unity)
foreach(fmt ${_OUA_JUCE_FORMATS})
    string(TOUPPER ${fmt} _FMT_UPPER)
    set(_FMT_DIR "${MODULE_DIR}/cpp/${fmt}")
    if(EXISTS "${_FMT_DIR}" AND TARGET "${TARGET_NAME}_${_FMT_UPPER}")
        file(GLOB_RECURSE _FMT_SOURCES "${_FMT_DIR}/*.cpp" "${_FMT_DIR}/*.h")
        if(_FMT_SOURCES)
            target_sources(${TARGET_NAME}_${_FMT_UPPER} PRIVATE ${_FMT_SOURCES})
            target_include_directories(${TARGET_NAME}_${_FMT_UPPER} PRIVATE "${_FMT_DIR}")
            message(STATUS "[Ouaricon]   Added ${MODULE_NAME}/cpp/${fmt} sources to ${TARGET_NAME}_${_FMT_UPPER}")
        endif()
    endif()
endforeach()
```
</interfaces>

<consumer_invariants>
<!-- These files compile UNCHANGED. Do NOT edit them in this plan (D-23, D-32). -->
<!-- Their existence here is to make the executor verify, not edit. -->

- plugins/O-Lyrica/CMakeLists.txt line 80: `ouaricon_add_module(OLyrica note-expression)` — the one-liner contract.
- plugins/O-Lyrica/Source/PluginProcessor.h line 200: `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;` member; line 119: `getVST3ClientExtensions() override { return &vst3Extensions; }` accessor.
- plugins/O-Lyrica/Source/PluginProcessor.cpp line 506: `voice->setPendingTuningSource(&vst3Extensions.getPendingTable());`. Line 708: `vst3Extensions.drainAndUpdate();` call from processBlock. **These are the load-bearing references** that the SharedCode link line MUST be able to resolve — which is why ctor/dtor/drainAndUpdate live in `cpp/NoteExpression.cpp` (SharedCode), not in `cpp/vst3/NoteExpression_VST3.cpp`.
- plugins/O-Lyrica/Source/HarpSynthVoice.h: `Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;` member.
- plugins/O-Lyrica/Source/HarpSynthVoice.cpp: `currentFrequency = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNoteNumber, currentFrequency);` call in startNote.
- modules/tuning/note-expression/module.yaml: unchanged (D-29).
- modules/tuning/note-expression/module.cmake: unchanged (D-34).
- modules/registry.yaml: unchanged (D-29).
- scripts/juce-patches/note-expression-juce-8.0.4.patch: unchanged (D-34).
- scripts/apply-juce-patches.sh: unchanged (D-34).
</consumer_invariants>
</context>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Module ↔ Plugin AU/VST3/Standalone link line | Per-format symbol leakage from SharedCode IR causes undefined-symbol failures on non-VST3 link lines. Mitigated by two-TU split: SharedCode-bound TU is Steinberg-free; VST3-only TU is consolidated home for `<pluginterfaces/...>` symbols. |
| SharedCode dtor ↔ unique_ptr<Controller> with incomplete-Controller | Default `unique_ptr<T>` dtor instantiation requires `T` complete. Mitigated by custom function-pointer deleter — `unique_ptr<Controller, void(*)(Controller*)>`'s dtor only invokes the stored function pointer; `Controller`'s definition is irrelevant at the dtor instantiation site. |
| SharedCode drainAndUpdate ↔ Steinberg::Vst::kTuningTypeID | Calling updatePendingFromEvents directly would re-introduce Steinberg refs into SharedCode. Mitigated by dispatch-slot indirection: SharedCode loads `std::atomic<NEUpdateFn>` and dispatches if non-null; VST3 TU's static-init registers the actual implementation. |
| CMake module-glob ↔ filesystem layout | `file(GLOB_RECURSE)` previously swept format-specific files into SharedCode. Mitigated by narrowing to non-recursive `file(GLOB)` and per-format routing. |
| `auval` ↔ installed AU bundle | AU may link cleanly but fail load-time validation (code signing, plist, IID registration). Mitigated by adding `auval` to verify gate (D-30). |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-23-05-01 | Tampering | `cpp/NoteExpression.h` | mitigate | Strip ALL `<pluginterfaces/...>` includes and ALL `#if JucePlugin_Build_VST3` guards. Forward-declare `Controller`. Replace `Controller nec;` with `std::unique_ptr<Controller, void(*)(Controller*)> nec;` (custom function-pointer deleter — D-21 amended). Move ctor/dtor/queryIEditController/drainAndUpdate to out-of-line declarations. Acceptance: `grep -c "pluginterfaces" cpp/NoteExpression.h` returns `0` and `grep -c "JucePlugin_Build_VST3" cpp/NoteExpression.h` returns `0`. |
| T-23-05-02 | Denial of Service | OLyrica AU/Standalone link | mitigate | Two-TU split (D-22 amended): `VST3Extensions::VST3Extensions()`, `~VST3Extensions()`, `drainAndUpdate()` live in `cpp/NoteExpression.cpp` (SharedCode-bound, Steinberg-free). Per-format routing in `OuariconModules.cmake` ensures `cpp/vst3/*.cpp` is added ONLY to `${TARGET}_VST3`, never to SharedCode. Acceptance: `ninja OLyrica_AU` succeeds with no `Undefined symbols` errors of ANY kind (broadened grep — catches Steinberg AND Ouaricon-namespace symbol leaks). |
| T-23-05-02b | Denial of Service | "Fix introduces NEW undefined-symbol class on VST3Extensions ctor/dtor" | mitigate | Plan-checker caught the previous Plan 23-05 iteration moving ctor/dtor/drainAndUpdate into the VST3-only TU, which would have produced `Undefined symbols: Ouaricon::NoteExpression::VST3Extensions::VST3Extensions(), ~VST3Extensions(), drainAndUpdate()` on AU/Standalone link lines (because `OLyrica/PluginProcessor` in SharedCode holds `vst3Extensions` as a value member and calls `drainAndUpdate()` from `processBlock`). Amended D-22 places those three symbols in the SharedCode-bound TU. Custom-deleter pimpl (D-21 amended) makes the dtor linkable without `Controller` complete. Acceptance: Task 5 build-log grep is broadened to `grep -E "Undefined symbols\|^[[:space:]]+\".+\", referenced from:"` — flags ANY undefined-symbol failure, not just D-23-04-A's specific names. |
| T-23-05-02c | Denial of Service | SharedCode `drainAndUpdate` calling `updatePendingFromEvents` directly would re-leak `kTuningTypeID` | mitigate | Dispatch-slot indirection: SharedCode owns `std::atomic<NEUpdateFn> g_neUpdate {nullptr}`. `drainAndUpdate` does `if (auto fn = g_neUpdate.load(std::memory_order_acquire)) fn(rawEventScratch, pendingTable);`. VST3 TU's static-init calls `registerNEUpdate(&updatePendingFromEvents)` at TU load. AU/Standalone don't link the VST3 TU → slot stays nullptr → no correlation, which is correct (no VST3 host = no kTuningTypeID events). Acceptance: `grep -c "Steinberg::" modules/tuning/note-expression/cpp/NoteExpression.cpp` returns `0`. |
| T-23-05-03 | Information Disclosure | None applicable | accept | This is an internal refactor. No user data, no PII, no network surface. |
| T-23-05-04 | Repudiation | "AU built clean" claim regression | mitigate | `scripts/verify-au-link.sh OLyrica` is a reproducible CLI gate that runs `auval` against the installed `.component`. Re-runnable in CI and by Phase 24 plans verbatim. Acceptance: script returns exit code 0 after fresh install. |
| T-23-05-05 | Spoofing | Stale on-disk AU artefact masking link failure | mitigate | Verify gate per CLAUDE.md: kill `AudioComponentRegistrar`, clear `~/Library/Caches/AudioUnitCache/` and `~/Library/Caches/com.apple.audiounits.cache`, `rm -rf` old `.component`, `cp -R` fresh build artefact. The Plan 23-03 SUMMARY's stale-Apr-13 issue (per 23-04 SUMMARY D-23-04-A) is the exact threat being mitigated here. Bundle mtime check enforces freshness. |
| T-23-05-06 | Elevation of Privilege | None applicable | accept | No new privileges, no new attack surface. |
| T-23-05-07 | Tampering | `OuariconModules.cmake` per-format glob applied to wrong target | mitigate | Per-format `target_sources(${TARGET_NAME}_${_FMT_UPPER} PRIVATE ...)` routes ONLY to format-specific subtargets. PRIVATE include_directories prevents non-format TUs from pulling in format-private headers (D-25). Acceptance: configure log emits `[Ouaricon]   Added note-expression/cpp/vst3 sources to OLyrica_VST3` exactly once per per-format directory present. |
| T-23-05-08 | Tampering | Static-init registration race vs first audio callback | accept | C++ static-init guarantees: TU-load-time static initializers complete before `dlopen`/`LoadLibrary` returns; the VST3 host loads the wrapper before any `processBlock` callback. The atomic register/load on a function-pointer slot is single-write-single-read in practice (one writer at TU load, audio-thread readers afterward). Acquire/release semantics on the atomic slot give well-defined memory ordering under the C++ memory model. Documented in cpp/NoteExpression.cpp. |
</threat_model>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Create cpp/NoteExpression.cpp (SharedCode-bound, Steinberg-free TU)</name>
  <files>
    modules/tuning/note-expression/cpp/NoteExpression.cpp
  </files>
  <read_first>
    - modules/tuning/note-expression/cpp/NoteExpression.h (current pre-strip state — copy the existing inline ctor's `reserve(64)` calls verbatim from lines 241-245; confirm `drainBlockEvents` is already a header-resident no-Steinberg method whose declaration will survive Task 2's strip)
    - .planning/phases/23-extract/23-CONTEXT.md (D-21 amended, D-22 amended — exact specification of the SharedCode-bound TU's contents and the custom-deleter pimpl rationale)
    - .planning/phases/23-extract/23-04-version-readme-dorico-smoketest-SUMMARY.md (D-23-04-A — confirms why `VST3Extensions::VST3Extensions()`, `~VST3Extensions()`, `drainAndUpdate()` MUST be defined in a SharedCode-bound TU: PluginProcessor in SharedCode references those symbols, so AU/Standalone link lines must resolve them WITHOUT pulling in any Steinberg symbol)
  </read_first>
  <action>
**Create the file** `modules/tuning/note-expression/cpp/NoteExpression.cpp` with the exact content below. This is the SharedCode-bound translation unit per amended D-22. It compiles into `libO-Lyrica-dev_SharedCode.a` (and every plugin's SharedCode going forward) via the new non-recursive `file(GLOB cpp/*.cpp)` SharedCode glob in Task 4.

**Critical constraint:** this file must contain ZERO `<pluginterfaces/...>` includes and ZERO direct `Steinberg::*` symbol references. The only "JUCE-pluginterfaces-adjacent" type allowed is the typedef alias `juce::VST3ClientExtensions::Vst3RawEvent` (which is a JUCE-side struct defined in `juce_VST3ClientExtensions.h:72` — verified JUCE-side, not Steinberg-side).

```cpp
/*
  ==============================================================================
    NoteExpression.cpp — SharedCode-bound translation unit for note-expression
    module v1.0.0.

    Plan 23-05 amended D-22 — two-TU split:
      - This file (cpp/NoteExpression.cpp) is picked up by the new non-recursive
        `file(GLOB cpp/*.cpp ...)` glob in OuariconModules.cmake and links into
        SharedCode (libO-Lyrica-dev_SharedCode.a). Because PluginProcessor (in
        SharedCode) holds `vst3Extensions` as a value member and calls
        `drainAndUpdate()` from processBlock, the SharedCode link line MUST be
        able to resolve VST3Extensions::VST3Extensions(), ~VST3Extensions(),
        and drainAndUpdate(). Those definitions live here.
      - The companion file (cpp/vst3/NoteExpression_VST3.cpp) carries the
        Steinberg-touching code (Controller body, queryIEditController,
        updatePendingFromEvents) and is routed only into ${TARGET}_VST3.

    This file is required to be Steinberg-symbol-free. The non-VST3 link lines
    (AU, Standalone, VST2, AAX, LV2, Unity) link this file via SharedCode but
    DO NOT link pluginterfaces, so any `Steinberg::*` reference here would
    re-introduce the D-23-04-A undefined-symbol failure class.

    Dispatch-slot pattern:
      - This TU owns `std::atomic<NEUpdateFn> g_neUpdate {nullptr}`.
      - drainAndUpdate() loads the slot and dispatches if non-null.
      - When the VST3 TU is linked, its static-init calls
        registerNEUpdate(&updatePendingFromEvents) at TU load — populating the
        slot with the Steinberg-aware NE-correlation body.
      - When the VST3 TU is NOT linked (AU/Standalone), the slot stays nullptr
        and drainAndUpdate skips correlation. This is correct: non-VST3 hosts
        cannot deliver kTuningTypeID events anyway.

    Custom-deleter pimpl (D-21 amended):
      - VST3Extensions::nec is `std::unique_ptr<Controller, void(*)(Controller*)>`.
      - This file initializes `nec(nullptr, &noopControllerDelete)` in the ctor.
      - The dtor calls the function pointer, NOT `delete nec.get()`, so it can
        compile without seeing Controller's body. When nec is null,
        noopControllerDelete is a no-op.
      - The VST3 TU's queryIEditController lazy-creates the Controller via
        move-assignment with realControllerDelete (defined in that TU), which
        atomically swaps both the managed pointer AND the deleter.
  ==============================================================================
*/

#include "NoteExpression.h"

#include <atomic>

namespace Ouaricon::NoteExpression
{

//==============================================================================
// noopControllerDelete — internal-linkage no-op deleter.
//
// Used as the initial deleter for `nec` in VST3Extensions's ctor. Only ever
// invoked when nec holds a nullptr (e.g. AU/Standalone builds where the VST3
// TU's lazy-create swap-deleter pattern never runs). With nullptr managed
// pointer, the unique_ptr's dtor still calls the deleter; this no-op accepts
// the call and does nothing.
//
// Internal linkage (anonymous namespace): the function pointer's address is
// taken in the ctor below, so the symbol must exist in this TU but does not
// need external visibility.
//==============================================================================
namespace
{
    void noopControllerDelete (Controller* /*p*/) noexcept
    {
        // No-op. Real deletion happens via realControllerDelete defined in
        // cpp/vst3/NoteExpression_VST3.cpp, which is installed via swap-deleter
        // during lazy-create in queryIEditController.
    }

    //==========================================================================
    // Dispatch slot for NE-correlation hand-off (D-22 amended).
    //
    // Non-zero only when the VST3-only TU has been linked (its static-init
    // registers updatePendingFromEvents into this slot). Audio-thread readers
    // use acquire ordering paired with the VST3 TU's release-store registration.
    //==========================================================================
    std::atomic<NEUpdateFn> g_neUpdate { nullptr };
}

//==============================================================================
// registerNEUpdate — namespace-scoped (external linkage). Called by the VST3
// TU's static-init at TU load. Single-write expected; release-store semantics
// pair with the audio-thread acquire-load in drainAndUpdate.
//==============================================================================
void registerNEUpdate (NEUpdateFn fn) noexcept
{
    g_neUpdate.store (fn, std::memory_order_release);
}

//==============================================================================
// VST3Extensions out-of-line members — SharedCode-bound (D-22 amended).
//
// These three symbols MUST link from SharedCode because PluginProcessor (in
// SharedCode) holds `vst3Extensions` as a value member and calls
// drainAndUpdate() from processBlock. Defining them here keeps the AU /
// Standalone / VST2 / AAX / LV2 / Unity link lines resolvable without pulling
// in any Steinberg symbol.
//==============================================================================

VST3Extensions::VST3Extensions()
    : nec (nullptr, &noopControllerDelete)   // custom-deleter pimpl per D-21 amended
{
    // Reserves match the previous header inline ctor (T-23-02: no audio-thread
    // allocation; pre-reserve to 64 events per block).
    blockEvents.reserve (64);
    rawEventScratch.reserve (64);
}

VST3Extensions::~VST3Extensions() = default;
// = default works because nec's deleter is a function pointer (the stored
// noopControllerDelete or the swapped-in realControllerDelete), NOT a default
// `delete Controller*`. The unique_ptr dtor instantiation only needs to call
// the function pointer — Controller's body is irrelevant at this site.

void VST3Extensions::drainAndUpdate()
{
    drainBlockEvents (rawEventScratch);

    // Dispatch via slot. Non-null iff the VST3 TU is linked and its static-init
    // registered updatePendingFromEvents. Acquire ordering pairs with the VST3
    // TU's release-store via registerNEUpdate.
    if (auto fn = g_neUpdate.load (std::memory_order_acquire))
        fn (rawEventScratch, pendingTable);

    // Non-VST3 builds: fn is null, correlation is skipped. Correct because
    // non-VST3 hosts cannot deliver kTuningTypeID events to this plugin.
}

} // namespace Ouaricon::NoteExpression
```

**Notes for the executor:**
- `#include "NoteExpression.h"` is the unquoted relative path; `cpp/NoteExpression.cpp` and `cpp/NoteExpression.h` are siblings.
- Do NOT add any `<pluginterfaces/...>` include here. If `grep -c '#include <pluginterfaces' cpp/NoteExpression.cpp` returns anything but `0`, the task has failed.
- Do NOT reference any `Steinberg::*` symbol here. The body uses only `juce::VST3ClientExtensions::Vst3RawEvent` (typedef), `std::atomic`, `std::unique_ptr`'s dtor, and `std::memory_order_*`. All Steinberg-touching work is dispatched through the function-pointer slot.
- `noopControllerDelete` lives in an anonymous namespace (internal linkage). Its address is taken in the ctor — the compiler is free to inline; what matters is that the function pointer value is well-defined throughout the TU.
- `registerNEUpdate` is namespace-scoped (external linkage) so the VST3 TU's static-init can call it.
- `g_neUpdate` is in the anonymous namespace (internal linkage) — only this TU's `drainAndUpdate` reads it, only this TU's `registerNEUpdate` writes it.
- After this task, the new file exists but is NOT yet compiled (CMake glob in Task 4 picks it up).
  </action>
  <verify>
    <automated>test -f modules/tuning/note-expression/cpp/NoteExpression.cpp && grep -c '#include <pluginterfaces' modules/tuning/note-expression/cpp/NoteExpression.cpp | grep -q '^0$' && grep -c 'Steinberg::' modules/tuning/note-expression/cpp/NoteExpression.cpp | grep -q '^0$' && grep -c 'VST3Extensions::VST3Extensions' modules/tuning/note-expression/cpp/NoteExpression.cpp | grep -qE '^[1-9]' && grep -c 'VST3Extensions::~VST3Extensions' modules/tuning/note-expression/cpp/NoteExpression.cpp | grep -qE '^[1-9]' && grep -c 'VST3Extensions::drainAndUpdate' modules/tuning/note-expression/cpp/NoteExpression.cpp | grep -qE '^[1-9]' && grep -c 'noopControllerDelete' modules/tuning/note-expression/cpp/NoteExpression.cpp | grep -qE '^[2-9]|^[1-9][0-9]+$' && grep -c 'std::atomic<NEUpdateFn>' modules/tuning/note-expression/cpp/NoteExpression.cpp | grep -qE '^[1-9]' && grep -c 'registerNEUpdate' modules/tuning/note-expression/cpp/NoteExpression.cpp | grep -qE '^[2-9]|^[1-9][0-9]+$'</automated>
  </verify>
  <acceptance_criteria>
    - File `modules/tuning/note-expression/cpp/NoteExpression.cpp` exists.
    - `grep -c '#include <pluginterfaces' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns `0` (NO Steinberg includes).
    - `grep -c 'Steinberg::' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns `0` (NO direct Steinberg type/symbol references).
    - `grep -c '#include "NoteExpression.h"' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns `1`.
    - `grep -c 'VST3Extensions::VST3Extensions' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns at least `1` (ctor definition).
    - `grep -c 'VST3Extensions::~VST3Extensions' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns at least `1` (dtor definition).
    - `grep -c 'VST3Extensions::drainAndUpdate' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns at least `1` (drainAndUpdate definition).
    - `grep -c 'noopControllerDelete' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns at least `2` (definition + reference in ctor's mem-init).
    - `grep -c 'std::atomic<NEUpdateFn>' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns at least `1` (dispatch slot storage).
    - `grep -c 'g_neUpdate' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns at least `2` (declaration + load in drainAndUpdate + store in registerNEUpdate).
    - `grep -c 'registerNEUpdate' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns at least `1` (definition).
    - `grep -c 'memory_order_acquire' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns at least `1` (audio-thread load semantics).
    - `grep -c 'memory_order_release' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns at least `1` (registerNEUpdate store semantics).
    - `grep -c 'reserve (64)' modules/tuning/note-expression/cpp/NoteExpression.cpp` returns `2` (blockEvents + rawEventScratch reserves preserved verbatim from the previous header inline ctor).
    - JUCE-side type alias allowed: `grep -c 'Vst3RawEvent' /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h` returns at least `1` (sanity check that `Vst3RawEvent` is JUCE-side, not Steinberg-side; this acceptance also confirms our `juce::VST3ClientExtensions::Vst3RawEvent` typedef use in the header is safe).
  </acceptance_criteria>
  <done>cpp/NoteExpression.cpp exists, contains VST3Extensions::VST3Extensions / ~VST3Extensions / drainAndUpdate definitions, noopControllerDelete in an anonymous namespace, the dispatch slot storage (std::atomic<NEUpdateFn> g_neUpdate), and registerNEUpdate definition. ZERO Steinberg includes, ZERO Steinberg::* references — links cleanly into AU, Standalone, and any non-VST3 format.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Create cpp/vst3/NoteExpression_VST3.cpp (Steinberg-touching VST3-only TU)</name>
  <files>
    modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp
  </files>
  <read_first>
    - modules/tuning/note-expression/cpp/NoteExpression.h (current pre-strip state — the bodies inside lines 126-227 (Controller class), 247-269 (queryIEditController VST3 branch), 87-115 (updatePendingFromEvents inline) become the canonical content of this new file)
    - modules/tuning/note-expression/cpp/NoteExpression.cpp (just created in Task 1 — confirms the dispatch-slot accessor `registerNEUpdate(NEUpdateFn)` is the wiring contract between the two TUs)
    - .planning/phases/23-extract/23-CONTEXT.md (D-20, D-21 amended, D-22 amended, D-23 — exact specification)
    - .planning/phases/23-extract/23-04-version-readme-dorico-smoketest-SUMMARY.md (D-23-04-A — `Undefined symbols` list; this TU is the ONLY home for those symbols going forward)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md (NEC + queryIEditController patterns; reference for the Steinberg API contract)
    - .claude/skills/spike-findings-VST-development/sources/shared-code/NoteExpressionSupport.spike.h (canonical pre-strip spike — for any detail not preserved in the current header)
  </read_first>
  <action>
**Create the directory** `modules/tuning/note-expression/cpp/vst3/` (mkdir -p; the per-format CMake routing in Task 4 will pick it up automatically).

**Write `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp`** with the exact content below. This is the ONLY translation unit that may include `<pluginterfaces/...>` headers.

```cpp
/*
  ==============================================================================
    NoteExpression_VST3.cpp — VST3-only translation unit for note-expression
    module v1.0.0.

    Plan 23-05 amended D-22 — two-TU split:
      - This file owns all `<pluginterfaces/...>` includes and all
        `Steinberg::*` symbol references.
      - This file compiles ONLY into ${TARGET}_VST3 via the per-format routing
        in modules/cmake/OuariconModules.cmake (D-27). SharedCode does NOT
        compile this file, so non-VST3 link lines never see Steinberg symbols.
      - Companion SharedCode-bound TU (cpp/NoteExpression.cpp) owns the
        VST3Extensions ctor/dtor/drainAndUpdate (Steinberg-free).

    Contents:
      - Full Controller class body (NEC implementation; subclass of
        Steinberg::Vst::INoteExpressionController).
      - realControllerDelete — internal-linkage actual deleter, swapped into
        nec via the lazy-create idiom in queryIEditController.
      - VST3Extensions::queryIEditController body — references
        INoteExpressionController::iid; lazy-creates Controller on first call.
      - updatePendingFromEvents free helper — references kTuningTypeID; called
        by SharedCode's drainAndUpdate via the dispatch slot.
      - Static-init dispatcher: registers updatePendingFromEvents into the
        SharedCode TU's std::atomic<NEUpdateFn> g_neUpdate slot at TU load.

    Custom-deleter pimpl (D-21 amended) — load-bearing detail:
      - The header declares `std::unique_ptr<Controller, void(*)(Controller*)> nec;`
      - SharedCode's ctor initializes it with `noopControllerDelete`.
      - When THIS TU's queryIEditController is invoked (VST3 host only), it
        lazy-creates a Controller AND swaps in realControllerDelete via:
          nec = std::unique_ptr<Controller, void(*)(Controller*)>(
                    new Controller, &realControllerDelete);
      - The move-assignment atomically swaps both the managed pointer AND the
        deleter member of the unique_ptr. (Note: unique_ptr's deleter slot is
        immutable post-construction in the sense that you can't `.reset(ptr)`
        and pick a new deleter — `.reset()` keeps the existing deleter. The
        only way to install a different deleter is to construct a fresh
        unique_ptr and move-assign it.)
  ==============================================================================
*/

#include "../NoteExpression.h"

#include <pluginterfaces/vst/ivstnoteexpression.h>
#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/base/ustring.h>
#include <public.sdk/source/common/pluginview.h>

#include <cstdio>
#include <cstring>
#include <map>

namespace Ouaricon::NoteExpression
{

//==============================================================================
/** Advertises kTuningTypeID as a supported Note Expression for all (busIndex,
    channel) pairs. Dorico queries this on plugin load to decide whether to
    send NE or fall back to pitch bend.
*/
class Controller : public Steinberg::Vst::INoteExpressionController
{
public:
    Controller() = default;

    Steinberg::int32 PLUGIN_API getNoteExpressionCount (Steinberg::int32 /*busIndex*/,
                                                        Steinberg::int16 /*channel*/) override
    {
        return 1;
    }

    Steinberg::tresult PLUGIN_API getNoteExpressionInfo (Steinberg::int32 /*busIndex*/,
                                                         Steinberg::int16 /*channel*/,
                                                         Steinberg::int32 noteExpressionIndex,
                                                         Steinberg::Vst::NoteExpressionTypeInfo& info) override
    {
        if (noteExpressionIndex != 0)
            return Steinberg::kResultFalse;

        std::memset (&info, 0, sizeof (info));
        info.typeId = Steinberg::Vst::kTuningTypeID;
        Steinberg::UString (info.title,      128).assign (STR16 ("Tuning"));
        Steinberg::UString (info.shortTitle, 128).assign (STR16 ("Tun"));
        Steinberg::UString (info.units,      128).assign (STR16 ("semitones"));
        info.unitId = -1;
        info.valueDesc.defaultValue = 0.5;
        info.valueDesc.minimum      = 0.0;
        info.valueDesc.maximum      = 1.0;
        info.valueDesc.stepCount    = 0;
        info.associatedParameterId  = Steinberg::Vst::kNoParamId;
        info.flags = Steinberg::Vst::NoteExpressionTypeInfo::kIsBipolar
                   | Steinberg::Vst::NoteExpressionTypeInfo::kIsAbsolute;
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API getNoteExpressionStringByValue (Steinberg::int32 /*busIndex*/,
                                                                  Steinberg::int16 /*channel*/,
                                                                  Steinberg::Vst::NoteExpressionTypeID id,
                                                                  Steinberg::Vst::NoteExpressionValue valueNormalized,
                                                                  Steinberg::Vst::String128 string) override
    {
        if (id != Steinberg::Vst::kTuningTypeID)
            return Steinberg::kResultFalse;

        // Format "+/-N.NN" as ASCII, then widen to char16. Avoids juce::String <-> UString
        // conversion pitfalls. This is display-only; Dorico doesn't rely on it.
        const double semitones = 240.0 * (valueNormalized - 0.5);
        char ascii[32];
        std::snprintf (ascii, sizeof (ascii), "%.2f", semitones);
        int i = 0;
        for (; i < 31 && ascii[i] != '\0'; ++i)
            string[i] = (Steinberg::Vst::TChar) (unsigned char) ascii[i];
        string[i] = 0;
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API getNoteExpressionValueByString (Steinberg::int32 /*busIndex*/,
                                                                  Steinberg::int16 /*channel*/,
                                                                  Steinberg::Vst::NoteExpressionTypeID id,
                                                                  const Steinberg::Vst::TChar* string,
                                                                  Steinberg::Vst::NoteExpressionValue& valueNormalized) override
    {
        if (id != Steinberg::Vst::kTuningTypeID)
            return Steinberg::kResultFalse;

        // Narrow char16 -> ASCII for atof. Values only contain digits / '.' / '-' / '+'.
        char ascii[32] = {};
        for (int i = 0; i < 31 && string[i] != 0; ++i)
            ascii[i] = (char) (string[i] & 0x7F);
        const double semitones = std::atof (ascii);
        valueNormalized = juce::jlimit (0.0, 1.0, semitones / 240.0 + 0.5);
        return Steinberg::kResultTrue;
    }

    //==============================================================================
    // FUnknown / refcount plumbing — NEC is owned by the extensions object so the
    // refcount is effectively ignored, but Steinberg hosts may still query it.
    Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID iid, void** obj) override
    {
        if (obj == nullptr)
            return Steinberg::kInvalidArgument;

        if (Steinberg::FUnknownPrivate::iidEqual (iid, Steinberg::Vst::INoteExpressionController::iid)
         || Steinberg::FUnknownPrivate::iidEqual (iid, Steinberg::FUnknown::iid))
        {
            *obj = static_cast<Steinberg::Vst::INoteExpressionController*> (this);
            addRef();
            return Steinberg::kResultOk;
        }

        *obj = nullptr;
        return Steinberg::kNoInterface;
    }

    Steinberg::uint32 PLUGIN_API addRef()  override { return ++refCount; }
    Steinberg::uint32 PLUGIN_API release() override { return --refCount; }

private:
    std::atomic<Steinberg::uint32> refCount { 1 };
};

//==============================================================================
// realControllerDelete — internal-linkage real deleter. Swapped into nec by
// the lazy-create idiom in queryIEditController. Controller is complete in
// THIS TU, so `delete p` is a complete-type expression here.
//==============================================================================
namespace
{
    void realControllerDelete (Controller* p) noexcept
    {
        delete p;
    }
}

//==============================================================================
// updatePendingFromEvents — body of the free helper declared in
// NoteExpression.h. References Steinberg::Vst::kTuningTypeID, so its body
// must live in this VST3-only TU. Called by SharedCode's drainAndUpdate via
// the std::atomic<NEUpdateFn> dispatch slot (registered at static-init below).
//==============================================================================
void updatePendingFromEvents (
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

//==============================================================================
// VST3Extensions::queryIEditController — body lives here because it references
// INoteExpressionController::iid and FUnknown::iid (Steinberg symbols).
//
// Lazy-create swap-deleter idiom (LOAD-BEARING — D-21 amended):
//   The header declares `nec` as
//     std::unique_ptr<Controller, void(*)(Controller*)>
//   initialized in SharedCode's ctor with the no-op deleter. unique_ptr's
//   deleter member is fixed at construction; .reset(ptr) reuses the existing
//   deleter. The only way to install a different deleter is to construct a
//   fresh unique_ptr (with the new deleter and the new managed pointer) and
//   move-assign into nec — the move-assignment atomically swaps BOTH the
//   managed pointer AND the deleter slot.
//==============================================================================
int32_t VST3Extensions::queryIEditController (const Steinberg::TUID targetIID, void** obj)
{
    if (Steinberg::FUnknownPrivate::iidEqual (targetIID, Steinberg::Vst::INoteExpressionController::iid))
    {
        if (! nec)
        {
            // Move-assign a fresh unique_ptr that carries (a) the freshly
            // constructed Controller and (b) the real deleter. This swaps both
            // slots atomically — replacing the noopControllerDelete that was
            // installed by SharedCode's ctor. Subsequent dtor of `nec` will
            // call realControllerDelete (which actually `delete`s the object,
            // because Controller is complete in THIS TU).
            nec = std::unique_ptr<Controller, void(*)(Controller*)> (
                      new Controller, &realControllerDelete);
        }
        nec->addRef();
        *obj = static_cast<Steinberg::Vst::INoteExpressionController*> (nec.get());
        return Steinberg::kResultOk;
    }

    *obj = nullptr;
    return Steinberg::kNoInterface;
}

//==============================================================================
// Static-init dispatch registration (D-22 amended).
//
// Registers updatePendingFromEvents into the SharedCode-bound TU's
// std::atomic<NEUpdateFn> g_neUpdate slot at TU load. Static-init is correct
// here because:
//   (a) the VST3 host loads the wrapper before any audio-thread callback
//       fires — TU-load static initializers run during dlopen/LoadLibrary,
//       which completes before the host calls processBlock;
//   (b) registerNEUpdate is a single atomic store (release ordering), well-
//       defined under the C++ memory model; the audio-thread load in
//       drainAndUpdate uses paired acquire ordering.
//
// When this file is NOT linked (AU / Standalone / VST2 / AAX / LV2 / Unity
// — all non-VST3 builds), the static-init never runs, the slot stays nullptr,
// and SharedCode's drainAndUpdate skips correlation. Correct: non-VST3 hosts
// cannot deliver kTuningTypeID events.
//==============================================================================
namespace
{
    struct DispatchRegistrar
    {
        DispatchRegistrar() noexcept
        {
            registerNEUpdate (&updatePendingFromEvents);
        }
    };

    [[maybe_unused]] static const DispatchRegistrar g_dispatchRegistrar;
}

} // namespace Ouaricon::NoteExpression
```

**Notes for the executor:**
- The `#include "../NoteExpression.h"` path is correct: `cpp/vst3/NoteExpression_VST3.cpp` → `../NoteExpression.h` → `cpp/NoteExpression.h`.
- All Steinberg includes live in this file ONLY. Do NOT add any `<pluginterfaces/...>` to `cpp/NoteExpression.h` (Task 3) or `cpp/NoteExpression.cpp` (Task 1).
- `Controller` is defined inside the same `Ouaricon::NoteExpression` namespace as the forward declaration in the header — they refer to the same type. The `nec.get()` in queryIEditController is well-typed because the unique_ptr's element type is `Controller` and Controller is complete here.
- `realControllerDelete` is in an anonymous namespace (internal linkage). Its address is taken in queryIEditController via `&realControllerDelete` — that pointer travels with the `nec` unique_ptr.
- `DispatchRegistrar`'s ctor calls `registerNEUpdate` once at TU load. The `[[maybe_unused]] static const` qualifier suppresses unused-variable warnings.
- After this task, the new file exists but is NOT yet compiled (CMake per-format routing in Task 4 picks it up).
  </action>
  <verify>
    <automated>test -f modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp && grep -c '#include <pluginterfaces' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp | grep -qE '^[1-9]' && grep -c '#include "../NoteExpression.h"' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp | grep -q '^1$' && grep -c 'class Controller : public Steinberg::Vst::INoteExpressionController' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp | grep -q '^1$' && grep -c 'VST3Extensions::queryIEditController' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp | grep -qE '^[1-9]' && grep -c '^void updatePendingFromEvents ' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp | grep -q '^1$' && grep -c 'realControllerDelete' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp | grep -qE '^[2-9]' && grep -c 'registerNEUpdate' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp | grep -qE '^[1-9]' && grep -c 'kTuningTypeID' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp | grep -qE '^[3-9]'</automated>
  </verify>
  <acceptance_criteria>
    - File `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` exists.
    - `grep -c '#include <pluginterfaces/vst/ivstnoteexpression.h>' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns `1`.
    - `grep -c '#include <pluginterfaces/base/ustring.h>' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns `1`.
    - `grep -c '#include "../NoteExpression.h"' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns `1`.
    - `grep -c 'class Controller : public Steinberg::Vst::INoteExpressionController' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns `1`.
    - `grep -c 'INoteExpressionController' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns at least `2` (base class + iid reference in queryIEditController).
    - `grep -c '^int32_t VST3Extensions::queryIEditController' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns `1`.
    - `grep -c '^void updatePendingFromEvents ' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns `1`.
    - `grep -c 'Steinberg::Vst::kTuningTypeID' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns at least `3` (Controller body x2, updatePendingFromEvents x1).
    - `grep -c 'Steinberg::UString' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns at least `3` (title, shortTitle, units assignments in Controller::getNoteExpressionInfo).
    - `grep -c 'realControllerDelete' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns at least `2` (definition + reference in queryIEditController's lazy-create).
    - `grep -c 'registerNEUpdate' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns at least `1` (call from DispatchRegistrar ctor — the static-init dispatch registration that wires updatePendingFromEvents into the SharedCode slot).
    - `grep -c 'DispatchRegistrar' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns at least `2` (struct definition + static instance).
    - `grep -c 'std::unique_ptr<Controller, void(\*)(Controller\*)>' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns at least `1` (the lazy-create swap-deleter idiom in queryIEditController — the LOAD-BEARING line).
    - `grep -c 'new Controller, &realControllerDelete' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns at least `1` (lazy-create installs realControllerDelete via move-assignment).
    - `grep -c 'nec->addRef' modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns at least `1` (refcount bump on host query).
  </acceptance_criteria>
  <done>cpp/vst3/NoteExpression_VST3.cpp exists with full Controller class body, VST3Extensions::queryIEditController body using the lazy-create swap-deleter idiom, updatePendingFromEvents free helper body, realControllerDelete in an anonymous namespace, and a static-init DispatchRegistrar that calls registerNEUpdate(&updatePendingFromEvents) at TU load. All Steinberg includes consolidated here.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Strip cpp/NoteExpression.h to Steinberg-free public header (custom-deleter pimpl)</name>
  <files>
    modules/tuning/note-expression/cpp/NoteExpression.h
  </files>
  <read_first>
    - modules/tuning/note-expression/cpp/NoteExpression.h (full file — current state, 313 lines, with Steinberg includes at lines 28-33, Controller class at 126-227, queryIEditController VST3 branch at 247-269, nec member guard at 304-306, updatePendingFromEvents inline at 87-115, inline ctor body at 241-245)
    - modules/tuning/note-expression/cpp/NoteExpression.cpp (Task 1) — confirms `registerNEUpdate(NEUpdateFn)` and the `NEUpdateFn` typedef are the wiring contract that must appear in the header
    - modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp (Task 2) — confirms the symbols moved out-of-line match the header declarations being added
    - .planning/phases/23-extract/23-CONTEXT.md (D-20, D-21 amended, D-22 amended, D-23)
    - plugins/O-Lyrica/Source/PluginProcessor.h, plugins/O-Lyrica/Source/HarpSynthVoice.h (consumer-side; verify the post-strip header still satisfies their `Ouaricon::NoteExpression::VST3Extensions` and `PendingTuningTable*` usages — D-23 invariant)
  </read_first>
  <action>
**Replace `modules/tuning/note-expression/cpp/NoteExpression.h`** entirely with the content below.

This strips:
- All `<pluginterfaces/...>` includes (current lines 28-33, including the `#if JucePlugin_Build_VST3 ... #endif` wrapping).
- The full inline `Controller` class body (current lines 126-227, including the `#if JucePlugin_Build_VST3` / `#endif` wrapping).
- The VST3 branch of `queryIEditController` (current lines 247-270).
- The `#if JucePlugin_Build_VST3` guard on `Controller nec;` (current lines 304-306) — `nec` becomes a custom-deleter unique_ptr unconditionally.
- The inline body of `updatePendingFromEvents` (current lines 87-115 — becomes a forward declaration).
- The inline `VST3Extensions()` ctor body (current lines 241-245 — becomes a declaration; body moves to Task 1's cpp/NoteExpression.cpp).

And adds:
- Forward declaration of `Controller`.
- `NEUpdateFn` typedef + `registerNEUpdate` declaration (the wiring contract for the dispatch slot).
- Forward declaration of `updatePendingFromEvents` (signature unchanged, body removed).
- Out-of-line declarations of `VST3Extensions::VST3Extensions()`, `~VST3Extensions()`, `queryIEditController(...)`, `drainAndUpdate()`.
- `std::unique_ptr<Controller, void(*)(Controller*)> nec;` member (custom function-pointer deleter — D-21 amended).
- `<memory>` include for `std::unique_ptr`.

**Exact final content:**

```cpp
/*
  ==============================================================================
    NoteExpression.h — note-expression module v1.0.0
    VST3 Note Expression (kTuningTypeID) support for Dorico microtonal playback.
    Public API lives under the Ouaricon::NoteExpression nested namespace.

    Translation-unit segregation (Plan 23-05 amended D-22) — two-TU split:
      - This header is Steinberg-free. SharedCode (used by AU / Standalone /
        VST2 / AAX / LV2 / Unity link lines) can include it without pulling in
        any pluginterfaces symbol.
      - cpp/NoteExpression.cpp (SharedCode-bound) defines VST3Extensions ctor
        and dtor and drainAndUpdate. Zero Steinberg refs. Links into every
        format's link line.
      - cpp/vst3/NoteExpression_VST3.cpp (VST3-only via OuariconModules.cmake
        per-format routing) defines the Controller body, queryIEditController,
        updatePendingFromEvents, and a static-init that registers
        updatePendingFromEvents into the SharedCode TU's dispatch slot.

    Custom-deleter pimpl (D-21 amended):
      - VST3Extensions::nec is std::unique_ptr<Controller, void(*)(Controller*)>.
      - SharedCode's ctor initializes nec(nullptr, &noopControllerDelete) — a
        no-op deleter defined in cpp/NoteExpression.cpp.
      - The VST3 TU's queryIEditController lazy-creates the Controller via
        move-assignment with realControllerDelete (defined in that TU), which
        atomically swaps both the managed pointer AND the deleter slot of nec.
      - The custom function-pointer deleter is what allows the SharedCode-bound
        dtor (= default) to compile without seeing Controller's body — the
        unique_ptr dtor calls a function pointer, never `delete Controller*`
        directly, so Controller's definition is irrelevant at the dtor
        instantiation site.
  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace Ouaricon::NoteExpression
{

//==============================================================================
/** 128-slot atomic table of pending NE tuning offsets, indexed by MIDI pitch.
    Owned by VST3Extensions; voices receive a pointer via setPendingTuningSource.
    Each slot stores a semitone delta (in 12-TET semitones, the unit Dorico uses
    when computing the offset against the EDO12 neighbour pitch).
*/
using PendingTuningTable = std::array<std::atomic<double>, 128>;

//==============================================================================
/** Consumes the pending NE tuning delta for `midiNoteNumber` and returns the
    new frequency. exchange(0.0) ensures a retriggered note at the same pitch
    in a later block does not inherit a stale offset. Encapsulates the
    pow(2, semis/12) call so voice code never invokes it directly. Composes
    multiplicatively with the caller's base frequency — caller should pass the
    frequency AFTER any TuningEngine / humanize lookup (D-10).

    This helper is Steinberg-free and therefore stays inline in the header.
*/
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

//==============================================================================
// Forward declarations — full bodies live in cpp/vst3/NoteExpression_VST3.cpp.

/** NEC implementation. Forward-declared so the header carries no Steinberg
    SDK references; full body in cpp/vst3/NoteExpression_VST3.cpp.
*/
class Controller;

/** Two-pass correlation: (1) build noteId -> midi-pitch map from NoteOns in
    the block; (2) for each kTuningTypeID NE, compute 240*(value-0.5) semitones
    and store into table[pitch]. Caller must call
    VST3Extensions::drainBlockEvents(events) immediately before this.

    Defined out-of-line in cpp/vst3/NoteExpression_VST3.cpp because the
    kTuningTypeID reference must not leak into SharedCode (Plan 23-05).
*/
void updatePendingFromEvents (
    const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>& events,
    PendingTuningTable&                                          table);

//==============================================================================
// Dispatch-slot wiring (Plan 23-05 amended D-22).
//
// SharedCode-bound cpp/NoteExpression.cpp owns a std::atomic<NEUpdateFn>
// dispatch slot. The VST3-only cpp/vst3/NoteExpression_VST3.cpp registers its
// updatePendingFromEvents body into that slot via static-init at TU load.
// SharedCode's drainAndUpdate loads the slot and dispatches if non-null.
// Non-VST3 builds: VST3 TU is not linked, slot stays nullptr, correlation is
// skipped — correct because non-VST3 hosts cannot deliver kTuningTypeID events.
using NEUpdateFn = void (*) (
    const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>&,
    PendingTuningTable&);

void registerNEUpdate (NEUpdateFn fn) noexcept;

//==============================================================================
/** VST3 client extensions for note-expression-aware plugins.
    - Advertises the Controller on IEditController queries (VST3 only — body
      lives in cpp/vst3/NoteExpression_VST3.cpp).
    - Buffers raw VST3 events (pushed by the patched JUCE wrapper) so the
      processor can correlate NE tuning deltas with their NoteOn noteIds and
      apply per-voice pitch offsets before voices emit their first sample.
    - Owns the 128-slot PendingTuningTable (D-09): plugins do not need to
      re-declare it, voices receive a pointer via getPendingTable().

    The class itself is fully usable from non-VST3 builds (AU / Standalone /
    VST2 / AAX / LV2 / Unity) — those builds simply never invoke
    queryIEditController, the VST3 TU is not linked, the dispatch slot stays
    nullptr, and drainAndUpdate is a fast pass-through.

    Custom-deleter pimpl (D-21 amended): nec is
        std::unique_ptr<Controller, void(*)(Controller*)>
    which lets the SharedCode-bound dtor (= default in cpp/NoteExpression.cpp)
    compile without seeing Controller's body. The deleter starts as
    noopControllerDelete (cpp/NoteExpression.cpp); queryIEditController's
    lazy-create idiom in the VST3 TU swaps it to realControllerDelete via
    move-assignment.
*/
class VST3Extensions : public juce::VST3ClientExtensions
{
public:
    VST3Extensions();
    ~VST3Extensions() override;

    int32_t queryIEditController (const Steinberg::TUID targetIID, void** obj) override;

    void onVst3RawEvent (const Vst3RawEvent& e) override
    {
        // Called on the audio thread just before processBlock. Plain push is
        // safe: drain happens on the same thread at the top of processBlock.
        // blockEvents is reserved to 64 slots in the constructor (T-23-02).
        blockEvents.push_back (e);
    }

    /** Called by the processor at the top of processBlock. Moves the current
        block's raw events to `out` and clears the internal buffer.
    */
    void drainBlockEvents (std::vector<Vst3RawEvent>& out)
    {
        out.clear();
        out.swap (blockEvents);
        blockEvents.clear();
        blockEvents.reserve (64);
    }

    /** Convenience: drain + correlate in one call. Plugins use this from
        processBlock before renderNextBlock. Defined out-of-line in
        cpp/NoteExpression.cpp; dispatches via the dispatch slot (D-22). */
    void drainAndUpdate();

    /** Voice wiring entry point. Hand this to each voice's
        setPendingTuningSource(). */
    PendingTuningTable& getPendingTable() noexcept { return pendingTable; }

private:
    // Custom function-pointer deleter pimpl (D-21 amended). The deleter slot
    // is set by the ctor (in cpp/NoteExpression.cpp) to noopControllerDelete;
    // the VST3 TU swaps it to realControllerDelete via move-assignment in
    // queryIEditController's lazy-create. The dtor (= default) only invokes
    // the stored function pointer — Controller's body is not required here.
    std::unique_ptr<Controller, void(*)(Controller*)> nec;

    std::vector<Vst3RawEvent>   blockEvents;
    std::vector<Vst3RawEvent>   rawEventScratch;
    PendingTuningTable          pendingTable {};
};

} // namespace Ouaricon::NoteExpression
```

**Notes for the executor:**
- The `Steinberg::TUID` type referenced in `queryIEditController`'s signature comes from `juce::VST3ClientExtensions` itself (it's the base class's override signature). `JuceHeader.h` already pulls in `juce_audio_processors`, which forward-declares `Steinberg::TUID` for the `juce::VST3ClientExtensions::queryIEditController` declaration. No additional include needed in this header.
- `Vst3RawEvent` is defined inside `juce::VST3ClientExtensions` itself (verified: `juce/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h:72`) — already visible via the base class.
- Do NOT add any `#include <pluginterfaces/...>` directive to this file. If any acceptance grep finds one, the task has failed.
- Do NOT add any `#if JucePlugin_Build_VST3` guard to this file. If any acceptance grep finds one, the task has failed.
- The public API surface is preserved verbatim per D-23: same class names, same method names, same signatures. O-Lyrica consumer call-sites (`vst3Extensions.drainAndUpdate()` at PluginProcessor.cpp:708, `vst3Extensions.getPendingTable()` at line 506) are not edited in this plan.
- The `nec` member type changes from `Controller nec;` (value, VST3-guarded) to `std::unique_ptr<Controller, void(*)(Controller*)> nec;` (pimpl, custom function-pointer deleter, unconditional). This is the load-bearing change for D-21 amended.
  </action>
  <verify>
    <automated>test -f modules/tuning/note-expression/cpp/NoteExpression.h && grep -c "pluginterfaces" modules/tuning/note-expression/cpp/NoteExpression.h | grep -q '^0$' && grep -c "JucePlugin_Build_VST3" modules/tuning/note-expression/cpp/NoteExpression.h | grep -q '^0$' && grep -c "^class Controller;" modules/tuning/note-expression/cpp/NoteExpression.h | grep -q '^1$' && grep -c "std::unique_ptr<Controller, void" modules/tuning/note-expression/cpp/NoteExpression.h | grep -qE '^[1-9]' && grep -c "VST3Extensions();" modules/tuning/note-expression/cpp/NoteExpression.h | grep -q '^1$' && grep -c "~VST3Extensions() override;" modules/tuning/note-expression/cpp/NoteExpression.h | grep -q '^1$' && grep -c "void drainAndUpdate();" modules/tuning/note-expression/cpp/NoteExpression.h | grep -q '^1$' && grep -c "registerNEUpdate" modules/tuning/note-expression/cpp/NoteExpression.h | grep -qE '^[1-9]' && grep -c "using NEUpdateFn" modules/tuning/note-expression/cpp/NoteExpression.h | grep -q '^1$'</automated>
  </verify>
  <acceptance_criteria>
    - `grep -c "pluginterfaces" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0` (no Steinberg pluginterfaces includes anywhere in the header).
    - `grep -c "JucePlugin_Build_VST3" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0` (all per-TU guards removed — the ENTIRE point of Plan 23-05).
    - `grep -c "Steinberg::Vst::INoteExpressionController" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0` (Controller's base class is no longer referenced in the header).
    - `grep -c "Steinberg::Vst::kTuningTypeID" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0` (kTuningTypeID reference moved to .cpp).
    - `grep -c "Steinberg::UString" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0`.
    - `grep -c "Steinberg::FUnknown" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0`.
    - `grep -c "^#include <memory>" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1`.
    - `grep -c "^class Controller;" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (forward declaration present).
    - `grep -c "class Controller {" modules/tuning/note-expression/cpp/NoteExpression.h` returns `0` (Controller definition removed from header — moved to VST3 TU).
    - `grep -c "std::unique_ptr<Controller, void" modules/tuning/note-expression/cpp/NoteExpression.h` returns at least `1` (custom function-pointer deleter signature visible — D-21 amended; the ENTIRE point of the dtor-without-complete-Controller workaround).
    - `grep -c "PendingTuningTable" modules/tuning/note-expression/cpp/NoteExpression.h` returns at least `2` (typedef declaration + member type at minimum; usually more for the helper signatures).
    - `grep -cE "^    VST3Extensions\(\);" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (out-of-line ctor declared — no inline body).
    - `grep -cE "^    ~VST3Extensions\(\) override;" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (out-of-line dtor declared).
    - `grep -cE "^    int32_t queryIEditController .*override;" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (out-of-line declaration only — no body).
    - `grep -cE "^    void drainAndUpdate\(\);" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (out-of-line declaration only).
    - `grep -cE "^void updatePendingFromEvents " modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (forward declaration of free helper).
    - `awk '/^void updatePendingFromEvents/,/;$/' modules/tuning/note-expression/cpp/NoteExpression.h | grep -c "^{"` returns `0` (no inline body — declaration ends with `;`).
    - `grep -c "using NEUpdateFn" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (dispatch typedef visible to both TUs).
    - `grep -c "void registerNEUpdate (NEUpdateFn fn) noexcept;" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1` (dispatch registration accessor declared — VST3 TU's static-init calls this; SharedCode TU defines it).
    - Public API preserved: `grep -c "using PendingTuningTable = std::array<std::atomic<double>, 128>;" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1`.
    - Public API preserved: `grep -c "inline double applyPendingTuning " modules/tuning/note-expression/cpp/NoteExpression.h` returns `1`.
    - Public API preserved: `grep -c "PendingTuningTable& getPendingTable()" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1`.
    - Public API preserved: `grep -c "void onVst3RawEvent (const Vst3RawEvent& e) override" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1`.
    - Public API preserved: `grep -c "void drainBlockEvents (std::vector<Vst3RawEvent>& out)" modules/tuning/note-expression/cpp/NoteExpression.h` returns `1`.
  </acceptance_criteria>
  <done>NoteExpression.h is Steinberg-free, Controller is forward-declared, nec is `std::unique_ptr<Controller, void(*)(Controller*)>` (custom function-pointer deleter — D-21 amended), ctor/dtor + queryIEditController + drainAndUpdate + updatePendingFromEvents are all out-of-line declarations only. Dispatch wiring (NEUpdateFn typedef + registerNEUpdate declaration) is visible to both TUs. Public API surface preserved verbatim per D-23 — every public symbol still exists with its original signature.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 4: Extend OuariconModules.cmake with per-format source routing</name>
  <files>
    modules/cmake/OuariconModules.cmake
  </files>
  <read_first>
    - modules/cmake/OuariconModules.cmake (full file — current state, 151 lines; modify only inside `function(ouaricon_add_module ...)` lines 30-107)
    - .planning/phases/23-extract/23-CONTEXT.md (D-24, D-25, D-26, D-27, D-28, D-29 — exact specification of the per-format convention)
    - plugins/O-Lyrica/CMakeLists.txt line 6 (juce_add_plugin) and line 10 (FORMATS VST3 AU Standalone) — confirms which `${OLyrica}_<FORMAT>` subtargets exist when `ouaricon_add_module()` is called at line 80
    - modules/tuning/note-expression/cpp/NoteExpression.cpp (Task 1) — top-level cpp/ file; goes to SharedCode via the narrowed non-recursive glob
    - modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp (Task 2) — confirms the per-format directory `cpp/vst3/` exists and contains a .cpp file that the new routing must pick up
  </read_first>
  <action>
**Modify `modules/cmake/OuariconModules.cmake`** in-place. Two structural changes are required inside the `ouaricon_add_module()` function (lines 30-107):

**Change A — narrow the SharedCode glob (current line 54-57):**

CURRENT (lines 53-64):
```cmake
    # Add C++ sources to target
    if(EXISTS "${MODULE_DIR}/cpp")
        file(GLOB_RECURSE MODULE_CPP_SOURCES
            "${MODULE_DIR}/cpp/*.cpp"
            "${MODULE_DIR}/cpp/*.h"
        )

        if(MODULE_CPP_SOURCES)
            target_sources(${TARGET_NAME} PRIVATE ${MODULE_CPP_SOURCES})
            target_include_directories(${TARGET_NAME} PRIVATE "${MODULE_DIR}/cpp")
            message(STATUS "[Ouaricon]   Added ${CMAKE_CURRENT_LIST_DIR} C++ sources")
        endif()
    endif()
```

REPLACE WITH:
```cmake
    # Add C++ sources to target — SharedCode glob (top-level cpp/ ONLY).
    # Per-format subdirectories (cpp/vst3/, cpp/au/, etc.) are handled in the
    # per-format routing block below and MUST NOT be swept into SharedCode,
    # otherwise format-specific symbol references leak into the shared static
    # library and break non-format link lines (Plan 23-05 D-23-04-A).
    if(EXISTS "${MODULE_DIR}/cpp")
        file(GLOB MODULE_CPP_SOURCES
            "${MODULE_DIR}/cpp/*.cpp"
            "${MODULE_DIR}/cpp/*.h"
        )

        if(MODULE_CPP_SOURCES)
            target_sources(${TARGET_NAME} PRIVATE ${MODULE_CPP_SOURCES})
            target_include_directories(${TARGET_NAME} PRIVATE "${MODULE_DIR}/cpp")
            message(STATUS "[Ouaricon]   Added ${MODULE_NAME} SharedCode sources to ${TARGET_NAME}")
        endif()

        # Per-format routing (Plan 23-05 D-24..D-28): files placed under
        # cpp/<format>/ are added to ${TARGET_NAME}_<FORMAT> only, with PRIVATE
        # include directories so format-specific headers cannot be pulled in by
        # SharedCode or other-format translation units. Silently no-ops when
        # ${TARGET_NAME}_<FORMAT> does not exist (e.g. plugin excludes that
        # format from its FORMATS list).
        set(_OUA_JUCE_FORMATS vst3 au standalone vst2 aax lv2 unity)
        foreach(fmt ${_OUA_JUCE_FORMATS})
            string(TOUPPER ${fmt} _FMT_UPPER)
            set(_FMT_DIR "${MODULE_DIR}/cpp/${fmt}")
            if(EXISTS "${_FMT_DIR}" AND TARGET "${TARGET_NAME}_${_FMT_UPPER}")
                file(GLOB_RECURSE _FMT_SOURCES
                    "${_FMT_DIR}/*.cpp"
                    "${_FMT_DIR}/*.h"
                )
                if(_FMT_SOURCES)
                    target_sources(${TARGET_NAME}_${_FMT_UPPER} PRIVATE ${_FMT_SOURCES})
                    target_include_directories(${TARGET_NAME}_${_FMT_UPPER} PRIVATE "${_FMT_DIR}")
                    message(STATUS "[Ouaricon]   Added ${MODULE_NAME}/cpp/${fmt} sources to ${TARGET_NAME}_${_FMT_UPPER}")
                endif()
            endif()
        endforeach()
    endif()
```

**Two structural changes summarized:**
1. `file(GLOB_RECURSE ...)` → `file(GLOB ...)` on line 54-57 (narrow SharedCode to top-level cpp/ only).
2. Append a per-format loop iterating `vst3 au standalone vst2 aax lv2 unity` AFTER the SharedCode `endif()` and BEFORE the existing JS-copy `if(EXISTS "${MODULE_DIR}/js")` block (current line 67).

**STATUS message wording change:**
Old: `[Ouaricon]   Added ${CMAKE_CURRENT_LIST_DIR} C++ sources` — `CMAKE_CURRENT_LIST_DIR` resolves to the directory containing the .cmake file (i.e. always `modules/cmake/`), which is uninformative.
New: `[Ouaricon]   Added ${MODULE_NAME} SharedCode sources to ${TARGET_NAME}` — distinguishes SharedCode adds from per-format adds in build logs (D-27 sketch).

**Do NOT modify** the JS-copy block (lines 67-80), the `module.cmake` hook block (lines 82-86), the `ARG_CONFIG` block (lines 88-102), or anything outside `ouaricon_add_module()`. The `ouaricon_list_modules()` function (lines 114-131) and `ouaricon_check_module_updates()` function (lines 139-150) are untouched.

**Verify the consumer one-liner stays unchanged (D-27, D-32):** after this edit, `plugins/O-Lyrica/CMakeLists.txt:80` (`ouaricon_add_module(OLyrica note-expression)`) is byte-identical to before. Same for all 7 Phase 24 plugins when they adopt the module.
  </action>
  <verify>
    <automated>grep -c "file(GLOB MODULE_CPP_SOURCES" modules/cmake/OuariconModules.cmake | grep -q '^1$' && grep -c "GLOB_RECURSE MODULE_CPP_SOURCES" modules/cmake/OuariconModules.cmake | grep -q '^0$' && grep -c "_OUA_JUCE_FORMATS vst3 au standalone vst2 aax lv2 unity" modules/cmake/OuariconModules.cmake | grep -q '^1$' && grep -c "TARGET \"\${TARGET_NAME}_\${_FMT_UPPER}\"" modules/cmake/OuariconModules.cmake | grep -q '^1$' && grep -c "target_sources(\${TARGET_NAME}_\${_FMT_UPPER} PRIVATE \${_FMT_SOURCES})" modules/cmake/OuariconModules.cmake | grep -q '^1$' && grep -c "target_include_directories(\${TARGET_NAME}_\${_FMT_UPPER} PRIVATE \"\${_FMT_DIR}\")" modules/cmake/OuariconModules.cmake | grep -q '^1$'</automated>
  </verify>
  <acceptance_criteria>
    - `grep -c "file(GLOB MODULE_CPP_SOURCES" modules/cmake/OuariconModules.cmake` returns `1` (non-recursive SharedCode glob present).
    - `grep -c "GLOB_RECURSE MODULE_CPP_SOURCES" modules/cmake/OuariconModules.cmake` returns `0` (the old recursive glob is GONE — would otherwise sweep cpp/vst3/ into SharedCode and re-introduce the defect).
    - `grep -c "_OUA_JUCE_FORMATS vst3 au standalone vst2 aax lv2 unity" modules/cmake/OuariconModules.cmake` returns `1` (the seven recognized format subdirectories per D-24).
    - `grep -c 'TARGET "${TARGET_NAME}_${_FMT_UPPER}"' modules/cmake/OuariconModules.cmake` returns `1` (per-format target existence guard per D-28).
    - `grep -c 'target_sources(${TARGET_NAME}_${_FMT_UPPER} PRIVATE ${_FMT_SOURCES})' modules/cmake/OuariconModules.cmake` returns `1` (per-format source routing).
    - `grep -c 'target_include_directories(${TARGET_NAME}_${_FMT_UPPER} PRIVATE "${_FMT_DIR}")' modules/cmake/OuariconModules.cmake` returns `1` (PRIVATE per-format include per D-25).
    - `grep -c 'GLOB_RECURSE _FMT_SOURCES' modules/cmake/OuariconModules.cmake` returns `1` (per-format glob IS recursive — modules can nest cpp/vst3/sub/file.cpp).
    - `grep -c 'message(STATUS "\[Ouaricon\]   Added \${MODULE_NAME}/cpp/\${fmt} sources to \${TARGET_NAME}_\${_FMT_UPPER}")' modules/cmake/OuariconModules.cmake` returns `1` (per-format STATUS message).
    - `grep -c 'message(STATUS "\[Ouaricon\]   Added \${MODULE_NAME} SharedCode sources to \${TARGET_NAME}")' modules/cmake/OuariconModules.cmake` returns `1` (updated SharedCode STATUS message).
    - `grep -c "ouaricon_add_module(OLyrica note-expression)" plugins/O-Lyrica/CMakeLists.txt` still returns `1` (consumer one-liner D-32 invariant — no edits to plugin CMakeLists in this plan).
    - The optional `module.cmake` hook block at the end of `ouaricon_add_module()` is preserved: `grep -c 'include("${MODULE_DIR}/module.cmake")' modules/cmake/OuariconModules.cmake` returns `1` (D-34 — no change to JUCE-NE-PATCH marker check pathway).
  </acceptance_criteria>
  <done>OuariconModules.cmake's `ouaricon_add_module()` narrows the SharedCode glob to non-recursive top-level `cpp/*.cpp` + `cpp/*.h`, and adds a per-format loop iterating `vst3 au standalone vst2 aax lv2 unity` that routes `cpp/<format>/` sources to `${TARGET_NAME}_<FORMAT>` via PRIVATE target_sources + target_include_directories. Per-format routing silently no-ops when the format-specific subtarget does not exist. STATUS messages distinguish SharedCode adds from per-format adds. The optional `module.cmake` hook (D-34) is preserved unchanged.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 5: Create scripts/verify-au-link.sh (reusable AU verification gate)</name>
  <files>
    scripts/verify-au-link.sh
  </files>
  <read_first>
    - scripts/verify-backup.sh (style template — bash + set -e + color codes + early exit)
    - scripts/apply-juce-patches.sh (existing analog — same style; verifies the project's bash idiom and color-code variables)
    - plugins/O-Lyrica/CMakeLists.txt (target plugin for the first invocation; lines 6-17 contain `juce_add_plugin(OLyrica ... PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE} PLUGIN_CODE OLyr ... IS_SYNTH TRUE ...)`)
    - CMakeLists.txt at repo root (line 32: `set(OUARICON_MANUFACTURER_CODE OuAu)`, line 37: `set(OUARICON_MANUFACTURER_CODE OuDv)` — the dev-suffix branch is what the installed `O-Lyrica-dev.component` uses)
    - CLAUDE.md (Plugin Cache Clearing protocol — `auval -a | grep -i [pluginname]` is the existing macOS verification step; this script extends that with a targeted `auval -v <type> <subtype> <manuf>` call)
    - .planning/phases/23-extract/23-CONTEXT.md (D-30, D-31 — exact spec of the verify gate the script implements; reused verbatim by every Phase 24 plan)
  </read_first>
  <action>
**Create `scripts/verify-au-link.sh`** with the exact content below. Mark executable with `chmod +x`.

The script: takes a plugin name (e.g. `OLyrica`), parses `PLUGIN_CODE`, `PLUGIN_MANUFACTURER_CODE`, and the AU type (from `IS_SYNTH TRUE` → `aumu`, else `aufx` for effects, with explicit `PLUGIN_AU_MAIN_TYPE` overriding when present) from the plugin's `CMakeLists.txt`. Resolves the manufacturer-code CMake variable to its dev-build value by reading the root `CMakeLists.txt`. Invokes `auval -v <type> <subtype> <manuf>`. Exits 0 on `auval` success, non-zero otherwise. Reusable verbatim by Phase 24 plans (D-31).

```bash
#!/bin/bash
# ==============================================================================
# verify-au-link.sh — AU link + load verification gate
#
# Usage:
#   ./scripts/verify-au-link.sh <PluginName>
#
# Examples:
#   ./scripts/verify-au-link.sh OLyrica
#   ./scripts/verify-au-link.sh O-Bells
#
# Requires:
#   - macOS (auval is macOS-only)
#   - Plugin already built and installed at:
#       ~/Library/Audio/Plug-Ins/Components/<PluginName>-dev.component
#       (or <PluginName>.component for non-dev builds)
#
# What it does:
#   1. Parses PLUGIN_CODE, PLUGIN_MANUFACTURER_CODE, and AU main type from
#      plugins/<PluginName>/CMakeLists.txt.
#   2. Resolves OUARICON_MANUFACTURER_CODE from the root CMakeLists.txt
#      (dev-suffix branch — matches the installed -dev .component).
#   3. Invokes `auval -v <type> <subtype> <manuf>`.
#   4. Exits 0 on auval success (AU loads, validates), non-zero otherwise.
#
# Reusable verbatim by every Phase 24 propagation plan (D-30, D-31).
# Plan 23-05 (note-expression module AU-link defect fix) is the inaugural
# consumer of this gate.
# ==============================================================================

set -e

# Color codes (match scripts/verify-backup.sh style)
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

if [[ $# -lt 1 ]]; then
    echo -e "${RED}Usage: $0 <PluginName>${NC}" >&2
    echo "Example: $0 OLyrica" >&2
    exit 2
fi

PLUGIN="$1"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PLUGIN_CMAKE="$REPO_ROOT/plugins/$PLUGIN/CMakeLists.txt"
ROOT_CMAKE="$REPO_ROOT/CMakeLists.txt"

# 1. Preflight: macOS only
if [[ "$(uname)" != "Darwin" ]]; then
    echo -e "${YELLOW}[verify-au-link] auval is macOS-only — skipping (uname=$(uname))${NC}"
    exit 0
fi

# 2. Preflight: plugin CMakeLists exists
if [[ ! -f "$PLUGIN_CMAKE" ]]; then
    echo -e "${RED}[verify-au-link] Plugin CMakeLists not found: $PLUGIN_CMAKE${NC}" >&2
    exit 3
fi

# 3. Parse PLUGIN_CODE (literal, e.g. "OLyr")
PLUGIN_CODE=$(grep -E '^[[:space:]]*PLUGIN_CODE[[:space:]]+' "$PLUGIN_CMAKE" \
    | head -1 \
    | sed -E 's/^[[:space:]]*PLUGIN_CODE[[:space:]]+([A-Za-z0-9]+).*/\1/')

if [[ -z "$PLUGIN_CODE" || ${#PLUGIN_CODE} -ne 4 ]]; then
    echo -e "${RED}[verify-au-link] Could not parse 4-char PLUGIN_CODE from $PLUGIN_CMAKE${NC}" >&2
    echo -e "Got: '$PLUGIN_CODE'" >&2
    exit 4
fi

# 4. Parse PLUGIN_MANUFACTURER_CODE — may be a literal (4 chars) or a CMake variable reference
MANUF_RAW=$(grep -E '^[[:space:]]*PLUGIN_MANUFACTURER_CODE[[:space:]]+' "$PLUGIN_CMAKE" \
    | head -1 \
    | sed -E 's/^[[:space:]]*PLUGIN_MANUFACTURER_CODE[[:space:]]+([^[:space:]]+).*/\1/')

if [[ -z "$MANUF_RAW" ]]; then
    echo -e "${RED}[verify-au-link] Could not parse PLUGIN_MANUFACTURER_CODE from $PLUGIN_CMAKE${NC}" >&2
    exit 5
fi

# Resolve ${OUARICON_MANUFACTURER_CODE} from root CMakeLists.txt (dev-suffix branch).
# Pattern: lines like `set(OUARICON_MANUFACTURER_CODE OuDv)` (dev) and
# `set(OUARICON_MANUFACTURER_CODE OuAu)` (release). Dev .component is what's
# installed via the build-and-install scripts; prefer the dev value.
if [[ "$MANUF_RAW" == "\${OUARICON_MANUFACTURER_CODE}" || "$MANUF_RAW" == "\$(OUARICON_MANUFACTURER_CODE)" ]]; then
    if [[ ! -f "$ROOT_CMAKE" ]]; then
        echo -e "${RED}[verify-au-link] Root CMakeLists not found: $ROOT_CMAKE${NC}" >&2
        exit 6
    fi
    # Pick the LAST set(...) line — the dev branch follows the release branch in root CMakeLists.txt
    MANUF=$(grep -E '^[[:space:]]*set\(OUARICON_MANUFACTURER_CODE[[:space:]]+' "$ROOT_CMAKE" \
        | tail -1 \
        | sed -E 's/^[[:space:]]*set\(OUARICON_MANUFACTURER_CODE[[:space:]]+([A-Za-z0-9]+).*/\1/')
else
    MANUF="$MANUF_RAW"
fi

if [[ -z "$MANUF" || ${#MANUF} -ne 4 ]]; then
    echo -e "${RED}[verify-au-link] Could not resolve 4-char manufacturer code (raw='$MANUF_RAW' resolved='$MANUF')${NC}" >&2
    exit 7
fi

# 5. Determine AU type (4-char code).
#    - Explicit PLUGIN_AU_MAIN_TYPE wins (rare; not present in O-Lyrica).
#    - IS_SYNTH TRUE -> kAudioUnitType_MusicDevice -> aumu
#    - Otherwise default to kAudioUnitType_Effect -> aufx
AU_MAIN_TYPE_RAW=$(grep -E '^[[:space:]]*PLUGIN_AU_MAIN_TYPE[[:space:]]+' "$PLUGIN_CMAKE" \
    | head -1 \
    | sed -E 's/^[[:space:]]*PLUGIN_AU_MAIN_TYPE[[:space:]]+([A-Za-z0-9_]+).*/\1/')

if [[ -n "$AU_MAIN_TYPE_RAW" ]]; then
    case "$AU_MAIN_TYPE_RAW" in
        kAudioUnitType_MusicDevice)  AU_TYPE="aumu" ;;
        kAudioUnitType_Effect)       AU_TYPE="aufx" ;;
        kAudioUnitType_MusicEffect)  AU_TYPE="aumf" ;;
        kAudioUnitType_Generator)    AU_TYPE="augn" ;;
        kAudioUnitType_MIDIProcessor) AU_TYPE="aumi" ;;
        *) echo -e "${RED}[verify-au-link] Unknown PLUGIN_AU_MAIN_TYPE: $AU_MAIN_TYPE_RAW${NC}" >&2; exit 8 ;;
    esac
elif grep -qE '^[[:space:]]*IS_SYNTH[[:space:]]+TRUE' "$PLUGIN_CMAKE"; then
    AU_TYPE="aumu"
else
    AU_TYPE="aufx"
fi

# 6. Run auval. Default: -v aumu OLyr OuDv
echo -e "${YELLOW}[verify-au-link] Plugin:        $PLUGIN${NC}"
echo -e "${YELLOW}[verify-au-link] AU codes:      type=$AU_TYPE  subtype=$PLUGIN_CODE  manuf=$MANUF${NC}"
echo -e "${YELLOW}[verify-au-link] Running:       auval -v $AU_TYPE $PLUGIN_CODE $MANUF${NC}"

# Clear the AU registrar cache so a fresh load is forced (matches CLAUDE.md
# Plugin Cache Clearing protocol — verifies the on-disk artefact, not a cached
# image of an older one).
killall -9 AudioComponentRegistrar 2>/dev/null || true

# Invoke auval. We do NOT use `set -e` short-circuit here — capture exit code
# explicitly so we can emit a clear error.
set +e
auval -v "$AU_TYPE" "$PLUGIN_CODE" "$MANUF"
RC=$?
set -e

if [[ $RC -eq 0 ]]; then
    echo -e "${GREEN}[verify-au-link] PASS: auval accepted $PLUGIN ($AU_TYPE $PLUGIN_CODE $MANUF)${NC}"
    exit 0
else
    echo -e "${RED}[verify-au-link] FAIL: auval rejected $PLUGIN (exit code $RC)${NC}" >&2
    echo -e "${RED}[verify-au-link] Confirm the .component is installed:${NC}" >&2
    echo -e "  ls -l ~/Library/Audio/Plug-Ins/Components/${PLUGIN}*.component" >&2
    echo -e "${RED}[verify-au-link] Then re-run, or rebuild + install per CLAUDE.md.${NC}" >&2
    exit $RC
fi
```

After writing the file, mark it executable:
```bash
chmod +x scripts/verify-au-link.sh
```

**Notes:**
- Bash style follows `scripts/verify-backup.sh` and `scripts/apply-juce-patches.sh`.
- Codes for OLyrica resolve to: `AU_TYPE=aumu` (IS_SYNTH TRUE → MusicDevice), `PLUGIN_CODE=OLyr`, `MANUF=OuDv` (dev-suffix branch from root CMakeLists.txt:37). So the invocation will be: `auval -v aumu OLyr OuDv`.
- The script is idempotent and safe to re-run; it does not modify any plugin or build state, only invokes `auval`.
- The `killall -9 AudioComponentRegistrar` is per CLAUDE.md Plugin Cache Clearing protocol — forces auval to load the freshly-installed bundle rather than a cached image.
- Exit codes: 0 = pass, 2 = bad args, 3 = plugin CMake missing, 4 = bad PLUGIN_CODE, 5 = no manufacturer code, 6 = root CMake missing, 7 = could not resolve manuf, 8 = unknown AU type, anything else = auval's own exit code on rejection.
- This script is reused verbatim by every Phase 24 plan's verify gate (D-31).
  </action>
  <verify>
    <automated>test -x scripts/verify-au-link.sh && grep -c "auval -v" scripts/verify-au-link.sh | grep -qE '^[1-9]' && grep -c "PLUGIN_CODE" scripts/verify-au-link.sh | grep -qE '^[3-9]|^[1-9][0-9]+$' && grep -c "PLUGIN_MANUFACTURER_CODE" scripts/verify-au-link.sh | grep -qE '^[2-9]|^[1-9][0-9]+$' && grep -c "IS_SYNTH" scripts/verify-au-link.sh | grep -q '^1$' && grep -c "killall -9 AudioComponentRegistrar" scripts/verify-au-link.sh | grep -q '^1$' && bash -n scripts/verify-au-link.sh</automated>
  </verify>
  <acceptance_criteria>
    - File `scripts/verify-au-link.sh` exists.
    - File is executable: `test -x scripts/verify-au-link.sh` succeeds.
    - Bash syntax check passes: `bash -n scripts/verify-au-link.sh` exits 0.
    - `grep -c "^#!/bin/bash" scripts/verify-au-link.sh` returns `1`.
    - `grep -c "^set -e" scripts/verify-au-link.sh` returns `1`.
    - `grep -c "auval -v" scripts/verify-au-link.sh` returns at least `1`.
    - `grep -c "PLUGIN_CODE" scripts/verify-au-link.sh` returns at least `3` (parses, validates, prints).
    - `grep -c "PLUGIN_MANUFACTURER_CODE" scripts/verify-au-link.sh` returns at least `2`.
    - `grep -c "OUARICON_MANUFACTURER_CODE" scripts/verify-au-link.sh` returns at least `2` (resolves the variable from root CMake).
    - `grep -c "IS_SYNTH" scripts/verify-au-link.sh` returns `1` (heuristic for AU type when PLUGIN_AU_MAIN_TYPE absent).
    - `grep -c "PLUGIN_AU_MAIN_TYPE" scripts/verify-au-link.sh` returns at least `1` (handles explicit override).
    - `grep -c "killall -9 AudioComponentRegistrar" scripts/verify-au-link.sh` returns `1` (matches CLAUDE.md protocol).
    - `grep -c "kAudioUnitType_MusicDevice" scripts/verify-au-link.sh` returns `1` (mapping table for AU types).
  </acceptance_criteria>
  <done>scripts/verify-au-link.sh exists, is executable, has valid bash syntax, parses PLUGIN_CODE / PLUGIN_MANUFACTURER_CODE / IS_SYNTH (or PLUGIN_AU_MAIN_TYPE) from `plugins/<PluginName>/CMakeLists.txt`, resolves `${OUARICON_MANUFACTURER_CODE}` from root CMakeLists.txt, invokes `auval -v <type> <subtype> <manuf>`, and exits 0 on auval success / non-zero on rejection. Reusable by Phase 24 plans.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 6: Clean rebuild + fresh install + auval gate (autonomous, broadened build-log grep)</name>
  <files>
    (build/install steps only — no source edits)
  </files>
  <read_first>
    - CLAUDE.md (Plugin Cache Clearing protocol — REQUIRED EXACT SEQUENCE for the rebuild + install)
    - plugins/O-Lyrica/CMakeLists.txt line 11 (`PRODUCT_NAME "O-Lyrica${OUARICON_DEV_SUFFIX}"` — confirms the dev-build artefact filename is `O-Lyrica-dev.vst3` / `O-Lyrica-dev.component`)
    - .planning/phases/23-extract/23-CONTEXT.md (D-30 — verify gate spec verbatim)
    - .planning/phases/23-extract/23-04-version-readme-dorico-smoketest-SUMMARY.md (D-23-04-A — reference for the original `Undefined symbols` strings whose absence proves the original defect is fixed)
  </read_first>
  <action>
**This task verifies the fix end-to-end via clean rebuild, system install per CLAUDE.md, and `auval` runtime check. No source edits.**

Run the full sequence in order. Each sub-step's success is a precondition for the next.

**Sub-step 6.1 — clean reconfigure:**
```bash
cd /Users/taylorbrook/Dev/VST-development/build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release 2>&1 | tee /tmp/plan-23-05-configure.log
```

**Acceptance:** configure log contains the new STATUS messages from Task 4:
- `[Ouaricon]   Added note-expression SharedCode sources to OLyrica`
- `[Ouaricon]   Added note-expression/cpp/vst3 sources to OLyrica_VST3`

And does NOT contain any cmake fatal-error.

**Sub-step 6.2 — clean rebuild of all three OLyrica targets, with broadened undefined-symbol grep:**
```bash
ninja OLyrica_VST3 OLyrica_AU OLyrica_Standalone 2>&1 | tee /tmp/plan-23-05-build.log
echo "Ninja exit code: $?"
```

**Acceptance — broadened (per plan-checker WARNING):**

Check (A) — original D-23-04-A symbols MUST be gone:
```bash
grep -E "Undefined symbols.*Steinberg|INoteExpressionController::iid|Steinberg::UString::assign|Steinberg::FUnknown::iid" /tmp/plan-23-05-build.log
```
**Required:** zero matches.

Check (B) — ANY undefined-symbol failure on ANY link line (catches future symbol leaks beyond the original D-23-04-A names; this is the fix for the plan-checker WARNING that the previous plan would have replaced one undefined-symbol failure class with a NEW one on `Ouaricon::NoteExpression::VST3Extensions::*`):
```bash
grep -E 'Undefined symbols|^[[:space:]]+".+", referenced from:' /tmp/plan-23-05-build.log
```
**Required:** zero matches. If ANY match appears (even if not Steinberg-related), the fix has not landed cleanly — abort and re-investigate Tasks 1, 2, 3.

Check (C) — ninja exits 0:
```bash
echo "Build exit code: $?"
```
**Required:** `0`.

If any of (A), (B), (C) fails, abort and re-investigate which task introduced the regression:
- (A) failure → original Steinberg symbols still leaking; re-check Task 3 (header strip) — `pluginterfaces` includes or `Steinberg::*` refs may still be present in the header.
- (B) failure with `VST3Extensions::*` symbols → ctor/dtor/drainAndUpdate not defined in the SharedCode-bound TU; re-check Task 1 (NoteExpression.cpp).
- (B) failure with `updatePendingFromEvents` symbol → SharedCode is calling it directly instead of via the dispatch slot; re-check Task 1's `drainAndUpdate` body.
- (B) failure with other Ouaricon-namespace symbols → check that all out-of-line declarations in Task 3 have matching definitions in either Task 1 or Task 2.
- (C) failure without (A) or (B) match → unrelated build break; investigate.

**Sub-step 6.3 — fresh system install per CLAUDE.md (EXACT sequence, no paraphrase):**

Per CLAUDE.md "CRITICAL: Plugin Cache Clearing":
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache

rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component
cp -R /Users/taylorbrook/Dev/VST-development/build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/O-Lyrica-dev.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R /Users/taylorbrook/Dev/VST-development/build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/O-Lyrica-dev.component ~/Library/Audio/Plug-Ins/Components/
```

**Acceptance:**
- `test -d ~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3` succeeds.
- `test -d ~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component` succeeds.
- The .component bundle's mtime is newer than 60 seconds (proves it's freshly copied, not stale): `find ~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component -mmin -1 -maxdepth 0` returns the path. (Exact safeguard against the Plan 23-03 stale-Apr-13-artefact failure mode per 23-04 SUMMARY D-23-04-A.)

**Sub-step 6.4 — auval runtime acceptance (D-30 step 3):**
```bash
/Users/taylorbrook/Dev/VST-development/scripts/verify-au-link.sh OLyrica
```

**Acceptance:** Exit code 0. Output ends with `[verify-au-link] PASS: auval accepted OLyrica (aumu OLyr OuDv)`. The exact AU type/subtype/manuf codes are derived by the script from the plugin and root CMakeLists.txt.

**Sub-step 6.5 — auval -a sanity check (CLAUDE.md "Testing Requirements"):**
```bash
auval -a 2>&1 | grep -i lyrica
```
**Acceptance:** Returns at least one line mentioning `O-Lyrica-dev` or `OLyrica`. (auval -a lists all registered AUs; the freshly-installed component must appear.)

**If any sub-step fails:**
- 6.1 configure error → Task 4 has a CMake bug; re-read Task 4 acceptance.
- 6.2 (B) link error mentioning `VST3Extensions::*` Ouaricon symbols → Task 1 didn't define ctor/dtor/drainAndUpdate in the SharedCode TU; re-read Task 1 acceptance.
- 6.2 (B) link error mentioning `updatePendingFromEvents` → Task 1's drainAndUpdate is calling it directly instead of dispatching via the slot; re-read Task 1's drainAndUpdate body.
- 6.2 (A) link error mentioning Steinberg symbols → Task 3 didn't fully strip the header; re-read Task 3 acceptance.
- 6.3 install command failure → check that the build artefact path matches the actual JUCE-generated layout. Verify with `find /Users/taylorbrook/Dev/VST-development/build -name "O-Lyrica-dev.component" -maxdepth 8`.
- 6.4 auval rejection → AU loaded but failed validation; check `auval -v aumu OLyr OuDv` output for the specific failure (signing, plist, IID registration).
- 6.5 no auval -a hits → AU not registered; re-run `killall -9 AudioComponentRegistrar` and try again, then `auval -a | grep -i lyrica`.
  </action>
  <verify>
    <automated>test -d ~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3 && test -d ~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component && /Users/taylorbrook/Dev/VST-development/scripts/verify-au-link.sh OLyrica</automated>
  </verify>
  <acceptance_criteria>
    - `/tmp/plan-23-05-configure.log` contains: `[Ouaricon]   Added note-expression SharedCode sources to OLyrica`
    - `/tmp/plan-23-05-configure.log` contains: `[Ouaricon]   Added note-expression/cpp/vst3 sources to OLyrica_VST3`
    - `ninja OLyrica_VST3 OLyrica_AU OLyrica_Standalone` exits 0.
    - **Original D-23-04-A grep:** `grep -cE "Undefined symbols.*Steinberg|INoteExpressionController::iid|Steinberg::UString::assign|Steinberg::FUnknown::iid" /tmp/plan-23-05-build.log` returns `0`.
    - **Broadened grep (catches ANY undefined-symbol failure, per plan-checker WARNING):** `grep -cE 'Undefined symbols\|^[[:space:]]+".+", referenced from:' /tmp/plan-23-05-build.log` returns `0` (regression-proof against the previous plan's mistake of moving ctor/dtor/drainAndUpdate to the VST3 TU).
    - `test -d ~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3` succeeds.
    - `test -d ~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component` succeeds (proves the AU bundle was actually built and installed — the very thing that failed in 23-04).
    - `find ~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component -mmin -5 -maxdepth 0` returns the path (proves freshness — guard against Plan 23-03's stale-Apr-13 failure mode).
    - `scripts/verify-au-link.sh OLyrica` exits 0 with `PASS: auval accepted OLyrica (aumu OLyr OuDv)`.
    - `auval -a 2>&1 | grep -ci lyrica` returns at least `1` (component is registered with the AU host).
    - Module public API surface preserved (re-verify after build, no consumer-side edits): `grep -c "ouaricon_add_module(OLyrica note-expression)" plugins/O-Lyrica/CMakeLists.txt` returns `1`; `grep -c "vst3Extensions.drainAndUpdate" plugins/O-Lyrica/Source/PluginProcessor.cpp` returns at least `1`; `grep -c "Ouaricon::NoteExpression::applyPendingTuning" plugins/O-Lyrica/Source/HarpSynthVoice.cpp` returns at least `1`.
    - JUCE-NE-PATCH marker check still active (D-34): `/tmp/plan-23-05-configure.log` contains `[note-expression] JUCE-NE-PATCH markers verified`.
  </acceptance_criteria>
  <done>OLyrica VST3 + AU + Standalone all rebuild cleanly with NO undefined-symbol errors of ANY kind in the link logs (broadened grep — catches Steinberg leaks AND would-be Ouaricon-namespace leaks). AU bundle is freshly installed and accepted by auval. The D-23-04-A defect is verified resolved.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 7: Dorico LYR-03 quarter-sharp smoke-test re-pass (regression check)</name>
  <what-built>
    The note-expression module's Steinberg-touching code has been relocated from the header (which was leaking into SharedCode IR and breaking the AU/Standalone link lines) into a TWO-TU split (per amended D-21/D-22):

    - `cpp/NoteExpression.cpp` (NEW, SharedCode-bound) carries `VST3Extensions::VST3Extensions()`, `~VST3Extensions()`, `drainAndUpdate()`, plus the `noopControllerDelete` no-op deleter and the `std::atomic<NEUpdateFn> g_neUpdate` dispatch slot. Zero Steinberg references — links into AU, Standalone, and any non-VST3 format.
    - `cpp/vst3/NoteExpression_VST3.cpp` (NEW, VST3-only) carries the full `Controller` body, `queryIEditController` body (with the lazy-create swap-deleter idiom that installs `realControllerDelete` via move-assignment), `updatePendingFromEvents` body, `realControllerDelete`, and a static-init that registers `updatePendingFromEvents` into the SharedCode dispatch slot.
    - The header is now Steinberg-free; `Controller` is forward-declared and held via `std::unique_ptr<Controller, void(*)(Controller*)>` (custom function-pointer deleter — D-21 amended).

    `OuariconModules.cmake` has been extended with a per-format source-routing convention (`cpp/<format>/` → `${TARGET}_<FORMAT>`). All three OLyrica targets (VST3, AU, Standalone) now link cleanly. `auval` accepts the AU bundle. `scripts/verify-au-link.sh` is now available as a reusable AU verification gate for Phase 24.

    The PUBLIC API surface of the module is preserved verbatim — O-Lyrica's processor and voice code build without any source edits. This human-verify task confirms that the refactor preserved RUNTIME behavior by re-running the LYR-03 Dorico smoke test via VST3.
  </what-built>
  <how-to-verify>
    Repeat the LYR-03 5-test smoke-test from Plan 23-04 verbatim, in Dorico, via the VST3 plugin (NOT the AU — VST3 is the canonical Dorico microtonal path). The freshly-installed `O-Lyrica-dev.vst3` is at `~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3` (Task 6 sub-step 6.3 just placed it).

    1. Open Dorico. Load (or create) a project that uses O-Lyrica with a VST3 Note Expression-enabled expression map (per CHANGELOG [2.3.0] technical notes). If you already have the project from Plan 23-04, reopen it; the saved expression-map setup persists.

    2. **Test 1 — Quarter-sharp C4 pitch accuracy.** Place a quarter-sharp C4 (50 cents above C4 = 261.63 Hz, expected = ~269.29 Hz) and play it. Verify the pitch lands correctly (use Dorico's tuner or external pitch monitor). PASS = pitch is at +50 cents above C4.

    3. **Test 2 — No attack zipper on quarter-sharp C4.** Play the same quarter-sharp C4 with a sharp pluck. Listen for any audible "zipper" / glide at the onset. PASS = onset is clean, no audible pitch slide.

    4. **Test 3 — noteId correlation with multi-note chord.** Play a chord where ONLY the D4 has a tuning offset (e.g. quarter-sharp D4) and the other notes are 12-TET. Verify that ONLY the D4 is detuned — the other notes play at their normal 12-TET frequencies. PASS = exactly one note in the chord is detuned (proves noteId correlation, not pitch arithmetic).

    5. **Test 4 — TuningEngine composition with JI Scala tuning.** Switch O-Lyrica to a JI Scala tuning. Play a quarter-sharp note; verify the resulting frequency is the JI base * 2^(0.5/12) = JI * 1.02930 (multiplicative composition per D-10). PASS = pitch is +50¢ above the JI base, not above the 12-TET base.

    6. **Test 5 — Retrigger safety.** Play the same quarter-sharp note twice in succession (with a clear silence gap between). On the SECOND trigger, Dorico does NOT send a fresh NE event (only the noteId-tagged NoteOn). Verify the second trigger plays at the 12-TET pitch, NOT the still-detuned pitch from the first. PASS = second trigger reverts to 12-TET (proves `exchange(0.0)` consumed the slot).

    Report each test as PASS or FAIL. Resume signal: "approved" if all 5 pass; otherwise describe which tests failed and the observed behavior so Plan 23-05 can be revised.

    **If all 5 PASS:** Plan 23-05 is complete. The D-23-04-A defect is closed and Phase 24 propagation is unblocked.
  </how-to-verify>
  <resume-signal>Type "approved" if all 5 LYR-03 smoke tests pass via VST3, or describe the failures for revision.</resume-signal>
</task>

</tasks>

<verification>
**Plan 23-05 verify gate (D-30 verbatim, plus regression check):**

1. **Clean rebuild** (Task 6 sub-step 6.2): `ninja OLyrica_VST3 OLyrica_AU OLyrica_Standalone` exits 0. Build log is GREP-VERIFIED with TWO patterns:
   - (A) original D-23-04-A symbols: `Undefined symbols.*Steinberg`, `INoteExpressionController::iid`, `Steinberg::UString::assign`, `Steinberg::FUnknown::iid` — zero matches required.
   - (B) **broadened (per plan-checker WARNING):** `Undefined symbols` OR `^[[:space:]]+".+", referenced from:` — zero matches required. Catches ANY undefined-symbol failure regardless of namespace, regression-proofing against the previous plan's mistake of replacing the original Steinberg failure class with a new `Ouaricon::NoteExpression::VST3Extensions::*` failure class.

2. **Fresh install per CLAUDE.md** (Task 6 sub-step 6.3): AU cache cleared (`AudioComponentRegistrar` killed, `~/Library/Caches/AudioUnitCache/` and `~/Library/Caches/com.apple.audiounits.cache` removed); old VST3 + .component bundles `rm -rf`'d; new ones `cp -R`'d to `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/`. Bundle mtime <5 minutes proves freshness — explicit guard against Plan 23-03's stale-Apr-13 silent-pass failure mode.

3. **AU runtime check** (Task 6 sub-step 6.4): `scripts/verify-au-link.sh OLyrica` exits 0. Codes `aumu OLyr OuDv` are auto-extracted by the script. The script is reusable verbatim by every Phase 24 plan (D-31).

4. **Dorico smoke test re-pass** (Task 7): LYR-03 5-test set passes via VST3 — proves the refactor preserved runtime behavior (the public API surface preservation per D-23 was the design intent; this verifies the implementation honored it).

5. **Public API preservation** (frontmatter must_haves, verified throughout Tasks 3-4): all symbols listed in `<interfaces>` exist in their post-refactor form with their pre-refactor signatures. O-Lyrica's `PluginProcessor.{h,cpp}`, `HarpSynthVoice.{h,cpp}`, and `CMakeLists.txt` build UNCHANGED — no consumer-side edits in this plan (verified by grep in Task 6's acceptance and by the build itself succeeding).

6. **JUCE-NE-PATCH marker check intact** (D-34): configure log contains `[note-expression] JUCE-NE-PATCH markers verified` — confirms the `module.cmake` hook contract is preserved.

7. **Two-TU split structural invariants:**
   - `cpp/NoteExpression.cpp` is Steinberg-free: `grep -c "Steinberg::" modules/tuning/note-expression/cpp/NoteExpression.cpp` returns `0`.
   - Static-init dispatch registration is wired: `grep -c "registerNEUpdate" modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` returns at least `1`, and `grep -c "registerNEUpdate" modules/tuning/note-expression/cpp/NoteExpression.cpp` returns at least `1` (definition in SharedCode TU).
   - Custom-deleter pimpl visible in header: `grep -c "std::unique_ptr<Controller, void" modules/tuning/note-expression/cpp/NoteExpression.h` returns at least `1`.
</verification>

<success_criteria>
Plan 23-05 succeeds when ALL of the following are TRUE:

1. `modules/tuning/note-expression/cpp/NoteExpression.h` is Steinberg-free (zero `<pluginterfaces/...>` includes, zero `JucePlugin_Build_VST3` guards, zero direct Steinberg type references — only `juce::VST3ClientExtensions::Vst3RawEvent` and `Steinberg::TUID` via JUCE's own forward declarations). `nec` is declared as `std::unique_ptr<Controller, void(*)(Controller*)>` (custom function-pointer deleter — D-21 amended). `NEUpdateFn` typedef and `registerNEUpdate` declaration are visible to both TUs.

2. `modules/tuning/note-expression/cpp/NoteExpression.cpp` exists and contains `VST3Extensions::VST3Extensions()` (initializes `nec(nullptr, &noopControllerDelete)`), `~VST3Extensions() = default`, `drainAndUpdate()` (drainBlockEvents + dispatch via `g_neUpdate.load(acquire)`), `noopControllerDelete` in an anonymous namespace, the `std::atomic<NEUpdateFn> g_neUpdate` slot in an anonymous namespace, and `registerNEUpdate` definition. ZERO `<pluginterfaces/...>` includes, ZERO `Steinberg::*` references — links cleanly into AU/Standalone/etc.

3. `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` exists and contains the full Controller body, `realControllerDelete`, `VST3Extensions::queryIEditController` body (using the lazy-create swap-deleter idiom: `nec = std::unique_ptr<Controller, void(*)(Controller*)>(new Controller, &realControllerDelete);`), `updatePendingFromEvents` body, and a static-init `DispatchRegistrar` that calls `registerNEUpdate(&updatePendingFromEvents)` at TU load. All `<pluginterfaces/...>` includes are consolidated here.

4. `modules/cmake/OuariconModules.cmake`'s `ouaricon_add_module()` (a) globs SharedCode non-recursively (`file(GLOB ...)`, NOT `GLOB_RECURSE`), and (b) routes `cpp/<format>/` source files to `${TARGET}_<FORMAT>` for the seven recognized format subdirectories (vst3, au, standalone, vst2, aax, lv2, unity), with PRIVATE include directories on the per-format target only. Per-format routing silently no-ops when the per-format target doesn't exist.

5. `scripts/verify-au-link.sh` exists, is executable, has valid bash syntax, parses plugin/root CMake codes, and on macOS invokes `auval -v <type> <subtype> <manuf>`.

6. `ninja OLyrica_VST3 OLyrica_AU OLyrica_Standalone` rebuilds all three targets cleanly with **zero matches** on the broadened undefined-symbol grep (`grep -E 'Undefined symbols|^[[:space:]]+".+", referenced from:' /tmp/plan-23-05-build.log`). The four exact symbols from D-23-04-A do NOT appear in the build log AND no NEW undefined-symbol class appears either.

7. `~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component` are freshly installed (mtime <5 min after install step). Cache clearing performed per CLAUDE.md.

8. `scripts/verify-au-link.sh OLyrica` exits 0 — `auval` accepts the freshly-installed `.component`.

9. `auval -a 2>&1 | grep -i lyrica` returns at least one line — the component is registered.

10. LYR-03 Dorico quarter-sharp smoke test (5 sub-tests) re-passes via VST3 — refactor preserved runtime behavior.

11. Module public API surface preserved verbatim (D-23, D-32): every symbol listed in the `<interfaces>` block exists post-refactor with its original signature. O-Lyrica's consumer call-sites (`plugins/O-Lyrica/CMakeLists.txt:80`, `PluginProcessor.{h,cpp}`, `HarpSynthVoice.{h,cpp}`) build UNCHANGED — no source edits in this plan.

12. `JUCE-NE-PATCH` marker check still active (D-34) — configure log contains `[note-expression] JUCE-NE-PATCH markers verified`.

13. Plans 23-01..04 and their SUMMARYs are NOT modified by this plan (verified by `ls -la .planning/phases/23-extract/23-0[1-4]-*-PLAN.md` showing pre-Plan-23-05 mtimes).
</success_criteria>

<output>
After completion, create `.planning/phases/23-extract/23-05-fix-au-link-steinberg-symbols-SUMMARY.md` per `.claude/get-shit-done/templates/summary.md`. Required sections:

- Outcome (one sentence: D-23-04-A resolved via two-TU split, all three OLyrica targets link cleanly with broadened grep proving zero undefined-symbol failures of any kind, auval accepts the .component, LYR-03 re-passes).
- Tasks completed (table — task name, commit, result).
- Files modified (final list, byte-for-byte from frontmatter — five files including the new SharedCode-bound `cpp/NoteExpression.cpp`).
- Two-TU split evidence (paste the relevant section of `/tmp/plan-23-05-configure.log` showing both `[Ouaricon]   Added note-expression SharedCode sources to OLyrica` AND `[Ouaricon]   Added note-expression/cpp/vst3 sources to OLyrica_VST3`).
- AU rebuild evidence (paste the relevant section of `/tmp/plan-23-05-build.log` showing the AU and Standalone link lines clean; explicitly note the broadened grep returned zero matches).
- auval evidence (paste the `verify-au-link.sh OLyrica` PASS line).
- LYR-03 re-test results (5/5 PASS table — same format as 23-04 SUMMARY).
- Phase 24 unblock confirmation (one sentence — Phase 24 propagation can now begin; 7 target plugins inherit the per-format convention automatically with no consumer-side change).
- Self-Check (PASSED).

Phase 23 status after this plan: ALL 5 success criteria from ROADMAP.md Phase 23 satisfied + AU build defect closed via the structural two-TU split. Phase 24 unblocked.
</output>
</content>
</invoke>