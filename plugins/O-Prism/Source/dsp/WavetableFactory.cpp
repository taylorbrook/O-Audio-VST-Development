/*
  ==============================================================================

    WavetableFactory.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    Procedural generation algorithms for 28 factory wavetables.

  ==============================================================================
*/

#include "WavetableFactory.h"
#include "MathConstants.h"
#include <cmath>
#include <random>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════

void WavetableFactory::normalizeFrame (float* buffer, int size)
{
    float maxVal = 0.0f;
    for (int i = 0; i < size; ++i)
        maxVal = std::max (maxVal, std::abs (buffer[i]));
    if (maxVal > 0.0f)
        for (int i = 0; i < size; ++i)
            buffer[i] /= maxVal;
}

void WavetableFactory::addHarmonic (float* buffer, int size, int harmonic, double amplitude)
{
    for (int i = 0; i < size; ++i)
        buffer[i] += static_cast<float> (amplitude * std::sin (harmonic * kTwoPi * i / size));
}

void WavetableFactory::addPartial (float* buffer, int size, double freqRatio, double amplitude)
{
    for (int i = 0; i < size; ++i)
        buffer[i] += static_cast<float> (amplitude * std::sin (freqRatio * kTwoPi * i / size));
}

namespace
{
    // Build a WavetableData by zero-filling each frame, calling the supplied
    // per-frame body, normalizing, and finally generating mipmaps. Used by
    // every generator whose per-frame body accumulates into `buf` (via
    // addHarmonic/addPartial or a custom += loop). Generators that write
    // every sample with `=` (FM, Wavefold) or have pre-loop setup state
    // (Bitcrush) keep their bespoke shape.
    template <typename Fn>
    std::unique_ptr<WavetableData> buildTable (int numFrames, Fn&& fillFrame)
    {
        auto table = std::make_unique<WavetableData>();
        table->allocate (numFrames);
        for (int f = 0; f < numFrames; ++f)
        {
            float* buf = table->getFrameData (0, f);
            std::fill (buf, buf + WavetableData::kTableSize, 0.0f);
            fillFrame (f, buf);
            WavetableFactory::normalizeFrame (buf, WavetableData::kTableSize);
        }
        WavetableGenerator::generateMipmaps (*table);
        return table;
    }
}

// ═══════════════════════════════════════════════════════════════════
// Table Info (metadata only)
// ═══════════════════════════════════════════════════════════════════

std::vector<TableInfo> WavetableFactory::getTableInfoList()
{
    return {
        // Analog (0-6)
        { "Saw",              "Analog",   1  },
        { "Square",           "Analog",   1  },
        { "Triangle",         "Analog",   1  },
        { "Sine",             "Analog",   1  },
        { "PWM Sweep",        "Analog",   32 },
        { "Supersaw",         "Analog",   32 },
        { "Sync Sweep",       "Analog",   32 },
        // Digital (7-11)
        { "FM E.Piano",       "Digital",  16 },
        { "FM Bell",          "Digital",  32 },
        { "FM Metallic",      "Digital",  16 },
        { "Wavefold",         "Digital",  32 },
        { "Bitcrush",         "Digital",  16 },
        // Formant (12-15)
        { "Vowel Morph",      "Formant",  64 },
        { "Choir Pad",        "Formant",  32 },
        { "Vocal Lead",       "Formant",  32 },
        { "Formant Filter",   "Formant",  32 },
        // Spectral (16-21)
        { "Harmonic Series",  "Spectral", 32 },
        { "Spectral Tilt",    "Spectral", 16 },
        { "Odd Harmonics",    "Spectral", 32 },
        { "Harmonic Stretch", "Spectral", 16 },
        { "Comb Sweep",       "Spectral", 32 },
        { "Prism Spectrum",   "Spectral", 32 },
        // Organic (22-27)
        { "Breath",           "Organic",  32 },
        { "Plucked String",   "Organic",  32 },
        { "Church Bell",      "Organic",  32 },
        { "Organ Sweep",      "Organic",  16 },
        { "Wind",             "Organic",  16 },
        { "Filtered Noise",   "Organic",  32 },
    };
}

// ═══════════════════════════════════════════════════════════════════
// Factory Library
// ═══════════════════════════════════════════════════════════════════

std::vector<FactoryEntry> WavetableFactory::createFactoryLibrary()
{
    std::vector<FactoryEntry> lib;
    auto infoList = getTableInfoList();

    // Analog basics (indices 0-3): use existing WavetableGenerator
    lib.push_back ({ WavetableGenerator::generateProceduralTable (WaveShape::Saw),      infoList[0]  });
    lib.push_back ({ WavetableGenerator::generateProceduralTable (WaveShape::Square),   infoList[1]  });
    lib.push_back ({ WavetableGenerator::generateProceduralTable (WaveShape::Triangle), infoList[2]  });
    lib.push_back ({ WavetableGenerator::generateProceduralTable (WaveShape::Sine),     infoList[3]  });

    // Analog multi-frame (4-6)
    lib.push_back ({ generatePWMSweep (32),   infoList[4]  });
    lib.push_back ({ generateSupersaw (32),   infoList[5]  });
    lib.push_back ({ generateSyncSweep (32),  infoList[6]  });

    // Digital (7-11)
    lib.push_back ({ generateFM (16, 1.0, 0.5, 3.0),    infoList[7]  }); // E.Piano
    lib.push_back ({ generateFM (32, 1.4, 3.0, 10.0),   infoList[8]  }); // Bell
    lib.push_back ({ generateFM (16, 7.0, 2.0, 6.0),    infoList[9]  }); // Metallic
    lib.push_back ({ generateWavefold (32),              infoList[10] });
    lib.push_back ({ generateBitcrush (16),              infoList[11] });

    // Formant (12-15)
    lib.push_back ({ generateVowelMorph (64),    infoList[12] });
    lib.push_back ({ generateChoirPad (32),      infoList[13] });
    lib.push_back ({ generateVocalLead (32),     infoList[14] });
    lib.push_back ({ generateFormantFilter (32), infoList[15] });

    // Spectral (16-21)
    lib.push_back ({ generateHarmonicSeries (32),  infoList[16] });
    lib.push_back ({ generateSpectralTilt (16),    infoList[17] });
    lib.push_back ({ generateOddHarmonics (32),    infoList[18] });
    lib.push_back ({ generateHarmonicStretch (16), infoList[19] });
    lib.push_back ({ generateCombSweep (32),       infoList[20] });
    lib.push_back ({ generatePrismSpectrum (32),   infoList[21] });

    // Organic (22-27)
    lib.push_back ({ generateBreath (32),         infoList[22] });
    lib.push_back ({ generatePluckedString (32),  infoList[23] });
    lib.push_back ({ generateChurchBell (32),     infoList[24] });
    lib.push_back ({ generateOrganSweep (16),     infoList[25] });
    lib.push_back ({ generateWind (16),           infoList[26] });
    lib.push_back ({ generateFilteredNoise (32),  infoList[27] });

    return lib;
}

// ═══════════════════════════════════════════════════════════════════
// ANALOG: PWM Sweep
// Duty cycle from 50% (square) to 5% (narrow pulse), 32 frames
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generatePWMSweep (int numFrames)
{
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        const int maxH = WavetableData::kTableSize / 2;
        const double dutyCycle = 0.50 - static_cast<double> (f) / (numFrames - 1) * 0.45;
        for (int n = 1; n <= maxH; ++n)
        {
            double amp = (2.0 / (n * kPi)) * std::sin (n * kPi * dutyCycle);
            addHarmonic (buf, WavetableData::kTableSize, n, amp);
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// ANALOG: Supersaw
// 7 detuned saws, detune spread increases across frames
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateSupersaw (int numFrames)
{
    // JP-8000-style offsets (normalized to ±1)
    static constexpr double offsets[7] = { -1.0, -0.58, -0.29, 0.0, 0.29, 0.58, 1.0 };

    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        const int maxH = WavetableData::kTableSize / 2;
        const double maxDetuneCents = static_cast<double> (f) / (numFrames - 1) * 50.0;

        for (int v = 0; v < 7; ++v)
        {
            double detuneCents = offsets[v] * maxDetuneCents;
            double freqRatio = std::pow (2.0, detuneCents / 1200.0);
            int harmonicLimit = static_cast<int> (maxH / std::max (freqRatio, 1.0));

            for (int n = 1; n <= harmonicLimit; ++n)
            {
                double amp = 1.0 / (n * 7.0); // 1/n amplitude, divided by 7 voices
                double phase = kTwoPi * n * freqRatio;
                for (int i = 0; i < WavetableData::kTableSize; ++i)
                    buf[i] += static_cast<float> (amp * std::sin (phase * i / WavetableData::kTableSize));
            }
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// ANALOG: Sync Sweep
// Simulated hard sync, slave ratio 1.0 to 4.0
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateSyncSweep (int numFrames)
{
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        const double slaveRatio = 1.0 + 3.0 * static_cast<double> (f) / (numFrames - 1);
        const int maxH = WavetableData::kTableSize / 2;

        // Hard sync spectrum: harmonics of the master (fundamental), modulated
        // by the slave frequency relationship
        for (int n = 1; n <= maxH; ++n)
        {
            // Sinc-like envelope based on slave ratio
            double x = (n - slaveRatio) * kPi / slaveRatio;
            double amp;
            if (std::abs (x) < 1e-10)
                amp = 1.0 / n;
            else
                amp = std::sin (x) / (x * n);

            addHarmonic (buf, WavetableData::kTableSize, n, amp);
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// DIGITAL: FM Synthesis
// Single-carrier FM: y = sin(2π*t + index * sin(2π*cmRatio*t))
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateFM (int numFrames, double cmRatio,
                                                              double minIndex, double maxIndex)
{
    auto table = std::make_unique<WavetableData>();
    table->allocate (numFrames);

    for (int f = 0; f < numFrames; ++f)
    {
        double modIndex = minIndex + (maxIndex - minIndex) * f / std::max (numFrames - 1, 1);
        float* buf = table->getFrameData (0, f);

        for (int i = 0; i < WavetableData::kTableSize; ++i)
        {
            double t = static_cast<double> (i) / WavetableData::kTableSize;
            double modulator = modIndex * std::sin (kTwoPi * cmRatio * t);
            buf[i] = static_cast<float> (std::sin (kTwoPi * t + modulator));
        }
        normalizeFrame (buf, WavetableData::kTableSize);
    }

    WavetableGenerator::generateMipmaps (*table);
    return table;
}

// ═══════════════════════════════════════════════════════════════════
// DIGITAL: Wavefold
// Sine through sine folder, gain 1x to 9x
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateWavefold (int numFrames)
{
    // Body uses `=` assignment (not `+=`), so the prefill from buildTable is a
    // no-op — every sample gets overwritten before normalize.
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        const double gain = 1.0 + 8.0 * static_cast<double> (f) / (numFrames - 1);

        for (int i = 0; i < WavetableData::kTableSize; ++i)
        {
            double phase = kTwoPi * i / WavetableData::kTableSize;
            double input = std::sin (phase);
            buf[i] = static_cast<float> (std::sin (gain * input));
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// DIGITAL: Bitcrush
// Progressive amplitude quantization from 256 to 4 levels
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateBitcrush (int numFrames)
{
    auto table = std::make_unique<WavetableData>();
    table->allocate (numFrames);

    // Start with a saw wave as source
    std::vector<float> sawBuf (WavetableData::kTableSize);
    for (int n = 1; n <= WavetableData::kTableSize / 2; ++n)
    {
        double amp = 1.0 / n;
        for (int i = 0; i < WavetableData::kTableSize; ++i)
            sawBuf[static_cast<size_t> (i)] += static_cast<float> (amp * std::sin (n * kTwoPi * i / WavetableData::kTableSize));
    }
    normalizeFrame (sawBuf.data(), WavetableData::kTableSize);

    for (int f = 0; f < numFrames; ++f)
    {
        // Exponential sweep from 256 to 4 levels
        double t = static_cast<double> (f) / (numFrames - 1);
        double levels = 256.0 * std::pow (4.0 / 256.0, t); // 256 → 4
        float* buf = table->getFrameData (0, f);

        for (int i = 0; i < WavetableData::kTableSize; ++i)
        {
            double sample = sawBuf[static_cast<size_t> (i)];
            // Quantize to N levels in [-1, 1]
            double quantized = std::round (sample * levels * 0.5) / (levels * 0.5);
            buf[i] = static_cast<float> (juce::jlimit (-1.0, 1.0, quantized));
        }
        normalizeFrame (buf, WavetableData::kTableSize);
    }

    WavetableGenerator::generateMipmaps (*table);
    return table;
}

// ═══════════════════════════════════════════════════════════════════
// FORMANT: Vowel Morph (A → E → I → O → U → A)
// Spectral envelope shaped by formant frequencies
// ═══════════════════════════════════════════════════════════════════

namespace FormantData
{
    struct Vowel { double F[5]; double A[5]; double BW[5]; };

    // Male (Tenor) formant data from KLATT model
    static constexpr Vowel vowels[5] = {
        // /a/ (father)
        {{ 650, 1080, 2650, 2900, 3250 }, { 1.0, 0.501, 0.447, 0.398, 0.079 }, { 80, 90, 120, 130, 140 }},
        // /e/ (bet)
        {{ 400, 1700, 2600, 3200, 3580 }, { 1.0, 0.200, 0.251, 0.200, 0.100 }, { 70, 80, 100, 120, 120 }},
        // /i/ (beet)
        {{ 290, 1870, 2800, 3250, 3540 }, { 1.0, 0.178, 0.355, 0.100, 0.040 }, { 40, 90, 100, 120, 120 }},
        // /o/ (boat)
        {{ 400, 800, 2600, 2800, 3000 }, { 1.0, 0.316, 0.251, 0.251, 0.050 }, { 40, 80, 100, 120, 120 }},
        // /u/ (boot)
        {{ 350, 600, 2700, 2900, 3300 }, { 1.0, 0.100, 0.141, 0.200, 0.050 }, { 40, 60, 100, 120, 120 }},
    };

    static double computeFormantGain (double freq, const Vowel& v)
    {
        double gain = 0.0;
        for (int f = 0; f < 5; ++f)
        {
            double dist = (freq - v.F[f]) / v.BW[f];
            gain += v.A[f] * std::exp (-0.5 * dist * dist);
        }
        return gain;
    }

    static Vowel lerpVowel (const Vowel& a, const Vowel& b, double t)
    {
        Vowel result;
        for (int i = 0; i < 5; ++i)
        {
            result.F[i]  = a.F[i]  + (b.F[i]  - a.F[i])  * t;
            result.A[i]  = a.A[i]  + (b.A[i]  - a.A[i])  * t;
            result.BW[i] = a.BW[i] + (b.BW[i] - a.BW[i]) * t;
        }
        return result;
    }
}

static std::unique_ptr<WavetableData> generateFormantTable (int numFrames,
    const std::vector<std::pair<int, int>>& vowelPairs,
    double refFreq = 261.63)
{
    return buildTable (numFrames, [numFrames, &vowelPairs, refFreq] (int f, float* buf)
    {
        const int maxH = WavetableData::kTableSize / 2;

        // Determine which vowel pair and blend for this frame
        double frameNorm = static_cast<double> (f) / (numFrames - 1);
        int segIndex = static_cast<int> (frameNorm * vowelPairs.size());
        segIndex = std::min (segIndex, static_cast<int> (vowelPairs.size()) - 1);
        double segStart = static_cast<double> (segIndex) / vowelPairs.size();
        double segEnd = static_cast<double> (segIndex + 1) / vowelPairs.size();
        double segT = (frameNorm - segStart) / (segEnd - segStart);
        segT = juce::jlimit (0.0, 1.0, segT);

        auto& [fromIdx, toIdx] = vowelPairs[static_cast<size_t> (segIndex)];
        auto vowel = FormantData::lerpVowel (FormantData::vowels[fromIdx],
                                              FormantData::vowels[toIdx], segT);

        for (int n = 1; n <= maxH; ++n)
        {
            double harmonicFreq = n * refFreq;
            double amp = FormantData::computeFormantGain (harmonicFreq, vowel);
            WavetableFactory::addHarmonic (buf, WavetableData::kTableSize, n, amp);
        }
    });
}

std::unique_ptr<WavetableData> WavetableFactory::generateVowelMorph (int numFrames)
{
    // A → E → I → O → U → A (cyclic)
    std::vector<std::pair<int, int>> pairs = { {0,1}, {1,2}, {2,3}, {3,4}, {4,0} };
    return generateFormantTable (numFrames, pairs);
}

std::unique_ptr<WavetableData> WavetableFactory::generateChoirPad (int numFrames)
{
    // Blended male formants, slow morph A → O → U → A
    std::vector<std::pair<int, int>> pairs = { {0,3}, {3,4}, {4,0} };
    return generateFormantTable (numFrames, pairs, 220.0); // Lower reference for warm pad
}

std::unique_ptr<WavetableData> WavetableFactory::generateVocalLead (int numFrames)
{
    // /e/ → /a/ morph (bright lead character)
    std::vector<std::pair<int, int>> pairs = { {1,0} };
    return generateFormantTable (numFrames, pairs, 330.0);
}

std::unique_ptr<WavetableData> WavetableFactory::generateFormantFilter (int numFrames)
{
    // Resonant peak sweep: single formant moving from 200 to 4000 Hz
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        const int maxH = WavetableData::kTableSize / 2;
        const double refFreq = 261.63;
        const double t = static_cast<double> (f) / (numFrames - 1);
        const double centerFreq = 200.0 * std::pow (4000.0 / 200.0, t); // 200 → 4000 Hz
        const double bandwidth = centerFreq * 0.15; // Q ≈ 6.67

        for (int n = 1; n <= maxH; ++n)
        {
            double harmonicFreq = n * refFreq;
            double dist = (harmonicFreq - centerFreq) / bandwidth;
            double amp = std::exp (-0.5 * dist * dist) / n; // Resonance × saw rolloff
            addHarmonic (buf, WavetableData::kTableSize, n, amp);
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// SPECTRAL: Harmonic Series (1 → 32 harmonics building up)
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateHarmonicSeries (int numFrames)
{
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        // Number of harmonics increases: 1 to 32
        const int numH = 1 + static_cast<int> (31.0 * f / (numFrames - 1));
        for (int n = 1; n <= numH; ++n)
            addHarmonic (buf, WavetableData::kTableSize, n, 1.0 / n);
    });
}

// ═══════════════════════════════════════════════════════════════════
// SPECTRAL: Spectral Tilt (flat → steep rolloff)
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateSpectralTilt (int numFrames)
{
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        const int maxH = 64; // Use 64 harmonics for clear tilt effect
        // Tilt from 0 dB/oct (flat) to -24 dB/oct (very steep)
        const double tiltDbPerOct = -24.0 * static_cast<double> (f) / (numFrames - 1);

        for (int n = 1; n <= maxH; ++n)
        {
            double octavesAboveFundamental = std::log2 (static_cast<double> (n));
            double dbAttenuation = tiltDbPerOct * octavesAboveFundamental;
            double amp = std::pow (10.0, dbAttenuation / 20.0);
            addHarmonic (buf, WavetableData::kTableSize, n, amp);
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// SPECTRAL: Odd Harmonics (building up odd-harmonic content)
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateOddHarmonics (int numFrames)
{
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        // Number of odd harmonics: 1 to 32
        const int numOddH = 1 + static_cast<int> (31.0 * f / (numFrames - 1));
        int count = 0;
        for (int n = 1; count < numOddH; n += 2, ++count)
            addHarmonic (buf, WavetableData::kTableSize, n, 1.0 / n);
    });
}

// ═══════════════════════════════════════════════════════════════════
// SPECTRAL: Harmonic Stretch (harmonic → inharmonic)
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateHarmonicStretch (int numFrames)
{
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        const int numPartials = 32;
        const double stretchFactor = 1.0 + 1.5 * static_cast<double> (f) / (numFrames - 1);

        for (int n = 1; n <= numPartials; ++n)
        {
            double actualFreq = std::pow (static_cast<double> (n), stretchFactor);
            if (actualFreq >= WavetableData::kTableSize / 2) break;
            double amp = 1.0 / n;
            addPartial (buf, WavetableData::kTableSize, actualFreq, amp);
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// SPECTRAL: Comb Filter Sweep
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateCombSweep (int numFrames)
{
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        const int maxH = 64;
        // Comb spacing from 2 to 16
        const double spacing = 2.0 + 14.0 * static_cast<double> (f) / (numFrames - 1);

        for (int n = 1; n <= maxH; ++n)
        {
            // Comb filter: cos^2 envelope at the spacing frequency
            double combGain = std::cos (kPi * n / spacing);
            combGain *= combGain; // cos² for smooth comb
            double amp = combGain / n;
            addHarmonic (buf, WavetableData::kTableSize, n, amp);
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// SPECTRAL: Prism Spectrum
// Rainbow-like spectral coloring — each frame emphasizes a
// different band of harmonics, sweeping from low to high
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generatePrismSpectrum (int numFrames)
{
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        const int maxH = 64;
        const double center = 1.0 + 63.0 * static_cast<double> (f) / (numFrames - 1);
        const double width = 4.0 + 12.0 * static_cast<double> (f) / (numFrames - 1);

        // Always include fundamental for pitch stability
        addHarmonic (buf, WavetableData::kTableSize, 1, 0.5);

        for (int n = 2; n <= maxH; ++n)
        {
            double dist = (n - center) / width;
            double amp = std::exp (-0.5 * dist * dist) / std::sqrt (static_cast<double> (n));
            addHarmonic (buf, WavetableData::kTableSize, n, amp);
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// ORGANIC: Breath
// Formant-shaped spectrum + noise-like random phases
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateBreath (int numFrames)
{
    // rng + phaseDist live in the enclosing scope so the per-frame lambda can
    // capture them by reference — the draw sequence across frames must remain
    // deterministic (seeded at 42) for render-harness identity.
    std::mt19937 rng (42);
    std::uniform_real_distribution<double> phaseDist (0.0, kTwoPi);

    return buildTable (numFrames, [numFrames, &rng, &phaseDist] (int f, float* buf)
    {
        const int maxH = WavetableData::kTableSize / 2;
        const double t = static_cast<double> (f) / (numFrames - 1);
        // Morph from tonal (saw with formant) to breathy (noise with formant)
        const double noiseAmount = t; // 0 = tonal, 1 = noisy
        const double refFreq = 261.63;

        // Use /a/ vowel for breath character
        const auto& vowel = FormantData::vowels[0];

        for (int n = 1; n <= maxH; ++n)
        {
            double harmonicFreq = n * refFreq;
            double formantGain = FormantData::computeFormantGain (harmonicFreq, vowel);
            double sawAmp = (1.0 - noiseAmount) / n;
            double noiseAmp = noiseAmount / std::sqrt (static_cast<double> (n));
            double amp = formantGain * (sawAmp + noiseAmp);

            double phase = phaseDist (rng) * noiseAmount; // Phase randomized by noise amount
            for (int i = 0; i < WavetableData::kTableSize; ++i)
                buf[i] += static_cast<float> (amp * std::sin (n * kTwoPi * i / WavetableData::kTableSize + phase));
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// ORGANIC: Plucked String
// Bright attack → dark sustain (progressive harmonic rolloff)
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generatePluckedString (int numFrames)
{
    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        const int maxH = 128;

        // Frame 0: bright (all harmonics), last frame: dark (steep rolloff)
        // rolloff goes from 0.3 (nearly flat) to 3.0 (very steep 1/n³)
        const double rolloff = 0.3 + 2.7 * static_cast<double> (f) / (numFrames - 1);

        for (int n = 1; n <= maxH; ++n)
        {
            double amp = 1.0 / std::pow (static_cast<double> (n), rolloff);
            addHarmonic (buf, WavetableData::kTableSize, n, amp);
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// ORGANIC: Church Bell
// Inharmonic bell partials with progressive decay across frames
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateChurchBell (int numFrames)
{
    auto table = std::make_unique<WavetableData>();
    table->allocate (numFrames);

    // Church bell partial ratios and initial amplitudes
    const double partialRatios[]  = { 0.5, 1.0, 1.183, 1.506, 2.0, 2.514, 2.997, 3.502 };
    const double partialAmps[]    = { 0.7, 1.0, 0.6,   0.4,   0.8, 0.3,   0.2,   0.15  };
    constexpr int numPartials = 8;

    for (int f = 0; f < numFrames; ++f)
    {
        double decayEnvelope = 1.0 - static_cast<double> (f) / numFrames;
        float* buf = table->getFrameData (0, f);
        std::fill (buf, buf + WavetableData::kTableSize, 0.0f);

        for (int p = 0; p < numPartials; ++p)
        {
            // Higher partials decay faster
            double partialDecay = std::pow (decayEnvelope, 1.0 + p * 0.4);
            double amp = partialAmps[p] * partialDecay;
            addPartial (buf, WavetableData::kTableSize, partialRatios[p], amp);
        }
        normalizeFrame (buf, WavetableData::kTableSize);
    }

    WavetableGenerator::generateMipmaps (*table);
    return table;
}

// ═══════════════════════════════════════════════════════════════════
// ORGANIC: Organ Sweep
// Hammond drawbar morph through classic registrations
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateOrganSweep (int numFrames)
{
    // Hammond drawbar harmonics: 16', 5⅓', 8', 4', 2⅔', 2', 1⅗', 1⅓', 1'
    static constexpr int drawbarHarmonics[9] = { 1, 3, 2, 4, 6, 8, 10, 12, 16 };

    // Classic registrations (drawbar values 0-8, normalized to 0-1)
    static constexpr float registrations[][9] = {
        { 0, 0, 8, 0, 0, 0, 0, 0, 0 }, // Pure 8' flute
        { 0, 0, 8, 4, 0, 0, 0, 0, 0 }, // + 4th harmonic
        { 0, 0, 8, 8, 0, 0, 0, 0, 0 }, // Two footages
        { 8, 0, 8, 0, 0, 0, 0, 0, 0 }, // + Sub
        { 8, 3, 8, 0, 0, 0, 0, 0, 0 }, // Jazz
        { 0, 0, 7, 2, 3, 4, 1, 0, 0 }, // Clarinet
        { 0, 0, 4, 5, 6, 5, 3, 0, 0 }, // Strings
        { 0, 0, 6, 7, 8, 8, 6, 5, 4 }, // Trumpet
        { 8, 8, 8, 6, 4, 3, 2, 0, 0 }, // Gospel
        { 8, 8, 8, 8, 0, 0, 0, 0, 0 }, // Rock
        { 8, 8, 8, 8, 8, 8, 8, 8, 8 }, // Full organ
    };
    static constexpr int numRegs = 11;

    return buildTable (numFrames, [numFrames] (int f, float* buf)
    {
        // Interpolate between registrations
        double pos = static_cast<double> (f) / (numFrames - 1) * (numRegs - 1);
        int regA = static_cast<int> (pos);
        int regB = std::min (regA + 1, numRegs - 1);
        double blend = pos - regA;

        for (int d = 0; d < 9; ++d)
        {
            double drawbarValue = registrations[regA][d] * (1.0 - blend) + registrations[regB][d] * blend;
            double amp = drawbarValue / 8.0; // Normalize drawbar 0-8 to 0-1
            if (amp > 0.001)
                addHarmonic (buf, WavetableData::kTableSize, drawbarHarmonics[d], amp);
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// ORGANIC: Wind
// Filtered noise with moving resonance
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateWind (int numFrames)
{
    // rng + phaseDist captured by reference so the cross-frame draw sequence
    // remains deterministic (seeded at 99) for render-harness identity.
    std::mt19937 rng (99);
    std::uniform_real_distribution<double> phaseDist (0.0, kTwoPi);

    return buildTable (numFrames, [numFrames, &rng, &phaseDist] (int f, float* buf)
    {
        const int maxH = 256;
        const double t = static_cast<double> (f) / (numFrames - 1);
        // Resonant frequency sweeps 400 → 3000 Hz
        const double centerFreq = 400.0 * std::pow (3000.0 / 400.0, t);
        const double bandwidth = centerFreq * 0.4;
        const double refFreq = 261.63;

        for (int n = 1; n <= maxH; ++n)
        {
            double harmonicFreq = n * refFreq;
            double dist = (harmonicFreq - centerFreq) / bandwidth;
            double spectralEnv = std::exp (-0.5 * dist * dist);
            double amp = spectralEnv / std::sqrt (static_cast<double> (n));
            double phase = phaseDist (rng);

            for (int i = 0; i < WavetableData::kTableSize; ++i)
                buf[i] += static_cast<float> (amp * std::sin (n * kTwoPi * i / WavetableData::kTableSize + phase));
        }
    });
}

// ═══════════════════════════════════════════════════════════════════
// ORGANIC: Filtered Noise
// Broadband noise → narrowband tonal transition
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<WavetableData> WavetableFactory::generateFilteredNoise (int numFrames)
{
    // rng + phaseDist captured by reference so the cross-frame draw sequence
    // remains deterministic (seeded at 77) for render-harness identity.
    std::mt19937 rng (77);
    std::uniform_real_distribution<double> phaseDist (0.0, kTwoPi);

    return buildTable (numFrames, [numFrames, &rng, &phaseDist] (int f, float* buf)
    {
        const int maxH = 256;
        const double t = static_cast<double> (f) / (numFrames - 1);

        // Frame 0: all harmonics with random phase (noise-like)
        // Last frame: mostly fundamental + a few harmonics (tonal)
        int numActive = static_cast<int> (maxH * (1.0 - t * 0.95)); // 256 → 13
        numActive = std::max (numActive, 2);

        for (int n = 1; n <= numActive; ++n)
        {
            // Amplitude: noise-like at start, harmonic rolloff at end
            double noiseAmp = 1.0 / std::sqrt (static_cast<double> (n));
            double tonalAmp = 1.0 / n;
            double amp = noiseAmp * (1.0 - t) + tonalAmp * t;

            // Phase: random at start, coherent at end
            double phase = phaseDist (rng) * (1.0 - t);
            for (int i = 0; i < WavetableData::kTableSize; ++i)
                buf[i] += static_cast<float> (amp * std::sin (n * kTwoPi * i / WavetableData::kTableSize + phase));
        }
    });
}
