/*
  ==============================================================================

    WavetableData.h
    Multi-Bank Wavetable System for O-IntonationPad

    9 wavetable banks with mipmap anti-aliasing (11 band-limited levels per frame)
    Frame 0 = pure sine, Frame 255 = full character for each bank
    Banks are lazily generated on first access and cached statically

  ==============================================================================
*/

#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <memory>

namespace WavetableData
{
    constexpr int NUM_FRAMES = 256;
    constexpr int SAMPLES_PER_FRAME = 2048;
    constexpr int NUM_MIPMAPS = 11;
    constexpr int NUM_BANKS = 9;
    constexpr double PI = 3.14159265358979323846;
    constexpr double TWO_PI = 2.0 * PI;
    constexpr double ASSUMED_SAMPLE_RATE = 48000.0;
    constexpr double NYQUIST = ASSUMED_SAMPLE_RATE / 2.0;

    enum class Bank
    {
        JIHarmonic = 0,
        WarmAnalog,
        Choir,
        Strings,
        Glass,
        Evolving,
        Organ,
        Ethereal,
        DarkMatter
    };

    // Base frequencies for each mipmap level (octave boundaries)
    constexpr std::array<double, NUM_MIPMAPS> mipmapBaseFreqs = {{
        20.0, 40.0, 80.0, 160.0, 320.0, 640.0, 1280.0, 2560.0, 5120.0, 10240.0, 20480.0
    }};

    // ========================================================================
    // Partial definition shared by all banks
    // ========================================================================

    struct Partial
    {
        double ratio;
        double fadeInStart;
        double fadeInEnd;
        double maxAmp;
    };

    // ========================================================================
    // Bank partial definitions
    // ========================================================================

    // Bank 0: JI Harmonic — pure JI ratios, consonant morphing
    constexpr std::array<Partial, 12> jiHarmonicPartials = {{
        { 1.0,      0.00, 0.00, 1.00 },
        { 2.0,      0.05, 0.25, 0.70 },
        { 1.5,      0.10, 0.35, 0.50 },
        { 1.25,     0.15, 0.40, 0.40 },
        { 3.0,      0.25, 0.50, 0.35 },
        { 2.5,      0.30, 0.55, 0.30 },
        { 4.0,      0.35, 0.60, 0.25 },
        { 1.875,    0.40, 0.65, 0.20 },
        { 1.2,      0.50, 0.75, 0.18 },
        { 5.0,      0.55, 0.80, 0.15 },
        { 6.0,      0.65, 0.85, 0.12 },
        { 1.666667, 0.70, 0.90, 0.10 },
    }};

    // Bank 1: Warm Analog — saw-like with soft rolloff, classic analog pad
    constexpr std::array<Partial, 12> warmAnalogPartials = {{
        { 1.0,  0.00, 0.00, 1.00 },
        { 2.0,  0.03, 0.15, 0.50 },
        { 3.0,  0.08, 0.25, 0.33 },
        { 4.0,  0.12, 0.30, 0.25 },
        { 5.0,  0.18, 0.40, 0.20 },
        { 6.0,  0.25, 0.45, 0.16 },
        { 7.0,  0.30, 0.50, 0.14 },
        { 8.0,  0.38, 0.55, 0.12 },
        { 9.0,  0.45, 0.65, 0.10 },
        { 10.0, 0.52, 0.72, 0.08 },
        { 11.0, 0.60, 0.80, 0.06 },
        { 12.0, 0.70, 0.90, 0.05 },
    }};

    // Bank 2: Choir — formant resonances at ~500, ~1500, ~2500 Hz regions
    constexpr std::array<Partial, 12> choirPartials = {{
        { 1.0,   0.00, 0.00, 1.00 },
        { 2.0,   0.05, 0.20, 0.60 },
        { 3.0,   0.10, 0.30, 0.70 },   // ~1st formant emphasis
        { 4.0,   0.15, 0.35, 0.30 },
        { 5.0,   0.20, 0.40, 0.55 },   // ~2nd formant emphasis
        { 6.0,   0.28, 0.48, 0.25 },
        { 7.0,   0.35, 0.55, 0.40 },   // ~3rd formant emphasis
        { 8.0,   0.40, 0.60, 0.18 },
        { 9.0,   0.50, 0.70, 0.22 },
        { 10.0,  0.55, 0.75, 0.15 },
        { 11.0,  0.65, 0.82, 0.12 },
        { 12.0,  0.72, 0.90, 0.08 },
    }};

    // Bank 3: Strings — bowed sawtooth with gentle spectral rolloff
    constexpr std::array<Partial, 12> stringsPartials = {{
        { 1.0,  0.00, 0.00, 1.00 },
        { 2.0,  0.04, 0.18, 0.55 },
        { 3.0,  0.08, 0.22, 0.40 },
        { 4.0,  0.12, 0.28, 0.30 },
        { 5.0,  0.16, 0.35, 0.24 },
        { 6.0,  0.22, 0.42, 0.20 },
        { 7.0,  0.28, 0.50, 0.17 },
        { 8.0,  0.35, 0.58, 0.14 },
        { 9.0,  0.42, 0.65, 0.11 },
        { 10.0, 0.50, 0.72, 0.09 },
        { 11.0, 0.58, 0.80, 0.07 },
        { 12.0, 0.66, 0.88, 0.05 },
    }};

    // Bank 4: Glass — inharmonic bell-like partials with slight detuning
    constexpr std::array<Partial, 12> glassPartials = {{
        { 1.0,    0.00, 0.00, 1.00 },
        { 2.76,   0.05, 0.20, 0.55 },   // Bell partial (~minor 7th above octave)
        { 5.404,  0.10, 0.30, 0.40 },   // Bell partial
        { 8.933,  0.18, 0.40, 0.25 },   // High bell partial
        { 1.506,  0.25, 0.45, 0.45 },   // Slightly sharp fifth
        { 3.516,  0.32, 0.52, 0.30 },   // Inharmonic
        { 6.684,  0.40, 0.62, 0.20 },   // High inharmonic
        { 2.0,    0.15, 0.35, 0.35 },   // Octave (anchor)
        { 4.0,    0.48, 0.68, 0.18 },   // Double octave (anchor)
        { 11.34,  0.55, 0.75, 0.12 },   // Very high bell
        { 7.21,   0.62, 0.82, 0.10 },   // Inharmonic shimmer
        { 14.07,  0.70, 0.90, 0.06 },   // Ultra-high sparkle
    }};

    // Bank 5: Evolving — different partial groups fade in at different rates
    constexpr std::array<Partial, 12> evolvingPartials = {{
        { 1.0,   0.00, 0.00, 1.00 },
        { 1.5,   0.02, 0.12, 0.50 },   // Fifth - early
        { 3.0,   0.05, 0.20, 0.35 },   // 12th - early
        { 2.0,   0.25, 0.45, 0.55 },   // Octave - mid entry
        { 5.0,   0.20, 0.40, 0.30 },   // 2oct+3rd - mid
        { 4.0,   0.40, 0.60, 0.40 },   // 2 octaves - late entry
        { 7.0,   0.35, 0.55, 0.20 },   // 7th harmonic
        { 6.0,   0.55, 0.75, 0.25 },   // Very late
        { 8.0,   0.60, 0.80, 0.18 },   // Very late
        { 1.25,  0.50, 0.70, 0.30 },   // Major third - late bloom
        { 10.0,  0.70, 0.88, 0.12 },   // Final shimmer
        { 9.0,   0.75, 0.92, 0.10 },   // Final shimmer
    }};

    // Bank 6: Organ — strong odd harmonics (pipe organ stops)
    constexpr std::array<Partial, 12> organPartials = {{
        { 1.0,  0.00, 0.00, 1.00 },    // 8' fundamental
        { 2.0,  0.03, 0.12, 0.70 },    // 4' octave
        { 3.0,  0.08, 0.20, 0.55 },    // 2 2/3' nazard
        { 4.0,  0.14, 0.28, 0.50 },    // 2' super octave
        { 5.0,  0.20, 0.38, 0.35 },    // 1 3/5' tierce
        { 6.0,  0.28, 0.45, 0.30 },    // 1 1/3' larigot
        { 8.0,  0.35, 0.52, 0.25 },    // 1' piccolo
        { 0.5,  0.40, 0.58, 0.45 },    // 16' sub-octave (bourdon)
        { 10.0, 0.48, 0.68, 0.15 },    // Mixture partial
        { 12.0, 0.55, 0.75, 0.12 },    // Mixture partial
        { 16.0, 0.65, 0.85, 0.08 },    // Cymbal stop
        { 0.25, 0.72, 0.90, 0.20 },    // 32' sub-sub (pedal)
    }};

    // Bank 7: Ethereal — sparse upper partials, airy and spacious
    constexpr std::array<Partial, 12> etherealPartials = {{
        { 1.0,   0.00, 0.00, 1.00 },
        { 2.0,   0.05, 0.20, 0.40 },   // Soft octave
        { 3.0,   0.15, 0.35, 0.25 },   // Gentle 12th
        { 4.0,   0.30, 0.55, 0.15 },   // Distant 2 octaves
        { 1.498, 0.08, 0.28, 0.50 },   // Slightly flat fifth — ethereal beating
        { 2.003, 0.10, 0.30, 0.30 },   // Slightly sharp octave — shimmer
        { 5.0,   0.40, 0.65, 0.10 },   // High, faint
        { 3.003, 0.45, 0.70, 0.12 },   // Detuned 12th
        { 6.0,   0.55, 0.78, 0.08 },   // Very faint
        { 8.0,   0.65, 0.85, 0.06 },   // Sparkle
        { 0.999, 0.20, 0.50, 0.15 },   // Micro-detuned unison — chorus effect
        { 1.001, 0.22, 0.52, 0.15 },   // Micro-detuned unison — chorus effect
    }};

    // Bank 8: Dark Matter — sub-heavy, minimal upper partials
    constexpr std::array<Partial, 12> darkMatterPartials = {{
        { 1.0,   0.00, 0.00, 1.00 },
        { 0.5,   0.03, 0.15, 0.80 },   // Sub-octave — dominant
        { 0.25,  0.08, 0.25, 0.50 },   // Sub-sub-octave
        { 2.0,   0.15, 0.35, 0.30 },   // Gentle octave
        { 0.333, 0.20, 0.40, 0.35 },   // Sub-fifth (~19th below)
        { 1.5,   0.30, 0.50, 0.20 },   // Fifth — restrained
        { 3.0,   0.45, 0.65, 0.12 },   // 12th — sparse
        { 0.75,  0.35, 0.55, 0.25 },   // Sub-fourth
        { 4.0,   0.60, 0.80, 0.08 },   // Faint upper
        { 0.667, 0.50, 0.72, 0.18 },   // Sub-fifth (inverted)
        { 5.0,   0.72, 0.90, 0.05 },   // Barely present
        { 0.125, 0.40, 0.65, 0.25 },   // 3 octaves below — deep rumble
    }};

    // ========================================================================
    // Generation utilities
    // ========================================================================

    inline double calculateFade(double framePos, double fadeStart, double fadeEnd)
    {
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

    inline bool partialWouldAlias(double partialRatio, int mipmapLevel)
    {
        double maxFundamental = mipmapBaseFreqs[static_cast<size_t>(mipmapLevel)] * 2.0;
        double partialFreq = maxFundamental * partialRatio;
        return partialFreq > (NYQUIST * 0.9);
    }

    // Mipmap storage type: [mipmap_level][frame][sample]
    using MipmapTable = std::array<std::array<std::array<float, SAMPLES_PER_FRAME>, NUM_FRAMES>, NUM_MIPMAPS>;

    // Generic frame generator for any partial set
    template <size_t N>
    inline std::array<float, SAMPLES_PER_FRAME> generateFrame(
        const std::array<Partial, N>& partials, int frameIndex, int mipmapLevel)
    {
        std::array<float, SAMPLES_PER_FRAME> frame{};
        double framePos = static_cast<double>(frameIndex) / static_cast<double>(NUM_FRAMES - 1);

        for (int sample = 0; sample < SAMPLES_PER_FRAME; ++sample)
        {
            double phase = static_cast<double>(sample) / static_cast<double>(SAMPLES_PER_FRAME);
            double sampleValue = 0.0;
            double totalAmplitude = 0.0;

            for (const auto& partial : partials)
            {
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

            if (totalAmplitude > 0.0)
            {
                double normFactor = 1.0 / std::max(totalAmplitude, 1.0);
                sampleValue *= normFactor;
            }

            frame[static_cast<size_t>(sample)] = static_cast<float>(sampleValue);
        }

        return frame;
    }

    // Fill a mipmap table in-place from a partial set (no large return values)
    template <size_t N>
    inline void fillBankMipmapTable(MipmapTable& table, const std::array<Partial, N>& partials)
    {
        for (int mipmap = 0; mipmap < NUM_MIPMAPS; ++mipmap)
        {
            for (int frame = 0; frame < NUM_FRAMES; ++frame)
            {
                table[static_cast<size_t>(mipmap)][static_cast<size_t>(frame)] =
                    generateFrame(partials, frame, mipmap);
            }
        }
    }

    // ========================================================================
    // Lazy bank cache — heap-allocates each bank on first access
    // Each MipmapTable is ~22 MB, so they MUST be heap-allocated (not inline
    // in a static object) to avoid bloating the binary's __DATA segment.
    // ========================================================================

    class BankCache
    {
    public:
        static const MipmapTable& getBank(int bankIndex)
        {
            auto& instance = getInstance();
            auto idx = static_cast<size_t>(std::max(0, std::min(bankIndex, NUM_BANKS - 1)));

            // Fast path: already generated (acquire ensures we see the filled data)
            if (instance.generated[idx].load(std::memory_order_acquire))
                return *instance.banks[idx];

            // Slow path: heap-allocate and generate under lock
            std::lock_guard<std::mutex> lock(instance.mutexes[idx]);
            if (!instance.generated[idx].load(std::memory_order_relaxed))
            {
                instance.banks[idx] = std::make_unique<MipmapTable>();
                fillBank(*instance.banks[idx], static_cast<int>(idx));
                instance.generated[idx].store(true, std::memory_order_release);
            }
            return *instance.banks[idx];
        }

    private:
        std::array<std::unique_ptr<MipmapTable>, NUM_BANKS> banks;
        std::array<std::atomic<bool>, NUM_BANKS> generated;
        std::array<std::mutex, NUM_BANKS> mutexes;

        static BankCache& getInstance()
        {
            static BankCache cache;
            return cache;
        }

        static void fillBank(MipmapTable& table, int bankIndex)
        {
            switch (bankIndex)
            {
                case 0:  fillBankMipmapTable(table, jiHarmonicPartials); break;
                case 1:  fillBankMipmapTable(table, warmAnalogPartials); break;
                case 2:  fillBankMipmapTable(table, choirPartials); break;
                case 3:  fillBankMipmapTable(table, stringsPartials); break;
                case 4:  fillBankMipmapTable(table, glassPartials); break;
                case 5:  fillBankMipmapTable(table, evolvingPartials); break;
                case 6:  fillBankMipmapTable(table, organPartials); break;
                case 7:  fillBankMipmapTable(table, etherealPartials); break;
                case 8:  fillBankMipmapTable(table, darkMatterPartials); break;
                default: fillBankMipmapTable(table, jiHarmonicPartials); break;
            }
        }

        BankCache()
        {
            for (auto& flag : generated)
                flag.store(false, std::memory_order_relaxed);
        }
    };

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

} // namespace WavetableData
