// v1.6.2 gate: corpus state touched from host threads. Runs the real processor;
// the main thread is the JUCE message thread.
//
//   A. race — a host thread loops prepareToPlay (44.1k/48k) + getStateInformation
//      while this thread publishes corpora (no audio thread: every publish frees
//      the previous parked corpus at once). The deleter POISONS a corpus (rate -1,
//      path -> a real POISON.wav) and quarantines it. A host-thread read of a freed
//      corpus then shows up in the saved state, or as prepareToPlay loading
//      POISON.wav (v1.6.1 did: it read the path of a corpus being freed).
//   B. dedupe — a loaded 44.1k corpus, then prepareToPlay(48000) x3 on a host thread:
//      exactly one reload, ending at 48 kHz.
//   C. stale rate — fresh instance, first load in flight at 44.1k, then
//      prepareToPlay(48000): the corpus must end at 48 kHz.
//
// usage: prepare_stress <scratchDir>   (exit 0 = all pass; prints each check)
#include <JuceHeader.h>
#include <atomic>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "KDTreeSearch.h"
#define private public
#include "PluginProcessor.h"
#undef private

static juce::String poisonPath;
static std::mutex quarantineLock;
static std::vector<std::unique_ptr<SharedCorpus>> quarantine;
static int fails = 0;

static void check(const char* name, bool ok, const juce::String& detail)
{
    std::printf("%s %s  %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
    if (!ok) ++fails;
}

static std::shared_ptr<SharedCorpus> makeFakeCorpus(int k)
{
    auto* c = new SharedCorpus();
    c->audioData.setSize(1, 4096);
    c->audioData.clear();
    GrainMetadata g {};
    g.lengthSamples = 2048;
    c->grains.push_back(g);
    c->sampleRate = 44100.0;
    c->filePath = "/nonexistent/corpus-" + juce::String(k) + ".wav";
    return std::shared_ptr<SharedCorpus>(c, [](SharedCorpus* dead)
    {
        dead->filePath = poisonPath;
        dead->sampleRate = -1.0;
        dead->audioData.setSize(0, 0);   // 200k of these are quarantined
        const std::lock_guard<std::mutex> lock(quarantineLock);
        quarantine.emplace_back(dead);
    });
}

static void onHostThread(std::function<void()> fn)
{
    std::thread t(std::move(fn));
    t.join();
}

// Pumps the message loop until pred() or timeout; returns how many times the
// current corpus pointer changed meanwhile.
static int pumpUntil(TextureForgeProcessor& p, std::function<bool()> pred, int timeoutMs, int settleMs)
{
    int changes = 0;
    auto last = p.getCurrentCorpus().get();
    const auto poll = [&]
    {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(5);
        auto now = p.getCurrentCorpus().get();
        if (now != last) { ++changes; last = now; }
    };
    const auto t0 = juce::Time::getMillisecondCounter();
    while (!pred() && juce::Time::getMillisecondCounter() - t0 < (juce::uint32) timeoutMs)
        poll();
    const auto t1 = juce::Time::getMillisecondCounter();
    while (juce::Time::getMillisecondCounter() - t1 < (juce::uint32) settleMs)
        poll();
    return changes;
}

static double corpusRate(TextureForgeProcessor& p)
{
    auto c = p.getCurrentCorpus();
    return c != nullptr ? c->sampleRate : 0.0;
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const juce::File scratch(argc > 1 ? juce::String(argv[1]) : juce::File::getSpecialLocation(juce::File::tempDirectory).getFullPathName());

    // A real file: 4 s, 44.1 kHz. Loaded by B and C; a copy is A's poison path.
    const auto wav = scratch.getChildFile("tf_prepare_stress.wav");
    {
        juce::AudioBuffer<float> b(1, 4 * 44100);
        juce::Random r(7);
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample(0, i, 0.3f * std::sin(0.03f * (float) i) + 0.1f * (r.nextFloat() * 2.0f - 1.0f));
        wav.deleteFile();
        juce::WavAudioFormat fmt;
        std::unique_ptr<juce::AudioFormatWriter> w(fmt.createWriterFor(new juce::FileOutputStream(wav), 44100.0, 1, 24, {}, 0));
        w->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
    }
    const auto poisonWav = scratch.getChildFile("tf_prepare_stress_POISON.wav");
    wav.copyFileTo(poisonWav);
    poisonPath = poisonWav.getFullPathName();

    // ---- A. race
    {
        auto proc = std::make_unique<TextureForgeProcessor>();
        std::atomic<bool> stop { false };
        std::atomic<int> states { 0 }, poisoned { 0 };
        std::thread host([&]
        {
            juce::MemoryBlock mb;
            for (int k = 0; !stop.load(); ++k)
            {
                proc->prepareToPlay(k % 2 ? 48000.0 : 44100.0, 512);
                proc->getStateInformation(mb);
                if (auto xml = juce::AudioProcessor::getXmlFromBinary(mb.getData(), (int) mb.getSize()))
                    if (auto* c = xml->getChildByName("CORPUS"))
                        if (!c->getStringAttribute("filePath").startsWith("/nonexistent/corpus-"))
                            poisoned.fetch_add(1);
                states.fetch_add(1);
            }
        });
        int poisonLoads = 0;
        const auto sawPoisonLoad = [&]
        {
            auto c = proc->getCurrentCorpus();
            if (c != nullptr && c->filePath == poisonPath && c->sampleRate > 0.0)
            {
                ++poisonLoads;
                proc->publishCorpus(makeFakeCorpus(-1));
            }
        };
        for (int k = 0; k < 200000; ++k)
        {
            proc->publishCorpus(makeFakeCorpus(k));
            if (k % 200 == 0)
            {
                juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
                sawPoisonLoad();
            }
        }
        stop.store(true);
        host.join();
        for (int t = 0; t < 600; ++t)   // 3 s for any POISON.wav load to land
        {
            juce::MessageManager::getInstance()->runDispatchLoopUntil(5);
            sawPoisonLoad();
        }
        check("A. host threads never read a freed corpus", poisoned.load() == 0 && poisonLoads == 0,
              juce::String(states.load()) + " states, " + juce::String(poisoned.load()) + " poisoned, "
                  + juce::String(poisonLoads) + " POISON.wav load(s)");
        proc.reset();
    }

    // ---- B. dedupe
    {
        auto proc = std::make_unique<TextureForgeProcessor>();
        onHostThread([&] { proc->prepareToPlay(44100.0, 512); });
        proc->loadCorpusFile(wav);
        pumpUntil(*proc, [&] { return corpusRate(*proc) == 44100.0; }, 30000, 300);
        check("B0. initial load at 44.1 kHz", corpusRate(*proc) == 44100.0, juce::String(corpusRate(*proc)));

        onHostThread([&] { for (int i = 0; i < 3; ++i) proc->prepareToPlay(48000.0, 512); });
        const int changes = pumpUntil(*proc, [&] { return corpusRate(*proc) == 48000.0; }, 30000, 2000);
        check("B. 3x prepareToPlay(48k) -> one reload at 48 kHz", corpusRate(*proc) == 48000.0 && changes == 1,
              "rate " + juce::String(corpusRate(*proc)) + ", " + juce::String(changes) + " publish(es)");
        proc.reset();
    }

    // ---- C. stale rate
    {
        auto proc = std::make_unique<TextureForgeProcessor>();
        onHostThread([&] { proc->prepareToPlay(44100.0, 512); });
        proc->loadCorpusFile(wav);
        onHostThread([&] { proc->prepareToPlay(48000.0, 512); });
        pumpUntil(*proc, [&] { return corpusRate(*proc) == 48000.0; }, 30000, 2000);
        check("C. load in flight across a rate change ends at the new rate", corpusRate(*proc) == 48000.0,
              juce::String(corpusRate(*proc)));
        proc.reset();
    }

    wav.deleteFile();
    poisonWav.deleteFile();
    juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    quarantine.clear();
    std::printf("%d failure(s)\n", fails);
    return fails;
}

// UMAP is stubbed to keep umappp/Eigen out of the build; the loader still runs it
// after PCA, so the stub returns at once with no points.
void initEigenOnMainThread() {}
void UMAPProjection::compute(const std::vector<GrainMetadata>&, std::vector<float>&, std::vector<float>&,
                             ProgressCallback, std::function<bool()>) {}
