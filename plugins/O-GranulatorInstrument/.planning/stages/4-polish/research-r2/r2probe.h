// R2 research probe (scratch only).
#pragma once
namespace r2p
{
    using eg21::Session;
    using eg21::P;
    constexpr double kFs = 48000.0;

    inline double envD (const char* k, double d) { const char* v = std::getenv (k); return v ? std::atof (v) : d; }
    inline void cfgFromEnv()
    {
        auto& m = ogi::gMk;
        m.on = envD ("OGI_MK", 1) != 0;
        m.tauS = envD ("OGI_MK_TAU", 1.0);
        m.capDb = (float) envD ("OGI_MK_CAP", 40.0);
        m.oneSided = envD ("OGI_MK_ONESIDED", 1) != 0;
        m.peakLim = envD ("OGI_MK_PK", 1) != 0;
        m.slewDb = (float) envD ("OGI_MK_SLEW", 0.25);
        m.pkRelS = envD ("OGI_MK_PKREL", 0.05);
        m.kappa = envD ("OGI_MK_KAPPA", 1) != 0;
        m.pkOver = (float) std::pow (10.0, envD ("OGI_MK_PKOVER", 0.0) / 20.0);
        m.stCeil = envD ("OGI_MK_ST", 1) != 0;
        m.tailLatch = envD ("OGI_MK_TAIL", 1) != 0;
        m.stX = (float) std::pow (10.0, envD ("OGI_MK_STX", 6.0) / 10.0);
        m.stRelS = envD ("OGI_MK_STREL", 0.05);
        std::printf ("st %d stX %.2f stRel %.3f\n", (int) m.stCeil, m.stX, m.stRelS);
        std::printf ("MK on %d tau %.2f cap %.1f oneSided %d peakLim %d slew %.2f pkRel %.2f kappa %d\n",
                     (int) m.on, m.tauS, m.capDb, (int) m.oneSided, (int) m.peakLim, m.slewDb, m.pkRelS, (int) m.kappa);
    }

    inline double db (double ms) { return 10.0 * std::log10 (std::max (ms, 1e-30)); }

    struct Lane { double ms = 0, pk = 0; double lapS = 0; };
    inline Lane lane (Session& s, int k)
    {
        Lane o;
        const auto* t = s.eng().getCurrentTape (k);
        if (t == nullptr) return o;
        const auto& hd = s.eng().getHeadForTest (k);
        std::int64_t lo = hd.lo, hi = hd.hi;
        if (hi <= lo) { lo = 0; hi = t->frames; }
        double acc = 0;
        for (std::int64_t i = lo; i < hi; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                const double v = t->tape.levelSample (0, ch, i);
                acc += v * v;
                o.pk = std::max (o.pk, std::abs (v));
            }
        o.ms = acc / (2.0 * (double) (hi - lo));
        o.lapS = (double) (hi - lo) / kFs;
        return o;
    }

    inline double winDb (const std::vector<float>& L, const std::vector<float>& R, double t0, double t1)
    {
        const size_t a = (size_t) (t0 * kFs), b = std::min ({ (size_t) (t1 * kFs), L.size(), R.size() });
        double acc = 0;
        for (size_t i = a; i < b; ++i) acc += (double) L[i] * L[i] + (double) R[i] * R[i];
        return db (b > a ? acc / (2.0 * (double) (b - a)) : 0.0);
    }
    inline double bufDb (const std::vector<float>& v, double t0, double t1)
    {
        const size_t a = (size_t) (t0 * kFs) * 2, b = std::min ((size_t) (t1 * kFs) * 2, v.size());
        double acc = 0;
        for (size_t i = a; i < b; ++i) acc += (double) v[i] * v[i];
        return db (b > a ? acc / (double) (b - a) : 0.0);
    }
    /** Spectral centroid (Hz) of the mono sum over [t0, t1), by zero-crossing-free first-difference estimate:
        fc ~ fs/(2 pi) * sqrt (E[dx^2] / E[x^2]) (the RMS frequency). */
    inline double rmsHz (const std::vector<float>& L, const std::vector<float>& R, double t0, double t1)
    {
        const size_t a = (size_t) (t0 * kFs) + 1, b = std::min ({ (size_t) (t1 * kFs), L.size(), R.size() });
        double e = 0, d = 0;
        for (size_t i = a; i < b; ++i)
        {
            const double x = 0.5 * (L[i] + R[i]), x1 = 0.5 * (L[i - 1] + R[i - 1]);
            e += x * x; d += (x - x1) * (x - x1);
        }
        return e > 0 ? kFs / (2.0 * 3.14159265358979) * std::sqrt (d / e) : 0.0;
    }

    struct Out { juce::String line; };

    /** Runs `s` (already set up, notes scheduled) for `seconds`; reports slot k. */
    inline void measure (Session& s, const juce::String& name, const std::vector<int>& slots, double seconds, const char* fileStem = nullptr)
    {
        const int k0 = slots.empty() ? 0 : slots[0];
        const std::int64_t N = (std::int64_t) (seconds * kFs);
        std::vector<float> tap ((size_t) N * 2, 0.0f), ret ((size_t) N * 2, 0.0f);
        s.hk().tapProbe = { tap.data(), N, k0 };
        s.hk().retProbe = { ret.data(), N, k0 };
        const double marks[] = { 0.0, 2.0, 5.0, 10.0, 20.0, 30.0, 45.0, 60.0 };
        std::vector<std::vector<Lane>> ln (slots.size());
        s.run (64, 32);
        double tNow = 64.0 / kFs;
        for (double m : marks)
        {
            if (m > seconds) break;
            const std::int64_t tgt = std::max<std::int64_t> ((std::int64_t) (m * kFs), s.pos);
            if (tgt > s.pos) s.run (tgt - s.pos, 512);
            for (size_t j = 0; j < slots.size(); ++j) ln[j].push_back (lane (s, slots[j]));
            tNow = m;
        }
        if (s.pos < N) s.run (N - s.pos, 512);
        s.hk().tapProbe = {}; s.hk().retProbe = {};
        std::printf ("%s\n", name.toRawUTF8());
        for (size_t j = 0; j < slots.size(); ++j)
        {
            const int k = slots[j];
            const auto& hd = s.eng().getHeadForTest (k);
            std::printf ("   slot %d lap %.2f s  lane dB:", k + 1, ln[j].empty() ? 0.0 : ln[j][0].lapS);
            for (const auto& l : ln[j]) std::printf (" %6.1f", db (l.ms));
            std::printf ("  | d60 %+.1f | pk %.3f->%.3f", ln[j].empty() ? 0.0 : db (ln[j].back().ms) - db (ln[j][0].ms),
                         ln[j].empty() ? 0.0 : ln[j][0].pk, ln[j].empty() ? 0.0 : ln[j].back().pk);
            const auto& mk = hd.mk;
            std::printf (" | g mean %+.1f dB max %+.1f cap %.0f%% pkl %.0f%% st %.0f%% | rClamps %llu writes %llu\n",
                         mk.cells ? 20.0 * mk.gLogSum / (double) mk.cells : 0.0, 20.0 * std::log10 (std::max (mk.gMax, 1e-9f)),
                         mk.cells ? 100.0 * (double) mk.capCells / (double) mk.cells : 0.0, mk.cells ? 100.0 * (double) mk.pkCells / (double) mk.cells : 0.0, mk.cells ? 100.0 * (double) mk.stCells / (double) mk.cells : 0.0,
                         (unsigned long long) s.hk().head[k].rClamps, (unsigned long long) s.hk().head[k].writes);
        }
        const double e = std::min (seconds, 60.0);
        std::printf ("   slot %d tap dB [5,10] %.1f [%.0f,%.0f] %.1f | ret dB %.1f / %.1f | out dB [5,10] %.1f [25,30] %.1f [%.0f,%.0f] %.1f | out rmsHz [5,10] %.0f [%.0f,%.0f] %.0f\n",
                     k0 + 1, bufDb (tap, 5, 10), e - 5, e, bufDb (tap, e - 5, e), bufDb (ret, 5, 10), bufDb (ret, e - 5, e),
                     winDb (s.L, s.R, 5, 10), winDb (s.L, s.R, 25, 30), e - 5, e, winDb (s.L, s.R, e - 5, e),
                     rmsHz (s.L, s.R, 5, 10), e - 5, e, rmsHz (s.L, s.R, e - 5, e));
        if (fileStem != nullptr)
        {
            std::vector<float> v (s.L); v.insert (v.end(), s.R.begin(), s.R.end());
            juce::File (juce::String (fileStem) + ".f32").replaceWithData (v.data(), v.size() * sizeof (float));
        }
        std::fflush (stdout);
    }

    inline void presets (double seconds)
    {
        const auto& b = pg4::bank();
        std::vector<int> idx;
        for (int i = 0; i < (int) b.specs.size(); ++i)
        {
            const auto& sp = b.specs[(size_t) i];
            bool any = false;
            for (int n = 1; n <= 4; ++n)
                any = any || (pg4::streamOn (sp, n) && pg4::sval (sp, n, "feedback", 0.0f) > 0.0f && pg4::sval (sp, n, "fb_enable", 1.0f) > 0.5f);
            if (any) idx.push_back (i);
        }
        std::printf ("presets with feedback: %d\n", (int) idx.size());
        std::vector<juce::String> outs (idx.size());
        for (size_t q = 0; q < idx.size(); ++q)
        {
            const int i = idx[q];
            const auto& sp = b.specs[(size_t) i];
            pg4::PSess ps;
            ps.apply (i);
            std::vector<int> slots;
            juce::String nm = juce::String (sp.name) + "  [";
            for (int n = 1; n <= 4; ++n)
                if (pg4::streamOn (sp, n) && pg4::sval (sp, n, "feedback", 0.0f) > 0.0f && pg4::sval (sp, n, "fb_enable", 1.0f) > 0.5f)
                {
                    slots.push_back (n - 1);
                    nm << "s" << n << " fb " << pg4::sval (sp, n, "feedback", 0.0f) << " sat " << pg4::sval (sp, n, "fb_saturation", 0.0f)
                       << (pg4::sval (sp, n, "reverb_pre", 0.0f) > 0.5f ? " PRE" : "") << "; ";
                }
            nm << "]" << (pg4::val (sp, "freeze", 0.0f) > 0.5f ? " FREEZE" : "");
            ps.s->noteOn (64, 48, 100);
            ps.s->noteOn (64, 55, 100);
            ps.s->noteOn ((std::int64_t) (1.5 * kFs), 60, 90);
            measure (*ps.s, nm, slots, seconds);
        }
    }

    inline void synth (double seconds)
    {
        struct C { const char* name; float fb, sat; int pre; float mix; int src; float amp; };
        const C cs[] = {
            { "2.4-1 noise fb100 sat0",   100, 0,   0, 0,  0, 0.25f }, { "2.4-1 noise fb100 sat20",  100, 20,  0, 0,  0, 0.25f },
            { "2.4-1 noise fb100 sat100", 100, 100, 0, 0,  0, 0.25f }, { "noise fb50 sat0",          50,  0,   0, 0,  0, 0.25f },
            { "noise fb25 sat0",          25,  0,   0, 0,  0, 0.25f }, { "noise fb10 sat0",          10,  0,   0, 0,  0, 0.25f },
            { "noise fb100 sat0 PRE mix50",   100, 0,   1, 50,  0, 0.25f }, { "noise fb100 sat20 PRE mix100 size100", 100, 20, 1, 100, 0, 0.25f },
            { "noise fb100 sat100 PRE mix100 size100", 100, 100, 1, 100, 0, 0.25f },
            { "sparse fb100 sat0",        100, 0,   0, 0,  1, 0.5f },  { "sparse fb50 sat0",         50,  0,   0, 0,  1, 0.5f },
            { "hot noise 0.9 fb100 sat0", 100, 0,   0, 0,  0, 0.9f },  { "quiet noise 0.003 fb100 sat0", 100, 0, 0, 0, 0, 0.003f },
            { "sine220 0.5 fb100 sat20",  100, 20,  0, 0,  2, 0.5f },
        };
        const double only = envD ("OGI_ONLY", -1);
        int ci = 0;
        for (const auto& c : cs)
        {
            if (only >= 0 && (int) only != ci++) continue;
            P p { { "s1_feedback", c.fb }, { "s1_fb_saturation", c.sat }, { "s1_reverb_pre", (float) c.pre }, { "s1_reverb_mix", c.mix },
                  { "s1_reverb_size", c.pre && c.mix >= 100 ? 100.0f : 50.0f } };
            Session s (kFs, p, eg21::kSeed, 512);
            eg22::shipping (s);
            const int n = (int) (4.0 * kFs);
            juce::AudioBuffer<float> buf;
            if (c.src == 0) buf = eg21::noiseBuf (2, n, c.amp, 5);
            else if (c.src == 2) buf = eg21::sineBuf (2, n, 220.0, kFs, c.amp);
            else
            {
                buf = eg21::noiseBuf (2, n, c.amp, 5);   // 4 hits of 100 ms, 900 ms gaps
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < n; ++i)
                    {
                        const int ph = i % 48000;
                        const float e = ph < 4800 ? std::exp (-(float) ph / 1200.0f) : 0.0f;
                        buf.getWritePointer (ch)[i] *= e;
                    }
            }
            s.load (0, buf, kFs);
            for (int note : { 48, 55, 60 }) s.noteOn (64, note, 100);
            measure (s, c.name, { 0 }, seconds);
        }
    }

    /** Block-size invariance + determinism of a feedback render with the makeup on. */
    inline int inv()
    {
        auto render = [] (int block, int pre)
        {
            P p { { "s1_feedback", 100.0f }, { "s1_fb_saturation", 20.0f }, { "s1_reverb_pre", (float) pre }, { "s1_reverb_mix", pre ? 50.0f : 0.0f },
                  { "s1_spray", 30.0f }, { "s1_scan_rate", 8.0f }, { "s1_scan_mode", 2.0f } };
            Session s (kFs, p, eg21::kSeed, 512);
            eg22::shipping (s);
            s.load (0, eg21::noiseBuf (2, (int) (4.0 * kFs), 0.1f, 5), kFs);
            for (int note : { 48, 55, 60 }) s.noteOn (64, note, 100);
            s.run ((std::int64_t) (12.0 * kFs), block);
            std::vector<float> v (s.L); v.insert (v.end(), s.R.begin(), s.R.end());
            const auto* t = s.eng().getCurrentTape (0);
            return std::make_pair (juce::SHA256 (v.data(), v.size() * sizeof (float)).toHexString().substring (0, 16), t ? t->tape.hashForTest() : 0);
        };
        for (int pre = 0; pre < 2; ++pre)
        {
            const auto a = render (512, pre), b = render (32, pre), c = render (0, pre), d = render (512, pre), e = render (1, pre);
            std::printf ("%s: 512 %s/%llx | 32 %s | var %s | 512 again %s | 1 %s -> %s\n", pre ? "PRE" : "POST", a.first.toRawUTF8(), (unsigned long long) a.second,
                         b.first.toRawUTF8(), c.first.toRawUTF8(), d.first.toRawUTF8(), e.first.toRawUTF8(),
                         (a == b && a == c && a == d && a == e) ? "ALL EQUAL (output + tape hash)" : "DIFFER");
        }
        return 0;
    }

    /** Second synthetic set: gate candidates. */
    inline void synthB (double seconds)
    {
        struct C { const char* name; float fb, sat; int pre; float mix; int src; float amp; float lowCut, highCut; float loopEnd; double offAt; };
        const C cs[] = {
            { "G noise0.1 fb100 sat0",    100, 0,   0, 0,  0, 0.1f, 80, 12000, 100, -1 }, { "G noise0.1 fb100 sat20",  100, 20,  0, 0,  0, 0.1f, 80, 12000, 100, -1 },
            { "G noise0.1 fb100 sat100",  100, 100, 0, 0,  0, 0.1f, 80, 12000, 100, -1 },
            { "G noise0.1 fb100 sat0 PRE",   100, 0,   1, 50, 0, 0.1f, 80, 12000, 100, -1 }, { "G noise0.1 fb100 sat20 PRE", 100, 20, 1, 50, 0, 0.1f, 80, 12000, 100, -1 },
            { "G noise0.1 fb100 sat100 PRE", 100, 100, 1, 50, 0, 0.1f, 80, 12000, 100, -1 },
            { "G noise0.1 fb50 sat20",    50, 20,  0, 0,  0, 0.1f, 80, 12000, 100, -1 }, { "G noise0.1 fb25 sat20",    25, 20,  0, 0,  0, 0.1f, 80, 12000, 100, -1 },
            { "G noise0.1 fb10 sat20",    10, 20,  0, 0,  0, 0.1f, 80, 12000, 100, -1 },
            { "G crossed filters fb100 (lowcut 2000 / highcut 1000)", 100, 20, 0, 0, 0, 0.1f, 2000, 1000, 100, -1 },
            { "G zero tape fb100",        100, 20,  0, 0,  3, 0.0f, 80, 12000, 100, -1 },
            { "G floor tape 1e-5 fb100",  100, 20,  0, 0,  0, 1.0e-5f, 80, 12000, 100, -1 },
            { "G two-level tape, region = quiet half (0..50 %)", 100, 20, 0, 0, 4, 0.2f, 80, 12000, 50, -1 },
            { "G notes off at 20 s, POST", 100, 20, 0, 0,  0, 0.1f, 80, 12000, 100, 20.0 },
            { "G notes off at 20 s, PRE mix100 size100", 100, 20, 1, 100, 0, 0.1f, 80, 12000, 100, 20.0 },
        };
        const double only = envD ("OGI_ONLY", -1);
        int ci = 0;
        for (const auto& c : cs)
        {
            if (only >= 0 && (int) only != ci++) continue;
            P p { { "s1_feedback", c.fb }, { "s1_fb_saturation", c.sat }, { "s1_reverb_pre", (float) c.pre }, { "s1_reverb_mix", c.mix },
                  { "s1_reverb_size", c.pre && c.mix >= 100 ? 100.0f : 50.0f }, { "s1_fb_low_cut", c.lowCut }, { "s1_fb_high_cut", c.highCut },
                  { "s1_loop_end", c.loopEnd } };
            Session s (kFs, p, eg21::kSeed, 512);
            eg22::shipping (s);
            const int n = (int) (4.0 * kFs);
            juce::AudioBuffer<float> buf = eg21::noiseBuf (2, n, c.src == 3 ? 0.0f : c.amp, 5);
            if (c.src == 4)   // first half 20 dB down
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < n / 2; ++i)
                        buf.getWritePointer (ch)[i] *= 0.1f;
            s.load (0, buf, kFs);
            for (int note : { 48, 55, 60 })
            {
                s.noteOn (64, note, 100);
                if (c.offAt > 0) s.noteOff ((std::int64_t) (c.offAt * kFs), note);
            }
            measure (s, c.name, { 0 }, seconds);
        }
    }

    /** Note-off on real presets: the lane after the release / tail. */
    inline void noteOff()
    {
        const auto& b = pg4::bank();
        for (const char* want : { "Iron Sky", "Deep Well", "Organ Loft", "Slow Tide", "Tape Halo", "Fifth Bloom" })
            for (int i = 0; i < (int) b.specs.size(); ++i)
            {
                const auto& sp = b.specs[(size_t) i];
                if (! juce::String (sp.name).contains (want)) continue;
                pg4::PSess ps;
                ps.apply (i);
                ps.s->record = false;
                int slot = 0;
                for (int n = 4; n >= 1; --n)
                    if (pg4::streamOn (sp, n) && pg4::sval (sp, n, "feedback", 0.0f) > 0.0f) slot = n - 1;
                for (int note : { 48, 55, 60 }) { ps.s->noteOn (64, note, 100); ps.s->noteOff ((std::int64_t) (20.0 * kFs), note); }
                ps.s->run (64, 32);
                const double l0 = db (lane (*ps.s, slot).ms);
                std::printf ("%-12s slot %d (release %.0f ms) lane dB re t0 at 10 / 20 / 25 / 30 / 40 / 60 s:", want, slot + 1, pg4::val (sp, "amp_release", 0.0f));
                double tprev = 0;
                for (double t : { 10.0, 20.0, 25.0, 30.0, 40.0, 60.0 })
                {
                    ps.s->run ((std::int64_t) ((t - tprev) * kFs), 512); tprev = t;
                    std::printf (" %+.1f", db (lane (*ps.s, slot).ms) - l0);
                }
                std::printf (" | writes %.1f s\n", (double) ps.s->hk().head[slot].writes / kFs);
                std::fflush (stdout);
            }
    }

    /** Long runs: drift. */
    inline void longRun()
    {
        const auto& b = pg4::bank();
        for (const char* want : { "Iron Sky", "Tape Halo", "Fifth Bloom", "Dotted Echo" })
            for (int i = 0; i < (int) b.specs.size(); ++i)
            {
                const auto& sp = b.specs[(size_t) i];
                if (! juce::String (sp.name).contains (want)) continue;
                pg4::PSess ps;
                ps.apply (i);
                ps.s->record = false;
                int slot = 0;
                for (int n = 4; n >= 1; --n)
                    if (pg4::streamOn (sp, n) && pg4::sval (sp, n, "feedback", 0.0f) > 0.0f) slot = n - 1;
                ps.s->noteOn (64, 48, 100); ps.s->noteOn (64, 55, 100); ps.s->noteOn (72000, 60, 90);
                ps.s->run (64, 32);
                const double l0 = db (lane (*ps.s, slot).ms);
                std::printf ("%s slot %d lane dB re t0, every 30 s to 600 s:", want, slot + 1);
                double mn = 0, mx = 0;
                for (int k = 1; k <= 20; ++k)
                {
                    ps.s->run ((std::int64_t) (30.0 * kFs), 512);
                    const auto l = lane (*ps.s, slot);
                    const double d = db (l.ms) - l0;
                    mn = std::min (mn, d); mx = std::max (mx, d);
                    std::printf (" %+.1f", d);
                }
                std::printf ("  | min %+.1f max %+.1f | pk %.3f\n", mn, mx, lane (*ps.s, slot).pk);
                std::fflush (stdout);
            }
    }

    inline int run()
    {
        cfgFromEnv();
        const double sec = envD ("OGI_SEC", 60.0);
        const int what = (int) envD ("OGI_WHAT", 3);
        if (what & 1) synth (sec);
        if (what & 2) presets (sec);
        if (what & 4) synthB (sec);
        if (what & 8) longRun();
        if (what & 16) noteOff();
        return 0;
    }
}
