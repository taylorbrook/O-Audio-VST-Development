/*
   This file is part of O-simpleBeatmaker, an Ouaricon Audio plugin.
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

    O-simpleBeatmaker - Concept-isolating factory presets (FUNC-05)

    Six read-only "lesson" patterns that drive the WebView preset-tour buttons.
    Each isolates exactly ONE beatmaking concept so a student can reverse-engineer
    the move (straight → accents → ghosts → swing → humanize → quantize tradeoff).

    This is plain constexpr C++ data — NOT a juce_add_binary_data blob — so it adds
    no second BinaryData namespace (O-simpleGrain Stage-3.1 collision lesson) and
    no new APVTS parameter (the 42-param contract is frozen). The processor applies
    a preset on the message thread via applyConceptPreset(): timing-feel params go
    through setValueNotifyingHost (so the two-way-bound knobs + host automation
    update for free), the grid through the existing thread-safe atomic writers.

    Row order is the Voice enum order (Kick, Snare, Clap, ClosedHat, OpenHat, Tom)
    — identical to kVoicePrefix / kGmNotes / the JS VOICES[] array. Velocities snap
    to the JS display tiers (ghost ≤55 / normal 56–112 / accent >112): ghost 40,
    normal 100, accent 127 (intermediate values used where musical). All six are
    16-step patterns (patternLengthChoice = 1); columns 16..31 stay cleared.

  ==============================================================================
*/

#pragma once
#include <array>
#include <cstdint>
#include "BeatmakerIDs.h"   // kNumVoices, Voice enum order

namespace OSimpleBeatmaker
{
    // One concept-isolating lesson preset. Grids are 16 steps (one bar).
    struct BeatPreset
    {
        const char* name;             // matches the data-preset button label (caption only)
        int   patternLengthChoice;    // APVTS choice index: 0=8, 1=16, 2=32  (all use 1)
        float swing01;                // stored 0..1  (display ×75 → %)
        float humanize01;             // stored 0..1  (display ×100 → %)
        float quantize01;             // stored 0..1  (display ×100 → %)
        float tempoBpm;               // 40..240 (free-run only; ignored while host plays)
        std::array<std::array<uint8_t, 16>, (size_t) kNumVoices> grid; // row=voice, 0=off,1..127=vel
    };

    // Index order MUST match the .tour-btn DOM order in index.html (0 Straight … 5 Quantize Demo).
    inline constexpr std::array<BeatPreset, 6> kBeatPresets {{
        // name                 len  swing hum  quant tempo  grid {Kick,Snare,Clap,ClosedHat,OpenHat,Tom}
        { "Straight",            1, 0.00f,0.00f,1.00f,120.f, {{
            {100,0,0,0,100,0,0,0,100,0,0,0,100,0,0,0},
            {  0,0,0,0,100,0,0,0,  0,0,0,0,100,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {100,0,100,0,100,0,100,0,100,0,100,0,100,0,100,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
        { "Backbeat + Accents",  1, 0.00f,0.00f,1.00f,100.f, {{
            {127,0,0,0, 90,0,0,0,100,0,0,0, 90,0,0,0},
            {  0,0,0,0,127,0,0,0,  0,0,0,0,127,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {100,0,50,0,100,0,50,0,100,0,50,0,100,0,50,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
        { "Ghost Notes",         1, 0.00f,0.00f,1.00f, 90.f, {{
            {110,0,0,0,  0,0,0,0,100,0,0,0,  0,0,0,0},
            {  0,0,40,0,127,0,40,0,  0,0,40,0,127,0,40,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {100,0,100,0,100,0,100,0,100,0,100,0,100,0,100,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
        { "Triplet Swing",       1, 0.80f,0.00f,1.00f, 95.f, {{
            {100,0,0,0,  0,0,0,0,100,0,0,0,  0,0,0,0},
            {  0,0,0,0,110,0,0,0,  0,0,0,0,110,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {100,80,100,80,100,80,100,80,100,80,100,80,100,80,100,80},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
        { "Humanized",           1, 0.00f,0.70f,0.25f,110.f, {{
            {100,0,0,0,  0,0,0,0,100,0,0,0,  0,0,0,0},
            {  0,0,0,0,110,0,0,0,  0,0,0,0,110,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {100,0,100,0,100,0,100,0,100,0,100,0,100,0,100,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
        { "Quantize Demo",       1, 0.60f,0.85f,0.50f,100.f, {{
            {100,0,0,0,  0,0,0,0,100,0,0,0,  0,0,0,0},
            {  0,0,0,0,120,0,0,0,  0,0,0,0,120,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            { 90,80,90,80,90,80,90,80,90,80,90,80,90,80,90,80},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
    }};
}
