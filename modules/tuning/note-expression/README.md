# note-expression v1.1.0

VST3 Note Expression (`kTuningTypeID`) support for Dorico microtonal playback.
The module wraps the VST3 Note Expression mechanism so JUCE-based plugins can
respond to Dorico's per-note tuning deltas.

This is a header-only C++ module under the `Ouaricon::NoteExpression` namespace.
It owns the 128-slot `PendingTuningTable`, advertises the Note Expression
Controller (NEC) to hosts, drains the patched JUCE wrapper's raw-event queue,
and provides a one-line voice helper that composes a Dorico-driven semitone
delta with any base frequency (e.g. a `TuningEngine` lookup).

## Quick Start

1. Apply the JUCE patch once (per JUCE version):
   ```bash
   ./scripts/apply-juce-patches.sh
   ```
2. Add the module to your plugin's `CMakeLists.txt`:
   ```cmake
   include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
   ouaricon_add_module(YourPlugin note-expression)
   ```
3. See [Installation](#installation) and [Integration Approach](#integration-approach)
   below for processor + voice wiring.

## Features

- **`Ouaricon::NoteExpression::Controller`** — advertises `kTuningTypeID` to
  hosts on `IEditController` query (Cubase / Nuendo handshake; Dorico ignores
  it but other VST3 hosts use it).
- **`Ouaricon::NoteExpression::VST3Extensions`** — subclass of
  `juce::VST3ClientExtensions` that owns the 128-slot `PendingTuningTable`,
  drains raw VST3 events forwarded by the patched JUCE wrapper, and
  dispatches `queryIEditController` to the NEC.
- **`Ouaricon::NoteExpression::updatePendingFromEvents`** — two-pass
  drain/correlate helper that maps NE events to MIDI pitches via `noteId`
  (never by pitch — Dorico represents quarter-sharp C4 as `pitch=C#4, NE=-50¢`).
- **`Ouaricon::NoteExpression::applyPendingTuning`** — one-line voice helper.
  Composes multiplicatively with any base frequency (TuningEngine, MIDI
  standard, MTS-ESP future). `pow(2, semis/12)` is encapsulated inside the
  helper, so voice code never calls `std::pow` directly.

## Installation

### 1. Apply the JUCE patch

Upstream JUCE 8.0.4 drops `kNoteExpressionValueEvent` and `noteId`-tagged
NoteOn/NoteOff events inside `MidiEventList::toMidiBuffer` because they
have no representation in `juce::MidiMessage`. The local patch at
`scripts/juce-patches/note-expression-juce-8.0.4.patch` adds
`VST3ClientExtensions::onVst3RawEvent` so the plugin sees the raw events
before the lossy conversion.

Run the idempotent applier:
```bash
./scripts/apply-juce-patches.sh
```
The script skips application if the `JUCE-NE-PATCH` marker is already
present in the JUCE tree (defaults to `/Users/taylorbrook/JUCE`; override
via the `JUCE_DIR` environment variable). After a JUCE upgrade, regenerate
the patch file (procedure documented in the patch header) and re-run
`apply-juce-patches.sh`.

### 2. Register the module in your plugin's CMakeLists.txt

```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
ouaricon_add_module(YourPlugin note-expression)
```

`ouaricon_add_module` auto-includes the module's `module.cmake`, which
fatal-errors at configure time if the `JUCE-NE-PATCH` marker is missing
from the JUCE tree. The check is opt-in — only plugins that consume
`note-expression` are gated.

### 3. Wire the processor

Include the header, declare the extensions member, return it from
`getVST3ClientExtensions()`, and call `drainAndUpdate()` at the top of
`processBlock` before `renderNextBlock`:

```cpp
#include "NoteExpression.h"   // from modules/tuning/note-expression

class YourPluginProcessor : public juce::AudioProcessor
{
public:
    juce::VST3ClientExtensions* getVST3ClientExtensions() override
    {
        return &vst3Extensions;
    }

    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midi) override
    {
        buffer.clear();
        vst3Extensions.drainAndUpdate();   // drain raw events + correlate by noteId
        synthesiser.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
    }

    // In your voice-init loop (e.g. after addVoice):
    //   voice->setPendingTuningSource (&vst3Extensions.getPendingTable());

private:
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
    juce::Synthesiser synthesiser;
};
```

### 4. Wire the voice

The voice receives a pointer to the module-owned `PendingTuningTable` and
calls `applyPendingTuning` after computing its base frequency, before any
DSP trigger:

```cpp
class YourVoice : public juce::SynthesiserVoice
{
public:
    void setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* t)
    {
        pendingTuningSource = t;
    }

    void startNote (int midi, float vel,
                    juce::SynthesiserSound*, int /*pitchWheel*/) override
    {
        // 1) Compute base frequency via your tuning pipeline
        //    (TuningEngine, MIDI-standard, humanize, etc.)
        double currentFrequency = computeBaseFrequency (midi);

        // 2) Apply the NE semitone delta — AFTER base frequency, BEFORE the DSP trigger.
        //    The helper consumes the slot via exchange(0.0) so retriggered notes
        //    at the same pitch in a later block don't inherit a stale offset.
        if (pendingTuningSource != nullptr)
            currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
                                   *pendingTuningSource, midi, currentFrequency);

        // 3) Trigger the DSP model with the final tuned frequency
        stringModel.trigger (currentFrequency, vel);
    }

private:
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
};
```

**Composition order is load-bearing.** `applyPendingTuning` must run AFTER
base-frequency computation (so tunings stack multiplicatively) and BEFORE
the DSP trigger (so the first output sample sizes to the tuned frequency —
no attack zipper). See landmine 4 in
`.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md`.

## Dorico End-User Setup (v1.1.0+ Path B import flow)

Starting in module v1.1.0, every cohort plugin's installer ships a
single canonical `.doricolib` to a platform-specific Ouaricon shared
path. The user performs a one-time Library Manager Import per machine,
then assigns the expression map per project.

### Quick Start

- Install any Ouaricon plugin (PKG on macOS, EXE on Windows). The
  installer drops `Ouaricon-VST3-NoteExpression.doricolib` to the
  Ouaricon shared path (see [Source of Truth](#source-of-truth)).
- One-time per machine: run `Library → Library Manager → Import…` in
  Dorico and select the `.doricolib` from the shared path.
- Per project: load any Ouaricon plugin via `Play → Endpoints → Add
  Plug-in`, then assign the imported expression map to that channel
  via `Play → Endpoints → Expression Map` dropdown.

### Manual Import Steps

1. Open Dorico (any project).
2. `Library → Library Manager → Import…`.
3. Navigate to the install location and select
   `Ouaricon-VST3-NoteExpression.doricolib`:
   - macOS: `~/Library/Application Support/Ouaricon/Microtonal Suite/`
   - Windows: `%APPDATA%\Ouaricon\Microtonal Suite\`
4. Confirm the import. The expression map "Ouaricon VST3 Note
   Expression" now appears under `Library → Expression Maps`.
5. Per project: load any Ouaricon plugin under
   `Play → Endpoints → Add Plug-in`, then assign the expression map to
   the plugin's channel via `Play → Endpoints → Expression Map`.

The import persists across Dorico restarts and version upgrades.
Re-importing after a plugin reinstall is harmless but not required —
the `.doricolib` content is stable.

**Verification.** Write a quarter-sharp accidental on C4 and play.
Pitch lands at C4 + 50¢ ≈ 269.29 Hz (vs standard C4 = 261.63 Hz). If
the note plays at standard C4, the expression map is not assigned to
the plugin's channel under `Play → Endpoints`, or the import has not
yet been performed.

### Source of Truth

The canonical `.doricolib` lives in-repo at
`modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`
and is installed to:

- macOS: `~/Library/Application Support/Ouaricon/Microtonal Suite/`
- Windows: `%APPDATA%\Ouaricon\Microtonal Suite\`

There are no writes to Dorico's user-data directories under v1.5 — a
ship-time flow that pre-stages the `.doricolib` directly into
`Library → Expression Maps` without a manual Import is a v1.6 revival
candidate, deferred per Phase 25 D-08 carry-forward.

**Underlying mechanics.** Dorico's default and "Auto" microtonality
settings route microtones to VST2 detune or pitch bend for non-Steinberg
VST3 plugins — neither reaches a JUCE-based VST3 plugin. The shipped
expression map sets `microtonalPlaybackMethod=kVST3NoteExpression`, the
load-bearing Dorico setting that routes microtones as VST3 Note
Expression events.

## JUCE Patch Management

- **Marker (do not rename):** `JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)`.
  Both `grep -rn "JUCE-NE-PATCH" /Users/taylorbrook/JUCE/modules/` and the
  CMake-time check rely on this verbatim string. Renaming the marker
  silently breaks the configure-time gate and the apply-script
  idempotency check.
- **Re-apply after JUCE upgrade.** Diff pristine-new vs forked-new (procedure
  in the patch file's header), rename the file to
  `note-expression-juce-<NEW-VERSION>.patch`, update `module.yaml`'s
  `requirements.juce_patch.file` and `.juce_version`, commit, and re-run
  `scripts/apply-juce-patches.sh`.
- **What the patch does.** Surfaces `kNoteExpressionValueEvent` plus
  `noteId`-tagged NoteOn/NoteOff events to
  `VST3ClientExtensions::onVst3RawEvent` BEFORE
  `MidiEventList::toMidiBuffer` drops them. Approach 2 (side-channel
  queue) was chosen over Approach 1 (MidiBuffer mutation) to avoid
  round-tripping through JUCE's MIDI layer.

## Integration Approach

Unlike `scala-tuning-engine` (a self-contained UI panel with 880 lines of
JS, 5 visualizations, and 20+ native functions), `note-expression` is
header-only C++ with **no UI** — no JS, no CSS, no native functions, no
APVTS parameters.

Consumers need exactly three lines:

1. The `ouaricon_add_module(YourPlugin note-expression)` CMake call.
2. A `Ouaricon::NoteExpression::VST3Extensions` member on the processor.
3. A voice-side `applyPendingTuning` call after base-frequency computation.

Phase 24's seven pitched plugins (O-Bells, O-Wind, O-Reed, O-Bowed,
O-Formant, O-IntonationPad, O-Prism) all follow this same shape. O-Lyrica
v2.3.0 is the reference consumer.

## Dependencies

- JUCE modules: `juce_audio_processors` (for `VST3ClientExtensions`),
  `juce_core`.
- C++20.
- Local JUCE patch (see [JUCE Patch Management](#juce-patch-management)).
- **No dependency on `scala-tuning-engine`** — the module composes
  multiplicatively with any base-frequency source, so consumers without
  a tuning engine can use it directly against MIDI-standard frequencies.

## License

Part of the Ouaricon Audio module library.
