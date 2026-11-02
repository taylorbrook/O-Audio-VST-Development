# Quick Task 260719-k5o: Re-base JUCE-NE-PATCH onto JUCE 8.0.14 — Research

**Researched:** 2026-07-19
**Domain:** Vendored JUCE fork maintenance (VST3 Note Expression patch re-base)
**Confidence:** HIGH (all upstream facts fetched from raw.githubusercontent.com at tag 8.0.14; all repo facts grep-verified with file:line)

## Summary

The re-base is mechanically clean. The NE patch is a **small, self-contained addition** to two files:
one inline nested struct + one inline virtual in the header, and one braced event-forwarding block in
the VST3 wrapper `.cpp`. Both anchors survive verbatim in 8.0.14; the only structural change is that the
header **moved modules** (`juce_audio_processors` → `juce_audio_processors_headless`) and gained a
companion `.cpp` that does **not** need patching. The bulk of the work is re-pointing paths (CI copy step,
two grep gates, vendored layout, and one local-build guard `module.cmake`).

**Primary recommendation:** Re-vendor the two pristine 8.0.14 files (header at the NEW headless-module path),
re-stitch the two NE blocks (verbatim from the current vendored files — see Code Examples), leave the
companion `.cpp` unpatched, and update the four path touchpoints. The companion `.cpp` decision is
**do NOT patch** (verified: it defines only `getCompatibleParameterIds` + `convertJuceParameterId`, no NE code).

## Current Vendored State (grep-verified)

Two files under `vendored/JUCE-overrides/`:

| File | JUCE-NE-PATCH marker | NE modification extent |
|------|---------------------|------------------------|
| `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` | line 3699 | Braced block, vendored lines **3697–3749**: expands the one-liner `if (isMidiInputBusEnabled && data.inputEvents != nullptr) toMidiBuffer(...)` into `{ ...event-forward loop... MidiEventList::toMidiBuffer(...) }`, inside `#if JucePlugin_WantsMidiInput` |
| `modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h` | line 64 | Inserts `struct Vst3RawEvent` (lines 72–88) + `virtual void onVst3RawEvent (const Vst3RawEvent&) {}` (line 95), between `virtual ~VST3ClientExtensions() = default;` (line 61) and `virtual int32_t queryIEditController` (line 101). All inline. |

**Critical fact:** The vendored files are **byte-identical** to the local `/Users/taylorbrook/JUCE` 8.0.9 tree
(`diff` → IDENTICAL for `.h`; 0 changed lines for `.cpp`). The local JUCE install is already patched, and the
vendored overrides were captured from it. `/Users/taylorbrook/JUCE/modules/juce_core/juce_core.h:47` = `version: 8.0.9`.
→ The true NE-only delta cannot be obtained by diffing vendored-vs-local (both patched); use the Code Examples
blocks below, which ARE the complete NE additions.

## The Existing Patch File

`scripts/juce-patches/note-expression-juce-8.0.4.patch` (51 KB, `patch -p1` format from JUCE root):
- Touches exactly the two files (old header path + the `.cpp`).
- Header says `Target: JUCE 8.0.4`. **But** the diff carries **~33 hunks on the `.cpp`** including large
  non-NE hunks (`@@ -3969,189 +4131,21 @@`, `@@ -4168,136 +4162,24 @@`) — i.e. it is a diff against a base
  (8.0.4) that differs from the vendored/local content (8.0.9) well beyond the NE additions. `[VERIFIED: grep of patch hunk headers]`
  The NE-relevant `.cpp` hunk is `@@ -3583,7 +3695,57 @@`; the NE-relevant `.h` hunks are `@@ -60,6 +60,40 @@` and `@@ -104,... @@`.
- **Consumed by** `scripts/apply-juce-patches.sh` — local-dev only (fails loud if `/Users/taylorbrook/JUCE` missing;
  hard-codes `PATCH_FILE=".../note-expression-juce-8.0.4.patch"` at script line ~28 and the OLD header path at line 39).
  It does CRLF→LF normalization then `patch -p1`. **This script is NOT used by CI** (CI uses `cp -R` of vendored files).

> **Naming note:** CONTEXT decides rename `8.0.4.patch → 8.0.9.patch`. The mis-named base is confirmed, but be aware
> the *cleanest* regeneration of the 8.0.9 patch is `diff pristine-8.0.9 vendored/` (not a rename of the current file,
> which still carries 8.0.4-vs-8.0.9 drift hunks). The 8.0.14 patch = `diff pristine-8.0.14 re-stitched-8.0.14`.
> If you literally rename the current file, `apply-juce-patches.sh`'s `PATCH_FILE` string must also change.

## CI Touchpoints (`.github/workflows/build-and-release.yml`, live line numbers)

| What | Line(s) | Content |
|------|---------|---------|
| `JUCE_VERSION` env | **39** | `JUCE_VERSION: '8.0.9'` — **OUT OF SCOPE** (do not bump) |
| macOS JUCE download | 93 | `curl -L ".../releases/download/${JUCE_VERSION}/juce-${JUCE_VERSION}-osx.zip"` |
| **macOS cp -R copy** | **102** | `cp -R vendored/JUCE-overrides/modules/. JUCE/modules/` |
| **macOS grep gate (.h)** | **104** | `grep -q "JUCE-NE-PATCH" JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h` ← **path must change** |
| **macOS grep gate (.cpp)** | **105** | `grep -q "JUCE-NE-PATCH" JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` |
| Windows JUCE download | 442 | `Invoke-WebRequest .../juce-${JUCE_VERSION}-windows.zip` |
| **Windows cp -R copy** | **451** | `cp -R vendored/JUCE-overrides/modules/. JUCE/modules/` |
| **Windows grep gate (.h)** | **453** | same old `.h` path ← **path must change** |
| **Windows grep gate (.cpp)** | **454** | `.cpp` path (unchanged) |

The `cp -R vendored/JUCE-overrides/modules/. JUCE/modules/` lines (102, 451) need **no text change** IF the vendored
tree mirrors the new module path — `cp -R` walks whatever subtree exists. Only the two **`.h` grep-gate paths**
(104, 453) must be repointed to `modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h`.
The `.cpp` gate paths (105, 454) are unchanged.

## JUCE 8.0.14 Upstream Reality (VERIFIED via raw.githubusercontent.com @ tag 8.0.14)

1. **New header exists:** `modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h` — contains
   `struct VST3ClientExtensions` with `virtual ~VST3ClientExtensions() = default;` immediately followed by
   `virtual int32_t queryIEditController(...)`. → The NE insertion point (between destructor and `queryIEditController`)
   **survives verbatim.** `[VERIFIED: WebFetch]`
2. **Companion `.cpp` exists:** `modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.cpp` — defines
   **only** `getCompatibleParameterIds` and `convertJuceParameterId` out-of-line. **No** `Vst3RawEvent`, **no**
   `onVst3RawEvent`, no NE reference. → **DECISION: the companion `.cpp` does NOT need patching.** The NE patch adds only
   an inline nested struct + an inline empty-body virtual to the header; nothing goes out-of-line. `[VERIFIED: WebFetch]`
3. **`.cpp` anchor survives:** `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` @ 8.0.14, **lines 3589–3592**:
   ```cpp
          #if JucePlugin_WantsMidiInput
           if (isMidiInputBusEnabled && data.inputEvents != nullptr)
               MidiEventList::toMidiBuffer (midiBuffer, *data.inputEvents);
          #endif
   ```
   Pristine 8.0.14 is a **one-liner `if`** (no braces). The re-stitch replaces it with the braced NE block below. `[VERIFIED: curl+grep]`
4. **Old header path is GONE:** `modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h` @ 8.0.14 → **HTTP 404**
   (no forwarding shim). `[VERIFIED: WebFetch → 404]`
5. **Pristine source download:** `https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.14.tar.gz`
   (302-redirects to codeload; `curl -L` follows). Extracts to `JUCE-8.0.14/`. `[VERIFIED: curl -sI → 302]`
   Per-file raw fetch also works: `https://raw.githubusercontent.com/juce-framework/JUCE/8.0.14/<path>`.

## Compile-Check Feasibility (scratch 8.0.14, no touch to /Users/taylorbrook/JUCE)

- **JUCE location logic** (`CMakeLists.txt:35–41`): `if(DEFINED ENV{JUCE_DIR}) add_subdirectory($ENV{JUCE_DIR} JUCE)`
  else platform fallback (`/Users/taylorbrook/JUCE` on macOS). → **Set `JUCE_DIR=<scratch>/JUCE-8.0.14`** to override
  cleanly. Use a temp build dir: `cmake -B /tmp/ne-rebase-build -S . -DCMAKE_BUILD_TYPE=Release` with `JUCE_DIR` exported.
- **Compile-check plugin:** **O-Lyrica** — confirmed NE consumer (`plugins/O-Lyrica/Source/PluginProcessor.h:115`
  overrides `getVST3ClientExtensions()`) and the project's validated NE spike reference. Target `O-Lyrica_VST3`.
- **NE hook consumers** (11 plugins, all override `getVST3ClientExtensions`): O-Lyrica, O-Formant, O-MicrotonalSampler,
  O-Wind, O-Prism, O-Reed, O-Bells, O-Contrabass, O-Bassoon, O-IntonationPad, O-Bowed. The shared logic lives in
  `modules/tuning/note-expression/` (uses `juce::VST3ClientExtensions::Vst3RawEvent` + `onVst3RawEvent` override).

## Common Pitfalls

1. **`module.cmake` hard-codes the OLD header path AND wrong version** — `modules/tuning/note-expression/module.cmake:22`
   sets `_NE_FILE1 = "${_NE_JUCE_ROOT}/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h"` and its
   FATAL_ERROR message says "Ensure JUCE 8.0.4 is installed". This is a **configure-time guard that fires for any plugin
   consuming the note-expression module** (incl. O-Lyrica). Against a scratch 8.0.14 tree the old path won't exist →
   `FATAL_ERROR: Expected JUCE source not found`. **The compile-check WILL fail unless this path is updated to the new
   headless-module location** (or overridden for the check). This is a **5th path touchpoint** the plan must handle.
   Note: pointing it at the new path breaks local 8.0.9 dev builds — consistent with the "unmergeable-by-design" branch.
2. **Header moved modules, but no plugin `#include`s it directly** — grep of `plugins/` found **zero** direct includes of
   `juce_VST3ClientExtensions.h`; plugins use `juce::VST3ClientExtensions` via the umbrella `juce_audio_processors` module
   header. So the move needs **no plugin-source edits** — only the vendored layout + guard paths. `[VERIFIED: grep]`
3. **Vendored dir must be created at the new path** — moving (not copying) the vendored `.h` to
   `vendored/JUCE-overrides/modules/juce_audio_processors_headless/utilities/`. Leaving a stale copy at the old path is
   harmless to `cp -R` but confusing; remove the empty old dir.
4. **`apply-juce-patches.sh` OLD path (line 39) + PATCH_FILE name (line ~28)** — local-dev only, still 8.0.9-targeted.
   Out of scope for CI, but if the `.patch` is renamed, update the `PATCH_FILE` string or the script breaks. Leave the
   8.0.9 path in this script until the JUCE bump lands.
5. **CRLF on the Windows JUCE zip** — `apply-juce-patches.sh` normalizes CRLF→LF before `patch`. When re-generating the
   8.0.14 patch, generate against LF-normalized pristine source so it applies on the Windows CI download too.

## Code Examples (the complete NE additions — re-stitch these verbatim)

### `.h` — insert after `virtual ~VST3ClientExtensions() = default;`, before `queryIEditController`
```cpp
    //==============================================================================
    // JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22):
    // Surface raw VST3 input events — including kNoteExpressionValueEvent and the
    // noteIds on kNoteOnEvent/kNoteOffEvent — to the plugin ...
    struct Vst3RawEvent
    {
        enum class Kind : uint8_t { NoteOn, NoteOff, NoteExpressionValue, };
        Kind     kind{};
        int32_t  sampleOffset = 0;
        int32_t  noteId       = -1;
        int16_t  pitch        = -1;
        int16_t  channel      = -1;
        uint32_t typeId       = 0;
        double   value        = 0.0;
    };
    virtual void onVst3RawEvent (const Vst3RawEvent&) {}
```
(Full text: `vendored/.../juce_VST3ClientExtensions.h` lines 63–95 — copy verbatim.)

### `.cpp` — replace the pristine one-liner `if` (8.0.14 lines 3590–3591) with the braced block
```cpp
        if (isMidiInputBusEnabled && data.inputEvents != nullptr)
        {
            // JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22): forward raw VST3 events...
            if (auto* ext = pluginInstance->getVST3ClientExtensions())
            {
                const auto numRawEvents = data.inputEvents->getEventCount();
                for (Steinberg::int32 i = 0; i < numRawEvents; ++i)
                {
                    Steinberg::Vst::Event e;
                    if (data.inputEvents->getEvent (i, e) != Steinberg::kResultOk) continue;
                    VST3ClientExtensions::Vst3RawEvent raw{};
                    raw.sampleOffset = (int32_t) e.sampleOffset;
                    using Kind = VST3ClientExtensions::Vst3RawEvent::Kind;
                    if (e.type == Steinberg::Vst::Event::kNoteOnEvent) { raw.kind = Kind::NoteOn; raw.noteId = e.noteOn.noteId; raw.pitch = e.noteOn.pitch; raw.channel = e.noteOn.channel; }
                    else if (e.type == Steinberg::Vst::Event::kNoteOffEvent) { raw.kind = Kind::NoteOff; raw.noteId = e.noteOff.noteId; raw.pitch = e.noteOff.pitch; raw.channel = e.noteOff.channel; }
                    else if (e.type == Steinberg::Vst::Event::kNoteExpressionValueEvent) { raw.kind = Kind::NoteExpressionValue; raw.noteId = e.noteExpressionValue.noteId; raw.typeId = e.noteExpressionValue.typeId; raw.value = e.noteExpressionValue.value; }
                    else { continue; }
                    ext->onVst3RawEvent (raw);
                }
            }
            MidiEventList::toMidiBuffer (midiBuffer, *data.inputEvents);
        }
```
(Full text: `vendored/.../juce_audio_plugin_client_VST3.cpp` lines 3697–3749 — copy verbatim.)

## Path Change Summary (the actionable checklist)

| Touchpoint | From | To |
|-----------|------|-----|
| Vendored `.h` location | `vendored/JUCE-overrides/modules/juce_audio_processors/utilities/` | `.../juce_audio_processors_headless/utilities/` |
| Vendored `.cpp` location | (unchanged) `modules/juce_audio_plugin_client/` | (unchanged) |
| CI grep gate `.h` (yml:104, :453) | `JUCE/modules/juce_audio_processors/utilities/...` | `JUCE/modules/juce_audio_processors_headless/utilities/...` |
| CI grep gate `.cpp` (yml:105, :454) | (unchanged) | (unchanged) |
| CI `cp -R` (yml:102, :451) | (unchanged — walks new subtree) | (unchanged) |
| `module.cmake:22` `_NE_FILE1` | `.../juce_audio_processors/utilities/...` | `.../juce_audio_processors_headless/utilities/...` (needed for compile-check) |
| Patch file name | `note-expression-juce-8.0.4.patch` | rename → `-8.0.9.patch` + add new `-8.0.14.patch` |
| Companion `.cpp` @ new headless path | — | **NO patch needed** |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The current `.patch` file's non-NE hunks are 8.0.4↔8.0.9 drift (not additional intentional edits) | Existing Patch | Low — regenerating 8.0.9 patch as `diff pristine-8.0.9 vendored/` sidesteps this entirely; the vendored files are the source of truth |
| A2 | The braced `.cpp` block re-stitches cleanly onto the 8.0.14 one-liner without surrounding-context drift | Code Examples | Low — anchor + `#if`/`#endif` guard verified identical at 8.0.14; compile-check catches any surprise |

## Sources
- Primary (HIGH): raw.githubusercontent.com/juce-framework/JUCE/**8.0.14**/ — new `.h` path (exists), companion `.cpp`
  (2 out-of-line fns, no NE), old `.h` path (404), `.cpp` anchor (lines 3589–3592). `github.com/.../archive/refs/tags/8.0.14.tar.gz` (302 OK).
- Primary (HIGH): repo grep with file:line — vendored files, `build-and-release.yml`, `module.cmake`, `apply-juce-patches.sh`, 11 NE-consumer plugins.
- Local: `/Users/taylorbrook/JUCE` @ 8.0.9 == vendored (byte-identical).
