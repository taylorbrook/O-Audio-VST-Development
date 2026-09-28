/*
   This file is part of O-SimpleReverb, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/
/*
    O-SimpleReverb render check (v1.11.0)

    Offline console gate for the v1.11.0 full-review fixes. Build with
    -DOUARICON_BUILD_TESTS=ON, target O-SimpleReverb-render-check.

      1. Every factory preset recalls the TYPE its name says — in the DSP
         (roundToInt of the raw value), in the host (getIndex) and in the page
         (JS Math.round(norm * 5)).
      2. CHARACTER moves do not click: toggling it every 8 blocks keeps the
         wet output's max |second difference| near the held value (v1.10.0:
         332x..2154x).
      3. Ambient at 192 kHz renders finite, non-silent audio.
      4. The VU peak is HELD across blocks until read, then clears.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <random>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

static int failures = 0;

static void check(bool ok, const juce::String& what)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
    if (! ok) ++failures;
}

static std::unique_ptr<OSimpleReverbAudioProcessor> makeProcessor()
{
    std::unique_ptr<juce::AudioProcessor> base(createPluginFilter());
    return std::unique_ptr<OSimpleReverbAudioProcessor>(dynamic_cast<OSimpleReverbAudioProcessor*>(base.release()));
}

static void setParam(OSimpleReverbAudioProcessor& proc, const char* id, float value)
{
    auto* p = proc.parameters.getParameter(id);
    p->setValueNotifyingHost(p->convertTo0to1(value));
}

// Renders `blocks` blocks of a 300 Hz sine at 0.5, calling perBlock(b) first.
template <typename PerBlock>
static std::vector<float> renderSine(OSimpleReverbAudioProcessor& proc, double sr, int blockSize,
                                     int blocks, PerBlock perBlock)
{
    proc.setPlayConfigDetails(2, 2, sr, blockSize);
    proc.prepareToPlay(sr, blockSize);
    juce::AudioBuffer<float> buf(2, blockSize);
    juce::MidiBuffer midi;
    std::vector<float> out;
    double phase = 0.0;
    for (int b = 0; b < blocks; ++b) {
        perBlock(b);
        for (int i = 0; i < blockSize; ++i) {
            const float x = 0.5f * (float) std::sin(phase);
            phase += 2.0 * juce::MathConstants<double>::pi * 300.0 / sr;
            buf.setSample(0, i, x);
            buf.setSample(1, i, x);
        }
        proc.processBlock(buf, midi);
        for (int i = 0; i < blockSize; ++i)
            out.push_back(buf.getSample(0, i));
    }
    return out;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    // ── 1. Factory presets recall their TYPE ─────────────────────────────────
    {
        auto proc = makeProcessor();
        if (proc == nullptr) { std::printf("FAIL: not an OSimpleReverbAudioProcessor\n"); return 1; }

        const juce::StringArray typeNames { "Booth", "Room", "Hall", "Spring", "Plate", "Ambient" };
        auto* typeParam = dynamic_cast<juce::AudioParameterChoice*>(proc->parameters.getParameter("TYPE"));
        auto* raw = proc->parameters.getRawParameterValue("TYPE");

        int factoryCount = 0, wrong = 0;
        for (const auto& name : proc->presetManager.getPresetList()) {
            if (! proc->presetManager.isFactoryPreset(name))
                continue;
            const int expected = typeNames.indexOf(name.upToFirstOccurrenceOf(" - ", false, false));
            if (expected < 0)
                continue;
            ++factoryCount;
            proc->presetManager.loadPreset(name);
            const int dsp  = juce::roundToInt(raw->load());
            const int host = typeParam->getIndex();
            const int page = (int) std::floor(static_cast<juce::AudioProcessorParameter*>(typeParam)->getValue() * 5.0f + 0.5f);  // JS Math.round
            if (dsp != expected || host != expected || page != expected) {
                ++wrong;
                std::printf("  %s: expected %d, dsp %d, host %d, page %d\n",
                            name.toRawUTF8(), expected, dsp, host, page);
            }
        }
        check(factoryCount == 24, "24 factory presets found (" + juce::String(factoryCount) + ")");
        check(wrong == 0, "every factory preset recalls its named TYPE in DSP, host and page ("
                          + juce::String(wrong) + " wrong)");
    }

    // ── 2. CHARACTER moves are click-free ────────────────────────────────────
    // Click metric: max |second difference| of the wet output (a step or a
    // filter-state blow-up spikes it; a 300 Hz sine barely registers). Toggling
    // CHARACTER every 8 blocks is compared against holding it. v1.10.0 measured
    // 2154x (-80<->-10), 762x (-80<->0) and 332x (10<->60).
    {
        auto maxD2 = [](float a, float b) {
            auto proc = makeProcessor();
            setParam(*proc, "TYPE", 1.0f); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
            setParam(*proc, "CHARACTER", a);
            const auto out = renderSine(*proc, 48000.0, 256, 600, [&](int blk) {
                setParam(*proc, "CHARACTER", (blk / 8) % 2 ? b : a);
            });
            float m = 0.0f;
            for (size_t i = 100 * 256; i < out.size(); ++i)
                m = std::max(m, std::abs(out[i] - 2.0f * out[i - 1] + out[i - 2]));
            return m;
        };
        const float held = juce::jmax(maxD2(-80.0f, -80.0f), maxD2(0.0f, 0.0f), maxD2(60.0f, 60.0f));
        struct Case { float a, b, limit; };
        for (auto c : { Case { -80.0f, -10.0f, 1.5f }, Case { -80.0f, 0.0f, 1.5f },
                        Case { -30.0f, 30.0f, 10.0f }, Case { 10.0f, 60.0f, 10.0f } }) {
            const float ratio = maxD2(c.a, c.b) / held;
            check(ratio < c.limit, "CHARACTER " + juce::String(c.a, 0) + "<->" + juce::String(c.b, 0)
                                   + " click ratio " + juce::String(ratio, 2) + " < " + juce::String(c.limit, 1));
        }
    }

    // ── 3. Ambient at 192 kHz ────────────────────────────────────────────────
    {
        auto proc = makeProcessor();
        setParam(*proc, "TYPE", 5.0f); setParam(*proc, "WET", 100.0f); setParam(*proc, "DRY", 0.0f);
        setParam(*proc, "DECAY", 2.0f); setParam(*proc, "SIZE", 100.0f);
        const auto out = renderSine(*proc, 192000.0, 512, 750, [](int) {});
        bool finite = true; float peak = 0.0f;
        for (float x : out) { finite = finite && std::isfinite(x); peak = std::max(peak, std::abs(x)); }
        check(finite && peak > 0.01f, "Ambient @ 192 kHz renders finite, non-silent (peak "
                                      + juce::String(peak, 4) + ")");
    }

    // ── 4. VU peak is held until read ────────────────────────────────────────
    {
        auto proc = makeProcessor();
        setParam(*proc, "WET", 0.0f); setParam(*proc, "DRY", 100.0f);
        proc->outputPeak.exchange(0.0f);
        renderSine(*proc, 48000.0, 256, 20, [](int) {});                   // loud blocks...
        juce::AudioBuffer<float> silence(2, 256); silence.clear();
        juce::MidiBuffer midi;
        for (int i = 0; i < 400; ++i) { silence.clear(); proc->processBlock(silence, midi); }  // ...then 2 s of silence
        const float held = proc->outputPeak.exchange(0.0f);
        const float after = proc->outputPeak.load();
        check(held > 0.45f, "peak from earlier loud blocks survives later silent blocks (" + juce::String(held, 3) + ")");
        check(after == 0.0f, "reading the peak clears it");
    }

    std::printf("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
