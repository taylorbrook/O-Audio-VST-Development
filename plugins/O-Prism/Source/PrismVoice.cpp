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

static constexpr double kHalfPi = 1.5707963267948966;

PrismVoice::PrismVoice() = default;

void PrismVoice::setAPVTS (juce::AudioProcessorValueTreeState* apvts)
{
    parameters = apvts;
    modMatrix.setAPVTS (apvts);
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
    noteVelocity = velocity;
    currentPitchWheel = currentPitchWheelPosition;

    // Get base frequency from TuningEngine
    if (tuningEngine != nullptr)
        currentFrequency = tuningEngine->getFrequency (midiNoteNumber);
    else
        currentFrequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    if (parameters == nullptr)
        return;

    // Glide
    int glideMode = static_cast<int> (parameters->getRawParameterValue ("glideMode")->load());
    float glideTime = parameters->getRawParameterValue ("glideTime")->load();
    glide.setMode (glideMode);
    glide.setTime (static_cast<double> (glideTime));
    glide.setWasActive (wasActive);
    glide.setTarget (currentFrequency);

    // Osc A: apply coarse/fine tuning
    int coarseA = static_cast<int> (parameters->getRawParameterValue ("oscACoarse")->load());
    float fineA = parameters->getRawParameterValue ("oscAFine")->load();
    double freqA = currentFrequency * std::pow (2.0, (coarseA + fineA / 100.0) / 12.0);
    oscA.setFrequency (freqA);

    // Osc A unison
    int unisonA = static_cast<int> (parameters->getRawParameterValue ("oscAUnison")->load());
    float detuneA = parameters->getRawParameterValue ("oscADetune")->load();
    float widthA = parameters->getRawParameterValue ("oscAWidth")->load();
    oscA.setUnison (unisonA, detuneA, widthA);

    if (! wasActive || glideMode == 0)
    {
        oscA.resetWithRandomPhases();
    }

    // Osc B: apply coarse/fine tuning
    int coarseB = static_cast<int> (parameters->getRawParameterValue ("oscBCoarse")->load());
    float fineB = parameters->getRawParameterValue ("oscBFine")->load();
    double freqB = currentFrequency * std::pow (2.0, (coarseB + fineB / 100.0) / 12.0);
    oscB.setFrequency (freqB);

    // Osc B unison
    int unisonB = static_cast<int> (parameters->getRawParameterValue ("oscBUnison")->load());
    float detuneB = parameters->getRawParameterValue ("oscBDetune")->load();
    float widthB = parameters->getRawParameterValue ("oscBWidth")->load();
    oscB.setUnison (unisonB, detuneB, widthB);

    if (! wasActive || glideMode == 0)
    {
        oscB.resetWithRandomPhases();
    }

    // Sub oscillator
    int subShape = static_cast<int> (parameters->getRawParameterValue ("subShape")->load());
    int subOctaveIndex = static_cast<int> (parameters->getRawParameterValue ("subOctave")->load());
    int subOctave = -(subOctaveIndex + 1); // index 0=-1, 1=-2, 2=-3, 3=-4
    subOsc.setShape (subShape);
    subOsc.setOctave (subOctave);
    subOsc.setFrequency (currentFrequency);
    subOsc.reset();

    // Noise
    int noiseType = static_cast<int> (parameters->getRawParameterValue ("noiseType")->load());
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
    float attack = parameters->getRawParameterValue ("ampAttack")->load();
    float decay = parameters->getRawParameterValue ("ampDecay")->load();
    float sustain = parameters->getRawParameterValue ("ampSustain")->load();
    float release = parameters->getRawParameterValue ("ampRelease")->load();
    ampEnvelope.setParameters ({ attack, decay, sustain, release });
    ampEnvelope.noteOn();

    // Filter ADSR
    float fAttack = parameters->getRawParameterValue ("filtAttack")->load();
    float fDecay = parameters->getRawParameterValue ("filtDecay")->load();
    float fSustain = parameters->getRawParameterValue ("filtSustain")->load();
    float fRelease = parameters->getRawParameterValue ("filtRelease")->load();
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

    // ─── Read per-block parameters ───────────────────────────────
    float oscAPos = parameters->getRawParameterValue ("oscAPos")->load();
    float oscALevel = parameters->getRawParameterValue ("oscALevel")->load();
    float oscAPan = parameters->getRawParameterValue ("oscAPan")->load();
    float oscBPos = parameters->getRawParameterValue ("oscBPos")->load();
    float oscBLevel = parameters->getRawParameterValue ("oscBLevel")->load();
    float oscBPan = parameters->getRawParameterValue ("oscBPan")->load();
    float oscMix = parameters->getRawParameterValue ("oscMix")->load();

    float subLevel = parameters->getRawParameterValue ("subLevel")->load();
    float noiseLevel = parameters->getRawParameterValue ("noiseLevel")->load();

    // Filter parameters
    int filtAType = static_cast<int> (parameters->getRawParameterValue ("filtAType")->load());
    float filtACutoff = parameters->getRawParameterValue ("filtACutoff")->load();
    float filtARes = parameters->getRawParameterValue ("filtARes")->load();
    float filtADrive = parameters->getRawParameterValue ("filtADrive")->load();
    float filtAKeyTrack = parameters->getRawParameterValue ("filtAKeyTrack")->load();

    int filtBType = static_cast<int> (parameters->getRawParameterValue ("filtBType")->load());
    float filtBCutoff = parameters->getRawParameterValue ("filtBCutoff")->load();
    float filtBRes = parameters->getRawParameterValue ("filtBRes")->load();
    float filtBDrive = parameters->getRawParameterValue ("filtBDrive")->load();
    float filtBKeyTrack = parameters->getRawParameterValue ("filtBKeyTrack")->load();

    int filtRouting = static_cast<int> (parameters->getRawParameterValue ("filtRouting")->load());
    float filtEnvDepth = parameters->getRawParameterValue ("filtEnvDepth")->load();

    // LFO parameters (rate + shape only — routing via mod matrix)
    float lfo1Rate = parameters->getRawParameterValue ("lfo1Rate")->load();
    int lfo1Shape = static_cast<int> (parameters->getRawParameterValue ("lfo1Shape")->load());
    float lfo2Rate = parameters->getRawParameterValue ("lfo2Rate")->load();
    int lfo2Shape = static_cast<int> (parameters->getRawParameterValue ("lfo2Shape")->load());
    float lfo3Rate = parameters->getRawParameterValue ("lfo3Rate")->load();
    int lfo3Shape = static_cast<int> (parameters->getRawParameterValue ("lfo3Shape")->load());
    float lfo4Rate = parameters->getRawParameterValue ("lfo4Rate")->load();
    int lfo4Shape = static_cast<int> (parameters->getRawParameterValue ("lfo4Shape")->load());

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

        // Update oscillator frequencies with glide
        int coarseA = static_cast<int> (parameters->getRawParameterValue ("oscACoarse")->load());
        float fineA = parameters->getRawParameterValue ("oscAFine")->load();
        oscA.setFrequency (glidedFreq * std::pow (2.0, (coarseA + fineA / 100.0) / 12.0));

        int coarseB = static_cast<int> (parameters->getRawParameterValue ("oscBCoarse")->load());
        float fineB = parameters->getRawParameterValue ("oscBFine")->load();
        oscB.setFrequency (glidedFreq * std::pow (2.0, (coarseB + fineB / 100.0) / 12.0));

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

        // Noise with modulation
        float modNoiseLevel = juce::jlimit (0.0f, 1.0f,
            noiseLevel + modMatrix.getModOffset (ModDest::NoiseLevel));

        if (modNoiseLevel > 0.001f)
        {
            double noise = noiseGen.getNextSample() * modNoiseLevel;
            mixedL += noise;
            mixedR += noise;
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

        // Sub oscillator with modulation (bypasses filters, direct to output)
        float modSubLevel = juce::jlimit (0.0f, 1.0f,
            subLevel + modMatrix.getModOffset (ModDest::SubLevel));

        double subSample = 0.0;
        if (modSubLevel > 0.001f)
        {
            subOsc.setFrequency (glidedFreq);
            subSample = subOsc.getNextSample() * modSubLevel;
        }

        // Apply envelope and velocity
        double outL = (filteredL + subSample) * envVal * noteVelocity;
        double outR = (filteredR + subSample) * envVal * noteVelocity;

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
