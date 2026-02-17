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

static constexpr double kHalfPi = 1.5707963267948966;

PrismVoice::PrismVoice() = default;

void PrismVoice::setAPVTS (juce::AudioProcessorValueTreeState* apvts)
{
    parameters = apvts;
}

void PrismVoice::setTuningEngine (TuningEngine* engine)
{
    tuningEngine = engine;
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
    filterA.prepare (sampleRate);
    filterB.prepare (sampleRate);
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
    int subOctave = static_cast<int> (parameters->getRawParameterValue ("subOctave")->load());
    subOsc.setShape (subShape);
    subOsc.setOctave (subOctave);
    subOsc.setFrequency (currentFrequency);
    subOsc.reset();

    // Noise
    int noiseType = static_cast<int> (parameters->getRawParameterValue ("noiseType")->load());
    noiseGen.setType (noiseType);
    noiseGen.reset();

    // Reset filters for new note
    filterA.reset();
    filterB.reset();

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

    // Read per-block parameters
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

    // Configure filters
    filterA.setType (filtAType);
    filterA.setResonance (static_cast<double> (filtARes));
    filterA.setDrive (static_cast<double> (filtADrive));

    filterB.setType (filtBType);
    filterB.setResonance (static_cast<double> (filtBRes));
    filterB.setDrive (static_cast<double> (filtBDrive));

    oscA.setPosition (oscAPos);
    oscB.setPosition (oscBPos);

    // Per-oscillator pan gains (equal-power)
    double panANorm = (oscAPan + 1.0) * 0.5;
    double panAL = std::cos (panANorm * kHalfPi);
    double panAR = std::sin (panANorm * kHalfPi);

    double panBNorm = (oscBPan + 1.0) * 0.5;
    double panBL = std::cos (panBNorm * kHalfPi);
    double panBR = std::sin (panBNorm * kHalfPi);

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

        // Oscillator stereo output
        double oscAL, oscAR, oscBL, oscBR;
        oscA.getNextSampleStereo (oscAL, oscAR);
        oscB.getNextSampleStereo (oscBL, oscBR);

        // Apply level and pan
        oscAL *= oscALevel * panAL;
        oscAR *= oscALevel * panAR;
        oscBL *= oscBLevel * panBL;
        oscBR *= oscBLevel * panBR;

        // Mix oscillators
        double mixedL = oscAL * (1.0 - oscMix) + oscBL * oscMix;
        double mixedR = oscAR * (1.0 - oscMix) + oscBR * oscMix;

        // Add noise to oscillator signal (routed to filter input)
        if (noiseLevel > 0.001f)
        {
            double noise = noiseGen.getNextSample() * noiseLevel;
            mixedL += noise;
            mixedR += noise;
        }

        // Filter envelope modulation on cutoff
        double baseCutoffA = static_cast<double> (filtACutoff);
        double baseCutoffB = static_cast<double> (filtBCutoff);

        double modulatedCutoffA = baseCutoffA * std::pow (2.0, filtEnvVal * filtEnvDepth * 4.0);
        double modulatedCutoffB = baseCutoffB * std::pow (2.0, filtEnvVal * filtEnvDepth * 4.0);

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

        filterA.setCutoff (modulatedCutoffA);
        filterB.setCutoff (modulatedCutoffB);

        // Filter routing (process mono signal for filter, maintain stereo after)
        double monoInput = (mixedL + mixedR) * 0.5;
        double filteredMono;

        if (filtRouting == 0) // Serial: A -> B
        {
            double filtered = filterA.processSample (monoInput);
            filteredMono = filterB.processSample (filtered);
        }
        else // Parallel: A + B
        {
            filteredMono = filterA.processSample (monoInput) + filterB.processSample (monoInput);
        }

        // Reconstruct stereo from filtered mono using original stereo balance
        double stereoBalance = (std::abs (mixedL) + std::abs (mixedR) > 0.0001)
                                   ? mixedL / (std::abs (mixedL) + std::abs (mixedR))
                                   : 0.5;
        double filteredL = filteredMono * stereoBalance * 2.0;
        double filteredR = filteredMono * (1.0 - stereoBalance) * 2.0;

        // Sub oscillator (bypasses filters, direct to output)
        double subSample = 0.0;
        if (subLevel > 0.001f)
        {
            subOsc.setFrequency (glidedFreq);
            subSample = subOsc.getNextSample() * subLevel;
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
}
