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

    WavetableData.h
    Multi-Bank Wavetable System for O-IntonationPad

    20 wavetable banks with mipmap anti-aliasing (11 band-limited levels per frame)
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
    constexpr int NUM_BANKS = 20;
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
        DarkMatter,
        Sine,
        Square,
        Triangle,
        SpectralCloud,
        MetallicResonance,
        FormantVowel,
        WarmSub,
        SoftFlute,
        VelvetPad,
        Whisper,
        DeepHaze
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
        double phaseOffset = 0.0;
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

    // Bank 2: Choir — ensemble detuning + formant peaks (F1~700, F2~1200, F3~2500 Hz)
    // Micro-detuned pairs simulate multiple singers; phase diversity for natural interference
    constexpr std::array<Partial, 16> choirPartials = {{
        { 1.0,    0.00, 0.00, 1.00, 0.0  },    // Fundamental
        { 1.003,  0.02, 0.08, 0.80, 1.2  },    // Detuned unison (sharp) — ensemble width
        { 0.997,  0.02, 0.08, 0.80, 2.5  },    // Detuned unison (flat) — ensemble width
        { 2.0,    0.04, 0.15, 0.50, 0.4  },    // Octave
        { 2.005,  0.06, 0.18, 0.40, 1.8  },    // Detuned octave — choral spread
        { 3.0,    0.08, 0.22, 0.75, 0.7  },    // F1 formant peak (~700 Hz)
        { 4.0,    0.12, 0.30, 0.25, 1.5  },    // Valley between F1 and F2
        { 5.0,    0.16, 0.35, 0.65, 2.2  },    // F2 formant rise
        { 6.0,    0.20, 0.40, 0.70, 0.3  },    // F2 formant peak (~1200 Hz)
        { 7.0,    0.28, 0.48, 0.20, 1.9  },    // Valley — spectral dip
        { 8.0,    0.35, 0.55, 0.18, 2.8  },    // Low energy region
        { 9.0,    0.40, 0.60, 0.45, 0.6  },    // F3 formant rise
        { 10.0,   0.45, 0.65, 0.50, 3.1  },    // F3 formant peak (~2500 Hz)
        { 12.0,   0.55, 0.75, 0.35, 1.4  },    // Singer's formant support
        { 14.0,   0.62, 0.82, 0.20, 2.0  },    // Breathiness / air
        { 16.0,   0.72, 0.90, 0.10, 0.9  },    // High-frequency presence
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

    // Bank 9: Sine — pure fundamental, no harmonics
    constexpr std::array<Partial, 1> sinePartials = {{
        { 1.0, 0.00, 0.00, 1.00 },
    }};

    // Bank 10: Square — odd harmonics, amplitude 1/n
    constexpr std::array<Partial, 12> squarePartials = {{
        { 1.0,  0.00, 0.00, 1.0000 },
        { 3.0,  0.05, 0.20, 0.3333 },
        { 5.0,  0.10, 0.30, 0.2000 },
        { 7.0,  0.15, 0.40, 0.1429 },
        { 9.0,  0.20, 0.50, 0.1111 },
        { 11.0, 0.28, 0.58, 0.0909 },
        { 13.0, 0.35, 0.65, 0.0769 },
        { 15.0, 0.42, 0.72, 0.0667 },
        { 17.0, 0.50, 0.78, 0.0588 },
        { 19.0, 0.55, 0.82, 0.0526 },
        { 21.0, 0.62, 0.87, 0.0476 },
        { 23.0, 0.70, 0.92, 0.0435 },
    }};

    // Bank 11: Triangle — odd harmonics, amplitude 1/n², alternating phase
    constexpr std::array<Partial, 8> trianglePartials = {{
        { 1.0,  0.00, 0.00, 1.0000, 0.0 },
        { 3.0,  0.05, 0.20, 0.1111, PI  },
        { 5.0,  0.12, 0.32, 0.0400, 0.0 },
        { 7.0,  0.20, 0.45, 0.0204, PI  },
        { 9.0,  0.30, 0.55, 0.0123, 0.0 },
        { 11.0, 0.42, 0.68, 0.0083, PI  },
        { 13.0, 0.55, 0.78, 0.0059, 0.0 },
        { 15.0, 0.65, 0.88, 0.0044, PI  },
    }};

    // Bank 12: Spectral Cloud — dense atmospheric texture with micro-detuned partials
    constexpr std::array<Partial, 16> spectralCloudPartials = {{
        { 1.0,    0.00, 0.00, 1.00, 0.0 },    // Fundamental
        { 1.003,  0.02, 0.10, 0.70, 1.2 },    // Micro-detuned unison (sharp)
        { 0.997,  0.02, 0.10, 0.70, 2.8 },    // Micro-detuned unison (flat)
        { 2.0,    0.05, 0.20, 0.45, 0.5 },    // Octave
        { 2.01,   0.08, 0.25, 0.40, 1.9 },    // Detuned octave — shimmer
        { 1.498,  0.10, 0.30, 0.50, 3.1 },    // Slightly flat fifth — width
        { 1.503,  0.10, 0.30, 0.50, 0.7 },    // Slightly sharp fifth — width
        { 3.0,    0.15, 0.35, 0.30, 2.2 },    // 12th
        { 0.5,    0.20, 0.40, 0.55, 1.4 },    // Sub-octave — warmth
        { 4.0,    0.25, 0.50, 0.20, 0.3 },    // 2 octaves
        { 2.5,    0.30, 0.55, 0.25, 2.6 },    // Major 10th
        { 1.333,  0.35, 0.60, 0.30, 1.8 },    // Perfect fourth
        { 5.0,    0.45, 0.70, 0.15, 3.4 },    // High partial
        { 3.5,    0.50, 0.75, 0.18, 0.9 },    // Harmonic 7th
        { 6.0,    0.60, 0.82, 0.10, 2.0 },    // Shimmer
        { 7.01,   0.70, 0.90, 0.08, 1.1 },    // Detuned 7th harmonic
    }};

    // Bank 13: Metallic Resonance — inharmonic partials from circular membrane modes (Bessel zeros)
    constexpr std::array<Partial, 12> metallicResonancePartials = {{
        { 1.0,    0.00, 0.00, 1.00, 0.0 },    // Fundamental
        { 1.593,  0.05, 0.18, 0.65, 0.8 },    // Mode (0,1)
        { 2.136,  0.10, 0.28, 0.50, 1.6 },    // Mode (1,1)
        { 2.296,  0.08, 0.22, 0.55, 2.4 },    // Mode (2,0)
        { 2.653,  0.15, 0.35, 0.40, 0.4 },    // Mode (0,2)
        { 3.156,  0.22, 0.45, 0.30, 3.0 },    // Mode (1,2)
        { 3.501,  0.30, 0.55, 0.25, 1.2 },    // Mode (2,1)
        { 4.059,  0.38, 0.62, 0.18, 2.0 },    // Mode (3,0)
        { 4.601,  0.48, 0.72, 0.12, 0.6 },    // Mode (0,3)
        { 5.132,  0.55, 0.78, 0.10, 2.8 },    // Higher mode
        { 5.406,  0.65, 0.85, 0.08, 1.4 },    // Higher mode
        { 6.345,  0.72, 0.92, 0.05, 3.2 },    // Highest mode
    }};

    // Bank 14: Formant Vowel — vowel color shift across wavetable position (ah→eh→ee→oh→oo)
    // Integer harmonics 1-12 with staggered fade-in to approximate formant sweeps
    // Designed for tenor range (~200-400 Hz fundamental)
    constexpr std::array<Partial, 12> formantVowelPartials = {{
        { 1.0,   0.00, 0.00, 1.00, 0.0 },    // H1: fundamental — all vowels
        { 2.0,   0.00, 0.00, 0.75, 0.0 },    // H2: "ah" F1 anchor (~650 Hz)
        { 3.0,   0.00, 0.08, 0.60, 0.0 },    // H3: "ah" F2 lower (~1080 Hz)
        { 4.0,   0.05, 0.18, 0.45, 0.0 },    // H4: "ah" F2 upper region
        { 5.0,   0.16, 0.32, 0.55, 0.0 },    // H5: "eh" F2 transition (~1700 Hz)
        { 6.0,   0.24, 0.42, 0.65, 0.0 },    // H6: "ee" F2 peak (~1870 Hz)
        { 7.0,   0.34, 0.52, 0.40, 0.0 },    // H7: "ee" F2 upper
        { 8.0,   0.44, 0.62, 0.30, 0.0 },    // H8: bridge to F3 region
        { 9.0,   0.48, 0.65, 0.55, 0.0 },    // H9: F3 all vowels (~2650 Hz)
        { 10.0,  0.56, 0.74, 0.25, 0.0 },    // H10: upper F3 shimmer
        { 11.0,  0.66, 0.82, 0.18, 0.0 },    // H11: breathiness
        { 12.0,  0.76, 0.92, 0.10, 0.0 },    // H12: air/sibilance
    }};

    // Bank 15: Warm Sub — sub-heavy with controlled upper harmonics, phase-coherent
    constexpr std::array<Partial, 10> warmSubPartials = {{
        { 1.0,    0.00, 0.00, 1.00, 0.0 },    // Fundamental
        { 0.5,    0.00, 0.00, 0.85, 0.0 },    // Sub-octave — always present
        { 0.25,   0.05, 0.15, 0.50, 0.0 },    // 2 octaves below
        { 0.333,  0.08, 0.22, 0.40, 0.0 },    // Sub-fifth
        { 0.75,   0.10, 0.25, 0.35, 0.0 },    // Sub-fourth
        { 1.5,    0.20, 0.40, 0.25, 0.0 },    // Fifth — gentle warmth
        { 2.0,    0.30, 0.50, 0.30, 0.0 },    // Octave up — subtle
        { 3.0,    0.42, 0.62, 0.18, 0.0 },    // 12th — faint
        { 4.0,    0.55, 0.75, 0.12, 0.0 },    // 2 octaves up — barely present
        { 5.0,    0.68, 0.88, 0.06, 0.0 },    // Major 17th — hint of brightness
    }};

    // Bank 16: Soft Flute — breathy, fundamental-heavy with faint odd harmonics
    // Emulates a wooden flute played softly: dominant fundamental, gentle upper partials, airy quality
    constexpr std::array<Partial, 10> softFlutePartials = {{
        { 1.0,   0.00, 0.00, 1.00, 0.0 },    // Strong fundamental
        { 2.0,   0.08, 0.25, 0.25, 0.0 },    // Gentle octave — warmth
        { 3.0,   0.12, 0.35, 0.15, 0.0 },    // Soft 12th — faint color
        { 4.0,   0.25, 0.50, 0.08, 0.0 },    // Very faint double octave
        { 5.0,   0.40, 0.65, 0.05, 0.0 },    // Barely present
        { 1.002, 0.05, 0.18, 0.20, 1.5 },    // Micro-detune — breathiness
        { 0.998, 0.05, 0.18, 0.20, 2.8 },    // Micro-detune — breathiness
        { 6.0,   0.55, 0.78, 0.03, 0.0 },    // Air/breath noise hint
        { 2.003, 0.15, 0.38, 0.12, 0.9 },    // Detuned octave — soft chorus
        { 7.0,   0.65, 0.88, 0.02, 0.0 },    // Highest whisper
    }};

    // Bank 17: Velvet Pad — ultra-smooth with even harmonics and steep rolloff
    // Creamy analog pad texture: strong even harmonics, suppressed odds, no inharmonics
    constexpr std::array<Partial, 10> velvetPadPartials = {{
        { 1.0,   0.00, 0.00, 1.00, 0.0 },    // Fundamental
        { 2.0,   0.03, 0.15, 0.60, 0.0 },    // Dominant octave — creamy
        { 4.0,   0.08, 0.25, 0.35, 0.0 },    // Double octave — smooth
        { 6.0,   0.15, 0.38, 0.20, 0.0 },    // Even harmonic — warm
        { 8.0,   0.25, 0.48, 0.12, 0.0 },    // Even harmonic — gentle
        { 3.0,   0.30, 0.55, 0.10, 0.0 },    // Suppressed odd — subtle body
        { 10.0,  0.40, 0.65, 0.06, 0.0 },    // High even — faint shimmer
        { 5.0,   0.45, 0.68, 0.05, 0.0 },    // Suppressed odd — hint
        { 12.0,  0.55, 0.78, 0.03, 0.0 },    // Very high even — sparkle
        { 0.5,   0.20, 0.42, 0.30, 0.0 },    // Sub-octave — depth and body
    }};

    // Bank 18: Whisper — micro-detuned partials at low amplitudes, airy shimmer
    // Ghostly, barely-there texture: chorus from detuning, late-blooming overtones
    constexpr std::array<Partial, 14> whisperPartials = {{
        { 1.0,    0.00, 0.00, 0.80, 0.0 },    // Fundamental — softer than usual
        { 1.004,  0.02, 0.08, 0.65, 1.3 },    // Micro-detune pair A+
        { 0.996,  0.02, 0.08, 0.65, 2.7 },    // Micro-detune pair A-
        { 2.0,    0.10, 0.30, 0.20, 0.0 },    // Soft octave
        { 2.007,  0.12, 0.32, 0.18, 1.8 },    // Detuned octave+
        { 1.997,  0.12, 0.32, 0.18, 3.1 },    // Detuned octave-
        { 1.5,    0.20, 0.45, 0.15, 0.5 },    // Gentle fifth
        { 1.502,  0.22, 0.48, 0.12, 2.2 },    // Detuned fifth
        { 3.0,    0.35, 0.60, 0.08, 0.0 },    // Very faint 12th
        { 3.005,  0.38, 0.62, 0.06, 1.0 },    // Detuned 12th
        { 4.0,    0.50, 0.75, 0.04, 0.0 },    // Ghost partial
        { 0.5,    0.15, 0.40, 0.25, 0.0 },    // Sub warmth
        { 5.0,    0.60, 0.82, 0.03, 0.0 },    // Highest ghost
        { 1.333,  0.45, 0.70, 0.06, 1.6 },    // Soft fourth — late bloom
    }};

    // Bank 19: Deep Haze — sub-dominant with slow-blooming mid partials
    // Foggy ambient warmth: prominent sub-octaves, very gradual mid entry, muted highs
    constexpr std::array<Partial, 12> deepHazePartials = {{
        { 1.0,    0.00, 0.00, 1.00, 0.0 },    // Fundamental
        { 0.5,    0.00, 0.00, 0.75, 0.0 },    // Sub-octave — always present
        { 0.25,   0.05, 0.20, 0.45, 0.0 },    // Deep sub — rumble
        { 2.0,    0.20, 0.50, 0.30, 0.0 },    // Slow-blooming octave
        { 0.75,   0.08, 0.28, 0.35, 0.0 },    // Sub-fourth — warmth
        { 1.5,    0.30, 0.58, 0.20, 0.0 },    // Slow fifth
        { 3.0,    0.45, 0.72, 0.10, 0.0 },    // Very late 12th
        { 0.333,  0.12, 0.35, 0.30, 0.0 },    // Sub-fifth — deep fog
        { 4.0,    0.60, 0.82, 0.05, 0.0 },    // Barely audible
        { 1.001,  0.10, 0.30, 0.15, 1.4 },    // Micro-detune — haze effect
        { 0.999,  0.10, 0.30, 0.15, 2.6 },    // Micro-detune — haze effect
        { 2.0,    0.55, 0.78, 0.08, 1.0 },    // Detuned octave — late haze
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
                    sampleValue += amplitude * std::sin(phase * TWO_PI * partial.ratio + partial.phaseOffset);
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
        // Pre-warm all banks in the calling thread (call from background thread)
        static void preWarmAll()
        {
            for (int i = 0; i < NUM_BANKS; ++i)
                getBank(i);
        }

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

        // CR-01: real-time-safe accessor for the audio thread. Returns the bank ONLY if
        // it has already been generated (lock-free, no allocation); returns nullptr
        // otherwise so the caller can keep its current bank until the background
        // pre-warm finishes. NEVER takes the mutex or allocates — unlike getBank().
        static const MipmapTable* getBankIfReady(int bankIndex) noexcept
        {
            auto& instance = getInstance();
            auto idx = static_cast<size_t>(std::max(0, std::min(bankIndex, NUM_BANKS - 1)));
            if (instance.generated[idx].load(std::memory_order_acquire))
                return instance.banks[idx].get();
            return nullptr;
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
                case 9:  fillBankMipmapTable(table, sinePartials); break;
                case 10: fillBankMipmapTable(table, squarePartials); break;
                case 11: fillBankMipmapTable(table, trianglePartials); break;
                case 12: fillBankMipmapTable(table, spectralCloudPartials); break;
                case 13: fillBankMipmapTable(table, metallicResonancePartials); break;
                case 14: fillBankMipmapTable(table, formantVowelPartials); break;
                case 15: fillBankMipmapTable(table, warmSubPartials); break;
                case 16: fillBankMipmapTable(table, softFlutePartials); break;
                case 17: fillBankMipmapTable(table, velvetPadPartials); break;
                case 18: fillBankMipmapTable(table, whisperPartials); break;
                case 19: fillBankMipmapTable(table, deepHazePartials); break;
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
