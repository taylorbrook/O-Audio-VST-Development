---
status: issues_found
phase: 23-extract
reviewer: gsd-code-reviewer
reviewed: 2026-04-25T20:38:54Z
depth: standard
files_reviewed: 11
files_reviewed_list:
  - modules/tuning/note-expression/cpp/NoteExpression.h
  - modules/tuning/note-expression/cpp/NoteExpression.cpp
  - modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp
  - modules/cmake/OuariconModules.cmake
  - scripts/verify-au-link.sh
  - plugins/O-Lyrica/CMakeLists.txt
  - plugins/O-Lyrica/Source/PluginProcessor.h
  - plugins/O-Lyrica/Source/PluginProcessor.cpp (drainAndUpdate call site only)
  - plugins/O-Lyrica/Source/HarpSynthVoice.h
  - plugins/O-Lyrica/Source/HarpSynthVoice.cpp (startNote NE wiring only)
  - modules/tuning/note-expression/module.cmake
critical: 0
high: 1
medium: 0
low: 1
---

# Phase 23: Code Review Report

**Reviewed:** 2026-04-25T20:38:54Z
**Depth:** standard
**Files Reviewed:** 11
**Status:** issues_found

## Summary

Reviewed the note-expression module extraction (Plans 23-01 through 23-05) with focus on
the two-TU split introduced in 23-05: custom-deleter pimpl, dual dispatch slots, and per-format
CMake source routing. The C++ memory-safety properties are sound — static-init order is correct
(constant-initialized atomics, no SIOF risk), the dispatch slot release/acquire pairing is
correct, the refcount design is intentionally non-COM (no double-delete risk), and the
`Steinberg::TUID` reference in the public header resolves cleanly through JUCE's own forward
declaration rather than pluginterfaces headers.

One latent correctness bug was found in `OuariconModules.cmake`: the per-format target lookup
uses `string(TOUPPER)` on lowercase format names, producing target names like `OLyrica_STANDALONE`
that JUCE never generates. JUCE names mixed-case format targets (`OLyrica_Standalone`,
`OLyrica_AAX`, `OLyrica_LV2`, `OLyrica_Unity`), so per-format source routing silently no-ops
for every format except VST3 and AU today — and will silently fail to route any future
`cpp/standalone/` sources to the Standalone target. One low-severity code quality note is also
included.

---

## High Issues

### HR-01: Per-format CMake routing silently fails for Standalone, AAX, LV2, Unity, AUv3

**File:** `modules/cmake/OuariconModules.cmake:77-87`

**Issue:** The format loop converts format names to uppercase with `string(TOUPPER ${fmt} _FMT_UPPER)`
and then checks `TARGET "${TARGET_NAME}_${_FMT_UPPER}"`. JUCE generates mixed-case target names
(`OLyrica_Standalone`, `OLyrica_AAX`, `OLyrica_LV2`, `OLyrica_Unity`) — not all-uppercase. The
`TARGET` guard therefore never matches for those four formats, and the `target_sources(...)` call
silently does not fire. VST3 (`VST3`) and AU (`AU`) happen to work because JUCE also uses uppercase
for them. Confirmed against `JUCE/extras/Build/CMake/JUCEUtils.cmake:1186,1605`:

```
if(kind STREQUAL "Standalone")  → target is ${name}_Standalone
TARGET ${target}_Standalone     → line 1605
```

Currently `cpp/vst3/` is the only per-format subdirectory that exists, and `VST3` resolves
correctly, so there is no functional impact today. The bug is latent: any future module that adds
`cpp/standalone/` or `cpp/aax/` sources will see a silent build-time no-op rather than an error,
making it hard to diagnose.

**Fix:** Use the format name directly (preserving JUCE's casing) rather than uppercasing it.
Either maintain a lookup map from lowercase key to JUCE's mixed-case target suffix, or require
callers to pass format names in JUCE's casing:

```cmake
# Option A: direct mapping (preferred — explicit about JUCE's exact target suffixes)
set(_OUA_JUCE_FORMATS VST3 AU Standalone VST2 AAX LV2 AUv3 Unity)
foreach(fmt ${_OUA_JUCE_FORMATS})
    # fmt is already the JUCE target suffix — no TOUPPER needed
    set(_FMT_DIR "${MODULE_DIR}/cpp/${fmt}")          # keep a lowercase alias dir if desired
    # ...or use lowercase dir name and uppercase fmt only for target lookup:
endforeach()

# Option B: lowercase dir → uppercase/mixed target suffix lookup table
set(_OUA_FMT_DIRS    vst3  au  standalone  vst2  aax  lv2  auv3  unity)
set(_OUA_FMT_TARGETS VST3  AU  Standalone  VST2  AAX  LV2  AUv3  Unity)
foreach(dir target IN ZIP_LISTS _OUA_FMT_DIRS _OUA_FMT_TARGETS)
    set(_FMT_DIR "${MODULE_DIR}/cpp/${dir}")
    if(EXISTS "${_FMT_DIR}" AND TARGET "${TARGET_NAME}_${target}")
        # ... add sources to ${TARGET_NAME}_${target}
    endif()
endforeach()
```

---

## Low Issues

### LR-01: drainBlockEvents — redundant clear after swap

**File:** `modules/tuning/note-expression/cpp/NoteExpression.h:178-184`

**Issue:** After `out.swap(blockEvents)`, `blockEvents` already holds `out`'s previous contents
(which were cleared by the preceding `out.clear()`). The subsequent `blockEvents.clear()` on line
182 is therefore a no-op that adds no correctness guarantee.

```cpp
void drainBlockEvents (std::vector<Vst3RawEvent>& out)
{
    out.clear();
    out.swap (blockEvents);  // blockEvents is now empty (held out's cleared state)
    blockEvents.clear();     // redundant — already empty
    blockEvents.reserve (64);
}
```

**Fix:** Remove the redundant clear:

```cpp
void drainBlockEvents (std::vector<Vst3RawEvent>& out)
{
    out.clear();
    out.swap (blockEvents);
    blockEvents.reserve (64);
}
```

---

## Items Verified Clean

The following concerns raised in the scope notes were reviewed and found sound:

- **Static-init order (AU/Standalone):** `g_neUpdate` and `g_neQuery` are both constant-initialized
  (`std::atomic<Fn>{nullptr}` at namespace scope with a constexpr-compatible initializer). They are
  guaranteed initialized to `nullptr` before any dynamic initialization runs. `DispatchRegistrar`'s
  ctor is dynamic initialization; it always fires after the slots exist. In AU/Standalone builds
  where the VST3 TU is not linked, the registrar never runs and the slots stay `nullptr` for the
  lifetime of the process. No static-initialization-order-fiasco risk.

- **Steinberg::TUID in Steinberg-free header:** `Steinberg::TUID` on lines 131 and 165 of
  `NoteExpression.h` resolves through JUCE's own forward declaration in
  `juce_VST3ClientExtensions.h` (`using TUID = char[16]`), which is included transitively via
  `JuceHeader.h`. No pluginterfaces header is required. The Steinberg-free invariant holds.

- **Refcount / double-delete:** `Controller::release()` never deletes the object. The comment
  at line 159 documents this explicitly ("NEC is owned by the extensions object so the refcount
  is effectively ignored"). `addRef()` in `vst3QueryIEditController` bumps the count from 1 to 2;
  the host's single matching `release()` brings it to 1. The object is destroyed only by
  `realControllerDelete` via the `unique_ptr` deleter when `VST3Extensions` is destroyed. No
  double-delete path exists.

- **Audio-thread safety of `nec`:** `nec` is only accessed in `queryIEditController`, called from
  the host's initialization/UI thread. The audio thread never touches `nec` — it only calls
  `drainAndUpdate()` which accesses `rawEventScratch` and `pendingTable`. No concurrent access.

---

_Reviewed: 2026-04-25T20:38:54Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
