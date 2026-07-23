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

    // Loads a preset and measures the peak gain reduction each band reaches.
    BandGr measurePreset(OMultiBandCompressorAudioProcessor& processor,
                         const juce::String& name,
                         bool& loadFailed)
    {
        BandGr out { { 0.0f, 0.0f, 0.0f, 0.0f }, false };

        loadFailed = ! processor.presetManager.loadPreset(name);
        if (loadFailed)
            return out;

        processor.prepareToPlay(kSampleRate, kBlockSize);

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

    std::printf("%-22s %8s %8s %8s %8s   %s\n",
                "PRESET", "LOW", "LOMID", "HIMID", "HIGH", "VERDICT");
    std::printf("%s\n", juce::String::repeatedString("-", 78).toRawUTF8());

    int enginesFound = 0;
    int failures = 0;

    std::map<juce::String, BandGr> forward;

    for (const auto& name : presets)
    {
        bool loadFailed = false;
        const auto measured = measurePreset(processor, name, loadFailed);
        forward[name] = measured;

        if (loadFailed)
        {
            std::printf("%-22s  LOAD FAILED\n", name.toRawUTF8());
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
        else if (name == "Init Flat")
        {
            // The one preset that is meant to do nothing.
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

        std::printf("%-22s %7.2f %8.2f %8.2f %8.2f   %s\n",
                    name.toRawUTF8(),
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

    std::printf("\n%d preset(s) active, %d failure(s)\n", enginesFound, failures);
    return failures == 0 ? 0 : 1;
}
