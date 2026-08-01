/*
   This file is part of O-IntonationPad, an Ouaricon Audio plugin.
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

    ReverbProcessor.h
    O-IntonationPad - Wavetable Pad Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class ReverbProcessor
{
public:
    ReverbProcessor() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::dsp::AudioBlock<float>& block);
    void reset();

    // Thread-safe setters (store to atomics; applied in process())
    void setSize (float size);
    void setDamping (float damp);
    void setPredelay (float ms);
    void setMix (float mix);

private:
    void applyReverbParams();

    juce::dsp::Reverb reverb;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayL { 19200 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayR { 19200 };
    juce::dsp::DryWetMixer<float> dryWetMixer;

    float currentSampleRate = 44100.0f;

    // Atomic targets (written from any thread, read on audio thread in process())
    std::atomic<float> targetSize { 0.5f };
    std::atomic<float> targetDamping { 0.5f };
    std::atomic<float> targetPredelayMs { 0.0f };
    std::atomic<float> targetMix { 0.0f };

    // Dirty-flag caching
    float appliedSize = -1.0f;
    float appliedDamping = -1.0f;
};
