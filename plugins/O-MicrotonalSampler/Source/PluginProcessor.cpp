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
#include "LoopDetector.h"

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
//      Reset path: run LoopDetector::detectLoop on slot->audio; on valid →
//        loopStart/loopEnd from detector + Auto; on invalid → loopStart=loopEnd=0
//        + OneShot.
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
        // Run the auto-detector on the slot's audio buffer. On valid → Auto;
        // on invalid → OneShot (loop disabled).
        if (targetSlot->audio == nullptr || targetSlot->audio->getNumSamples() == 0)
        {
            DBG ("resetLoopToAutoDetect: empty audio buffer (midi=" << midiPitch
                 << " vel=" << velocityLayer << ")");
            targetSlot->loopStart = 0;
            targetSlot->loopEnd   = 0;
            targetSlot->loopMode  = LoopMode::OneShot;
        }
        else
        {
            const auto region = LoopDetector::detectLoop (
                *targetSlot->audio, targetSlot->sourceSampleRate);

            if (region.valid)
            {
                targetSlot->loopStart = region.loopStart;
                targetSlot->loopEnd   = region.loopEnd;
                targetSlot->loopMode  = LoopMode::Auto;
                DBG ("resetLoopToAutoDetect: midi=" << midiPitch
                     << " vel=" << velocityLayer
                     << " auto loop=[" << region.loopStart << ", " << region.loopEnd << "]");
            }
            else
            {
                targetSlot->loopStart = 0;
                targetSlot->loopEnd   = 0;
                targetSlot->loopMode  = LoopMode::OneShot;
                DBG ("resetLoopToAutoDetect: midi=" << midiPitch
                     << " vel=" << velocityLayer
                     << " auto-detect invalid → one-shot");
            }
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
