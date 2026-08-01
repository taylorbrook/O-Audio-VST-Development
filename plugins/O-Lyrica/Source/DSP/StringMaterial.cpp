/*
   This file is part of O-Lyrica, an Ouaricon Audio plugin.
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

    StringMaterial.cpp
    String Material System - Phase 2.5
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "StringMaterial.h"

StringMaterial StringMaterial::fromType(MaterialType type)
{
    StringMaterial material;

    switch (type)
    {
        case MaterialType::Gut:
            material.dampingCoeff = 0.50f;           // v1.0.3: More damping for warmer sound
            material.brightnessCutoff = 2000.0f;     // v1.0.3: Much darker for audible difference
            material.stiffnessAmount = 0.05f;        // v1.0.3: Lower for softer harmonics
            material.sympatheticCoupling = 0.80f;
            material.noiseContent = 0.2f;
            break;

        case MaterialType::Nylon:
            material.dampingCoeff = 0.25f;
            material.brightnessCutoff = 5000.0f;
            material.stiffnessAmount = 0.20f;
            material.sympatheticCoupling = 0.50f;
            material.noiseContent = 0.5f;
            break;

        case MaterialType::Wire:
            material.dampingCoeff = 0.10f;
            material.brightnessCutoff = 8000.0f;
            material.stiffnessAmount = 0.05f;
            material.sympatheticCoupling = 0.20f;
            material.noiseContent = 0.7f;
            break;

        case MaterialType::Carbon:
            material.dampingCoeff = 0.08f;
            material.brightnessCutoff = 10000.0f;
            material.stiffnessAmount = 0.30f;
            material.sympatheticCoupling = 0.15f;
            material.noiseContent = 0.6f;
            break;

        case MaterialType::MetalAlloy:
            material.dampingCoeff = 0.05f;
            material.brightnessCutoff = 12000.0f;
            material.stiffnessAmount = 0.15f;
            material.sympatheticCoupling = 0.10f;
            material.noiseContent = 0.4f;
            break;

        case MaterialType::Glass:
            material.dampingCoeff = 0.20f;
            material.brightnessCutoff = 9000.0f;
            material.stiffnessAmount = 0.60f;
            material.sympatheticCoupling = 0.70f;
            material.noiseContent = 0.2f;
            break;

        case MaterialType::Crystal:
            material.dampingCoeff = 0.02f;           // v1.0.3: Very long sustain
            material.brightnessCutoff = 16000.0f;    // v1.0.3: Very bright for audible difference
            material.stiffnessAmount = 0.70f;        // v1.0.3: High inharmonicity for bell-like sound
            material.sympatheticCoupling = 0.85f;
            material.noiseContent = 0.05f;           // v1.0.3: Minimal attack noise
            break;

        case MaterialType::Energy:
            // Energy material has variable/dynamic properties
            // These are starting values - can be modulated in real-time
            material.dampingCoeff = 0.15f;
            material.brightnessCutoff = 10000.0f;
            material.stiffnessAmount = 0.40f;
            material.sympatheticCoupling = 0.60f;
            material.noiseContent = 0.8f;
            break;

        default:
            // Default to Nylon (already set in constructor)
            break;
    }

    return material;
}

StringMaterial StringMaterial::interpolate(const StringMaterial& other, float t) const
{
    StringMaterial result;

    result.dampingCoeff = lerp(dampingCoeff, other.dampingCoeff, t);
    result.brightnessCutoff = lerp(brightnessCutoff, other.brightnessCutoff, t);
    result.stiffnessAmount = lerp(stiffnessAmount, other.stiffnessAmount, t);
    result.sympatheticCoupling = lerp(sympatheticCoupling, other.sympatheticCoupling, t);
    result.noiseContent = lerp(noiseContent, other.noiseContent, t);

    // CR-07: no name interpolation — this runs every block during a material crossfade on the
    // AUDIO thread. Building a mixed juce::String here heap-allocated on the render path and the
    // DSP never reads it. Display names are resolved on the message thread via getNameFromType().

    return result;
}

MaterialType StringMaterial::typeFromIndex(int index)
{
    // Clamp to valid range
    index = juce::jlimit(0, 7, index);
    return static_cast<MaterialType>(index);
}

juce::String StringMaterial::getNameFromType(MaterialType type)
{
    switch (type)
    {
        case MaterialType::Gut:         return "Gut";
        case MaterialType::Nylon:       return "Nylon";
        case MaterialType::Wire:        return "Wire";
        case MaterialType::Carbon:      return "Carbon";
        case MaterialType::MetalAlloy:  return "Metal Alloy";
        case MaterialType::Glass:       return "Glass";
        case MaterialType::Crystal:     return "Crystal";
        case MaterialType::Energy:      return "Energy";
        default:                        return "Nylon";
    }
}
