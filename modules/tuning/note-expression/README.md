# note-expression v1.0.0

VST3 Note Expression (`kTuningTypeID`) support for Dorico microtonal playback.

This module provides a header-only `Ouaricon::NoteExpression::Controller` (NEC),
a `juce::VST3ClientExtensions` subclass that owns the 128-slot pending-tuning
table and drains the patched JUCE wrapper's raw-event queue, and a voice-side
`applyPendingTuning` helper that composes per-note semitone deltas with any
base frequency.

> **Stub README.** Consumer integration, JUCE patch procedure, and Dorico
> expression-map setup are documented in Plan 04 / Phase 23 final (the
> comprehensive README ships with the JUCE patch tooling in Plan 02).

## Public API (preview)

```cpp
namespace Ouaricon::NoteExpression {
    using PendingTuningTable = std::array<std::atomic<double>, 128>;
    class Controller;       // NEC advertising kTuningTypeID
    class VST3Extensions;   // owns NEC + raw-event queue + PendingTuningTable
    inline double applyPendingTuning (PendingTuningTable&, int midi, double freq);
    inline void   updatePendingFromEvents (const std::vector<...>&, PendingTuningTable&);
}
```

## Requirements

- JUCE 8.0.4 with the `JUCE-NE-PATCH` markers applied
  (see `scripts/juce-patches/note-expression-juce-8.0.4.patch`, ships in Plan 02).
- C++20.
