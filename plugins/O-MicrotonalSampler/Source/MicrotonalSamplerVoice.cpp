/*
  ==============================================================================

    MicrotonalSamplerVoice.cpp
    Microtonal Sample Engine - Synthesiser Voice (Phase 2.1 + 2.3)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1 implementation:
      - Inline cubic-Hermite (Catmull-Rom) interpolation, random-access semantics
        with optional loop wrap (loop wrap dormant in 2.1; active in 2.5).
        Body matches JUCE's CatmullRomTraits::valueAtOffset (RESEARCH RQ-1, R2).
      - juce::ADSR per voice. setSampleRate wired in prepareToPlay AND
        setCurrentPlaybackSampleRate (RESEARCH pitfall #1). APVTS values read
        ONCE at startNote (RESEARCH pitfall #2).
      - SampleMap acquired by std::shared_ptr copy from the processor's slot at
        startNote (RESEARCH pitfall #4); held for note duration (pitfall #5).
        Phase 2.3: now uses std::atomic_load for the snapshot (TSan-clean).
      - Microtonal pitch: TuningEngine::getFrequency with ET fallback for
        Standalone (no host tuning table), then NE delta consumption via
        Ouaricon::NoteExpression::applyPendingTuning (D2-12, R6).
      - Varispeed:
            playRate = (currentFrequency / refFreqOfSlotNote) * (slotSR / hostSR)
        where the slot was recorded at MIDI `slot.midiNote`.

    Phase 2.3 implementation (this file):
      - Equal-power velocity-layer crossfade (RQ-7 Site 1). At startNote:
          1. Read `velocity_crossfade` once from APVTS.
          2. Compute layerIdx, layerCenter, distanceCenter, fadeWidthSamples.
          3. If within fade region AND adjacent layer exists:
               - Pick adjacent layer (idx-1 if distanceCenter<0, idx+1 if >0).
               - Compute equalPowerWeights(x) where x∈[0,1] across the fade.
               - Set slotHigh / wLow / wHigh / playRateHigh accordingly.
             Else slotHigh = nullptr; wLow=1; wHigh=0.
          4. EC-5 (vel exactly at boundary): naturally yields wLow=wHigh=0.707.
      - renderNextBlock mixes two slots when slotHigh != nullptr:
          (cubicInterp(slotLow,...) * wLow + cubicInterp(slotHigh,...) * wHigh) * env
        Each slot advances its own pos cursor at its own playRate. EC-4 handled
        per slot independently.

    Phase 2.4 implementation (this file):
      - Per-voice steal-tail scratch buffers (`stealTailBufferL/R`) sized in
        `prepareToPlay` to ceil(0.005 * sampleRate) + 16 samples (D2-3, PERF-01).
      - `renderTailRamp(int rampSamples)` private helper renders OLD-note
        audio (preserving dual-slot crossfade) × linear-down ramp from
        last ADSR env to 0, into the scratch buffers. Allocation-free.
      - `startNote` self-detects active steal via `adsr.isActive() && slotLow`
        and calls `renderTailRamp` BEFORE wiping state for the new note.
      - `renderNextBlock` mixes the captured tail additively (out.addFrom)
        BEFORE the early-out check — so EC-1/EC-2 (no slot for new note)
        still play out the captured tail cleanly.
      - JUCE's default findVoiceToSteal kept (R1, D2-2 satisfied by default).
        `synthesiser.setNoteStealingEnabled(true)` already set in processor.

    Phase 2.5 will activate the loop-wrap branch of cubicInterp + 8-sample
    boundary crossfade.

  ==============================================================================
*/

#include "MicrotonalSamplerVoice.h"

#include <atomic>
#include <cmath>
#include <utility>

namespace
{
    //==============================================================================
    // Reference (12-TET) frequency for a MIDI note. Used as the denominator of
    // the varispeed ratio when comparing the desired microtonal frequency to
    // the slot's recorded pitch.
    inline double referenceFrequencyForNote (int midiNote) noexcept
    {
        return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
    }

    //==============================================================================
    // Equal-power crossfade weights (RQ-7). x ∈ [0,1] is the fade position.
    //   x = 0 → (1, 0)        — slotLow only
    //   x = 0.5 → (0.707, 0.707) — equal energy contribution (EC-5 at boundary)
    //   x = 1 → (0, 1)        — slotHigh only
    // Sum-of-squares = cos²(t) + sin²(t) = 1 → constant power, no notch dip.
    // Inlined trig is cheap at note-rate (called once per startNote, not per
    // sample).
    static inline std::pair<float, float> equalPowerWeights (float x) noexcept
    {
        const float t = juce::jlimit (0.0f, 1.0f, x) * juce::MathConstants<float>::halfPi;
        return { std::cos (t), std::sin (t) };
    }

    //==============================================================================
    // Cubic-Hermite (Catmull-Rom) interpolation, random-access.
    //
    // Returns the interpolated sample value at fractional position `pos` within
    // the buffer `buf` of length `N`. Uses 4 surrounding samples (y0, y1, y2,
    // y3) at indices (i-1, i, i+1, i+2) where i = floor(pos).
    //
    // Loop semantics:
    //   - If loopEnd > 0: wrap indices via loopStart + ((idx - loopStart) mod
    //     loopLen). Active in Phase 2.5.
    //   - Else: clamp to [0, N-1]. Active in Phase 2.1+2.3.
    //
    // Body matches JUCE's CatmullRomTraits::valueAtOffset (juce_Interpolators.h
    // lines 118-131) modulo the fact that JUCE's variant uses a circular
    // 4-element ring buffer; ours indexes a flat buffer + wrap function.
    static inline float cubicInterp (const float* buf,
                                     int          N,
                                     double       pos,
                                     int          loopStart,
                                     int          loopEnd) noexcept
    {
        const int  i      = (int) std::floor (pos);
        const auto offset = (float) (pos - (double) i);

        auto wrap = [&] (int idx) noexcept -> int
        {
            if (loopEnd > 0)
            {
                const int loopLen = loopEnd - loopStart;
                if (loopLen <= 0)
                    return juce::jlimit (0, N - 1, idx);
                int rel = (idx - loopStart) % loopLen;
                if (rel < 0) rel += loopLen;
                return loopStart + rel;
            }
            return juce::jlimit (0, N - 1, idx);
        };

        const float y0 = buf[wrap (i - 1)];
        const float y1 = buf[wrap (i)];
        const float y2 = buf[wrap (i + 1)];
        const float y3 = buf[wrap (i + 2)];

        const float halfY0 = 0.5f * y0;
        const float halfY3 = 0.5f * y3;

        return y1 + offset * ((0.5f * y2 - halfY0)
                  + (offset * (((y0 + 2.0f * y2) - (halfY3 + 2.5f * y1))
                  + (offset * ((halfY3 + 1.5f * y1) - (halfY0 + 1.5f * y2))))));
    }

    //==============================================================================
    // Compute playRate for a given slot at the desired output frequency.
    //   playRate = (desiredFreq / slotRecordedFreq) * (slotSR / hostSR)
    // Slot is assumed non-null (caller must check).
    static inline double computePlayRateForSlot (const SampleSlot& slot,
                                                 double            desiredFreq,
                                                 double            hostSR) noexcept
    {
        const double slotRefFreq = referenceFrequencyForNote (slot.midiNote);
        const double slotSR      = slot.sourceSampleRate > 0.0
                                       ? slot.sourceSampleRate
                                       : hostSR;
        return (desiredFreq / slotRefFreq) * (slotSR / hostSR);
    }
} // namespace

//==============================================================================
bool MicrotonalSamplerVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<MicrotonalSamplerSound*> (sound) != nullptr;
}

//==============================================================================
void MicrotonalSamplerVoice::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    // RESEARCH pitfall #1: setSampleRate MUST be called before setParameters
    // (the latter has a jassert precondition).
    adsr.setSampleRate (sampleRate);
    adsr.setParameters ({ 0.005f, 0.1f, 1.0f, 0.3f });
    adsr.reset();

    // Phase 2.4: voice-steal tail-ramp scratch (5 ms + 16-sample safety margin).
    // Pre-allocated on message thread; never resized in render path (PERF-01).
    kMaxStealRamp = (int) std::ceil (0.005 * sampleRate) + 16;
    stealTailBufferL.assign ((size_t) kMaxStealRamp, 0.0f);
    stealTailBufferR.assign ((size_t) kMaxStealRamp, 0.0f);
    stealTailSamplesRemaining = 0;
}

void MicrotonalSamplerVoice::setCurrentPlaybackSampleRate (double newRate)
{
    juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
    if (newRate > 0.0)
        adsr.setSampleRate (newRate);
}

//==============================================================================
// Phase 2.4: render OLD-note audio × linear-down ramp into the steal-tail
// scratch buffers. Called from startNote BEFORE state reset, when the voice
// is already active. Mirrors the dual-slot read pattern of renderNextBlock
// (deliberate duplication — keeps the Phase 2.3 verified path untouched).
//
// Allocation-free, lock-free, deterministic. rampSamples is clamped at the
// call site to <= kMaxStealRamp <= stealTailBufferL.size().
//
// One-sample ADSR advance via getNextSample() at entry: harmless because the
// caller will reset the ADSR for the new note immediately after this returns.
void MicrotonalSamplerVoice::renderTailRamp (int rampSamples) noexcept
{
    // Defensive bounds — caller already clamps, but guard against degenerate
    // state (slotLow nullptr should never happen here since caller checked).
    if (rampSamples <= 0
        || slotLow == nullptr
        || stealTailBufferL.empty()
        || stealTailBufferR.empty())
    {
        // Zero out whatever portion we were asked to fill (defensive).
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

    // Snapshot env at entry; ramp linearly to 0 across rampSamples.
    const float lastEnv = adsr.getNextSample();

    // ---- Resolve per-slot read pointers (mirrors renderNextBlock) ----
    const int    slotLowN        = slotLow->audio.getNumSamples();
    const int    slotLowChannels = slotLow->audio.getNumChannels();
    const float* readLowL        = (slotLowChannels > 0)
                                       ? slotLow->audio.getReadPointer (0)
                                       : nullptr;
    const float* readLowR        = (slotLowChannels > 1)
                                       ? slotLow->audio.getReadPointer (1)
                                       : readLowL;  // mono → duplicate (D2-10)

    if (readLowL == nullptr || slotLowN <= 0)
    {
        for (int i = 0; i < rampSamples; ++i)
        {
            stealTailBufferL[(size_t) i] = 0.0f;
            stealTailBufferR[(size_t) i] = 0.0f;
        }
        return;
    }

    const int slotLowLoopStart = slotLow->loopStart;
    const int slotLowLoopEnd   = slotLow->loopEnd;

    // High slot (optional, dual-slot crossfade preserved during the tail).
    const bool         haveHigh         = (slotHigh != nullptr);
    const int          slotHighN        = haveHigh ? slotHigh->audio.getNumSamples()  : 0;
    const int          slotHighChannels = haveHigh ? slotHigh->audio.getNumChannels() : 0;
    const float* const readHighL        = (haveHigh && slotHighChannels > 0)
                                              ? slotHigh->audio.getReadPointer (0)
                                              : nullptr;
    const float* const readHighR        = (haveHigh && slotHighChannels > 1)
                                              ? slotHigh->audio.getReadPointer (1)
                                              : readHighL;
    const int          slotHighLoopStart = haveHigh ? slotHigh->loopStart : 0;
    const int          slotHighLoopEnd   = haveHigh ? slotHigh->loopEnd   : 0;
    const bool         highValid         = haveHigh && (readHighL != nullptr) && (slotHighN > 0);

    for (int i = 0; i < rampSamples; ++i)
    {
        // Linear ramp lastEnv → 0 across rampSamples. At i=0 → lastEnv;
        // at i=rampSamples-1 → lastEnv * (1/rampSamples) (last non-zero step).
        const float ramp = lastEnv * (1.0f - (float) i / (float) rampSamples);

        // ---- Low slot read (EC-4 hold per renderNextBlock) ----
        const double readPosLow = (slotLowLoopEnd == 0)
                                      ? juce::jmin (posLow, (double) (slotLowN - 1))
                                      : posLow;
        const float lLow = cubicInterp (readLowL, slotLowN, readPosLow,
                                        slotLowLoopStart, slotLowLoopEnd);
        const float rLow = (slotLowChannels > 1)
                               ? cubicInterp (readLowR, slotLowN, readPosLow,
                                              slotLowLoopStart, slotLowLoopEnd)
                               : lLow;

        // ---- High slot read (optional, EC-4 per slot) ----
        float lHigh = 0.0f;
        float rHigh = 0.0f;
        if (highValid)
        {
            const double readPosHigh = (slotHighLoopEnd == 0)
                                           ? juce::jmin (posHigh, (double) (slotHighN - 1))
                                           : posHigh;
            lHigh = cubicInterp (readHighL, slotHighN, readPosHigh,
                                 slotHighLoopStart, slotHighLoopEnd);
            rHigh = (slotHighChannels > 1)
                        ? cubicInterp (readHighR, slotHighN, readPosHigh,
                                       slotHighLoopStart, slotHighLoopEnd)
                        : lHigh;
        }

        // ---- Equal-power mix × ramp (no env multiply — env is folded into ramp) ----
        const float yL = (lLow * layerWeightLow + lHigh * layerWeightHigh) * ramp;
        const float yR = (rLow * layerWeightLow + rHigh * layerWeightHigh) * ramp;

        stealTailBufferL[(size_t) i] = yL;
        stealTailBufferR[(size_t) i] = yR;

        // ---- Advance per-slot cursors ----
        posLow += playRateLow;
        if (highValid)
            posHigh += playRateHigh;
    }
}

//==============================================================================
void MicrotonalSamplerVoice::startNote (int   midiNoteNumber,
                                        float velocity,
                                        juce::SynthesiserSound* /*sound*/,
                                        int /*currentPitchWheelPosition*/)
{
    // ---------- 0. Voice-steal detection (Phase 2.4) ----------
    // If the voice is currently active (ADSR running on a previous note's slot),
    // capture a 5 ms linear-down tail of the OLD note's audio into the steal-
    // tail scratch buffers BEFORE we wipe state for the new note. The renderer
    // will mix this tail additively on top of the new note's render in
    // subsequent processBlock calls.
    if (adsr.isActive() && slotLow != nullptr)
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
        // Voice was idle — no tail to capture. Clear remaining counter (defensive;
        // would already be 0 if previous tail completed cleanly).
        stealTailSamplesRemaining = 0;
    }

    // ---------- 1. Snapshot SampleMap (lifetime owner) ----------
    // RESEARCH pitfall #4: shared_ptr copy is the single atomic op. Phase 2.3
    // upgrades from plain deref to std::atomic_load to match the producer-side
    // std::atomic_store in PluginProcessor (TSan-clean). Both sides are guarded
    // by __cpp_lib_atomic_shared_ptr; the fallback (plain copy) is safe in
    // practice for aligned-pointer reads on x86-64/ARM64.
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

    if (currentMap == nullptr || currentMap->slots.empty())
    {
        // EC-2: no map loaded. Clear and bail.
        slotLow         = nullptr;
        slotHigh        = nullptr;
        currentMidiNote = -1;
        clearCurrentNote();
        return;
    }

    // ---------- 2. Velocity → primary layer index ----------
    const int vel        = juce::jlimit (1, 127, (int) std::round (velocity * 127.0f));
    const int numLayers  = juce::jmax (1, currentMap->numVelocityLayers);
    const int layerWidth = juce::jmax (1, 128 / numLayers);
    const int layerIdx   = juce::jlimit (0, numLayers - 1, (vel - 1) / layerWidth);

    // ---------- 3. findSlot lookup (primary / "low" layer) ----------
    slotLow = currentMap->findSlot (midiNoteNumber, layerIdx);
    if (slotLow == nullptr)
    {
        // EC-1: note out of range / layer empty.
        slotHigh        = nullptr;
        currentMidiNote = -1;
        clearCurrentNote();
        return;
    }

    currentMidiNote = midiNoteNumber;

    // ---------- 4. Base frequency from TuningEngine (with ET fallback) ----------
    // Standalone may not have a tuning table set up; ET fallback keeps the
    // plugin usable in that context.
    currentFrequency = (tuningEngine != nullptr)
                           ? tuningEngine->getFrequency (midiNoteNumber)
                           : referenceFrequencyForNote (midiNoteNumber);

    // ---------- 5. Apply Note Expression delta (D2-12, R6) ----------
    if (pendingTuningSource != nullptr)
        currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
            *pendingTuningSource, midiNoteNumber, currentFrequency);

    // ---------- 6. Compute layer crossfade geometry (Phase 2.3, RQ-7 Site 1) ----------
    // Geometry (intent per PLAN Gate 3 / RQ-7):
    //   - At deep layer center (|d| small) → primary at full weight (1, 0).
    //   - At layer boundary (|d| → halfWidth) → equal-power split (0.707, 0.707).
    //   - velocity_crossfade=0 → no fade region, hard switch at boundaries.
    //   - velocity_crossfade=1 → fade region = entire half-layer (max overlap).
    //
    // Definitions:
    //   halfWidth = layerWidth / 2
    //   d         = vel - layerCenter          // signed, |d| ≤ halfWidth
    //   fw        = velocity_crossfade * halfWidth   // fade region size on each side
    //
    // Inside fade region (|d| ≥ halfWidth - fw):
    //   x = 0.5 * (|d| - (halfWidth - fw)) / fw   // ∈ [0, 0.5]
    //   (wPrim, wAdj) = equalPowerWeights(x)
    //
    // Outer edge (|d| = halfWidth, the layer boundary): x = 0.5 → (0.707, 0.707).
    // Inner edge (|d| = halfWidth - fw): x = 0 → (1, 0). Smooth onset.
    //
    // RESEARCH RQ-7 pseudocode reads "if |d|<fw" but that geometry crossfades
    // at LAYER CENTERS — opposite of the PLAN Gate 3 intent ("Velocity exactly
    // at 64 → both adjacent layers contribute"). RQ-7 was inverted; this site
    // corrects to the boundary-fade geometry the gate verifies.
    //
    // velocity_crossfade is consumed ONCE per note (not smoothed) — the value
    // bakes into wLow/wHigh and persists for the note's lifetime.
    float velCrossfade = 1.0f;
    if (parameters != nullptr)
    {
        if (auto* xfp = parameters->getRawParameterValue ("velocity_crossfade"))
            velCrossfade = juce::jlimit (0.0f, 1.0f, xfp->load());
    }

    slotHigh        = nullptr;
    layerWeightLow  = 1.0f;
    layerWeightHigh = 0.0f;
    posHigh         = 0.0;
    playRateHigh    = 1.0;

    if (numLayers >= 2 && velCrossfade > 0.0f)
    {
        const float halfWidth    = (float) layerWidth * 0.5f;
        const float layerCenter  = ((float) layerIdx + 0.5f) * (float) layerWidth;
        const float d            = (float) vel - layerCenter;                 // signed
        const float absD         = std::abs (d);
        const float fw           = velCrossfade * halfWidth;                  // fade extent on each side
        const float innerEdge    = halfWidth - fw;                            // 0 at xfade=1, halfWidth at xfade=0

        if (fw > 0.0f && absD >= innerEdge)
        {
            // Pick the adjacent layer in the direction d points (positive d
            // → upper neighbour layerIdx+1; negative d → lower neighbour
            // layerIdx-1).
            const int adjacentIdx = (d < 0.0f) ? layerIdx - 1
                                               : layerIdx + 1;

            if (adjacentIdx >= 0 && adjacentIdx < numLayers)
            {
                if (auto* slotAdj = currentMap->findSlot (midiNoteNumber, adjacentIdx))
                {
                    // x ∈ [0, 0.5]. At inner edge of fade: x=0 → (1, 0);
                    // at boundary: x=0.5 → (0.707, 0.707).
                    const float x  = 0.5f * (absD - innerEdge) / fw;
                    const auto  ws = equalPowerWeights (x);

                    slotHigh        = slotAdj;
                    layerWeightLow  = ws.first;   // primary (in-layer)
                    layerWeightHigh = ws.second;  // adjacent
                }
            }
        }
    }

    // ---------- 7. Compute per-slot playRates ----------
    const double hostSR = getSampleRate();
    playRateLow  = computePlayRateForSlot (*slotLow, currentFrequency, hostSR);
    if (slotHigh != nullptr)
        playRateHigh = computePlayRateForSlot (*slotHigh, currentFrequency, hostSR);

    // ---------- 8. Read APVTS ADSR values ONCE (RESEARCH pitfall #2) ----------
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
        slotLow         = nullptr;
        slotHigh        = nullptr;
        currentMidiNote = -1;
        clearCurrentNote();
    }
}

//==============================================================================
void MicrotonalSamplerVoice::pitchWheelMoved (int /*newPitchWheelValue*/)
{
    // Phase 2.1: no-op. (Pitch bend handled via TuningEngine if/when wired by
    // the processor; not part of Phase 2.x scope.)
}

void MicrotonalSamplerVoice::controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/)
{
    // Phase 2.x: no-op.
}

//==============================================================================
void MicrotonalSamplerVoice::renderNextBlock (juce::AudioBuffer<float>& out,
                                              int startSample,
                                              int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    // ---------- Phase 2.4: mix steal tail (additive) ----------
    // stealTailBufferL/R contain OLD-note audio × linear-down ramp captured at
    // startNote time. Mix the still-pending portion additively on top of the
    // caller's buffer before the new-note render runs below. This must run
    // BEFORE the early-out check: even if the new note failed to acquire a
    // slot (slotLow == nullptr after startNote returned on EC-1/EC-2), the
    // captured tail still needs to play out cleanly.
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
            // Defensive: shouldn't happen; reset to recover gracefully.
            stealTailSamplesRemaining = 0;
        }
    }

    // Inactive voice → nothing to render. Both slots may be null or ADSR done.
    if (slotLow == nullptr || ! adsr.isActive())
    {
        if (slotLow != nullptr && ! adsr.isActive())
        {
            // ADSR finished a release tail; release per-note state.
            slotLow         = nullptr;
            slotHigh        = nullptr;
            currentMidiNote = -1;
            clearCurrentNote();
        }
        return;
    }

    // ---------- Resolve per-slot read pointers ----------
    const int    slotLowN        = slotLow->audio.getNumSamples();
    const int    slotLowChannels = slotLow->audio.getNumChannels();
    const float* readLowL        = (slotLowChannels > 0)
                                       ? slotLow->audio.getReadPointer (0)
                                       : nullptr;
    const float* readLowR        = (slotLowChannels > 1)
                                       ? slotLow->audio.getReadPointer (1)
                                       : readLowL;  // mono → duplicate (D2-10)

    if (readLowL == nullptr || slotLowN <= 0)
    {
        slotLow         = nullptr;
        slotHigh        = nullptr;
        currentMidiNote = -1;
        clearCurrentNote();
        return;
    }

    const int    slotLowLoopStart = slotLow->loopStart;
    const int    slotLowLoopEnd   = slotLow->loopEnd;  // 0 = no loop in 2.3

    // High slot is optional (Phase 2.3 dual-slot crossfade).
    const bool         haveHigh         = (slotHigh != nullptr);
    const int          slotHighN        = haveHigh ? slotHigh->audio.getNumSamples()  : 0;
    const int          slotHighChannels = haveHigh ? slotHigh->audio.getNumChannels() : 0;
    const float* const readHighL        = (haveHigh && slotHighChannels > 0)
                                              ? slotHigh->audio.getReadPointer (0)
                                              : nullptr;
    const float* const readHighR        = (haveHigh && slotHighChannels > 1)
                                              ? slotHigh->audio.getReadPointer (1)
                                              : readHighL;
    const int          slotHighLoopStart = haveHigh ? slotHigh->loopStart : 0;
    const int          slotHighLoopEnd   = haveHigh ? slotHigh->loopEnd   : 0;

    // If the high slot pointer is degenerate, gracefully fall back to single-slot.
    const bool highValid = haveHigh && (readHighL != nullptr) && (slotHighN > 0);

    const int outChans = out.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        const float env = adsr.getNextSample();

        // ---- Low slot read (always present at this point) ----
        // EC-4 per slot: end-of-sample with no loop → hold last sample value
        // × env (cubicInterp clamps via its wrap when loopEnd == 0; clamping
        // posLow to slotLowN-1 produces buf[N-1] which decays cleanly under
        // ADSR release).
        const double readPosLow = (slotLowLoopEnd == 0)
                                      ? juce::jmin (posLow, (double) (slotLowN - 1))
                                      : posLow;

        const float lLow = cubicInterp (readLowL, slotLowN, readPosLow,
                                        slotLowLoopStart, slotLowLoopEnd);
        const float rLow = (slotLowChannels > 1)
                               ? cubicInterp (readLowR, slotLowN, readPosLow,
                                              slotLowLoopStart, slotLowLoopEnd)
                               : lLow;

        // ---- High slot read (Phase 2.3, optional) ----
        float lHigh = 0.0f;
        float rHigh = 0.0f;
        if (highValid)
        {
            // EC-4 per slot — independently of low.
            const double readPosHigh = (slotHighLoopEnd == 0)
                                           ? juce::jmin (posHigh, (double) (slotHighN - 1))
                                           : posHigh;

            lHigh = cubicInterp (readHighL, slotHighN, readPosHigh,
                                 slotHighLoopStart, slotHighLoopEnd);
            rHigh = (slotHighChannels > 1)
                        ? cubicInterp (readHighR, slotHighN, readPosHigh,
                                       slotHighLoopStart, slotHighLoopEnd)
                        : lHigh;
        }

        // ---- Equal-power mix and envelope ----
        // Single-slot path: layerWeightLow = 1, layerWeightHigh = 0 → lHigh
        // contribution is exactly zero (and lHigh is also 0.0 because
        // !highValid skipped its read).
        const float yL = (lLow * layerWeightLow + lHigh * layerWeightHigh) * env;
        const float yR = (rLow * layerWeightLow + rHigh * layerWeightHigh) * env;

        if (outChans > 0) out.addSample (0, startSample + i, yL);
        if (outChans > 1) out.addSample (1, startSample + i, yR);

        // ---- Advance fractional cursors (per-slot) ----
        posLow += playRateLow;
        if (highValid)
            posHigh += playRateHigh;

        // If the ADSR has finished, stop. Both slot pointers are reset so
        // subsequent blocks early-out at the top.
        if (! adsr.isActive())
        {
            slotLow         = nullptr;
            slotHigh        = nullptr;
            currentMidiNote = -1;
            clearCurrentNote();
            return;
        }
    }
}
