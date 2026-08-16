/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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
/*
  ==============================================================================

    O-Bitrot - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <cmath>

// NOTE: PluginEditor.h is deliberately NOT included here — the editor include
// lives inside the #if JUCE_WEB_BROWSER guard above createEditor() so the
// Stage-2 render harness (JUCE_WEB_BROWSER=0, no editor sources) can compile
// this TU (pattern_render_harness_breaks_on_webview_editor).

juce::AudioProcessorValueTreeState::ParameterLayout OBitrotAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========================================================================
    // GLOBAL (6)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CLOCK_MODE", 1 },
        "Clock Mode",
        juce::StringArray { "Sync", "Free" },
        0  // Default: Sync
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CLOCK_SYNC_DIV", 1 },
        "Clock Division",
        juce::StringArray { "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 bar" },
        2  // Default: 1/8
    ));

    {
        auto range = juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f);
        range.setSkewForCentre(1.414f);  // sqrt(0.1 * 20) — exponential feel
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "CLOCK_FREE_RATE", 1 },
            "Clock Rate",
            range,
            2.0f,
            "Hz"
        ));
    }

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "SEED", 1 },
        "Seed",
        0, 9999,
        0
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "HARD_EDGES", 1 },
        "Hard Edges",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    // ========================================================================
    // TAPE (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "TAPE_ENABLE", 1 },
        "Tape Enable",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_PROB", 1 },
        "Tape Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_STOP_PROB", 1 },
        "Tape-Stop Share",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        10.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_RAMP", 1 },
        "Tape Ramp",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.1f),
        150.0f,
        "ms"
    ));

    // ========================================================================
    // CD SKIP (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "CD_ENABLE", 1 },
        "CD Enable",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CD_PROB", 1 },
        "CD Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CD_SEVERITY", 1 },
        "CD Severity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CD_SEGMENT", 1 },
        "CD Segment",
        juce::NormalisableRange<float>(10.0f, 400.0f, 0.1f),
        100.0f,
        "ms"
    ));

    // ========================================================================
    // VINYL (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "VINYL_ENABLE", 1 },
        "Vinyl Enable",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VINYL_PROB", 1 },
        "Vinyl Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "VINYL_RPM", 1 },
        "Vinyl RPM",
        juce::StringArray { "33 1/3", "45" },  // ASCII-only; UI renders 33 1/3 glyph in Stage 3
        0  // Default: 33 1/3
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VINYL_POP", 1 },
        "Vinyl Pop",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"
    ));

    // ========================================================================
    // PACKET LOSS (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "PACKET_ENABLE", 1 },
        "Packet Enable",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "PACKET_LOSS", 1 },
        "Packet Loss",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        20.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "PACKET_BURST", 1 },
        "Packet Burstiness",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        30.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "PACKET_CONCEAL", 1 },
        "Concealment",
        juce::StringArray { "Silence", "Repeat", "Decay", "Substitute" },
        2  // Default: Decay
    ));

    // ========================================================================
    // CODEC (3)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "CODEC_ENABLE", 1 },
        "Codec Enable",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CODEC_MODE", 1 },
        "Codec Mode",
        juce::StringArray { "Mu-law", "GSM" },  // ASCII-only; UI renders the mu glyph in Stage 3
        0  // Default: Mu-law
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CODEC_MIX", 1 },
        "Codec Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    // ========================================================================
    // CRUSH (6)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "CRUSH_ENABLE", 1 },
        "Crush Enable",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_BITS", 1 },
        "Crush Bits",
        juce::NormalisableRange<float>(1.0f, 16.0f, 0.01f),
        16.0f,
        "bits"
    ));

    {
        auto range = juce::NormalisableRange<float>(500.0f, 20000.0f, 1.0f);
        range.setSkewForCentre(3162.0f);  // sqrt(500 * 20000) — exponential feel
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "CRUSH_RATE", 1 },
            "Crush Rate",
            range,
            20000.0f,
            "Hz"
        ));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_JITTER", 1 },
        "Crush Jitter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_ENV_AMT", 1 },
        "Crush Env Amount",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_DITHER", 1 },
        "Crush Dither",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        0.0f,
        "LSB"
    ));

    return layout;
}

OBitrotAudioProcessor::OBitrotAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache raw parameter pointers (atomic, real-time safe).
    // Global
    clockModeParam     = apvts.getRawParameterValue("CLOCK_MODE");
    clockSyncDivParam  = apvts.getRawParameterValue("CLOCK_SYNC_DIV");
    clockFreeRateParam = apvts.getRawParameterValue("CLOCK_FREE_RATE");
    seedParam          = apvts.getRawParameterValue("SEED");
    hardEdgesParam     = apvts.getRawParameterValue("HARD_EDGES");
    mixParam           = apvts.getRawParameterValue("MIX");

    // Tape
    tapeEnableParam   = apvts.getRawParameterValue("TAPE_ENABLE");
    tapeProbParam     = apvts.getRawParameterValue("TAPE_PROB");
    tapeStopProbParam = apvts.getRawParameterValue("TAPE_STOP_PROB");
    tapeRampParam     = apvts.getRawParameterValue("TAPE_RAMP");

    // CD Skip
    cdEnableParam   = apvts.getRawParameterValue("CD_ENABLE");
    cdProbParam     = apvts.getRawParameterValue("CD_PROB");
    cdSeverityParam = apvts.getRawParameterValue("CD_SEVERITY");
    cdSegmentParam  = apvts.getRawParameterValue("CD_SEGMENT");

    // Vinyl
    vinylEnableParam = apvts.getRawParameterValue("VINYL_ENABLE");
    vinylProbParam   = apvts.getRawParameterValue("VINYL_PROB");
    vinylRpmParam    = apvts.getRawParameterValue("VINYL_RPM");
    vinylPopParam    = apvts.getRawParameterValue("VINYL_POP");

    // Packet Loss
    packetEnableParam  = apvts.getRawParameterValue("PACKET_ENABLE");
    packetLossParam    = apvts.getRawParameterValue("PACKET_LOSS");
    packetBurstParam   = apvts.getRawParameterValue("PACKET_BURST");
    packetConcealParam = apvts.getRawParameterValue("PACKET_CONCEAL");

    // Codec
    codecEnableParam = apvts.getRawParameterValue("CODEC_ENABLE");
    codecModeParam   = apvts.getRawParameterValue("CODEC_MODE");
    codecMixParam    = apvts.getRawParameterValue("CODEC_MIX");

    // Crush
    crushEnableParam = apvts.getRawParameterValue("CRUSH_ENABLE");
    crushBitsParam   = apvts.getRawParameterValue("CRUSH_BITS");
    crushRateParam   = apvts.getRawParameterValue("CRUSH_RATE");
    crushJitterParam = apvts.getRawParameterValue("CRUSH_JITTER");
    crushEnvAmtParam = apvts.getRawParameterValue("CRUSH_ENV_AMT");
    crushDitherParam = apvts.getRawParameterValue("CRUSH_DITHER");
}

OBitrotAudioProcessor::~OBitrotAudioProcessor()
{
}

void OBitrotAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Constant latency, all modes: kCompLatency = ceil(0.020 * fs) — an exact
    // integer at every standard rate. CodecStage presents exactly this delay
    // in every state; reported ONCE here, never renegotiated on CODEC_MODE.
    compLatencySamples = (int) std::ceil(0.020 * sampleRate);
    jassert(compLatencySamples <= kMaxWetLatencySamples);
    setLatencySamples(compLatencySamples);

    captureRing.prepare(sampleRate);
    readHead.prepare(sampleRate, captureRing.getSize());
    mediaClock.prepare(sampleRate, samplesPerBlock);
    tapeTransport.prepare(sampleRate);
    cdSkip.prepare(sampleRate);
    vinylTransport.prepare(sampleRate);
    artifactSynth.prepare(sampleRate);
    packetStage.prepare(sampleRate, packetEnableParam->load() > 0.5f);
    codecStage.prepare(sampleRate, compLatencySamples,
                       codecEnableParam->load() > 0.5f,
                       ((int) codecModeParam->load()) == 1,
                       codecMixParam->load() * 0.01f);
    crushStage.prepare(sampleRate, crushEnableParam->load() > 0.5f,
                       (double) crushRateParam->load());
    quantStage.prepare(sampleRate, crushEnableParam->load() > 0.5f,
                       (double) crushBitsParam->load());

    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) juce::jmax(1, samplesPerBlock),
                                  2u };
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::linear);
    dryWetMixer.setWetLatency((float) compLatencySamples);
    dryWetMixer.setWetMixProportion(juce::jlimit(0.0f, 1.0f, mixParam->load() * 0.01f));

    lastSeed = (int) seedParam->load();
    rngBank.reseed(lastSeed);
    lastAppliedRate = 1.0;
}

void OBitrotAudioProcessor::releaseResources()
{
    // Ring / delay buffers stay allocated (< 5 MB at 192 kHz); prepareToPlay
    // re-sizes them on the next start. GSM handles are freed here (guarded;
    // prepareToPlay recreates them, and processBlock skips codec work on
    // null handles).
    codecStage.releaseHandles();
}

bool OBitrotAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Stereo-in / stereo-out only
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void OBitrotAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples             = buffer.getNumSamples();

    // Clear any output channels that don't have corresponding input data.
    // Bound by buffer.getNumChannels() (Standalone canonical-channelset trap).
    for (int channel = totalNumInputChannels;
         channel < totalNumOutputChannels && channel < buffer.getNumChannels();
         ++channel)
    {
        buffer.clear(channel, 0, numSamples);
    }

    // Defensive: layout is locked stereo/stereo, and the engine needs
    // prepareToPlay to have run (ring sized, latency reported).
    if (numSamples == 0 || buffer.getNumChannels() < 2 || captureRing.getSize() == 0)
        return;

    // Scrub non-finite INPUT at the boundary (QUAL-01). Downstream state that
    // is fed signal — the capture ring, packet history, and especially the
    // DryWetMixer's Thiran dry-delay (whose allpass state v computes
    // `alpha * (x - v)` and holds NaN FOREVER once poisoned, even at
    // alpha == 0, since 0 * NaN == NaN) — cannot recover from a NaN era on
    // its own. Finite samples are never written, so the all-off path stays
    // bit-exact (FUNC-02).
    for (int channel = 0; channel < 2; ++channel)
    {
        auto* d = buffer.getWritePointer(channel);
        for (int n = 0; n < numSamples; ++n)
            if (! std::isfinite(d[n]))
                d[n] = 0.0f;
    }

    // ── Block-start bookkeeping (never inside the sample loop) ──────────────

    // Seed-change detection: reseed all streams deterministically at the
    // block boundary (FUNC-04; dice writes SEED from the message thread).
    const int seedNow = (int) seedParam->load();
    if (seedNow != lastSeed)
    {
        lastSeed = seedNow;
        rngBank.reseed(seedNow);
    }

    const bool   hardEdges = hardEdgesParam->load() > 0.5f;
    const int    clockMode = (int) clockModeParam->load();      // 0 Sync, 1 Free
    const int    divIndex  = (int) clockSyncDivParam->load();
    const double freeRate  = (double) clockFreeRateParam->load();

    Arbitration::Params arbParams;
    arbParams.tapeEnabled   = tapeEnableParam->load() > 0.5f;
    arbParams.cdEnabled     = cdEnableParam->load() > 0.5f;
    arbParams.vinylEnabled  = vinylEnableParam->load() > 0.5f;
    arbParams.tapeProb      = (double) tapeProbParam->load() * 0.01;
    arbParams.cdProb        = (double) cdProbParam->load() * 0.01;
    arbParams.vinylProb     = (double) vinylProbParam->load() * 0.01;
    arbParams.tapeStopShare = (double) tapeStopProbParam->load() * 0.01;
    arbParams.tapeRampMs    = (double) tapeRampParam->load();
    arbParams.cdSeverity    = (double) cdSeverityParam->load();
    arbParams.cdSegmentMs   = (double) cdSegmentParam->load();
    arbParams.vinylRpmIndex = (int) vinylRpmParam->load();
    arbParams.vinylPop01    = vinylPopParam->load() * 0.01f;

    // Packet stage per-block snapshot. The grid + GE chain run
    // unconditionally (documented determinism convention in
    // PacketLossStage.h); PACKET_ENABLE only gates audibility via a ~10 ms
    // fade — off is bit-transparent.
    packetStage.setParams(packetLossParam->load() * 0.01f,
                          packetBurstParam->load() * 0.01f,
                          (int) packetConcealParam->load(),
                          packetEnableParam->load() > 0.5f,
                          hardEdges);

    // Crush + Quant per-block snapshot (one section enable gates both).
    // Determinism convention as PacketLossStage: latch/follower + jitter and
    // dither draws run unconditionally; CRUSH_ENABLE gates audibility.
    const bool crushEnabled = crushEnableParam->load() > 0.5f;
    crushStage.setParams(crushRateParam->load(),
                         crushJitterParam->load() * 0.01f,
                         crushEnabled);
    quantStage.setParams(crushBitsParam->load(),
                         crushDitherParam->load(),
                         crushEnvAmtParam->load() * 0.01f,
                         crushEnabled);

    // Codec per-block snapshot. No RNG in the codec path; the 8 kHz latch
    // phase and alignment rings run unconditionally (pure functions of the
    // sample count); CODEC_ENABLE rides the EnableFade rails.
    codecStage.setParams(codecEnableParam->load() > 0.5f,
                         ((int) codecModeParam->load()) == 1,
                         codecMixParam->load() * 0.01f);

    // Mid-event disable releases gracefully (ramp back / recovery jump /
    // stop re-jumping), never teleports.
    if (! arbParams.tapeEnabled)
        tapeTransport.release(arbParams.tapeRampMs);
    if (! arbParams.cdEnabled)
        cdSkip.release(readHead, captureRing, hardEdges);
    if (! arbParams.vinylEnabled)
        vinylTransport.release();

    // ── Dry path (before any mutation) ──────────────────────────────────────
    dryWetMixer.setWetMixProportion(juce::jlimit(0.0f, 1.0f, mixParam->load() * 0.01f));
    juce::dsp::AudioBlock<float> block(buffer);
    dryWetMixer.pushDrySamples(block);

    // ── Clock: sample-accurate tick offsets for this block ──────────────────
    mediaClock.processBlock(getPlayHead(), numSamples, clockMode, divIndex, freeRate);

    // ── Wet path: per-sample write-then-read (blockSize-invariance trap) ────
    const float* inL  = buffer.getReadPointer(0);
    const float* inR  = buffer.getReadPointer(1);
    float*       outL = buffer.getWritePointer(0);
    float*       outR = buffer.getWritePointer(1);

    int tickIndex = 0;

    for (int n = 0; n < numSamples; ++n)
    {
        // 1. Write the ring FIRST — the NORMAL read head sits at lag 0.
        captureRing.push(inL[n], inR[n]);

        // 2. Ticks land at exact sample offsets (split-block equivalent).
        //    RNG is consumed ONLY here, in fixed roll order tape->cd->vinyl.
        while (tickIndex < mediaClock.getNumTicks()
               && mediaClock.getTickOffset(tickIndex) == n)
        {
            Arbitration::TickContext ctx { tapeTransport, cdSkip, vinylTransport,
                                           readHead, captureRing, artifactSynth,
                                           lastAppliedRate, hardEdges };
            arbitration.onTick(rngBank, arbParams, ctx);
            ++tickIndex;
        }

        // 3. Rate for this sample: tape state machine while a tape event is
        //    in flight; EXACTLY 1.0 while a CD loop or locked groove runs
        //    (exact repeat intervals; pitch never changes); gentle
        //    re-approach trim (<= +2%, ramped) only when fully NORMAL.
        const double lag = readHead.getLag(captureRing.getTotalWritten());
        double rate;
        if (! tapeTransport.isIdle())
        {
            readHead.clearTrim();               // NORMAL trim restarts from 0
            rate = tapeTransport.nextRate(lag);
        }
        else if (cdSkip.isLooping() || vinylTransport.isLocked())
        {
            readHead.clearTrim();
            rate = 1.0;
        }
        else
        {
            rate = readHead.reapproachRate(lag);
        }
        lastAppliedRate = rate;

        // 4. Read heads render the transport output.
        float wetL = 0.0f, wetR = 0.0f;
        readHead.renderSample(captureRing, rate, hardEdges, wetL, wetR);

        // 5. CD ladder (conceal dip / mute / loop wrap) then vinyl locked-
        //    groove wrap — both operate on the head/rendered signal.
        cdSkip.processSample(readHead, captureRing, hardEdges, artifactSynth, wetL, wetR);
        vinylTransport.processSample(readHead, captureRing, hardEdges, artifactSynth, rngBank);

        // 6. Artifact bus (pops / ticks / chirps): mono, both channels, runs
        //    every sample so the IIR state stays continuous. Exact 0.0f when
        //    nothing was ever triggered (FUNC-02 preserved).
        const float artifact = artifactSynth.renderSample();
        wetL += artifact;
        wetR += artifact;

        // 7. Packet loss (own 20 ms grid, GE Markov, concealment). RNG
        //    (packet stream) consumed only at packet boundaries.
        packetStage.processSample(rngBank, wetL, wetR);

        // 8. CodecStage: phone chain (mono -> BP -> 8 kHz latch -> mu-law |
        //    GSM -> post-LPF -> equal-power blend), presenting exactly
        //    kCompLatency delay in every state — bit-transparent alignment
        //    delay when disabled. ARCHITECTURE chain order:
        //    Packet -> Codec -> Crush -> Quant.
        codecStage.processSample(wetL, wetR);

        // 9. Crush (fractional-hold SRR + jitter) then Quant (fractional
        //    bits + TPDF + env-driven depth) — the "output converter"
        //    position. Jitter/dither streams draw unconditionally per sample.
        crushStage.processSample(rngBank, wetL, wetR);
        quantStage.processSample(rngBank, wetL, wetR);

        outL[n] = wetL;
        outR[n] = wetR;
    }

    // ── Dry/wet blend (dry is delayed inside the mixer by setWetLatency) ────
    dryWetMixer.mixWetSamples(block);
}

// The editor include lives INSIDE the guard: the Stage-2 render harness
// compiles this file with JUCE_WEB_BROWSER=0 and no editor sources, so a
// top-of-file include would break the harness the moment the editor gains
// WebView types (pattern_render_harness_breaks_on_webview_editor).
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OBitrotAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OBitrotAudioProcessorEditor(*this);
#else
    return new juce::GenericAudioProcessorEditor(*this);   // harness build
#endif
}

void OBitrotAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void OBitrotAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBitrotAudioProcessor();
}
