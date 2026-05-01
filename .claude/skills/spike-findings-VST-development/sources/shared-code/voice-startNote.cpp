// Extracted from O-Lyrica HarpSynthVoice.cpp startNote() (spike 001/003).
// Must run AFTER the voice's base frequency is computed (via TuningEngine,
// humanize, etc.) and BEFORE the DSP model's trigger call.
//
// Required member:
//   std::array<std::atomic<double>, 128>* pendingTuningSource = nullptr;
//
// Setter (wired up from processor constructor):
//   void setPendingTuningSource(std::array<std::atomic<double>, 128>* src)
//   { pendingTuningSource = src; }

// ... currentFrequency already set from TuningEngine / humanization ...

// VST3 Note Expression tuning delta (Dorico microtonal playback).
// exchange(0.0) ensures a retriggered note at the same pitch in a later
// block doesn't inherit a stale offset; the slot is always consumed.
if (pendingTuningSource != nullptr
    && midiNoteNumber >= 0
    && midiNoteNumber < 128)
{
    const double semis = (*pendingTuningSource)[(size_t) midiNoteNumber]
                              .exchange (0.0, std::memory_order_acq_rel);
    if (semis != 0.0)
        currentFrequency *= std::pow (2.0, semis / 12.0);
}

// ... stringModel.trigger(currentFrequency, velocity, ...) follows ...

// Why this ordering matters (Spike 003):
// - processor's drain ran before renderNextBlock this block, so
//   pendingTuningSource[midi] is already populated.
// - We apply the delta BEFORE calling trigger() so the DSP model sizes
//   its delay line / oscillator state from the final tuned frequency.
// - First output sample is therefore at the tuned frequency, no zipper.
