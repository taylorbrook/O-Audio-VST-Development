// W2 probe: Base64::convertFromBase64 into an unsized vs pre-sized MemoryBlock,
// plus the message-thread costs around it (UTF-8 length scan, JSON parse of the
// bridge message as a proxy). juce_core only. -O3, JUCE 8.0.15.
#include <juce_core/juce_core.h>
#include <cstdio>

using namespace juce;

static double nowMs() { return Time::getMillisecondCounterHiRes(); }

int main (int argc, char** argv)
{

    (void) argc; (void) argv;

    const size_t sizesMiB[] = { 1, 2, 4, 8, 16, 32, 64, 96 };
    Random rng (12345);

    std::printf ("MiB,chars,utf8len_ms,decode_unsized_ms,decode_presized_ms,decode_preallocate_ms,json_parse_ms,ok\n");
    for (auto mib : sizesMiB)
    {
        const size_t n = mib * 1024 * 1024;
        MemoryBlock src (n, false);
        auto* p = static_cast<uint8*> (src.getData());
        for (size_t i = 0; i < n; ++i)
            p[i] = (uint8) rng.nextInt (256);

        const String b64 = Base64::toBase64 (src.getData(), src.getSize());

        double t0 = nowMs();
        const auto bytesUtf8 = b64.getNumBytesAsUTF8();
        const double tLen = nowMs() - t0;

        // (a) shipped: unsized MemoryBlock (PluginProcessor.cpp:1121-1126)
        MemoryBlock a;
        t0 = nowMs();
        bool okA;
        {
            MemoryOutputStream out (a, false);
            okA = Base64::convertFromBase64 (out, b64);
        }
        const double tA = nowMs() - t0;

        // (b) pre-sized: setSize(expected + 1) before the stream (append = false keeps the allocation)
        MemoryBlock b;
        t0 = nowMs();
        bool okB;
        {
            b.setSize ((size_t) bytesUtf8 / 4 * 3 + 4, false);
            MemoryOutputStream out (b, false);
            okB = Base64::convertFromBase64 (out, b64);
        }
        const double tB = nowMs() - t0;

        // (c) MemoryOutputStream::preallocate
        MemoryBlock c;
        t0 = nowMs();
        bool okC;
        {
            MemoryOutputStream out (c, false);
            out.preallocate ((size_t) bytesUtf8 / 4 * 3 + 3);
            okC = Base64::convertFromBase64 (out, b64);
        }
        const double tC = nowMs() - t0;

        // proxy for the WKWebView bridge: JSON::fromString of the invoke message
        double tJ = -1.0;
        {
            String msg;
            msg.preallocateBytes ((size_t) bytesUtf8 + 200);
            msg << "{\"eventId\":\"__juce__invoke\",\"payload\":{\"name\":\"importDroppedAudio\",\"params\":[\"drop.wav\",\"" << b64 << "\"],\"resultId\":7}}";
            t0 = nowMs();
            const var v = JSON::fromString (msg);
            tJ = nowMs() - t0;
            if (! v.isObject()) tJ = -2.0;
        }

        const bool ok = okA && okB && okC && a == src && b == src && c == src;
        std::printf ("%zu,%d,%.2f,%.1f,%.1f,%.1f,%.1f,%s\n", mib, b64.length(), tLen, tA, tB, tC, tJ, ok ? "yes" : "NO");
        std::fflush (stdout);
    }
    return 0;
}
