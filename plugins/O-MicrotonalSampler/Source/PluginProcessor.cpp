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

    // ========== v1.8.0 Round-Robin Mode (1) ==========
    //
    // 0 = Cycle (sequential 0..N-1, wraps)
    // 1 = Random No-Repeat (default; uniform from {0..N-1} \ {last})
    // 2 = Random (uniform; may repeat)
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rr_mode", 1 },
        "Round-Robin Mode",
        juce::StringArray { "Cycle", "Random No-Repeat", "Random" },
        /*default*/ 1
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
    // see a valid (findCell-returns-nullptr) target before any folder is loaded.
    currentSampleMap = std::make_shared<SampleMap>();

    // v1.8.0: zero out RR counters with sentinel 0xFF (no-last). Atomic stores
    // here are safe — no voices are running yet.
    for (auto& c : rrCounters)
        c.store ((uint8_t) 0xFFu, std::memory_order_relaxed);

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
        voice->setRrCounterArray      (&rrCounters);                            // v1.8.0
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
        map->cells.reserve (108 - 21 + 1);
        map->lowestNote        = 21;
        map->highestNote       = 108;
        map->numVelocityLayers = 1;

        for (int midi = 21; midi <= 108; ++midi)
        {
            SampleVariant v;
            v.sourceSampleRate = hostSR;
            v.loopStart        = 0;
            v.loopEnd          = 0;          // one-shot in 2.1
            v.audio = std::make_shared<juce::AudioBuffer<float>> (2, numFrames);
            v.audio->clear();

            const double freq = 440.0 * std::pow (2.0, (midi - 69) / 12.0);
            const double twoPi = juce::MathConstants<double>::twoPi;
            const double phaseInc = twoPi * freq / hostSR;
            const float  amp  = 0.25f;
            const int fadeSamples = juce::jmin (numFrames / 4,
                                                (int) std::floor (0.005 * hostSR));

            float* L = v.audio->getWritePointer (0);
            float* R = v.audio->getWritePointer (1);

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
                const float val = amp * w * (float) std::sin (phase);
                L[n] = val;
                R[n] = val;
                phase += phaseInc;
                if (phase > twoPi) phase -= twoPi;
            }

            SampleCell c;
            c.midiNote      = midi;
            c.velocityLayer = 0;
            c.variants.push_back (std::move (v));
            map->cells.push_back (std::move (c));
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
//
// v1.12.0: legacy 4-arg overload forwards to the full overload with
// origin="filesystem", displayName=folder.getFileName(), embedAudio=false —
// preserves v1.11.x behaviour for callers that don't carry origin metadata
// (missing-folder relocate, legacy state restore via setStateInformation).
void OMicrotonalSamplerAudioProcessor::loadSampleFolder (const juce::File& folder,
                                                          int      targetLayer,
                                                          LoadMode mode,
                                                          bool     overrideTokens)
{
    loadSampleFolder (folder, targetLayer, mode, overrideTokens,
                      "filesystem", folder.getFileName(), false);
}

void OMicrotonalSamplerAudioProcessor::loadSampleFolder (const juce::File& folder,
                                                          int      targetLayer,
                                                          LoadMode mode,
                                                          bool     overrideTokens,
                                                          const juce::String& origin,
                                                          const juce::String& displayName,
                                                          bool     embedAudio)
{
    if (sampleLoader == nullptr)
        return;

    const int  clampedLayer = juce::jlimit (0, 3, targetLayer);

    LoadOp op;
    op.path           = folder.getFullPathName();
    op.targetLayer    = clampedLayer;
    op.mode           = mode;
    op.overrideTokens = overrideTokens;
    op.origin         = origin.isNotEmpty() ? origin : juce::String ("filesystem");
    op.displayName    = displayName.isNotEmpty() ? displayName : folder.getFileName();
    op.embedAudio     = embedAudio;

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
        [this, op](std::shared_ptr<SampleMap> newMap,
                   juce::StringArray skipped,
                   std::vector<SampleLoader::AmbiguousDuplicate> ambig)
        {
            // v1.8.0: ambiguous duplicates → stage the map and surface the
            // confirmation modal instead of applying immediately.
            if (! ambig.empty())
            {
                pendingDuplicateMap            = std::move (newMap);
                pendingDuplicateSkippedFiles   = std::move (skipped);
                pendingAmbiguousDuplicates     = std::move (ambig);
                pendingDuplicateOp             = op;
                pendingDuplicateChainContinuation = nullptr;
                if (ambiguousDuplicateCallback)
                    ambiguousDuplicateCallback (pendingAmbiguousDuplicates);
                return;
            }
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
        merged->cells = newSlotsMap->cells;
        loadOpHistory.clear();
        // v1.8.0: ReplaceAll resets every RR counter to the sentinel.
        for (auto& c : rrCounters)
            c.store ((uint8_t) 0xFFu, std::memory_order_relaxed);
    }
    else
    {
        merged->cells = prev->cells;

        if (op.mode == LoadMode::ReplaceLayer)
        {
            const int target = juce::jlimit (0, 3, op.targetLayer);
            merged->cells.erase (
                std::remove_if (merged->cells.begin(), merged->cells.end(),
                    [target] (const SampleCell& c)
                    {
                        return c.velocityLayer == target;
                    }),
                merged->cells.end());
            // v1.8.0: drop counters for the wiped layer.
            for (int midi = 0; midi < 128; ++midi)
                rrCounters[(size_t) (midi * 4 + target)].store (
                    (uint8_t) 0xFFu, std::memory_order_relaxed);
        }

        // Per-cell collision behaviour depends on mode:
        //   Append / ReplaceLayer — new cell replaces the old one entirely.
        //   MergeRR (v1.9.0)       — concatenate new variants onto the existing
        //                            cell's variants vector (capped at
        //                            kMaxVariantsPerCell). Cells with no
        //                            existing match are added as today.
        // Counter for every touched cell is reset so the first note-on doesn't
        // pick a now-out-of-range index.
        constexpr int kMaxVariantsPerCell = 64;

        for (const auto& newCell : newSlotsMap->cells)
        {
            const int counterIdx = juce::jlimit (0, 511,
                newCell.midiNote * 4 + newCell.velocityLayer);

            if (op.mode == LoadMode::MergeRR)
            {
                applyMergeRrCell (merged->cells, newCell,
                                  kMaxVariantsPerCell, lastSkippedFiles);
            }
            else
            {
                merged->cells.erase (
                    std::remove_if (merged->cells.begin(), merged->cells.end(),
                        [&newCell] (const SampleCell& c)
                        {
                            return c.midiNote      == newCell.midiNote
                                && c.velocityLayer == newCell.velocityLayer;
                        }),
                    merged->cells.end());
                merged->cells.push_back (newCell);
            }

            rrCounters[(size_t) counterIdx].store ((uint8_t) 0xFFu,
                                                    std::memory_order_relaxed);
        }
    }

    merged->lowestNote        = 127;
    merged->highestNote       = 0;
    int maxLayer              = 0;
    for (const auto& c : merged->cells)
    {
        merged->lowestNote  = juce::jmin (merged->lowestNote,  c.midiNote);
        merged->highestNote = juce::jmax (merged->highestNote, c.midiNote);
        maxLayer            = juce::jmax (maxLayer,            c.velocityLayer);
    }
    if (merged->cells.empty())
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

    // v1.12.0: when embedAudio is on, snapshot the slots produced by THIS op
    // so captureStateValueTree can serialise per-variant audio later. The
    // shared_ptr<AudioBuffer>s inside each SampleVariant are also held by
    // currentSampleMap, so memory cost is metadata only — we are NOT
    // duplicating the audio data, just keeping a per-op slot manifest alive.
    LoadOp persistedOp = op;
    if (persistedOp.embedAudio && newSlotsMap != nullptr)
        persistedOp.embeddedSlots = newSlotsMap;

    loadOpHistory.push_back (std::move (persistedOp));

    DBG ("loadSampleFolder applied: mode=" << (int) op.mode
         << " layer=" << op.targetLayer
         << " override=" << (int) op.overrideTokens
         << " origin=" << op.origin
         << " name=" << op.displayName
         << " embed=" << (int) op.embedAudio
         << " merged cells=" << (int) merged->cells.size()
         << " (v" << merged->version << ")");

    if (sampleMapChangedCallback)
        sampleMapChangedCallback();
}

//==============================================================================
// v1.8.0: confirm or reject a staged folder load with ambiguous duplicates.
// accept=true → apply the staged map (treats duplicates as variants).
// accept=false → discard staged map; no map change.
void OMicrotonalSamplerAudioProcessor::confirmRoundRobinLoad (bool accept)
{
    if (pendingDuplicateMap == nullptr)
        return;

    auto map      = std::move (pendingDuplicateMap);
    auto skipped  = std::move (pendingDuplicateSkippedFiles);
    auto op       = pendingDuplicateOp;
    auto chain    = std::move (pendingDuplicateChainContinuation);

    pendingDuplicateMap.reset();
    pendingDuplicateSkippedFiles.clear();
    pendingAmbiguousDuplicates.clear();
    pendingDuplicateChainContinuation = nullptr;

    if (accept)
    {
        applyFolderLoad (std::move (map), skipped, op);
    }
    else
    {
        DBG ("confirmRoundRobinLoad: user declined; discarding staged map");
        // Surface the skipped files anyway so the UI can hint about what was
        // dropped.
        lastSkippedFiles = skipped;
        if (sampleMapChangedCallback)
            sampleMapChangedCallback();
    }

    // Continue any chained replay sequence (state-restore path).
    if (chain) chain();
}

//==============================================================================
// v1.6.0: replay-queue dispatcher. Pops the next op, skips it if its folder
// is gone (first missing path raises the existing missing-folder modal), and
// dispatches an async load whose completion chains back here. Stack-safe via
// JUCE's MessageManager::callAsync — each chained call is a fresh message.
//
// v1.12.0: extends to three op kinds:
//   1. Embedded op (op.embeddedSlots != nullptr) — apply slots directly,
//      no SampleLoader call. Synchronous merge into currentSampleMap;
//      chain to next op immediately.
//   2. Drag-drop op (op.origin == "drag-drop") without embed — the temp dir
//      is gone and never user-meaningful. Surface the missing-folder modal
//      with kind="drag-drop" so JS can render the "re-drag or browse"
//      copy. Skip the load and chain.
//   3. Filesystem op without embed — existing path: walk on disk via
//      SampleLoader. Surface missing-folder modal with kind="filesystem"
//      if the path is gone.
void OMicrotonalSamplerAudioProcessor::kickNextReplayOp()
{
    while (! pendingReplayOps.empty())
    {
        const LoadOp op = pendingReplayOps.front();
        pendingReplayOps.erase (pendingReplayOps.begin());

        // ---- Case 1: embedded audio — synchronous merge, no loader call ----
        if (op.embeddedSlots != nullptr)
        {
            // applyFolderLoad will re-attach embeddedSlots to the persisted
            // op via its own embedAudio branch (so the next save round-trips
            // the embed). Pass empty skipped — the embed source is in-memory,
            // there is nothing for the loader to skip.
            applyFolderLoad (op.embeddedSlots, juce::StringArray{}, op);
            continue;   // chain to next op immediately (no async wait)
        }

        // ---- Case 2: drag-drop without embed — surface drag-drop missing modal ----
        if (op.origin == "drag-drop")
        {
            if (pendingMissingFolderPath.isEmpty()
                && pendingMissingFolderName.isEmpty())
            {
                pendingMissingFolderPath = juce::String();
                pendingMissingFolderKind = "drag-drop";
                pendingMissingFolderName = op.displayName;
                if (missingFolderCallback)
                    missingFolderCallback (juce::String{}, "drag-drop", op.displayName);
            }
            continue;   // try the next op
        }

        // ---- Case 3: filesystem without embed — existing path ----
        const juce::File f (op.path);
        if (! f.isDirectory())
        {
            // Surface the FIRST missing folder; subsequent ones are silently
            // skipped to keep the user's modal interaction simple. They can
            // re-locate or load fresh after dismissing.
            if (pendingMissingFolderPath.isEmpty()
                && pendingMissingFolderName.isEmpty())
            {
                pendingMissingFolderPath = op.path;
                pendingMissingFolderKind = "filesystem";
                pendingMissingFolderName = op.displayName.isNotEmpty()
                                              ? op.displayName
                                              : f.getFileName();
                if (missingFolderCallback)
                    missingFolderCallback (op.path, "filesystem", pendingMissingFolderName);
            }
            continue;   // try the next op
        }

        sampleLoader->loadFolder (
            f,
            getSampleRate() > 0.0 ? getSampleRate() : 48000.0,
            LoadOptions { op.targetLayer, op.overrideTokens },
            [this, op] (std::shared_ptr<SampleMap> newMap,
                        juce::StringArray skipped,
                        std::vector<SampleLoader::AmbiguousDuplicate> ambig)
            {
                if (! ambig.empty())
                {
                    // v1.8.0: stage the map and surface the modal; chain the
                    // next replay op once the user confirms/rejects.
                    pendingDuplicateMap            = std::move (newMap);
                    pendingDuplicateSkippedFiles   = std::move (skipped);
                    pendingAmbiguousDuplicates     = std::move (ambig);
                    pendingDuplicateOp             = op;
                    pendingDuplicateChainContinuation = [this]() { kickNextReplayOp(); };
                    if (ambiguousDuplicateCallback)
                        ambiguousDuplicateCallback (pendingAmbiguousDuplicates);
                    return;
                }
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
                                                          const juce::File& file,
                                                          bool mergeAsRr)
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
    sampleLoader->loadSingleVariant (
        file,
        midiPitch,
        velocityLayer,
        sr,
        [this, mergeAsRr] (int targetMidi, int targetVel,
                           SampleVariant newVariant, juce::String skipReason)
        {
            if (newVariant.audio == nullptr)
            {
                if (skipReason.isNotEmpty())
                {
                    lastSkippedFiles.add (skipReason);
                    DBG ("loadSingleSample failed: " << skipReason);
                }
                if (sampleMapChangedCallback)
                    sampleMapChangedCallback();
                return;
            }

           #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
            auto currentMap = std::atomic_load (&currentSampleMap);
           #else
            auto currentMap = currentSampleMap;
           #endif

            constexpr int kMaxVariantsPerCell = 64;

            // v1.9.0: detect collision with an existing cell. mergeAsRr=true
            // appends to the cell's variants; mergeAsRr=false (default) keeps
            // v1.8.0 semantics — replace the cell with a fresh single-variant.
            const SampleCell* existingCell = nullptr;
            if (currentMap != nullptr)
            {
                for (const auto& c : currentMap->cells)
                    if (c.midiNote == targetMidi && c.velocityLayer == targetVel)
                    {
                        existingCell = &c;
                        break;
                    }
            }

            const bool doMerge = mergeAsRr && existingCell != nullptr
                && (int) existingCell->variants.size() < kMaxVariantsPerCell;

            if (mergeAsRr && existingCell != nullptr
                && (int) existingCell->variants.size() >= kMaxVariantsPerCell)
            {
                lastSkippedFiles.add ("variant cap reached: " + newVariant.filename);
                DBG ("loadSingleSample: variant cap (" << kMaxVariantsPerCell
                     << ") reached at midi=" << targetMidi << " vel=" << targetVel
                     << " — skipping " << newVariant.filename);
                if (sampleMapChangedCallback)
                    sampleMapChangedCallback();
                return;
            }

            // Build a fresh map: copy all cells (modifying the matching one
            // in place when merging) and push the new variant.
            auto next = std::make_shared<SampleMap>();
            bool placed = false;
            if (currentMap != nullptr)
            {
                next->cells.reserve (currentMap->cells.size() + 1);
                for (const auto& c : currentMap->cells)
                {
                    if (c.midiNote == targetMidi && c.velocityLayer == targetVel)
                    {
                        if (doMerge)
                        {
                            SampleCell mergedCell = c;
                            mergedCell.variants.push_back (newVariant);
                            next->cells.push_back (std::move (mergedCell));
                            placed = true;
                        }
                        // mergeAsRr=false → drop the old cell (replace path).
                        continue;
                    }
                    next->cells.push_back (c);
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

            if (! placed)
            {
                SampleCell freshCell;
                freshCell.midiNote      = targetMidi;
                freshCell.velocityLayer = targetVel;
                freshCell.variants.push_back (std::move (newVariant));
                next->cells.push_back (std::move (freshCell));
            }

            next->lowestNote  = juce::jmin (next->lowestNote,  targetMidi);
            next->highestNote = juce::jmax (next->highestNote, targetMidi);
            next->numVelocityLayers = juce::jlimit (1, 4,
                juce::jmax (next->numVelocityLayers, targetVel + 1));

            if (next->cells.size() == 1)
            {
                next->lowestNote  = targetMidi;
                next->highestNote = targetMidi;
            }

            next->version = (currentMap != nullptr ? currentMap->version : 0) + 1;

           #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
            std::atomic_store (&currentSampleMap, next);
           #else
            currentSampleMap = next;
           #endif

            // v1.8.0: per-cell replace resets the RR counter — the old cell
            // may have had multiple variants; the new cell has exactly one.
            // v1.9.0: merge path also resets so the next note-on doesn't
            // index past the just-grown variants vector with a stale value.
            const int counterIdx = juce::jlimit (0, 511, targetMidi * 4 + targetVel);
            rrCounters[(size_t) counterIdx].store ((uint8_t) 0xFFu,
                                                    std::memory_order_relaxed);

            DBG ("loadSingleSample success: midi=" << targetMidi
                 << " vel=" << targetVel
                 << " mode=" << (doMerge ? "merge_rr" : "replace")
                 << " cells=" << (int) next->cells.size()
                 << " v" << next->version);

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
                                                            bool resetToAutoDetect,
                                                            int variantIndex)
{
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

    const auto* foundCell = current->findCell (midiPitch, velocityLayer);
    if (foundCell == nullptr || foundCell->variants.empty())
    {
        DBG ("overrideLoopPoints: cell absent (midi=" << midiPitch
             << " vel=" << velocityLayer << ")");
        return;
    }

    auto next = std::make_shared<SampleMap> (*current);

    SampleCell* targetCell = nullptr;
    for (auto& c : next->cells)
    {
        if (c.midiNote == midiPitch && c.velocityLayer == velocityLayer)
        {
            targetCell = &c;
            break;
        }
    }

    if (targetCell == nullptr || targetCell->variants.empty())
    {
        DBG ("overrideLoopPoints: target cell vanished after deep copy");
        return;
    }

    // v1.8.0: variantIndex selects which variant to mutate. -1 = primary (0).
    const int vIdx = (variantIndex < 0) ? 0
                                        : juce::jlimit (0, (int) targetCell->variants.size() - 1,
                                                        variantIndex);
    SampleVariant& targetVariant = targetCell->variants[(size_t) vIdx];

    if (resetToAutoDetect)
    {
        const int numSamples = (targetVariant.audio != nullptr)
                                   ? targetVariant.audio->getNumSamples()
                                   : 0;

        if (numSamples >= 18)
        {
            targetVariant.loopStart = 0;
            targetVariant.loopEnd   = numSamples - 2;
            targetVariant.loopMode  = LoopMode::Auto;
            DBG ("resetLoopToAutoDetect: midi=" << midiPitch
                 << " vel=" << velocityLayer
                 << " variant=" << vIdx
                 << " whole-file loop=[0, " << (numSamples - 2) << "]");
        }
        else
        {
            targetVariant.loopStart = 0;
            targetVariant.loopEnd   = 0;
            targetVariant.loopMode  = LoopMode::OneShot;
            DBG ("resetLoopToAutoDetect: midi=" << midiPitch
                 << " vel=" << velocityLayer
                 << " variant=" << vIdx
                 << " buffer too short → one-shot");
        }
    }
    else
    {
        const int numSamples = (targetVariant.audio != nullptr)
                                   ? targetVariant.audio->getNumSamples()
                                   : 0;
        const int clampedStart = juce::jlimit (0, juce::jmax (0, numSamples - 1), loopStart);
        const int clampedEnd   = juce::jlimit (clampedStart + 1,
                                               juce::jmax (clampedStart + 1, numSamples),
                                               loopEnd);

        targetVariant.loopStart = clampedStart;
        targetVariant.loopEnd   = clampedEnd;
        targetVariant.loopMode  = LoopMode::Manual;

        DBG ("overrideLoopPoints: midi=" << midiPitch
             << " vel=" << velocityLayer
             << " variant=" << vIdx
             << " manual loop=[" << clampedStart << ", " << clampedEnd << "]"
             << " xfade=" << crossfadeLen << " (recorded for v1.1; ignored in v1.0)");
        juce::ignoreUnused (crossfadeLen);
    }

    next->version = current->version + 1;

   #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    std::atomic_store (&currentSampleMap, next);
   #else
    currentSampleMap = next;
   #endif

    if (sampleMapChangedCallback)
        sampleMapChangedCallback();
}

void OMicrotonalSamplerAudioProcessor::resetLoopToAutoDetect (int midiPitch,
                                                               int velocityLayer,
                                                               int variantIndex)
{
    overrideLoopPoints (midiPitch, velocityLayer, 0, 0, 0,
                        /*resetToAutoDetect*/ true,
                        variantIndex);
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

    // v1.8.0: clear all RR counters.
    for (auto& cnt : rrCounters)
        cnt.store ((uint8_t) 0xFFu, std::memory_order_relaxed);

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
             << "\"numVelocityLayers\":1,\"cells\":[],\"slots\":[],\"skippedFiles\":[]}";
        return json;
    }

    json << "\"version\":"           << map->version
         << ",\"lowestNote\":"       << map->lowestNote
         << ",\"highestNote\":"      << map->highestNote
         << ",\"numVelocityLayers\":" << map->numVelocityLayers
         << ",\"cells\":[";

    bool firstCell = true;
    for (const auto& c : map->cells)
    {
        if (! firstCell) json << ",";
        firstCell = false;

        json << "{"
             << "\"midiNote\":"      << c.midiNote
             << ",\"velocityLayer\":" << c.velocityLayer
             << ",\"variants\":[";

        bool firstVar = true;
        for (const auto& v : c.variants)
        {
            if (! firstVar) json << ",";
            firstVar = false;

            const int lengthSamples = (v.audio != nullptr) ? v.audio->getNumSamples() : 0;
            json << "{"
                 << "\"filename\":"         << juce::JSON::toString (juce::var (v.filename))
                 << ",\"lengthSamples\":"    << lengthSamples
                 << ",\"sourceSampleRate\":" << juce::String (v.sourceSampleRate, 4)
                 << ",\"loopStart\":"        << v.loopStart
                 << ",\"loopEnd\":"          << v.loopEnd
                 << ",\"loopMode\":\""       << loopModeToString (v.loopMode) << "\""
                 << "}";
        }
        json << "]}";
    }

    // Back-compat: also emit a flat `slots` array of primary-variant entries
    // for any v1.7.x consumer that hasn't been variant-aware updated. Mirrors
    // the old schema exactly (one entry per cell, variants[0] = primary).
    json << "],\"slots\":[";

    bool firstSlot = true;
    for (const auto& c : map->cells)
    {
        if (c.variants.empty()) continue;
        const auto& v = c.primary();

        if (! firstSlot) json << ",";
        firstSlot = false;

        const int lengthSamples = (v.audio != nullptr) ? v.audio->getNumSamples() : 0;

        json << "{"
             << "\"midiNote\":"          << c.midiNote
             << ",\"velocityLayer\":"    << c.velocityLayer
             << ",\"filename\":"         << juce::JSON::toString (juce::var (v.filename))
             << ",\"lengthSamples\":"    << lengthSamples
             << ",\"sourceSampleRate\":" << juce::String (v.sourceSampleRate, 4)
             << ",\"loopStart\":"        << v.loopStart
             << ",\"loopEnd\":"          << v.loopEnd
             << ",\"loopMode\":\""       << loopModeToString (v.loopMode) << "\""
             << ",\"variantCount\":"     << (int) c.variants.size()
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
                                                                      int targetBins,
                                                                      int variantIndex) const
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

    const auto* cell = map->findCell (midiPitch, velocityLayer);
    if (cell == nullptr || cell->variants.empty())
        return "{}";

    const int vIdx = juce::jlimit (0, (int) cell->variants.size() - 1,
                                   variantIndex >= 0 ? variantIndex : 0);
    const auto& variant = cell->variants[(size_t) vIdx];
    if (variant.audio == nullptr || variant.audio->getNumSamples() == 0)
        return "{}";

    const int numFrames   = variant.audio->getNumSamples();
    const int numChannels = juce::jmax (1, variant.audio->getNumChannels());
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
                sum += variant.audio->getReadPointer (ch)[n];
            const float val = sum / static_cast<float> (numChannels);
            if (val < minV) minV = val;
            if (val > maxV) maxV = val;
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
    obj->setProperty ("midiNote",         cell->midiNote);
    obj->setProperty ("velocityLayer",    cell->velocityLayer);
    obj->setProperty ("variantIndex",     vIdx);
    obj->setProperty ("variantCount",     (int) cell->variants.size());
    obj->setProperty ("lengthSamples",    numFrames);
    obj->setProperty ("sourceSampleRate", variant.sourceSampleRate);
    obj->setProperty ("loopStart",        variant.loopStart);
    obj->setProperty ("loopEnd",          variant.loopEnd);
    obj->setProperty ("loopMode",         juce::String (loopModeToString (variant.loopMode)));
    obj->setProperty ("filename",         variant.filename);
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

    // v1.12.0: <Audio><Cell><Variant/></Cell></Audio> sub-tree under each <Op>
    // when the op was loaded with embedAudio=true. Stores per-variant audio
    // inline as base64-encoded WAV (24-bit PCM) so projects survive folder
    // moves, cross-machine transfer, and drag-drop temp-dir cleanup.
    constexpr const char* kEmbeddedAudioTag    = "Audio";
    constexpr const char* kEmbeddedCellTag     = "Cell";
    constexpr const char* kEmbeddedVariantTag  = "Variant";

    // v1.6.0: human-readable mode strings in XML so saved presets are
    // diffable / editable by hand. Unknown values fall back to ReplaceAll.
    juce::String loadModeToString (LoadMode m) noexcept
    {
        switch (m)
        {
            case LoadMode::Append:       return "append";
            case LoadMode::ReplaceLayer: return "replace_layer";
            case LoadMode::ReplaceAll:   return "replace_all";
            case LoadMode::MergeRR:      return "merge_rr";
        }
        return "replace_all";
    }

    LoadMode loadModeFromString (const juce::String& s) noexcept
    {
        if (s == "append")        return LoadMode::Append;
        if (s == "replace_layer") return LoadMode::ReplaceLayer;
        if (s == "merge_rr")      return LoadMode::MergeRR;
        return LoadMode::ReplaceAll;
    }

    juce::String loopModeToString (LoopMode m) noexcept
    {
        switch (m)
        {
            case LoopMode::OneShot: return "one_shot";
            case LoopMode::Auto:    return "auto";
            case LoopMode::Manual:  return "manual";
        }
        return "auto";
    }

    LoopMode loopModeFromString (const juce::String& s) noexcept
    {
        if (s == "one_shot") return LoopMode::OneShot;
        if (s == "manual")   return LoopMode::Manual;
        return LoopMode::Auto;
    }

    //--------------------------------------------------------------------------
    // v1.12.0 — per-variant audio (de)serialisation for inline state embed.
    //
    // 24-bit PCM WAV (good quality / standard sample-library bit depth) →
    // base64 string. The user has explicitly opted in to embed via the
    // folder-load options modal, so the size cost is informed.
    //
    // Round-trip lossiness: 24-bit PCM has a -141 dB noise floor (well below
    // hearing threshold for any reasonable signal level). Float samples
    // outside [-1, +1) will clip on encode — same constraint as any 24-bit
    // export pipeline. The plugin's own SampleLoader path produces
    // post-resample float data that is bit-equivalent to its source, so
    // typical sample-library content stays well within range.
    //
    // Sample rate: stored at the buffer's current rate (which is the host
    // sample rate at load time, since SampleLoader resamples to host SR).
    // Restoring at a different host SR will play at a slightly different
    // rate via the voice's existing repitch path — same outcome as today's
    // resample-on-load behaviour, just without the source file available.

    juce::String encodeVariantAsBase64Wav (const SampleVariant& v)
    {
        if (v.audio == nullptr) return {};
        const int numChannels = v.audio->getNumChannels();
        const int numSamples  = v.audio->getNumSamples();
        if (numChannels <= 0 || numSamples <= 0) return {};

        juce::MemoryBlock memBlock;
        {
            // JUCE 8 createWriterFor takes a unique_ptr<OutputStream>& and
            // transfers ownership on success. memBlock is referenced by the
            // stream and outlives the writer (writer destructs at scope end →
            // stream destructs → memBlock retains the written bytes).
            std::unique_ptr<juce::OutputStream> stream
                = std::make_unique<juce::MemoryOutputStream> (memBlock, false);

            juce::WavAudioFormat wav;
            const auto opts = juce::AudioFormatWriterOptions{}
                .withSampleRate    (v.sourceSampleRate > 0.0 ? v.sourceSampleRate : 48000.0)
                .withNumChannels   (numChannels)
                .withBitsPerSample (24);

            auto writer = wav.createWriterFor (stream, opts);
            if (writer == nullptr)
                return {};

            writer->writeFromAudioSampleBuffer (*v.audio, 0, numSamples);
        } // writer destructs → flushes WAV chunks into memBlock

        return juce::Base64::toBase64 (memBlock.getData(), memBlock.getSize());
    }

    bool decodeVariantFromBase64Wav (const juce::String& base64, SampleVariant& outVariant)
    {
        if (base64.isEmpty()) return false;

        juce::MemoryBlock raw;
        {
            juce::MemoryOutputStream mos (raw, false);
            if (! juce::Base64::convertFromBase64 (mos, base64))
                return false;
        }

        if (raw.getSize() == 0) return false;

        juce::WavAudioFormat wav;
        // Reader takes ownership of the InputStream (deleteStreamWhenDestroyed=true).
        // MemoryInputStream(MemoryBlock&, bool keepInternalCopy) — true so the
        // stream owns its own copy and survives `raw` going out of scope (it
        // doesn't here, but guards against future refactors).
        std::unique_ptr<juce::AudioFormatReader> reader (
            wav.createReaderFor (new juce::MemoryInputStream (raw, /*keepInternalCopy=*/true),
                                 /*deleteStreamWhenDestroyed=*/true));
        if (reader == nullptr) return false;

        const int numChannels = static_cast<int> (reader->numChannels);
        const int numSamples  = static_cast<int> (reader->lengthInSamples);
        if (numChannels <= 0 || numSamples <= 0) return false;

        outVariant.audio = std::make_shared<juce::AudioBuffer<float>> (numChannels, numSamples);
        outVariant.audio->clear();
        if (! reader->read (outVariant.audio.get(), 0, numSamples, 0, true, true))
            return false;

        outVariant.sourceSampleRate = reader->sampleRate;
        return true;
    }

    juce::ValueTree buildEmbeddedAudioTree (const SampleMap& slots)
    {
        juce::ValueTree audioTree (kEmbeddedAudioTag);
        for (const auto& cell : slots.cells)
        {
            juce::ValueTree cellTree (kEmbeddedCellTag);
            cellTree.setProperty ("midi",  cell.midiNote,      nullptr);
            cellTree.setProperty ("layer", cell.velocityLayer, nullptr);

            for (const auto& v : cell.variants)
            {
                juce::ValueTree vTree (kEmbeddedVariantTag);
                vTree.setProperty ("filename", v.filename, nullptr);
                vTree.setProperty ("loopMode", loopModeToString (v.loopMode), nullptr);
                vTree.setProperty ("loopStart", v.loopStart, nullptr);
                vTree.setProperty ("loopEnd",   v.loopEnd,   nullptr);

                const auto b64 = encodeVariantAsBase64Wav (v);
                if (b64.isNotEmpty())
                    vTree.setProperty ("wav", b64, nullptr);

                cellTree.appendChild (vTree, nullptr);
            }

            audioTree.appendChild (cellTree, nullptr);
        }
        return audioTree;
    }

    // Returns nullptr when the tree is invalid OR no variants decoded
    // successfully. Caller falls back to the regular filesystem replay path
    // when this happens (best-effort recovery).
    std::shared_ptr<SampleMap> decodeEmbeddedAudioTree (const juce::ValueTree& audioTree)
    {
        if (! audioTree.isValid() || ! audioTree.hasType (kEmbeddedAudioTag))
            return nullptr;

        auto slots = std::make_shared<SampleMap>();
        slots->cells.reserve (static_cast<size_t> (audioTree.getNumChildren()));

        int maxLayer = 0;
        for (int ci = 0; ci < audioTree.getNumChildren(); ++ci)
        {
            const auto cellTree = audioTree.getChild (ci);
            if (! cellTree.hasType (kEmbeddedCellTag)) continue;

            SampleCell cell;
            cell.midiNote      = juce::jlimit (0, 127, static_cast<int> (cellTree.getProperty ("midi", 0)));
            cell.velocityLayer = juce::jlimit (0, 3,   static_cast<int> (cellTree.getProperty ("layer", 0)));

            for (int vi = 0; vi < cellTree.getNumChildren(); ++vi)
            {
                const auto vTree = cellTree.getChild (vi);
                if (! vTree.hasType (kEmbeddedVariantTag)) continue;
                if (! vTree.hasProperty ("wav"))           continue;

                SampleVariant variant;
                variant.filename  = vTree.getProperty ("filename").toString();
                variant.loopMode  = loopModeFromString (vTree.getProperty ("loopMode").toString());
                variant.loopStart = static_cast<int> (vTree.getProperty ("loopStart", 0));
                variant.loopEnd   = static_cast<int> (vTree.getProperty ("loopEnd",   0));

                if (! decodeVariantFromBase64Wav (vTree.getProperty ("wav").toString(), variant))
                {
                    DBG ("decodeEmbeddedAudioTree: variant decode failed: " << variant.filename);
                    continue;
                }

                cell.variants.push_back (std::move (variant));
            }

            if (! cell.variants.empty())
            {
                slots->lowestNote  = juce::jmin (slots->lowestNote,  cell.midiNote);
                slots->highestNote = juce::jmax (slots->highestNote, cell.midiNote);
                maxLayer           = juce::jmax (maxLayer,           cell.velocityLayer);
                slots->cells.push_back (std::move (cell));
            }
        }

        if (slots->cells.empty()) return nullptr;
        slots->numVelocityLayers = juce::jlimit (1, 4, maxLayer + 1);
        return slots;
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
    //
    // v1.12.0 additive attrs (all optional — old states omit them and decode
    // identically as filesystem ops with no embed):
    //   kind   : "filesystem" (default) | "drag-drop"
    //   name   : human-friendly folder name for the missing-folder modal
    //   embed  : "1" when an inline <Audio> blob child is present
    //
    // For drag-drop ops we DROP the temp path on save (it's not stable
    // across sessions and re-using it would defeat the new modal copy). The
    // path attr is omitted; restoreStateValueTree reconstructs an op with an
    // empty path + drag-drop kind.
    juce::ValueTree folders (kSampleFoldersTag);
    for (const auto& op : loadOpHistory)
    {
        juce::ValueTree opTree (kSampleFolderOpTag);

        // Don't persist the drag-drop temp path — it's session-scoped and
        // reaped at the next drop session start. Without embed, the op
        // restores as "drag-drop, missing"; with embed, the audio blob is
        // self-sufficient and the path is irrelevant.
        if (op.origin != "drag-drop")
            opTree.setProperty ("path", op.path, nullptr);

        opTree.setProperty ("layer",    op.targetLayer,                     nullptr);
        opTree.setProperty ("mode",     loadModeToString (op.mode),         nullptr);
        opTree.setProperty ("override", op.overrideTokens ? 1 : 0,          nullptr);

        // v1.12.0:
        opTree.setProperty ("kind",  op.origin.isNotEmpty() ? op.origin : juce::String ("filesystem"), nullptr);
        if (op.displayName.isNotEmpty())
            opTree.setProperty ("name", op.displayName, nullptr);
        if (op.embedAudio)
            opTree.setProperty ("embed", 1, nullptr);

        // Inline audio blob — only emitted when the user opted in via embed
        // AND the per-op slot snapshot was attached at load time.
        if (op.embedAudio && op.embeddedSlots != nullptr)
            opTree.appendChild (buildEmbeddedAudioTree (*op.embeddedSlots), nullptr);

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
    pendingMissingFolderKind.clear();
    pendingMissingFolderName.clear();
    pendingReplayOps.clear();
    loadOpHistory.clear();   // applyFolderLoad will rebuild as ops complete

    auto foldersTree = root.getChildWithName (kSampleFoldersTag);
    if (foldersTree.isValid())
    {
        for (int i = 0; i < foldersTree.getNumChildren(); ++i)
        {
            const auto opTree = foldersTree.getChild (i);
            if (! opTree.hasType (kSampleFolderOpTag)) continue;

            // v1.12.0: kind defaults to "filesystem" for old states. drag-drop
            // ops can have empty path (we don't persist their temp dir).
            const juce::String kind = opTree.hasProperty ("kind")
                ? opTree.getProperty ("kind").toString()
                : juce::String ("filesystem");
            const auto path = opTree.getProperty ("path").toString();
            const bool embed = static_cast<int> (opTree.getProperty ("embed", 0)) != 0;

            // Skip ops that have no usable rehydrate path:
            //   - filesystem with empty path AND no embed → nothing to do
            //   - drag-drop without embed AND no displayName → no modal copy possible
            // (A drag-drop op with displayName but no embed is a useful signal
            //  for the missing-folder modal — keep it.)
            if (kind == "filesystem" && path.isEmpty() && ! embed)
                continue;

            LoadOp op;
            op.path           = path;
            op.targetLayer    = juce::jlimit (0, 3, static_cast<int> (opTree.getProperty ("layer", 0)));
            op.mode           = loadModeFromString (opTree.getProperty ("mode").toString());
            op.overrideTokens = static_cast<int> (opTree.getProperty ("override", 0)) != 0;
            op.origin         = kind;
            op.displayName    = opTree.getProperty ("name").toString();
            op.embedAudio     = embed;

            // Inline audio blob — when present, deserialise the slots directly
            // so kickNextReplayOp can apply them synchronously (no SampleLoader
            // call). On decode failure, fall back to the regular replay path
            // (filesystem walk if path is set; missing modal otherwise).
            if (embed)
            {
                const auto audioTree = opTree.getChildWithName (kEmbeddedAudioTag);
                if (audioTree.isValid())
                {
                    auto slots = decodeEmbeddedAudioTree (audioTree);
                    if (slots != nullptr)
                        op.embeddedSlots = std::move (slots);
                    else
                        DBG ("restoreStateValueTree: embedded decode failed, falling back");
                }
            }

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
                op.origin         = "filesystem";
                op.displayName    = juce::File (path).getFileName();
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
