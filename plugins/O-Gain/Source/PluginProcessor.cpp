/*
  ==============================================================================

    O-Gain - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    DSP Architecture:
      Input -> Phase Invert -> Channel Swap -> M/S Encode -> Mono Sum
           -> Input Metering -> Learn Measurement (K-weight, LUFS)
           -> Apply Gain (gain_offset + trim)
           -> Output Metering -> Output

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================================
// K-Weight Filter Coefficients
// ITU-R BS.1770-5 published coefficients at 48 kHz
// =============================================================================

namespace KWeight
{
    // Pre-filter (high-shelf ~+4dB above 2kHz) at 48 kHz
    constexpr double pre_b0_48k =  1.53512485958697;
    constexpr double pre_b1_48k = -2.69169618940638;
    constexpr double pre_b2_48k =  1.19839281085285;
    constexpr double pre_a0_48k =  1.0;
    constexpr double pre_a1_48k = -1.69065929318241;
    constexpr double pre_a2_48k =  0.73248077421585;

    // RLB high-pass filter at 48 kHz
    constexpr double rlb_b0_48k =  1.0;
    constexpr double rlb_b1_48k = -2.0;
    constexpr double rlb_b2_48k =  1.0;
    constexpr double rlb_a0_48k =  1.0;
    constexpr double rlb_a1_48k = -1.99004745483398;
    constexpr double rlb_a2_48k =  0.99007225036621;

    // Pre-calculated coefficients for common sample rates
    // These are derived via bilinear transform from the analog prototype

    // ---- 44100 Hz ----
    constexpr double pre_b0_44k =  1.5308412300498355;
    constexpr double pre_b1_44k = -2.6509799951547297;
    constexpr double pre_b2_44k =  1.1690790799215869;
    constexpr double pre_a1_44k = -1.6636551132560204;
    constexpr double pre_a2_44k =  0.7125954280732254;

    constexpr double rlb_b0_44k =  1.0;
    constexpr double rlb_b1_44k = -2.0;
    constexpr double rlb_b2_44k =  1.0;
    constexpr double rlb_a1_44k = -1.9891696736297957;
    constexpr double rlb_a2_44k =  0.9891990357870394;

    // ---- 88200 Hz ----
    constexpr double pre_b0_88k =  1.5718655948453216;
    constexpr double pre_b1_88k = -2.8246403771498979;
    constexpr double pre_b2_88k =  1.2680661998373718;
    constexpr double pre_a1_88k = -1.8416597323498903;
    constexpr double pre_a2_88k =  0.8564426084498903;

    constexpr double rlb_b0_88k =  1.0;
    constexpr double rlb_b1_88k = -2.0;
    constexpr double rlb_b2_88k =  1.0;
    constexpr double rlb_a1_88k = -1.9946144559930252;
    constexpr double rlb_a2_88k =  0.9946217070140947;

    // ---- 96000 Hz ----
    constexpr double pre_b0_96k =  1.5766494674498160;
    constexpr double pre_b1_96k = -2.8392814014246960;
    constexpr double pre_b2_96k =  1.2780537318498160;
    constexpr double pre_a1_96k = -1.8530804608247200;
    constexpr double pre_a2_96k =  0.8694985837247200;

    constexpr double rlb_b0_96k =  1.0;
    constexpr double rlb_b1_96k = -2.0;
    constexpr double rlb_b2_96k =  1.0;
    constexpr double rlb_a1_96k = -1.9950656972399765;
    constexpr double rlb_a2_96k =  0.9950720717900230;
}

// =============================================================================
// Tuning Constants
// =============================================================================

namespace
{
    // Peak-hold meter decay time constant (seconds). Applied as a per-block
    // coefficient pow(perSample, numSamples) so decay is block-size independent.
    constexpr float  kPeakDecaySeconds    = 0.3f;

    // Learn safety guards (CR-02): refuse to derive a gain from an unusable capture.
    constexpr double kLearnMinSeconds     = 1.0;    // minimum capture length before a gain write
    constexpr double kLearnInvalidLevelDB = -70.0;  // measured level at/below this = no usable signal

    // Gain-stage ramp (seconds): click-free smoothing on gain changes (IN-06).
    constexpr double kGainRampSeconds     = 0.02;   // 20 ms

    // VU ballistics (IN-06): ANSI standard 300 ms integration, symmetric.
    constexpr float  kVuBallisticsMs      = 300.0f;

    // BS.1770 loudness windowing (IN-06).
    constexpr double kLufsBlockSeconds    = 0.4;    // 400 ms integration block
    constexpr double kLufsHopSeconds      = 0.1;    // 100 ms hop (75% overlap)
    constexpr int    kMaxGatingBlocks     = 4000;   // ~400 s of learn at 100 ms hop

    // Learn confidence thresholds (IN-06): based on elapsed time + gating blocks.
    constexpr float  kConfidenceLowSeconds    = 5.0f;   // below this  -> low
    constexpr float  kConfidenceMediumSeconds = 15.0f;  // below this  -> medium, else high
    constexpr int    kConfidenceMinBlocks     = 50;     // fewer gating blocks than this -> low

    // WR-04: recompute the running (display-only) integrated LUFS at ~1 Hz
    // (every 10 hops of 100 ms) instead of on every hop.
    constexpr int    kIntegratedRecomputeHops = 10;

    // WR-03: Learn safety ceiling. truePeak/samplePeak here is an un-oversampled
    // digital sample peak, so inter-sample peaks (ISPs) — which can exceed the
    // sample peak by a few dB — are not measured. Drop the effective ceiling by
    // kInterSamplePeakHeadroomDB below the nominal -1 dBFS target so the applied
    // gain still leaves room for those un-measured ISPs.
    constexpr double kLearnCeilingDBFS          = -1.0;  // nominal sample-peak ceiling
    constexpr double kInterSamplePeakHeadroomDB =  3.0;  // extra headroom for un-measured ISPs
}

// =============================================================================
// Parameter Layout
// =============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout OGainAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // gain_offset - Calculated by learn mode or manually adjustable (-40 to +40 dB)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gain_offset", 1 },
        "Gain Offset",
        juce::NormalisableRange<float>(-40.0f, 40.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // trim - Manual fine adjustment (-6 to +6 dB)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "trim", 1 },
        "Trim",
        juce::NormalisableRange<float>(-6.0f, 6.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // target_level - Target level for learn mode (-36 to 0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "target_level", 1 },
        "Target Level",
        juce::NormalisableRange<float>(-36.0f, 0.0f, 0.1f),
        -18.0f,
        "dB"
    ));

    // measurement_mode - Which metric to use for learn mode (LUFS or RMS)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "measurement_mode", 1 },
        "Measurement Mode",
        juce::StringArray { "LUFS", "RMS" },
        0  // Default: LUFS
    ));

    // meter_mode - Display meter type (Peak, RMS, VU, LUFS)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "meter_mode", 1 },
        "Meter Mode",
        juce::StringArray { "Peak", "RMS", "VU", "LUFS" },
        2  // Default: VU
    ));

    // phase_invert_l - Left channel polarity inversion
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "phase_invert_l", 1 },
        "Phase Invert L",
        false
    ));

    // phase_invert_r - Right channel polarity inversion
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "phase_invert_r", 1 },
        "Phase Invert R",
        false
    ));

    // channel_swap - L/R channel swap
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "channel_swap", 1 },
        "Channel Swap",
        false
    ));

    // mono_sum - Mono summing
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "mono_sum", 1 },
        "Mono Sum",
        false
    ));

    // ms_mode - Mid-Side processing mode (Off, Encode, Decode)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "ms_mode", 1 },
        "M/S Mode",
        juce::StringArray { "Off", "Encode", "Decode" },
        0  // Default: Off
    ));

    return layout;
}

// =============================================================================
// Constructor
// =============================================================================

OGainAudioProcessor::OGainAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // IN-02: watch the gain controls so a manual edit after a Learn clears the
    // "DONE" display (see parameterChanged).
    parameters.addParameterListener("gain_offset", this);
    parameters.addParameterListener("trim", this);
}

OGainAudioProcessor::~OGainAudioProcessor()
{
    cancelPendingUpdate(); // AsyncUpdater: drop any queued learn finalize (CR-01)
    parameters.removeParameterListener("gain_offset", this);
    parameters.removeParameterListener("trim", this);
}

// =============================================================================
// K-Weight Filter Setup
// =============================================================================

void OGainAudioProcessor::setupKWeightFilters(double sampleRate)
{
    double pre_b0, pre_b1, pre_b2, pre_a1, pre_a2;
    double rlb_b0, rlb_b1, rlb_b2, rlb_a1, rlb_a2;

    // Select pre-calculated coefficients based on sample rate
    int sr = static_cast<int>(sampleRate + 0.5);

    switch (sr)
    {
        case 44100:
            pre_b0 = KWeight::pre_b0_44k; pre_b1 = KWeight::pre_b1_44k; pre_b2 = KWeight::pre_b2_44k;
            pre_a1 = KWeight::pre_a1_44k; pre_a2 = KWeight::pre_a2_44k;
            rlb_b0 = KWeight::rlb_b0_44k; rlb_b1 = KWeight::rlb_b1_44k; rlb_b2 = KWeight::rlb_b2_44k;
            rlb_a1 = KWeight::rlb_a1_44k; rlb_a2 = KWeight::rlb_a2_44k;
            break;

        case 88200:
            pre_b0 = KWeight::pre_b0_88k; pre_b1 = KWeight::pre_b1_88k; pre_b2 = KWeight::pre_b2_88k;
            pre_a1 = KWeight::pre_a1_88k; pre_a2 = KWeight::pre_a2_88k;
            rlb_b0 = KWeight::rlb_b0_88k; rlb_b1 = KWeight::rlb_b1_88k; rlb_b2 = KWeight::rlb_b2_88k;
            rlb_a1 = KWeight::rlb_a1_88k; rlb_a2 = KWeight::rlb_a2_88k;
            break;

        case 96000:
            pre_b0 = KWeight::pre_b0_96k; pre_b1 = KWeight::pre_b1_96k; pre_b2 = KWeight::pre_b2_96k;
            pre_a1 = KWeight::pre_a1_96k; pre_a2 = KWeight::pre_a2_96k;
            rlb_b0 = KWeight::rlb_b0_96k; rlb_b1 = KWeight::rlb_b1_96k; rlb_b2 = KWeight::rlb_b2_96k;
            rlb_a1 = KWeight::rlb_a1_96k; rlb_a2 = KWeight::rlb_a2_96k;
            break;

        case 48000:
        default:
            // Use published 48 kHz coefficients as default
            pre_b0 = KWeight::pre_b0_48k; pre_b1 = KWeight::pre_b1_48k; pre_b2 = KWeight::pre_b2_48k;
            pre_a1 = KWeight::pre_a1_48k; pre_a2 = KWeight::pre_a2_48k;
            rlb_b0 = KWeight::rlb_b0_48k; rlb_b1 = KWeight::rlb_b1_48k; rlb_b2 = KWeight::rlb_b2_48k;
            rlb_a1 = KWeight::rlb_a1_48k; rlb_a2 = KWeight::rlb_a2_48k;
            break;
    }

    // Create coefficient objects (b0, b1, b2, a0, a1, a2 format)
    // Using ref-counted smart pointers as JUCE expects
    auto preCoeffs = juce::dsp::IIR::Coefficients<double>::Ptr(
        new juce::dsp::IIR::Coefficients<double>(
            pre_b0, pre_b1, pre_b2, 1.0, pre_a1, pre_a2));

    auto rlbCoeffs = juce::dsp::IIR::Coefficients<double>::Ptr(
        new juce::dsp::IIR::Coefficients<double>(
            rlb_b0, rlb_b1, rlb_b2, 1.0, rlb_a1, rlb_a2));

    // Assign to filters
    kWeightPreFilterL.coefficients = preCoeffs;
    kWeightPreFilterR.coefficients = preCoeffs;
    kWeightRlbFilterL.coefficients = rlbCoeffs;
    kWeightRlbFilterR.coefficients = rlbCoeffs;
}

// =============================================================================
// Learn Accumulator Reset
// =============================================================================

void OGainAudioProcessor::resetLearnAccumulators()
{
    // Reset sub-block accumulators
    for (int i = 0; i < kNumSubBlocks; ++i)
    {
        subBlockPowerL[i] = 0.0;
        subBlockPowerR[i] = 0.0;
        subBlockSampleCount[i] = 0;
    }
    currentSubBlock = 0;
    lufsHopCounter = 0;

    // Reset gating block storage
    gatingBlockCount = 0;

    // Reset short-term buffer
    for (int i = 0; i < kShortTermBlocks; ++i)
        shortTermPowers[i] = 0.0;
    shortTermWritePos = 0;
    shortTermBlockCount = 0;

    // Reset sample peak (WR-03)
    samplePeakMax = 0.0;

    // Reset RMS accumulators
    rmsAccumL = 0.0;
    rmsAccumR = 0.0;
    rmsSampleCount = 0;

    // Reset learn sample count
    learnSampleCount = 0.0;

    // Reset the integrated-LUFS recompute throttle (WR-04)
    integratedHopCounter = 0;

    // Reset K-weight filter state (remove history from previous learn sessions)
    kWeightPreFilterL.reset();
    kWeightPreFilterR.reset();
    kWeightRlbFilterL.reset();
    kWeightRlbFilterR.reset();

    // Reset the audio-thread working copy of the Learn snapshot (WR-05). Its
    // state field is set by the caller (learn-start edge / prepareToPlay).
    liveLearn = LearnSnapshot{};
}

// =============================================================================
// Integrated LUFS Calculation (EBU R128 Dual-Gate)
// =============================================================================

double OGainAudioProcessor::calculateIntegratedLUFS() const
{
    if (gatingBlockCount == 0)
        return -100.0;

    // Step 1: Absolute gate at -70 LUFS
    // -70 LUFS corresponds to a mean power threshold
    // L = -0.691 + 10*log10(power) => power = 10^((L + 0.691)/10)
    const double absoluteGateThreshold = std::pow(10.0, (-70.0 + 0.691) / 10.0);

    double absoluteGatedSum = 0.0;
    int absoluteGatedCount = 0;

    for (int i = 0; i < gatingBlockCount; ++i)
    {
        if (gatingBlockPowers[static_cast<size_t>(i)] >= absoluteGateThreshold)
        {
            absoluteGatedSum += gatingBlockPowers[static_cast<size_t>(i)];
            ++absoluteGatedCount;
        }
    }

    if (absoluteGatedCount == 0)
        return -100.0;

    // Step 2: Relative gate at (absolute-gated mean - 10 LU)
    double absoluteGatedMeanPower = absoluteGatedSum / absoluteGatedCount;
    // relativeThreshold in power domain: mean_power * 10^(-10/10) = mean_power * 0.1
    double relativeGateThreshold = absoluteGatedMeanPower * 0.1;

    double relativeGatedSum = 0.0;
    int relativeGatedCount = 0;

    for (int i = 0; i < gatingBlockCount; ++i)
    {
        if (gatingBlockPowers[static_cast<size_t>(i)] >= relativeGateThreshold)
        {
            relativeGatedSum += gatingBlockPowers[static_cast<size_t>(i)];
            ++relativeGatedCount;
        }
    }

    if (relativeGatedCount == 0)
        return -100.0;

    double integratedPower = relativeGatedSum / relativeGatedCount;
    return -0.691 + 10.0 * std::log10(integratedPower);
}

// =============================================================================
// Finalize Learn Mode
// =============================================================================

void OGainAudioProcessor::finalizeLearn()
{
    // MESSAGE THREAD ONLY (invoked from handleAsyncUpdate, CR-01). The audio thread
    // has already cleared learnActive, so the learn accumulators are stable to read.

    const float targetLevel = parameters.getRawParameterValue("target_level")->load();

    // Did we actually capture a usable measurement? (CR-02)
    const bool haveValidMeasurement =
        (learnMeasurementModeAtStart == 0 ? gatingBlockCount > 0 : rmsSampleCount > 0);

    const double learnSeconds = (currentSampleRate > 0.0)
        ? learnSampleCount / currentSampleRate
        : 0.0;

    double measuredLevel;

    if (learnMeasurementModeAtStart == 0) // LUFS
    {
        measuredLevel = calculateIntegratedLUFS();
    }
    else // RMS
    {
        if (rmsSampleCount > 0)
        {
            // Divide by the channel count actually accumulated so a mono capture
            // (R accumulator stays zero) is not under-reported by 3 dB (WR-02).
            const double channels = static_cast<double>(juce::jmax(1, learnChannelsAtStart));
            double avgPower = (rmsAccumL + rmsAccumR)
                            / (channels * static_cast<double>(rmsSampleCount));
            measuredLevel = 20.0 * std::log10(std::sqrt(avgPower) + 1e-30);
        }
        else
        {
            measuredLevel = -100.0;
        }
    }

    // Sample peak (WR-03: un-oversampled digital peak, dBFS) for display + ceiling.
    const double currentSamplePeakDB = (samplePeakMax > 0.0)
        ? 20.0 * std::log10(samplePeakMax)
        : -100.0;

    // Publish the final Learn panel as one coherent unit (WR-05). Start from the
    // last audio-thread snapshot (keeps momentary/short-term/elapsed) and stamp the
    // final integrated + sample-peak + complete state on top.
    LearnSnapshot snap = readLearnSnapshot();
    snap.state          = 2; // complete
    snap.integratedLUFS = static_cast<float>(measuredLevel);
    snap.samplePeakDBFS = static_cast<float>(currentSamplePeakDB);

    // CR-02: refuse to derive a gain from silence / near-silence / too-short a capture.
    // Leaving gain_offset untouched avoids the +40 dB full-scale slam (a loudness /
    // hearing-safety hazard) on a mono instance or a brief tap over a quiet passage.
    if (! haveValidMeasurement
        || measuredLevel <= kLearnInvalidLevelDB
        || learnSeconds < kLearnMinSeconds)
    {
        snap.confidence = 0;       // none / invalid
        publishLearnSnapshot(snap); // still show the user what was (not) measured
        return;
    }

    publishLearnSnapshot(snap);

    // Calculate gain with the sample-peak safety ceiling, dropped by an ISP
    // headroom allowance because sample peak under-reports inter-sample peaks (WR-03).
    double gainDB = static_cast<double>(targetLevel) - measuredLevel;

    const double ceiling     = kLearnCeilingDBFS - kInterSamplePeakHeadroomDB; // effective dBFS
    const double maxSafeGain = ceiling - currentSamplePeakDB;
    if (gainDB > maxSafeGain)
        gainDB = maxSafeGain;

    // Clamp to parameter range.
    gainDB = juce::jlimit(-40.0, 40.0, gainDB);

    // Write to APVTS parameter (host + listener notification — MESSAGE THREAD ONLY).
    if (auto* gainParam = parameters.getParameter("gain_offset"))
    {
        const float normalizedValue = gainParam->convertTo0to1(static_cast<float>(gainDB));
        // IN-02: mark this as a Learn-driven write so parameterChanged does not
        // immediately clear the "DONE" state we just published above.
        ignoreLearnGainWrite.store(true, std::memory_order_release);
        gainParam->setValueNotifyingHost(normalizedValue);
        ignoreLearnGainWrite.store(false, std::memory_order_release);
    }
}

// =============================================================================
// Learn Snapshot seqlock (WR-05) + IN-02 parameter listener
// =============================================================================

void OGainAudioProcessor::publishLearnSnapshot(const LearnSnapshot& s) noexcept
{
    const uint32_t seq = learnSnapshotSeq.load(std::memory_order_relaxed);
    learnSnapshotSeq.store(seq + 1, std::memory_order_release);   // odd: write in progress
    std::atomic_thread_fence(std::memory_order_release);
    learnSnapshotData = s;
    std::atomic_thread_fence(std::memory_order_release);
    learnSnapshotSeq.store(seq + 2, std::memory_order_release);   // even: published
    learnDisplayState.store(s.state, std::memory_order_release);  // cheap mirror for IN-02
}

OGainAudioProcessor::LearnSnapshot OGainAudioProcessor::readLearnSnapshot() const noexcept
{
    LearnSnapshot s;
    uint32_t seqBefore, seqAfter;
    do
    {
        seqBefore = learnSnapshotSeq.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_acquire);
        s = learnSnapshotData;
        std::atomic_thread_fence(std::memory_order_acquire);
        seqAfter = learnSnapshotSeq.load(std::memory_order_acquire);
    } while ((seqBefore & 1u) != 0 || seqBefore != seqAfter); // retry if torn / mid-write
    return s;
}

void OGainAudioProcessor::parameterChanged(const juce::String& parameterID, float)
{
    // IN-02: only interested in the two gain controls.
    if (parameterID != "gain_offset" && parameterID != "trim")
        return;

    // Ignore finalizeLearn's own gain write (it just set state to "DONE").
    if (ignoreLearnGainWrite.load(std::memory_order_acquire))
        return;

    // A genuine user edit after a completed Learn: clear "DONE" back to idle so
    // the button stops reading DONE indefinitely. Do nothing while a learn is
    // still running (state 1) so we never race the audio-thread snapshot writer.
    if (learnDisplayState.load(std::memory_order_acquire) == 2)
    {
        LearnSnapshot snap = readLearnSnapshot();
        snap.state = 0; // idle
        publishLearnSnapshot(snap);
    }
}

// =============================================================================
// AsyncUpdater: message-thread half of the learn-stop handoff (CR-01)
// =============================================================================

void OGainAudioProcessor::handleAsyncUpdate()
{
    if (learnFinalizePending.exchange(false, std::memory_order_acquire))
        finalizeLearn();
}

// =============================================================================
// prepareToPlay
// =============================================================================

void OGainAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Zero latency
    setLatencySamples(0);

    // Prepare gain stage
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    gainStage.prepare(spec);
    gainStage.reset();
    gainStage.setRampDurationSeconds(kGainRampSeconds); // click-free gain changes

    // Prepare VU ballistics filters (single channel each)
    juce::dsp::ProcessSpec monoSpec;
    monoSpec.sampleRate = sampleRate;
    monoSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    monoSpec.numChannels = 1;

    vuBallisticsL.prepare(monoSpec);
    vuBallisticsL.reset();
    vuBallisticsL.setAttackTime(kVuBallisticsMs);   // ANSI VU attack
    vuBallisticsL.setReleaseTime(kVuBallisticsMs);  // symmetric release
    vuBallisticsL.setLevelCalculationType(juce::dsp::BallisticsFilterLevelCalculationType::RMS);

    vuBallisticsR.prepare(monoSpec);
    vuBallisticsR.reset();
    vuBallisticsR.setAttackTime(kVuBallisticsMs);
    vuBallisticsR.setReleaseTime(kVuBallisticsMs);
    vuBallisticsR.setLevelCalculationType(juce::dsp::BallisticsFilterLevelCalculationType::RMS);

    // Prepare K-weight filters
    juce::dsp::ProcessSpec doubleMonoSpec;
    doubleMonoSpec.sampleRate = sampleRate;
    doubleMonoSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    doubleMonoSpec.numChannels = 1;

    kWeightPreFilterL.prepare(doubleMonoSpec);
    kWeightPreFilterR.prepare(doubleMonoSpec);
    kWeightRlbFilterL.prepare(doubleMonoSpec);
    kWeightRlbFilterR.prepare(doubleMonoSpec);

    setupKWeightFilters(sampleRate);

    kWeightPreFilterL.reset();
    kWeightPreFilterR.reset();
    kWeightRlbFilterL.reset();
    kWeightRlbFilterR.reset();

    // Calculate LUFS block and hop sizes
    lufsBlockSize = static_cast<int>(sampleRate * kLufsBlockSeconds);
    lufsHopSize   = static_cast<int>(sampleRate * kLufsHopSeconds);

    // Pre-allocate gating block storage (~400 s of learn at a 100 ms hop)
    gatingBlockPowers.resize(static_cast<size_t>(kMaxGatingBlocks), 0.0);
    gatingBlockCount = 0;

    // Reset all learn state
    resetLearnAccumulators();
    prevLearnActive = false;

    // Reset peak decay state
    inputPeakDecayL  = 0.0f;
    inputPeakDecayR  = 0.0f;
    outputPeakDecayL = 0.0f;
    outputPeakDecayR = 0.0f;
}

// =============================================================================
// releaseResources
// =============================================================================

void OGainAudioProcessor::releaseResources()
{
    // Release large pre-allocated buffers
    gatingBlockPowers.clear();
    gatingBlockPowers.shrink_to_fit();
}

// =============================================================================
// processBlock
// =============================================================================

void OGainAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, numSamples);

    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    if (numChannels < 2)
    {
        // Mono path: just apply gain and basic metering, skip stereo utilities
        // (Simplified -- most DAWs will provide stereo for an effect plugin)
    }

    // =========================================================================
    // Read parameters (atomic, real-time safe)
    // =========================================================================

    const float gainOffsetDB    = parameters.getRawParameterValue("gain_offset")->load();
    const float trimDB          = parameters.getRawParameterValue("trim")->load();
    const bool  phaseInvertL    = parameters.getRawParameterValue("phase_invert_l")->load() > 0.5f;
    const bool  phaseInvertR    = parameters.getRawParameterValue("phase_invert_r")->load() > 0.5f;
    const bool  channelSwap     = parameters.getRawParameterValue("channel_swap")->load() > 0.5f;
    const bool  monoSum         = parameters.getRawParameterValue("mono_sum")->load() > 0.5f;
    const int   msMode          = static_cast<int>(parameters.getRawParameterValue("ms_mode")->load());
    const bool  isLearnActive   = learnActive.load(std::memory_order_acquire);

    // Total gain in dB
    const float totalGainDB = gainOffsetDB + trimDB;

    // =========================================================================
    // STEP 1: Channel Utilities (sample-by-sample for proper interleaving)
    // =========================================================================

    if (numChannels >= 2)
    {
        auto* leftData  = buffer.getWritePointer(0);
        auto* rightData = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            float L = leftData[i];
            float R = rightData[i];

            // 1a. Phase inversion
            if (phaseInvertL) L = -L;
            if (phaseInvertR) R = -R;

            // 1b. Channel swap
            if (channelSwap) std::swap(L, R);

            // 1c. M/S encode/decode (priority over mono sum)
            if (msMode == 1) // Encode
            {
                float mid  = (L + R) * 0.5f;
                float side = (L - R) * 0.5f;
                L = mid;
                R = side;
            }
            else if (msMode == 2) // Decode
            {
                float left  = L + R;  // M + S
                float right = L - R;  // M - S
                L = left;
                R = right;
            }

            // 1d. Mono sum
            if (monoSum && msMode == 0) // Only if M/S is off
            {
                float mono = (L + R) * 0.5f;
                L = mono;
                R = mono;
            }

            leftData[i]  = L;
            rightData[i] = R;
        }
    }

    // =========================================================================
    // STEP 2: Input Metering (post-utilities, pre-gain)
    // =========================================================================

    {
        float peakL = 0.0f, peakR = 0.0f;
        float rmsAccL = 0.0f, rmsAccR = 0.0f;

        if (numChannels >= 2)
        {
            const auto* leftData  = buffer.getReadPointer(0);
            const auto* rightData = buffer.getReadPointer(1);

            for (int i = 0; i < numSamples; ++i)
            {
                float absL = std::abs(leftData[i]);
                float absR = std::abs(rightData[i]);

                if (absL > peakL) peakL = absL;
                if (absR > peakR) peakR = absR;

                rmsAccL += leftData[i] * leftData[i];
                rmsAccR += rightData[i] * rightData[i];
            }
        }
        else if (numChannels == 1)
        {
            const auto* data = buffer.getReadPointer(0);
            for (int i = 0; i < numSamples; ++i)
            {
                float absVal = std::abs(data[i]);
                if (absVal > peakL) peakL = absVal;
                rmsAccL += data[i] * data[i];
            }
            peakR = peakL;
            rmsAccR = rmsAccL;
        }

        // Peak with decay (smooth falloff). WR-01: the ~300ms time constant is
        // per-sample, so raise it to a per-block coefficient. Applying the raw
        // per-sample factor once per block froze the meter (block-size dependent).
        const float perSampleDecay = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * kPeakDecaySeconds));
        const float blockDecay     = std::pow(perSampleDecay, static_cast<float>(numSamples));
        inputPeakDecayL = juce::jmax(peakL, inputPeakDecayL * blockDecay);
        inputPeakDecayR = juce::jmax(peakR, inputPeakDecayR * blockDecay);

        float rmsL = std::sqrt(rmsAccL / static_cast<float>(numSamples));
        float rmsR = std::sqrt(rmsAccR / static_cast<float>(numSamples));

        inputPeakL.store(inputPeakDecayL, std::memory_order_relaxed);
        inputPeakR.store(inputPeakDecayR, std::memory_order_relaxed);
        inputRmsL.store(rmsL, std::memory_order_relaxed);
        inputRmsR.store(rmsR, std::memory_order_relaxed);
    }

    // =========================================================================
    // STEP 3: VU Meter Ballistics (post-utilities, pre-gain)
    // =========================================================================

    if (numChannels >= 2)
    {
        const auto* leftData  = buffer.getReadPointer(0);
        const auto* rightData = buffer.getReadPointer(1);

        // Process each sample through ballistics filter
        for (int i = 0; i < numSamples; ++i)
        {
            float vuL = vuBallisticsL.processSample(0, std::abs(leftData[i]));
            float vuR = vuBallisticsR.processSample(0, std::abs(rightData[i]));

            // Store final sample values (last sample of block is representative)
            if (i == numSamples - 1)
            {
                vuLevelL.store(vuL, std::memory_order_relaxed);
                vuLevelR.store(vuR, std::memory_order_relaxed);
            }
        }
    }
    else if (numChannels == 1)
    {
        // Mono path (WR-02): drive the L ballistics filter from channel 0 and
        // mirror the result into both VU meters so the display is not dead in mono.
        const auto* data = buffer.getReadPointer(0);

        float vu = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            vu = vuBallisticsL.processSample(0, std::abs(data[i]));

        vuLevelL.store(vu, std::memory_order_relaxed);
        vuLevelR.store(vu, std::memory_order_relaxed);
    }

    // =========================================================================
    // STEP 4: Learn Mode Measurement (K-weight + LUFS accumulation)
    // =========================================================================

    // Edge detection: learn just started or just stopped
    if (isLearnActive && !prevLearnActive)
    {
        // Learn just started
        learnMeasurementModeAtStart = static_cast<int>(
            parameters.getRawParameterValue("measurement_mode")->load());
        learnChannelsAtStart = numChannels; // 1 = mono, 2 = stereo (WR-02)
        resetLearnAccumulators();           // also clears liveLearn
        liveLearn.state = 1;                // learning
        publishLearnSnapshot(liveLearn);    // WR-05
    }
    else if (!isLearnActive && prevLearnActive)
    {
        // Learn just stopped. Do NOT compute/write the gain here (CR-01): the
        // measurement math and setValueNotifyingHost() must not run on the audio
        // thread. Flag it and hand off to the message thread via AsyncUpdater,
        // which publishes the final "complete" snapshot from finalizeLearn (WR-05).
        learnFinalizePending.store(true, std::memory_order_release);
        triggerAsyncUpdate();
    }

    prevLearnActive = isLearnActive;

    if (isLearnActive && numChannels >= 1)
    {
        // Mono support (WR-02): feed channel 0 into the L accumulators only. The R
        // accumulators stay zero, so the LUFS block power (meanPowerL + meanPowerR)
        // collapses to the correct single-channel loudness, and the RMS divisor uses
        // learnChannelsAtStart == 1 in finalizeLearn().
        const bool  stereo    = (numChannels >= 2);
        const auto* leftData  = buffer.getReadPointer(0);
        const auto* rightData = stereo ? buffer.getReadPointer(1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            const double sampleL = static_cast<double>(leftData[i]);

            // Sample-peak detection (WR-03: un-oversampled digital peak, dBFS)
            double absL = std::abs(sampleL);
            if (absL > samplePeakMax) samplePeakMax = absL;

            // RMS accumulation (for RMS measurement mode)
            rmsAccumL += sampleL * sampleL;

            // K-weight filtering: pre-filter then RLB
            double kWeightedL = kWeightPreFilterL.processSample(sampleL);
            kWeightedL = kWeightRlbFilterL.processSample(kWeightedL);
            subBlockPowerL[currentSubBlock] += kWeightedL * kWeightedL;

            if (stereo)
            {
                const double sampleR = static_cast<double>(rightData[i]);

                double absR = std::abs(sampleR);
                if (absR > samplePeakMax) samplePeakMax = absR;

                rmsAccumR += sampleR * sampleR;

                double kWeightedR = kWeightPreFilterR.processSample(sampleR);
                kWeightedR = kWeightRlbFilterR.processSample(kWeightedR);
                subBlockPowerR[currentSubBlock] += kWeightedR * kWeightedR;
            }

            ++rmsSampleCount;
            subBlockSampleCount[currentSubBlock]++;

            learnSampleCount += 1.0;
            lufsHopCounter++;

            // When we complete a hop (100ms worth of samples)
            if (lufsHopCounter >= lufsHopSize)
            {
                lufsHopCounter = 0;

                // Calculate 400ms block power (sum of all 4 sub-blocks)
                // Only if we have accumulated at least 4 sub-blocks (first 400ms)
                int totalSubBlocks = 0;
                double totalPowerL = 0.0;
                double totalPowerR = 0.0;
                int totalSamples = 0;

                for (int sb = 0; sb < kNumSubBlocks; ++sb)
                {
                    if (subBlockSampleCount[sb] > 0)
                    {
                        totalPowerL += subBlockPowerL[sb];
                        totalPowerR += subBlockPowerR[sb];
                        totalSamples += subBlockSampleCount[sb];
                        ++totalSubBlocks;
                    }
                }

                if (totalSubBlocks == kNumSubBlocks && totalSamples > 0)
                {
                    // Mean power per channel (stereo: equal weight 1.0 for L and R)
                    double meanPowerL = totalPowerL / static_cast<double>(totalSamples);
                    double meanPowerR = totalPowerR / static_cast<double>(totalSamples);
                    double blockMeanPower = meanPowerL + meanPowerR; // sum across channels per BS.1770

                    // Store block power for integrated LUFS gating
                    if (gatingBlockCount < static_cast<int>(gatingBlockPowers.size()))
                    {
                        gatingBlockPowers[static_cast<size_t>(gatingBlockCount)] = blockMeanPower;
                        ++gatingBlockCount;
                    }

                    // Momentary LUFS (this single 400ms block)
                    liveLearn.momentaryLUFS = (blockMeanPower > 0.0)
                        ? static_cast<float>(-0.691 + 10.0 * std::log10(blockMeanPower))
                        : -100.0f;

                    // Short-term buffer (last 3s = 30 blocks at 100ms hop)
                    shortTermPowers[shortTermWritePos] = blockMeanPower;
                    shortTermWritePos = (shortTermWritePos + 1) % kShortTermBlocks;
                    if (shortTermBlockCount < kShortTermBlocks)
                        ++shortTermBlockCount;

                    // Calculate short-term LUFS (mean of last 3s)
                    double stSum = 0.0;
                    for (int sb = 0; sb < shortTermBlockCount; ++sb)
                        stSum += shortTermPowers[sb];

                    if (shortTermBlockCount > 0 && stSum > 0.0)
                    {
                        double stMeanPower = stSum / static_cast<double>(shortTermBlockCount);
                        liveLearn.shortTermLUFS = static_cast<float>(-0.691 + 10.0 * std::log10(stMeanPower));
                    }

                    // Running integrated LUFS (dual-gate). WR-04: the recompute is
                    // O(gatingBlockCount); it is display-only, so throttle it to
                    // ~1 Hz (first qualifying block + every kIntegratedRecomputeHops)
                    // rather than paying it on every 100 ms hop. The authoritative
                    // final value is computed once in finalizeLearn().
                    if ((integratedHopCounter++ % kIntegratedRecomputeHops) == 0)
                        liveLearn.integratedLUFS = static_cast<float>(calculateIntegratedLUFS());
                }

                // Advance to next sub-block (circular)
                int nextSubBlock = (currentSubBlock + 1) % kNumSubBlocks;
                subBlockPowerL[nextSubBlock] = 0.0;
                subBlockPowerR[nextSubBlock] = 0.0;
                subBlockSampleCount[nextSubBlock] = 0;
                currentSubBlock = nextSubBlock;

                // Update sample peak (WR-03: dBFS, un-oversampled)
                if (samplePeakMax > 0.0)
                    liveLearn.samplePeakDBFS = static_cast<float>(20.0 * std::log10(samplePeakMax));

                // Update elapsed time
                const float elapsed = static_cast<float>(learnSampleCount / currentSampleRate);
                liveLearn.elapsedSeconds = elapsed;

                // Update confidence indicator
                if (elapsed < kConfidenceLowSeconds || gatingBlockCount < kConfidenceMinBlocks)
                    liveLearn.confidence = 1; // low
                else if (elapsed < kConfidenceMediumSeconds)
                    liveLearn.confidence = 2; // medium
                else
                    liveLearn.confidence = 3; // high

                // Publish the whole Learn panel coherently once per hop (WR-05).
                liveLearn.state = 1; // learning
                publishLearnSnapshot(liveLearn);
            }
        }
    }

    // =========================================================================
    // STEP 5: Apply Gain (gain_offset + trim)
    // =========================================================================

    gainStage.setGainDecibels(totalGainDB);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    gainStage.process(context);

    // =========================================================================
    // STEP 6: Output Metering (post-gain)
    // =========================================================================

    {
        float peakL = 0.0f, peakR = 0.0f;
        float rmsAccL = 0.0f, rmsAccR = 0.0f;

        if (numChannels >= 2)
        {
            const auto* leftData  = buffer.getReadPointer(0);
            const auto* rightData = buffer.getReadPointer(1);

            for (int i = 0; i < numSamples; ++i)
            {
                float absL = std::abs(leftData[i]);
                float absR = std::abs(rightData[i]);

                if (absL > peakL) peakL = absL;
                if (absR > peakR) peakR = absR;

                rmsAccL += leftData[i] * leftData[i];
                rmsAccR += rightData[i] * rightData[i];
            }
        }
        else if (numChannels == 1)
        {
            const auto* data = buffer.getReadPointer(0);
            for (int i = 0; i < numSamples; ++i)
            {
                float absVal = std::abs(data[i]);
                if (absVal > peakL) peakL = absVal;
                rmsAccL += data[i] * data[i];
            }
            peakR = peakL;
            rmsAccR = rmsAccL;
        }

        // Peak with decay. WR-01: per-block coefficient (see input metering above).
        const float perSampleDecay = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * kPeakDecaySeconds));
        const float blockDecay     = std::pow(perSampleDecay, static_cast<float>(numSamples));
        outputPeakDecayL = juce::jmax(peakL, outputPeakDecayL * blockDecay);
        outputPeakDecayR = juce::jmax(peakR, outputPeakDecayR * blockDecay);

        float rmsL = std::sqrt(rmsAccL / static_cast<float>(numSamples));
        float rmsR = std::sqrt(rmsAccR / static_cast<float>(numSamples));

        outputPeakL.store(outputPeakDecayL, std::memory_order_relaxed);
        outputPeakR.store(outputPeakDecayR, std::memory_order_relaxed);
        outputRmsL.store(rmsL, std::memory_order_relaxed);
        outputRmsR.store(rmsR, std::memory_order_relaxed);
    }
}

// =============================================================================
// Editor
// =============================================================================

juce::AudioProcessorEditor* OGainAudioProcessor::createEditor()
{
    return new OGainAudioProcessorEditor(*this);
}

// =============================================================================
// State Persistence
// =============================================================================

void OGainAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OGainAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// =============================================================================
// Factory function
// =============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OGainAudioProcessor();
}
