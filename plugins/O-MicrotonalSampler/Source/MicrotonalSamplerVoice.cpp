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

#include <array>
#include <atomic>
#include <cmath>
#include <utility>

namespace
{
    inline double referenceFrequencyForNote (int midiNote) noexcept
    {
        return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
    }

    static inline std::pair<float, float> equalPowerWeights (float x) noexcept
    {
        const float t = juce::jlimit (0.0f, 1.0f, x) * juce::MathConstants<float>::halfPi;
        return { std::cos (t), std::sin (t) };
    }

    static const std::array<std::pair<float, float>, 8>& loopXfadeLut() noexcept
    {
        static const std::array<std::pair<float, float>, 8> lut = []()
        {
            std::array<std::pair<float, float>, 8> a {};
            for (int i = 0; i < 8; ++i)
                a[(size_t) i] = equalPowerWeights ((float) i / 8.0f);
            return a;
        }();
        return lut;
    }

    static inline float cubicInterp (const float* buf, int N, double pos) noexcept
    {
        const int  i      = (int) std::floor (pos);
        const auto offset = (float) (pos - (double) i);

        auto clamp = [N] (int idx) noexcept -> int
        {
            return juce::jlimit (0, N - 1, idx);
        };

        const float y0 = buf[clamp (i - 1)];
        const float y1 = buf[clamp (i)];
        const float y2 = buf[clamp (i + 1)];
        const float y3 = buf[clamp (i + 2)];

        const float halfY0 = 0.5f * y0;
        const float halfY3 = 0.5f * y3;

        return y1 + offset * ((0.5f * y2 - halfY0)
                  + (offset * (((y0 + 2.0f * y2) - (halfY3 + 2.5f * y1))
                  + (offset * ((halfY3 + 1.5f * y1) - (halfY0 + 1.5f * y2))))));
    }

    static inline float readVariantWithLoop (const float* buf, int N, double pos,
                                             int lpStart, int lpEnd) noexcept
    {
        if (lpEnd <= 0)
        {
            const double clamped = juce::jmin (pos, (double) (N - 1));
            return cubicInterp (buf, N, clamped);
        }

        const int lpLen = lpEnd - lpStart;
        if (lpLen <= 0)
        {
            const double clamped = juce::jmin (pos, (double) (N - 1));
            return cubicInterp (buf, N, clamped);
        }

        const double fadeStart = (double) (lpEnd - 8);
        if (pos < fadeStart)
            return cubicInterp (buf, N, pos);

        int xIdx = (int) std::floor (pos - fadeStart);
        if (xIdx < 0) xIdx = 0;
        if (xIdx > 7) xIdx = 7;

        const auto& w = loopXfadeLut()[(size_t) xIdx];

        const float outSample = cubicInterp (buf, N, pos);
        const float inSample  = cubicInterp (buf, N, pos - (double) lpLen + 8.0);

        return outSample * w.first + inSample * w.second;
    }

    static inline void wrapLoopPosition (double& pos, int lpStart, int lpEnd) noexcept
    {
        if (lpEnd <= 0) return;
        const int lpLen = lpEnd - lpStart;
        if (lpLen <= 0) return;
        while (pos >= (double) lpEnd)
            pos -= (double) lpLen;
    }

    static inline double computePlayRateForVariant (const SampleVariant& variant,
                                                    int                  cellMidiNote,
                                                    double               desiredFreq,
                                                    double               hostSR) noexcept
    {
        const double cellRefFreq = referenceFrequencyForNote (cellMidiNote);
        const double slotSR      = variant.sourceSampleRate > 0.0
                                       ? variant.sourceSampleRate
                                       : hostSR;
        return (desiredFreq / cellRefFreq) * (slotSR / hostSR);
    }

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

    const int counterIdx = juce::jlimit (0, kRrCounterSize - 1,
                                         cell.midiNote * 4 + cell.velocityLayer);
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
    if (rampSamples <= 0
        || variantLow == nullptr
        || stealTailBufferL.empty()
        || stealTailBufferR.empty())
    {
        const int n = juce::jmin (rampSamples,
                                  (int) stealTailBufferL.size(),
                                  (int) stealTailBufferR.size());
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
    if (adsr.isActive() && variantLow != nullptr)
    {
        const int rampSamples = juce::jmin (kMaxStealRamp,
                                            (int) stealTailBufferL.size());
        if (rampSamples > 0)
        {
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

    if (currentMap == nullptr || currentMap->cells.empty())
    {
        cellLow         = nullptr;
        cellHigh        = nullptr;
        variantLow      = nullptr;
        variantHigh     = nullptr;
        currentMidiNote = -1;
        clearCurrentNote();
        return;
    }

    // ---------- 2. Velocity → primary layer index ----------
    const int vel        = juce::jlimit (1, 127, (int) std::round (velocity * 127.0f));
    const int numLayers  = juce::jmax (1, currentMap->numVelocityLayers);
    const int layerWidth = juce::jmax (1, 128 / numLayers);
    const int layerIdx   = juce::jlimit (0, numLayers - 1, (vel - 1) / layerWidth);

    // ---------- 3. findCell lookup (primary "low" layer) ----------
    cellLow = currentMap->findCell (midiNoteNumber, layerIdx);
    if (cellLow == nullptr || cellLow->variants.empty())
    {
        cellLow         = nullptr;
        cellHigh        = nullptr;
        variantLow      = nullptr;
        variantHigh     = nullptr;
        currentMidiNote = -1;
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
                if (auto* cellAdj = currentMap->findCell (midiNoteNumber, adjacentIdx))
                {
                    if (! cellAdj->variants.empty())
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

    // ---------- 8. Read APVTS ADSR values ONCE ----------
    if (parameters != nullptr)
    {
        const float a = parameters->getRawParameterValue ("attack")->load();
        const float d = parameters->getRawParameterValue ("decay")->load();
        const float s = parameters->getRawParameterValue ("sustain")->load();
        const float r = parameters->getRawParameterValue ("release")->load();
        adsr.setParameters ({ a, d, s, r });
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
        cellLow         = nullptr;
        cellHigh        = nullptr;
        variantLow      = nullptr;
        variantHigh     = nullptr;
        currentMidiNote = -1;
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
