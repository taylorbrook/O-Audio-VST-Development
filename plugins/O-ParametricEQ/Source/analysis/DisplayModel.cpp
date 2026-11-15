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

    O-ParametricEQ - display model. See DisplayModel.h.

  ==============================================================================
*/

#include "DisplayModel.h"

#include "../PluginProcessor.h"

#include <cstring>

static_assert (DisplayModel::numBands == OuariconParametricEQAudioProcessor::kNumBands,
               "one packet per band");

//==============================================================================
DisplayModel::DisplayModel (OuariconParametricEQAudioProcessor& p)
    : processor (p)
{
}

bool DisplayModel::sleeps (const eq::BandInput& input) noexcept
{
    return eq::isGainType (input.settings.type) && eq::sameValue (input.settings.gainDbEff, 0.0);
}

//==============================================================================
juce::Array<int> DisplayModel::poll()
{
    // The rate and the layout the processor prepared; 48 kHz stereo before
    // its first prepare.
    const double rate = processor.getPreparedRate();
    const bool known = rate > 0.0;
    const double fs = known ? rate : fallbackRate;
    const bool st = known ? processor.getPreparedStereo() : true;

    if (! designerReady || ! juce::exactlyEqual (fs, sampleRate) || st != stereo || known != rateKnown)
    {
        designer.prepare (fs);
        sampleRate = fs;
        stereo = st;
        rateKnown = known;
        designerReady = true;

        for (auto& e : entries)
            e.valid = false;
    }

    // The same raw reads as the audio thread's block start.
    const float gainScaleRaw = processor.readGainScaleRaw();

    juce::Array<int> changed;

    for (int b = 0; b < numBands; ++b)
    {
        auto& e = entries[static_cast<std::size_t> (b)];

        const auto now = processor.readBandSnapshot (b, gainScaleRaw);
        static_assert (sizeof (now) == sizeof (e.snapshot), "nine floats, compared by bit pattern");

        if (e.valid && std::memcmp (&now, e.snapshot.data(), sizeof (now)) == 0)
            continue;

        std::memcpy (e.snapshot.data(), &now, sizeof (now));
        e.valid = true;

        e.input = eq::effectiveBandInput (now.raw, now.gainScale, stereo);
        designer.design (e.input.settings, e.design);
        e.active = e.input.active && ! sleeps (e.input);
        ++designCount;

        changed.add (b);
    }

    return changed;
}

//==============================================================================
juce::var DisplayModel::packetFor (int band) const
{
    if (band < 0 || band >= numBands)
        return {};

    const auto& e = entries[static_cast<std::size_t> (band)];

    juce::Array<juce::var> sections;
    juce::Array<juce::var> onePoles;

    for (int i = 0; i < e.design.numSections; ++i)
    {
        const eq::SectionParams& s = e.design.params[static_cast<std::size_t> (i)];

        // An absent half is sent as its neutral values (response 1).
        juce::Array<juce::var> svf;

        if (s.hasSvf)
            svf = { s.g, s.k, s.mL, s.mB, s.mH };
        else
            svf = { 1.0, 1.0, 1.0, 1.0, 1.0 };

        juce::Array<juce::var> onePole;

        if (s.hasOnePole)
            onePole = { s.G, s.cL, s.cH };
        else
            onePole = { 1.0, 1.0, 1.0 };

        sections.add (juce::var (svf));
        onePoles.add (juce::var (onePole));
    }

    auto* object = new juce::DynamicObject();
    object->setProperty ("band", band);
    object->setProperty ("type", static_cast<int> (e.input.settings.type));
    object->setProperty ("placement", static_cast<int> (e.input.placement));
    object->setProperty ("active", e.active);
    object->setProperty ("sections", juce::var (sections));
    object->setProperty ("onePoles", juce::var (onePoles));
    return juce::var (object);
}

juce::var DisplayModel::headerVar() const
{
    auto* object = new juce::DynamicObject();
    object->setProperty ("sampleRate", sampleRate);
    object->setProperty ("stereo", stereo);
    object->setProperty ("rateKnown", rateKnown);
    return juce::var (object);
}

juce::var DisplayModel::allPackets() const
{
    juce::Array<juce::var> packets;

    for (int b = 0; b < numBands; ++b)
        packets.add (packetFor (b));

    return juce::var (packets);
}
