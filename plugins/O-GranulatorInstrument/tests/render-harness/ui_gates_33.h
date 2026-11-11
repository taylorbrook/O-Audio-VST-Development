/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 3 / Phase 3.3 processor-surface
    gates (PLAN-3.3 PD21, T5). Processor only: the harness never compiles the
    editor. Included by main.cpp AFTER ui_gates_32.h.

    Row IDs carry the U prefix of ui_gates_31.h / ui_gates_32.h. OGI_ROWS groups:
    UVR, UVS, UWS, UME, ULF, USRC, UMN, ULH, UMB, UMT, ULM, UCM6b, UAL2, UTL33,
    UPL2 / UPB2 (the two INFO rows).

      UVR1   16 display rows; every counted voice has a row; a voice keeps its
             row for its life; polyphony 16 with the steal spares in use -> a
             voice in engine slot 16-19 is shown
      UVR1n  identity map (row = engine slot) -> the slot >= 16 voice is missing
      UVR2   newest == the row of the engine's newestSer (counted) across note-
             ons, releases, steals, Mono / Legato; -1 after All Sound Off while
             a stolen row holds
      UVR2n  page-style guess (highest row with a state) -> mismatch after a steal
      UVS1   polyphony 4, a 5th note: the victim's row shows 3 for >= 1 poll and
             <= 300 ms + 1 poll, then empty; the new voice in its own row
      UVS1n  hold 0 -> state 3 never seen
      UVS2   release -> state 2, empty after the env ends; CC64 keeps state 1
      UVS3   polyphony shrink, All Sound Off, Mono handovers leave stolenCount
             alone; each real steal adds exactly 1
      UWS1   note 50 st above root: column 8 == 50, grain bit 2 on that stream
             only (key track 0 stream clear)
      UWS1n  stream 0's wantSt for every stream -> the bit on the wrong stream
      UME1   menv levels / stages == the engine's envelopes through the map
      ULF1   lfoPh global == getLfoPhaseTelemetry; per-voice == vm[k].lfo[n]
             expression; -1 for Global-mode LFOs and empty rows
      USRC1  sources[i] == getSourceTelemetry (i + 1); Key / Bend / Slide signs;
             held when silent
      UMN1   modNorm key set == routed set, values == destTel; a per-voice
             route follows the newest voice and rests at base with no voice; a
             global source moves its key without notes
      UMN1n  all 125 keys sent -> extra keys
      ULH1   lanes[].lo / hi only on a lane with a routed loop dest, == min /
             max of the two destTel
      ULH1n  always emitted -> the unrouted lanes carry lo / hi
      UMB1   17 x 125 x LFO mode: the engine's mono predicates == the page rule
             (a C++ transcription; the constants below, pinned by closure [16])
      UMT1   out[L] / out[R] == the harness's own max |x| since the last poll,
             bit-exact
      UMT2   one stream hard left: out[R] / out[L] <= -60 dB, out[L] > -40 dBFS
      UMT2n  [max, max] -> FAILS
      UMT3   SC fs/4 sine on L (0.5, 0.1): rec.peak[0] == A +-0.01 dB, [1] == 0;
             997 Hz within [A cos (pi 997 / fs), A]
      UMT4   two polls after one block: the second reads 0 (out and SC)
      UMT4n  read without the exchange -> the second read is not 0
      ULM1   limit == 32 / 64 / 128 / 256 for grain_limit 0-3
      UCM6b  the Recordings dir pre-seeded with GI-Rec-<stamp>-S1 / S2 for now
             - 1 ... now + 5 s: a failWav take names no file and no seeded name;
             a succeeding take beside it is distinct too
      UCM6bn legacyTakeName -> the failed take names a seeded file
      UAL2   0 audio-thread allocations: 256 grains, 16 voices, steals (ring
             writes), per-voice LFOs, both L / R peak splits publishing
      UTL33  telemetryToVar 3.3 shape (PD13) through JSON text
      INFO UPL2  widest 3.3 JSON (16 routes, 256 grains, 16 rows) + pull cost
      INFO UPB2  publishTelemetry voice-loop cost before / after the 3.3
                 extension at 20 slots x 4 streams (harness re-enactment)

    Conventions as ui_gates_32.h: every row prints "meas ... | thr ... | ctrl
    ..."; an `n` row runs its parent's predicate on the broken input and PASSES
    only when that predicate FAILS ("ctrl FIRED"). Each row asserts its
    stimulus before its result. Clocks are scripted (the sample position); the
    only wall clock is the take stamp UCM6b seeds around.

    Sandbox: every file lives under one temp dir (deleted at exit) or under the
    harness Recordings / Imports temp tree (make(), PLAN-3.2 PD24).

  ==============================================================================
*/
#pragma once

#include <set>

namespace { namespace ug33
{
    using eg21::Session;
    using eg21::Proc;
    using eg21::P;
    using eg21::with;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::noiseBuf;
    using eg21::kSeed;
    using eg21::setP;
    using eg21::rawOf;
    using eg21::sameF;
    using eg22::sn;
    using eg24b::want;
    using eg24b::section;
    using eg25a::baseMod;
    using eg25a::route;
    using eg26a::ScSig;
    using eg26a::inject;
    using eg26a::recBase;
    using ug31::keysOf;
    using ug31::isNumber;
    using ug32::nowOf;
    using ug32::yn;
    using ug32::fullPool;
    using Tel = Proc::UiTelemetry;

    constexpr double kFs = 48000.0;
    constexpr juce::int64 kPoll = 1584;   // 33 ms at 48 k: the page's pull

    juce::File gTmp33;

    //==========================================================================
    // UMB1: the page's mono-badge rule, transcribed from
    // Source/ui/public/index.html (PER_VOICE_SOURCES, isPerVoiceSource,
    // isGlobalDest). Closure check [16] (ui_frontend_check.js, PLAN-3.3 PD23)
    // pins THESE constants to the page text: keep them in this one block.
    //
    //   isPerVoiceSource (src) = PER_VOICE_SOURCES.includes (src)
    //                         || (1 <= src <= 4 && lfo<src>_mode == 1)
    //   isGlobalDest (d)       = d == 1 || 114 <= d <= 121
    //                         || (122 <= d <= 125 && lfo<d - 121>_mode == 0)
    //   mono-on                = isPerVoiceSource (src) && isGlobalDest (d)
    namespace pageRule
    {
        constexpr int kPerVoiceSources[] = { 5, 6, 7, 8, 9, 11, 12, 13 };   // PER_VOICE_SOURCES
        constexpr int kLfoSrcLo = 1, kLfoSrcHi = 4;                         // LFO n per-voice while lfoN_mode == 1
        constexpr int kGlobalDest = 1;                                      // output_gain
        constexpr int kGlobalDestLo = 114, kGlobalDestHi = 121;             // feedback ... reverb_damp
        constexpr int kLfoRateDestLo = 122, kLfoRateDestHi = 125;           // lfoN_rate: global while lfoN_mode == 0
        constexpr int kLfoRateDestBase = 121;                               // lfo<d - 121>_mode

        inline bool isPerVoiceSource (int src, const int (&lfoMode)[4])
        {
            for (const int v : kPerVoiceSources)
                if (v == src)
                    return true;
            return src >= kLfoSrcLo && src <= kLfoSrcHi && lfoMode[src - 1] == 1;
        }
        inline bool isGlobalDest (int d, const int (&lfoMode)[4])
        {
            if (d == kGlobalDest || (d >= kGlobalDestLo && d <= kGlobalDestHi))
                return true;
            if (d >= kLfoRateDestLo && d <= kLfoRateDestHi)
                return lfoMode[d - kLfoRateDestBase - 1] == 0;
            return false;
        }
    }

    //==========================================================================
    // Helpers

    /** The display row showing `note` with a state in [lo, hi]; -1 none, -2 more than one. */
    int rowOfNote (const Tel& t, int note, int lo = 1, int hi = 2)
    {
        int found = -1;
        for (int r = 0; r < Tel::kVoices; ++r)
        {
            const auto& v = t.voices[(size_t) r];
            if (v.state >= lo && v.state <= hi && v.note == note)
            {
                if (found >= 0)
                    return -2;
                found = r;
            }
        }
        return found;
    }

    /** Engine slot of a non-free voice with this serial, -1 none. */
    int slotOfSerial (Session& s, std::uint64_t ser)
    {
        for (int k = 0; k < ogi::kVoiceSlots; ++k)
            if (! s.eng().getVoice (k).isFree() && s.eng().getVoice (k).serial == ser)
                return k;
        return -1;
    }

    /** The counted engine slot playing `note`, -1 none. */
    int countedSlotOfNote (Session& s, int note)
    {
        for (int k = 0; k < ogi::kVoiceSlots; ++k)
            if (s.eng().getVoice (k).isCounted() && s.eng().getVoice (k).note == note)
                return k;
        return -1;
    }

    int countedNow (Session& s)
    {
        int n = 0;
        for (int k = 0; k < ogi::kVoiceSlots; ++k)
            n += s.eng().getVoice (k).isCounted() ? 1 : 0;
        return n;
    }

    int ampStageOf (ogi::AmpEnv::Stage st)
    {
        using S = ogi::AmpEnv::Stage;
        return st == S::attack ? 1 : st == S::decay ? 2 : st == S::sustain ? 3 : (st == S::release || st == S::forced) ? 4 : 0;
    }

    int menvStageOf (ogi::GiModEnv::Stage st)
    {
        using S = ogi::GiModEnv::Stage;
        return st == S::attack ? 1 : st == S::decay ? 2 : st == S::sustain ? 3 : st == S::release ? 4 : 0;
    }

    void cc (Session& s, juce::int64 t, int number, int value) { s.addMidi (t, juce::MidiMessage::controllerEvent (1, number, value)); }

    juce::String db (double lin) { return lin > 0.0 ? juce::String (20.0 * std::log10 (lin), 2) + " dB" : juce::String ("-inf"); }

    //==========================================================================
    // UVR1 / UVR1n — 16 rows, every counted voice shown, rows for life, slot >= 16
    struct Vr1 { int polls = 0, counted = 0, missing = 0, moved = 0, highSlotPolls = 0, highSlotShown = 0; std::uint64_t steals = 0; };

    Vr1 vr1Run (bool identity)
    {
        Session s (kFs, with (baseMod (1), P { { sn (1, "density"), 20.0f } }), kSeed, 256);
        s.p->testRowMapIdentity = identity;
        s.load (0, noiseBuf (2, 96000, 0.2f, 401), kFs);
        for (int n = 0; n < 16; ++n)
            s.noteOn (512 + 256 * n, 40 + n);
        s.noteOn (24000, 70);   // the 17th note: a steal, the new voice lands in slot 16
        s.noteOn (36000, 71);   // a second steal
        s.record = false;
        Vr1 v;
        std::map<std::uint64_t, int> rowOf;
        juce::int64 next = 0;
        s.afterBlock = [&] (Session& ss)
        {
            if (ss.pos < next)
                return;
            next = ss.pos + kPoll;
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            ++v.polls;
            bool high = false;
            for (int k = 0; k < ogi::kVoiceSlots; ++k)
            {
                const auto& vo = ss.eng().getVoice (k);
                if (! vo.isCounted())
                    continue;
                ++v.counted;
                high = high || k >= Tel::kVoices;
                const int r = rowOfNote (t, vo.note);
                if (r < 0)
                {
                    ++v.missing;
                    continue;
                }
                v.highSlotShown += k >= Tel::kVoices ? 1 : 0;
                const auto it = rowOf.find (vo.serial);
                if (it == rowOf.end())
                    rowOf[vo.serial] = r;
                else if (it->second != r)
                    ++v.moved;
            }
            v.highSlotPolls += high ? 1 : 0;
        };
        s.run (60000, 256);
        v.steals = s.hk().voiceSteals;
        return v;
    }

    bool vr1Predicate (const Vr1& v) { return v.missing == 0 && v.moved == 0 && v.highSlotShown > 0; }

    void rowsVR1()
    {
        static_assert (Tel::kVoices == 16, "PD1: 16 display rows");
        const auto a = vr1Run (false);
        const auto n = vr1Run (true);
        const bool stim = a.steals >= 2 && a.highSlotPolls > 0 && a.polls > 30;
        row (stim && vr1Predicate (a), "UVR1 PD1 16 rows: every counted voice shown, rows for life, an engine slot >= 16 voice shown",
             "meas " + juce::String (a.polls) + " polls, " + juce::String (a.counted) + " counted voice observations, missing " + juce::String (a.missing)
                 + ", row moves " + juce::String (a.moved) + ", polls with a counted voice in slot 16-19 " + juce::String (a.highSlotPolls) + " (shown "
                 + juce::String (a.highSlotShown) + "), steals " + i64 (a.steals) + " | thr 0 missing, 0 moves, slot >= 16 shown | ctrl UVR1n");
        const bool fired = n.steals >= 2 && n.highSlotPolls > 0 && ! vr1Predicate (n);
        row (fired, "UVR1n control: identity map (row = engine slot) -> the slot >= 16 voice is missing (predicate must FAIL)",
             "meas missing " + juce::String (n.missing) + " observations over " + juce::String (n.highSlotPolls) + " polls with a slot >= 16 voice, shown "
                 + juce::String (n.highSlotShown) + " | thr predicate FAILS | " + ctrl (fired, "testRowMapIdentity -> " + juce::String (n.missing) + " missing"));
    }

    //==========================================================================
    // UVR2 / UVR2n — newest from the engine's rule
    void rowsVR2()
    {
        Session s (kFs, with (baseMod (1), P { { "polyphony", 4.0f }, { sn (1, "density"), 20.0f } }), kSeed, 256);
        s.load (0, noiseBuf (2, 96000, 0.2f, 402), kFs);
        // Poly: notes, a release, two steals.
        s.noteOn (4800, 48);  s.noteOn (9600, 52);  s.noteOn (14400, 55);
        s.noteOff (19200, 52);
        s.noteOn (24000, 59); s.noteOn (28800, 62);
        s.noteOn (33600, 64);                        // 5th counted -> steal
        s.noteOn (48000, 65);                        // steal
        s.noteOn (64800, 66);                        // steal after the holds expired: a low row is reused
        // Mono, then Legato.
        s.at (72000, [] (Session& ss) { setP (*ss.p, "voice_mode", 1.0f); });
        s.noteOn (74400, 67); s.noteOn (79200, 69); s.noteOff (84000, 67); s.noteOff (88800, 69);
        for (int nn : { 48, 55, 59, 62, 64, 65, 66 })
            s.noteOff (73000, nn);
        s.at (93600, [] (Session& ss) { setP (*ss.p, "voice_mode", 2.0f); });
        s.noteOn (96000, 71); s.noteOn (100800, 72); s.noteOff (105600, 71); s.noteOff (110400, 72);
        // Poly again: a steal, then All Sound Off inside its hold.
        s.at (115200, [] (Session& ss) { setP (*ss.p, "voice_mode", 0.0f); });
        s.noteOn (117600, 50); s.noteOn (120000, 53); s.noteOn (122400, 57); s.noteOn (124800, 60);
        s.noteOn (127200, 63);                       // steal
        juce::int64 asoAt = 129600;                  // 50 ms later: All Sound Off
        cc (s, asoAt, 120, 0);
        s.record = false;

        int polls = 0, bad = 0, guessBad = 0, guessBadAfterSteal = 0, withNewest = 0, monoPolls = 0, asoHeld = 0, asoBad = 0;
        juce::String first;
        juce::int64 next = 0;
        s.afterBlock = [&] (Session& ss)
        {
            if (ss.pos < next)
                return;
            next = ss.pos + kPoll;
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            ++polls;
            const std::uint64_t ns = ss.hk().newestSerial;
            const int k = slotOfSerial (ss, ns);
            int expect = -1;
            if (k >= 0 && ss.eng().getVoice (k).isCounted())
                expect = rowOfNote (t, ss.eng().getVoice (k).note);
            if (expect == -2)
                expect = -99;   // ambiguous: always a mismatch
            withNewest += expect >= 0 ? 1 : 0;
            monoPolls += (ss.pos > 72000 && ss.pos < 115200 && expect >= 0) ? 1 : 0;
            if (t.newest != expect)
            {
                ++bad;
                if (first.isEmpty())
                    first = "at " + i64 ((std::uint64_t) ss.pos) + ": newest " + juce::String (t.newest) + " expected " + juce::String (expect);
            }
            int guess = -1;   // the page-style guess: the highest row with a state
            for (int r = 0; r < Tel::kVoices; ++r)
                guess = t.voices[(size_t) r].state != 0 ? r : guess;
            if (guess != expect)
            {
                ++guessBad;
                guessBadAfterSteal += ss.hk().voiceSteals > 0 ? 1 : 0;
            }
            if (ss.pos > asoAt)
            {
                bool held = false;
                for (const auto& v : t.voices)
                    held = held || v.state == 3;
                if (held)
                {
                    ++asoHeld;
                    asoBad += t.newest == -1 ? 0 : 1;
                }
            }
        };
        s.run (140000, 256);
        const auto steals = s.hk().voiceSteals;
        const bool stim = polls > 70 && steals >= 4 && withNewest > 40 && monoPolls > 10 && asoHeld > 0;
        row (stim && bad == 0 && asoBad == 0, "UVR2 PD4 newest == the row of the engine's newestSer (counted) across notes, releases, steals, Mono / Legato, ASO",
             "meas " + juce::String (polls - bad) + "/" + juce::String (polls) + " polls equal (" + juce::String (withNewest) + " with a newest row, "
                 + juce::String (monoPolls) + " in Mono / Legato), steals " + i64 (steals) + ", polls after All Sound Off with a stolen row held "
                 + juce::String (asoHeld) + " (newest -1 on " + juce::String (asoHeld - asoBad) + ")" + (first.isNotEmpty() ? ", first " + first : juce::String())
                 + " | thr every poll | ctrl UVR2n");
        const bool fired = stim && guessBadAfterSteal > 0;
        row (fired, "UVR2n control: page-style guess (highest row with a state) -> mismatch after a steal (predicate must FAIL)",
             "meas guess differs on " + juce::String (guessBad) + "/" + juce::String (polls) + " polls (" + juce::String (guessBadAfterSteal)
                 + " after a steal) | thr predicate FAILS | " + ctrl (fired, "highest-row guess -> " + juce::String (guessBadAfterSteal) + " mismatches"));
    }

    //==========================================================================
    // UVS1 / UVS1n — steal visibility
    struct Vs1
    {
        std::uint64_t steals = 0, victim = 0;
        int victimNote = -1, victimRow = -1, newRow = -1, seen3 = 0, newOwnRow = 0, emptyAfter = 0, polls = 0;
        double stealMs = 0.0, firstMs = -1.0, lastMs = -1.0, emptyMs = -1.0;
    };

    constexpr juce::int64 kPollVs1 = 1536;   // 3 blocks of 512: a 32 ms pull on block boundaries

    Vs1 vs1Run (double holdMs)
    {
        // Block 512 with the 5th note ON a block start: the 5 ms victim fade ends
        // inside that block, so the frame never carries state 3 -- the row's 3
        // can only come from the steal ring + the hold.
        Session s (kFs, with (baseMod (1), P { { "polyphony", 4.0f }, { sn (1, "density"), 20.0f } }), kSeed, 512);
        if (holdMs >= 0.0)
            s.p->testSetStealHoldMs (holdMs);
        s.load (0, noiseBuf (2, 96000, 0.2f, 403), kFs);
        const int notes[] = { 48, 52, 55, 59 };
        for (int i = 0; i < 4; ++i)
            s.noteOn (512 * (i + 1), notes[i]);
        constexpr juce::int64 kSteal = 512 * 94;   // 48128
        s.noteOn (kSteal, 62);
        s.record = false;
        Vs1 v;
        std::map<std::uint64_t, int> noteOfSerial;
        std::uint64_t stealsBefore = 0;
        juce::int64 next = 0;
        s.afterBlock = [&] (Session& ss)
        {
            for (int k = 0; k < ogi::kVoiceSlots; ++k)
                if (ss.eng().getVoice (k).isCounted())
                    noteOfSerial[ss.eng().getVoice (k).serial] = ss.eng().getVoice (k).note;
            if (ss.hk().voiceSteals > stealsBefore && v.stealMs == 0.0)
                v.stealMs = 1000.0 + 1000.0 * (double) kSteal / kFs;
            stealsBefore = ss.hk().voiceSteals;
            if (ss.pos < next)
                return;
            next = ss.pos + kPollVs1;
            const double now = nowOf (ss);
            const auto t = ss.p->buildUiTelemetry (now);
            ++v.polls;
            if (ss.hk().voiceSteals == 0)
                return;
            if (v.victim == 0)
            {
                v.victim = ss.hk().stolenSerials[0];
                v.victimNote = noteOfSerial.count (v.victim) ? noteOfSerial[v.victim] : -1;
            }
            const int r3 = rowOfNote (t, v.victimNote, 3, 3);
            if (r3 >= 0)
            {
                ++v.seen3;
                v.victimRow = r3;
                if (v.firstMs < 0.0)
                    v.firstMs = now;
                v.lastMs = now;
                const int rn = rowOfNote (t, 62);
                if (rn >= 0 && rn != r3)
                {
                    ++v.newOwnRow;
                    v.newRow = rn;
                }
            }
            else if (v.firstMs >= 0.0 && v.emptyMs < 0.0 && v.victimRow >= 0 && t.voices[(size_t) v.victimRow].state == 0)
            {
                v.emptyMs = now;
                ++v.emptyAfter;
            }
        };
        s.run (96000, 512);
        v.steals = s.hk().voiceSteals;
        return v;
    }

    bool vs1Predicate (const Vs1& v, double holdMs)
    {
        const double poll = 1000.0 * (double) kPollVs1 / kFs;
        return v.seen3 >= 1 && v.lastMs - v.stealMs <= holdMs + poll && v.emptyAfter == 1 && v.emptyMs - v.stealMs <= holdMs + 2.0 * poll && v.newOwnRow > 0;
    }

    void rowsVS1()
    {
        const auto a = vs1Run (-1.0);
        const auto n = vs1Run (0.0);
        const double hold = GiVoiceRowMap::kStealHoldMs;
        const bool stim = a.steals == 1 && a.victimNote >= 0 && n.steals == 1 && n.victimNote >= 0;
        row (stim && vs1Predicate (a, hold), "UVS1 PD2 steal visibility: the victim's row shows 3 for >= 1 poll and <= 300 ms + 1 poll, then empty",
             "meas victim note " + juce::String (a.victimNote) + " row " + juce::String (a.victimRow) + ": state 3 on " + juce::String (a.seen3)
                 + " polls, first +" + juce::String (a.firstMs - a.stealMs, 1) + " ms, last +" + juce::String (a.lastMs - a.stealMs, 1) + " ms, empty at +"
                 + juce::String (a.emptyMs - a.stealMs, 1) + " ms; new note 62 in its own row " + juce::String (a.newRow) + " (" + juce::String (a.newOwnRow)
                 + " polls); steals " + i64 (a.steals) + " (frame never carries state 3: the steal sits on a 512 block start) | thr >= 1 poll, last <= "
                 + juce::String (hold + 32.0, 0) + " ms, empty <= " + juce::String (hold + 64.0, 0) + " ms (32 ms polls) | ctrl UVS1n");
        const bool fired = stim && n.seen3 == 0 && ! vs1Predicate (n, hold);
        row (fired, "UVS1n control: steal hold 0 -> state 3 never seen (predicate must FAIL)",
             "meas state 3 on " + juce::String (n.seen3) + " polls of " + juce::String (n.polls) + " | thr predicate FAILS | "
                 + ctrl (fired, "testSetStealHoldMs (0) -> the steal is invisible"));
    }

    //==========================================================================
    // UVS2 — release and CC64
    void rowVS2()
    {
        Session s (kFs, with (baseMod (1), P { { "amp_release", 400.0f }, { sn (1, "density"), 20.0f } }), kSeed, 256);
        s.load (0, noiseBuf (2, 96000, 0.2f, 404), kFs);
        s.noteOn (4800, 60);
        s.noteOff (24000, 60);
        cc (s, 72000, 64, 127);
        s.noteOn (76800, 64);
        s.noteOff (86400, 64);   // key up, pedal down: still gated
        cc (s, 100800, 64, 0);   // pedal up: release
        s.record = false;
        int held = 0, heldBad = 0, rel = 0, relBad = 0, emptyA = 0, emptyBad = 0, ped = 0, pedBad = 0, rel2 = 0, emptyB = 0;
        int rowA = -1, rowB = -1;
        juce::int64 next = 0;
        s.afterBlock = [&] (Session& ss)
        {
            if (ss.pos < next)
                return;
            next = ss.pos + kPoll;
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            const int kA = [&] { for (int k = 0; k < ogi::kVoiceSlots; ++k) if (! ss.eng().getVoice (k).isFree() && ss.eng().getVoice (k).note == 60) return k; return -1; }();
            const int kB = [&] { for (int k = 0; k < ogi::kVoiceSlots; ++k) if (! ss.eng().getVoice (k).isFree() && ss.eng().getVoice (k).note == 64) return k; return -1; }();
            const int rA = rowOfNote (t, 60, 1, 3), rB = rowOfNote (t, 64, 1, 3);
            if (rA >= 0) rowA = rA;
            if (rB >= 0) rowB = rB;
            if (ss.pos > 4800 && ss.pos < 24000)
            {
                ++held;
                heldBad += (rA >= 0 && t.voices[(size_t) rA].state == 1) ? 0 : 1;
            }
            else if (ss.pos >= 24000 + 256 && ss.pos < 72000)
            {
                if (kA >= 0)
                {
                    ++rel;
                    relBad += (rA >= 0 && t.voices[(size_t) rA].state == 2) ? 0 : 1;
                }
                else
                {
                    ++emptyA;
                    emptyBad += (rA < 0 && rowA >= 0 && t.voices[(size_t) rowA].state == 0 && t.voices[(size_t) rowA].note == -1) ? 0 : 1;
                }
            }
            else if (ss.pos > 86400 + 256 && ss.pos < 100800)
            {
                ++ped;
                pedBad += (kB >= 0 && ss.eng().getVoice (kB).gated && rB >= 0 && t.voices[(size_t) rB].state == 1) ? 0 : 1;
            }
            else if (ss.pos >= 100800 + 256)
            {
                if (kB >= 0)
                {
                    ++rel2;
                    relBad += (rB >= 0 && t.voices[(size_t) rB].state == 2) ? 0 : 1;
                }
                else
                {
                    ++emptyB;
                    emptyBad += (rB < 0 && rowB >= 0 && t.voices[(size_t) rowB].state == 0) ? 0 : 1;
                }
            }
        };
        s.run (144000, 256);
        const bool stim = held > 5 && rel > 5 && emptyA > 5 && ped > 5 && rel2 > 5 && emptyB > 5;
        row (stim && heldBad == 0 && relBad == 0 && emptyBad == 0 && pedBad == 0,
             "UVS2 PD3 release -> state 2, empty after the env ends; CC64 keeps state 1 after key-up",
             "meas held " + juce::String (held - heldBad) + "/" + juce::String (held) + " state 1, released " + juce::String (rel + rel2 - relBad) + "/"
                 + juce::String (rel + rel2) + " state 2, after the env " + juce::String (emptyA + emptyB - emptyBad) + "/" + juce::String (emptyA + emptyB)
                 + " empty, pedal-held after key-up " + juce::String (ped - pedBad) + "/" + juce::String (ped) + " state 1 | thr all | ctrl -");
    }

    //==========================================================================
    // UVS3 — only real steals write the ring
    void rowVS3()
    {
        Session s (kFs, with (baseMod (1), P { { "polyphony", 8.0f }, { sn (1, "density"), 20.0f } }), kSeed, 256);
        s.load (0, noiseBuf (2, 96000, 0.2f, 405), kFs);
        auto& tel = s.eng().getTelemetry();
        const int six[] = { 48, 50, 52, 53, 55, 57 };
        for (int i = 0; i < 6; ++i)
            s.noteOn (4800 + 480 * i, six[i]);
        std::uint32_t c0 = 0, cShrink = 0, cAso = 0, cMono = 0, cSteal0 = 0, cEnd = 0;
        std::uint64_t forcedShrink = 0, steals0 = 0, stealsShrink = 0, stealsMono = 0;
        int countedBeforeAso = 0, countedAfterAso = 0, monoMaxCounted = 0;
        std::set<std::uint64_t> monoSerials;
        s.at (12000, [&] (Session& ss) { c0 = tel.stolenCount.load(); forcedShrink = ss.hk().polyForceReleases; steals0 = ss.hk().voiceSteals; });
        s.at (14400, [] (Session& ss) { setP (*ss.p, "polyphony", 2.0f); });
        s.at (19200, [&] (Session& ss)
        {
            cShrink = tel.stolenCount.load();
            forcedShrink = ss.hk().polyForceReleases - forcedShrink;
            stealsShrink = ss.hk().voiceSteals;
            countedBeforeAso = countedNow (ss);
        });
        cc (s, 21600, 120, 0);
        s.at (24000, [&] (Session& ss)
        {
            cAso = tel.stolenCount.load();
            countedAfterAso = countedNow (ss);
            setP (*ss.p, "polyphony", 8.0f);
            setP (*ss.p, "voice_mode", 1.0f);
        });
        s.noteOn (28800, 60); s.noteOn (33600, 62); s.noteOn (38400, 64);
        for (int nn : { 60, 62, 64 })
            s.noteOff (43200, nn);
        s.at (48000, [&] (Session& ss)
        {
            cMono = tel.stolenCount.load();
            stealsMono = ss.hk().voiceSteals;
            setP (*ss.p, "voice_mode", 0.0f);
            setP (*ss.p, "polyphony", 2.0f);
        });
        s.at (50000, [&] (Session& ss) { cSteal0 = tel.stolenCount.load(); steals0 = ss.hk().voiceSteals; });
        s.noteOn (50400, 65); s.noteOn (52800, 67); s.noteOn (55200, 69); s.noteOn (57600, 71);
        s.afterBlock = [&] (Session& ss)
        {
            if (ss.pos > 28800 && ss.pos < 43200)
            {
                monoMaxCounted = std::max (monoMaxCounted, countedNow (ss));
                for (int k = 0; k < ogi::kVoiceSlots; ++k)
                    if (ss.eng().getVoice (k).isCounted())
                        monoSerials.insert (ss.eng().getVoice (k).serial);
            }
        };
        s.record = false;
        s.run (62400, 256);
        cEnd = tel.stolenCount.load();
        const auto stealsEnd = s.hk().voiceSteals;
        const bool stim = forcedShrink > 0 && countedBeforeAso > 0 && countedAfterAso == 0 && monoMaxCounted == 1 && monoSerials.size() >= 3 && stealsEnd - steals0 == 2;
        const bool ok = cShrink == c0 && cAso == cShrink && cMono == cAso && stealsShrink == 0 && stealsMono == 0 && cSteal0 == cMono
                     && cEnd - cSteal0 == 2u && (std::uint64_t) (cEnd - cSteal0) == stealsEnd - steals0;
        row (stim && ok, "UVS3 PD2 shrink / All Sound Off / Mono handovers leave stolenCount alone; each real steal adds exactly 1",
             "meas stolenCount " + juce::String ((int) c0) + " -> shrink (" + i64 (forcedShrink) + " forced releases) " + juce::String ((int) cShrink)
                 + " -> ASO (" + juce::String (countedBeforeAso) + " -> " + juce::String (countedAfterAso) + " counted) " + juce::String ((int) cAso)
                 + " -> Mono (one cloud, " + juce::String ((int) monoSerials.size()) + " serials: two retrigger handovers) " + juce::String ((int) cMono) + " -> 2 steals "
                 + juce::String ((int) cEnd) + " (engine steals +" + i64 (stealsEnd - steals0) + ") | thr unchanged, unchanged, unchanged, +2 == steals | ctrl -");
    }

    //==========================================================================
    // UWS1 / UWS1n — wantSt column and the grain clamp bit
    struct Ws1 { float col8 = 0.0f, want0 = 0.0f, want1 = 0.0f; int s1 = 0, s1Bit = 0, s2 = 0, s2Bit = 0, rowState = 0; };

    Ws1 ws1Run (bool fromStream0)
    {
        Session s (kFs, with (baseMod (2), P { { sn (1, "density"), 40.0f }, { sn (2, "density"), 40.0f }, { sn (1, "size"), 400.0f },
                                               { sn (2, "size"), 400.0f }, { sn (2, "key_track"), 0.0f } }), kSeed, 512);
        s.p->testClampFromStream0 = fromStream0;
        s.load (0, noiseBuf (2, 96000, 0.2f, 406), kFs);
        s.load (1, noiseBuf (2, 96000, 0.2f, 407), kFs);
        s.noteOn (4800, 98);   // root 48: 50 st above
        s.record = false;
        s.run (48000, 512);
        const auto t = s.p->buildUiTelemetry (nowOf (s));
        const auto& f = s.eng().getTelemetry().frames.read();
        Ws1 w;
        const int r = rowOfNote (t, 98);
        if (r >= 0)
        {
            w.col8 = t.voices[(size_t) r].wantSt;
            w.rowState = t.voices[(size_t) r].state;
        }
        const int k = countedSlotOfNote (s, 98);
        if (k >= 0)
        {
            w.want0 = f.voices[k].wantSt[0];
            w.want1 = f.voices[k].wantSt[1];
        }
        for (int i = 0; i < t.grainCount; ++i)
        {
            const auto& g = t.grains[(size_t) i];
            const bool bit = (g.flags & 2) != 0;
            if (g.stream == 1) { ++w.s1; w.s1Bit += bit ? 1 : 0; }
            if (g.stream == 2) { ++w.s2; w.s2Bit += bit ? 1 : 0; }
        }
        return w;
    }

    bool ws1Predicate (const Ws1& w) { return std::abs (w.col8 - 50.0f) <= 1.0e-3f && w.s1Bit == w.s1 && w.s2Bit == 0; }

    void rowsWS1()
    {
        const auto a = ws1Run (false);
        const auto n = ws1Run (true);
        const bool stim = a.rowState == 1 && a.s1 > 5 && a.s2 > 5 && std::abs (a.want0 - 50.0f) <= 1.0e-3f && std::abs (a.want1) <= 1.0e-3f;
        row (stim && ws1Predicate (a), "UWS1 PD11 column 8 == 50 st (note 98, root 48); grain bit 2 on that stream only (key track 0 stream clear)",
             "meas column 8 " + juce::String (a.col8, 4) + " (frame wantSt s1 " + juce::String (a.want0, 4) + ", s2 " + juce::String (a.want1, 4)
                 + "), stream 1 bit on " + juce::String (a.s1Bit) + "/" + juce::String (a.s1) + " grains, stream 2 bit on " + juce::String (a.s2Bit) + "/"
                 + juce::String (a.s2) + " | thr 50 +-1e-3, all / none | ctrl UWS1n");
        const bool fired = n.s2 > 5 && ! ws1Predicate (n);
        row (fired, "UWS1n control: stream 0's wantSt decides every stream's bit -> the bit on the wrong stream (predicate must FAIL)",
             "meas stream 2 bit on " + juce::String (n.s2Bit) + "/" + juce::String (n.s2) + " grains | thr predicate FAILS | "
                 + ctrl (fired, "testClampFromStream0 -> " + juce::String (n.s2Bit) + " wrong-stream bits"));
    }

    //==========================================================================
    // UME1 — menv through the map
    void rowME1()
    {
        P ps = with (baseMod (1), P { { "menv1_attack", 100.0f }, { "menv1_decay", 200.0f }, { "menv1_sustain", 50.0f }, { "menv1_release", 300.0f },
                                      { "menv2_attack", 50.0f },  { "menv2_decay", 100.0f }, { "menv2_sustain", 80.0f }, { "menv2_release", 600.0f },
                                      { "amp_attack", 80.0f },    { "amp_decay", 150.0f },   { "amp_sustain", 60.0f },   { "amp_release", 250.0f },
                                      { sn (1, "density"), 20.0f } });
        Session s (kFs, ps, kSeed, 256);
        s.load (0, noiseBuf (2, 96000, 0.2f, 408), kFs);
        s.noteOn (4800, 60);
        s.noteOn (9600, 64);
        s.noteOff (48000, 60);
        s.noteOff (52800, 64);
        s.record = false;
        int rows = 0, bad = 0, emptyBad = 0;
        std::array<int, 5> st1 {}, st2 {}, amp {};
        juce::String first;
        juce::int64 next = 0;
        s.afterBlock = [&] (Session& ss)
        {
            if (ss.pos < next)
                return;
            next = ss.pos + 528;   // 11 ms: the short stages are each seen
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            for (int r = 0; r < Tel::kVoices; ++r)
            {
                const auto& v = t.voices[(size_t) r];
                const auto& m = t.menv[(size_t) r];
                if (v.state == 0)
                {
                    emptyBad += (m.e1 == 0.0f && m.e2 == 0.0f && m.st1 == 0 && m.st2 == 0 && m.ampSt == 0) ? 0 : 1;
                    continue;
                }
                const int k = countedSlotOfNote (ss, v.note);
                if (k < 0)
                    continue;
                ++rows;
                const auto& vm = ss.eng().getVoiceModForTest (k);
                const auto& vo = ss.eng().getVoice (k);
                const bool ok = sameF (m.e1, (float) vm.env[0].getLevel()) && sameF (m.e2, (float) vm.env[1].getLevel())
                             && m.st1 == menvStageOf (vm.env[0].getStage()) && m.st2 == menvStageOf (vm.env[1].getStage())
                             && m.ampSt == ampStageOf (vo.env.getStage()) && sameF (v.amp, (float) vo.env.getLevel());
                if (! ok)
                {
                    ++bad;
                    if (first.isEmpty())
                        first = "row " + juce::String (r) + " e1 " + juce::String (m.e1, 5) + "/" + juce::String (vm.env[0].getLevel(), 5) + " st "
                                + juce::String (m.st1) + "/" + juce::String (menvStageOf (vm.env[0].getStage()));
                }
                ++st1[(size_t) juce::jlimit (0, 4, m.st1)];
                ++st2[(size_t) juce::jlimit (0, 4, m.st2)];
                ++amp[(size_t) juce::jlimit (0, 4, m.ampSt)];
            }
        };
        s.run (96000, 256);
        bool allStages = true;
        for (int k = 1; k <= 4; ++k)
            allStages = allStages && st1[(size_t) k] > 0 && st2[(size_t) k] > 0 && amp[(size_t) k] > 0;
        const auto hist = [] (const std::array<int, 5>& h) { return juce::String (h[1]) + "/" + juce::String (h[2]) + "/" + juce::String (h[3]) + "/" + juce::String (h[4]); };
        row (rows > 100 && allStages && bad == 0 && emptyBad == 0, "UME1 PD7 menv levels / stages == the engine's envelopes through the map (scripted ADSR)",
             "meas " + juce::String (rows - bad) + "/" + juce::String (rows) + " rows bit-equal; stages A/D/S/R seen env1 " + hist (st1) + ", env2 " + hist (st2)
                 + ", amp " + hist (amp) + "; empty rows all zero " + yn (emptyBad == 0) + (first.isNotEmpty() ? ", first " + first : juce::String())
                 + " | thr bit-equal, every stage seen | ctrl -");
    }

    //==========================================================================
    // ULF1 — lfoPh
    void rowLF1()
    {
        P ps = with (baseMod (1), P { { "lfo1_mode", 1.0f }, { "lfo1_phase", 90.0f }, { "lfo1_rate", 2.0f },
                                      { "lfo2_mode", 0.0f }, { "lfo2_rate", 1.5f }, { sn (1, "density"), 20.0f } });
        Session s (kFs, ps, kSeed, 256);
        s.load (0, noiseBuf (2, 96000, 0.2f, 409), kFs);
        s.noteOn (4800, 60);
        s.noteOn (14400, 67);
        s.record = false;
        int polls = 0, gBad = 0, pvRows = 0, pvBad = 0, minusBad = 0, emptyBad = 0, distinct = 0;
        double worst = 0.0;
        juce::int64 next = 0;
        s.afterBlock = [&] (Session& ss)
        {
            if (ss.pos < next)
                return;
            next = ss.pos + kPoll;
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            ++polls;
            for (int n = 0; n < 4; ++n)
                gBad += sameF (t.lfoPh[(size_t) n][0], ss.eng().getLfoPhaseTelemetry (n)) ? 0 : 1;
            float ph[2] = { -1.0f, -1.0f };
            int nPv = 0;
            for (int r = 0; r < Tel::kVoices; ++r)
            {
                const auto& v = t.voices[(size_t) r];
                if (v.state == 0)
                {
                    for (int n = 0; n < 4; ++n)
                        emptyBad += t.lfoPh[(size_t) n][(size_t) (1 + r)] == -1.0f ? 0 : 1;
                    continue;
                }
                const int k = countedSlotOfNote (ss, v.note);
                if (k < 0)
                    continue;
                const auto& vm = ss.eng().getVoiceModForTest (k);
                for (int n = 0; n < 4; ++n)
                {
                    const float got = t.lfoPh[(size_t) n][(size_t) (1 + r)];
                    if (n == 0 && vm.lfoOn[0])
                    {
                        const double p = vm.lfo[0].position() - (double) vm.lfo[0].getPhaseDeg() / 360.0;
                        float want = (float) (p - std::floor (p));
                        if (! (want < 1.0f)) want = 0.0f;
                        ++pvRows;
                        worst = std::max (worst, (double) std::abs (got - want));
                        pvBad += sameF (got, want) ? 0 : 1;
                        if (nPv < 2) ph[nPv++] = got;
                    }
                    else
                        minusBad += got == -1.0f ? 0 : 1;   // Global-mode LFOs (2-4)
                }
            }
            distinct += (nPv == 2 && std::abs (ph[0] - ph[1]) > 1.0e-3f) ? 1 : 0;
        };
        s.run (72000, 256);
        row (polls > 30 && pvRows > 30 && distinct > 5 && gBad == 0 && pvBad == 0 && minusBad == 0 && emptyBad == 0,
             "ULF1 PD8 lfoPh: global == getLfoPhaseTelemetry; per-voice == the vm[k].lfo[n] expression; -1 for Global-mode LFOs / empty rows",
             "meas global " + juce::String (polls * 4 - gBad) + "/" + juce::String (polls * 4) + " equal; LFO 1 per-voice (phase 90) " + juce::String (pvRows - pvBad)
                 + "/" + juce::String (pvRows) + " rows bit-equal (worst " + juce::String (worst, 8) + "), two voices apart on " + juce::String (distinct)
                 + " polls; Global-mode entries -1 " + yn (minusBad == 0) + ", empty rows -1 " + yn (emptyBad == 0) + " | thr bit-equal | ctrl -");
    }

    //==========================================================================
    // USRC1 — sources
    void rowSRC1()
    {
        Session s (kFs, with (baseMod (1), P { { sn (1, "density"), 20.0f }, { "lfo1_rate", 2.0f } }), kSeed, 256);
        s.load (0, noiseBuf (2, 96000, 0.2f, 410), kFs);
        s.noteOn (4800, 40);
        s.addMidi (4800, juce::MidiMessage::pitchWheel (1, 2048));   // bend down
        cc (s, 4800, 74, 20);                                          // slide down
        s.noteOff (28800, 40);
        s.noteOn (33600, 92);
        s.addMidi (33600, juce::MidiMessage::pitchWheel (1, 14000));  // bend up
        cc (s, 33600, 74, 110);                                        // slide up
        s.noteOff (57600, 92);
        s.record = false;
        struct Snap { float key = 0.0f, bend = 0.0f, slide = 0.0f; bool any = false; };
        Snap neg, pos, silent;
        int polls = 0, bad = 0;
        juce::int64 next = 0;
        s.afterBlock = [&] (Session& ss)
        {
            if (ss.pos < next)
                return;
            next = ss.pos + kPoll;
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            ++polls;
            for (int i = 0; i < Tel::kSources; ++i)
                bad += sameF (t.sources[(size_t) i], ss.eng().getSourceTelemetry (i + 1)) ? 0 : 1;
            const Snap sn2 { t.sources[ogi::GiSrc::Key - 1], t.sources[ogi::GiSrc::Bend - 1], t.sources[ogi::GiSrc::Slide - 1], true };
            if (ss.pos > 24000 && ss.pos < 28800) neg = sn2;
            if (ss.pos > 52800 && ss.pos < 57600) pos = sn2;
            if (ss.pos > 90000) silent = sn2;
        };
        s.run (96000, 256);
        bool counted = false;
        for (int k = 0; k < ogi::kVoiceSlots; ++k)
            counted = counted || ! s.eng().getVoice (k).isFree();
        const bool signs = neg.any && pos.any && neg.key < 0.0f && neg.bend < 0.0f && neg.slide < 0.0f && pos.key > 0.0f && pos.bend > 0.0f && pos.slide > 0.0f;
        const bool held = silent.any && ! counted && silent.key == pos.key && silent.key != 0.0f;
        row (polls > 50 && bad == 0 && signs && held, "USRC1 PD6 sources[i] == getSourceTelemetry (i + 1); Key / Bend / Slide signs; held when silent",
             "meas " + juce::String (polls * Tel::kSources - bad) + "/" + juce::String (polls * Tel::kSources) + " equal; note 40 / bend down / CC74 20: Key "
                 + juce::String (neg.key, 4) + " Bend " + juce::String (neg.bend, 4) + " Slide " + juce::String (neg.slide, 4) + "; note 92 / up / 110: "
                 + juce::String (pos.key, 4) + " / " + juce::String (pos.bend, 4) + " / " + juce::String (pos.slide, 4) + "; silent (no voice) Key "
                 + juce::String (silent.key, 4) + " held " + yn (held) + " | thr bit-equal, signs, held | ctrl -");
    }

    //==========================================================================
    // UMN1 / UMN1n — routed modNorm
    struct Mn1 { int polls = 0, keyBad = 0, valBad = 0, extra = 0, baseRest = 0, baseBad = 0, voiceMoved = 0; float gLo = 2.0f, gHi = -1.0f; };

    Mn1 mn1Run (bool all)
    {
        // mod1 Env 1 (per-voice) -> s1_level (a per-voice dest): destTel's voice
        //      part is the newest voice's offset; with no voice it is the base.
        // mod2 LFO 1 (global)    -> reverb_mix: moves without notes.
        // mod3 Velocity -> output_gain (a mono route: the global pass reads the
        //      HELD srcG, so it does not rest at base after the last voice --
        //      not asserted), mod4 Macro 1 (depth 0) -> s1_pan: routed (depth ignored).
        P ps = with (baseMod (1), P { { sn (1, "density"), 20.0f }, { "lfo1_rate", 3.0f }, { "menv1_attack", 50.0f }, { "menv1_decay", 300.0f },
                                      { "menv1_sustain", 70.0f } });
        ps = with (ps, route (1, ogi::GiSrc::ModEnv1, 2, 60.0f));
        ps = with (ps, route (2, ogi::GiSrc::Lfo1, 119, 50.0f));
        ps = with (ps, route (3, ogi::GiSrc::Velocity, 1, 40.0f));
        ps = with (ps, route (4, ogi::GiSrc::Macro1, 3, 0.0f));
        Session s (kFs, ps, kSeed, 256);
        s.p->testModNormAll = all;
        s.load (0, noiseBuf (2, 96000, 0.2f, 411), kFs);
        s.noteOn (48000, 60);
        s.noteOff (72000, 60);
        s.record = false;
        auto* og = dynamic_cast<juce::AudioParameterFloat*> (param (*s.p, sn (1, "level")));
        const std::set<int> routed { 1, 2, 3, 119 };
        Mn1 m;
        juce::int64 next = 0;
        s.afterBlock = [&] (Session& ss)
        {
            if (ss.pos < next)
                return;
            next = ss.pos + kPoll;
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            ++m.polls;
            std::set<int> keys;
            for (int d = 1; d < Tel::kDests; ++d)
                if (t.modRouted[(size_t) d])
                {
                    keys.insert (d);
                    m.valBad += sameF (t.modNorm[(size_t) d], ss.eng().getDestTelemetry (d)) ? 0 : 1;
                }
            m.keyBad += keys == routed ? 0 : 1;
            m.extra = std::max (m.extra, (int) keys.size() - (int) routed.size());
            const bool voice = [&] { for (int k = 0; k < ogi::kVoiceSlots; ++k) if (! ss.eng().getVoice (k).isFree()) return true; return false; }();
            const float base = og != nullptr ? og->getNormalisableRange().convertTo0to1 (rawOf (*ss.p, sn (1, "level"))) : -1.0f;
            if (! voice)
            {
                ++m.baseRest;
                m.baseBad += std::abs (t.modNorm[2] - base) <= 1.0e-6f ? 0 : 1;
                if (ss.pos < 48000)
                {
                    m.gLo = std::min (m.gLo, t.modNorm[119]);
                    m.gHi = std::max (m.gHi, t.modNorm[119]);
                }
            }
            else if (std::abs (t.modNorm[2] - base) > 0.01f)
                ++m.voiceMoved;
        };
        s.run (120000, 256);
        return m;
    }

    bool mn1Predicate (const Mn1& m) { return m.keyBad == 0 && m.valBad == 0 && m.baseBad == 0; }

    void rowsMN1()
    {
        const auto a = mn1Run (false);
        const auto n = mn1Run (true);
        const bool stim = a.polls > 60 && a.baseRest > 20 && a.voiceMoved > 5 && a.gHi - a.gLo > 0.1f;
        row (stim && mn1Predicate (a), "UMN1 PD9 modNorm: key set == routed set, values == destTel; per-voice route follows the newest, rests at base; global moves",
             "meas key set == {1, 2, 3, 119} on " + juce::String (a.polls - a.keyBad) + "/" + juce::String (a.polls) + " polls, values bit-equal (" + juce::String (a.valBad)
                 + " bad); Env 1 -> s1_level: at base on " + juce::String (a.baseRest - a.baseBad) + "/" + juce::String (a.baseRest) + " voiceless polls, off base on "
                 + juce::String (a.voiceMoved) + " polls with a voice; LFO 1 -> reverb_mix without notes " + juce::String (a.gLo, 3) + " ... " + juce::String (a.gHi, 3)
                 + " | thr exact | ctrl UMN1n");
        const bool fired = n.polls > 60 && ! mn1Predicate (n) && n.extra > 0;
        row (fired, "UMN1n control: all 125 modNorm keys sent -> extra keys (predicate must FAIL)",
             "meas " + juce::String (n.extra) + " extra keys on " + juce::String (n.keyBad) + " polls | thr predicate FAILS | "
                 + ctrl (fired, "testModNormAll -> +" + juce::String (n.extra) + " keys"));
    }

    //==========================================================================
    // ULH1 / ULH1n — lanes[].lo / hi
    struct Lh1 { int polls = 0, lane0 = 0, lane0Bad = 0, others = 0; float hiLo = 2.0f, hiHi = -1.0f; };

    Lh1 lh1Run (bool always)
    {
        P ps = with (baseMod (2), P { { "lfo1_rate", 2.0f }, { sn (1, "density"), 20.0f } });
        ps = with (ps, route (1, ogi::GiSrc::Lfo1, 28, 30.0f));   // s1_loop_end
        Session s (kFs, ps, kSeed, 256);
        s.p->testLoHiAlways = always;
        s.load (0, noiseBuf (2, 96000, 0.2f, 412), kFs);
        s.load (1, noiseBuf (2, 96000, 0.2f, 413), kFs);
        s.noteOn (4800, 60);
        s.record = false;
        Lh1 l;
        juce::int64 next = 0;
        s.afterBlock = [&] (Session& ss)
        {
            if (ss.pos < next)
                return;
            next = ss.pos + kPoll;
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            ++l.polls;
            const float a = ss.eng().getDestTelemetry (27), b = ss.eng().getDestTelemetry (28);
            const auto& L0 = t.lanes[0];
            ++l.lane0;
            l.lane0Bad += (L0.hasLoHi && sameF (L0.lo, std::min (a, b)) && sameF (L0.hi, std::max (a, b))) ? 0 : 1;
            l.hiLo = std::min (l.hiLo, L0.hi);
            l.hiHi = std::max (l.hiHi, L0.hi);
            for (int s2 = 1; s2 < 4; ++s2)
                l.others += t.lanes[(size_t) s2].hasLoHi ? 1 : 0;
        };
        s.run (72000, 256);
        return l;
    }

    bool lh1Predicate (const Lh1& l) { return l.lane0Bad == 0 && l.others == 0; }

    void rowsLH1()
    {
        const auto a = lh1Run (false);
        const auto n = lh1Run (true);
        const bool stim = a.polls > 30 && a.hiHi - a.hiLo > 0.05f;
        row (stim && lh1Predicate (a), "ULH1 PD10 lanes[].lo / hi only on a lane with a routed loop dest, == min / max of the two destTel",
             "meas lane 1 (LFO 1 -> s1_loop_end) " + juce::String (a.lane0 - a.lane0Bad) + "/" + juce::String (a.lane0) + " polls exact, hi moves "
                 + juce::String (a.hiLo, 3) + " ... " + juce::String (a.hiHi, 3) + "; lanes 2-4 with lo / hi " + juce::String (a.others)
                 + " | thr exact, 0 on unrouted lanes | ctrl ULH1n");
        const bool fired = n.polls > 30 && ! lh1Predicate (n) && n.others > 0;
        row (fired, "ULH1n control: lo / hi always emitted -> the unrouted lanes carry them (predicate must FAIL)",
             "meas " + juce::String (n.others) + " unrouted lane-polls with lo / hi | thr predicate FAILS | "
                 + ctrl (fired, "testLoHiAlways -> " + juce::String (n.others)));
    }

    //==========================================================================
    // UMB1 — mono badge parity
    void rowMB1()
    {
        Session s (kFs, baseMod (1), kSeed, 256);
        s.record = false;
        struct Cfg { const char* name; int mode[4]; };
        const Cfg cfgs[] = { { "Global", { 0, 0, 0, 0 } }, { "Per-voice", { 1, 1, 1, 1 } }, { "mixed 1/3 per-voice", { 1, 0, 1, 0 } } };
        int pairs = 0, bad = 0, monoOn = 0;
        juce::String first, perCfg;
        for (const auto& c : cfgs)
        {
            for (int n = 0; n < 4; ++n)
                setP (*s.p, "lfo" + juce::String (n + 1) + "_mode", (float) c.mode[n]);
            s.run (9600, 256);   // the 20 ms mode crossfade settles (lfoPerVoiceActive follows the mode only when settled)
            int on = 0;
            for (int src = 1; src <= 17; ++src)
                for (int d = 1; d <= 125; ++d)
                {
                    ++pairs;
                    const bool engine = s.eng().srcPerVoiceNowForTest (src) && ! s.eng().destVoiceNowForTest (d);
                    const bool page = pageRule::isPerVoiceSource (src, c.mode) && pageRule::isGlobalDest (d, c.mode);
                    on += page ? 1 : 0;
                    if (engine != page)
                    {
                        ++bad;
                        if (first.isEmpty())
                            first = juce::String (c.name) + " src " + juce::String (src) + " dest " + juce::String (d);
                    }
                }
            monoOn += on;
            perCfg << (perCfg.isEmpty() ? "" : ", ") << c.name << " " << on;
        }
        row (pairs == 3 * 17 * 125 && bad == 0 && monoOn > 0, "UMB1 PD23 mono badge: engine srcPerVoiceNow && ! destVoiceNow == the page rule, 17 x 125 x LFO mode",
             "meas " + juce::String (pairs - bad) + "/" + juce::String (pairs) + " pairs agree (mono-on pairs: " + perCfg + ")" + (first.isNotEmpty() ? ", first " + first : juce::String())
                 + " | thr all | ctrl - (closure [16] pins the transcription)");
    }

    //==========================================================================
    // UMT1 — output peaks bit-exact against the rendered buffer
    void rowMT1()
    {
        Session s (kFs, with (baseMod (2), P { { sn (1, "pan"), -80.0f }, { sn (2, "pan"), 60.0f }, { sn (1, "density"), 30.0f }, { sn (2, "density"), 30.0f },
                                               { sn (2, "level"), -9.0f } }), kSeed, 256);
        s.load (0, noiseBuf (2, 96000, 0.2f, 414), kFs);
        s.load (1, noiseBuf (2, 96000, 0.2f, 415), kFs);
        s.noteOn (4800, 60);
        s.noteOn (24000, 67);
        s.noteOff (60000, 60);
        s.record = false;
        float mL = 0.0f, mR = 0.0f;
        int polls = 0, bad = 0, differ = 0, nonZero = 0, blocks = 0;
        s.afterBlock = [&] (Session& ss)
        {
            const int n = ss.blk.getNumSamples();
            for (int i = 0; i < n; ++i)
            {
                mL = std::max (mL, std::abs (ss.blk.getSample (0, i)));
                mR = std::max (mR, std::abs (ss.blk.getSample (1, i)));
            }
            if (++blocks % 3 != 0)
                return;
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            ++polls;
            bad += (sameF (t.outL, mL) && sameF (t.outR, mR)) ? 0 : 1;
            differ += (! sameF (mL, mR)) ? 1 : 0;
            nonZero += mL > 0.0f ? 1 : 0;
            mL = mR = 0.0f;
        };
        s.run (96000, 256);
        row (polls > 100 && nonZero > 50 && differ > 50 && bad == 0, "UMT1 PD5 out[L] / out[R] == the harness's max |x| of the output since the last poll, bit-exact",
             "meas " + juce::String (polls - bad) + "/" + juce::String (polls) + " polls bit-equal (" + juce::String (nonZero) + " non-zero, L != R on " + juce::String (differ)
                 + "); compared against the processBlock output: the processor writes nothing after Engine::process on channels 0 / 1 "
                   "(PluginProcessor.cpp processBlock), so this IS the engine output | thr bit-exact | ctrl -");
    }

    //==========================================================================
    // UMT2 / UMT2n — L / R separation
    struct Mt2 { int polls = 0; float maxL = 0.0f; double worstRatio = -400.0; };

    Mt2 mt2Run (bool peaksMax)
    {
        Session s (kFs, with (baseMod (1), P { { sn (1, "pan"), -100.0f }, { sn (1, "stereo_spread"), 0.0f }, { "reverb_mix", 0.0f },
                                               { sn (1, "density"), 30.0f } }), kSeed, 256);
        s.p->testPeaksMax = peaksMax;
        s.load (0, noiseBuf (2, 96000, 0.3f, 416), kFs);
        s.noteOn (512, 60);
        s.record = false;
        Mt2 m;
        juce::int64 next = 24000;
        s.afterBlock = [&] (Session& ss)
        {
            if (ss.pos < next)
            {
                if (ss.pos < 24000)
                    ss.p->buildUiTelemetry (nowOf (ss));   // drain the attack
                return;
            }
            next = ss.pos + kPoll;
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            ++m.polls;
            m.maxL = std::max (m.maxL, t.outL);
            if (t.outL > 0.0f)
                m.worstRatio = std::max (m.worstRatio, t.outR > 0.0f ? 20.0 * std::log10 ((double) t.outR / (double) t.outL) : -400.0);
        };
        s.run (72000, 256);
        return m;
    }

    bool mt2Predicate (const Mt2& m) { return m.worstRatio <= -60.0; }

    void rowsMT2()
    {
        const auto a = mt2Run (false);
        const auto n = mt2Run (true);
        const bool stim = a.polls > 20 && a.maxL > 0.01f;
        row (stim && mt2Predicate (a), "UMT2 PD5 one stream hard left: out[R] / out[L] <= -60 dB, out[L] > -40 dBFS",
             "meas out[L] max " + db (a.maxL) + " over " + juce::String (a.polls) + " polls, worst R / L " + juce::String (a.worstRatio, 1)
                 + " dB | thr <= -60 dB, L > -40 dBFS | ctrl UMT2n");
        const bool fired = n.polls > 20 && n.maxL > 0.01f && ! mt2Predicate (n);
        row (fired, "UMT2n control: out sent as [max, max] -> FAILS (predicate must FAIL)",
             "meas worst R / L " + juce::String (n.worstRatio, 1) + " dB | thr predicate FAILS | " + ctrl (fired, "testPeaksMax -> R == L"));
    }

    //==========================================================================
    // UMT3 — SC known levels
    struct Mt3 { float l = -1.0f, r = -1.0f; };

    /** The SC sine on L only (R silent); quarter == true: fs/4 phase 0 (0, A, 0, -A exactly). */
    Mt3 mt3Run (float A, bool quarter)
    {
        Session s (kFs, recBase (0, 0), kSeed, 512);
        s.beforeBlock = [A, quarter] (Session& ss, juce::int64 b0, int n)
        {
            float* l = ss.blk.getWritePointer (0);
            float* r = ss.blk.getWritePointer (1);
            for (int i = 0; i < n; ++i)
            {
                const juce::int64 t = b0 + i;
                if (quarter)
                {
                    const int ph = (int) (t & 3);
                    l[i] = ph == 1 ? A : (ph == 3 ? -A : 0.0f);
                }
                else
                    l[i] = (float) ((double) A * std::sin (2.0 * juce::MathConstants<double>::pi * 997.0 * (double) t / kFs));
                r[i] = 0.0f;
            }
        };
        s.record = false;
        s.run (24000, 512);
        s.p->buildUiTelemetry (nowOf (s));   // drain the ramp-in
        s.run (24000, 512);
        const auto t = s.p->buildUiTelemetry (nowOf (s));
        return { t.rec.peakL, t.rec.peakR };
    }

    void rowMT3()
    {
        const auto a = mt3Run (0.5f, true), b = mt3Run (0.1f, true), c = mt3Run (0.5f, false);
        const auto dbErr = [] (float got, float A) { return got > 0.0f ? std::abs (20.0 * std::log10 ((double) got / (double) A)) : 999.0; };
        const double lo = 0.5 * std::cos (juce::MathConstants<double>::pi * 997.0 / kFs);
        const bool ok = dbErr (a.l, 0.5f) <= 0.01 && a.r == 0.0f && dbErr (b.l, 0.1f) <= 0.01 && b.r == 0.0f && (double) c.l >= lo && c.l <= 0.5f && c.r == 0.0f;
        row (ok, "UMT3 PD25 SC known level: fs/4 sine on L (0.5, 0.1): rec.peak[0] == A +-0.01 dB, [1] == 0; 997 Hz in [A cos (pi f / fs), A]",
             "meas A 0.5 -> " + juce::String (a.l, 7) + " (" + juce::String (dbErr (a.l, 0.5f), 4) + " dB) / R " + juce::String (a.r, 7) + "; A 0.1 -> "
                 + juce::String (b.l, 7) + " (" + juce::String (dbErr (b.l, 0.1f), 4) + " dB) / R " + juce::String (b.r, 7) + "; 997 Hz 0.5 -> "
                 + juce::String (c.l, 7) + " in [" + juce::String (lo, 7) + ", 0.5] / R " + juce::String (c.r, 7) + " | thr +-0.01 dB, R 0 | ctrl -");
    }

    //==========================================================================
    // UMT4 / UMT4n — one reader: the second poll reads 0
    struct Mt4 { float bOutL = 0.0f, bOutR = 0.0f, bScL = 0.0f, bScR = 0.0f, cOutL = 0.0f, cOutR = 0.0f, cScL = 0.0f, cScR = 0.0f; };

    Mt4 mt4Run (bool noExchange)
    {
        Session s (kFs, with (baseMod (1), P { { sn (1, "pan"), 0.0f }, { sn (1, "stereo_spread"), 50.0f }, { sn (1, "density"), 30.0f } }), kSeed, 512);
        s.p->testPeaksNoExchange = noExchange;
        s.load (0, noiseBuf (2, 96000, 0.3f, 417), kFs);
        s.noteOn (512, 60);
        inject (s, ScSig { 0.5f, 418 });
        s.record = false;
        s.run (24064, 512);
        s.p->buildUiTelemetry (nowOf (s));          // a: clears (normal path)
        s.run (512, 512);                            // ONE block
        const auto b = s.p->buildUiTelemetry (nowOf (s));
        const auto c = s.p->buildUiTelemetry (nowOf (s));   // no block since
        return { b.outL, b.outR, b.rec.peakL, b.rec.peakR, c.outL, c.outR, c.rec.peakL, c.rec.peakR };
    }

    bool mt4Predicate (const Mt4& m) { return m.cOutL == 0.0f && m.cOutR == 0.0f && m.cScL == 0.0f && m.cScR == 0.0f; }

    void rowsMT4()
    {
        const auto a = mt4Run (false);
        const auto n = mt4Run (true);
        const bool stim = a.bOutL > 0.0f && a.bOutR > 0.0f && a.bScL > 0.0f && a.bScR > 0.0f;
        row (stim && mt4Predicate (a), "UMT4 PD15 two polls after one block: the second reads 0 on both feeds (out, SC)",
             "meas poll 1 out [" + juce::String (a.bOutL, 4) + ", " + juce::String (a.bOutR, 4) + "] SC [" + juce::String (a.bScL, 4) + ", " + juce::String (a.bScR, 4)
                 + "]; poll 2 out [" + juce::String (a.cOutL, 4) + ", " + juce::String (a.cOutR, 4) + "] SC [" + juce::String (a.cScL, 4) + ", " + juce::String (a.cScR, 4)
                 + "] | thr poll 2 all 0 | ctrl UMT4n");
        const bool fired = n.bOutL > 0.0f && n.bScL > 0.0f && ! mt4Predicate (n);
        row (fired, "UMT4n control: the peaks read without the exchange -> the second read is not 0 (predicate must FAIL)",
             "meas poll 2 out [" + juce::String (n.cOutL, 4) + ", " + juce::String (n.cOutR, 4) + "] SC [" + juce::String (n.cScL, 4) + ", " + juce::String (n.cScR, 4)
                 + "] | thr predicate FAILS | " + ctrl (fired, "testPeaksNoExchange -> stale peaks re-read"));
    }

    //==========================================================================
    // ULM1 — limit
    void rowLM1()
    {
        auto p = make();
        p->prepareToPlay (kFs, 512);
        int got[4] {};
        bool ok = true;
        for (int k = 0; k < 4; ++k)
        {
            setP (*p, "grain_limit", (float) k);
            got[k] = p->buildUiTelemetry (1000.0 + 10.0 * k).limit;
            ok = ok && got[k] == (32 << k) && juce::roundToInt (rawOf (*p, "grain_limit")) == k;
        }
        row (ok, "ULM1 PD12 limit == 32 / 64 / 128 / 256 for grain_limit 0 - 3",
             "meas " + juce::String (got[0]) + " / " + juce::String (got[1]) + " / " + juce::String (got[2]) + " / " + juce::String (got[3]) + " | thr exact | ctrl -");
    }

    //==========================================================================
    // UCM6b / UCM6bn — take names are collision-proof (CM6, D49b)
    struct Cm6b
    {
        int seeded = 0;
        bool collisionPresent = false, failedOk = false, okOk = false;
        juce::String failedName, okName;
        bool failedExists = true, failedSeeded = true, okExists = false, okSeeded = true, distinct = false, failedFlag = false;
        bool sameSlotReuse = false;   // INFO: a later successful take on the SAME slot in the same second
        juce::String sameSlotName;
    };

    Cm6b cm6bRun (bool legacy, const juce::String& tag)
    {
        Cm6b c;
        const auto dir = gTmp33.getChildFile ("ucm6b-" + tag).getChildFile ("Recordings");
        dir.createDirectory();
        Session s (kFs, baseMod (1), kSeed, 512);
        auto& ld = s.p->getLoader();
        ld.setRecordingsDirectory (dir);
        ld.hooks.legacyTakeName.store (legacy);
        s.record = false;
        s.run (2048, 512);

        // Seed now - 1 ... now + 5 s for S1 and S2 (the wall-clock stamp the commit reads).
        std::set<juce::String> seededPaths;
        const auto now = juce::Time::getCurrentTime();
        for (int d = -1; d <= 5; ++d)
            for (int sl = 1; sl <= 2; ++sl)
            {
                const auto stamp = (now + juce::RelativeTime::seconds ((double) d)).formatted ("%Y%m%d-%H%M%S");
                const auto f = dir.getChildFile ("GI-Rec-" + stamp + "-S" + juce::String (sl) + ".wav");
                if (f.replaceWithText ("seeded", false, false, "\n"))
                {
                    ++c.seeded;
                    seededPaths.insert (f.getFullPathName());
                }
            }
        const auto stampNow = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
        c.collisionPresent = dir.getChildFile ("GI-Rec-" + stampNow + "-S1.wav").existsAsFile();

        // A take whose WAV write fails (slot 1), then a succeeding take beside it (slot 2).
        ld.hooks.failWav.store (true);
        c.failedOk = ug32::commitTake (s, 0, 4800, 0.3f, 501);
        ld.hooks.failWav.store (false);
        c.okOk = ug32::commitTake (s, 1, 4800, 0.3f, 502);
        const auto i0 = s.p->getSlotInfo (0), i1 = s.p->getSlotInfo (1);
        const juce::File f0 (i0.source.fromFirstOccurrenceOf ("rec:", false, false)), f1 (i1.source.fromFirstOccurrenceOf ("rec:", false, false));
        c.failedName = f0.getFileName();
        c.okName = f1.getFileName();
        c.failedFlag = i0.wavFailed;
        c.failedExists = f0.existsAsFile();
        c.failedSeeded = seededPaths.count (f0.getFullPathName()) > 0;
        c.okExists = f1.existsAsFile();
        c.okSeeded = seededPaths.count (f1.getFullPathName()) > 0;
        c.distinct = f0 != f1 && i0.source.startsWith ("rec:") && i1.source.startsWith ("rec:");

        // INFO only: a later SUCCESSFUL take on slot 1 (the failed take's lane is
        // replaced; its name is checked against the failed take's intended path).
        if (! legacy)
        {
            const auto failedPath = f0;
            if (ug32::commitTake (s, 0, 4800, 0.3f, 503))
            {
                const juce::File f2 (s.p->getSlotInfo (0).source.fromFirstOccurrenceOf ("rec:", false, false));
                c.sameSlotName = f2.getFileName();
                c.sameSlotReuse = f2 == failedPath;
            }
        }
        return c;
    }

    bool cm6bPredicate (const Cm6b& c) { return ! c.failedExists && ! c.failedSeeded && c.okExists && ! c.okSeeded && c.distinct; }

    void rowsCM6b()
    {
        const auto a = cm6bRun (false, "fix");
        const auto n = cm6bRun (true, "legacy");
        const bool stim = a.seeded == 14 && a.collisionPresent && a.failedOk && a.okOk && a.failedFlag;
        row (stim && cm6bPredicate (a), "UCM6b PD17 take names: a failWav take names no file and no seeded name; the take beside it is distinct",
             "meas seeded " + juce::String (a.seeded) + " (now - 1 ... now + 5 s, S1 / S2; this second's S1 name present before the take " + yn (a.collisionPresent)
                 + "); failed take '" + a.failedName + "' wavFailed " + yn (a.failedFlag) + ", file exists " + (a.failedExists ? "YES" : "no") + ", a seeded name "
                 + (a.failedSeeded ? "YES" : "no") + "; take beside it '" + a.okName + "' exists " + yn (a.okExists) + ", seeded " + (a.okSeeded ? "YES" : "no")
                 + ", distinct " + yn (a.distinct) + " | thr no file, no seeded name, distinct | ctrl UCM6bn");
        const bool fired = n.seeded == 14 && n.collisionPresent && n.failedOk && ! cm6bPredicate (n) && n.failedSeeded;
        row (fired, "UCM6bn control: legacyTakeName (sibling chosen inside the write branch) -> the failed take names a seeded file (predicate must FAIL)",
             "meas failed take '" + n.failedName + "' a seeded name " + (n.failedSeeded ? "YES" : "no") + ", that file exists " + (n.failedExists ? "YES" : "no")
                 + " | thr predicate FAILS | " + ctrl (fired, "legacyTakeName -> rec: points at another take's file"));
        std::printf ("  INFO UCM6b-same-slot a later successful take on the failed take's slot in the same second: '%s' %s the failed take's intended name "
                     "(the failed name is never created on disk, so getNonexistentSibling can hand it out again; the new take REPLACES that slot's reference, "
                     "so only a session / preset saved inside that second could still name it; S<n> keeps other slots apart; PD17 scope is failed-vs-existing)\n",
                     a.sameSlotName.toRawUTF8(), a.sameSlotReuse ? "REUSES" : "differs from");
        std::fflush (stdout);
    }

    //==========================================================================
    // UAL2 — allocations with the 3.3 publish path
    void rowAL2()
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

        P ps = with (fullPool(), P { { "lfo1_mode", 1.0f }, { "lfo2_mode", 1.0f }, { "lfo1_rate", 3.0f }, { "lfo2_rate", 1.0f } });
        ps = with (ps, route (1, ogi::GiSrc::Lfo1, 3, 30.0f));
        ps = with (ps, route (2, ogi::GiSrc::ModEnv1, 1, 30.0f));
        Session s (kFs, ps, kSeed, 256);
        s.record = false;
        for (int k = 0; k < 4; ++k)
            s.load (k, noiseBuf (2, 96000, 0.2f, 431 + k), kFs);
        inject (s, ScSig { 0.4f, 435 });
        for (int n = 0; n < 16; ++n)
            s.noteOn (64 + 64 * n, 40 + n);
        for (int n = 0; n < 12; ++n)   // steals while armed (ring writes)
            s.noteOn (30000 + 2400 * n, 60 + n);
        int maxGrains = 0, maxLive = 0, pvLfo = 0, polls = 0, shownStolen = 0;
        float maxOutL = 0.0f, maxOutR = 0.0f, maxScL = 0.0f, maxScR = 0.0f;
        std::uint32_t ring0 = 0;
        std::uint64_t steals0 = 0;
        s.at (24000, [&] (Session& ss) { ring0 = ss.eng().getTelemetry().stolenCount.load(); steals0 = ss.hk().voiceSteals; ss.arm = true; });
        s.afterBlock = [&] (Session& ss)
        {
            // A poll block builds FIRST (frames.read() consumes the fresh flag;
            // the builder must see this block's frame), then re-reads the same
            // frame; other blocks only peek.
            const bool poll = ss.blocksDone % 6 == 0;
            Tel t;
            if (poll)
                t = ss.p->buildUiTelemetry (nowOf (ss));
            const auto& f = ss.eng().getTelemetry().frames.read();
            maxGrains = std::max (maxGrains, (int) f.count);
            int live = 0;
            for (int k = 0; k < (int) f.count; ++k)
                live += (f.grains[k].flags & 2u) == 0 ? 1 : 0;
            maxLive = std::max (maxLive, live);
            for (int k = 0; k < ogi::kVoiceSlots; ++k)
                pvLfo += (f.voices[k].lfoPh[0] >= 0.0f && f.voices[k].lfoPh[1] >= 0.0f) ? 1 : 0;
            if (poll)   // the page's pull (outside the armed processBlock)
            {
                ++polls;
                maxOutL = std::max (maxOutL, t.outL);
                maxOutR = std::max (maxOutR, t.outR);
                maxScL = std::max (maxScL, t.rec.peakL);
                maxScR = std::max (maxScR, t.rec.peakR);
                for (const auto& v : t.voices)
                    shownStolen += v.state == 3 ? 1 : 0;
            }
        };
        s.run (60000 + 4800, 256);
        s.arm = false;
        s.afterBlock = nullptr;
        malloc_logger = nullptr;
        const int allocs = allocCount;
        const auto ringW = s.eng().getTelemetry().stolenCount.load() - ring0;
        const auto steals = s.hk().voiceSteals - steals0;

        const bool stim = maxLive == 256 && maxGrains >= 256 && steals >= 10 && ringW == (std::uint32_t) steals && pvLfo > 0 && polls > 20
                       && maxOutL > 0.0f && maxOutR > 0.0f && maxScL > 0.0f && maxScR > 0.0f;
        row (stim && allocs == 0 && liveness > 0, "UAL2 PERF-01 0 audio-thread allocations: 256 grains, 16 voices, steals, per-voice LFOs, L / R peaks publishing",
             "meas " + juce::String (allocs) + " allocs; frame max " + juce::String (maxGrains) + " grains (" + juce::String (maxLive) + " live), " + i64 (steals)
                 + " steals while armed (ring +" + juce::String ((int) ringW) + "), per-voice LFO 1 + 2 slot observations " + juce::String (pvLfo) + ", " + juce::String (polls)
                 + " polls (stolen rows shown " + juce::String (shownStolen) + ": at polyphony 16 the new voice takes the victim's row, PD1 accepted limit), out max [" + juce::String (maxOutL, 3) + ", " + juce::String (maxOutR, 3)
                 + "], SC max [" + juce::String (maxScL, 3) + ", " + juce::String (maxScR, 3) + "] | thr 0 | "
                 + ctrl (liveness > 0, "liveness malloc(64) armed -> " + juce::String (liveness)));
       #else
        row (true, "UAL2 (mac only)", "skipped");
       #endif
    }

    //==========================================================================
    // UTL33 — the 3.3 telemetry shape, as the page receives it (through JSON text)
    juce::String tl33Problems (const Tel& src, int& routedKeys)
    {
        const auto text = juce::JSON::toString (GiUiJson::telemetryToVar (src), false);
        const auto t = juce::JSON::parse (text);
        juce::StringArray bad;
        const auto near = [] (const juce::var& v, double x) { return isNumber (v) && std::abs (static_cast<double> (v) - x) <= 5.0e-5; };
        const juce::StringArray topKeys { "scaleRev", "slotRev", "lanes", "voices", "newest", "menv", "lfoPh", "grains", "counts", "limit", "out",
                                          "rec", "sources", "modNorm" };
        if (keysOf (t) != topKeys)
            bad.add ("top keys " + keysOf (t).joinIntoString (","));

        const auto lanes = t["lanes"];
        for (int i = 0; i < 4 && lanes.isArray() && lanes.size() == 4; ++i)
        {
            const auto& sl = src.lanes[(size_t) i];
            const juce::StringArray want = sl.hasLoHi ? juce::StringArray { "ovRev", "fb", "rec", "lo", "hi" } : juce::StringArray { "ovRev", "fb", "rec" };
            if (keysOf (lanes[i]) != want)
                bad.add ("lane " + juce::String (i) + " keys " + keysOf (lanes[i]).joinIntoString (","));
            if (sl.hasLoHi && ! (near (lanes[i]["lo"], sl.lo) && near (lanes[i]["hi"], sl.hi)))
                bad.add ("lane " + juce::String (i) + " lo / hi");
        }

        const auto voices = t["voices"];
        if (! voices.isArray() || voices.size() != 16)
            bad.add ("voices not 16");
        else
            for (int k = 0; k < 16; ++k)
            {
                const auto v = voices[k];
                const auto& sv = src.voices[(size_t) k];
                bool ok = v.isArray() && v.size() == 8 && v[0].isInt() && v[1].isInt() && static_cast<int> (v[0]) == sv.state && static_cast<int> (v[1]) == sv.note
                       && near (v[2], sv.amp) && near (v[7], sv.wantSt);
                for (int c = 0; ok && c < 4; ++c)
                    ok = near (v[3 + c], sv.scan[(size_t) c]);
                if (! ok)
                    bad.add ("voice row " + juce::String (k));
            }
        if (! (t["newest"].isInt() && static_cast<int> (t["newest"]) == src.newest))
            bad.add ("newest");

        const auto menv = t["menv"];
        if (! menv.isArray() || menv.size() != 16)
            bad.add ("menv not 16");
        else
            for (int k = 0; k < 16; ++k)
            {
                const auto m = menv[k];
                const auto& sm = src.menv[(size_t) k];
                if (! (m.isArray() && m.size() == 5 && near (m[0], sm.e1) && m[1].isInt() && static_cast<int> (m[1]) == sm.st1 && near (m[2], sm.e2)
                       && m[3].isInt() && static_cast<int> (m[3]) == sm.st2 && m[4].isInt() && static_cast<int> (m[4]) == sm.ampSt))
                    bad.add ("menv row " + juce::String (k));
            }

        const auto lfoPh = t["lfoPh"];
        if (! lfoPh.isArray() || lfoPh.size() != 4)
            bad.add ("lfoPh not 4");
        else
            for (int n = 0; n < 4; ++n)
            {
                bool ok = lfoPh[n].isArray() && lfoPh[n].size() == 17;
                for (int c = 0; ok && c < 17; ++c)
                    ok = near (lfoPh[n][c], src.lfoPh[(size_t) n][(size_t) c]);
                if (! ok)
                    bad.add ("lfoPh row " + juce::String (n));
            }

        const auto grains = t["grains"];
        if (! grains.isArray() || grains.size() != src.grainCount)
            bad.add ("grains");
        else
            for (int k = 0; k < grains.size(); ++k)
            {
                const auto g = grains[k];
                const auto& sg = src.grains[(size_t) k];
                if (! (g.isArray() && g.size() == 8 && g[6].isInt() && static_cast<int> (g[6]) == sg.voice && g[7].isInt() && static_cast<int> (g[7]) == sg.flags
                       && static_cast<int> (g[6]) >= -1 && static_cast<int> (g[6]) < 16 && (static_cast<int> (g[7]) & ~3) == 0))
                {
                    bad.add ("grain row " + juce::String (k));
                    break;
                }
            }

        if (! (t["limit"].isInt() && static_cast<int> (t["limit"]) == src.limit))
            bad.add ("limit");
        if (! (t["out"].isArray() && t["out"].size() == 2 && near (t["out"][0], src.outL) && near (t["out"][1], src.outR)))
            bad.add ("out");
        const auto r = t["rec"];
        if (keysOf (r) != juce::StringArray { "bus", "signal", "run", "slot", "pos", "len", "capped", "peak" })
            bad.add ("rec keys");
        if (! (r["peak"].isArray() && r["peak"].size() == 2 && near (r["peak"][0], src.rec.peakL) && near (r["peak"][1], src.rec.peakR)))
            bad.add ("rec.peak");
        const auto sources = t["sources"];
        bool sOk = sources.isArray() && sources.size() == 17;
        for (int i = 0; sOk && i < 17; ++i)
            sOk = near (sources[i], src.sources[(size_t) i]);
        if (! sOk)
            bad.add ("sources");

        const auto mn = t["modNorm"];
        routedKeys = 0;
        if (mn.getDynamicObject() == nullptr)
            bad.add ("modNorm not an object");
        else
        {
            juce::StringArray want;
            for (int d = 1; d < Tel::kDests; ++d)
                if (src.modRouted[(size_t) d])
                    want.add (juce::String (d));
            const auto got = keysOf (mn);
            routedKeys = got.size();
            bool ok = got == want;
            for (const auto& k : got)
                ok = ok && k.containsOnly ("0123456789") && k.getIntValue() >= 1 && k.getIntValue() <= 125 && near (mn[juce::Identifier (k)], src.modNorm[(size_t) k.getIntValue()]);
            if (! ok)
                bad.add ("modNorm keys " + got.joinIntoString (","));
        }
        for (const char* k : { "frozen", "modRouted", "stale" })
            if (t.hasProperty (k))
                bad.add (juce::String (k) + " present");
        if (std::regex_search (text.toStdString(), std::regex ("[0-9]\\.[0-9]{5,}|[0-9][eE][-+]?[0-9]")))
            bad.add ("a number with more than 4 decimals / an exponent");
        return bad.joinIntoString ("; ");
    }

    void rowTL33()
    {
        Tally t;
        juce::String d;
        // (1) defaults: nothing loaded, no block.
        {
            auto p = make();
            p->prepareToPlay (kFs, 512);
            const auto tel = p->buildUiTelemetry (1000.0);
            int keys = 0;
            const auto problems = tl33Problems (tel, keys);
            t.check (problems.isEmpty() && keys == 0 && tel.newest == -1 && tel.limit == 256, "defaults: " + problems);
            d << "defaults: 14 keys, 16 empty rows, modNorm {}";
        }
        // (2) live: routes, per-voice LFO 1, a released and a stolen row, SC on L.
        {
            P ps = with (baseMod (2), P { { "polyphony", 4.0f }, { "lfo1_mode", 1.0f }, { "lfo1_rate", 2.0f }, { "amp_release", 400.0f },
                                          { sn (1, "density"), 30.0f }, { sn (2, "density"), 30.0f } });
            ps = with (ps, route (1, ogi::GiSrc::Lfo1, 28, 30.0f));
            ps = with (ps, route (2, ogi::GiSrc::ModEnv2, 1, 50.0f));
            ps = with (ps, route (3, ogi::GiSrc::Lfo1, 119, 20.0f));
            Session s (kFs, ps, kSeed, 256);
            s.load (0, noiseBuf (2, 96000, 0.2f, 441), kFs);
            s.load (1, noiseBuf (2, 96000, 0.2f, 442), kFs);
            s.beforeBlock = [] (Session& ss, juce::int64 b0, int n)
            {
                for (int i = 0; i < n; ++i)
                {
                    ss.blk.setSample (0, i, 0.4f * eg21::hashNoise (b0 + i, 0, 1.0f));
                    ss.blk.setSample (1, i, 0.1f * eg21::hashNoise (b0 + i, 1, 1.0f));
                }
            };
            for (int i = 0; i < 4; ++i)
                s.noteOn (4800 * (i + 1), 60 + 4 * i);
            s.noteOn (24000, 98);        // polyphony 4: a steal; 50 st above root
            s.noteOff (24576, 98);       // the newest releases (400 ms tail): a state 2 row
            juce::int64 next = 0;
            s.afterBlock = [&next] (Session& ss)   // the page's pull: the map learns every voice before the steal
            {
                if (ss.pos < next)
                    return;
                next = ss.pos + kPoll;
                ss.p->buildUiTelemetry (nowOf (ss));
            };
            s.record = false;
            s.run (24000 + 2048, 256);
            s.afterBlock = nullptr;
            const auto tel = s.p->buildUiTelemetry (nowOf (s));
            int keys = 0;
            const auto problems = tl33Problems (tel, keys);
            std::array<int, 4> st {};
            int pv = 0, bit2 = 0;
            for (const auto& v : tel.voices)
                ++st[(size_t) juce::jlimit (0, 3, v.state)];
            for (int r = 0; r < 16; ++r)
                pv += tel.lfoPh[0][(size_t) (1 + r)] >= 0.0f ? 1 : 0;
            for (int i = 0; i < tel.grainCount; ++i)
                bit2 += (tel.grains[(size_t) i].flags & 2) ? 1 : 0;
            const bool stim = st[1] > 0 && st[2] > 0 && st[3] > 0 && tel.lanes[0].hasLoHi && keys == 3 && pv > 0 && tel.rec.peakL > tel.rec.peakR && tel.rec.peakR > 0.0f
                           && tel.newest >= 0;
            t.check (stim && problems.isEmpty(), "live: states " + juce::String (st[1]) + "/" + juce::String (st[2]) + "/" + juce::String (st[3]) + " keys "
                                                     + juce::String (keys) + " pv " + juce::String (pv) + " " + problems);
            d << "; live: rows state 1/2/3 = " << st[1] << "/" << st[2] << "/" << st[3] << ", newest " << tel.newest << ", modNorm keys " << keys
              << ", lane 1 lo / hi " << juce::String (tel.lanes[0].lo, 3) << " / " << juce::String (tel.lanes[0].hi, 3) << ", per-voice LFO 1 rows " << pv
              << ", grain bit 2 on " << bit2 << ", rec.peak [" << juce::String (tel.rec.peakL, 3) << ", " << juce::String (tel.rec.peakR, 3) << "]";
        }
        row (t.bad == 0, "UTL33 PD13 telemetryToVar 3.3 shape: 14 keys, 16 x 8 voices, 16 x 5 menv, 4 x 17 lfoPh, 17 sources, out / peak L R, modNorm routed",
             "meas " + t.detail (2) + ", " + d + " | thr exact keys, values == the struct within 5e-5, lo / hi only when routed, frozen / modRouted / stale absent, "
                 "4 decimals | ctrl - (case 1 empty vs case 2 live)");
    }

    //==========================================================================
    // INFO UPL2 / UPB2 — never fail, never counted
    void infoRows()
    {
        // UPL2: 16 routes, 256 grains, 16 rows, per-voice LFOs.
        {
            P ps = with (fullPool(), P { { "feedback", 60.0f }, { "lfo1_mode", 1.0f }, { "lfo2_mode", 1.0f } });
            for (int n = 1; n <= 4; ++n)
                ps = with (ps, P { { sn (n, "stereo_spread"), 100.0f }, { sn (n, "gain_rand"), 50.0f }, { sn (n, "reverse_prob"), 30.0f } });
            for (int m = 1; m <= 16; ++m)   // 16 routes to 16 distinct destinations, loop ends among them
                ps = with (ps, route (m, 1 + (m - 1) % 17, m <= 4 ? 28 * m : 2 + 7 * m, 25.0f));
            Session s (kFs, ps, kSeed, 256);
            s.record = false;
            eg22::loadNoise4 (s, 4.0, 0.2f);
            for (int n = 0; n < 16; ++n)
                s.noteOn (64, 40 + n);
            int widest = 0, widestGrains = 0, widestKeys = 0;
            s.afterBlock = [&] (Session& ss)
            {
                if (ss.pos < 24000)
                    return;
                const auto t = ss.p->buildUiTelemetry (nowOf (ss));
                if (t.grainCount < widestGrains)
                    return;
                const auto text = juce::JSON::toString (GiUiJson::telemetryToVar (t), true);
                if ((int) text.getNumBytesAsUTF8() > widest)
                {
                    widest = (int) text.getNumBytesAsUTF8();
                    widestGrains = t.grainCount;
                    widestKeys = 0;
                    for (const bool b : t.modRouted)
                        widestKeys += b ? 1 : 0;
                }
            };
            s.run (72000, 256);
            s.afterBlock = nullptr;
            constexpr int kIter = 300;
            double buildMs = 0.0, encodeMs = 0.0, textMs = 0.0;
            for (int k = 0; k < kIter; ++k)
            {
                s.run (256, 256);
                const double a = juce::Time::getMillisecondCounterHiRes();
                const auto t = s.p->buildUiTelemetry (nowOf (s));
                const double b = juce::Time::getMillisecondCounterHiRes();
                const auto v = GiUiJson::telemetryToVar (t);
                const double c = juce::Time::getMillisecondCounterHiRes();
                const auto text = juce::JSON::toString (v, true);
                const double e = juce::Time::getMillisecondCounterHiRes();
                buildMs += b - a;
                encodeMs += c - b;
                textMs += e - c;
                juce::ignoreUnused (text);
            }
            const double total = (buildMs + encodeMs + textMs) / kIter;
            std::printf ("  INFO UPL2 widest 3.3 telemetry: %d bytes of one-line JSON (%d grain rows, 16 voice / menv rows, 4 x 17 lfoPh, %d modNorm keys); per pull over %d "
                         "pulls: buildUiTelemetry %.1f us, telemetryToVar %.1f us, JSON text %.1f us, total %.1f us%s\n",
                         widest, widestGrains, widestKeys, kIter, 1000.0 * buildMs / kIter, 1000.0 * encodeMs / kIter, 1000.0 * textMs / kIter, 1000.0 * total,
                         total > 1.0 ? "  ** above 1 ms per pull: report (PLAN-3.3 PD21) **" : "");
        }
        // UPB2: the publish loop's voice part, 3.2 fields vs 3.2 + 3.3 fields, 20
        // slots x 4 streams, every field computed for every slot (worst case:
        // the engine skips free slots). A harness re-enactment through the test
        // accessors (voiceSemis via getVoiceSemisForTest); the engine's own loop
        // is private.
        {
            P ps = with (fullPool(), P { { "lfo1_mode", 1.0f }, { "lfo2_mode", 1.0f }, { "lfo3_mode", 1.0f }, { "lfo4_mode", 1.0f } });
            Session s (kFs, ps, kSeed, 256);
            s.record = false;
            eg22::loadNoise4 (s, 4.0, 0.2f);
            for (int n = 0; n < 16; ++n)
                s.noteOn (64, 40 + n);
            for (int n = 0; n < 4; ++n)
                s.noteOn (48100 + n, 60 + n);   // 4 steals 28 samples before the run ends: their 5 ms fades still run, 20 slots non-free
            s.run (48128, 256);
            int nonFree = 0;
            for (int k = 0; k < ogi::kVoiceSlots; ++k)
                nonFree += s.eng().getVoice (k).isFree() ? 0 : 1;
            auto& e = s.eng();
            const auto absPos = e.getSamplePosition();
            std::vector<ogi::GiUiVoice> fr ((size_t) ogi::kVoiceSlots);
            constexpr int kIter = 20000;
            volatile float sink = 0.0f;
            const double a = juce::Time::getMillisecondCounterHiRes();
            for (int it = 0; it < kIter; ++it)
            {
                for (int k = 0; k < ogi::kVoiceSlots; ++k)
                {
                    const auto& v = e.getVoice (k);
                    auto& u = fr[(size_t) k];
                    u.note = v.isCounted() ? (std::int8_t) std::clamp (v.note, 0, 127) : (std::int8_t) -1;
                    u.amp = (float) v.env.getLevel();
                    for (int st = 0; st < 4; ++st)
                        u.scan[st] = (float) v.scan[st].at (absPos);
                }
                sink = sink + fr[(size_t) (it % ogi::kVoiceSlots)].amp;
            }
            const double b = juce::Time::getMillisecondCounterHiRes();
            for (int it = 0; it < kIter; ++it)
            {
                for (int k = 0; k < ogi::kVoiceSlots; ++k)
                {
                    const auto& v = e.getVoice (k);
                    const auto& vm = e.getVoiceModForTest (k);
                    auto& u = fr[(size_t) k];
                    u.note = v.isCounted() ? (std::int8_t) std::clamp (v.note, 0, 127) : (std::int8_t) -1;
                    u.amp = (float) v.env.getLevel();
                    for (int st = 0; st < 4; ++st)
                        u.scan[st] = (float) v.scan[st].at (absPos);
                    u.state = v.isFree() ? 0 : v.isStolen() ? 3 : (v.gated ? 1 : 2);
                    u.serial = v.serial;
                    u.ampStage = (std::uint8_t) ampStageOf (v.env.getStage());
                    for (int m = 0; m < 2; ++m)
                    {
                        u.menv[m] = (float) vm.env[m].getLevel();
                        u.menvStage[m] = (std::uint8_t) menvStageOf (vm.env[m].getStage());
                    }
                    for (int n = 0; n < 4; ++n)
                    {
                        const double p = vm.lfo[n].position() - (double) vm.lfo[n].getPhaseDeg() / 360.0;
                        u.lfoPh[n] = vm.lfoOn[n] ? (float) (p - std::floor (p)) : -1.0f;
                    }
                    for (int st = 0; st < 4; ++st)
                        u.wantSt[st] = (float) e.getVoiceSemisForTest (k, st, absPos);
                }
                sink = sink + fr[(size_t) (it % ogi::kVoiceSlots)].wantSt[0];
            }
            const double c = juce::Time::getMillisecondCounterHiRes();
            const double oldNs = 1.0e6 * (b - a) / kIter, newNs = 1.0e6 * (c - b) / kIter;
            const double blockNs = 1.0e9 * 256.0 / kFs;
            std::printf ("  INFO UPB2 publish voice loop at %d non-free slots x 4 streams (harness re-enactment; every field for every slot): 3.2 fields %.0f ns per block, "
                         "3.2 + 3.3 fields %.0f ns per block (%+.0f ns = %.4f %% of a 256-frame block at 48 k); GiUiVoice %d bytes, GiUiFrame %d bytes\n",
                         nonFree, oldNs, newNs, newNs - oldNs, 100.0 * (newNs - oldNs) / blockNs, (int) sizeof (ogi::GiUiVoice), (int) sizeof (ogi::GiUiFrame));
        }
        std::fflush (stdout);
    }

    //==========================================================================
    void runAll()
    {
        gTmp33 = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ogi-ug33-" + juce::String (juce::Time::currentTimeMillis()));
        gTmp33.createDirectory();
        struct Guard { ~Guard() { gTmp33.deleteRecursively(); } } guard;

        struct NullLogger final : juce::Logger { void logMessage (const juce::String&) override {} } nullLogger;
        struct LoggerScope
        {
            explicit LoggerScope (juce::Logger* l) : previous (juce::Logger::getCurrentLogger()) { juce::Logger::setCurrentLogger (l); }
            ~LoggerScope() { juce::Logger::setCurrentLogger (previous); }
            juce::Logger* previous;
        } loggerScope (&nullLogger);

        section ("3.3  voice rows / steals / MOD telemetry / meters / take names (PLAN-3.3 T5)");
        if (want ("UVR"))   { rowsVR1(); rowsVR2(); }
        if (want ("UVS"))   { rowsVS1(); rowVS2(); rowVS3(); }
        if (want ("UWS"))   rowsWS1();
        if (want ("UME"))   rowME1();
        if (want ("ULF"))   rowLF1();
        if (want ("USRC"))  rowSRC1();
        if (want ("UMN"))   rowsMN1();
        if (want ("ULH"))   rowsLH1();
        if (want ("UMB"))   rowMB1();
        if (want ("UMT"))   { rowMT1(); rowsMT2(); rowMT3(); rowsMT4(); }
        if (want ("ULM"))   rowLM1();
        if (want ("UCM6b")) rowsCM6b();
        if (want ("UAL2"))  rowAL2();
        if (want ("UTL33")) rowTL33();
        if (want ("UPL2") || want ("UPB2")) infoRows();
    }
} }
