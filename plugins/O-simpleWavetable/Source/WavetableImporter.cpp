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

    O-simpleWavetable - WavetableImporter implementation (Stage 2.4)

  ==============================================================================
*/

#include "WavetableImporter.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>

#include "MipmapBuilder.h"

namespace WavetableImporter
{
    const char* errorCode (ImportError e) noexcept
    {
        switch (e)
        {
            case ImportError::tooShort:   return "tooShort";
            case ImportError::tooLarge:   return "tooLarge";
            case ImportError::cancelled:  return "cancelled";
            case ImportError::unreadable: return "unreadable";
            case ImportError::none:       break;
        }
        return "";
    }

    //==========================================================================
    ImportResult decode (juce::AudioFormatReader& r, const juce::String& filename,
                         const std::function<bool()>& cancelled)
    {
        ImportResult res;

        if (! std::isfinite (r.sampleRate) || r.sampleRate <= 0.0 || r.numChannels == 0 || r.lengthInSamples <= 0)
        {
            res.error = ImportError::unreadable;
            return res;
        }

        // Never trust the header: read at most 256 frames.
        const juce::int64 n64 = juce::jmin<juce::int64> (r.lengthInSamples, (juce::int64) kMaxFrames * kFrame);
        if (n64 < kFrame)
        {
            res.error = ImportError::tooShort;
            return res;
        }

        const int frames = (int) (n64 / kFrame);                    // tail dropped; 1..256
        const int n = frames * kFrame;
        const int nc = (int) juce::jmin<unsigned int> (r.numChannels, (unsigned int) kMaxChannelsAveraged);

        std::vector<float> mono ((size_t) n, 0.0f);
        juce::AudioBuffer<float> chunk (nc, kReadChunk);

        for (int pos = 0; pos < n; pos += kReadChunk)
        {
            if (cancelled != nullptr && cancelled())
            {
                res.error = ImportError::cancelled;
                return res;
            }

            const int len = juce::jmin (kReadChunk, n - pos);
            chunk.clear();
            if (! r.read (chunk.getArrayOfWritePointers(), nc, (juce::int64) pos, len))
            {
                res.error = ImportError::unreadable;
                return res;
            }

            for (int c = 0; c < nc; ++c)
            {
                const float* s = chunk.getReadPointer (c);
                for (int i = 0; i < len; ++i)
                    mono[(size_t) (pos + i)] += (std::isfinite (s[i]) ? s[i] : 0.0f);   // float WAV NaN / inf scrub
            }
        }

        const float invC = 1.0f / (float) nc;

        juce::dsp::FFT fft (11);
        std::vector<float> work ((size_t) (2 * kFrame), 0.0f);

        res.pcm.filename  = filename;
        res.pcm.numFrames = frames;
        res.pcm.pcm.assign ((size_t) n, 0);

        for (int f = 0; f < frames; ++f)
        {
            if (cancelled != nullptr && cancelled())
            {
                res.error = ImportError::cancelled;
                res.pcm = {};
                return res;
            }

            std::fill (work.begin(), work.end(), 0.0f);
            for (int i = 0; i < kFrame; ++i)
                work[(size_t) i] = mono[(size_t) (f * kFrame + i)] * invC;

            fft.performRealOnlyForwardTransform (work.data());
            work[0] = 0.0f;                                        // DC (DSP-04)
            work[1] = 0.0f;
            work[(size_t) kFrame]     = 0.0f;                      // Nyquist
            work[(size_t) kFrame + 1] = 0.0f;
            std::fill (work.begin() + kFrame + 2, work.end(), 0.0f);   // never read by the real inverse
            fft.performRealOnlyInverseTransform (work.data());    // scaled 1/N internally

            float peak = 0.0f;
            for (int i = 0; i < kFrame; ++i)
                peak = juce::jmax (peak, std::abs (work[(size_t) i]));

            const float g = (peak > 0.0f && std::isfinite (peak)) ? juce::jmin (1.0f / peak, kMaxBoost) : 0.0f;   // silent stays silent

            for (int i = 0; i < kFrame; ++i)
            {
                float v = work[(size_t) i] * g;
                if (! (v >= -1.0f)) v = -1.0f;                     // NaN-safe clamp
                if (v > 1.0f)       v = 1.0f;
                res.pcm.pcm[(size_t) (f * kFrame + i)] = (std::int16_t) std::lround (v * 32767.0f);   // canonical int16
            }
        }

        return res;
    }

    //==========================================================================
    std::shared_ptr<const WavetableBank> buildImportedBank (const ImportedPcm& p)
    {
        if (p.numFrames < 1 || p.numFrames > kMaxFrames
            || p.pcm.size() != (size_t) p.numFrames * (size_t) kFrame)
            return nullptr;

        std::vector<float> level0 (p.pcm.size());
        for (size_t i = 0; i < p.pcm.size(); ++i)
            level0[i] = toFloat (p.pcm[i]);

        auto bank = buildFromLevel0 (level0.data(), p.numFrames, p.filename);
        return std::shared_ptr<const WavetableBank> (std::move (bank));
    }

    //==========================================================================
    juce::String sanitiseName (const juce::String& raw)
    {
        auto base = raw.fromLastOccurrenceOf ("/", false, false)
                       .fromLastOccurrenceOf ("\\", false, false);

        juce::String clean;
        for (auto p = base.getCharPointer(); ! p.isEmpty();)
        {
            const juce::juce_wchar c = p.getAndAdvance();
            if (c >= 0x20 && c != 0x7f)
                clean += c;
        }

        clean = clean.trim();
        if (clean.length() > kMaxNameChars)
            clean = clean.substring (0, kMaxNameChars);

        return clean.isEmpty() ? juce::String ("imported") : clean;
    }

    //==========================================================================
    // Writes left-justified ints (s * 65536): exact, no float conversion.
    // The writer OWNS the stream and rewrites STREAMINFO on destruction, so
    // it is destroyed before the block is read.
    juce::String encodeFlac16 (const std::vector<std::int16_t>& pcm)
    {
        if (pcm.empty())
            return {};

        juce::MemoryBlock mb;
        {
            juce::FlacAudioFormat flac;
            std::unique_ptr<juce::OutputStream> os = std::make_unique<juce::MemoryOutputStream> (mb, false);
            auto w = flac.createWriterFor (os, juce::AudioFormatWriterOptions{}
                                                   .withSampleRate (48000.0)
                                                   .withNumChannels (1)
                                                   .withBitsPerSample (16)
                                                   .withQualityOptionIndex (5));
            if (w == nullptr)
                return {};                                         // no retry loop (JUCE leaks the stream once on failure)

            std::vector<int> wide (pcm.size());
            for (size_t i = 0; i < pcm.size(); ++i)
                wide[i] = (int) pcm[i] * 65536;

            const int* ch[] = { wide.data(), nullptr };
            if (! w->write (ch, (int) wide.size()))
                return {};
        }                                                          // writer + stream destroyed HERE -> STREAMINFO final

        return juce::Base64::toBase64 (mb.getData(), mb.getSize());
    }

    bool decodeFlac16 (const juce::MemoryBlock& bytes, int numFrames, std::vector<std::int16_t>& out)
    {
        out.clear();
        if (numFrames < 1 || numFrames > kMaxFrames || bytes.getSize() == 0)
            return false;

        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (std::make_unique<juce::MemoryInputStream> (bytes, false)));

        const juce::int64 want = (juce::int64) numFrames * kFrame;
        if (r == nullptr || r->getFormatName() != "FLAC file" || r->numChannels != 1
            || r->bitsPerSample != 16 || r->lengthInSamples != want)
            return false;

        std::vector<int> wide ((size_t) want, 0);
        int* dst[] = { wide.data() };
        if (! r->read (dst, 1, 0, (int) want, false))
            return false;

        out.resize ((size_t) want);
        for (size_t i = 0; i < out.size(); ++i)
            out[i] = (std::int16_t) (wide[i] >> 16);
        return true;
    }

    //==========================================================================
    // gzip-wrapped little-endian int16 (honest name).
    juce::String encodePcm16Gz (const std::vector<std::int16_t>& pcm)
    {
        if (pcm.empty())
            return {};

        juce::MemoryBlock mb;
        {
            juce::MemoryOutputStream mos (mb, false);
            juce::GZIPCompressorOutputStream z (mos, 9, juce::GZIPCompressorOutputStream::windowBitsGZIP);
            for (auto s : pcm)
                z.writeShort ((short) s);                          // little-endian
            z.flush();
        }
        return juce::Base64::toBase64 (mb.getData(), mb.getSize());
    }

    // Exact-length read (zip-bomb safe): exactly numFrames * 2048 samples,
    // and trailing data is rejected.
    bool decodePcm16Gz (const juce::MemoryBlock& bytes, int numFrames, std::vector<std::int16_t>& out)
    {
        out.clear();
        if (numFrames < 1 || numFrames > kMaxFrames || bytes.getSize() == 0)
            return false;

        const int numSamples = numFrames * kFrame;
        const int wantBytes = numSamples * 2;

        juce::MemoryInputStream mis (bytes, false);
        juce::GZIPDecompressorInputStream unz (&mis, false, juce::GZIPDecompressorInputStream::gzipFormat);

        std::vector<unsigned char> raw ((size_t) wantBytes, 0);
        int got = 0;
        while (got < wantBytes)                                    // bounded: every pass reads >= 1 byte or exits
        {
            const int r = unz.read (raw.data() + got, wantBytes - got);
            if (r <= 0)
                return false;
            got += r;
        }

        unsigned char extra = 0;
        if (unz.read (&extra, 1) > 0)
            return false;                                          // trailing data

        out.resize ((size_t) numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto lo = (unsigned int) raw[(size_t) (2 * i)];
            const auto hi = (unsigned int) raw[(size_t) (2 * i + 1)];
            out[(size_t) i] = (std::int16_t) (std::uint16_t) (lo | (hi << 8));
        }
        return true;
    }
}
