/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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

    O-Emulator render-harness — Stage-1 scaffold.

    Probes (Stage 1: passthrough shell):

      P0 contract      — all 5 APVTS parameters exist with the BINDING
                         parameter-spec.md types and defaults; console has
                         exactly 5 choices.
      P1 passthrough   — position-deterministic noise in, output bit-equals
                         input over every block (latency 0 in Stage 1, so no
                         compensation offset). NO tolerance.
      P2 blocksizes    — ragged block sizes {1, 7, 64, 333, 4096} render
                         bit-identically to a flat 512-sample feed (trivially
                         true for a passthrough; the probe exists so Phase 2.1
                         inherits a running invariance gate, not a new one).
      digest           — FNV-1a 64 over the canonical 512-block render,
                         printed as the Stage-1 passthrough baseline.

    Phase 2.1 replaces P1 with the delay-compensated FUNC-02 null and adds the
    DSP probes; this file is the scaffold those land in.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
    constexpr double kFs       = 48000.0;
    constexpr int    kMaxBlock = 4096;
    constexpr int    kRenderSamples = 512 * 64;   // canonical render length

    int failures = 0;

    void check (const char* name, bool ok, const juce::String& detail = {})
    {
        std::printf ("  [%s] %s%s%s\n", ok ? "PASS" : "FAIL", name,
                     detail.isEmpty() ? "" : " — ",
                     detail.isEmpty() ? "" : detail.toRawUTF8());
        if (! ok)
            ++failures;
    }

    std::unique_ptr<OEmulatorAudioProcessor> makeProc()
    {
        auto p = std::make_unique<OEmulatorAudioProcessor>();
        p->setPlayConfigDetails (2, 2, kFs, kMaxBlock);
        p->prepareToPlay (kFs, kMaxBlock);
        return p;
    }

    // Position-deterministic noise: the sample at absolute index n on channel
    // ch is a pure function of (ch, n), independent of block slicing
    // (pattern: block-size invariance probes need position-determinism, not a
    // shared sequential RNG).
    float noiseAt (int ch, int n)
    {
        uint32_t h = (uint32_t) n * 2654435761u ^ (uint32_t) (ch + 1) * 40503u;
        h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
        return (float) ((double) h / 2147483648.0 - 1.0);   // [-1, 1)
    }

    // Render kRenderSamples through a fresh instance using the given block
    // slicing; append the output to `out` (interleaved L,R per sample).
    void render (const std::vector<int>& blockSizes, std::vector<float>& out)
    {
        auto p = makeProc();
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> buf (2, kMaxBlock);

        int pos = 0, sizeIdx = 0;
        while (pos < kRenderSamples)
        {
            const int want = blockSizes[(size_t) sizeIdx % blockSizes.size()];
            const int len  = juce::jmin (want, kRenderSamples - pos);
            ++sizeIdx;

            buf.setSize (2, len, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* w = buf.getWritePointer (ch);
                for (int i = 0; i < len; ++i)
                    w[i] = noiseAt (ch, pos + i);
            }

            p->processBlock (buf, midi);

            for (int i = 0; i < len; ++i)
                for (int ch = 0; ch < 2; ++ch)
                    out.push_back (buf.getSample (ch, i));

            pos += len;
        }
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

    std::printf ("O-Emulator render-harness (Stage 1 scaffold) — fs=%.0f\n", kFs);

    //==========================================================================
    // P0 — parameter contract (BINDING parameter-spec.md).
    {
        auto p = makeProc();

        auto* console = dynamic_cast<juce::AudioParameterChoice*> (p->apvts.getParameter ("console"));
        check ("P0 console exists+choice", console != nullptr);
        if (console != nullptr)
        {
            check ("P0 console 5 choices", console->choices.size() == 5,
                   juce::String (console->choices.size()));
            check ("P0 console default SNES", console->getIndex() == 0);
        }

        struct { const char* id; float def; } floats[] = {
            { "crush", 50.0f }, { "age", 20.0f }, { "reverb", 0.0f }, { "mix", 100.0f } };

        for (const auto& f : floats)
        {
            auto* fp = dynamic_cast<juce::AudioParameterFloat*> (p->apvts.getParameter (f.id));
            check ((juce::String ("P0 ") + f.id + " exists+float").toRawUTF8(), fp != nullptr);
            if (fp != nullptr)
            {
                check ((juce::String ("P0 ") + f.id + " default").toRawUTF8(),
                       juce::approximatelyEqual (fp->get(), f.def),
                       juce::String (fp->get()));
                check ((juce::String ("P0 ") + f.id + " range 0-100").toRawUTF8(),
                       juce::approximatelyEqual (fp->range.start, 0.0f)
                           && juce::approximatelyEqual (fp->range.end, 100.0f));
            }
        }

        check ("P0 latency 0 in Stage 1", p->getLatencySamples() == 0,
               juce::String (p->getLatencySamples()));
    }

    //==========================================================================
    // P1 — passthrough bit-identity: out[n] == in[n], no tolerance.
    std::vector<float> flat;
    flat.reserve ((size_t) kRenderSamples * 2);
    {
        render ({ 512 }, flat);

        bool bitEqual = true;
        for (int n = 0; n < kRenderSamples && bitEqual; ++n)
            for (int ch = 0; ch < 2 && bitEqual; ++ch)
            {
                const float expected = noiseAt (ch, n);
                if (std::memcmp (&flat[(size_t) n * 2 + (size_t) ch],
                                 &expected, sizeof (float)) != 0)
                    bitEqual = false;
            }

        check ("P1 passthrough bit-equal", bitEqual);
    }

    //==========================================================================
    // P2 — ragged block sizes bit-identical to the flat 512 feed.
    {
        std::vector<float> ragged;
        ragged.reserve ((size_t) kRenderSamples * 2);
        render ({ 1, 7, 64, 333, 4096 }, ragged);

        check ("P2 ragged==flat", ragged.size() == flat.size()
                   && std::memcmp (ragged.data(), flat.data(),
                                   flat.size() * sizeof (float)) == 0);
    }

    //==========================================================================
    // Digest — Stage-1 passthrough baseline.
    std::printf ("  digest: fnv1a64=%016llx (%d samples x 2ch, flat 512)\n",
                 (unsigned long long) fnv1a64 (flat), kRenderSamples);

    std::printf ("%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
