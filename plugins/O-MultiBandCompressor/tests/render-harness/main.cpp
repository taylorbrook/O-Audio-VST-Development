/*
   This file is part of O-MultiBandCompressor, an Ouaricon Audio plugin.
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

    O-MultiBandCompressor — factory preset verification harness

    Loads every factory preset through the preset manager, renders a pink-ish
    noise bed through the processor, and reports the gain reduction each band
    actually reaches. Then does it all again in reverse order and requires the
    numbers to match.

    Two things it checks:

    1. That the thresholds engage. Threshold in this plugin is measured on the
       *band* signal, not the full-range input. A high band carries far less
       energy than a low band, so a preset whose thresholds do not step down per
       band would compress the lows hard and leave the top completely untouched.

    2. That a preset sounds the same regardless of what was loaded before it.
       The reverse pass caught the v1.6.0 sidechain-filter bug, where a band
       silently inherited the previous preset's filter-enabled state and gain
       reduction moved 3-4 dB on load order alone.

    Neither is visible to auval or pluginval: the values are in range and the
    plugin is perfectly happy, it just does the wrong thing.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdio>
#include <random>
#include <map>
#include <cmath>

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 512;
    constexpr int    kBlocks     = 240;        // ~2.5 s of audio

    // Pink-ish noise via the Voss-McCartney-style one-pole cascade. Real program
    // material rolls off with frequency; white noise would put equal energy in the
    // top band and flatter the high-band thresholds.
    struct PinkNoise
    {
        std::mt19937 rng { 20260722 };
        std::uniform_real_distribution<float> dist { -1.0f, 1.0f };
        float b0 = 0, b1 = 0, b2 = 0;

        float next()
        {
            const float white = dist(rng);
            b0 = 0.99765f * b0 + white * 0.0990460f;
            b1 = 0.96300f * b1 + white * 0.2965164f;
            b2 = 0.57000f * b2 + white * 1.0526913f;
            return (b0 + b1 + b2 + white * 0.1848f) * 0.25f;
        }
    };
}

namespace
{
    struct BandGr { float gr[4]; bool nonFinite; };

    // Renders pink noise at `rate` and measures the peak gain reduction each band
    // reaches. Split out of measurePreset() at v1.12.2 so the low-rate stability
    // pass can drive the same signal at a different sample rate.
    BandGr renderAndMeasure(OMultiBandCompressorAudioProcessor& processor, double rate)
    {
        BandGr out { { 0.0f, 0.0f, 0.0f, 0.0f }, false };

        processor.prepareToPlay(rate, kBlockSize);

        PinkNoise noiseL, noiseR;
        juce::AudioBuffer<float> buffer(2, kBlockSize);
        juce::MidiBuffer midi;

        for (int block = 0; block < kBlocks; ++block)
        {
            for (int s = 0; s < kBlockSize; ++s)
            {
                const float mid  = noiseL.next();
                const float diff = noiseR.next() * 0.35f;
                buffer.setSample(0, s, mid + diff);
                buffer.setSample(1, s, mid - diff);
            }

            processor.processBlock(buffer, midi);

            // Skip the first few blocks: filters and detectors are still settling.
            if (block < 12)
                continue;

            const float gr[4] = {
                processor.getLowBandGainReduction(),
                processor.getLoMidBandGainReduction(),
                processor.getHiMidBandGainReduction(),
                processor.getHighBandGainReduction()
            };

            for (int b = 0; b < 4; ++b)
                out.gr[b] = juce::jmin(out.gr[b], gr[b]);   // GR is negative dB

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int s = 0; s < kBlockSize; ++s)
                    if (! std::isfinite(buffer.getSample(ch, s)))
                        out.nonFinite = true;
        }

        return out;
    }

    // Loads a preset and measures it at the harness's standard rate.
    BandGr measurePreset(OMultiBandCompressorAudioProcessor& processor,
                         const juce::String& name,
                         bool& loadFailed)
    {
        loadFailed = ! processor.presetManager.loadPreset(name);

        if (loadFailed)
            return BandGr { { 0.0f, 0.0f, 0.0f, 0.0f }, false };

        return renderAndMeasure(processor, kSampleRate);
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    OMultiBandCompressorAudioProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    const auto presets = processor.presetManager.getPresetList();

    if (presets.isEmpty())
    {
        std::printf("FAIL: no presets found\n");
        return 1;
    }

    std::printf("%-22s %-12s %8s %8s %8s %8s   %s\n",
                "PRESET", "CATEGORY", "LOW", "LOMID", "HIMID", "HIGH", "VERDICT");
    std::printf("%s\n", juce::String::repeatedString("-", 92).toRawUTF8());

    int enginesFound = 0;
    int failures = 0;

    std::map<juce::String, BandGr> forward;

    for (const auto& name : presets)
    {
        bool loadFailed = false;
        const auto measured = measurePreset(processor, name, loadFailed);
        forward[name] = measured;

        const auto category = processor.getPresetCategory(name);

        if (loadFailed)
        {
            std::printf("%-22s %-12s  LOAD FAILED\n",
                        name.toRawUTF8(), category.toRawUTF8());
            ++failures;
            continue;
        }

        const float* peakGr = measured.gr;
        const bool nonFinite = measured.nonFinite;

        // A band counts as engaged at 0.1 dB of gain reduction or more.
        int engaged = 0;
        for (int b = 0; b < 4; ++b)
            if (peakGr[b] <= -0.1f)
                ++engaged;

        juce::String verdict;
        if (nonFinite)
        {
            verdict = "*** NON-FINITE OUTPUT ***";
            ++failures;
        }
        else if (category == OMultiBandCompressorAudioProcessor::kInertPresetCategory)
        {
            // v1.7.0: keyed off the category, not off the single name "Init Flat".
            // Every Init preset is 1:1 on all four bands by definition, so the whole
            // group is expected to measure zero — and a new one added later is covered
            // without this check having to learn its name. The flip side is that an
            // Init preset that accidentally ships a ratio above 1:1 now fails here
            // instead of passing as a normal working preset.
            verdict = (engaged == 0) ? "inert (correct)" : "*** should be inert ***";
            if (engaged != 0) ++failures;
        }
        else if (engaged == 0)
        {
            verdict = "*** NO BAND ENGAGES ***";
            ++failures;
        }
        else
        {
            verdict = juce::String(engaged) + "/4 bands engage";
            ++enginesFound;
        }

        std::printf("%-22s %-12s %7.2f %8.2f %8.2f %8.2f   %s\n",
                    name.toRawUTF8(),
                    category.toRawUTF8(),
                    peakGr[0], peakGr[1], peakGr[2], peakGr[3],
                    verdict.toRawUTF8());
    }

    // ---- Order-independence pass -------------------------------------------------
    //
    // Measure everything again in reverse. Each preset must produce identical numbers
    // regardless of which preset preceded it: any difference means DSP state survived
    // the preset switch. This is not hypothetical — it is exactly how the v1.6.0
    // sidechain-filter bug was found. `updateSidechainFilters` only set its enabled
    // flag inside the "frequency changed" branch, so a band asking for the same
    // frequency a previous preset had already cached silently kept that preset's
    // enabled/disabled state, and gain reduction shifted by 3-4 dB depending on load
    // order alone.
    std::printf("\nOrder-independence check (reverse pass)...\n");

    int mismatches = 0;

    for (int i = presets.size() - 1; i >= 0; --i)
    {
        const auto& name = presets[i];
        bool loadFailed = false;
        const auto again = measurePreset(processor, name, loadFailed);

        if (loadFailed)
            continue;

        for (int b = 0; b < 4; ++b)
        {
            // Tolerance well under the 3-4 dB the real bug produced, but loose enough
            // not to trip on float non-determinism.
            if (std::abs(again.gr[b] - forward[name].gr[b]) > 0.01f)
            {
                std::printf("  MISMATCH %-22s band %d: forward %.2f, reverse %.2f\n",
                            name.toRawUTF8(), b, forward[name].gr[b], again.gr[b]);
                ++mismatches;
            }
        }
    }

    if (mismatches == 0)
        std::printf("  OK - all %d presets identical in both orders\n", presets.size());
    else
        failures += mismatches;

    // ---- v1.12.2: detector low-pass stability below 40 kHz -----------------------
    //
    // "20 kHz is Off" in updateSidechainFilters is an ABSOLUTE ceiling; Nyquist is
    // not. At 32 kHz Nyquist is 16 kHz, so an SC LPF of 18 kHz handed makeLowPass a
    // corner above it and built a biquad whose pole pair sits at |z| = 1.32. That
    // band's detector diverges.
    //
    // WHAT THAT LOOKS LIKE HERE IS NOT WHAT IT LOOKS LIKE IN O-Comp. The divergence
    // never reaches a sample: EnvelopeDetector::processSample carries the v1.6.1
    // non-finite guard, which rewrites the runaway detector value to 0 before it is
    // measured. The output stays finite, no band slams shut, and nothing is NaN by
    // the time it is observable. The band simply stops seeing its own signal and
    // quietly gives up compressing. A "finite output" check and a "gain reduction
    // within sane bounds" check both go green on the broken build — this pass was
    // written with both and neither could tell the two builds apart.
    //
    // So discriminate against the CLAMP TARGET instead. 0.45 x 32 kHz is 14.4 kHz,
    // so with the clamp in place "18 kHz" and "14.4 kHz" are the same request and
    // must render identically. Without it, 18 kHz builds the divergent filter, the
    // guard zeroes that band's detector, and the gain reduction moves by several dB.
    std::printf("\nLow-rate detector stability (32 kHz)...\n");

    {
        // Probe with a preset that actually compresses, so "no band engages" cannot
        // be mistaken for "stable".
        juce::String probe;

        for (const auto& name : presets)
        {
            const auto& m = forward[name];

            if (processor.getPresetCategory(name)
                    != OMultiBandCompressorAudioProcessor::kInertPresetCategory
                && (m.gr[0] <= -0.1f || m.gr[1] <= -0.1f
                 || m.gr[2] <= -0.1f || m.gr[3] <= -0.1f))
            {
                probe = name;
                break;
            }
        }

        if (probe.isEmpty())
        {
            std::printf("  *** no compressing preset to probe with — pass is vacuous ***\n");
            ++failures;
        }
        else
        {
            // Set every band's SC LPF, in HZ. The normalised value must come from the
            // parameter's own range: SC_LPF carries a 0.3 skew, so a hand-computed
            // proportion probes a corner three-odd octaves from the one named here.
            // getParameters() on this processor returns the APVTS, not
            // AudioProcessor's parameter array — walk the array through the base.
            auto setAllScLpf = [&] (float hz)
            {
                int setCount = 0;

                for (auto* param :
                        static_cast<juce::AudioProcessor&>(processor).getParameters())
                    if (auto* asFloat = dynamic_cast<juce::AudioParameterFloat*>(param))
                        if (asFloat->paramID.endsWith("_SC_LPF"))
                        {
                            asFloat->setValueNotifyingHost(
                                asFloat->getNormalisableRange().convertTo0to1(hz));
                            ++setCount;
                        }

                return setCount;
            };

            auto renderAt = [&] (float scLpfHz, bool& loadFailed) -> BandGr
            {
                loadFailed = ! processor.presetManager.loadPreset(probe);

                if (loadFailed)
                    return BandGr { { 0.0f, 0.0f, 0.0f, 0.0f }, false };

                // AFTER the preset load: the preset names SC_LPF and would overwrite it.
                if (setAllScLpf(scLpfHz) != 4)
                {
                    std::printf("  *** expected 4 SC_LPF parameters ***\n");
                    ++failures;
                }

                return renderAndMeasure(processor, 32000.0);
            };

            bool failA = false, failB = false;
            const auto aboveNyquist = renderAt(18000.0f, failA);   // clamped to 14400
            const auto atClampTarget = renderAt(14400.0f, failB);  // 0.45 * 32000

            if (failA || failB)
            {
                std::printf("  *** could not load probe preset '%s' ***\n",
                            probe.toRawUTF8());
                ++failures;
            }
            else
            {
                std::printf("  preset '%s'\n", probe.toRawUTF8());
                std::printf("    SC LPF 18000 Hz (above Nyquist)  %7.2f %7.2f %7.2f %7.2f  finite=%s\n",
                            aboveNyquist.gr[0], aboveNyquist.gr[1],
                            aboveNyquist.gr[2], aboveNyquist.gr[3],
                            aboveNyquist.nonFinite ? "no" : "yes");
                std::printf("    SC LPF 14400 Hz (clamp target)   %7.2f %7.2f %7.2f %7.2f  finite=%s\n",
                            atClampTarget.gr[0], atClampTarget.gr[1],
                            atClampTarget.gr[2], atClampTarget.gr[3],
                            atClampTarget.nonFinite ? "no" : "yes");

                if (aboveNyquist.nonFinite || atClampTarget.nonFinite)
                {
                    std::printf("  *** NON-FINITE OUTPUT at 32 kHz ***\n");
                    ++failures;
                }

                // LIVENESS. If the reference render does not compress, the comparison
                // below is two zeros agreeing with each other.
                bool engaged = false;
                for (int b = 0; b < 4; ++b)
                    if (atClampTarget.gr[b] <= -0.1f)
                        engaged = true;

                if (! engaged)
                {
                    std::printf("  *** reference render pulls no gain reduction —"
                                " this pass is vacuous ***\n");
                    ++failures;
                }

                // Same tolerance as the order-independence pass: well under the several
                // dB the real bug moves a band, loose enough for float non-determinism.
                for (int b = 0; b < 4; ++b)
                    if (std::abs(aboveNyquist.gr[b] - atClampTarget.gr[b]) > 0.01f)
                    {
                        std::printf("  *** band %d: 18 kHz reads %.2f dB where the"
                                    " 14.4 kHz clamp target reads %.2f dB — the corner"
                                    " is not being clamped, so the detector biquad is"
                                    " above Nyquist and its poles are outside the unit"
                                    " circle ***\n",
                                    b, aboveNyquist.gr[b], atClampTarget.gr[b]);
                        ++failures;
                    }
            }
        }
    }

    std::printf("\n%d preset(s) active, %d failure(s)\n", enginesFound, failures);
    return failures == 0 ? 0 : 1;
}
