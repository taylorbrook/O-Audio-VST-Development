/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    O-simpleAmbisonics render-harness — Stage-1 scaffold.

    Probes (Stage 1: pass-through shell with the FINAL 64-sample latency):

      P0 contract       — all 12 APVTS parameters exist with the ARCHITECTURE.md
                          types/defaults; order 3 choices, decoder 2, inputMode 2;
                          distance / orbitRate have non-unity skew; after
                          prepareToPlay getLatencySamples() == kPartition == 64.
      P1 pass-through   — Mono sum, 0 dB, position-deterministic noise in:
                          out[c][n] == 0.5f*(L[n-64]+R[n-64]) BIT-EQUAL on both
                          channels; out[0..63] == 0; out[64] exact (no fade-in).
      P1b Left only +   — inputMode = 1 → out == L[n-64]; then a mono input bus
          mono-in         (setPlayConfigDetails(1, 2, …)) → out == in0[n-64].
      P2 block sizes    — ragged {1, 7, 64, 333, 4096} ≡ flat 512, bit-equal.
      P3 oversized      — prepareToPlay(fs, 64) then ONE 4096-sample block ≡
                          prepareToPlay(fs, 4096) render, bit-equal (slice loop).
      P4 latency        — unit impulse at index 100 → first nonzero output
                          index == 100 + getLatencySamples().
      P5 silence floor  — outputLevel = −60 set BEFORE prepareToPlay → every
                          output sample exactly 0 (seed is 0, target is 0).
      digest            — FNV-1a 64 over the P1 flat render, printed as the
                          Stage-1 baseline (recorded in SUMMARY.md).

    Phase 2.1 keeps P0/P2/P3/P4/P5, replaces P1's expectation with the engine
    render, and adds the allocation counter (PERF-01) and DSP probes.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "dsp/SpatialEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

namespace
{
    constexpr double kFs            = 48000.0;
    constexpr int    kDefaultBlock  = 512;
    constexpr int    kRenderSamples = 8192;
    constexpr int    kLatency       = OSimpleAmbisonics::dsp::kPartition;

    int failures = 0;

    void check (const char* name, bool ok, const juce::String& detail = {})
    {
        std::printf ("  [%s] %s%s%s\n", ok ? "PASS" : "FAIL", name,
                     detail.isEmpty() ? "" : " — ",
                     detail.isEmpty() ? "" : detail.toRawUTF8());
        if (! ok)
            ++failures;
    }

    using Proc = OSimpleAmbisonicsAudioProcessor;

    // Set a parameter by its plain (denormalised) value, through the host path.
    void setParam (Proc& p, const char* id, float plainValue)
    {
        auto* param = p.apvts.getParameter (id);
        jassert (param != nullptr);
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            ranged->setValueNotifyingHost (ranged->convertTo0to1 (plainValue));
    }

    // Fresh instance: optional pre-prepare parameter setup, then
    // setPlayConfigDetails + prepareToPlay.
    std::unique_ptr<Proc> makeProc (int numIn, int maxBlock,
                                    const std::function<void (Proc&)>& preSetup = {})
    {
        auto p = std::make_unique<Proc>();
        if (preSetup)
            preSetup (*p);
        p->setPlayConfigDetails (numIn, 2, kFs, maxBlock);
        p->prepareToPlay (kFs, maxBlock);
        return p;
    }

    // Position-deterministic noise: the sample at absolute index n on channel
    // ch is a pure function of (ch, n), independent of block slicing.
    float noiseAt (int ch, int n)
    {
        uint32_t h = (uint32_t) n * 2654435761u ^ (uint32_t) (ch + 1) * 40503u;
        h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
        return (float) ((double) h / 2147483648.0 - 1.0);   // [-1, 1)
    }

    // Render kRenderSamples through `p` with the given block slicing, feeding
    // `fill(ch, n)` on `numIn` input legs; output interleaved L,R per sample.
    // The buffer is always 2 wide (max(in, out)), as the host passes it.
    template <typename Fill>
    void renderWith (Proc& p, int numIn, const std::vector<int>& blockSizes,
                     Fill fill, std::vector<float>& out)
    {
        juce::MidiBuffer midi;
        const int maxLen = *std::max_element (blockSizes.begin(), blockSizes.end());
        juce::AudioBuffer<float> buf (2, maxLen);

        int pos = 0, sizeIdx = 0;
        while (pos < kRenderSamples)
        {
            const int want = blockSizes[(size_t) sizeIdx % blockSizes.size()];
            const int len  = juce::jmin (want, kRenderSamples - pos);
            ++sizeIdx;

            buf.setSize (2, len, false, false, true);
            buf.clear();
            for (int ch = 0; ch < numIn; ++ch)
            {
                auto* w = buf.getWritePointer (ch);
                for (int i = 0; i < len; ++i)
                    w[i] = fill (ch, pos + i);
            }

            p.processBlock (buf, midi);

            for (int i = 0; i < len; ++i)
                for (int ch = 0; ch < 2; ++ch)
                    out.push_back (buf.getSample (ch, i));

            pos += len;
        }
    }

    bool bitEqual (float a, float b)
    {
        return std::memcmp (&a, &b, sizeof (float)) == 0;
    }

    bool buffersBitEqual (const std::vector<float>& a, const std::vector<float>& b)
    {
        return a.size() == b.size()
            && std::memcmp (a.data(), b.data(), a.size() * sizeof (float)) == 0;
    }

    // Compare an interleaved stereo render against expected(n) on both channels,
    // with the 64-sample latency offset; also gates out[0..63] == 0 and out[64]
    // exact. Returns the first mismatching sample index or -1.
    template <typename Expected>
    int firstMismatch (const std::vector<float>& out, Expected expected)
    {
        for (int n = 0; n < kRenderSamples; ++n)
        {
            const float e = n < kLatency ? 0.0f : expected (n - kLatency);
            for (int ch = 0; ch < 2; ++ch)
                if (! bitEqual (out[(size_t) n * 2 + (size_t) ch], e))
                    return n;
        }
        return -1;
    }

    uint64_t fnv1a64 (const std::vector<float>& v)
    {
        uint64_t h = 1469598103934665603ull;
        const auto* bytes = reinterpret_cast<const unsigned char*> (v.data());
        for (size_t i = 0; i < v.size() * sizeof (float); ++i)
        {
            h ^= bytes[i];
            h *= 1099511628211ull;
        }
        return h;
    }
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    using namespace OSimpleAmbisonics;

    std::printf ("O-simpleAmbisonics render-harness (Stage 1 scaffold) — fs=%.0f, latency=%d\n",
                 kFs, kLatency);

    //==========================================================================
    // P0 — parameter contract (ARCHITECTURE.md §Parameter Mapping).
    {
        auto p = makeProc (2, kDefaultBlock);

        struct FloatSpec { const char* id; float lo, hi, def; bool skewed; };
        const FloatSpec floats[] = {
            { ParamIDs::azimuth,     -180.0f, 180.0f, 0.0f, false },
            { ParamIDs::elevation,    -90.0f,  90.0f, 0.0f, false },
            { ParamIDs::distance,       0.2f,  20.0f, 1.0f, true  },
            { ParamIDs::yaw,         -180.0f, 180.0f, 0.0f, false },
            { ParamIDs::pitch,       -180.0f, 180.0f, 0.0f, false },
            { ParamIDs::roll,        -180.0f, 180.0f, 0.0f, false },
            { ParamIDs::orbitRate,     0.02f,   2.0f, 0.1f, true  },
            { ParamIDs::outputLevel,  -60.0f,   0.0f, 0.0f, false },
        };

        for (const auto& f : floats)
        {
            auto* fp = dynamic_cast<juce::AudioParameterFloat*> (p->apvts.getParameter (f.id));
            check ((juce::String ("P0 ") + f.id + " exists+float").toRawUTF8(), fp != nullptr);
            if (fp == nullptr)
                continue;

            check ((juce::String ("P0 ") + f.id + " range").toRawUTF8(),
                   juce::approximatelyEqual (fp->range.start, f.lo)
                       && juce::approximatelyEqual (fp->range.end, f.hi),
                   juce::String (fp->range.start) + ".." + juce::String (fp->range.end));
            check ((juce::String ("P0 ") + f.id + " default").toRawUTF8(),
                   juce::approximatelyEqual (fp->get(), f.def), juce::String (fp->get()));
            check ((juce::String ("P0 ") + f.id + (f.skewed ? " skewed" : " linear")).toRawUTF8(),
                   f.skewed ? ! juce::approximatelyEqual (fp->range.skew, 1.0f)
                            :   juce::approximatelyEqual (fp->range.skew, 1.0f),
                   juce::String (fp->range.skew));
        }

        struct ChoiceSpec { const char* id; int count; const char* first; };
        const ChoiceSpec choices[] = {
            { ParamIDs::order,     3, "1" },
            { ParamIDs::decoder,   2, "Binaural" },
            { ParamIDs::inputMode, 2, "Mono sum" },
        };

        for (const auto& c : choices)
        {
            auto* cp = dynamic_cast<juce::AudioParameterChoice*> (p->apvts.getParameter (c.id));
            check ((juce::String ("P0 ") + c.id + " exists+choice").toRawUTF8(), cp != nullptr);
            if (cp == nullptr)
                continue;
            check ((juce::String ("P0 ") + c.id + " choices").toRawUTF8(),
                   cp->choices.size() == c.count, juce::String (cp->choices.size()));
            check ((juce::String ("P0 ") + c.id + " default").toRawUTF8(),
                   cp->getIndex() == 0 && cp->choices[0] == c.first, cp->getCurrentChoiceName());
        }

        auto* orbit = dynamic_cast<juce::AudioParameterBool*> (p->apvts.getParameter (ParamIDs::orbit));
        check ("P0 orbit exists+bool", orbit != nullptr);
        if (orbit != nullptr)
            check ("P0 orbit default off", orbit->get() == false);

        int count = 0;
        for (auto* param : p->getParameters())
            if (dynamic_cast<juce::RangedAudioParameter*> (param) != nullptr)
                ++count;
        check ("P0 exactly 12 parameters", count == 12, juce::String (count));

        check ("P0 latency == kPartition == 64",
               p->getLatencySamples() == kLatency && kLatency == 64,
               juce::String (p->getLatencySamples()));
    }

    //==========================================================================
    // P1 — pass-through: Mono sum, 0 dB. out[n] == 0.5*(L[n-64]+R[n-64]).
    std::vector<float> flat;
    flat.reserve ((size_t) kRenderSamples * 2);
    {
        auto p = makeProc (2, kDefaultBlock);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, flat);

        const auto expected = [] (int n) { return 0.5f * (noiseAt (0, n) + noiseAt (1, n)); };
        const int mm = firstMismatch (flat, expected);
        check ("P1 pass-through bit-equal (both ch, 64 delayed)", mm < 0, juce::String (mm));

        bool headZero = true;
        for (int n = 0; n < kLatency && headZero; ++n)
            headZero = flat[(size_t) n * 2] == 0.0f && flat[(size_t) n * 2 + 1] == 0.0f;
        check ("P1 out[0..63] == 0", headZero);

        check ("P1 out[64] exact (no fade-in)",
               bitEqual (flat[(size_t) kLatency * 2], expected (0))
                   && bitEqual (flat[(size_t) kLatency * 2 + 1], expected (0)),
               juce::String (flat[(size_t) kLatency * 2]) + " vs " + juce::String (expected (0)));
    }

    //==========================================================================
    // P1b — Left only (stereo in), then a mono input bus.
    {
        auto p = makeProc (2, kDefaultBlock,
                           [] (Proc& q) { setParam (q, ParamIDs::inputMode, 1.0f); });
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, out);
        const int mm = firstMismatch (out, [] (int n) { return noiseAt (0, n); });
        check ("P1b Left only == L[n-64] on both ch", mm < 0, juce::String (mm));
    }
    {
        auto p = makeProc (1, kDefaultBlock);   // mono input bus, stereo out
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 1, { kDefaultBlock }, noiseAt, out);
        const int mm = firstMismatch (out, [] (int n) { return noiseAt (0, n); });
        check ("P1b mono-in == in0[n-64] on both ch", mm < 0, juce::String (mm));
    }

    //==========================================================================
    // P2 — ragged block sizes bit-identical to the flat 512 feed.
    {
        auto p = makeProc (2, 4096);
        std::vector<float> ragged;
        ragged.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { 1, 7, 64, 333, 4096, 13, 512, 2 }, noiseAt, ragged);
        check ("P2 ragged == flat", buffersBitEqual (ragged, flat));
    }

    //==========================================================================
    // P3 — oversized host block: prepared for 64, fed 4096-sample blocks.
    {
        auto small = makeProc (2, 64);
        std::vector<float> outSmall;
        outSmall.reserve ((size_t) kRenderSamples * 2);
        renderWith (*small, 2, { 4096 }, noiseAt, outSmall);

        auto big = makeProc (2, 4096);
        std::vector<float> outBig;
        outBig.reserve ((size_t) kRenderSamples * 2);
        renderWith (*big, 2, { 4096 }, noiseAt, outBig);

        check ("P3 oversized block (prep 64, feed 4096) == prep 4096",
               buffersBitEqual (outSmall, outBig));
        check ("P3 oversized == flat", buffersBitEqual (outBig, flat));
    }

    //==========================================================================
    // P4 — latency measured: impulse at 100 → first nonzero at 100 + latency.
    {
        auto p = makeProc (2, kDefaultBlock);
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock },
                    [] (int, int n) { return n == 100 ? 1.0f : 0.0f; }, out);

        int firstNonZero = -1;
        for (int n = 0; n < kRenderSamples && firstNonZero < 0; ++n)
            if (out[(size_t) n * 2] != 0.0f || out[(size_t) n * 2 + 1] != 0.0f)
                firstNonZero = n;

        const int reported = p->getLatencySamples();
        check ("P4 measured latency == reported", firstNonZero == 100 + reported,
               juce::String ("first nonzero ") + juce::String (firstNonZero)
                   + ", reported " + juce::String (reported));
        check ("P4 impulse amplitude exact", firstNonZero >= 0
                   && bitEqual (out[(size_t) firstNonZero * 2], 1.0f));
    }

    //==========================================================================
    // P5 — silence floor: outputLevel = −60 before prepare → all zeros.
    {
        auto p = makeProc (2, kDefaultBlock,
                           [] (Proc& q) { setParam (q, ParamIDs::outputLevel, -60.0f); });
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, out);

        bool allZero = true;
        for (float v : out)
            if (v != 0.0f) { allZero = false; break; }
        check ("P5 -60 dB == exact silence", allZero);
    }

    //==========================================================================
    // Digest — Stage-1 pass-through baseline.
    std::printf ("  digest: fnv1a64=%016llx (%d samples x 2ch, flat %d, mono sum, 0 dB)\n",
                 (unsigned long long) fnv1a64 (flat), kRenderSamples, kDefaultBlock);

    std::printf ("%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
