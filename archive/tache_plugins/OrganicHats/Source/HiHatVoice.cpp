#include "HiHatVoice.h"

HiHatVoice::HiHatVoice(juce::AudioProcessorValueTreeState& apvts)
    : parameters(apvts)
{
}

void HiHatVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;  // Per-voice mono processing

    toneFilter.prepare(spec);
    noiseColorFilter.prepare(spec);
    toneFilter.reset();
    noiseColorFilter.reset();

    // Initialize resonators (Phase 4.3) - Fixed peaks at 7kHz, 10kHz, 13kHz
    const std::array<float, 3> peakFreqs = {7000.0f, 10000.0f, 13000.0f};
    const float Q = 4.0f;  // Moderate resonance for organic body
    const float gainDB = -6.0f;  // Subtle enhancement

    for (int i = 0; i < 3; ++i)
    {
        resonators[i].prepare(spec);
        *resonators[i].coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, peakFreqs[i], Q, juce::Decibels::decibelsToGain(gainDB));
        resonators[i].reset();
    }
}

bool HiHatVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<HiHatSound*>(sound) != nullptr;
}

void HiHatVoice::startNote(int midiNoteNumber, float velocity,
                           juce::SynthesiserSound*, int)
{
    // Determine if this is a closed or open hi-hat
    isClosed = (midiNoteNumber == 36);  // C1 = closed, D1 (38) = open

    // Store velocity as linear gain (0.0-1.0)
    velocityGain = velocity;

    // Configure ADSR based on note type
    if (isClosed)
    {
        // Closed hi-hat: Short decay, no sustain
        // Read CLOSED_DECAY parameter (20-200ms)
        auto* decayParam = parameters.getRawParameterValue("CLOSED_DECAY");
        float decayMs = decayParam->load();

        juce::ADSR::Parameters adsrParams;
        adsrParams.attack = 0.0001f;   // 0.1ms attack
        adsrParams.decay = decayMs / 1000.0f;  // Convert ms to seconds
        adsrParams.sustain = 0.0f;     // No sustain
        adsrParams.release = 0.005f;   // 5ms release

        envelope.setParameters(adsrParams);
    }
    else
    {
        // Open hi-hat: No decay, full sustain, long release
        // Read OPEN_RELEASE parameter (100-1000ms)
        auto* releaseParam = parameters.getRawParameterValue("OPEN_RELEASE");
        float releaseMs = releaseParam->load();

        juce::ADSR::Parameters adsrParams;
        adsrParams.attack = 0.0001f;   // 0.1ms attack
        adsrParams.decay = 0.0f;       // No decay
        adsrParams.sustain = 1.0f;     // Full sustain
        adsrParams.release = releaseMs / 1000.0f;  // Convert ms to seconds

        envelope.setParameters(adsrParams);
    }

    // Trigger envelope
    envelope.noteOn();
}

void HiHatVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        envelope.noteOff();
    }
    else
    {
        // Immediate cutoff
        clearCurrentNote();
        envelope.reset();
    }
}

void HiHatVoice::forceRelease()
{
    // Force envelope to release phase with fast release (<5ms)
    // This is called by choke logic when closed hi-hat cuts open hi-hat
    envelope.noteOff();
}

void HiHatVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                 int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    // Read parameters once per block (atomic reads)
    const char* toneParamID = isClosed ? "CLOSED_TONE" : "OPEN_TONE";
    const char* colorParamID = isClosed ? "CLOSED_NOISE_COLOR" : "OPEN_NOISE_COLOR";

    float toneValue = parameters.getRawParameterValue(toneParamID)->load() / 100.0f;  // Normalize to 0.0-1.0
    float colorValue = parameters.getRawParameterValue(colorParamID)->load() / 100.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // 1. Generate white noise: range [-1.0, 1.0]
        float noiseSample = (noiseGenerator.nextFloat() * 2.0f) - 1.0f;

        // 2. Apply Tone Filter (brightness control)
        // Exponential frequency mapping: 3kHz-15kHz
        float velocityToneMod = velocityGain * 0.3f;  // Up to +30% cutoff modulation
        float baseFreq = 3000.0f * std::pow(5.0f, toneValue);
        float finalCutoff = baseFreq * (1.0f + velocityToneMod);
        finalCutoff = juce::jlimit(20.0f, 20000.0f, finalCutoff);

        // Update Tone Filter coefficients (LP below 50%, HP above 50%)
        if (toneValue < 0.5f)
        {
            *toneFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                currentSampleRate, finalCutoff, 0.707f);
        }
        else
        {
            *toneFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                currentSampleRate, finalCutoff, 0.707f);
        }

        // Process through Tone Filter
        noiseSample = toneFilter.processSample(noiseSample);

        // 3. Apply Noise Color Filter (warmth control)
        // Bypass zone at 50% ±2%
        if (std::abs(colorValue - 0.5f) > 0.02f)
        {
            // Exponential frequency mapping: 5kHz-10kHz
            float colorFreq = 5000.0f * std::pow(2.0f, (colorValue - 0.5f) * 2.0f);
            colorFreq = juce::jlimit(20.0f, 20000.0f, colorFreq);

            // Update Noise Color Filter coefficients (LP below 50%, HP above 50%)
            if (colorValue < 0.5f)
            {
                *noiseColorFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                    currentSampleRate, colorFreq, 0.707f);
            }
            else
            {
                *noiseColorFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                    currentSampleRate, colorFreq, 0.707f);
            }

            // Process through Noise Color Filter
            noiseSample = noiseColorFilter.processSample(noiseSample);
        }
        // else: bypass (no filtering at 50%)

        // 4. Apply resonators (Phase 4.3) - Fixed peaks for organic body
        for (auto& resonator : resonators)
        {
            noiseSample = resonator.processSample(noiseSample);
        }

        // 5. Apply envelope
        float envelopeSample = envelope.getNextSample();

        // 6. Apply velocity scaling
        float outputSample = noiseSample * envelopeSample * velocityGain;

        // 7. Add to output buffer (don't replace - multiple voices may be active)
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addSample(channel, startSample + sample, outputSample);
        }

        // Stop voice if envelope finished
        if (!envelope.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}
