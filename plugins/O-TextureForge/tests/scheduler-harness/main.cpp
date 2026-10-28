// O-TextureForge scheduler harness: renders GrainScheduler on a synthetic corpus
// with an absolute-time event list, chopped into blocks of a chosen pattern.
// Built twice: against the working tree (-DNEW_ENGINE) and against the v1.5.4 backup.
//
// usage: tfh <mode 0|1|2> <density> <variation> <seed> <block 32|512|441|var> <align free|512>
//            <path kd|proj|rand> <out.f32> [processorTail 0|1]
#include <JuceHeader.h>
#include "SharedCorpus.h"
#include "KDTreeSearch.h"
#include "GrainScheduler.h"
#include <cstdio>
#include <vector>

static constexpr double kFs = 44100.0;
static constexpr int kLen = 441000;   // 10 s

struct Lcg { uint64_t s; uint32_t next() { s = s * 6364136223846793005ull + 1442695040888963407ull; return (uint32_t) (s >> 33); }
             float uni() { return (float) next() / 2147483648.0f; } };

static std::unique_ptr<SharedCorpus> makeCorpus(const std::string& path)
{
    auto c = std::make_unique<SharedCorpus>();
    const int n = (int) (kFs * 12.0);
    c->audioData.setSize(1, n);
    auto* d = c->audioData.getWritePointer(0);
    Lcg r { 12345 };
    for (int i = 0; i < n; ++i)
    {
        const double t = i / kFs;
        d[i] = (float) (0.3 * std::sin(2 * M_PI * 220.0 * t) * (0.5 + 0.5 * std::sin(2 * M_PI * 0.7 * t))
                      + 0.2 * std::sin(2 * M_PI * (300.0 + 200.0 * std::sin(0.3 * t)) * t)
                      + 0.15 * (r.uni() * 2.0f - 1.0f));
    }
    for (uint32_t s = 0; s + 2048 <= (uint32_t) n; s += 1024)
    {
        GrainMetadata g {};
        g.startSample = s; g.lengthSamples = 2048;
        for (auto& v : g.descriptors) v = r.uni() * 3.0f - 1.5f;
        c->grains.push_back(g);
    }
    if (path != "rand")
        for (size_t i = 0; i < c->grains.size(); ++i) { c->pcaX.push_back(r.uni()); c->pcaY.push_back(r.uni()); }
    if (path == "kd")
        c->kdTree = std::make_unique<KDTreeSearch>(c->grains);
    c->sampleRate = kFs;
    return c;
}

struct Ev { int pos; juce::MidiMessage msg; };

static std::vector<Ev> makeEvents(int mode, int align)
{
    std::vector<Ev> ev;
    Lcg r { 777 };
    auto at = [&](int p) { return align ? (p / align) * align : p; };
    if (mode == 2)
    {
        for (int k = 0; k < 40; ++k)
        {
            const int cc = k % 3 == 0 ? 1 : k % 3 == 1 ? 11 : 74;
            ev.push_back({ at((int) (r.next() % kLen)), juce::MidiMessage::controllerEvent(1, cc, (int) (r.next() % 128)) });
        }
    }
    else
    {
        for (int k = 0; k < 220; ++k)
        {
            const int on = (int) (r.next() % (kLen - 30000));
            const int note = 40 + (int) (r.next() % 50);
            const int dur = 500 + (int) (r.next() % 20000);
            ev.push_back({ at(on), juce::MidiMessage::noteOn(1, note, (juce::uint8) (20 + r.next() % 107)) });
            ev.push_back({ at(on + dur), juce::MidiMessage::noteOff(1, note) });
        }
        // a 70-note cluster forces steals (64 voices)
        const int cl = at(200000 + 37);
        for (int k = 0; k < 70; ++k)
            ev.push_back({ cl, juce::MidiMessage::noteOn(1, 30 + k, (juce::uint8) 100) });
        for (int k = 0; k < 30; ++k)
        {
            ev.push_back({ at((int) (r.next() % kLen)), juce::MidiMessage::controllerEvent(1, 1, (int) (r.next() % 128)) });
            ev.push_back({ at((int) (r.next() % kLen)), juce::MidiMessage::channelPressureChange(1, (int) (r.next() % 128)) });
        }
    }
    std::stable_sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.pos < b.pos; });
    return ev;
}

int main(int argc, char** argv)
{
    if (argc < 9) { std::fprintf(stderr, "bad args\n"); return 2; }
    const int mode = std::atoi(argv[1]);
    const int density = std::atoi(argv[2]);
    const float variation = (float) std::atof(argv[3]);
    const uint64_t seed = std::strtoull(argv[4], nullptr, 10);
    const std::string block = argv[5];
    const int align = std::atoi(argv[6]);   // 0 = free, else snap events to this grid
    const std::string path = argv[7];
    const bool tail = argc > 9 && std::atoi(argv[9]) != 0;

    auto corpus = makeCorpus(path);
    auto events = makeEvents(mode, align);

    SchedulerParams p;
    p.midiMode = mode;
    p.grainDensity = density;
    p.variation = variation;
    p.grainSizeMs = 180.0f;
    p.crossfadePercent = 40.0f;
    p.scatterX = 0.3f; p.scatterY = 0.6f; p.position = 0.4f;
#ifdef NEW_ENGINE
    p.rngSeed = seed;
#else
    juce::ignoreUnused(seed);
#endif

    GrainScheduler sched;                       // fresh instance per render
    sched.prepare(kFs, p.outputGainDb);
    sched.reset();

    std::vector<float> out((size_t) kLen * 2);
    Lcg br { 4242 };
    size_t ei = 0;
    juce::AudioBuffer<float> buf(2, 1024);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < kLen;)
    {
        int n = block == "var" ? 1 + (int) (br.next() % 1024) : std::atoi(block.c_str());
        if (br.next() % 13 == 0 && block == "var") n = 1;
        n = std::min(n, kLen - pos);
        buf.setSize(2, n, false, false, true);
        midi.clear();
        while (ei < events.size() && events[ei].pos < pos + n)
        {
            midi.addEvent(events[ei].msg, events[ei].pos - pos);
            ++ei;
        }
        sched.processBlock(buf, midi, corpus.get(), p);
        if (tail)
            for (int ch = 0; ch < 2; ++ch)
                juce::FloatVectorOperations::clip(buf.getWritePointer(ch), buf.getReadPointer(ch), -1.0f, 1.0f, n);
        for (int i = 0; i < n; ++i)
        {
            out[(size_t) (pos + i)] = buf.getSample(0, i);
            out[(size_t) (kLen + pos + i)] = buf.getSample(1, i);
        }
        pos += n;
    }
    FILE* f = std::fopen(argv[8], "wb");
    std::fwrite(out.data(), sizeof(float), out.size(), f);
    std::fclose(f);
    return 0;
}

namespace juce { extern const char* const juce_compilationDate; extern const char* const juce_compilationTime; const char* const juce_compilationDate = __DATE__; const char* const juce_compilationTime = __TIME__; }
