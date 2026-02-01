/*
  ==============================================================================

    WavetableData.h
    JI Harmonic Morphing Wavetable for O-IntonationPad

    Stage 4.2: Mipmap anti-aliasing with 11 band-limited levels per frame

    Frame 0 = pure sine, Frame 255 = rich JI spectrum
    Partials use consonant JI ratios instead of integer harmonics
    Each mipmap level filters harmonics that would alias at that octave

  ==============================================================================
*/

#pragma once
#include <array>
#include <cmath>
#include <algorithm>

namespace WavetableData
{
    constexpr int NUM_FRAMES = 256;
    constexpr int SAMPLES_PER_FRAME = 2048;
    constexpr int NUM_MIPMAPS = 11;  // One per octave (covers ~20Hz to ~20kHz)
    constexpr double PI = 3.14159265358979323846;
    constexpr double TWO_PI = 2.0 * PI;
    constexpr double ASSUMED_SAMPLE_RATE = 48000.0;
    constexpr double NYQUIST = ASSUMED_SAMPLE_RATE / 2.0;

    // Base frequencies for each mipmap level (octave boundaries)
    // Mipmap 0 is for lowest notes (~20Hz), Mipmap 10 for highest (~20kHz)
    constexpr std::array<double, NUM_MIPMAPS> mipmapBaseFreqs = {{
        20.0,    // Mipmap 0: 20-40 Hz
        40.0,    // Mipmap 1: 40-80 Hz
        80.0,    // Mipmap 2: 80-160 Hz
        160.0,   // Mipmap 3: 160-320 Hz
        320.0,   // Mipmap 4: 320-640 Hz
        640.0,   // Mipmap 5: 640-1280 Hz
        1280.0,  // Mipmap 6: 1280-2560 Hz
        2560.0,  // Mipmap 7: 2560-5120 Hz
        5120.0,  // Mipmap 8: 5120-10240 Hz
        10240.0, // Mipmap 9: 10240-20480 Hz
        20480.0  // Mipmap 10: 20480+ Hz (only fundamental)
    }};

    // 5-limit Just Intonation partial ratios (frequency multipliers)
    struct JIPartial
    {
        double ratio;       // Frequency ratio relative to fundamental
        double fadeInStart; // Frame position (0-1) where partial begins fading in
        double fadeInEnd;   // Frame position (0-1) where partial reaches full amplitude
        double maxAmp;      // Maximum amplitude for this partial
    };

    // 12 JI partials with staggered fade-in points for smooth morphing
    constexpr std::array<JIPartial, 12> jiPartials = {{
        // Fundamental - always present
        { 1.0,      0.00, 0.00, 1.00 },   // Unison (1:1)

        // First layer: basic consonances (frames 0-128)
        { 2.0,      0.05, 0.25, 0.70 },   // Octave (2:1)
        { 1.5,      0.10, 0.35, 0.50 },   // Perfect 5th (3:2)
        { 1.25,     0.15, 0.40, 0.40 },   // Major 3rd (5:4)

        // Second layer: extended consonances (frames 64-192)
        { 3.0,      0.25, 0.50, 0.35 },   // Octave + 5th (3:1)
        { 2.5,      0.30, 0.55, 0.30 },   // Octave + M3 (5:2)
        { 4.0,      0.35, 0.60, 0.25 },   // 2 Octaves (4:1)
        { 1.875,    0.40, 0.65, 0.20 },   // Major 7th (15:8)

        // Third layer: color tones (frames 128-255)
        { 1.2,      0.50, 0.75, 0.18 },   // Minor 3rd (6:5)
        { 5.0,      0.55, 0.80, 0.15 },   // 2 Oct + M3 (5:1)
        { 6.0,      0.65, 0.85, 0.12 },   // 2 Oct + 5th (6:1)
        { 1.666667, 0.70, 0.90, 0.10 },   // Minor 7th (5:3)
    }};

    // Smooth fade function (sine-squared curve for "breathing" feel)
    inline double calculateFade(double framePos, double fadeStart, double fadeEnd)
    {
        // Special case: if fadeStart == fadeEnd, partial is always at full amplitude
        if (fadeStart >= fadeEnd)
            return 1.0;

        if (framePos <= fadeStart)
            return 0.0;
        if (framePos >= fadeEnd)
            return 1.0;

        double t = (framePos - fadeStart) / (fadeEnd - fadeStart);
        double sinVal = std::sin(t * PI * 0.5);
        return sinVal * sinVal;
    }

    // Check if a partial would alias at a given mipmap level
    inline bool partialWouldAlias(double partialRatio, int mipmapLevel)
    {
        // Use the upper frequency of this mipmap's octave range
        double maxFundamental = mipmapBaseFreqs[static_cast<size_t>(mipmapLevel)] * 2.0;
        double partialFreq = maxFundamental * partialRatio;

        // Leave headroom (use 0.9 * Nyquist to avoid edge artifacts)
        return partialFreq > (NYQUIST * 0.9);
    }

    // Generate a single wavetable frame for a specific mipmap level
    inline std::array<float, SAMPLES_PER_FRAME> generateFrameWithMipmap(int frameIndex, int mipmapLevel)
    {
        std::array<float, SAMPLES_PER_FRAME> frame{};
        double framePos = static_cast<double>(frameIndex) / static_cast<double>(NUM_FRAMES - 1);

        // Accumulate partials (filtering those that would alias)
        for (int sample = 0; sample < SAMPLES_PER_FRAME; ++sample)
        {
            double phase = static_cast<double>(sample) / static_cast<double>(SAMPLES_PER_FRAME);
            double sampleValue = 0.0;
            double totalAmplitude = 0.0;

            for (const auto& partial : jiPartials)
            {
                // Skip partials that would alias at this mipmap level
                if (partialWouldAlias(partial.ratio, mipmapLevel))
                    continue;

                double fade = calculateFade(framePos, partial.fadeInStart, partial.fadeInEnd);
                double amplitude = partial.maxAmp * fade;

                if (amplitude > 0.0001)
                {
                    sampleValue += amplitude * std::sin(phase * TWO_PI * partial.ratio);
                    totalAmplitude += amplitude;
                }
            }

            // Normalize to prevent clipping
            if (totalAmplitude > 0.0)
            {
                double normFactor = 1.0 / std::max(totalAmplitude, 1.0);
                sampleValue *= normFactor;
            }

            frame[static_cast<size_t>(sample)] = static_cast<float>(sampleValue);
        }

        return frame;
    }

    // Mipmap storage type: [mipmap_level][frame][sample]
    using MipmapTable = std::array<std::array<std::array<float, SAMPLES_PER_FRAME>, NUM_FRAMES>, NUM_MIPMAPS>;

    // Generate complete mipmap wavetable (11 levels × 256 frames × 2048 samples)
    inline MipmapTable createMipmapWavetable()
    {
        MipmapTable table{};

        for (int mipmap = 0; mipmap < NUM_MIPMAPS; ++mipmap)
        {
            for (int frame = 0; frame < NUM_FRAMES; ++frame)
            {
                table[static_cast<size_t>(mipmap)][static_cast<size_t>(frame)] =
                    generateFrameWithMipmap(frame, mipmap);
            }
        }

        return table;
    }

    // Calculate appropriate mipmap level for a given frequency
    inline int getMipmapLevel(double frequency)
    {
        for (int level = 0; level < NUM_MIPMAPS - 1; ++level)
        {
            if (frequency < mipmapBaseFreqs[static_cast<size_t>(level + 1)])
                return level;
        }
        return NUM_MIPMAPS - 1;
    }

    // Runtime mipmap wavetable storage
    inline const MipmapTable mipmapWavetable = createMipmapWavetable();

    // Legacy accessor (for backward compatibility) - returns mipmap level 0 (full spectrum)
    inline const auto& sineWavetable = mipmapWavetable[0];

} // namespace WavetableData
