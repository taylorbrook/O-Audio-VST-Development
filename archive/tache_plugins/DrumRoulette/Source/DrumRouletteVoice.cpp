#include "DrumRouletteVoice.h"
#include <juce_audio_formats/juce_audio_formats.h>

DrumRouletteVoice::DrumRouletteVoice(int slotNum)
    : slotNumber(slotNum)
{
}

void DrumRouletteVoice::setParameterPointers(std::atomic<float>* attack, std::atomic<float>* decay, std::atomic<float>* pitch,
                                              std::atomic<float>* tilt, std::atomic<float>* volume)
{
    attackParam = attack;
    decayParam = decay;
    pitchParam = pitch;
    tiltFilterParam = tilt;
    volumeParam = volume;
}

void DrumRouletteVoice::setSoloMutePointers(std::atomic<float>* solo, std::atomic<float>* mute, bool* anySoloActive)
{
    soloParam = solo;
    muteParam = mute;
    this->anySoloActive = anySoloActive;
}

bool DrumRouletteVoice::shouldRenderToMainMix() const
{
    // Check mute state
    if (muteParam != nullptr && muteParam->load() > 0.5f)
    {
        // If muted, check if this voice is soloed (solo overrides mute)
        if (soloParam != nullptr && soloParam->load() > 0.5f)
        {
            return true;  // Soloed voice is always audible (even if muted)
        }
        return false;  // Muted and not soloed
    }

    // Check solo state
    if (anySoloActive != nullptr && *anySoloActive)
    {
        // If ANY solo is active, only soloed voices are audible
        if (soloParam != nullptr && soloParam->load() > 0.5f)
        {
            return true;  // This voice is soloed
        }
        return false;  // Another voice is soloed, this one is silent
    }

    // No mute, no solo active - render normally
    return true;
}

void DrumRouletteVoice::setCurrentPlaybackSampleRate(double newRate)
{
    // Store sample rate for DSP components (Phase 4.3)
    voiceSampleRate = newRate;

    // Initialize ADSR with sample rate (Phase 4.2)
    envelope.setSampleRate(newRate);

    // Prepare DSP components (Phase 4.3)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = newRate;
    spec.maximumBlockSize = 512;  // Reasonable default for per-voice processing
    spec.numChannels = 1;  // Per-voice is mono

    lowShelfFilter.prepare(spec);
    highShelfFilter.prepare(spec);
    volumeGain.prepare(spec);

    // Reset filter states
    lowShelfFilter.reset();
    highShelfFilter.reset();
    volumeGain.reset();
}

bool DrumRouletteVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<DrumRouletteSound*>(sound) != nullptr;
}

void DrumRouletteVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    juce::ignoreUnused(midiNoteNumber);

    currentPosition = 0.0;
    noteVelocity = velocity;
    isActive = true;

    // Read envelope parameters atomically (Phase 4.2)
    if (attackParam != nullptr && decayParam != nullptr)
    {
        float attackMs = attackParam->load();
        float decayMs = decayParam->load();

        juce::ADSR::Parameters adsrParams;
        adsrParams.attack = attackMs / 1000.0f;   // Convert ms to seconds
        adsrParams.decay = decayMs / 1000.0f;
        adsrParams.sustain = 0.0f;                // Fixed at 0 (decay-focused percussive)
        adsrParams.release = 0.05f;               // 50ms fixed (prevents clicks on note-off)

        envelope.setParameters(adsrParams);
        envelope.noteOn();
    }

    // Read pitch parameter and calculate playback rate (Phase 4.2)
    if (pitchParam != nullptr)
    {
        float pitchSemitones = pitchParam->load();
        pitchRatio = std::pow(2.0f, pitchSemitones / 12.0f);
    }
    else
    {
        pitchRatio = 1.0f;  // Default: no pitch shift
    }

    // Update tilt filter coefficients (Phase 4.3)
    if (tiltFilterParam != nullptr)
    {
        float tiltDb = tiltFilterParam->load();
        float tiltGain = juce::Decibels::decibelsToGain(tiltDb);

        // Low-shelf (below 1kHz): Same polarity as tilt value
        auto lowShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            voiceSampleRate, 1000.0f, 0.707f, tiltGain);
        *lowShelfFilter.coefficients = *lowShelfCoeffs;

        // High-shelf (above 1kHz): Opposite polarity (inverse gain)
        auto highShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            voiceSampleRate, 1000.0f, 0.707f, 1.0f / tiltGain);
        *highShelfFilter.coefficients = *highShelfCoeffs;
    }
}

void DrumRouletteVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        // Trigger envelope release (50ms tail-off)
        envelope.noteOff();
    }
    else
    {
        // Immediate stop
        isActive = false;
        clearCurrentNote();
    }
}

void DrumRouletteVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isActive || sampleBuffer.getNumSamples() == 0)
        return;

    // Check if envelope finished (Phase 4.2)
    if (!envelope.isActive())
    {
        isActive = false;
        clearCurrentNote();
        return;
    }

    // Phase 4.4: Check if voice should render to main mix (solo/mute logic)
    const bool renderToMix = shouldRenderToMainMix();
    const float soloMuteGain = renderToMix ? 1.0f : 0.0f;

    const int numChannels = juce::jmin(outputBuffer.getNumChannels(), sampleBuffer.getNumChannels());
    const int sampleLength = sampleBuffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const int intPosition = static_cast<int>(currentPosition);

        // Check if sample finished playing
        if (intPosition >= sampleLength - 1)
        {
            isActive = false;
            clearCurrentNote();
            break;
        }

        // Get envelope value for this sample (Phase 4.2)
        const float envelopeValue = envelope.getNextSample();

        // Linear interpolation for pitch shifting (Phase 4.2)
        const float frac = currentPosition - static_cast<float>(intPosition);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const float sample0 = sampleBuffer.getSample(channel, intPosition);
            const float sample1 = sampleBuffer.getSample(channel, intPosition + 1);

            // Interpolate between adjacent samples
            float interpolatedSample = sample0 + frac * (sample1 - sample0);

            // Apply velocity and envelope
            float outputValue = interpolatedSample * noteVelocity * envelopeValue;

            // Apply tilt filter (Phase 4.3)
            if (tiltFilterParam != nullptr)
            {
                // Process single sample through filters
                outputValue = lowShelfFilter.processSample(outputValue);
                outputValue = highShelfFilter.processSample(outputValue);
            }

            // Apply volume control (Phase 4.3)
            if (volumeParam != nullptr)
            {
                float volumeDb = volumeParam->load();
                float volumeGainValue = juce::Decibels::decibelsToGain(volumeDb, -100.0f);
                outputValue *= volumeGainValue;
            }

            // Phase 4.4: Apply solo/mute gain to main mix
            outputValue *= soloMuteGain;

            outputBuffer.addSample(channel, startSample + sample, outputValue);
        }

        // Advance position by pitch ratio (Phase 4.2)
        currentPosition += pitchRatio;
    }
}

void DrumRouletteVoice::loadSample(const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader != nullptr)
    {
        const int numChannels = static_cast<int>(reader->numChannels);
        const int numSamples = static_cast<int>(reader->lengthInSamples);

        sampleBuffer.setSize(numChannels, numSamples);
        reader->read(&sampleBuffer, 0, numSamples, 0, true, true);
    }
    else
    {
        // Failed to load - clear sample buffer
        sampleBuffer.setSize(0, 0);
    }
}
