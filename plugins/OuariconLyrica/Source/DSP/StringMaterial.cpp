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
            material.dampingCoeff = 0.40f;
            material.brightnessCutoff = 3000.0f;
            material.stiffnessAmount = 0.10f;
            material.sympatheticCoupling = 0.80f;
            material.noiseContent = 0.3f;
            material.name = "Gut";
            break;

        case MaterialType::Nylon:
            material.dampingCoeff = 0.25f;
            material.brightnessCutoff = 5000.0f;
            material.stiffnessAmount = 0.20f;
            material.sympatheticCoupling = 0.50f;
            material.noiseContent = 0.5f;
            material.name = "Nylon";
            break;

        case MaterialType::Wire:
            material.dampingCoeff = 0.10f;
            material.brightnessCutoff = 8000.0f;
            material.stiffnessAmount = 0.05f;
            material.sympatheticCoupling = 0.20f;
            material.noiseContent = 0.7f;
            material.name = "Wire";
            break;

        case MaterialType::Carbon:
            material.dampingCoeff = 0.08f;
            material.brightnessCutoff = 10000.0f;
            material.stiffnessAmount = 0.30f;
            material.sympatheticCoupling = 0.15f;
            material.noiseContent = 0.6f;
            material.name = "Carbon";
            break;

        case MaterialType::MetalAlloy:
            material.dampingCoeff = 0.05f;
            material.brightnessCutoff = 12000.0f;
            material.stiffnessAmount = 0.15f;
            material.sympatheticCoupling = 0.10f;
            material.noiseContent = 0.4f;
            material.name = "Metal Alloy";
            break;

        case MaterialType::Glass:
            material.dampingCoeff = 0.20f;
            material.brightnessCutoff = 9000.0f;
            material.stiffnessAmount = 0.60f;
            material.sympatheticCoupling = 0.70f;
            material.noiseContent = 0.2f;
            material.name = "Glass";
            break;

        case MaterialType::Crystal:
            material.dampingCoeff = 0.03f;
            material.brightnessCutoff = 14000.0f;
            material.stiffnessAmount = 0.50f;
            material.sympatheticCoupling = 0.85f;
            material.noiseContent = 0.1f;
            material.name = "Crystal";
            break;

        case MaterialType::Energy:
            // Energy material has variable/dynamic properties
            // These are starting values - can be modulated in real-time
            material.dampingCoeff = 0.15f;
            material.brightnessCutoff = 10000.0f;
            material.stiffnessAmount = 0.40f;
            material.sympatheticCoupling = 0.60f;
            material.noiseContent = 0.8f;
            material.name = "Energy";
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

    // Name interpolation: show percentage mix
    if (t < 0.01f)
        result.name = name;
    else if (t > 0.99f)
        result.name = other.name;
    else
    {
        int percentage = static_cast<int>(t * 100.0f);
        result.name = name + " → " + other.name + " (" + juce::String(percentage) + "%)";
    }

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
