/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): the frozen 196-parameter layout in 25 parameter
    groups, the host bypass parameter, symmetric mono / stereo buses and the
    v1 state format. Contract: .planning/parameter-spec.md. IDs, order, ranges
    and skews are frozen.

    Stage 2, Phase 2.2 (static engine): audio runs through up to 24 bands
    (Source/dsp: Designer -> Band -> Router), eight types, 6 - 96 dB/oct, five
    placements, double state, zero latency. Float in, float out, and nothing
    between the two except the bands.

      - With no active band the buffer is not touched: bit-identical output,
        denormal samples included.
      - A lane (channel) is converted to double and written back only when an
        active band touches it in this block. A lane no band touches is never
        converted: under flush-to-zero the round trip would lose its denormal
        samples.
      - Parameters are read once per host block, on the audio thread, from the
        raw values cached at construction. No lock, no listener, no
        allocation in the audio path.

    Stage 2, Phase 2.3 (continuous modulation): frequency, gain, Q and gain
    scale moves ramp for 20 ms on a global control grid (PLAN-2.3 P7 - P9).

      - The tick clock: one sample counter, zeroed in prepareToPlay and
        advanced by the length of EVERY block handed to processBlock or
        processBlockBypassed, whatever that block then does (bypassed, not
        prepared, no running band). Control ticks fall where the counter is a
        multiple of K (16 / 32 / 64 samples by rate, ControlRamp.h). The grid
        therefore does not depend on the host's block boundaries.
      - A continuous change seen at a block start is handed to the band as a
        target; it takes effect at the next tick (at once when the block
        starts on one). The band ramps, designs once per tick while it moves
        and interpolates its section parameters per sample (Band.h). A settled
        band is bit-identical to a static design and does no design work.
      - A gain or gain-scale edit on a Low Cut, High Cut, Notch or Band Pass
        compares as identical: no ramp, no design.
      - Every path that clears a band's filter state also snaps its ramp.
      - A non-finite sample at the end of a processed chunk clears every
        band's state, so a non-finite input does not stick. The chunk's
        output is written as it is.

    Stage 2, Phase 2.4 (structural crossfades and bypass): every band is an
    eq::SlotBand (SlotBand.h: two eq::Band slots, a 20 ms raised-cosine fade,
    a latched request, the neutral sleep), and the global bypass is an
    eq::BypassFade on the same fade table (Fade.h). PLAN-2.4 P6 - P9.

      - A type, slope, placement, enable or band-bypass change is handed to
        the band as a request at the block start. The band designs its idle
        slot from the full target, starts it from rest and crossfades to it;
        a request that arrives during a fade waits for the fade's end. The
        processor no longer classifies a change: it compares the raw snapshot
        and, on a difference, passes the canonical input on.
      - A gain type that has sat at exactly 0 dB for 50 ms leaves the running
        list. With no running band the buffer is not touched.
      - One bypass path. Bypass is wanted when the `bypass` parameter is on
        or the block arrived through processBlockBypassed. A toggle
        crossfades to the dry input (or back) over the fade table's length,
        in double, as the last stage before the conversion to float; a
        toggle during the fade is latched and the fade never reverses. At
        the end of the fade to bypass every band goes idle (state at rest,
        fades ended) and from then on the buffer is not touched and no band
        parameter is read.
      - Un-bypass reads every band's parameters afresh and starts every band
        from rest, settled, with no band fade and no ramp; the bypass fade
        covers the start. An edit made while bypassed is therefore in effect,
        settled, from the first processed sample.
      - The first block after a prepare starts every band settled and sets
        the bypass state without a fade: nothing fades in. A session that
        opens bypassed opens idle.
      - Lanes. A lane is converted to double for a host block when a running
        band may touch it before the next block start: the placement of
        every slot that is not off and, where a request is latched, of the
        requested input (SlotBand::touchesLane). A lane that was converted
        is written back for the chunks it was converted in, also when its
        last band stopped inside the block.
      - With no running band and the output stage idle (2.5) a bypass toggle
        is a flag: no fade, no sample touched.

    Stage 2, Phase 2.5 (output stage and auto gain): eq::OutputStage
    (OutputStage.h: two four-pole smoothers, auto gain x output gain, one
    multiply) between the bands and the bypass mix, and eq::AutoGain
    (AutoGain.h: the curve's negative pink-weighted mean power gain on 512
    cell midpoints). PLAN-2.5 P5 - P7.

      - output_gain and auto_gain are read at every processed block start.
        The output target is handed to its smoother at the next control
        tick; auto_gain takes effect, and the estimate is refreshed, on
        refresh ticks (every autoGainRefreshTicks ticks of the clock). The
        estimate is a pure function of the bands' latest requests, so a
        skipped refresh is invisible and offline and real-time renders are
        bit-identical.
      - The stage is idle for a block when both smoothers are settled at
        exactly 1.0, the pending output target is 1.0, and auto gain has
        nothing to change (off and staying off, or on and staying on with
        no dirty band). Idle, it touches nothing: with no running band the
        buffer is still not touched, and only the lanes a band touches are
        converted. Not idle, every lane of the layout is converted and a
        bypass toggle fades.
      - Seeded: the first block after a prepare and both un-bypass restarts
        snap the stage to its targets (every band's estimate at once), so
        nothing fades in.
      - A max-hold peak per output channel, updated after every block on
        every path and read-and-reset by the editor (Stage 3).

    Stage 2, Phase 2.6 (analyzer taps, display model, listen): PLAN-2.6
    P10, P11.

      - Two spectrum taps (eq::SpectrumTap, SpectrumTap.h): the float mono
        sum of the input (before processAudio) and of the output (after it),
        pushed in 256-sample stack pieces into fixed rings, on every prepared
        path. A tap writes only while an analyzer consumer is registered
        (setAnalyzerConsumer) and analyzerMode has its bit (1 Pre, 2 Post);
        the mode is read once per block. The taps read the buffer only.
      - Band listen (eq::Listen, Listen.h): listenBand (-1 off; not saved,
        not a parameter) is read at every processed block start and handed
        to listen at the next control tick, like a band request. Listen
        sits after auto gain and before output gain (call C3): with listen
        engaged a segment runs lanes x auto gain, listen (the band's region
        of the dry input, solo-routed), lanes x output gain; with listen
        not engaged the 2.5 fused multiply runs, unchanged. Listen engages
        over a 30 ms input ramp plus the 20 ms fade (call C2) and is seeded
        with the bands at the first block and both un-bypass restarts.
      - The idle test of steps 5 and 6 is pathIdleForBlock(): the 2.5 stage
        test and listen idle (no target, no move, nothing pending).
      - preparedRate and preparedStereo are published at the end of
        prepareToPlay for the message-thread display model and analyzer
        (Source/analysis), which design and transform on their own thread.
      - With listen off and no consumer registered, the output is
        bit-identical to 0.5.0.

    Phase 3.1: the WebView editor (createEditor, behind JUCE_WEB_BROWSER)
    and the preset manager (user presets; factory presets in Stage 4).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "dsp/AutoGain.h"
#include "dsp/ControlRamp.h"
#include "dsp/Designer.h"
#include "dsp/Fade.h"
#include "dsp/Listen.h"
#include "dsp/OutputStage.h"
#include "dsp/Router.h"
#include "dsp/SlotBand.h"
#include "dsp/SpectrumTap.h"

// vu-meter 1.0.0 compile proof (Stage 1 plan, P6): the header is included so
// that its first build inside a plugin happens now. No member is declared;
// the meter itself is mounted in Stage 3.
#include "VUMeterBridge.h"

// preset-manager 1.1.1 (Phase 3.1, call C2): the processor owns the manager.
#include "OuariconPresetManager.h"

class OuariconParametricEQAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconParametricEQAudioProcessor();
    ~OuariconParametricEQAudioProcessor() override = default;

    static constexpr int kNumBands = 24;

    //==========================================================================
    /** Sets the designer's rate, the layout flag, the working lanes, the
        control grid (K and the ramp length, from the rate, into every band),
        the fade table and the sleep length; zeroes the tick clock, prepares
        every band (both slots at rest and off, no fade) and invalidates
        every snapshot. The next block is then the first block: every band
        starts settled on its current values and the bypass state is set
        without a fade. Reports latency 0. Safe to call repeatedly and at a
        different rate: nothing is carried across it. Phase 2.6: prepares
        listen (idle), empties both taps (generation bumped), then publishes
        preparedStereo and preparedRate. */
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    /** Mono -> mono and stereo -> stereo only. The base class accepts every
        layout a wrapper enumerates, asymmetric ones included. */
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    using juce::AudioProcessor::processBlock;
    using juce::AudioProcessor::processBlockBypassed;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    /** The same path as processBlock, with bypass wanted whatever the
        `bypass` parameter says: the first such block after processed ones
        starts the 20 ms fade to the dry input, the blocks after the fade
        leave the buffer untouched with the engine idle, and the first
        processBlock call after them fades back in on the current
        parameters. The tick clock advances by the block's length. JUCE's
        own VST3 and AU wrappers never call this while a bypass parameter
        exists; a host built on the graph classes and the render harness do. */
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    /** The `bypass` parameter. */
    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    /** The bands ring. 2 s is the figure the architecture fixes; it does not
        follow the settings. */
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    /** Test accessor (render-harness row V1; DSP-07, engine half): the dynamic
        gain offset of band `band` (0 ... 23) in dB. 0 in v1; an index out of
        range reads 0. The only engine value the processor exposes. */
    double getBandDynOffsetDb (int band) const noexcept;

    /** The largest |sample| of output channel `ch` (0, or 1 on a stereo
        layout) since the last call, and resets it to 0. Any thread.

        The audio thread updates it after every block on every path
        (processed, bypassed, untouched, not prepared) with a load and a
        store: one writer, so a read that falls between the two keeps that
        peak for one more read. A NaN sample never stores; an inf stores inf
        (the meter shows the overflow). An index out of range reads 0. */
    float readAndResetOutputPeak (int ch) noexcept;

    /** Test accessors (render-harness row XR-blk5): the estimator's band
        evaluations and refreshes since the last prepare. Audio-thread
        values, read by the harness between blocks only. */
    unsigned long getAutoGainEvaluationCount() const noexcept   { return autoGain.getEvaluationCount(); }
    unsigned long getAutoGainRefreshCount() const noexcept      { return autoGain.getRefreshCount(); }

    //==========================================================================
    // Phase 2.6: analyzer taps, display model, listen (PLAN-2.6 P10, P11)
    //==========================================================================

    /** The analyzer's owner registers (true) and unregisters (false) here:
        the editor in Stage 3, the render harness in Stage 2. While false the
        audio thread writes neither tap. Any thread. */
    void setAnalyzerConsumer (bool registered) noexcept  { analyzerConsumer.store (registered, std::memory_order_release); }
    bool hasAnalyzerConsumer() const noexcept           { return analyzerConsumer.load (std::memory_order_acquire); }

    /** The two rings (pre: the input, post: the output). The audio thread is
        the only producer; one reader drains each (SpectrumAnalyzer). Fixed
        storage, members from construction. */
    eq::SpectrumTap& getPreTap() noexcept               { return preTap; }
    eq::SpectrumTap& getPostTap() noexcept              { return postTap; }
    const eq::SpectrumTap& getPreTap() const noexcept   { return preTap; }
    const eq::SpectrumTap& getPostTap() const noexcept  { return postTap; }

    /** The rate and layout of the last prepareToPlay, published at its end
        (release, after the taps' generation bump). 0 / true before the
        first prepare. Any thread. */
    double getPreparedRate() const noexcept             { return preparedRate.load (std::memory_order_acquire); }
    bool getPreparedStereo() const noexcept             { return preparedStereo.load (std::memory_order_acquire); }

    /** Test accessor (render-harness rows XR-g9id and the G9 rows): the
        design band `band` runs on, from its current slot (Band::
        getRealisedDesign: numSections and params[] are realised values).
        False, with `out` untouched, for an index out of range and when the
        current slot is off or the band asleep. Audio-thread state: read by
        the harness between blocks only. */
    bool getBandRealisedDesign (int band, eq::BandDesign& out) const noexcept;

    /** Test accessor: listen's target, move and positions (eq::ListenState).
        Audio-thread state: read by the harness between blocks only. */
    eq::ListenState getListenState() const noexcept     { return listen.getState(); }

    //==========================================================================
    /** 196 parameters. State tree type "OParametricEQ". */
    juce::AudioProcessorValueTreeState parameters;

    /** User presets (Phase 3.1, call C2): ~/Library/O-ParametricEQ/Presets.
        output_gain and bypass are excluded (presets never set them); each
        applied value is validated and is its own host gesture. Empty factory
        set until Stage 4; directories are created lazily, so construction
        does no file I/O. Session state does NOT go through it: the codec in
        getStateInformation / setStateInformation is its own. Declared after
        `parameters`, which it references. */
    OuariconPresetManager presetManager { parameters, "O-ParametricEQ" };

    //==========================================================================
    // Session UI state. NOT host parameters, not in APVTS, not part of presets.
    // Atomics because a host may restore state off the message thread.
    //
    // uiLanguage is persisted as a ROOT attribute of the state (a language
    // code); the other five live in the state's `UI` child.

    /** 0 = en, 1 = fr, 2 = zh-Hans. */
    std::atomic<int> uiLanguage { 0 };

    /** 0 Off, 1 Pre, 2 Post, 3 Pre+Post: the taps' bit mask (PLAN-2.6 P11). */
    std::atomic<int> analyzerMode { 2 };

    /** The listened band, 0 ... 23; -1 (or any value out of range) is off.
        Transient UI state: not saved, not a parameter. Read at every
        processed block start, handed to listen at the next control tick. */
    std::atomic<int> listenBand { -1 };

    /** Plus / minus dB shown by the response display: 6, 12 or 30. */
    std::atomic<int> uiDisplayRange { 12 };

    /** 0 = none, 1 ... 24. */
    std::atomic<int> uiSelectedBand { 0 };

    /** Editor size, 800 x 500 ... 1600 x 1000. */
    std::atomic<int> uiEditorWidth { 960 };
    std::atomic<int> uiEditorHeight { 600 };

    /** The language codec. Pure ASCII by contract: what is persisted is a
        language code. Anything that is neither "fr" nor "zh-Hans" maps to 0,
        so a hand-edited session degrades to English and is never stored raw. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

private:
    /** The display model reads the raw parameter values through
        readBandSnapshot and readGainScaleRaw (PLAN-2.6 P8: friend, so the
        raw-value cache stays private and there is one raw read path). */
    friend class DisplayModel;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // Raw parameter values, in registration order:
    //   [0 ... 3]            bypass, output_gain, gain_scale, auto_gain
    //   [4 + 8 b + f]        band b (0 ... 23), field f in eq::RawBand order
    static constexpr int kNumGlobalParams = 4;
    static constexpr int kNumBandParams   = eq::numRawBandValues;
    static constexpr int kNumParams       = kNumGlobalParams + kNumBands * kNumBandParams;

    static_assert (kNumParams == 196, "the layout is frozen at 196 parameters");

    /** What the change detection compares by bit pattern: the band's eight
        raw values and the raw gain_scale. Nine floats, no padding. */
    struct BandSnapshot
    {
        eq::RawBand raw;
        float gainScale;
    };

    static_assert (sizeof (BandSnapshot) == 9 * sizeof (float),
                   "BandSnapshot is compared by bit pattern: nine floats, no padding");

    /** Per band: the last raw snapshot and whether it can be trusted. An
        invalid snapshot sends the band's canonical input to the band again
        (the band compares it with what it realises, so a repeat costs
        nothing). The realised input and the latest request live in the
        band (SlotBand). Snapshots are invalid after a prepare and stale
        after every fully bypassed block. */
    struct BandSlot
    {
        BandSnapshot snapshot {};
        bool valid = false;
    };

    /** Band indices, in index order. */
    using RunningList = std::array<int, static_cast<std::size_t> (kNumBands)>;

    /** The one audio path (PLAN-2.4 P6, P8). processBlock passes false,
        processBlockBypassed true. */
    void processAudio (juce::AudioBuffer<float>&, bool viaBypassed);

    /** The Stage 1 line: clears any output channel that has no input. */
    void clearOutputsWithoutInput (juce::AudioBuffer<float>&);

    /** Advances the tick clock by the length of a block and returns the
        clock value at the block's first sample. Called first thing by
        processAudio, so no return path can skip it. */
    std::uint64_t advanceTickClock (int numSamples) noexcept;

    /** Reads the raw values of band `band` and the raw gain_scale. */
    BandSnapshot readBandSnapshot (int band, float gainScaleRaw) const noexcept;

    /** The raw gain_scale value. */
    float readGainScaleRaw() const noexcept;

    /** The canonical input of a snapshot, with the placement that takes
        effect on the negotiated layout and a dynamic offset of 0: the band
        reads its own offset. eq::effectiveBandInput, the one definition the
        display model also calls. */
    eq::BandInput makeInput (const BandSnapshot&) const noexcept;

    /** Writes the indices of the bands that are running (a fade in flight,
        or a current slot that is not off) to the front of `list`, in index
        order, and returns their number. */
    int collectRunningBands (RunningList& list) const noexcept;

    /** Every band's latest request, in index order (the estimator's input). */
    using DesiredList = std::array<const eq::BandInput*, static_cast<std::size_t> (kNumBands)>;
    DesiredList collectDesired() const noexcept;

    /** PLAN-2.5 P6: the output stage has nothing to do in this block and
        nothing can change that before the next block start. Computed after
        the block-start parameter reads, never latched. Steps 5 and 6. */
    bool stageIdleForBlock() const noexcept;

    /** PLAN-2.6 P10: the whole path after the bands has nothing to do in
        this block: stageIdleForBlock() and listen idle (no target, no move,
        no pending request). Computed after step 4, never latched. Steps 5
        and 6, nowhere else. */
    bool pathIdleForBlock() const noexcept;

    /** Seeding (first block, both un-bypass restarts): auto gain takes its
        wanted state and the stage snaps to its targets, every band's
        estimate evaluated at once. */
    void seedOutputStage() noexcept;

    /** Seeding of listen, after the bands and the output stage: settled on
        the wish read at this block's start. */
    void seedListen() noexcept;

    /** After every block, on every path. */
    void updateOutputPeak (const juce::AudioBuffer<float>&) noexcept;

    /** PLAN-2.6 P11: the tap bit mask for this block (bit 0 Pre, bit 1
        Post): 0 without a consumer, when not prepared, or when the buffer
        is narrower than the layout. Reads analyzerMode once. */
    int tapMaskForBlock (const juce::AudioBuffer<float>&) const noexcept;

    /** Pushes the float mono sum of the buffer (0.5f * (L + R), or the
        single channel on a mono layout) into `tap`, in 256-sample stack
        pieces. Read-only on the buffer. */
    void pushTap (eq::SpectrumTap& tap, const juce::AudioBuffer<float>&) const noexcept;

    //==========================================================================
    juce::AudioProcessorParameter* bypassParam = nullptr;

    /** Cached once in the constructor; read on the audio thread. */
    std::array<std::atomic<float>*, static_cast<std::size_t> (kNumParams)> rawValues {};

    /** The audio thread's designer. prepare() runs in prepareToPlay, before
        any design at that rate. */
    eq::Designer designer;

    std::array<eq::SlotBand, static_cast<std::size_t> (kNumBands)> bands;
    std::array<BandSlot, static_cast<std::size_t> (kNumBands)> slots;

    /** The one fade table of this engine, built in prepareToPlay from the
        rate: read by every band's crossfade and by the bypass fade. */
    eq::FadeTable fadeTable;

    /** The global soft bypass. Audio thread only. */
    eq::BypassFade bypassFade;

    /** Consecutive control ticks at exactly 0 dB before a gain type falls
        asleep: 50 ms on the grid of this rate, from prepareToPlay. */
    int sleepTicks = 1;

    /** False from prepareToPlay until the first block that gets past the
        guards: that block starts every band settled and sets the bypass
        state without a fade. Audio thread only (prepareToPlay does not run
        beside processing). */
    bool started = false;

    /** Double working lanes (0 = L or mono, 1 = R), sized in prepareToPlay to
        the prepared block size. A longer host block runs in chunks. */
    std::array<std::vector<double>, 2> lanes;

    /** From the negotiated output set in prepareToPlay; never from the
        buffer's width. */
    bool stereoLayout = false;

    /** 0 until prepareToPlay has run. */
    int preparedBlockSize = 0;

    /** The tick clock (PLAN-2.3 P7): samples handed to processBlock or
        processBlockBypassed since prepareToPlay, bypassed blocks included.
        Audio thread only. */
    std::uint64_t tickClock = 0;

    /** K: samples between two control ticks, from the rate in prepareToPlay.
        A tick falls on every sample whose clock value is a multiple of it. */
    int controlGrid = 16;

    /** Auto gain x output gain, between the bands and the bypass mix. */
    eq::OutputStage outputStage;

    /** The estimator: about 220 KB of fixed storage, a member of this
        heap-allocated object. */
    eq::AutoGain autoGain;

    /** auto_gain as read at the last block start, and as in effect: the
        second follows the first at the next refresh tick (or at seeding). */
    bool agWanted = false;
    bool agOn = false;

    /** Ticks between two auto-gain refreshes, from the rate. */
    int refreshTicks = 1;

    /** Max-hold output peaks (readAndResetOutputPeak). */
    std::array<std::atomic<float>, 2> outputPeak { { 0.0f, 0.0f } };

    /** The spectrum taps: 2 x 128 KB of fixed storage, members of this
        heap-allocated object from construction (never sized in prepare). */
    eq::SpectrumTap preTap;
    eq::SpectrumTap postTap;

    /** Set by the analyzer's owner; the audio thread's write gate. */
    std::atomic<bool> analyzerConsumer { false };

    /** Band listen: two voices and the 30 ms ramp table, about 90 KB of
        fixed storage. Audio thread only (prepare in prepareToPlay). */
    eq::Listen listen;

    /** Published at the end of prepareToPlay; 0 / true before. */
    std::atomic<double> preparedRate { 0.0 };
    std::atomic<bool> preparedStereo { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OuariconParametricEQAudioProcessor)
};
