/*
  ==============================================================================

    MicrotonalSamplerVoice.cpp
    Microtonal Sample Engine - Synthesiser Voice (Phase 2.1)
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
      - Microtonal pitch: TuningEngine::getFrequency with ET fallback for
        Standalone (no host tuning table), then NE delta consumption via
        Ouaricon::NoteExpression::applyPendingTuning (D2-12, R6).
      - Varispeed:
            playRate = (currentFrequency / refFreqOfSlotNote) * (slotSR / hostSR)
        where the slot was recorded at MIDI `slot.midiNote`. In Phase 2.1 the
        in-memory test fixture has slot.midiNote == played note, so refFreq is
        the ET pitch of the played note and (currentFrequency / refFreq) is the
        microtonal speed factor.

    Phase 2.3 will replace the single-slot path with two-slot equal-power layer
    crossfade. Phase 2.4 will add voice-steal tail-ramp scratch buffers.
    Phase 2.5 will activate the loop-wrap branch of cubicInterp + 8-sample
    boundary crossfade.

  ==============================================================================
*/

#include "MicrotonalSamplerVoice.h"

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
    // Cubic-Hermite (Catmull-Rom) interpolation, random-access.
    //
    // Returns the interpolated sample value at fractional position `pos` within
    // the buffer `buf` of length `N`. Uses 4 surrounding samples (y0, y1, y2,
    // y3) at indices (i-1, i, i+1, i+2) where i = floor(pos).
    //
    // Loop semantics:
    //   - If loopEnd > 0: wrap indices via loopStart + ((idx - loopStart) mod
    //     loopLen). Active in Phase 2.5.
    //   - Else: clamp to [0, N-1]. Active in Phase 2.1.
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
}

void MicrotonalSamplerVoice::setCurrentPlaybackSampleRate (double newRate)
{
    juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
    if (newRate > 0.0)
        adsr.setSampleRate (newRate);
}

//==============================================================================
void MicrotonalSamplerVoice::startNote (int   midiNoteNumber,
                                        float velocity,
                                        juce::SynthesiserSound* /*sound*/,
                                        int /*currentPitchWheelPosition*/)
{
    // ---------- 1. Snapshot SampleMap (lifetime owner) ----------
    // RESEARCH pitfall #4: shared_ptr copy is the single atomic op. The
    // processor side will atomic_store from the message thread (Phase 2.2);
    // the simple deref here is acceptable because pointer assignment in the
    // processor is wrapped in std::atomic_store on its side.
    currentMap = (sampleMapSource != nullptr) ? *sampleMapSource : nullptr;

    if (currentMap == nullptr || currentMap->slots.empty())
    {
        // EC-2: no map loaded. Clear and bail.
        currentSlot     = nullptr;
        currentMidiNote = -1;
        clearCurrentNote();
        return;
    }

    // ---------- 2. Velocity → layer index ----------
    const int vel        = juce::jlimit (1, 127, (int) std::round (velocity * 127.0f));
    const int numLayers  = juce::jmax (1, currentMap->numVelocityLayers);
    const int layerWidth = juce::jmax (1, 128 / numLayers);
    const int layerIdx   = juce::jlimit (0, numLayers - 1, (vel - 1) / layerWidth);

    // ---------- 3. findSlot lookup ----------
    currentSlot = currentMap->findSlot (midiNoteNumber, layerIdx);
    if (currentSlot == nullptr)
    {
        // EC-1: note out of range / layer empty.
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
    // applyPendingTuning is a free function returning the new frequency
    // (header-inline, Steinberg-free). Caller passes the post-tuning-engine
    // frequency per the docstring's composition rule.
    if (pendingTuningSource != nullptr)
        currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
            *pendingTuningSource, midiNoteNumber, currentFrequency);

    // ---------- 6. Compute varispeed ratio ----------
    //   playRate = (desiredFreq / slotRecordedFreq) × (slotSR / hostSR)
    // The slot was recorded at MIDI `currentSlot->midiNote`; its recorded
    // pitch is the ET frequency of that note. In Phase 2.1's test fixture the
    // slot's MIDI note equals the played note, so the ratio collapses to the
    // microtonal cents factor times the SR ratio.
    const double slotRefFreq = referenceFrequencyForNote (currentSlot->midiNote);
    const double hostSR      = getSampleRate();
    const double slotSR      = currentSlot->sourceSampleRate > 0.0
                                   ? currentSlot->sourceSampleRate
                                   : hostSR;
    playRate = (currentFrequency / slotRefFreq) * (slotSR / hostSR);

    // ---------- 7. Read APVTS ADSR values ONCE (RESEARCH pitfall #2) ----------
    if (parameters != nullptr)
    {
        const float a = parameters->getRawParameterValue ("attack")->load();
        const float d = parameters->getRawParameterValue ("decay")->load();
        const float s = parameters->getRawParameterValue ("sustain")->load();
        const float r = parameters->getRawParameterValue ("release")->load();
        adsr.setParameters ({ a, d, s, r });
    }

    // ---------- 8. Reset cursor and trigger ADSR ----------
    pos = 0.0;
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
        currentSlot     = nullptr;
        currentMidiNote = -1;
        clearCurrentNote();
    }
}

//==============================================================================
void MicrotonalSamplerVoice::pitchWheelMoved (int /*newPitchWheelValue*/)
{
    // Phase 2.1: no-op. (Pitch bend handled via TuningEngine if/when wired by
    // the processor; not part of Phase 2.1 scope.)
}

void MicrotonalSamplerVoice::controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/)
{
    // Phase 2.1: no-op.
}

//==============================================================================
void MicrotonalSamplerVoice::renderNextBlock (juce::AudioBuffer<float>& out,
                                              int startSample,
                                              int numSamples)
{
    // Inactive voice → nothing to render.
    if (currentSlot == nullptr || ! adsr.isActive())
    {
        if (currentSlot != nullptr && ! adsr.isActive())
        {
            // ADSR finished a release tail; release per-note state.
            currentSlot     = nullptr;
            currentMidiNote = -1;
            clearCurrentNote();
        }
        return;
    }

    const int   slotN          = currentSlot->audio.getNumSamples();
    const int   slotChannels   = currentSlot->audio.getNumChannels();
    const float* readL         = (slotChannels > 0)
                                     ? currentSlot->audio.getReadPointer (0)
                                     : nullptr;
    const float* readR         = (slotChannels > 1)
                                     ? currentSlot->audio.getReadPointer (1)
                                     : readL;  // mono → duplicate (D2-10)

    if (readL == nullptr || slotN <= 0)
    {
        currentSlot     = nullptr;
        currentMidiNote = -1;
        clearCurrentNote();
        return;
    }

    const int loopStart = currentSlot->loopStart;
    const int loopEnd   = currentSlot->loopEnd;  // 0 = no loop in Phase 2.1
    const int outChans  = out.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        const float env = adsr.getNextSample();

        // EC-4: end-of-sample with no loop → hold the last sample value × env.
        // (cubicInterp clamps via wrap when loopEnd == 0; positions beyond N-1
        // produce buf[N-1] × env which decays cleanly under ADSR release.)
        const double readPos = (loopEnd == 0)
                                   ? juce::jmin (pos, (double) (slotN - 1))
                                   : pos;

        const float lSamp = cubicInterp (readL, slotN, readPos, loopStart, loopEnd);
        const float rSamp = (slotChannels > 1)
                                ? cubicInterp (readR, slotN, readPos, loopStart, loopEnd)
                                : lSamp;

        const float yL = lSamp * env;
        const float yR = rSamp * env;

        if (outChans > 0) out.addSample (0, startSample + i, yL);
        if (outChans > 1) out.addSample (1, startSample + i, yR);

        // Advance fractional cursor.
        pos += playRate;

        // If the ADSR has finished and we've passed end-of-sample, stop.
        if (! adsr.isActive())
        {
            currentSlot     = nullptr;
            currentMidiNote = -1;
            clearCurrentNote();
            return;
        }
    }
}
