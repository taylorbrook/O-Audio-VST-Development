/*
  ==============================================================================

    StringMaterial.h
    String Material System - Phase 2.5
    Ouaricon Audio
    Developer: Taylor Brook

    Defines material presets for different string types with associated
    physical properties affecting damping, brightness, stiffness, and
    sympathetic coupling. Includes interpolation for smooth morphing
    between materials.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * Material types for harp strings
 */
enum class MaterialType
{
    Gut = 0,        // Warm, mellow, blended
    Nylon,          // Balanced, modern
    Wire,           // Bright, punchy, defined
    Carbon,         // Bright, sustained, piercing
    MetalAlloy,     // Metallic, ethereal, ringing
    Glass,          // Crystalline, bell-like
    Crystal,        // Pure, sustained, magical
    Energy          // Impossible, synthetic (variable)
};

/**
 * String Material Definition
 *
 * Encapsulates all physical properties that define a string material's
 * acoustic behavior in the waveguide model.
 */
struct StringMaterial
{
    float dampingCoeff;        // 0.0-1.0 (higher = faster decay)
    float brightnessCutoff;    // Hz for damping filter
    float stiffnessAmount;     // Allpass dispersion amount 0.0-1.0
    float sympatheticCoupling; // How much resonates with other strings 0.0-1.0
    float noiseContent;        // Attack noise amount 0.0-1.0
    juce::String name;

    /**
     * Construct default material (Nylon)
     */
    StringMaterial()
        : dampingCoeff(0.25f)
        , brightnessCutoff(5000.0f)
        , stiffnessAmount(0.20f)
        , sympatheticCoupling(0.50f)
        , noiseContent(0.5f)
        , name("Nylon")
    {}

    /**
     * Create material from predefined type
     * @param type MaterialType enum value
     * @return StringMaterial with appropriate values
     */
    static StringMaterial fromType(MaterialType type);

    /**
     * Interpolate between this material and another
     * @param other Target material to interpolate toward
     * @param t Interpolation amount (0.0 = this, 1.0 = other)
     * @return New interpolated StringMaterial
     */
    StringMaterial interpolate(const StringMaterial& other, float t) const;

    /**
     * Get material type from integer index (for parameter choice)
     * @param index Material index (0-7)
     * @return MaterialType enum
     */
    static MaterialType typeFromIndex(int index);

    /**
     * Get material name from type
     * @param type MaterialType enum
     * @return Human-readable name
     */
    static juce::String getNameFromType(MaterialType type);

private:
    /**
     * Helper: Linear interpolation with clamping
     */
    static float lerp(float a, float b, float t)
    {
        return a + juce::jlimit(0.0f, 1.0f, t) * (b - a);
    }
};
