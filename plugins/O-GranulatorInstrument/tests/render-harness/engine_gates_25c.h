/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.5b engine gates,
    part c (PLAN-2.5b T11): voice modes, glide, the D32 switch, sustain.
    Included by main.cpp AFTER engine_gates_25b.h (uses eg22 / eg24b / eg25a).

    No parent equivalent (O-Granulator @ 82406fce is an effect): law gates only.

      VM1   Mono retrigger: new serial; amp + Mod Env level continuous; sigma 0;
            LFO = lfoN_phase; NR redrawn; velocity source 0.9     ctrl VM1n ampResetZero
      VM2   amp gain linear over 5 ms to g_v (0.9) (<= 1e-6)    ctrl VM2n gainSnap
      VM3   Legato: serial / env / NR / velocity / LFO bitwise == a single-note
            render; note + glide + Key moved                    ctrl VM3n legatoAsMono
      VM4   no key held / release tail: same-slot retrigger from the current level;
            no cloud: fresh voice from 0 (both modes)
      VM5   Mono retrigger, coherence on: lawGain / gFbLerp untouched  ctrl VM5n retrigViaStartVoice
      VM6   per-voice LFO -> level + vel -> pan: per-cell step <= 1.05 x 20 ms bound
                                                                 ctrl VM6n noPvResidual
      FK1   D30 fallback (Mono + Legato): glide to C4, serial kept, env unchanged
                                                                 ctrl FK1n fallbackRetrig
      FK2   3 keys: non-top release = stack only; last release releases the cloud
      FK3   Legato off+on at one sample, both host orders, blocks 32 / 4096 (PD16)
      GL1   Mono glide 300 ms C4 -> C6: every spawn's semis == closed form <= 1e-9
                                                                 ctrl GL1n glideStep
      GL2   1 st vs 24 st: identical N
      GL3   retarget at 40 %: continuous at t, new N from there
      GL4   Poly glide-from-last
      GL5   glide 0: no glide state, exact jump
      GL6   law tracks a 2-octave glide every tick              ctrl GL6n pvKeyOff
      VS1   16 Poly voices -> Mono: newest kept, 15 forced, stack == physical keys
      VS2   Mono -> Poly: the cloud continues bitwise; new notes allocate
      VS3   Mono <-> Legato with a held cloud: bitwise == no switch
      VS4   PD5 detached cloud
      SU1   deferred note-off: release at the pedal-up sample (+-0)
      SU2   victim order: sustained = "no key down", not "released"
      SU3   PD9 re-strike: release with tail; other channel untouched
      SU4   Mono under CC64: fallback skips sustained keys; gate holds to pedal-up
      SU5   CC121 / CC123 / CC120 under the pedal
      SU6   polyphony 16 -> 4 + CC64 + Poly -> Mono in one tick: no stuck voice, no NaN

  ==============================================================================
*/
#pragma once

namespace { namespace eg25c
{
    using eg21::Session;
    using eg21::P;
    using eg21::with;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::noiseBuf;
    using eg21::kSeed;
    using eg21::setP;
    using eg21::sameD;
    using eg21::sameF;
    using eg21::diffCount;
    using eg22::cat;
    using eg22::sn;
    using eg22::shipping;
    using eg22::setAt;
    using eg24b::want;
    using eg24b::section;
    using eg25a::route;
    using eg25a::baseMod;
    using eg25a::dS;
    using eg25a::loadN;
    namespace S = eg25a::S;
    using F = ogi::GiDest::Field;

    constexpr double kFs = 48000.0;
    constexpr int kRampN = 240;   // 5 ms @ 48 k (kStealMs)

    //==========================================================================
    // Helpers
    P vmBase (int mode, float glideMs, int poly = 16)
    {
        return cat ({ baseMod (1),
                      { { "voice_mode", (float) mode }, { "glide", glideMs }, { "mpe_enable", 0.0f }, { "polyphony", (float) poly },
                        { sn (1, "density"), 200.0f }, { sn (1, "size"), 40.0f },
                        { "amp_attack", 200.0f }, { "amp_decay", 300.0f }, { "amp_sustain", 50.0f }, { "amp_release", 300.0f },
                        { "vel_to_amp", 70.0f },
                        { "menv1_attack", 200.0f }, { "menv1_decay", 300.0f }, { "menv1_sustain", 50.0f }, { "menv1_release", 300.0f },
                        { "lfo1_mode", 1.0f }, { "lfo1_rate", 3.0f }, { "lfo1_phase", 90.0f } } });
    }

    juce::MidiMessage on (int ch, int note, int vel = 100) { return juce::MidiMessage::noteOn (ch, note, (juce::uint8) vel); }
    juce::MidiMessage off (int ch, int note)               { return juce::MidiMessage::noteOff (ch, note); }
    juce::MidiMessage cc (int ch, int n, int v)            { return juce::MidiMessage::controllerEvent (ch, n, v); }

    int cloud (Session& s) { return s.eng().getMonoCloudForTest(); }

    /** The non-free voice slot holding (channel, note); −1 when none. */
    int voiceOf (Session& s, int note, int ch = 1, bool countedOnly = true)
    {
        int best = -1;
        for (int k = 0; k < ogi::kVoiceSlots; ++k)
        {
            const auto& v = s.eng().getVoice (k);
            if ((countedOnly ? v.isCounted() : ! v.isFree()) && v.note == note && v.channel == ch
                && (best < 0 || v.serial > s.eng().getVoice (best).serial))
                best = k;
        }
        return best;
    }

    double semisOf (const ogi::GrainLogEntry& e, double tapeRate) { return 12.0 * std::log2 (e.rate * kFs / tapeRate); }

    int countState (Session& s, int st)   // 0 free, 1 counted, 2 stolen
    {
        int n = 0;
        for (int k = 0; k < ogi::kVoiceSlots; ++k)
        {
            const auto& v = s.eng().getVoice (k);
            n += (st == 0 ? v.isFree() : st == 1 ? v.isCounted() : v.isStolen()) ? 1 : 0;
        }
        return n;
    }

    /** Runs until absolute sample t (blocks of `block`, last one shortened). */
    void runTo (Session& s, juce::int64 t, int block = 32)
    {
        if (t > s.pos)
            s.run (t - s.pos, block);
    }

    //==========================================================================
    // VM1 / VM2 / VM3 — a per-sample probe across the second note-on at t1.
    struct Snap
    {
        double amp = 0.0, menv = 0.0; float gain = 0.0f; std::uint64_t serial = 0; int note = 0;
        float lfo = 0.0f, nr = 0.0f, vel = 0.0f, key = 0.0f; double scanSec = -1.0; juce::int64 scanT0 = -1;
        int stage = 0; bool gated = false;
    };

    Snap snapOf (Session& s, int k)
    {
        Snap o;
        if (k < 0) return o;
        const auto& v = s.eng().getVoice (k);
        const auto& m = s.eng().getVoiceModForTest (k);
        o.amp = v.env.getLevel(); o.menv = m.env[0].getLevel(); o.gain = v.env.getGain(); o.serial = v.serial; o.note = v.note;
        o.lfo = m.src[S::Lfo1]; o.nr = m.src[S::Rand]; o.vel = m.src[S::Vel]; o.key = m.src[S::Key];
        o.scanSec = v.scan[0].sec; o.scanT0 = v.scan[0].t0;
        o.stage = (int) v.env.getStage(); o.gated = v.gated;
        return o;
    }

    struct VmProbe
    {
        std::vector<Snap> per;          // per[i] = after processing samples up to t1 − 64 + i (pos = t1 − 64 + i)
        Snap atOn0;                      // just after the first note-on sample
        juce::int64 t1 = 0;
        int k = -1;
        std::vector<float> L, R;
        Snap at (juce::int64 pos) const { const auto i = pos - (t1 - 64); return (i >= 0 && i < (juce::int64) per.size()) ? per[(size_t) i] : Snap {}; }
    };

    VmProbe vmRun (int mode, bool secondNote, std::function<void (Session&)> hooks = {}, float glideMs = 0.0f, int vel1 = 51, int vel2 = 114,
                   int note2 = 64)
    {
        Session s (kFs, vmBase (mode, glideMs), kSeed, 32);
        shipping (s);
        if (hooks) hooks (s);
        loadN (s, 1);
        VmProbe o;
        const juce::int64 t0 = 1000;
        o.t1 = 30017;
        s.addMidi (t0, on (1, 60, vel1));
        if (secondNote) s.addMidi (o.t1, on (1, note2, vel2));
        runTo (s, t0);
        s.run (1, 1);
        o.atOn0 = snapOf (s, cloud (s));
        runTo (s, o.t1 - 64);
        s.afterBlock = [&o] (Session& ss) { o.per.push_back (snapOf (ss, cloud (ss))); };
        o.per.push_back (snapOf (s, cloud (s)));
        s.run (64 + kRampN + 64, 1);
        s.afterBlock = nullptr;
        o.k = cloud (s);
        runTo (s, o.t1 + 9600, 512);
        o.L = s.L; o.R = s.R;
        return o;
    }

    struct Vm1 { bool ok = false; double dAmp = 0, dMenv = 0; juce::String detail; };

    Vm1 vm1Check (const VmProbe& o)
    {
        Vm1 r;
        const Snap b = o.at (o.t1), a = o.at (o.t1 + 1);   // before / after the sample t1
        r.dAmp  = std::abs (a.amp - b.amp);
        r.dMenv = std::abs (a.menv - b.menv);
        const bool newSerial = a.serial != b.serial && a.serial > b.serial;
        const bool scan0 = a.scanSec == 0.0 && a.scanT0 == o.t1;
        const bool lfo = sameF (a.lfo, o.atOn0.lfo);
        const bool nr = ! sameF (a.nr, o.atOn0.nr);
        const bool vel = sameF (a.vel, 114.0f / 127.0f);
        r.ok = newSerial && r.dAmp <= 1.0e-3 && r.dMenv <= 1.0e-3 && scan0 && lfo && nr && vel;
        r.detail = "serial " + i64 (b.serial) + " -> " + i64 (a.serial) + ", |d amp| " + juce::String (r.dAmp, 6) + ", |d menv| " + juce::String (r.dMenv, 6)
                 + ", sigma " + f2 (a.scanSec, 3) + " @ " + i64 ((std::uint64_t) a.scanT0) + ", lfo1 " + (lfo ? "== phase" : "!= phase")
                 + ", NR " + (nr ? "redrawn" : "SAME") + ", vel " + f2 (a.vel, 4);
        return r;
    }

    void rowsVM()
    {
        // VM1 / VM1n / VM2 / VM2n
        {
            const auto o = vmRun (1, true);
            const auto c = vmRun (1, true, [] (Session& s) { s.hk().ampResetZero = true; });
            const auto r = vm1Check (o), rc = vm1Check (c);
            row (r.ok, "VM1 D29 Mono retrigger: new serial, amp + Mod Env from the current level, sigma 0, LFO phase, NR, velocity",
                 "meas " + r.detail + " | thr |d| <= 1e-3, exact");
            row (! rc.ok && rc.dAmp > 1.0e-3, "VM1n control ampResetZero: the amp env restarts from 0 -> continuity fails",
                 "meas |d amp| " + juce::String (rc.dAmp, 6) + ", |d menv| " + juce::String (rc.dMenv, 6) + " | " + ctrl (rc.dAmp > 1.0e-3, "> 1e-3"));

            auto gainErr = [] (const VmProbe& p)
            {
                const float g0 = p.at (p.t1).gain;
                const float g1 = ogi::AmpEnv::velocityGain (114.0f / 127.0f, 0.7f);
                double worst = 0.0;
                for (int i = 1; i <= kRampN + 32; ++i)
                {
                    const double want = i >= kRampN ? (double) g1 : (double) g0 + ((double) g1 - (double) g0) * (double) i / (double) kRampN;
                    worst = std::max (worst, std::abs ((double) p.at (p.t1 + i).gain - want));
                }
                return std::make_pair (worst, (double) g1 - (double) g0);
            };
            const auto e = gainErr (o);
            const auto ec = gainErr (vmRun (1, true, [] (Session& s) { s.hk().gainSnap = true; }));
            row (e.first <= 1.0e-6 && std::abs (e.second) > 0.1, "VM2 PD1 Mono amp gain glides linearly over 5 ms to g_v (0.9)",
                 "meas worst |g - linear| " + juce::String (e.first, 9) + " over 272 samples (gain step " + f2 (e.second, 4) + ") | thr <= 1e-6");
            row (ec.first > 1.0e-6, "VM2n control gainSnap: the velocity gain steps",
                 "meas worst " + juce::String (ec.first, 6) + " | " + ctrl (ec.first > 1.0e-6, "> 1e-6"));
        }
        // VM3 / VM3n — Legato vs a single-note render
        {
            auto legatoOk = [] (const VmProbe& o, const VmProbe& ref, juce::String& d)
            {
                bool same = true;
                for (juce::int64 pos = o.t1 - 63; pos <= o.t1 + kRampN + 60; ++pos)
                {
                    const Snap a = o.at (pos), b = ref.at (pos);
                    same = same && a.serial == b.serial && sameD (a.amp, b.amp) && sameD (a.menv, b.menv) && sameF (a.gain, b.gain)
                                && sameF (a.nr, b.nr) && sameF (a.vel, b.vel) && sameF (a.lfo, b.lfo) && a.scanT0 == b.scanT0;
                }
                const Snap a = o.at (o.t1 + 40);   // after the next tick
                const bool moved = a.note == 64 && sameF (a.key, 4.0f / 64.0f);
                d = juce::String ("serial / env / gain / NR / velocity / LFO / scan ") + (same ? "bitwise == single note" : "DIFFER")
                  + ", note " + juce::String (a.note) + ", Key " + f2 (a.key, 4);
                return same && moved;
            };
            const auto o = vmRun (2, true, {}, 100.0f), ref = vmRun (2, false, {}, 100.0f);
            const auto c = vmRun (2, true, [] (Session& s) { s.hk().legatoAsMono = true; }, 100.0f);
            juce::String d, dc;
            const bool ok = legatoOk (o, ref, d), okc = legatoOk (c, ref, dc);
            row (ok, "VM3 D29 Legato: same serial, nothing retriggers; only note / glide / Key move (PD3)", "meas " + d + " | thr bitwise");
            row (! okc, "VM3n control legatoAsMono: the Legato note-on retriggers", "meas " + dc + " | " + ctrl (! okc, "differs"));
        }
        // VM4 — no key held: same-slot retrigger from the current level; no cloud: fresh voice
        {
            Tally t;
            for (int mode : { 1, 2 })
            {
                Session s (kFs, vmBase (mode, 0.0f), kSeed, 32);
                loadN (s, 1);
                s.addMidi (1000, on (1, 60, 100));
                s.addMidi (24000, off (1, 60));
                s.addMidi (30017, on (1, 64, 100));   // release tail (300 ms release)
                s.addMidi (30500, off (1, 64));
                s.addMidi (200017, on (1, 67, 100));  // the cloud has ended
                runTo (s, 30017);
                const int k0 = cloud (s);
                const Snap b = snapOf (s, k0);
                s.run (1, 1);
                const int k1 = cloud (s);
                const Snap a = snapOf (s, k1);
                const bool tail = k0 >= 0 && ! b.gated && b.amp > 0.01;
                t.check (tail && k1 == k0 && a.serial > b.serial && std::abs (a.amp - b.amp) <= 1.0e-3 && a.gated,
                         "mode " + juce::String (mode) + " tail retrigger (slot " + juce::String (k0) + "->" + juce::String (k1) + ", amp "
                             + f2 (b.amp, 4) + "->" + f2 (a.amp, 4) + ")");
                runTo (s, 200017);
                const bool none = cloud (s) < 0;
                s.run (1, 1);
                const Snap f = snapOf (s, cloud (s));
                t.check (none && f.amp < 1.0e-3 && f.serial > a.serial, "mode " + juce::String (mode) + " fresh voice (amp " + juce::String (f.amp, 6) + ")");
            }
            row (t.bad == 0, "VM4 PD2 note-on with no key held: same-slot retrigger from the current level; no cloud: fresh voice from 0",
                 "meas " + t.detail (4) + " | thr exact");
        }
        // VM5 / VM5n — lawGain / gFbLerp untouched by a Mono retrigger
        {
            auto lawRun = [] (bool viaStart, bool& moved)
            {
                // Coherence is non-zero only within 0.5 st of the scan pitch (kCoherP0St): 48 (c = 1) -> 49 (c = 0).
                P p = with (vmBase (1, 0.0f), { { sn (1, "scan_rate"), 100.0f }, { sn (1, "spray"), 0.0f } });
                Session s (kFs, p, kSeed, 32);
                shipping (s);
                s.hk().retrigViaStartVoice = viaStart;
                loadN (s, 1);
                s.addMidi (1000, on (1, 48));
                s.addMidi (30010, on (1, 49));
                runTo (s, 30010);
                const int k = cloud (s);
                const auto b = s.eng().getLawLerpForTest (k, 0);
                s.run (1, 1);
                const auto a = s.eng().getLawLerpForTest (cloud (s), 0);
                moved = ! (sameF (a[0], b[0]) && sameF (a[1], b[1]) && sameF (a[2], b[2]) && sameF (a[3], b[3]));
                return juce::String ("lawGain ") + f2 (b[0], 6) + "/" + f2 (b[1], 6) + " -> " + f2 (a[0], 6) + "/" + f2 (a[1], 6)
                     + ", gFb " + f2 (b[2], 6) + "/" + f2 (b[3], 6) + " -> " + f2 (a[2], 6) + "/" + f2 (a[3], 6);
            };
            bool m0 = true, m1 = false;
            const auto d0 = lawRun (false, m0), d1 = lawRun (true, m1);
            row (! m0, "VM5 P2 Mono retrigger keeps lawGain / gFbLerp (no startVoice snap; coherence on)", "meas " + d0 + " | thr bitwise");
            row (m1, "VM5n control retrigViaStartVoice: the per-pair law gain snaps", "meas " + d1 + " | " + ctrl (m1, "changed"));
        }
        // VM6 / VM6n — per-voice level / pan residual
        {
            struct G { std::vector<std::array<float, 2>> g; };
            auto run6 = [] (bool noRes)
            {
                P p = cat ({ vmBase (1, 0.0f), { { "lfo1_rate", 2.0f }, { "lfo1_phase", 0.0f } },
                             route (1, S::Lfo1, dS (1, F::Level), 50.0f), route (2, S::Vel, dS (1, F::Pan), 80.0f) });
                Session s (kFs, p, kSeed, 32);
                s.hk().noPvResidual = noRes;
                loadN (s, 1);
                s.addMidi (1000, on (1, 60, 30));
                s.addMidi (30010 + 4800, on (1, 64, 120));   // the LFO near its peak at the retrigger
                G o;
                runTo (s, 30010 + 4800 - 32 * 800);   // a full LFO cycle (0.5 s) before: the natural per-cell motion
                s.afterBlock = [&o] (Session& ss) { o.g.push_back (ss.eng().getPairGainForTest (std::max (0, cloud (ss)), 0)); };
                s.run (32 * 920, 32);
                return o;
            };
            const auto r = run6 (false), c = run6 (true);
            auto maxD = [] (const G& g, size_t a, size_t b)
            {
                double m = 0.0;
                for (size_t i = std::max<size_t> (a, 1); i < b && i < g.g.size(); ++i)
                    for (size_t ch = 0; ch < 2; ++ch)
                        m = std::max (m, (double) std::abs (g.g[i][ch] - g.g[i - 1][ch]));
                return m;
            };
            const size_t e = 800;   // the retrigger opens block 800
            const double nat = maxD (r, 1, e - 1);
            const double stepN = maxD (c, e - 1, e + 3);
            const double B = nat + stepN / 30.0;   // the 20 ms residual over 30 cells
            const double sub = maxD (r, e - 1, r.g.size()), ctl = maxD (c, e - 1, c.g.size());
            const bool fired = ctl > 1.05 * B;
            row (sub <= 1.05 * B && fired, "VM6 PD4 Mono retrigger: per-voice LFO -> level + vel -> pan step within the 20 ms ramp bound",
                 "meas max per-cell |d gain| " + juce::String (sub, 6) + " (natural " + juce::String (nat, 6) + ", bound B " + juce::String (B, 6)
                     + ") | thr <= 1.05 B | " + ctrl (fired, "noPvResidual -> " + juce::String (ctl, 6)));
            row (fired, "VM6n control noPvResidual: a one-cell gain step", "meas " + juce::String (ctl, 6) + " | " + ctrl (fired, "> 1.05 B"));
        }
    }

    //==========================================================================
    // FK — D30 fallback (FB is taken by the 2.3 FX-state rows)
    void rowsFB()
    {
        auto fb1 = [] (int mode, bool retrig, juce::String& d)
        {
            Session s (kFs, vmBase (mode, 100.0f), kSeed, 32);
            s.hk().fallbackRetrig = retrig;
            loadN (s, 1);
            s.addMidi (1000, on (1, 60));
            s.addMidi (20000, on (1, 64));
            s.addMidi (40010, off (1, 64));
            runTo (s, 40010);
            const int k = cloud (s);
            const Snap b = snapOf (s, k);
            s.hk().resetCounters();
            s.run (1, 1);
            const int k2 = cloud (s);
            const Snap a = snapOf (s, k2);
            const auto& v = s.eng().getVoice (k2);
            const bool ok = k2 == k && a.serial == b.serial && a.note == 60 && a.stage == b.stage && std::abs (a.amp - b.amp) <= 1.0e-3
                         && v.glN > 0.0 && sameD (v.glTo, 60.0) && s.hk().fallbacks == 1;
            d += "mode " + juce::String (mode) + ": serial " + i64 (b.serial) + "->" + i64 (a.serial) + ", note " + juce::String (a.note)
               + ", stage " + juce::String (b.stage) + "->" + juce::String (a.stage) + ", fallbacks " + i64 (s.hk().fallbacks) + "; ";
            return ok;
        };
        juce::String d, dc;
        const bool ok = fb1 (1, false, d) && fb1 (2, false, d);
        const bool okc = fb1 (1, true, dc) && fb1 (2, true, dc);
        row (ok, "FK1 D30 fallback glides back to the held key, same serial, env unchanged (Mono + Legato)", "meas " + d + "| thr exact");
        row (! okc, "FK1n control fallbackRetrig: the fallback mints a new serial", "meas " + dc + "| " + ctrl (! okc, "serial changes"));

        // FK2
        {
            Tally t;
            Session s (kFs, vmBase (2, 0.0f), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (1, 60)); s.addMidi (2000, on (1, 64)); s.addMidi (3000, on (1, 67));
            s.addMidi (6000, off (1, 64)); s.addMidi (9000, off (1, 67)); s.addMidi (12000, off (1, 60));
            runTo (s, 5000);
            const auto ser = snapOf (s, cloud (s)).serial;
            runTo (s, 6100);
            t.check (s.eng().getStackForTest().size() == 2 && snapOf (s, cloud (s)).note == 67 && snapOf (s, cloud (s)).serial == ser, "non-top removal");
            runTo (s, 9100);
            t.check (snapOf (s, cloud (s)).note == 60 && snapOf (s, cloud (s)).serial == ser && snapOf (s, cloud (s)).gated, "fallback to the bottom key");
            runTo (s, 12100);
            t.check (s.eng().getStackForTest().empty() && ! snapOf (s, cloud (s)).gated, "last release releases the cloud");
            row (t.bad == 0, "FK2 D30 non-top release = stack only; the last release releases the cloud", "meas " + t.detail (3) + " | thr exact");
        }
        // FK3 — PD16 host order at one sample
        {
            Tally t;
            juce::String d3;
            for (bool offFirst : { true, false })
            {
                std::vector<float> outs[2];
                std::uint64_t serA[2] {}, serB[2] {};
                for (int bi = 0; bi < 2; ++bi)
                {
                    Session s (kFs, vmBase (2, 0.0f), kSeed, 32);
                    loadN (s, 1);
                    s.addMidi (1000, on (1, 60));
                    if (offFirst) { s.addMidi (24000, off (1, 60)); s.addMidi (24000, on (1, 64)); }
                    else          { s.addMidi (24000, on (1, 64));  s.addMidi (24000, off (1, 60)); }
                    const int blk = bi == 0 ? 32 : 4096;
                    runTo (s, 23000, blk);
                    serA[bi] = snapOf (s, cloud (s)).serial;
                    runTo (s, 25000, blk);
                    serB[bi] = snapOf (s, cloud (s)).serial;
                    runTo (s, 48000, blk);
                    outs[bi] = eg22::lr (s);
                }
                const bool expect = offFirst ? serB[0] != serA[0] : serB[0] == serA[0];
                const int nd = diffCount (outs[0], outs[1]);
                t.check (expect && serA[0] == serA[1] && serB[0] == serB[1] && nd == 0,
                         (offFirst ? "off-first" : "on-first") + juce::String (" serial ") + i64 (serA[0]) + "->" + i64 (serB[0]) + ", 32 vs 4096 diffs " + juce::String (nd));
                d3 += (offFirst ? "off-first: retrigger " : "on-first: legato ") + juce::String (serB[0] != serA[0] ? "(new serial)" : "(same serial)")
                   + ", 32 vs 4096 " + juce::String (nd) + " diffs; ";
            }
            row (t.bad == 0, "FK3 PD16 Legato off+on at one sample: host order decides, block-size invariant", "meas " + d3 + "| thr exact, 0 diffs");
        }
    }

    //==========================================================================
    // GL — glide
    struct GlOut { double worst = 0.0; int grains = 0; double N = 0.0; double tapeRate = 0.0; };

    GlOut glRun (int mode, int n1, int n2, float glideMs, bool step, juce::int64 t1 = 24000)
    {
        Session s (kFs, vmBase (mode, glideMs), kSeed, 32);
        s.hk().glideStep = step;
        loadN (s, 1);
        s.addMidi (1000, on (1, n1));
        s.addMidi (t1, on (1, n2));
        s.run (t1 + 24000, 512);
        GlOut o;
        o.tapeRate = s.hk().tapeRate[0];
        o.N = (double) s.eng().getGridParams().glideMs * 0.001 * kFs;
        const double root = (double) s.eng().getGridParams().s[0].rootNote;
        for (const auto& e : s.hk().logInOrder())
        {
            const double t = (double) e.onsetSample;
            double key = (double) n1;
            if (e.onsetSample >= t1)
            {
                const double x = o.N > 0.0 ? std::max (0.0, 1.0 - (t - (double) t1) / o.N) : 0.0;
                key = (double) n2 + ((double) n1 - (double) n2) * x;
            }
            o.worst = std::max (o.worst, std::abs (semisOf (e, o.tapeRate) - (key - root)));
            ++o.grains;
        }
        return o;
    }

    void rowsGL()
    {
        // GL1 / GL1n
        {
            const auto g = glRun (1, 60, 84, 300.0f, false), c = glRun (1, 60, 84, 300.0f, true);
            row (g.worst <= 1.0e-9 && g.grains > 150, "GL1 Mono glide 300 ms C4 -> C6: every spawn's semis == the closed form",
                 "meas worst " + juce::String (g.worst, 12) + " st over " + juce::String (g.grains) + " grains (N " + f2 (g.N, 1) + ") | thr <= 1e-9 st");
            row (c.worst > 1.0e-9, "GL1n control glideStep: the pitch jumps at t0", "meas worst " + f2 (c.worst, 4) + " st | " + ctrl (c.worst > 1.0e-9, "> 1e-9"));
        }
        // GL2 — constant time
        {
            auto nOf = [] (int n2)
            {
                Session s (kFs, vmBase (1, 300.0f), kSeed, 32);
                loadN (s, 1);
                s.addMidi (1000, on (1, 60));
                s.addMidi (24000, on (1, n2));
                runTo (s, 24100);
                const auto& v = s.eng().getVoice (cloud (s));
                return std::make_pair (v.glN, v.glT0);
            };
            const auto a = nOf (61), b = nOf (84);
            row (sameD (a.first, b.first) && a.second == b.second && a.first > 14000.0, "GL2 glide time is constant: 1 st and 24 st take the same N",
                 "meas N " + f2 (a.first, 3) + " / " + f2 (b.first, 3) + " samples, t0 " + i64 ((std::uint64_t) a.second) + " / " + i64 ((std::uint64_t) b.second) + " | thr bitwise");
        }
        // GL3 — retarget at 40 %
        {
            Session s (kFs, vmBase (2, 300.0f), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (1, 60));
            s.addMidi (24000, on (1, 84));
            const juce::int64 te = 24000 + (juce::int64) (0.4 * 300.0 * 48.0) + 7;
            s.addMidi (te, on (1, 67));
            runTo (s, te);
            const int k = cloud (s);
            const double before = s.eng().getVoiceSemisForTest (k, 0, te);
            s.run (1, 1);
            const double after = s.eng().getVoiceSemisForTest (cloud (s), 0, te);
            const auto& v = s.eng().getVoice (cloud (s));
            const double N = (double) s.eng().getGridParams().glideMs * 0.001 * kFs;
            const bool ok = std::abs (after - before) <= 1.0e-9 && v.glT0 == te && sameD (v.glN, N) && sameD (v.glTo, 67.0);
            row (ok, "GL3 retarget at 40 % of a glide: continuous at t, a new N from there",
                 "meas |key after - before| " + juce::String (std::abs (after - before), 12) + " st (key " + f2 (before + 48.0, 6) + "), t0 "
                     + i64 ((std::uint64_t) v.glT0) + ", N " + f2 (v.glN, 1) + " | thr <= 1e-9 st");
        }
        // GL4 — Poly glide-from-last
        {
            Session s (kFs, vmBase (0, 200.0f), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (1, 60));
            s.addMidi (24000, on (1, 67));
            runTo (s, 1001);
            const auto& a = s.eng().getVoice (voiceOf (s, 60));
            const bool firstNone = ! (a.glN > 0.0);
            runTo (s, 24000);
            s.run (1, 1);
            const int kb = voiceOf (s, 67);
            const auto& b = s.eng().getVoice (kb);
            const double key0 = s.eng().getVoiceSemisForTest (kb, 0, 24000);
            const bool ok = firstNone && b.glN > 0.0 && sameD (b.glFrom, 60.0) && sameD (b.glTo, 67.0) && std::abs (key0 - 12.0) <= 1.0e-9;
            row (ok, "GL4 Poly glide 200 ms: the first note has no glide; the second starts at the first's pitch",
                 "meas first glN " + f2 (a.glN, 1) + ", second from " + f2 (b.glFrom, 3) + " to " + f2 (b.glTo, 3) + ", key at t " + f2 (key0 + 48.0, 9) + " | thr exact");
        }
        // GL5 — glide 0
        {
            Session s (kFs, vmBase (1, 0.0f), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (1, 60));
            s.addMidi (24000, on (1, 64));
            bool untouched = true;
            s.afterBlock = [&untouched] (Session& ss)
            {
                const int k = cloud (ss);
                if (k < 0) return;
                const auto& v = ss.eng().getVoice (k);
                untouched = untouched && v.glN == 0.0 && v.glFrom == 0.0 && v.glTo == 0.0 && v.glT0 == 0;
            };
            s.run (48000, 256);
            s.afterBlock = nullptr;
            const double tr = s.hk().tapeRate[0];
            double worst = 0.0;
            int n = 0;
            for (const auto& e : s.hk().logInOrder())
            {
                const double want = e.onsetSample >= 24000 ? 16.0 : 12.0;
                worst = std::max (worst, std::abs (semisOf (e, tr) - want));
                ++n;
            }
            row (untouched && worst <= 1.0e-12 && n > 100, "GL5 glide 0: no glide state written; the spawn pitch jumps exactly",
                 "meas glide state " + juce::String (untouched ? "untouched" : "WRITTEN") + ", worst " + juce::String (worst, 14) + " st over " + juce::String (n) + " grains | thr exact");
        }
        // GL6 / GL6n — law tracks per-voice pitch
        {
            auto run6 = [] (bool pvOff, int& ticks, int& changed)
            {
                // The coherence pitch term is non-zero only within 0.5 st of the scan pitch
                // (kCoherP0St), so a 2-octave glide crosses it in ~9 ticks; a 2 s glide
                // 47 -> 49 keeps it live for ~1500 ticks (stimulus fix, recorded in SUMMARY).
                P p = with (vmBase (2, 2000.0f), { { sn (1, "scan_rate"), 100.0f }, { sn (1, "pitch_rand"), 0.0f }, { sn (1, "spray"), 0.0f } });
                Session s (kFs, p, kSeed, 32);
                shipping (s);
                s.hk().pvKeyOff = pvOff;
                loadN (s, 1);
                s.addMidi (1000, on (1, 47));
                s.addMidi (24000, on (1, 49));
                runTo (s, 24000);
                double worst = 0.0;
                float last = -1.0f;
                ticks = changed = 0;
                s.afterBlock = [&] (Session& ss)
                {
                    const int k = cloud (ss);
                    if (k < 0 || (ss.pos & 31) != 0 || ss.pos < 24000 + 64) return;   // the first tick precedes the note event
                    const float tgt = ss.eng().getLawTargetForTest (k, 0);
                    const float fresh = ss.eng().getLawFreshForTest (k, 0, ss.pos - 32);
                    worst = std::max (worst, (double) std::abs (tgt - fresh) / std::max (1.0e-9, (double) std::abs (fresh)));
                    ++ticks;
                    changed += (last >= 0.0f && ! sameF (tgt, last)) ? 1 : 0;
                    last = tgt;
                };
                s.run (96000 + 3200, 32);
                s.afterBlock = nullptr;
                return worst;
            };
            int t0 = 0, c0 = 0, t1 = 0, c1 = 0;
            const double w = run6 (false, t0, c0), wc = run6 (true, t1, c1);
            row (w <= 1.0e-6 && c0 > 100, "GL6 PD12 law key sees per-voice pitch: lawTarget == a fresh pairLaw every tick of a glide (47 -> 49, 2 s)",
                 "meas worst rel " + juce::String (w, 9) + " over " + juce::String (t0) + " ticks (" + juce::String (c0) + " target changes) | thr <= 1e-6 | "
                     + ctrl (wc > 1.0e-6, "pvKeyOff -> " + juce::String (wc, 6)));
            row (wc > 1.0e-6, "GL6n control pvKeyOff: the law goes stale during the glide", "meas worst rel " + juce::String (wc, 6) + " | " + ctrl (wc > 1.0e-6, "> 1e-6"));
        }
    }

    //==========================================================================
    // VS — D32 mode switch (MS is taken by the 2.4 mip-settle rows)
    void rowsMS()
    {
        // VS1
        {
            Session s (kFs, vmBase (0, 0.0f), kSeed, 32);
            loadN (s, 1);
            for (int i = 0; i < 16; ++i) s.addMidi (100 + 100 * i, on (1, 40 + i));
            const juce::int64 T = 48000;   // a tick
            setAt (s, T - 32, "voice_mode", 1.0f);   // latched at the next tick (T − 32 is itself a tick: the set lands before it)
            runTo (s, T - 32);
            std::uint64_t maxSer = 0;
            for (int k = 0; k < ogi::kVoiceSlots; ++k) maxSer = std::max (maxSer, s.eng().getVoice (k).serial);
            s.hk().resetCounters();
            s.run (32, 32);   // the tick at T − 32 latches Mono
            const int counted = countState (s, 1), stolen = countState (s, 2);
            const int kc = cloud (s);
            const auto& st = s.eng().getStackForTest();
            const bool stackOk = st.size() == 16 && st.top() != nullptr && st.top()->note == 55;
            runTo (s, T + 48000);
            const int nonFree = ogi::kVoiceSlots - countState (s, 0);
            const bool ok = counted == 1 && stolen == 15 && kc >= 0 && s.eng().getVoice (kc).serial == maxSer && stackOk && nonFree == 1
                         && s.hk().modeSwitches == 1;
            row (ok, "VS1 D32 16 Poly voices -> Mono at a tick: newest kept, 15 forced, 1 left after 1 s, stack == physical keys",
                 "meas counted " + juce::String (counted) + ", stolen " + juce::String (stolen) + ", kept serial " + i64 (kc >= 0 ? s.eng().getVoice (kc).serial : 0)
                     + " (max " + i64 (maxSer) + "), stack " + juce::String (st.size()) + ", non-free after 1 s " + juce::String (nonFree) + " | thr exact");
        }
        // VS2
        {
            auto run2 = [] (bool sw)
            {
                Session s (kFs, vmBase (1, 0.0f), kSeed, 32);
                loadN (s, 1);
                s.addMidi (1000, on (1, 60));
                if (sw)
                {
                    setAt (s, 24000, "voice_mode", 0.0f);
                    s.addMidi (36000, on (1, 64)); s.addMidi (36100, on (1, 67)); s.addMidi (36200, on (1, 71));
                }
                s.run (40000, 256);
                std::vector<float> pre (s.L.begin(), s.L.begin() + 36000);
                pre.insert (pre.end(), s.R.begin(), s.R.begin() + 36000);
                return std::make_pair (pre, countState (s, 1));
            };
            const auto a = run2 (true), b = run2 (false);
            const int nd = diffCount (a.first, b.first);
            row (nd == 0 && a.second == 4, "VS2 D32 Mono -> Poly: the cloud continues bitwise; new notes allocate",
                 "meas " + juce::String (nd) + " samples differ before the first new note, counted after 3 notes " + juce::String (a.second) + " | thr 0, 4");
        }
        // VS3
        {
            auto run3 = [] (bool sw)
            {
                Session s (kFs, vmBase (1, 100.0f), kSeed, 32);
                loadN (s, 1);
                s.addMidi (1000, on (1, 60));
                s.addMidi (2000, on (1, 64));
                if (sw) { setAt (s, 20000, "voice_mode", 2.0f); setAt (s, 30000, "voice_mode", 1.0f); setAt (s, 34000, "voice_mode", 2.0f); }
                s.addMidi (40000, on (1, 67));
                s.run (40000, 256);
                return eg22::lr (s);
            };
            const int nd = diffCount (run3 (true), run3 (false));
            row (nd == 0, "VS3 D32 Mono <-> Legato with a held cloud: bitwise == no switch until the next note event",
                 "meas " + juce::String (nd) + " samples differ | thr 0");
        }
        // VS4 — PD5 detached cloud
        {
            Tally t;
            Session s (kFs, vmBase (0, 0.0f), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (1, 60));       // held throughout (until 30000)
            s.addMidi (2000, cc (1, 64, 127));
            s.addMidi (3000, on (1, 64));
            s.addMidi (4000, off (1, 64));      // sustained (newest, no key down)
            setAt (s, 9600, "voice_mode", 1.0f);
            s.addMidi (20000, off (1, 60));     // the held key's release
            s.addMidi (30017, on (1, 67));
            runTo (s, 9700);
            const int kc = cloud (s);
            t.check (kc >= 0 && s.eng().getVoice (kc).note == 64 && s.eng().getDetachedForTest() && s.eng().getStackForTest().size() == 1,
                     "detached after the switch");
            const auto ser = kc >= 0 ? s.eng().getVoice (kc).serial : 0;
            runTo (s, 20100);
            t.check (cloud (s) == kc && s.eng().getVoice (kc).gated, "held-key release keeps the pedal-held cloud");
            runTo (s, 30017);
            s.run (1, 1);
            t.check (cloud (s) == kc && s.eng().getVoice (kc).serial > ser && s.eng().getVoice (kc).note == 67 && ! s.eng().getDetachedForTest(),
                     "next note-on = retrigger on the cloud slot");
            row (t.bad == 0, "VS4 PD5 detached cloud: held-key off keeps it; the next note-on retriggers the cloud's slot", "meas " + t.detail (3) + " | thr exact");
        }
    }

    //==========================================================================
    // SU — sustain
    void rowsSU()
    {
        // SU1
        {
            Session s (kFs, vmBase (0, 0.0f), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (1, 60));
            s.addMidi (2000, cc (1, 64, 127));
            s.addMidi (10000, off (1, 60));
            const juce::int64 tu = 10000 + 48000 + 5;
            s.addMidi (tu, cc (1, 64, 0));
            runTo (s, tu);
            const int k = voiceOf (s, 60);
            const int stB = (int) s.eng().getVoice (k).env.getStage();
            const bool gB = s.eng().getVoice (k).gated, kB = s.eng().getVoice (k).keyDown;
            s.run (1, 1);
            const int stA = (int) s.eng().getVoice (k).env.getStage();
            const int rel = (int) ogi::AmpEnv::Stage::release;
            row (stB != rel && gB && ! kB && stA == rel && s.hk().sustainDeferred >= 1,
                 "SU1 CC64: a note-off under the pedal releases at the pedal-up sample (+-0)",
                 "meas stage before " + juce::String (stB) + " (gated " + juce::String ((int) gB) + ", keyDown " + juce::String ((int) kB) + "), at the pedal-up sample "
                     + juce::String (stA) + " | thr release starts exactly at t");
        }
        // SU2
        {
            Session s (kFs, vmBase (0, 0.0f, 4), kSeed, 32);
            loadN (s, 1);
            s.addMidi (500, cc (1, 64, 127));
            s.addMidi (1000, on (1, 60));
            s.addMidi (2000, on (1, 58)); s.addMidi (2500, off (1, 58));   // sustained
            s.addMidi (3000, on (1, 62));
            s.addMidi (4000, on (1, 64));
            s.addMidi (6000, on (1, 66));                                    // at the limit (4)
            runTo (s, 5000);
            const auto ser58 = s.eng().getVoice (voiceOf (s, 58)).serial;
            s.hk().resetCounters();
            runTo (s, 6100);
            const auto stolen = s.hk().stolenCount > 0 ? s.hk().stolenSerials[0] : 0;
            row (stolen == ser58, "SU2 at the polyphony limit a sustained voice is 'no key down', not 'released' (victim order)",
                 "meas stolen serial " + i64 (stolen) + " (sustained 58 = " + i64 (ser58) + ") | thr == the sustained voice");
        }
        // SU3
        {
            Session s (kFs, vmBase (0, 0.0f), kSeed, 32);
            loadN (s, 1);
            s.addMidi (500, cc (1, 64, 127));
            s.addMidi (1000, on (1, 60)); s.addMidi (2000, off (1, 60));
            s.addMidi (1500, on (2, 60)); s.addMidi (2500, off (2, 60));
            s.addMidi (8000, on (1, 60));
            runTo (s, 7000);
            const int kOld = voiceOf (s, 60, 1), kOther = voiceOf (s, 60, 2);
            s.hk().resetCounters();
            runTo (s, 8100);
            const int rel = (int) ogi::AmpEnv::Stage::release;
            const int kNew = voiceOf (s, 60, 1);
            const bool ok = kOld >= 0 && kNew != kOld && (int) s.eng().getVoice (kOld).env.getStage() == rel && s.eng().getVoice (kOld).isCounted()
                         && s.eng().getVoice (kNew).gated && s.eng().getVoice (kNew).keyDown && s.hk().restrikeReleases == 1
                         && (int) s.eng().getVoice (kOther).env.getStage() != rel;
            row (ok, "SU3 PD9 re-strike of a sustained C4: the old voice releases with its tail, the new one is fresh; ch 2 untouched",
                 "meas old slot " + juce::String (kOld) + " stage " + juce::String ((int) s.eng().getVoice (kOld).env.getStage()) + ", new slot " + juce::String (kNew)
                     + ", restrikeReleases " + i64 (s.hk().restrikeReleases) + ", ch 2 stage " + juce::String ((int) s.eng().getVoice (kOther).env.getStage()) + " | thr exact");
        }
        // SU4
        {
            Tally t;
            Session s (kFs, vmBase (2, 0.0f), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (1, 60)); s.addMidi (1100, on (1, 64)); s.addMidi (1200, on (1, 67));
            s.addMidi (2000, cc (1, 64, 127));
            s.addMidi (3000, off (1, 64));    // non-top, sustained: leaves the stack
            s.addMidi (4000, off (1, 67));    // top: fallback skips 64
            s.addMidi (5000, off (1, 60));    // last key: gate holds (pedal)
            s.addMidi (9000, cc (1, 64, 0));
            runTo (s, 4100);
            t.check (snapOf (s, cloud (s)).note == 60, "fallback to 60 (not the sustained 64)");
            runTo (s, 5100);
            t.check (snapOf (s, cloud (s)).gated && ! s.eng().getVoice (cloud (s)).keyDown && s.eng().getStackForTest().empty(), "gate holds after the last key");
            runTo (s, 9000);
            s.run (1, 1);
            t.check (! snapOf (s, cloud (s)).gated && snapOf (s, cloud (s)).stage == (int) ogi::AmpEnv::Stage::release, "pedal-up releases");
            row (t.bad == 0, "SU4 Mono under CC64: the fallback skips sustained keys; the gate holds until pedal-up", "meas " + t.detail (3) + " | thr exact");
        }
        // SU5
        {
            Tally t;
            const int rel = (int) ogi::AmpEnv::Stage::release;
            {
                Session s (kFs, vmBase (0, 0.0f), kSeed, 32);
                loadN (s, 1);
                s.addMidi (500, cc (1, 64, 127)); s.addMidi (1000, on (1, 60)); s.addMidi (2000, off (1, 60));
                s.addMidi (6007, cc (1, 121, 0));
                runTo (s, 6007);
                const int k = voiceOf (s, 60);
                const int b = (int) s.eng().getVoice (k).env.getStage();
                s.run (1, 1);
                t.check (b != rel && (int) s.eng().getVoice (k).env.getStage() == rel && ! s.eng().getSustainForTest(), "CC121 lifts sustain at its sample");
            }
            {
                Session s (kFs, vmBase (1, 0.0f), kSeed, 32);
                loadN (s, 1);
                s.addMidi (500, cc (1, 64, 127)); s.addMidi (1000, on (1, 60)); s.addMidi (1100, on (1, 64));
                s.addMidi (6000, cc (1, 123, 0));
                runTo (s, 6100);
                t.check (! snapOf (s, cloud (s)).gated && s.eng().getStackForTest().empty(), "CC123 releases all (pedal ignored) + clears the stack");
            }
            {
                Session s (kFs, vmBase (1, 0.0f), kSeed, 32);
                loadN (s, 1);
                s.addMidi (500, cc (1, 64, 127)); s.addMidi (1000, on (1, 60)); s.addMidi (1100, on (1, 64));
                s.addMidi (6000, cc (1, 120, 0));
                runTo (s, 6100);
                t.check (countState (s, 1) == 0 && s.eng().getStackForTest().empty(), "CC120 forces + clears the stack");
            }
            row (t.bad == 0, "SU5 CC121 lifts sustain (pending offs now); CC123 releases all + clears the stack; CC120 forces + clears",
                 "meas " + t.detail (3) + " | thr exact");
        }
        // SU6
        {
            Session s (kFs, vmBase (0, 0.0f, 16), kSeed, 32);
            loadN (s, 1);
            s.addMidi (100, cc (1, 64, 127));
            for (int i = 0; i < 16; ++i) s.addMidi (200 + 50 * i, on (1, 40 + i));
            for (int i = 0; i < 16; i += 2) s.addMidi (3000 + 50 * i, off (1, 40 + i));   // half sustained
            const juce::int64 T = 9600;
            s.at (T, [] (Session& ss) { setP (*ss.p, "polyphony", 4.0f); setP (*ss.p, "voice_mode", 1.0f); });
            s.addMidi (T, cc (1, 64, 0));
            for (int i = 1; i < 16; i += 2) s.addMidi (20000 + 10 * i, off (1, 40 + i));
            int maxSlots = 0;
            s.afterBlock = [&maxSlots] (Session& ss) { maxSlots = std::max (maxSlots, ogi::kVoiceSlots - countState (ss, 0)); };
            s.run (20000 + 96000, 128);
            s.afterBlock = nullptr;
            int nan = 0;
            for (float x : s.L) nan += std::isfinite (x) ? 0 : 1;
            for (float x : s.R) nan += std::isfinite (x) ? 0 : 1;
            const int left = ogi::kVoiceSlots - countState (s, 0);
            row (maxSlots <= ogi::kVoiceSlots && left == 0 && nan == 0,
                 "SU6 polyphony 16 -> 4 + CC64 up + Poly -> Mono in one tick: no stuck voice after 2 s, no NaN",
                 "meas max non-free " + juce::String (maxSlots) + ", non-free after 2 s " + juce::String (left) + ", non-finite " + juce::String (nan) + " | thr <= 20, 0, 0");
        }
    }

    //==========================================================================
    void runAll()
    {
        std::printf ("\n== Phase 2.5b engine gates (eg25c): voice modes, glide, mode switch, sustain\n");
        if (want ("VM")) { section ("VM. Mono / Legato (D29)"); rowsVM(); }
        if (want ("FK")) { section ("FK. fallback (D30)"); rowsFB(); }
        if (want ("GL")) { section ("GL. glide"); rowsGL(); }
        if (want ("VS")) { section ("VS. mode switch (D32)"); rowsMS(); }
        if (want ("SU")) { section ("SU. sustain (CC64)"); rowsSU(); }
    }
}} // namespace ::eg25c
