/*
  ==============================================================================

    O-MicrotonalSampler - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): silent shell. APVTS + headless TuningEngine + NE drain
    + sample-map shared_ptr surface + SampleLoader skeleton. First audio: Phase 2.1.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <atomic>
#include <cmath>
#include <utility>

//==============================================================================
// Parameter Layout (frozen 7-parameter spec — PLAN.md §6 / RESEARCH.md §5)
juce::AudioProcessorValueTreeState::ParameterLayout OMicrotonalSamplerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========== ADSR (4) ==========

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 },
        "Attack",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.5f),
        0.005f,
        " s"
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "decay", 1 },
        "Decay",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.5f),
        0.1f,
        " s"
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sustain", 1 },
        "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        1.0f
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 },
        "Release",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.5f),
        0.3f,
        " s"
    ));

    // ========== Voicing (1) ==========

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "polyphony", 1 },
        "Polyphony",
        1, 16, 16
    ));

    // ========== Velocity-layer crossfade (1) ==========

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "velocity_crossfade", 1 },
        "Velocity Crossfade",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        1.0f
    ));

    // ========== Output (1) ==========

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output_gain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f),
        0.0f,
        " dB"
    ));

    return layout;
}

//==============================================================================
OMicrotonalSamplerAudioProcessor::OMicrotonalSamplerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , parameters (*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize sample-map slot with an empty SampleMap so audio-thread reads
    // see a valid (findSlot-returns-nullptr) target before any folder is loaded.
    currentSampleMap = std::make_shared<SampleMap>();

    // Pre-allocate 16 voices (matches max polyphony cap). Voice manager will
    // enforce the runtime cap; pre-allocating prevents processBlock allocations
    // when the user raises the cap (PERF-01).
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new MicrotonalSamplerVoice();
        voice->setAPVTS               (&parameters);
        voice->setTuningEngine        (&tuningEngine);                          // D-4: global namespace
        voice->setPendingTuningSource (&vst3Extensions.getPendingTable());      // module-owned table
        voice->setSampleMapSource     (&currentSampleMap);                      // shared_ptr slot
        synthesiser.addVoice (voice);
    }

    // Single shared sound (accepts all notes / all channels)
    synthesiser.addSound (new MicrotonalSamplerSound());

    // Voice-stealing: explicit (JUCE default is true; we make it explicit per
    // RESEARCH R1 — D2-2 satisfied by JUCE default findVoiceToSteal).
    synthesiser.setNoteStealingEnabled (true);

    // Background sample loader (Stage 1 stub — loadFolder dispatches a failure
    // callback to the message thread; run() is empty until Stage 2.2).
    sampleLoader = std::make_unique<SampleLoader>();
}

OMicrotonalSamplerAudioProcessor::~OMicrotonalSamplerAudioProcessor() = default;

//==============================================================================
void OMicrotonalSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Sampler is feed-forward; latency = 0 — do NOT call setLatencySamples.
    // (getLatencySamples is non-virtual in JUCE 8; default returns 0.)
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);

    // Phase 2.1: prepare every voice's ADSR sample-rate.
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* v = dynamic_cast<MicrotonalSamplerVoice*> (synthesiser.getVoice (i)))
            v->prepareToPlay (sampleRate, samplesPerBlock);
    }

    // Output-gain smoothing — 10 ms ramp (RESEARCH R7, pitfall #8).
    outputGainSmoother.reset (sampleRate, 0.01);
    if (auto* gp = parameters.getRawParameterValue ("output_gain"))
        outputGainSmoother.setCurrentAndTargetValue (
            juce::Decibels::decibelsToGain (gp->load()));

   #ifdef O_MICROTONAL_SAMPLER_PHASE_2_1_TEST_FIXTURE
    // Phase 2.1 in-memory test fixture: build a SampleMap with one slot per
    // MIDI note 21..108 (FUNC-04 range), 1 layer each, 0.25 s sine burst at
    // each note's ET frequency recorded at host SR (so playRate=1.0 plays the
    // correct pitch when the note is played at its native key).
    {
        const double hostSR = sampleRate;
        const int    numFrames = (int) std::floor (0.25 * hostSR);

        auto map = std::make_shared<SampleMap>();
        map->slots.reserve (108 - 21 + 1);
        map->lowestNote        = 21;
        map->highestNote       = 108;
        map->numVelocityLayers = 1;

        for (int midi = 21; midi <= 108; ++midi)
        {
            SampleSlot s;
            s.midiNote         = midi;
            s.velocityLayer    = 0;
            s.sourceSampleRate = hostSR;
            s.loopStart        = 0;
            s.loopEnd          = 0;          // one-shot in 2.1
            // Phase 3.1: audio held via shared_ptr<AudioBuffer<float>>.
            s.audio = std::make_shared<juce::AudioBuffer<float>> (2, numFrames);
            s.audio->clear();

            const double freq = 440.0 * std::pow (2.0, (midi - 69) / 12.0);
            const double twoPi = juce::MathConstants<double>::twoPi;
            const double phaseInc = twoPi * freq / hostSR;
            const float  amp  = 0.25f;       // -12 dBFS to leave plenty of headroom
            // 5 ms cosine fade-in / fade-out to keep the burst click-free; the
            // ADSR will further shape, but a clean source matters.
            const int fadeSamples = juce::jmin (numFrames / 4,
                                                (int) std::floor (0.005 * hostSR));

            float* L = s.audio->getWritePointer (0);
            float* R = s.audio->getWritePointer (1);

            double phase = 0.0;
            for (int n = 0; n < numFrames; ++n)
            {
                float w = 1.0f;
                if (fadeSamples > 0)
                {
                    if (n < fadeSamples)
                        w = (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi
                                       * (double) n / (double) fadeSamples));
                    else if (n >= numFrames - fadeSamples)
                    {
                        const int k = numFrames - 1 - n;
                        w = (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi
                                       * (double) k / (double) fadeSamples));
                    }
                }
                const float v = amp * w * (float) std::sin (phase);
                L[n] = v;
                R[n] = v;
                phase += phaseInc;
                if (phase > twoPi) phase -= twoPi;
            }

            map->slots.push_back (std::move (s));
        }

        // Phase 3.1: bump version (test fixture path).
        map->version = 1;

        // Atomic-store into the processor's slot. std::atomic_store on
        // shared_ptr is deprecated in C++20; both the deprecated free function
        // and a plain assignment are acceptable here because the processor
        // ctor only ever assigns once at startup before any voice runs.
        // Voices snapshot via copy at startNote; that copy is the lock-free
        // refcount inc.
       #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
        std::atomic_store (&currentSampleMap, map);
       #else
        currentSampleMap = map;
       #endif
    }
   #endif
}

void OMicrotonalSamplerAudioProcessor::releaseResources()
{
    // Stage 1: nothing to release (no externally-allocated resources).
}

bool OMicrotonalSamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Synth: stereo output only, no input bus.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Reject any input bus (synths are output-only).
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    return true;
}

void OMicrotonalSamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Synth: clear all channels (no input; voices add to buffer).
    buffer.clear();

    // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
    // correlate tuning deltas to their NoteOn's MIDI pitch.
    // MUST run BEFORE renderNextBlock so per-voice startNote sees pending NE deltas.
    vst3Extensions.drainAndUpdate();

    // Render all voices via synthesiser (handles MIDI routing + voice allocation).
    synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Output-gain smoothing (RESEARCH pitfall #8 / R7). Read parameter atomically,
    // convert to linear, drive the smoother target. Apply via applyGainRamp using
    // start/end snapshots (RESEARCH pitfall #9 — single ramp per block, no zipper).
    if (auto* gp = parameters.getRawParameterValue ("output_gain"))
        outputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (gp->load()));

    const int numSamples = buffer.getNumSamples();
    const float startGain = outputGainSmoother.getCurrentValue();
    outputGainSmoother.skip (numSamples);
    const float endGain   = outputGainSmoother.getCurrentValue();

    if (! juce::approximatelyEqual (startGain, 1.0f) || ! juce::approximatelyEqual (endGain, 1.0f))
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGainRamp (ch, 0, numSamples, startGain, endGain);
    }
}

//==============================================================================
void OMicrotonalSamplerAudioProcessor::loadSampleFolder (const juce::File& folder)
{
    if (sampleLoader == nullptr)
        return;

    // Capture `this` by raw pointer — folder load is short-lived and the
    // processor outlives the loader (sampleLoader is a unique_ptr member;
    // ~SampleLoader joins the thread before the processor finishes destruction).
    sampleLoader->loadFolder (
        folder,
        getSampleRate() > 0.0 ? getSampleRate() : 48000.0,

        // Completion callback — runs on the message thread.
        [this](std::shared_ptr<SampleMap> newMap, juce::StringArray skipped)
        {
            lastSkippedFiles = std::move (skipped);

            // Phase 3.1: bump version on every map replace. Voices snapshot
            // the map at startNote; the version field is read by the Stage 3
            // UI for diff detection (RESEARCH §RQ3-2).
            if (newMap != nullptr)
            {
               #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
                auto prev = std::atomic_load (&currentSampleMap);
               #else
                auto prev = currentSampleMap;
               #endif
                newMap->version = (prev != nullptr ? prev->version : 0) + 1;
            }

            // Atomic-store into the processor's slot. Voices snapshot via
            // shared_ptr copy at startNote (refcount inc — RT-safe). Use the
            // same C++20 feature guard already established in prepareToPlay.
           #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
            std::atomic_store (&currentSampleMap, newMap);
           #else
            currentSampleMap = newMap;
           #endif

            DBG ("SampleLoader complete: " << (int) currentSampleMap->slots.size()
                 << " slot(s), " << lastSkippedFiles.size() << " skipped (v"
                 << currentSampleMap->version << ")");

            // Phase 3.1: notify editor (message thread). Editor's lambda
            // forwards to webView->emitEventIfBrowserIsVisible("sampleMapUpdated",
            // snapshotSampleMapJson()). No-op if no editor is open.
            if (sampleMapChangedCallback)
                sampleMapChangedCallback();
        },

        // Failure callback — runs on the message thread.
        [this](const juce::String& reason)
        {
            DBG ("SampleLoader failure: " << reason);
            juce::ignoreUnused (reason);
            lastSkippedFiles.clear();
        });
}

//==============================================================================
// Phase 3.1: per-cell sample load (skeleton — full impl in 3.2).
void OMicrotonalSamplerAudioProcessor::loadSingleSample (int midiPitch,
                                                          int velocityLayer,
                                                          const juce::File& file)
{
    DBG ("loadSingleSample (skeleton): midi=" << midiPitch
         << " vel=" << velocityLayer
         << " file=" << file.getFullPathName());
    juce::ignoreUnused (midiPitch, velocityLayer, file);
}

//==============================================================================
// Phase 3.1: loop-point override (skeleton — full impl in 3.4).
void OMicrotonalSamplerAudioProcessor::overrideLoopPoints (int midiPitch,
                                                            int velocityLayer,
                                                            int loopStart,
                                                            int loopEnd,
                                                            int crossfadeLen,
                                                            bool resetToAutoDetect)
{
    DBG ("overrideLoopPoints (skeleton): midi=" << midiPitch
         << " vel=" << velocityLayer
         << " start=" << loopStart << " end=" << loopEnd
         << " xfade=" << crossfadeLen
         << " reset=" << (resetToAutoDetect ? "1" : "0"));
    juce::ignoreUnused (midiPitch, velocityLayer, loopStart, loopEnd,
                        crossfadeLen, resetToAutoDetect);
}

//==============================================================================
// Phase 3.1: snapshot the current sample map as a JSON string per RESEARCH
// §RQ3-2 schema. Read-only — atomic_load on the shared_ptr is the only
// thread sync; lastSkippedFiles is touched only on the message thread (this
// path) and the loader completion path which also runs on the message thread.
juce::String OMicrotonalSamplerAudioProcessor::snapshotSampleMapJson() const
{
   #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    auto map = std::atomic_load (&currentSampleMap);
   #else
    auto map = currentSampleMap;
   #endif

    auto loopModeToString = [] (LoopMode m) -> const char*
    {
        switch (m)
        {
            case LoopMode::OneShot: return "one-shot";
            case LoopMode::Auto:    return "auto";
            case LoopMode::Manual:  return "manual";
        }
        return "one-shot";
    };

    juce::String json;
    json.preallocateBytes (1024);
    json << "{";

    if (map == nullptr)
    {
        json << "\"version\":0,\"lowestNote\":0,\"highestNote\":0,"
             << "\"numVelocityLayers\":1,\"slots\":[],\"skippedFiles\":[]}";
        return json;
    }

    json << "\"version\":"           << map->version
         << ",\"lowestNote\":"       << map->lowestNote
         << ",\"highestNote\":"      << map->highestNote
         << ",\"numVelocityLayers\":" << map->numVelocityLayers
         << ",\"slots\":[";

    bool firstSlot = true;
    for (const auto& s : map->slots)
    {
        if (! firstSlot) json << ",";
        firstSlot = false;

        const int lengthSamples = (s.audio != nullptr) ? s.audio->getNumSamples() : 0;

        json << "{"
             << "\"midiNote\":"          << s.midiNote
             << ",\"velocityLayer\":"    << s.velocityLayer
             << ",\"filename\":"         << juce::JSON::toString (juce::var (s.filename))
             << ",\"lengthSamples\":"    << lengthSamples
             << ",\"sourceSampleRate\":" << juce::String (s.sourceSampleRate, 4)
             << ",\"loopStart\":"        << s.loopStart
             << ",\"loopEnd\":"          << s.loopEnd
             << ",\"loopMode\":\""       << loopModeToString (s.loopMode) << "\""
             << "}";
    }
    json << "],\"skippedFiles\":[";

    bool firstSkip = true;
    for (const auto& sf : lastSkippedFiles)
    {
        if (! firstSkip) json << ",";
        firstSkip = false;
        json << juce::JSON::toString (juce::var (sf));
    }
    json << "]}";

    return json;
}

//==============================================================================
// Phase 3.1: skeleton — full impl in 3.4. Returns empty object so JS callers
// do not crash on `JSON.parse(await getWaveformPeaks(...))`.
juce::String OMicrotonalSamplerAudioProcessor::snapshotWaveformPeaks (int midiPitch,
                                                                      int velocityLayer,
                                                                      int targetBins) const
{
    juce::ignoreUnused (midiPitch, velocityLayer, targetBins);
    return "{}";
}

//==============================================================================
juce::AudioProcessorEditor* OMicrotonalSamplerAudioProcessor::createEditor()
{
    return new OMicrotonalSamplerAudioProcessorEditor (*this);
}

//==============================================================================
void OMicrotonalSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Standard APVTS XML round-trip.
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void OMicrotonalSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// Factory function (JUCE plugin entry point)
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OMicrotonalSamplerAudioProcessor();
}
