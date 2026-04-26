/*
  ==============================================================================

    ReedWindVoice.cpp
    O-Reed - Physical Modeling Reed Wind Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "ReedWindVoice.h"
#include "TuningEngine.h"

ReedWindVoice::ReedWindVoice(int index)
    : breathNoise(index)
{
}

void ReedWindVoice::setAPVTS(juce::AudioProcessorValueTreeState* apvts)
{
    parameters = apvts;

    // Cache all 35 parameter pointers for real-time access

    // Primary Controls
    pBreathPressure     = apvts->getRawParameterValue("breathPressure");
    pEmbouchure         = apvts->getRawParameterValue("embouchure");
    pReedHardness       = apvts->getRawParameterValue("reedHardness");
    pBoreCharacter      = apvts->getRawParameterValue("boreCharacter");
    pInstrumentPreset   = apvts->getRawParameterValue("instrumentPreset");

    // Secondary Controls
    pReedOpening        = apvts->getRawParameterValue("reedOpening");
    pBellSize           = apvts->getRawParameterValue("bellSize");
    pAirNoise           = apvts->getRawParameterValue("airNoise");
    pDoubleReed         = apvts->getRawParameterValue("doubleReed");
    pBoreDiameter       = apvts->getRawParameterValue("boreDiameter");

    // Advanced / Sound Design
    pReedMass           = apvts->getRawParameterValue("reedMass");
    pReedDamping        = apvts->getRawParameterValue("reedDamping");
    pMouthpieceVol      = apvts->getRawParameterValue("mouthpieceVol");
    pToneHoleCutoff     = apvts->getRawParameterValue("toneHoleCutoff");
    pRegisterHole       = apvts->getRawParameterValue("registerHole");
    pBoreLength         = apvts->getRawParameterValue("boreLength");
    pBoreProfile        = apvts->getRawParameterValue("boreProfile");

    // Expressive Controls
    pVibratoDepth       = apvts->getRawParameterValue("vibratoDepth");
    pVibratoRate        = apvts->getRawParameterValue("vibratoRate");
    pVibratoSource      = apvts->getRawParameterValue("vibratoSource");
    pGrowlAmount        = apvts->getRawParameterValue("growlAmount");
    pFlutterTongue      = apvts->getRawParameterValue("flutterTongue");
    pSubtone            = apvts->getRawParameterValue("subtone");
    pAttackChiff        = apvts->getRawParameterValue("attackChiff");

    // Impossible Physics
    pInfiniteSustain    = apvts->getRawParameterValue("infiniteSustain");
    pReverseBore        = apvts->getRawParameterValue("reverseBore");
    pDualBore           = apvts->getRawParameterValue("dualBore");
    pDronePitch         = apvts->getRawParameterValue("dronePitch");
    pFeedbackPath       = apvts->getRawParameterValue("feedbackPath");

    // Tuning
    pReferencePitch     = apvts->getRawParameterValue("referencePitch");
    pTuningSystem       = apvts->getRawParameterValue("tuningSystem");

    // Voice Configuration
    pPolyMode           = apvts->getRawParameterValue("polyMode");
    pMaxVoices          = apvts->getRawParameterValue("maxVoices");
    pOversampling       = apvts->getRawParameterValue("oversampling");

    // Output
    pOutputGain         = apvts->getRawParameterValue("outputGain");
}

void ReedWindVoice::prepare(double sampleRate, int maxBlockSize)
{
    sr = static_cast<float>(sampleRate);
    currentOsFactor = 1;  // Default 2x

    // Prepare DSP components at default 2x oversampled rate
    double osRate = sampleRate * 2.0;
    int osBlockSize = maxBlockSize * 2;
    reedModel.prepare(osRate);
    bore.prepare(osRate, osBlockSize);
    bore2.prepare(osRate, osBlockSize);
    breathEnv.prepare(osRate);
    breathNoise.prepare(osRate, osBlockSize);
    chamber.prepare(osRate);

    // Initialize both oversampling instances at native block size
    oversampling2x.initProcessing(static_cast<size_t>(maxBlockSize));
    oversampling4x.initProcessing(static_cast<size_t>(maxBlockSize));
    oversampling2x.reset();
    oversampling4x.reset();

    // Allocate mono voice buffer at native block size
    voiceBuffer.setSize(1, maxBlockSize);
    voiceBuffer.clear();

    prevBoreMinus = 0.0f;
    prevBore2Minus = 0.0f;
    prevUReed = 0.0f;
    smoothedMouthpieceVol = 0.0f;
    chamberWasActive = false;
    vibratoPhase = 0.0f;
    growlPhase = 0.0f;
    flutterPhase = 0.0f;
    releaseDampGain = 1.0f;

    // ~20ms smoothing time constant (at oversampled rate for per-sample use)
    paramSmoothCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(osRate) * 0.020f));

    // Post-release bore damping: ~50ms time constant at oversampled rate
    // Gives ~500ms bore tail after breath fully off (10 time constants to ~1e-4)
    releaseDampCoeff = std::exp(-1.0f / (static_cast<float>(osRate) * 0.05f));
}

float ReedWindVoice::getBaseFrequencyFromTuning(int midiNote) const
{
    double freq = (tuningEngine != nullptr)
        ? tuningEngine->getFrequency(midiNote)
        : juce::MidiMessage::getMidiNoteInHertz(midiNote);

    // VST3 Note Expression tuning delta (Dorico microtonal). Phase 24.
    // Helper consumes slot via exchange(0.0) — first call (in noteStarted)
    // tunes; subsequent calls in the same block (notePitchbendChanged during
    // a held note) return base unchanged, which is correct: NE applies once
    // per noteStarted. Single source of truth for all three call sites
    // (noteStarted legato, noteStarted normal, notePitchbendChanged).
    if (pendingTuningSource != nullptr)
        freq = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNote, freq);

    return static_cast<float>(freq);
}

juce::dsp::Oversampling<float>& ReedWindVoice::getActiveOversampling() noexcept
{
    return (currentOsFactor == 2) ? oversampling4x : oversampling2x;
}

void ReedWindVoice::noteStarted()
{
    int polyMode = (pPolyMode != nullptr) ? static_cast<int>(pPolyMode->load()) : 1;

    if (polyMode == 0 && bore.getEnergy() > 0.001f)
    {
        // LEGATO: bore active in mono mode -- retune only, preserve all DSP state
        auto note = getCurrentlyPlayingNote();
        float frequency = getBaseFrequencyFromTuning(note.initialNote);
        float bendSemitones = static_cast<float>(note.totalPitchbendInSemitones);
        if (std::abs(bendSemitones) > 0.001f)
            frequency *= std::pow(2.0f, bendSemitones / 12.0f);

        float boreCharacter = (pBoreCharacter != nullptr) ? pBoreCharacter->load() : 0.0f;
        float bellSize      = (pBellSize != nullptr) ? pBellSize->load() : 0.5f;
        float boreDiameter  = (pBoreDiameter != nullptr) ? pBoreDiameter->load() : 0.5f;
        float boreLength    = (pBoreLength != nullptr) ? pBoreLength->load() : 0.5f;

        // Phase 3.4: read impossible physics params for legato bore update
        float infiniteSustain = (pInfiniteSustain != nullptr) ? pInfiniteSustain->load() : 0.0f;
        float reverseBore     = (pReverseBore != nullptr) ? pReverseBore->load() : 0.0f;
        float boreProfileVal  = (pBoreProfile != nullptr) ? pBoreProfile->load() : 0.0f;

        bore.updateParams(boreCharacter, bellSize, boreDiameter, boreLength,
                          infiniteSustain, reverseBore, boreProfileVal);

        float toneHoleCutoff = (pToneHoleCutoff != nullptr) ? pToneHoleCutoff->load() : 8000.0f;
        float registerHole   = (pRegisterHole != nullptr) ? pRegisterHole->load() : 0.0f;
        bore.updateToneHoles(toneHoleCutoff, registerHole);

        bore.setFrequency(frequency);  // 50ms smoothing handles glitch-free transition

        // Phase 3.4: update bore2 in legato if dual bore active
        bool dualBoreActive = (pDualBore != nullptr) && (pDualBore->load() > 0.5f);
        if (dualBoreActive)
        {
            bore2.updateParams(boreCharacter, bellSize, boreDiameter, boreLength,
                               infiniteSustain, reverseBore, boreProfileVal);
            bore2.updateToneHoles(toneHoleCutoff, registerHole);
            float dronePitchCents = (pDronePitch != nullptr) ? pDronePitch->load() : 0.0f;
            float bore2Hz = frequency * std::pow(2.0f, dronePitchCents / 1200.0f);
            bore2.setFrequency(bore2Hz);
        }

        return;
    }

    // NORMAL onset (Poly mode, or Mono with no active bore)

    // Reset all DSP state for clean note onset
    reedModel.reset();
    bore.reset();
    bore2.reset();
    breathEnv.reset();
    breathNoise.reset();
    chamber.reset();
    getActiveOversampling().reset();
    prevBoreMinus = 0.0f;
    prevBore2Minus = 0.0f;
    prevUReed = 0.0f;
    smoothedMouthpieceVol = 0.0f;
    chamberWasActive = false;
    vibratoPhase = 0.0f;
    growlPhase = 0.0f;
    flutterPhase = 0.0f;
    releaseDampGain = 1.0f;

    // Get note information (tuning-aware frequency + MPE pitchbend)
    auto note = getCurrentlyPlayingNote();
    float frequency = getBaseFrequencyFromTuning(note.initialNote);
    {
        float bendSemitones = static_cast<float>(note.totalPitchbendInSemitones);
        if (std::abs(bendSemitones) > 0.001f)
            frequency *= std::pow(2.0f, bendSemitones / 12.0f);
    }

    // Read parameters for initial bore setup
    float boreCharacter = (pBoreCharacter != nullptr) ? pBoreCharacter->load() : 0.0f;
    float bellSize      = (pBellSize != nullptr) ? pBellSize->load() : 0.5f;
    float boreDiameter  = (pBoreDiameter != nullptr) ? pBoreDiameter->load() : 0.5f;
    float boreLength    = (pBoreLength != nullptr) ? pBoreLength->load() : 0.5f;

    // Initialize bore parameters before setting frequency
    bore.updateParams(boreCharacter, bellSize, boreDiameter, boreLength);

    // Initialize tone holes
    float toneHoleCutoff = (pToneHoleCutoff != nullptr) ? pToneHoleCutoff->load() : 8000.0f;
    float registerHole   = (pRegisterHole != nullptr) ? pRegisterHole->load() : 0.0f;
    bore.updateToneHoles(toneHoleCutoff, registerHole);

    bore.setFrequency(frequency);

    // Initialize reed parameters (including doubleReed)
    float breathPressure = (pBreathPressure != nullptr) ? pBreathPressure->load() : 0.5f;
    float embouchure     = (pEmbouchure != nullptr) ? pEmbouchure->load() : 0.4f;
    float reedHardness   = (pReedHardness != nullptr) ? pReedHardness->load() : 0.5f;
    float reedOpening    = (pReedOpening != nullptr) ? pReedOpening->load() : 0.4f;
    float reedMass       = (pReedMass != nullptr) ? pReedMass->load() : 0.3f;
    float reedDamping    = (pReedDamping != nullptr) ? pReedDamping->load() : 0.5f;
    float doubleReed     = (pDoubleReed != nullptr) ? pDoubleReed->load() : 0.0f;

    reedModel.updateParams(breathPressure, embouchure, reedHardness,
                           reedOpening, reedMass, reedDamping, boreDiameter, doubleReed);

    // Trigger breath envelope with velocity and chiff
    float velocity = note.noteOnVelocity.asUnsignedFloat();
    float chiffAmount = (pAttackChiff != nullptr) ? pAttackChiff->load() : 0.3f;

    breathEnv.setTarget(breathPressure);
    breathEnv.noteOn(velocity, chiffAmount);
}

void ReedWindVoice::noteStopped(bool allowTailOff)
{
    if (allowTailOff)
    {
        // Trigger breath release -- bore will ring down naturally
        breathEnv.noteOff();
    }
    else
    {
        // Immediate cutoff
        breathEnv.reset();
        bore.reset();
        bore2.reset();
        reedModel.reset();
        breathNoise.reset();
        chamber.reset();
        oversampling2x.reset();
        oversampling4x.reset();
        prevBoreMinus = 0.0f;
        prevBore2Minus = 0.0f;
        prevUReed = 0.0f;
        smoothedMouthpieceVol = 0.0f;
        chamberWasActive = false;
        vibratoPhase = 0.0f;
        growlPhase = 0.0f;
        flutterPhase = 0.0f;
        releaseDampGain = 1.0f;
        clearCurrentNote();
    }
}

void ReedWindVoice::notePressureChanged()
{
    // MPE pressure -> breath pressure modulation
    auto note = getCurrentlyPlayingNote();
    float pressure = note.pressure.asUnsignedFloat();

    // Use MPE pressure to modulate breath target
    float breathParam = (pBreathPressure != nullptr) ? pBreathPressure->load() : 0.5f;
    // Blend: MPE pressure overrides when active (pressure > 0 means MPE is active)
    float effectiveBreath = (pressure > 0.01f) ? pressure : breathParam;
    breathEnv.setTarget(effectiveBreath);
}

void ReedWindVoice::notePitchbendChanged()
{
    // Pitchbend updates frequency -- handled per-block in renderNextBlock
    // via getBaseFrequencyFromTuning(note.initialNote) * pow(2, bend/12)
}

void ReedWindVoice::noteTimbreChanged()
{
    // MPE slide -> embouchure modulation
    // Handled per-block in renderNextBlock via getCurrentlyPlayingNote().timbre
}

void ReedWindVoice::noteKeyStateChanged()
{
    // No special handling needed for Phase 3.1
}

void ReedWindVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                     int startSample, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    if (numSamples == 0) return;

    // --- Per-block: read all active APVTS parameters (atomic loads) ---
    float breathPressure = (pBreathPressure != nullptr) ? pBreathPressure->load() : 0.5f;
    float embouchure     = (pEmbouchure != nullptr) ? pEmbouchure->load() : 0.4f;
    float reedHardness   = (pReedHardness != nullptr) ? pReedHardness->load() : 0.5f;
    float reedOpening    = (pReedOpening != nullptr) ? pReedOpening->load() : 0.4f;
    float reedMass       = (pReedMass != nullptr) ? pReedMass->load() : 0.3f;
    float reedDamping    = (pReedDamping != nullptr) ? pReedDamping->load() : 0.5f;
    float boreCharacter  = (pBoreCharacter != nullptr) ? pBoreCharacter->load() : 0.0f;
    float bellSize       = (pBellSize != nullptr) ? pBellSize->load() : 0.5f;
    float boreDiameter   = (pBoreDiameter != nullptr) ? pBoreDiameter->load() : 0.5f;
    float boreLength     = (pBoreLength != nullptr) ? pBoreLength->load() : 0.5f;
    float outputGainDb   = (pOutputGain != nullptr) ? pOutputGain->load() : 0.0f;

    // Phase 3.2 parameters
    float airNoise       = (pAirNoise != nullptr) ? pAirNoise->load() : 0.0f;
    float doubleReed     = (pDoubleReed != nullptr) ? pDoubleReed->load() : 0.0f;
    float mouthpieceVol  = (pMouthpieceVol != nullptr) ? pMouthpieceVol->load() : 0.0f;

    // Phase 3.3 expression parameters
    float vibratoDepth   = (pVibratoDepth != nullptr) ? pVibratoDepth->load() : 0.0f;
    float vibratoRate    = (pVibratoRate != nullptr) ? pVibratoRate->load() : 5.0f;
    int   vibratoSrc     = (pVibratoSource != nullptr) ? static_cast<int>(pVibratoSource->load()) : 0;
    float growlAmount    = (pGrowlAmount != nullptr) ? pGrowlAmount->load() : 0.0f;
    float flutterTongue  = (pFlutterTongue != nullptr) ? pFlutterTongue->load() : 0.0f;
    float subtone        = (pSubtone != nullptr) ? pSubtone->load() : 0.0f;

    // Phase 3.3 tone hole parameters
    float toneHoleCutoff = (pToneHoleCutoff != nullptr) ? pToneHoleCutoff->load() : 8000.0f;
    float registerHole   = (pRegisterHole != nullptr) ? pRegisterHole->load() : 0.0f;

    // Phase 3.4 impossible physics parameters
    float infiniteSustain = (pInfiniteSustain != nullptr) ? pInfiniteSustain->load() : 0.0f;
    float reverseBore     = (pReverseBore != nullptr) ? pReverseBore->load() : 0.0f;
    bool  dualBoreActive  = (pDualBore != nullptr) && (pDualBore->load() > 0.5f);
    float dronePitchCents = (pDronePitch != nullptr) ? pDronePitch->load() : 0.0f;
    float feedbackPath    = (pFeedbackPath != nullptr) ? pFeedbackPath->load() : 0.0f;
    float boreProfileVal  = (pBoreProfile != nullptr) ? pBoreProfile->load() : 0.0f;

    // MPE expression
    auto note = getCurrentlyPlayingNote();
    float mpeTimbre = note.timbre.asUnsignedFloat();
    float mpePressure = note.pressure.asUnsignedFloat();

    // MPE slide -> embouchure blend
    if (mpeTimbre > 0.01f)
        embouchure = mpeTimbre;

    // MPE pressure -> breath target blend
    float effectiveBreath = (mpePressure > 0.01f) ? mpePressure : breathPressure;
    breathEnv.setTarget(effectiveBreath);

    // Update DSP parameters (including doubleReed for Psi confinement)
    // Note: embouchure_eff is computed per-sample in the loop for expression modifiers
    reedModel.updateParams(breathPressure, embouchure, reedHardness,
                           reedOpening, reedMass, reedDamping, boreDiameter, doubleReed);

    bore.updateParams(boreCharacter, bellSize, boreDiameter, boreLength,
                      infiniteSustain, reverseBore, boreProfileVal);
    bore.updateToneHoles(toneHoleCutoff, registerHole);

    // Update frequency with tuning-aware base + MPE pitchbend
    float frequency = getBaseFrequencyFromTuning(note.initialNote);
    float bendSemitones = static_cast<float>(note.totalPitchbendInSemitones);
    if (std::abs(bendSemitones) > 0.001f)
        frequency *= std::pow(2.0f, bendSemitones / 12.0f);
    bore.setFrequency(frequency);


    // Phase 3.4: wire bore2 if dual bore active
    if (dualBoreActive)
    {
        bore2.updateParams(boreCharacter, bellSize, boreDiameter, boreLength,
                           infiniteSustain, reverseBore, boreProfileVal);
        bore2.updateToneHoles(toneHoleCutoff, registerHole);
        float bore2Hz = frequency * std::pow(2.0f, dronePitchCents / 1200.0f);
        bore2.setFrequency(bore2Hz);
    }

    // Get cached impedance and convert output gain
    float Z_c = reedModel.getZc();
    float outputGain = juce::Decibels::decibelsToGain(outputGainDb);

    // Output normalization: bore pressure at the reed stabilizes near p_closure during
    // self-oscillation. Normalize by 2*p_closure so tanh operates in its useful range
    // (output ~0.5-0.8 at typical playing levels, soft-limiting at fortissimo).
    float closurePa = reedModel.getClosurePressure();
    float normalization = 1.0f / std::max(closurePa * 2.0f, 200.0f);

    // Mouthpiece chamber: compute physical volume and bore area
    // mouthpieceVol 0-1 -> 0 to 15 cm^3 (1.5e-5 m^3)
    float V_m = mouthpieceVol * 1.5e-5f;

    // Bore cross-section area at throat (must match ReedModel computation)
    constexpr float pi = 3.14159265f;
    float throatRadius = 0.002f + boreDiameter * 0.018f;
    float A_bore = pi * throatRadius * throatRadius;

    chamber.setParams(V_m, A_bore, 1.2f, 343.0f);

    int numChannels = outputBuffer.getNumChannels();

    // --- Oversampling factor change detection ---
    int osChoice = (pOversampling != nullptr) ? static_cast<int>(pOversampling->load()) : 0;
    int osFactor = osChoice + 1;  // 0=2x(factor 1), 1=4x(factor 2)
    if (osFactor != currentOsFactor)
    {
        currentOsFactor = osFactor;
        double osRate = static_cast<double>(sr) * std::pow(2.0, currentOsFactor);
        int osBlockSize = numSamples * (1 << currentOsFactor);
        reedModel.prepare(osRate);
        bore.prepare(osRate, osBlockSize);
        bore2.prepare(osRate, osBlockSize);
        breathEnv.prepare(osRate);
        breathNoise.prepare(osRate, osBlockSize);
        chamber.prepare(osRate);
        getActiveOversampling().reset();
        // Update smoothing coeff for new oversampled rate
        paramSmoothCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(osRate) * 0.020f));
        releaseDampCoeff = std::exp(-1.0f / (static_cast<float>(osRate) * 0.05f));
        releaseDampGain = 1.0f;
    }

    // Oversampled rate for LFO phase increments
    float osRate = sr * static_cast<float>(1 << currentOsFactor);

    // --- Block-based oversampling pathway ---

    // 1. Clear voiceBuffer subblock and create AudioBlock
    voiceBuffer.clear(0, 0, numSamples);
    juce::dsp::AudioBlock<float> inputBlock(voiceBuffer);
    auto block = inputBlock.getSubBlock(0, static_cast<size_t>(numSamples));

    // 2. Upsample (silence in -> oversampled buffer)
    auto& activeOS = getActiveOversampling();
    auto oversampledBlock = activeOS.processSamplesUp(block);
    auto numOversampledSamples = static_cast<int>(oversampledBlock.getNumSamples());
    auto* oversampledData = oversampledBlock.getChannelPointer(0);

    // 3. Per-sample processing loop at oversampled rate
    for (int sample = 0; sample < numOversampledSamples; ++sample)
    {
        // 1. Get current breath pressure from envelope
        //    Scale to reed's closure pressure (not fixed 12000 Pa) so the reed
        //    operates in the self-oscillation regime at any parameter setting
        float p_closure = reedModel.getClosurePressure();
        float p_mouth_raw = breathEnv.processSample() * p_closure;

        // 2. Expression modifiers that affect noise (computed before noise generation)
        float airNoise_eff = airNoise;
        float embouchure_eff = embouchure;

        if (subtone > 1e-6f)
        {
            airNoise_eff = std::min(airNoise + subtone * 0.3f, 1.0f);
            embouchure_eff += subtone * 0.3f;
        }

        // 3. Add breath noise: flow-dependent (Phase 3.2) + turbulence seeding
        //    Turbulence seeds bore resonance even when reed is closed
        float noisePressure = breathNoise.processSample(airNoise_eff, prevUReed, Z_c, p_mouth_raw);
        // Breath-proportional turbulence (2%): seeds self-oscillation startup
        static juce::Random _turbRng(42);
        float turbNoise = (_turbRng.nextFloat() * 2.0f - 1.0f) * p_mouth_raw * 0.02f;
        float p_mouth = p_mouth_raw + noisePressure + turbNoise;

        // Subtone: reduce mouth pressure
        if (subtone > 1e-6f)
            p_mouth *= (1.0f - subtone * 0.3f);

        // Vibrato (sine LFO with 3 target sources) -- uses oversampled rate
        if (vibratoDepth > 1e-6f)
        {
            vibratoPhase += 6.2831853f * vibratoRate / osRate;
            if (vibratoPhase >= 6.2831853f) vibratoPhase -= 6.2831853f;
            float vibMod = vibratoDepth * std::sin(vibratoPhase);
            switch (vibratoSrc) {
                case 0: embouchure_eff += vibMod * 0.15f; break;      // Lip
                case 1: p_mouth *= (1.0f + vibMod * 0.1f); break;    // Breath
                case 2: bore.modulateScaleFactor(vibMod * 0.03f); break; // Throat
            }
        }

        // Growl (~120 Hz sine on mouth pressure) -- uses oversampled rate
        if (growlAmount > 1e-6f)
        {
            growlPhase += 6.2831853f * 120.0f / osRate;
            if (growlPhase >= 6.2831853f) growlPhase -= 6.2831853f;
            p_mouth *= (1.0f + growlAmount * std::sin(growlPhase) * 0.3f);
        }

        // Flutter tongue (~25 Hz smoothed square on mouth pressure) -- uses oversampled rate
        if (flutterTongue > 1e-6f)
        {
            flutterPhase += 6.2831853f * 25.0f / osRate;
            if (flutterPhase >= 6.2831853f) flutterPhase -= 6.2831853f;
            float squarish = std::tanh(4.0f * std::sin(flutterPhase));
            p_mouth *= (1.0f - flutterTongue * 0.4f * std::max(squarish, 0.0f));
        }

        // Apply per-sample embouchure to reed model
        reedModel.setEmbouchure(embouchure_eff);

        // 4. Get bore feedback (wave arriving at reed from previous sample)
        float p_bore_minus = prevBoreMinus;
        if (dualBoreActive)
        {
            float safeFeedback = feedbackPath * 0.5f;
            p_bore_minus = prevBoreMinus * (1.0f - safeFeedback)
                         + prevBore2Minus * safeFeedback;
        }

        // 5. Reed model: compute volume flow through reed (returns u_reed)
        float u_reed = reedModel.processSample(p_mouth, p_bore_minus);
        prevUReed = u_reed;

        // 6. Smooth mouthpiece volume and handle chamber activation
        smoothedMouthpieceVol += (mouthpieceVol - smoothedMouthpieceVol) * paramSmoothCoeff;
        bool chamberIsActive = smoothedMouthpieceVol > 1e-4f;

        // Initialize chamber state on activation to prevent transients
        if (chamberIsActive && !chamberWasActive)
            chamber.initializeState(p_mouth);
        chamberWasActive = chamberIsActive;

        // 7. Mouthpiece chamber: converts u_reed to p_bore_plus via lumped compliance
        //    At mouthpieceVol=0: bypasses to direct Z_c * u_reed + p_bore_minus
        float p_bore_plus = chamber.processSample(u_reed, p_bore_minus, Z_c);

        // 8. Bore waveguide(s): push forward, process bell/loss, returns next p_bore_minus
        prevBoreMinus = bore.processSample(p_bore_plus);

        // Soft-limit bore pressure to prevent runaway divergence.
        // Without this, mismatched Z_c vs closurePa drives bore to hundreds
        // of thousands of Pa, producing hard-clipped square waves.
        // Limit at 3x closure pressure preserves natural dynamics while
        // preventing the output tanh from hard-clipping.
        {
            float boreLimit = std::max(p_closure * 3.0f, 2000.0f);
            prevBoreMinus = boreLimit * std::tanh(prevBoreMinus / boreLimit);
        }

        // 8.5. Post-release bore damping: once breath envelope fully off,
        // apply exponential damping (~50ms TC) to accelerate bore decay.
        // Without this, bore rings 4-28s depending on pitch (0.5% visc loss/trip).
        if (breathEnv.getState() == BreathEnvelope::State::Off)
            releaseDampGain *= releaseDampCoeff;
        else
            releaseDampGain = 1.0f;
        prevBoreMinus *= releaseDampGain;

        // 9. Output: use bore pressure at reed (prevBoreMinus) as audio signal
        //    This bypasses the radiation highpass which kills the fundamental.
        //    Bore pressure naturally contains the full harmonic spectrum.
        //    Phase 3.4: mix bore2 when dual bore active
        float output;
        if (dualBoreActive)
        {
            prevBore2Minus = bore2.processSample(p_bore_plus);
            {
                float boreLimit2 = std::max(p_closure * 3.0f, 2000.0f);
                prevBore2Minus = boreLimit2 * std::tanh(prevBore2Minus / boreLimit2);
            }
            prevBore2Minus *= releaseDampGain;
            output = (prevBoreMinus + prevBore2Minus) * normalization * outputGain;
        }
        else
        {
            output = prevBoreMinus * normalization * outputGain;
        }

        // 10. NaN/Inf guard + safety clip via tanh soft saturation
        if (!std::isfinite(output))
        {
            output = 0.0f;
            prevBoreMinus = 0.0f;
            prevUReed = 0.0f;
        }
        output = std::tanh(output);

        // Write to oversampled buffer
        oversampledData[sample] = output;

    }

    // 4. Downsample back to native rate
    activeOS.processSamplesDown(block);

    // 5. Mix voiceBuffer into outputBuffer at native rate (mono -> stereo)
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = voiceBuffer.getSample(0, i);
        for (int ch = 0; ch < numChannels; ++ch)
            outputBuffer.addSample(ch, startSample + i, sample);
    }

    // --- Post-block cleanup ---
    bore.snapFiltersToZero();
    if (dualBoreActive) bore2.snapFiltersToZero();

    // Voice cleanup: release breath + bore energy decayed
    if (!breathEnv.isActive() && bore.getEnergy() < 1e-6f
        && (!dualBoreActive || bore2.getEnergy() < 1e-6f))
    {
        clearCurrentNote();
    }
}
