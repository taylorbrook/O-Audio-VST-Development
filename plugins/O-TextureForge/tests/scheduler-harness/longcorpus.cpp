// v1.6.1 gate (a): read-position precision. The corpus is exactly periodic (period
// P samples). Two single-grain corpora share that audio: one grain near the start,
// one a whole number of periods later, at <offset>. A pitched Mode 0 note plays
// each, so the two renders should match. A third number compares the far grain
// with a reference built from the exact position start + n * rate in double. Built against the working tree (double positions) and a
// baseline (float positions). Prints the worst residual in dBFS and the peak.
//
// usage: tfl <offsetSamples> <midiNote>     prints: residual peak refResidual (dBFS)
#include <JuceHeader.h>
#include "SharedCorpus.h"
#include "KDTreeSearch.h"
#include "GrainScheduler.h"
#include <cstdio>
#include <vector>

static constexpr double kFs = 44100.0;
static constexpr int kP = 147;          // 300 Hz
static constexpr int kRender = 10000;   // > one 180 ms grain

static std::vector<float> renderGrain(const SharedCorpus& c, int note)
{
    SchedulerParams p;
    p.midiMode = 0;
    p.variation = 0.0f;
    p.grainSizeMs = 180.0f;
    p.crossfadePercent = 40.0f;
    p.outputGainDb = 0.0f;

    GrainScheduler sched;
    sched.prepare(kFs, p.outputGainDb);
    sched.reset();

    std::vector<float> out;
    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < kRender; pos += 512)
    {
        const int n = std::min(512, kRender - pos);
        buf.setSize(2, n, false, false, true);
        midi.clear();
        if (pos == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) 100), 0);
        sched.processBlock(buf, midi, &c, p);
        out.insert(out.end(), buf.getReadPointer(0), buf.getReadPointer(0) + n);
    }
    return out;
}

int main(int argc, char** argv)
{
    if (argc < 3) { std::fprintf(stderr, "bad args\n"); return 2; }
    const long long offset = std::atoll(argv[1]);
    const int note = std::atoi(argv[2]);

    const uint32_t start0 = kP * 10;
    const uint32_t start1 = (uint32_t) ((offset / kP) * kP);
    const int n = (int) start1 + 60000;   // room for a 180 ms grain at rate 5.3

    // One grain per corpus, so the "rand" path has nothing else to pick.
    SharedCorpus c[2];
    for (int k = 0; k < 2; ++k)
    {
        c[k].audioData.setSize(1, n);
        auto* d = c[k].audioData.getWritePointer(0);
        for (int i = 0; i < n; ++i)
        {
            const double ph = 2.0 * M_PI * (double) (i % kP) / kP;
            d[i] = (float) (0.4 * std::sin(ph) + 0.25 * std::sin(3.0 * ph + 0.3) + 0.15 * std::sin(7.0 * ph + 1.1));
        }
        GrainMetadata g {};
        g.startSample = k == 0 ? start0 : start1;
        g.lengthSamples = 2048;
        c[k].grains.push_back(g);
        c[k].sampleRate = kFs;
    }

    const auto a = renderGrain(c[0], note);
    const auto b = renderGrain(c[1], note);
    // Reference: the scheduler's float envelope and gain, exact double position.
    const float rate = std::pow(2.0f, static_cast<float>(note - 60) / 12.0f);
    const uint32_t len = static_cast<uint32_t>(180.0f * static_cast<float>(kFs) / 1000.0f);
    const float gain = juce::MidiMessage::noteOn(1, note, (juce::uint8) 100).getFloatVelocity();
    const float* d = c[1].audioData.getReadPointer(0);
    std::vector<float> ref(b.size(), 0.0f);
    for (uint32_t i = 0; i < len && i < ref.size(); ++i)
    {
        const double pos = (double) start1 + (double) i * (double) rate;
        const int i0 = (int) pos;
        const float frac = (float) (pos - (double) i0);
        const float smp = d[i0] + frac * (d[i0 + 1] - d[i0]);
        ref[i] = smp * GrainVoice::envelopeAt((float) i / (float) len, 0.4f) * gain;
    }

    double worst = 0.0, peak = 0.0, refWorst = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
    {
        worst = std::max(worst, std::abs((double) a[i] - (double) b[i]));
        peak = std::max(peak, std::abs((double) a[i]));
        refWorst = std::max(refWorst, std::abs((double) b[i] - (double) ref[i]));
    }
    auto db = [](double x) { return x > 0.0 ? 20.0 * std::log10(x) : -999.0; };
    std::printf("%.1f %.1f %.1f\n", db(worst), db(peak), db(refWorst));
    return 0;
}

namespace juce { extern const char* const juce_compilationDate; extern const char* const juce_compilationTime; const char* const juce_compilationDate = __DATE__; const char* const juce_compilationTime = __TIME__; }
