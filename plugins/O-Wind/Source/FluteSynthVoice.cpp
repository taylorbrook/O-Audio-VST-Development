/*
  ==============================================================================

    FluteSynthVoice.cpp
    O-Wind - Physical Modeling Flute Synthesiser Voice
    Ouaricon Audio
    Developer: Taylor Brook

    Per-sample pipeline at 2x oversampled rate implementing the Verge (1995)
    jet-drive model:
    1. JetExciter: breath -> Bernoulli jet velocity + noise + bore feedback
    2. Jet Delay: embouchure-controlled Lagrange3rd delay
    3. JetNonlinearity: tanh saturation at labium
    4. DCBlocker: remove DC offset
    5. BoreWaveguide: bidirectional Thiran delays + loss/reflection/radiation filters
    6. Output + feedback loop (bore feedback stored for next sample)

    All DSP runs at 2x internal rate. Oversampling wraps the entire
    feedback loop per the architecture contract.

  ==============================================================================
*/

#include "FluteSynthVoice.h"
#include "FluteSynthSound.h"
#include "TuningEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>

FluteSynthVoice::FluteSynthVoice (juce::AudioProcessorValueTreeState* apvts,
                                    TuningEngine* tuning)
    : parameters (apvts), tuningEngine (tuning)
{
}

bool FluteSynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<FluteSynthSound*> (sound) != nullptr;
}

void FluteSynthVoice::startNote (int midiNoteNumber, float velocity,
                                  juce::SynthesiserSound*, int currentPitchWheelPos)
{
    currentMidiNote = midiNoteNumber;
    pitchWheelValue = currentPitchWheelPos;

    // Calculate pitch bend
    pitchBendSemitones = ((static_cast<float> (pitchWheelValue) - 8192.0f) / 8192.0f) * pitchBendRange;

    // Get frequency from tuning engine
    if (tuningEngine != nullptr)
        currentFrequency = static_cast<float> (tuningEngine->getFrequency (midiNoteNumber));
    else
        currentFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));

    // Apply pitch bend
    float bendedFreq = currentFrequency * std::pow (2.0f, pitchBendSemitones / 12.0f);

    // Calculate total loop delay (bore + jet) at internal (oversampled) sample rate
    // Filter group delay is compensated dynamically per-block (not static)
    float totalLoopDelay = static_cast<float> (internalSampleRate) / bendedFreq;
    totalLoopDelay = std::max (4.0f, totalLoopDelay);

    // Store total loop delay (split between bore and jet in render loop)
    totalDelaySmoothed.setCurrentAndTargetValue (totalLoopDelay);

    // Set initial bore delay estimate based on current embouchure
    float initEmb = (parameters != nullptr)
        ? parameters->getRawParameterValue ("embouchure")->load() : 0.5f;
    float initJetRatio = juce::jmap (initEmb, 0.3f, 0.6f);
    boreWaveguide.setBoreDelay (totalLoopDelay / (1.0f + initJetRatio));

    // Reset vibrato phase for new note
    vibratoPhase = 0.0f;

    // Apply instrument preset coefficients
    applyPresetCoefficients();

    // Start breath attack envelope
    jetExciter.startNote (velocity);

    // Reset silence counter and release fade
    silentSampleCount = 0;
    releaseFade = 1.0f;
    releaseFadeInc = 0.0f;
    releaseFading = false;
}

void FluteSynthVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        jetExciter.stopNote();
    }
    else
    {
        clearCurrentNote();
        jetExciter.reset();
        jetNonlinearity.reset();
        dcBlocker.reset();
        boreWaveguide.reset();
        jetDelay.reset();
    }
}

void FluteSynthVoice::pitchWheelMoved (int newValue)
{
    pitchWheelValue = newValue;
    pitchBendSemitones = ((static_cast<float> (newValue) - 8192.0f) / 8192.0f) * pitchBendRange;

    // Recalculate total loop delay with new pitch bend
    // Filter group delay is compensated dynamically per-block (not static)
    float bendedFreq = currentFrequency * std::pow (2.0f, pitchBendSemitones / 12.0f);
    float totalLoopDelay = static_cast<float> (internalSampleRate) / bendedFreq;
    totalLoopDelay = std::max (4.0f, totalLoopDelay);
    totalDelaySmoothed.setTargetValue (totalLoopDelay);  // smooth transition
}

void FluteSynthVoice::controllerMoved (int controllerNumber, int newValue)
{
    float normalized = static_cast<float> (newValue) / 127.0f;

    switch (controllerNumber)
    {
        case 2:   // CC2: Breath controller -> breath pressure
            ccBreathPressure = normalized;
            break;
        case 74:  // CC74: MPE Y / Slide -> embouchure
            ccEmbouchure = normalized;
            break;
        case 1:   // CC1: Mod wheel -> vibrato depth
            ccVibratoDepth = normalized;
            break;
        default:
            break;
    }
}

void FluteSynthVoice::prepareToPlay (double sampleRate, int maxBlockSize)
{
    nativeSampleRate = sampleRate;
    internalSampleRate = sampleRate * 2.0;  // 2x oversampled

    // Prepare oversampling
    oversampling.initProcessing (static_cast<size_t> (maxBlockSize));
    oversampling.reset();

    // Prepare all DSP components at internal (oversampled) rate
    jetExciter.prepare (internalSampleRate);

    // BoreWaveguide needs block size * 2 for oversampled processing
    boreWaveguide.prepare (internalSampleRate, maxBlockSize * 2);

    // Jet delay line at internal rate
    juce::dsp::ProcessSpec spec {
        internalSampleRate,
        static_cast<juce::uint32> (maxBlockSize * 2),
        1
    };
    jetDelay.prepare (spec);
    jetDelay.setMaximumDelayInSamples (1024);

    // SmoothedValue setup at internal rate (advances per oversampled sample)
    totalDelaySmoothed.reset (internalSampleRate, 0.003);  // 3ms delay crossfade
    embouchureSmoothed.reset (internalSampleRate, 0.005);  // 5ms embouchure smoothing

    // Preallocate temp buffer for oversampling I/O
    tempBuffer.setSize (1, maxBlockSize);
}

float FluteSynthVoice::getOversamplingLatency() const
{
    return oversampling.getLatencyInSamples();
}

void FluteSynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                        int startSample, int numSamples)
{
    if (numSamples == 0)
        return;

    // Read APVTS parameters once per block
    updateParametersFromAPVTS();

    // Voice clearing: once breath envelope has fully released, begin a fade-out
    // to ensure the voice clears even if the bore waveguide has residual energy
    if (jetExciter.isReleasing() && jetExciter.getEnvelopeLevel() < energyThreshold)
    {
        if (! releaseFading)
        {
            // Start 10ms fade-out at native sample rate
            releaseFading = true;
            float fadeSamples = static_cast<float> (nativeSampleRate * 0.010);
            releaseFadeInc = -releaseFade / std::max (1.0f, fadeSamples);
        }

        if (releaseFade <= 0.0f)
        {
            releaseFade = 0.0f;
            jetExciter.reset();
            jetNonlinearity.reset();
            dcBlocker.reset();
            boreWaveguide.reset();
            jetDelay.reset();
            clearCurrentNote();
            return;
        }
    }

    // ========== OVERSAMPLED PROCESSING ==========
    // 1. Fill temp buffer with zeros (the oversampler needs a block to upsample)
    tempBuffer.clear (0, 0, numSamples);

    // 2. Upsample: get the oversampled block structure
    auto inputBlock = juce::dsp::AudioBlock<float> (tempBuffer).getSubBlock (0, static_cast<size_t> (numSamples));
    auto osBlock = oversampling.processSamplesUp (inputBlock);

    float* osData = osBlock.getChannelPointer (0);
    auto osNumSamples = static_cast<int> (osBlock.getNumSamples());

    // 3. Process entire feedback loop at 2x rate
    for (int i = 0; i < osNumSamples; ++i)
    {
        // Get smoothed total loop delay (bore + jet)
        float totalDelay = totalDelaySmoothed.getNextValue();

        // Apply dynamic filter delay compensation (computed once per block)
        totalDelay -= filterDelayCompensation;
        totalDelay = std::max (4.0f, totalDelay);

        // Apply pitch vibrato (modulates delay for true frequency vibrato)
        if (vibratoDepthParam > 0.0f)
        {
            float vibSemitones = vibratoDepthParam * std::sin (vibratoPhase);
            totalDelay *= std::pow (2.0f, -vibSemitones / 12.0f);
            vibratoPhase += vibratoPhaseInc;
            if (vibratoPhase > juce::MathConstants<float>::twoPi)
                vibratoPhase -= juce::MathConstants<float>::twoPi;
        }

        // Split total delay between bore and jet based on embouchure
        float emb = embouchureSmoothed.getNextValue();
        float jetDelayRatio = juce::jmap (emb, 0.3f, 0.6f);
        float boreDelay = totalDelay / (1.0f + jetDelayRatio);
        boreWaveguide.setBoreDelay (boreDelay);

        // ========== STEP 1: Jet Exciter ==========
        float boreFeedback = boreWaveguide.getFeedback();
        float excitation = jetExciter.processSample (boreFeedback);

        // ========== STEP 2: Jet Delay (Lagrange3rd) ==========
        float jetDelayLength = totalDelay - boreDelay;
        jetDelayLength = std::max (2.0f, jetDelayLength);

        jetDelay.pushSample (0, excitation);
        float jetOutput = jetDelay.popSample (0, jetDelayLength);

        // ========== STEP 3: Jet-Labium Nonlinearity (Verge 1995) ==========
        float labiumOutput = jetNonlinearity.processSample (jetOutput,
                                                             jetExciter.getLastJetVelocity());

        // ========== STEP 4: DC Blocker ==========
        float dcOut = dcBlocker.processSample (labiumOutput);

        // ========== STEP 5: Bore Waveguide ==========
        auto result = boreWaveguide.processSample (dcOut);
        float sample = result.voiceOutput;

        // Apply output gain
        sample *= outputGainLinear;

        // Safety hard-clip
        sample = juce::jlimit (-2.0f, 2.0f, sample);

        // Track silence for voice cleanup
        if (std::abs (sample) < energyThreshold)
            ++silentSampleCount;
        else
            silentSampleCount = 0;

        // Store in oversampled buffer
        osData[i] = sample;
    }

    // 4. Downsample back to native rate
    auto outBlock = juce::dsp::AudioBlock<float> (tempBuffer).getSubBlock (0, static_cast<size_t> (numSamples));
    oversampling.processSamplesDown (outBlock);

    // 5. Write downsampled output to all channels (with release fade if active)
    const float* outData = tempBuffer.getReadPointer (0);
    for (int s = 0; s < numSamples; ++s)
    {
        float sample = outData[s];

        // Apply release tail fade to prevent stuck voices
        if (releaseFading)
        {
            sample *= releaseFade;
            releaseFade += releaseFadeInc;
            if (releaseFade < 0.0f) releaseFade = 0.0f;
        }

        for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
            outputBuffer.addSample (ch, startSample + s, sample);
    }
}

void FluteSynthVoice::updateParametersFromAPVTS()
{
    if (parameters == nullptr)
        return;

    // Read all parameters atomically (once per block)
    float breathPressure = parameters->getRawParameterValue ("breathPressure")->load();
    float embouchure     = parameters->getRawParameterValue ("embouchure")->load();
    float breathNoise    = parameters->getRawParameterValue ("breathNoise")->load();
    float toneColor      = parameters->getRawParameterValue ("toneColor")->load();
    float airColumn      = parameters->getRawParameterValue ("airColumn")->load();
    float jetReflection  = parameters->getRawParameterValue ("jetReflection")->load();
    float endReflection  = parameters->getRawParameterValue ("endReflection")->load();
    float vibratoRate    = parameters->getRawParameterValue ("vibratoRate")->load();
    float vibratoDepth   = parameters->getRawParameterValue ("vibratoDepth")->load();
    float outputLevel    = parameters->getRawParameterValue ("outputLevel")->load();
    float infSustain     = parameters->getRawParameterValue ("infiniteSustain")->load();
    float reversedJet    = parameters->getRawParameterValue ("reversedJet")->load();
    float subHarmonics   = parameters->getRawParameterValue ("subHarmonics")->load();

    // CC overrides (CC takes priority when non-zero)
    if (ccBreathPressure > 0.0f) breathPressure = ccBreathPressure;
    if (ccEmbouchure > 0.0f) embouchure = ccEmbouchure;
    if (ccVibratoDepth > 0.0f) vibratoDepth = ccVibratoDepth;

    // Update embouchure smooth target
    embouchureSmoothed.setTargetValue (embouchure);

    // Remap endReflection: raw [-1,1] → magnitude [0.7, 0.98], preserve sign
    float endReflSign = (endReflection >= 0.0f) ? 1.0f : -1.0f;
    float endReflMapped = endReflSign * (0.7f + std::abs (endReflection) * 0.28f);

    // Remap jetReflection: raw [-1,1] → magnitude [0.3, 0.75], preserve sign
    float jetReflSign = (jetReflection >= 0.0f) ? 1.0f : -1.0f;
    float jetReflMapped = jetReflSign * (0.3f + std::abs (jetReflection) * 0.45f);

    // Update jet exciter (vibrato is now pitch-based in voice, not breath-based)
    jetExciter.setBreathPressure (breathPressure);
    jetExciter.setBreathNoise (breathNoise);
    jetExciter.setJetReflection (jetReflMapped);
    jetExciter.setVibratoRate (0.0f);
    jetExciter.setVibratoDepth (0.0f);

    // Pitch vibrato parameters (applied per-sample in render loop)
    vibratoDepthParam = vibratoDepth;
    vibratoPhaseInc = static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                           * vibratoRate / internalSampleRate);

    // Update jet nonlinearity
    jetNonlinearity.setReversedJet (reversedJet);

    // Update bore waveguide
    boreWaveguide.setEndReflection (endReflMapped);
    boreWaveguide.setInfiniteSustain (infSustain);
    boreWaveguide.setSubHarmonics (subHarmonics);

    // Tone color -> bore loss filter cutoff (1000-12000 Hz, log scale)
    float cutoffBase = 1000.0f * std::pow (12.0f, toneColor);
    float cutoffReduction = 1.0f - airColumn * 0.7f;
    float lossCutoff = cutoffBase * cutoffReduction;
    float lossQ = 0.707f * (1.0f - airColumn * 0.3f);
    boreWaveguide.updateBoreLossFilter (lossCutoff, lossQ);

    // Update noise filter cutoff proportional to breath pressure
    float noiseCutoff = 1000.0f + breathPressure * 5000.0f;
    jetExciter.updateNoiseFilterCutoff (noiseCutoff);

    // Output gain (dB to linear)
    outputGainLinear = juce::Decibels::decibelsToGain (outputLevel);

    // Dynamic filter delay compensation: compute phase delay of bore filters
    // at the current fundamental frequency (replaces static 2.0 sample estimate)
    // BoreWaveguide sampleRate is already the internal (2x) rate
    float bendedFreq = currentFrequency * std::pow (2.0f, pitchBendSemitones / 12.0f);
    filterDelayCompensation = boreWaveguide.getFilterPhaseDelay (bendedFreq);

    // Read tone hole toggle from APVTS (stored for future ToneHoleSystem integration)
    // ToneHoleSystem scattering will be wired in a future DSP update
    juce::ignoreUnused (parameters->getRawParameterValue ("toneHoleToggle")->load());

    // Update instrument preset if changed (read from APVTS)
    int presetIdx = static_cast<int> (parameters->getRawParameterValue ("instrumentPreset")->load());
    if (presetIdx != lastPresetIndex)
    {
        lastPresetIndex = presetIdx;
        currentPreset = InstrumentPresets::getPreset (presetIdx);
        applyPresetCoefficients();
    }
}

void FluteSynthVoice::applyPresetCoefficients()
{
    if (parameters != nullptr)
    {
        int idx = static_cast<int> (parameters->getRawParameterValue ("instrumentPreset")->load());
        currentPreset = InstrumentPresets::getPreset (idx);
    }

    // Apply preset internal coefficients to DSP components
    jetNonlinearity.setJetGain (currentPreset.jetGain);
    jetExciter.setJetGain (currentPreset.jetGain);

    // Update radiation filter cutoff from preset
    boreWaveguide.updateRadiationFilter (currentPreset.radiationCutoff);

    // Update end reflection filter cutoff from preset
    boreWaveguide.updateEndReflectionFilter (currentPreset.endReflCutoff);

    // Update noise filter from preset
    jetExciter.updateNoiseFilterCutoff (currentPreset.noiseCutoffBase);
}
