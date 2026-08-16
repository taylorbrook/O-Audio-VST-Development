/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "PluginProcessor.h"

#include <cmath>

// The editor is WebView-bound from Stage 3; the Stage-2 render harness builds
// this translation unit with JUCE_WEB_BROWSER=0 and no editor sources
// (pattern_render_harness_breaks_on_webview_editor).
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

namespace
{
    // The single shared tempo-division list — deliberately triplet-free
    // (7 entries; NOT O-Bitrot's list). Indices: 0=1/16, 1=1/8, 2=1/4,
    // 3=1/2, 4=1 bar, 5=2 bars, 6=4 bars.
    juce::StringArray syncDivisionChoices()
    {
        return { "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars" };
    }

    // 10–8000 ms, skew 0.35 — shared by STOP_FREE_MS / START_FREE_MS /
    // ENV_FREE_MS (parameter-spec.md, BINDING). The 8000 ms ceiling is DERIVED
    // from kMaxGestureSeconds so the ring-sizing static_assert in the header
    // and this range can never drift apart silently
    // (pattern_test_fixture_mirrors_drift_silently).
    juce::NormalisableRange<float> freeMsRange()
    {
        return { 10.0f,
                 (float) (TapestopProcessor::kMaxGestureSeconds * 1000.0),
                 0.0f, 0.35f };
    }

    juce::NormalisableRange<float> percentRange()
    {
        return { 0.0f, 100.0f, 0.0f, 1.0f };
    }

    // Division table {1/16, 1/8, 1/4, 1/2, 1 bar, 2 bars, 4 bars} → beats.
    // ASSUMES 4/4 (suite precedent — recorded in stages/2-dsp/NOTES.md).
    // Indices match syncDivisionChoices() above.
    constexpr double kDivisionBeats[7] = { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0 };
} // namespace

TapestopProcessor::TapestopProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache raw parameter atomics once — read per block on the audio thread
    // (suite convention; research/ARCHITECTURE.md line 348).
    pEngage       = parameters.getRawParameterValue("ENGAGE");
    pMode         = parameters.getRawParameterValue("MODE");
    pSyncMode     = parameters.getRawParameterValue("SYNC_MODE");
    pStopSyncDiv  = parameters.getRawParameterValue("STOP_SYNC_DIV");
    pStopFreeMs   = parameters.getRawParameterValue("STOP_FREE_MS");
    pStopCurve    = parameters.getRawParameterValue("STOP_CURVE");
    pStartSyncDiv = parameters.getRawParameterValue("START_SYNC_DIV");
    pStartFreeMs  = parameters.getRawParameterValue("START_FREE_MS");
    pStartCurve   = parameters.getRawParameterValue("START_CURVE");
    pEnvSyncDiv   = parameters.getRawParameterValue("ENV_SYNC_DIV");
    pEnvFreeMs    = parameters.getRawParameterValue("ENV_FREE_MS");
    pToneTrack    = parameters.getRawParameterValue("TONE_TRACK");
    pMix          = parameters.getRawParameterValue("MIX");
    pOutputGain   = parameters.getRawParameterValue("OUTPUT_GAIN");

    // getRawParameterValue returns nullptr only for an id missing from the
    // layout — a typo, never a runtime condition. Fail loudly here, in debug,
    // on first instantiation (O-ReverseDelay v1.7.3 IN-06 posture).
    jassert(pEngage != nullptr && pMode != nullptr && pSyncMode != nullptr
            && pStopSyncDiv != nullptr && pStopFreeMs != nullptr && pStopCurve != nullptr
            && pStartSyncDiv != nullptr && pStartFreeMs != nullptr && pStartCurve != nullptr
            && pEnvSyncDiv != nullptr && pEnvFreeMs != nullptr
            && pToneTrack != nullptr && pMix != nullptr && pOutputGain != nullptr);
}

TapestopProcessor::~TapestopProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout TapestopProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ── Trigger & Mode ──────────────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "ENGAGE", 1 },
        "Engage",
        false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MODE", 1 },
        "Mode",
        juce::StringArray { "Stop", "Scratch" },
        0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "SYNC_MODE", 1 },
        "Sync Mode",
        juce::StringArray { "Sync", "Free" },
        0));

    // ── Stop / Start (Stop mode) ────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "STOP_SYNC_DIV", 1 },
        "Stop Time",
        syncDivisionChoices(),
        3)); // 1/2

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "STOP_FREE_MS", 1 },
        "Stop Time (Free)",
        freeMsRange(),
        500.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "STOP_CURVE", 1 },
        "Stop Curve",
        percentRange(),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "START_SYNC_DIV", 1 },
        "Start Time",
        syncDivisionChoices(),
        2)); // 1/4

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "START_FREE_MS", 1 },
        "Start Time (Free)",
        freeMsRange(),
        250.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "START_CURVE", 1 },
        "Start Curve",
        percentRange(),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── Scratch (Scratch mode) ──────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "ENV_SYNC_DIV", 1 },
        "Env Length",
        syncDivisionChoices(),
        4)); // 1 bar

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ENV_FREE_MS", 1 },
        "Env Length (Free)",
        freeMsRange(),
        1000.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // ── Output ──────────────────────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TONE_TRACK", 1 },
        "Tone Track",
        percentRange(),
        60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        percentRange(),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float> { -24.0f, 12.0f, 0.0f, 1.0f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    return layout;
}

void TapestopProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    currentFs = sampleRate;

    // The ONLY allocation on any audio-adjacent path (PERF-01). processBlock
    // itself allocates nothing.
    capture.prepare(sampleRate, kCaptureSeconds);

    // Transport → Bypassed; no cached sample counts survive a rate change
    // (durations are converted from ms/beats at gesture edges using currentFs).
    transport.prepare(sampleRate);
    voices[0] = VarispeedVoice{};
    voices[1] = VarispeedVoice{};

    mixSmoothed.reset(sampleRate, 0.02);
    mixSmoothed.setCurrentAndTargetValue(pMix->load() * 0.01f);
    gainSmoothed.reset(sampleRate, 0.02);
    gainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(pOutputGain->load()));

    // A session restored with ENGAGE held ON must re-trigger cleanly: leaving
    // the edge detector at `false` makes the next block header see the held-on
    // parameter as a fresh engage edge (research/ARCHITECTURE.md, Sample Rate
    // Handling).
    lastEngage = false;

    // Zero latency — no setLatencySamples call anywhere (Stage-0 constraint).
}

void TapestopProcessor::releaseResources()
{
}

bool TapestopProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Stereo or mono only; in == out; no side-chain, no multi-out
    // (research/ARCHITECTURE.md File I/O).
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != mainOut)
        return false;

    if (layouts.inputBuses.size() > 1 || layouts.outputBuses.size() > 1)
        return false;

    return true;
}

double TapestopProcessor::gestureDurationSamples(bool isStopGesture) const noexcept
{
    // SYNC_MODE choice index 0 = Sync, 1 = Free. Only the gesture-edge latch
    // reads this pair — mid-gesture flips are inert (latch contract).
    const bool sync = pSyncMode->load() < 0.5f;

    if (sync)
    {
        const auto* divParam = isStopGesture ? pStopSyncDiv : pStartSyncDiv;
        const int   div      = juce::jlimit(0, 6, (int) std::lround((double) divParam->load()));

        return juce::jmax(1.0, kDivisionBeats[div] * (60.0 / currentBpm) * currentFs);
    }

    const auto* msParam = isStopGesture ? pStopFreeMs : pStartFreeMs;
    return juce::jmax(1.0, (double) msParam->load() * 0.001 * currentFs);
}

void TapestopProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    const int numInputChannels  = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear(channel, 0, numSamples);

    // ── Block header: read atomics + host BPM, detect the ENGAGE edge, update
    // transport (research/ARCHITECTURE.md Processing Order step 1). Durations
    // and curves are LATCHED at the edge — mid-gesture parameter/tempo moves
    // are inert (latch contract; a live ramp never retargets).
    //
    // BPM: O-Polystutter fallback+clamp pattern — no playhead / offline host /
    // missing getBpm() → 120; clamp 20–999. Read per block; the ONLY consumer
    // is the gesture-edge conversion in gestureDurationSamples().
    currentBpm = 120.0;
    if (auto* playHead = getPlayHead())
        if (auto posInfo = playHead->getPosition())
            if (auto bpm = posInfo->getBpm())
                currentBpm = juce::jlimit(20.0, 999.0, *bpm);

    const bool engagedNow = pEngage->load() > 0.5f;

    if (engagedNow != lastEngage)
    {
        if (engagedNow)
        {
            const double curveP = std::exp2(2.0 * (double) pStopCurve->load() * 0.01);
            transport.engage(gestureDurationSamples(true), curveP, voices, capture);
        }
        else
        {
            const double curveP = std::exp2(2.0 * (double) pStartCurve->load() * 0.01);
            transport.release(gestureDurationSamples(false), curveP, voices, capture);
        }

        lastEngage = engagedNow;
    }

    mixSmoothed.setTargetValue(pMix->load() * 0.01f);
    gainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(pOutputGain->load()));

    // Capture is stereo-fixed; mono buses write/read channel 0 twice
    // (bounded by the BUFFER's channel count, never the layout's —
    // pattern_standalone_canonical_channelset_oob).
    const int  chans = juce::jmin(buffer.getNumChannels(), 2);
    const int  chR   = chans > 1 ? 1 : 0;
    auto*      dL    = buffer.getWritePointer(0);
    auto*      dR    = buffer.getWritePointer(chR);

    for (int n = 0; n < numSamples; ++n)
    {
        const float inL = dL[n];
        const float inR = dR[n];

        // WRITE capture first, then read — per-sample write-then-read makes
        // 512-vs-4096 bit-identity structural and legalises the d = 0 integer
        // live-read (pattern_grain_read_before_capture_write_blocksize). The
        // ring records in EVERY state, including Bypassed.
        capture.pushSample(inL, inR);

        // Smoothers tick every sample in every state (block-size invariance:
        // consumption count is a function of the absolute timeline only).
        const float m = mixSmoothed.getNextValue();
        const float g = gainSmoothed.getNextValue();

        float outL = inL;
        float outR = inR;

        if (! transport.isBypassed())
        {
            // Transport advances the carrier (and any fading voice) and
            // returns per-sample gains. On the crossfade-complete tick the
            // state flips to Bypassed and this sample renders dry — the
            // previous sample's blend was already within ~1e-6 of dry
            // (fadeIn ≈ 1, fadeOut ≈ 0), so the handoff is continuous and
            // every sample from here is bitwise dry (DSP-03).
            const auto t = transport.tick(voices, capture);

            if (! transport.isBypassed())
            {
                float wetL = voices[t.carrierIdx].read(capture, 0)   * t.carrierWetGain;
                float wetR = voices[t.carrierIdx].read(capture, chR) * t.carrierWetGain;

                if (t.xfActive)
                {
                    // Skip-splice crossfade: the old voice fades out at its
                    // latched rate while the carrier fades in. During resync
                    // the carrier is the integer live-rider, so the fade
                    // lands on bitwise-dry content (DSP-03).
                    wetL = voices[t.fadingIdx].read(capture, 0)   * t.fadeOutGain + wetL * t.fadeInGain;
                    wetR = voices[t.fadingIdx].read(capture, chR) * t.fadeOutGain + wetR * t.fadeInGain;
                }

                // MIX blend, then OUTPUT_GAIN last — ENGAGED CHAIN ONLY. The
                // trim rides the transport's engaged-trim blend: it releases
                // to trimAmount = 0 (gain EXACTLY 1.0) across the resync
                // fade, so a non-default trim cannot step at the Bypassed
                // handoff, and glides back in over 50 ms on engage.
                const float dry      = 1.0f - m;
                const float trimGain = 1.0f + (g - 1.0f) * t.trimAmount;

                outL = (inL * dry + wetL * m) * trimGain;
                outR = (inR * dry + wetR * m) * trimGain;
            }
        }
        // Bypassed: TRUE hard pass-through — out = in, NO arithmetic at all
        // (bitwise null regardless of MIX/OUTPUT_GAIN; Stage-0 decision #6).
        // The trim CANNOT ride the bypass path even nominally at 0 dB: the
        // APVTS normalized round-trip of the 0 dB default over −24..+12
        // returns ~1.2e-6 dB, so decibelsToGain gives ≈ 1.0000001f, not
        // exactly 1.0f — the multiply flips bits. This is also what makes
        // the Phase-2.2 post-resync null bitwise BY CONSTRUCTION.

        dL[n] = outL;
        dR[n] = outR;
    }
}

juce::AudioProcessorEditor* TapestopProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new TapestopEditor(*this);
#else
    return nullptr;
#endif
}

bool TapestopProcessor::hasEditor() const
{
#if JUCE_WEB_BROWSER
    return true;
#else
    return false;
#endif
}

const juce::String TapestopProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TapestopProcessor::acceptsMidi() const           { return false; }
bool TapestopProcessor::producesMidi() const          { return false; }
bool TapestopProcessor::isMidiEffect() const          { return false; }
double TapestopProcessor::getTailLengthSeconds() const { return 0.0; }

int TapestopProcessor::getNumPrograms()                                { return 1; }
int TapestopProcessor::getCurrentProgram()                             { return 0; }
void TapestopProcessor::setCurrentProgram(int index)                   { juce::ignoreUnused(index); }
const juce::String TapestopProcessor::getProgramName(int index)        { juce::ignoreUnused(index); return {}; }
void TapestopProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void TapestopProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Standard APVTS XML round-trip. Stage 2.3's scratchEnvelopeJson property
    // rides this same ValueTree — nothing extra needed now.
    auto state = parameters.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void TapestopProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapestopProcessor();
}
