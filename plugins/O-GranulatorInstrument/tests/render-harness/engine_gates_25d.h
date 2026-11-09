/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.5b engine gates,
    part d (PLAN-2.5b T12): MPE, VST3 note expression, clicks, block-size
    invariance, allocations, true bypass + --cpu25b. Included by main.cpp
    AFTER engine_gates_25c.h.

      MP1   member bend on ch 3 moves only its note (<= 1e-9 st); ch 2 / 4 grain
            logs bitwise == no sweep                            ctrl MP1n mpeBroadcast
      MP2   RPN 0 / 14-bit / NRPN / RPN null / RPN 0 on ch 1 (MidiRPNDetector)
      MP3   master + member + NE add (<= 1e-9 st)
      MP4   D31: master AT / CC74 + poly AT ignored under MPE; the same events move
            the sources with MPE off
      MP5   a new member note starts at its channel's bend / AT / CC74
      MP6   member pressure / slide move only their note; CC121 on ch 3 resets ch 3 only
      MP7   mpe_enable toggled with notes held: no step; next events follow the mode
      MP8   MCM: non-member channel; MCM resets the range to 48; MCM 0 != MPE off
      MR1-4 2.5a rows under MPE through member channels: source ranges, per-voice
            independence, newest-voice handover, allocation-order invariance
      NE1   C#4 + NE -50 c, root 60 -> every grain at +50 c (<= 0.01 c)
      NE1c  control: no NE / a mismatched noteId -> 50 c away
      NE2   a live NE change 1 s later, mid-block (441 / 4096): only later grains move
      NE2n  control neModuleTable: the live change is lost; one-pitch notes collide
      NE3   two notes on one pitch, different noteIds -> separate rates
      NE4   NE + Legato glides to the tuned pitch; D30 lands on the held key's tuning
      NE5   70 NE events in a block -> 64 applied, 6 dropped, 0 allocs; bypassed blocks never replay
      CK5a-g local kappa vs the static render <= 2.0x: Mono retrigger, Legato glide,
            D30 fallback, D32 switch, pedal-up, member bend sweep, Mono velocity change
      CK5an / dn / en / gn  hard controls (ampResetZero, modeSwitchHard, pedalUpHard,
            gainSnap) > 2.0x. CK5b / c / f: no constructible hard control (pitch is
            latched per grain) -> bound only (HANDS-ON-2.5b row 10)
      BS1   the full script at 32 / 441 / 512 / 4096 / var -> 0 diffs; BS1n seed + 1 differs
      AL1   0 allocations: BS1 script + 130 held keys + a 70-event NE block; liveness
      TB5   Poly, glide 0, MPE off, no CC64 -> renders-2.4 + legacy byte-identical
      TB6   MPE off with RPN / NRPN traffic parsed -> renders-2.4 byte-identical; the
            2.5a any-channel source rule unchanged

    --cpu25b (PD18): C25mpe INFO (C25m + MPE, 16 notes on 15 member channels,
    member bends sweeping at 0.5 Hz, Poly glide 300 ms retriggered every 500 ms).

  ==============================================================================
*/
#pragma once

namespace { namespace eg25d
{
    using eg21::Session;
    using eg21::P;
    using eg21::with;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::noiseBuf;
    using eg21::sineBuf;
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
    using eg24b::kappa;
    using eg24b::Out;
    using eg25a::route;
    using eg25a::baseMod;
    using eg25a::dS;
    using eg25a::loadN;
    using eg25c::on;
    using eg25c::off;
    using eg25c::cc;
    using eg25c::cloud;
    using eg25c::voiceOf;
    using eg25c::runTo;
    using eg25c::semisOf;
    namespace S = eg25a::S;
    using F = ogi::GiDest::Field;
    using Raw = juce::VST3ClientExtensions::Vst3RawEvent;

    constexpr double kFs = 48000.0;

    //==========================================================================
    // Helpers
    P mpBase (bool mpe = true, int mode = 0, float glideMs = 0.0f)
    {
        return cat ({ baseMod (1),
                      { { "voice_mode", (float) mode }, { "glide", glideMs }, { "mpe_enable", mpe ? 1.0f : 0.0f }, { "pitch_bend_range", 2.0f },
                        { sn (1, "density"), 100.0f }, { sn (1, "size"), 40.0f } } });
    }

    juce::MidiMessage pw (int ch, int v)  { return juce::MidiMessage::pitchWheel (ch, v); }
    juce::MidiMessage at (int ch, int v)  { return juce::MidiMessage::channelPressureChange (ch, v); }
    juce::MidiMessage pat (int ch, int n, int v) { return juce::MidiMessage::aftertouchChange (ch, n, v); }

    /** The engine's 14-bit bend conversion (Engine::applyEvent). */
    float bendOf (int v14) { const int d = v14 - 8192; return d >= 0 ? (float) d / 8191.0f : (float) d / 8192.0f; }

    void rpn (Session& s, juce::int64 t, int ch, int msb, int lsb, int dataMsb, int dataLsb = -1)
    {
        s.addMidi (t, cc (ch, 101, msb));
        s.addMidi (t, cc (ch, 100, lsb));
        s.addMidi (t, cc (ch, 6, dataMsb));
        if (dataLsb >= 0) s.addMidi (t, cc (ch, 38, dataLsb));
    }

    GiVst3Extensions& ext (Session& s) { return *static_cast<GiVst3Extensions*> (s.p->getVST3ClientExtensions()); }

    Raw rawOn (int ch1, int pitch, int id)
    {
        Raw r; r.kind = Raw::Kind::NoteOn; r.channel = (int16_t) (ch1 - 1); r.pitch = (int16_t) pitch; r.noteId = id; return r;
    }
    Raw rawNe (int id, double cents)
    {
        Raw r; r.kind = Raw::Kind::NoteExpressionValue; r.noteId = id; r.typeId = 2; r.value = 0.5 + cents / 100.0 / 240.0; return r;
    }

    /** Raw events at absolute samples, delivered with sampleOffset = t − blockStart. */
    struct NeSched
    {
        std::vector<std::pair<juce::int64, Raw>> ev;
        size_t cur = 0;
        void add (juce::int64 t, Raw r) { ev.push_back ({ t, r }); std::stable_sort (ev.begin(), ev.end(), [] (const auto& a, const auto& b) { return a.first < b.first; }); }
        void attach (Session& s)
        {
            s.beforeBlock = [this] (Session& ss, juce::int64 b0, int len)
            {
                while (cur < ev.size() && ev[cur].first < b0 + len)
                {
                    Raw r = ev[cur].second;
                    r.sampleOffset = (int32_t) std::max<juce::int64> (0, ev[cur].first - b0);
                    ext (ss).onVst3RawEvent (r);
                    ++cur;
                }
            };
        }
    };

    std::vector<ogi::GrainLogEntry> ofSerial (const std::vector<ogi::GrainLogEntry>& log, std::uint64_t ser)
    {
        std::vector<ogi::GrainLogEntry> o;
        for (const auto& e : log) if (e.serial == ser) o.push_back (e);
        return o;
    }

    bool sameGrains (const std::vector<ogi::GrainLogEntry>& a, const std::vector<ogi::GrainLogEntry>& b)
    {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (a[i].onsetSample != b[i].onsetSample || ! sameD (a[i].rate, b[i].rate) || ! sameD (a[i].len, b[i].len)
                || ! sameD (a[i].startPos, b[i].startPos) || ! sameF (a[i].gainL, b[i].gainL) || ! sameF (a[i].gainR, b[i].gainR))
                return false;
        return true;
    }

    //==========================================================================
    // MP — MPE
    void rowsMP()
    {
        // MP1 / MP1n
        {
            struct M1 { std::vector<ogi::GrainLogEntry> log; double tr = 0; std::vector<std::pair<juce::int64, float>> bends; };
            auto run1 = [] (bool sweep, bool broadcast)
            {
                Session s (kFs, mpBase(), kSeed, 32);
                s.hk().mpeBroadcast = broadcast;
                loadN (s, 1);
                s.addMidi (1000, on (2, 60)); s.addMidi (1100, on (3, 64)); s.addMidi (1200, on (4, 67));
                M1 o;
                if (sweep)
                    for (int i = 0; i < 90; ++i)
                    {
                        const juce::int64 t = 4800 + 480 * i + 7;
                        // +-2000 → |b| <= 0.244 → +-11.7 st: inside the +-48 st spawn clamp (16 + 48 b).
                        const int v = juce::jlimit (0, 16383, 8192 + (int) std::lround (2000.0 * std::sin (2.0 * juce::MathConstants<double>::pi * i / 40.0)));
                        s.addMidi (t, pw (3, v));
                        o.bends.push_back ({ t, bendOf (v) });
                    }
                s.run (48000, 512);
                o.log = s.hk().logInOrder();
                o.tr = s.hk().tapeRate[0];
                return o;
            };
            auto check = [] (const M1& sw, const M1& ref, double& worst, int& n3)
            {
                worst = 0.0; n3 = 0;
                for (const auto& e : ofSerial (sw.log, 2))
                {
                    float b = 0.0f;
                    for (const auto& bb : sw.bends) if (bb.first <= e.onsetSample) b = bb.second;
                    worst = std::max (worst, std::abs (semisOf (e, sw.tr) - (16.0 + (double) b * 48.0)));
                    ++n3;
                }
                return sameGrains (ofSerial (sw.log, 1), ofSerial (ref.log, 1)) && sameGrains (ofSerial (sw.log, 3), ofSerial (ref.log, 3));
            };
            const auto ref = run1 (false, false);
            double w = 0, wc = 0; int n = 0, nc = 0;
            const bool others = check (run1 (true, false), ref, w, n);
            const bool othersC = check (run1 (true, true), ref, wc, nc);
            row (others && w <= 1.0e-9 && n > 50, "MP1 DSP-06 MPE: a member bend sweep on ch 3 moves only its note; ch 2 / 4 grain logs bitwise unchanged",
                 "meas ch 3 worst " + juce::String (w, 12) + " st over " + juce::String (n) + " grains (memberRange 48), ch 2 / 4 " + (others ? "bitwise" : "DIFFER")
                     + " | thr <= 1e-9 st, bitwise");
            row (! othersC, "MP1n control mpeBroadcast: member writes reach every voice", "meas ch 2 / 4 " + juce::String (othersC ? "bitwise (vacuous)" : "moved") + " | "
                 + ctrl (! othersC, "differ"));
        }
        // MP2 — RPN table
        {
            Tally t;
            Session s (kFs, mpBase(), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (2, 60));
            s.addMidi (1000, pw (2, 16383));
            auto delta = [] (Session& ss) { const int k = voiceOf (ss, 60, 2); return ss.eng().getVoiceSemisForTest (k, 0, ss.pos) - 12.0; };
            runTo (s, 2000);
            t.check (std::abs (delta (s) - 48.0) <= 1.0e-9, "default 48 (" + f2 (delta (s), 6) + ")");
            rpn (s, 3000, 2, 0, 0, 12);
            runTo (s, 3100);
            t.check (std::abs (delta (s) - 12.0) <= 1.0e-9, "RPN 0 = 12 (" + f2 (delta (s), 6) + ")");
            s.addMidi (4000, cc (2, 38, 50));
            runTo (s, 4100);
            t.check (std::abs (delta (s) - 12.5) <= 1.0e-9, "14-bit LSB 50 (" + f2 (delta (s), 6) + ")");
            s.addMidi (5000, cc (2, 99, 0)); s.addMidi (5000, cc (2, 98, 0)); s.addMidi (5000, cc (2, 6, 30));
            runTo (s, 5100);
            t.check (std::abs (delta (s) - 12.5) <= 1.0e-9, "NRPN 0/0 ignored (" + f2 (delta (s), 6) + ")");
            rpn (s, 6000, 2, 127, 127, 40);
            runTo (s, 6100);
            t.check (std::abs (delta (s) - 12.5) <= 1.0e-9, "RPN null then CC 6 ignored (" + f2 (delta (s), 6) + ")");
            rpn (s, 7000, 1, 0, 0, 7);
            runTo (s, 7100);
            t.check (std::abs (delta (s) - 12.5) <= 1.0e-9, "RPN 0 on ch 1 ignored (" + f2 (delta (s), 6) + ")");
            t.check (s.hk().rpnMessages == 2, "rpnMessages " + i64 (s.hk().rpnMessages));
            row (t.bad == 0, "MP2 PD7 RPN 0 -> member range (default 48, 14-bit cents kept); NRPN / RPN null / RPN 0 on ch 1 ignored",
                 "meas " + t.detail (7) + " | thr <= 1e-9 st");
        }
        // MP3 — master + member + NE
        {
            Session s (kFs, mpBase(), kSeed, 32);
            loadN (s, 1);
            NeSched ne;
            ne.add (1000, rawOn (2, 61, 5));
            ne.add (1000, rawNe (5, -50.0));
            ne.attach (s);
            s.addMidi (1000, on (2, 61));
            s.addMidi (1100, pw (1, 12288));
            s.addMidi (1100, pw (2, 6144));
            runTo (s, 1200);
            const int k = voiceOf (s, 61, 2);
            const double got = s.eng().getVoiceSemisForTest (k, 0, s.pos);
            const double want = (61.0 - 48.0 + (double) bendOf (12288) * 2.0) + (double) bendOf (6144) * 48.0 + (-0.5);
            row (std::abs (got - want) <= 1.0e-9 && s.hk().neApplied == 1, "MP3 PD11 master bend x range + member bend x memberRange + NE cents add",
                 "meas key " + f2 (got + 48.0, 9) + " vs " + f2 (want + 48.0, 9) + " (err " + juce::String (std::abs (got - want), 12) + ") | thr <= 1e-9 st");
        }
        // MP4 — D31 ignores
        {
            auto run4 = [] (bool mpe)
            {
                Session s (kFs, mpBase (mpe), kSeed, 32);
                loadN (s, 1);
                s.addMidi (1000, on (2, 60)); s.addMidi (1100, on (3, 64));
                runTo (s, 2000);
                const int ka = voiceOf (s, 60, 2), kb = voiceOf (s, 64, 3);
                const float pa = s.eng().getVoiceModForTest (ka).src[S::Press], sa = s.eng().getVoiceModForTest (ka).src[S::Slide];
                const float pb = s.eng().getVoiceModForTest (kb).src[S::Press];
                s.addMidi (2000, at (1, 100)); s.addMidi (2000, cc (1, 74, 0)); s.addMidi (2000, pat (3, 64, 120));
                runTo (s, 2100);
                const bool same = sameF (pa, s.eng().getVoiceModForTest (ka).src[S::Press]) && sameF (sa, s.eng().getVoiceModForTest (ka).src[S::Slide])
                               && sameF (pb, s.eng().getVoiceModForTest (kb).src[S::Press]);
                return same;
            };
            const bool unchangedOn = run4 (true), unchangedOff = run4 (false);
            row (unchangedOn && ! unchangedOff, "MP4 D31 master AT / master CC74 / poly AT ignored under MPE (the same events move the sources with MPE off)",
                 "meas MPE on: sources " + juce::String (unchangedOn ? "bitwise unchanged" : "MOVED") + "; MPE off: " + (unchangedOff ? "unchanged (vacuous)" : "moved") + " | thr bitwise");
        }
        // MP5 — initial member values
        {
            Session s (kFs, mpBase(), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, pw (5, 12288)); s.addMidi (1000, at (5, 90)); s.addMidi (1000, cc (5, 74, 100));
            s.addMidi (2000, on (5, 60));
            runTo (s, 2000);
            s.run (1, 1);
            const int k = voiceOf (s, 60, 5);
            const auto& m = s.eng().getVoiceModForTest (k);
            const bool ok = sameF (m.src[S::Press], 90.0f / 127.0f) && sameF (m.src[S::Slide], 36.0f / 64.0f) && sameF (m.src[S::Bend], bendOf (12288))
                         && sameD (s.eng().getVoice (k).mpeSemis, (double) bendOf (12288) * 48.0);
            row (ok, "MP5 a new note on ch 5 starts at ch 5's bend / AT / CC74 (MPE initial values)",
                 "meas pressure " + f2 (m.src[S::Press], 6) + ", slide " + f2 (m.src[S::Slide], 6) + ", bend " + f2 (m.src[S::Bend], 6)
                     + ", member semis " + f2 (s.eng().getVoice (k).mpeSemis, 6) + " | thr exact");
        }
        // MP6 — member pressure / slide + CC121 per channel
        {
            Tally t;
            Session s (kFs, mpBase(), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (2, 60)); s.addMidi (1000, on (3, 64)); s.addMidi (1000, on (4, 67));
            s.addMidi (2000, at (2, 50)); s.addMidi (2000, at (3, 100)); s.addMidi (2000, cc (3, 74, 20));
            runTo (s, 2100);
            auto pr = [&s] (int note, int ch) { return s.eng().getVoiceModForTest (voiceOf (s, note, ch)).src[S::Press]; };
            auto sl = [&s] (int note, int ch) { return s.eng().getVoiceModForTest (voiceOf (s, note, ch)).src[S::Slide]; };
            t.check (sameF (pr (60, 2), 50.0f / 127.0f) && sameF (pr (64, 3), 100.0f / 127.0f) && sameF (pr (67, 4), 0.0f), "pressure per channel");
            t.check (sameF (sl (64, 3), -44.0f / 64.0f) && sameF (sl (60, 2), 0.0f) && sameF (sl (67, 4), 0.0f), "slide per channel");
            s.addMidi (3000, cc (3, 121, 0));
            runTo (s, 3100);
            t.check (sameF (pr (64, 3), 0.0f) && sameF (sl (64, 3), 0.0f) && sameF (pr (60, 2), 50.0f / 127.0f), "CC121 on ch 3 resets ch 3 only");
            row (t.bad == 0, "MP6 member pressure / slide move only their note; CC121 on ch 3 resets ch 3 only", "meas " + t.detail (3) + " | thr bitwise");
        }
        // MP7 — mpe_enable toggled with notes held
        {
            Tally t;
            Session s (kFs, mpBase(), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (2, 60)); s.addMidi (1000, on (3, 64));
            s.addMidi (2000, pw (2, 12288)); s.addMidi (2000, at (3, 77));
            const juce::int64 T = 9600;
            setAt (s, T - 64, "mpe_enable", 0.0f);
            runTo (s, T - 64);
            const int ka = voiceOf (s, 60, 2), kb = voiceOf (s, 64, 3);
            const double sa = s.eng().getVoiceSemisForTest (ka, 0, T), sb = s.eng().getVoiceSemisForTest (kb, 0, T);
            const float pb = s.eng().getVoiceModForTest (kb).src[S::Press], ba = s.eng().getVoiceModForTest (ka).src[S::Bend];
            runTo (s, T);
            t.check (! s.eng().getGridParams().mpeEnable, "MPE off latched");
            t.check (sameD (sa, s.eng().getVoiceSemisForTest (ka, 0, T)) && sameD (sb, s.eng().getVoiceSemisForTest (kb, 0, T))
                         && sameF (pb, s.eng().getVoiceModForTest (kb).src[S::Press]) && sameF (ba, s.eng().getVoiceModForTest (ka).src[S::Bend]),
                     "no step at the toggle");
            s.addMidi (T + 100, pw (2, 4096));   // MPE off: a channel bend moves every note (2.5a)
            runTo (s, T + 200);
            t.check (! sameD (sb, s.eng().getVoiceSemisForTest (kb, 0, T + 200)), "next event follows MPE off (ch 3 note moved by a ch 2 bend)");
            row (t.bad == 0, "MP7 PD8 mpe_enable toggled with notes held: no step; the next events follow the new mode", "meas " + t.detail (3) + " | thr bitwise");
        }
        // MP8 — MCM
        {
            Tally t;
            Session s (kFs, mpBase(), kSeed, 32);
            loadN (s, 1);
            rpn (s, 500, 2, 0, 0, 12);
            rpn (s, 1000, 1, 0, 6, 3);   // MCM = 3 → members ch 2–4
            s.addMidi (2000, on (6, 60));
            s.addMidi (2100, pw (6, 16383));
            runTo (s, 2200);
            t.check (s.eng().getMpeForTest().memberCount == 3 && sameD (s.eng().getMpeForTest().memberRange, 48.0), "MCM 3, range reset to 48");
            const int k6 = voiceOf (s, 60, 6);
            t.check (sameD (s.eng().getVoice (k6).mpeSemis, 0.0) && sameF (s.eng().getVoiceModForTest (k6).src[S::Bend], 0.0f), "ch 6 non-member");
            rpn (s, 3000, 1, 0, 6, 0);   // MCM 0
            s.addMidi (3100, on (2, 64));
            s.addMidi (3200, pw (2, 16383));
            runTo (s, 3300);
            const int k2 = voiceOf (s, 64, 2);
            t.check (sameD (s.eng().getVoice (k2).mpeSemis, 0.0) && s.eng().getGridParams().mpeEnable, "MCM 0: no member routing, MPE still on");
            row (t.bad == 0, "MP8 PD7 MCM: a channel above the zone is non-member; MCM resets the range to 48; MCM 0 != MPE off",
                 "meas " + t.detail (3) + " | thr exact");
        }
    }

    //==========================================================================
    // MR — 2.5a rows under MPE through member channels
    void rowsMR()
    {
        // MR1 — source ranges
        {
            Tally t;
            Session s (kFs, mpBase(), kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (2, 60));
            auto chk = [&] (juce::int64 time, const juce::MidiMessage& m, int src, float want, const juce::String& what)
            {
                s.addMidi (time, m);
                runTo (s, time + 64);
                const float got = s.eng().getVoiceModForTest (voiceOf (s, 60, 2)).src[src];
                t.check (sameF (got, want), what + " " + f2 (got, 6));
            };
            chk (2000, at (2, 127), S::Press, 1.0f, "pressure 127");
            chk (3000, cc (2, 74, 0), S::Slide, -1.0f, "slide 0");
            chk (4000, cc (2, 74, 127), S::Slide, 63.0f / 64.0f, "slide 127");
            chk (5000, pw (2, 16383), S::Bend, 1.0f, "bend max");
            chk (6000, pw (2, 0), S::Bend, -1.0f, "bend min");
            row (t.bad == 0, "MR1 DSP-06 source ranges through a member channel (pressure / slide / bend)", "meas " + t.detail (5) + " | thr exact");
        }
        // MR2 — per-voice independence (Pressure -> s1 size per voice)
        {
            P p = cat ({ mpBase(), route (1, S::Press, dS (1, F::Size), 50.0f) });
            Session s (kFs, p, kSeed, 32);
            loadN (s, 1);
            s.addMidi (1000, on (2, 60)); s.addMidi (1000, on (3, 64));
            runTo (s, 2000);
            const int ka = voiceOf (s, 60, 2), kb = voiceOf (s, 64, 3);
            const int d = dS (1, F::Size);
            const float a0 = s.eng().getVoiceModForTest (ka).offCurAt (d), b0 = s.eng().getVoiceModForTest (kb).offCurAt (d);
            s.addMidi (2000, at (2, 127));
            runTo (s, 2100);
            const float a1 = s.eng().getVoiceModForTest (ka).offCurAt (d), b1 = s.eng().getVoiceModForTest (kb).offCurAt (d);
            row (! sameF (a0, a1) && sameF (b0, b1), "MR2 member pressure modulates only its own voice (per-voice route)",
                 "meas voice ch 2 offset " + f2 (a0, 4) + " -> " + f2 (a1, 4) + ", voice ch 3 " + f2 (b0, 4) + " -> " + f2 (b1, 4) + " | thr bitwise for ch 3");
        }
        // MR3 — newest-voice handover with member pressure
        {
            Session s (kFs, mpBase(), kSeed, 32);
            loadN (s, 1);
            s.addMidi (500, at (2, 30)); s.addMidi (500, at (3, 100));
            s.addMidi (1000, on (2, 60));
            s.addMidi (5000, on (3, 64));
            runTo (s, 4900);
            const float g1 = s.eng().getMonoForTest (S::Press);
            runTo (s, 5000 + 4800);
            const float g2 = s.eng().getMonoForTest (S::Press);
            row (sameF (g1, 30.0f / 127.0f) && sameF (g2, 100.0f / 127.0f), "MR3 D4 newest-voice global value follows the newest note's member pressure",
                 "meas global Pressure " + f2 (g1, 6) + " (ch 2) -> " + f2 (g2, 6) + " (ch 3, after the 20 ms ramp) | thr exact");
        }
        // MR4 — allocation-order invariance under MPE
        {
            auto run4 = [] (int offset)
            {
                P p = cat ({ mpBase(), { { "lfo1_mode", 1.0f }, { "lfo1_rate", 2.0f } }, route (1, S::Press, dS (1, F::Size), 40.0f),
                             route (2, S::Bend, dS (1, F::Pan), 60.0f), route (3, S::Lfo1, dS (1, F::Level), 30.0f) });
                Session s (kFs, p, kSeed, 32);
                s.hk().setSlotOffset (offset);
                loadN (s, 1);
                for (int i = 0; i < 6; ++i) s.addMidi (1000 + 700 * i, on (2 + i, 55 + 2 * i));
                for (int i = 0; i < 40; ++i)
                {
                    s.addMidi (6000 + 600 * i, pw (2 + i % 6, (i * 977) % 16384));
                    s.addMidi (6100 + 600 * i, at (2 + (i + 3) % 6, (i * 37) % 128));
                }
                s.addMidi (20000, off (3, 57));
                s.addMidi (21000, on (3, 70));
                s.run (36000, 512);
                return eg22::lr (s);
            };
            const int nd = diffCount (run4 (0), run4 (7));
            row (nd == 0, "MR4 allocation-order invariance under MPE (slot offset 0 vs 7, member bend / AT routes)",
                 "meas " + juce::String (nd) + " samples differ | thr 0");
        }
    }

    //==========================================================================
    // NE — VST3 note expression (stamped path)
    double centsOf (const ogi::GrainLogEntry& e, double tr) { return 1200.0 * std::log2 (e.rate * kFs / tr); }

    void rowsNE()
    {
        auto neBase = [] () { return with (mpBase (false), { { sn (1, "root_note"), 60.0f } }); };
        // NE1 / NE1c
        {
            auto run1 = [&neBase] (bool sendNe, int neId, double& worst, int& n)
            {
                Session s (kFs, neBase(), kSeed, 32);
                loadN (s, 1);
                NeSched ne;
                ne.add (1000, rawOn (1, 61, 11));
                if (sendNe) ne.add (1000, rawNe (neId, -50.0));
                ne.attach (s);
                s.addMidi (1000, on (1, 61));
                s.run (24000, 512);
                const double tr = s.hk().tapeRate[0];
                worst = 0.0; n = 0;
                for (const auto& e : s.hk().logInOrder()) { worst = std::max (worst, std::abs (centsOf (e, tr) - 50.0)); ++n; }
            };
            double w = 0, wa = 0, wb = 0; int n = 0, na = 0, nb = 0;
            run1 (true, 11, w, n);
            run1 (false, 11, wa, na);
            run1 (true, 99, wb, nb);
            row (w <= 0.01 && n > 40, "NE1 D5 C#4 + NE -50 c at the note-on (root 60, kappa 100 %): every grain at +50 c",
                 "meas worst |cents - 50| " + juce::String (w, 6) + " over " + juce::String (n) + " grains | thr <= 0.01 c");
            const bool fired = wa >= 49.99 && wb >= 49.99;
            row (fired, "NE1c control: no NE / a mismatched noteId -> 50 c away (C#4 untouched)",
                 "meas no NE " + f2 (wa, 4) + " c away, noteId 99 " + f2 (wb, 4) + " c away | " + ctrl (fired, ">= 49.99 c"));
        }
        // NE2 / NE2n (+ NE3's collision under the module table)
        {
            struct R2 { double worstBefore = 0, worstAfter = 0; int nb = 0, na = 0; double collide = 0; };
            auto run2 = [&neBase] (int block, bool moduleTable)
            {
                Session s (kFs, neBase(), kSeed, 32);
                s.hk().neModuleTable = moduleTable;
                loadN (s, 1);
                NeSched ne;
                ne.add (1000, rawOn (1, 61, 11));
                ne.add (1000, rawNe (11, -50.0));
                const juce::int64 t2 = 1000 + 48000 + 441 + 17;
                ne.add (t2, rawNe (11, 20.0));
                ne.attach (s);
                s.addMidi (1000, on (1, 61));
                s.run (t2 + 24000, block);
                const double tr = s.hk().tapeRate[0];
                R2 o;
                for (const auto& e : s.hk().logInOrder())
                {
                    if (e.onsetSample < t2) { o.worstBefore = std::max (o.worstBefore, std::abs (centsOf (e, tr) - 50.0)); ++o.nb; }
                    else                    { o.worstAfter  = std::max (o.worstAfter,  std::abs (centsOf (e, tr) - 120.0)); ++o.na; }
                }
                return o;
            };
            const auto a = run2 (441, false), b = run2 (4096, false), c = run2 (441, true);
            const bool ok = a.worstBefore <= 0.01 && a.worstAfter <= 0.01 && b.worstBefore <= 0.01 && b.worstAfter <= 0.01 && a.na > 20 && a.nb > 20;
            row (ok, "NE2 D5 a live NE change 1 s after the note-on, mid-block: only grains spawned from its sample move (blocks 441 / 4096)",
                 "meas 441: before " + juce::String (a.worstBefore, 6) + " / after " + juce::String (a.worstAfter, 6) + " c; 4096: " + juce::String (b.worstBefore, 6)
                     + " / " + juce::String (b.worstAfter, 6) + " c (" + juce::String (a.nb) + " + " + juce::String (a.na) + " grains) | thr <= 0.01 c");

            // NE3 collision under the module's pitch-keyed model.
            auto collide = [&neBase] (bool moduleTable)
            {
                Session s (kFs, neBase(), kSeed, 32);
                s.hk().neModuleTable = moduleTable;
                loadN (s, 1);
                NeSched ne;
                ne.add (1000, rawOn (1, 60, 21)); ne.add (1000, rawOn (2, 60, 22));
                ne.add (1000, rawNe (21, -50.0)); ne.add (1000, rawNe (22, 30.0));
                ne.attach (s);
                s.addMidi (1000, on (1, 60)); s.addMidi (1000, on (2, 60));
                s.run (24000, 512);
                const double tr = s.hk().tapeRate[0];
                double w1 = 0, w2 = 0;
                for (const auto& e : ofSerial (s.hk().logInOrder(), 1)) w1 = std::max (w1, std::abs (centsOf (e, tr) - (-50.0)));
                for (const auto& e : ofSerial (s.hk().logInOrder(), 2)) w2 = std::max (w2, std::abs (centsOf (e, tr) - 30.0));
                return std::make_pair (w1, w2);
            };
            const auto mc = collide (true);
            const bool fired = c.worstAfter > 1.0 && (mc.first > 1.0 || mc.second > 1.0);
            row (fired, "NE2n control neModuleTable: the live change is lost and one-pitch notes collide (the module's pitch-keyed table)",
                 "meas after the change " + f2 (c.worstAfter, 3) + " c off; one-pitch errors " + f2 (mc.first, 3) + " / " + f2 (mc.second, 3) + " c | "
                     + ctrl (fired, "> 1 c"));
            const auto sc = collide (false);
            row (sc.first <= 0.01 && sc.second <= 0.01, "NE3 D5 two notes on one pitch, different noteIds + tunings -> separate rates",
                 "meas noteId 21 (-50 c) err " + juce::String (sc.first, 6) + ", noteId 22 (+30 c) err " + juce::String (sc.second, 6) + " | thr <= 0.01 c");
        }
        // NE4 — NE + Legato + D30
        {
            Session s (kFs, with (mpBase (false, 2, 100.0f), { { sn (1, "root_note"), 48.0f } }), kSeed, 32);
            loadN (s, 1);
            NeSched ne;
            ne.add (1000, rawOn (1, 60, 1)); ne.add (1000, rawNe (1, -30.0));
            ne.add (24000, rawOn (1, 64, 2)); ne.add (24000, rawNe (2, 25.0));
            ne.attach (s);
            s.addMidi (1000, on (1, 60));
            s.addMidi (24000, on (1, 64));
            s.addMidi (40000, off (1, 64));
            runTo (s, 24000);
            const int k = cloud (s);
            const double k0 = s.eng().getVoiceSemisForTest (k, 0, 24000);
            runTo (s, 24001);
            const double k1 = s.eng().getVoiceSemisForTest (cloud (s), 0, 24000);   // glide start: continuous
            runTo (s, 24000 + 4800 + 64);
            const double kEnd = s.eng().getVoiceSemisForTest (cloud (s), 0, s.pos);
            runTo (s, 40000 + 4800 + 64);
            const double kFb = s.eng().getVoiceSemisForTest (cloud (s), 0, s.pos);
            const bool ok = std::abs (k1 - k0) <= 1.0e-9 && std::abs (kEnd - 16.25) <= 1.0e-9 && std::abs (kFb - 11.70) <= 1.0e-9;
            row (ok, "NE4 D5 NE + Legato glides (continuously) to the tuned pitch; the D30 fallback lands on the held key's own tuning",
                 "meas start " + f2 (k0 + 48.0, 6) + " -> " + f2 (k1 + 48.0, 6) + ", end " + f2 (kEnd + 48.0, 9) + " (64.25), fallback " + f2 (kFb + 48.0, 9)
                     + " (59.70) | thr <= 1e-9 st");
        }
        // NE5 — capacity, drop, allocations, bypassed blocks
        {
            Session s (kFs, neBase(), kSeed, 512);
            loadN (s, 1);
            NeSched ne;
            ne.add (1000, rawOn (1, 61, 11));
            ne.attach (s);
            s.addMidi (1000, on (1, 61));
            runTo (s, 4096, 512);
            s.beforeBlock = nullptr;
            s.hk().resetCounters();
           #if JUCE_MAC
            eg21::audioThread = pthread_self();
            malloc_logger = eg21::countAllocation;
            eg21::allocCount = 0;
           #endif
            s.beforeBlock = [] (Session& ss, juce::int64, int)
            {
                for (int i = 0; i < 70; ++i) { Raw r = rawNe (11, (double) (i % 20)); r.sampleOffset = i; ext (ss).onVst3RawEvent (r); }
            };
            s.arm = true;
            s.blocksDone = 1;
            s.run (512, 512);
            s.arm = false;
            s.beforeBlock = nullptr;
           #if JUCE_MAC
            malloc_logger = nullptr;
            const int allocs = eg21::allocCount;
           #else
            const int allocs = 0;
           #endif
            const auto applied = s.hk().neApplied, dropped = s.hk().neDropped;
            // A bypassed block's events never replay.
            {
                for (int i = 0; i < 5; ++i) { Raw r = rawNe (11, 90.0); r.sampleOffset = i; ext (s).onVst3RawEvent (r); }
                juce::AudioBuffer<float> buf (2, 256);
                juce::MidiBuffer mb;
                buf.clear();
                s.p->processBlockBypassed (buf, mb);
            }
            s.hk().resetCounters();
            s.run (512, 512);
            const auto replay = s.hk().neApplied;
            row (applied == 64 && dropped == 6 && allocs == 0 && replay == 0,
                 "NE5 PD6 70 NE events in one block -> 64 applied, 6 dropped, 0 allocations; a bypassed block's events never replay",
                 "meas applied " + i64 (applied) + ", dropped " + i64 (dropped) + ", allocs " + juce::String (allocs) + ", replayed after bypass " + i64 (replay)
                     + " | thr 64 / 6 / 0 / 0");
        }
    }

    //==========================================================================
    // CK5 — clicks (local kappa vs the static render)
    Out ckRun (const P& p, const std::function<void (Session&)>& setup, double seconds = 2.0)
    {
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.load (0, sineBuf (2, (int) (4.0 * kFs), 220.0, kFs, 0.5), kFs);
        if (setup) setup (s);
        s.run ((juce::int64) (seconds * kFs), 512);
        return { s.L, s.R };
    }

    P ckBase (int mode, float glideMs = 0.0f, bool mpe = false)
    {
        return cat ({ mpBase (mpe, mode, glideMs),
                      { { sn (1, "density"), 40.0f }, { sn (1, "size"), 120.0f }, { sn (1, "position"), 50.0f }, { sn (1, "loop_end"), 66.0f },
                        { "amp_attack", 20.0f }, { "amp_decay", 200.0f }, { "amp_sustain", 70.0f }, { "amp_release", 400.0f }, { "vel_to_amp", 70.0f } } });
    }

    /** kappa vs the SAME-MOTION reference (the 2.5a RE form): the max of the
        held pre / post renders (a pitch move alone changes a tone's local
        kappa by f^2, so a static pre-only reference is not same-motion). */
    void ck (const char* id, const char* title, const Out& subj, const Out& pre, const Out& post, double te, const Out* hard, const char* hardId,
             const char* hardName)
    {
        const double t0 = te - 0.05, t1 = te + 0.15;
        const double ks = kappa (subj, t0, t1), kc = std::max (kappa (pre, t0, t1), kappa (post, t0, t1));
        const double r = ks / std::max (1e-30, kc);
        juce::String d = "meas kappa ratio " + f2 (r, 3) + " (subject " + f2 (ks, 4) + ", held pre / post " + f2 (kc, 4) + ", window " + f2 (t0, 2) + "-" + f2 (t1, 2)
                       + " s) | thr <= 2.0x | margin " + f2 (2.0 - r, 3);
        if (hard == nullptr)
        {
            row (r <= 2.0, juce::String (id) + " " + title, d + " | bound only: no constructible hard control (pitch latched per grain; HANDS-ON-2.5b row 10)");
            return;
        }
        const double rh = kappa (*hard, t0, t1) / std::max (1e-30, kc);
        row (r <= 2.0, juce::String (id) + " " + title, d + " | " + ctrl (rh > 2.0, juce::String (hardName) + " -> " + f2 (rh, 3) + "x"));
        row (rh > 2.0, juce::String (hardId) + " control " + hardName + " > 2.0x", "meas " + f2 (rh, 3) + "x | " + ctrl (rh > 2.0, "> 2.0x"));
    }

    void rowsCK()
    {
        const double te = 1.0;
        const juce::int64 T = (juce::int64) (te * kFs) + 11;
        // a — Mono retrigger 60 -> 64 (pre: 60 held; post: 64 held)
        {
            auto r = [T] (bool ev, bool hard)
            {
                return ckRun (ckBase (1), [=] (Session& s) { s.hk().ampResetZero = hard; s.addMidi (1000, on (1, 60, 90)); if (ev) s.addMidi (T, on (1, 64, 90)); });
            };
            const auto post = ckRun (ckBase (1), [] (Session& s) { s.addMidi (1000, on (1, 64, 90)); });
            const auto h = r (true, true);
            ck ("CK5a", "Mono retrigger from the current level", r (true, false), r (false, false), post, te, &h, "CK5an", "ampResetZero");
        }
        // b — Legato glide an octave
        {
            auto r = [T] (bool ev) { return ckRun (ckBase (2, 300.0f), [=] (Session& s) { s.addMidi (1000, on (1, 60)); if (ev) s.addMidi (T, on (1, 72)); }); };
            const auto post = ckRun (ckBase (2, 300.0f), [] (Session& s) { s.addMidi (1000, on (1, 72)); });
            ck ("CK5b", "Legato glide (300 ms, an octave)", r (true), r (false), post, te, nullptr, "", "");
        }
        // c — D30 fallback 67 -> 60
        {
            auto r = [T] (bool ev)
            {
                return ckRun (ckBase (1, 100.0f), [=] (Session& s) { s.addMidi (1000, on (1, 60)); s.addMidi (2000, on (1, 67)); if (ev) s.addMidi (T, off (1, 67)); });
            };
            const auto post = ckRun (ckBase (1, 100.0f), [] (Session& s) { s.addMidi (1000, on (1, 60)); });
            ck ("CK5c", "D30 fallback glide", r (true), r (false), post, te, nullptr, "", "");
        }
        // d — D32 Poly -> Mono (pre: 8 voices; post: the newest voice alone)
        {
            auto r = [T] (bool ev, bool hard)
            {
                return ckRun (ckBase (0), [=] (Session& s)
                {
                    s.hk().modeSwitchHard = hard;
                    for (int i = 0; i < 8; ++i) s.addMidi (1000 + 300 * i, on (1, 48 + 3 * i, 80));
                    if (ev) setAt (s, (T / 32) * 32, "voice_mode", 1.0f);
                });
            };
            const auto post = ckRun (ckBase (1), [] (Session& s) { s.addMidi (1000 + 300 * 7, on (1, 48 + 21, 80)); });
            const auto h = r (true, true);
            ck ("CK5d", "D32 Poly -> Mono with 8 voices (7 x 5 ms releases)", r (true, false), r (false, false), post, te, &h, "CK5dn", "modeSwitchHard");
        }
        // e — pedal-up (pre: sustained; post = pre: the release starts from the held sound)
        {
            auto r = [T] (bool ev, bool hard)
            {
                return ckRun (ckBase (0), [=] (Session& s)
                {
                    s.hk().pedalUpHard = hard;
                    s.addMidi (500, cc (1, 64, 127));
                    for (int i = 0; i < 4; ++i) { s.addMidi (1000 + 300 * i, on (1, 55 + 4 * i, 80)); s.addMidi (20000, off (1, 55 + 4 * i)); }
                    if (ev) s.addMidi (T, cc (1, 64, 0));
                });
            };
            const auto pre = r (false, false);
            const auto h = r (true, true);
            ck ("CK5e", "sustain pedal-up release", r (true, false), pre, pre, te, &h, "CK5en", "pedalUpHard");
        }
        // f — member bend sweep (+-3000: post = the note held at the sweep's top)
        {
            auto r = [T] (bool ev)
            {
                return ckRun (ckBase (0, 0.0f, true), [=] (Session& s)
                {
                    s.addMidi (1000, on (2, 60));
                    if (ev)
                        for (int i = 0; i < 200; ++i)
                            s.addMidi (T + 48 * i, pw (2, 8192 + (int) std::lround (3000.0 * std::sin (2.0 * juce::MathConstants<double>::pi * i / 100.0))));
                });
            };
            const auto post = ckRun (ckBase (0, 0.0f, true), [] (Session& s) { s.addMidi (500, pw (2, 8192 + 3000)); s.addMidi (1000, on (2, 60)); });
            ck ("CK5f", "MPE member bend sweep", r (true), r (false), post, te, nullptr, "", "");
        }
        // g — Mono velocity change on one pitch (pre: vel 40; post: vel 127)
        {
            auto r = [T] (bool ev, bool hard)
            {
                P p = with (ckBase (1), { { "vel_to_amp", 100.0f } });
                return ckRun (p, [=] (Session& s) { s.hk().gainSnap = hard; s.addMidi (1000, on (1, 60, 40)); if (ev) s.addMidi (T, on (1, 60, 127)); });
            };
            const auto post = ckRun (with (ckBase (1), { { "vel_to_amp", 100.0f } }), [] (Session& s) { s.addMidi (1000, on (1, 60, 127)); });
            const auto h = r (true, true);
            ck ("CK5g", "Mono velocity change (5 ms gain glide, vel_to_amp 100 %)", r (true, false), r (false, false), post, te, &h, "CK5gn", "gainSnap");
        }
    }

    //==========================================================================
    // BS1 / AL1 — the full script
    struct Script
    {
        NeSched ne;
        void install (Session& s, bool big)
        {
            // Poly + CC64 + re-strike; Mono / Legato with glide; D32; MPE member bend /
            // AT / CC74; RPN; NE mid-block.
            s.addMidi (100, cc (1, 64, 127));
            for (int i = 0; i < 6; ++i) { s.addMidi (500 + 211 * i, on (1, 50 + 3 * i, 60 + 9 * i)); s.addMidi (3000 + 177 * i, off (1, 50 + 3 * i)); }
            s.addMidi (5000, on (1, 53));
            s.addMidi (7000, cc (1, 64, 0));
            setAt (s, 9600, "glide", 120.0f);
            setAt (s, 12800, "voice_mode", 1.0f);
            s.addMidi (13000, on (1, 60)); s.addMidi (14111, on (1, 67)); s.addMidi (16222, off (1, 67));
            setAt (s, 19200, "voice_mode", 2.0f);
            s.addMidi (19500, on (1, 72)); s.addMidi (21000, off (1, 72)); s.addMidi (23000, off (1, 60));
            setAt (s, 25600, "voice_mode", 0.0f);
            setAt (s, 25600, "mpe_enable", 1.0f);
            rpn (s, 26000, 2, 0, 0, 24, 30);
            ne.add (27007, rawOn (2, 61, 41));
            ne.add (27007, rawNe (41, -50.0));
            s.addMidi (27007, on (2, 61));
            s.addMidi (27500, on (3, 65));
            for (int i = 0; i < 60; ++i)
            {
                s.addMidi (28000 + 101 * i, pw (2 + (i % 2), (i * 523) % 16384));
                s.addMidi (28050 + 101 * i, at (3, (i * 17) % 128));
                s.addMidi (28070 + 101 * i, cc (2, 74, (i * 29) % 128));
            }
            ne.add (30441, rawNe (41, 35.0));
            ne.add (33333, rawNe (41, -10.0));
            s.addMidi (36000, off (2, 61)); s.addMidi (36500, off (3, 65));
            if (big)
            {
                setAt (s, 38400, "mpe_enable", 0.0f);
                setAt (s, 38400, "voice_mode", 1.0f);
                for (int i = 0; i < 130; ++i) s.addMidi (39000 + 7 * i, on (1 + (i / 128), i % 128, 70));   // > 128 held keys: stack overflow
                for (int i = 0; i < 130; ++i) s.addMidi (44000 + 5 * i, off (1 + (i / 128), i % 128));
            }
            ne.attach (s);
        }
    };

    std::vector<float> bsRun (int block, std::uint64_t seed)
    {
        P p = cat ({ mpBase (false, 0, 0.0f), { { "amp_release", 120.0f }, { sn (1, "pitch_rand"), 20.0f }, { sn (1, "spray"), 30.0f }, { "polyphony", 6.0f } } });
        Session s (kFs, p, seed, 512);
        shipping (s);
        loadN (s, 1, 3.0);
        Script sc;
        sc.install (s, false);
        s.run (40000, block);
        return eg22::lr (s);
    }

    void rowsBS()
    {
        const auto ref = bsRun (32, kSeed);
        juce::String d;
        int worst = 0;
        for (int b : { 441, 512, 4096, 0 })
        {
            const int n = diffCount (ref, bsRun (b, kSeed));
            worst = std::max (worst, n);
            d += (b == 0 ? juce::String ("var") : juce::String (b)) + " " + juce::String (n) + ", ";
        }
        const int nSeed = diffCount (ref, bsRun (32, kSeed + 1));
        row (worst == 0 && nSeed > 0, "BS1 PERF-03 block-size null: CC64, Mono / Legato + glide, D32, MPE member expression, RPN, NE mid-block",
             "meas differing samples vs 32: " + d + "| thr 0 | " + ctrl (nSeed > 0, "seed + 1 -> " + juce::String (nSeed) + " differ"));
        row (nSeed > 0, "BS1n control seed + 1 differs", "meas " + juce::String (nSeed) + " | " + ctrl (nSeed > 0, "> 0"));
    }

    void rowAL1()
    {
       #if JUCE_MAC
        using namespace eg21;
        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocCount = 0;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);
        const int liveness = allocCount;
        allocCount = 0;

        P p = cat ({ mpBase (false, 0, 0.0f), { { "amp_release", 120.0f }, { "polyphony", 6.0f } } });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.record = false;
        loadN (s, 1, 3.0);
        Script sc;
        sc.install (s, true);
        // + one 70-event NE block (on the held ch 2 note)
        auto prev = s.beforeBlock;
        s.beforeBlock = [prev] (Session& ss, juce::int64 b0, int len)
        {
            if (prev) prev (ss, b0, len);
            if (b0 <= 31000 && 31000 < b0 + len)
                for (int i = 0; i < 70; ++i) { Raw r = rawNe (41, (double) i); r.sampleOffset = 0; ext (ss).onVst3RawEvent (r); }
        };
        s.arm = true;
        s.run (52000, 512);
        s.arm = false;
        malloc_logger = nullptr;
        const int n = allocCount;
        auto& h = s.hk();
        const bool live = h.retriggers > 0 && h.legatoMoves > 0 && h.fallbacks > 0 && h.modeSwitches >= 4 && h.sustainDeferred > 0
                       && h.restrikeReleases > 0 && h.rpnMessages > 0 && h.memberWrites > 0 && h.neApplied > 0 && h.neDropped > 0
                       && s.eng().getStackForTest().dropped > 0 && h.serialsMinted > 0;
        row (liveness == 1 && n == 0 && live, "AL1 PERF-01 0 audio-thread allocations: MIDI / MPE / NE script, 130 held keys (stack overflow), a 70-event NE block",
             "meas " + juce::String (n) + " allocs over " + i64 ((std::uint64_t) s.blocksDone) + " blocks (retriggers " + i64 (h.retriggers) + ", legato " + i64 (h.legatoMoves)
                 + ", fallbacks " + i64 (h.fallbacks) + ", switches " + i64 (h.modeSwitches) + ", deferred " + i64 (h.sustainDeferred) + ", restrikes " + i64 (h.restrikeReleases)
                 + ", rpn " + i64 (h.rpnMessages) + ", member writes " + i64 (h.memberWrites) + ", NE applied " + i64 (h.neApplied) + " / dropped " + i64 (h.neDropped)
                 + ", stack dropped " + i64 (s.eng().getStackForTest().dropped) + ") | thr 0 | " + ctrl (liveness == 1, "liveness malloc(64) armed -> " + juce::String (liveness)));
       #else
        row (true, "AL1 PERF-01 allocation gate (macOS only)", "INFO skipped");
       #endif
    }

    //==========================================================================
    // TB5 / TB6
    void rowsTB()
    {
        {
            rh::Overrides o;
            o.extra = { { "voice_mode", 0.0f }, { "glide", 0.0f }, { "mpe_enable", 0.0f } };
            juce::StringArray ship, legacy;
            {
                rh::ScopedOverrides so (o);
                ship = rh::computeAll();
                legacy = rh::computeAllLegacy();
            }
            const int a = eg24::sameLines (eg24::toMap (ship), eg24::readTable ("renders-2.4.sha256"));
            const int b = eg24::sameLines (eg24::toMap (legacy), eg24::readTable ("renders-2.2.sha256"));
            row (a == 14 && b == 14, "TB5 true bypass: Poly, glide 0, MPE off, no CC64 -> renders-2.4 + legacy byte-identical",
                 "meas shipping " + juce::String (a) + "/14, legacy " + juce::String (b) + "/14 identical | thr 14 / 14");
        }
        {
            rh::Overrides o;
            o.extra = { { "mpe_enable", 0.0f } };
            for (int i = 0; i < 16; ++i)
            {
                const int ch = 1 + i;
                const juce::int64 t = 700 + 1900 * i;
                o.midi.push_back ({ t, cc (ch, 101, 0) }); o.midi.push_back ({ t, cc (ch, 100, i % 2 == 0 ? 0 : 6) });
                o.midi.push_back ({ t, cc (ch, 6, 5 + i) }); o.midi.push_back ({ t, cc (ch, 38, 3 * i) });
                o.midi.push_back ({ t + 50, cc (ch, 99, 1) }); o.midi.push_back ({ t + 50, cc (ch, 98, 2) }); o.midi.push_back ({ t + 50, cc (ch, 6, 9) });
            }
            juce::StringArray ship;
            {
                rh::ScopedOverrides so (o);
                ship = rh::computeAll();
            }
            const int a = eg24::sameLines (eg24::toMap (ship), eg24::readTable ("renders-2.4.sha256"));
            // The 2.5a any-channel source rule with MPE off (eg25a RG1 / MD1 form) after RPN traffic.
            Tally t;
            Session s (kFs, mpBase (false), kSeed, 32);
            loadN (s, 1);
            rpn (s, 500, 2, 0, 0, 12);
            rpn (s, 600, 1, 0, 6, 3);
            s.addMidi (1000, on (1, 60)); s.addMidi (1000, on (2, 64));
            s.addMidi (2000, cc (5, 74, 0)); s.addMidi (2000, at (7, 127)); s.addMidi (2000, pw (9, 16383));
            runTo (s, 2100);
            for (const auto& kn : { std::make_pair (60, 1), std::make_pair (64, 2) })
            {
                const auto& m = s.eng().getVoiceModForTest (voiceOf (s, kn.first, kn.second));
                t.check (sameF (m.src[S::Slide], -1.0f) && sameF (m.src[S::Press], 1.0f) && sameF (m.src[S::Bend], 1.0f)
                             && sameD (s.eng().getVoice (voiceOf (s, kn.first, kn.second)).mpeSemis, 0.0), "note " + juce::String (kn.first));
            }
            row (a == 14 && t.bad == 0, "TB6 MPE off with RPN / NRPN traffic parsed: renders-2.4 byte-identical; the 2.5a any-channel source rule unchanged",
                 "meas " + juce::String (a) + "/14 identical; sources " + t.detail (2) + " | thr 14, exact");
        }
    }

    //==========================================================================
    void runAll()
    {
        std::printf ("\n== Phase 2.5b engine gates (eg25d): MPE, note expression, clicks, invariance, bypass\n");
        if (want ("MP")) { section ("MP. MPE (D31, PD7 / PD8)"); rowsMP(); }
        if (want ("MR")) { section ("MR. 2.5a rows under MPE"); rowsMR(); }
        if (want ("NE")) { section ("NE. VST3 note expression (D5, PD6)"); rowsNE(); }
        if (want ("CK5")) { section ("CK5. click gates (local kappa)"); rowsCK(); }
        if (want ("BS1")) { section ("BS1. block-size null"); rowsBS(); }
        if (want ("AL1")) { section ("AL1. allocations"); rowAL1(); }
        if (want ("TB")) { section ("TB5 / TB6. true bypass"); rowsTB(); }
    }

    //==========================================================================
    // --cpu25b (PD18): C25mpe INFO
    int cpu25b()
    {
        constexpr int kBlock = 128;
        const double refNs = cal::readRef (gGoldenDir);
        std::printf ("== --cpu25b (PLAN-2.5b T16, PD18): ref_ns %.0f\n", refNs);
        auto mk = [] ()
        {
            auto s = eg24::c25Session (kFs, kBlock, true, eg23::worstFx(),
                                       cat ({ eg24::c25Extra(), eg25b::c25mRoutes(), { { "mpe_enable", 1.0f }, { "voice_mode", 0.0f }, { "glide", 300.0f } } }));
            s->record = false;
            return s;
        };
        auto s = mk();
        juce::AudioBuffer<float> buf (2, kBlock);
        juce::MidiBuffer mb;
        std::int64_t pos = 0;
        const std::int64_t period = (std::int64_t) (0.5 * kFs);
        int prevNote[16];
        std::fill (std::begin (prevNote), std::end (prevNote), -1);
        auto block = [&] (bool timeIt, double& cpu)
        {
            buf.clear();
            mb.clear();
            if (pos % period < kBlock)
                for (int i = 0; i < 16; ++i)
                {
                    // 16 notes on 15 member channels, re-struck every 500 ms (Poly glide 300 ms from the last key).
                    const int ch = 2 + (i % 15), note = 40 + i + (int) ((pos / period) % 2) * 3;
                    if (prevNote[i] >= 0)
                        mb.addEvent (juce::MidiMessage::noteOff (ch, prevNote[i]), 0);
                    mb.addEvent (juce::MidiMessage::noteOn (ch, note, (juce::uint8) 100), 1);
                    prevNote[i] = note;
                }
            for (int c = 2; c <= 16; ++c)
            {
                const double ph = 2.0 * juce::MathConstants<double>::pi * (0.5 * (double) pos / kFs + 0.07 * c);
                mb.addEvent (juce::MidiMessage::pitchWheel (c, 8192 + (int) std::lround (2000.0 * std::sin (ph))), 0);
            }
            const double a = timeIt ? eg22::threadNs() : 0.0;
            s->p->processBlock (buf, mb);
            if (timeIt) cpu += eg22::threadNs() - a;
            pos += kBlock;
        };
        double cpu = 0.0;
        const double calB = cal::calibNs();
        for (int b = 0; b < (int) (5.0 * kFs / kBlock); ++b) block (false, cpu);
        s->hk().resetCounters();
        const auto ticks0 = s->hk().gridTicks;
        std::vector<double> pct;
        double minDuty = 1e9;
        for (int r = 0; r < 5; ++r)
        {
            cpu = 0.0;
            const double w0 = eg22::wallNs(), t0 = eg22::threadNs();
            const int nb = (int) (10.0 * kFs / kBlock);
            for (int b = 0; b < nb; ++b) block (true, cpu);
            const double wall = eg22::wallNs() - w0, thr = eg22::threadNs() - t0;
            minDuty = std::min (minDuty, wall > 0 ? thr / wall : 0.0);
            pct.push_back (100.0 * cpu / (10.0 * 1.0e9));
        }
        const double calA = cal::calibNs();
        std::sort (pct.begin(), pct.end());
        const double recomp = (double) s->hk().lawRecomputes / std::max<std::uint64_t> (1, s->hk().gridTicks - ticks0);
        const bool valid = std::isfinite (refNs) && calB <= 1.15 * refNs && calA / calB <= 1.10 && minDuty >= 0.9 && recomp >= 0.9;
        std::printf ("  INFO C25mpe: median %.2f %% (min %.2f, max %.2f), duty min %.3f, calib before %.0f (%.3f x ref) after %.0f (%.3f x before) | witness: law recomputes %.1f %% of ticks (>= 90 %%), member writes %llu | %s\n",
                     pct[pct.size() / 2], pct.front(), pct.back(), minDuty, calB, calB / refNs, calA, calA / calB, 100.0 * recomp,
                     (unsigned long long) s->hk().memberWrites, valid ? "witness valid" : "INVALID");
        std::fflush (stdout);
        return valid ? 0 : 1;
    }
}} // namespace ::eg25d
