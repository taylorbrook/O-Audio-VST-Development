// Stage 4 Part-B gate prototypes, run against the shipped Source (orig) and the
// scratch src-fix tree (FIXBUILD). Real OSimpleWavetableAudioProcessor, hooks on.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include "BankFactory.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

extern "C"
{
    typedef void (malloc_logger_t) (uint32_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t);
    extern malloc_logger_t* malloc_logger;
}

namespace
{
    using Proc = OSimpleWavetableAudioProcessor;
    namespace ids = OSimpleWavetable::ParamIDs;
    constexpr double kPi = 3.14159265358979323846;

    volatile bool armed = false; volatile int allocs = 0; pthread_t audioThread;
    void hook (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
    {
        if (armed && (type & 2u) && pthread_equal (pthread_self(), audioThread)) { allocs = allocs + 1; }
    }

    struct Ev { int pos; juce::MidiMessage m; };
    Ev on (int p, int n, int v = 127) { return { p, juce::MidiMessage::noteOn (1, n, (juce::uint8) v) }; }
    Ev off (int p, int n) { return { p, juce::MidiMessage::noteOff (1, n) }; }
    Ev wheel (int p, int v) { return { p, juce::MidiMessage::pitchWheel (1, v) }; }
    Ev cc (int p, int c, int v) { return { p, juce::MidiMessage::controllerEvent (1, c, v) }; }
    int secs (double t, double fs) { return (int) std::lround (t * fs); }

    using Params = std::vector<std::pair<const char*, float>>;
    void setP (Proc& p, const char* id, float v) { if (auto* r = p.getAPVTS().getParameter (id)) r->setValueNotifyingHost (r->convertTo0to1 (v)); }
    Params base (int bank, float pos)
    {
        return { { ids::bank, (float) bank }, { ids::position, pos }, { ids::interp, 1.0f }, { ids::bandlimit, 1.0f },
                 { ids::bitDepth, 0.0f }, { ids::ampAttack, 0.001f }, { ids::ampDecay, 0.001f }, { ids::ampSustain, 1.0f },
                 { ids::ampRelease, 0.05f }, { ids::voiceMode, 0.0f }, { ids::outputLevel, 0.0f } };
    }
    Params with (Params p, const char* id, float v) { p.emplace_back (id, v); return p; }

    struct Rig
    {
        std::unique_ptr<Proc> proc = std::make_unique<Proc>(); int block; int cursor = 0; int blocks = 0;
        juce::AudioBuffer<float> buf; juce::MidiBuffer midi; bool countAllocs = false;
        Rig (double fs, int blk, const Params& ps) : block (blk), buf (2, blk)
        {
            for (auto& p : ps) setP (*proc, p.first, p.second);
            proc->setPlayConfigDetails (0, 2, fs, blk); proc->prepareToPlay (fs, blk);
            midi.ensureSize (1 << 20);
        }
        void set (const char* id, float v) { setP (*proc, id, v); }
        std::vector<float> run (int n, const std::vector<Ev>& evs = {})
        {
            std::vector<float> out; out.reserve ((size_t) n);
            for (int d = 0; d < n;)
            {
                const int len = std::min (block, n - d);
                buf.setSize (2, len, false, false, true); midi.clear();
                for (auto& e : evs) if (e.pos >= cursor + d && e.pos < cursor + d + len) midi.addEvent (e.m, e.pos - cursor - d);
                armed = countAllocs && blocks > 0;
                proc->processBlock (buf, midi);
                armed = false; ++blocks;
                out.insert (out.end(), buf.getReadPointer (0), buf.getReadPointer (0) + len);
                d += len;
            }
            cursor += n; return out;
        }
    };

    double zcHz (const std::vector<float>& x, int a, int b, double fs)
    {
        std::vector<double> fr;
        for (int i = std::max (a, 0); i + 1 < std::min (b, (int) x.size()); ++i)
            if (x[(size_t) i] < 0.0f && x[(size_t) i + 1] >= 0.0f)
                fr.push_back (i + x[(size_t) i] / (double) (x[(size_t) i] - x[(size_t) i + 1]));
        if (fr.size() < 3) return 0.0;
        return (double) (fr.size() - 1) / ((fr.back() - fr.front()) / fs);
    }
    double rmsOf (const std::vector<float>& x, int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += (double) x[(size_t) i] * x[(size_t) i]; return std::sqrt (s / std::max (1, b - a)); }
    double dB (double r) { return 20.0 * std::log10 (std::max (r, 1e-300)); }

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

   #ifdef FIXBUILD
    constexpr bool kFix = true;
   #else
    constexpr bool kFix = false;
   #endif

    //==========================================================================
    // W3: Poly wheel history -> Poly->Mono -> Mono note. hookOn = fix enabled.
    void w3 (bool hookOn)
    {
        const double fs = 48000.0;
        auto arm = [&] (const char* name, std::vector<Ev> pre, double expectHz)
        {
            Rig r (fs, 512, base (BankFactory::sineSaw, 0.0f));
           #ifdef FIXBUILD
            r.proc->setMonoWheelSeedForTesting (hookOn);
           #endif
            r.run (512 * 56, pre);                 // ~0.6 s of Poly history
            r.set (ids::voiceMode, 1.0f);          // switch lands on a block boundary
            const int t0 = r.cursor;
            const auto y = r.run (secs (1.0, fs), { on (t0 + 64, 69) });
            const double f = zcHz (y, secs (0.2, fs), secs (0.9, fs), fs);
            std::printf ("  W3 %-38s fix=%d hook=%d: %.3f Hz, expected %.3f, error %+.2f c\n", name, (int) kFix, (int) hookOn, f, expectHz,
                         1200.0 * std::log2 (f / expectHz));
        };
        arm ("A up->off->centre(idle)->Mono", { on (0, 69), wheel (secs (0.1, fs), 16383), off (secs (0.15, fs), 69), wheel (secs (0.5, fs), 8192) }, 440.0);
        arm ("B -1 st while idle ->Mono", { wheel (secs (0.1, fs), 4096) }, 440.0 * std::pow (2.0, -1.0 / 12.0));
        arm ("C control, no wheel", {}, 440.0);
    }

    //==========================================================================
    // W4: exactness of the fade vs (1-w) yOld + w yHard, per trigger source.
    struct Arm { const char* name; int bank; bool mono; std::vector<Ev> evs, evsOld; int tTrig; };

    std::vector<float> renderArm (const Arm& a, double fs, int xfOverride, bool keepRate, bool old, double total = 0.6)
    {
        Rig r (fs, 64, with (base (a.bank, 1.0f), ids::voiceMode, a.mono ? 1.0f : 0.0f));
        if (xfOverride >= 0) r.proc->setXfadeLenOverrideForTesting (xfOverride);
       #ifdef FIXBUILD
        r.proc->setXfadeKeepRateForTesting (keepRate);
       #else
        (void) keepRate;
       #endif
        return r.run (secs (total, fs), old ? a.evsOld : a.evs);
    }

    void w4 (bool keepRate)
    {
        const double fs = 48000.0;
        const int t = secs (0.25, fs);
        const int len = (int) std::lround (0.005 * fs);
        std::vector<Arm> arms {
            { "legato 60->96 SineSaw", BankFactory::sineSaw, true, { on (0, 60), on (t, 96) }, { on (0, 60) }, t },
            { "legato 60->96 Drive", BankFactory::drive, true, { on (0, 60), on (t, 96) }, { on (0, 60) }, t },
            { "fallback 60->96 (release top) Drive", BankFactory::drive, true, { on (0, 96), on (100, 60), off (t, 60) }, { on (0, 96), on (100, 60) }, t },
            { "sounding retrig 60->96 Drive", BankFactory::drive, true, { on (0, 60), off (t - 480, 60), on (t, 96) }, { on (0, 60), off (t - 480, 60), on (t, 60) }, t },
            { "Poly wheel F#4 +2st Drive", BankFactory::drive, false, { on (0, 66), wheel (t, 16383) }, { on (0, 66) }, t },
        };
        for (auto& a : arms)
        {
            const auto y = renderArm (a, fs, -1, keepRate, false);
            const auto yOld = renderArm (a, fs, -1, keepRate, true);
            const auto yHard = renderArm (a, fs, 0, keepRate, false);
            double resid = 0, scale = 0;
            for (int k = 0; k < len; ++k)
            {
                const double w = 0.5 - 0.5 * std::cos (kPi * k / len);
                const double ref = (1.0 - w) * yOld[(size_t) (a.tTrig + k)] + w * yHard[(size_t) (a.tTrig + k)];
                resid = std::max (resid, std::abs (y[(size_t) (a.tTrig + k)] - ref));
                scale = std::max (scale, std::abs (ref));
            }
            std::printf ("  W4-EXACT %-36s fix=%d keep=%d: max|y-ref| %.3e (ref peak %.3f)\n", a.name, (int) kFix, (int) keepRate, resid, scale);
        }
        // magnifier: 2 s fade, legato 60 -> 96
        for (int bank : { (int) BankFactory::sineSaw, (int) BankFactory::drive })
        {
            Arm a { "mag", bank, true, { on (0, 60), on (t, 96) }, { on (0, 60) }, t };
            const auto y = renderArm (a, fs, secs (2.0, fs), keepRate, false, 2.0);
            const auto y0 = renderArm (a, fs, secs (2.0, fs), keepRate, true, 2.0);
            const double f60 = juce::MidiMessage::getMidiNoteInHertz (60);
            std::printf ("  W4-ALIAS %-8s fix=%d keep=%d: inharmonic %.1f dB (no-jump floor %.1f dB)\n", bank == 0 ? "SineSaw" : "Drive", (int) kFix,
                         (int) keepRate, inharmonicDb (y, t + secs (0.35, fs), f60, fs), inharmonicDb (y0, t + secs (0.35, fs), f60, fs));
        }
    }

    //==========================================================================
    // note 3: MIDI flood in ONE host block.
    void flood (bool guardOn)
    {
        const double fs = 48000.0;
        Rig r (fs, 512, base (BankFactory::sineSaw, 0.0f));
       #ifdef FIXBUILD
        r.proc->setMidiCapacityGuardForTesting (guardOn);
       #else
        (void) guardOn;
       #endif
        r.countAllocs = true;
        r.run (512);                                                   // warm-up (unarmed)
        const int b0 = r.cursor;
        std::vector<Ev> evs { on (b0 + 0, 69) };
        for (int i = 0; i < 6000; ++i) evs.push_back (cc (b0 + 100, 1, i & 127));   // 6000 CC at ONE sample
        evs.push_back (wheel (b0 + 101, 16383));                       // after the flood: must still land
        allocs = 0;
        auto y = r.run (512, evs);
        const int a = allocs;
        r.countAllocs = false;
        auto y2 = r.run (secs (0.5, fs), { off (r.cursor + secs (0.3, fs), 69) });
        const double f = zcHz (y2, secs (0.05, fs), secs (0.28, fs), fs);
        const double tail = rmsOf (y2, secs (0.45, fs), secs (0.5, fs));
        std::printf ("  NOTE3 fix=%d guard=%d: allocs in flood block %d; pitch after flood %.2f Hz (expect %.2f = +2 st); tail rms after note-off %.2e\n",
                     (int) kFix, (int) guardOn, a, f, 440.0 * std::pow (2.0, 2.0 / 12.0), tail);
        // Mono flood too
        Rig m (fs, 512, with (base (BankFactory::sineSaw, 0.0f), ids::voiceMode, 1.0f));
       #ifdef FIXBUILD
        m.proc->setMidiCapacityGuardForTesting (guardOn);
       #endif
        m.countAllocs = true; m.run (512);
        const int c0 = m.cursor;
        std::vector<Ev> e2 { on (c0, 69) };
        for (int i = 0; i < 6000; ++i) e2.push_back (wheel (c0 + 100, 8192 + (i % 2)));
        e2.push_back (wheel (c0 + 101, 16383));
        allocs = 0; m.run (512, e2); const int a2 = allocs; m.countAllocs = false;
        auto ym = m.run (secs (0.3, fs));
        std::printf ("  NOTE3-MONO fix=%d guard=%d: allocs %d; pitch %.2f Hz\n", (int) kFix, (int) guardOn, a2, zcHz (ym, secs (0.05, fs), secs (0.28, fs), fs));
    }

    //==========================================================================
    // note 4: processBlock before prepareToPlay, in a forked child.
    void unprepared (bool guardOn)
    {
        fflush (stdout);
        const pid_t pid = fork();
        if (pid == 0)
        {
            auto p = std::make_unique<Proc>();
           #ifdef FIXBUILD
            p->setPreparedGuardForTesting (guardOn);
           #else
            (void) guardOn;
           #endif
            p->setPlayConfigDetails (0, 2, 48000.0, 512);
            juce::AudioBuffer<float> b (2, 512);
            for (int c = 0; c < 2; ++c) juce::FloatVectorOperations::fill (b.getWritePointer (c), 1.0f, 512);
            juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            p->processBlock (b, m);
            p->processBlock (b, m);
            float mx = 0; for (int c = 0; c < 2; ++c) mx = std::max (mx, b.getMagnitude (c, 0, 512));
            p->prepareToPlay (48000.0, 512);                          // then a normal life
            juce::MidiBuffer m2; m2.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            p->processBlock (b, m2); p->processBlock (b, m2);
            const float live = b.getMagnitude (0, 0, 512);
            _exit (mx == 0.0f ? (live > 0.01f ? 0 : 4) : 3);
        }
        int st = 0; waitpid (pid, &st, 0);
        if (WIFSIGNALED (st)) std::printf ("  NOTE4 fix=%d guard=%d: child KILLED by signal %d\n", (int) kFix, (int) guardOn, WTERMSIG (st));
        else std::printf ("  NOTE4 fix=%d guard=%d: child exit %d (0 = silent while unprepared, sound after prepare)\n", (int) kFix, (int) guardOn, WEXITSTATUS (st));
    }

    //==========================================================================
    void n4 (bool hookOn)
    {
        Rig r (48000.0, 512, base (BankFactory::sineSaw, 0.3f));
       #ifdef FIXBUILD
        r.proc->setReleaseClearsDisplayForTesting (hookOn);
       #else
        (void) hookOn;
       #endif
        r.run (9600, { on (0, 64) });
        const bool before = r.proc->isDisplaySounding();
        r.proc->releaseResources();
        std::printf ("  N4 fix=%d hook=%d: sounding before release %d, after release %d, note %d, hz %.1f, amp %.2f\n", (int) kFix, (int) hookOn,
                     (int) before, (int) r.proc->isDisplaySounding(), r.proc->getDisplayNote(), r.proc->getDisplayHz(), r.proc->getDisplayAmpEnv());
    }

    //==========================================================================
    // Regression renders for the orig-vs-fix null comparison.
    void dump (const char* path)
    {
        const double fs = 48000.0;
        std::ofstream o (path, std::ios::binary);
        auto put = [&] (const std::string& name, const std::vector<float>& y)
        {
            const int nl = (int) name.size(), n = (int) y.size();
            o.write ((const char*) &nl, 4); o.write (name.data(), nl); o.write ((const char*) &n, 4); o.write ((const char*) y.data(), (std::streamsize) (n * 4));
        };
        auto def = [] { return Params { { ids::outputLevel, 0.0f } }; };
        { Rig r (fs, 512, def()); put ("default_chord", r.run (secs (1.5, fs), { on (0, 48), on (0, 55), on (0, 64), on (0, 71), off (secs (1.0, fs), 48), off (secs (1.0, fs), 55), off (secs (1.0, fs), 64), off (secs (1.0, fs), 71) })); }
        { Rig r (fs, 512, base (0, 0.5f)); auto a = r.run (secs (0.3, fs), { on (0, 45), on (10, 57) }); r.set (ids::bank, 4.0f); auto b = r.run (secs (0.3, fs)); r.set (ids::interp, 0.0f); auto c = r.run (secs (0.2, fs)); r.set (ids::bandlimit, 0.0f); auto d = r.run (secs (0.2, fs));
          a.insert (a.end(), b.begin(), b.end()); a.insert (a.end(), c.begin(), c.end()); a.insert (a.end(), d.begin(), d.end()); put ("switches_bank_interp_bl", a); }
        { Rig r (fs, 512, with (base (0, 0.0f), ids::voiceMode, 1.0f)); put ("mono_legato_same_level_60_64", r.run (secs (1.0, fs), { on (0, 60), on (secs (0.2, fs), 64), off (secs (0.5, fs), 64), off (secs (0.8, fs), 60) })); }
        { Rig r (fs, 512, with (base (0, 0.0f), ids::voiceMode, 1.0f)); put ("mono_retrig_48", r.run (secs (0.8, fs), { on (0, 48), off (secs (0.5, fs), 48), on (secs (0.6, fs), 48, 38) })); }
        { Rig r (fs, 512, base (0, 0.0f)); std::vector<Ev> e; for (int i = 0; i < 17; ++i) e.push_back (on (i * 64, 36 + i)); put ("steal17", r.run (secs (0.8, fs), e)); }
        { Rig r (fs, 512, base (0, 0.0f)); auto a = r.run (512 * 40, { on (0, 48), on (0, 55) }); r.set (ids::voiceMode, 1.0f); auto b = r.run (secs (0.3, fs), { on (secs (0.05, fs) + 512 * 40, 60) }); a.insert (a.end(), b.begin(), b.end()); put ("poly_to_mono_no_wheel", a); }
        { Rig r (fs, 512, with (with (with (base (3, 0.5f), ids::lfoShape, 4.0f), ids::lfoDepth, 1.0f), ids::lfoRate, 3.0f)); put ("lfo_sh", r.run (secs (1.0, fs), { on (0, 57) })); }
        { Rig r (fs, 512, with (base (4, 1.0f), ids::voiceMode, 1.0f)); put ("mono_legato_crossing_65_67", r.run (secs (1.0, fs), { on (0, 65), on (secs (0.5, fs), 67) })); }
        { Rig r (fs, 512, base (4, 1.0f)); std::vector<Ev> e { on (0, 66) }; for (int i = 0; i < 40; ++i) e.push_back (wheel (secs (0.2, fs) + i * 480, 8192 + i * 200)); put ("poly_wheel_sweep_crossing", r.run (secs (0.8, fs), e)); }
        { Rig r (fs, 512, with (base (4, 1.0f), ids::voiceMode, 1.0f)); put ("mono_legato_60_96", r.run (secs (0.6, fs), { on (0, 60), on (secs (0.25, fs), 96) })); }
    }
}

namespace
{
    // W4 fold residual: legato 60 -> 96, then a bank switch FOLDS 128 samples later.
    void w4fold (bool keepRate)
    {
        const double fs = 48000.0;
        for (int L : { -1, (int) std::lround (2.0 * fs) })
        {
            Rig r (fs, 64, with (base (BankFactory::drive, 1.0f), ids::voiceMode, 1.0f));
            if (L > 0) r.proc->setXfadeLenOverrideForTesting (L);
           #ifdef FIXBUILD
            r.proc->setXfadeKeepRateForTesting (keepRate);
           #else
            (void) keepRate;
           #endif
            const int t = 64 * 188;
            auto y = r.run (t + 128, { on (0, 60), on (t, 96) });
            r.set (ids::bank, 0.0f);
            auto y2 = r.run (secs (1.6, fs));
            y.insert (y.end(), y2.begin(), y2.end());
            if (L < 0)
            {
                double st = 0, fd = 0;
                for (int i = t + 2400; i < t + 7200; ++i) st = std::max (st, (double) std::abs (y[(size_t) i] - y[(size_t) i - 1]));
                for (int i = t - 32; i < t + 2400; ++i) fd = std::max (fd, (double) std::abs (y[(size_t) i] - y[(size_t) i - 1]));
                std::printf ("  W4-FOLD real 5 ms fix=%d keep=%d: max|dy| in fades / steady new = %.3f\n", (int) kFix, (int) keepRate, fd / st);
            }
            else
                std::printf ("  W4-FOLD magnifier fix=%d keep=%d: inharmonic %.1f dB\n", (int) kFix, (int) keepRate,
                             inharmonicDb (y, t + secs (0.35, fs), juce::MidiMessage::getMidiNoteInHertz (60), fs));
        }
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::SharedResourcePointer<BuiltInBanks> banks;
    audioThread = pthread_self(); malloc_logger = hook;

    if (argc > 1 && std::string (argv[1]) == "--fold") { w4fold (true); if (kFix) w4fold (false); return 0; }
    if (argc > 2 && std::string (argv[1]) == "--dump") { dump (argv[2]); malloc_logger = nullptr; return 0; }

    std::printf ("== build: %s\n", kFix ? "src-fix" : "Source (shipped)");
    w3 (true);
    if (kFix) w3 (false);
    w4 (true);
    if (kFix) w4 (false);
    flood (true);
    if (kFix) flood (false);
    unprepared (true);
    if (kFix) unprepared (false);
    n4 (true);
    if (kFix) n4 (false);
    malloc_logger = nullptr;
    return 0;
}
