// v1.6.1 gate (c): retired-corpus reaping. An audio thread runs the real
// TextureForgeProcessor::processBlock (drone mode) nonstop while this thread
// publishes pre-built corpora as fast as it can.
//
// No ASan (its runtime hangs before main on macOS 26 / Xcode 26.3). Instead every
// corpus is owned through a deleter that POISONS it (audio -> 1e30) and parks it in
// a quarantine rather than freeing it. A block that reads a "freed" corpus then hits
// the processor's +-1 clip; the real signal peaks far below that.
//
//   reaper_stress new   — publishCorpus() (generation + entry-counter reap). 0 poisoned blocks.
//   reaper_stress old   — the v1.6.0 single retired slot, replayed here. Must show
//                         poisoned blocks, else the gate is vacuous.
//
// Then, in "new": with the audio thread stopped (host idling), 50 more publishes
// must keep at most one parked corpus, and a final reap must leave only the
// current corpus alive (live-instance count).
#include <JuceHeader.h>
#include <atomic>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include "KDTreeSearch.h"
#define private public
#include "PluginProcessor.h"
#undef private

static std::atomic<int> liveCorpora { 0 };
static std::mutex quarantineLock;
static std::vector<std::unique_ptr<SharedCorpus>> quarantine;

static std::shared_ptr<SharedCorpus> makeCorpus()
{
    auto* c = new SharedCorpus();
    const int n = 8000;
    c->audioData.setSize(1, n);
    auto* d = c->audioData.getWritePointer(0);
    for (int i = 0; i < n; ++i)
        d[i] = 0.2f * std::sin(0.05f * (float) i);
    for (uint32_t s = 0; s + 2048 <= (uint32_t) n; s += 1024)
    {
        GrainMetadata g {};
        g.startSample = s;
        g.lengthSamples = 2048;
        c->grains.push_back(g);
    }
    c->sampleRate = 44100.0;
    liveCorpora.fetch_add(1);
    return std::shared_ptr<SharedCorpus>(c, [](SharedCorpus* dead)
    {
        juce::FloatVectorOperations::fill(dead->audioData.getWritePointer(0), 1.0e30f, dead->audioData.getNumSamples());
        liveCorpora.fetch_sub(1);
        const std::lock_guard<std::mutex> lock(quarantineLock);
        quarantine.emplace_back(dead);
    });
}

int main(int argc, char** argv)
{
    const bool useNew = argc > 1 && std::string(argv[1]) == "new";
    juce::ScopedJuceInitialiser_GUI juceInit;

    auto proc = std::make_unique<TextureForgeProcessor>();
    proc->prepareToPlay(44100.0, 2048);
    *proc->parameters.getRawParameterValue("GRAIN_DENSITY") = 64.0f;   // a spawn every 20 ms
    *proc->parameters.getRawParameterValue("GRAIN_SIZE") = 500.0f;

    constexpr int kPublishes = 6000;
    std::vector<std::shared_ptr<SharedCorpus>> pending;
    for (int k = 0; k < kPublishes + 50; ++k)
        pending.push_back(makeCorpus());

    std::atomic<bool> stop { false };
    std::atomic<uint64_t> blocks { 0 }, poisoned { 0 };
    std::thread audio([&]
    {
        juce::AudioBuffer<float> buf(2, 2048);
        juce::MidiBuffer midi;
        while (!stop.load())
        {
            proc->processBlock(buf, midi);
            const auto r = buf.findMinMax(0, 0, buf.getNumSamples());
            if (r.getStart() <= -1.0f || r.getEnd() >= 1.0f)
                poisoned.fetch_add(1);
            blocks.fetch_add(1);
        }
    });

    std::mt19937 rng(1);
    std::shared_ptr<SharedCorpus> oldRetiredSlot;
    size_t maxParked = 0;
    for (int k = 0; k < kPublishes; ++k)
    {
        auto c = std::move(pending[(size_t) k]);
        if (useNew)
        {
            proc->publishCorpus(std::move(c));
            maxParked = std::max(maxParked, proc->retiredCorpora.size());
        }
        else
        {
            oldRetiredSlot = proc->currentCorpus;   // v1.6.0 completion callback, verbatim
            proc->currentCorpus = c;
            proc->corpusForAudio.store(proc->currentCorpus.get(), std::memory_order_release);
        }
        if (rng() % 2 == 0)
            std::this_thread::sleep_for(std::chrono::microseconds(rng() % 200));
    }
    stop.store(true);
    audio.join();

    int fails = 0;
    std::printf("publishes %d, audio blocks %llu, poisoned blocks %llu, max parked %zu\n", kPublishes,
                (unsigned long long) blocks.load(), (unsigned long long) poisoned.load(), maxParked);
    if (useNew)
    {
        // maxParked is report-only: it counts publishes within the last two blocks,
        // and this loop publishes ~1.4x per block (a real load takes seconds).
        if (poisoned.load() != 0) ++fails;

        size_t idleMax = 0;
        for (int k = kPublishes; k < kPublishes + 50; ++k)
        {
            proc->publishCorpus(std::move(pending[(size_t) k]));
            idleMax = std::max(idleMax, proc->retiredCorpora.size());
        }
        proc->reapRetiredCorpora();
        pending.clear();
        std::printf("idle host: max parked %zu over 50 publishes, %zu parked and %d corpora alive after reap\n",
                    idleMax, proc->retiredCorpora.size(), liveCorpora.load());
        if (idleMax > 1 || !proc->retiredCorpora.empty() || liveCorpora.load() != 1) ++fails;
    }
    else if (poisoned.load() == 0)
    {
        std::printf("control did not reproduce the v1.6.0 race\n");
        ++fails;
    }
    pending.clear();
    oldRetiredSlot.reset();
    proc.reset();
    quarantine.clear();   // before JUCE's leak detector runs
    std::printf(fails ? "FAIL\n" : "OK\n");
    return fails;
}

// UMAP is never reached (no corpus is loaded through CorpusLoader here); stubbed
// to keep umappp/Eigen out of the build.
void initEigenOnMainThread() {}
void UMAPProjection::compute(const std::vector<GrainMetadata>&, std::vector<float>&, std::vector<float>&,
                             ProgressCallback, std::function<bool()>) {}
