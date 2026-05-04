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

    // ========== v1.14.0 Playing Techniques (5) ==========
    //
    // Adds the technique axis. Back-compat: default state (technique_count=1,
    // ks_enabled=false) reproduces v1.13.0 behaviour exactly — every cell
    // lives at technique=0 and the synth never reads anything past slot 0.
    //
    // technique_count   1..8 — number of active technique slots
    // technique_select  0..7 — current technique slot (mirrors keyswitch /
    //                          CC / PC routing). Audio thread loads this
    //                          atom at startNote.
    // ks_enabled        bool — true = scan note-ons in [ks_low..ks_high] on
    //                          processBlock and absorb them as keyswitches
    // ks_low_note       0..127 — KS range low edge (default C-2 / MIDI 0)
    // ks_high_note      0..127 — KS range high edge (default MIDI 9 →
    //                          covers all 8 default technique slots, one
    //                          MIDI semitone each)
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "technique_count", 1 },
        "Technique Count",
        1, 8, 1
    ));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "technique_select", 1 },
        "Technique Select",
        0, 7, 0
    ));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "ks_enabled", 1 },
        "Keyswitch Enabled",
        false
    ));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "ks_low_note", 1 },
        "Keyswitch Low Note",
        0, 127, 0
    ));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "ks_high_note", 1 },
        "Keyswitch High Note",
        0, 127, 9
    ));

    // ========== v1.15.0 CC + PC triggers (3) ==========
    //
    // Three trigger mechanisms share one pendingTechniqueIndex atom with
    // KS > CC > PC > history precedence in processBlock. Tables for the
    // CC sub-bands and PC#-to-technique map live outside APVTS in
    // currentCcMapping / currentPcMapping (8-slot COW shared_ptrs) so we
    // are not paying for 24 APVTS slots that the host UI cannot cleanly
    // display. The three knobs below are the per-trigger gate + the CC#
    // selector — small enough to round-trip through APVTS and through
    // host automation.
    //
    // cc_select_enabled  bool   — true = CC scan active in processBlock
    // cc_number          0..119 — controller number to listen on
    //                              (default 32 = General Purpose 1, free
    //                              of standardised meanings: not CC1
    //                              modulation, not CC11 expression, not
    //                              the bank-select pair). The 0..119 cap
    //                              avoids the channel-mode CCs at 120+.
    // pc_enabled         bool   — true = PC scan active in processBlock
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "cc_select_enabled", 1 },
        "CC Select Enabled",
        false
    ));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cc_number", 1 },
        "CC Number",
        0, 119, 32
    ));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "pc_enabled", 1 },
        "Program Change Enabled",
        false
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

    // v1.14.0: seed the curated default technique vocabulary. The user can
    // rename slots later; persisted via captureStateValueTree's <Techniques>
    // child.
    resetTechniqueNames();

    // v1.15.0: seed the CC + PC trigger mapping tables. Defaults: CC splits
    // 0..127 equally across the current technique_count (1 by default →
    // entire range routes to tech 0); PC#i routes to tech i.
    {
        const int initialCount = juce::jlimit (1, 8,
            (int) parameters.getRawParameterValue ("technique_count")->load());
        currentCcMapping = std::make_shared<OMtsTrigger::CcMapping> (
            OMtsTrigger::defaultCcMapping (initialCount));
        currentPcMapping = std::make_shared<OMtsTrigger::PcMapping> (
            OMtsTrigger::defaultPcMapping());
    }

    // Pre-allocate 16 voices (matches max polyphony cap). Voice manager will
    // enforce the runtime cap; pre-allocating prevents processBlock allocations
    // when the user raises the cap (PERF-01).
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new MicrotonalSamplerVoice();
        voice->setAPVTS                  (&parameters);
        voice->setTuningEngine           (&tuningEngine);                          // D-4: global namespace
        voice->setPendingTuningSource    (&vst3Extensions.getPendingTable());      // module-owned table
        voice->setSampleMapSource        (&currentSampleMap);                      // shared_ptr slot
        voice->setRrCounterArray         (&rrCounters);                            // v1.8.0
        voice->setPendingTechniqueSource (&pendingTechniqueIndex);                 // v1.14.0
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

OMicrotonalSamplerAudioProcessor::~OMicrotonalSamplerAudioProcessor()
{
    // v1.12.3 (HG-05): clear the WeakReference master FIRST so any
    // SampleLoader callback already queued on the message thread sees a null
    // weak handle and bails on entry. The order matters — clearing before
    // members run down means later teardown (APVTS, sampleLoader) can't be
    // observed by a still-pending callback that holds a strong this pointer.
    masterReference.clear();

    // v1.12.1 (CR-01): defensively cancel any pending CC 11 forward so the
    // message thread can't fire handleAsyncUpdate after the processor's APVTS
    // is gone. AsyncUpdater's own destructor performs this, but doing it
    // explicitly here documents the intent and runs before APVTS teardown.
    cancelPendingUpdate();
}

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

    // v1.14.0: pre-allocate the keyswitch filter buffer. 2048 bytes covers
    // any reasonable per-block MIDI density (a worst-case 1024-sample block
    // with 32-byte SysEx every sample would still fit). Allocation is a
    // message-thread call here — processBlock's addEvent stays alloc-free.
    ksFilteredBuffer.ensureSize (2048);

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
    //
    // v1.12.1 (CR-01): the audio thread no longer calls setValueNotifyingHost
    // — that is a real-time correctness violation (the call dispatches to
    // listeners that may take host locks, allocate, or block). We stage the
    // last CC 11 byte of the block into a lock-free atomic and ask the
    // message thread to forward it to the host via handleAsyncUpdate. The
    // squared-curve smoothing applied below uses the live "expression"
    // APVTS atom directly, so audio gain still tracks the CC stream within
    // ~one message-thread cycle (≈ tens of µs to a few ms — far below the
    // 10 ms smoother ramp).
    int latestCC11 = -1;
    for (const auto meta : midiMessages)
    {
        const auto msg = meta.getMessage();
        if (msg.isController() && msg.getControllerNumber() == 11)
            latestCC11 = msg.getControllerValue();
    }
    if (latestCC11 >= 0)
    {
        pendingCC11Value.store (latestCC11, std::memory_order_relaxed);
        triggerAsyncUpdate();
    }

    // FUNC-03: propagate the polyphony APVTS cap into the synth before MIDI is
    // dispatched, so CappedSynthesiser::noteOn enforces the user's cap on this
    // block's note-ons.
    if (auto* pp = parameters.getRawParameterValue ("polyphony"))
        synthesiser.setVoiceCap ((int) pp->load());

    // v1.14.0 / v1.15.0: KS / CC / PC technique routing.
    //
    // Three trigger mechanisms share one pendingTechniqueIndex atom with
    // KS > CC > PC > history precedence:
    //
    //   * KS: a note-on inside [ks_low..ks_high] is absorbed (never
    //     forwarded to the synth) and its semitone offset from ks_low
    //     becomes the candidate technique.
    //   * CC: a controller event with controllerNumber == cc_number is
    //     looked up against the 8-slot CC table; the value's containing
    //     band's technique is the candidate.
    //   * PC: a program-change event is looked up against the 8-slot PC
    //     table; the matching slot's technique is the candidate.
    //
    // We do NOT overwrite the atomic on every event — that would let a
    // late-block PC clobber an earlier-block KS. Instead we accumulate
    // candidate values across the block and resolve precedence once at
    // the end via OMtsTrigger::resolveTriggerPrecedence. If no trigger
    // fired, the atomic is left alone (history persists).
    //
    // RT-safety: the KS filter buffer was pre-sized in prepareToPlay, so
    // addEvent does not allocate. CC + PC reads consult atomic_load'd
    // shared_ptr snapshots of the mapping tables — the COW pattern keeps
    // the audio thread free of locks. CC + PC scans never modify
    // midiMessages — they observe only — so no MIDI re-dispatch is
    // needed for those two triggers (the host's CC/PC bytes still reach
    // any downstream NE / parameter listeners).
    bool        ksEnabled  = false;
    int         ksLowNote  = 0;
    int         ksHighNote = 0;
    int         techCount  = 1;
    bool        ccEnabled  = false;
    int         ccNumber   = 0;
    bool        pcEnabled  = false;
    if (auto* p = parameters.getRawParameterValue ("ks_enabled"))
        ksEnabled = (p->load() > 0.5f);
    if (auto* p = parameters.getRawParameterValue ("ks_low_note"))
        ksLowNote = juce::jlimit (0, 127, (int) p->load());
    if (auto* p = parameters.getRawParameterValue ("ks_high_note"))
        ksHighNote = juce::jlimit (0, 127, (int) p->load());
    if (auto* p = parameters.getRawParameterValue ("technique_count"))
        techCount = juce::jlimit (1, 8, (int) p->load());
    if (auto* p = parameters.getRawParameterValue ("cc_select_enabled"))
        ccEnabled = (p->load() > 0.5f);
    if (auto* p = parameters.getRawParameterValue ("cc_number"))
        ccNumber = juce::jlimit (0, 119, (int) p->load());
    if (auto* p = parameters.getRawParameterValue ("pc_enabled"))
        pcEnabled = (p->load() > 0.5f);

    const juce::MidiBuffer* dispatchMidi = &midiMessages;
    bool techniqueChangedThisBlock = false;

    // Snapshot the mapping tables once per block (COW shared_ptr; lock-free
    // load — std::atomic_load on shared_ptr is RT-safe per the v1.14.0
    // sample-map pattern).
   #if (defined(__cplusplus) && __cplusplus >= 202002L)
    auto ccTable = std::atomic_load (&currentCcMapping);
    auto pcTable = std::atomic_load (&currentPcMapping);
   #else
    auto ccTable = std::atomic_load (&currentCcMapping);
    auto pcTable = std::atomic_load (&currentPcMapping);
   #endif

    int ksTechCandidate = -1;
    int ccTechCandidate = -1;
    int pcTechCandidate = -1;

    if (ksEnabled && ksHighNote >= ksLowNote)
    {
        ksFilteredBuffer.clear();
        for (const auto meta : midiMessages)
        {
            const auto msg = meta.getMessage();
            if ((msg.isNoteOn() || msg.isNoteOff())
                && msg.getNoteNumber() >= ksLowNote
                && msg.getNoteNumber() <= ksHighNote)
            {
                if (msg.isNoteOn())
                {
                    ksTechCandidate = juce::jlimit (
                        0, juce::jmin (7, techCount - 1),
                        msg.getNoteNumber() - ksLowNote);
                }
                continue;   // absorb (do not forward to synth)
            }

            // CC + PC scan happens inside this loop too so we only walk
            // the buffer once. The events are forwarded unchanged to the
            // filtered buffer.
            if (ccEnabled && ccTable != nullptr
                && msg.isController() && msg.getControllerNumber() == ccNumber)
            {
                const int t = OMtsTrigger::resolveCcTechnique (
                    msg.getControllerValue(), *ccTable);
                if (t >= 0)
                    ccTechCandidate = juce::jlimit (
                        0, juce::jmin (7, techCount - 1), t);
            }
            if (pcEnabled && pcTable != nullptr && msg.isProgramChange())
            {
                const int t = OMtsTrigger::resolvePcTechnique (
                    msg.getProgramChangeNumber(), *pcTable);
                if (t >= 0)
                    pcTechCandidate = juce::jlimit (
                        0, juce::jmin (7, techCount - 1), t);
            }

            ksFilteredBuffer.addEvent (msg, meta.samplePosition);
        }
        dispatchMidi = &ksFilteredBuffer;
    }
    else if (ccEnabled || pcEnabled)
    {
        // KS path is disabled but at least one of CC / PC is on, so we
        // still need to walk the buffer once to harvest candidates. We do
        // NOT need the filter buffer here — no events are absorbed.
        for (const auto meta : midiMessages)
        {
            const auto msg = meta.getMessage();
            if (ccEnabled && ccTable != nullptr
                && msg.isController() && msg.getControllerNumber() == ccNumber)
            {
                const int t = OMtsTrigger::resolveCcTechnique (
                    msg.getControllerValue(), *ccTable);
                if (t >= 0)
                    ccTechCandidate = juce::jlimit (
                        0, juce::jmin (7, techCount - 1), t);
            }
            if (pcEnabled && pcTable != nullptr && msg.isProgramChange())
            {
                const int t = OMtsTrigger::resolvePcTechnique (
                    msg.getProgramChangeNumber(), *pcTable);
                if (t >= 0)
                    pcTechCandidate = juce::jlimit (
                        0, juce::jmin (7, techCount - 1), t);
            }
        }
    }

    // Resolve precedence — KS > CC > PC > history. If nothing fired we
    // leave pendingTechniqueIndex alone (history persists across blocks).
    if (ksTechCandidate >= 0 || ccTechCandidate >= 0 || pcTechCandidate >= 0)
    {
        const int historyTech = pendingTechniqueIndex.load (std::memory_order_acquire);
        const int newTech = OMtsTrigger::resolveTriggerPrecedence (
            ksTechCandidate, ccTechCandidate, pcTechCandidate, historyTech);
        if (newTech != historyTech)
        {
            pendingTechniqueIndex.store (newTech, std::memory_order_release);
            techniqueChangedThisBlock = true;
        }
    }

    // Render all voices via synthesiser (handles MIDI routing + voice allocation).
    synthesiser.renderNextBlock (buffer, *const_cast<juce::MidiBuffer*> (dispatchMidi),
                                  0, buffer.getNumSamples());

    if (techniqueChangedThisBlock)
    {
        // Hand the message thread a one-shot notification so the UI's
        // technique tab strip refreshes the active highlight. Last-store-wins
        // within a block is fine — the cursor is already in the atomic.
        techniqueStateDirty.store (true, std::memory_order_release);
        triggerAsyncUpdate();
    }

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
// v1.12.1 (CR-01): drain the pending CC 11 value on the message thread and
// forward to the host. setValueNotifyingHost dispatches to APVTS listeners
// (including the WebSliderRelay that updates the UI knob) and to the host's
// parameter machinery (for automation recording) — both paths are
// message-thread-safe but illegal from processBlock.
//
// Coalescing: if multiple processBlock invocations stage values between
// scheduled async updates, only the latest value reaches the host. This is
// fine — last-value-wins is the documented CC 11 contract and matches the
// pre-fix behaviour at the message-thread granularity.
void OMicrotonalSamplerAudioProcessor::handleAsyncUpdate()
{
    const int v = pendingCC11Value.exchange (-1, std::memory_order_acquire);
    if (v >= 0)
    {
        if (auto* ep = parameters.getParameter ("expression"))
            ep->setValueNotifyingHost ((float) v / 127.0f);
    }

    // v1.14.0: technique cursor / vocab change notification. Coalesced via
    // techniqueStateDirty — multiple processBlock invocations between
    // scheduled async updates collapse to one callback. The cursor itself
    // lives in pendingTechniqueIndex (atomic); the editor reads via
    // getActiveTechnique().
    if (techniqueStateDirty.exchange (false, std::memory_order_acquire))
    {
        // Mirror the audio-thread cursor onto the APVTS host-facing param so
        // automation / preset round-trip work and host UIs see the change.
        if (auto* sp = parameters.getParameter ("technique_select"))
        {
            const auto range = parameters.getParameterRange ("technique_select");
            const int  tech  = pendingTechniqueIndex.load (std::memory_order_acquire);
            sp->setValueNotifyingHost (range.convertTo0to1 ((float) tech));
        }

        if (techniqueStateChangedCallback)
            techniqueStateChangedCallback();
    }

    // v1.15.0: CC/PC trigger-table change notification. Same coalescing
    // pattern as the technique-state path. The editor uses this to refresh
    // the CC/PC panel UI after native-fn mutations.
    if (triggerStateDirty.exchange (false, std::memory_order_acquire))
    {
        if (triggerStateChangedCallback)
            triggerStateChangedCallback();
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

    // v1.12.3 (HG-05): capture a WeakReference instead of raw `this`. The
    // ~SampleLoader 2-second join only flushes the loader's worker thread —
    // it does NOT cancel callbacks already posted to the message thread via
    // MessageManager::callAsync. If the user closes the project mid-load,
    // those queued callbacks would otherwise run with a dangling `this`.
    juce::WeakReference<OMicrotonalSamplerAudioProcessor> safeThis (this);

    sampleLoader->loadFolder (
        folder,
        getSampleRate() > 0.0 ? getSampleRate() : 48000.0,
        LoadOptions { clampedLayer, overrideTokens,
                      op.targetTechnique, op.overrideTechnique },   // v1.14.0

        // Completion callback — runs on the message thread.
        [safeThis, op](std::shared_ptr<SampleMap> newMap,
                       juce::StringArray skipped,
                       std::vector<SampleLoader::AmbiguousDuplicate> ambig)
        {
            auto* self = safeThis.get();
            if (self == nullptr)
                return;   // processor destroyed before callback ran

            // v1.8.0: ambiguous duplicates → stage the map and surface the
            // confirmation modal instead of applying immediately.
            if (! ambig.empty())
            {
                self->pendingDuplicateMap            = std::move (newMap);
                self->pendingDuplicateSkippedFiles   = std::move (skipped);
                self->pendingAmbiguousDuplicates     = std::move (ambig);
                self->pendingDuplicateOp             = op;
                self->pendingDuplicateChainContinuation = nullptr;
                if (self->ambiguousDuplicateCallback)
                    self->ambiguousDuplicateCallback (self->pendingAmbiguousDuplicates);
                return;
            }
            self->applyFolderLoad (std::move (newMap), skipped, op);
        },

        // Failure callback — runs on the message thread.
        [safeThis](const juce::String& reason)
        {
            DBG ("SampleLoader failure: " << reason);
            juce::ignoreUnused (reason);
            auto* self = safeThis.get();
            if (self == nullptr)
                return;
            {
                // v1.12.1 (HG-08).
                const juce::ScopedLock persistLock (self->persistenceLock);
                self->lastSkippedFiles.clear();
            }
            // Drop the recorded folder so a failed reload doesn't get
            // re-persisted. (clearSampleMap leaves currentSampleFolder
            // untouched on purpose — only an explicit load-failure clears.)
            self->currentSampleFolder = juce::File();
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

    // v1.12.1 (HG-08): hold persistenceLock across loadOpHistory + lastSkipped
    // mutations so a Reaper save thread reading captureStateValueTree can't
    // observe a half-built history vector. The lock is released before the
    // sampleMapChangedCallback fires so editor work can't deadlock against it.
    const juce::ScopedLock persistLock (persistenceLock);

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
            // v1.8.0: drop counters for the wiped layer (across all
            // techniques as of v1.14.0 — ReplaceLayer wipes the layer
            // wholesale regardless of technique slot).
            for (int midi = 0; midi < 128; ++midi)
                for (int tech = 0; tech < 8; ++tech)
                    rrCounters[(size_t) (midi * 4 * 8 + target * 8 + tech)].store (
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
        // v1.12.3 (HG-04): cap is owned by MicrotonalSamplerVoice, where the
        // uint8_t / 0xFF-sentinel coupling lives. static_assert there.
        constexpr int kMaxVariantsPerCell = MicrotonalSamplerVoice::kMaxVariantsPerCell;

        for (const auto& newCell : newSlotsMap->cells)
        {
            // v1.14.0: collision + counter index are keyed on the triplet.
            const int counterIdx = juce::jlimit (
                0, MicrotonalSamplerVoice::kRrCounterSize - 1,
                newCell.midiNote * 4 * 8 + newCell.velocityLayer * 8 + newCell.technique);

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
                                && c.velocityLayer == newCell.velocityLayer
                                && c.technique     == newCell.technique;
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

    // v1.14.0: auto-grow technique_count to fit any populated slot. Without
    // this, a folder of `_pizz` files (slot 5) would land correctly but the
    // technique tab strip would stay hidden (count==1, ks_enabled==false →
    // back-compat collapsed state) and the user would see an empty grid
    // because the grid filters by active technique. Auto-growing surfaces
    // the populated slots immediately.
    int maxTech = 0;
    for (const auto& c : merged->cells)
        maxTech = juce::jmax (maxTech, c.technique);
    const int requiredCount = juce::jlimit (1, 8, maxTech + 1);
    if (auto* tcParam = parameters.getParameter ("technique_count"))
    {
        const int currentCount = juce::jlimit (1, 8,
            (int) parameters.getRawParameterValue ("technique_count")->load());
        if (requiredCount > currentCount)
        {
            const auto range = parameters.getParameterRange ("technique_count");
            tcParam->setValueNotifyingHost (range.convertTo0to1 ((float) requiredCount));
            techniqueStateDirty.store (true, std::memory_order_release);
            triggerAsyncUpdate();
        }
    }

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
        {
            // v1.12.1 (HG-08).
            const juce::ScopedLock persistLock (persistenceLock);
            lastSkippedFiles = skipped;
        }
        if (sampleMapChangedCallback)
            sampleMapChangedCallback();
    }

    // Continue any chained replay sequence (state-restore path).
    // v1.12.3 (HG-01): the chain itself carries a queue-generation token
    // captured at staging time and bails out internally if the queue has
    // since been wiped/rebuilt by an unrelated path. We just fire it.
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
    // v1.12.3 (HG-01): re-entry guard. If a synchronous callback chain
    // (e.g. applyFolderLoad → sampleMapChangedCallback → editor synchronously
    // triggering another path that lands back here) re-enters while an outer
    // kick is mid-walk over pendingReplayOps, the inner kick would pop ops
    // from under the outer iterator and corrupt the chain. The outer kick
    // either completes synchronously (Case 1: embedded) or returns after
    // dispatching async work (Case 3: filesystem); the next legitimate kick
    // arrives on a fresh stack via the loader's message-thread callback.
    if (replayKickReentryGuard)
    {
        DBG ("kickNextReplayOp: re-entry rejected (cascaded callback)");
        return;
    }
    replayKickReentryGuard = true;
    struct ReentryGuard { bool& flag; ~ReentryGuard() { flag = false; } }
        guard { replayKickReentryGuard };

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

        // v1.12.3 (HG-01 + HG-05): WeakReference + queue-generation stamp.
        // - WeakReference: the loader's message-thread callback may run after
        //   the processor is gone if the user closes the project mid-load.
        // - Generation stamp: when the callback stages a chain continuation
        //   (ambiguous duplicates), the queue may be wiped/rebuilt before the
        //   user confirms; capture the generation now so the deferred chain
        //   can detect that and abort.
        juce::WeakReference<OMicrotonalSamplerAudioProcessor> safeThis (this);
        const auto kickGeneration = replayQueueGeneration.load (std::memory_order_relaxed);

        sampleLoader->loadFolder (
            f,
            getSampleRate() > 0.0 ? getSampleRate() : 48000.0,
            LoadOptions { op.targetLayer, op.overrideTokens,
                          op.targetTechnique, op.overrideTechnique },   // v1.14.0
            [safeThis, op, kickGeneration] (std::shared_ptr<SampleMap> newMap,
                        juce::StringArray skipped,
                        std::vector<SampleLoader::AmbiguousDuplicate> ambig)
            {
                auto* self = safeThis.get();
                if (self == nullptr)
                    return;   // processor destroyed mid-replay

                // Generation check — if pendingReplayOps was wiped/rebuilt
                // (state restore, clearSampleMap) since this op was dispatched,
                // this callback's op no longer belongs to the live chain.
                if (self->replayQueueGeneration.load (std::memory_order_relaxed)
                        != kickGeneration)
                {
                    DBG ("SampleLoader replay completion: stale generation, abandoning");
                    return;
                }

                if (! ambig.empty())
                {
                    // v1.8.0: stage the map and surface the modal; chain the
                    // next replay op once the user confirms/rejects.
                    self->pendingDuplicateMap            = std::move (newMap);
                    self->pendingDuplicateSkippedFiles   = std::move (skipped);
                    self->pendingAmbiguousDuplicates     = std::move (ambig);
                    self->pendingDuplicateOp             = op;
                    // v1.12.3 (HG-01): chain continuation captures the
                    // generation that staged it; bails on mismatch.
                    self->pendingDuplicateChainContinuation =
                        [safeThis, kickGeneration]()
                        {
                            auto* s = safeThis.get();
                            if (s == nullptr)
                                return;
                            if (s->replayQueueGeneration.load (std::memory_order_relaxed)
                                    != kickGeneration)
                            {
                                DBG ("chain continuation: stale generation, abandoning");
                                return;
                            }
                            s->kickNextReplayOp();
                        };
                    if (self->ambiguousDuplicateCallback)
                        self->ambiguousDuplicateCallback (self->pendingAmbiguousDuplicates);
                    return;
                }
                self->applyFolderLoad (std::move (newMap), skipped, op);
                self->kickNextReplayOp();   // chain to next op
            },
            [safeThis, kickGeneration] (const juce::String& reason)
            {
                DBG ("SampleLoader replay failure: " << reason);
                juce::ignoreUnused (reason);
                auto* self = safeThis.get();
                if (self == nullptr)
                    return;
                if (self->replayQueueGeneration.load (std::memory_order_relaxed)
                        != kickGeneration)
                    return;
                self->kickNextReplayOp();   // continue chain even on failure
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
                                                          bool mergeAsRr,
                                                          int technique)
{
    if (sampleLoader == nullptr)
        return;

    technique = juce::jlimit (0, kMaxTechniques - 1, technique);

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

    // v1.12.3 (HG-05): WeakReference instead of raw `this`. Single-sample
    // loads share the SampleLoader thread with folder loads; the same
    // post-destructor risk applies.
    juce::WeakReference<OMicrotonalSamplerAudioProcessor> safeThis (this);

    // ------------------------ Async load ------------------------
    sampleLoader->loadSingleVariant (
        file,
        midiPitch,
        velocityLayer,
        sr,
        [safeThis, mergeAsRr, technique] (int targetMidi, int targetVel,
                               SampleVariant newVariant, juce::String skipReason)
        {
            auto* self = safeThis.get();
            if (self == nullptr)
                return;

            if (newVariant.audio == nullptr)
            {
                if (skipReason.isNotEmpty())
                {
                    {
                        // v1.12.1 (HG-08).
                        const juce::ScopedLock persistLock (self->persistenceLock);
                        self->lastSkippedFiles.add (skipReason);
                    }
                    DBG ("loadSingleSample failed: " << skipReason);
                }
                if (self->sampleMapChangedCallback)
                    self->sampleMapChangedCallback();
                return;
            }

           #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
            auto currentMap = std::atomic_load (&self->currentSampleMap);
           #else
            auto currentMap = self->currentSampleMap;
           #endif

            // v1.12.3 (HG-04): cap is owned by MicrotonalSamplerVoice.
            constexpr int kMaxVariantsPerCell = MicrotonalSamplerVoice::kMaxVariantsPerCell;

            // v1.9.0: detect collision with an existing cell. mergeAsRr=true
            // appends to the cell's variants; mergeAsRr=false (default) keeps
            // v1.8.0 semantics — replace the cell with a fresh single-variant.
            //
            // v1.14.0: collision is per-(midi, layer, technique) triplet.
            // Loading a single sample into technique slot 1 leaves slot 0's
            // cell at the same (midi, layer) untouched.
            const SampleCell* existingCell = nullptr;
            if (currentMap != nullptr)
            {
                for (const auto& c : currentMap->cells)
                    if (c.midiNote == targetMidi
                        && c.velocityLayer == targetVel
                        && c.technique == technique)
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
                {
                    // v1.12.1 (HG-08).
                    const juce::ScopedLock persistLock (self->persistenceLock);
                    self->lastSkippedFiles.add ("variant cap reached: " + newVariant.filename);
                }
                DBG ("loadSingleSample: variant cap (" << kMaxVariantsPerCell
                     << ") reached at midi=" << targetMidi << " vel=" << targetVel
                     << " — skipping " << newVariant.filename);
                if (self->sampleMapChangedCallback)
                    self->sampleMapChangedCallback();
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
                    if (c.midiNote == targetMidi
                        && c.velocityLayer == targetVel
                        && c.technique == technique)
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
                freshCell.technique     = technique;     // v1.14.0
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
            std::atomic_store (&self->currentSampleMap, next);
           #else
            self->currentSampleMap = next;
           #endif

            // v1.8.0: per-cell replace resets the RR counter — the old cell
            // may have had multiple variants; the new cell has exactly one.
            // v1.9.0: merge path also resets so the next note-on doesn't
            // index past the just-grown variants vector with a stale value.
            // v1.14.0: counter is now keyed on the (midi, layer, technique)
            // triplet (4096 entries; layout matches MicrotonalSamplerVoice).
            const int counterIdx = juce::jlimit (
                0, MicrotonalSamplerVoice::kRrCounterSize - 1,
                targetMidi * 4 * 8 + targetVel * 8 + technique);
            self->rrCounters[(size_t) counterIdx].store ((uint8_t) 0xFFu,
                                                          std::memory_order_relaxed);

            DBG ("loadSingleSample success: midi=" << targetMidi
                 << " vel=" << targetVel
                 << " tech=" << technique
                 << " mode=" << (doMerge ? "merge_rr" : "replace")
                 << " cells=" << (int) next->cells.size()
                 << " v" << next->version);

            // v1.14.0: auto-grow technique_count if this single-sample load
            // landed on a slot beyond the current count. Same rationale as
            // applyFolderLoad — keeps the technique tab strip visible.
            const int requiredCount = juce::jlimit (1, 8, technique + 1);
            auto& apvts = self->parameters;
            if (auto* tcParam = apvts.getParameter ("technique_count"))
            {
                const int currentCount = juce::jlimit (1, 8,
                    (int) apvts.getRawParameterValue ("technique_count")->load());
                if (requiredCount > currentCount)
                {
                    const auto range = apvts.getParameterRange ("technique_count");
                    tcParam->setValueNotifyingHost (range.convertTo0to1 ((float) requiredCount));
                    self->techniqueStateDirty.store (true, std::memory_order_release);
                    self->triggerAsyncUpdate();
                }
            }

            if (self->sampleMapChangedCallback)
                self->sampleMapChangedCallback();
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
                                                            int variantIndex,
                                                            int technique)
{
    technique = juce::jlimit (0, kMaxTechniques - 1, technique);

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

    // v1.14.0: triplet lookup. The (midi, vel, tech) cell either exists or
    // it doesn't — overrideLoopPoints does NOT fall through to tech=0 (the
    // user is editing a specific technique slot's loop region).
    const auto* foundCell = current->findCell (midiPitch, velocityLayer, technique);
    if (foundCell == nullptr || foundCell->variants.empty()
        || foundCell->technique != technique)
    {
        DBG ("overrideLoopPoints: cell absent (midi=" << midiPitch
             << " vel=" << velocityLayer << " tech=" << technique << ")");
        return;
    }

    auto next = std::make_shared<SampleMap> (*current);

    SampleCell* targetCell = nullptr;
    for (auto& c : next->cells)
    {
        if (c.midiNote == midiPitch
            && c.velocityLayer == velocityLayer
            && c.technique == technique)
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
                                                               int variantIndex,
                                                               int technique)
{
    overrideLoopPoints (midiPitch, velocityLayer, 0, 0, 0,
                        /*resetToAutoDetect*/ true,
                        variantIndex,
                        technique);
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

    {
        // v1.12.1 (HG-08): synchronise against off-thread getStateInformation.
        const juce::ScopedLock persistLock (persistenceLock);
        lastSkippedFiles.clear();

        // v1.6.0: drop the load-op history so the next save reflects the
        // cleared state. (currentSampleFolder is intentionally left alone —
        // same comment as v1.3.0.)
        loadOpHistory.clear();
    }

    // v1.12.3 (HG-01): bump replay-queue generation. If a state-restore
    // chain is in flight (rare, but possible if user mashes Clear during
    // project reopen), the deferred chain continuation will detect the
    // mismatch and abort instead of replaying ops onto the freshly
    // cleared map.
    replayQueueGeneration.fetch_add (1, std::memory_order_relaxed);

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
// thread sync needed for the map. lastSkippedFiles is mutated on the message
// thread (loader completion path) AND read here on the message thread, so
// these access pairs serialise naturally. The persistenceLock added in v1.12.1
// (HG-08) is for the off-thread getStateInformation read; this method does
// not need it. Acquire the lock here too if a future caller invokes this
// from a worker thread.
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
             << ",\"technique\":"    << c.technique          // v1.14.0
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
                                                                      int variantIndex,
                                                                      int technique) const
{
    technique = juce::jlimit (0, kMaxTechniques - 1, technique);

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

    // v1.14.0: triplet lookup. Falls back to technique=0 when slot is empty
    // so the loop-editor preview keeps rendering even after a user clicks
    // an "ord" cell while a different technique tab is active in the UI.
    const auto* cell = map->findCell (midiPitch, velocityLayer, technique);
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
// v1.14.0 — technique vocabulary helpers.
//
// Names live outside APVTS (string params are clumsy and don't surface to
// the host nicely). We persist them via the state ValueTree's <Techniques>
// child instead. Default vocabulary mirrors the curated list from the plan
// (RF-1 / D2-7) so single-technique back-compat libraries see "ord" as the
// default name for slot 0.
juce::StringArray OMicrotonalSamplerAudioProcessor::getTechniqueNames() const
{
    return techniqueNames;
}

void OMicrotonalSamplerAudioProcessor::setTechniqueName (int index, const juce::String& name)
{
    if (index < 0 || index >= 8)
        return;
    if (name.trim().isEmpty())
        return;

    while (techniqueNames.size() <= index)
        techniqueNames.add ("ord");

    techniqueNames.set (index, name.trim());

    techniqueStateDirty.store (true, std::memory_order_release);
    triggerAsyncUpdate();
}

void OMicrotonalSamplerAudioProcessor::resetTechniqueNames()
{
    techniqueNames.clearQuick();
    techniqueNames.add ("ord");
    techniqueNames.add ("sp");
    techniqueNames.add ("st");
    techniqueNames.add ("sv");
    techniqueNames.add ("cs");
    techniqueNames.add ("pizz");
    techniqueNames.add ("harm");
    techniqueNames.add ("mart");

    techniqueStateDirty.store (true, std::memory_order_release);
    triggerAsyncUpdate();
}

void OMicrotonalSamplerAudioProcessor::setActiveTechnique (int technique)
{
    technique = juce::jlimit (0, 7, technique);

    pendingTechniqueIndex.store (technique, std::memory_order_release);

    // Mirror onto APVTS so automation / project save round-trips. Routes
    // through setValueNotifyingHost so listeners (host UI, sliders) update.
    if (auto* sp = parameters.getParameter ("technique_select"))
    {
        const auto range = parameters.getParameterRange ("technique_select");
        sp->setValueNotifyingHost (range.convertTo0to1 ((float) technique));
    }

    techniqueStateDirty.store (true, std::memory_order_release);
    triggerAsyncUpdate();
}

//==============================================================================
// v1.15.0 — CC + PC trigger mapping accessors / mutators.
//
// Tables live in std::shared_ptr slots that the audio thread snapshots via
// atomic_load. Mutations build a new table, atomic_store it, and let the
// old table die when the last audio-thread reference releases. Memory cost
// is small (each table is < 100 bytes) so the COW pattern's overhead is
// negligible compared to the lock-free read benefit.

OMtsTrigger::CcMapping OMicrotonalSamplerAudioProcessor::getCcMapping() const
{
    auto snapshot = std::atomic_load (&currentCcMapping);
    return snapshot != nullptr ? *snapshot : OMtsTrigger::defaultCcMapping (1);
}

OMtsTrigger::PcMapping OMicrotonalSamplerAudioProcessor::getPcMapping() const
{
    auto snapshot = std::atomic_load (&currentPcMapping);
    return snapshot != nullptr ? *snapshot : OMtsTrigger::defaultPcMapping();
}

void OMicrotonalSamplerAudioProcessor::setCcMappingSlot (int slot,
                                                          int rangeLow,
                                                          int rangeHigh,
                                                          int technique)
{
    if (slot < 0 || slot >= OMtsTrigger::kMaxSlots) return;

    auto current = std::atomic_load (&currentCcMapping);
    auto next = std::make_shared<OMtsTrigger::CcMapping> (
        current != nullptr ? *current : OMtsTrigger::defaultCcMapping (1));

    int lo = juce::jlimit (0, 127, rangeLow);
    int hi = juce::jlimit (0, 127, rangeHigh);
    if (hi < lo) std::swap (lo, hi);

    auto& s = next->at ((size_t) slot);
    s.rangeLow  = lo;
    s.rangeHigh = hi;
    s.technique = juce::jlimit (0, OMtsTrigger::kMaxTech - 1, technique);

    std::atomic_store (&currentCcMapping, next);

    triggerStateDirty.store (true, std::memory_order_release);
    triggerAsyncUpdate();
}

void OMicrotonalSamplerAudioProcessor::setPcMappingSlot (int slot,
                                                          int pcNumber,
                                                          int technique)
{
    if (slot < 0 || slot >= OMtsTrigger::kMaxSlots) return;

    auto current = std::atomic_load (&currentPcMapping);
    auto next = std::make_shared<OMtsTrigger::PcMapping> (
        current != nullptr ? *current : OMtsTrigger::defaultPcMapping());

    auto& s = next->at ((size_t) slot);
    s.pc        = juce::jlimit (0, 127, pcNumber);
    s.technique = juce::jlimit (0, OMtsTrigger::kMaxTech - 1, technique);

    std::atomic_store (&currentPcMapping, next);

    triggerStateDirty.store (true, std::memory_order_release);
    triggerAsyncUpdate();
}

void OMicrotonalSamplerAudioProcessor::resetTriggerMappings()
{
    const int count = juce::jlimit (1, 8,
        (int) parameters.getRawParameterValue ("technique_count")->load());

    auto cc = std::make_shared<OMtsTrigger::CcMapping> (
        OMtsTrigger::defaultCcMapping (count));
    auto pc = std::make_shared<OMtsTrigger::PcMapping> (
        OMtsTrigger::defaultPcMapping());

    std::atomic_store (&currentCcMapping, cc);
    std::atomic_store (&currentPcMapping, pc);

    triggerStateDirty.store (true, std::memory_order_release);
    triggerAsyncUpdate();
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

    // v1.14.0: <TechniqueNames><Slot index="0" name="ord"/>…</TechniqueNames>
    // Sparse child list (only populated slots emitted) so old sessions with no
    // child decode cleanly back to the default vocab.
    constexpr const char* kTechniqueNamesTag = "TechniqueNames";
    constexpr const char* kTechniqueSlotTag  = "Slot";

    // v1.15.0:
    //   <CcMapping><Slot index="0" lo="0" hi="15" tech="0"/>…</CcMapping>
    //   <PcMapping><Slot index="0" pc="0" tech="0"/>…</PcMapping>
    //
    // Always 8 slots. Old (v1.14.0) sessions have neither child — the
    // processor constructor pre-seeds defaults so back-compat is automatic.
    constexpr const char* kCcMappingTag    = "CcMapping";
    constexpr const char* kPcMappingTag    = "PcMapping";
    constexpr const char* kTriggerSlotTag  = "Slot";

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
            || child.hasType (kTuningStateTag)
            || child.hasType (kTechniqueNamesTag)      // v1.14.0
            || child.hasType (kCcMappingTag)           // v1.15.0
            || child.hasType (kPcMappingTag))          // v1.15.0
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
    {
        // v1.12.1 (HG-08): Reaper can call getStateInformation off the
        // message thread while applyFolderLoad is mid-mutation. Snapshot the
        // history under the lock; the embeddedSlots shared_ptrs keep the
        // audio buffers alive for buildEmbeddedAudioTree, which we run
        // outside the lock to avoid blocking message-thread mutations on
        // (potentially expensive) per-variant WAV serialisation. The local
        // copy is cheap — LoadOps are small POD-ish structs holding
        // shared_ptrs and short juce::Strings.
        std::vector<LoadOp> historySnapshot;
        {
            const juce::ScopedLock persistLock (persistenceLock);
            historySnapshot = loadOpHistory;
        }
        for (const auto& op : historySnapshot)
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

            // Inline audio blob — only emitted when the user opted in via
            // embed AND the per-op slot snapshot was attached at load time.
            if (op.embedAudio && op.embeddedSlots != nullptr)
                opTree.appendChild (buildEmbeddedAudioTree (*op.embeddedSlots), nullptr);

            folders.appendChild (opTree, nullptr);
        }
    }
    root.appendChild (folders, nullptr);

    // TuningState — full engine snapshot.
    root.appendChild (captureTuningValueTree (tuningEngine), nullptr);

    // v1.14.0 — technique vocabulary. Only the names need persistence — the
    // count / KS range / active-select are APVTS params and round-trip via
    // <PARAM> children automatically.
    {
        juce::ValueTree names (kTechniqueNamesTag);
        for (int i = 0; i < techniqueNames.size(); ++i)
        {
            juce::ValueTree slot (kTechniqueSlotTag);
            slot.setProperty ("index", i, nullptr);
            slot.setProperty ("name",  techniqueNames[i], nullptr);
            names.appendChild (slot, nullptr);
        }
        root.appendChild (names, nullptr);
    }

    // v1.15.0 — CC + PC mapping tables. The cc_select_enabled / cc_number /
    // pc_enabled gates round-trip via <PARAM> children automatically; only
    // the per-slot tables need an explicit XML payload.
    {
        const auto cc = getCcMapping();
        juce::ValueTree ccTree (kCcMappingTag);
        for (int i = 0; i < OMtsTrigger::kMaxSlots; ++i)
        {
            juce::ValueTree slot (kTriggerSlotTag);
            slot.setProperty ("index", i,                                nullptr);
            slot.setProperty ("lo",    cc[(size_t) i].rangeLow,          nullptr);
            slot.setProperty ("hi",    cc[(size_t) i].rangeHigh,         nullptr);
            slot.setProperty ("tech",  cc[(size_t) i].technique,         nullptr);
            ccTree.appendChild (slot, nullptr);
        }
        root.appendChild (ccTree, nullptr);

        const auto pc = getPcMapping();
        juce::ValueTree pcTree (kPcMappingTag);
        for (int i = 0; i < OMtsTrigger::kMaxSlots; ++i)
        {
            juce::ValueTree slot (kTriggerSlotTag);
            slot.setProperty ("index", i,                                nullptr);
            slot.setProperty ("pc",    pc[(size_t) i].pc,                nullptr);
            slot.setProperty ("tech",  pc[(size_t) i].technique,         nullptr);
            pcTree.appendChild (slot, nullptr);
        }
        root.appendChild (pcTree, nullptr);
    }

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

    // 2b. v1.14.0 — technique vocabulary. Default is already seeded by the
    // ctor (resetTechniqueNames); only override the slots that the saved
    // tree actually carries. v1.13.0 sessions have no <TechniqueNames>
    // child, so the default vocab survives the restore — the back-compat
    // contract from the plan.
    auto techNamesTree = root.getChildWithName (kTechniqueNamesTag);
    if (techNamesTree.isValid())
    {
        for (int i = 0; i < techNamesTree.getNumChildren(); ++i)
        {
            const auto slot = techNamesTree.getChild (i);
            if (! slot.hasType (kTechniqueSlotTag)) continue;
            const int  idx  = juce::jlimit (0, 7,
                                  static_cast<int> (slot.getProperty ("index", 0)));
            const auto name = slot.getProperty ("name").toString();
            if (name.isNotEmpty())
            {
                while (techniqueNames.size() <= idx)
                    techniqueNames.add ("ord");
                techniqueNames.set (idx, name);
            }
        }
        techniqueStateDirty.store (true, std::memory_order_release);
        triggerAsyncUpdate();
    }

    // 2c. v1.14.0 — sync the audio-thread cursor to the restored
    // technique_select APVTS value. processBlock's atomic mirroring stays
    // in lockstep with APVTS once a note plays, but we want the cursor
    // already correct before the very first post-restore note-on.
    if (auto* tp = parameters.getRawParameterValue ("technique_select"))
        pendingTechniqueIndex.store (juce::jlimit (0, 7, (int) tp->load()),
                                     std::memory_order_release);

    // 2d. v1.15.0 — restore CC + PC mapping tables. v1.14.0 sessions have
    // neither child; the constructor's defaults survive. v1.15.0 sessions
    // override the defaults slot-by-slot — sparse trees fall back to the
    // default for the missing slots.
    {
        auto ccTree = root.getChildWithName (kCcMappingTag);
        if (ccTree.isValid())
        {
            const int initialCount = juce::jlimit (1, 8,
                (int) parameters.getRawParameterValue ("technique_count")->load());
            auto cc = std::make_shared<OMtsTrigger::CcMapping> (
                OMtsTrigger::defaultCcMapping (initialCount));
            for (int i = 0; i < ccTree.getNumChildren(); ++i)
            {
                const auto slot = ccTree.getChild (i);
                if (! slot.hasType (kTriggerSlotTag)) continue;
                const int idx = juce::jlimit (0, OMtsTrigger::kMaxSlots - 1,
                    static_cast<int> (slot.getProperty ("index", -1)));
                if (idx < 0) continue;
                auto& s = cc->at ((size_t) idx);
                s.rangeLow  = juce::jlimit (0, 127, static_cast<int> (slot.getProperty ("lo",   s.rangeLow)));
                s.rangeHigh = juce::jlimit (0, 127, static_cast<int> (slot.getProperty ("hi",   s.rangeHigh)));
                if (s.rangeHigh < s.rangeLow) std::swap (s.rangeLow, s.rangeHigh);
                s.technique = juce::jlimit (0, OMtsTrigger::kMaxTech - 1,
                                            static_cast<int> (slot.getProperty ("tech", s.technique)));
            }
            std::atomic_store (&currentCcMapping, cc);
        }

        auto pcTree = root.getChildWithName (kPcMappingTag);
        if (pcTree.isValid())
        {
            auto pc = std::make_shared<OMtsTrigger::PcMapping> (
                OMtsTrigger::defaultPcMapping());
            for (int i = 0; i < pcTree.getNumChildren(); ++i)
            {
                const auto slot = pcTree.getChild (i);
                if (! slot.hasType (kTriggerSlotTag)) continue;
                const int idx = juce::jlimit (0, OMtsTrigger::kMaxSlots - 1,
                    static_cast<int> (slot.getProperty ("index", -1)));
                if (idx < 0) continue;
                auto& s = pc->at ((size_t) idx);
                s.pc        = juce::jlimit (0, 127, static_cast<int> (slot.getProperty ("pc",   s.pc)));
                s.technique = juce::jlimit (0, OMtsTrigger::kMaxTech - 1,
                                            static_cast<int> (slot.getProperty ("tech", s.technique)));
            }
            std::atomic_store (&currentPcMapping, pc);
        }

        if (ccTree.isValid() || pcTree.isValid())
        {
            triggerStateDirty.store (true, std::memory_order_release);
            triggerAsyncUpdate();
        }
    }

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
    // v1.12.3 (HG-01): bump generation BEFORE we rebuild — any deferred
    // chain continuation captured by a previous restore will see the
    // mismatch and bail out instead of operating on the rebuilt queue.
    replayQueueGeneration.fetch_add (1, std::memory_order_relaxed);
    {
        // v1.12.1 (HG-08): brief lock around loadOpHistory mutation —
        // applyFolderLoad re-acquires the lock as each replay op completes.
        const juce::ScopedLock persistLock (persistenceLock);
        loadOpHistory.clear();   // applyFolderLoad will rebuild as ops complete
    }

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
