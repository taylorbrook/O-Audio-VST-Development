#include "PluginProcessor.h"

#include <cmath>

ReverseDelayProcessor::ReverseDelayProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache raw parameter atomics once — read per block on the audio thread.
    pDelayTime    = parameters.getRawParameterValue("delayTime");
    pSyncMode     = parameters.getRawParameterValue("syncMode");
    pNoteDivision = parameters.getRawParameterValue("noteDivision");
    pGrainSize    = parameters.getRawParameterValue("grainSize");
    pDensity      = parameters.getRawParameterValue("density");
    pFeedback     = parameters.getRawParameterValue("feedback");
    pLowCut       = parameters.getRawParameterValue("lowCut");
    pHighCut      = parameters.getRawParameterValue("highCut");
    pWidth        = parameters.getRawParameterValue("width");
    pMix          = parameters.getRawParameterValue("mix");
}

ReverseDelayProcessor::~ReverseDelayProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout ReverseDelayProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // delayTime: 50–2000 ms, default 500, skew centred on geometric mean (316 ms)
    {
        juce::NormalisableRange<float> range { 50.0f, 2000.0f, 0.01f };
        range.setSkewForCentre(316.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "delayTime", 1 }, "Delay Time", range, 500.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));
    }

    // syncMode: Free / Sync, default Sync
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "syncMode", 1 }, "Sync Mode",
        juce::StringArray { "Free", "Sync" }, 1));

    // noteDivision: 13 entries, contract order, default index 6 (1/4)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "noteDivision", 1 }, "Note Division",
        juce::StringArray { "1/16", "1/16D", "1/16T",
                            "1/8",  "1/8D",  "1/8T",
                            "1/4",  "1/4D",  "1/4T",
                            "1/2",  "1/2D",  "1/2T",
                            "1/1" },
        6));

    // grainSize: 50–500 ms, default 200, skew centred on 158 ms
    {
        juce::NormalisableRange<float> range { 50.0f, 500.0f, 0.01f };
        range.setSkewForCentre(158.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "grainSize", 1 }, "Grain Size", range, 200.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));
    }

    // density: 0–100 %, default 60, linear
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "density", 1 }, "Density",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // feedback: 0–100 %, default 40, linear
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1 }, "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 40.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // lowCut: 20–2000 Hz, default 100, skew centred on 200 Hz
    {
        juce::NormalisableRange<float> range { 20.0f, 2000.0f, 0.01f };
        range.setSkewForCentre(200.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "lowCut", 1 }, "Low Cut", range, 100.0f,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));
    }

    // highCut: 500–20000 Hz, default 8000, skew centred on 3162 Hz
    {
        juce::NormalisableRange<float> range { 500.0f, 20000.0f, 0.01f };
        range.setSkewForCentre(3162.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "highCut", 1 }, "High Cut", range, 8000.0f,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));
    }

    // width: 0–100 %, default 60, linear
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "width", 1 }, "Width",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // mix: 0–100 %, default 35, linear
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 35.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return layout;
}

void ReverseDelayProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int maxBlock = juce::jmax(1, samplesPerBlock);

    // ALL allocation happens here — processBlock touches only preallocated state.
    capture.prepare(sampleRate, 3.5f);   // covers Dmax + 2·Gmax = 3.0 s + margin
    scheduler.prepare(sampleRate);
    grainPool.clear();

    wetScratch.setSize(2, maxBlock);
    fbScratch.setSize(2, maxBlock);
    wetScratch.clear();
    fbScratch.clear();

    const double smoothingSeconds = 0.02;   // ~20 ms per contract
    feedbackSmoothed.reset(sampleRate, smoothingSeconds);
    mixSmoothed.reset(sampleRate, smoothingSeconds);
    lowCutSmoothed.reset(sampleRate, smoothingSeconds);
    highCutSmoothed.reset(sampleRate, smoothingSeconds);

    feedbackSmoothed.setCurrentAndTargetValue(pFeedback->load() * 0.01f);
    mixSmoothed.setCurrentAndTargetValue(pMix->load() * 0.01f);
    lowCutSmoothed.setCurrentAndTargetValue(pLowCut->load());
    highCutSmoothed.setCurrentAndTargetValue(pHighCut->load());
}

void ReverseDelayProcessor::releaseResources()
{
    // Buffers are modest (capture ring ~3.5 s stereo); keep them allocated so a
    // transport stop/start cycle never reallocates. Nothing to do here.
}

bool ReverseDelayProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in.isDisabled() || out.isDisabled())
        return false;

    const bool inOk  = in  == juce::AudioChannelSet::mono() || in  == juce::AudioChannelSet::stereo();
    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();

    // Accept mono→mono, mono→stereo, stereo→stereo; reject stereo→mono (no down-mix path)
    return inOk && outOk && in.size() <= out.size();
}

void ReverseDelayProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();

    if (numSamples == 0 || buffer.getNumChannels() == 0)
        return;

    // Defensive: host delivered a block larger than prepared, or prepareToPlay
    // never ran — bail silent-safe rather than read past scratch buffers.
    if (numSamples > wetScratch.getNumSamples() || capture.getBufferSize() == 0)
        return;

    const int numInputChannels  = juce::jmin(getTotalNumInputChannels(),  buffer.getNumChannels());
    const int numOutputChannels = juce::jmin(getTotalNumOutputChannels(), buffer.getNumChannels());

    // ---- (0) once-per-block parameter reads (atomic) ------------------------
    // Latched-per-grain parameters — read raw, NEVER smoothed (latching at
    // spawn is the click-free mechanism per contract).
    const float delayTimeMs = pDelayTime->load();
    const float grainSizeMs = pGrainSize->load();
    const float densityPct  = pDensity->load();

    // Smoothed (~20 ms) parameters — set targets once per block.
    feedbackSmoothed.setTargetValue(pFeedback->load() * 0.01f);
    mixSmoothed.setTargetValue(pMix->load() * 0.01f);
    lowCutSmoothed.setTargetValue(pLowCut->load());
    highCutSmoothed.setTargetValue(pHighCut->load());

    // ---- (1) resolve grain anchor delay D -----------------------------------
    // Phase 2.1: free mode only. Tempo sync (Phase 2.3) will change only the
    // VALUE of D here — never the spawn timing (no conditional routing).
    const int D = juce::jmax(1, static_cast<int>(delayTimeMs * 0.001 * currentSampleRate));
    const int G = juce::jmax(2, static_cast<int>(grainSizeMs * 0.001 * currentSampleRate));

    const float overlap         = 1.0f + (densityPct * 0.01f) * 7.0f;
    const int   intervalSamples = juce::jmax(1, static_cast<int>(static_cast<float>(G) / overlap));
    const float grainGain       = 1.0f / std::sqrt(overlap);   // compensation, latched per grain,
                                                               // applied BEFORE the feedback tap

    // ---- (2) advance smoothers ----------------------------------------------
    // mix advances per sample in the mix loop (step 7). The Phase-2.2 loop
    // parameters advance here so their timelines stay consistent when the
    // feedback path lands.
    feedbackSmoothed.skip(numSamples);
    lowCutSmoothed.skip(numSamples);
    highCutSmoothed.skip(numSamples);

    // ---- (3) schedule spawns, latch per-grain state -------------------------
    const int spawnCount = scheduler.processBlock(numSamples, intervalSamples, spawnRequests);
    const juce::int64 blockStartAbs = capture.getTotalWritten();   // capture write happens in step 6

    for (int s = 0; s < spawnCount; ++s)
    {
        const int offset = spawnRequests[static_cast<size_t>(s)].sampleOffset;
        auto& g = grainPool.obtain();

        g.active      = true;
        g.readAbs     = (blockStartAbs + static_cast<juce::int64>(offset)) - static_cast<juce::int64>(D);
        g.n           = 0;
        g.G           = G;
        g.invG        = 1.0f / static_cast<float>(G);
        g.gain        = grainGain;
        g.gL          = 0.70710677f;   // width spread lands in Phase 2.3 — center pan for now
        g.gR          = 0.70710677f;
        g.age         = 0;
        g.startOffset = offset;
    }

    // ---- (4) render active grains into wetScratch (overlap-add) -------------
    wetScratch.clear();
    float* wetL = wetScratch.getWritePointer(0);
    float* wetR = wetScratch.getWritePointer(1);

    for (auto& g : grainPool.grains)
    {
        if (!g.active)
            continue;

        const int   start = g.startOffset;
        const int   end   = juce::jmin(numSamples, start + (g.G - g.n));
        juce::int64 readAbs = g.readAbs;
        int         n       = g.n;
        const float invG = g.invG, gain = g.gain, gL = g.gL, gR = g.gR;

        // Branch-free inner loop: LUT lerp + mul-adds, per-grain constants
        // precomputed at spawn. Integer reverse read: readAbs steps −1 while
        // the write head advances +1 → net offset growth D+2n.
        for (int i = start; i < end; ++i)
        {
            const float src = capture.monoSum(readAbs);
            const float env = hannLut.read(static_cast<float>(n) * invG);
            const float v   = src * env * gain;
            wetL[i] += v * gL;
            wetR[i] += v * gR;
            --readAbs;
            ++n;
        }

        g.readAbs = readAbs;
        g.n       = n;
        g.age    += (end - start);
        g.startOffset = 0;

        if (g.n >= g.G)
            g.active = false;
    }

    // ---- (5) feedback return — Phase 2.1 stub: fb = 0 -----------------------
    // Phase 2.2 fills this with: wet → smoothed fbGain → HP(lowCut) →
    // LP(highCut) → tanh → non-finite guard.
    fbScratch.clear();
    const float* fbL = fbScratch.getReadPointer(0);
    const float* fbR = fbScratch.getReadPointer(1);

    // ---- (6) capture write: input + feedback return -------------------------
    // Mono input feeds both capture channels (L = R = in).
    const float* inL = buffer.getReadPointer(0);
    const float* inR = numInputChannels > 1 ? buffer.getReadPointer(1) : inL;

    for (int i = 0; i < numSamples; ++i)
        capture.pushSample(inL[i] + fbL[i], inR[i] + fbR[i]);

    // ---- (7) equal-power dry/wet mix ----------------------------------------
    // Dry comes from the untouched input buffer (wet never rendered in-place).
    constexpr float halfPi = juce::MathConstants<float>::halfPi;

    if (numOutputChannels > 1)
    {
        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            const float m       = mixSmoothed.getNextValue();
            const float dryGain = std::cos(m * halfPi);
            const float wetGain = std::sin(m * halfPi);
            const float dryL    = outL[i];
            const float dryR    = numInputChannels > 1 ? outR[i] : dryL;   // mono→stereo: duplicate dry

            outL[i] = dryGain * dryL + wetGain * wetL[i];
            outR[i] = dryGain * dryR + wetGain * wetR[i];
        }
    }
    else
    {
        float* outM = buffer.getWritePointer(0);

        for (int i = 0; i < numSamples; ++i)
        {
            const float m       = mixSmoothed.getNextValue();
            const float dryGain = std::cos(m * halfPi);
            const float wetGain = std::sin(m * halfPi);

            // Mono out: equal-power fold of the centered wet pair
            // (0.7071·(L+R) → unity for the width-0 dual-mono wet).
            outM[i] = dryGain * outM[i] + wetGain * 0.70710677f * (wetL[i] + wetR[i]);
        }
    }
}

juce::AudioProcessorEditor* ReverseDelayProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

bool ReverseDelayProcessor::hasEditor() const { return true; }

const juce::String ReverseDelayProcessor::getName() const { return JucePlugin_Name; }

bool ReverseDelayProcessor::acceptsMidi() const { return false; }
bool ReverseDelayProcessor::producesMidi() const { return false; }
bool ReverseDelayProcessor::isMidiEffect() const { return false; }
// Conservative real tail so hosts don't truncate the reverse tail on bounce
// (RESEARCH pitfall 11 — offline renders honour this).
double ReverseDelayProcessor::getTailLengthSeconds() const { return 10.0; }

int ReverseDelayProcessor::getNumPrograms() { return 1; }
int ReverseDelayProcessor::getCurrentProgram() { return 0; }
void ReverseDelayProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String ReverseDelayProcessor::getProgramName(int index) { juce::ignoreUnused(index); return {}; }
void ReverseDelayProcessor::changeProgramName(int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

void ReverseDelayProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ReverseDelayProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverseDelayProcessor();
}
