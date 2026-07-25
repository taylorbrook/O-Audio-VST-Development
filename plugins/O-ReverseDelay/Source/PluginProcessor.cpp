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

    // ── Stage 4 (D16): 8 factory presets ────────────────────────────────────
    // Authored in ENGINEERING UNITS (ms, %, Hz, choice index) and converted once
    // through each parameter's own NormalisableRange below. Four params are
    // skewed (delayTime centre 316 ms, grainSize 158 ms, lowCut 200 Hz, highCut
    // 3162 Hz); a hand-written normalised fraction on any of them recalls 10–30×
    // wrong (pattern_factory_preset_normalized_ignores_skew).
    //
    // All ten keys are explicit in every preset. Omitted keys would revert to the
    // APVTS default (applyPresetJson resets everything first), which is safe but
    // makes the table's intent unreadable.
    //
    // No "/" in any name — OuariconPresetManager sanitises it to "_", so
    // "Reverse 1/8" would round-trip as "Reverse 1_8"
    // (critical_preset_name_slash_path_separator). Hence "Rhythmic Reverse".
    //
    // syncMode: 0 = Free, 1 = Sync. noteDivision: 4 = 1/8D, 6 = 1/4 (the default).
    //
    // ── v1.0.1 (A3): density values re-authored ─────────────────────────────
    // density no longer means what it meant at v1.0.0. The old map was
    // overlap = 1 + d·7; the new one is overlap = 2 + d·6. Every preset's
    // density is therefore rewritten to the value that reproduces its SHIPPED
    // overlap exactly:
    //     d_new = (7·d_old − 100) / 6
    // so all eight presets render bit-identically to v1.0.0 (the 0.1 % density
    // step resolves each of these exactly). Only the knob's *scale* moved; the
    // presets did not. Old -> new: 60->53.3, 55->47.5, 70->65, 30->18.3,
    // 90->88.3, 65->59.2, 80->76.7.
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        { "Reverse Bloom",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  500.0f},
           {"grainSize", 200.0f}, {"density", 53.3f}, {"feedback",  40.0f},
           {"lowCut",    100.0f}, {"highCut", 8000.0f},
           {"width",      60.0f}, {"mix",       40.0f}}, {} },

        { "Guitar Swell",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  700.0f},
           {"grainSize", 300.0f}, {"density", 47.5f}, {"feedback",  45.0f},
           {"lowCut",    120.0f}, {"highCut", 6500.0f},
           {"width",      55.0f}, {"mix",       55.0f}}, {} },

        { "Vocal Halo",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  380.0f},
           {"grainSize", 180.0f}, {"density", 65.0f}, {"feedback",  30.0f},
           {"lowCut",    300.0f}, {"highCut", 7000.0f},
           {"width",      70.0f}, {"mix",       25.0f}}, {} },

        { "Slow Wash",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime", 1400.0f},
           {"grainSize", 450.0f}, {"density", 18.3f}, {"feedback",  65.0f},
           {"lowCut",     80.0f}, {"highCut", 5000.0f},
           {"width",      85.0f}, {"mix",       50.0f}}, {} },

        { "Tight Smear",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  180.0f},
           {"grainSize",  70.0f}, {"density", 88.3f}, {"feedback",  35.0f},
           {"lowCut",    150.0f}, {"highCut", 11000.0f},
           {"width",      35.0f}, {"mix",       45.0f}}, {} },

        { "Dark Cavern",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  850.0f},
           {"grainSize", 320.0f}, {"density", 59.2f}, {"feedback",  70.0f},
           {"lowCut",    220.0f}, {"highCut", 1800.0f},
           {"width",      75.0f}, {"mix",       55.0f}}, {} },

        // feedback = 100 %: doubles as the preset-driven DSP-03 stability
        // statement (probe N renders this one for 30 s, not 10). Its density is
        // re-authored to hold overlap at 5.9, so the v1.0.1 loop duty cycle —
        // and with it the measured decay — is unchanged from v1.0.0.
        { "Near-Infinite",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  900.0f},
           {"grainSize", 350.0f}, {"density", 65.0f}, {"feedback", 100.0f},
           {"lowCut",    180.0f}, {"highCut", 2500.0f},
           {"width",      80.0f}, {"mix",       50.0f}}, {} },

        { "Rhythmic Reverse",
          {{"syncMode", 1.0f}, {"noteDivision", 4.0f}, {"delayTime",  500.0f},
           {"grainSize", 120.0f}, {"density", 76.7f}, {"feedback",  50.0f},
           {"lowCut",    140.0f}, {"highCut", 9000.0f},
           {"width",      50.0f}, {"mix",       45.0f}}, {} },
    };

    // C1: engineering units → normalised, through each param's own range. Handles
    // skew, step and choice-index uniformly (AudioParameterChoice's range is
    // 0…n-1 step 1, so convertTo0to1(6.0f) on the 13-entry division list = 0.5).
    // initializeFactoryPresets stores the normalised value verbatim;
    // applyPresetJson feeds it back through convertFrom0to1 on load.
    for (auto& preset : factoryPresets)
        for (auto& [id, value] : preset.parameters)
            if (auto* p = parameters.getParameter(id))
                value = p->convertTo0to1(value);

    // Only re-seeds when JucePlugin_VersionString changes (.factory-version
    // sentinel). At a frozen 1.0.0 that means edits to the table above are a
    // SILENT no-op until ~/Library/O-ReverseDelay/Presets/Factory is removed.
    // The v1.0.0 -> v1.0.1 CMake VERSION bump is what makes the density and
    // range edits above actually reach disk.
    presetManager.initializeFactoryPresets(factoryPresets);

    // A1: user presets are stored as NORMALISED fractions and must be rescaled
    // against the widened delayTime range. Runs immediately after the factory
    // seed, on the message thread, once per version.
    migrateUserPresets();
}

//==============================================================================
// v1.0.1 (A1) — user-preset delayTime migration.
//
// Two DIFFERENT storage formats are in play, and they need opposite treatment:
//
//   * Session state (getStateInformation) is an APVTS ValueTree, and APVTS
//     stores each PARAM's DENORMALISED value — literal milliseconds. JUCE
//     restores it through ParameterAdapter::setDenormalisedValue(), which
//     re-normalises against whatever range is current. A v1.0.0 session saved
//     at 1400 ms therefore recalls 1400 ms under the 4000 ms range with no
//     migration at all — and rescaling it would actively corrupt it. Nothing to
//     do here; harness probe P asserts the round trip.
//
//   * Preset JSON (OuariconPresetManager::createPresetJson) stores
//     RangedAudioParameter::getValue(), i.e. the normalised 0..1 fraction, and
//     applyPresetJson feeds it straight back to setValueNotifyingHost(). Those
//     fractions DO shift meaning when the range max moves: a v1.0.0 preset saved
//     at 500 ms holds ~0.606, which under the 4000 ms range reads back as
//     ~1240 ms. Those files are what this function rewrites.
//
// Factory presets need no migration — the version bump re-seeds them from the
// engineering-unit table above.
//
// Guarded by a version sentinel mirroring initializeFactoryPresets()'s: without
// it every processor construction (each auval/pluginval scan pass, each instance
// added to a session) would re-read every user preset on the message thread, and
// two instances constructing concurrently would race on the same files. The
// trade-off is that a v1.0.0 preset restored from a backup AFTER the sentinel is
// stamped will not be migrated.
void ReverseDelayProcessor::migrateUserPresets()
{
    auto userDir  = presetManager.getUserPresetsDirectory();
    auto sentinel = presetManager.getPresetsDirectory().getChildFile(".user-migration-version");

    if (sentinel.existsAsFile()
        && sentinel.loadFileAsString().trim() == JucePlugin_VersionString)
        return;

    if (userDir.isDirectory())
    {
        // Reconstruct v1.0.0's delayTime range exactly: same min, same skew
        // centre, only the max differs. setSkewForCentre re-solves the exponent
        // against the max, so the two curves are genuinely different mappings —
        // this is not a linear rescale.
        juce::NormalisableRange<float> legacyRange { 50.0f, kLegacyDelayTimeMaxMs, 0.01f };
        legacyRange.setSkewForCentre(kDelayTimeSkewCentreMs);

        auto* delayParam = parameters.getParameter("delayTime");

        for (const auto& file : userDir.findChildFiles(juce::File::findFiles, false, "*.json"))
        {
            auto data = juce::JSON::parse(file.loadFileAsString());
            auto* obj = data.getDynamicObject();
            if (obj == nullptr || delayParam == nullptr)
                continue;

            // Only v1.0.0 files carry the old mapping. A missing "version" is
            // treated as 1.0.0 — that field has been written since the preset
            // manager's first release, so absence means hand-authored/ancient.
            const juce::String version = obj->hasProperty("version")
                                           ? obj->getProperty("version").toString()
                                           : juce::String("1.0.0");

            if (version != "1.0.0")
                continue;

            auto paramsVar = obj->getProperty("parameters");
            auto* params   = paramsVar.getDynamicObject();

            if (params != nullptr && params->hasProperty("delayTime"))
            {
                const float oldNorm = static_cast<float>(
                    static_cast<double>(params->getProperty("delayTime")));
                const float ms      = legacyRange.convertFrom0to1(juce::jlimit(0.0f, 1.0f, oldNorm));
                const float newNorm = delayParam->getNormalisableRange().convertTo0to1(ms);

                params->setProperty("delayTime", newNorm);
            }

            obj->setProperty("version", JucePlugin_VersionString);
            file.replaceWithText(juce::JSON::toString(data, true));
        }
    }

    sentinel.getParentDirectory().createDirectory();
    sentinel.replaceWithText(JucePlugin_VersionString);
}

//==============================================================================
void ReverseDelayProcessor::reset()
{
    // C: hosts call reset() to drop tail state between transport passes. v1.0.0
    // had no override, so the capture ring, in-flight grains and filter memory
    // survived and bled a stale reverse tail into the next pass. Everything here
    // is alloc-free (CaptureBuffer::clear never calls setSize).
    capture.clear();
    grainPool.clear();
    scheduler.clear();

    hpL.reset(); hpR.reset();
    lpL.reset(); lpR.reset();

    wetScratch.clear();
    fbScratch.clear();

    // Same fixed seed as prepareToPlay — a reset must not desynchronise the
    // width-spread sequence the harness depends on.
    rngState = 0x12345678u;
    panSign  = 1.0f;

    // Jump the smoothers to their current targets rather than ramping from
    // whatever the previous pass ended on.
    if (pFeedback != nullptr) feedbackSmoothed.setCurrentAndTargetValue(pFeedback->load() * 0.01f);
    if (pMix      != nullptr) mixSmoothed.setCurrentAndTargetValue(pMix->load() * 0.01f);
    if (pLowCut   != nullptr) lowCutSmoothed.setCurrentAndTargetValue(pLowCut->load());
    if (pHighCut  != nullptr) highCutSmoothed.setCurrentAndTargetValue(pHighCut->load());
}

ReverseDelayProcessor::~ReverseDelayProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout ReverseDelayProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // delayTime: 50–4000 ms, default 500, skew centred on 316 ms.
    //
    // v1.0.1 (A1): max raised 2000 -> 4000 ms. At 2000 ms the sync clamp silently
    // collapsed 1/1 below 120 BPM, 1/2D below 90 and 1/2 below 60 — i.e. across the
    // whole 70–100 BPM band this plugin is written for, the UI named a division it
    // was not playing and two divisions landed on the same delay. 4000 ms covers
    // 1/1 down to 60 BPM.
    //
    // The skew centre deliberately stays at 316 ms: setSkewForCentre re-solves the
    // exponent against the new max, so short times keep their knob resolution
    // instead of being crushed into the bottom of a linear-ish 4 s sweep.
    {
        juce::NormalisableRange<float> range { 50.0f, kDelayTimeMaxMs, 0.01f };
        range.setSkewForCentre(kDelayTimeSkewCentreMs);
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
    // v1.0.1 (A1): 3.5 -> 5.5 s. The ring must cover Dmax + 2·Gmax, and Dmax grew
    // 2.0 -> 4.0 s, so the requirement is 4.0 + 2·0.5 = 5.0 s. Costs ~2.1 MB
    // stereo at 48 kHz.
    capture.prepare(sampleRate, kCaptureSeconds);
    scheduler.prepare(sampleRate);
    grainPool.clear();

    // Deterministic width-spread sequence per prepare (harness reproducibility).
    rngState = 0x12345678u;
    panSign  = 1.0f;

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

    // In-loop damping filters. prepare() + reset() here; coefficients seeded now
    // so the cached-cutoff guards start valid (the guard only gates recompute).
    const juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(maxBlock), 1 };
    hpL.prepare(spec); hpR.prepare(spec);
    lpL.prepare(spec); lpR.prepare(spec);
    hpL.reset(); hpR.reset();
    lpL.reset(); lpR.reset();

    const float fsF = static_cast<float>(sampleRate);
    lastLowCut  = juce::jlimit(20.0f,  0.49f * fsF, lowCutSmoothed.getCurrentValue());
    lastHighCut = juce::jlimit(500.0f, 0.49f * fsF, highCutSmoothed.getCurrentValue());

    const auto hpCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(sampleRate, lastLowCut);
    const auto lpCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, lastHighCut);
    *hpL.coefficients = hpCoeffs;  *hpR.coefficients = hpCoeffs;
    *lpL.coefficients = lpCoeffs;  *lpR.coefficients = lpCoeffs;
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

    const int numInputChannels  = juce::jmin(getTotalNumInputChannels(),  buffer.getNumChannels());
    const int numOutputChannels = juce::jmin(getTotalNumOutputChannels(), buffer.getNumChannels());

    // Defensive: host delivered a block larger than prepared, or prepareToPlay
    // never ran. Bail without touching the wet path — but pass DRY through rather
    // than emitting whatever the extra output channels happen to hold.
    //
    // (C: the v1.0.0 bare `return` did already leave channel 0 dry — the review's
    // "bails to total silence" reading is wrong. What it genuinely leaked is the
    // mono->stereo case, where output channel 1 is never written by this plugin
    // and would carry stale host memory. Duplicating dry closes that.)
    if (numSamples > wetScratch.getNumSamples() || capture.getBufferSize() == 0)
    {
        for (int ch = numInputChannels; ch < numOutputChannels; ++ch)
            buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);

        return;
    }

    // ---- (0) once-per-block parameter reads (atomic) ------------------------
    // Latched-per-grain parameters — read raw, NEVER smoothed (latching at
    // spawn is the click-free mechanism per contract).
    const float delayTimeMs = pDelayTime->load();
    const float grainSizeMs = pGrainSize->load();
    const float densityPct  = pDensity->load();
    const float widthNorm   = pWidth->load() * 0.01f;
    const bool  syncMode    = pSyncMode->load() >= 0.5f;

    // Smoothed (~20 ms) parameters — set targets once per block.
    feedbackSmoothed.setTargetValue(pFeedback->load() * 0.01f);
    mixSmoothed.setTargetValue(pMix->load() * 0.01f);
    lowCutSmoothed.setTargetValue(pLowCut->load());
    highCutSmoothed.setTargetValue(pHighCut->load());

    // ---- (1) resolve grain anchor delay D -----------------------------------
    // Sync only changes the VALUE of D per block — never the spawn timing (no
    // conditional routing; the scheduler always runs the free countdown).
    // Fallback to the free delayTime when the playhead is null OR getBpm() is
    // empty (COMPAT-02). D is consumed only at spawn time, so a Sync<->Free
    // switch changes only next-spawn D — in-flight grains finish on latched
    // values (the click-free mechanism).
    float effectiveDelayMs = delayTimeMs;

    if (syncMode)
    {
        if (auto* playHead = getPlayHead())
        {
            if (const auto position = playHead->getPosition())
            {
                if (const auto bpm = position->getBpm())
                {
                    // noteDivision -> beats, contract order:
                    // 1/16, 1/16D, 1/16T, 1/8, 1/8D, 1/8T, 1/4, 1/4D, 1/4T,
                    // 1/2, 1/2D, 1/2T, 1/1
                    static constexpr double kDivisionBeats[13] = {
                        0.25, 0.375, 1.0 / 6.0,
                        0.5,  0.75,  1.0 / 3.0,
                        1.0,  1.5,   2.0 / 3.0,
                        2.0,  3.0,   4.0 / 3.0,
                        4.0
                    };

                    const int div = juce::jlimit(0, 12, static_cast<int>(pNoteDivision->load()));
                    const double ms = kDivisionBeats[div] * 60000.0 / juce::jmax(1.0, *bpm);
                    // A1: clamp tracks the delayTime range. It MUST stay in sync
                    // with kDelayTimeMaxMs — a stale literal here is exactly the
                    // v1.0.0 defect (division named but not played, silently).
                    effectiveDelayMs = static_cast<float>(
                        juce::jlimit(50.0, static_cast<double>(kDelayTimeMaxMs), ms));
                }
            }
        }
    }

    const int D = juce::jmax(1, static_cast<int>(effectiveDelayMs * 0.001 * currentSampleRate));
    const int G = juce::jmax(2, static_cast<int>(grainSizeMs * 0.001 * currentSampleRate));

    // A3 (v1.0.1): overlap floor raised 1 -> 2. At overlap = 1 the hop equals the
    // grain length, so Hann-windowed grains ABUT rather than overlap and the wet
    // output amplitude-modulates to full silence at every boundary — a 100 %-depth
    // 5 Hz tremolo at grainSize 200 ms, not the "denser wash" the control claims.
    // Hann reaches constant-overlap-add at hop = G/2, i.e. overlap >= 2, so the
    // bottom ~14 % of the knob was a gated-pulse region. 2 + d·6 keeps the same
    // maximum (8) and makes the whole travel a smooth->dense sweep.
    const float overlap         = 2.0f + (densityPct * 0.01f) * 6.0f;
    const int   intervalSamples = juce::jmax(1, static_cast<int>(static_cast<float>(G) / overlap));
    const float grainGain       = 1.0f / std::sqrt(overlap);   // compensation, latched per grain,
                                                               // applied BEFORE the feedback tap

    // ---- (2) advance smoothers + update damping coefficients ----------------
    // mix advances per sample in the mix loop (step 7); feedback gain advances
    // per sample in the feedback fill (step 5). Cutoffs advance at block rate:
    // skip() returns the smoothed value after the block, clamped before the
    // tan() prewarp territory (highCut jlimit 500..0.49*fs per contract).
    const float fsF = static_cast<float>(currentSampleRate);
    const float lc  = juce::jlimit(20.0f,  0.49f * fsF, lowCutSmoothed.skip(numSamples));
    const float hc  = juce::jlimit(500.0f, 0.49f * fsF, highCutSmoothed.skip(numSamples));

    // Cached-cutoff guards gate ONLY the recompute (no enabled flag exists).
    // ArrayCoefficients returns a stack std::array; operator= assigns the
    // normalised values in place into the existing Coefficients — no allocation.
    if (! juce::exactlyEqual (lc, lastLowCut))
    {
        lastLowCut = lc;
        const auto a = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(currentSampleRate, lc);
        *hpL.coefficients = a;
        *hpR.coefficients = a;
    }
    if (! juce::exactlyEqual (hc, lastHighCut))
    {
        lastHighCut = hc;
        const auto a = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(currentSampleRate, hc);
        *lpL.coefficients = a;
        *lpR.coefficients = a;
    }

    // ---- (3)–(6) sub-blocked engine pass ------------------------------------
    // A2 (v1.0.1): a grain spawned at block offset i latches
    // readAbs = blockStart + i − D, but the capture write for this block only
    // happens in step 6. Every read is therefore already-written ONLY while
    // i < D. In v1.0.0 the whole host block ran as one pass, so at
    // blockSize >= D (2205 samples at 44.1 kHz / delayTime 50 ms — i.e. any
    // 2048- or 4096-sample buffer, common in offline bounce and high-latency
    // live rigs) the late grains read positions not yet written this pass:
    // silence early on, then a full ring lap of stale audio.
    //
    // Fix: bound each pass to D samples. i < D then holds by construction at
    // ANY host block size, with no change to the delay time and no partial
    // feedback (which is what "write input first, add feedback second" would
    // leave behind — the block's own regeneration is still missing there).
    // The engine becomes block-size invariant, which is what probe O asserts.
    //
    // Cost at the shipped block size is exactly zero: D >= 2400 at 48 kHz, so
    // a 512-sample block is one pass and the code path is bit-identical to
    // v1.0.0. Only oversized blocks (or very low sample rates) split.
    wetScratch.clear();
    float* wetL = wetScratch.getWritePointer(0);
    float* wetR = wetScratch.getWritePointer(1);
    float* fbLw = fbScratch.getWritePointer(0);
    float* fbRw = fbScratch.getWritePointer(1);

    // Mono input feeds both capture channels (L = R = in).
    const float* inL = buffer.getReadPointer(0);
    const float* inR = numInputChannels > 1 ? buffer.getReadPointer(1) : inL;

    const int passLen = juce::jmax(1, juce::jmin(numSamples, D));

    for (int off = 0; off < numSamples; off += passLen)
    {
        const int len = juce::jmin(passLen, numSamples - off);
        const int passEnd = off + len;

        // ---- (3) schedule spawns, latch per-grain state ---------------------
        // Offsets come back pass-relative; startOffset stays block-relative so
        // the render loop below indexes the shared scratch buffers directly.
        const int spawnCount = scheduler.processBlock(len, intervalSamples, spawnRequests);
        const juce::int64 passStartAbs = capture.getTotalWritten();   // capture write happens in step 6

        for (int s = 0; s < spawnCount; ++s)
        {
            const int passOffset = spawnRequests[static_cast<size_t>(s)].sampleOffset;
            auto& g = grainPool.obtain();

            g.active      = true;
            g.readAbs     = (passStartAbs + static_cast<juce::int64>(passOffset)) - static_cast<juce::int64>(D);
            g.n           = 0;
            g.G           = G;
            g.invG        = 1.0f / static_cast<float>(G);
            g.gain        = grainGain;
            g.age         = 0;
            g.startOffset = off + passOffset;

            // Width spread: per-grain equal-power pan, latched at spawn, never
            // smoothed. Alternating-sign random bias — consecutive grains ping
            // left/right; magnitude in [kPanBias, 1] scaled by width. width=0
            // collapses to pan 0.5 -> gL = gR = 1/sqrt(2) (centered dual-mono).
            panSign = -panSign;
            const float spread = panSign * (kPanBias + (1.0f - kPanBias) * nextRand01());
            const float pan    = 0.5f + widthNorm * 0.5f * spread;
            const float phase  = pan * juce::MathConstants<float>::halfPi;
            g.gL = std::cos(phase);
            g.gR = std::sin(phase);
        }

        // ---- (4) render active grains into wetScratch (overlap-add) ---------
        for (auto& g : grainPool.grains)
        {
            if (!g.active)
                continue;

            // Grains spawned in THIS pass carry a block-relative startOffset
            // inside [off, passEnd); grains carried over from an earlier pass
            // or block have it cleared to 0 and resume at the pass start.
            const int start = juce::jmax(off, g.startOffset);
            const int end   = juce::jmin(passEnd, start + (g.G - g.n));
            g.startOffset   = 0;

            if (end <= start)
                continue;

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

            if (g.n >= g.G)
                g.active = false;
        }

        // ---- (5) feedback return: wet → fbGain → HP → LP → tanh → guard -----
        // Tap is post grain-gain (overlap compensation already applied in step 4 —
        // loop gain stays density-independent), pre width/mix (they never enter
        // the loop). tanh bounds the loop to ±1 at any feedback setting.
        {
            float acc = 0.0f;   // NaN detector: tanh output is bounded, so only NaN can escape

            for (int i = off; i < passEnd; ++i)
            {
                const float g = feedbackSmoothed.getNextValue();
                float l = hpL.processSample(wetL[i] * g);
                float r = hpR.processSample(wetR[i] * g);
                l = lpL.processSample(l);
                r = lpR.processSample(r);
                l = std::tanh(l);
                r = std::tanh(r);
                fbLw[i] = l;
                fbRw[i] = r;
                acc += l + r;
            }

            // Non-finite guard at the loop write point: reset BOTH filter pairs AND
            // zero the feedback source for this pass; keep last-known-good
            // coefficients (sticky-silence pattern — never reset only the filters).
            if (! std::isfinite(acc))
            {
                hpL.reset(); hpR.reset();
                lpL.reset(); lpR.reset();
                juce::FloatVectorOperations::clear(fbLw + off, len);
                juce::FloatVectorOperations::clear(fbRw + off, len);
            }
        }

        // ---- (6) capture write: input + feedback return ---------------------
        for (int i = off; i < passEnd; ++i)
            capture.pushSample(inL[i] + fbLw[i], inR[i] + fbRw[i]);
    }

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

// The editor include lives INSIDE the guard: the Stage-2 render harness compiles
// this file with JUCE_WEB_BROWSER=0 and no editor sources, so a top-of-file
// include would make the harness un-buildable the moment the editor gained
// WebView types (pattern_render_harness_breaks_on_webview_editor). PluginProcessor.h
// stays editor-include-free for the same reason.
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* ReverseDelayProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new ReverseDelayEditor(*this);
#else
    return new juce::GenericAudioProcessorEditor(*this);   // harness build
#endif
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

// Stage 4: session state routed through OuariconPresetManager so the current
// preset NAME survives a save/reload alongside the parameters.
//
// Backward compatible in both directions: getStateAsXml() wraps the SAME APVTS
// root, adding only a `currentPreset` attribute, and setStateFromXml() accepts a
// plain pre-Stage-4 APVTS tree (the attribute simply defaults to "Default").
// Stage 1–3 sessions therefore still load. No setCustomStateCallbacks — this
// plugin holds no state outside the APVTS.
void ReverseDelayProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void ReverseDelayProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        presetManager.setStateFromXml(xml.get());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverseDelayProcessor();
}
