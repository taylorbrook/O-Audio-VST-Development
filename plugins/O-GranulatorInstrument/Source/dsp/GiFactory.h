/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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
#pragma once

#include <juce_core/juce_core.h>
#include <cstddef>
#include <cstdint>

// The FactorySources binary-data target (CMakeLists.txt, NAMESPACE
// FactorySourceData, HEADER_NAME FactorySourceData.h). Until it is linked the
// table is empty and every factory: source resolves to missing.
#if __has_include(<FactorySourceData.h>)
 #include <FactorySourceData.h>
 #define OGI_HAS_FACTORY_SOURCES 1
#else
 #define OGI_HAS_FACTORY_SOURCES 0
#endif

// PLAN-4 PD7: the two 2.6 stubs are harness-only fixtures (the
// OuariconGranulatorInstrument_TestFactory target, linked to the render
// harness only). Shipping builds never see them.
#if OGI_TEST_HOOKS && __has_include(<TestFactoryData.h>)
 #include <TestFactoryData.h>
 #define OGI_HAS_TEST_FACTORY 1
#else
 #define OGI_HAS_TEST_FACTORY 0
#endif

// ============================================================================
// GiFactory — the factory:<id> table (CONTEXT-2.6 D33, PLAN-2.6 PD16, PLAN-4
// PD5 – PD7).
//
// Ids use underscores only (juce_add_binary_data strips hyphens). Entries
// name their BinaryData symbols explicitly (a rename is a compile error);
// never getNamedResource strings. Two pins per source, checked on every
// platform (FSP1; the stubs: FA1):
//   kHash*  the XXH64 content hash of the DECODED PCM (GiHash.h contentHash,
//           the first 131 072 frames) — what a slot entry would carry;
//   kFull*  the same canonical stream over EVERY frame (the full-PCM pin).
// The .flac bytes are pinned on mac arm64 only (tests/golden/
// factory-sources.sha256, factory-stubs.sha256; FLAC encode is not proven
// identical across architectures, RESEARCH-2.6 Summary 3).
//
// The 8 curated sources (render harness --gen-factory, tests/render-harness/
// factory_gen.cpp; 48 kHz, 24-bit; pitched ones at C3 = 130.8128 Hz):
//   harmonic   mono    8 s  24 additive partials, slow per-partial swells
//   pluck      mono    3 s  KS-like decaying partials + a noise pick
//   bell       mono    6 s  inharmonic partials, prime at C3
//   vowel      mono    8 s  pulse harmonics through formants a -> o -> i
//   noise_bed  stereo  6 s  pink-ish noise, slow spectral sweep (16-bit grid)
//   dust       stereo  6 s  sparse crackle + band noise (16-bit grid)
//   drone      stereo 12 s  C2 / G2 / C3 stacks, decorrelated L / R detune
//   pulse      mono    4 s  120 bpm, 2 bars: sine-drop kick + noise hat
// Test fixtures (OGI_TEST_HOOKS only, after the 8):
//   stub_tone   mono,   48 kHz, 3.0 s, 220 Hz, harmonics k = 1…8 at 1/k
//   stub_noise  stereo, 48 kHz, 3.0 s, splitmix noise, one-pole LP
// ============================================================================
namespace ogi
{
    struct FactoryEntry
    {
        const char*   id    = nullptr;
        const void*   data  = nullptr;
        std::size_t   size  = 0;
        std::uint64_t hash  = 0;   // decoded-PCM content hash (131 072 frames)
        std::uint64_t full  = 0;   // decoded-PCM hash over every frame (PLAN-4 PD6)
    };

    // Pinned at PLAN-4 T7 (--gen-factory, mac arm64, 2026-11-11).
    inline constexpr std::uint64_t kHashHarmonic = 0x8d3f53401dd512abull, kFullHarmonic = 0xc66d8c372e1a6466ull;
    inline constexpr std::uint64_t kHashPluck    = 0xced194ec27e461b4ull, kFullPluck    = 0xd852d9429eaded57ull;
    inline constexpr std::uint64_t kHashBell     = 0xf0195e2384e5b366ull, kFullBell     = 0x8d3f987172d73125ull;
    inline constexpr std::uint64_t kHashVowel    = 0x0e61dc5699763d74ull, kFullVowel    = 0xd7725f3426607bf1ull;
    inline constexpr std::uint64_t kHashNoiseBed = 0x3ab6faaf08e4f577ull, kFullNoiseBed = 0x3859a39daead5356ull;
    inline constexpr std::uint64_t kHashDust     = 0x77aae6be696b9d35ull, kFullDust     = 0x139ab02c9dab4494ull;
    inline constexpr std::uint64_t kHashDrone    = 0x428c82363a9af2c5ull, kFullDrone    = 0xa89ce6f4c53fa4b5ull;
    inline constexpr std::uint64_t kHashPulse    = 0x09cc60d1e810229cull, kFullPulse    = 0x1ee8522dcdb667bbull;

    // The stubs: pinned at PLAN-2.6 T10 step 3 (unchanged; harness fixtures).
    inline constexpr std::uint64_t kHashTone  = 0x6d117c4cc9d31ec3ull;
    inline constexpr std::uint64_t kHashNoise = 0xe6c6444b102af6b8ull;

    inline constexpr int kCuratedFactoryCount = 8;

    inline int factoryCount() noexcept
    {
       #if OGI_HAS_FACTORY_SOURCES
        return kCuratedFactoryCount + (OGI_HAS_TEST_FACTORY ? 2 : 0);
       #else
        return 0;
       #endif
    }

    inline FactoryEntry factoryEntry (int i) noexcept
    {
       #if OGI_HAS_FACTORY_SOURCES
        namespace D = FactorySourceData;
        switch (i)
        {
            case 0: return { "harmonic",  D::harmonic_flac,  (std::size_t) D::harmonic_flacSize,  kHashHarmonic, kFullHarmonic };
            case 1: return { "pluck",     D::pluck_flac,     (std::size_t) D::pluck_flacSize,     kHashPluck,    kFullPluck };
            case 2: return { "bell",      D::bell_flac,      (std::size_t) D::bell_flacSize,      kHashBell,     kFullBell };
            case 3: return { "vowel",     D::vowel_flac,     (std::size_t) D::vowel_flacSize,     kHashVowel,    kFullVowel };
            case 4: return { "noise_bed", D::noise_bed_flac, (std::size_t) D::noise_bed_flacSize, kHashNoiseBed, kFullNoiseBed };
            case 5: return { "dust",      D::dust_flac,      (std::size_t) D::dust_flacSize,      kHashDust,     kFullDust };
            case 6: return { "drone",     D::drone_flac,     (std::size_t) D::drone_flacSize,     kHashDrone,    kFullDrone };
            case 7: return { "pulse",     D::pulse_flac,     (std::size_t) D::pulse_flacSize,     kHashPulse,    kFullPulse };
           #if OGI_HAS_TEST_FACTORY
            case 8: return { "stub_tone",  TestFactoryData::stub_tone_flac,  (std::size_t) TestFactoryData::stub_tone_flacSize,  kHashTone,  0 };
            case 9: return { "stub_noise", TestFactoryData::stub_noise_flac, (std::size_t) TestFactoryData::stub_noise_flacSize, kHashNoise, 0 };
           #endif
            default: break;
        }
       #else
        juce::ignoreUnused (i);
       #endif
        return {};
    }

    /** factory:<id> lookup (exact id). False when unknown. No disk access. */
    inline bool findFactory (const juce::String& id, FactoryEntry& out) noexcept
    {
        for (int i = 0; i < factoryCount(); ++i)
        {
            const auto e = factoryEntry (i);
            if (e.id != nullptr && id == e.id)
            {
                out = e;
                return true;
            }
        }
        return false;
    }
} // namespace ogi
