// O-TextureForge alias gate harness (v1.7.0): pitched-up grains read the grain-engine
// band-limited mipmap instead of the linear interpolator. Built twice by build.sh:
// tfa_new against the working tree (-DNEW_ENGINE, builds the mipmap exactly as the
// corpus loader does) and tfa_old against the v1.6.2 backup (linear only).
//
// usage:
//   tfa grain <f0 Hz> <midiNote> <out.f32>
//       10 s sine (0.5 peak) at 48 kHz, one grain at 1 s; Pitch-Mapped mode plays one
//       note, so the grain reads at rate 2^((note - 60) / 12). 60 ms pure Hann grain
//       (crossfade 0), rendered alone. Writes channel 0 (1 s).
//   tfa far <offsetSamples> <midiNote>
//       periodic corpus (period 147): the same grain near the start and about
//       <offset> later; prints the worst residual and the peak in dBFS (position
//       precision of the band-limited read deep in a long corpus). Both starts are
//       multiples of 147 * 64, so every mip level (decimated by up to 2^6) sees the
//       far grain on the same sample grid as the near one: any residual is read
//       precision, not the interpolator's grid dependence.
#include <JuceHeader.h>
#include "SharedCorpus.h"
#include "KDTreeSearch.h"
#include "GrainScheduler.h"
#include <cstdio>
#include <string>
#include <vector>

static constexpr double kFs = 48000.0;

static std::vector<float> renderGrain(SharedCorpus& c, int note, int len, float sizeMs)
{
#ifdef NEW_ENGINE
    c.buildBandlimited();
#endif
    SchedulerParams p;
    p.midiMode = 0;
    p.variation = 0.0f;
    p.grainSizeMs = sizeMs;
    p.crossfadePercent = 0.0f;
    p.outputGainDb = 0.0f;

    GrainScheduler sched;
    sched.prepare(kFs, p.outputGainDb);
    sched.reset();

    std::vector<float> out;
    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < len; pos += 512)
    {
        const int n = std::min(512, len - pos);
        buf.setSize(2, n, false, false, true);
        midi.clear();
        if (pos == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) 127), 0);
        sched.processBlock(buf, midi, &c, p);
        out.insert(out.end(), buf.getReadPointer(0), buf.getReadPointer(0) + n);
    }
    return out;
}

static void oneGrain(SharedCorpus& c, uint32_t start)
{
    GrainMetadata g {};
    g.startSample = start;
    g.lengthSamples = 2048;
    c.grains.push_back(g);
    c.sampleRate = kFs;
}

int main(int argc, char** argv)
{
    if (argc < 4) { std::fprintf(stderr, "bad args\n"); return 2; }
    const std::string cmd = argv[1];

    if (cmd == "grain" && argc >= 5)
    {
        const double f0 = std::atof(argv[2]);
        const int note = std::atoi(argv[3]);
        SharedCorpus c;
        const int n = (int) (kFs * 10.0);
        c.audioData.setSize(1, n);
        auto* d = c.audioData.getWritePointer(0);
        for (int i = 0; i < n; ++i)
            d[i] = (float) (0.5 * std::sin(2.0 * M_PI * f0 * (double) i / kFs));
        oneGrain(c, (uint32_t) kFs);
        const auto y = renderGrain(c, note, (int) kFs, 60.0f);
        FILE* f = std::fopen(argv[4], "wb");
        std::fwrite(y.data(), sizeof(float), y.size(), f);
        std::fclose(f);
        return 0;
    }

    if (cmd == "far")
    {
        constexpr int kP = 147;
        const long long offset = std::atoll(argv[2]);
        const int note = std::atoi(argv[3]);
        constexpr long long kAlign = kP * 64;
        const uint32_t start0 = (uint32_t) (2 * kAlign);
        const uint32_t start1 = (uint32_t) ((offset / kAlign) * kAlign);
        const int n = (int) start1 + 450000;   // a 180 ms grain at note 127 reads ~414 k
        std::vector<float> r[2];
        for (int k = 0; k < 2; ++k)
        {
            SharedCorpus c;
            c.audioData.setSize(1, n);
            auto* d = c.audioData.getWritePointer(0);
            for (int i = 0; i < n; ++i)
            {
                const double ph = 2.0 * M_PI * (double) (i % kP) / kP;
                d[i] = (float) (0.4 * std::sin(ph) + 0.25 * std::sin(3.0 * ph + 0.3) + 0.15 * std::sin(7.0 * ph + 1.1));
            }
            oneGrain(c, k == 0 ? start0 : start1);
            r[k] = renderGrain(c, note, 10000, 180.0f);
        }
        double worst = 0.0, peak = 0.0;
        for (size_t i = 0; i < r[0].size(); ++i)
        {
            worst = std::max(worst, std::abs((double) r[0][i] - (double) r[1][i]));
            peak = std::max(peak, std::abs((double) r[0][i]));
        }
        auto db = [](double x) { return x > 0.0 ? 20.0 * std::log10(x) : -999.0; };
        std::printf("%.1f %.1f\n", db(worst), db(peak));
        return 0;
    }
    std::fprintf(stderr, "bad args\n");
    return 2;
}

namespace juce { extern const char* const juce_compilationDate; extern const char* const juce_compilationTime; const char* const juce_compilationDate = __DATE__; const char* const juce_compilationTime = __TIME__; }
