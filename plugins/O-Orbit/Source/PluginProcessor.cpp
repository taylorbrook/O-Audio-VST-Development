#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

OOrbitProcessor::OOrbitProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("OOrbitParams"), createParameterLayout())
{
    // Cache parameter pointers
    pathParam          = parameters.getRawParameterValue ("path");
    speedParam         = parameters.getRawParameterValue ("speed");
    widthParam         = parameters.getRawParameterValue ("width");
    depthParam         = parameters.getRawParameterValue ("depth");
    tiltParam          = parameters.getRawParameterValue ("tilt");
    phaseParam         = parameters.getRawParameterValue ("phase");
    elevEnableParam    = parameters.getRawParameterValue ("elevation_enable");
    elevRangeParam     = parameters.getRawParameterValue ("elevation_range");
    tempoSyncParam     = parameters.getRawParameterValue ("tempo_sync");

    speakerLayoutParam = parameters.getRawParameterValue ("speaker_layout");
    distanceParam      = parameters.getRawParameterValue ("distance");
    airAbsorptionParam = parameters.getRawParameterValue ("air_absorption");
    attenCurveParam    = parameters.getRawParameterValue ("attenuation_curve");
    centerDivergeParam = parameters.getRawParameterValue ("center_diverge");

    sourceModeParam    = parameters.getRawParameterValue ("source_mode");
    lrOffsetParam      = parameters.getRawParameterValue ("lr_offset");
    mixParam           = parameters.getRawParameterValue ("mix");
}

OOrbitProcessor::~OOrbitProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout OOrbitProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ── Motion Group ──
    auto motionGroup = std::make_unique<juce::AudioProcessorParameterGroup> (
        "motion", "Motion", "|");

    motionGroup->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "path", 1 }, "Path",
        juce::StringArray { "Orbit", "Pendulum", "Linear", "Drift" }, 0));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "speed", 1 }, "Speed",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.0f, 0.5f), 1.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "width", 1 }, "Width",
        juce::NormalisableRange<float> (0.0f, 360.0f), 180.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tilt", 1 }, "Tilt",
        juce::NormalisableRange<float> (-90.0f, 90.0f), 0.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phase", 1 }, "Phase",
        juce::NormalisableRange<float> (0.0f, 360.0f), 0.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "elevation_enable", 1 }, "Elevation", false));

    motionGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "elevation_range", 1 }, "Elev Range",
        juce::NormalisableRange<float> (0.0f, 90.0f), 45.0f));

    motionGroup->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "tempo_sync", 1 }, "Tempo Sync",
        juce::StringArray { "Off", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D",
                            "1/4T", "1/4", "1/4D", "1/2", "1/2D", "1 Bar", "2 Bars", "4 Bars" },
        0));

    layout.add (std::move (motionGroup));

    // ── Spatial Group ──
    auto spatialGroup = std::make_unique<juce::AudioProcessorParameterGroup> (
        "spatial", "Spatial", "|");

    spatialGroup->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "speaker_layout", 1 }, "Speaker Layout",
        juce::StringArray { "Stereo", "Quad", "5.1", "7.1", "5.1.4", "7.1.4", "Hexaphonic", "Octaphonic" },
        0));

    spatialGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "distance", 1 }, "Distance",
        juce::NormalisableRange<float> (0.1f, 30.0f, 0.0f, 0.5f), 1.0f));

    spatialGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "air_absorption", 1 }, "Air Absorption",
        juce::NormalisableRange<float> (0.0f, 100.0f), 50.0f));

    spatialGroup->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "attenuation_curve", 1 }, "Atten Curve",
        juce::StringArray { "Linear", "Inverse", "Inverse Square" }, 1));

    spatialGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "center_diverge", 1 }, "Center Diverge",
        juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));

    layout.add (std::move (spatialGroup));

    // ── Mix Group ──
    auto mixGroup = std::make_unique<juce::AudioProcessorParameterGroup> (
        "mix", "Mix", "|");

    mixGroup->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "source_mode", 1 }, "Source Mode",
        juce::StringArray { "Mono", "L+R Split" }, 0));

    mixGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lr_offset", 1 }, "L/R Offset",
        juce::NormalisableRange<float> (0.0f, 360.0f), 180.0f));

    mixGroup->addChild (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f));

    layout.add (std::move (mixGroup));

    return layout;
}

const juce::String OOrbitProcessor::getName() const           { return JucePlugin_Name; }
bool OOrbitProcessor::acceptsMidi() const                     { return false; }
bool OOrbitProcessor::producesMidi() const                    { return false; }
bool OOrbitProcessor::isMidiEffect() const                    { return false; }
double OOrbitProcessor::getTailLengthSeconds() const          { return 0.0; }
int OOrbitProcessor::getNumPrograms()                         { return 1; }
int OOrbitProcessor::getCurrentProgram()                      { return 0; }
void OOrbitProcessor::setCurrentProgram (int)                 {}
const juce::String OOrbitProcessor::getProgramName (int)      { return {}; }
void OOrbitProcessor::changeProgramName (int, const juce::String&) {}
bool OOrbitProcessor::hasEditor() const                       { return true; }

juce::AudioProcessorEditor* OOrbitProcessor::createEditor()
{
    return new OOrbitEditor (*this);
}

bool OOrbitProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto mainInput  = layouts.getMainInputChannelSet();
    auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput.isDisabled() || mainOutput.isDisabled())
        return false;

    // Input: mono or stereo
    if (mainInput != juce::AudioChannelSet::mono()
        && mainInput != juce::AudioChannelSet::stereo())
        return false;

    // Output: 2-24 channels
    int numOut = mainOutput.size();
    return numOut >= 2 && numOut <= 24;
}

float OOrbitProcessor::shortestArc (float from, float to)
{
    float diff = std::fmod (to - from + 540.0f, 360.0f) - 180.0f;
    return from + diff;
}

float OOrbitProcessor::wrapAngle (float angle)
{
    float result = std::fmod (angle + 180.0f, 360.0f);
    if (result < 0.0f) result += 360.0f;
    return result - 180.0f;
}

void OOrbitProcessor::handleAsyncUpdate()
{
    // Called on message thread when speaker layout changes
    int layoutIndex = static_cast<int> (speakerLayoutParam->load());
    currentLayout = SpeakerPresets::getPreset (layoutIndex);
    layoutNumSpeakers = currentLayout.getChannelCount();
    vbapRenderer.prepare (currentLayout);
    lastSpeakerLayoutIndex = layoutIndex;

    // Trigger background VBAP gain table recomputation for 4+ speakers
    if (layoutNumSpeakers >= 4)
        vbapThread.requestRecomputation (currentLayout);
}

void OOrbitProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setLatencySamples (0);

    motionEngine.prepare (sampleRate);
    distanceModel.prepare (sampleRate);
    distanceModelR.prepare (sampleRate);

    // Initialize speaker layout from current param
    int layoutIndex = static_cast<int> (speakerLayoutParam->load());
    currentLayout = SpeakerPresets::getPreset (layoutIndex);
    layoutNumSpeakers = currentLayout.getChannelCount();
    vbapRenderer.prepare (currentLayout);
    lastSpeakerLayoutIndex = layoutIndex;

    // Start VBAP compute thread and trigger initial gain table generation
    if (! vbapThread.isThreadRunning())
        vbapThread.startThread (juce::Thread::Priority::normal);
    if (layoutNumSpeakers >= 4)
        vbapThread.requestRecomputation (currentLayout);

    // Initialize auto-downmix engine
    downmixEngine.prepare (currentLayout, getTotalNumOutputChannels());

    // Allocate buffers
    int maxChannels = std::max (getTotalNumOutputChannels(), layoutNumSpeakers);
    dryBuffer.setSize (2, samplesPerBlock);
    spatialBuffer.setSize (maxChannels, samplesPerBlock);

    // Smoothed values: 20ms ramp
    speedSmoothed.reset (sampleRate, 0.02);
    widthSmoothed.reset (sampleRate, 0.02);
    depthSmoothed.reset (sampleRate, 0.02);
    tiltSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.reset (sampleRate, 0.02);

    // Reset gain smoothing
    previousGains.fill (0.0f);
    currentGains.fill (0.0f);
    previousGainsR.fill (0.0f);
    currentGainsR.fill (0.0f);
}

void OOrbitProcessor::releaseResources() {}

void OOrbitProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    if (numSamples == 0)
        return;

    // Update VBAP gain table from background thread (lock-free try)
    vbapExchange.updateAudioThreadData();
    if (auto* vbapData = vbapExchange.getActiveData())
    {
        vbapRenderer.setExternalGainTable (
            vbapData->gainTable.data(),
            (int) vbapData->gainTable.size(),
            vbapData->numOutputChannels,
            vbapData->aziRes,
            vbapData->elevRes,
            vbapData->is3D,
            vbapData->speakerToChannelMap.data(),
            vbapData->numVBAPSpeakers);
    }

    // Check for speaker layout change
    int layoutIndex = static_cast<int> (speakerLayoutParam->load());
    if (layoutIndex != lastSpeakerLayoutIndex)
        triggerAsyncUpdate();

    // Read parameters
    int pathIdx           = static_cast<int> (pathParam->load());
    float speedVal        = speedParam->load();
    float widthVal        = widthParam->load();
    float depthVal        = depthParam->load();
    float tiltVal         = tiltParam->load();
    float phaseVal        = phaseParam->load();
    bool elevEnabled      = elevEnableParam->load() > 0.5f;
    float elevRange       = elevRangeParam->load();
    int tempoSyncIdx      = static_cast<int> (tempoSyncParam->load());
    float dist            = distanceParam->load();
    float airAbs          = airAbsorptionParam->load();
    int attenCurve        = static_cast<int> (attenCurveParam->load());
    float diverge         = centerDivergeParam->load() / 100.0f;
    int sourceMode        = static_cast<int> (sourceModeParam->load());
    float lrOffset        = lrOffsetParam->load();
    float mixVal          = mixParam->load() / 100.0f;

    // Read host BPM
    double hostBpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto bpm = pos->getBpm())
                hostBpm = *bpm;
        }
    }

    // Update motion engine parameters
    motionEngine.setPath (pathIdx);
    motionEngine.setSpeed (speedVal);
    motionEngine.setWidth (widthVal);
    motionEngine.setDepth (depthVal);
    motionEngine.setTilt (tiltVal);
    motionEngine.setPhase (phaseVal);
    motionEngine.setElevationEnabled (elevEnabled);
    motionEngine.setElevationRange (elevRange);
    motionEngine.setTempoSync (tempoSyncIdx);
    motionEngine.setHostBpm (hostBpm);

    // Update distance model
    distanceModel.updateDistance (dist, airAbs, attenCurve);
    distanceModelR.updateDistance (dist, airAbs, attenCurve);

    // Update VBAP renderer
    vbapRenderer.setCenterDiverge (diverge);

    // Copy input to dry buffer for dry/wet mix
    dryBuffer.setSize (totalNumInputChannels, numSamples, false, false, true);
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // Get motion state: start and end for this block
    MotionState startState = motionEngine.getCurrentState();
    motionEngine.advance (numSamples);
    MotionState endState = motionEngine.getCurrentState();

    // Prepare azimuth end for shortest-arc interpolation
    float endAz = shortestArc (startState.azimuth, endState.azimuth);

    // Save previous gains for smoothing, compute new block gains
    previousGains = currentGains;
    previousGainsR = currentGainsR;

    // Compute VBAP gains at end position for this block
    currentGains.fill (0.0f);
    vbapRenderer.computeGains (endState.azimuth, endState.elevation,
                               currentGains.data(), layoutNumSpeakers);

    if (sourceMode == 1) // L+R Split
    {
        float rAz = wrapAngle (endState.azimuth + lrOffset);
        currentGainsR.fill (0.0f);
        vbapRenderer.computeGains (rAz, endState.elevation,
                                   currentGainsR.data(), layoutNumSpeakers);
    }

    // Clear spatial buffer
    int spatialChannels = std::min (layoutNumSpeakers, (int) spatialBuffer.getNumChannels());
    if (spatialChannels > spatialBuffer.getNumChannels())
        spatialBuffer.setSize (spatialChannels, numSamples, false, false, true);
    for (int ch = 0; ch < spatialChannels; ++ch)
        spatialBuffer.clear (ch, 0, numSamples);

    // Per-sample processing
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float t = (float) sample / (float) numSamples;

        // Interpolated motion state
        float az   = startState.azimuth   + t * (endAz - startState.azimuth);
        float el   = startState.elevation + t * (endState.elevation - startState.elevation);
        float d    = startState.distance  + t * (endState.distance  - startState.distance);

        // Interpolated VBAP gains
        float gains[24] = {};
        for (int s = 0; s < spatialChannels; ++s)
            gains[s] = previousGains[(size_t) s] + t * (currentGains[(size_t) s] - previousGains[(size_t) s]);

        if (sourceMode == 0) // Mono
        {
            // Sum input to mono
            float inputL = (totalNumInputChannels > 0) ? buffer.getSample (0, sample) : 0.0f;
            float inputR = (totalNumInputChannels > 1) ? buffer.getSample (1, sample) : inputL;
            float mono = (inputL + inputR) * 0.5f;

            // Apply distance model
            float processed = distanceModel.processSample (mono);

            // Distribute to speakers via VBAP gains
            for (int s = 0; s < spatialChannels; ++s)
                spatialBuffer.addSample (s, sample, processed * gains[s]);
        }
        else // L+R Split
        {
            float inputL = (totalNumInputChannels > 0) ? buffer.getSample (0, sample) : 0.0f;
            float inputR = (totalNumInputChannels > 1) ? buffer.getSample (1, sample) : inputL;

            // L source at current position
            float processedL = distanceModel.processSample (inputL);

            // R source gains (interpolated)
            float gainsR[24] = {};
            for (int s = 0; s < spatialChannels; ++s)
                gainsR[s] = previousGainsR[(size_t) s] + t * (currentGainsR[(size_t) s] - previousGainsR[(size_t) s]);

            float processedR = distanceModelR.processSample (inputR);

            // Accumulate both sources
            for (int s = 0; s < spatialChannels; ++s)
            {
                spatialBuffer.addSample (s, sample, processedL * gains[s]);
                spatialBuffer.addSample (s, sample, processedR * gainsR[s]);
            }
        }
    }

    // Copy spatial buffer to output, with auto-downmix if needed
    int outChannels = totalNumOutputChannels;
    buffer.clear();

    if (outChannels >= spatialChannels)
    {
        // Direct mapping: copy spatial channels to output
        for (int ch = 0; ch < spatialChannels; ++ch)
            buffer.copyFrom (ch, 0, spatialBuffer, ch, 0, numSamples);
    }
    else if (downmixEngine.isActive())
    {
        // Use DownmixEngine for energy-preserving fold-down
        const float* srcPtrs[24] = {};
        float* destPtrs[24] = {};
        for (int ch = 0; ch < spatialChannels && ch < 24; ++ch)
            srcPtrs[ch] = spatialBuffer.getReadPointer (ch);
        for (int ch = 0; ch < outChannels && ch < 24; ++ch)
            destPtrs[ch] = buffer.getWritePointer (ch);

        downmixEngine.process (srcPtrs, destPtrs, numSamples);
    }
    else
    {
        // Fallback: copy what fits
        for (int ch = 0; ch < std::min (outChannels, spatialChannels); ++ch)
            buffer.copyFrom (ch, 0, spatialBuffer, ch, 0, numSamples);
    }

    // Apply dry/wet mix
    for (int ch = 0; ch < std::min (totalNumOutputChannels, totalNumInputChannels); ++ch)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            float dry = dryBuffer.getSample (ch, s);
            float wet = buffer.getSample (ch, s);
            buffer.setSample (ch, s, mixVal * wet + (1.0f - mixVal) * dry);
        }
    }
}

void OOrbitProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void OOrbitProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OOrbitProcessor();
}
