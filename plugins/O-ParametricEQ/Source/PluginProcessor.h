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

    What this phase does in place of later ones:

      2.3 (ramps, control grid)   a parameter move redesigns the band at the
                                  next block start, state kept: an audible step
      2.4 (crossfades, sleep,     a type / slope / placement / activation
           soft bypass)           change clears the band's state: a click.
                                  A 0 dB band is an ordinary active band.
                                  Global bypass is a hard switch.
      2.5 (output stage)          output_gain and auto_gain are read by nothing
      2.6                         no taps, no display model, no listen

    No WebView editor (Stage 3), no presets (Stage 4).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <vector>

#include "dsp/Designer.h"
#include "dsp/Router.h"

// vu-meter 1.0.0 compile proof (Stage 1 plan, P6): the header is included so
// that its first build inside a plugin happens now. No member is declared;
// the meter itself is mounted in Stage 3.
#include "VUMeterBridge.h"

class OuariconParametricEQAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconParametricEQAudioProcessor();
    ~OuariconParametricEQAudioProcessor() override = default;

    static constexpr int kNumBands = 24;

    //==========================================================================
    /** Sets the designer's rate, the layout flag and the working lanes, clears
        every band and invalidates every snapshot, so the first block designs
        every active band. Reports latency 0. Safe to call repeatedly and at a
        different rate: nothing is carried across it. */
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    /** Mono -> mono and stereo -> stereo only. The base class accepts every
        layout a wrapper enumerates, asymmetric ones included. */
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    using juce::AudioProcessor::processBlock;
    using juce::AudioProcessor::processBlockBypassed;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    /** JUCE's own VST3 and AU wrappers never call this while a bypass
        parameter exists; hosts that do get the hard bypass: the buffer is
        left untouched and every band's state is cleared. */
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

    //==========================================================================
    /** 196 parameters. State tree type "OParametricEQ". */
    juce::AudioProcessorValueTreeState parameters;

    //==========================================================================
    // Session UI state. NOT host parameters, not in APVTS, not part of presets.
    // Atomics because a host may restore state off the message thread.
    //
    // uiLanguage is persisted as a ROOT attribute of the state (a language
    // code); the other five live in the state's `UI` child.

    /** 0 = en, 1 = fr, 2 = zh-Hans. */
    std::atomic<int> uiLanguage { 0 };

    /** 0 Off, 1 Pre, 2 Post, 3 Pre+Post. */
    std::atomic<int> analyzerMode { 2 };

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

    /** Per band: the last raw snapshot, the canonical input built from it
        (with the EFFECTIVE placement), and whether the snapshot can be
        trusted. An invalid snapshot forces the band through makeBandInput and,
        when active, through the designer. */
    struct BandSlot
    {
        BandSnapshot snapshot {};
        eq::BandInput input {};
        bool valid = false;
    };

    /** The Stage 1 line: clears any output channel that has no input. */
    void clearOutputsWithoutInput (juce::AudioBuffer<float>&);

    /** Zeroes the filter state of all 24 bands. Designs and snapshots stay. */
    void resetAllBands() noexcept;

    /** Reads the raw values of band `band` and the raw gain_scale. */
    BandSnapshot readBandSnapshot (int band, float gainScaleRaw) const noexcept;

    //==========================================================================
    juce::AudioProcessorParameter* bypassParam = nullptr;

    /** Cached once in the constructor; read on the audio thread. */
    std::array<std::atomic<float>*, static_cast<std::size_t> (kNumParams)> rawValues {};

    /** The audio thread's designer. prepare() runs in prepareToPlay, before
        any design at that rate. */
    eq::Designer designer;

    std::array<eq::Band, static_cast<std::size_t> (kNumBands)> bands;
    std::array<BandSlot, static_cast<std::size_t> (kNumBands)> slots;

    /** Double working lanes (0 = L or mono, 1 = R), sized in prepareToPlay to
        the prepared block size. A longer host block runs in chunks. */
    std::array<std::vector<double>, 2> lanes;

    /** From the negotiated output set in prepareToPlay; never from the
        buffer's width. */
    bool stereoLayout = false;

    /** 0 until prepareToPlay has run. */
    int preparedBlockSize = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OuariconParametricEQAudioProcessor)
};
