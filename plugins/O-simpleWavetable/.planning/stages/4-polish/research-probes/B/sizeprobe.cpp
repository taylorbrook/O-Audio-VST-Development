// W2 / notes 6-7 probe: (1) file size that carries exactly the importer's
// 524,288-sample cap per format; (2) the shipped importer's cost per stage on
// such a file (decode -> encodeFlac16 -> buildImportedBank), plus the restore
// path (base64 -> decodeFlac16 -> build) and the time to free a 256-frame bank.
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include "WavetableImporter.h"
#include <cstdio>

using namespace juce;
static double nowMs() { return Time::getMillisecondCounterHiRes(); }

static void fillSignal (AudioBuffer<float>& b, bool noise)
{
    Random r (7);
    for (int c = 0; c < b.getNumChannels(); ++c)
    {
        auto* d = b.getWritePointer (c);
        double ph = 0.0;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            if (noise) { d[i] = r.nextFloat() * 1.6f - 0.8f; continue; }
            const double f = 55.0 * std::pow (2.0, 4.0 * i / (double) b.getNumSamples());   // 4-octave saw sweep
            ph += f / 44100.0; ph -= std::floor (ph);
            d[i] = (float) (0.7 * (2.0 * ph - 1.0)) * (c == 0 ? 1.0f : 0.9f);
        }
    }
}

static MemoryBlock writeFile (AudioFormat& fmt, const AudioBuffer<float>& b, int bits)
{
    MemoryBlock mb;
    {
        std::unique_ptr<OutputStream> os = std::make_unique<MemoryOutputStream> (mb, false);
        auto w = fmt.createWriterFor (os, AudioFormatWriterOptions{}.withSampleRate (44100.0)
                                              .withNumChannels (b.getNumChannels()).withBitsPerSample (bits)
                                              .withSampleFormat (bits == 32 ? AudioFormatWriterOptions::SampleFormat::floatingPoint
                                                                            : AudioFormatWriterOptions::SampleFormat::integral));
        if (w == nullptr) return {};
        w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
    return mb;
}

int main()
{
    constexpr int kCap = 256 * 2048;   // 524,288
    std::printf ("format,ch,bits,signal,bytes,MiB,frames_decoded,decode_ms,encflac_ms,build_ms\n");
    WavAudioFormat wav; AiffAudioFormat aiff; FlacAudioFormat flac;
    struct Row { AudioFormat* f; const char* name; int ch; int bits; bool noise; };
    const Row rows[] = {
        { &wav, "wav", 1, 16, false }, { &wav, "wav", 2, 16, false }, { &wav, "wav", 1, 24, false }, { &wav, "wav", 2, 24, false },
        { &wav, "wav", 1, 32, false }, { &wav, "wav", 2, 32, false }, { &wav, "wav", 6, 24, false }, { &wav, "wav", 8, 32, false },
        { &aiff, "aiff", 2, 16, false }, { &aiff, "aiff", 2, 24, false },
        { &flac, "flac", 1, 16, false }, { &flac, "flac", 2, 16, false }, { &flac, "flac", 2, 24, false },
        { &flac, "flac", 2, 16, true },  { &flac, "flac", 2, 24, true },
    };
    for (const auto& r : rows)
    {
        AudioBuffer<float> b (r.ch, kCap);
        fillSignal (b, r.noise);
        auto file = writeFile (*r.f, b, r.bits);
        if (file.getSize() == 0) { std::printf ("%s,%d,%d,%s,WRITER-REFUSED\n", r.name, r.ch, r.bits, r.noise ? "noise" : "sweep"); continue; }

        AudioFormatManager fm; fm.registerBasicFormats();
        double t0 = nowMs();
        std::unique_ptr<AudioFormatReader> rd (fm.createReaderFor (std::make_unique<MemoryInputStream> (file, false)));
        auto res = WavetableImporter::decode (*rd, "x.wav", [] { return false; });
        const double tDec = nowMs() - t0;
        t0 = nowMs();
        auto data = WavetableImporter::encodeFlac16 (res.pcm.pcm);
        const double tEnc = nowMs() - t0;
        t0 = nowMs();
        auto bank = WavetableImporter::buildImportedBank (res.pcm);
        const double tBuild = nowMs() - t0;
        std::printf ("%s,%d,%d,%s,%zu,%.3f,%d,%.1f,%.1f,%.1f\n", r.name, r.ch, r.bits, r.noise ? "noise" : "sweep",
                     file.getSize(), file.getSize() / 1048576.0, res.pcm.numFrames, tDec, tEnc, tBuild);
    }

    // Restore path (note 7): base64 -> MemoryBlock -> decodeFlac16 -> build; then free.
    {
        AudioBuffer<float> b (1, kCap); fillSignal (b, false);
        auto file = writeFile (wav, b, 16);
        AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<AudioFormatReader> rd (fm.createReaderFor (std::make_unique<MemoryInputStream> (file, false)));
        auto res = WavetableImporter::decode (*rd, "x.wav", [] { return false; });
        const auto data = WavetableImporter::encodeFlac16 (res.pcm.pcm);

        double t0 = nowMs();
        MemoryBlock bytes;
        { MemoryOutputStream mos (bytes, false); Base64::convertFromBase64 (mos, data); }
        const double tB64 = nowMs() - t0;
        ImportedPcm pcm; pcm.numFrames = 256;
        t0 = nowMs();
        const bool ok = WavetableImporter::decodeFlac16 (bytes, 256, pcm.pcm);
        const double tFlac = nowMs() - t0;
        t0 = nowMs();
        auto bank = WavetableImporter::buildImportedBank (pcm);
        const double tBuild = nowMs() - t0;
        const size_t bankBytes = bank->data.size() * sizeof (float) + bank->thumbs.size() * sizeof (float);
        t0 = nowMs();
        bank.reset();
        const double tFree = nowMs() - t0;
        std::printf ("restore256: blob %d chars, base64 %.2f ms, flac %.2f ms (%s), build %.2f ms, bank %.1f MB, free %.3f ms\n",
                     data.length(), tB64, tFlac, ok ? "ok" : "FAIL", tBuild, bankBytes / 1.0e6, tFree);
        // copies taken under bankStateLock in writeImportedBank: String refcount copy
        t0 = nowMs();
        var v;
        for (int i = 0; i < 1000; ++i) v = var (data);
        std::printf ("String->var refcount copy of the blob x1000: %.3f ms (len %d)\n", nowMs() - t0, v.toString().length());
    }
    return 0;
}
