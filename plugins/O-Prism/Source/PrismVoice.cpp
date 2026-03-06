/*
  ==============================================================================

    PrismVoice.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "PrismVoice.h"
#include "PrismSound.h"
#include "TuningEngine.h"
#include "PluginProcessor.h"
#include "dsp/MathConstants.h"

PrismVoice::PrismVoice() = default;

void PrismVoice::setAPVTS (juce::AudioProcessorValueTreeState* apvts)
{
    parameters = apvts;
    modMatrix.setAPVTS (apvts);

    if (apvts == nullptr)
        return;

    // Cache all parameter pointers once — eliminates hash map lookups in audio path
    pGlideMode     = apvts->getRawParameterValue ("glideMode");
    pGlideTime     = apvts->getRawParameterValue ("glideTime");

    pOscACoarse    = apvts->getRawParameterValue ("oscACoarse");
    pOscAFine      = apvts->getRawParameterValue ("oscAFine");
    pOscAUnison    = apvts->getRawParameterValue ("oscAUnison");
    pOscADetune    = apvts->getRawParameterValue ("oscADetune");
    pOscAWidth     = apvts->getRawParameterValue ("oscAWidth");
    pOscAPos       = apvts->getRawParameterValue ("oscAPos");
    pOscALevel     = apvts->getRawParameterValue ("oscALevel");
    pOscAPan       = apvts->getRawParameterValue ("oscAPan");

    pOscBCoarse    = apvts->getRawParameterValue ("oscBCoarse");
    pOscBFine      = apvts->getRawParameterValue ("oscBFine");
    pOscBUnison    = apvts->getRawParameterValue ("oscBUnison");
    pOscBDetune    = apvts->getRawParameterValue ("oscBDetune");
    pOscBWidth     = apvts->getRawParameterValue ("oscBWidth");
    pOscBPos       = apvts->getRawParameterValue ("oscBPos");
    pOscBLevel     = apvts->getRawParameterValue ("oscBLevel");
    pOscBPan       = apvts->getRawParameterValue ("oscBPan");

    pOscMix        = apvts->getRawParameterValue ("oscMix");

    pSubShape      = apvts->getRawParameterValue ("subShape");
    pSubOctave     = apvts->getRawParameterValue ("subOctave");
    pSubLevel      = apvts->getRawParameterValue ("subLevel");
    pSubRouting    = apvts->getRawParameterValue ("subRouting");
    pNoiseType     = apvts->getRawParameterValue ("noiseType");
    pNoiseLevel    = apvts->getRawParameterValue ("noiseLevel");

    pFiltAType     = apvts->getRawParameterValue ("filtAType");
    pFiltACutoff   = apvts->getRawParameterValue ("filtACutoff");
    pFiltARes      = apvts->getRawParameterValue ("filtARes");
    pFiltADrive    = apvts->getRawParameterValue ("filtADrive");
    pFiltAKeyTrack = apvts->getRawParameterValue ("filtAKeyTrack");

    pFiltBType     = apvts->getRawParameterValue ("filtBType");
    pFiltBCutoff   = apvts->getRawParameterValue ("filtBCutoff");
    pFiltBRes      = apvts->getRawParameterValue ("filtBRes");
    pFiltBDrive    = apvts->getRawParameterValue ("filtBDrive");
    pFiltBKeyTrack = apvts->getRawParameterValue ("filtBKeyTrack");

    pFiltRouting   = apvts->getRawParameterValue ("filtRouting");
    pFiltEnvDepth  = apvts->getRawParameterValue ("filtEnvDepth");

    pAmpAttack     = apvts->getRawParameterValue ("ampAttack");
    pAmpDecay      = apvts->getRawParameterValue ("ampDecay");
    pAmpSustain    = apvts->getRawParameterValue ("ampSustain");
    pAmpRelease    = apvts->getRawParameterValue ("ampRelease");

    pFiltAttack    = apvts->getRawParameterValue ("filtAttack");
    pFiltDecay     = apvts->getRawParameterValue ("filtDecay");
    pFiltSustain   = apvts->getRawParameterValue ("filtSustain");
    pFiltRelease   = apvts->getRawParameterValue ("filtRelease");

    pLfo1Rate      = apvts->getRawParameterValue ("lfo1Rate");
    pLfo1Shape     = apvts->getRawParameterValue ("lfo1Shape");
    pLfo1Sync      = apvts->getRawParameterValue ("lfo1Sync");
    pLfo1Division  = apvts->getRawParameterValue ("lfo1Division");
    pLfo2Rate      = apvts->getRawParameterValue ("lfo2Rate");
    pLfo2Shape     = apvts->getRawParameterValue ("lfo2Shape");
    pLfo2Sync      = apvts->getRawParameterValue ("lfo2Sync");
    pLfo2Division  = apvts->getRawParameterValue ("lfo2Division");
    pLfo3Rate      = apvts->getRawParameterValue ("lfo3Rate");
    pLfo3Shape     = apvts->getRawParameterValue ("lfo3Shape");
    pLfo3Sync      = apvts->getRawParameterValue ("lfo3Sync");
    pLfo3Division  = apvts->getRawParameterValue ("lfo3Division");
    pLfo4Rate      = apvts->getRawParameterValue ("lfo4Rate");
    pLfo4Shape     = apvts->getRawParameterValue ("lfo4Shape");
    pLfo4Sync      = apvts->getRawParameterValue ("lfo4Sync");
    pLfo4Division  = apvts->getRawParameterValue ("lfo4Division");

    pVelocityCurve = apvts->getRawParameterValue ("velocityCurve");
}

void PrismVoice::setTuningEngine (TuningEngine* engine)
{
    tuningEngine = engine;
}

void PrismVoice::setProcessor (OPrismAudioProcessor* proc)
{
    processor = proc;
}

void PrismVoice::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    voiceSampleRate = sampleRate;
    oscA.prepare (sampleRate);
    oscB.prepare (sampleRate);
    subOsc.prepare (sampleRate);
    noiseGen.prepare (sampleRate);
    glide.prepare (sampleRate);
    ampEnvelope.setSampleRate (sampleRate);
    filterEnvelope.setSampleRate (sampleRate);
    filterAL.prepare (sampleRate);
    filterAR.prepare (sampleRate);
    filterBL.prepare (sampleRate);
    filterBR.prepare (sampleRate);
    lfo1.prepare (sampleRate);
    lfo2.prepare (sampleRate);
    lfo3.prepare (sampleRate);
    lfo4.prepare (sampleRate);
}

bool PrismVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<PrismSound*> (sound) != nullptr;
}

void PrismVoice::setWavetableA (const WavetableData* table)
{
    oscA.setWavetable (table);
}

void PrismVoice::setWavetableB (const WavetableData* table)
{
    oscB.setWavetable (table);
}

void PrismVoice::startNote (int midiNoteNumber, float velocity,
                            juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    bool wasActive = getCurrentlyPlayingNote() >= 0;
    currentMidiNote = midiNoteNumber;
    currentPitchWheel = currentPitchWheelPosition;

    // Apply velocity curve transformation
    if (pVelocityCurve != nullptr)
    {
        int curve = static_cast<int> (pVelocityCurve->load());
        switch (curve)
        {
            case 1:  noteVelocity = std::sqrt (velocity);       break; // Soft
            case 2:  noteVelocity = velocity * velocity;         break; // Hard
            case 3:  noteVelocity = 1.0f;                        break; // Fixed
            default: noteVelocity = velocity;                    break; // Linear
        }
    }
    else
    {
        noteVelocity = velocity;
    }

    // Get base frequency from TuningEngine
    if (tuningEngine != nullptr)
        currentFrequency = tuningEngine->getFrequency (midiNoteNumber);
    else
        currentFrequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    if (parameters == nullptr)
        return;

    // Glide
    int glideMode = static_cast<int> (pGlideMode->load());
    float glideTime = pGlideTime->load();
    glide.setMode (glideMode);
    glide.setTime (static_cast<double> (glideTime));
    glide.setWasActive (wasActive);
    glide.setTarget (currentFrequency);

    // Osc A: apply coarse/fine tuning
    int coarseA = static_cast<int> (pOscACoarse->load());
    float fineA = pOscAFine->load();
    double freqA = currentFrequency * std::pow (2.0, (coarseA + fineA / 100.0) / 12.0);
    oscA.setFrequency (freqA);

    // Osc A unison
    int unisonA = static_cast<int> (pOscAUnison->load());
    float detuneA = pOscADetune->load();
    float widthA = pOscAWidth->load();
    oscA.setUnison (unisonA, detuneA, widthA);

    if (! wasActive || glideMode == 0)
    {
        oscA.resetWithRandomPhases();
    }

    // Osc B: apply coarse/fine tuning
    int coarseB = static_cast<int> (pOscBCoarse->load());
    float fineB = pOscBFine->load();
    double freqB = currentFrequency * std::pow (2.0, (coarseB + fineB / 100.0) / 12.0);
    oscB.setFrequency (freqB);

    // Osc B unison
    int unisonB = static_cast<int> (pOscBUnison->load());
    float detuneB = pOscBDetune->load();
    float widthB = pOscBWidth->load();
    oscB.setUnison (unisonB, detuneB, widthB);

    if (! wasActive || glideMode == 0)
    {
        oscB.resetWithRandomPhases();
    }

    // Sub oscillator
    int subShape = static_cast<int> (pSubShape->load());
    int subOctaveIndex = static_cast<int> (pSubOctave->load());
    int subOctave = -(subOctaveIndex + 1); // index 0=-1, 1=-2, 2=-3, 3=-4
    subOsc.setShape (subShape);
    subOsc.setOctave (subOctave);
    subOsc.setFrequency (currentFrequency);
    subOsc.reset();

    // Noise
    int noiseType = static_cast<int> (pNoiseType->load());
    noiseGen.setType (noiseType);
    noiseGen.reset();

    // Reset filters for new note
    filterAL.reset();
    filterAR.reset();
    filterBL.reset();
    filterBR.reset();

    // Reset LFOs for consistent per-note modulation
    lfo1.reset();
    lfo2.reset();
    lfo3.reset();
    lfo4.reset();

    // Amplitude ADSR
    float attack = pAmpAttack->load();
    float decay = pAmpDecay->load();
    float sustain = pAmpSustain->load();
    float release = pAmpRelease->load();
    ampEnvelope.setParameters ({ attack, decay, sustain, release });
    ampEnvelope.noteOn();

    // Filter ADSR
    float fAttack = pFiltAttack->load();
    float fDecay = pFiltDecay->load();
    float fSustain = pFiltSustain->load();
    float fRelease = pFiltRelease->load();
    filterEnvelope.setParameters ({ fAttack, fDecay, fSustain, fRelease });
    filterEnvelope.noteOn();
}

void PrismVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnvelope.noteOff();
        filterEnvelope.noteOff();
    }
    else
    {
        ampEnvelope.reset();
        filterEnvelope.reset();
        clearCurrentNote();
    }
}

void PrismVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                   int startSample, int numSamples)
{
    if (parameters == nullptr || ! ampEnvelope.isActive())
        return;

    // ─── Read per-block parameters (via cached pointers) ──────────
    float oscAPos = pOscAPos->load();
    float oscALevel = pOscALevel->load();
    float oscAPan = pOscAPan->load();
    float oscBPos = pOscBPos->load();
    float oscBLevel = pOscBLevel->load();
    float oscBPan = pOscBPan->load();
    float oscMix = pOscMix->load();

    float subLevel = pSubLevel->load();
    int subRouting = static_cast<int> (pSubRouting->load());
    float noiseLevel = pNoiseLevel->load();

    // Oscillator coarse/fine tuning (integer/float params — don't change per-sample)
    int coarseA = static_cast<int> (pOscACoarse->load());
    float fineA = pOscAFine->load();
    int coarseB = static_cast<int> (pOscBCoarse->load());
    float fineB = pOscBFine->load();
    double pitchRatioA = std::pow (2.0, (coarseA + fineA / 100.0) / 12.0);
    double pitchRatioB = std::pow (2.0, (coarseB + fineB / 100.0) / 12.0);

    // Filter parameters
    int filtAType = static_cast<int> (pFiltAType->load());
    float filtACutoff = pFiltACutoff->load();
    float filtARes = pFiltARes->load();
    float filtADrive = pFiltADrive->load();
    float filtAKeyTrack = pFiltAKeyTrack->load();

    int filtBType = static_cast<int> (pFiltBType->load());
    float filtBCutoff = pFiltBCutoff->load();
    float filtBRes = pFiltBRes->load();
    float filtBDrive = pFiltBDrive->load();
    float filtBKeyTrack = pFiltBKeyTrack->load();

    int filtRouting = static_cast<int> (pFiltRouting->load());
    float filtEnvDepth = pFiltEnvDepth->load();

    // Note division multipliers: how many beats per LFO cycle
    // Index order: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32,
    //              1/1D, 1/2D, 1/4D, 1/8D, 1/16D, 1/32D,
    //              1/1T, 1/2T, 1/4T, 1/8T, 1/16T, 1/32T
    static constexpr float kDivBeats[18] = {
        4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f,                   // straight
        6.0f, 3.0f, 1.5f, 0.75f, 0.375f, 0.1875f,                // dotted (1.5x)
        2.6667f, 1.3333f, 0.6667f, 0.3333f, 0.1667f, 0.0833f     // triplet (2/3x)
    };

    // Read BPM from processor
    double bpm = (processor != nullptr) ? processor->getCurrentBPM() : 120.0;

    // Helper: compute LFO rate (Hz) from sync state
    auto calcLfoRate = [&] (std::atomic<float>* pRate, std::atomic<float>* pSync,
                            std::atomic<float>* pDiv) -> float
    {
        if (pSync->load() > 0.5f)
        {
            int divIdx = juce::jlimit (0, 17, static_cast<int> (pDiv->load()));
            float beats = kDivBeats[divIdx];
            float seconds = static_cast<float> (beats * 60.0 / bpm);
            return 1.0f / seconds;
        }
        return pRate->load();
    };

    // LFO parameters (rate + shape — with optional tempo sync)
    float lfo1Rate = calcLfoRate (pLfo1Rate, pLfo1Sync, pLfo1Division);
    int lfo1Shape = static_cast<int> (pLfo1Shape->load());
    float lfo2Rate = calcLfoRate (pLfo2Rate, pLfo2Sync, pLfo2Division);
    int lfo2Shape = static_cast<int> (pLfo2Shape->load());
    float lfo3Rate = calcLfoRate (pLfo3Rate, pLfo3Sync, pLfo3Division);
    int lfo3Shape = static_cast<int> (pLfo3Shape->load());
    float lfo4Rate = calcLfoRate (pLfo4Rate, pLfo4Sync, pLfo4Division);
    int lfo4Shape = static_cast<int> (pLfo4Shape->load());

    // Configure LFOs
    lfo1.setRate (lfo1Rate);
    lfo1.setShape (static_cast<LFO::Shape> (lfo1Shape));
    lfo2.setRate (lfo2Rate);
    lfo2.setShape (static_cast<LFO::Shape> (lfo2Shape));
    lfo3.setRate (lfo3Rate);
    lfo3.setShape (static_cast<LFO::Shape> (lfo3Shape));
    lfo4.setRate (lfo4Rate);
    lfo4.setShape (static_cast<LFO::Shape> (lfo4Shape));

    // ─── Update mod matrix routing from APVTS (once per block) ───
    modMatrix.updateFromAPVTS();

    // Global mod sources from processor
    float modWheelVal = (processor != nullptr) ? processor->getModWheelValue() : 0.0f;
    float aftertouchVal = (processor != nullptr) ? processor->getAftertouchValue() : 0.0f;

    // Configure filters (L and R share same settings)
    filterAL.setType (filtAType);
    filterAL.setDrive (static_cast<double> (filtADrive));
    filterAR.setType (filtAType);
    filterAR.setDrive (static_cast<double> (filtADrive));

    filterBL.setType (filtBType);
    filterBL.setDrive (static_cast<double> (filtBDrive));
    filterBR.setType (filtBType);
    filterBR.setDrive (static_cast<double> (filtBDrive));

    auto* leftChannel = outputBuffer.getWritePointer (0);
    auto* rightChannel = outputBuffer.getNumChannels() > 1
                             ? outputBuffer.getWritePointer (1)
                             : nullptr;

    for (int sample = startSample; sample < startSample + numSamples; ++sample)
    {
        double envVal = static_cast<double> (ampEnvelope.getNextSample());
        double filtEnvVal = static_cast<double> (filterEnvelope.getNextSample());

        if (! ampEnvelope.isActive())
        {
            clearCurrentNote();
            break;
        }

        // Glide frequency
        double glidedFreq = glide.getNextFrequency();

        // Pitch modulation from mod matrix (±12 semitones at full offset)
        float pitchModOffset = modMatrix.getModOffset (ModDest::Pitch);
        double pitchModRatio = 1.0;
        if (std::abs (pitchModOffset) > 0.0001f)
            pitchModRatio = std::pow (2.0, static_cast<double> (pitchModOffset) * 12.0 / 12.0);

        // Update oscillator frequencies with glide + pitch ratios + pitch mod
        oscA.setFrequency (glidedFreq * pitchRatioA * pitchModRatio);
        oscB.setFrequency (glidedFreq * pitchRatioB * pitchModRatio);

        // ─── Per-sample modulation sources ───────────────────────
        float lfo1Val = lfo1.getNextSample();  // [-1, 1]
        float lfo2Val = lfo2.getNextSample();  // [-1, 1]
        float lfo3Val = lfo3.getNextSample();  // [-1, 1]
        float lfo4Val = lfo4.getNextSample();  // [-1, 1]

        // Set all source values for the mod matrix
        modMatrix.setSourceValue (ModSource::LFO1, lfo1Val);
        modMatrix.setSourceValue (ModSource::LFO2, lfo2Val);
        modMatrix.setSourceValue (ModSource::LFO3, lfo3Val);
        modMatrix.setSourceValue (ModSource::LFO4, lfo4Val);
        modMatrix.setSourceValue (ModSource::AmpEnv, static_cast<float> (envVal));
        modMatrix.setSourceValue (ModSource::FilterEnv, static_cast<float> (filtEnvVal));
        modMatrix.setSourceValue (ModSource::Velocity, noteVelocity);
        modMatrix.setSourceValue (ModSource::NoteNum, currentMidiNote / 127.0f);
        modMatrix.setSourceValue (ModSource::ModWheel, modWheelVal);
        modMatrix.setSourceValue (ModSource::Aftertouch, aftertouchVal);

        // Evaluate all active routes
        modMatrix.evaluate();

        // ─── Apply modulation offsets to parameters ──────────────

        // Osc positions (additive, clamped 0-1)
        float modulatedPosA = juce::jlimit (0.0f, 1.0f,
            oscAPos + modMatrix.getModOffset (ModDest::OscAPos));
        float modulatedPosB = juce::jlimit (0.0f, 1.0f,
            oscBPos + modMatrix.getModOffset (ModDest::OscBPos));
        oscA.setPosition (modulatedPosA);
        oscB.setPosition (modulatedPosB);

        // Oscillator stereo output
        double oscAL, oscAR, oscBL, oscBR;
        oscA.getNextSampleStereo (oscAL, oscAR);
        oscB.getNextSampleStereo (oscBL, oscBR);

        // Osc pan with modulation (equal-power)
        float modPanA = juce::jlimit (-1.0f, 1.0f,
            oscAPan + modMatrix.getModOffset (ModDest::OscAPan));
        float modPanB = juce::jlimit (-1.0f, 1.0f,
            oscBPan + modMatrix.getModOffset (ModDest::OscBPan));

        double panANorm = (modPanA + 1.0) * 0.5;
        double panALGain = std::cos (panANorm * kHalfPi);
        double panARGain = std::sin (panANorm * kHalfPi);

        double panBNorm = (modPanB + 1.0) * 0.5;
        double panBLGain = std::cos (panBNorm * kHalfPi);
        double panBRGain = std::sin (panBNorm * kHalfPi);

        // Apply level and pan
        oscAL *= oscALevel * panALGain;
        oscAR *= oscALevel * panARGain;
        oscBL *= oscBLevel * panBLGain;
        oscBR *= oscBLevel * panBRGain;

        // Osc mix with modulation
        float modOscMix = juce::jlimit (0.0f, 1.0f,
            oscMix + modMatrix.getModOffset (ModDest::OscMix));

        double mixedL = oscAL * (1.0 - modOscMix) + oscBL * modOscMix;
        double mixedR = oscAR * (1.0 - modOscMix) + oscBR * modOscMix;

        // Noise with modulation (true stereo — independent noise per channel)
        float modNoiseLevel = juce::jlimit (0.0f, 1.0f,
            noiseLevel + modMatrix.getModOffset (ModDest::NoiseLevel));

        if (modNoiseLevel > 0.001f)
        {
            double noiseL, noiseR;
            noiseGen.getNextSampleStereo (noiseL, noiseR);
            mixedL += noiseL * modNoiseLevel;
            mixedR += noiseR * modNoiseLevel;
        }

        // ─── Sub oscillator with modulation ────────────────────────
        float modSubLevel = juce::jlimit (0.0f, 1.0f,
            subLevel + modMatrix.getModOffset (ModDest::SubLevel));

        double subSample = 0.0;
        if (modSubLevel > 0.001f)
        {
            subOsc.setFrequency (glidedFreq * pitchModRatio);
            subSample = subOsc.getNextSample() * modSubLevel;
        }

        // Pre-filter sub routing: mix sub into signal before filters
        if (subRouting == 1 && subSample != 0.0)
        {
            mixedL += subSample;
            mixedR += subSample;
        }

        // ─── Filters with modulation ─────────────────────────────

        // Filter envelope modulation on cutoff (dedicated param, always active)
        double baseCutoffA = static_cast<double> (filtACutoff);
        double baseCutoffB = static_cast<double> (filtBCutoff);

        double modulatedCutoffA = baseCutoffA * std::pow (2.0, filtEnvVal * filtEnvDepth * 4.0);
        double modulatedCutoffB = baseCutoffB * std::pow (2.0, filtEnvVal * filtEnvDepth * 4.0);

        // Mod matrix cutoff offsets (multiplicative, octave-scaled)
        float cutoffModA = modMatrix.getModOffset (ModDest::FiltACutoff);
        float cutoffModB = modMatrix.getModOffset (ModDest::FiltBCutoff);
        if (std::abs (cutoffModA) > 0.001f)
            modulatedCutoffA *= std::pow (2.0, static_cast<double> (cutoffModA) * 4.0);
        if (std::abs (cutoffModB) > 0.001f)
            modulatedCutoffB *= std::pow (2.0, static_cast<double> (cutoffModB) * 4.0);

        // Key tracking
        if (filtAKeyTrack > 0.001f)
        {
            double ktOffset = filtAKeyTrack * (currentMidiNote - 60);
            modulatedCutoffA *= std::pow (2.0, ktOffset / 12.0);
        }
        if (filtBKeyTrack > 0.001f)
        {
            double ktOffset = filtBKeyTrack * (currentMidiNote - 60);
            modulatedCutoffB *= std::pow (2.0, ktOffset / 12.0);
        }

        modulatedCutoffA = juce::jlimit (20.0, 20000.0, modulatedCutoffA);
        modulatedCutoffB = juce::jlimit (20.0, 20000.0, modulatedCutoffB);

        filterAL.setCutoff (modulatedCutoffA);
        filterAR.setCutoff (modulatedCutoffA);
        filterBL.setCutoff (modulatedCutoffB);
        filterBR.setCutoff (modulatedCutoffB);

        // Filter resonance with modulation
        float modResA = juce::jlimit (0.0f, 1.0f,
            filtARes + modMatrix.getModOffset (ModDest::FiltARes));
        float modResB = juce::jlimit (0.0f, 1.0f,
            filtBRes + modMatrix.getModOffset (ModDest::FiltBRes));
        filterAL.setResonance (static_cast<double> (modResA));
        filterAR.setResonance (static_cast<double> (modResA));
        filterBL.setResonance (static_cast<double> (modResB));
        filterBR.setResonance (static_cast<double> (modResB));

        // Filter routing (true stereo processing)
        double filteredL, filteredR;

        if (filtRouting == 0) // Serial: A -> B
        {
            filteredL = filterBL.processSample (filterAL.processSample (mixedL));
            filteredR = filterBR.processSample (filterAR.processSample (mixedR));
        }
        else // Parallel: A + B
        {
            filteredL = filterAL.processSample (mixedL) + filterBL.processSample (mixedL);
            filteredR = filterAR.processSample (mixedR) + filterBR.processSample (mixedR);
        }

        // Post-filter sub routing (default): add sub after filters
        double outL = filteredL;
        double outR = filteredR;
        if (subRouting == 0 && subSample != 0.0)
        {
            outL += subSample;
            outR += subSample;
        }

        // Apply envelope and velocity
        outL *= envVal * noteVelocity;
        outR *= envVal * noteVelocity;

        leftChannel[sample] += static_cast<float> (outL);
        if (rightChannel != nullptr)
            rightChannel[sample] += static_cast<float> (outR);
    }
}

void PrismVoice::pitchWheelMoved (int newPitchWheelValue)
{
    currentPitchWheel = newPitchWheelValue;

    if (tuningEngine != nullptr && currentMidiNote >= 0)
    {
        float normalizedBend = (static_cast<float> (newPitchWheelValue) - 8192.0f) / 8192.0f;
        tuningEngine->setPitchBend (currentMidiNote, normalizedBend);
        currentFrequency = tuningEngine->getFrequency (currentMidiNote);
        glide.setTarget (currentFrequency);
    }
}

void PrismVoice::controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/)
{
    // MIDI CC handling is done in PluginProcessor::processBlock
    // ModWheel and Aftertouch values are read from processor atomics
}
