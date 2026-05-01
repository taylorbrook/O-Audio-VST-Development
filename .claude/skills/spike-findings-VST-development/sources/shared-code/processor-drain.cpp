// Extracted from O-Lyrica PluginProcessor.cpp (spike 001/002).
// Placed at the top of processBlock, immediately after `buffer.clear()`
// and BEFORE any call to synthesiser.renderNextBlock.
//
// Required processor-level members (in the plugin class header):
//   OLyrica::LyricaVST3Extensions vst3Extensions;   // from NoteExpressionSupport.h
//   std::array<std::atomic<double>, 128> pendingTuningSemis {};
//   std::vector<juce::VST3ClientExtensions::Vst3RawEvent> rawEventScratch;
//
// And override:
//   juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
//
// Each voice must receive a pointer to pendingTuningSemis via a setter wired
// up where other voice setters live (e.g. constructor voice-initialization loop):
//   voice->setPendingTuningSource (&pendingTuningSemis);

{
    vst3Extensions.drainBlockEvents (rawEventScratch);

    if (! rawEventScratch.empty())
    {
        using Kind = juce::VST3ClientExtensions::Vst3RawEvent::Kind;

        // Pass 1: build noteId -> MIDI pitch map for this block's NoteOns.
        // Important: Dorico represents microtonal notes by the *nearest*
        // neighbor semitone + an NE tuning delta (e.g. quarter-sharp C4
        // arrives as pitch=C#4 with NE=-50c, not pitch=C4 with NE=+50c).
        // Correlate via noteId, never via MIDI pitch arithmetic.
        std::map<int32_t, int> noteIdToPitch;
        for (const auto& e : rawEventScratch)
            if (e.kind == Kind::NoteOn)
                noteIdToPitch[e.noteId] = (int) e.pitch;

        // Pass 2: apply tuning NE events to their pitch bucket.
        for (const auto& e : rawEventScratch)
        {
            if (e.kind != Kind::NoteExpressionValue)
                continue;
            if (e.typeId != Steinberg::Vst::kTuningTypeID)
                continue;

            auto it = noteIdToPitch.find (e.noteId);
            if (it == noteIdToPitch.end())
                continue;  // NE for a note from a prior block.
                           // Real build: keep a rolling noteId->voice map
                           // so mid-note NE updates from other hosts work.

            const int pitch = it->second;
            if (pitch < 0 || pitch >= 128)
                continue;

            // VST3 kTuningTypeID: normalized [0,1] -> plain semitones [-120, +120].
            // Formula: plain = 240 * (norm - 0.5)
            // Example: 0.497917 -> -0.5 semitones = -50 cents
            const double semitones = 240.0 * (e.value - 0.5);

            pendingTuningSemis[(size_t) pitch]
                .store (semitones, std::memory_order_release);
        }
    }
}

// After this block, synthesiser.renderNextBlock (...) runs. Each voice's
// startNote reads-and-clears its pending tuning slot via exchange(0.0).
