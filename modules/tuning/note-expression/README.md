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

## Dorico End-User Setup (v1.1.0+ auto-discovery flow)

Starting in module v1.1.0, every cohort plugin's installer ships the
Ouaricon Microtonal Suite Playback Template + expression-map library
directly to Dorico's auto-scan directories. NO manual import is
required for the typical case.

**Auto-discovery (default):**

1. Install any Ouaricon plugin (PKG on macOS or EXE on Windows).
2. Restart Dorico.
3. `Play -> Playback Template -> Ouaricon Microtonal Suite -> Apply and Close`.
4. Quarter-sharp C4 verifies routing: pitch lands at +50¢ (~269.29 Hz).

The installer writes to:

- macOS: `~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/` and `.../Default Library Additions/`
- Windows: `%APPDATA%\Steinberg\Dorico [N]\PlaybackTemplateSpecs\Ouaricon Microtonal Suite\` and `...\DefaultLibraryAdditions\` (note: NO spaces on Windows; spaces on macOS — Pitfall 3)

A canonical editable copy of both files (`Ouaricon-Microtonal-Suite.dorico_pt`
and `Ouaricon-VST3-NoteExpression.doricolib`) is also written to the
shared `~/Library/Application Support/Ouaricon/Microtonal Suite/`
(macOS) or `%APPDATA%\Ouaricon\Microtonal Suite\` (Windows) directory
for manual reference.

**Manual import fallback** (if Dorico did not pick up the auto-discovered files):

1. `Play -> Playback Template -> Import...` -> select `Ouaricon-Microtonal-Suite.dorico_pt` from the shared directory above.
2. `Library -> Import Library...` -> select `Ouaricon-VST3-NoteExpression.doricolib` from the same shared directory.

**Underlying mechanics.** Dorico's default and "Auto" microtonality
settings route microtones to VST2 detune or pitch bend for non-Steinberg
VST3 plugins — neither reaches a JUCE-based VST3 plugin. The shipped
expression map sets `microtonalPlaybackMethod=kVST3NoteExpression`, the
load-bearing Dorico setting that routes microtones as VST3 Note
Expression events.

**Verification.** Write a quarter-sharp accidental on C4. Playback should
land at C4 + 50¢ ≈ 269.29 Hz (vs standard C4 = 261.63 Hz). If the note
plays at standard C4, the Playback Template is not applied (apply via
`Play -> Playback Template`) or Dorico did not auto-discover the files
(use the manual fallback above).

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
