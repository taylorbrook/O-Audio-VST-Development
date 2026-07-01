/*
  ==============================================================================

    MicrotonalSamplerVoice.cpp
    Microtonal Sample Engine - Synthesiser Voice
    Ouaricon Audio
    Developer: Taylor Brook

    See header for staged history. v1.8.0 introduces per-cell variant selection
    (round-robin) at startNote. Render path reads from a SampleVariant pointer
    instead of a SampleSlot — semantically identical for single-variant cells
    (render-harness identity test verifies bit-exact match against v1.7.1).

  ==============================================================================
*/

#include "MicrotonalSamplerVoice.h"
#include "VoiceDsp.h"        // v1.23.2: extracted leaf DSP helpers (W9/W11 fixes live here)
#include "RetiredMapReaper.h" // v1.23.2: W10 message-thread SampleMap reaper

#include <array>
#include <atomic>
#include <cmath>
#include <utility>

// v1.23.2: the pure varispeed-read helpers (referenceFrequencyForNote,
// equalPowerWeights, cubicInterp, readVariantWithLoop, wrapLoopPosition,
// computePlayRateForVariant) moved to VoiceDsp.h so a standalone regression
// test can exercise the REAL implementations. Bring them into scope unqualified
// so every call site below is unchanged.
using namespace OMtsVoiceDsp;

namespace
{
    // v1.8.0: xorshift32 — RT-safe per-voice PRNG. Tiny and stateful; mutates
    // the seed in place. Called only from selectVariantIndex (audio thread,
    // startNote-time only — never per-sample).
    static inline uint32_t xorshift32 (uint32_t& s) noexcept
    {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
}

//==============================================================================
bool MicrotonalSamplerVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<MicrotonalSamplerSound*> (sound) != nullptr;
}

//==============================================================================
void MicrotonalSamplerVoice::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    adsr.setSampleRate (sampleRate);
    adsr.setParameters ({ 0.005f, 0.1f, 1.0f, 0.3f });
    adsr.reset();

    kMaxStealRamp = (int) std::ceil (0.005 * sampleRate) + 16;
    stealTailBufferL.assign ((size_t) kMaxStealRamp, 0.0f);
    stealTailBufferR.assign ((size_t) kMaxStealRamp, 0.0f);
    stealTailSamplesRemaining = 0;

    // v1.8.0: seed the per-voice PRNG. Use the address of `this` mixed with
    // the sample rate so each voice gets a distinct seed (still deterministic
    // within one process, which is fine — variant selection isn't a security
    // boundary).
    const auto thisPtr = reinterpret_cast<uintptr_t> (this);
    rngState = (uint32_t) (thisPtr ^ (uintptr_t) (sampleRate * 1000.0));
    if (rngState == 0)
        rngState = 0x12345678u;

    // v1.11.3: cache ADSR atomic pointers. getRawParameterValue returns
    // nullptr on missing/typo'd IDs; resolving once here lets startNote skip
    // a per-note 4-way null check and avoids the audio-thread crash on
    // load() against a null pointer. (REVIEW DSP CRITICAL #2.)
    attackParam  = nullptr;
    decayParam   = nullptr;
    sustainParam = nullptr;
    releaseParam = nullptr;
    if (parameters != nullptr)
    {
        attackParam  = parameters->getRawParameterValue ("attack");
        decayParam   = parameters->getRawParameterValue ("decay");
        sustainParam = parameters->getRawParameterValue ("sustain");
        releaseParam = parameters->getRawParameterValue ("release");
        jassert (attackParam  != nullptr);
        jassert (decayParam   != nullptr);
        jassert (sustainParam != nullptr);
        jassert (releaseParam != nullptr);
    }

    // v1.21.0: cache the CC-crossfade param atoms (same null-safe pattern as
    // the ADSR pointers above) and prepare the per-voice dynamic smoother.
    // 20 ms ramp keeps the timbre/loudness morph zipper-free as CC 11 moves,
    // while staying responsive on fast hairpins. Seeded per-note in startNote.
    dynamicsModeParam = nullptr;
    expressionParam   = nullptr;
    dynamicRangeParam = nullptr;
    if (parameters != nullptr)
    {
        dynamicsModeParam = parameters->getRawParameterValue ("dynamics_mode");
        expressionParam   = parameters->getRawParameterValue ("expression");
        dynamicRangeParam = parameters->getRawParameterValue ("dynamic_range"); // v1.22.0
    }
    dynamicsSmoother.reset (sampleRate, 0.02);
    dynamicsSmoother.setCurrentAndTargetValue (1.0f);
}

void MicrotonalSamplerVoice::setCurrentPlaybackSampleRate (double newRate)
{
    juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
    if (newRate > 0.0)
        adsr.setSampleRate (newRate);
}

//==============================================================================
// v1.8.0: pick a variant index for `cell` using mode + the persistent counter
// stored in `rrCounters`. RT-safe — pure atomic ops + integer math.
//
// Counter encoding (per cell):
//   0xFF        = sentinel "no last variant" (set by processor on ReplaceAll)
//   0..N-1      = index of the variant played most recently
//
// Cycle:           next = (last + 1) mod N    (sentinel → 0)
// RandomNoRepeat:  uniform from {0..N-1} \ {last}, falls back to uniform
//                  when N == 1 or sentinel
// Random:          uniform from {0..N-1}, no counter use
int MicrotonalSamplerVoice::selectVariantIndex (const SampleCell& cell,
                                                RoundRobinMode    mode) noexcept
{
    const int N = (int) cell.variants.size();
    if (N <= 1)
        return 0;

    if (rrCounters == nullptr)
        return 0;

    // v1.14.0: counter is now keyed on the (midi, layer, technique) triplet
    // so RR progression is independent per technique slot. Index packing
    // routes through MicrotonalSamplerVoice::packRrCounterIndex (see
    // MicrotonalSamplerVoice.h) since v1.16.10 (MEDIUM-02).
    const int counterIdx = juce::jlimit (0, kRrCounterSize - 1,
        packRrCounterIndex (cell.midiNote, cell.velocityLayer, cell.technique));
    auto& counter = (*rrCounters)[(size_t) counterIdx];

    const uint8_t  last = counter.load (std::memory_order_relaxed);
    const bool     hasLast = (last != 0xFFu) && ((int) last < N);

    int next = 0;
    switch (mode)
    {
        case RoundRobinMode::Cycle:
        {
            next = hasLast ? ((int) last + 1) % N : 0;
            break;
        }
        case RoundRobinMode::Random:
        {
            next = (int) (xorshift32 (rngState) % (uint32_t) N);
            break;
        }
        case RoundRobinMode::RandomNoRepeat:
        default:
        {
            if (! hasLast)
            {
                next = (int) (xorshift32 (rngState) % (uint32_t) N);
            }
            else
            {
                // Pick from {0..N-1} \ {last} → equivalent to picking from
                // {0..N-2} and shifting any result >= last up by 1.
                const uint32_t r = xorshift32 (rngState) % (uint32_t) (N - 1);
                next = ((int) r >= (int) last) ? (int) r + 1 : (int) r;
            }
            break;
        }
    }

    counter.store ((uint8_t) juce::jlimit (0, 254, next),
                   std::memory_order_relaxed);
    return next;
}

//==============================================================================
void MicrotonalSamplerVoice::renderTailRamp (int rampSamples) noexcept
{
    // Restructured to a positive "render iff prereqs met" form so the render
    // path below is unambiguously reachable. (REVIEW DSP CRITICAL #1.)
    //
    // rampSamples >= 2 also guards the ramp coefficient `(float) i / rampSamples`
    // against precision underflow / NaN-Inf when rampSamples is 0 or 1 — the
    // 1-sample case has no audible fade anyway, so we treat it as the no-tail
    // case and zero the buffer. (REVIEW DSP HIGH: ramp coefficient division.)
    const bool prereqsMet = (rampSamples >= 2)
                         && (variantLow != nullptr)
                         && ! stealTailBufferL.empty()
                         && ! stealTailBufferR.empty();

    if (! prereqsMet)
    {
        const int n = juce::jmax (0,
                                  juce::jmin (rampSamples,
                                              (int) stealTailBufferL.size(),
                                              (int) stealTailBufferR.size()));
        for (int i = 0; i < n; ++i)
        {
            stealTailBufferL[(size_t) i] = 0.0f;
            stealTailBufferR[(size_t) i] = 0.0f;
        }
        return;
    }

    const float lastEnv = adsr.getNextSample();

    const juce::AudioBuffer<float>* lowBuf = variantLow->audio.get();
    const int    bufLowN        = (lowBuf != nullptr) ? lowBuf->getNumSamples()  : 0;
    const int    bufLowChannels = (lowBuf != nullptr) ? lowBuf->getNumChannels() : 0;
    const float* readLowL       = (bufLowChannels > 0) ? lowBuf->getReadPointer (0) : nullptr;
    const float* readLowR       = (bufLowChannels > 1) ? lowBuf->getReadPointer (1) : readLowL;

    if (readLowL == nullptr || bufLowN <= 0)
    {
        for (int i = 0; i < rampSamples; ++i)
        {
            stealTailBufferL[(size_t) i] = 0.0f;
            stealTailBufferR[(size_t) i] = 0.0f;
        }
        return;
    }

    const int variantLowLoopStart = variantLow->loopStart;
    const int variantLowLoopEnd   = variantLow->loopEnd;

    const juce::AudioBuffer<float>* highBuf = (variantHigh != nullptr) ? variantHigh->audio.get() : nullptr;
    const bool         haveHigh           = (highBuf != nullptr);
    const int          bufHighN           = haveHigh ? highBuf->getNumSamples()  : 0;
    const int          bufHighChannels    = haveHigh ? highBuf->getNumChannels() : 0;
    const float* const readHighL          = (haveHigh && bufHighChannels > 0)
                                                ? highBuf->getReadPointer (0)
                                                : nullptr;
    const float* const readHighR          = (haveHigh && bufHighChannels > 1)
                                                ? highBuf->getReadPointer (1)
                                                : readHighL;
    const int          variantHighLoopStart = haveHigh ? variantHigh->loopStart : 0;
    const int          variantHighLoopEnd   = haveHigh ? variantHigh->loopEnd   : 0;
    const bool         highValid           = haveHigh && (readHighL != nullptr) && (bufHighN > 0);

    for (int i = 0; i < rampSamples; ++i)
    {
        const float ramp = lastEnv * (1.0f - (float) i / (float) rampSamples);

        const float lLow = readVariantWithLoop (readLowL, bufLowN, posLow,
                                                variantLowLoopStart, variantLowLoopEnd);
        const float rLow = (bufLowChannels > 1)
                               ? readVariantWithLoop (readLowR, bufLowN, posLow,
                                                      variantLowLoopStart, variantLowLoopEnd)
                               : lLow;

        float lHigh = 0.0f;
        float rHigh = 0.0f;
        if (highValid)
        {
            lHigh = readVariantWithLoop (readHighL, bufHighN, posHigh,
                                         variantHighLoopStart, variantHighLoopEnd);
            rHigh = (bufHighChannels > 1)
                        ? readVariantWithLoop (readHighR, bufHighN, posHigh,
                                               variantHighLoopStart, variantHighLoopEnd)
                        : lHigh;
        }

        const float yL = (lLow * layerWeightLow + lHigh * layerWeightHigh) * ramp;
        const float yR = (rLow * layerWeightLow + rHigh * layerWeightHigh) * ramp;

        stealTailBufferL[(size_t) i] = yL;
        stealTailBufferR[(size_t) i] = yR;

        posLow += playRateLow;
        wrapLoopPosition (posLow, variantLowLoopStart, variantLowLoopEnd);
        if (highValid)
        {
            posHigh += playRateHigh;
            wrapLoopPosition (posHigh, variantHighLoopStart, variantHighLoopEnd);
        }
    }
}

//==============================================================================
void MicrotonalSamplerVoice::startNote (int   midiNoteNumber,
                                        float velocity,
                                        juce::SynthesiserSound* /*sound*/,
                                        int /*currentPitchWheelPosition*/)
{
    // ---------- 0. Voice-steal detection ----------
    // Hold the prior map alive for the duration of this whole startNote call.
    // renderTailRamp below reads `variantLow` which points into the OLD map's
    // variants vector; without this local hold, swapping `currentMap` to the
    // newly-loaded map (step 1) would drop the prior shared_ptr's last refcount
    // and free the audio buffers `variantLow` indexes into. (REVIEW CR-04.)
    std::shared_ptr<SampleMap> prevMap = currentMap;

    // v1.21.0: the prior note may have been a CC-crossfade note (rendered from
    // `dynLayers`, with `variantLow` unused) or a legacy velocity note. Pick
    // the matching tail-ramp renderer so voice-stealing stays click-free in
    // both modes. ccDynamicsActive / dynLayers still hold the OLD note's state
    // here (the new note's stack is resolved later, in step 7b).
    const bool priorHadContent = (ccDynamicsActive && dynLayerCount > 0)
                              || (variantLow != nullptr);

    if (adsr.isActive() && priorHadContent)
    {
        const int rampSamples = juce::jmin (kMaxStealRamp,
                                            (int) stealTailBufferL.size());
        if (rampSamples > 0)
        {
            if (ccDynamicsActive && dynLayerCount > 0)
                renderTailRampCc (rampSamples);
            else
                renderTailRamp (rampSamples);
            stealTailSamplesRemaining = rampSamples;
        }
    }
    else
    {
        stealTailSamplesRemaining = 0;
    }

    // ---------- 1. Snapshot SampleMap ----------
    if (sampleMapSource != nullptr)
    {
       #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
        currentMap = std::atomic_load (sampleMapSource);
       #else
        currentMap = *sampleMapSource;
       #endif
    }
    else
    {
        currentMap.reset();
    }

    // ---------- 1b. v1.23.2 (W10 / REVIEW WR-02): retire the prior map ----------
    // renderTailRamp above has already fully rendered the steal tail into the
    // scratch buffers, so nothing reads `prevMap`-backed memory past this point:
    // `variantLow`/`dynLayers` are dead here and get repointed into `currentMap`
    // (or nulled on the failure returns below) before their next dereference.
    // If this voice now holds the last reference to a DIFFERENT map (a reload
    // boundary — the processor has already atomic-stored the new map), hand it
    // to the message-thread reaper so the large SampleMap free never runs on the
    // audio thread. Steady-state playback has prevMap == currentMap (no push);
    // with no sink wired (unit tests) prevMap just destructs at end of scope, as
    // before. The reaper defers the actual free to its Timer, so even the
    // now-stale `variantLow` stays backed by live memory until it is reassigned.
    if (retiredMapSink != nullptr && prevMap != nullptr && prevMap != currentMap)
        retiredMapSink->retire (std::move (prevMap));

    if (currentMap == nullptr || currentMap->cells.empty())
    {
        cellLow          = nullptr;
        cellHigh         = nullptr;
        variantLow       = nullptr;
        variantHigh      = nullptr;
        ccDynamicsActive = false;   // v1.23.1 (CR-01): match stopNote hard-off — leaving
        dynLayerCount    = 0;       // these set after a CC note lets renderCcCrossfade read
                                    // dynLayers[] into a just-freed SampleMap (use-after-free).
        currentMidiNote  = -1;
        clearCurrentNote();
        return;
    }

    // ---------- 2. Velocity → primary layer index ----------
    const int vel        = juce::jlimit (1, 127, (int) std::round (velocity * 127.0f));
    const int numLayers  = juce::jmax (1, currentMap->numVelocityLayers);
    const int layerWidth = juce::jmax (1, 128 / numLayers);
    const int layerIdx   = juce::jlimit (0, numLayers - 1, (vel - 1) / layerWidth);

    // ---------- 2b. v1.14.0: capture technique cursor at note-on ----------
    // The cursor is mutated by KS/CC/PC routing in PluginProcessor::processBlock
    // (atomic store) before the synth's renderNextBlock dispatches new notes
    // for this block — so a same-block "switch then play" sequence sees the
    // newly-stored value. Once captured here it is frozen for the rest of
    // the note's lifetime; in-flight voices are immune to subsequent
    // technique flips (RT-safety contract from the v1.14.0 plan).
    startTechnique = (pendingTechniqueSource != nullptr)
        ? juce::jlimit (0, kMaxTechniques - 1,
                        pendingTechniqueSource->load (std::memory_order_acquire))
        : 0;

    // ---------- 3. findCell lookup (primary "low" layer) ----------
    // v1.14.0: triplet lookup. SampleMap::findCell falls back to technique=0
    // ("ord") when the requested slot is empty, so partially-populated
    // technique sets still play.
    // v1.17.2: layer-tolerant lookup — if the velocity-bucketed layer is empty
    // (e.g. a single-dynamic library that landed every slot on one non-zero
    // layer), fall back to the NEAREST populated layer instead of going silent.
    // The crossfade-partner lookup below stays exact (findCell), so an empty
    // adjacent layer correctly means "no crossfade".
    cellLow = currentMap->findCellNearestLayer (midiNoteNumber, layerIdx, startTechnique);
    if (cellLow == nullptr || cellLow->variants.empty())
    {
        cellLow          = nullptr;
        cellHigh         = nullptr;
        variantLow       = nullptr;
        variantHigh      = nullptr;
        ccDynamicsActive = false;   // v1.23.1 (CR-01): match stopNote hard-off — leaving
        dynLayerCount    = 0;       // these set after a CC note lets renderCcCrossfade read
                                    // dynLayers[] into a just-freed SampleMap (use-after-free).
        currentMidiNote  = -1;
        clearCurrentNote();
        return;
    }

    currentMidiNote = midiNoteNumber;

    // ---------- 4. Base frequency from TuningEngine (with ET fallback) ----------
    currentFrequency = (tuningEngine != nullptr)
                           ? tuningEngine->getFrequency (midiNoteNumber)
                           : referenceFrequencyForNote (midiNoteNumber);

    // ---------- 5. Apply Note Expression delta ----------
    if (pendingTuningSource != nullptr)
        currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
            *pendingTuningSource, midiNoteNumber, currentFrequency);

    // ---------- 6. Layer crossfade geometry ----------
    float velCrossfade = 1.0f;
    if (parameters != nullptr)
    {
        if (auto* xfp = parameters->getRawParameterValue ("velocity_crossfade"))
            velCrossfade = juce::jlimit (0.0f, 1.0f, xfp->load());
    }

    cellHigh        = nullptr;
    variantHigh     = nullptr;
    layerWeightLow  = 1.0f;
    layerWeightHigh = 0.0f;
    posHigh         = 0.0;
    playRateHigh    = 1.0;

    if (numLayers >= 2 && velCrossfade > 0.0f)
    {
        const float halfWidth   = (float) layerWidth * 0.5f;
        const float layerCenter = ((float) layerIdx + 0.5f) * (float) layerWidth;
        const float d           = (float) vel - layerCenter;
        const float absD        = std::abs (d);
        const float fw          = velCrossfade * halfWidth;
        const float innerEdge   = halfWidth - fw;

        if (fw > 0.0f && absD >= innerEdge)
        {
            const int adjacentIdx = (d < 0.0f) ? layerIdx - 1
                                               : layerIdx + 1;

            if (adjacentIdx >= 0 && adjacentIdx < numLayers)
            {
                // v1.14.0: crossfade partner MUST share technique with cellLow
                // (the resolved technique — may be `startTechnique` or 0 if
                // we hit the fallback path). Without this guard, mixing two
                // different techniques' samples would produce an audibly
                // wrong articulation crossfade.
                const int xfadeTech = cellLow->technique;
                if (auto* cellAdj = currentMap->findCell (midiNoteNumber, adjacentIdx, xfadeTech))
                {
                    if (! cellAdj->variants.empty()
                        && cellAdj->technique == xfadeTech)
                    {
                        const float x  = 0.5f * (absD - innerEdge) / fw;
                        const auto  ws = equalPowerWeights (x);

                        cellHigh        = cellAdj;
                        layerWeightLow  = ws.first;
                        layerWeightHigh = ws.second;
                    }
                }
            }
        }
    }

    // ---------- 6a. v1.23.0: per-technique / per-layer loudness trim ----------
    // Fold the (technique, layer) trim into the equal-power layer weights. This
    // costs nothing per sample AND is inherited automatically by the voice-steal
    // tail (renderTailRamp reuses layerWeightLow/High). `resolvedTech` is the
    // technique that actually sounded — cellLow->technique — which equals
    // startTechnique unless findCellNearestLayer fell back to "ord". cellHigh
    // (when present) shares that technique (the xfade guard above enforces it)
    // but may sit on a different velocity layer, so it gets its own layer trim.
    if (trimTable != nullptr)
    {
        const int velResolvedTech = cellLow->technique;
        layerWeightLow *= trimTable->gainFor (velResolvedTech, cellLow->velocityLayer);
        if (cellHigh != nullptr)
            layerWeightHigh *= trimTable->gainFor (velResolvedTech, cellHigh->velocityLayer);
    }

    // ---------- 6b. v1.8.0: variant selection per cell ----------
    RoundRobinMode rrMode = RoundRobinMode::RandomNoRepeat;
    if (parameters != nullptr)
    {
        if (auto* rmp = parameters->getRawParameterValue ("rr_mode"))
        {
            const int idx = juce::jlimit (0, 2, (int) std::round (rmp->load()));
            rrMode = (RoundRobinMode) idx;
        }
    }

    const int idxLow = selectVariantIndex (*cellLow, rrMode);
    variantLow = &cellLow->variants[(size_t) juce::jlimit (0, (int) cellLow->variants.size() - 1, idxLow)];

    if (cellHigh != nullptr)
    {
        const int idxHigh = selectVariantIndex (*cellHigh, rrMode);
        variantHigh = &cellHigh->variants[(size_t) juce::jlimit (0, (int) cellHigh->variants.size() - 1, idxHigh)];
    }

    // ---------- 7. Compute per-variant playRates ----------
    const double hostSR = getSampleRate();
    playRateLow = computePlayRateForVariant (*variantLow, cellLow->midiNote,
                                             currentFrequency, hostSR);
    if (variantHigh != nullptr && cellHigh != nullptr)
        playRateHigh = computePlayRateForVariant (*variantHigh, cellHigh->midiNote,
                                                  currentFrequency, hostSR);

    // ---------- 7b. v1.21.0: CC Crossfade dynamics resolution ----------
    // When Dynamics Mode = CC Crossfade, ignore velocity for the dynamic axis
    // and instead resolve EVERY populated velocity layer for this note's
    // resolved technique into `dynLayers`. CC 11 then morphs across them at
    // render time. The velocity-path state (cellLow/High etc.) computed above
    // is left intact but unused while ccDynamicsActive — it provides the
    // "any cell at all?" guard already passed (cellLow != nullptr) and the
    // resolved technique (cellLow->technique).
    ccDynamicsActive = false;
    dynLayerCount    = 0;

    const bool ccMode = (dynamicsModeParam != nullptr)
                     && (dynamicsModeParam->load() > 0.5f);
    if (ccMode)
    {
        // Gather sharing ONE technique (no articulation mixing across the
        // loudness axis). cellLow->technique is what actually resolved — the
        // requested startTechnique, or technique 0 if findCellNearestLayer
        // fell back to "ord". gatherLayerCells does not itself fall back.
        const int resolvedTech = cellLow->technique;
        const SampleCell* gathered[(size_t) kMaxDynLayers] = { nullptr };
        const int n = currentMap->gatherLayerCells (midiNoteNumber, resolvedTech,
                                                     gathered, kMaxDynLayers);

        for (int k = 0; k < n; ++k)
        {
            const SampleCell* c = gathered[k];
            if (c == nullptr || c->variants.empty())
                continue;

            // v1.23.2 (IN-02): snapshot this cell's RR counter BEFORE selecting.
            // selectVariantIndex advances the persistent per-cell counter; if the
            // selected variant is degenerate (empty buffer) and we skip it below,
            // restore the counter so a failed/empty variant does not consume an
            // RR step and skew the per-cell progression. The counter is a relaxed
            // atomic mutated only on the audio thread, so this save/restore is
            // race-free.
            const int counterIdx = juce::jlimit (0, kRrCounterSize - 1,
                packRrCounterIndex (c->midiNote, c->velocityLayer, c->technique));
            const uint8_t prevCounter = (rrCounters != nullptr)
                ? (*rrCounters)[(size_t) counterIdx].load (std::memory_order_relaxed)
                : (uint8_t) 0xFFu;

            const int            idx = selectVariantIndex (*c, rrMode);
            const SampleVariant* var =
                &c->variants[(size_t) juce::jlimit (0, (int) c->variants.size() - 1, idx)];

            // Skip degenerate (empty-buffer) variants so the render path never
            // brackets a silent layer — keeps the dynamic morph continuous.
            if (var->audio == nullptr || var->audio->getNumSamples() <= 0)
            {
                if (rrCounters != nullptr)                                    // v1.23.2 (IN-02)
                    (*rrCounters)[(size_t) counterIdx].store (prevCounter,
                                                              std::memory_order_relaxed);
                continue;
            }

            auto& dl   = dynLayers[(size_t) dynLayerCount];
            dl.variant  = var;
            dl.pos      = 0.0;
            dl.playRate = computePlayRateForVariant (*var, c->midiNote,
                                                     currentFrequency, hostSR);
            // v1.23.0: per-(technique,layer) trim for this gathered layer. All
            // gathered cells share `resolvedTech`; each contributes its own
            // velocityLayer's trim. Folded once here (note-on), applied per
            // sample in renderCcCrossfade / renderTailRampCc.
            dl.trimLin  = (trimTable != nullptr)
                              ? trimTable->gainFor (resolvedTech, c->velocityLayer)
                              : 1.0f;
            ++dynLayerCount;
        }

        // Active whenever we have at least one valid layer. With exactly one
        // populated layer there is nothing to crossfade — the render path
        // (renderCcCrossfade, `single == true`) applies only the v1.22 dB-linear
        // loudness ramp `dynGain = decibelsToGain(rangeDb·(d−1))` on that layer,
        // so CC 11 still shapes dynamics (a single-dynamic library is otherwise
        // flat in CC mode, which would be a regression vs the Velocity-mode
        // post-mix gain). v1.23.2 (IN-03): comment corrected — the pre-v1.22
        // "squared CC gain" fallback no longer exists.
        ccDynamicsActive = (dynLayerCount >= 1);

        // Seed the dynamic position from the CURRENT Expression / CC 11 value
        // (NOT velocity — design contract). setCurrentAndTargetValue so the
        // first block doesn't ramp up from a stale position.
        const float d0 = (expressionParam != nullptr)
                           ? juce::jlimit (0.0f, 1.0f, expressionParam->load())
                           : 1.0f;
        dynamicsSmoother.setCurrentAndTargetValue (d0);
    }

    // ---------- 8. Read APVTS ADSR values ONCE ----------
    // v1.11.3: use atomic pointers cached in prepareToPlay to avoid
    // dereferencing a null getRawParameterValue() return on the audio thread.
    // (REVIEW DSP CRITICAL #2.)
    if (attackParam  != nullptr && decayParam   != nullptr
     && sustainParam != nullptr && releaseParam != nullptr)
    {
        adsr.setParameters ({ attackParam->load(),
                              decayParam->load(),
                              sustainParam->load(),
                              releaseParam->load() });
    }

    // ---------- 9. Reset cursors and trigger ADSR ----------
    posLow  = 0.0;
    posHigh = 0.0;
    adsr.reset();
    adsr.noteOn();
}

//==============================================================================
void MicrotonalSamplerVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        cellLow          = nullptr;
        cellHigh         = nullptr;
        variantLow       = nullptr;
        variantHigh      = nullptr;
        ccDynamicsActive = false;   // v1.21.0
        dynLayerCount    = 0;       // v1.21.0
        currentMidiNote  = -1;
        clearCurrentNote();
    }
}

//==============================================================================
void MicrotonalSamplerVoice::pitchWheelMoved (int /*newPitchWheelValue*/) {}
void MicrotonalSamplerVoice::controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/) {}

//==============================================================================
void MicrotonalSamplerVoice::renderNextBlock (juce::AudioBuffer<float>& out,
                                              int startSample,
                                              int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    // ---------- mix steal tail ----------
    if (stealTailSamplesRemaining > 0
        && ! stealTailBufferL.empty()
        && ! stealTailBufferR.empty())
    {
        const int n      = juce::jmin (stealTailSamplesRemaining, numSamples);
        const int offset = kMaxStealRamp - stealTailSamplesRemaining;
        if (n > 0 && offset >= 0
            && offset + n <= (int) stealTailBufferL.size())
        {
            const int outChans = out.getNumChannels();
            if (outChans > 0)
                out.addFrom (0, startSample, stealTailBufferL.data() + offset, n);
            if (outChans > 1)
                out.addFrom (1, startSample, stealTailBufferR.data() + offset, n);
            stealTailSamplesRemaining -= n;
        }
        else
        {
            stealTailSamplesRemaining = 0;
        }
    }

    // v1.21.0: CC Crossfade dynamics path. Drives the smoother target from the
    // live Expression / CC 11 value (per-block; per-sample ramp inside) and
    // renders the equal-power layer morph. Returns before the legacy
    // velocity-path render below (which stays bit-identical for Velocity mode).
    if (ccDynamicsActive)
    {
        if (expressionParam != nullptr)
            dynamicsSmoother.setTargetValue (juce::jlimit (0.0f, 1.0f,
                                                           expressionParam->load()));
        renderCcCrossfade (out, startSample, numSamples);
        return;
    }

    if (variantLow == nullptr || ! adsr.isActive())
    {
        if (variantLow != nullptr && ! adsr.isActive())
        {
            cellLow         = nullptr;
            cellHigh        = nullptr;
            variantLow      = nullptr;
            variantHigh     = nullptr;
            currentMidiNote = -1;
            clearCurrentNote();
        }
        return;
    }

    const juce::AudioBuffer<float>* lowBuf = variantLow->audio.get();
    const int    bufLowN        = (lowBuf != nullptr) ? lowBuf->getNumSamples()  : 0;
    const int    bufLowChannels = (lowBuf != nullptr) ? lowBuf->getNumChannels() : 0;
    const float* readLowL       = (bufLowChannels > 0) ? lowBuf->getReadPointer (0) : nullptr;
    const float* readLowR       = (bufLowChannels > 1) ? lowBuf->getReadPointer (1) : readLowL;

    if (readLowL == nullptr || bufLowN <= 0)
    {
        cellLow         = nullptr;
        cellHigh        = nullptr;
        variantLow      = nullptr;
        variantHigh     = nullptr;
        currentMidiNote = -1;
        clearCurrentNote();
        return;
    }

    const int    variantLowLoopStart = variantLow->loopStart;
    const int    variantLowLoopEnd   = variantLow->loopEnd;

    const juce::AudioBuffer<float>* highBuf = (variantHigh != nullptr) ? variantHigh->audio.get() : nullptr;
    const bool         haveHigh             = (highBuf != nullptr);
    const int          bufHighN             = haveHigh ? highBuf->getNumSamples()  : 0;
    const int          bufHighChannels      = haveHigh ? highBuf->getNumChannels() : 0;
    const float* const readHighL            = (haveHigh && bufHighChannels > 0)
                                                  ? highBuf->getReadPointer (0)
                                                  : nullptr;
    const float* const readHighR            = (haveHigh && bufHighChannels > 1)
                                                  ? highBuf->getReadPointer (1)
                                                  : readHighL;
    const int          variantHighLoopStart = haveHigh ? variantHigh->loopStart : 0;
    const int          variantHighLoopEnd   = haveHigh ? variantHigh->loopEnd   : 0;

    const bool highValid = haveHigh && (readHighL != nullptr) && (bufHighN > 0);

    const int outChans = out.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        const float env = adsr.getNextSample();

        const float lLow = readVariantWithLoop (readLowL, bufLowN, posLow,
                                                variantLowLoopStart, variantLowLoopEnd);
        const float rLow = (bufLowChannels > 1)
                               ? readVariantWithLoop (readLowR, bufLowN, posLow,
                                                      variantLowLoopStart, variantLowLoopEnd)
                               : lLow;

        float lHigh = 0.0f;
        float rHigh = 0.0f;
        if (highValid)
        {
            lHigh = readVariantWithLoop (readHighL, bufHighN, posHigh,
                                         variantHighLoopStart, variantHighLoopEnd);
            rHigh = (bufHighChannels > 1)
                        ? readVariantWithLoop (readHighR, bufHighN, posHigh,
                                               variantHighLoopStart, variantHighLoopEnd)
                        : lHigh;
        }

        const float yL = (lLow * layerWeightLow + lHigh * layerWeightHigh) * env;
        const float yR = (rLow * layerWeightLow + rHigh * layerWeightHigh) * env;

        if (outChans > 0) out.addSample (0, startSample + i, yL);
        if (outChans > 1) out.addSample (1, startSample + i, yR);

        posLow += playRateLow;
        wrapLoopPosition (posLow, variantLowLoopStart, variantLowLoopEnd);
        if (highValid)
        {
            posHigh += playRateHigh;
            wrapLoopPosition (posHigh, variantHighLoopStart, variantHighLoopEnd);
        }

        if (! adsr.isActive())
        {
            cellLow         = nullptr;
            cellHigh        = nullptr;
            variantLow      = nullptr;
            variantHigh     = nullptr;
            currentMidiNote = -1;
            clearCurrentNote();
            return;
        }
    }
}

//==============================================================================
// v1.21.0: CC Crossfade dynamics render. Every populated layer in `dynLayers`
// advances each sample (time-synced → click-free bracket entry); only the two
// layers bracketing the live, smoothed dynamic position `d` are summed with
// equal-power weights (a pure timbre morph). v1.22.0: a dB-linear loudness
// ramp `dynGain = decibelsToGain(rangeDb·(d−1))` is layered on top so CC 11
// shapes BOTH timbre and loudness — `rangeDb` (the "Dynamic Range" param) is
// the dB span pp(d=0)→ff(d=1). 0 dB → flat (v1.21.0 behaviour); the default
// 20 dB restores a musical pp→ff sweep that flat multi-layer crossfade lacked
// (forte-too-soft / piano-too-loud in Dorico). Applies to single- AND
// multi-layer alike. ADSR `env` applies to the mix; the post-mix Expression
// gain is bypassed in the processor for this mode so dynamics are never
// double-attenuated (the original Dorico pp problem).
void MicrotonalSamplerVoice::renderCcCrossfade (juce::AudioBuffer<float>& out,
                                                int startSample,
                                                int numSamples) noexcept
{
    if (dynLayerCount <= 0 || ! adsr.isActive())
    {
        if (! adsr.isActive())
        {
            cellLow          = nullptr;
            cellHigh         = nullptr;
            variantLow       = nullptr;
            variantHigh      = nullptr;
            ccDynamicsActive = false;
            dynLayerCount    = 0;
            currentMidiNote  = -1;
            clearCurrentNote();
        }
        return;
    }

    // Hoist per-layer read state once (mirrors the legacy path's caching).
    const float* readL  [kMaxDynLayers] = { nullptr };
    const float* readR  [kMaxDynLayers] = { nullptr };
    int          bufN   [kMaxDynLayers] = { 0 };
    int          lpS    [kMaxDynLayers] = { 0 };
    int          lpE    [kMaxDynLayers] = { 0 };
    bool         stereo [kMaxDynLayers] = { false };

    for (int k = 0; k < dynLayerCount; ++k)
    {
        const SampleVariant* var = dynLayers[(size_t) k].variant;
        const juce::AudioBuffer<float>* buf = (var != nullptr) ? var->audio.get() : nullptr;
        const int n  = (buf != nullptr) ? buf->getNumSamples()  : 0;
        const int ch = (buf != nullptr) ? buf->getNumChannels() : 0;
        readL [k] = (ch > 0) ? buf->getReadPointer (0) : nullptr;
        readR [k] = (ch > 1) ? buf->getReadPointer (1) : readL[k];
        bufN  [k] = n;
        lpS   [k] = (var != nullptr) ? var->loopStart : 0;
        lpE   [k] = (var != nullptr) ? var->loopEnd   : 0;
        stereo[k] = (ch > 1);
    }

    const int  outChans = out.getNumChannels();
    const int  lastIdx  = dynLayerCount - 1;
    const bool single   = (dynLayerCount == 1);

    // v1.22.0: dB-linear loudness ramp on top of the equal-power timbre morph.
    // `rangeDb` is the dB span pp(d=0)→ff(d=1): dynGain = decibelsToGain
    // (rangeDb·(d−1)). 0 dB → flat (v1.21.0). Read once per block (RT-safe).
    const float rangeDb = (dynamicRangeParam != nullptr)
                            ? juce::jlimit (0.0f, 40.0f, dynamicRangeParam->load())
                            : 20.0f;

    auto readLayer = [&] (int k, bool right) noexcept -> float
    {
        const float* p = right ? readR[k] : readL[k];
        if (p == nullptr) return 0.0f;
        return readVariantWithLoop (p, bufN[k], dynLayers[(size_t) k].pos,
                                    lpS[k], lpE[k]);
    };

    for (int i = 0; i < numSamples; ++i)
    {
        const float env = adsr.getNextSample();
        const float d   = juce::jlimit (0.0f, 1.0f, dynamicsSmoother.getNextValue());

        int   ia = 0, ib = 0;
        float wA = 1.0f, wB = 0.0f;
        if (! single)
        {
            const float p    = d * (float) lastIdx;
            ia               = juce::jlimit (0, lastIdx, (int) std::floor (p));
            ib               = juce::jmin (ia + 1, lastIdx);
            const float frac = p - (float) ia;
            const auto  w    = equalPowerWeights (frac);
            wA = w.first;
            wB = w.second;
        }

        const float dynGain = juce::Decibels::decibelsToGain (rangeDb * (d - 1.0f)); // v1.22.0

        const float lA = readLayer (ia, false);
        const float rA = stereo[ia] ? readLayer (ia, true) : lA;

        float lB = 0.0f, rB = 0.0f;
        if (ib != ia)
        {
            lB = readLayer (ib, false);
            rB = stereo[ib] ? readLayer (ib, true) : lB;
        }

        // v1.23.0: per-(technique,layer) trim for the two bracketed layers.
        // Pre-folded at note-on (technique master × this layer's trim). The
        // bracket (ia/ib) moves with CC 11, so fetch per sample — a member read.
        const float gA = dynLayers[(size_t) ia].trimLin;
        const float gB = dynLayers[(size_t) ib].trimLin;

        const float yL = (lA * gA * wA + lB * gB * wB) * env * dynGain;
        const float yR = (rA * gA * wA + rB * gB * wB) * env * dynGain;

        if (outChans > 0) out.addSample (0, startSample + i, yL);
        if (outChans > 1) out.addSample (1, startSample + i, yR);

        // Advance EVERY layer so the bracket can change mid-note without a
        // discontinuity (the newly-entered layer is already time-aligned).
        for (int k = 0; k < dynLayerCount; ++k)
        {
            dynLayers[(size_t) k].pos += dynLayers[(size_t) k].playRate;
            wrapLoopPosition (dynLayers[(size_t) k].pos, lpS[k], lpE[k]);
        }

        if (! adsr.isActive())
        {
            cellLow          = nullptr;
            cellHigh         = nullptr;
            variantLow       = nullptr;
            variantHigh      = nullptr;
            ccDynamicsActive = false;
            dynLayerCount    = 0;
            currentMidiNote  = -1;
            clearCurrentNote();
            return;
        }
    }
}

//==============================================================================
// v1.21.0: CC-crossfade voice-steal tail. The CC analogue of renderTailRamp —
// renders a 5 ms linear-down ramp of the current bracketed mix into the steal
// buffers. The dynamic position is FROZEN at the smoother's current value (the
// ramp is far too short for CC motion to matter), matching renderTailRamp's
// single-`lastEnv` snapshot model.
void MicrotonalSamplerVoice::renderTailRampCc (int rampSamples) noexcept
{
    const bool prereqsMet = (rampSamples >= 2)
                         && (dynLayerCount > 0)
                         && ! stealTailBufferL.empty()
                         && ! stealTailBufferR.empty();

    if (! prereqsMet)
    {
        const int n = juce::jmax (0,
                                  juce::jmin (rampSamples,
                                              (int) stealTailBufferL.size(),
                                              (int) stealTailBufferR.size()));
        for (int i = 0; i < n; ++i)
        {
            stealTailBufferL[(size_t) i] = 0.0f;
            stealTailBufferR[(size_t) i] = 0.0f;
        }
        return;
    }

    const float lastEnv = adsr.getNextSample();
    const float d       = juce::jlimit (0.0f, 1.0f, dynamicsSmoother.getCurrentValue());
    const int   lastIdx = dynLayerCount - 1;
    const bool  single  = (dynLayerCount == 1);
    const float rangeDb = (dynamicRangeParam != nullptr)   // v1.22.0 (matches renderCcCrossfade)
                            ? juce::jlimit (0.0f, 40.0f, dynamicRangeParam->load())
                            : 20.0f;

    int   ia = 0, ib = 0;
    float wA = 1.0f, wB = 0.0f;
    if (! single)
    {
        const float p    = d * (float) lastIdx;
        ia               = juce::jlimit (0, lastIdx, (int) std::floor (p));
        ib               = juce::jmin (ia + 1, lastIdx);
        const float frac = p - (float) ia;
        const auto  w    = equalPowerWeights (frac);
        wA = w.first;
        wB = w.second;
    }
    const float dynGain = juce::Decibels::decibelsToGain (rangeDb * (d - 1.0f)); // v1.22.0
    const float gA      = dynLayers[(size_t) ia].trimLin;  // v1.23.0 (frozen with ia/ib)
    const float gB      = dynLayers[(size_t) ib].trimLin;

    const float* readL  [kMaxDynLayers] = { nullptr };
    const float* readR  [kMaxDynLayers] = { nullptr };
    int          bufN   [kMaxDynLayers] = { 0 };
    int          lpS    [kMaxDynLayers] = { 0 };
    int          lpE    [kMaxDynLayers] = { 0 };
    bool         stereo [kMaxDynLayers] = { false };
    for (int k = 0; k < dynLayerCount; ++k)
    {
        const SampleVariant* var = dynLayers[(size_t) k].variant;
        const juce::AudioBuffer<float>* buf = (var != nullptr) ? var->audio.get() : nullptr;
        const int n  = (buf != nullptr) ? buf->getNumSamples()  : 0;
        const int ch = (buf != nullptr) ? buf->getNumChannels() : 0;
        readL [k] = (ch > 0) ? buf->getReadPointer (0) : nullptr;
        readR [k] = (ch > 1) ? buf->getReadPointer (1) : readL[k];
        bufN  [k] = n;
        lpS   [k] = (var != nullptr) ? var->loopStart : 0;
        lpE   [k] = (var != nullptr) ? var->loopEnd   : 0;
        stereo[k] = (ch > 1);
    }

    auto readLayer = [&] (int k, bool right) noexcept -> float
    {
        const float* p = right ? readR[k] : readL[k];
        if (p == nullptr) return 0.0f;
        return readVariantWithLoop (p, bufN[k], dynLayers[(size_t) k].pos,
                                    lpS[k], lpE[k]);
    };

    for (int i = 0; i < rampSamples; ++i)
    {
        const float ramp = lastEnv * (1.0f - (float) i / (float) rampSamples);

        const float lA = readLayer (ia, false);
        const float rA = stereo[ia] ? readLayer (ia, true) : lA;
        float lB = 0.0f, rB = 0.0f;
        if (ib != ia)
        {
            lB = readLayer (ib, false);
            rB = stereo[ib] ? readLayer (ib, true) : lB;
        }

        const float yL = (lA * gA * wA + lB * gB * wB) * ramp * dynGain;
        const float yR = (rA * gA * wA + rB * gB * wB) * ramp * dynGain;

        stealTailBufferL[(size_t) i] = yL;
        stealTailBufferR[(size_t) i] = yR;

        for (int k = 0; k < dynLayerCount; ++k)
        {
            dynLayers[(size_t) k].pos += dynLayers[(size_t) k].playRate;
            wrapLoopPosition (dynLayers[(size_t) k].pos, lpS[k], lpE[k]);
        }
    }
}
