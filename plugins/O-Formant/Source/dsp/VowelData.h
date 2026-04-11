/*
  ==============================================================================

    VowelData.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Constexpr formant data for 5 cardinal vowels (Csound bass voice).
    Frequencies, bandwidths, gains, and XY positions for vowel morphing.

  ==============================================================================
*/

#pragma once
#include <cmath>

struct VowelEntry
{
    float freq[5];       // F1-F5 Hz
    float bandwidth[5];  // BW1-BW5 Hz
    float gain[5];       // G1-G5 linear (pre-converted from dB)
    float x, y;          // XY position in morph space [0,1]
};

namespace VowelData
{
    static constexpr int kNumVowels = 7;

    // Helper: compile-time dB to linear (approximation using constexpr)
    // For the actual values we just store pre-computed linear gains
    // dB values: A(0,-7,-9,-9,-20), E(0,-12,-9,-12,-18), I(0,-30,-16,-22,-28),
    //            O(0,-11,-21,-20,-40), U(0,-20,-32,-28,-36),
    //            R(0,-8,-14,-24,-30), L(0,-6,-16,-22,-28)

    // Pre-computed linear gain values (pow(10, dB/20))
    // 0dB=1.0, -6dB=0.5012, -7dB=0.4467, -8dB=0.3981, -9dB=0.3548,
    // -11dB=0.2818, -12dB=0.2512, -14dB=0.1995, -16dB=0.1585, -18dB=0.1259,
    // -20dB=0.1000, -21dB=0.0891, -22dB=0.0794, -24dB=0.0631, -28dB=0.0398,
    // -30dB=0.0316, -32dB=0.0251, -36dB=0.0158, -40dB=0.0100

    static constexpr VowelEntry vowels[kNumVowels] =
    {
        // A /a/ - Low open (high F1, mid F2)
        {
            { 600.0f, 1040.0f, 2250.0f, 2450.0f, 2750.0f },   // freq
            { 60.0f,  70.0f,   110.0f,  120.0f,  130.0f },     // bandwidth
            { 1.0f,   0.4467f, 0.3548f, 0.3548f, 0.1000f },    // gain (0,-7,-9,-9,-20 dB)
            0.83f, 0.00f                                         // x, y
        },
        // E /e/ - Mid front
        {
            { 400.0f, 1620.0f, 2400.0f, 2800.0f, 3100.0f },
            { 40.0f,  80.0f,   100.0f,  120.0f,  120.0f },
            { 1.0f,   0.2512f, 0.3548f, 0.2512f, 0.1259f },    // (0,-12,-9,-12,-18 dB)
            0.31f, 0.43f
        },
        // I /i/ - High front (low F1, high F2)
        {
            { 250.0f, 1750.0f, 2600.0f, 3050.0f, 3340.0f },
            { 60.0f,  90.0f,   100.0f,  120.0f,  120.0f },
            { 1.0f,   0.0316f, 0.1585f, 0.0794f, 0.0398f },    // (0,-30,-16,-22,-28 dB)
            0.00f, 1.00f
        },
        // O /o/ - Mid back (mid F1, low F2)
        {
            { 400.0f, 750.0f,  2400.0f, 2600.0f, 2900.0f },
            { 40.0f,  80.0f,   100.0f,  120.0f,  120.0f },
            { 1.0f,   0.2818f, 0.0891f, 0.1000f, 0.0100f },    // (0,-11,-21,-20,-40 dB)
            1.00f, 0.35f
        },
        // U /u/ - High back (low F1, low F2)
        {
            { 350.0f, 600.0f,  2400.0f, 2675.0f, 2950.0f },
            { 40.0f,  80.0f,   100.0f,  120.0f,  120.0f },
            { 1.0f,   0.1000f, 0.0251f, 0.0398f, 0.0158f },    // (0,-20,-32,-28,-36 dB)
            0.98f, 0.93f
        },
        // R /r/ - Retroflex approximant (very low F3 signature)
        {
            { 340.0f, 1050.0f, 1600.0f, 3500.0f, 4300.0f },
            { 60.0f,  90.0f,   130.0f,  250.0f,  280.0f },
            { 1.0f,   0.3981f, 0.1995f, 0.0631f, 0.0316f },    // (0,-8,-14,-24,-30 dB)
            0.12f, 0.72f
        },
        // L /l/ - Dark lateral approximant (low F2, velarized)
        {
            { 400.0f, 900.0f,  2600.0f, 3400.0f, 4200.0f },
            { 80.0f,  120.0f,  150.0f,  250.0f,  280.0f },
            { 1.0f,   0.5012f, 0.1585f, 0.0794f, 0.0398f },    // (0,-6,-16,-22,-28 dB)
            0.55f, 0.85f
        }
    };
} // namespace VowelData
