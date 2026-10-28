// O-TextureForge scheduler harness: renders GrainScheduler on a synthetic corpus
// with an absolute-time event list, chopped into blocks of a chosen pattern.
// Built twice: against the working tree (-DNEW_ENGINE) and against the v1.5.4 backup.
//
// usage: tfh <mode 0|1|2> <density> <variation> <seed> <block 32|512|441|var> <align free|512>
//            <path kd|proj|rand> <out.f32> [processorTail 0|1]
// env (v1.7.0): TF_BLR=1 builds the corpus mipmap (new engine only), as the loader
// does; TF_NOTE_MAX=<n> clamps every note to n (both engines), e.g. 60 = rate <= 1.
// env (v1.8.0, new engine only): TF_W="t,b,x,e" weight multipliers (timbre,
// brightness, texture, energy; 1 = shipped) -> a SearchIndex built as the
// processor's worker does (TF_W_FORCE=1 builds one even at all-1); TF_SPREAD,
// TF_AVOID in 0..1; TF_ENERGY/TF_BRIGHT/TF_TEXTURE override the macro knobs.
// TF_PICKS=<file> (both engines) logs each new grain as "sample index e b x"
// (its energy/brightness/texture descriptors), detected through getActiveGrains
// after every block — use block 32 (a spawn interval is >= 100 samples).
// HAS_SEED (compile flag) sets rngSeed in the old build too (baselines >= 1.6.0).
#include <JuceHeader.h>
#include "SharedCorpus.h"
#include "KDTreeSearch.h"
#include "GrainScheduler.h"
#include <cstdio>
#include <vector>

// v1.8.0 alloc probe: every operator new while gCountAllocs is set is counted
// (set around processBlock after the first block). TF_ALLOC=1 prints the count.
#include <atomic>
#include <new>
static std::atomic<bool> gCountAllocs { false };
static std::atomic<long> gAllocs { 0 };
void* operator new(std::size_t n) { if (gCountAllocs.load(std::memory_order_relaxed)) gAllocs.fetch_add(1); if (void* q = std::malloc(n ? n : 1)) return q; throw std::bad_alloc(); }
void* operator new[](std::size_t n) { if (gCountAllocs.load(std::memory_order_relaxed)) gAllocs.fetch_add(1); if (void* q = std::malloc(n ? n : 1)) return q; throw std::bad_alloc(); }
void operator delete(void* q) noexcept { std::free(q); }
void operator delete[](void* q) noexcept { std::free(q); }
void operator delete(void* q, std::size_t) noexcept { std::free(q); }
void operator delete[](void* q, std::size_t) noexcept { std::free(q); }

static constexpr double kFs = 44100.0;
static constexpr int kLen = 441000;   // 10 s

struct Lcg { uint64_t s; uint32_t next() { s = s * 6364136223846793005ull + 1442695040888963407ull; return (uint32_t) (s >> 33); }
             float uni() { return (float) next() / 2147483648.0f; } };

static std::shared_ptr<SharedCorpus> makeCorpus(const std::string& path)
{
    auto c = std::make_shared<SharedCorpus>();
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
#ifdef NEW_ENGINE
    if (const char* e = std::getenv("TF_BLR"); e != nullptr && std::atoi(e) != 0)
        c->buildBandlimited();
#endif
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
    if (const char* e = std::getenv("TF_NOTE_MAX"))
    {
        const int cap = std::atoi(e);
        for (auto& x : ev)
            if (x.msg.isNoteOnOrOff() && x.msg.getNoteNumber() > cap)
                x.msg.setNoteNumber(cap);
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
    if (std::getenv("TF_NO_EVENTS") != nullptr)   // v1.8.0 pick probes: a fixed target (no drone CCs)
        events.clear();

    SchedulerParams p;
    p.midiMode = mode;
    p.grainDensity = density;
    p.variation = variation;
    p.grainSizeMs = 180.0f;
    p.crossfadePercent = 40.0f;
    p.scatterX = 0.3f; p.scatterY = 0.6f; p.position = 0.4f;
#if defined(NEW_ENGINE) || defined(HAS_SEED)
    p.rngSeed = seed;
#else
    juce::ignoreUnused(seed);
#endif
    auto envf = [](const char* k, float def) { const char* e = std::getenv(k); return e ? (float) std::atof(e) : def; };
    p.energy = envf("TF_ENERGY", p.energy);
    p.brightness = envf("TF_BRIGHT", p.brightness);
    p.texture = envf("TF_TEXTURE", p.texture);
#ifdef NEW_ENGINE
    std::shared_ptr<SearchIndex> index;
    if (const char* w = std::getenv("TF_W"))
    {
        WeightMults m {};
        std::sscanf(w, "%f,%f,%f,%f", &m[0], &m[1], &m[2], &m[3]);
        const char* force = std::getenv("TF_W_FORCE");
        if (!SearchIndex::isDefault(m) || (force != nullptr && std::atoi(force) != 0))
            index = SearchIndex::build(corpus, m, 1);
    }
    p.searchIndex = index.get();
    p.pickSpread = envf("TF_SPREAD", 0.0f);
    p.repeatAvoid = envf("TF_AVOID", 0.0f);
#endif
    FILE* picks = std::getenv("TF_PICKS") ? std::fopen(std::getenv("TF_PICKS"), "w") : nullptr;
    const int grainLen = (int) (p.grainSizeMs * (float) kFs / 1000.0f);

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
        gCountAllocs.store(pos > 0, std::memory_order_relaxed);
        sched.processBlock(buf, midi, corpus.get(), p);
        gCountAllocs.store(false, std::memory_order_relaxed);
        if (picks != nullptr)
        {
            GrainScheduler::ActiveGrainInfo ag[64];
            int cnt = 0;
            sched.getActiveGrains(ag, cnt);
            for (int k = 0; k < cnt; ++k)
            {
                const auto& g = corpus->grains[ag[k].grainIndex];
                const int len = (int) std::min<uint32_t>((uint32_t) grainLen, (uint32_t) corpus->audioData.getNumSamples() - g.startSample);
                if (ag[k].readPositionNorm * (float) len <= (float) n + 0.5f)
                    std::fprintf(picks, "%d %u %.6f %.6f %.6f\n", pos, ag[k].grainIndex,
                                 g.descriptors[ENERGY_DIM], g.descriptors[BRIGHTNESS_DIM], g.descriptors[TEXTURE_DIM]);
            }
        }
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
    if (picks != nullptr)
        std::fclose(picks);
    if (const char* e = std::getenv("TF_ALLOC"); e != nullptr && std::atoi(e) != 0)
        std::fprintf(stderr, "allocs in processBlock: %ld\n", gAllocs.load());
    FILE* f = std::fopen(argv[8], "wb");
    std::fwrite(out.data(), sizeof(float), out.size(), f);
    std::fclose(f);
    return 0;
}

namespace juce { extern const char* const juce_compilationDate; extern const char* const juce_compilationTime; const char* const juce_compilationDate = __DATE__; const char* const juce_compilationTime = __TIME__; }
