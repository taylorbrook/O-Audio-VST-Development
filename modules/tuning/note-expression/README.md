# note-expression v1.0.0

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

## Dorico End-User Setup

Dorico's default and "Auto" microtonality settings route microtones to VST2
detune or pitch bend for non-Steinberg VST3 plugins — neither reaches a
JUCE-based VST3 plugin. The plugin will play 12-TET no matter how
microtonal the score is. Users MUST create a custom expression map.

Four-step procedure:

1. `Dorico -> Library -> Expression Maps...`
2. Duplicate an existing compatible map (or create a new one).
3. Set **Microtonality** to **"VST3 Note Expression"**. Save the map.
4. In `Play -> Endpoint Setup`, assign the new expression map to your
   plugin's channel.

**Verification.** Write a quarter-sharp accidental on C4. Playback should
land at C4 + 50¢ ≈ 269.29 Hz (vs standard C4 = 261.63 Hz). If the note
plays at standard C4, either the expression map is not assigned to the
channel or its Microtonality is still set to Auto.

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

## Canonical Dorico Expression Map (v1.1.0+)

Module v1.1.0 ships a canonical pre-configured Dorico expression map at
`modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap`.
Microtonality is hard-coded to **"VST3 Note Expression"** — the load-bearing
invariant from spike-findings Landmine 3 (Dorico's `Auto` selection picks
pitch-bend for non-Steinberg VST3s and silently breaks microtonal playback).

### Auto-install on consumer plugins

No per-plugin code is needed. The module's `module.cmake` registers
`install()` rules that fire automatically when a plugin consumes the module
via `ouaricon_add_module(<Plugin> note-expression)`. The plugin's installer
(PKG on macOS, EXE on Windows) inherits the rules and writes the
`.doricoexpmap` to two paths on the user's machine:

| Platform | Editable canonical copy | Dorico auto-scan path |
|----------|------------------------|------------------------|
| macOS | `~/Library/Application Support/Ouaricon/Expression Maps/` | `~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/` |
| Windows | `%APPDATA%\Ouaricon\Expression Maps\` | `%APPDATA%\Steinberg\Dorico [N]\Expression Maps\User\` |

`[N]` is the latest Dorico major version detected at install time
(probed in order: 6, 5, 4). When no Dorico install is detected, the
auto-scan write is skipped and the file's `README-doricoexpmap.txt`
explains manual import via `Dorico → Library → Expression Maps…`.

### Source of truth

Edits to installed copies are overwritten by the next plugin installer
run. The canonical edit point is the file in the module repo:
`modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap`.

### Supported plugins (v1.5 cohort)

All 8 v1.5 microtonal-cohort plugins inherit the install rules:
O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed,
O-Formant.

## License

Part of the Ouaricon Audio module library.
