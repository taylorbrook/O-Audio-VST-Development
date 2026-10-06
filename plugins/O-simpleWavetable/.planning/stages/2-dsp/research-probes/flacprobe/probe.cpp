#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <random>
#include <cstdio>

using namespace juce;

static bool roundTrip (const std::vector<int16_t>& pcm, const char* label, double nominalRate)
{
    // ---- encode ----
    MemoryBlock flacBytes;
    {
        FlacAudioFormat flac;
        std::unique_ptr<OutputStream> os = std::make_unique<MemoryOutputStream> (flacBytes, false);
        auto opts = AudioFormatWriterOptions{}.withSampleRate (nominalRate).withNumChannels (1)
                                              .withBitsPerSample (16).withQualityOptionIndex (5);
        auto writer = flac.createWriterFor (os, opts);
        if (writer == nullptr) { std::printf ("%s: createWriterFor FAILED\n", label); return false; }
        std::vector<int> wide (pcm.size());
        for (size_t i = 0; i < pcm.size(); ++i) wide[i] = (int) pcm[i] * 65536;   // left-justified 32-bit
        const int* chans[] = { wide.data(), nullptr };
        if (! writer->write (chans, (int) wide.size())) { std::printf ("%s: write FAILED\n", label); return false; }
        writer.reset();   // finish + STREAMINFO seek-back + stream flush/delete
    }
    // ---- base64 ----
    const String b64 = Base64::toBase64 (flacBytes.getData(), flacBytes.getSize());
    MemoryBlock decodedBytes;
    {
        MemoryOutputStream mos (decodedBytes, false);
        if (! Base64::convertFromBase64 (mos, b64)) { std::printf ("%s: base64 decode FAILED\n", label); return false; }
    }
    // ---- decode ----
    AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> r (fm.createReaderFor (std::make_unique<MemoryInputStream> (decodedBytes, false)));
    if (r == nullptr) { std::printf ("%s: reader FAILED\n", label); return false; }
    std::vector<int> back ((size_t) r->lengthInSamples + 7);
    int* dst[] = { back.data() };
    const bool ok = r->read (dst, 1, 0, (int) r->lengthInSamples, false);
    size_t mism = 0;
    for (size_t i = 0; i < pcm.size(); ++i) if ((int16_t) (back[i] >> 16) != pcm[i] || (back[i] & 0xffff) != 0) ++mism;
    std::printf ("%s: format=%s rate=%.0f ch=%u bits=%u len=%lld (want %zu) read=%d mismatches=%zu flacBytes=%zu (%.1f%% of pcm) b64chars=%d\n",
                 label, r->getFormatName().toRawUTF8(), r->sampleRate, r->numChannels, r->bitsPerSample,
                 (long long) r->lengthInSamples, pcm.size(), (int) ok, mism, flacBytes.getSize(),
                 100.0 * flacBytes.getSize() / (pcm.size() * 2.0), b64.length());
    return ok && mism == 0 && (size_t) r->lengthInSamples == pcm.size();
}

static bool gzRoundTrip (const std::vector<int16_t>& pcm)
{
    MemoryBlock gz;
    {
        MemoryOutputStream mos (gz, false);
        GZIPCompressorOutputStream z (mos, 9);
        for (auto s : pcm) z.writeShort (s);   // little-endian
        z.flush();
    }
    MemoryInputStream mis (gz, false);
    GZIPDecompressorInputStream unz (mis);
    size_t mism = 0, n = 0;
    for (; n < pcm.size(); ++n) { if (unz.isExhausted()) break; if (unz.readShort() != pcm[n]) ++mism; }
    const bool tail = unz.isExhausted();
    std::printf ("pcm16gz: n=%zu mism=%zu exhaustedAfter=%d bytes=%zu (%.1f%%)\n", n, mism, (int) tail, gz.getSize(), 100.0 * gz.getSize() / (pcm.size() * 2.0));
    return mism == 0 && n == pcm.size();
}

int main()
{
    const size_t N = 256 * 2048;
    std::vector<int16_t> noise (N), sine (N), edges (2048 * 3);
    std::mt19937 rng (1234);
    std::uniform_int_distribution<int> d (-32768, 32767);
    for (auto& s : noise) s = (int16_t) d (rng);
    for (size_t i = 0; i < N; ++i) sine[i] = (int16_t) std::lround (32767.0 * std::sin (2.0 * 3.14159265358979 * (double) (i % 2048) / 2048.0 * (1 + (int) (i / 2048) % 7)));
    for (size_t i = 0; i < edges.size(); ++i) edges[i] = (int16_t) (i % 3 == 0 ? -32768 : i % 3 == 1 ? 32767 : 0);
    bool all = true;
    all &= roundTrip (noise, "noise256", 48000.0);
    all &= roundTrip (sine,  "sine256",  48000.0);
    all &= roundTrip (edges, "edges3",   48000.0);
    all &= roundTrip (std::vector<int16_t> (sine.begin(), sine.begin() + 2048), "sine1", 48000.0);
    all &= gzRoundTrip (noise);
    std::printf (all ? "ALL PASS\n" : "SOMETHING FAILED\n");
    return all ? 0 : 1;
}
