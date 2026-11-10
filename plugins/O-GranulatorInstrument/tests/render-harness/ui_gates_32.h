/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 3 / Phase 3.2 processor-surface
    gates (PLAN-3.2 PD23, T6). Processor only: the harness never compiles the
    editor. Included by main.cpp AFTER ui_gates_31.h.

    Row IDs carry the U prefix of ui_gates_31.h. OGI_ROWS groups: UGD, UCN, UPH,
    UFB, UOV, URC, UCL, UPM6c, UIM, USV, UEB, UST, UAL, UTL32, UPL (the two INFO
    rows).

      UGD1   every emitted dot == the pool grain it was copied from (normPos
             bitwise, stream 1-based, voice, reversed bit) and == the spawn
             log's startPos + dir * rate * (age + frac)
      UGD1n  the frame's stream left 0-based -> no stream-1 dot (must FAIL)
      UGD2   spread 0 -> pan exactly 0, gain exactly 1, env == Hann (phase);
             spread 100 + gain_rand 50 + window 0.6 / skew 30 -> panGains
             inverse, env == the bank curve from the PARAMETERS; stream pan
             -100 / +100 / +50 -> -1 / +1 / 0.5 exactly (added once)
      UCN1   counts[s] == an independent pool census every frame while two
             tape swaps kill grains and the grain limit drops 256 -> 32; limit 0
      UCN1n  killing / fade grains counted -> sum > census (must FAIL)
      UPH1   a held note: scan of a stream with a tape inside its loop region,
             -1 for a stream without a tape; free rows state 0 / note -1 / -1
      UPH1n  the frame's own scan for a stream without a tape is 0 (must FAIL)
      UFB1   fb null at feedback 0, == headPos / frames and moving with
             feedback on, non-null and still when frozen, null on lanes
             without a tape
      UFB1n  the "head not parked" rule alone -> fb null when frozen (must FAIL)
      UOV2   feedback writes: ovRev moves, live bins replace the pristine fold,
             dirty > 0 only where a bin differs, slots[i].dirty + slotRev
             flip; after Reset dirty 0 and the flag clears
      UOV2n  testOvRevNoWrites -> ovRev static while the head writes (must FAIL)
      UOV2n-b testSlotRevNoDirty -> slotRev static when the flag flips (must FAIL)
      URC1   a scripted take: run 0 -> 1 -> 2 -> 3 -> 4 (held 4000 scripted ms)
             -> 0; pos monotonic to 1; lanes[target].rec only while run == 2;
             a silent take: 0 -> 2 -> 5 (held) -> 0
      URC1n  the hold never expires (frozen clock) -> 4 / 5 forever, the
             "no signal" state stays masked (must FAIL)
      URC2   rec.signal true within 2000 ms of a peak >= kPresenceLin, false
             after, false at -95 dBFS; rec.capped across F_max at two tempi ==
             the recorder's own truncation of a real take
      UCL1   clearSlot: a loaded slot empties through the sentinel; refused
             while Recording / Committing into it (Armed allowed); an in-flight
             load is superseded; badSlot
      UPM6c  D45: a preset listing only slot 3 leaves slot 1's tape, generation
             and evolved working copy alone
      UPM6cn restoreSlots (list, true) -> slot 1 cleared (must FAIL)
      UIM1   drop import: Imports/<XXH64 of the file>.<ext>, source file:, the
             dropped name, a repeat drop reuses the file, session round trip,
             name survives a rate rebuild; non-ASCII directory
      UIM1n  empty display name -> the temp file's name shown (must FAIL)
      UIM2   import failure (Imports dir / move / rename): slot unchanged,
             error set, no temp-path source, no .part, nothing left behind
      UIM3   Xxh64Stream == xxh64 for 0 ... 65537 and 3 MiB in every chunking;
             equal contentHash + different tails -> two Imports files; a
             superseded import leaves no .part
      USV1   save at feedback 0 == the tape's level 0 through the same 24-bit
             writer, bitwise (hot take: clipped at +-1); 24-bit stereo at the
             tape rate; with feedback on every saved sample == the tape before
             or after the block that copied its chunk; guards notRecording /
             empty / inUse (own take, another slot's file); tape swap -> changed
      USV2   lifetime: audio stopped -> timeout (real 2000 ms) with the buffer
             held, a second save -> busy, audio resumes -> acknowledged and
             freed, a third save succeeds; prepareToPlay reclaims; destruction
             with a lent buffer
      UEB1   encodedBytes 0 before an embed, == the blob's decoded size after
             (file, take, restored state), 16 MiB + 1 when too large
      UST1   no processBlock for >= 250 scripted ms -> 20 free voice rows, no
             grains, zero counts; the next block resumes
      UAL1   0 audio-thread allocations with 256 grains publishing and a tape
             snapshot copying
      UTL32  telemetryToVar (UiTelemetry): the exact PD13 key set and shapes
             through JSON text; fb / rec null when absent; 4 decimals
      INFO UPL1  widest telemetry JSON bytes + build / encode / text cost
      INFO UPB1  frame-copy cost, old 12-byte grain vs the 3.2 40-byte grain
                 (a harness re-enactment: the old engine code is gone)

    Conventions as ui_gates_31.h: every row prints "meas ... | thr ... | ctrl
    ..."; an `n` row runs its parent's predicate with the guard removed and
    PASSES only when that predicate FAILS ("ctrl FIRED"). Each row asserts its
    stimulus before its result. Clocks are scripted (the sample position, or
    explicit values); the only wall-clock waits are worker joins and the real
    2000 ms save timeout (PD19).

    Sandbox: every file lives under one temp dir (deleted at exit) or under the
    harness Recordings / Imports temp trees (make(), PD24).

  ==============================================================================
*/
#pragma once

namespace { namespace ug32
{
    using eg21::Session;
    using eg21::Proc;
    using eg21::P;
    using eg21::with;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::noiseBuf;
    using eg21::kSeed;
    using eg21::setP;
    using eg21::rawOf;
    using eg21::sameF;
    using eg22::sn;
    using eg22::loadNoise4;
    using eg24b::want;
    using eg24b::section;
    using eg25a::baseMod;
    using eg26a::ScSig;
    using eg26a::inject;
    using eg26a::rec;
    using eg26a::pr;
    using eg26a::recBase;
    using eg26a::armAt;
    using eg26a::stName;
    using eg26b::writeWav;
    using eg26b::settle;
    using eg26b::recordTake;
    using eg26b::stBase;
    using eg26b::recFileOf;
    using ug31::savePreset;
    using ug31::loadPreset;
    using ug31::keysOf;
    using ug31::isNumber;
    using ug31::factorySlot;
    using RS = ogi::RecState;
    using RR = ogi::RecReason;
    using CS = juce::AudioChannelSet;
    using SlotInfo = ogi::SlotInfo;
    using Tel = Proc::UiTelemetry;
    using Snap = ogi::GiTapeSnapshot;

    constexpr double kFs = 48000.0;

    juce::File gTmp32;

    //==========================================================================
    // Helpers

    /** The scripted clock: the session's sample position in ms (+ 1000). */
    double nowOf (Session& s) { return 1000.0 + 1000.0 * (double) s.pos / s.eng().getSampleRate(); }

    P fourStreams (float density, float sizeMs)
    {
        P p = baseMod (4);
        for (int n = 1; n <= 4; ++n)
            p = with (p, P { { sn (n, "density"), density }, { sn (n, "size"), sizeMs },
                             { sn (n, "position"), 15.0f * (float) n }, { sn (n, "spray"), 20.0f } });
        return p;
    }

    /** An independent census of the pool at the end of a block. */
    struct Census { std::array<int, 4> live {}; int killing = 0, active = 0; };

    Census census (Session& s)
    {
        Census c;
        const auto& pool = s.eng().getPool();
        const auto* base = pool.getGrains().data();
        for (int i = 0; i < ogi::GiPool::kNumSlots; ++i)
        {
            if (! pool.isActive (i))
                continue;
            ++c.active;
            if (i >= ogi::kPoolLive || s.eng().getSpawner().isKilling (base[i], i))
                ++c.killing;
            else if (base[i].slot >= 0 && base[i].slot < 4)
                ++c.live[(size_t) base[i].slot];
        }
        return c;
    }

    /** Level 0 of the slot's current tape: L then R. Empty without a tape. */
    std::vector<float> tapeCopy (Session& s, int slot)
    {
        std::vector<float> out;
        if (const ogi::SlotTape* tp = s.eng().getCurrentTape (slot))
        {
            out.reserve (2 * (size_t) tp->frames);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < tp->frames; ++i)
                    out.push_back (tp->tape.levelSample (0, ch, i));
        }
        return out;
    }

    int bitDiffs (const std::vector<float>& a, const std::vector<float>& b)
    {
        if (a.size() != b.size())
            return -1;
        int n = 0;
        for (size_t i = 0; i < a.size(); ++i)
            n += sameF (a[i], b[i]) ? 0 : 1;
        return n;
    }

    std::shared_ptr<ogi::SlotLoader::Pristine> makeTake (int frames, float amp, int salt)
    {
        auto t = std::make_shared<ogi::SlotLoader::Pristine>();
        t->pcm = noiseBuf (2, frames, amp, salt);
        t->rate = kFs;
        t->name = "take";
        return t;
    }

    /** A take through the loader's commit body (source rec:<path> in the harness Recordings dir). */
    bool commitTake (Session& s, int slot, int frames, float amp, int salt)
    {
        juce::String e;
        const bool ok = s.p->getLoader().commitSync (slot, makeTake (frames, amp, salt), e);
        s.run (2048, 256);
        return ok && s.p->getSlotInfo (slot).source.startsWith ("rec:") && s.eng().getCurrentTape (slot) != nullptr;
    }

    struct WavBack { bool ok = false; int bits = 0, channels = 0, frames = 0; double rate = 0.0; std::vector<float> pcm; };

    WavBack readWav (const juce::File& f)
    {
        WavBack w;
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f));
        if (rd == nullptr)
            return w;
        w.bits = (int) rd->bitsPerSample;
        w.channels = (int) rd->numChannels;
        w.rate = rd->sampleRate;
        w.frames = (int) rd->lengthInSamples;
        juce::AudioBuffer<float> b (juce::jmax (1, w.channels), juce::jmax (1, w.frames));
        w.ok = rd->read (&b, 0, w.frames, 0, true, true);
        for (int ch = 0; ch < w.channels; ++ch)
            w.pcm.insert (w.pcm.end(), b.getReadPointer (ch), b.getReadPointer (ch) + w.frames);
        return w;
    }

    juce::File copyTo (const juce::File& src, const juce::String& name)
    {
        const auto dst = gTmp32.getChildFile ("drops").getChildFile (name);
        dst.getParentDirectory().createDirectory();
        dst.deleteFile();
        return src.copyFileTo (dst) ? dst : juce::File();
    }

    std::uint64_t fileHash (const juce::File& f)
    {
        juce::MemoryBlock mb;
        f.loadFileAsData (mb);
        return ogi::gihash::xxh64 (mb.getData(), mb.getSize());
    }

    int countFiles (const juce::File& dir, const juce::String& pattern = "*")
    {
        int n = 0;
        if (dir.isDirectory())
            for (const auto& e : juce::RangedDirectoryIterator (dir, false, pattern, juce::File::findFiles))
            {
                juce::ignoreUnused (e);
                ++n;
            }
        return n;
    }

    bool nonAscii (const juce::String& s)
    {
        for (auto c = s.getCharPointer(); ! c.isEmpty(); ++c)
            if (*c > 127)
                return true;
        return false;
    }

    juce::String yn (bool b) { return b ? "yes" : "NO"; }

    //==========================================================================
    // UGD1 / UGD1n — every dot against the pool and the spawn log
    struct Gd1Streams { int truth1 = 0, shown1 = 0, wrong = 0; };

    /** emitted[i] = the stream number on the wire, truth0[i] = the grain's tape slot. */
    Gd1Streams gd1Streams (const std::vector<int>& emitted, const std::vector<int>& truth0)
    {
        Gd1Streams g;
        for (size_t i = 0; i < emitted.size() && i < truth0.size(); ++i)
        {
            if (truth0[i] == 0)
            {
                ++g.truth1;
                g.shown1 += emitted[i] == 1 ? 1 : 0;
            }
            g.wrong += emitted[i] == truth0[i] + 1 ? 0 : 1;
        }
        return g;
    }

    bool gd1Predicate (const Gd1Streams& g) { return g.truth1 > 0 && g.shown1 == g.truth1 && g.wrong == 0; }

    void rowsGD1()
    {
        Session s (kFs, with (fourStreams (40.0f, 200.0f), P { { sn (1, "reverse_prob"), 30.0f } }), kSeed, 512);
        loadNoise4 (s, 4.0, 0.2f);
        for (int n : { 48, 60 })
            s.noteOn (64, n);
        s.record = false;
        s.run (72000, 512);

        const auto t = s.p->buildUiTelemetry (nowOf (s));
        const auto& f = s.eng().getTelemetry().frames.read();   // the SAME frame (no publish since)
        const auto& pool = s.eng().getPool();
        const auto* base = pool.getGrains().data();

        std::map<int, ogi::GrainLogEntry> bySlot;   // newest spawn per pool slot
        for (const auto& e : s.hk().logInOrder())
            bySlot[e.poolSlot] = e;

        int n = 0, bad = 0, checked = 0, logBad = 0, reversed = 0;
        std::array<int, 4> perStream {};
        double worst = 0.0;
        std::vector<int> emitted, truth0, rawStream;
        for (int i = 0; i < ogi::GiPool::kNumSlots && n < t.grainCount; ++i)
        {
            if (! pool.isActive (i))
                continue;
            const auto& vg = base[i];
            const auto& dot = t.grains[(size_t) n];
            rawStream.push_back ((int) f.grains[n].stream);
            ++n;
            emitted.push_back (dot.stream);
            truth0.push_back (vg.slot);
            const double frames = vg.tape != nullptr ? (double) vg.tape->frames : 0.0;
            const bool same = frames > 0.0 && sameF (dot.normPos, (float) (vg.g.readAbs / frames)) && dot.stream == vg.slot + 1
                           && dot.voice == vg.voice && dot.flags == (vg.g.dir < 0 ? 1 : 0);
            bad += same ? 0 : 1;
            reversed += dot.flags;
            if (dot.stream >= 1 && dot.stream <= 4)
                ++perStream[(size_t) (dot.stream - 1)];

            const auto it = bySlot.find (i);
            if (i >= ogi::kPoolLive || it == bySlot.end() || it->second.serial != vg.serial || vg.wrapW != 0.0 || frames <= 0.0)
                continue;
            const auto& e = it->second;
            const double want = (e.startPos + (double) e.dir * e.rate * ((double) vg.g.age + e.frac)) / frames;
            const double err = std::abs ((double) dot.normPos - want);
            worst = std::max (worst, err);
            ++checked;
            logBad += (err <= 1.0e-6 && dot.stream == e.stream + 1) ? 0 : 1;
        }
        const auto c = census (s);
        const auto g = gd1Streams (emitted, truth0);
        const bool stim = t.grainCount >= 20 && ! t.stale && perStream[0] > 0 && perStream[1] > 0 && perStream[2] > 0 && perStream[3] > 0
                       && reversed > 0;
        row (stim && n == t.grainCount && n == c.active && (int) f.count == n && bad == 0 && checked >= 20 && logBad == 0 && gd1Predicate (g),
             "UGD1 PD1 every dot == its pool grain (normPos bitwise, stream 1-based, voice, reversed) and == the spawn log",
             "meas " + juce::String (t.grainCount) + " dots / " + juce::String (c.active) + " active pool slots, " + juce::String (n - bad) + "/"
                 + juce::String (n) + " bitwise, " + juce::String (checked) + " checked vs the log (worst |normPos - (p0 + dir * rate * (age + frac)) / frames| "
                 + juce::String (worst, 9) + "), per stream " + juce::String (perStream[0]) + "/" + juce::String (perStream[1]) + "/"
                 + juce::String (perStream[2]) + "/" + juce::String (perStream[3]) + ", reversed " + juce::String (reversed) + ", stream-1 dots "
                 + juce::String (g.shown1) + "/" + juce::String (g.truth1) + " | thr all bitwise, log <= 1e-6, >= 20 checked | ctrl UGD1n");

        // UGD1n: the frame's own (0-based) stream sent as is.
        const auto gn = gd1Streams (rawStream, truth0);
        const bool fired = stim && ! gd1Predicate (gn) && gn.truth1 > 0 && gn.shown1 == 0;
        row (fired, "UGD1n control: the frame's stream left 0-based -> no stream-1 dot (predicate must FAIL)",
             "meas stream-1 grains " + juce::String (gn.truth1) + ", sent as stream 1: " + juce::String (gn.shown1) + ", mislabelled "
                 + juce::String (gn.wrong) + "/" + juce::String ((int) rawStream.size())
                 + " | thr predicate 'every stream-1 grain is sent as 1' FAILS | "
                 + ctrl (fired, "UGD1n 0-based copy -> predicate failed, " + juce::String (gn.shown1) + " of " + juce::String (gn.truth1) + " stream-1 dots"));
    }

    //==========================================================================
    // UGD2 — the reader-side derivations
    void rowGD2()
    {
        Tally t;
        juce::String det;

        // (a) spread 0 / gain_rand 0 / pan 0 / Hann.
        {
            Session s (kFs, fourStreams (40.0f, 200.0f), kSeed, 512);
            loadNoise4 (s, 4.0, 0.2f);
            for (int n : { 48, 60 })
                s.noteOn (64, n);
            s.record = false;
            s.run (60000, 512);
            const auto tel = s.p->buildUiTelemetry (nowOf (s));
            const auto& f = s.eng().getTelemetry().frames.read();
            int panBad = 0, gainBad = 0, envBad = 0, lenBad = 0;
            double envWorst = 0.0;
            for (int i = 0; i < tel.grainCount; ++i)
            {
                const auto& dot = tel.grains[(size_t) i];
                const auto& u = f.grains[i];
                panBad  += dot.pan == 0.0f ? 0 : 1;
                gainBad += dot.gain == 1.0f ? 0 : 1;
                const double hann = (u.phase >= 0.0f && u.phase < 1.0f) ? 0.5 - 0.5 * std::cos (2.0 * eg21::kPi * (double) u.phase) : 0.0;
                const double err = std::abs ((double) dot.env - hann * (double) u.gain);
                envWorst = std::max (envWorst, err);
                envBad += err <= 1.0e-6 ? 0 : 1;
                lenBad += (sameF (dot.lengthSec, (float) ((double) u.lenSamples / kFs)) && std::abs (dot.lengthSec - 0.2f) <= 1.0e-5f) ? 0 : 1;
            }
            t.check (tel.grainCount >= 20 && panBad + gainBad + envBad + lenBad == 0,
                     "a: n " + juce::String (tel.grainCount) + " pan " + juce::String (panBad) + " gain " + juce::String (gainBad) + " env "
                         + juce::String (envBad) + " len " + juce::String (lenBad));
            det << "a: " << tel.grainCount << " dots pan 0 / gain 1 exact, Hann env worst " << juce::String (envWorst, 9) << ", length 0.2 s; ";
        }

        // (b) spread 100 / gain_rand 50 / window 0.6 / skew 30.
        {
            P ps = fourStreams (40.0f, 200.0f);
            for (int n = 1; n <= 4; ++n)
                ps = with (ps, P { { sn (n, "stereo_spread"), 100.0f }, { sn (n, "gain_rand"), 50.0f },
                                   { sn (n, "window"), 0.6f }, { sn (n, "window_skew"), 30.0f } });
            Session s (kFs, ps, kSeed, 512);
            loadNoise4 (s, 4.0, 0.2f);
            for (int n : { 48, 60 })
                s.noteOn (64, n);
            s.record = false;
            s.run (60000, 512);
            const auto tel = s.p->buildUiTelemetry (nowOf (s));
            const auto& f = s.eng().getTelemetry().frames.read();

            // The bank curve from the PARAMETERS, sampled finely (independent of grainWindow).
            constexpr int kN = 8193;
            std::vector<float> curve ((size_t) kN);
            s.eng().windowCurve (rawOf (*s.p, sn (1, "window")), rawOf (*s.p, sn (1, "window_skew")), curve.data(), kN);

            int bad = 0, spreadSeen = 0, cutSeen = 0;
            double invWorst = 0.0, envWorst = 0.0;
            for (int i = 0; i < tel.grainCount; ++i)
            {
                const auto& dot = tel.grains[(size_t) i];
                const auto& u = f.grains[i];
                bad += (dot.pan >= -1.0f && dot.pan <= 1.0f && dot.gain > 0.0f && dot.gain <= 1.0f + 1.0e-6f) ? 0 : 1;
                spreadSeen += std::abs (dot.pan) > 0.2f ? 1 : 0;
                cutSeen += dot.gain < 0.9f ? 1 : 0;
                const auto pg = ogi::panGains (dot.pan);
                invWorst = std::max (invWorst, (double) std::max (std::abs (pg.a * dot.gain - u.gainL), std::abs (pg.b * dot.gain - u.gainR)));
                if (! (u.phase >= 0.0f && u.phase < 1.0f))
                    continue;
                const double x = (double) u.phase * (double) (kN - 1);
                const int i0 = juce::jlimit (0, kN - 2, (int) x);
                const double w = (double) curve[(size_t) i0] + (x - (double) i0) * ((double) curve[(size_t) i0 + 1] - (double) curve[(size_t) i0]);
                envWorst = std::max (envWorst, std::abs ((double) dot.env - w * (double) u.gain));
            }
            t.check (tel.grainCount >= 20 && bad == 0 && spreadSeen > 0 && cutSeen > 0 && invWorst <= 1.0e-5 && envWorst <= 1.0e-4,
                     "b: bad " + juce::String (bad) + " spread " + juce::String (spreadSeen) + " cut " + juce::String (cutSeen) + " inv "
                         + juce::String (invWorst, 8) + " env " + juce::String (envWorst, 8));
            det << "b: panGains inverse worst " << juce::String (invWorst, 9) << ", " << spreadSeen << " spread / " << cutSeen
                << " cut dots, bank env (window 0.6, skew 30) worst " << juce::String (envWorst, 9) << "; ";
        }

        // (c) the stream pan is added once: -100 / +100 / +50 / 0 with spread 0.
        {
            Session s (kFs, with (fourStreams (40.0f, 200.0f), P { { sn (1, "pan"), -100.0f }, { sn (2, "pan"), 100.0f }, { sn (3, "pan"), 50.0f } }),
                       kSeed, 512);
            loadNoise4 (s, 4.0, 0.2f);
            for (int n : { 48, 60 })
                s.noteOn (64, n);
            s.record = false;
            s.run (60000, 512);
            const auto tel = s.p->buildUiTelemetry (nowOf (s));
            const float want[4] = { -1.0f, 1.0f, 0.5f, 0.0f };
            std::array<int, 4> seen {}, okN {};
            for (int i = 0; i < tel.grainCount; ++i)
            {
                const auto& dot = tel.grains[(size_t) i];
                if (dot.stream < 1 || dot.stream > 4)
                    continue;
                ++seen[(size_t) (dot.stream - 1)];
                okN[(size_t) (dot.stream - 1)] += dot.pan == want[dot.stream - 1] ? 1 : 0;
            }
            bool ok = true;
            for (size_t k = 0; k < 4; ++k)
                ok = ok && seen[k] > 0 && okN[k] == seen[k];
            t.check (ok, "c: " + juce::String (okN[0]) + "/" + juce::String (seen[0]) + " " + juce::String (okN[1]) + "/" + juce::String (seen[1]) + " "
                             + juce::String (okN[2]) + "/" + juce::String (seen[2]) + " " + juce::String (okN[3]) + "/" + juce::String (seen[3]));
            det << "c: stream pan -100 / +100 / +50 / 0 -> " << okN[0] << "/" << seen[0] << " at -1, " << okN[1] << "/" << seen[1] << " at +1, "
                << okN[2] << "/" << seen[2] << " at 0.5, " << okN[3] << "/" << seen[3] << " at 0";
        }

        row (t.bad == 0, "UGD2 PD1 pan / gain / env / length derivations; the stream pan added once",
             "meas " + (t.bad ? t.detail (3) : det) + " | thr exact pan / gain, env <= 1e-6 (Hann) / 1e-4 (bank), inverse <= 1e-5 | ctrl - (b differs from a)");
    }

    //==========================================================================
    // UCN1 / UCN1n — counts against the pool census, killing grains present
    void rowsCN1()
    {
        struct Scan { int frames = 0, mismatch = 0, mismatchAll = 0, killingMax = 0, killingFrames = 0, maxSum = 0, excessMax = 0, limitBad = 0, countBad = 0; };
        Scan sc;
        Session s (kFs, with (fourStreams (60.0f, 400.0f), P { { "grain_limit", 3.0f } }), kSeed, 128);
        loadNoise4 (s, 4.0, 0.2f);
        for (int n : { 40, 43, 47, 50, 52, 55, 59, 62 })
            s.noteOn (64, n);
        // The killing stimulus: a new source lands in a sounding slot (the old
        // tape's grains get the 5 ms forced release), twice; then the limit
        // drops 256 -> 32.
        s.at (60000, [] (Session& ss) { ss.load (0, noiseBuf (2, 96000, 0.2f, 205), kFs); });
        s.at (78000, [] (Session& ss) { ss.load (2, noiseBuf (2, 96000, 0.2f, 206), kFs); });
        s.at (96000, [] (Session& ss) { setP (*ss.p, "grain_limit", 0.0f); });
        s.record = false;
        s.afterBlock = [&sc] (Session& ss)
        {
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            const auto& f = ss.eng().getTelemetry().frames.read();
            const auto c = census (ss);
            std::array<int, 4> all {};   // the control: every frame grain counted, killing / fade included
            int killing = 0;
            for (int k = 0; k < (int) f.count; ++k)
            {
                if (f.grains[k].stream < 4)
                    ++all[(size_t) f.grains[k].stream];
                killing += (f.grains[k].flags & 2u) != 0 ? 1 : 0;
            }
            int sum = 0, sumAll = 0;
            bool same = true, sameAll = true;
            for (size_t k = 0; k < 4; ++k)
            {
                same = same && t.counts[k] == c.live[k];
                sameAll = sameAll && all[k] == c.live[k];
                sum += t.counts[k];
                sumAll += all[k];
            }
            ++sc.frames;
            sc.mismatch += same ? 0 : 1;
            sc.mismatchAll += sameAll ? 0 : 1;
            sc.killingMax = std::max (sc.killingMax, killing);
            sc.killingFrames += killing > 0 ? 1 : 0;
            sc.maxSum = std::max (sc.maxSum, sum);
            sc.excessMax = std::max (sc.excessMax, sumAll - sum);
            sc.limitBad += t.limit == 0 ? 0 : 1;
            sc.countBad += (killing == c.killing && t.grainCount == c.active) ? 0 : 1;
        };
        s.run (120000, 128);

        const auto swapKills = s.hk().swapKills;
        const bool stim = sc.frames > 500 && sc.killingMax > 0 && sc.maxSum > 32 && swapKills > 0 && s.hk().liveNow <= 32;
        row (stim && sc.mismatch == 0 && sc.limitBad == 0 && sc.countBad == 0,
             "UCN1 PD3 counts[s] == an independent pool census every frame (two tape swaps kill grains, grain limit 256 -> 32); limit 0",
             "meas " + juce::String (sc.frames - sc.mismatch) + "/" + juce::String (sc.frames) + " frames equal, max sum " + juce::String (sc.maxSum)
                 + " (ends at " + juce::String (s.hk().liveNow) + "), swap kills " + i64 (swapKills) + ", frames holding killing / fade grains "
                 + juce::String (sc.killingFrames) + " (max " + juce::String (sc.killingMax)
                 + " in one frame), dots == active slots " + juce::String (sc.frames - sc.countBad) + "/" + juce::String (sc.frames)
                 + ", limit 0 on every frame " + yn (sc.limitBad == 0) + " | thr 0 mismatches, killing present | ctrl UCN1n");
        const bool fired = stim && sc.mismatchAll > 0 && sc.mismatchAll == sc.killingFrames && sc.excessMax == sc.killingMax;
        row (fired, "UCN1n control: killing / fade grains counted -> counts exceed the census (predicate must FAIL)",
             "meas " + juce::String (sc.mismatchAll) + "/" + juce::String (sc.frames) + " frames differ (every frame that holds a killing grain), sum over the census by up to "
                 + juce::String (sc.excessMax) + " | thr predicate 'counts == census on every frame' FAILS | "
                 + ctrl (fired, "UCN1n flags bit 2 ignored -> predicate failed on " + juce::String (sc.mismatchAll) + " frames"));
    }

    //==========================================================================
    // UPH1 / UPH1n — playheads
    void rowsPH1()
    {
        struct Scan
        {
            int frames = 0, held = 0, bad = 0, freeBad = 0, rawZero = 0, rawSeen = 0;
            float lo0 = 2.0f, hi0 = -1.0f, lo2 = 2.0f, hi2 = -1.0f;
            float sample1 = 9.0f, sample3 = 9.0f;
        };
        Scan sc;
        Session s (kFs, with (fourStreams (20.0f, 100.0f), P { { sn (1, "loop_start"), 20.0f }, { sn (1, "loop_end"), 60.0f },
                                                               { sn (1, "scan_rate"), 50.0f }, { sn (3, "scan_rate"), 50.0f } }), kSeed, 512);
        s.load (0, noiseBuf (2, 96000, 0.2f, 211), kFs);   // slots 1 and 3 stay empty
        s.load (2, noiseBuf (2, 96000, 0.2f, 212), kFs);
        s.noteOn (4800, 60);
        s.record = false;
        s.afterBlock = [&sc] (Session& ss)
        {
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            const auto& f = ss.eng().getTelemetry().frames.read();
            const ogi::SlotTape* tp = ss.eng().getCurrentTape (0);
            const double frames = tp != nullptr ? (double) tp->frames : 0.0;
            ++sc.frames;
            int held = 0;
            for (int k = 0; k < Tel::kVoices; ++k)
            {
                const auto& v = t.voices[(size_t) k];
                if (v.state == 0)
                {
                    sc.freeBad += (v.note == -1 && v.scan[0] == -1.0f && v.scan[1] == -1.0f && v.scan[2] == -1.0f && v.scan[3] == -1.0f
                                   && f.voices[k].note < 0) ? 0 : 1;
                    continue;
                }
                ++held;
                const double lo = frames > 0.0 ? ss.hk().regionLoS[0] / frames : 0.0, hi = frames > 0.0 ? ss.hk().regionHiS[0] / frames : 0.0;
                const bool in0 = (double) v.scan[0] >= lo - 1.0e-6 && (double) v.scan[0] <= hi + 1.0e-6;
                const bool in2 = v.scan[2] >= 0.0f && v.scan[2] <= 1.0f;
                sc.bad += (v.state == 1 && v.note == 60 && in0 && in2 && v.scan[1] == -1.0f && v.scan[3] == -1.0f) ? 0 : 1;
                sc.lo0 = std::min (sc.lo0, v.scan[0]);
                sc.hi0 = std::max (sc.hi0, v.scan[0]);
                sc.lo2 = std::min (sc.lo2, v.scan[2]);
                sc.hi2 = std::max (sc.hi2, v.scan[2]);
                sc.sample1 = v.scan[1];
                sc.sample3 = v.scan[3];
                // The control's input: what the frame itself carries for the tape-less streams.
                ++sc.rawSeen;
                sc.rawZero += (f.voices[k].scan[1] == 0.0f && f.voices[k].scan[3] == 0.0f) ? 1 : 0;
            }
            sc.held += held == 1 ? 1 : 0;
        };
        s.run (4800 + 96000, 512);
        const double frames0 = s.eng().getCurrentTape (0) != nullptr ? (double) s.eng().getCurrentTape (0)->frames : 1.0;
        const double lo = s.hk().regionLoS[0] / frames0, hi = s.hk().regionHiS[0] / frames0;
        const bool stim = sc.held > 150 && std::abs (lo - 0.2) < 1.0e-3 && std::abs (hi - 0.6) < 1.0e-3 && sc.hi0 - sc.lo0 > 0.05f;

        row (stim && sc.bad == 0 && sc.freeBad == 0,
             "UPH1 PD2 playheads: a stream with a tape scans inside its loop region; -1 for a stream without a tape",
             "meas " + juce::String (sc.held) + " frames with the note held (1 row state 1 / note 60), stream 1 scan " + juce::String (sc.lo0, 4) + " ... "
                 + juce::String (sc.hi0, 4) + " inside [" + juce::String (lo, 4) + ", " + juce::String (hi, 4) + "], stream 3 scan "
                 + juce::String (sc.lo2, 4) + " ... " + juce::String (sc.hi2, 4) + ", streams 2 / 4 (no tape) " + juce::String (sc.sample1, 1) + " / "
                 + juce::String (sc.sample3, 1) + ", bad rows " + juce::String (sc.bad) + ", bad free rows " + juce::String (sc.freeBad)
                 + " | thr inside the region, -1 exactly, free rows 0 / -1 / -1 | ctrl UPH1n");
        const bool fired = stim && sc.rawSeen > 0 && sc.rawZero == sc.rawSeen;
        row (fired, "UPH1n control: the frame's own scan for a stream without a tape is 0 (predicate must FAIL)",
             "meas frame scan == 0 on " + juce::String (sc.rawZero) + "/" + juce::String (sc.rawSeen)
                 + " held rows (a playhead at the lane's left edge) | thr predicate 'no-tape scan == -1' FAILS | "
                 + ctrl (fired, "UPH1n frame scan sent as is -> predicate failed, 0 emitted on " + juce::String (sc.rawZero) + " rows"));
    }

    //==========================================================================
    // UFB1 / UFB1n — the feedback head
    void rowsFB1()
    {
        Session s (kFs, with (baseMod (1), P { { sn (1, "density"), 30.0f }, { "feedback", 0.0f } }), kSeed, 512);
        s.load (0, noiseBuf (2, 96000, 0.3f, 221), kFs);
        for (int n : { 48, 60 })
            s.noteOn (64, n);
        s.record = false;
        auto& tel = s.eng().getTelemetry();
        const double frames = 96000.0;
        const auto parked = [&tel] { return (tel.swapState[0].load() & 4u) != 0; };

        // 1. feedback 0: the head is parked, fb null.
        s.run (24000, 512);
        const auto t0 = s.p->buildUiTelemetry (nowOf (s));
        const bool nullAtZero = ! t0.lanes[0].hasFb && parked();

        // 2. feedback 80: fb == headPos / frames, moving.
        setP (*s.p, "feedback", 80.0f);
        int polls = 0, shown = 0, equal = 0, otherLanes = 0;
        std::set<float> values;
        s.afterBlock = [&] (Session& ss)
        {
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            ++polls;
            if (t.lanes[0].hasFb)
            {
                ++shown;
                const double want = juce::jlimit (0.0, 1.0, (double) tel.headPos[0].load() / frames);
                equal += sameF (t.lanes[0].fb, (float) want) ? 1 : 0;
                values.insert (t.lanes[0].fb);
            }
            otherLanes += (t.lanes[1].hasFb || t.lanes[2].hasFb || t.lanes[3].hasFb) ? 1 : 0;
        };
        s.run (48000, 512);
        const int shownRun = shown, pollsRun = polls, distinct = (int) values.size();
        const bool tracks = shownRun > pollsRun / 2 && equal == shownRun && distinct > 20 && ! parked();

        // 3. frozen with feedback > 0: non-null and still, though the head is parked.
        s.afterBlock = nullptr;
        setP (*s.p, "freeze", 1.0f);
        s.run (24000, 512);
        std::set<float> frozenValues;
        int frozenShown = 0, frozenParked = 0, ruleAloneShown = 0;
        for (int k = 0; k < 20; ++k)
        {
            s.run (512, 512);
            const auto t = s.p->buildUiTelemetry (nowOf (s));
            frozenShown += t.lanes[0].hasFb ? 1 : 0;
            frozenParked += parked() ? 1 : 0;
            ruleAloneShown += ! parked() ? 1 : 0;   // UFB1n: "has a tape and the head is not parked", nothing else
            if (t.lanes[0].hasFb)
                frozenValues.insert (t.lanes[0].fb);
        }
        const float frozenAt = frozenValues.empty() ? -1.0f : *frozenValues.begin();
        const bool frozenOk = frozenShown == 20 && frozenValues.size() == 1 && frozenParked == 20;

        // 4. frozen with feedback 0: null again.
        setP (*s.p, "feedback", 0.0f);
        s.run (12000, 512);
        const auto t4 = s.p->buildUiTelemetry (nowOf (s));
        const bool nullFrozenZero = ! t4.lanes[0].hasFb;

        row (nullAtZero && tracks && frozenOk && nullFrozenZero && otherLanes == 0,
             "UFB1 PD5 fb: null at feedback 0, == headPos / frames with feedback on, non-null and still when frozen",
             juce::String ("meas feedback 0: ") + (nullAtZero ? "null (head parked)" : "NOT NULL / not parked") + "; feedback 80: shown on "
                 + juce::String (shownRun) + "/" + juce::String (pollsRun) + " polls, == headPos / frames on " + juce::String (equal) + ", "
                 + juce::String (distinct) + " distinct values; frozen: shown " + juce::String (frozenShown) + "/20 at " + juce::String (frozenAt, 4)
                 + " (" + juce::String ((int) frozenValues.size()) + " value, head parked " + juce::String (frozenParked) + "/20); frozen + feedback 0: "
                 + (nullFrozenZero ? "null" : "NOT NULL") + "; lanes 2 - 4 shown " + juce::String (otherLanes)
                 + " | thr null / bitwise / 1 still value / null / 0 | ctrl UFB1n");
        const bool fired = frozenParked == 20 && frozenShown == 20 && ruleAloneShown == 0;
        row (fired, "UFB1n control: the 'head not parked' rule alone -> fb null when frozen (predicate must FAIL)",
             "meas frozen polls where the rule alone shows a head " + juce::String (ruleAloneShown) + "/20 (shipping " + juce::String (frozenShown)
                 + "/20) | thr predicate 'non-null when frozen with feedback > 0' FAILS | "
                 + ctrl (fired, "UFB1n no freeze term -> predicate failed, fb null on 20/20 frozen polls"));
    }

    //==========================================================================
    // UOV2 / UOV2n / UOV2n-b — the live overview, dirty, the two revisions
    struct OvR
    {
        bool setup = false;
        int idleOvMoves = 0, idleSlotMoves = 0;
        int fbOvMoves = 0, fbPolls = 0, slotMovesAtFlip = 0, flagFlips = 0;
        std::uint32_t writes0 = 0, writes1 = 0;
        int liveDiffBins = 0, dirtyPeaks = 0, ticks = 0, dirtyWrong = 0, dirtyMissed = 0, peakBad = 0, maxByte = 0;
        bool flag = false, jsonDirty = false, revMatches = false;
        // after Reset
        int dirtyAfter = -1;
        bool flagAfter = true, slotMovedAfter = false, ovMovedAfter = false, jsonDirtyAfter = true;
    };

    OvR ovRun (int hook)   // 0 shipping, 1 testOvRevNoWrites, 2 testSlotRevNoDirty
    {
        constexpr int kN = Proc::kOverviewPeaks;
        OvR r;
        Session s (kFs, with (baseMod (1), P { { sn (1, "density"), 40.0f }, { sn (1, "size"), 150.0f }, { "feedback", 0.0f } }), kSeed, 512);
        s.p->testOvRevNoWrites  = hook == 1;
        s.p->testSlotRevNoDirty = hook == 2;
        r.setup = s.load (0, noiseBuf (2, 96000, 0.3f, 231), kFs);
        for (int n : { 48, 55, 60 })
            s.noteOn (64, n);
        s.record = false;
        auto& tel = s.eng().getTelemetry();
        auto& ld = s.p->getLoader();

        // A. feedback 0: nothing moves.
        s.run (12000, 512);
        auto last = s.p->buildUiTelemetry (nowOf (s));
        s.afterBlock = [&] (Session& ss)
        {
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            r.idleOvMoves += t.lanes[0].ovRev != last.lanes[0].ovRev ? 1 : 0;
            r.idleSlotMoves += t.slotRev != last.slotRev ? 1 : 0;
            last = t;
        };
        s.run (24000, 512);
        r.setup = r.setup && ! s.p->getSlotDirty (0);

        // B. feedback 90: the head writes.
        r.writes0 = tel.getOvWrites (0);
        setP (*s.p, "feedback", 90.0f);
        bool flag = false;
        s.afterBlock = [&] (Session& ss)
        {
            const auto t = ss.p->buildUiTelemetry (nowOf (ss));
            ++r.fbPolls;
            r.fbOvMoves += t.lanes[0].ovRev != last.lanes[0].ovRev ? 1 : 0;
            const bool now = ss.p->getSlotDirty (0);
            if (now != flag)
            {
                ++r.flagFlips;
                r.slotMovesAtFlip += t.slotRev != last.slotRev ? 1 : 0;
                flag = now;
            }
            last = t;
        };
        s.run (72000, 512);
        s.afterBlock = nullptr;
        r.writes1 = tel.getOvWrites (0);
        r.flag = s.p->getSlotDirty (0);

        // The overview against an independent fold of live-over-pristine.
        const auto pairs = ld.getOverview (0);
        const auto bins = (std::int64_t) (pairs.size() / 2);
        std::array<float, (size_t) kN> pk;
        std::array<std::uint8_t, (size_t) kN> dirty;
        pk.fill (9.0f);
        dirty.fill (9);
        std::uint32_t rev = 0;
        const bool ok = s.p->getSlotOverview (0, pk.data(), dirty.data(), kN, rev);
        r.revMatches = ok && rev == s.p->getOverviewRev (0);
        for (std::int64_t b = 0; b < bins; ++b)
        {
            float lo = 0.0f, hi = 0.0f;
            if (tel.overview.read (0, (int) b, lo, hi))
                r.liveDiffBins += (! sameF (lo, pairs[(size_t) (2 * b)]) || ! sameF (hi, pairs[(size_t) (2 * b + 1)])) ? 1 : 0;
        }
        for (int i = 0; ok && i < kN; ++i)
        {
            const std::int64_t b0 = (std::int64_t) i * bins / kN;
            const std::int64_t b1 = std::max (b0 + 1, std::min (bins, (std::int64_t) (i + 1) * bins / kN));
            float want = 0.0f, moved = 0.0f;
            bool differs = false;
            for (std::int64_t b = b0; b < b1; ++b)
            {
                const float pLo = pairs[(size_t) (2 * b)], pHi = pairs[(size_t) (2 * b + 1)];
                float lo = pLo, hi = pHi;
                if (tel.overview.read (0, (int) b, lo, hi))
                {
                    moved = std::max (moved, std::abs (lo - pLo) + std::abs (hi - pHi));
                    differs = differs || ! sameF (lo, pLo) || ! sameF (hi, pHi);
                }
                want = std::max (want, std::max (std::abs (lo), std::abs (hi)));
            }
            const int wantByte = (int) std::min (255.0f, std::round (255.0f * moved / Proc::kDirtyFullScale));
            r.peakBad += sameF (pk[(size_t) i], std::min (1.0f, want)) ? 0 : 1;
            r.dirtyPeaks += dirty[(size_t) i] > 0 ? 1 : 0;
            r.ticks += dirty[(size_t) i] > 12 ? 1 : 0;   // the page's 5 % tick threshold (byte 13)
            r.dirtyWrong += (dirty[(size_t) i] > 0 && ! differs) ? 1 : 0;
            r.dirtyMissed += (int) dirty[(size_t) i] != wantByte ? 1 : 0;
            r.maxByte = std::max (r.maxByte, (int) dirty[(size_t) i]);
        }
        r.jsonDirty = static_cast<bool> (GiUiJson::slotStateToVar (*s.p)["slots"][0]["dirty"]);

        // C. feedback 0 (the head parks), then Reset: clean again.
        setP (*s.p, "feedback", 0.0f);
        s.run (24000, 512);
        const auto before = s.p->buildUiTelemetry (nowOf (s));
        s.p->resetSlotSync (0);
        s.run (9600, 64);
        const auto after = s.p->buildUiTelemetry (nowOf (s));
        std::array<std::uint8_t, (size_t) kN> d2;
        d2.fill (9);
        std::uint32_t rev2 = 0;
        if (s.p->getSlotOverview (0, pk.data(), d2.data(), kN, rev2))
        {
            r.dirtyAfter = 0;
            for (const auto d : d2)
                r.dirtyAfter += d > 0 ? 1 : 0;
        }
        r.flagAfter = s.p->getSlotDirty (0);
        r.slotMovedAfter = after.slotRev != before.slotRev;
        r.ovMovedAfter = after.lanes[0].ovRev != before.lanes[0].ovRev;
        r.jsonDirtyAfter = static_cast<bool> (GiUiJson::slotStateToVar (*s.p)["slots"][0]["dirty"]);
        return r;
    }

    void rowsOV2()
    {
        const auto r = ovRun (0);
        const bool stim = r.setup && r.writes1 != r.writes0 && r.liveDiffBins > 0 && r.ticks > 0;
        row (stim && r.idleOvMoves == 0 && r.idleSlotMoves == 0 && r.fbOvMoves > 10 && r.revMatches && r.peakBad == 0 && r.dirtyPeaks > 0
                 && r.dirtyWrong == 0 && r.dirtyMissed == 0 && r.flag && r.jsonDirty && r.flagFlips == 1 && r.slotMovesAtFlip == 1
                 && r.dirtyAfter == 0 && ! r.flagAfter && ! r.jsonDirtyAfter && r.slotMovedAfter && r.ovMovedAfter,
             "UOV2 PD6 / PD7 live overview: ovRev moves while the head writes, dirty only where a bin differs, flag + slotRev flip; Reset clears",
             "meas feedback 0: ovRev moves " + juce::String (r.idleOvMoves) + ", slotRev moves " + juce::String (r.idleSlotMoves) + "; feedback 90: head folds "
                 + juce::String ((int) (r.writes1 - r.writes0)) + ", ovRev moved on " + juce::String (r.fbOvMoves) + "/" + juce::String (r.fbPolls)
                 + " polls, " + juce::String (r.liveDiffBins) + " live bins differ from the pristine, " + juce::String (r.dirtyPeaks) + "/656 peaks dirty ("
                 + juce::String (r.ticks) + " above the page's tick threshold, max byte " + juce::String (r.maxByte) + "), dirty without a differing bin "
                 + juce::String (r.dirtyWrong) + ", bytes off the PD7 formula " + juce::String (r.dirtyMissed) + ", peaks off the live fold "
                 + juce::String (r.peakBad) + ", flag " + yn (r.flag) + " (JSON " + yn (r.jsonDirty) + "), slotRev moved at the flip "
                 + juce::String (r.slotMovesAtFlip) + "/" + juce::String (r.flagFlips) + "; after Reset: dirty peaks " + juce::String (r.dirtyAfter)
                 + ", flag " + (r.flagAfter ? "STILL SET" : "clear") + ", slotRev moved " + yn (r.slotMovedAfter) + ", ovRev moved " + yn (r.ovMovedAfter)
                 + " | thr 0 / 0 idle, moves > 10, 0 wrong / off, 1/1, clean after Reset | ctrl UOV2n, UOV2n-b");

        const auto n = ovRun (1);
        const bool firedN = n.setup && n.writes1 != n.writes0 && n.liveDiffBins > 0 && n.fbOvMoves == 0;
        row (firedN, "UOV2n control: testOvRevNoWrites -> ovRev static while the head writes (predicate must FAIL)",
             "meas head folds " + juce::String ((int) (n.writes1 - n.writes0)) + ", live bins differing " + juce::String (n.liveDiffBins) + ", ovRev moved on "
                 + juce::String (n.fbOvMoves) + "/" + juce::String (n.fbPolls) + " polls (shipping " + juce::String (r.fbOvMoves)
                 + ") | thr predicate 'ovRev moves while the head writes' FAILS | "
                 + ctrl (firedN, "UOV2n no write counter in ovRev -> predicate failed, 0 moves over " + juce::String (n.fbPolls) + " polls"));

        const auto nb = ovRun (2);
        const bool firedB = nb.setup && nb.flag && nb.flagFlips == 1 && nb.slotMovesAtFlip == 0;
        row (firedB, "UOV2n-b control: testSlotRevNoDirty -> slotRev static when the dirty flag flips (predicate must FAIL)",
             "meas dirty flag flipped " + juce::String (nb.flagFlips) + "x (now " + yn (nb.flag) + "), slotRev moved at the flip "
                 + juce::String (nb.slotMovesAtFlip) + " (shipping " + juce::String (r.slotMovesAtFlip)
                 + ") | thr predicate 'slotRev moves when the flag flips' FAILS | "
                 + ctrl (firedB, "UOV2n-b flag not in the fingerprint -> predicate failed, the page would never pull the new flag"));
    }

    //==========================================================================
    // URC1 / URC1n — the Record-in run value, progress and the 4 s hold
    struct RcTrace
    {
        std::vector<int> runs;          // run values, consecutive duplicates removed
        bool posMonotonic = true, laneOk = true, slotOk = true, lenOk = true, signalEver = false;
        float posFirst = -1.0f, posLast = -1.0f;
        double holdStart = -1.0, holdLastSeen = -1.0, zeroAfter = -1.0;   // scripted ms
        int heldValue = 0, finalRun = -1, polls = 0, recLanePolls = 0;
        bool finalSignal = true;
        RS endState = RS::Idle;
        RR endReason = RR::None;
    };

    juce::String runsText (const std::vector<int>& v)
    {
        juce::String t;
        for (const int x : v)
            t << (t.isEmpty() ? "" : " -> ") << x;
        return t;
    }

    /** silent = a Manual take on a silent sidechain (NoSignal); else a Threshold
        take into slot 2. frozenClock = every build after the hold starts gets
        the SAME nowMs (the hold can never expire). */
    RcTrace rcTrace (bool silent, bool frozenClock)
    {
        RcTrace tr;
        const int slot = 1;
        Session s (kFs, recBase (slot, silent ? 0 : 1, { { "rec_length", 1.0f } }), kSeed, 512);
        ScSig g { silent ? 0.0f : 0.5f, 241 };
        if (! silent)
            g.from = 12000;
        inject (s, g);
        armAt (s, 4800);
        s.record = false;
        float lastPos = -1.0f;
        double frozenAt = -1.0;
        s.afterBlock = [&] (Session& ss)
        {
            const double now = (frozenClock && frozenAt >= 0.0) ? frozenAt : nowOf (ss);
            const auto t = ss.p->buildUiTelemetry (now);
            ++tr.polls;
            if (tr.runs.empty() || tr.runs.back() != t.rec.run)
                tr.runs.push_back (t.rec.run);
            tr.signalEver = tr.signalEver || t.rec.signal;
            int lanes = 0;
            for (int k = 0; k < 4; ++k)
                lanes += t.lanes[(size_t) k].hasRec ? 1 : 0;
            if (t.rec.run == 2)
            {
                ++tr.recLanePolls;
                if (tr.posFirst < 0.0f)
                    tr.posFirst = t.rec.pos;
                tr.posMonotonic = tr.posMonotonic && t.rec.pos >= lastPos && t.rec.pos >= 0.0f && t.rec.pos <= 1.0f;
                lastPos = t.rec.pos;
                tr.posLast = t.rec.pos;
                tr.laneOk = tr.laneOk && lanes == 1 && t.lanes[(size_t) slot].hasRec && sameF (t.lanes[(size_t) slot].rec, t.rec.pos);
                tr.slotOk = tr.slotOk && t.rec.slot == slot + 1;
                tr.lenOk = tr.lenOk && sameF (t.rec.len, 1.0f);
            }
            else
            {
                tr.laneOk = tr.laneOk && lanes == 0;
            }
            if (t.rec.run == 4 || t.rec.run == 5)
            {
                if (tr.holdStart < 0.0)
                {
                    tr.holdStart = nowOf (ss);
                    tr.heldValue = t.rec.run;
                    frozenAt = now;
                }
                tr.holdLastSeen = nowOf (ss);
            }
            else if (tr.holdStart >= 0.0 && tr.zeroAfter < 0.0 && t.rec.run == 0)
            {
                tr.zeroAfter = nowOf (ss);
            }
            tr.finalRun = t.rec.run;
            tr.finalSignal = t.rec.signal;
        };
        s.run (4800 + 7200 + 48000 + 6000, 512);     // armed, recording, the take ends
        tr.endState = pr (s).state;
        tr.endReason = pr (s).reason;
        s.p->commitSync();                            // Committing -> Idle + Committed (a no-op after a discard)
        s.run ((juce::int64) (6.0 * kFs), 512);       // 6 s: past the 4 s hold
        return tr;
    }

    bool rcHoldPredicate (const RcTrace& tr)
    {
        // The hold lasts 4000 scripted ms (one 512-frame poll of slack) and then reads 0.
        constexpr double kPollMs = 1000.0 * 512.0 / kFs;
        return tr.holdStart >= 0.0 && tr.zeroAfter >= 0.0 && tr.zeroAfter - tr.holdStart >= GiRecRunHold::kHoldMs
            && tr.zeroAfter - tr.holdStart < GiRecRunHold::kHoldMs + kPollMs + 1.0e-6 && tr.holdLastSeen - tr.holdStart < GiRecRunHold::kHoldMs
            && tr.finalRun == 0;
    }

    void rowsRC1()
    {
        const auto a = rcTrace (false, false);
        const auto b = rcTrace (true, false);
        const bool takeOk = a.runs == std::vector<int> { 0, 1, 2, 3, 4, 0 } && a.heldValue == 4 && rcHoldPredicate (a) && a.posMonotonic
                         && a.posFirst >= 0.0f && a.posFirst < 0.05f && a.posLast > 0.98f && a.laneOk && a.slotOk && a.lenOk && a.recLanePolls > 80
                         && a.endReason == RR::Committed;
        const bool silentOk = b.runs == std::vector<int> { 0, 2, 5, 0 } && b.heldValue == 5 && rcHoldPredicate (b) && b.laneOk
                           && b.endReason == RR::NoSignal && ! b.signalEver && ! b.finalSignal;
        row (takeOk && silentOk, "URC1 PD9 run 0 -> 1 -> 2 -> 3 -> 4 (4000 ms) -> 0; pos 0 -> 1; lanes[target].rec only while recording; silent 0 -> 2 -> 5 -> 0",
             "meas take: " + runsText (a.runs) + ", pos " + juce::String (a.posFirst, 4) + " -> " + juce::String (a.posLast, 4) + " monotonic "
                 + yn (a.posMonotonic) + " over " + juce::String (a.recLanePolls) + " polls, lane 2 only " + yn (a.laneOk) + ", rec.slot 2 " + yn (a.slotOk)
                 + ", len 1.0 " + yn (a.lenOk) + ", 4 held " + juce::String (a.holdLastSeen - a.holdStart, 2) + " ms then 0 at +"
                 + juce::String (a.zeroAfter - a.holdStart, 2) + " ms; silent take: " + runsText (b.runs) + ", 5 then 0 at +"
                 + juce::String (b.zeroAfter - b.holdStart, 2) + " ms, signal " + (b.signalEver ? "SEEN" : "never")
                 + " | thr exact sequences, 0 at 4000 ... 4010.67 scripted ms | ctrl URC1n");

        const auto an = rcTrace (false, true);
        const auto bn = rcTrace (true, true);
        const bool stim = an.heldValue == 4 && bn.heldValue == 5 && an.polls > 500 && bn.polls > 500;
        const bool fired = stim && ! rcHoldPredicate (an) && ! rcHoldPredicate (bn) && an.finalRun == 4 && bn.finalRun == 5 && ! bn.finalSignal;
        row (fired, "URC1n control: the hold never expires (frozen clock) -> 4 / 5 forever, 'no signal' masked (predicate must FAIL)",
             "meas take: " + runsText (an.runs) + " (still " + juce::String (an.finalRun) + " after " + juce::String (an.holdLastSeen - an.holdStart, 0)
                 + " ms of audio); silent take: " + runsText (bn.runs) + " (still " + juce::String (bn.finalRun) + ", signal false: the page shows "
                 + "'Discarded', never 'no signal') | thr predicate 'run reads 0 after 4000 ms' FAILS | "
                 + ctrl (fired, "URC1n no expiry -> predicate failed, run " + juce::String (an.finalRun) + " / " + juce::String (bn.finalRun) + " forever"));
    }

    //==========================================================================
    // URC2 — rec.signal and rec.capped
    void rowRC2()
    {
        // signal: a loud stretch [4800, 9600), silence after; explicit scripted times.
        bool sigOk = false;
        float loudPeak = 0.0f, quietPeak = -1.0f;
        juce::String sigDetail;
        {
            Session s (kFs, recBase (0, 0), kSeed, 512);
            ScSig g { 0.5f, 251 };
            g.from = 4800;
            g.to = 9600;
            inject (s, g);
            s.record = false;
            s.run (4608, 512);
            const auto a = s.p->buildUiTelemetry (10000.0);             // before any signal
            s.run (5120, 512);                                          // the loud stretch
            const auto b = s.p->buildUiTelemetry (20000.0);             // the peak is consumed here
            loudPeak = b.rec.peak;
            s.run (2048, 512);                                          // silence
            const auto c = s.p->buildUiTelemetry (21999.0);
            const auto d = s.p->buildUiTelemetry (22000.0);             // exactly 2000 ms: still inside
            const auto e = s.p->buildUiTelemetry (22000.5);
            s.run (2048, 512);
            const auto f = s.p->buildUiTelemetry (30000.0);
            sigOk = ! a.rec.signal && a.rec.peak == 0.0f && loudPeak >= ogi::GiRecorder::kPresenceLin && b.rec.signal && c.rec.signal
                 && c.rec.peak == 0.0f && d.rec.signal && ! e.rec.signal && ! f.rec.signal;
            sigDetail << "before " << (a.rec.signal ? "TRUE" : "false") << ", at the peak " << (b.rec.signal ? "true" : "FALSE") << " (peak "
                      << juce::String (juce::Decibels::gainToDecibels (loudPeak), 1) << " dBFS, [p, p]), +1999 ms " << (c.rec.signal ? "true" : "FALSE")
                      << ", +2000 ms " << (d.rec.signal ? "true" : "FALSE") << ", +2000.5 ms " << (e.rec.signal ? "TRUE" : "false");
        }
        bool quietOk = false;
        {
            Session s (kFs, recBase (0, 0), kSeed, 512);
            ScSig g { 0.0f, 0 };
            g.dc = juce::Decibels::decibelsToGain (-95.0f);
            inject (s, g);
            s.record = false;
            s.run (9600, 512);
            const auto a = s.p->buildUiTelemetry (10000.0);
            quietPeak = a.rec.peak;
            quietOk = ! a.rec.signal && quietPeak > 0.0f && quietPeak < ogi::GiRecorder::kPresenceLin;
        }

        // capped: Tempo mode across F_max (30 s @ 48 k = 1440000 frames), 4/4.
        struct Cap { double bpm; int barsIdx; bool wantCapped; };
        const Cap caps[] = { { 120.0, 4, true }, { 140.0, 4, false }, { 120.0, 3, false }, { 140.0, 5, true } };
        Tally tc;
        juce::String capDetail;
        for (const auto& c : caps)
        {
            Session s (kFs, recBase (0, 0, { { "sync_mode", 1.0f }, { "rec_length_sync", (float) c.barsIdx }, { "rec_length", 1.0f } }), kSeed, 512);
            auto ph = std::make_shared<MockPlayHead>();
            ph->sampleRate = kFs;
            MockPlayHead::Seg seg;
            seg.at = 0;
            seg.ppq0 = 0.0;
            seg.bpm = c.bpm;
            seg.playing = true;
            ph->script.push_back (seg);
            s.p->setPlayHead (ph.get());
            inject (s, ScSig { 0.5f, 252 }, -1, [ph] (Session&, juce::int64 b0, int) { ph->timeInSamples = b0; });
            s.record = false;
            s.run (2048, 512);
            const auto idle = s.p->buildUiTelemetry (nowOf (s));
            // The real take the recorder starts at this tempo.
            armAt (s, s.pos);
            s.run (4096, 512);
            const auto during = s.p->buildUiTelemetry (nowOf (s));
            const int target = rec (s).getTargetTelemetry();
            const auto untruncated = ogi::GiRecorder::takeLengthFrames (true, c.barsIdx, 4.0, c.bpm / 60.0 / kFs, 0.0, kFs);
            const bool truncated = untruncated > (std::int64_t) target && target == ogi::GiRecorder::maxFramesFor (kFs);
            s.p->setPlayHead (nullptr);
            tc.check (idle.rec.run == 0 && during.rec.run == 2 && idle.rec.capped == c.wantCapped && idle.rec.capped == truncated && ! during.rec.capped,
                      juce::String (c.bpm, 0) + " bpm idx " + juce::String (c.barsIdx) + ": capped " + (idle.rec.capped ? "1" : "0") + " truncated "
                          + (truncated ? "1" : "0") + " target " + juce::String (target) + " run " + juce::String (during.rec.run));
            capDetail << (capDetail.isEmpty() ? "" : "; ") << juce::String (c.bpm, 0) << " bpm x " << (1 << c.barsIdx) << " bars: capped "
                      << (idle.rec.capped ? "true" : "false") << ", take " << juce::String ((juce::int64) untruncated) << " -> " << target;
        }
        bool freeOk = false;
        {
            Session s (kFs, recBase (0, 0, { { "sync_mode", 0.0f }, { "rec_length_sync", 5.0f }, { "rec_length", 30.0f } }), kSeed, 512);
            s.record = false;
            s.run (2048, 512);
            freeOk = ! s.p->buildUiTelemetry (nowOf (s)).rec.capped;
        }

        row (sigOk && quietOk && tc.bad == 0 && freeOk,
             "URC2 PD10 / PD11 rec.signal true within 2000 ms of a peak >= -90 dBFS; rec.capped == the recorder's own truncation",
             "meas signal: " + sigDetail + "; -95 dBFS: " + (quietOk ? "false" : "TRUE / no peak") + " (peak "
                 + juce::String (juce::Decibels::gainToDecibels (quietPeak), 1) + " dBFS); capped " + tc.detail (4) + ": " + capDetail
                 + "; Free mode at 30 s: " + (freeOk ? "false" : "TRUE") + " | thr presence " + juce::String (juce::Decibels::gainToDecibels (ogi::GiRecorder::kPresenceLin), 0)
                 + " dBFS, hold 2000 ms, F_max " + juce::String (ogi::GiRecorder::maxFramesFor (kFs)) + " | ctrl - (the -95 dBFS and 140 bpm cases read false)");
    }

    //==========================================================================
    // UCL1 — clearSlot
    void rowCL1()
    {
        juce::String e;
        const auto fa = writeWav (gTmp32.getChildFile ("cl-a.wav"), noiseBuf (2, 24000, 0.3f, 261), kFs, 32);
        const auto big = writeWav (gTmp32.getChildFile ("cl-big.wav"), noiseBuf (2, 1440000, 0.2f, 262), kFs, 32);   // 30 s: a load that takes a while

        // (a) a loaded, sounding slot empties through the sentinel.
        bool aOk = false;
        juce::String aDetail;
        {
            Session s (kFs, stBase(), kSeed, 512);
            const bool l = s.p->loadSlotSync (0, fa, e);
            for (int n : { 48, 60 })
                s.noteOn (64, n);
            s.record = false;
            s.run (12000, 256);
            const auto clears0 = s.p->getHandoff().getClearsPublished();
            const bool had = s.eng().getCurrentTape (0) != nullptr && s.p->getSlotInfo (0).loaded;
            const auto rev0 = s.p->getSlotStateRevision();
            const auto ov0 = s.p->getOverviewRev (0);
            const auto res = s.p->clearSlot (0);
            const auto res2 = s.p->clearSlot (3);   // already empty: ok, no sentinel
            s.run (4800, 256);
            const auto i = s.p->getSlotInfo (0);
            const auto clears = s.p->getHandoff().getClearsPublished() - clears0;
            aOk = l && had && res.isEmpty() && res2.isEmpty() && clears == 1 && s.eng().getCurrentTape (0) == nullptr && ! i.loaded
               && i.source.isEmpty() && s.p->getSlotStateRevision() != rev0 && s.p->getOverviewRev (0) != ov0
               && s.p->clearSlot (4) == "badSlot" && s.p->clearSlot (-1) == "badSlot";
            aDetail << "loaded slot: '" << res << "', sentinels " << (juce::int64) clears << ", tape " << (s.eng().getCurrentTape (0) == nullptr ? "gone" : "STILL THERE")
                    << ", source '" << i.source << "', revisions moved " << yn (s.p->getSlotStateRevision() != rev0) << ", empty slot '" << res2
                    << "', slots 5 / 0: " << s.p->clearSlot (4) << " / " << s.p->clearSlot (-1);
        }

        // (b) refused while Recording / Committing into the slot; Armed is allowed; other slots are not refused.
        bool bOk = false;
        juce::String bDetail;
        {
            Session s (kFs, recBase (1, 0, { { "rec_length", 0.6f } }), kSeed, 512);
            s.load (1, noiseBuf (2, 24000, 0.2f, 263), kFs);
            s.load (2, noiseBuf (2, 24000, 0.2f, 264), kFs);
            inject (s, ScSig { 0.5f, 265 });
            s.record = false;
            s.run (4096, 512);
            armAt (s, s.pos);
            s.run (9600, 512);
            const auto st1 = pr (s).state;
            const auto r1 = s.p->clearSlot (1);          // the target, Recording
            const auto r1o = s.p->clearSlot (2);         // another slot
            s.run (28800, 512);
            const auto st2 = pr (s).state;
            const auto r2 = s.p->clearSlot (1);          // Committing
            const bool kept = s.eng().getCurrentTape (1) != nullptr && s.p->getSlotInfo (1).loaded;
            s.p->commitSync();
            s.run (2048, 256);
            const auto r3 = s.p->clearSlot (1);          // Idle again: allowed

            Session a (kFs, recBase (1, 1), kSeed, 512); // Threshold on a silent sidechain: Armed
            a.load (1, noiseBuf (2, 24000, 0.2f, 266), kFs);
            inject (a, ScSig { 0.0f, 0 });
            a.record = false;
            a.run (2048, 512);
            armAt (a, a.pos);
            a.run (4096, 512);
            const auto stA = pr (a).state;
            const auto rA = a.p->clearSlot (1);
            bOk = st1 == RS::Recording && r1 == "recording" && r1o.isEmpty() && st2 == RS::Committing && r2 == "recording" && kept && r3.isEmpty()
               && stA == RS::Armed && rA.isEmpty();
            bDetail << stName (st1) << " -> '" << r1 << "' (another slot '" << r1o << "'), " << stName (st2) << " -> '" << r2 << "', tape kept " << yn (kept)
                    << ", Idle -> '" << r3 << "', " << stName (stA) << " -> '" << rA << "'";
        }

        // (c) a load in flight is superseded.
        bool cOk = false;
        juce::String cDetail;
        {
            Session s (kFs, stBase(), kSeed, 512);
            s.record = false;
            s.run (2048, 512);
            const auto pub0 = s.p->getHandoff().getPublishedCount();
            s.p->requestSlotLoad (2, big);
            const bool inFlight = s.p->getSlotInfo (2).loading;
            const auto res = s.p->clearSlot (2);
            const bool idle = s.p->getLoader().waitForIdle (30000);
            s.run (4096, 512);
            const auto i = s.p->getSlotInfo (2);
            const auto pubs = s.p->getHandoff().getPublishedCount() - pub0;
            cOk = big.existsAsFile() && inFlight && res.isEmpty() && idle && ! i.loaded && ! i.loading && i.source.isEmpty()
               && s.eng().getCurrentTape (2) == nullptr && pubs == 0;
            cDetail << "loading at the clear " << yn (inFlight) << ", '" << res << "', after: loaded " << (i.loaded ? "YES" : "no") << ", source '"
                    << i.source << "', publishes " << (juce::int64) pubs;
        }

        row (aOk && bOk && cOk, "UCL1 PD14 clearSlot: sentinel clear; refused while Recording / Committing into the slot; a load in flight superseded",
             "meas " + aDetail + "; " + bDetail + "; in-flight 30 s load: " + cDetail
                 + " | thr 1 sentinel, 'recording' x 2, Armed allowed, 0 publishes | ctrl - (the Idle / Armed / other-slot calls are allowed)");
    }

    //==========================================================================
    // UPM6c / UPM6cn — D45: a preset leaves the lanes it has no source for alone
    struct Pm6cR
    {
        bool setup = false, applied = false;
        int evolved = 0;            // working-copy samples that differ from the freshly loaded tape
        bool dirtyBefore = false;
        bool samePtr = false, sameGen = false, sameSource = false, dirtyAfter = false, loaded = false;
        int copyDiffs = -2;
        bool slot3 = false;
    };

    Pm6cR runPM6c (bool clearUnlisted, const juce::File& preset, const juce::File& wav)
    {
        Pm6cR r;
        juce::String e;
        Session s (kFs, with (stBase(), P { { sn (1, "density"), 40.0f } }), kSeed, 512);
        const bool l = s.p->loadSlotSync (0, wav, e);
        s.record = false;
        s.run (2048, 256);
        const auto fresh = tapeCopy (s, 0);
        for (int n : { 48, 60 })
            s.noteOn (s.pos + 64, n);
        setP (*s.p, "feedback", 90.0f);
        s.run (48000, 512);
        setP (*s.p, "feedback", 0.0f);
        s.run (24000, 512);                               // the head parks: the working copy is static
        (void) s.p->buildUiTelemetry (nowOf (s));
        const auto before = tapeCopy (s, 0);
        const auto* tape0 = s.eng().getCurrentTape (0);
        const auto info0 = s.p->getSlotInfo (0);
        r.evolved = bitDiffs (fresh, before);
        r.dirtyBefore = s.p->getSlotDirty (0);
        r.setup = l && tape0 != nullptr && info0.source == "file:" + wav.getFullPathName() && preset.existsAsFile();

        if (clearUnlisted)
        {
            // The control: the same list through the session rule.
            s.p->getLoader().restoreSlots ({ factorySlot (2, "stub_tone") }, true);
            r.applied = true;
        }
        else
        {
            r.applied = loadPreset (*s.p, preset);
        }
        s.p->getLoader().waitForIdle (20000);
        s.run (4096, 256);
        (void) s.p->buildUiTelemetry (nowOf (s) + 200.0);

        const auto info1 = s.p->getSlotInfo (0);
        r.samePtr = s.eng().getCurrentTape (0) == tape0 && tape0 != nullptr;
        r.sameGen = info1.generation == info0.generation;
        r.sameSource = info1.source == info0.source;
        r.loaded = info1.loaded;
        r.copyDiffs = r.samePtr ? bitDiffs (before, tapeCopy (s, 0)) : -2;
        r.dirtyAfter = s.p->getSlotDirty (0);
        const auto i2 = s.p->getSlotInfo (2);
        r.slot3 = i2.loaded && i2.source == "factory:stub_tone" && s.eng().getCurrentTape (2) != nullptr;
        return r;
    }

    bool pm6cPredicate (const Pm6cR& r)
    {
        return r.loaded && r.samePtr && r.sameGen && r.sameSource && r.copyDiffs == 0 && r.dirtyAfter;
    }

    void rowsPM6c()
    {
        const auto wav = writeWav (gTmp32.getChildFile ("pm6c-file.wav"), noiseBuf (2, 48000, 0.3f, 271), kFs, 32);
        const auto preset = gTmp32.getChildFile ("pm6c.json");
        int listed = -1;
        {
            // A preset that names ONLY slot 3 (the factory stub).
            Session a (kFs, stBase(), kSeed, 512);
            a.p->getLoader().restoreSlots ({ factorySlot (2, "stub_tone") });
            settle (a);
            savePreset (*a.p, preset);
            const auto sl = ug31::parseFile (preset)["customState"]["slots"];
            listed = sl.isArray() ? sl.size() : -1;
        }

        const auto r = runPM6c (false, preset, wav);
        const bool stim = r.setup && r.applied && listed == 1 && r.evolved > 0 && r.dirtyBefore && r.slot3;
        row (stim && pm6cPredicate (r), "UPM6c PD15 / D45 a preset listing only slot 3 leaves slot 1's tape, generation and evolved working copy alone",
             "meas preset lists " + juce::String (listed) + " slot; slot 1 before: " + juce::String (r.evolved) + " working-copy samples differ from the file, dirty "
                 + yn (r.dirtyBefore) + "; after the load: same tape " + yn (r.samePtr) + ", same generation " + yn (r.sameGen) + ", source kept "
                 + yn (r.sameSource) + ", working copy diffs " + juce::String (r.copyDiffs) + ", dirty " + yn (r.dirtyAfter) + "; slot 3 loaded from the preset "
                 + yn (r.slot3) + " | thr same tape, 0 diffs | ctrl UPM6cn");

        const auto n = runPM6c (true, preset, wav);
        const bool fired = n.setup && n.evolved > 0 && n.slot3 && ! pm6cPredicate (n) && ! n.loaded && ! n.samePtr;
        row (fired, "UPM6cn control: restoreSlots (list, clearUnlisted = true) -> slot 1 cleared (predicate must FAIL)",
             juce::String ("meas slot 1 after: loaded ") + (n.loaded ? "YES" : "no") + ", same tape " + (n.samePtr ? "YES" : "no") + ", source kept "
                 + (n.sameSource ? "YES" : "no") + " | thr predicate 'slot 1 untouched' FAILS | "
                 + ctrl (fired, "UPM6cn flag flipped -> predicate failed, the evolved lane was cleared"));
    }

    //==========================================================================
    // UIM1 / UIM1n — the drop import
    void rowsIM1()
    {
        const auto imports = harnessImportsDir();
        imports.deleteRecursively();
        const auto display  = juce::String::fromUTF8 ("F\xc3\xaf" "eld Rec 01 \xe2\x80\x94 d\xc3\xa9p\xc3\xb4t.WAV");
        const auto display2 = juce::String ("another name.wav");
        const auto orig = writeWav (gTmp32.getChildFile ("im1-orig.wav"), noiseBuf (2, 36000, 0.3f, 281), kFs, 24);
        const auto h = fileHash (orig);
        const auto target = imports.getChildFile (ogi::gihash::toHex (h) + ".wav");   // the extension lower-cased

        Session s (kFs, stBase(), kSeed, 512);
        s.record = false;
        s.p->getLoader().hooks.resetCounters();
        const auto t1 = copyTo (orig, "wkdrop-1.WAV");
        const bool q1 = s.p->importDroppedFile (0, t1, display);
        settle (s);
        const auto i0 = s.p->getSlotInfo (0);
        const auto& hk = s.p->getLoader().hooks;
        const bool first = q1 && target.existsAsFile() && target.getSize() == orig.getSize() && ! t1.exists() && i0.loaded
                        && i0.source == "file:" + target.getFullPathName() && i0.name == display && hk.importsMoved.load() == 1
                        && hk.importsHashed.load() == 1 && hk.importsLoaded.load() == 1 && s.eng().getCurrentTape (0) != nullptr;
        const bool nonAsciiDir = nonAscii (imports.getFullPathName()) && target.isAChildOf (imports) && countFiles (imports, "*.part") == 0;

        // The same bytes again, into another lane under another name: one file.
        const auto t2 = copyTo (orig, "wkdrop-2.WAV");
        const bool q2 = s.p->importDroppedFile (1, t2, display2);
        settle (s);
        const auto i1 = s.p->getSlotInfo (1);
        const int files = countFiles (imports);
        const bool repeat = q2 && ! t2.exists() && i1.loaded && i1.source == i0.source && i1.name == display2 && files == 1
                         && hk.importsReused.load() == 1 && hk.importsMoved.load() == 1;

        // Session round trip.
        const auto state = stateOf (*s.p);
        bool round = false;
        int rung = -1;
        {
            Session b (kFs, stBase(), kSeed, 512);
            b.record = false;
            load (*b.p, state);
            settle (b);
            const auto j0 = b.p->getSlotInfo (0), j1 = b.p->getSlotInfo (1);
            rung = j0.resolvedBy;
            round = j0.loaded && j0.source == i0.source && j0.name == display && j0.contentHash == i0.contentHash && j1.loaded && j1.name == display2
                 && b.eng().getCurrentTape (0) != nullptr;
        }

        // A rate rebuild keeps the dropped name (the loader re-installs the pristine's).
        s.p->prepareToPlay (96000.0, 512);
        for (int k = 0; k < 40; ++k)
        {
            eg21::pump (10);
            if (s.p->getLoader().waitForIdle (2000) && s.p->getSlotInfo (0).tapeRate == 96000.0)
                break;
        }
        const auto ir = s.p->getSlotInfo (0);
        const bool rebuilt = ir.loaded && ir.tapeRate == 96000.0 && ir.name == display;

        row (first && nonAsciiDir && repeat && round && rebuilt,
             "UIM1 PD16 / D44 a drop lands in Imports/<XXH64 of the file>.<ext>, source file:, the dropped name; a repeat reuses it; round trip",
             "meas file " + target.getFileName() + " (" + juce::String (target.getSize()) + " bytes == the drop " + yn (target.getSize() == orig.getSize())
                 + "), temp removed " + yn (! t1.exists()) + ", source file:<Imports> " + yn (i0.source == "file:" + target.getFullPathName())
                 + ", name carried " + yn (i0.name == display) + ", non-ASCII Imports dir " + yn (nonAsciiDir) + "; repeat drop: " + juce::String (files)
                 + " file in Imports, reused " + i64 (hk.importsReused.load()) + ", moved " + i64 (hk.importsMoved.load()) + ", lane 2 named '" + i1.name
                 + "'; restored session: rung " + juce::String (rung) + ", names " + yn (round) + "; after a 96 k rebuild the name is kept " + yn (rebuilt)
                 + " | thr 1 file, hash name, both names | ctrl UIM1n");

        // UIM1n: the editor forgets the display name.
        const auto t3 = copyTo (orig, "wkdrop-3.WAV");
        Session c (kFs, stBase(), kSeed, 512);
        c.record = false;
        const bool q3 = c.p->importDroppedFile (0, t3, {});
        settle (c);
        const auto in = c.p->getSlotInfo (0);
        const bool fired = q3 && in.loaded && in.source == i0.source && in.name != display && in.name == "wkdrop-3.WAV";
        row (fired, "UIM1n control: empty display name -> the temp file's name is shown (predicate must FAIL)",
             "meas lane named '" + in.name + "' (loaded " + yn (in.loaded) + ") | thr predicate 'name == the dropped file's name' FAILS | "
                 + ctrl (fired, "UIM1n name not carried -> predicate failed, '" + in.name + "' shown"));
    }

    //==========================================================================
    // UIM2 — import failures leave the lane as it was
    void rowIM2()
    {
        const auto imports = harnessImportsDir();
        imports.deleteRecursively();
        juce::String e;
        const auto fa = writeWav (gTmp32.getChildFile ("im2-a.wav"), noiseBuf (2, 24000, 0.3f, 291), kFs, 32);
        const auto fb = writeWav (gTmp32.getChildFile ("im2-b.wav"), noiseBuf (2, 24000, 0.3f, 292), kFs, 32);

        Session s (kFs, stBase(), kSeed, 512);
        s.record = false;
        const bool l = s.p->loadSlotSync (0, fa, e);
        s.run (2048, 256);
        auto& ld = s.p->getLoader();
        ld.hooks.resetCounters();
        const auto* tape0 = s.eng().getCurrentTape (0);
        const auto info0 = s.p->getSlotInfo (0);

        struct Case { const char* name; std::atomic<bool>* hook; bool async; };
        const Case cases[] = { { "Imports dir", &ld.hooks.failImportDir, false }, { "move", &ld.hooks.failImportMove, true },
                               { "rename", &ld.hooks.failImportRename, false } };
        Tally t;
        juce::String d;
        int k = 0;
        for (const auto& c : cases)
        {
            const auto temp = copyTo (fb, "im2-drop-" + juce::String (++k) + ".wav");
            const auto rev0 = s.p->getSlotStateRevision();
            c.hook->store (true);
            juce::String err;
            bool ret = true;
            if (c.async)
            {
                ret = ! s.p->importDroppedFile (0, temp, "B take.wav");   // queued (true) -> ret false = "no synchronous failure"
                ld.waitForIdle (20000);
            }
            else
            {
                ret = ld.importSync (0, temp, "B take.wav", err);
            }
            c.hook->store (false);
            s.run (1024, 256);
            const auto i = s.p->getSlotInfo (0);
            const auto json = GiUiJson::slotStateToVar (*s.p)["slots"][0];
            const bool unchanged = s.eng().getCurrentTape (0) == tape0 && tape0 != nullptr && i.loaded && i.generation == info0.generation
                                && i.source == info0.source && i.name == info0.name && ! i.loading;
            const bool reported = i.lastError.startsWith ("import failed") && json["error"].toString() == i.lastError
                               && s.p->getSlotStateRevision() != rev0;
            const bool clean = ! temp.exists() && countFiles (imports, "*.part") == 0 && countFiles (imports) == 0
                            && ! i.source.contains (temp.getFileName());
            t.check (! ret && unchanged && reported && clean,
                     juce::String (c.name) + ": ret " + (ret ? "1" : "0") + " unchanged " + (unchanged ? "1" : "0") + " reported " + (reported ? "1" : "0")
                         + " clean " + (clean ? "1" : "0") + " '" + i.lastError + "'");
            d << (d.isEmpty() ? "" : "; ") << c.name << ": '" << i.lastError << "'";
        }
        // An empty lane stays empty, with the error.
        const auto temp4 = copyTo (fb, "im2-drop-4.wav");
        ld.hooks.failImportDir.store (true);
        juce::String err4;
        const bool r4 = ld.importSync (3, temp4, "B take.wav", err4);
        ld.hooks.failImportDir.store (false);
        const auto i3 = s.p->getSlotInfo (3);
        const bool emptyOk = ! r4 && ! i3.loaded && i3.source.isEmpty() && i3.lastError.startsWith ("import failed") && ! temp4.exists();

        row (l && t.bad == 0 && emptyOk && ld.hooks.importsFailed.load() == 4 && ld.hooks.importPartsRemoved.load() >= 1 && ld.hooks.importsLoaded.load() == 0,
             "UIM2 PD16 an import that fails (Imports dir / move / rename) leaves the lane as it was, sets the error, leaves nothing behind",
             "meas " + t.detail (3) + " lanes unchanged (same tape, generation, source, name), " + d + "; empty lane stays empty with the error " + yn (emptyOk)
                 + "; failed " + i64 (ld.hooks.importsFailed.load()) + ", .part removed " + i64 (ld.hooks.importPartsRemoved.load()) + ", Imports files "
                 + juce::String (countFiles (imports)) + " | thr unchanged x 3, error set, slotRev moved, 0 files | ctrl - (UIM1 imports the same bytes)");
    }

    //==========================================================================
    // UIM3 — the file hash, hash scope, a superseded import
    void rowIM3()
    {
        using ogi::gihash::xxh64;
        using ogi::gihash::Xxh64Stream;

        // (a) streaming == one-shot, every length, three chunkings.
        constexpr int kBig = 3 * 1024 * 1024;
        std::vector<std::uint8_t> bytes ((size_t) kBig);
        {
            juce::Random r ((juce::int64) 0x32A30001);
            for (auto& b : bytes)
                b = (std::uint8_t) r.nextInt (256);
        }
        const int lengths[] = { 0, 1, 31, 32, 33, 65535, 65536, 65537, kBig };
        const int cycle[] = { 7, 31, 32, 33, 4096, 65536, 1, 100000 };
        int compared = 0, hashBad = 0;
        for (const int len : lengths)
        {
            const auto want = xxh64 (bytes.data(), (size_t) len);
            Xxh64Stream one;
            one.update (bytes.data(), (size_t) len);
            Xxh64Stream cyc;
            for (int pos = 0, k = 0; pos < len; ++k)
            {
                const int n = std::min (cycle[k % 8], len - pos);
                cyc.update (bytes.data() + pos, (size_t) n);
                pos += n;
            }
            Xxh64Stream fine;   // byte by byte (17-byte steps for the 3 MiB buffer)
            const int step = len > 70000 ? 17 : 1;
            for (int pos = 0; pos < len; pos += step)
                fine.update (bytes.data() + pos, (size_t) std::min (step, len - pos));
            for (const auto got : { one.digest(), cyc.digest(), fine.digest() })
            {
                ++compared;
                hashBad += got == want ? 0 : 1;
            }
        }
        // A reused stream object after reset().
        Xxh64Stream re;
        re.update (bytes.data(), 1000);
        re.reset();
        re.update (bytes.data(), 33);
        ++compared;
        hashBad += re.digest() == xxh64 (bytes.data(), 33) ? 0 : 1;

        // (b) equal contentHash (identical first 131072 frames), different tails.
        const auto imports = harnessImportsDir();
        imports.deleteRecursively();
        auto a = noiseBuf (2, 140000, 0.3f, 301);
        auto b = a;
        for (int i = 135000; i < 140000; ++i)
            b.setSample (0, i, a.getSample (0, i) * 0.5f);
        const auto fa = writeWav (gTmp32.getChildFile ("im3-a.wav"), a, kFs, 32);
        const auto fb = writeWav (gTmp32.getChildFile ("im3-b.wav"), b, kFs, 32);
        Session s (kFs, stBase(), kSeed, 512);
        s.record = false;
        auto& ld = s.p->getLoader();
        ld.hooks.resetCounters();
        juce::String e;
        const bool ia = ld.importSync (0, copyTo (fa, "im3-drop-a.wav"), "a.wav", e);
        const bool ib = ld.importSync (1, copyTo (fb, "im3-drop-b.wav"), "b.wav", e);
        s.run (2048, 256);
        const auto i0 = s.p->getSlotInfo (0), i1 = s.p->getSlotInfo (1);
        const bool sameSize = fa.getSize() == fb.getSize();
        const bool sameContentHash = i0.hasHash && i1.hasHash && i0.contentHash == i1.contentHash
                                  && ogi::gihash::contentHash (a) == ogi::gihash::contentHash (b);
        const bool distinct = ia && ib && i0.loaded && i1.loaded && i0.source != i1.source && countFiles (imports) == 2
                           && recFileOf (i0).getFileNameWithoutExtension() == ogi::gihash::toHex (fileHash (fa))
                           && recFileOf (i1).getFileNameWithoutExtension() == ogi::gihash::toHex (fileHash (fb));

        // (c) a superseded import leaves no .part and no file.
        const auto fc = writeWav (gTmp32.getChildFile ("im3-c.wav"), noiseBuf (2, 24000, 0.3f, 302), kFs, 32);
        const auto fd = writeWav (gTmp32.getChildFile ("im3-d.wav"), noiseBuf (2, 24000, 0.3f, 303), kFs, 32);
        const auto tc = copyTo (fc, "im3-drop-c.wav");
        const auto targetC = imports.getChildFile (ogi::gihash::toHex (fileHash (fc)) + ".wav");
        ld.hooks.importHoldMs.store (20000);
        const bool qc = s.p->importDroppedFile (2, tc, "c.wav");
        bool holding = false;
        for (int k = 0; k < 5000 && ! holding; ++k)
        {
            holding = ld.hooks.importHolding.load();
            if (! holding)
                juce::Thread::sleep (1);
        }
        const int partsDuring = countFiles (imports, "*.part");
        const auto gen2 = s.p->getSlotInfo (2).generation;
        ld.hooks.importHoldMs.store (0);
        const bool qd = s.p->importDroppedFile (2, copyTo (fd, "im3-drop-d.wav"), "d.wav");   // the newer drop wins
        const bool idle = ld.waitForIdle (30000);
        s.run (2048, 256);
        const auto i2 = s.p->getSlotInfo (2);
        const bool superseded = qc && qd && holding && partsDuring == 1 && gen2 == 0 && idle && countFiles (imports, "*.part") == 0
                             && ! targetC.exists() && ! tc.exists() && i2.loaded && i2.name == "d.wav"
                             && ld.hooks.importsSuperseded.load() == 1 && ld.hooks.importPartsRemoved.load() == 1 && countFiles (imports) == 3;

        row (hashBad == 0 && sameSize && sameContentHash && distinct && superseded,
             "UIM3 PD16 Xxh64Stream == xxh64 (9 lengths x 3 chunkings); equal contentHash + different tails -> 2 files; superseded import leaves no .part",
             "meas stream == one-shot " + juce::String (compared - hashBad) + "/" + juce::String (compared) + " (0, 1, 31, 32, 33, 65535, 65536, 65537, 3 MiB); "
                 + "two files of " + juce::String (fa.getSize()) + " bytes, contentHash equal " + yn (sameContentHash) + " ("
                 + ogi::gihash::toHex (i0.contentHash) + "), Imports names " + recFileOf (i0).getFileNameWithoutExtension() + " / "
                 + recFileOf (i1).getFileNameWithoutExtension() + " distinct " + yn (distinct) + "; superseded: held with " + juce::String (partsDuring)
                 + " .part on disk, lane untouched meanwhile " + yn (gen2 == 0) + ", after: .part " + juce::String (countFiles (imports, "*.part"))
                 + ", its target " + (targetC.exists() ? "EXISTS" : "absent") + ", temp " + (tc.exists() ? "EXISTS" : "removed") + ", lane holds '" + i2.name
                 + "', superseded " + i64 (ld.hooks.importsSuperseded.load()) + ", .part removed " + i64 (ld.hooks.importPartsRemoved.load())
                 + " | thr all equal, 2 names, 0 .part | ctrl - (b: contentHash cannot tell the two files apart)");
    }

    //==========================================================================
    // USV1 — Save recording
    void rowSV1()
    {
        const auto outDir = gTmp32.getChildFile ("saves");
        outDir.createDirectory();
        juce::String e;

        // (a) feedback 0, a HOT take (samples beyond +-1): the file == the tape through the same 24-bit writer.
        bool exactOk = false;
        juce::String exactDetail;
        bool guardsOk = false;
        juce::String guardDetail;
        {
            Session s (kFs, stBase(), kSeed, 512);
            s.record = false;
            const bool take = commitTake (s, 0, 60000, 1.6f, 311);
            const auto tc = tapeCopy (s, 0);
            const int frames = (int) (tc.size() / 2);
            int hot = 0;
            for (const float v : tc)
                hot += std::abs (v) > 1.0f ? 1 : 0;

            const auto r = s.p->saveRecordingSync (0, outDir.getChildFile ("sv1-take.aiff"), [&s] { s.run (512, 512); });
            const auto w = readWav (juce::File (r.path));

            juce::AudioBuffer<float> ref (2, juce::jmax (1, frames));
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < frames; ++i)
                    ref.setSample (ch, i, juce::jlimit (-1.0f, 1.0f, tc[(size_t) ch * (size_t) frames + (size_t) i]));
            const auto refBack = readWav (writeWav (outDir.getChildFile ("sv1-ref.wav"), ref, kFs, 24));
            const int diffs = bitDiffs (w.pcm, refBack.pcm);
            float peak = 0.0f;
            for (const float v : w.pcm)
                peak = std::max (peak, std::abs (v));
            const bool unchanged = bitDiffs (tc, tapeCopy (s, 0)) == 0;
            exactOk = take && frames == 60000 && hot > 0 && r.ok && r.error.isEmpty() && r.path.endsWith ("sv1-take.wav") && w.ok && w.bits == 24
                   && w.channels == 2 && w.rate == kFs && w.frames == frames && diffs == 0 && peak <= 1.0f && unchanged
                   && ! juce::File (r.path + ".part").exists() && ! outDir.getChildFile ("sv1-take.aiff").exists();
            exactDetail << "hot take (" << hot << " tape samples beyond +-1): ok " << yn (r.ok) << ", '" << juce::File (r.path).getFileName() << "' "
                        << w.bits << "-bit " << w.channels << " ch " << juce::String (w.rate, 0) << " Hz " << w.frames << " frames, diffs vs the tape's level 0 "
                        << "(clamped, same writer) " << diffs << "/" << (int) w.pcm.size() << ", peak " << juce::String (peak, 7) << ", no .part";

            // The guards, in order.
            const auto fx = writeWav (gTmp32.getChildFile ("sv1-file.wav"), noiseBuf (2, 24000, 0.3f, 312), kFs, 32);
            s.p->loadSlotSync (1, fx, e);
            ogi::SavedSlot gone;
            gone.index = 2;
            gone.source = "rec:/nonexistent-ogi-32/gone.wav";
            gone.name = "gone.wav";
            auto list = s.p->getLoader().getSavedSlots();
            list.push_back (gone);
            s.p->getLoader().restoreSlots (list);
            settle (s);
            const auto own = recFileOf (s.p->getSlotInfo (0));
            const auto g1 = s.p->saveRecordingSync (1, outDir.getChildFile ("g1.wav")).error;          // a file: lane
            const auto g2 = s.p->saveRecordingSync (3, outDir.getChildFile ("g2.wav")).error;          // an empty lane
            const auto g3 = s.p->saveRecordingSync (2, outDir.getChildFile ("g3.wav")).error;          // rec: but missing
            const auto g4 = s.p->saveRecordingSync (0, own).error;                                     // its own take
            const auto g5 = s.p->saveRecordingSync (0, fx).error;                                      // another lane's file
            const auto g6 = s.p->saveRecordingSync (7, outDir.getChildFile ("g6.wav")).error;
            guardsOk = own.existsAsFile() && g1 == "notRecording" && g2 == "notRecording" && g3 == "empty" && g4 == "inUse" && g5 == "inUse"
                    && g6.isNotEmpty() && ! outDir.getChildFile ("g1.wav").exists() && ! outDir.getChildFile ("g3.wav").exists()
                    && fx.getSize() > 100000 && s.p->getSlotInfo (2).missing;
            guardDetail << "file: lane '" << g1 << "', empty lane '" << g2 << "', missing rec: lane '" << g3 << "', own take '" << g4
                        << "', another lane's file '" << g5 << "', slot 8 '" << g6 << "'";
        }

        // (b) feedback on: every saved sample is a value the tape held around the block that copied its chunk.
        bool fbOk = false;
        juce::String fbDetail;
        {
            Session s (kFs, with (stBase(), P { { sn (1, "density"), 40.0f } }), kSeed, 512);
            s.record = false;
            const bool take = commitTake (s, 0, 48000, 0.4f, 313);
            for (int n : { 48, 60 })
                s.noteOn (s.pos + 64, n);
            setP (*s.p, "feedback", 90.0f);
            s.run (24000, 512);
            auto& snap = s.eng().tapeSnapshot();
            struct Chunk { std::uint32_t index = 0; std::vector<float> pre, post; };
            std::vector<Chunk> chunks;
            bool copied = false;   // every chunk seen: stop copying the tape per block
            const auto r = s.p->saveRecordingSync (0, outDir.getChildFile ("sv1-fb.wav"), [&]
            {
                if (copied)
                {
                    s.run (512, 512);
                    return;
                }
                // The worker raises the request at any moment, so EVERY block
                // until the last chunk is bracketed by a tape copy.
                Chunk c;
                c.pre = tapeCopy (s, 0);
                const auto before = snap.progress();
                s.run (512, 512);
                const auto after = snap.progress();
                if (after == before || after == 0)
                    return;                       // nothing copied in this block
                c.index = after - 1;              // one chunk per process call at most
                c.post = tapeCopy (s, 0);
                copied = (int) after * Snap::kChunkFrames >= (int) (c.post.size() / 2);
                chunks.push_back (std::move (c));
            });
            const auto w = readWav (juce::File (r.path));
            constexpr double kLsb = 1.0 / 8388608.0;
            int covered = 0, neither = 0, moved = 0, fromPost = 0;
            const int frames = w.frames;
            for (const auto& c : chunks)
            {
                const int f0 = (int) c.index * Snap::kChunkFrames, f1 = std::min (frames, f0 + Snap::kChunkFrames);
                if ((int) c.pre.size() != 2 * frames || (int) c.post.size() != 2 * frames)
                    continue;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = f0; i < f1; ++i)
                    {
                        const size_t k = (size_t) ch * (size_t) frames + (size_t) i;
                        const double v = (double) w.pcm[k];
                        const bool a = std::abs (v - (double) c.pre[k]) <= kLsb, b = std::abs (v - (double) c.post[k]) <= kLsb;
                        ++covered;
                        neither += (a || b) ? 0 : 1;
                        moved += sameF (c.pre[k], c.post[k]) ? 0 : 1;
                        fromPost += (b && ! a) ? 1 : 0;
                    }
            }
            fbOk = take && r.ok && w.ok && frames == 48000 && (int) chunks.size() == 3 && covered == 2 * frames && neither == 0 && moved > 0;
            fbDetail << "feedback 90: ok " << yn (r.ok) << ", " << (int) chunks.size() << " chunks in " << (int) chunks.size() << " blocks, " << covered
                     << " samples each == the tape before or after its copy block (within 1 LSB), neither " << neither << ", tape samples the head rewrote in those blocks "
                     << moved << " (" << fromPost << " saved in their new value)";
        }

        // (c) the tape is swapped while the copy runs -> changed.
        bool changedOk = false;
        juce::String changedDetail;
        {
            Session s (kFs, stBase(), kSeed, 512);
            s.record = false;
            const bool take = commitTake (s, 0, 144000, 0.4f, 314);   // 9 chunks
            int calls = 0;
            bool swapped = false;
            std::uint32_t progressAtSwap = 0;
            const auto r = s.p->saveRecordingSync (0, outDir.getChildFile ("sv1-changed.wav"), [&]
            {
                if (++calls == 3 && ! swapped)
                {
                    progressAtSwap = s.eng().tapeSnapshot().progress();
                    juce::String err;
                    swapped = s.p->getLoader().commitSync (0, makeTake (144000, 0.4f, 315), err);
                }
                s.run (512, 512);
            });
            changedOk = take && swapped && progressAtSwap >= 1 && progressAtSwap < 9 && ! r.ok && r.error == "changed"
                     && ! outDir.getChildFile ("sv1-changed.wav").exists() && ! outDir.getChildFile ("sv1-changed.wav.part").exists()
                     && ! s.p->saveBufferHeldForTest();
            changedDetail << "a new take lands after chunk " << (int) progressAtSwap << "/9: '" << r.error << "', no file, buffer freed "
                          << yn (! s.p->saveBufferHeldForTest());
        }

        row (exactOk && guardsOk && fbOk && changedOk,
             "USV1 PD18 / PD19 Save: file == the tape's level 0 (24-bit stereo, tape rate, clipped); feedback window; guards; tape swap -> changed",
             "meas " + exactDetail + "; guards: " + guardDetail + "; " + fbDetail + "; " + changedDetail
                 + " | thr 0 diffs, 0 neither, notRecording / empty / inUse / changed | ctrl - (timeout: USV2)");
    }

    //==========================================================================
    // USV2 — the snapshot buffer lifetime
    void rowSV2()
    {
        const auto outDir = gTmp32.getChildFile ("saves2");
        outDir.createDirectory();
        bool ok = false;
        juce::String d;
        {
            Session s (kFs, stBase(), kSeed, 512);
            s.record = false;
            const bool take = commitTake (s, 0, 96000, 0.4f, 321);
            const auto pump = [&s] { s.run (512, 512); };

            // 1. audio stopped: the real 2000 ms no-progress limit.
            const double w0 = juce::Time::getMillisecondCounterHiRes();
            const auto r1 = s.p->saveRecordingSync (0, outDir.getChildFile ("t1.wav"));
            const double elapsed = juce::Time::getMillisecondCounterHiRes() - w0;
            const bool held1 = s.p->saveBufferHeldForTest();
            const auto st1 = s.eng().tapeSnapshot().state();
            // 2. a second save while the buffer is still lent.
            const auto r2 = s.p->saveRecordingSync (0, outDir.getChildFile ("t2.wav"));
            // 3. audio resumes: the audio thread acknowledges, the worker frees.
            s.run (1024, 512);
            const auto st3 = s.eng().tapeSnapshot().state();
            int waited = 0;
            while (s.p->saveBufferHeldForTest() && waited < 2000)
            {
                juce::Thread::sleep (5);
                waited += 5;
            }
            const bool freed = ! s.p->saveBufferHeldForTest();
            // 4. a third save, audio running.
            const auto r3 = s.p->saveRecordingSync (0, outDir.getChildFile ("t3.wav"), pump);
            const auto w3 = readWav (juce::File (r3.path));
            // 5. a timeout reclaimed by prepareToPlay (audio stopped: rule (b)), then a save.
            const auto r4 = s.p->saveRecordingSync (0, outDir.getChildFile ("t4.wav"));
            const bool held4 = s.p->saveBufferHeldForTest();
            s.p->prepareToPlay (kFs, 512);
            const bool freed4 = ! s.p->saveBufferHeldForTest();
            s.run (2048, 512);
            const auto r5 = s.p->saveRecordingSync (0, outDir.getChildFile ("t5.wav"), pump);
            // 6. a timeout, then the processor is destroyed with the buffer lent.
            const auto r6 = s.p->saveRecordingSync (0, outDir.getChildFile ("t6.wav"));
            const bool held6 = s.p->saveBufferHeldForTest();

            ok = take && r1.error == "timeout" && ! r1.ok && elapsed >= Proc::kSaveNoProgressMs && elapsed < Proc::kSaveNoProgressMs + 1000.0 && held1
              && ! Snap::isTerminal (st1) && st1 != Snap::Idle && r2.error == "busy" && Snap::isTerminal (st3) && freed && r3.ok && w3.ok
              && w3.frames == 96000 && r4.error == "timeout" && held4 && freed4 && r5.ok && r6.error == "timeout" && held6
              && ! outDir.getChildFile ("t1.wav").exists() && ! outDir.getChildFile ("t2.wav").exists();
            d << "audio stopped: '" << r1.error << "' after " << juce::String (elapsed, 0) << " ms (wall), buffer held " << yn (held1)
              << "; second save: '" << r2.error << "'; audio resumes: acknowledged " << yn (Snap::isTerminal (st3)) << ", freed " << yn (freed) << " (+"
              << waited << " ms); third save ok " << yn (r3.ok) << " (" << w3.frames << " frames); second timeout '" << r4.error
              << "' -> prepareToPlay frees " << yn (freed4) << ", save ok " << yn (r5.ok) << "; third timeout '" << r6.error << "' -> destroyed with the buffer lent";
        }   // the processor is destroyed here, the buffer still lent
        row (ok, "USV2 PD18 lifetime: timeout with audio stopped (buffer held), busy, acknowledged + freed on resume, third save ok; prepare / destroy",
             "meas " + d + " (no crash); ASan not run (the harness has no sanitizer variant configured) | thr timeout >= 2000 ms, busy, freed, ok | "
                 "ctrl - (the third save succeeds through the same path)");
    }

    //==========================================================================
    // UEB1 — encodedBytes
    void rowEB1()
    {
        juce::String e;
        const auto blobBytes = [] (Session& s, int slot)
        {
            juce::MemoryOutputStream mo;
            juce::Base64::convertFromBase64 (mo, s.p->getLoader().getBlob (slot));
            return (juce::int64) mo.getDataSize();
        };
        const auto jsonOf = [] (Session& s, int slot) { return GiUiJson::slotStateToVar (*s.p)["slots"][slot]; };

        bool fileOk = false, takeOk = false, restoredOk = false, bigOk = false;
        juce::int64 fileBytes = -1, takeBytes = -1, restoredBytes = -1, bigBytes = -1;
        {
            Session s (kFs, stBase(), kSeed, 512);
            s.record = false;
            const auto f = writeWav (gTmp32.getChildFile ("eb-file.wav"), noiseBuf (2, 48000, 0.3f, 331), kFs, 24);
            const bool l = s.p->loadSlotSync (0, f, e);
            const auto before = s.p->getSlotInfo (0);
            const auto j0 = jsonOf (s, 0);
            const bool zero = ! before.hasBlob && before.encodedBytes == 0 && static_cast<int> (j0["encodedBytes"]) == 0 && static_cast<bool> (j0["embeddable"]);
            s.p->setSlotEmbed (0, true);
            s.p->getLoader().waitForIdle (20000);
            const auto after = s.p->getSlotInfo (0);
            fileBytes = after.encodedBytes;
            const auto j1 = jsonOf (s, 0);
            fileOk = l && zero && after.hasBlob && fileBytes > 0 && fileBytes == blobBytes (s, 0) && static_cast<int> (j1["encodedBytes"]) == (int) fileBytes
                  && static_cast<bool> (j1["embeddable"]);

            recordTake (s, 1, ScSig { 0.4f, 332 }, 1.0);       // a take embeds by default
            s.p->getLoader().waitForIdle (20000);
            const auto ti = s.p->getSlotInfo (1);
            takeBytes = ti.encodedBytes;
            takeOk = ti.hasBlob && takeBytes > 0 && takeBytes == blobBytes (s, 1);

            // Restored state: the carried blob counts at once (before the resolve).
            Session b (kFs, stBase(), kSeed, 512);
            b.record = false;
            load (*b.p, stateOf (*s.p));
            const auto r0 = b.p->getSlotInfo (0), r1 = b.p->getSlotInfo (1);
            restoredBytes = r0.encodedBytes;
            settle (b);
            restoredOk = r0.encodedBytes == fileBytes && r1.encodedBytes == takeBytes && b.p->getSlotInfo (0).encodedBytes == fileBytes
                      && b.p->getSlotInfo (1).encodedBytes == takeBytes;

            // Embed off: 0 again.
            s.p->setSlotEmbed (0, false);
            s.p->getLoader().waitForIdle (20000);
            fileOk = fileOk && s.p->getSlotInfo (0).encodedBytes == 0 && ! s.p->getSlotInfo (0).hasBlob;
        }
        {
            // 30 s of full-scale stereo noise at 96 k: more than 16 MiB of FLAC-24.
            constexpr double fs = 96000.0;
            Session s (fs, stBase(), kSeed, 4096);
            s.record = false;
            const auto f = writeWav (gTmp32.getChildFile ("eb-big.wav"), noiseBuf (2, 2880000, 1.0f, 333), fs, 32);
            const bool l = s.p->loadSlotSync (0, f, e);
            s.p->setSlotEmbed (0, true);
            s.p->getLoader().waitForIdle (120000);
            const auto i = s.p->getSlotInfo (0);
            bigBytes = i.encodedBytes;
            const auto j = jsonOf (s, 0);
            bigOk = l && i.tooLarge && ! i.hasBlob && bigBytes == ogi::SlotLoader::kEmbedMaxBytes + 1
                 && static_cast<int> (j["encodedBytes"]) == (int) (ogi::SlotLoader::kEmbedMaxBytes + 1) && ! static_cast<bool> (j["embeddable"]);
            f.deleteFile();
        }
        row (fileOk && takeOk && restoredOk && bigOk, "UEB1 PD20 encodedBytes: 0 before an embed, == the blob's decoded size after, 16 MiB + 1 when too large",
             "meas file: 0 -> " + juce::String (fileBytes) + " (== the decoded blob " + yn (fileOk) + ", 0 again with embed off); take: " + juce::String (takeBytes)
                 + " (" + yn (takeOk) + "); restored state, before the resolve: " + juce::String (restoredBytes) + " (" + yn (restoredOk)
                 + "); 30 s @ 96 k full-scale: " + juce::String (bigBytes) + " (tooLarge, embeddable false " + yn (bigOk) + ") | thr exact, "
                 + juce::String (ogi::SlotLoader::kEmbedMaxBytes + 1) + " | ctrl - (0 before the embed)");
    }

    //==========================================================================
    // UST1 — the stale frame
    void rowST1()
    {
        Session s (kFs, fourStreams (40.0f, 200.0f), kSeed, 512);
        loadNoise4 (s, 4.0, 0.2f);
        for (int n : { 48, 60 })
            s.noteOn (64, n);
        s.record = false;
        s.run (48000, 512);
        const double t0 = 50000.0;
        const auto live = s.p->buildUiTelemetry (t0);                 // the frame moved: fresh
        const auto a = s.p->buildUiTelemetry (t0 + 249.5);            // no block since: 249.5 ms
        const auto b = s.p->buildUiTelemetry (t0 + 250.0);            // 250 ms: stale
        const auto c = s.p->buildUiTelemetry (t0 + 300.0);
        s.run (512, 512);                                             // one block
        const auto d = s.p->buildUiTelemetry (t0 + 301.0);

        const auto empty = [] (const Tel& t)
        {
            bool ok = t.stale && t.grainCount == 0;
            for (const auto& v : t.voices)
                ok = ok && v.state == 0 && v.note == -1 && v.scan[0] == -1.0f && v.scan[1] == -1.0f && v.scan[2] == -1.0f && v.scan[3] == -1.0f;
            for (const int n : t.counts)
                ok = ok && n == 0;
            return ok;
        };
        const auto held = [] (const Tel& t)
        {
            int n = 0;
            for (const auto& v : t.voices)
                n += v.state;
            return n;
        };
        const auto text = juce::JSON::toString (GiUiJson::telemetryToVar (c), false);
        const auto parsed = juce::JSON::parse (text);
        bool jsonOk = parsed["voices"].size() == 20 && parsed["grains"].size() == 0 && parsed["counts"].size() == 4;
        for (int k = 0; jsonOk && k < 20; ++k)
            jsonOk = static_cast<int> (parsed["voices"][k][0]) == 0 && static_cast<int> (parsed["voices"][k][1]) == -1;
        for (int k = 0; jsonOk && k < 4; ++k)
            jsonOk = static_cast<int> (parsed["counts"][k]) == 0;

        const bool stim = ! live.stale && live.grainCount >= 20 && held (live) == 2;
        row (stim && ! a.stale && a.grainCount == live.grainCount && held (a) == 2 && empty (b) && empty (c) && jsonOk && ! d.stale && d.grainCount >= 20
                 && held (d) == 2,
             "UST1 PD4 stale frame: no processBlock for >= 250 scripted ms -> 20 free voice rows, no grains, zero counts; the next block resumes",
             "meas live " + juce::String (live.grainCount) + " grains / " + juce::String (held (live)) + " voices; +249.5 ms " + juce::String (a.grainCount)
                 + " grains (stale " + (a.stale ? "YES" : "no") + "); +250 ms stale " + yn (b.stale) + ", empty " + yn (empty (b)) + "; +300 ms empty " + yn (empty (c))
                 + ", JSON 20 free rows / [] / [0, 0, 0, 0] " + yn (jsonOk) + "; one block later " + juce::String (d.grainCount) + " grains / "
                 + juce::String (held (d)) + " voices | thr stale at exactly 250 ms on the injected clock | ctrl - (the +249.5 ms read is live)");
    }

    //==========================================================================
    // UAL1 — allocations: 256 grains publishing + a snapshot copying
    P fullPool()
    {
        return with (fourStreams (100.0f, 400.0f), P { { "grain_limit", 3.0f }, { "polyphony", 16.0f } });
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

        const auto out = gTmp32.getChildFile ("al1-save.wav");
        Session s (kFs, fullPool(), kSeed, 256);
        s.record = false;
        for (int k = 0; k < 3; ++k)
            s.load (k, noiseBuf (2, 96000, 0.2f, 341 + k), kFs);
        const bool take = commitTake (s, 3, 144000, 0.3f, 344);   // 9 chunks; stream 4 reads it
        for (int n = 0; n < 16; ++n)
            s.noteOn (s.pos + 64, 40 + n);
        int maxGrains = 0, maxLive = 0;
        std::uint32_t maxProgress = 0;
        int copyingBlocks = 0;
        s.afterBlock = [&] (Session& ss)
        {
            const auto& f = ss.eng().getTelemetry().frames.read();   // this row never builds telemetry: the frame is free to read
            maxGrains = std::max (maxGrains, (int) f.count);
            int live = 0;
            for (int k = 0; k < (int) f.count; ++k)
                live += (f.grains[k].flags & 2u) == 0 ? 1 : 0;
            maxLive = std::max (maxLive, live);
            const auto st = ss.eng().tapeSnapshot().state();
            copyingBlocks += st == Snap::Copying ? 1 : 0;
            maxProgress = std::max (maxProgress, ss.eng().tapeSnapshot().progress());
        };
        s.arm = true;
        s.run (48000, 256);                                        // the pool fills
        const auto r = s.p->saveRecordingSync (3, out, [&s] { s.run (256, 256); });
        s.run (4800, 256);
        s.arm = false;
        s.afterBlock = nullptr;
        malloc_logger = nullptr;
        const int allocs = allocCount;
        const auto w = readWav (out);

        const bool stim = take && maxLive == 256 && maxGrains >= 256 && r.ok && maxProgress == 9 && copyingBlocks >= 8 && w.frames == 144000;
        row (stim && allocs == 0 && liveness > 0, "UAL1 PERF-01 0 audio-thread allocations with 256 grains publishing and a tape snapshot copying",
             "meas " + juce::String (allocs) + " allocs; frame max " + juce::String (maxGrains) + " grains (" + juce::String (maxLive)
                 + " live), snapshot " + juce::String ((int) maxProgress) + " chunks over " + juce::String (copyingBlocks) + " Copying blocks, save ok " + yn (r.ok)
                 + " (" + juce::String (w.frames) + " frames) | thr 0 | " + ctrl (liveness > 0, "liveness malloc(64) armed -> " + juce::String (liveness)));
       #else
        row (true, "UAL1 (mac only)", "skipped");
       #endif
    }

    //==========================================================================
    // UTL32 — the 3.2 telemetry shape, as the page receives it (through JSON text)
    juce::String tl32Problems (const Tel& src)
    {
        const auto text = juce::JSON::toString (GiUiJson::telemetryToVar (src), false);
        const auto t = juce::JSON::parse (text);
        juce::StringArray bad;
        const juce::StringArray topKeys { "scaleRev", "slotRev", "lanes", "voices", "newest", "menv", "grains", "counts", "limit", "out",
                                          "rec", "sources", "modNorm" };
        if (keysOf (t) != topKeys)
            bad.add ("top keys " + keysOf (t).joinIntoString (","));
        if (! (t["scaleRev"].isInt() && t["slotRev"].isInt()))
            bad.add ("revisions not ints");

        const auto lanes = t["lanes"];
        int nulls = 0;
        if (! lanes.isArray() || lanes.size() != 4)
            bad.add ("lanes not 4");
        else
            for (int i = 0; i < 4; ++i)
            {
                const auto l = lanes[i];
                const auto& sl = src.lanes[(size_t) i];
                if (keysOf (l) != juce::StringArray { "ovRev", "fb", "rec" })
                    bad.add ("lane " + juce::String (i) + " keys " + keysOf (l).joinIntoString (","));
                if (! l["ovRev"].isInt())
                    bad.add ("lane " + juce::String (i) + " ovRev");
                if (sl.hasFb ? ! (isNumber (l["fb"]) && std::abs (static_cast<double> (l["fb"]) - (double) sl.fb) <= 5.0e-5) : ! l["fb"].isVoid())
                    bad.add ("lane " + juce::String (i) + " fb");
                if (sl.hasRec ? ! (isNumber (l["rec"]) && std::abs (static_cast<double> (l["rec"]) - (double) sl.rec) <= 5.0e-5) : ! l["rec"].isVoid())
                    bad.add ("lane " + juce::String (i) + " rec");
                nulls += (sl.hasFb ? 0 : 1) + (sl.hasRec ? 0 : 1);
            }
        // In the text the page parses, an absent value is a literal null (never 0).
        int literal = 0;
        for (int at = text.indexOf ("null"); at >= 0; at = text.indexOf (at + 4, "null"))
            ++literal;
        if (literal != nulls)
            bad.add ("literal nulls " + juce::String (literal) + " != " + juce::String (nulls));

        const auto voices = t["voices"];
        if (! voices.isArray() || voices.size() != 20)
            bad.add ("voices not 20");
        else
            for (int k = 0; k < 20; ++k)
            {
                const auto v = voices[k];
                bool ok = v.isArray() && v.size() == 8;
                for (int c = 0; ok && c < 8; ++c)
                    ok = isNumber (v[c]);
                ok = ok && static_cast<int> (v[0]) == src.voices[(size_t) k].state && static_cast<int> (v[1]) == src.voices[(size_t) k].note
                        && static_cast<int> (v[7]) == 0;
                for (int c = 0; ok && c < 4; ++c)
                    ok = std::abs (static_cast<double> (v[3 + c]) - (double) src.voices[(size_t) k].scan[(size_t) c]) <= 5.0e-5;
                if (! ok)
                    bad.add ("voice row " + juce::String (k));
            }

        const auto grains = t["grains"];
        if (! grains.isArray() || grains.size() != src.grainCount)
            bad.add ("grains " + juce::String (grains.size()) + " != " + juce::String (src.grainCount));
        else
            for (int k = 0; k < grains.size(); ++k)
            {
                const auto g = grains[k];
                const auto& sg = src.grains[(size_t) k];
                bool ok = g.isArray() && g.size() == 8;
                for (int c = 0; ok && c < 8; ++c)
                    ok = isNumber (g[c]);
                ok = ok && g[0].isInt() && static_cast<int> (g[0]) >= 1 && static_cast<int> (g[0]) <= 4 && static_cast<int> (g[0]) == sg.stream
                        && g[6].isInt() && static_cast<int> (g[6]) == sg.voice && g[7].isInt() && (static_cast<int> (g[7]) == 0 || static_cast<int> (g[7]) == 1)
                        && std::abs (static_cast<double> (g[1]) - (double) sg.normPos) <= 5.0e-5 && std::abs (static_cast<double> (g[2]) - (double) sg.pan) <= 5.0e-5
                        && std::abs (static_cast<double> (g[3]) - (double) sg.env) <= 5.0e-5 && std::abs (static_cast<double> (g[4]) - (double) sg.gain) <= 5.0e-5
                        && std::abs (static_cast<double> (g[5]) - (double) sg.lengthSec) <= 5.0e-5;
                if (! ok)
                {
                    bad.add ("grain row " + juce::String (k));
                    break;
                }
            }

        const auto counts = t["counts"];
        bool cOk = counts.isArray() && counts.size() == 4;
        for (int k = 0; cOk && k < 4; ++k)
            cOk = counts[k].isInt() && static_cast<int> (counts[k]) == src.counts[(size_t) k];
        if (! cOk)
            bad.add ("counts");

        if (! (t["newest"].isInt() && static_cast<int> (t["newest"]) == -1))
            bad.add ("newest");
        for (const char* k : { "menv", "sources" })
            if (! t[k].isArray() || t[k].size() != 0)
                bad.add (juce::String (k) + " not []");
        if (! (t["limit"].isInt() && static_cast<int> (t["limit"]) == 0))
            bad.add ("limit");
        if (! (t["out"].isArray() && t["out"].size() == 2 && static_cast<double> (t["out"][0]) == 0.0 && static_cast<double> (t["out"][1]) == 0.0))
            bad.add ("out");
        if (! (t["modNorm"].getDynamicObject() != nullptr && keysOf (t["modNorm"]).isEmpty()))
            bad.add ("modNorm not {}");
        for (const char* k : { "lfoPh", "frozen", "modRouted", "stale" })
            if (t.hasProperty (k))
                bad.add (juce::String (k) + " present");

        const auto r = t["rec"];
        if (keysOf (r) != juce::StringArray { "bus", "signal", "run", "slot", "pos", "len", "capped", "peak" })
            bad.add ("rec keys " + keysOf (r).joinIntoString (","));
        if (! (r["bus"].isInt() && static_cast<int> (r["bus"]) == (src.rec.bus > 0 ? 1 : 0) && r["signal"].isBool()
               && static_cast<bool> (r["signal"]) == src.rec.signal && r["run"].isInt() && static_cast<int> (r["run"]) == src.rec.run && r["slot"].isInt()
               && static_cast<int> (r["slot"]) == src.rec.slot && isNumber (r["pos"]) && std::abs (static_cast<double> (r["pos"]) - (double) src.rec.pos) <= 5.0e-5
               && isNumber (r["len"]) && std::abs (static_cast<double> (r["len"]) - (double) src.rec.len) <= 5.0e-5 && r["capped"].isBool()
               && static_cast<bool> (r["capped"]) == src.rec.capped && r["peak"].isArray() && r["peak"].size() == 2
               && std::abs (static_cast<double> (r["peak"][0]) - (double) src.rec.peak) <= 5.0e-5
               && static_cast<double> (r["peak"][0]) == static_cast<double> (r["peak"][1])))
            bad.add ("rec values");

        // PD12: floats carry at most 4 decimals in the text.
        if (std::regex_search (text.toStdString(), std::regex ("[0-9]\\.[0-9]{5,}|[0-9][eE][-+]?[0-9]")))
            bad.add ("a number with more than 4 decimals / an exponent");
        return bad.joinIntoString ("; ");
    }

    void rowTL32()
    {
        Tally t;
        juce::String d;

        // (1) defaults: nothing loaded, no block run.
        {
            auto p = make();
            p->prepareToPlay (kFs, 512);
            const auto tel = p->buildUiTelemetry (1000.0);
            bool allNull = true;
            for (const auto& l : tel.lanes)
                allNull = allNull && ! l.hasFb && ! l.hasRec;
            const auto problems = tl32Problems (tel);
            t.check (allNull && tel.grainCount == 0 && tel.rec.bus == 1 && tel.rec.slot == 0 && problems.isEmpty(), "defaults: " + problems);
            d << "defaults: 8 nulls, 20 free rows, rec.slot 0";
        }
        // (2) grains on four streams + the feedback head on lane 1.
        {
            Session s (kFs, with (fourStreams (40.0f, 200.0f), P { { "feedback", 80.0f }, { sn (1, "reverse_prob"), 30.0f } }), kSeed, 512);
            loadNoise4 (s, 4.0, 0.2f);
            for (int n : { 48, 60 })
                s.noteOn (64, n);
            s.record = false;
            s.run (48000, 512);
            const auto tel = s.p->buildUiTelemetry (nowOf (s));
            int fbs = 0;
            for (const auto& l : tel.lanes)
                fbs += l.hasFb ? 1 : 0;
            const auto problems = tl32Problems (tel);
            t.check (tel.grainCount >= 20 && fbs == 4 && problems.isEmpty(), "live: fb lanes " + juce::String (fbs) + " " + problems);
            d << "; live: " << tel.grainCount << " grain rows of 8, counts " << tel.counts[0] << "/" << tel.counts[1] << "/" << tel.counts[2] << "/"
              << tel.counts[3] << ", fb numbers on " << fbs << " lanes";
        }
        // (3) a take in progress on lane 2.
        {
            Session s (kFs, recBase (1, 0, { { "rec_length", 2.0f } }), kSeed, 512);
            inject (s, ScSig { 0.5f, 351 });
            armAt (s, 4096);
            s.record = false;
            s.run (4096 + 38400, 512);
            const auto tel = s.p->buildUiTelemetry (nowOf (s));
            const auto problems = tl32Problems (tel);
            t.check (tel.rec.run == 2 && tel.rec.slot == 2 && tel.lanes[1].hasRec && sameF (tel.rec.pos, 0.4f) && tel.rec.signal && problems.isEmpty(),
                     "recording: run " + juce::String (tel.rec.run) + " pos " + juce::String (tel.rec.pos, 4) + " " + problems);
            d << "; recording: run " << tel.rec.run << ", slot " << tel.rec.slot << ", lanes[1].rec " << juce::String (tel.lanes[1].rec, 4) << ", len "
              << juce::String (tel.rec.len, 1) << ", signal " << (tel.rec.signal ? "true" : "false");
        }
        row (t.bad == 0, "UTL32 PD13 telemetryToVar (UiTelemetry): exact key set, fb / rec null when absent, voices 20 x 8, grains n x 8, counts x 4, 4 decimals",
             "meas " + t.detail (3) + ", " + d + " | thr 13 keys exact, values == the struct within 5e-5 | ctrl - (case 1 nulls vs case 2 / 3 numbers)");
    }

    //==========================================================================
    // INFO UPL1 / UPB1 — never fail, never counted
    void infoRows()
    {
        // UPL1: the widest frame (256 live grains + fade spares) through the builder and the encoder.
        {
            // Spread, gain_rand and reverse on: every grain row carries signed 4-decimal numbers.
            P ps = with (fullPool(), P { { "feedback", 60.0f } });
            for (int n = 1; n <= 4; ++n)
                ps = with (ps, P { { sn (n, "stereo_spread"), 100.0f }, { sn (n, "gain_rand"), 50.0f }, { sn (n, "reverse_prob"), 30.0f } });
            Session s (kFs, ps, kSeed, 256);
            s.record = false;
            loadNoise4 (s, 4.0, 0.2f);
            for (int n = 0; n < 16; ++n)
                s.noteOn (64, 40 + n);
            int widest = 0, widestGrains = 0;
            juce::String widestText;
            Tel widestTel;
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
                    widestTel = t;
                }
            };
            s.run (72000, 256);
            s.afterBlock = nullptr;

            constexpr int kIter = 300;
            double buildMs = 0.0, encodeMs = 0.0, textMs = 0.0;
            for (int k = 0; k < kIter; ++k)
            {
                s.run (256, 256);   // a fresh frame for every build
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
            std::printf ("  INFO UPL1 widest telemetry: %d bytes of one-line JSON (%d grain rows, counts %d/%d/%d/%d); per pull over %d pulls at 256 live grains: "
                         "buildUiTelemetry %.1f us, telemetryToVar %.1f us, JSON text %.1f us, total %.1f us%s\n",
                         widest, widestGrains, widestTel.counts[0], widestTel.counts[1], widestTel.counts[2], widestTel.counts[3], kIter,
                         1000.0 * buildMs / kIter, 1000.0 * encodeMs / kIter, 1000.0 * textMs / kIter, 1000.0 * total,
                         total > 1.0 ? "  ** above 1 ms per pull: report (PLAN-3.2 Risks, Payload) **" : "");
        }
        // UPB1: the frame's grain copy, old layout vs the 3.2 layout. The old
        // engine code is gone, so this re-enacts Engine::publishTelemetry's
        // grain loop in the harness for both layouts over the SAME full pool.
        {
            struct OldGrain { std::uint8_t voice = 0, stream = 0, flags = 0; float normPos = 0.0f, gain = 0.0f; };
            Session s (kFs, fullPool(), kSeed, 256);
            s.record = false;
            loadNoise4 (s, 4.0, 0.2f);
            for (int n = 0; n < 16; ++n)
                s.noteOn (64, 40 + n);
            s.run (48000, 256);
            const auto& pool = s.eng().getPool();
            const auto* base = pool.getGrains().data();
            const auto& spawner = s.eng().getSpawner();
            std::vector<OldGrain> oldF ((size_t) ogi::GiPool::kNumSlots);
            std::vector<ogi::GiUiGrain> newF ((size_t) ogi::GiPool::kNumSlots);
            constexpr int kIter = 20000;
            int active = 0;
            volatile float sink = 0.0f;
            const double a = juce::Time::getMillisecondCounterHiRes();
            for (int k = 0; k < kIter; ++k)
            {
                int count = 0;
                for (int i = 0; i < ogi::GiPool::kNumSlots; ++i)
                {
                    if (! pool.isActive (i))
                        continue;
                    const auto& vg = base[i];
                    const double frames = vg.tape != nullptr ? (double) vg.tape->frames : 0.0;
                    auto& u = oldF[(size_t) count++];
                    u.voice = (std::uint8_t) vg.voice;
                    u.stream = (std::uint8_t) vg.slot;
                    u.flags = (std::uint8_t) ((vg.g.dir < 0 ? 1 : 0) | ((i >= ogi::kPoolLive || spawner.isKilling (vg, i)) ? 2 : 0));
                    u.normPos = frames > 0.0 ? (float) (vg.g.readAbs / frames) : 0.0f;
                    u.gain = vg.g.killGain;
                }
                active = count;
                sink = sink + oldF[(size_t) (k % std::max (1, count))].normPos;
            }
            const double b = juce::Time::getMillisecondCounterHiRes();
            for (int k = 0; k < kIter; ++k)
            {
                int count = 0;
                for (int i = 0; i < ogi::GiPool::kNumSlots; ++i)
                {
                    if (! pool.isActive (i))
                        continue;
                    const auto& vg = base[i];
                    const double frames = vg.tape != nullptr ? (double) vg.tape->frames : 0.0;
                    auto& u = newF[(size_t) count++];
                    u.voice = (std::uint8_t) vg.voice;
                    u.stream = (std::uint8_t) vg.slot;
                    u.flags = (std::uint8_t) ((vg.g.dir < 0 ? 1 : 0) | ((i >= ogi::kPoolLive || spawner.isKilling (vg, i)) ? 2 : 0));
                    u.normPos = frames > 0.0 ? (float) (vg.g.readAbs / frames) : 0.0f;
                    u.gain = vg.g.killGain;
                    u.windowStep = (std::uint8_t) std::clamp (vg.g.windowStep, 0, 255);
                    u.gainL = vg.g.gainL;
                    u.gainR = vg.g.gainR;
                    u.phase = (float) (((double) vg.g.age + vg.g.frac) * vg.g.invLen);
                    u.lenSamples = (float) vg.g.len;
                    u.tiltT = vg.g.tiltT;
                    u.tiltA = vg.g.tiltA;
                    u.tiltB = vg.g.tiltB;
                }
                sink = sink + newF[(size_t) (k % std::max (1, count))].phase;
            }
            const double c = juce::Time::getMillisecondCounterHiRes();
            const double oldNs = 1.0e6 * (b - a) / kIter, newNs = 1.0e6 * (c - b) / kIter;
            const double blockNs = 1.0e9 * 256.0 / kFs;
            std::printf ("  INFO UPB1 frame grain copy at %d active slots (harness re-enactment of the publish loop; the pre-3.2 engine code is not in the tree, "
                         "so 'before' is not measured on the engine): old %d-byte grain %.0f ns per frame, 3.2 %d-byte grain %.0f ns per frame (%+.0f ns = %.4f %% of a "
                         "256-frame block at 48 k); GiUiFrame now %d bytes\n",
                         active, (int) sizeof (OldGrain), oldNs, (int) sizeof (ogi::GiUiGrain), newNs, newNs - oldNs, 100.0 * (newNs - oldNs) / blockNs,
                         (int) sizeof (ogi::GiUiFrame));
        }
        std::fflush (stdout);
    }

    //==========================================================================
    void runAll()
    {
        gTmp32 = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ogi-ug32-" + juce::String (juce::Time::currentTimeMillis()));
        gTmp32.createDirectory();
        struct Guard { ~Guard() { gTmp32.deleteRecursively(); } } guard;

        // The preset manager logs every save / load with its absolute path: silenced (as ug31).
        struct NullLogger final : juce::Logger { void logMessage (const juce::String&) override {} } nullLogger;
        struct LoggerScope
        {
            explicit LoggerScope (juce::Logger* l) : previous (juce::Logger::getCurrentLogger()) { juce::Logger::setCurrentLogger (l); }
            ~LoggerScope() { juce::Logger::setCurrentLogger (previous); }
            juce::Logger* previous;
        } loggerScope (&nullLogger);

        section ("3.2  lanes / telemetry / clear / import / save / encodedBytes (PLAN-3.2 T6)");
        std::printf ("  INFO PD24 test directories: Recordings + Imports redirected to a temp tree (non-ASCII components %s), neither under the real "
                     "~/Music/Ouaricon (%s)\n",
                     (nonAscii (harnessRecordingsDir().getFileName()) && nonAscii (harnessImportsDir().getFileName())) ? "yes" : "NO",
                     dirGuardRedirected() ? "checked at start" : "FAILED");
        if (want ("UGD"))   { rowsGD1(); rowGD2(); }
        if (want ("UCN"))   rowsCN1();
        if (want ("UPH"))   rowsPH1();
        if (want ("UFB"))   rowsFB1();
        if (want ("UOV"))   rowsOV2();
        if (want ("URC"))   { rowsRC1(); rowRC2(); }
        if (want ("UCL"))   rowCL1();
        if (want ("UPM6c")) rowsPM6c();
        if (want ("UIM"))   { rowsIM1(); rowIM2(); rowIM3(); }
        if (want ("USV"))   { rowSV1(); rowSV2(); }
        if (want ("UEB"))   rowEB1();
        if (want ("UST"))   rowST1();
        if (want ("UAL"))   rowAL1();
        if (want ("UTL32")) rowTL32();
        if (want ("UPL"))   infoRows();
    }
} }
