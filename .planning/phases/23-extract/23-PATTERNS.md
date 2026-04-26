# Phase 23: Extract — Pattern Map

**Mapped:** 2026-04-24
**Files analyzed:** 17 (11 CREATE, 8 MODIFY, 1 DELETE — some overlap)
**Analogs found:** 14 / 17 (3 are new conventions per D-12/D-13/D-15)

## File Classification

| File | Action | Role | Data Flow | Closest Analog | Match Quality |
|------|--------|------|-----------|----------------|---------------|
| `modules/tuning/note-expression/module.yaml` | CREATE | config / module metadata | n/a (static) | `modules/tuning/scala-tuning-engine/module.yaml` | exact (same category, same shape) |
| `modules/tuning/note-expression/README.md` | CREATE | docs | n/a | `modules/tuning/scala-tuning-engine/README.md` | role-match (instrument vs NE) |
| `modules/tuning/note-expression/cpp/NoteExpression.h` | CREATE | header-only module (Controller + helpers + Extensions) | event-driven (VST3 events) | `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` (spike code — stripped) | exact (this IS the extraction target) |
| `modules/tuning/note-expression/cpp/NoteExpression.cpp` | CREATE *(optional — only if not header-only)* | glue | request-response | `modules/core/licensing/cpp/OuariconLicense.cpp` | role-match |
| `modules/tuning/note-expression/module.cmake` *(optional, new convention)* | CREATE | config / marker-check hook | n/a | — | **new convention (D-15)** |
| `scripts/juce-patches/note-expression-juce-8.0.4.patch` | CREATE | patch data | n/a (static) | — | **new convention (D-12, D-13)** |
| `scripts/apply-juce-patches.sh` | CREATE | idempotent shell script | batch / CLI | `scripts/verify-backup.sh` | role-match (bash + colored output + early exit) |
| `modules/registry.yaml` | MODIFY | config / registry entry | n/a | existing `scala-tuning-engine` entry (same file, lines 195–249) | exact (append new entry in same `tuning` section) |
| `modules/cmake/OuariconModules.cmake` | MODIFY *(potentially)* | CMake function | build-time | itself | self-match — extend only if module.cmake hook needed for D-15 |
| `plugins/O-Lyrica/CMakeLists.txt` | MODIFY | build config | build-time | Many plugins using `ouaricon_add_module()` (see Shared Patterns §1) | exact |
| `plugins/O-Lyrica/Source/PluginProcessor.h` | MODIFY | controller header | request-response | self (remove spike members) | self-match |
| `plugins/O-Lyrica/Source/PluginProcessor.cpp` | MODIFY | controller impl | event-driven | `.claude/skills/.../sources/shared-code/processor-drain.cpp` | exact (excerpt IS the pre-extraction code) |
| `plugins/O-Lyrica/Source/HarpSynthVoice.h` | MODIFY | voice header | request-response | self (swap pointer type) | self-match |
| `plugins/O-Lyrica/Source/HarpSynthVoice.cpp` | MODIFY | voice impl | request-response / streaming | `.claude/skills/.../sources/shared-code/voice-startNote.cpp` | exact |
| `plugins/O-Lyrica/Source/DSP/TuningEngine.{h,cpp}` | READ-ONLY | DSP service | request-response | n/a (read only to confirm `getFrequency(midi)` signature) | exact confirmed: `double getFrequency(int midiNote, int midiChannel = 0)` @ line 260 of TuningEngine.h |
| `plugins/O-Lyrica/CHANGELOG.md` | MODIFY | docs | n/a | existing `[2.2.2]` / `[2.2.1]` entries (same file) | exact |
| `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` | DELETE | — | — | n/a | n/a (per D-16) |

---

## Pattern Assignments

### `modules/tuning/note-expression/module.yaml` (CREATE)

**Analog:** `modules/tuning/scala-tuning-engine/module.yaml`

**Shape** (lines 1–27 of analog — front matter, category, provides):
```yaml
# note-expression module v1.0.0
# VST3 Note Expression (kTuningTypeID) for Dorico microtonal playback.
# Surfaces NE events via a local JUCE patch and exposes a header-only
# Controller + Extensions + voice helper for per-note semitone offsets.

name: note-expression
version: 1.0.0
description: |
  VST3 Note Expression (kTuningTypeID) support for Dorico-driven microtonal
  playback. Owns the pending-tuning table, advertises the NEC, drains the
  patched JUCE wrapper's raw-event queue, and provides a header-only voice
  helper (applyPendingTuning) that composes an NE semitone delta with any
  existing base frequency (e.g. a TuningEngine lookup).

  Requires a local JUCE patch (see scripts/juce-patches/) that adds
  VST3ClientExtensions::onVst3RawEvent so kNoteExpressionValueEvent and
  noteId-tagged NoteOn/NoteOff reach the plugin before MidiEventList::toMidiBuffer
  drops them.

category: tuning
author: Ouaricon Audio
```

**`provides:` block** (mirror analog lines 30–71 structure; strip js/native-functions since NE has no UI):
```yaml
provides:
  cpp-classes:
    - Ouaricon::NoteExpression::Controller
    - Ouaricon::NoteExpression::VST3Extensions
  cpp-free-functions:
    - Ouaricon::NoteExpression::applyPendingTuning
    - Ouaricon::NoteExpression::updatePendingFromEvents
  cpp-types:
    - Ouaricon::NoteExpression::PendingTuningTable
```

**`dependencies:` / `requirements:` / `sources:` blocks** (mirror analog lines 120–143):
```yaml
dependencies: []   # D-11: no dep on scala-tuning-engine

requirements:
  juce_modules:
    - juce_audio_processors   # for VST3ClientExtensions
    - juce_core
  cpp_standard: 20
  juce_patch:
    marker: "JUCE-NE-PATCH"
    file: "scripts/juce-patches/note-expression-juce-8.0.4.patch"
    juce_version: "8.0.4"

sources:
  cpp:
    - cpp/NoteExpression.h
```

**`used_by:` + `changelog:` blocks** (analog lines 205–240):
```yaml
used_by:
  - plugin: OLyrica
    version: 2.3.0

changelog:
  - version: 1.0.0
    date: 2026-04-24
    changes:
      - "Initial extraction from O-Lyrica spike 001/002/003"
      - "Header-only public API under Ouaricon::NoteExpression"
      - "Owns PendingTuningTable (128-slot atomic<double> array)"
      - "Provides VST3Extensions + Controller + voice helper"
      - "CMake-time JUCE-NE-PATCH marker verification"
```

**Rationale:**
- D-01 category = `tuning` → direct mirror.
- D-03 version = `1.0.0`, D-04 namespace `Ouaricon::NoteExpression` → reflected in `provides:`.
- D-11 no dependency on scala-tuning-engine → `dependencies: []`.
- `juce_patch:` sub-block under `requirements:` is a new convention (documented here; the actual enforcement is CMake-time per D-15).

---

### `modules/tuning/note-expression/cpp/NoteExpression.h` (CREATE — header-only)

**Analog:** `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` (spike) — this file IS being extracted, stripped, and renamespaced.

**Source of truth:** `.claude/skills/spike-findings-VST-development/sources/shared-code/NoteExpressionSupport.spike.h`.

**What to copy verbatim** (analog lines 19–22, 58–167 of spike): the `Controller` class body (renamed from `TuningNoteExpressionController`), the `VST3Extensions` class body (renamed from `LyricaVST3Extensions`).

**What to STRIP before commit** (D-18, landmine 5):
- Lines 35–57 of spike (the entire `namespace detail { neTrace, iidToHex }` block)
- `#include <fstream>` (line 28 of spike)
- `#include <mutex>` (line 29 of spike — only needed for `neTrace`)
- Every `detail::neTrace(...)` call site (spike lines 72, 82, 186, 207)
- Every `detail::iidToHex(...)` call site (spike line 187)

**What to add (D-05, D-06, D-07, D-08, D-09) — new code not in spike:**

```cpp
// ===== Public aliases per D-06 =====
namespace Ouaricon::NoteExpression
{
    using PendingTuningTable = std::array<std::atomic<double>, 128>;

    // ===== Voice-side helper (D-07, MOD-04 header-only) =====
    /** Consumes the pending NE tuning delta for `midiNoteNumber` and returns
        the new frequency. `exchange(0.0)` ensures a retriggered note at the
        same pitch in a later block does not inherit a stale offset.
        Encapsulates std::pow so voice code never calls pow directly.
        Composes multiplicatively with the caller's base frequency — caller
        should pass the frequency AFTER any TuningEngine / humanize lookup (D-10).
    */
    inline double applyPendingTuning (PendingTuningTable& table,
                                      int               midiNoteNumber,
                                      double            currentFrequency)
    {
        if (midiNoteNumber < 0 || midiNoteNumber >= 128)
            return currentFrequency;

        const double semis = table[(size_t) midiNoteNumber]
                                  .exchange (0.0, std::memory_order_acq_rel);
        if (semis == 0.0)
            return currentFrequency;

        return currentFrequency * std::pow (2.0, semis / 12.0);
    }

    // ===== Processor-side drain+correlate (D-08) =====
    /** Two-pass correlation: (1) map noteId -> midi pitch from NoteOns in the
        block; (2) for each kTuningTypeID NE, compute 240*(value-0.5) semitones
        and store into table[pitch]. Caller must call
        VST3Extensions::drainBlockEvents(events) immediately before this.
    */
    inline void updatePendingFromEvents (
        const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>& events,
        PendingTuningTable& table)
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

            // VST3 kTuningTypeID: norm [0,1] → plain semitones [-120, +120].
            const double semitones = 240.0 * (e.value - 0.5);
            table[(size_t) pitch].store (semitones, std::memory_order_release);
        }
    }

} // namespace Ouaricon::NoteExpression
```

**VST3Extensions additions per D-09:** the `VST3Extensions` class gains ownership of `PendingTuningTable` plus a scratch buffer for raw events. Public accessors:
```cpp
class VST3Extensions : public juce::VST3ClientExtensions
{
public:
    // ... existing spike methods (queryIEditController, onVst3RawEvent) ...

    /** Called by the plugin at the top of processBlock, BEFORE renderNextBlock. */
    void drainAndUpdate()
    {
        drainBlockEvents (rawEventScratch);
        updatePendingFromEvents (rawEventScratch, pendingTable);
    }

    /** Voice wiring: hand this to each voice's setPendingTuningSource(). */
    PendingTuningTable& getPendingTable() noexcept { return pendingTable; }

private:
    Controller nec;
    std::vector<Vst3RawEvent> rawEventScratch;
    PendingTuningTable pendingTable {};   // D-09: owned here, not on plugin
};
```

**Rationale:** Per D-18, zero `neTrace` sites remain. Per D-09, table ownership moves from processor to extensions. Per D-10, voice calls `applyPendingTuning` AFTER `TuningEngine::getFrequency()`.

---

### `modules/tuning/note-expression/README.md` (CREATE)

**Analog:** `modules/tuning/scala-tuning-engine/README.md`

**Structure to mirror** (lines 1–16 of analog — header + Quick Start):
```markdown
# note-expression v1.0.0

VST3 Note Expression (kTuningTypeID) support for Dorico microtonal playback.

## Quick Start

1. Apply the JUCE patch (one time, per JUCE upgrade):
   ```bash
   ./scripts/apply-juce-patches.sh
   ```
2. Add the module to your plugin's CMakeLists.txt:
   ```cmake
   include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
   ouaricon_add_module(YourPlugin note-expression)
   ```
3. See `Integration` below for processor + voice wiring.
```

**Sections to include** (mirror analog's section hierarchy — Features, Installation, Usage in Audio Processing, State Persistence, Integration Approach):
- `## Features` (4 bullets: NEC advertisement, raw-event drain, correlate, voice helper).
- `## Installation` — 3 steps (JUCE patch apply, CMake include, processor wiring) — **analog lines 56–165** are the template.
- `## Usage in Audio Processing` — mirror analog lines 166–181 structure but with NE patterns instead of tuning.
- `## Dorico End-User Setup` — NEW section, no analog. Pull from landmine 3 of `vst3-note-expression-dorico.md` (4-step expression-map instructions).
- `## JUCE Patch Management` — NEW section, no analog. Document the marker convention and pointer to `scripts/apply-juce-patches.sh`.
- `## Integration Approach` — mirror analog lines 209–219 but say "header-only C++ with no UI" instead of "standalone ES6 class approach".

**Depth guidance (discretion):** analog README is 231 lines. Per CONTEXT.md deferred section, Phase 23 README is "functional" and Phase 25 produces the comprehensive `research/microtonal-dorico-integration.md`. Aim ~150 lines.

---

### `scripts/juce-patches/note-expression-juce-8.0.4.patch` (CREATE — new convention)

**Analog:** none. New convention per D-12/D-13.

**Source of truth for content:** `.claude/skills/spike-findings-VST-development/sources/shared-code/juce-patch.md` lines 10–44 (patch 1/2) and lines 46–113 (patch 2/2).

**Target format:** unified diff compatible with `git apply` / `patch -p1`. Generation procedure (per D-12):
```bash
# 1. Stash current JUCE state (patch already applied)
cd /Users/taylorbrook/JUCE
# 2. Make a temp clone of pristine JUCE 8.0.4
git clone --depth=1 --branch=8.0.4 https://github.com/juce-framework/JUCE /tmp/juce-pristine
# 3. Diff the two trees, scoped to the two patched files
diff -u /tmp/juce-pristine/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h \
       /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h \
    > note-expression-juce-8.0.4.patch
diff -u /tmp/juce-pristine/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp \
       /Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp \
    >> note-expression-juce-8.0.4.patch
```

**Required content invariants:**
- Both hunks MUST retain the `// JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)` comment marker (D-15 verification greps for this string).
- Exact content = the `cpp` block in `juce-patch.md` lines 13–44 (hunk 1) and lines 60–112 (hunk 2).
- Filename MUST encode target version (`-juce-8.0.4.patch`) per D-13.

**New convention marker:** first `.patch` file in the repo; sets the precedent for future JUCE vendor patches.

---

### `scripts/apply-juce-patches.sh` (CREATE)

**Analog:** `scripts/verify-backup.sh` — same bash style (color codes, early exit, `set -e`).

**Style excerpt to copy** (analog lines 1–20):
```bash
#!/bin/bash
set -e

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color
```

**Idempotency pattern (D-14):**
```bash
JUCE_DIR="${JUCE_DIR:-/Users/taylorbrook/JUCE}"
PATCH_DIR="$(dirname "$0")/juce-patches"
PATCH_FILE="$PATCH_DIR/note-expression-juce-8.0.4.patch"
MARKER="JUCE-NE-PATCH"

# 1. Preflight: JUCE path must exist
if [[ ! -d "$JUCE_DIR" ]]; then
  echo -e "${RED}[apply-juce-patches] JUCE tree not found at ${JUCE_DIR}${NC}"
  echo -e "Set JUCE_DIR env var or install JUCE at /Users/taylorbrook/JUCE (see CLAUDE.md)."
  exit 1
fi

# 2. Idempotency: skip if marker already present
MARKER_HITS=$(grep -rln "$MARKER" "$JUCE_DIR/modules/juce_audio_processors/utilities/" \
                                    "$JUCE_DIR/modules/juce_audio_plugin_client/" 2>/dev/null | wc -l | tr -d ' ')
if [[ "$MARKER_HITS" -ge 2 ]]; then
  echo -e "${GREEN}[apply-juce-patches] Marker ${MARKER} already present in JUCE tree — skipping.${NC}"
  exit 0
fi

# 3. Apply
echo -e "${YELLOW}[apply-juce-patches] Applying ${PATCH_FILE}...${NC}"
( cd "$JUCE_DIR" && patch -p1 < "$PATCH_FILE" )
echo -e "${GREEN}[apply-juce-patches] Patch applied. Verify with:${NC}"
echo -e "  grep -rn \"$MARKER\" $JUCE_DIR/modules/juce_audio_processors/utilities/ $JUCE_DIR/modules/juce_audio_plugin_client/"
```

**Error-message wording (discretion):** name the script path + the recovery action. Example above satisfies this.

---

### CMake-time marker check (D-15 — new convention)

**Analog:** none. No existing `file(READ)` / `execute_process` pattern scans an external tree in any module. This is entirely new.

**Placement decision (recommended):** create a new file `modules/tuning/note-expression/module.cmake`, auto-`include()`'d from `ouaricon_add_module()` when present. This is a minimal extension of `OuariconModules.cmake` (one new block at the end of the function body):

**Patch to `modules/cmake/OuariconModules.cmake`** (insert after the JS-copy block, before the `ARG_CONFIG` block, around current line 81):
```cmake
    # Module-supplied CMake hook (optional)
    if(EXISTS "${MODULE_DIR}/module.cmake")
        include("${MODULE_DIR}/module.cmake")
    endif()
```

**Content of `modules/tuning/note-expression/module.cmake`:**
```cmake
# ==============================================================================
# note-expression module CMake hook
# Verifies the local JUCE fork has the JUCE-NE-PATCH markers applied.
# Fails loud + fails fast at configure time (D-15).
# ==============================================================================

# Locate JUCE tree the same way the root CMakeLists.txt does
if(DEFINED ENV{JUCE_DIR})
    set(_NE_JUCE_ROOT "$ENV{JUCE_DIR}")
elseif(WIN32)
    set(_NE_JUCE_ROOT "C:/JUCE")
else()
    set(_NE_JUCE_ROOT "/Users/taylorbrook/JUCE")
endif()

set(_NE_MARKER "JUCE-NE-PATCH")
set(_NE_FILE1 "${_NE_JUCE_ROOT}/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h")
set(_NE_FILE2 "${_NE_JUCE_ROOT}/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp")

foreach(_ne_f ${_NE_FILE1} ${_NE_FILE2})
    if(NOT EXISTS "${_ne_f}")
        message(FATAL_ERROR
            "[note-expression] Expected JUCE source not found: ${_ne_f}\n"
            "Ensure JUCE 8.0.4 is installed and run scripts/apply-juce-patches.sh")
    endif()
    file(READ "${_ne_f}" _ne_contents)
    string(FIND "${_ne_contents}" "${_NE_MARKER}" _ne_idx)
    if(_ne_idx EQUAL -1)
        message(FATAL_ERROR
            "[note-expression] JUCE patch marker '${_NE_MARKER}' not found in:\n"
            "  ${_ne_f}\n"
            "Run: ./scripts/apply-juce-patches.sh")
    endif()
endforeach()

message(STATUS "[note-expression] JUCE-NE-PATCH markers verified in ${_NE_JUCE_ROOT}")
```

**Rationale:** D-15 says "Enforcement lives at the module level... only gates plugins that actually consume note-expression." Putting the check in `module.cmake` + auto-including it from `ouaricon_add_module()` achieves this cleanly. The `file(READ)` + `string(FIND)` path is chosen over `execute_process(grep)` for cross-platform (Windows lacks grep).

---

### `modules/registry.yaml` (MODIFY — add new entry)

**Analog:** the existing `scala-tuning-engine` entry in the same file (lines 195–249).

**Insert location:** append inside the `# TUNING MODULES` section (after line 249, before the `# EFFECTS MODULES` divider at line 251).

**Shape to paste** (mirror structure of analog lines 195–249; shrink to NE's simpler provides):
```yaml
  - name: note-expression
    path: tuning/note-expression
    version: 1.0.0
    description: |
      VST3 Note Expression (kTuningTypeID) support for Dorico microtonal playback.
      Header-only C++ module under Ouaricon::NoteExpression — provides the NEC,
      the VST3Extensions subclass that owns the 128-slot pending-tuning table,
      a drain+correlate helper, and a voice-side applyPendingTuning helper.
      Requires a local JUCE patch (scripts/juce-patches/note-expression-juce-8.0.4.patch).
    category: tuning
    provides:
      - cpp-class: Ouaricon::NoteExpression::Controller
      - cpp-class: Ouaricon::NoteExpression::VST3Extensions
      - cpp-type: Ouaricon::NoteExpression::PendingTuningTable
      - cpp-free-function: Ouaricon::NoteExpression::applyPendingTuning
      - cpp-free-function: Ouaricon::NoteExpression::updatePendingFromEvents
    dependencies: []
    tags: [vst3, note-expression, dorico, microtuning, per-note-pitch, header-only]
    reuse_score: 10
    used_by:
      - plugin: OLyrica
        version: 2.3.0
```

---

### `modules/cmake/OuariconModules.cmake` (MODIFY — one optional insertion)

**Analog:** itself.

**Change required** (only if D-15 is implemented via `module.cmake` hook — see CMake-time marker check section above):

Insert between current line 80 (end of JS copy block) and line 83 (start of `ARG_CONFIG` block):
```cmake
    # Module-supplied CMake hook (optional)
    if(EXISTS "${MODULE_DIR}/module.cmake")
        message(STATUS "[Ouaricon]   Including ${MODULE_NAME}/module.cmake")
        include("${MODULE_DIR}/module.cmake")
    endif()
```

**Why minimal:** per CONTEXT.md code-context ("the new module plugs into this without extending the function"), avoid larger refactors. The `module.cmake` hook is backward-compatible — no existing module has one, so no behavior changes for existing plugins.

---

### `plugins/O-Lyrica/CMakeLists.txt` (MODIFY — 3 changes)

**Analog:** 22 existing `ouaricon_add_module(...)` call sites; most-pitched analog is **`plugins/O-AnalogEQ/CMakeLists.txt` lines 56 and 60** (demonstrates unconditional + conditional add in same file).

**Excerpt from `plugins/O-AnalogEQ/CMakeLists.txt` lines 55–61** (most analogous: two `ouaricon_add_module()` calls, one unconditional, one licensing-gated):
```cmake
# Preset manager module (always included)
ouaricon_add_module(OuariconAnalogEQ preset-manager)

# Licensing module (compile-flag gated, OFF for local dev)
if(OUARICON_LICENSING)
    ouaricon_add_module(OuariconAnalogEQ licensing)
    target_compile_definitions(OuariconAnalogEQ PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries(OuariconAnalogEQ PRIVATE juce::juce_cryptography)
endif()
```

**Exact syntax to add** (after line 75 `juce_generate_juce_header(OLyrica)`, before the licensing block at line 77):
```cmake
# Phase 23: VST3 Note Expression microtonal support (Dorico)
# Header-only module; triggers CMake-time JUCE-NE-PATCH marker check.
ouaricon_add_module(OLyrica note-expression)
```

**Target-name caveat:** O-Lyrica's JUCE target name is `OLyrica` (line 6 of its CMakeLists). All 22 analogs confirm the exact target-name convention varies per plugin (e.g., `O-Bells`, `OuariconAnalogEQ`, `OLyrica`) — copy the target name from the same file's `juce_add_plugin(TARGETNAME ...)` call.

**Additional removal:** line 49 already adds preset-manager via path (`../../modules/persistence/preset-manager/cpp`) but NOT via `ouaricon_add_module()` — out of scope for this phase; do not change.

---

### `plugins/O-Lyrica/Source/PluginProcessor.h` (MODIFY)

**Analog:** self (surgical edits to existing file — remove spike members, add module-owned extensions member).

**Lines to DELETE** (per CONTEXT.md refactor-targets):
- Line 22: `#include "VST3/NoteExpressionSupport.h"` → replace with `#include "NoteExpression.h"` (module provides header via `ouaricon_add_module()`'s `target_include_directories`).
- Lines 116–128 (the `getVST3ClientExtensions` + `getPendingTuningSource` public block):
  - Keep `getVST3ClientExtensions()` but change its body to `return &vst3Extensions;` where `vst3Extensions` is the new module type.
  - Delete `getPendingTuningSource()` entirely (D-09: table ownership moves into module).
- Lines 208–211 (private spike members):
  - Replace `OLyrica::LyricaVST3Extensions vst3Extensions;` with `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;`
  - Delete `std::array<std::atomic<double>, 128> pendingTuningSemis {};`
  - Delete `std::vector<juce::VST3ClientExtensions::Vst3RawEvent> rawEventScratch;`

**Excerpt to replace (current lines 115–128):**
```cpp
    //==============================================================================
    // Spike 001 (2026-04-22): VST3 Note Expression (kTuningTypeID) — Dorico microtonal
    // playback path. Requires local JUCE patch (VST3ClientExtensions::onVst3RawEvent).
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    /** Per-voice pending tuning offsets in semitones, keyed by MIDI pitch (0–127).
        ...
    */
    std::array<std::atomic<double>, 128>* getPendingTuningSource()
    {
        return &pendingTuningSemis;
    }
```
**Becomes:**
```cpp
    //==============================================================================
    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
    // Backed by note-expression module v1.0.0 (modules/tuning/note-expression).
    // Requires local JUCE patch (see scripts/apply-juce-patches.sh).
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
```

**Excerpt to replace (current lines 208–211):**
```cpp
    // Spike 001: VST3 Note Expression support
    OLyrica::LyricaVST3Extensions vst3Extensions;
    std::array<std::atomic<double>, 128> pendingTuningSemis {};
    std::vector<juce::VST3ClientExtensions::Vst3RawEvent> rawEventScratch;
```
**Becomes:**
```cpp
    // VST3 Note Expression support (module-owned table + raw-event scratch)
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
```

---

### `plugins/O-Lyrica/Source/PluginProcessor.cpp` (MODIFY — two touchpoints)

**Analog:** `.claude/skills/spike-findings-VST-development/sources/shared-code/processor-drain.cpp` (this is the pre-extraction shape that becomes the helper call).

**Touchpoint 1 — voice wiring (line 506):**

Current code:
```cpp
voice->setPendingTuningSource(&pendingTuningSemis); // Spike 001: VST3 NE tuning
```
**Becomes:**
```cpp
voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // D-09: module-owned table
```

**Touchpoint 2 — drain + correlate block (lines 705–763):**

The entire block at lines 705–763 (58 lines of spike code with `neTrace` calls) collapses to:
```cpp
    // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
    // correlate tuning deltas to their NoteOn's MIDI pitch. Pending table
    // is consumed atomically by each voice in startNote().
    vst3Extensions.drainAndUpdate();
```

**Verification (D-18):** after this edit, grep the whole file:
```bash
grep -c "neTrace\|detail::\|NoteExpressionSupport" plugins/O-Lyrica/Source/PluginProcessor.cpp
# expected: 0
```

---

### `plugins/O-Lyrica/Source/HarpSynthVoice.h` (MODIFY — one line)

**Analog:** self.

**Line 128 current:**
```cpp
std::array<std::atomic<double>, 128>* pendingTuningSource = nullptr;
```
**Becomes:**
```cpp
Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
```

The setter signature in `.cpp` line 85 (`setPendingTuningSource`) must match — update both.

---

### `plugins/O-Lyrica/Source/HarpSynthVoice.cpp` (MODIFY — three touchpoints)

**Analog:** `.claude/skills/spike-findings-VST-development/sources/shared-code/voice-startNote.cpp` — the target shape after extraction.

**Touchpoint 1 — remove include (line 13):**
```cpp
#include "VST3/NoteExpressionSupport.h"  // Spike 002: for detail::neTrace
```
**Delete entirely** (NoteExpression helper is pulled in transitively via PluginProcessor.h's new `#include "NoteExpression.h"` — voice only needs the type). If voice doesn't already see it via a header chain, add:
```cpp
#include "NoteExpression.h"   // from modules/tuning/note-expression
```

**Touchpoint 2 — setter signature (line 85–88):**
```cpp
void HarpSynthVoice::setPendingTuningSource(std::array<std::atomic<double>, 128>* source)
{
    pendingTuningSource = source;
}
```
**Becomes:**
```cpp
void HarpSynthVoice::setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source)
{
    pendingTuningSource = source;
}
```

**Touchpoint 3 — startNote NE block (lines 139–157):**

Current (19 lines, includes `neTrace` diagnostic calls):
```cpp
    // Spike 001 (2026-04-22): Apply VST3 Note Expression tuning delta (Dorico
    // microtonal playback). Consumed atomically so retriggered notes at the same
    // pitch in a later block don't inherit a stale offset.
    if (pendingTuningSource != nullptr && midiNoteNumber >= 0 && midiNoteNumber < 128)
    {
        double semis = (*pendingTuningSource)[(size_t) midiNoteNumber]
                          .exchange (0.0, std::memory_order_acq_rel);
        OLyrica::detail::neTrace (juce::String::formatted (...));  // DELETE
        if (semis != 0.0)
            currentFrequency *= std::pow (2.0, semis / 12.0);
    }
    else
    {
        OLyrica::detail::neTrace (juce::String::formatted (...));  // DELETE entire else branch
    }
```

**Becomes (D-07, D-10 composition order — TuningEngine frequency first, then NE delta):**
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

**Verification (D-18):** after edit:
```bash
grep -c "neTrace\|OLyrica::detail\|std::pow.*12\.0" plugins/O-Lyrica/Source/HarpSynthVoice.cpp
# expected: 0
```

---

### `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` (DELETE)

Per D-16: delete the file entirely. Also remove the parent `Source/VST3/` directory if it becomes empty.

**Verification:**
```bash
test ! -e plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h && echo "OK: spike header removed"
grep -rn "NoteExpressionSupport\.h\|VST3/NoteExpressionSupport" plugins/O-Lyrica/
# expected: 0 matches
```

---

### `plugins/O-Lyrica/CHANGELOG.md` (MODIFY)

**Analog:** existing `[2.2.2]` entry (lines 5–23 of the same file) — headings, sections, technical-note style.

**Shape to copy** (analog lines 5–23 structure):
```markdown
## [2.3.0] - 2026-04-24

### Added

- **VST3 Note Expression microtonal support for Dorico.** O-Lyrica now responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events), enabling correct microtonal playback of quarter-tones, third-tones, and arbitrary tuning deltas authored in Dorico's tonality system. Required procedure for end users: `Dorico → Library → Expression Maps…` → duplicate an existing map → set **Microtonality** to **"VST3 Note Expression"** → assign the new map to O-Lyrica's Endpoint Setup channel. Default expression maps route microtones to VST2 detune or pitch bend — neither reaches VST3 plugins, so without this setup microtonal playback falls back to 12-TET.
- **Shared `note-expression` module adoption.** O-Lyrica is the reference consumer for the new Ouaricon module at `modules/tuning/note-expression` (v1.0.0). The 128-slot pending-tuning table, NEC advertisement, raw-event drain, and voice-side pitch-offset helper now live in the module rather than O-Lyrica's own sources. Phase 24 propagates the same module to O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant, and two additional pitched plugins.

### Changed

- **Composition with existing tuning.** Voice code now computes its base frequency via `TuningEngine::getFrequency(midi)` FIRST, then applies the NE semitone delta via `Ouaricon::NoteExpression::applyPendingTuning(table, midi, freq)`. This is multiplicatively correct for any base tuning (12-TET, Scala, MTS-ESP future) and satisfies the "no raw `std::pow` in voice" constraint — the `pow(2, semis/12)` call lives inside the module helper.
- **Spike diagnostic code stripped.** `OLyrica::detail::neTrace` and `detail::iidToHex` helpers plus all `neTrace(...)` call sites removed from `PluginProcessor.cpp` and `HarpSynthVoice.cpp`. `<fstream>` include removed. No more audio-thread file I/O (the spike wrote `/tmp/olyrica-ne-trace.log` synchronously).

### Removed

- **`Source/VST3/NoteExpressionSupport.h`** — deleted entirely. Replaced by the shared module (no plugin-local shim kept; sets the clean reference shape for Phase 24 adopters).

### Technical notes

- **JUCE patch required:** the NE path only works if `/Users/taylorbrook/JUCE` has the `JUCE-NE-PATCH` markers applied. After a JUCE upgrade, run `./scripts/apply-juce-patches.sh`. CMake verifies the markers at configure time and fails fast if missing.
- **Composition order is load-bearing:** `applyPendingTuning` must run AFTER the TuningEngine lookup but BEFORE `stringModel.trigger()`, so the first waveguide sample sizes to the tuned frequency. Landmine 4 of `vst3-note-expression-dorico.md`.
- **Version bump rationale:** MINOR (v2.2.2 → v2.3.0) — new user-visible feature (Dorico microtonal playback), backward compatible, no preset impact, no parameter changes.
- **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/HarpSynthVoice.{h,cpp}`, `CMakeLists.txt`.
- **Files deleted:** `Source/VST3/NoteExpressionSupport.h`.
```

**Style fidelity:** follows the observed pattern across `[2.2.2]` / `[2.2.1]` / `[2.2.0]` — Added / Changed / Removed / Technical notes / Files modified / Version bump rationale.

---

### `plugins/O-Lyrica/CMakeLists.txt` — version bump (D-19)

**Observation:** `plugins/O-Lyrica/CMakeLists.txt` does **NOT** currently carry an explicit `VERSION` field in its `juce_add_plugin(OLyrica ...)` call (confirmed via grep — only `cmake_minimum_required(VERSION 3.15)` matches).

**Grep result:**
```
$ grep -n "version\|VERSION" plugins/O-Lyrica/CMakeLists.txt
1:cmake_minimum_required(VERSION 3.15)
```

**Implication:** version tracking today lives ONLY in `CHANGELOG.md` + in-source `// v2.2.2:` comments (see `plugins/O-Lyrica/Source/DSP/WaveguideString.{h,cpp}` references to `v2.2.2`). No `VERSION "2.2.2"` field in `juce_add_plugin()`.

**Action for planner:**
- If D-19 strictly means "bump the version string" → only the CHANGELOG edit + optionally add a `VERSION "2.3.0"` field to `juce_add_plugin(OLyrica ...)` for VST3 metadata.
- If adding `VERSION` is new, recommend the planner pick ONE of: (a) add `VERSION 2.3.0` to `juce_add_plugin()` now; (b) defer to a later consistency phase.
- **Recommendation:** add `VERSION "2.3.0"` to `juce_add_plugin(OLyrica ...)` — populates the VST3's `PFactoryInfo::kVersionMajor`-style metadata correctly and matches the CHANGELOG.

**Excerpt — current `juce_add_plugin` call (lines 6–16):**
```cmake
juce_add_plugin(OLyrica
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OLyr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Lyrica${OUARICON_DEV_SUFFIX}"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)
```

**Proposed addition (one line after `PRODUCT_NAME`):**
```cmake
    VERSION "2.3.0"
```

---

## Shared Patterns

### 1. `ouaricon_add_module()` call-site syntax

**Source:** 22 existing call sites across `plugins/*/CMakeLists.txt` (see grep results in analysis).

**Apply to:** `plugins/O-Lyrica/CMakeLists.txt` (Phase 23) and all Phase 24 plugins.

**Canonical form:**
```cmake
ouaricon_add_module(<TARGET_NAME> <module-name>)
```

**Key observations:**
- Target name matches the first argument of `juce_add_plugin(NAME ...)` in the same file. Varies per plugin (`OLyrica`, `O-Bells`, `OuariconAnalogEQ`, `O-Bowed`, etc.).
- Include of `OuariconModules.cmake` is at top of file (line 2–4 typically); always present before any `ouaricon_add_module()` call.
- No existing module uses the optional `CONFIG` argument — skip it unless NE needs one.
- Gating via `if(OUARICON_LICENSING)` is only for the licensing module; note-expression is unconditional.

**Unconditional + conditional pattern (analog: `O-AnalogEQ/CMakeLists.txt` lines 56–61):**
```cmake
ouaricon_add_module(<TARGET> <unconditional-module>)

if(OUARICON_LICENSING)
    ouaricon_add_module(<TARGET> licensing)
endif()
```

---

### 2. Module layout convention

**Source:** `modules/persistence/preset-manager/`, `modules/core/licensing/`, `modules/tuning/scala-tuning-engine/`.

**Observed structure:**
```
modules/<category>/<module-name>/
├── module.yaml        # metadata
├── README.md          # consumer integration docs
├── cpp/               # .h and .cpp files, globbed by OuariconModules.cmake
│   ├── <Class>.h
│   └── <Class>.cpp    # may be absent for header-only modules (preset-manager)
├── js/                # optional — client-side JS
└── snippets/          # optional — copy-paste integration code
```

**Apply to note-expression:**
- `cpp/NoteExpression.h` — ONE file (header-only per MOD-04, D-07). No `.cpp`.
- NO `js/` (no UI per CONTEXT.md).
- NO `snippets/` (header-only means the README's integration code suffices).
- OPTIONAL `module.cmake` — new convention for the D-15 marker check.

**Precedent for header-only modules:** `modules/persistence/preset-manager/cpp/` contains exactly one `.h` file and no `.cpp` — confirms header-only is already an accepted shape.

**CONTEXT.md reference to `include/Ouaricon/NoteExpression/*.h` vs repo convention:**
The CONTEXT prompt lists `include/Ouaricon/NoteExpression/Controller.h` etc. The repo convention is `cpp/<Class>.h` (no nested `include/` hierarchy). **Recommendation to planner:** use the repo convention (`cpp/NoteExpression.h` as a single header) since it's the established pattern and `OuariconModules.cmake` already globs `cpp/*.h` — changing the convention for one module creates a special-case that Phase 24 plugins would inherit unnecessarily. Namespacing (`Ouaricon::NoteExpression::`) is already accomplished inside the `.h` file; the directory structure doesn't need to mirror the namespace.

---

### 3. Module metadata (`module.yaml`) shape

**Source:** `modules/tuning/scala-tuning-engine/module.yaml` (richest), `modules/core/licensing/module.yaml` (canonical minimal).

**Required top-level keys:** `name`, `version`, `description`, `category`, `author`, `provides`, `dependencies`, `requirements`, `sources`, `used_by`, `changelog`.

**Apply to note-expression:** see §Pattern Assignments → module.yaml.

---

### 4. Registry entry shape

**Source:** existing entries in `modules/registry.yaml` (lines 47–378).

**Required keys per entry:** `name`, `path`, `version`, `description`, `category`, `provides`, `dependencies`, `tags`, `reuse_score`, `used_by`.

**`used_by` convention:**
```yaml
used_by:
  - plugin: <PluginTargetName>
    version: <plugin-semver-that-first-included-the-module>
```

Apply to note-expression with `OLyrica` / `2.3.0` (the version adding consumption).

---

### 5. Bash script idioms (for `apply-juce-patches.sh`)

**Source:** `scripts/verify-backup.sh`.

**Idioms to adopt:**
- `#!/bin/bash` + `set -e` (line 1–2).
- Color codes as variables (`GREEN`, `YELLOW`, `RED`, `NC`) at top of script (lines 8–12).
- Early exit with `exit 1` and `echo -e "${RED}❌ ...${NC}"` on failure.
- Success echo with `${GREEN}✓${NC}` prefix.
- Positional arg parsing: `"$1"`, `"$2"` (our script uses env-var defaults instead; both idiomatic).

---

## No Analog Found

| File | Why new | Planner action |
|------|---------|----------------|
| `scripts/juce-patches/note-expression-juce-8.0.4.patch` | First `.patch` file in repo; D-12 upgrades the spike's markdown-hunks approach. | Generate via `diff -u pristine patched` (procedure documented above). No existing repo file to mirror structurally — the `.patch` format itself is the convention. |
| `modules/tuning/note-expression/module.cmake` | No existing module has a CMake hook; D-15's module-level marker check is a new convention. | Use `file(READ)` + `string(FIND)` (cross-platform). Requires one small additive edit to `OuariconModules.cmake` (documented above). |
| `scripts/apply-juce-patches.sh` | No existing idempotent patch-applier in repo. | Mirror `scripts/verify-backup.sh` bash style; idempotency via `grep -rln "$MARKER"` on JUCE tree before applying. |

---

## Key Patterns Identified

- **All plugin CMakeLists.txt use `ouaricon_add_module(<target> <module>)`** — 22 call sites, consistent syntax, target name always matches `juce_add_plugin()`'s first arg.
- **Modules self-describe via `module.yaml`** with a stable set of top-level keys; `provides.cpp-classes` + `sources.cpp` + `dependencies` + `used_by` are load-bearing.
- **Registry entry format is stable** — `name / path / version / category / provides / dependencies / tags / reuse_score / used_by`.
- **Header-only modules are precedented** (`preset-manager/cpp/OuariconPresetManager.h` alone). No CMakeLists-level changes needed — `OuariconModules.cmake` globs `cpp/*.h`.
- **O-Lyrica's current spike code at lines 208–211 of `PluginProcessor.h` and lines 705–763 of `PluginProcessor.cpp`** is the exact surface area to replace.
- **No existing CMake marker check exists** — Phase 23 introduces the convention via `module.cmake` hook. Safe because the hook is opt-in (only activated if `${MODULE_DIR}/module.cmake` exists).
- **Bash script style is consistent** across `scripts/*.sh` (bash + set -e + color codes + early exit).

---

## Metadata

- **Analog search scope:** `plugins/*/CMakeLists.txt`, `modules/**`, `scripts/**`, `plugins/O-Lyrica/Source/**`, `.claude/skills/spike-findings-VST-development/**`.
- **Files scanned:** 34 (4 module directories, 22 plugin CMakeLists, 4 shell scripts, 4 spike source files).
- **Pattern extraction date:** 2026-04-24.
