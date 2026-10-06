/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - WavetableImporter (Stage 2.4)
    Ouaricon Audio
    Developer: Taylor Brook

    Pure functions (no processor state), run on the import worker or on the
    thread that calls setStateInformation. NEVER on the audio thread.

    decode (ARCHITECTURE A8, RESEARCH 5.2):
      - the header length is never trusted: at most 256 * 2048 samples are
        read (long files truncate, not an error); < 2048 -> tooShort
      - ALL channels averaged (cap 64), NaN / inf scrubbed (float WAV)
      - per 2048-sample frame: forward FFT, zero DC + Nyquist, inverse FFT,
        peak-normalise with the boost capped at +24 dB, then lround to the
        canonical int16 (* 32767)

    The int16 PCM is the canonical form of an imported bank. toFloat() is
    the ONLY int16 -> float mapping, and buildImportedBank() is the ONE
    builder used by import, by state restore and by the tests, so a
    restored bank is bit-identical to the imported one (FUNC-04).

    Persistence codecs (RESEARCH 6.1): flac16 (mono 16-bit FLAC, nominal
    48000 Hz, quality 5) and the pcm16gz fallback (gzip-wrapped LE int16),
    both carried as STANDARD base64 (juce::Base64). Decoders validate the
    exact declared length.

  ==============================================================================
*/

#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "WavetableBank.h"

//==============================================================================
struct ImportedPcm
{
    juce::String filename;
    int numFrames = 0;
    std::vector<std::int16_t> pcm;     // numFrames * 2048, canonical int16
};

enum class ImportError { none, unreadable, tooShort, tooLarge, cancelled };

struct ImportResult
{
    ImportError error = ImportError::none;
    ImportedPcm pcm;
};

//==============================================================================
namespace WavetableImporter
{
    constexpr int   kFrame               = WavetableBank::kTableSize;   // 2048
    constexpr int   kMaxFrames           = WavetableBank::kMaxFrames;   // 256
    constexpr int   kMaxChannelsAveraged = 64;
    constexpr float kMaxBoost            = 15.848932f;                  // 10^(24/20): +24 dB cap
    constexpr int   kReadChunk           = 8192;
    constexpr int   kMaxNameChars        = 128;
    constexpr int   kMaxDataChars        = 4 * 1024 * 1024;             // state blob cap (base64 chars)
    constexpr std::size_t kMaxMemoryBytes = (std::size_t) 96 * 1024 * 1024;   // importFromMemory cap (page DROP_MAX_BYTES)

    // Stage 3 error vocabulary ("cancelled" is internal, never surfaced).
    const char* errorCode (ImportError e) noexcept;

    // cancelled() is polled once per read chunk; returning true aborts with
    // ImportError::cancelled.
    ImportResult decode (juce::AudioFormatReader& reader, const juce::String& filename,
                         const std::function<bool()>& cancelled);

    // The ONLY int16 -> float mapping.
    inline float toFloat (std::int16_t q) noexcept { return (float) q / 32767.0f; }

    // ONE builder for import, restore and the FUNC-04 gate. nullptr if the
    // PCM is malformed (numFrames outside 1..256 or a length mismatch).
    std::shared_ptr<const WavetableBank> buildImportedBank (const ImportedPcm& p);

    // Untrusted names (file names, JS-supplied names, state XML): basename
    // only, control characters removed, at most 128 characters, never empty.
    juce::String sanitiseName (const juce::String& raw);

    // Codecs. Encoders return the base64 text (empty on failure); decoders
    // take the base64-DECODED bytes and the declared frame count.
    juce::String encodeFlac16 (const std::vector<std::int16_t>& pcm);
    bool decodeFlac16 (const juce::MemoryBlock& bytes, int numFrames, std::vector<std::int16_t>& out);
    juce::String encodePcm16Gz (const std::vector<std::int16_t>& pcm);
    bool decodePcm16Gz (const juce::MemoryBlock& bytes, int numFrames, std::vector<std::int16_t>& out);
}
