/*
  ==============================================================================

    O-Gain — offline LUFS meter harness (v1.6.0)

    Proves the v1.6.0 acceptance criterion without a host or a WebView:

      With Learn idle and meter mode LUFS, both columns read momentary LUFS
      and track a -18 LUFS pink-noise source to within 1 dB.

    The page's LUFS case (app.js case 3) is a straight read of two processor
    atomics, momentaryLufsIn / momentaryLufsOut, so the atomics ARE the
    columns. This harness:

      1. generates stereo pink noise (Paul Kellet's refined filter, two
         independent seeded streams),
      2. calibrates it to exactly -18.00 LUFS with an INDEPENDENT BS.1770
         implementation (ITU-R BS.1770-4 Table 1/2 coefficients at 48 kHz,
         direct-form biquads, ungated mean K-weighted power over the whole
         signal) — the plugin's own KWeight namespace is never read,
      3. runs the plugin at 48 kHz / 480-sample blocks with gain 0 dB, Learn
         never started, and samples the two atomics after every block,
      4. asserts every closed 400 ms window after the first reads -18 +/- 1 dB
         on BOTH columns, that the columns agree with each other, and that the
         plugin's blocks agree with the reference's blocks on the same hop grid
         to 0.05 dB,
      5. proves the reading is a LOUDNESS and not the old RMS fallback: the
         same signal's per-channel RMS in dBFS sits several dB away from its
         LUFS (K-weighting lifts the highs and cuts the lows of pink noise),
         and the gate demands the two differ by more than 0.5 dB,
      6. sets gain_offset = +6 dB: the output column must read -12 +/- 1 dB
         while the input column stays at -18,
      7. feeds a second of silence: both columns must fall to the floor.

    Exit 0 = every gate green; 1 = at least one red row (printed).

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    constexpr double kSampleRate   = 48000.0;
    constexpr int    kBlockSize    = 480;              // 10 ms; a 100 ms hop = exactly 10 blocks
    constexpr int    kHopSamples   = 4800;
    constexpr int    kBlocksPerHop = kHopSamples / kBlockSize;
    constexpr double kTargetLufs   = -18.0;
    constexpr double kToleranceDb  = 1.0;              // the acceptance criterion
    constexpr double kRefAgreeDb   = 0.05;             // plugin vs independent reference, same windows
    constexpr double kSettleSec    = 0.5;              // first full window + margin
    constexpr double kNoiseSeconds = 12.0;

    int failures = 0;

    void gate(bool ok, const char* name, const char* detail)
    {
        std::printf("  [%s] %-58s %s\n", ok ? "PASS" : "FAIL", name, detail);
        if (! ok) ++failures;
    }

    // ── Independent BS.1770 reference ───────────────────────────────────────
    struct Biquad
    {
        double b0, b1, b2, a1, a2;
        double z1 = 0.0, z2 = 0.0;

        double process(double x) noexcept
        {
            // Transposed direct form II
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    // ITU-R BS.1770-4, Tables 1 and 2, fs = 48 kHz. Typed from the Recommendation,
    // not copied from the plugin.
    Biquad makePre48k() { return { 1.53512485958697, -2.69169618940638, 1.19839281085285,
                                   -1.69065929318241, 0.73248077421585 }; }
    Biquad makeRlb48k() { return { 1.0, -2.0, 1.0,
                                   -1.99004745483398, 0.99007225036621 }; }

    struct KChain
    {
        Biquad preL = makePre48k(), preR = makePre48k();
        Biquad rlbL = makeRlb48k(), rlbR = makeRlb48k();

        void process(double l, double r, double& kl, double& kr) noexcept
        {
            kl = rlbL.process(preL.process(l));
            kr = rlbR.process(preR.process(r));
        }
    };

    double powerToLufs(double p) { return p > 0.0 ? -0.691 + 10.0 * std::log10(p) : -100.0; }

    // ── Pink noise (Paul Kellet, refined) ───────────────────────────────────
    struct Pink
    {
        std::mt19937 rng;
        std::uniform_real_distribution<double> uni { -1.0, 1.0 };
        double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;

        explicit Pink(unsigned seed) : rng(seed) {}

        double next() noexcept
        {
            const double w = uni(rng);
            b0 = 0.99886 * b0 + w * 0.0555179;
            b1 = 0.99332 * b1 + w * 0.0750759;
            b2 = 0.96900 * b2 + w * 0.1538520;
            b3 = 0.86650 * b3 + w * 0.3104856;
            b4 = 0.55000 * b4 + w * 0.5329522;
            b5 = -0.7616 * b5 - w * 0.0168980;
            const double pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362;
            b6 = w * 0.115926;
            return pink * 0.11;   // keeps raw peaks well inside +/-1 before calibration
        }
    };

    struct Reading { int blockIndex; float in; float out; };
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::printf("O-Gain LUFS meter harness (v1.6.0)\n");
    std::printf("  fs=%.0f, block=%d, hop=%d samples, target=%.1f LUFS, tolerance=%.1f dB\n\n",
                kSampleRate, kBlockSize, kHopSamples, kTargetLufs, kToleranceDb);

    // ── 1-2. Source: pink noise calibrated to -18.00 LUFS by the reference ──
    const int numNoise = static_cast<int>(kNoiseSeconds * kSampleRate);
    std::vector<double> srcL(static_cast<size_t>(numNoise)), srcR(static_cast<size_t>(numNoise));
    {
        Pink pl(0x5EED0001u), pr(0x5EED0002u);
        for (int i = 0; i < numNoise; ++i) { srcL[(size_t) i] = pl.next(); srcR[(size_t) i] = pr.next(); }

        KChain ref;
        double accL = 0.0, accR = 0.0;
        for (int i = 0; i < numNoise; ++i)
        {
            double kl, kr; ref.process(srcL[(size_t) i], srcR[(size_t) i], kl, kr);
            accL += kl * kl; accR += kr * kr;
        }
        const double rawLufs = powerToLufs(accL / numNoise + accR / numNoise);
        const double scale   = std::pow(10.0, (kTargetLufs - rawLufs) / 20.0);
        for (int i = 0; i < numNoise; ++i) { srcL[(size_t) i] *= scale; srcR[(size_t) i] *= scale; }
        std::printf("  source: raw %.2f LUFS -> scaled by %.4f to %.2f LUFS (ungated, whole signal)\n",
                    rawLufs, scale, kTargetLufs);
    }

    // Per-channel RMS of the calibrated source, for the LUFS-vs-RMS discriminator.
    double rmsDbL, rmsDbR, peakDb = -100.0;
    {
        double aL = 0.0, aR = 0.0, pk = 0.0;
        for (int i = 0; i < numNoise; ++i)
        {
            aL += srcL[(size_t) i] * srcL[(size_t) i];
            aR += srcR[(size_t) i] * srcR[(size_t) i];
            pk = std::max(pk, std::max(std::abs(srcL[(size_t) i]), std::abs(srcR[(size_t) i])));
        }
        rmsDbL = 10.0 * std::log10(aL / numNoise);
        rmsDbR = 10.0 * std::log10(aR / numNoise);
        peakDb = 20.0 * std::log10(pk);
        std::printf("  source: RMS L %.2f dBFS, R %.2f dBFS, sample peak %.2f dBFS\n\n", rmsDbL, rmsDbR, peakDb);
    }

    // Reference momentary series on the plugin's hop grid: window k covers
    // samples [(k-3)H, (k+1)H) and closes at sample (k+1)H - 1, i.e. at the end
    // of processBlock number (k+1)*kBlocksPerHop (1-based). Valid for k >= 3.
    std::vector<double> refMomentary;
    {
        KChain ref;
        std::vector<double> hopPower;
        double acc = 0.0; int n = 0;
        for (int i = 0; i < numNoise; ++i)
        {
            double kl, kr; ref.process(srcL[(size_t) i], srcR[(size_t) i], kl, kr);
            acc += kl * kl + kr * kr;
            if (++n == kHopSamples) { hopPower.push_back(acc / n); acc = 0.0; n = 0; }
        }
        refMomentary.assign(hopPower.size(), -100.0);
        for (size_t k = 3; k < hopPower.size(); ++k)
            refMomentary[k] = powerToLufs((hopPower[k] + hopPower[k - 1] + hopPower[k - 2] + hopPower[k - 3]) * 0.25);
    }

    // ── 3. Plugin ───────────────────────────────────────────────────────────
    std::unique_ptr<juce::AudioProcessor> base(createPluginFilter());
    auto* proc = dynamic_cast<OGainAudioProcessor*>(base.get());
    if (proc == nullptr) { std::printf("FAIL: createPluginFilter() did not return an OGainAudioProcessor\n"); return 1; }

    proc->setPlayConfigDetails(2, 2, kSampleRate, kBlockSize);
    proc->prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::MidiBuffer midi;

    auto runBlock = [&](int startSample, const std::vector<double>* l, const std::vector<double>* r)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = buffer.getWritePointer(ch);
            const auto* src = (ch == 0) ? l : r;
            for (int i = 0; i < kBlockSize; ++i)
                d[i] = (src != nullptr) ? static_cast<float>((*src)[(size_t) (startSample + i)]) : 0.0f;
        }
        proc->processBlock(buffer, midi);
    };

    // Phase A: gain 0 dB, Learn idle, the whole noise signal.
    std::vector<Reading> readings;
    const int numBlocks = numNoise / kBlockSize;
    for (int b = 0; b < numBlocks; ++b)
    {
        runBlock(b * kBlockSize, &srcL, &srcR);
        readings.push_back({ b + 1, proc->momentaryLufsIn.load(), proc->momentaryLufsOut.load() });
    }

    std::printf("Phase A — gain 0 dB, Learn idle, %.0f s of -18 LUFS pink noise\n", kNoiseSeconds);

    const auto learnSnap = proc->readLearnSnapshot();
    gate(! proc->learnActive.load() && learnSnap.state == 0,
         "Learn idle throughout (learnActive false, snapshot state 0)",
         learnSnap.state == 0 ? "state=0" : "state!=0");

    {
        const int settleBlocks = static_cast<int>(kSettleSec * kSampleRate / kBlockSize);
        double maxDevIn = 0.0, maxDevOut = 0.0, maxColDiff = 0.0, maxRefDiff = 0.0, sumIn = 0.0;
        int n = 0, nRef = 0;
        for (const auto& rd : readings)
        {
            if (rd.blockIndex <= settleBlocks) continue;
            maxDevIn   = std::max(maxDevIn,  std::abs(rd.in  - kTargetLufs));
            maxDevOut  = std::max(maxDevOut, std::abs(rd.out - kTargetLufs));
            maxColDiff = std::max(maxColDiff, (double) std::abs(rd.in - rd.out));
            sumIn += rd.in; ++n;

            // Compare on hop-closing blocks only: block (k+1)*kBlocksPerHop closed window k.
            if (rd.blockIndex % kBlocksPerHop == 0)
            {
                const int k = rd.blockIndex / kBlocksPerHop - 1;
                if (k >= 3 && k < (int) refMomentary.size())
                {
                    maxRefDiff = std::max(maxRefDiff, std::abs(rd.in - refMomentary[(size_t) k]));
                    ++nRef;
                }
            }
        }
        char d[160];
        std::snprintf(d, sizeof d, "max |in - (-18)| = %.3f dB over %d blocks (mean %.2f LUFS)", maxDevIn, n, sumIn / n);
        gate(maxDevIn <= kToleranceDb, "INPUT column tracks -18 LUFS within 1 dB", d);
        std::snprintf(d, sizeof d, "max |out - (-18)| = %.3f dB", maxDevOut);
        gate(maxDevOut <= kToleranceDb, "OUTPUT column tracks -18 LUFS within 1 dB at unity gain", d);
        std::snprintf(d, sizeof d, "max |in - out| = %.4f dB", maxColDiff);
        gate(maxColDiff <= 0.01, "both columns agree at unity gain", d);
        std::snprintf(d, sizeof d, "max |plugin - reference| = %.4f dB over %d windows", maxRefDiff, nRef);
        gate(nRef > 50 && maxRefDiff <= kRefAgreeDb, "plugin windows match the independent BS.1770 reference", d);

        // 5. Discriminator: a LUFS reading is NOT the old RMS fallback.
        const double lufsMinusRms = (sumIn / n) - std::max(rmsDbL, rmsDbR);
        std::snprintf(d, sizeof d, "mean LUFS - loudest-channel RMS dBFS = %+.2f dB (must exceed 0.5)", lufsMinusRms);
        gate(std::abs(lufsMinusRms) > 0.5, "reading is a loudness, distinguishable from the RMS it replaced", d);

        // Sanity: the reading appeared at all (the first full window closes at 0.4 s).
        const auto& first = readings[(size_t) (kBlocksPerHop * 4 - 1)];
        std::snprintf(d, sizeof d, "after block %d: in=%.2f out=%.2f", first.blockIndex, first.in, first.out);
        gate(first.in > -99.0f && first.out > -99.0f, "first window published at 0.4 s", d);
        const auto& before = readings[(size_t) (kBlocksPerHop * 4 - 2)];
        std::snprintf(d, sizeof d, "after block %d: in=%.1f out=%.1f", before.blockIndex, before.in, before.out);
        gate(before.in <= -99.0f && before.out <= -99.0f, "nothing published before the first full window", d);
    }

    // ── 6. Phase B: gain_offset = +6 dB ─────────────────────────────────────
    std::printf("\nPhase B — gain_offset +6 dB, Learn idle\n");
    {
        auto* gain = proc->parameters.getParameter("gain_offset");
        if (gain == nullptr) { std::printf("FAIL: no gain_offset parameter\n"); return 1; }
        gain->setValueNotifyingHost(gain->convertTo0to1(6.0f));

        // Re-run the same signal; the 20 ms ramp plus one window settle inside kSettleSec.
        std::vector<Reading> rb;
        for (int b = 0; b < numBlocks; ++b)
        {
            runBlock(b * kBlockSize, &srcL, &srcR);
            rb.push_back({ b + 1, proc->momentaryLufsIn.load(), proc->momentaryLufsOut.load() });
        }
        const int settleBlocks = static_cast<int>(kSettleSec * kSampleRate / kBlockSize);
        double maxDevIn = 0.0, maxDevOut = 0.0, sumOut = 0.0; int n = 0;
        for (const auto& rd : rb)
        {
            if (rd.blockIndex <= settleBlocks) continue;
            maxDevIn  = std::max(maxDevIn,  std::abs(rd.in  - kTargetLufs));
            maxDevOut = std::max(maxDevOut, std::abs(rd.out - (kTargetLufs + 6.0)));
            sumOut += rd.out; ++n;
        }
        char d[160];
        std::snprintf(d, sizeof d, "max |in - (-18)| = %.3f dB", maxDevIn);
        gate(maxDevIn <= kToleranceDb, "INPUT column unaffected by gain (still -18)", d);
        std::snprintf(d, sizeof d, "max |out - (-12)| = %.3f dB (mean %.2f LUFS)", maxDevOut, sumOut / n);
        gate(maxDevOut <= kToleranceDb, "OUTPUT column reads -12 LUFS at +6 dB", d);

        gain->setValueNotifyingHost(gain->convertTo0to1(0.0f));
    }

    // ── 7. Phase C: silence ─────────────────────────────────────────────────
    std::printf("\nPhase C — 1 s of silence\n");
    {
        const int silentBlocks = static_cast<int>(kSampleRate / kBlockSize);
        for (int b = 0; b < silentBlocks; ++b) runBlock(0, nullptr, nullptr);
        const float in = proc->momentaryLufsIn.load(), out = proc->momentaryLufsOut.load();
        char d[160];
        std::snprintf(d, sizeof d, "in=%.1f out=%.1f", in, out);
        gate(in == -100.0f && out == -100.0f, "both columns sit exactly on the -100 floor on silence", d);
    }

    proc->releaseResources();

    std::printf("\n%s — %d failing gate(s)\n", failures == 0 ? "ALL GATES PASSED" : "GATES FAILED", failures);
    return failures == 0 ? 0 : 1;
}
