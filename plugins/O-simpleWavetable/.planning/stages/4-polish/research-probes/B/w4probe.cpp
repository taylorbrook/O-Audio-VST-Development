// W4 probe: Mono legato 60 -> 96 (3 octaves up) through the shipped WtVoice
// crossfader (xfMode 0) and fix (c) (xfMode 1: the frozen outgoing cycle keeps
// the rate it was heard at). Scratch copy WtVoiceProbe.h; Source/ untouched.
//
//  A. real 5 ms fade, analytic alias: e[n] = (1-w)(F - BL_F)(phi_n) * env * 0.5,
//     BL_F = the frozen cycle with every harmonic k * f_read >= fs/2 removed.
//     Sampling a component above fs/2 folds it at full amplitude, so e IS the
//     aliased part. Reported as peak |e| and rms(e) over the fade, in dB rel the
//     new note's steady RMS.
//  B. exactness (the gate shape): fix output == (1-w) y60 + w yHard over the fade.
//  C. magnifier: fade forced to 2 s (xfadeLenOverride), Kaiser-38 FFT 2^15 at
//     0.35 s after the jump; max inharmonic bin (mask +/-14 bins around k * f60)
//     rel max harmonic bin.
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "BankFactory.h"
#include "MipmapBuilder.h"
#include "WtVoiceProbe.h"
#include <cstdio>
#include <vector>

namespace
{
    constexpr double kPi = 3.14159265358979323846;

    struct Out { std::vector<float> y; int jumpAt = 0; int capLevel = -1; int traceN = 0;
                 std::vector<float> w, env; std::vector<double> ph; std::vector<float> frozen; double readInc = 0; };

    Out render (const WavetableBank* bank, double fs, float pos, int n1, int n2, bool jump, int xfMode, int lenOverride,
                double total = 1.2, double tJump = 0.25)
    {
        WtVoice v; WtSound snd; BlockContext ctx;
        const int blk = 64;
        std::vector<float> knob ((size_t) blk, pos), zero ((size_t) blk, 0.0f);
        ctx.knobPos = knob.data(); ctx.lfo = zero.data(); ctx.lfoDepth = zero.data(); ctx.envAmount = zero.data();
        ctx.bank = bank; ctx.interp = true; ctx.bandlimit = true; ctx.bitDepthIndex = 0;
        v.setBlockContext (&ctx);
        const juce::ADSR::Parameters amp { 0.001f, 0.001f, 1.0f, 0.05f };
        v.prepareToPlay (fs, blk, amp); v.setBlockParams (amp);
        v.xfMode = xfMode; v.xfadeLenOverride = lenOverride;
        v.noteOnDirect (n1, 1.0f, true);

        Out o; const int N = (int) std::lround (total * fs); o.jumpAt = (int) std::lround (tJump * fs);
        o.y.reserve ((size_t) N);
        juce::AudioBuffer<float> b (1, blk);
        int done = 0; bool jumped = false;
        while (done < N)
        {
            int len = std::min (blk, N - done);
            if (! jumped && done < o.jumpAt && done + len > o.jumpAt) len = o.jumpAt - done;
            if (! jumped && done == o.jumpAt)
            {
                jumped = true;
                if (jump) { v.traceOn = true; v.noteOnDirect (n2, 1.0f, false); }
            }
            b.clear(); v.renderNextBlock (b, 0, len);
            if (jumped && jump && o.capLevel < 0 && v.capLevelPublic >= 0)
            {
                o.capLevel = v.capLevelPublic;
                o.frozen.assign (v.getFrozenActive(), v.getFrozenActive() + WavetableBank::kStride);
                o.readInc = xfMode == 1 ? 0.0 : v.getInc();
            }
            o.y.insert (o.y.end(), b.getReadPointer (0), b.getReadPointer (0) + len);
            done += len;
        }
        o.traceN = v.traceN; o.w = v.trW; o.env = v.trEnv; o.ph = v.trPh;
        return o;
    }

    double rms (const std::vector<float>& x, int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += (double) x[(size_t) i] * x[(size_t) i]; return std::sqrt (s / std::max (1, b - a)); }
    double dB (double r) { return 20.0 * std::log10 (std::max (r, 1e-300)); }

    // Frozen cycle with every harmonic whose read frequency reaches fs/2 removed.
    std::vector<float> bandLimit (const std::vector<float>& fr, double fRead, double fs)
    {
        juce::dsp::FFT fft (11);
        std::vector<float> w (4096, 0.0f);
        std::copy (fr.begin(), fr.begin() + 2048, w.begin());
        fft.performRealOnlyForwardTransform (w.data());
        for (int k = 1; k <= 1024; ++k)
            if ((double) k * fRead >= 0.5 * fs) { w[(size_t) (2 * k)] = 0; w[(size_t) (2 * k + 1)] = 0; }
        std::fill (w.begin() + 2050, w.end(), 0.0f);
        fft.performRealOnlyInverseTransform (w.data());
        std::vector<float> o (2049); std::copy (w.begin(), w.begin() + 2048, o.begin()); o[2048] = o[0];
        return o;
    }

    float lerpAt (const std::vector<float>& t, double ph)
    {
        const double idx = ph * 2048.0; int i0 = std::min (2047, (int) idx); const float f = (float) (idx - i0);
        return t[(size_t) i0] + f * (t[(size_t) i0 + 1] - t[(size_t) i0]);
    }

    double inharmonicDb (const std::vector<float>& x, int start, double f0, double fs)
    {
        constexpr int order = 15, n = 1 << order, guard = 14;
        juce::dsp::FFT fft (order);
        std::vector<float> win ((size_t) n), w ((size_t) (2 * n), 0.0f);
        juce::dsp::WindowingFunction<float>::fillWindowingTables (win.data(), (size_t) n, juce::dsp::WindowingFunction<float>::kaiser, false, 38.0f);
        for (int i = 0; i < n; ++i) w[(size_t) i] = x[(size_t) (start + i)] * win[(size_t) i];
        fft.performFrequencyOnlyForwardTransform (w.data(), true);
        const double binHz = fs / n; std::vector<unsigned char> mask ((size_t) (n / 2 + 1), 0);
        for (int b = 0; b <= guard; ++b) mask[(size_t) b] = 2;
        for (int h = 1; h * f0 < 0.5 * fs; ++h) { const int c = (int) std::lround (h * f0 / binHz); for (int b = std::max (0, c - guard); b <= std::min (n / 2, c + guard); ++b) if (! mask[(size_t) b]) mask[(size_t) b] = 1; }
        double mh = 0, mi = 0;
        for (int b = 0; b <= n / 2; ++b) { const double m = w[(size_t) b]; if (mask[(size_t) b] == 1) mh = std::max (mh, m); else if (mask[(size_t) b] == 0) mi = std::max (mi, m); }
        return dB (mi / std::max (mh, 1e-300));
    }
}

int main()
{
    MipmapBuilder builder;
    WavetableBank banks[3];
    const int ids[3] = { BankFactory::sineSaw, BankFactory::drive, BankFactory::pulseWidth };
    const char* names[3] = { "SineSaw", "Drive", "Pulse" };
    for (int i = 0; i < 3; ++i) BankFactory::build (ids[i], banks[i], builder);

    std::printf ("bank,fs,capLevel,newLevelKmax,A_bug_alias_peak_dB,A_bug_alias_rms_dB,A_fix_alias_peak_dB,B_fix_resid,B_bug_resid,C_bug_inh_dB,C_fix_inh_dB,C_nojump_inh_dB\n");
    for (int bi = 0; bi < 3; ++bi)
        for (double fs : { 44100.0, 48000.0, 96000.0 })
        {
            const WavetableBank* bk = &banks[bi];
            const double f96 = juce::MidiMessage::getMidiNoteInHertz (96), f60 = juce::MidiMessage::getMidiNoteInHertz (60);
            const int len = (int) std::lround (0.005 * fs);

            // A: analytic alias of the real 5 ms fade (bug: frozen read at f96; fix: at f60)
            auto bug = render (bk, fs, 1.0f, 60, 96, true, 0, -1);
            auto fix = render (bk, fs, 1.0f, 60, 96, true, 1, -1);
            const int j = bug.jumpAt;
            const double steady = rms (bug.y, j + (int) (0.3 * fs), j + (int) (0.5 * fs));
            auto alias = [&] (const Out& o, double fRead)
            {
                const auto bl = bandLimit (o.frozen, fRead, fs);
                double pk = 0, ss = 0;
                for (int k = 0; k < std::min (len, o.traceN); ++k)
                {
                    const double e = (1.0 - o.w[(size_t) k]) * (lerpAt (o.frozen, o.ph[(size_t) k]) - lerpAt (bl, o.ph[(size_t) k])) * o.env[(size_t) k] * WtVoice::kVoiceGain;
                    pk = std::max (pk, std::abs (e)); ss += e * e;
                }
                return std::pair<double, double> (dB (pk / steady), dB (std::sqrt (ss / len) / steady));
            };
            const auto aBug = alias (bug, f96);
            const auto aFix = alias (fix, f60);

            // B: exactness vs (1-w) y60 + w yHard
            auto y60 = render (bk, fs, 1.0f, 60, 96, false, 1, -1);
            auto yHard = render (bk, fs, 1.0f, 60, 96, true, 0, 0);
            double rFix = 0, rBug = 0;
            for (int k = 0; k < len; ++k)
            {
                const double w = 0.5 - 0.5 * std::cos (kPi * k / len);
                const double ref = (1.0 - w) * y60.y[(size_t) (j + k)] + w * yHard.y[(size_t) (j + k)];
                rFix = std::max (rFix, std::abs (fix.y[(size_t) (j + k)] - ref));
                rBug = std::max (rBug, std::abs (bug.y[(size_t) (j + k)] - ref));
            }
            // after the fade both builds must be bit-identical (live part only)
            int diffAfter = 0;
            for (size_t k = (size_t) (j + len); k < fix.y.size(); ++k) if (fix.y[k] != bug.y[k]) ++diffAfter;
            int diffBefore = 0;
            for (int k = 0; k < j; ++k) if (fix.y[(size_t) k] != bug.y[(size_t) k]) ++diffBefore;

            // C: magnifier (2 s fade)
            const int L2 = (int) std::lround (2.0 * fs);
            auto mBug = render (bk, fs, 1.0f, 60, 96, true, 0, L2, 2.0);
            auto mFix = render (bk, fs, 1.0f, 60, 96, true, 1, L2, 2.0);
            auto mNo  = render (bk, fs, 1.0f, 60, 96, false, 1, L2, 2.0);
            const int st = j + (int) (0.35 * fs);
            const double cBug = inharmonicDb (mBug.y, st, f60, fs), cFix = inharmonicDb (mFix.y, st, f60, fs), cNo = inharmonicDb (mNo.y, st, f60, fs);

            std::printf ("%s,%.0f,%d,%d,%.1f,%.1f,%.1f,%.2e,%.3f,%.1f,%.1f,%.1f  (diff before %d, after-fade %d)\n", names[bi], fs, bug.capLevel,
                         WavetableBank::kmax (wt::selectLevel (f96, fs, true)), aBug.first, aBug.second, aFix.first, rFix, rBug, cBug, cFix, cNo,
                         diffBefore, diffAfter);
        }
    return 0;
}
