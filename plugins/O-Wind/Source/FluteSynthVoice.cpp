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

// Non-linear embouchure-to-jet-ratio mapping with register sweet spots.
// Creates sticky plateaus at physical harmonic ratios (~0.33 fundamental,
// ~0.25 octave, ~0.20 third harmonic) with smooth tanh transitions.
// Higher breath pressure shifts transitions lower, making overblowing easier.
static float embouchureToJetRatio (float emb, float breathPressure, float overblowEase)
{
    // Physical harmonic ratios: jet/(jet+bore) for mode locking
    constexpr float r1 = 0.33f;  // fundamental: jet ≈ 1/3 total
    constexpr float r2 = 0.25f;  // 2nd harmonic: jet ≈ 1/4 total
    constexpr float r3 = 0.20f;  // 3rd harmonic: jet ≈ 1/5 total

    // Breath pressure shifts transition points downward in embouchure space
    float pressureShift = breathPressure * overblowEase * 0.15f;

    // Transition centers in embouchure [0,1] space
    float t1 = juce::jlimit (0.15f, 0.55f, 0.38f - pressureShift);
    float t2 = juce::jlimit (0.45f, 0.88f, 0.70f - pressureShift);

    // Steepness controls how sticky each register plateau feels
    constexpr float k = 10.0f;

    // Sigmoid blending between register plateaus
    float s1 = 0.5f * (1.0f + std::tanh (k * (emb - t1)));
    float s2 = 0.5f * (1.0f + std::tanh (k * (emb - t2)));

    return r1 + (r2 - r1) * s1 + (r3 - r2) * s2;
}

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

    // VST3 Note Expression tuning delta (Dorico).
    // Apply BEFORE pitch-bend (below) and BEFORE BoreWaveguide period derivation
    // (boreWaveguide.setBoreDelay(...) at the end of this block) so the
    // physical-model period sees the tuned frequency on sample 0
    // (Pattern 2 — no attack zipper). FluteSynthVoice uses float currentFrequency;
    // cast through double at the helper boundary (same pattern as O-Bells).
    if (pendingTuningSource != nullptr)
    {
        currentFrequency = static_cast<float> (Ouaricon::NoteExpression::applyPendingTuning (
            *pendingTuningSource, midiNoteNumber, static_cast<double> (currentFrequency)));
    }

    // Apply pitch bend
    float bendedFreq = currentFrequency * std::pow (2.0f, pitchBendSemitones / 12.0f);

    // Calculate total loop delay (bore + jet) at internal (oversampled) sample rate
    // Filter group delay is compensated dynamically per-block (not static)
    float totalLoopDelay = static_cast<float> (internalSampleRate) / bendedFreq;
    totalLoopDelay = std::max (4.0f, totalLoopDelay);

    // Store total loop delay (split between bore and jet in render loop)
    totalDelaySmoothed.setCurrentAndTargetValue (totalLoopDelay);

    // Set initial bore delay estimate based on current embouchure + register mapping
    float initEmb = (paramCache.embouchure != nullptr)
        ? paramCache.embouchure->load() : 0.5f;
    float initBreath = (paramCache.breathPressure != nullptr)
        ? paramCache.breathPressure->load() : 0.5f;
    float initJetRatio = embouchureToJetRatio (initEmb, initBreath, currentOverblowEase);
    boreWaveguide.setBoreDelay (totalLoopDelay / (1.0f + initJetRatio));

    // Random initial vibrato phase per note (humanization).
    // voiceRng, not Random::getSystemRandom() — the shared global instance is
    // not thread-safe against message-thread use inside JUCE.
    vibratoPhase = voiceRng.nextFloat() * juce::MathConstants<float>::twoPi;

    // Reset vibrato onset counter
    samplesSinceNoteOn = 0;

    // Randomize drift oscillator phases for variation between notes
    vibratoRateDriftPhase = voiceRng.nextFloat() * juce::MathConstants<float>::twoPi;
    vibratoDepthDriftPhase = voiceRng.nextFloat() * juce::MathConstants<float>::twoPi;

    // Apply instrument preset coefficients
    applyPresetCoefficients();

    // Chiff pitch overshoot: start bore delay 1-2% shorter than target
    // Scale overshoot amount by attackChiff param (1% at 0, 2% at 1)
    float overshootPct = juce::jmap (attackChiffParam, 0.0f, 1.0f, 0.01f, 0.02f);
    pitchOvershootFactor = 1.0f - overshootPct * velocity;
    pitchOvershootTarget = 1.0f;
    // One-pole settle: 50-100ms (faster at higher velocity)
    float settleMs = juce::jmap (velocity, 0.0f, 1.0f, 100.0f, 50.0f);
    float settleSamples = static_cast<float> (internalSampleRate * settleMs * 0.001);
    pitchOvershootCoeff = 1.0f - std::exp (-3.0f / std::max (1.0f, settleSamples));

    // Per-note humanization: draw random offsets scaled by humanize param
    float h = humanizeParam;
    attackTimeScale    = 1.0f + (voiceRng.nextFloat() * 2.0f - 1.0f) * 0.20f * h;
    noiseBurstScale    = 1.0f + (voiceRng.nextFloat() * 2.0f - 1.0f) * 0.30f * h;
    embouchureDelayOffset = (voiceRng.nextFloat() * 2.0f - 1.0f) * 0.01f * h;
    strouhalFreqScale  = 1.0f + (voiceRng.nextFloat() * 2.0f - 1.0f) * 0.10f * h;
    vibratoOnsetOffsetMs = (voiceRng.nextFloat() * 2.0f - 1.0f) * 50.0f * h;

    // Randomize growl oscillator frequency (70-120 Hz range) per note
    growlFreq = 70.0f + voiceRng.nextFloat() * 50.0f;
    growlPhaseInc = static_cast<float> (growlFreq / internalSampleRate);
    growlPhase = voiceRng.nextFloat();

    // Start breath attack envelope (with humanized attack time and chiff amplitude)
    jetExciter.startNote (velocity, attackTimeScale, noiseBurstScale);

    // Reset release fade
    releaseFade = 1.0f;
    releaseFadeInc = 0.0f;
    releaseFading = false;
    pendingJetRelease = false;

    // ADSR envelope: start attack if enabled, otherwise passthrough
    if (adsrEnabled)
    {
        adsrStage = ADSRStage::Attack;
        adsrLevel = 0.0f;
        float attackSamples = std::max (1.0f, static_cast<float> (internalSampleRate * adsrAttackSeconds));
        adsrIncrement = 1.0f / attackSamples;
    }
    else
    {
        adsrStage = ADSRStage::Idle;
        adsrLevel = 1.0f;
    }
}

void FluteSynthVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        // ADSR: enter release stage if enabled
        if (adsrEnabled && adsrStage != ADSRStage::Idle)
        {
            // Defer jet exciter release — keep breath excitation alive so the
            // waveguide has energy for the ADSR release envelope to shape.
            // jetExciter.stopNote() is called when ADSR reaches zero.
            pendingJetRelease = true;
            adsrStage = ADSRStage::Release;
            float releaseSamples = std::max (1.0f, static_cast<float> (internalSampleRate * adsrReleaseSeconds));
            adsrIncrement = -adsrLevel / releaseSamples;
        }
        else
        {
            jetExciter.stopNote();
        }
    }
    else
    {
        clearCurrentNote();
        jetExciter.reset();
        jetNonlinearity.reset();
        dcBlocker.reset();
        boreWaveguide.reset();
        jetDelay.reset();
        oversampling.reset();
        adsrStage = ADSRStage::Idle;
        adsrLevel = 0.0f;
        pendingJetRelease = false;
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
            ccBreathSeen = true;
            break;
        case 74:  // CC74: MPE Y / Slide -> embouchure
            ccEmbouchure = normalized;
            ccEmbouchureSeen = true;
            break;
        case 1:   // CC1: Mod wheel -> vibrato depth
            ccVibratoDepth = normalized;
            ccVibratoSeen = true;
            break;
        default:
            break;
    }
}

void FluteSynthVoice::cacheParameterPointers()
{
    if (parameters == nullptr)
        return;

    paramCache.breathPressure    = parameters->getRawParameterValue ("breathPressure");
    paramCache.embouchure        = parameters->getRawParameterValue ("embouchure");
    paramCache.breathNoise       = parameters->getRawParameterValue ("breathNoise");
    paramCache.toneColor         = parameters->getRawParameterValue ("toneColor");
    paramCache.airColumn         = parameters->getRawParameterValue ("airColumn");
    paramCache.jetReflection     = parameters->getRawParameterValue ("jetReflection");
    paramCache.endReflection     = parameters->getRawParameterValue ("endReflection");
    paramCache.vibratoRate       = parameters->getRawParameterValue ("vibratoRate");
    paramCache.vibratoDepth      = parameters->getRawParameterValue ("vibratoDepth");
    paramCache.vibratoTremolo    = parameters->getRawParameterValue ("vibratoTremolo");
    paramCache.vibratoDriftDepth = parameters->getRawParameterValue ("vibratoDriftDepth");
    paramCache.vibratoDriftSpeed = parameters->getRawParameterValue ("vibratoDriftSpeed");
    paramCache.vibratoOnset      = parameters->getRawParameterValue ("vibratoOnset");
    paramCache.outputLevel       = parameters->getRawParameterValue ("outputLevel");
    paramCache.infiniteSustain   = parameters->getRawParameterValue ("infiniteSustain");
    paramCache.reversedJet       = parameters->getRawParameterValue ("reversedJet");
    paramCache.subHarmonics      = parameters->getRawParameterValue ("subHarmonics");
    paramCache.material          = parameters->getRawParameterValue ("material");
    paramCache.attackChiff       = parameters->getRawParameterValue ("attackChiff");
    paramCache.humanize          = parameters->getRawParameterValue ("humanize");
    paramCache.flutterTongue     = parameters->getRawParameterValue ("flutterTongue");
    paramCache.flutterRate       = parameters->getRawParameterValue ("flutterRate");
    paramCache.growl             = parameters->getRawParameterValue ("growl");
    paramCache.inharmonicity     = parameters->getRawParameterValue ("inharmonicity");
    paramCache.adsrEnabled       = parameters->getRawParameterValue ("adsrEnabled");
    paramCache.adsrAttack        = parameters->getRawParameterValue ("adsrAttack");
    paramCache.adsrDecay         = parameters->getRawParameterValue ("adsrDecay");
    paramCache.adsrSustain       = parameters->getRawParameterValue ("adsrSustain");
    paramCache.adsrRelease       = parameters->getRawParameterValue ("adsrRelease");
    paramCache.instrumentPreset  = parameters->getRawParameterValue ("instrumentPreset");
}

void FluteSynthVoice::prepareToPlay (double sampleRate, int maxBlockSize)
{
    nativeSampleRate = sampleRate;
    internalSampleRate = sampleRate * 2.0;  // 2x oversampled

    // Cache raw parameter pointers once — the per-block read path must not do
    // string-keyed APVTS lookups on the audio thread (IN-01)
    cacheParameterPointers();

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
    // Jet delay reaches totalDelay * ratio/(1+ratio) (≤ ~25% of the loop);
    // size for the lowest MIDI note at the internal rate — a fixed 1024
    // silently clamps (shifts the bore/jet split) at >= 96 kHz hosts.
    jetDelay.setMaximumDelayInSamples (
        static_cast<int> (std::ceil (internalSampleRate / 8.176 * 0.5)) + 8);

    // SmoothedValue setup at internal rate (advances per oversampled sample)
    totalDelaySmoothed.reset (internalSampleRate, 0.003);  // 3ms delay crossfade
    embouchureSmoothed.reset (internalSampleRate, 0.005);  // 5ms embouchure smoothing

    // Vibrato drift oscillator phase increments (updated per-block from APVTS)
    // Initial values use defaults; updateParametersFromAPVTS() overwrites each block
    vibratoRateDriftInc = static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                                * 0.47 / internalSampleRate);
    vibratoDepthDriftInc = static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                                 * 0.31 / internalSampleRate);

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

    // Inactive voices produce exact zeros through the whole oversampled model —
    // skip the render (and its filter updates) entirely. State is already reset
    // at clearCurrentNote time, so early-out is safe.
    if (! isVoiceActive())
        return;

    // Read APVTS parameters once per block
    updateParametersFromAPVTS();

    // Voice clearing: once breath envelope has fully released, begin a fade-out
    // to ensure the voice clears even if the bore waveguide has residual energy.
    // If ADSR is enabled and still releasing, defer cleanup so the ADSR release
    // tail is heard (the physical model's waveguide output shapes the release).
    bool adsrStillReleasing = adsrEnabled && adsrStage == ADSRStage::Release;
    if (jetExciter.isReleasing() && jetExciter.getEnvelopeLevel() < energyThreshold
        && ! adsrStillReleasing)
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
            oversampling.reset();  // half-band state must not leak into the next note
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

        // Chiff pitch overshoot: shorten bore delay during onset, settle to target
        if (pitchOvershootFactor < 0.9999f)
        {
            pitchOvershootFactor += pitchOvershootCoeff * (pitchOvershootTarget - pitchOvershootFactor);
            totalDelay *= pitchOvershootFactor;
        }

        // Per-note humanization: embouchure delay offset (+/-1% of bore delay)
        totalDelay *= (1.0f + embouchureDelayOffset);

        // Apply humanized pitch vibrato (modulates delay for true frequency vibrato)
        // and phase-locked tremolo (amplitude modulation)
        float tremoloGain = 1.0f;
        if (vibratoDepthParam > 0.0f || vibratoTremoloDepthParam > 0.0f)
        {
            // Onset ramp: linear 0→1 over vibratoOnset ms after noteOn
            float onsetGain = (vibratoOnsetSamples > 0 && samplesSinceNoteOn < vibratoOnsetSamples)
                ? static_cast<float> (samplesSinceNoteOn) / static_cast<float> (vibratoOnsetSamples)
                : 1.0f;

            // Rate drift: slow noise modulating LFO rate +/- (0.75 * driftDepth) Hz
            float rateDrift = std::sin (vibratoRateDriftPhase) * 0.75f * vibratoDriftDepthParam;
            float driftedPhaseInc = vibratoPhaseInc
                + static_cast<float> (2.0 * juce::MathConstants<double>::pi * rateDrift / internalSampleRate);

            // Depth drift: independent slow noise +/- (25% * driftDepth)
            float depthScale = 1.0f + std::sin (vibratoDepthDriftPhase) * 0.25f * vibratoDriftDepthParam;

            // Shape asymmetry: sin(phase) + 0.1*sin(2*phase)
            float vibratoShape = std::sin (vibratoPhase)
                               + 0.1f * std::sin (2.0f * vibratoPhase);

            // Pitch vibrato: modulate bore delay
            if (vibratoDepthParam > 0.0f)
            {
                float vibSemitones = vibratoDepthParam * depthScale * onsetGain * vibratoShape;
                totalDelay *= std::pow (2.0f, -vibSemitones / 12.0f);
            }

            // Tremolo: amplitude modulation locked to same vibrato phase
            // Positive vibratoShape (pitch up) = louder, negative (pitch down) = softer
            // At max depth (1.0), amplitude varies ±30% (~2.5 dB)
            if (vibratoTremoloDepthParam > 0.0f)
                tremoloGain = 1.0f + vibratoTremoloDepthParam * depthScale * onsetGain * vibratoShape * 0.3f;

            vibratoPhase += driftedPhaseInc;
            if (vibratoPhase > juce::MathConstants<float>::twoPi)
                vibratoPhase -= juce::MathConstants<float>::twoPi;

            vibratoRateDriftPhase += vibratoRateDriftInc;
            if (vibratoRateDriftPhase > juce::MathConstants<float>::twoPi)
                vibratoRateDriftPhase -= juce::MathConstants<float>::twoPi;

            vibratoDepthDriftPhase += vibratoDepthDriftInc;
            if (vibratoDepthDriftPhase > juce::MathConstants<float>::twoPi)
                vibratoDepthDriftPhase -= juce::MathConstants<float>::twoPi;
        }
        ++samplesSinceNoteOn;

        // Split total delay between bore and jet based on embouchure (register-aware)
        float emb = embouchureSmoothed.getNextValue();
        float jetDelayRatio = embouchureToJetRatio (emb, breathPressureParam, currentOverblowEase);
        float boreDelay = totalDelay / (1.0f + jetDelayRatio);
        boreWaveguide.setBoreDelay (boreDelay);

        // ========== STEP 1: Jet Exciter ==========
        // Bore feedback is NEGATED before jet coupling: the embouchure end is
        // acoustically open, so the returning pressure wave inverts (pressure
        // node).  Combined with the far-end inversion (-endReflCoeff in
        // BoreWaveguide), this gives two sign inversions per round trip —
        // the correct open-open (flute) topology where f = sampleRate/D.
        float boreFeedback = boreWaveguide.getFeedback();

        // Growl: sawtooth AM on bore feedback (vocal-fold coupling roughness)
        if (growlParam > 0.0f)
        {
            boreFeedback *= (1.0f - growlParam * 0.6f * growlPhase);
            growlPhase += growlPhaseInc;
            if (growlPhase >= 1.0f)
                growlPhase -= 1.0f;
        }

        float excitation = jetExciter.processSample (-boreFeedback);

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

        // ADSR envelope: advance state machine per sample (oversampled rate)
        if (adsrEnabled)
        {
            switch (adsrStage)
            {
                case ADSRStage::Attack:
                    adsrLevel += adsrIncrement;
                    if (adsrLevel >= 1.0f)
                    {
                        adsrLevel = 1.0f;
                        adsrStage = ADSRStage::Decay;
                        float decaySamples = std::max (1.0f, static_cast<float> (internalSampleRate * adsrDecaySeconds));
                        adsrIncrement = (adsrSustainLevel - 1.0f) / decaySamples;
                    }
                    break;
                case ADSRStage::Decay:
                    adsrLevel += adsrIncrement;
                    if (adsrLevel <= adsrSustainLevel)
                    {
                        adsrLevel = adsrSustainLevel;
                        adsrStage = ADSRStage::Sustain;
                    }
                    break;
                case ADSRStage::Sustain:
                    adsrLevel = adsrSustainLevel;
                    break;
                case ADSRStage::Release:
                    adsrLevel += adsrIncrement;
                    if (adsrLevel <= 0.0f)
                    {
                        adsrLevel = 0.0f;
                        adsrStage = ADSRStage::Idle;
                        // Now release the breath excitation so voice cleanup can proceed
                        if (pendingJetRelease)
                        {
                            jetExciter.stopNote();
                            pendingJetRelease = false;
                        }
                    }
                    break;
                case ADSRStage::Idle:
                    break;
            }
        }

        // Apply output gain, phase-locked tremolo, and fixed voice attenuation
        // -6 dB (0.5x) attenuation keeps raw waveguide output at comfortable level
        sample *= outputGainLinear * tremoloGain * 0.5f * adsrLevel;

        // Soft safety clip (tanh-based to avoid aliasing from hard discontinuities)
        sample = std::tanh (sample * 0.5f) * 2.0f;

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
    // Pointers cached at prepare time (IN-01) — no string-keyed APVTS lookups
    // on the audio thread. renderNextBlock only runs after prepareToPlay.
    if (paramCache.breathPressure == nullptr)
        return;

    // Read all parameters atomically (once per block)
    float breathPressure = paramCache.breathPressure->load();
    float embouchure     = paramCache.embouchure->load();
    float breathNoise    = paramCache.breathNoise->load();
    float toneColor      = paramCache.toneColor->load();
    float airColumn      = paramCache.airColumn->load();
    float jetReflection  = paramCache.jetReflection->load();
    float endReflection  = paramCache.endReflection->load();
    float vibratoRate    = paramCache.vibratoRate->load();
    float vibratoDepth   = paramCache.vibratoDepth->load();
    float outputLevel    = paramCache.outputLevel->load();
    float infSustain     = paramCache.infiniteSustain->load();
    float reversedJet    = paramCache.reversedJet->load();
    float subHarmonics   = paramCache.subHarmonics->load();

    float material        = paramCache.material->load();

    float attackChiff     = paramCache.attackChiff->load();
    float humanize        = paramCache.humanize->load();
    float flutterTongue   = paramCache.flutterTongue->load();
    float flutterRate     = paramCache.flutterRate->load();
    float growl           = paramCache.growl->load();

    // CC overrides: once a controller has sent any value it owns the destination,
    // so CC value 0 is honored (a breath controller can end a phrase at zero)
    // instead of falling back to the knob.
    if (ccBreathSeen)     breathPressure = ccBreathPressure;
    if (ccEmbouchureSeen) embouchure = ccEmbouchure;
    if (ccVibratoSeen)    vibratoDepth = ccVibratoDepth;

    // Cache attackChiff for startNote pitch overshoot calculation
    attackChiffParam = attackChiff;

    // Cache humanize for per-note randomization in startNote
    humanizeParam = humanize;

    // Cache growl for per-sample bore feedback modulation
    growlParam = growl;

    // Store breath pressure for per-sample register transition mapping
    breathPressureParam = breathPressure;

    // Update embouchure smooth target
    embouchureSmoothed.setTargetValue (embouchure);

    // Remap endReflection: raw [-1,1] → magnitude [0.7, 0.98], preserve sign
    // Material nudges end reflection: metal (1) → +0.05 (brighter reflections),
    // wood (0) → -0.05 (more damped reflections), centered at material=0.5
    float endReflSign = (endReflection >= 0.0f) ? 1.0f : -1.0f;
    float endReflBase = 0.7f + std::abs (endReflection) * 0.28f;
    float materialEndReflNudge = (material - 0.5f) * 0.1f;
    float endReflMapped = endReflSign * juce::jlimit (0.5f, 0.99f, endReflBase + materialEndReflNudge);

    // Remap jetReflection: raw [-1,1] → magnitude [0.3, 0.75], preserve sign
    float jetReflSign = (jetReflection >= 0.0f) ? 1.0f : -1.0f;
    float jetReflMapped = jetReflSign * (0.3f + std::abs (jetReflection) * 0.45f);

    // Update jet exciter (vibrato is now pitch-based in voice, not breath-based)
    jetExciter.setBreathPressure (breathPressure);
    // Register-dependent noise: more breath noise for low notes, less for high
    float registerNoiseMult = 1.4f - (static_cast<float> (currentMidiNote) / 127.0f) * 0.8f;
    jetExciter.setBreathNoise (breathNoise * registerNoiseMult);
    jetExciter.setJetReflection (jetReflMapped);
    jetExciter.setAttackChiff (attackChiff);
    jetExciter.setVibratoRate (0.0f);
    jetExciter.setVibratoDepth (0.0f);
    jetExciter.setFlutterTongue (flutterTongue);
    jetExciter.setFlutterRate (flutterRate);

    // Pitch vibrato parameters (applied per-sample in render loop)
    vibratoDepthParam = vibratoDepth;
    vibratoTremoloDepthParam = paramCache.vibratoTremolo->load();
    vibratoPhaseInc = static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                           * vibratoRate / internalSampleRate);

    // Vibrato drift (evolution) parameters — user-controllable depth and speed
    vibratoDriftDepthParam = paramCache.vibratoDriftDepth->load();
    vibratoDriftSpeedParam = paramCache.vibratoDriftSpeed->load();

    // Update drift oscillator phase increments from user speed parameter
    // Rate drift oscillator runs at 1.175x the base speed, depth drift at 0.775x
    // (maintains the ~1.5:1 ratio from the original 0.47/0.31 Hz pairing)
    vibratoRateDriftInc = static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                                * vibratoDriftSpeedParam * 1.175 / internalSampleRate);
    vibratoDepthDriftInc = static_cast<float> (2.0 * juce::MathConstants<double>::pi
                                                 * vibratoDriftSpeedParam * 0.775 / internalSampleRate);

    // Vibrato onset delay (with per-note humanization offset)
    float vibratoOnset = paramCache.vibratoOnset->load();
    vibratoOnsetMs = std::max (0.0f, vibratoOnset + vibratoOnsetOffsetMs);
    vibratoOnsetSamples = static_cast<int> (internalSampleRate * vibratoOnsetMs * 0.001);

    // Update jet nonlinearity
    jetNonlinearity.setReversedJet (reversedJet);

    // Update bore waveguide
    boreWaveguide.setEndReflection (endReflMapped);
    boreWaveguide.setInfiniteSustain (infSustain);
    boreWaveguide.setSubHarmonics (subHarmonics);

    // Inharmonicity: APVTS param multiplies preset base for effective allpass coefficient
    float inharmonicity = paramCache.inharmonicity->load();
    boreWaveguide.setInharmonicity (inharmonicity * currentPreset.inharmonicityBase);

    // Material macro: continuous wood (0) to metal (1) timbral offset
    // Maps 0→0.6x, 0.5→1.0x, 1→1.4x for bore loss and noise
    float materialBoreScale = 0.6f + material * 0.8f;
    // Maps 0→0.7x, 0.5→1.0x, 1→1.3x for radiation
    float materialRadScale  = 0.7f + material * 0.6f;

    // Tone color -> bore loss filter cutoff (1000-12000 Hz, log scale)
    float cutoffBase = 1000.0f * std::pow (12.0f, toneColor);
    float lossCutoff = cutoffBase * materialBoreScale;

    // Register-dependent spectral shaping: modulate bore loss cutoff by pitch
    // Low notes (C3) ≈ 0.9x (wider relative to fundamental, richer harmonics)
    // High notes (C7) ≈ 1.2x (narrower relative to fundamental, purer tone)
    float registerCutoffMult = 0.6f + (static_cast<float> (currentMidiNote) / 127.0f) * 0.8f;
    lossCutoff *= registerCutoffMult;

    // Air column: bore geometry macro scaling the loss-filter corner ±½ octave
    // around neutral 0.5 (longer/wider column damps harmonics sooner, shorter
    // column keeps them). 0.5 leaves lossCutoff unchanged, so factory-default
    // timbre is identical to when this parameter was inert.
    lossCutoff *= std::pow (2.0f, 0.5f - airColumn);
    boreWaveguide.updateBoreLossFilter (lossCutoff);

    // Radiation filter: preset base cutoff scaled by material (wood=darker, metal=brighter)
    boreWaveguide.updateRadiationFilter (currentPreset.radiationCutoff * materialRadScale);

    // End reflection filter: preset base cutoff scaled by material
    boreWaveguide.updateEndReflectionFilter (currentPreset.endReflCutoff * materialRadScale);

    // Update Strouhal bandpass: center frequency scales with breath pressure
    // Material scales noise center similarly to bore loss (wood=darker noise, metal=brighter)
    // Per-note humanization: shift center freq by +/-10%
    jetExciter.updateStrouhalBandpass (breathPressure, strouhalFreqScale * materialBoreScale);

    // Output gain (dB to linear)
    outputGainLinear = juce::Decibels::decibelsToGain (outputLevel);

    // Re-query tuning engine per block so tuning changes take effect on held notes
    if (tuningEngine != nullptr)
    {
        float newFreq = static_cast<float> (tuningEngine->getFrequency (currentMidiNote));
        if (std::abs (newFreq - currentFrequency) > 0.01f)
        {
            currentFrequency = newFreq;
            float bendedFreq = currentFrequency * std::pow (2.0f, pitchBendSemitones / 12.0f);
            float totalLoopDelay = static_cast<float> (internalSampleRate) / bendedFreq;
            totalLoopDelay = std::max (4.0f, totalLoopDelay);
            totalDelaySmoothed.setTargetValue (totalLoopDelay);
        }
    }

    // Dynamic loop delay compensation: account for ALL phase/delay contributions
    // in the feedback loop beyond the explicit delay lines.
    // BoreWaveguide sampleRate is already the internal (2x) rate.
    float bendedFreq = currentFrequency * std::pow (2.0f, pitchBendSemitones / 12.0f);
    float boreFilterDelay = boreWaveguide.getFilterPhaseDelay (bendedFreq);
    float dcPhaseDelay = dcBlocker.getPhaseDelay (
        static_cast<double> (bendedFreq), internalSampleRate);

    // Total compensation = bore filter phase delay (positive, filters lag)
    //                     + DC blocker phase delay (negative, highpass advances)
    //                     + 1 implicit sample (feedback read from previous iteration)
    filterDelayCompensation = boreFilterDelay + dcPhaseDelay + 1.0f;

    // ADSR envelope parameters
    adsrEnabled = paramCache.adsrEnabled->load() >= 0.5f;
    adsrAttackSeconds = paramCache.adsrAttack->load();
    adsrDecaySeconds = paramCache.adsrDecay->load();
    adsrSustainLevel = paramCache.adsrSustain->load();
    adsrReleaseSeconds = paramCache.adsrRelease->load();

    // If ADSR was just disabled mid-note, reset to passthrough — and resolve
    // any deferred jet release. Without this a note that was releasing when
    // ADSR turned off never calls jetExciter.stopNote() and drones at full
    // level until stolen (the Release stage only advances while adsrEnabled).
    if (! adsrEnabled)
    {
        if (pendingJetRelease)
        {
            jetExciter.stopNote();
            pendingJetRelease = false;
        }
        adsrStage = ADSRStage::Idle;
        adsrLevel = 1.0f;
    }

    // Update instrument preset if changed (read from APVTS)
    int presetIdx = static_cast<int> (paramCache.instrumentPreset->load());
    if (presetIdx != lastPresetIndex)
    {
        lastPresetIndex = presetIdx;
        currentPreset = InstrumentPresets::getPreset (presetIdx);
        applyPresetCoefficients();
    }
}

void FluteSynthVoice::applyPresetCoefficients()
{
    if (paramCache.instrumentPreset != nullptr)
    {
        int idx = static_cast<int> (paramCache.instrumentPreset->load());
        currentPreset = InstrumentPresets::getPreset (idx);
    }

    // Apply preset internal coefficients to DSP components
    jetNonlinearity.setJetGain (currentPreset.jetGain);
    jetExciter.setJetAmplification (currentPreset.jetAmplification);
    jetExciter.setJetDiameter (currentPreset.jetDiameter);
    currentOverblowEase = currentPreset.overblowEase;

    // Update radiation filter cutoff from preset
    boreWaveguide.updateRadiationFilter (currentPreset.radiationCutoff);

    // Update end reflection filter cutoff from preset
    boreWaveguide.updateEndReflectionFilter (currentPreset.endReflCutoff);
}
