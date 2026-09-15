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
#include <cmath>
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

    // The key bus must be ACTIVE by default. The plan originally specified
    // inactive, to leave an existing session's negotiated layout untouched, but
    // Logic Pro does not offer its Side Chain menu for an inactive bus and the
    // whole feature is unreachable there. See the constructor for the evidence.
    //
    // Backward compatibility is now carried by the render probe below rather than
    // by this flag: an active-but-unrouted key must not change the main path.
    if (numInputBuses >= 2)
    {
        auto* key = proc->getBus (true, 1);

        if (key->getName() != "Sidechain")
        {
            std::cout << "# FAIL: input bus 1 is named '" << key->getName()
                      << "', expected 'Sidechain'\n";
            ++failures;
        }

        if (! key->isEnabledByDefault())
        {
            std::cout << "# FAIL: key bus is inactive by default — Logic will not offer "
                         "its Side Chain menu\n";
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
        // arrays hold 2. These constrain the MAIN bus only.
        { "quad main",                  quad,   quad,   none,   false },
        { "stereo in, mono out",        stereo, mono,   none,   false },
        { "mono in, stereo out",        mono,   stereo, none,   false },

        // A WIDE KEY IS ACCEPTED, not refused. Logic probes the key element with
        // counts past 2 and answers a refusal by dropping its Side Chain menu
        // entirely. The detector reads the first two key channels and ignores the
        // rest, so accepting these costs nothing.
        { "stereo main, quad key",      stereo, stereo, quad,   true  },

        // A host does not have to spell a 2-channel key as AudioChannelSet::stereo().
        // AudioChannelSet is a BITSET compared for exact equality, so discreteChannels(2)
        // is a different value from stereo() even though both are two channels.
        { "stereo main, discrete-2 key", stereo, stereo, juce::AudioChannelSet::discreteChannels (2), true },
        { "mono main, discrete-2 key",   mono,   mono,   juce::AudioChannelSet::discreteChannels (2), true },
        { "stereo main, discrete-1 key", stereo, stereo, juce::AudioChannelSet::discreteChannels (1), true },
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

    // ── 4. The external key actually drives the detector ─────────────────────
    // Main sits at -40 dBFS, under the -20 dB default threshold. The key carries a
    // 0 dBFS tone. With sc_source Internal the detector sees -40 and does nothing;
    // with External it sees 0 dBFS and pulls roughly 10 dB. The two must separate,
    // or "External" is a label on a control that changes nothing.
    if (failures == 0)
    {
        auto setParam = [&] (const juce::String& id, float normalised)
        {
            for (auto* param : proc->getParameters())
                if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
                    if (withID->paramID == id)
                    {
                        withID->setValueNotifyingHost (normalised);
                        return true;
                    }

            std::cout << "# FAIL: no parameter with id '" << id << "'\n";
            ++failures;
            return false;
        };

        // Set a frequency parameter in HZ. The normalised value must come from the
        // parameter's own NormalisableRange: sc_hpf / sc_lpf carry a 0.3 skew, so a
        // hand-computed proportion is wrong by a factor of three and silently probes
        // a different corner than the one named here.
        auto setParamHz = [&] (const juce::String& id, float hz)
        {
            for (auto* param : proc->getParameters())
                if (auto* asFloat = dynamic_cast<juce::AudioParameterFloat*> (param))
                    if (asFloat->paramID == id)
                    {
                        asFloat->setValueNotifyingHost (
                            asFloat->getNormalisableRange().convertTo0to1 (hz));
                        return true;
                    }

            std::cout << "# FAIL: no float parameter with id '" << id << "'\n";
            ++failures;
            return false;
        };

        const int    blockSize  = 256;
        const int    numBlocks  = 200;
        const double sampleRate = 48000.0;
        const float  mainLevel  = juce::Decibels::decibelsToGain (-40.0f);

        // external: 0 = Internal, 1 = External. listen: 0 = off, 1 = on.
        auto render = [&] (float external, float listen, float hpf = 0.0f,
                           bool keyRouted = true, bool keySilent = false,
                           double rate = 0.0, float lpfHz = 0.0f,
                           bool* sawNonFinite = nullptr,
                           float mainDB = -40.0f) -> float
        {
            juce::AudioProcessor::BusesLayout layout;
            layout.inputBuses.add (stereo);
            layout.inputBuses.add (keyRouted ? stereo : none);
            layout.outputBuses.add (stereo);

            if (! proc->setBusesLayout (layout))
            {
                std::cout << "# FAIL: setBusesLayout(stereo main + stereo key) refused\n";
                ++failures;
                return -1.0f;
            }

            // Parameters BEFORE prepareToPlay: a value set after prepare can miss
            // the first block's coefficient update.
            setParam   ("sc_source", external);
            setParam   ("sc_listen", listen);
            setParam   ("sc_hpf",    hpf);
            setParamHz ("sc_lpf",    lpfHz);

            // 0 means "the default rate for this harness" — a default argument cannot
            // reference sampleRate, so the sentinel resolves here instead of
            // duplicating the literal and letting the two drift.
            proc->prepareToPlay (rate > 0.0 ? rate : sampleRate, blockSize);

            const int totalChans = juce::jmax (proc->getTotalNumInputChannels(),
                                               proc->getTotalNumOutputChannels());
            juce::AudioBuffer<float> buffer (totalChans, blockSize);
            juce::MidiBuffer midi;
            float mainPeak = 0.0f;

            for (int b = 0; b < numBlocks; ++b)
            {
                buffer.clear();

                const float lvl = juce::Decibels::decibelsToGain (mainDB);

                for (int ch = 0; ch < 2 && ch < totalChans; ++ch)
                    for (int i = 0; i < blockSize; ++i)
                        buffer.setSample (ch, i, lvl);

                // keySilent leaves the key channels at the zeros buffer.clear() wrote:
                // that is what an AU host hands an ENABLED but unrouted sidechain.
                if (! keySilent)
                    for (int ch = 2; ch < totalChans; ++ch)
                        for (int i = 0; i < blockSize; ++i)
                            buffer.setSample (ch, i, 1.0f);

                proc->processBlock (buffer, midi);

                // Checked on EVERY block, not just the last: an unstable detector
                // biquad diverges to inf and then latches NaN, and a later block can
                // read finite again once the state has been clobbered.
                if (sawNonFinite != nullptr)
                    for (int ch = 0; ch < juce::jmin (2, totalChans); ++ch)
                    {
                        const float* p = buffer.getReadPointer (ch);

                        for (int i = 0; i < blockSize; ++i)
                            if (! std::isfinite (p[i]))
                                *sawNonFinite = true;
                    }

                if (b == numBlocks - 1)
                    mainPeak = buffer.getMagnitude (0, 0, blockSize);
            }

            proc->releaseResources();
            return mainPeak;
        };

        const float internalDB = juce::Decibels::gainToDecibels (render (0.0f, 0.0f), -120.0f);
        const float externalDB = juce::Decibels::gainToDecibels (render (1.0f, 0.0f), -120.0f);
        const float listenDB   = juce::Decibels::gainToDecibels (render (1.0f, 1.0f), -120.0f);

        // The key is a DC tone, so a working detector high-pass must remove it
        // ENTIRELY and the compressor must fall back to doing nothing. This is the
        // cheapest possible proof that the filters sit in the detector path and are
        // actually engaged, rather than being computed and discarded.
        const float hpfDB = juce::Decibels::gainToDecibels (render (1.0f, 0.0f, 1.0f), -120.0f);

        // AUTO-FALLBACK. External selected with no key must behave exactly like
        // Internal, not like a compressor whose detector reads silence.
        //
        // v1.10.1 — THE STIMULUS, NOT THE REFERENCE, IS WHAT MAKES THIS GATE REAL.
        // The v1.10.0 form measured against internalDB rather than a constant, which
        // was the right instinct, but drove the plugin at -40 dBFS. The default
        // threshold is -20 dB, so at -40 the compressor does nothing at all and
        // Internal, a working fallback and a detector reading pure silence ALL read
        // -40 dB. The gate could not tell them apart, and went green against a build
        // that went inert in Logic.
        //
        // These run at -6 dBFS instead: above the threshold, clear of the 6 dB knee,
        // where a working detector pulls threshold + overshoot/ratio = 7 dB of gain
        // reduction and a detector reading zeros pulls none. 7 dB of daylight.
        const float hotMainDB = -6.0f;

        auto renderHot = [&] (bool keyRouted, bool keySilent) -> float
        {
            return juce::Decibels::gainToDecibels (
                render (1.0f, 0.0f, 0.0f, keyRouted, keySilent, 0.0, 0.0f, nullptr,
                        hotMainDB),
                -120.0f);
        };

        // Internal at the same level: the reference every fallback is measured
        // against, so the gate cannot pass by both paths breaking the same way.
        const float hotInternalDB = juce::Decibels::gainToDecibels (
            render (0.0f, 0.0f, 0.0f, true, false, 0.0, 0.0f, nullptr, hotMainDB),
            -120.0f);

        // Key bus DISABLED — the VST3 case, where Bus::isEnabled() is false.
        const float fallbackDB = renderHot (false, false);

        // Key bus ROUTED but SILENT — the LOGIC case, and the branch the v1.10.0
        // probe never entered. An AU host does not disable an unrouted sidechain: it
        // negotiates the bus, reports it enabled, and hands it zeros. Every bus-state
        // test reads true here, so only signal presence can tell this from a live key.
        const float silentKeyDB = renderHot (true, true);

        // v1.10.1 — DETECTOR LOW-PASS STABILITY BELOW 40 kHz.
        //
        // "20 kHz is Off" is an absolute ceiling; Nyquist is not. At 32 kHz, Nyquist
        // is 16 kHz, so an sc_lpf of 18 kHz asked makeLowPass for w0 > pi and built a
        // biquad whose poles sit at |z| = 1.32 — the detector state diverges to inf
        // and latches NaN. The corner is named in HZ and converted through the
        // parameter's own skewed range, so this probes 18 kHz and not some other
        // frequency. Rendered at 32 kHz, where the old code was unstable; at 48 kHz
        // 18 kHz is legal and proves nothing.
        bool lowRateNonFinite = false;
        const float lowRateDB = juce::Decibels::gainToDecibels (
            render (0.0f, 0.0f, 0.0f, true, false, 32000.0, 18000.0f, &lowRateNonFinite),
            -120.0f);

        std::cout << "# key.internalOutDB\t" << internalDB << "\n";
        std::cout << "# key.externalOutDB\t" << externalDB << "\n";
        std::cout << "# key.listenOutDB\t"   << listenDB   << "\n";

        if (failures == 0)
        {
            if (std::abs (internalDB - (-40.0f)) > 0.5f)
            {
                std::cout << "# FAIL: Internal source compressed a -40 dBFS signal against a "
                             "-20 dB threshold (" << internalDB << " dB)\n";
                ++failures;
            }

            if ((internalDB - externalDB) < 5.0f)
            {
                std::cout << "# FAIL: External source changed the output by only "
                          << (internalDB - externalDB) << " dB — the key is not reaching "
                             "the detector\n";
                ++failures;
            }

            // SC Listen substitutes the filtered key, which is full scale here.
            if (listenDB < -1.0f)
            {
                std::cout << "# FAIL: SC Listen did not monitor the key (" << listenDB
                          << " dB, expected about 0)\n";
                ++failures;
            }

            std::cout << "# key.hpfOutDB\t"        << hpfDB         << "\n";
            std::cout << "# key.hotInternalDB\t"   << hotInternalDB << "\n";
            std::cout << "# key.hotFallbackDB\t"   << fallbackDB    << "\n";
            std::cout << "# key.hotSilentKeyDB\t"  << silentKeyDB   << "\n";
            std::cout << "# sc.lowRateOutDB\t"    << lowRateDB   << "\n";
            std::cout << "# sc.lowRateFinite\t"   << (lowRateNonFinite ? 0 : 1) << "\n";

            // LIVENESS. Everything below compares against hotInternalDB, so if the
            // reference itself stops compressing — a moved default threshold, ratio
            // or knee — the comparisons go vacuous exactly as the v1.10.0 gate did.
            // Require the reference to pull real gain reduction before trusting it.
            if (hotMainDB - hotInternalDB < 3.0f)
            {
                std::cout << "# FAIL: the -6 dBFS reference render pulled only "
                          << (hotMainDB - hotInternalDB) << " dB of gain reduction —"
                             " the fallback probes below cannot discriminate and this"
                             " gate is vacuous\n";
                ++failures;
            }

            if (std::abs (fallbackDB - hotInternalDB) > 0.1f)
            {
                std::cout << "# FAIL: External with the key bus DISABLED read "
                          << fallbackDB << " dB where Internal reads " << hotInternalDB
                          << " dB — the auto-fallback is not engaging\n";
                ++failures;
            }

            if (std::abs (silentKeyDB - hotInternalDB) > 0.1f)
            {
                std::cout << "# FAIL: External with the key bus ROUTED but SILENT read "
                          << silentKeyDB << " dB where Internal reads " << hotInternalDB
                          << " dB — this is the LOGIC case: the bus is enabled and"
                             " carries zeros, so no bus-state test can see it\n";
                ++failures;
            }

            if (lowRateNonFinite)
            {
                std::cout << "# FAIL: sc_lpf 18 kHz at a 32 kHz rate produced a"
                             " non-finite sample — the detector biquad is above"
                             " Nyquist and its poles are outside the unit circle\n";
                ++failures;
            }

            // Not just finite: the compressor must still be doing its job. A clamped
            // corner is a working filter, and -40 dBFS in must still come out at the
            // level Internal produces at 48 kHz.
            if (std::abs (lowRateDB - internalDB) > 0.5f)
            {
                std::cout << "# FAIL: sc_lpf 18 kHz at 32 kHz read " << lowRateDB
                          << " dB where the 48 kHz internal render reads " << internalDB
                          << " dB — the clamp is not producing a working filter\n";
                ++failures;
            }

            if (std::abs (hpfDB - (-40.0f)) > 0.5f)
            {
                std::cout << "# FAIL: SC HPF at 2 kHz did not remove a DC key — output "
                          << hpfDB << " dB, expected the uncompressed -40\n";
                ++failures;
            }
        }
    }

    std::cout << (failures == 0 ? "PASS\n" : "FAIL\n");
    std::cout << "# failures\t" << failures << "\n";

    return failures == 0 ? 0 : 1;
}
