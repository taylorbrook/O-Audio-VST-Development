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

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
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

    // ========== Expression (1) — v1.7.0 ==========
    //
    // Real-time dynamics control. Independent of velocity-layer selection:
    // velocity (note-on velocity) still selects which layer plays; expression
    // scales the post-mix output. Driven by MIDI CC 11 (Expression Controller,
    // industry-standard for orchestral mockups) AND by host automation —
    // last-touched wins. Squared curve (CC²) and 10 ms smoothing applied at
    // gain time in processBlock.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "expression", 1 },
        "Expression",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        1.0f,
        " %",
        juce::AudioProcessorParameter::genericParameter,
        [] (float v, int)         { return juce::String (juce::roundToInt (v * 100.0f)); },
        [] (const juce::String& s){ return juce::jlimit (0.0f, 1.0f, s.getFloatValue() / 100.0f); }
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

    // v1.7.0: expression smoothing — same 10 ms ramp; target stores the
    // squared curve so the ramp moves through final linear gain space.
    expressionSmoother.reset (sampleRate, 0.01);
    if (auto* ep = parameters.getRawParameterValue ("expression"))
    {
        const float v = ep->load();
        expressionSmoother.setCurrentAndTargetValue (v * v);
    }

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

    // v1.7.0: scan MIDI for CC 11 (Expression Controller). Last-value-wins
    // within the block — sample-accurate splitting was deemed unnecessary
    // (10 ms smoothing on the gain side covers per-block jumps without zipper).
    // setValueNotifyingHost forwards to host automation lanes AND triggers
    // the WebSliderRelay valueChangedEvent so the UI knob tracks the CC.
    for (const auto meta : midiMessages)
    {
        const auto msg = meta.getMessage();
        if (msg.isController() && msg.getControllerNumber() == 11)
        {
            if (auto* ep = parameters.getParameter ("expression"))
                ep->setValueNotifyingHost (msg.getControllerValue() / 127.0f);
        }
    }

    // FUNC-03: propagate the polyphony APVTS cap into the synth before MIDI is
    // dispatched, so CappedSynthesiser::noteOn enforces the user's cap on this
    // block's note-ons.
    if (auto* pp = parameters.getRawParameterValue ("polyphony"))
        synthesiser.setVoiceCap ((int) pp->load());

    // Render all voices via synthesiser (handles MIDI routing + voice allocation).
    synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();

    // v1.7.0: expression (dynamics) gain — squared curve, applied post-mix
    // before output_gain. Independent of velocity-layer selection (velocity
    // chooses the layer at note-on; expression scales the mix). Same
    // start/end ramp pattern as output_gain (RESEARCH pitfall #9).
    if (auto* ep = parameters.getRawParameterValue ("expression"))
    {
        const float v = ep->load();
        expressionSmoother.setTargetValue (v * v);
    }

    const float startExp = expressionSmoother.getCurrentValue();
    expressionSmoother.skip (numSamples);
    const float endExp   = expressionSmoother.getCurrentValue();

    if (! juce::approximatelyEqual (startExp, 1.0f) || ! juce::approximatelyEqual (endExp, 1.0f))
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGainRamp (ch, 0, numSamples, startExp, endExp);
    }

    // Output-gain smoothing (RESEARCH pitfall #8 / R7). Read parameter atomically,
    // convert to linear, drive the smoother target. Apply via applyGainRamp using
    // start/end snapshots (RESEARCH pitfall #9 — single ramp per block, no zipper).
    if (auto* gp = parameters.getRawParameterValue ("output_gain"))
        outputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (gp->load()));

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
// v1.7.1: held-notes snapshot for the TuningPanel TrueKeys / Circle / Polar
// visualizations. Walks the synth's active-notes bitmask in MIDI order and
// queries TuningEngine::getFrequency per held note so TrueKeys can compute
// real interval cents from the active tuning (not just 12-TET).
void OMicrotonalSamplerAudioProcessor::getHeldNotesData (std::vector<int>& notes,
                                                          std::vector<double>& freqs)
{
    notes.clear();
    freqs.clear();

    juce::uint64 low = 0, high = 0;
    synthesiser.getActiveNotes (low, high);

    // 128-bit bitmask → ordered MIDI list. At most 16 simultaneous voices
    // (polyphony cap), so the inner work is tiny.
    for (int midi = 0; midi < 128; ++midi)
    {
        const juce::uint64 mask = (juce::uint64) 1 << (midi & 63);
        const bool held = (midi < 64 ? (low & mask) : (high & mask)) != 0;
        if (! held) continue;

        notes.push_back (midi);
        freqs.push_back (tuningEngine.getFrequency (midi));
    }
}

//==============================================================================
// v1.6.0: folder load with explicit velocity-layer assignment.
//
// User-triggered loads (Load Folder… button, drag-drop, missing-folder
// relocate) flow through here. State-restore replays go through
// kickNextReplayOp() instead so the merge logic can chain via the same
// applyFolderLoad helper without contention with this public path.
void OMicrotonalSamplerAudioProcessor::loadSampleFolder (const juce::File& folder,
                                                          int      targetLayer,
                                                          LoadMode mode,
                                                          bool     overrideTokens)
{
    if (sampleLoader == nullptr)
        return;

    const int  clampedLayer = juce::jlimit (0, 3, targetLayer);
    const LoadOp op { folder.getFullPathName(), clampedLayer, mode, overrideTokens };

    // Track the most recent loaded folder for any single-path UI display.
    // Cleared on failure to keep stale paths out of the missing-folder modal.
    currentSampleFolder = folder;

    // Capture `this` by raw pointer — folder load is short-lived and the
    // processor outlives the loader (sampleLoader is a unique_ptr member;
    // ~SampleLoader joins the thread before the processor finishes destruction).
    sampleLoader->loadFolder (
        folder,
        getSampleRate() > 0.0 ? getSampleRate() : 48000.0,
        LoadOptions { clampedLayer, overrideTokens },

        // Completion callback — runs on the message thread.
        [this, op](std::shared_ptr<SampleMap> newMap, juce::StringArray skipped)
        {
            applyFolderLoad (std::move (newMap), skipped, op);
        },

        // Failure callback — runs on the message thread.
        [this](const juce::String& reason)
        {
            DBG ("SampleLoader failure: " << reason);
            juce::ignoreUnused (reason);
            lastSkippedFiles.clear();
            // Drop the recorded folder so a failed reload doesn't get
            // re-persisted. (clearSampleMap leaves currentSampleFolder
            // untouched on purpose — only an explicit load-failure clears.)
            currentSampleFolder = juce::File();
        });
}

//==============================================================================
// v1.6.0: shared completion logic for user-triggered and replay folder loads.
// Merges newSlotsMap (which contains ONLY the freshly loaded folder's slots)
// into currentSampleMap per op.mode, updates metadata + history, and fires
// the sample-map-changed callback. Runs on the message thread.
void OMicrotonalSamplerAudioProcessor::applyFolderLoad (
    std::shared_ptr<SampleMap> newSlotsMap,
    const juce::StringArray&   skipped,
    const LoadOp&              op)
{
    if (newSlotsMap == nullptr)
        return;

   #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    auto prev = std::atomic_load (&currentSampleMap);
   #else
    auto prev = currentSampleMap;
   #endif

    auto merged = std::make_shared<SampleMap>();

    if (op.mode == LoadMode::ReplaceAll || prev == nullptr)
    {
        // Wipe the existing map; truncate history to a single op.
        merged->slots = newSlotsMap->slots;
        loadOpHistory.clear();
    }
    else
    {
        // Start from the existing map's slots; mutate per mode.
        merged->slots = prev->slots;

        if (op.mode == LoadMode::ReplaceLayer)
        {
            const int target = juce::jlimit (0, 3, op.targetLayer);
            merged->slots.erase (
                std::remove_if (merged->slots.begin(), merged->slots.end(),
                    [target] (const SampleSlot& s)
                    {
                        return s.velocityLayer == target;
                    }),
                merged->slots.end());
        }

        // Append (or finish ReplaceLayer) — for any (midi, layer) collision,
        // the new slot wins.
        for (const auto& newSlot : newSlotsMap->slots)
        {
            merged->slots.erase (
                std::remove_if (merged->slots.begin(), merged->slots.end(),
                    [&newSlot] (const SampleSlot& s)
                    {
                        return s.midiNote      == newSlot.midiNote
                            && s.velocityLayer == newSlot.velocityLayer;
                    }),
                merged->slots.end());
            merged->slots.push_back (newSlot);
        }
    }

    // Recompute metadata over the merged slot list.
    merged->lowestNote        = 127;
    merged->highestNote       = 0;
    int maxLayer              = 0;
    for (const auto& s : merged->slots)
    {
        merged->lowestNote  = juce::jmin (merged->lowestNote,  s.midiNote);
        merged->highestNote = juce::jmax (merged->highestNote, s.midiNote);
        maxLayer            = juce::jmax (maxLayer,            s.velocityLayer);
    }
    if (merged->slots.empty())
    {
        merged->lowestNote  = 127;
        merged->highestNote = 0;
    }
    merged->numVelocityLayers = juce::jlimit (1, 4, maxLayer + 1);
    merged->version           = (prev != nullptr ? prev->version : 0) + 1;

   #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    std::atomic_store (&currentSampleMap, merged);
   #else
    currentSampleMap = merged;
   #endif

    lastSkippedFiles = skipped;
    loadOpHistory.push_back (op);

    DBG ("loadSampleFolder applied: mode=" << (int) op.mode
         << " layer=" << op.targetLayer
         << " override=" << (int) op.overrideTokens
         << " merged slots=" << (int) merged->slots.size()
         << " (v" << merged->version << ")");

    if (sampleMapChangedCallback)
        sampleMapChangedCallback();
}

//==============================================================================
// v1.6.0: replay-queue dispatcher. Pops the next op, skips it if its folder
// is gone (first missing path raises the existing missing-folder modal), and
// dispatches an async load whose completion chains back here. Stack-safe via
// JUCE's MessageManager::callAsync — each chained call is a fresh message.
void OMicrotonalSamplerAudioProcessor::kickNextReplayOp()
{
    while (! pendingReplayOps.empty())
    {
        const LoadOp op = pendingReplayOps.front();
        pendingReplayOps.erase (pendingReplayOps.begin());

        const juce::File f (op.path);
        if (! f.isDirectory())
        {
            // Surface the FIRST missing folder; subsequent ones are silently
            // skipped to keep the user's modal interaction simple. They can
            // re-locate or load fresh after dismissing.
            if (pendingMissingFolderPath.isEmpty())
            {
                pendingMissingFolderPath = op.path;
                if (missingFolderCallback)
                    missingFolderCallback (op.path);
            }
            continue;   // try the next op
        }

        sampleLoader->loadFolder (
            f,
            getSampleRate() > 0.0 ? getSampleRate() : 48000.0,
            LoadOptions { op.targetLayer, op.overrideTokens },
            [this, op] (std::shared_ptr<SampleMap> newMap, juce::StringArray skipped)
            {
                applyFolderLoad (std::move (newMap), skipped, op);
                kickNextReplayOp();   // chain to next op
            },
            [this] (const juce::String& reason)
            {
                DBG ("SampleLoader replay failure: " << reason);
                juce::ignoreUnused (reason);
                kickNextReplayOp();   // continue chain even on failure
            });
        return;   // wait for async completion before dispatching next
    }
}

//==============================================================================
// Phase 3.2: per-cell sample load (full impl).
//
// Pipeline (RESEARCH §RQ3-3 + Phase 3.2 PLAN Task 13):
//   1. Validate (midi, vel, ext, file existence).
//   2. Spawn SampleLoader::loadSingleSlot worker → SR-convert + loop-detect.
//   3. Completion callback runs on the message thread:
//        a. atomic_load currentSampleMap (snapshot).
//        b. Deep-copy header + slots, dropping any prior (midi, vel) match.
//        c. Push the new slot.
//        d. Update lowestNote/highestNote/numVelocityLayers if extended.
//        e. version = prev + 1.
//        f. atomic_store currentSampleMap.
//        g. Append skip reason to lastSkippedFiles if non-empty.
//        h. Fire sampleMapChangedCallback.
//
// Active-voice retention (Stage 2 EC-3): voices snapshot the SampleMap
// shared_ptr in startNote (lock-free refcount inc) and hold it for the
// note duration. Replacing a slot allocates a fresh SampleMap; held voices
// keep their old buffer alive transitively until the note releases.
void OMicrotonalSamplerAudioProcessor::loadSingleSample (int midiPitch,
                                                          int velocityLayer,
                                                          const juce::File& file)
{
    if (sampleLoader == nullptr)
        return;

    // ------------------------ Validation guards ------------------------
    if (midiPitch < 0 || midiPitch > 127)
    {
        DBG ("loadSingleSample: midi out of range (" << midiPitch << ")");
        return;
    }

    // velocityLayer must fit within the *current* map's layer count, OR the
    // expansion target (we cap at 4 layers map-wide). A vel layer up to 3 is
    // always permissible — the map's numVelocityLayers grows automatically.
    if (velocityLayer < 0 || velocityLayer > 3)
    {
        DBG ("loadSingleSample: velocityLayer out of range (" << velocityLayer
             << ") — must be 0..3");
        return;
    }

    if (! file.existsAsFile())
    {
        DBG ("loadSingleSample: file does not exist (" << file.getFullPathName() << ")");
        return;
    }

    const juce::String ext = file.getFileExtension().toLowerCase();
    if (ext != ".wav" && ext != ".aif" && ext != ".aiff" && ext != ".flac")
    {
        DBG ("loadSingleSample: unsupported extension (" << ext << ")");
        return;
    }

    const double sr = (getSampleRate() > 0.0) ? getSampleRate() : 48000.0;

    // ------------------------ Async load ------------------------
    sampleLoader->loadSingleSlot (
        file,
        midiPitch,
        velocityLayer,
        sr,
        [this, midiPitch, velocityLayer] (SampleSlot newSlot, juce::String skipReason)
        {
            // Failure path — newSlot.midiNote == -1 (default) signals failure.
            if (newSlot.midiNote < 0 || newSlot.audio == nullptr)
            {
                if (skipReason.isNotEmpty())
                {
                    lastSkippedFiles.add (skipReason);
                    DBG ("loadSingleSample failed: " << skipReason);
                }
                else
                {
                    DBG ("loadSingleSample failed (no reason supplied)");
                }
                // Still notify the editor so the UI can surface the new
                // skipped file even when no map change happened.
                if (sampleMapChangedCallback)
                    sampleMapChangedCallback();
                return;
            }

            // Success — the loader already populated newSlot with the right
            // midi/vel coordinates, but be explicit to defend against any
            // future loader-API drift.
            newSlot.midiNote      = midiPitch;
            newSlot.velocityLayer = velocityLayer;

            // Snapshot the current map (atomic_load).
           #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
            auto currentMap = std::atomic_load (&currentSampleMap);
           #else
            auto currentMap = currentSampleMap;
           #endif

            // Deep-copy header + filter old (midi, vel) match out, then push
            // the new slot. The vector copy is cheap because each slot's
            // audio is a shared_ptr (Phase 3.1 invariant addition — RQ3-3).
            auto next = std::make_shared<SampleMap>();
            if (currentMap != nullptr)
            {
                next->slots.reserve (currentMap->slots.size() + 1);
                for (const auto& s : currentMap->slots)
                {
                    if (s.midiNote == midiPitch && s.velocityLayer == velocityLayer)
                        continue;   // drop the old slot at this cell
                    next->slots.push_back (s);  // shared_ptr copy = pointer copy
                }
                next->lowestNote        = currentMap->lowestNote;
                next->highestNote       = currentMap->highestNote;
                next->numVelocityLayers = currentMap->numVelocityLayers;
                next->version           = currentMap->version;
            }
            else
            {
                next->lowestNote        = 127;
                next->highestNote       = 0;
                next->numVelocityLayers = 1;
                next->version           = 0;
            }
            next->slots.push_back (std::move (newSlot));

            // Extend header to cover the inserted cell.
            next->lowestNote  = juce::jmin (next->lowestNote,  midiPitch);
            next->highestNote = juce::jmax (next->highestNote, midiPitch);
            next->numVelocityLayers = juce::jlimit (1, 4,
                juce::jmax (next->numVelocityLayers, velocityLayer + 1));

            // First-load corner case — if the map was empty, lowestNote/highestNote
            // were both midiPitch and that's correct. Reset bounds defensively
            // when slots becomes the inserted note in isolation.
            if (next->slots.size() == 1)
            {
                next->lowestNote  = midiPitch;
                next->highestNote = midiPitch;
            }

            // Bump version (every atomic-store).
            next->version = (currentMap != nullptr ? currentMap->version : 0) + 1;

            // Atomic-store. Voices snapshot via shared_ptr copy at startNote
            // (refcount inc — RT-safe). Already-held voices continue with
            // their snapshot of the OLD map, keeping the old buffer alive
            // for the duration of the note (Stage 2 EC-3 invariant).
           #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
            std::atomic_store (&currentSampleMap, next);
           #else
            currentSampleMap = next;
           #endif

            DBG ("loadSingleSample success: midi=" << midiPitch
                 << " vel=" << velocityLayer
                 << " slots=" << (int) next->slots.size()
                 << " v" << next->version);

            // Notify editor (push event to JS via emitEventIfBrowserIsVisible).
            if (sampleMapChangedCallback)
                sampleMapChangedCallback();
        });
}

//==============================================================================
// Phase 3.4: loop-point override + reset-to-auto-detect (full implementation).
//
// Pipeline (RESEARCH §RQ3-4 + Phase 3.4 PLAN Task 23):
//   1. atomic_load currentSampleMap (snapshot).
//   2. Locate (midi, vel) slot via SampleMap::findSlot.
//   3. If absent: DBG log + return (no-op).
//   4. Deep-copy header + slots into next = std::make_shared<SampleMap>(*current)
//      (cheap — slots hold shared_ptrs to audio buffers).
//   5. Locate the matching slot in `next->slots` (mutable).
//   6. Override path: set loopStart, loopEnd, loopMode = Manual.
//      Reset path (v1.4.0): set loopStart=0, loopEnd=N-2, loopMode=Auto
//        (whole-file loop default). Falls back to OneShot only when audio
//        is too short (< 18 samples).
//   7. Bump version. atomic_store. Fire callback.
//
// Active-voice retention (Stage 2 EC-3): voices already snapshot
// std::shared_ptr<SampleMap> at startNote — replacing the map after override
// keeps held voices on their old snapshot for the held note's duration. New
// loop region applies on the next note-on (EC3-6).
//
// crossfadeLen: recorded in DBG log for v1.1 (per RP3-2: per-slot xfade is a
// v1.1 candidate — global crossfade stays in voices for v1.0).
void OMicrotonalSamplerAudioProcessor::overrideLoopPoints (int midiPitch,
                                                            int velocityLayer,
                                                            int loopStart,
                                                            int loopEnd,
                                                            int crossfadeLen,
                                                            bool resetToAutoDetect)
{
    // Snapshot the current map.
   #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    auto current = std::atomic_load (&currentSampleMap);
   #else
    auto current = currentSampleMap;
   #endif

    if (current == nullptr)
    {
        DBG ("overrideLoopPoints: no current map");
        return;
    }

    const auto* foundSlot = current->findSlot (midiPitch, velocityLayer);
    if (foundSlot == nullptr)
    {
        DBG ("overrideLoopPoints: slot absent (midi=" << midiPitch
             << " vel=" << velocityLayer << ")");
        return;
    }

    // Deep-copy header + slot vector. Each slot's audio is a shared_ptr so
    // this is cheap — vector of pointers + POD fields.
    auto next = std::make_shared<SampleMap> (*current);

    // Locate the matching slot in the COPIED vector (mutable).
    SampleSlot* targetSlot = nullptr;
    for (auto& s : next->slots)
    {
        if (s.midiNote == midiPitch && s.velocityLayer == velocityLayer)
        {
            targetSlot = &s;
            break;
        }
    }

    if (targetSlot == nullptr)
    {
        // Should never happen — current->findSlot found one but next's deep
        // copy didn't. Defensive bail-out.
        DBG ("overrideLoopPoints: target slot vanished after deep copy");
        return;
    }

    if (resetToAutoDetect)
    {
        // v1.4.0: "Reset" snaps loop points back to whole-file default
        // (loopStart = 0, loopEnd = N - 2). Mirror SampleLoader::processOneFile.
        const int numSamples = (targetSlot->audio != nullptr)
                                   ? targetSlot->audio->getNumSamples()
                                   : 0;

        if (numSamples >= 18)
        {
            targetSlot->loopStart = 0;
            targetSlot->loopEnd   = numSamples - 2;
            targetSlot->loopMode  = LoopMode::Auto;
            DBG ("resetLoopToAutoDetect: midi=" << midiPitch
                 << " vel=" << velocityLayer
                 << " whole-file loop=[0, " << (numSamples - 2) << "]");
        }
        else
        {
            targetSlot->loopStart = 0;
            targetSlot->loopEnd   = 0;
            targetSlot->loopMode  = LoopMode::OneShot;
            DBG ("resetLoopToAutoDetect: midi=" << midiPitch
                 << " vel=" << velocityLayer
                 << " buffer too short → one-shot");
        }
    }
    else
    {
        // Manual override. Clamp to the slot's audio length defensively.
        const int numSamples = (targetSlot->audio != nullptr)
                                   ? targetSlot->audio->getNumSamples()
                                   : 0;
        const int clampedStart = juce::jlimit (0, juce::jmax (0, numSamples - 1), loopStart);
        const int clampedEnd   = juce::jlimit (clampedStart + 1,
                                               juce::jmax (clampedStart + 1, numSamples),
                                               loopEnd);

        targetSlot->loopStart = clampedStart;
        targetSlot->loopEnd   = clampedEnd;
        targetSlot->loopMode  = LoopMode::Manual;

        DBG ("overrideLoopPoints: midi=" << midiPitch
             << " vel=" << velocityLayer
             << " manual loop=[" << clampedStart << ", " << clampedEnd << "]"
             << " xfade=" << crossfadeLen << " (recorded for v1.1; ignored in v1.0)");
        juce::ignoreUnused (crossfadeLen);
    }

    // Bump version (every atomic-store).
    next->version = current->version + 1;

    // Atomic-store. Voices snapshot via shared_ptr copy at startNote.
   #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    std::atomic_store (&currentSampleMap, next);
   #else
    currentSampleMap = next;
   #endif

    // Notify editor.
    if (sampleMapChangedCallback)
        sampleMapChangedCallback();
}

void OMicrotonalSamplerAudioProcessor::resetLoopToAutoDetect (int midiPitch,
                                                               int velocityLayer)
{
    // Reuse override path with the resetToAutoDetect flag set.
    overrideLoopPoints (midiPitch, velocityLayer, 0, 0, 0, /*resetToAutoDetect*/ true);
}

//==============================================================================
// v1.0.2: clearSampleMap — atomic-store an empty SampleMap, bumping the
// version counter. Active voices retain their previously snapshotted map
// (Stage 2 EC-3) so in-flight notes finish naturally; new note-ons after the
// clear find an empty map and produce silence. Skipped-file list is also
// cleared so the Issues disclosure resets. Fires the sample-map change
// callback so the WebView grid refreshes to its empty state.
void OMicrotonalSamplerAudioProcessor::clearSampleMap()
{
    auto fresh = std::make_shared<SampleMap>();

   #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    auto prev = std::atomic_load (&currentSampleMap);
   #else
    auto prev = currentSampleMap;
   #endif

    fresh->version = (prev != nullptr ? prev->version : 0) + 1;

   #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    std::atomic_store (&currentSampleMap, fresh);
   #else
    currentSampleMap = fresh;
   #endif

    lastSkippedFiles.clear();

    // v1.6.0: drop the load-op history so the next save reflects the cleared
    // state. (currentSampleFolder is intentionally left alone — same comment
    // as v1.3.0.)
    loadOpHistory.clear();

    DBG ("clearSampleMap: cleared (v" << fresh->version << ")");

    if (sampleMapChangedCallback)
        sampleMapChangedCallback();
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
// Phase 3.4: walk the slot's audio buffer and emit per-bin (min, max) pairs.
// Output JSON per RESEARCH.md §RQ3-5 schema:
//   { midiNote, velocityLayer, lengthSamples, sourceSampleRate,
//     loopStart, loopEnd, loopMode, peaks: [[min, max], ...] }
//
// Per-bin loop walks `framesPerBin = numFrames / targetBins` samples,
// summing channels per sample (mono mixdown) then dividing by numChannels
// for normalization. std::minmax tracks the bin extrema in O(N) total —
// typical 5 s sample at 48 kHz ≈ 240 k samples ≈ 1 ms on Apple Silicon,
// message-thread acceptable for the click-driven open path.
juce::String OMicrotonalSamplerAudioProcessor::snapshotWaveformPeaks (int midiPitch,
                                                                      int velocityLayer,
                                                                      int targetBins) const
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

    if (map == nullptr)
        return "{}";

    const auto* slot = map->findSlot (midiPitch, velocityLayer);
    if (slot == nullptr || slot->audio == nullptr || slot->audio->getNumSamples() == 0)
        return "{}";

    const int numFrames   = slot->audio->getNumSamples();
    const int numChannels = juce::jmax (1, slot->audio->getNumChannels());
    const int bins        = juce::jlimit (1, juce::jmax (1, numFrames),
                                          juce::jmax (1, targetBins));

    // framesPerBin floor; the tail-bin absorbs any leftover samples.
    const int framesPerBin = juce::jmax (1, numFrames / bins);

    juce::var peaksArray = juce::var (juce::Array<juce::var>{});
    auto* peaksArr = peaksArray.getArray();

    // Reuse one juce::var per bin for the (min, max) pair.
    for (int b = 0; b < bins; ++b)
    {
        const int binStart = b * framesPerBin;
        // Last bin extends to numFrames to absorb the floor remainder.
        const int binEnd   = (b == bins - 1) ? numFrames
                                              : juce::jmin (numFrames, binStart + framesPerBin);

        if (binStart >= numFrames)
        {
            juce::var pair (juce::Array<juce::var>{});
            pair.append (0.0);
            pair.append (0.0);
            peaksArr->add (pair);
            continue;
        }

        float minV =  std::numeric_limits<float>::infinity();
        float maxV = -std::numeric_limits<float>::infinity();

        for (int n = binStart; n < binEnd; ++n)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                sum += slot->audio->getReadPointer (ch)[n];
            const float v = sum / static_cast<float> (numChannels);
            if (v < minV) minV = v;
            if (v > maxV) maxV = v;
        }

        // Defensive — if the bin happened to be empty (binEnd == binStart),
        // both extrema remain at +/- infinity. Clamp to 0.
        if (! std::isfinite (minV)) minV = 0.0f;
        if (! std::isfinite (maxV)) maxV = 0.0f;

        juce::var pair (juce::Array<juce::var>{});
        pair.append (static_cast<double> (minV));
        pair.append (static_cast<double> (maxV));
        peaksArr->add (pair);
    }

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("midiNote",         slot->midiNote);
    obj->setProperty ("velocityLayer",    slot->velocityLayer);
    obj->setProperty ("lengthSamples",    numFrames);
    obj->setProperty ("sourceSampleRate", slot->sourceSampleRate);
    obj->setProperty ("loopStart",        slot->loopStart);
    obj->setProperty ("loopEnd",          slot->loopEnd);
    obj->setProperty ("loopMode",         juce::String (loopModeToString (slot->loopMode)));
    obj->setProperty ("filename",         slot->filename);
    obj->setProperty ("peaks",            peaksArray);

    return juce::JSON::toString (juce::var (obj), /*allOnOneLine*/ true);
}

//==============================================================================
juce::AudioProcessorEditor* OMicrotonalSamplerAudioProcessor::createEditor()
{
    return new OMicrotonalSamplerAudioProcessorEditor (*this);
}

//==============================================================================
// v1.3.0 — full-state persistence.
//
// Pre-v1.3.0 only round-tripped APVTS parameters, so reopening a project
// dropped the loaded sample folder and any tuning edits the user had made.
// v1.3.0 wraps the APVTS state ValueTree with two extra sibling children:
//
//   <APVTS>
//     <PARAM .../>             ← existing knob params (untouched)
//     <SampleFolder path="..."/>  ← absolute path of last-loaded folder
//     <TuningState ...>           ← masterTune, octaveStretch, mode, tonic,
//                                    intervals, generated SCL/KBM content
//   </APVTS>
//
// Backward compatibility: v1.2.0 sessions loaded in v1.3.0 simply lack the
// new children — sampler starts empty, tuning falls back to 12-TET. v1.3.0
// sessions loaded in v1.2.0 silently drop the new children (APVTS only
// reads PARAM children).
//
// The custom .omspreset file uses the same ValueTree, just serialized as
// plain XML text instead of JUCE's `copyXmlToBinary` framing — so users can
// share preset bundles across projects on the same machine. (Per Q1=A:
// sample data is referenced by path, not embedded.)

namespace
{
    constexpr const char* kSampleFolderTag  = "SampleFolder";   // legacy (v1.5.x and older)
    constexpr const char* kSampleFoldersTag = "SampleFolders";  // v1.6.0 op-list container
    constexpr const char* kSampleFolderOpTag = "Op";            // v1.6.0 child element
    constexpr const char* kTuningStateTag   = "TuningState";

    // v1.6.0: human-readable mode strings in XML so saved presets are
    // diffable / editable by hand. Unknown values fall back to ReplaceAll.
    juce::String loadModeToString (LoadMode m) noexcept
    {
        switch (m)
        {
            case LoadMode::Append:       return "append";
            case LoadMode::ReplaceLayer: return "replace_layer";
            case LoadMode::ReplaceAll:   return "replace_all";
        }
        return "replace_all";
    }

    LoadMode loadModeFromString (const juce::String& s) noexcept
    {
        if (s == "append")        return LoadMode::Append;
        if (s == "replace_layer") return LoadMode::ReplaceLayer;
        return LoadMode::ReplaceAll;
    }

    // Capture every accessible bit of TuningEngine state into a ValueTree.
    // The shared scala-tuning-engine module exposes enough getters for
    // round-trip without modifying the module itself: settings come from
    // direct getters; intervals come from getIntervals(); SCL/KBM round-trip
    // via the engine's own generate*FileContent helpers.
    juce::ValueTree captureTuningValueTree (TuningEngine& engine)
    {
        juce::ValueTree t (kTuningStateTag);
        t.setProperty ("masterTune",     engine.getMasterTune(),                       nullptr);
        t.setProperty ("octaveStretch",  static_cast<double> (engine.getOctaveStretch()), nullptr);
        t.setProperty ("mode",           static_cast<int>    (engine.getMode()),       nullptr);
        t.setProperty ("tonic",          engine.getTonicNote(),                        nullptr);
        t.setProperty ("preset",         static_cast<int>    (engine.getBuiltInPreset()), nullptr);
        t.setProperty ("name",           engine.getActiveTuningName(),                 nullptr);

        // Intervals as a comma-separated cents list (compact + human-readable).
        const auto intervals = engine.getIntervals();
        juce::String csv;
        for (size_t i = 0; i < intervals.size(); ++i)
        {
            if (i > 0) csv << ",";
            csv << juce::String (intervals[i], 6);
        }
        t.setProperty ("intervals", csv, nullptr);

        // Embed the engine's own SCL/KBM round-trip text. On restore we
        // write these to temp files and call loadScalaFile/loadKBMFile —
        // that path captures every non-getter-exposed bit of state (kbm
        // mapping, scaleName, scaleDegrees, etc.) without forking the module.
        t.setProperty ("scl", engine.generateScalaFileContent(), nullptr);
        t.setProperty ("kbm", engine.generateKBMFileContent(),   nullptr);
        return t;
    }

    void restoreTuningFromValueTree (TuningEngine& engine, const juce::ValueTree& t)
    {
        if (! t.isValid()) return;

        // Apply settings first — setMasterTune et al. recompute the
        // frequency table internally.
        if (t.hasProperty ("masterTune"))
            engine.setMasterTune (static_cast<double> (t.getProperty ("masterTune")));
        if (t.hasProperty ("octaveStretch"))
            engine.setOctaveStretch (static_cast<float>  (t.getProperty ("octaveStretch")));
        if (t.hasProperty ("preset"))
            engine.setBuiltInPreset (static_cast<TuningEngine::BuiltInPreset> (
                                         static_cast<int> (t.getProperty ("preset"))));

        // Restore intervals via setCustomIntervals (the engine's standard
        // entry point used by every UI write path — same code path as
        // applyGeneratedScale / loadEmbeddedTuning).
        if (t.hasProperty ("intervals"))
        {
            const auto csv  = t.getProperty ("intervals").toString();
            const auto name = t.hasProperty ("name") ? t.getProperty ("name").toString()
                                                     : juce::String ("Restored");
            std::vector<double> cents;
            cents.reserve (16);
            int start = 0;
            for (int i = 0; i <= csv.length(); ++i)
            {
                if (i == csv.length() || csv[i] == ',')
                {
                    if (i > start)
                        cents.push_back (csv.substring (start, i).getDoubleValue());
                    start = i + 1;
                }
            }
            if (! cents.empty())
                engine.setCustomIntervals (cents, name);
        }

        // KBM mapping — write to a temp .kbm file and load. The engine's
        // KBM parser is the only path to repopulate kbmMapping[], kbmFirstNote,
        // etc. (no public setters), and the round-trip is lossless because
        // we serialised via the engine's own generateKBMFileContent.
        if (t.hasProperty ("kbm"))
        {
            const auto kbm = t.getProperty ("kbm").toString();
            if (kbm.isNotEmpty())
            {
                auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory)
                               .getChildFile ("o-microtonalsampler-restore-" +
                                              juce::String (juce::Time::currentTimeMillis()) + ".kbm");
                if (tmp.replaceWithText (kbm))
                {
                    engine.loadKBMFile (tmp);
                    tmp.deleteFile();
                }
            }
        }

        // SCL — only reload if Mode was Scala at save time. setBuiltInPreset
        // above already applied the active intervals for non-Scala modes.
        if (t.hasProperty ("mode")
            && static_cast<TuningEngine::Mode> (static_cast<int> (t.getProperty ("mode")))
                   == TuningEngine::Mode::Scala
            && t.hasProperty ("scl"))
        {
            const auto scl = t.getProperty ("scl").toString();
            if (scl.isNotEmpty())
            {
                auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory)
                               .getChildFile ("o-microtonalsampler-restore-" +
                                              juce::String (juce::Time::currentTimeMillis()) + ".scl");
                if (tmp.replaceWithText (scl))
                {
                    engine.loadScalaFile (tmp);
                    tmp.deleteFile();
                }
            }
        }

        // Tonic last — affects the rotated cache, which depends on intervals
        // already being in place.
        if (t.hasProperty ("tonic"))
            engine.setTonicNote (static_cast<int> (t.getProperty ("tonic")));
    }
}

juce::ValueTree OMicrotonalSamplerAudioProcessor::captureStateValueTree()
{
    auto root = parameters.copyState();   // <APVTS> with all <PARAM> children

    // Strip any prior persistence siblings before re-adding — defensive
    // against repeated save-without-load cycles polluting the tree. Also
    // strip the legacy v1.5.x <SampleFolder> child since v1.6.0 always
    // emits the <SampleFolders> op-list container instead.
    for (int i = root.getNumChildren() - 1; i >= 0; --i)
    {
        auto child = root.getChild (i);
        if (child.hasType (kSampleFolderTag)
            || child.hasType (kSampleFoldersTag)
            || child.hasType (kTuningStateTag))
            root.removeChild (i, nullptr);
    }

    // v1.6.0: <SampleFolders><Op …/>…</SampleFolders> — ordered list of
    // every successful folder load since the last clearSampleMap() or
    // ReplaceAll load. Empty container when no folders are loaded.
    juce::ValueTree folders (kSampleFoldersTag);
    for (const auto& op : loadOpHistory)
    {
        juce::ValueTree opTree (kSampleFolderOpTag);
        opTree.setProperty ("path",     op.path,                            nullptr);
        opTree.setProperty ("layer",    op.targetLayer,                     nullptr);
        opTree.setProperty ("mode",     loadModeToString (op.mode),         nullptr);
        opTree.setProperty ("override", op.overrideTokens ? 1 : 0,          nullptr);
        folders.appendChild (opTree, nullptr);
    }
    root.appendChild (folders, nullptr);

    // TuningState — full engine snapshot.
    root.appendChild (captureTuningValueTree (tuningEngine), nullptr);
    return root;
}

void OMicrotonalSamplerAudioProcessor::restoreStateValueTree (const juce::ValueTree& root)
{
    if (! root.isValid() || ! root.hasType (parameters.state.getType()))
        return;

    // 1. APVTS replaceState. Non-PARAM children are preserved on the
    // returned tree but APVTS itself only walks PARAM nodes — no harm.
    parameters.replaceState (root);

    // 2. Tuning — restore synchronously. In-memory operation, fast.
    auto tuningTree = root.getChildWithName (kTuningStateTag);
    if (tuningTree.isValid())
        restoreTuningFromValueTree (tuningEngine, tuningTree);

    // 3. Sample folders — async via SampleLoader, sequenced via the replay
    //    queue so ops 0..N-1 are applied in order (preserves the user's
    //    original load sequence — Append mode order matters).
    //
    //    v1.6.0 path: <SampleFolders><Op …/>…</SampleFolders>.
    //    Legacy path: <SampleFolder path="…"/> from v1.5.x and older —
    //    treated as a single ReplaceAll op with default layer/override so
    //    behaviour is bit-for-bit identical for old saves.
    pendingMissingFolderPath.clear();
    pendingReplayOps.clear();
    loadOpHistory.clear();   // applyFolderLoad will rebuild as ops complete

    auto foldersTree = root.getChildWithName (kSampleFoldersTag);
    if (foldersTree.isValid())
    {
        for (int i = 0; i < foldersTree.getNumChildren(); ++i)
        {
            const auto opTree = foldersTree.getChild (i);
            if (! opTree.hasType (kSampleFolderOpTag)) continue;

            const auto path = opTree.getProperty ("path").toString();
            if (path.isEmpty()) continue;

            LoadOp op;
            op.path           = path;
            op.targetLayer    = juce::jlimit (0, 3, static_cast<int> (opTree.getProperty ("layer", 0)));
            op.mode           = loadModeFromString (opTree.getProperty ("mode").toString());
            op.overrideTokens = static_cast<int> (opTree.getProperty ("override", 0)) != 0;
            pendingReplayOps.push_back (std::move (op));
        }
    }
    else
    {
        auto folderTree = root.getChildWithName (kSampleFolderTag);
        if (folderTree.isValid())
        {
            const auto path = folderTree.getProperty ("path").toString();
            if (path.isNotEmpty())
            {
                LoadOp op;
                op.path           = path;
                op.targetLayer    = 0;
                op.mode           = LoadMode::ReplaceAll;
                op.overrideTokens = false;
                pendingReplayOps.push_back (std::move (op));
            }
        }
    }

    kickNextReplayOp();
}

void OMicrotonalSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto root = captureStateValueTree();
    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, destData);
}

void OMicrotonalSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
    {
        if (xmlState->hasTagName (parameters.state.getType()))
            restoreStateValueTree (juce::ValueTree::fromXml (*xmlState));
    }
}

juce::String OMicrotonalSamplerAudioProcessor::capturePresetXml()
{
    auto root = captureStateValueTree();
    if (auto xml = root.createXml())
        return xml->toString();
    return {};
}

bool OMicrotonalSamplerAudioProcessor::restorePresetXml (const juce::String& xmlText)
{
    if (xmlText.isEmpty())
        return false;
    auto xml = juce::parseXML (xmlText);
    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return false;
    restoreStateValueTree (juce::ValueTree::fromXml (*xml));
    return true;
}

//==============================================================================
// Factory function (JUCE plugin entry point)
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OMicrotonalSamplerAudioProcessor();
}
