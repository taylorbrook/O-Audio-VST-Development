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
// v1.8.0 gate: the steered SearchIndex's publish / park / reap, at processor level.
//
// A. Race. An audio thread runs the real processBlock (drone, density 64, Pick
//    Spread 50 %, so every block searches and the candidate cache is keyed on the
//    index generation) while this thread publishes pre-built indices for the
//    current corpus as fast as it can. No ASan (it hangs on macOS 26): an index is
//    owned through a deleter that stamps generation = kDead and quarantines it
//    instead of freeing it. A block that searched a "freed" index leaves
//    cache.indexGeneration == kDead, which the audio thread checks after every
//    block. The deleter also records whether it ran on the audio thread.
//      index_stress new — publishSearchIndex(): must show 0 dead reads, 0 audio-thread frees
//      index_stress old — a publish that drops the outgoing index at once (no
//                         parking): must show dead reads, else the gate is vacuous
// B. Worker path (new only), with the message loop pumped: a weight change gets a
//    matching index published; 200 rapid weight changes under audio settle on the
//    last value; a new corpus gets its own index; all-100 % unpublishes it and the
//    reap leaves no index alive.
#include <JuceHeader.h>
#include <atomic>
#include <chrono>
#include <cstdio>
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

static constexpr uint64_t kDead = 0xDEADDEADDEADull;
static std::atomic<int> liveIndices { 0 };
static std::atomic<int> audioThreadFrees { 0 };
static std::atomic<std::thread::id> audioThreadId {};
static std::mutex quarantineLock;
static std::vector<std::unique_ptr<SearchIndex>> quarantine;

static std::shared_ptr<SharedCorpus> makeCorpus(uint32_t seed)
{
    auto c = std::make_shared<SharedCorpus>();
    const int n = 44100 * 3;
    c->audioData.setSize(1, n);
    auto* d = c->audioData.getWritePointer(0);
    for (int i = 0; i < n; ++i)
        d[i] = 0.2f * std::sin(0.03f * (float) i);
    std::mt19937 r(seed);
    std::uniform_real_distribution<float> u(-1.5f, 1.5f);
    for (uint32_t s = 0; s + 2048 <= (uint32_t) n; s += 512)
    {
        GrainMetadata g {};
        g.startSample = s;
        g.lengthSamples = 2048;
        for (auto& v : g.descriptors) v = u(r);
        c->grains.push_back(g);
    }
    c->kdTree = std::make_unique<KDTreeSearch>(c->grains);
    c->sampleRate = 44100.0;
    return c;
}

static std::shared_ptr<SearchIndex> makeIndex(std::shared_ptr<SharedCorpus> corpus, float energyMult, uint64_t gen)
{
    WeightMults m { 1.0f, 1.0f, 1.0f, energyMult };
    auto built = SearchIndex::build(std::move(corpus), m, gen);
    auto* raw = new SearchIndex();
    raw->corpus = built->corpus; raw->mults = built->mults; raw->tree = std::move(built->tree); raw->generation = gen;
    liveIndices.fetch_add(1);
    return std::shared_ptr<SearchIndex>(raw, [](SearchIndex* dead)
    {
        if (std::this_thread::get_id() == audioThreadId.load())
            audioThreadFrees.fetch_add(1);
        dead->generation = kDead;   // tree and corpus stay valid: a late read is detected, not a crash
        liveIndices.fetch_sub(1);
        const std::lock_guard<std::mutex> lock(quarantineLock);
        quarantine.emplace_back(dead);
    });
}

static void pump(int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil(ms); }

int main(int argc, char** argv)
{
    const bool useNew = argc > 1 && std::string(argv[1]) == "new";
    juce::ScopedJuceInitialiser_GUI juceInit;
    int fails = 0;

    auto proc = std::make_unique<TextureForgeProcessor>();
    proc->stopTimer();   // A drives publishes itself; B restarts it
    proc->prepareToPlay(44100.0, 2048);
    *proc->parameters.getRawParameterValue("GRAIN_DENSITY") = 64.0f;
    *proc->parameters.getRawParameterValue("GRAIN_SIZE") = 100.0f;
    *proc->parameters.getRawParameterValue("PICK_SPREAD") = 50.0f;
    *proc->parameters.getRawParameterValue("REPEAT_AVOID") = 50.0f;

    auto corpus = makeCorpus(1);
    proc->publishCorpus(corpus);

    constexpr int kPublishes = 6000;
    std::vector<std::shared_ptr<SearchIndex>> pending;
    for (int k = 0; k < kPublishes; ++k)
        pending.push_back(makeIndex(corpus, 0.5f + (float) (k % 7), 1000 + (uint64_t) k));

    std::atomic<bool> stop { false };
    std::atomic<uint64_t> blocks { 0 }, deadReads { 0 }, steeredBlocks { 0 };
    std::thread audio([&]
    {
        audioThreadId.store(std::this_thread::get_id());
        juce::AudioBuffer<float> buf(2, 2048);
        juce::MidiBuffer midi;
        while (!stop.load())
        {
            proc->processBlock(buf, midi);
            const uint64_t g = proc->grainScheduler.cache.indexGeneration;
            if (g == kDead) deadReads.fetch_add(1);
            if (g >= 1000) steeredBlocks.fetch_add(1);
            blocks.fetch_add(1);
        }
    });

    std::mt19937 rng(3);
    std::shared_ptr<SearchIndex> oldHeld;
    for (int k = 0; k < kPublishes; ++k)
    {
        auto idx = std::move(pending[(size_t) k]);
        if (useNew)
            proc->publishSearchIndex(std::move(idx));
        else
        {
            oldHeld = proc->currentIndex;            // dropped at the next publish: no parking
            proc->currentIndex = std::move(idx);
            proc->indexForAudio.store(proc->currentIndex.get(), std::memory_order_seq_cst);
            oldHeld.reset();
        }
        if (rng() % 2 == 0)
            std::this_thread::sleep_for(std::chrono::microseconds(rng() % 300));
    }
    stop.store(true);
    audio.join();
    std::printf("A: publishes %d, audio blocks %llu (steered %llu), dead-index reads %llu, audio-thread frees %d\n",
                kPublishes, (unsigned long long) blocks.load(), (unsigned long long) steeredBlocks.load(),
                (unsigned long long) deadReads.load(), audioThreadFrees.load());

    if (!useNew)
    {
        if (deadReads.load() == 0) { std::printf("control did not reproduce the unparked-publish race\n"); ++fails; }
    }
    else
    {
        if (deadReads.load() != 0 || audioThreadFrees.load() != 0 || steeredBlocks.load() == 0) ++fails;
        proc->reapRetiredCorpora();
        std::printf("A: after reap %zu parked, %d indices alive (want 0 parked, 1 alive)\n",
                    proc->retiredCorpora.size(), liveIndices.load());
        if (!proc->retiredCorpora.empty() || liveIndices.load() != 1) ++fails;

        // ---- B. the timer / worker path ----
        proc->publishSearchIndex(nullptr);
        proc->reapRetiredCorpora();
        proc->startTimerHz(20);
        auto* we = proc->parameters.getRawParameterValue("WEIGHT_ENERGY");
        auto matches = [&](float mult)
        {
            auto* i = proc->indexForAudio.load();
            return i != nullptr && proc->currentIndex.get() == i && i->corpus == proc->getCurrentCorpus()
                && i->mults[kWeightEnergy] == mult && i->tree != nullptr;
        };
        auto waitFor = [&](auto pred, int maxMs)
        {
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            while (!pred() && juce::Time::getMillisecondCounterHiRes() - t0 < maxMs) pump(5);
            return juce::Time::getMillisecondCounterHiRes() - t0;
        };

        *we = 200.0f;
        const double t1 = waitFor([&] { return matches(2.0f); }, 3000);
        std::printf("B1: Energy Weight 200 %% -> matching index in %.0f ms\n", t1);
        if (!matches(2.0f)) ++fails;

        stop.store(false);
        std::thread audio2([&]
        {
            juce::AudioBuffer<float> buf(2, 512);
            juce::MidiBuffer midi;
            while (!stop.load()) proc->processBlock(buf, midi);
        });
        for (int k = 0; k < 200; ++k)
        {
            *we = (float) (10 + (k * 37) % 390);
            pump(3);
        }
        *we = 321.0f;
        const double t2 = waitFor([&] { return matches(3.21f); }, 3000);
        std::printf("B2: 200 rapid changes under audio, settled on 321 %% in %.0f ms\n", t2);
        if (!matches(3.21f)) ++fails;

        auto corpus2 = makeCorpus(2);
        proc->publishCorpus(corpus2);
        const double t3 = waitFor([&] { return matches(3.21f) && proc->currentIndex->corpus == corpus2; }, 3000);
        std::printf("B3: new corpus -> its own index in %.0f ms\n", t3);
        if (!(matches(3.21f) && proc->currentIndex->corpus == corpus2)) ++fails;

        *we = 100.0f;
        waitFor([&] { return proc->indexForAudio.load() == nullptr; }, 3000);
        stop.store(true);
        audio2.join();
        pump(100);
        proc->reapRetiredCorpora();
        const bool cleared = proc->indexForAudio.load() == nullptr && proc->currentIndex == nullptr;
        std::printf("B4: all 100 %% -> index %s, %zu parked after reap\n", cleared ? "unpublished" : "STILL PUBLISHED",
                    proc->retiredCorpora.size());
        if (!cleared || !proc->retiredCorpora.empty()) ++fails;
    }

    pending.clear();
    oldHeld.reset();
    proc.reset();
    corpus.reset();
    quarantine.clear();
    std::printf(fails ? "FAIL\n" : "OK\n");
    return fails;
}
// UMAP is never reached (no corpus is loaded through CorpusLoader here); stubbed
// to keep umappp/Eigen out of the build.
void initEigenOnMainThread() {}
void UMAPProjection::compute(const std::vector<GrainMetadata>&, std::vector<float>&, std::vector<float>&,
                             ProgressCallback, std::function<bool()>) {}
