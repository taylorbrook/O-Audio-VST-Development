/*
   This file is part of O-Comp, an Ouaricon Audio plugin.
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

    O-Comp bus-dump — a RUNTIME bus-layout gate for the v1.10.0 key input.

    Why this exists, and why it is not an auval/pluginval duplicate:

      - auval proves the AU WRAPPER publishes two input elements. pluginval
        reports only the MAIN bus. Neither walks isBusesLayoutSupported across
        the layouts a host will actually offer, so neither can see a rule that
        accepts a layout the processing code cannot serve.
      - The layout that matters most here is MONO MAIN + STEREO KEY. The v1.9.0
        detector sized its channel loop from buffer.getNumChannels() capped at
        2, which in that layout makes channelPtrs[1] a KEY channel — the
        detector would read it and the gain write would land in it. A host-level
        test only catches that if the tester happens to build a mono track with
        a stereo key routed to it.

    Exit code 0 only if every expectation below holds, so this is a gate and not
    a report. Output is `#`-prefixed context lines, then one TSV row per probed
    layout, then a verdict line.

  ==============================================================================
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{

juce::String setName (const juce::AudioChannelSet& s)
{
    return s.isDisabled() ? juce::String ("disabled") : s.getDescription();
}

struct Probe
{
    const char* what;
    juce::AudioChannelSet mainIn, mainOut, key;
    bool expectSupported;
};

} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::unique_ptr<juce::AudioProcessor> proc (createPluginFilter());

    if (proc == nullptr)
    {
        std::cout << "# FATAL: createPluginFilter() returned null\n";
        return 1;
    }

    const auto mono   = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto quad   = juce::AudioChannelSet::quadraphonic();
    const auto none   = juce::AudioChannelSet::disabled();

    int failures = 0;

    // ── 1. The DECLARATION ───────────────────────────────────────────────────
    const int numInputBuses  = proc->getBusCount (true);
    const int numOutputBuses = proc->getBusCount (false);

    std::cout << "# plugin\t"          << proc->getName()   << "\n";
    std::cout << "# inputBuses\t"      << numInputBuses     << "\n";
    std::cout << "# outputBuses\t"     << numOutputBuses    << "\n";

    for (int i = 0; i < numInputBuses; ++i)
    {
        auto* bus = proc->getBus (true, i);
        std::cout << "# inputBus[" << i << "]\t" << bus->getName()
                  << "\tdefault=" << setName (bus->getDefaultLayout())
                  << "\tenabledByDefault=" << (bus->isEnabledByDefault() ? "yes" : "no")
                  << "\n";
    }

    if (numInputBuses != 2)
    {
        std::cout << "# FAIL: expected 2 input buses (main + key), got " << numInputBuses << "\n";
        ++failures;
    }

    // The key bus MUST ship disabled: that is the entire backward-compatibility
    // argument for MINOR. A key bus enabled by default re-negotiates every
    // existing session's channel strip on first load.
    if (numInputBuses >= 2)
    {
        auto* key = proc->getBus (true, 1);

        if (key->getName() != "Sidechain")
        {
            std::cout << "# FAIL: input bus 1 is named '" << key->getName()
                      << "', expected 'Sidechain'\n";
            ++failures;
        }

        if (key->isEnabledByDefault())
        {
            std::cout << "# FAIL: key bus is enabled by default — an existing session would "
                         "re-negotiate its layout on load\n";
            ++failures;
        }
    }

    // ── 2. The NEGOTIATION ───────────────────────────────────────────────────
    const std::vector<Probe> probes
    {
        // The v1.9.0 layouts, which must still negotiate exactly as before.
        { "stereo main, no key",        stereo, stereo, none,   true  },
        { "mono main, no key",          mono,   mono,   none,   true  },

        // The new ones.
        { "stereo main, stereo key",    stereo, stereo, stereo, true  },
        { "stereo main, mono key",      stereo, stereo, mono,   true  },
        { "mono main, mono key",        mono,   mono,   mono,   true  },
        { "mono main, stereo key",      mono,   mono,   stereo, true  },

        // Still rejected, for the same reason as v1.9.0: the channel-pointer
        // arrays hold 2.
        { "quad main",                  quad,   quad,   none,   false },
        { "stereo in, mono out",        stereo, mono,   none,   false },
        { "mono in, stereo out",        mono,   stereo, none,   false },
        { "stereo main, quad key",      stereo, stereo, quad,   false },
    };

    std::cout << "layout\tmainIn\tmainOut\tkey\texpected\tactual\tverdict\n";

    for (const auto& p : probes)
    {
        juce::AudioProcessor::BusesLayout layout;
        layout.inputBuses.add (p.mainIn);
        layout.inputBuses.add (p.key);
        layout.outputBuses.add (p.mainOut);

        const bool supported = proc->checkBusesLayoutSupported (layout);
        const bool ok = (supported == p.expectSupported);

        if (! ok)
            ++failures;

        std::cout << p.what << "\t"
                  << setName (p.mainIn)  << "\t"
                  << setName (p.mainOut) << "\t"
                  << setName (p.key)     << "\t"
                  << (p.expectSupported ? "supported" : "rejected") << "\t"
                  << (supported         ? "supported" : "rejected") << "\t"
                  << (ok ? "OK" : "MISMATCH") << "\n";
    }

    // ── 3. The layout that broke v1.9.0's channel loop ───────────────────────
    // Mono main + stereo key, rendered for real.
    //
    // The probe is SELF-CALIBRATING, in two passes at the same main level:
    //
    //   A (reference)  mono main, key DISABLED
    //   B (subject)    mono main, stereo key carrying a 0 dBFS DC tone
    //
    // The main signal sits at -40 dBFS, well under the -20 dB default threshold,
    // so pass A compresses by nothing and its output level is exactly whatever
    // the default makeup/output gain happens to be. Comparing B against A rather
    // than against the input means the assertion does not encode a default that
    // a later version could legitimately move.
    //
    // If the detector reaches into the key, pass B sees 0 dBFS instead of -40 and
    // pulls roughly 10 dB of gain reduction, so the two passes separate by far
    // more than the tolerance.
    //
    // THE SETTLING TIME IS THE WHOLE POINT. The first cut of this probe rendered
    // ONE 256-sample block: at the default attack the envelope only climbs from
    // -60 dB to about -35 dB in that time, never crosses the threshold, and the
    // gain stays exactly 1.0 — so the buggy and the fixed build produced
    // identical output and the probe passed against both.
    if (failures == 0)
    {
        const int    blockSize   = 256;
        const int    numBlocks   = 200;          // ~1.07 s at 48 kHz
        const double sampleRate  = 48000.0;
        const float  mainLevel   = juce::Decibels::decibelsToGain (-40.0f);

        // Returns the settled main-output magnitude for the given layout.
        auto render = [&] (bool withKey, bool* keyIntactOut) -> float
        {
            juce::AudioProcessor::BusesLayout layout;
            layout.inputBuses.add (mono);
            layout.inputBuses.add (withKey ? stereo : none);
            layout.outputBuses.add (mono);

            if (! proc->setBusesLayout (layout))
            {
                std::cout << "# FAIL: setBusesLayout(mono main, key "
                          << (withKey ? "stereo" : "disabled") << ") refused\n";
                ++failures;
                return -1.0f;
            }

            proc->prepareToPlay (sampleRate, blockSize);

            const int totalChans = juce::jmax (proc->getTotalNumInputChannels(),
                                               proc->getTotalNumOutputChannels());
            juce::AudioBuffer<float> buffer (totalChans, blockSize);
            juce::MidiBuffer midi;

            bool keyIntact = true;
            float mainPeak = 0.0f;

            for (int b = 0; b < numBlocks; ++b)
            {
                // Refill every block: processBlock writes in place.
                buffer.clear();

                for (int i = 0; i < blockSize; ++i)
                    buffer.setSample (0, i, mainLevel);

                for (int ch = 1; ch < totalChans; ++ch)
                    for (int i = 0; i < blockSize; ++i)
                        buffer.setSample (ch, i, 1.0f);

                proc->processBlock (buffer, midi);

                // Only the last block is measured — the envelope has settled by then.
                if (b == numBlocks - 1)
                {
                    mainPeak = buffer.getMagnitude (0, 0, blockSize);

                    for (int ch = 1; ch < totalChans; ++ch)
                        for (int i = 0; i < blockSize; ++i)
                            if (std::abs (buffer.getSample (ch, i) - 1.0f) > 1.0e-6f)
                                keyIntact = false;
                }
            }

            proc->releaseResources();

            if (keyIntactOut != nullptr)
                *keyIntactOut = keyIntact;

            return mainPeak;
        };

        bool keyIntact = true;
        const float refPeak     = render (false, nullptr);
        const float subjectPeak = render (true,  &keyIntact);

        if (failures == 0)
        {
            const float refDB     = juce::Decibels::gainToDecibels (refPeak,     -120.0f);
            const float subjectDB = juce::Decibels::gainToDecibels (subjectPeak, -120.0f);
            const float deltaDB   = std::abs (subjectDB - refDB);

            std::cout << "# monoMainStereoKey.refOutDB\t"     << refDB     << "\n";
            std::cout << "# monoMainStereoKey.subjectOutDB\t" << subjectDB << "\n";
            std::cout << "# monoMainStereoKey.deltaDB\t"      << deltaDB   << "\n";
            std::cout << "# monoMainStereoKey.keyBufferIntact\t" << (keyIntact ? "yes" : "no") << "\n";

            // Task 2/3 are not written at this point, so the key must not reach
            // the detector at all. Once they land, this probe keeps its meaning:
            // sc_source defaults to Internal, so the key still must not be heard.
            if (deltaDB > 0.1f)
            {
                std::cout << "# FAIL: routing a key changed the main output by "
                          << deltaDB << " dB with sc_source at its Internal default — "
                             "a key channel reached the detector\n";
                ++failures;
            }

            if (! keyIntact)
            {
                std::cout << "# FAIL: the key buffer was written to — the gain write "
                             "reached past the main bus\n";
                ++failures;
            }
        }
    }
    else
    {
        std::cout << "# SKIP: render probe not run (earlier failures would make it "
                     "uninterpretable)\n";
    }

    std::cout << (failures == 0 ? "PASS\n" : "FAIL\n");
    std::cout << "# failures\t" << failures << "\n";

    return failures == 0 ? 0 : 1;
}
