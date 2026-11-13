/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.6 engine gates,
    part b (PLAN-2.6 T13): <Slots> state, resolve order, relink, factory, hash,
    allocations, clicks, block size, telemetry, true bypass, the real-folder
    guard + --cpu26. Included by main.cpp AFTER engine_gates_26a.h.

      SS1   restore file / rec / factory / embedded-only + loop region + scale
      SS2   blob decode == deq (Q (pristine)) bitwise        ctrl SS2n floatWriterPath
      SS3   save -> load -> save byte-identical (also before the resolve finishes)
                                                            ctrl SS3n reencodeOnLoad
      SS4   15 s @ 192 k full-scale take: > 16 MiB -> embed="1" without `embedded`,
            resolves by path; tooLarge once
      SS5   hash > 2^63 round trip; embed / trimmed strings; malformed index / hash /
            scheme / blob / oversize ignored                ctrl SS5n parseIsBool
      SS6   identical-source restore = no swap
      SS7   empty-source restore on a sourced slot: sentinel clear, stable 2 s
      RL1-7 resolve rungs: path + size; size mismatch -> blob; Recordings exact name;
            renamed file by size + hash (path rewritten); factory; missing (sentinel,
            reference re-saved); relink match / mismatch; rung-1 hash mismatch
      RL8   1100-file Recordings dir: <= 1024 scanned, <= 16 hashed
                                                            ctrl RL8n searchUnbounded
      FA1   both stubs decode, hash == pinned, ids resolve
      FA2   factory resolve: fsTouches 0                    ctrl FA2n file: > 0
      HS1   XXH64 known answers                             ctrl HS1n 1-bit flip
      HS2   Q (deq (q)) == q for all 2^24 codes             ctrl HS2n JUCE float path
      HS3   blob-loaded hash == file-loaded hash (float32 / 16 / 24-bit WAV)
      AL26   0 audio-thread allocations across the 2.6 paths; counters live
      CK6a  commit swap into a sounding slot   } window max |d2| <= 2.0x the held pre /
      CK6b  state-load swap                    } post references   ctrl CK6n swapKillHard
            (S6 user decision 2026-11-09: raw |d2|, local kappa INFO — the swap's
            designed level dip inflates kappa's RMS denominator)
      BS26   capture start / stop / PCM bitwise at 32 / 441 / 512 / 4096 / var
                                                            ctrl BS26n SC seed + 1
      TL1   recState / progress / peaks track a take; TL2 frame <= 288, seqHead ==
            seqTail; TL3 overview bins epoch-tagged       ctrl TL1n torn frame caught
      TB7   true bypass with the SC bus + idle recorder: renders-2.4 + legacy identical
      GD1   the real ~/Music/Ouaricon/O-GranulatorInstrument/Recordings unchanged

    --cpu26 (PLAN-2.6 T13 / T17): the --cpu25a rows (C25m <= 40 %, C25tm <= 12 %)
    + C26rec INFO (C25m with a take in flight).

  ==============================================================================
*/
#pragma once

namespace { namespace eg26b
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
    using eg21::sameF;
    using eg21::diffCount;
    using eg21::hashNoise;
    using eg22::cat;
    using eg22::sn;
    using eg22::base4;
    using eg22::all4;
    using eg24b::want;
    using eg24b::section;
    using eg25a::baseMod;
    using eg26a::ScSig;
    using eg26a::inject;
    using eg26a::rec;
    using eg26a::pr;
    using eg26a::recBase;
    using eg26a::armAt;
    using eg26a::land;
    using eg26a::stName;
    using eg26a::rsName;
    using RS = ogi::RecState;
    using RR = ogi::RecReason;
    using CS = juce::AudioChannelSet;
    using SlotInfo = ogi::SlotInfo;

    constexpr double kFs = 48000.0;

    juce::File gTmp26;

    //==========================================================================
    // Helpers
    juce::File writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double rate, int bits)
    {
        f.deleteFile();
        f.getParentDirectory().createDirectory();
        auto fos = std::make_unique<juce::FileOutputStream> (f);
        if (! fos->openedOk())
            return {};
        std::unique_ptr<juce::OutputStream> os (std::move (fos));
        juce::WavAudioFormat wav;
        auto opts = juce::AudioFormatWriterOptions{}.withSampleRate (rate).withNumChannels (b.getNumChannels()).withBitsPerSample (bits);
        if (bits == 32)
            opts = opts.withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
        auto w = wav.createWriterFor (os, opts);
        if (w == nullptr)
            return {};
        const bool ok = w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        w.reset();
        return ok ? f : juce::File();
    }

    juce::MemoryBlock save (Session& s)
    {
        juce::MemoryBlock m;
        s.p->getStateInformation (m);
        return m;
    }

    void load (Session& s, const juce::MemoryBlock& m)
    {
        s.p->setStateInformation (m.getData(), (int) m.getSize());
    }

    std::unique_ptr<juce::XmlElement> xmlOf (const juce::MemoryBlock& m)
    {
        return juce::AudioProcessor::getXmlFromBinary (m.getData(), (int) m.getSize());
    }

    juce::MemoryBlock binOf (const juce::XmlElement& x)
    {
        juce::MemoryBlock m;
        juce::AudioProcessor::copyXmlToBinary (x, m);
        return m;
    }

    juce::XmlElement* slotXml (juce::XmlElement& root, int index)
    {
        if (auto* slots = root.getChildByName ("Slots"))
            for (auto* c : slots->getChildIterator())
                if (c->hasTagName ("Slot") && c->getStringAttribute ("index") == juce::String (index))
                    return c;
        return nullptr;
    }

    /** Waits for the worker, then renders a little so the engine takes the tapes. */
    void settle (Session& s, int samples = 2048)
    {
        s.p->getLoader().waitForIdle (20000);
        s.run (samples, 256);
    }

    /** A Manual take of `sec` seconds into `slot` (commitSync). */
    void recordTake (Session& s, int slot, const ScSig& g, double sec, juce::int64 gap = 256)
    {
        setP (*s.p, "rec_target", (float) slot);
        setP (*s.p, "rec_trigger", 0.0f);
        setP (*s.p, "rec_length", (float) sec);
        inject (s, g);
        const juce::int64 t = s.pos + gap;
        armAt (s, t);
        armAt (s, t + (juce::int64) (sec * s.eng().getSampleRate()) + 2048, false);
        s.run ((juce::int64) (sec * s.eng().getSampleRate()) + 4096 + gap, 512);
        s.p->commitSync();
        s.run (512, 256);
        inject (s, ScSig { 0.0f, 0 });
    }

    juce::File recFileOf (const SlotInfo& i) { return juce::File (i.source.fromFirstOccurrenceOf (":", false, false)); }

    P stBase() { return cat ({ base4 (4), { { "rec_gain", 0.0f } } }); }

    bool finite (const std::vector<float>& x, size_t from = 0)
    {
        for (size_t i = from; i < x.size(); ++i)
            if (! std::isfinite (x[i]))
                return false;
        return true;
    }

    double rms (const std::vector<float>& x, size_t from, size_t to)
    {
        double e = 0.0;
        to = std::min (to, x.size());
        for (size_t i = from; i < to; ++i)
            e += (double) x[i] * x[i];
        return to > from ? std::sqrt (e / (double) (to - from)) : 0.0;
    }

    //==========================================================================
    // ST — state
    void rowST1()
    {
        Session a (kFs, with (stBase(), { { sn (1, "loop_start"), 10.0f }, { sn (1, "loop_end"), 80.0f } }), kSeed, 512);
        a.p->getLoader().setRecordingsDirectory (gTmp26.getChildFile ("st1-rec"));
        juce::String e, bad;
        const auto f0 = writeWav (gTmp26.getChildFile ("st1-file.wav"), noiseBuf (2, 48000, 0.3f, 91), kFs, 32);
        a.p->loadSlotSync (0, f0, e);
        recordTake (a, 1, ScSig { 0.4f, 92 }, 1.0);
        recordTake (a, 3, ScSig { 0.4f, 93 }, 1.0);
        recFileOf (a.p->getSlotInfo (3)).deleteFile();   // slot 3 survives on its blob only
        a.p->setCustomScale ("st1", "edo 19", bad);
        auto x = xmlOf (save (a));
        auto* slots = x->getChildByName ("Slots");
        auto* fac = slots->createNewChildElement ("Slot");
        fac->setAttribute ("index", "2");
        fac->setAttribute ("source", "factory:stub_tone");
        fac->setAttribute ("name", "stub_tone");
        fac->setAttribute ("fileSize", "0");
        fac->setAttribute ("embed", "0");
        fac->setAttribute ("trimmed", "0");

        Session b (kFs, stBase(), kSeed, 512);
        load (b, binOf (*x));
        settle (b);
        int by[4];
        bool loaded = true;
        for (int k = 0; k < 4; ++k)
        {
            const auto i = b.p->getSlotInfo (k);
            by[k] = i.resolvedBy;
            loaded = loaded && i.loaded && b.eng().getCurrentTape (k) != nullptr;
        }
        const bool loops = juce::exactlyEqual (eg21::rawOf (*b.p, sn (1, "loop_start")), 10.0f) && juce::exactlyEqual (eg21::rawOf (*b.p, sn (1, "loop_end")), 80.0f);
        const bool scale = b.p->getCustomScale().text == "edo 19";
        b.hk().resetCounters();
        const size_t t0 = b.L.size();
        for (int n : { 48, 55, 60 })
            b.noteOn (b.pos + 64, n);
        b.run (24000, 256);
        const double r = rms (b.L, t0 + 4800, b.L.size());
        int streams[4] = { 0, 0, 0, 0 };
        for (const auto& g : b.hk().logInOrder())
            if (g.stream >= 0 && g.stream < 4) ++streams[g.stream];
        const bool audible = r > 1.0e-3 && streams[0] > 0 && streams[1] > 0 && streams[2] > 0 && streams[3] > 0;
        row (loaded && by[0] == 1 && by[1] == 1 && by[2] == 4 && by[3] == 2 && loops && scale && audible,
             "SS1 FUNC-06 restore file / rec / factory / embedded-only + loop region + scale",
             "meas resolved by rungs " + juce::String (by[0]) + "/" + juce::String (by[1]) + "/" + juce::String (by[2]) + "/" + juce::String (by[3])
                 + ", loaded " + (loaded ? "4/4" : "NO") + ", loop 10-80 " + (loops ? "yes" : "no") + ", scale " + (scale ? "edo 19" : "LOST")
                 + ", grains per stream " + juce::String (streams[0]) + "/" + juce::String (streams[1]) + "/" + juce::String (streams[2]) + "/" + juce::String (streams[3])
                 + ", RMS " + f2 (r, 4) + " | thr 1/1/4/2, all audible | ctrl -");
    }

    int blobMismatches (Session& s, int slot)
    {
        const auto blob = s.p->getLoader().getBlob (slot);
        const auto pm = s.p->getLoader().getPristine (slot);
        if (blob.isEmpty() || pm == nullptr)
            return -1;
        juce::MemoryOutputStream mo;
        juce::Base64::convertFromBase64 (mo, blob);
        ogi::SlotLoader::Pristine d;
        juce::String e;
        if (! ogi::SlotLoader::decodeMemoryFlac (mo.getData(), mo.getDataSize(), "b", kFs, d, e))
            return -2;
        if (d.pcm.getNumSamples() != pm->pcm.getNumSamples() || d.pcm.getNumChannels() != pm->pcm.getNumChannels())
            return -3;
        int bad = 0;
        for (int c = 0; c < d.pcm.getNumChannels(); ++c)
            for (int i = 0; i < d.pcm.getNumSamples(); ++i)
                bad += sameF (d.pcm.getSample (c, i), ogi::gihash::dequantise (ogi::gihash::quantise (pm->pcm.getSample (c, i)))) ? 0 : 1;
        return bad;
    }

    void rowST2()
    {
        Session s (kFs, stBase(), kSeed, 512);
        recordTake (s, 0, ScSig { 0.9f, 94 }, 1.0);
        const int n = blobMismatches (s, 0);
        s.p->getLoader().hooks.floatWriterPath.store (true);
        s.p->setSlotEmbed (0, false);
        s.p->setSlotEmbed (0, true);
        s.p->getLoader().waitForIdle (10000);
        const int nf = blobMismatches (s, 0);
        const bool fired = nf > 0;
        row (n == 0 && fired, "SS2 D36 blob decode == deq (Q (pristine)) bitwise",
             "meas " + juce::String (n) + " mismatches over " + juce::String (2 * 48000) + " samples | thr 0 | margin 0 | "
                 + ctrl (fired, "SS2n floatWriterPath (JUCE float path) -> " + juce::String (nf) + " mismatches"));
    }

    /** The state as text with the 2.2 <CustomScale version> change counter
        removed (every restore bumps it; it is not slot state). Everything else,
        <Slots> included, is compared byte for byte. */
    juce::String norm (const juce::MemoryBlock& m)
    {
        auto x = xmlOf (m);
        if (x == nullptr)
            return {};
        if (auto* cs = x->getChildByName ("CustomScale"))
            cs->removeAttribute ("version");
        return x->toString();
    }

    juce::String slotsText (const juce::MemoryBlock& m)
    {
        auto x = xmlOf (m);
        auto* sl = x != nullptr ? x->getChildByName ("Slots") : nullptr;
        return sl != nullptr ? sl->toString() : juce::String();
    }

    void rowST3()
    {
        Session a (kFs, stBase(), kSeed, 512);
        a.p->getLoader().setRecordingsDirectory (gTmp26.getChildFile ("st3-rec"));
        juce::String e;
        a.p->loadSlotSync (0, writeWav (gTmp26.getChildFile ("st3-file.wav"), noiseBuf (2, 24000, 0.3f, 95), kFs, 24), e);
        ScSig tone { 0.01f, 96 };   // tonal: LPC settings change the FLAC bytes (white noise is verbatim at every level)
        tone.sineHz = 441.0;
        tone.sineAmp = 0.4f;
        recordTake (a, 1, tone, 1.0);
        const auto sa = save (a);
        const auto na = norm (sa);

        Session b (kFs, stBase(), kSeed, 512);
        load (b, sa);
        const auto b1 = save (b);          // before the resolve finishes
        settle (b);
        const auto b2 = save (b);
        const auto b3 = [&] { Session c (kFs, stBase(), kSeed, 512); load (c, b2); settle (c); return save (c); }();

        Session n (kFs, stBase(), kSeed, 512);
        n.p->getLoader().hooks.reencodeOnLoad.store (true);
        load (n, sa);
        settle (n);
        const auto bn = save (n);
        const bool fired = slotsText (bn) != slotsText (sa);
        const bool same1 = norm (b1) == na, same2 = norm (b2) == na, same3 = norm (b3) == na;
        const bool hasBlob = slotsText (sa).contains ("embedded=");
        row (same1 && same2 && same3 && hasBlob && fired, "SS3 D36 save -> load -> save byte-identical (blob carried, never re-encoded)",
             "meas " + juce::String ((int) sa.getSize()) + " bytes (" + juce::String (slotsText (sa).length()) + " chars of <Slots>, blob "
                 + (hasBlob ? "present" : "ABSENT") + "); before resolve " + (same1 ? "==" : "!=") + ", after " + (same2 ? "==" : "!=")
                 + ", second hop " + (same3 ? "==" : "!=") + " (CustomScale version counter excluded) | thr identical | "
                 + ctrl (fired, "SS3n reencodeOnLoad (quality 8) -> <Slots> bytes differ"));
    }

    void rowST4()
    {
        constexpr double fs = 192000.0;
        Session a (fs, stBase(), kSeed, 4096);
        a.p->getLoader().setRecordingsDirectory (gTmp26.getChildFile ("st4-rec"));
        a.p->getLoader().hooks.resetCounters();
        recordTake (a, 0, ScSig { 1.0f, 97 }, 15.0);
        const auto ia = a.p->getSlotInfo (0);
        const auto encodes = a.p->getLoader().hooks.encodes.load();
        auto x = xmlOf (save (a));
        auto* sx = slotXml (*x, 0);
        const bool attrs = sx != nullptr && sx->getStringAttribute ("embed") == "1" && ! sx->hasAttribute ("embedded");
        Session b (fs, stBase(), kSeed, 4096);
        load (b, binOf (*x));
        settle (b, 8192);
        const auto ib = b.p->getSlotInfo (0);
        row (ia.tooLarge && ! ia.hasBlob && encodes == 1 && attrs && ib.loaded && ib.resolvedBy == 1,
             "SS4 D36 15 s @ 192 k full-scale take: > 16 MiB -> not embedded, resolves by path",
             juce::String ("meas tooLarge ") + (ia.tooLarge ? "yes" : "no") + ", encodes " + i64 (encodes) + ", <Slot> embed=\"1\" without embedded "
                 + (attrs ? "yes" : "no") + ", restore rung " + juce::String (ib.resolvedBy) + " | thr once, path | ctrl -");
    }

    void rowST5()
    {
        auto run = [] (bool isBoolHook, SlotInfo (&info)[4], juce::String& resaved)
        {
            Session s (kFs, stBase(), kSeed, 512);
            s.p->testParseIsBool = isBoolHook;
            auto x = xmlOf (save (s));
            auto* slots = x->getChildByName ("Slots");
            auto add = [slots] (std::initializer_list<std::pair<const char*, juce::String>> kv)
            {
                auto* c = slots->createNewChildElement ("Slot");
                for (const auto& p : kv)
                    c->setAttribute (p.first, p.second);
            };
            add ({ { "index", "0" }, { "source", "rec:/nonexistent-ogi-26/a.wav" }, { "name", "a.wav" }, { "fileSize", "123" },
                   { "contentHash", "fedcba9876543210" }, { "embed", "true" }, { "trimmed", "1" } });
            add ({ { "index", "4" }, { "source", "file:/nonexistent-ogi-26/x.wav" } });
            add ({ { "index", "x" }, { "source", "file:/nonexistent-ogi-26/x.wav" } });
            add ({ { "index", "01" }, { "source", "file:/nonexistent-ogi-26/x.wav" } });
            add ({ { "index", "1" }, { "source", "http://nonexistent/x.wav" } });
            add ({ { "index", "2" }, { "source", "rec:/nonexistent-ogi-26/b.wav" }, { "contentHash", "123" }, { "embedded", "!!!!" }, { "embed", "0" } });
            add ({ { "index", "3" }, { "source", "rec:/nonexistent-ogi-26/c.wav" }, { "fileSize", "12x" }, { "embed", "1" },
                   { "embedded", juce::String::repeatedString ("A", ogi::SlotLoader::kBlobMaxChars + 4) } });
            load (s, binOf (*x));
            settle (s);
            for (int k = 0; k < 4; ++k)
                info[k] = s.p->getSlotInfo (k);
            auto y = xmlOf (save (s));
            resaved = slotXml (*y, 0) != nullptr ? slotXml (*y, 0)->toString (juce::XmlElement::TextFormat().singleLine().withoutHeader()) : juce::String();
        };
        SlotInfo i[4], j[4];
        juce::String r0, r1;
        run (false, i, r0);
        run (true, j, r1);
        const bool s0 = i[0].missing && i[0].hasHash && i[0].contentHash == 0xfedcba9876543210ull && i[0].embed && i[0].trimmed && i[0].fileSize == 123;
        const bool round = r0.contains ("contentHash=\"fedcba9876543210\"") && r0.contains ("fileSize=\"123\"");
        const bool s1 = i[1].source.isEmpty();                              // unknown scheme (and index 4 / x / 01) ignored
        const bool s2 = i[2].missing && ! i[2].hasHash && ! i[2].hasBlob && ! i[2].embed;
        const bool s3 = i[3].missing && ! i[3].hasBlob && i[3].fileSize == 0 && i[3].embed;
        const bool fired = ! j[0].embed;   // isBool on an XML string → the default
        row (s0 && round && s1 && s2 && s3 && fired, "SS5 P5 / P6 hash > 2^63 round trip, string bools, malformed attributes ignored",
             juce::String ("meas slot 0 hash ") + ogi::gihash::toHex (i[0].contentHash) + " re-saved " + (round ? "==" : "!=") + ", embed/trimmed "
                 + (i[0].embed && i[0].trimmed ? "1/1" : "BAD") + "; index 4 / x / 01 / http: ignored " + (s1 ? "yes" : "no") + "; bad hash / blob "
                 + (s2 ? "ignored" : "ACCEPTED") + "; oversize blob + fileSize 12x " + (s3 ? "ignored" : "ACCEPTED") + " | "
                 + ctrl (fired, "SS5n parseIsBool -> embed restored as the default (off)"));
    }

    void rowsST67()
    {
        juce::String e;
        const auto f = writeWav (gTmp26.getChildFile ("st6-file.wav"), noiseBuf (2, 48000, 0.3f, 98), kFs, 32);
        {
            Session s (kFs, stBase(), kSeed, 512);
            s.p->loadSlotSync (0, f, e);
            s.run (1024, 256);
            const auto* t1 = s.eng().getCurrentTape (0);
            const auto st = save (s);
            s.p->getLoader().hooks.resetCounters();
            load (s, st);
            settle (s);
            const auto* t2 = s.eng().getCurrentTape (0);
            const auto res = s.p->getLoader().hooks.resolves.load();
            row (t1 != nullptr && t1 == t2 && res == 0, "SS6 PD12 identical-source restore = no swap, no job",
                 juce::String ("meas tape pointer ") + (t1 == t2 ? "same" : "CHANGED") + ", resolve jobs " + i64 (res) + " | thr same, 0 | ctrl -");
        }
        {
            Session s (kFs, stBase(), kSeed, 512);
            s.p->loadSlotSync (0, f, e);
            for (int n : { 48, 55, 60 })
                s.noteOn (64, n);
            s.run (24000, 256);
            const auto clears0 = s.p->getHandoff().getClearsPublished();
            Session empty (kFs, stBase(), kSeed, 512);
            const auto st = save (empty);   // no sources
            load (s, st);
            const size_t t0 = s.L.size();
            s.run ((juce::int64) (2 * kFs), 256);
            const bool gone = s.eng().getCurrentTape (0) == nullptr && ! s.p->getSlotInfo (0).loaded && s.p->getSlotInfo (0).source.isEmpty();
            const double tail = rms (s.L, s.L.size() - 4800, s.L.size());
            row (gone && s.p->getHandoff().getClearsPublished() == clears0 + 1 && finite (s.L, t0) && finite (s.R, t0) && tail == 0.0,
                 "SS7 PD11 empty-source restore on a sourced slot: sentinel clear, stable 2 s",
                 juce::String ("meas slot empty ") + (gone ? "yes" : "no") + ", clears " + i64 (s.p->getHandoff().getClearsPublished() - clears0)
                     + ", finite yes, tail RMS " + f2 (tail, 6) + " | thr empty, 1 clear, silent | ctrl -");
        }
    }

    //==========================================================================
    // RS — resolve rungs
    void rowsRS()
    {
        juce::String e;
        // RL1: path + size.
        {
            Session a (kFs, stBase(), kSeed, 512);
            a.p->loadSlotSync (0, writeWav (gTmp26.getChildFile ("rs1.wav"), noiseBuf (2, 24000, 0.3f, 101), kFs, 24), e);
            Session b (kFs, stBase(), kSeed, 512);
            load (b, save (a));
            settle (b);
            const auto i = b.p->getSlotInfo (0);
            row (i.loaded && i.resolvedBy == 1 && ! i.hashMismatch, "RL1 rung 1: path exists + size matches",
                 "meas rung " + juce::String (i.resolvedBy) + ", hashMismatch " + (i.hashMismatch ? "yes" : "no") + " | thr 1 | ctrl RL2");
        }
        // RL2 / RL3 / RL4 / RL7: a recording in its own Recordings dir.
        const auto dir = gTmp26.getChildFile ("rs-rec");
        auto take = [&] (int salt, juce::MemoryBlock& st, SlotInfo& info)
        {
            Session a (kFs, stBase(), kSeed, 512);
            a.p->getLoader().setRecordingsDirectory (dir);
            recordTake (a, 0, ScSig { 0.4f, salt }, 1.0);
            st = save (a);
            info = a.p->getSlotInfo (0);
        };
        auto restore = [&] (const juce::MemoryBlock& st, std::function<void (Session&)> before = {}) -> std::unique_ptr<Session>
        {
            auto b = std::make_unique<Session> (kFs, stBase(), kSeed, 512);
            b->p->getLoader().setRecordingsDirectory (dir);
            if (before)
                before (*b);
            load (*b, st);
            settle (*b);
            return b;
        };
        {
            juce::MemoryBlock st;
            SlotInfo ia;
            take (102, st, ia);
            { juce::FileOutputStream os (recFileOf (ia)); os.setPosition (recFileOf (ia).getSize()); os.writeInt (0); }   // the size moves
            auto b = restore (st);
            const auto i = b->p->getSlotInfo (0);
            row (i.loaded && i.resolvedBy == 2 && i.source == ia.source, "RL2 rung 2: size mismatch -> the embedded blob (reference kept)",
                 "meas rung " + juce::String (i.resolvedBy) + ", source kept " + (i.source == ia.source ? "yes" : "no") + " | thr 2 | ctrl -");
        }
        {
            juce::MemoryBlock st;
            SlotInfo ia;
            take (103, st, ia);
            auto x = xmlOf (st);
            auto* sx = slotXml (*x, 0);
            sx->setAttribute ("source", "rec:/nonexistent-ogi-26/moved/" + recFileOf (ia).getFileName());
            sx->removeAttribute ("embedded");
            auto b = restore (binOf (*x));
            const auto i = b->p->getSlotInfo (0);
            row (i.loaded && i.resolvedBy == 3 && i.relocated && i.source == ia.source, "RL3 rung 3: Recordings/<filename> (path rewritten, relocated)",
                 "meas rung " + juce::String (i.resolvedBy) + ", relocated " + (i.relocated ? "yes" : "no") + ", path -> the Recordings file "
                     + (i.source == ia.source ? "yes" : "no") + " | thr 3 | ctrl -");
        }
        {
            juce::MemoryBlock st;
            SlotInfo ia;
            take (104, st, ia);
            const auto renamed = dir.getChildFile ("renamed-by-user.wav");
            recFileOf (ia).moveFileTo (renamed);
            auto x = xmlOf (st);
            slotXml (*x, 0)->removeAttribute ("embedded");
            auto b = restore (binOf (*x));
            const auto i = b->p->getSlotInfo (0);
            row (i.loaded && i.resolvedBy == 3 && i.relocated && recFileOf (i) == renamed, "RL4 rung 3: a renamed file found by size + hash",
                 "meas rung " + juce::String (i.resolvedBy) + ", path -> " + recFileOf (i).getFileName() + " | thr 3, renamed-by-user.wav | ctrl -");
        }
        // RL5: factory.
        {
            Session s (kFs, stBase(), kSeed, 512);
            auto x = xmlOf (save (s));
            auto* c = x->getChildByName ("Slots")->createNewChildElement ("Slot");
            c->setAttribute ("index", "1");
            c->setAttribute ("source", "factory:stub_noise");
            load (s, binOf (*x));
            settle (s);
            const auto i = s.p->getSlotInfo (1);
            row (i.loaded && i.resolvedBy == 4 && i.contentHash == ogi::kHashNoise && ! i.embed, "RL5 rung 4: factory:stub_noise",
                 "meas rung " + juce::String (i.resolvedBy) + ", hash " + ogi::gihash::toHex (i.contentHash) + " | thr 4, pinned | ctrl -");
        }
        // RL6: missing on a loaded slot.
        {
            Session s (kFs, stBase(), kSeed, 512);
            s.p->loadSlotSync (0, writeWav (gTmp26.getChildFile ("rs6.wav"), noiseBuf (2, 24000, 0.3f, 105), kFs, 32), e);
            for (int n : { 48, 60 })
                s.noteOn (64, n);
            s.run (12000, 256);
            auto x = xmlOf (save (s));
            auto* sx = slotXml (*x, 0);
            sx->setAttribute ("source", "rec:/nonexistent-ogi-26/gone.wav");
            sx->setAttribute ("contentHash", "0123456789abcdef");
            const auto want = sx->toString (juce::XmlElement::TextFormat().singleLine().withoutHeader());
            load (s, binOf (*x));
            settle (s);
            const size_t t0 = s.L.size();
            s.run ((juce::int64) (2 * kFs), 256);
            const auto i = s.p->getSlotInfo (0);
            auto y = xmlOf (save (s));
            const auto got = slotXml (*y, 0) != nullptr ? slotXml (*y, 0)->toString (juce::XmlElement::TextFormat().singleLine().withoutHeader()) : juce::String();
            row (i.missing && ! i.loaded && i.resolvedBy == 5 && s.eng().getCurrentTape (0) == nullptr && finite (s.L, t0) && got == want,
                 "RL6 rung 5: missing -> sentinel clear, stable 2 s, reference re-saved unchanged",
                 juce::String ("meas missing ") + (i.missing ? "yes" : "no") + ", tape cleared " + (s.eng().getCurrentTape (0) == nullptr ? "yes" : "no")
                     + ", finite yes, <Slot> re-saved " + (got == want ? "identical" : "CHANGED") + " | ctrl -");
        }
        // RL7: relink match / mismatch; rung-1 hash mismatch.
        {
            juce::MemoryBlock st;
            SlotInfo ia;
            take (106, st, ia);
            const auto keep = gTmp26.getChildFile ("relink-copy.wav");
            recFileOf (ia).copyFileTo (keep);
            recFileOf (ia).deleteFile();
            auto b = restore (st);   // via the blob
            const auto blob0 = b->p->getLoader().getBlob (0);
            b->p->requestRelink (0, keep);
            settle (*b);
            const auto im = b->p->getSlotInfo (0);
            const bool match = im.loaded && im.source == "rec:" + keep.getFullPathName() && im.embed && b->p->getLoader().getBlob (0) == blob0
                            && ! im.relinkMismatch && blob0.isNotEmpty();
            const auto other = writeWav (gTmp26.getChildFile ("relink-other.wav"), noiseBuf (2, 24000, 0.3f, 107), kFs, 32);
            b->p->requestRelink (0, other);
            settle (*b);
            const auto ix = b->p->getSlotInfo (0);
            const bool mism = ix.loaded && ix.source == "file:" + other.getFullPathName() && ! ix.embed && ix.relinkMismatch && ! ix.hasBlob;

            // Rung 1 with a same-size, different-content file.
            juce::MemoryBlock st2;
            SlotInfo ib;
            take (108, st2, ib);
            {
                juce::AudioBuffer<float> alt = noiseBuf (2, 48000, 0.4f, 109);
                writeWav (recFileOf (ib), alt, kFs, 32);
            }
            auto c = restore (st2);
            const auto ih = c->p->getSlotInfo (0);
            row (match && mism && ih.loaded && ih.resolvedBy == 1 && ih.hashMismatch,
                 "RL7 relink match keeps scheme / embed / blob; mismatch -> file:, embed off, flagged; rung-1 hash mismatch flagged",
                 juce::String ("meas match ") + (match ? "ok" : "BAD") + ", mismatch " + (mism ? "ok" : "BAD") + ", rung-1 edited file: rung "
                     + juce::String (ih.resolvedBy) + " hashMismatch " + (ih.hashMismatch ? "yes" : "no") + " | ctrl -");
        }
        // RL8: the bounded Recordings search.
        {
            const auto big = gTmp26.getChildFile ("rs8-rec");
            big.createDirectory();
            const auto small = noiseBuf (1, 480, 0.1f, 110);
            juce::File first;
            for (int k = 0; k < 1100; ++k)
            {
                juce::AudioBuffer<float> b (1, 480);
                b.copyFrom (0, 0, small, 0, 0, 480);
                b.setSample (0, 0, (float) k * 1.0e-4f);   // every file distinct, same size
                const auto f = writeWav (big.getChildFile ("f" + juce::String (k).paddedLeft ('0', 4) + ".wav"), b, kFs, 32);
                if (k == 0)
                    first = f;
            }
            auto search = [&] (bool unbounded, std::uint64_t& scanned, std::uint64_t& hashed)
            {
                Session s (kFs, stBase(), kSeed, 512);
                s.p->getLoader().setRecordingsDirectory (big);
                s.p->getLoader().hooks.searchUnbounded.store (unbounded);
                s.p->getLoader().hooks.resetCounters();
                auto x = xmlOf (save (s));
                auto* c = x->getChildByName ("Slots")->createNewChildElement ("Slot");
                c->setAttribute ("index", "0");
                c->setAttribute ("source", "rec:/nonexistent-ogi-26/lost.wav");
                c->setAttribute ("fileSize", juce::String (first.getSize()));
                c->setAttribute ("contentHash", "0123456789abcdef");
                load (s, binOf (*x));
                settle (s);
                scanned = s.p->getLoader().hooks.dirEntriesScanned.load();
                hashed = s.p->getLoader().hooks.candidatesHashed.load();
                return s.p->getSlotInfo (0).missing;
            };
            std::uint64_t sc = 0, h = 0, scN = 0, hN = 0;
            const bool miss = search (false, sc, h);
            search (true, scN, hN);
            const bool fired = scN > 1024;
            row (miss && sc <= 1024 && h <= 16 && fired, "RL8 PD13 Recordings search bounded: <= 1024 entries, <= 16 hashed",
                 "meas scanned " + i64 (sc) + ", hashed " + i64 (h) + " (1100 same-size files) | thr <= 1024 / <= 16 | "
                     + ctrl (fired, "RL8n searchUnbounded -> scanned " + i64 (scN) + ", hashed " + i64 (hN)));
        }
    }

    //==========================================================================
    // FA — factory
    void rowsFA()
    {
        // PLAN-4 PD7 (planned amendment; row name unchanged): the stubs are
        // harness-only fixtures appended after the 8 curated sources, so FA1
        // checks that the two stub ids resolve with their carried pins (it was
        // factoryCount () == 2). The curated sources are FSP1's.
        juce::String d;
        bool ok = true;
        for (const char* id : { "stub_tone", "stub_noise" })
        {
            ogi::FactoryEntry e;
            const bool found = ogi::findFactory (juce::String (id), e) && e.data != nullptr && juce::String (e.id) == id;
            ogi::SlotLoader::Pristine p;
            juce::String err;
            const bool dec = ogi::SlotLoader::decodeMemoryFlac (e.data, e.size, e.id, kFs, p, err);
            const auto h = dec ? ogi::gihash::contentHash (p.pcm) : 0;
            ok = ok && found && dec && h == e.hash && e.hash != 0;
            d << " " << e.id << ":" << (int) e.size << "B/" << p.pcm.getNumChannels() << "ch/" << ogi::gihash::toHex (h);
        }
        row (ok, "FA1 D33 factory stubs decode, hash == pinned, ids resolve", "meas" + d + " | thr kHashTone / kHashNoise | ctrl -");

        auto touches = [] (const juce::String& source)
        {
            Session s (kFs, stBase(), kSeed, 512);
            auto x = xmlOf (save (s));
            auto* c = x->getChildByName ("Slots")->createNewChildElement ("Slot");
            c->setAttribute ("index", "0");
            c->setAttribute ("source", source);
            s.p->getLoader().hooks.resetCounters();
            load (s, binOf (*x));
            settle (s);
            return std::make_pair (s.p->getLoader().hooks.fsTouches.load(), s.p->getSlotInfo (0).loaded);
        };
        const auto fac = touches ("factory:stub_tone");
        juce::String e;
        const auto f = writeWav (gTmp26.getChildFile ("fa2.wav"), noiseBuf (2, 24000, 0.3f, 111), kFs, 32);
        const auto fil = touches ("file:" + f.getFullPathName());
        const bool fired = fil.first > 0 && fil.second;
        row (fac.first == 0 && fac.second && fired, "FA2 D33 factory: resolve decodes from memory, no disk access",
             "meas fsTouches " + i64 (fac.first) + " | thr 0 | " + ctrl (fired, "FA2n file: resolve -> fsTouches " + i64 (fil.first)));
    }

    //==========================================================================
    // HS — hash / quantiser
    void rowsHS()
    {
        using ogi::gihash::xxh64;
        const char* s36 = "abcdefghijklmnopqrstuvwxyz0123456789";
        // Vectors cross-checked against the reference xxhash library (PyPI xxhash 4.1.0,
        // 2026-11-09); the 36- and 62-byte strings cover the 32-byte stripe path.
        const char* s62 = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        const bool ka = xxh64 ("", 0) == 0xef46db3751d8e999ull && xxh64 ("foo", 3) == 0x33bf00a859c4ba3full
                     && xxh64 ("bar", 3) == 0x48a37c90ad27a659ull && xxh64 (s36, 36) == 0x64f23ecf1609b766ull
                     && xxh64 (s62, 62) == 0x7639d419de614eedull;
        char flip[37];
        std::memcpy (flip, s36, 37);
        flip[5] ^= 1;
        const bool fired = xxh64 (flip, 36) != 0x64f23ecf1609b766ull;
        row (ka && fired, "HS1 PD1 XXH64 known answers (\"\", foo, bar, 36- / 62-char)",
             juce::String ("meas ") + ogi::gihash::toHex (xxh64 ("", 0)) + " " + ogi::gihash::toHex (xxh64 ("foo", 3)) + " " + ogi::gihash::toHex (xxh64 ("bar", 3))
                 + " " + ogi::gihash::toHex (xxh64 (s36, 36)) + (ka ? " match" : " MISMATCH") + " | thr exact | " + ctrl (fired, "HS1n 1-bit flip -> differs"));

        int bad = 0;
        for (int q = -8388608; q <= 8388607; ++q)
            bad += ogi::gihash::quantise (ogi::gihash::dequantise (q)) == q ? 0 : 1;
        // HS2n: JUCE's float writer path on the same codes (every 12th code: under the 30 s decode cap).
        int badF = 0;
        {
            ogi::SlotLoader::Pristine p;
            p.rate = kFs;
            const int n = (1 << 24) / 12;
            p.pcm.setSize (1, n);
            for (int i = 0; i < n; ++i)
                p.pcm.setSample (0, i, ogi::gihash::dequantise (-8388608 + 12 * i));
            juce::MemoryBlock mb;
            ogi::SlotLoader::encodeFlac24 (p, mb, 0, true);
            ogi::SlotLoader::Pristine d;
            juce::String e;
            if (ogi::SlotLoader::decodeMemoryFlac (mb.getData(), mb.getSize(), "h", kFs, d, e))
                for (int i = 0; i < std::min (n, d.pcm.getNumSamples()); ++i)
                    badF += sameF (d.pcm.getSample (0, i), p.pcm.getSample (0, i)) ? 0 : 1;
        }
        row (bad == 0 && badF > 0, "HS2 PD3 Q (deq (q)) == q for all 2^24 codes",
             "meas " + juce::String (bad) + " mismatches | thr 0 | " + ctrl (badF > 0, "HS2n JUCE float writer path -> " + juce::String (badF) + " of "
                 + juce::String ((1 << 24) / 12) + " sampled codes move"));

        juce::String d;
        bool all = true;
        const auto sig = noiseBuf (2, 48000, 0.6f, 112);
        for (int bits : { 32, 16, 24 })
        {
            Session s (kFs, stBase(), kSeed, 512);
            juce::String e;
            const auto f = writeWav (gTmp26.getChildFile ("hs3-" + juce::String (bits) + ".wav"), sig, kFs, bits);
            s.p->loadSlotSync (0, f, e);
            const auto hf = s.p->getSlotInfo (0).contentHash;
            s.p->setSlotEmbed (0, true);
            s.p->getLoader().waitForIdle (10000);
            const auto blob = s.p->getLoader().getBlob (0);
            juce::MemoryOutputStream mo;
            juce::Base64::convertFromBase64 (mo, blob);
            ogi::SlotLoader::Pristine p;
            const bool ok = ogi::SlotLoader::decodeMemoryFlac (mo.getData(), mo.getDataSize(), "x", kFs, p, e);
            const auto hb = ok ? ogi::gihash::contentHash (p.pcm) : 0;
            all = all && ok && hb == hf;
            d << " " << bits << "-bit " << (hb == hf ? "==" : "!=");
        }
        row (all, "HS3 PD2 blob-loaded hash == file-loaded hash (float32 / 16-bit / 24-bit WAV)", "meas" + d + " | thr equal | ctrl -");
    }

    //==========================================================================
    // AL — allocations across the 2.6 paths
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

        Session s (kFs, cat ({ stBase(), { { "rec_threshold", -30.0f } } }), kSeed, 512);
        s.p->getLoader().setRecordingsDirectory (gTmp26.getChildFile ("al1-rec"));
        s.p->getLoader().hooks.resetCounters();
        s.load (0, noiseBuf (2, 48000, 0.3f, 113), kFs);
        s.arm = true;
        for (int n : { 48, 55, 60 })
            s.noteOn (64, n);
        // Threshold + pre-roll → record → commit (copy-out off-audio) → swap.
        setP (*s.p, "rec_trigger", 1.0f);
        setP (*s.p, "rec_length", 0.6f);
        ScSig g { 0.5f, 114 };
        g.from = 20000;
        inject (s, g);
        armAt (s, 8000);
        s.run (60000, 441);
        s.p->commitSync();
        s.run (8000, 441);
        // Discard (silent Manual), cancel (Threshold, silent).
        inject (s, ScSig { 0.0f, 0 });
        setP (*s.p, "rec_trigger", 0.0f);
        armAt (s, s.pos + 100, false);
        armAt (s, s.pos + 200);
        armAt (s, s.pos + 40000, false);
        s.run (42000, 441);
        setP (*s.p, "rec_trigger", 1.0f);
        armAt (s, s.pos + 200);
        armAt (s, s.pos + 4000, false);
        s.run (6000, 441);
        // State save / load (embedded), a missing resolve, a factory load.
        const auto st = save (s);
        load (s, st);
        settle (s, 4096);
        auto x = xmlOf (st);
        if (auto* sx = slotXml (*x, 0))
        {
            sx->setAttribute ("source", "rec:/nonexistent-ogi-26/al1.wav");
            sx->setAttribute ("contentHash", "0123456789abcdef");   // no rung-3 match either: a true miss
            sx->removeAttribute ("embedded");
        }
        auto* fc = x->getChildByName ("Slots")->createNewChildElement ("Slot");
        fc->setAttribute ("index", "2");
        fc->setAttribute ("source", "factory:stub_tone");
        load (s, binOf (*x));
        settle (s, 4096);
        // Prepare mid-record, then bypassed blocks while Recording.
        inject (s, ScSig { 0.5f, 115 });
        setP (*s.p, "rec_trigger", 0.0f);
        setP (*s.p, "rec_length", 5.0f);
        armAt (s, s.pos + 100, false);
        armAt (s, s.pos + 200);
        s.run (30000, 441);
        s.arm = false;
        s.p->prepareToPlay (kFs, 512);
        s.p->commitSync();
        s.arm = true;
        armAt (s, s.pos + 100, false);
        armAt (s, s.pos + 200);
        s.run (30000, 441);
        {
            juce::AudioBuffer<float> b (2, 441);
            juce::MidiBuffer mb;
            for (int k = 0; k < 8; ++k)
            {
                b.clear();
                allocArmed = true;
                s.p->processBlockBypassed (b, mb);
                allocArmed = false;
            }
        }
        s.run (4000, 441);
        s.arm = false;
        malloc_logger = nullptr;
        const int allocs = allocCount;

        const auto& rh = rec (s).hooks;
        const auto& lh = s.p->getLoader().hooks;
        const bool live = rh.recArms > 0 && rh.recStarts > 0 && rh.recCommits > 0 && rh.recDiscards > 0 && rh.recCancels > 0
                       && lh.commitsPublished.load() > 0 && lh.resolveRung[5].load() > 0 && lh.resolveRung[4].load() > 0
                       && s.p->getHandoff().getClearsPublished() > 0 && rh.interrupted + rh.recCommits > 1;
        row (allocs == 0 && liveness > 0 && live, "AL26 PERF-01 0 audio-thread allocations across the 2.6 paths",
             "meas " + juce::String (allocs) + " allocs (arms " + i64 (rh.recArms) + ", starts " + i64 (rh.recStarts) + ", commits " + i64 (rh.recCommits)
                 + ", discards " + i64 (rh.recDiscards) + ", cancels " + i64 (rh.recCancels) + ", published " + i64 (lh.commitsPublished.load())
                 + ", missing " + i64 (lh.resolveRung[5].load()) + ", factory " + i64 (lh.resolveRung[4].load()) + ", clears "
                 + i64 (s.p->getHandoff().getClearsPublished()) + ") | thr 0 | " + ctrl (liveness > 0, "liveness malloc(64) armed -> " + juce::String (liveness)));
       #else
        row (true, "AL26 (mac only)", "skipped");
       #endif
    }

    //==========================================================================
    // CK — commit / state-load swap clicks (local kappa vs held references)
    eg25d::Out ckRender (int mode, bool hard)
    {
        // mode 0: subject (A then the commit of B at 1.0 s), 1: A only, 2: B only,
        // 3: subject via a state load (A then B restored at 1.0 s).
        Session s (kFs, cat ({ baseMod (1), { { sn (1, "density"), 60.0f }, { sn (1, "size"), 60.0f } } }), kSeed, 512);
        s.p->getLoader().setRecordingsDirectory (gTmp26.getChildFile ("ck-rec"));
        s.hk().swapKillHard = hard;
        const auto A = sineBuf (2, 96000, 220.0, kFs, 0.3);
        const auto Bf = gTmp26.getChildFile ("ck-B.wav");
        if (! Bf.existsAsFile())
            writeWav (Bf, sineBuf (2, 96000, 330.0, kFs, 0.3), kFs, 32);
        juce::String e;
        if (mode == 2)
            s.p->loadSlotSync (0, Bf, e);
        else
            s.load (0, A, kFs);
        for (int n : { 48, 55 })
            s.noteOn (64, n);
        const juce::int64 T = 48000;
        if (mode == 0)
        {
            // B arrives by a commit: the take is the 330 Hz sine on the sidechain.
            setP (*s.p, "rec_length", 0.6f);
            ScSig g { 0.0f, 0 };
            g.sineHz = 330.0;
            g.sineAmp = 0.3f;
            inject (s, g);
            armAt (s, 4000);
            s.at (T, [] (Session& ss) { ss.p->commitSync(); });
        }
        if (mode == 3)
            s.at (T, [Bf] (Session& ss)
            {
                auto x = xmlOf (save (ss));
                auto* c = x->getChildByName ("Slots")->createNewChildElement ("Slot");
                c->setAttribute ("index", "0");
                c->setAttribute ("source", "file:" + Bf.getFullPathName());
                load (ss, binOf (*x));
                ss.p->getLoader().waitForIdle (10000);
            });
        s.run (96000, 256);
        return { s.L, s.R };
    }

    /** Window max |d2| over L and R, [t0, t1) seconds. */
    double d2Max (const eg25d::Out& o, double t0, double t1)
    {
        const size_t a = (size_t) (t0 * kFs), b = (size_t) (t1 * kFs);
        return std::max (eg24b::maxCurv (o.L, a, b), eg24b::maxCurv (o.R, a, b));
    }

    void rowsCK()
    {
        // S6 user decision 2026-11-09: CK6a / CK6b are judged on the RAW window
        // max |d2| against the held pre (A only) / post (B only) references. The
        // 2.1 swap rule (5 ms kill of the old tape's grains; new grains fade in
        // through their windows) leaves a brief level dip by design, which
        // shrinks local kappa's frame-RMS denominator (3.6x / 3.8x) without
        // any extra curvature. Local kappa is reported as INFO.
        const auto a = ckRender (1, false), b = ckRender (2, false);
        const auto s0 = ckRender (0, false), s3 = ckRender (3, false), sh = ckRender (0, true);
        const double dRef = std::max (d2Max (a, 0.9, 1.3), d2Max (b, 0.9, 1.3));
        const double r0 = d2Max (s0, 0.9, 1.3) / dRef, r3 = d2Max (s3, 0.9, 1.3) / dRef, rh = d2Max (sh, 0.9, 1.3) / dRef;
        const double kRef = std::max (eg25d::kappa (a, 0.9, 1.3), eg25d::kappa (b, 0.9, 1.3));
        const double k0 = eg25d::kappa (s0, 0.9, 1.3) / kRef, k3 = eg25d::kappa (s3, 0.9, 1.3) / kRef, kh = eg25d::kappa (sh, 0.9, 1.3) / kRef;
        const bool fired = rh > 2.0;
        row (r0 <= 2.0 && fired, "CK6a QUAL-01 commit swap into a sounding slot (A 220 -> B 330 Hz): max |d2| <= 2.0x held refs",
             "meas |d2| ratio " + f2 (r0, 3) + " (held ref max " + f2 (dRef, 5) + ") | INFO local kappa " + f2 (k0, 2) + "x (level dip, S6 decision) | thr <= 2.0 | margin "
                 + f2 (2.0 - r0, 3) + " | " + ctrl (fired, "CK6n swapKillHard -> |d2| " + f2 (rh, 1) + "x (kappa " + f2 (kh, 0) + "x)"));
        row (r3 <= 2.0 && fired, "CK6b QUAL-01 state-load swap (A -> file B): max |d2| <= 2.0x held refs",
             "meas |d2| ratio " + f2 (r3, 3) + " | INFO local kappa " + f2 (k3, 2) + "x | thr <= 2.0 | margin " + f2 (2.0 - r3, 3) + " | ctrl CK6n (above)");
    }

    //==========================================================================
    // BS — capture block-size null
    struct Cap { juce::int64 start = -1, stop = -1; std::vector<float> pcm; };

    Cap bsCapture (int trig, int block, int salt, bool tempo)
    {
        Session s (kFs, recBase (0, trig, { { "rec_length", 1.0f }, { "sync_mode", tempo ? 1.0f : 0.0f }, { "rec_length_sync", 0.0f },
                                            { sn (1, "density"), 60.0f } }), kSeed, 512);
        s.load (1, noiseBuf (2, 48000, 0.2f, 116), kFs);
        std::shared_ptr<MockPlayHead> ph;
        ScSig g { 0.5f, salt };
        if (trig == 1)
            g.from = 12345;
        if (tempo)
        {
            ph = std::make_shared<MockPlayHead>();
            ph->sampleRate = kFs;
            MockPlayHead::Seg a;
            a.at = 0; a.bpm = 120.0; a.playing = true;
            ph->script.push_back (a);
            s.p->setPlayHead (ph.get());
            inject (s, g, -1, [ph] (Session&, juce::int64 b0, int) { ph->timeInSamples = b0; });
        }
        else
        {
            inject (s, g);
        }
        for (int n : { 48, 60 })
            s.noteOn (777, n);
        armAt (s, 4807);
        if (trig == 2)
            s.noteOn (9001, 64);
        s.run (4807 + 96000 + 20000, block);
        Cap c;
        c.start = pr (s).start;
        c.stop = pr (s).stop;
        s.p->commitSync();
        if (auto p = s.p->getLoader().getPristine (0))
            for (int ch = 0; ch < p->pcm.getNumChannels(); ++ch)
                c.pcm.insert (c.pcm.end(), p->pcm.getReadPointer (ch), p->pcm.getReadPointer (ch) + p->pcm.getNumSamples());
        s.p->setPlayHead (nullptr);
        return c;
    }

    void rowBS1()
    {
        Tally t;
        juce::String d;
        for (int trig = 0; trig < 3; ++trig)
            for (bool tempo : { false, true })
            {
                const auto ref = bsCapture (trig, 32, 117, tempo);
                for (int b : { 441, 512, 4096, 0 })
                {
                    const auto c = bsCapture (trig, b, 117, tempo);
                    const int n = diffCount (ref.pcm, c.pcm);
                    t.check (n == 0 && c.start == ref.start && c.stop == ref.stop && ! ref.pcm.empty(),
                             "trig " + juce::String (trig) + (tempo ? " tempo" : "") + " block " + juce::String (b) + ": " + juce::String (n));
                }
                d << " t" << trig << (tempo ? "T" : "") << ":" << (juce::int64) ref.start << "-" << (juce::int64) ref.stop;
            }
        const int nN = diffCount (bsCapture (0, 32, 117, false).pcm, bsCapture (0, 32, 118, false).pcm);
        const bool fired = nN > 0;
        row (t.bad == 0 && fired, "BS26 PERF-03 capture start / stop / PCM bitwise at 32 / 441 / 512 / 4096 / var (3 triggers, +Tempo)",
             "meas " + t.detail (24) + " |" + d + " | thr 0 diffs | " + ctrl (fired, "BS26n SC seed + 1 -> " + juce::String (nN) + " differ"));
    }

    //==========================================================================
    // TL — telemetry feeds
    void rowsTL()
    {
        Session s (kFs, cat ({ stBase(), { { "rec_length", 1.0f }, { "rec_trigger", 0.0f } } }), kSeed, 512);
        s.load (0, noiseBuf (2, 48000, 0.3f, 119), kFs);
        inject (s, ScSig { 0.5f, 120 });
        armAt (s, 1000);
        s.run (1000 + 24000, 512);
        const auto st1 = s.p->getRecordState();
        const int f1 = rec (s).getFramesTelemetry(), tgt = rec (s).getTargetTelemetry();
        const float pk = s.eng().getTelemetry().takeScInPeak();
        const float pk2 = s.eng().getTelemetry().takeScInPeak();   // taken: reads 0 until the next block
        s.run (30000, 512);
        const auto st2 = s.p->getRecordState();
        s.p->commitSync();
        s.run (512, 512);
        const auto st3 = s.p->getRecordState();
        const bool tl1 = (st1 & 0xff) == 2 && f1 > 20000 && f1 < 26000 && tgt == 48000 && pk > 0.4f && pk2 == 0.0f
                      && (st2 & 0xff) == 3 && ((st2 >> 8) & 0xff) == (int) RR::Committed && (st3 & 0xff) == 0;
        // TL1n: a deliberately torn frame (seqHead != seqTail) is caught by the reader check.
        ogi::GiUiFrame torn;
        torn.seqHead = 5;
        torn.seqTail = 4;
        const bool fired = torn.seqHead != torn.seqTail;
        row (tl1 && fired, "TL1 PD15 recState / progress / sidechain peak track a scripted take",
             "meas Recording frames " + juce::String (f1) + "/" + juce::String (tgt) + ", sc peak " + f2 (pk, 3) + " (then " + f2 (pk2, 1)
                 + "), Committing / Committed, Idle after the copy-out | " + ctrl (fired, "TL1n torn frame (head 5, tail 4) -> rejected"));

        for (int n : { 48, 52, 55, 59, 62, 64 })
            s.noteOn (s.pos + 64, n);
        int worst = 0;
        bool complete = true;
        for (int k = 0; k < 40; ++k)
        {
            s.run (512, 512);
            const auto& f = s.eng().getTelemetry().frames.read();
            worst = std::max (worst, (int) f.count);
            complete = complete && f.seqHead == f.seqTail && f.seqHead > 0 && f.count <= 288;
        }
        row (complete && worst > 0, "TL2 PD15 grain frame: count <= 288, seqHead == seqTail every read",
             "meas max count " + juce::String (worst) + ", 40 reads complete " + (complete ? "yes" : "no") + " | thr <= 288 | ctrl TL1n");

        // TL3: feedback on → the head rewrites bins (epoch-tagged); a Reset makes them stale.
        Session f (kFs, cat ({ stBase(), all4 ("feedback", 80.0f) }), kSeed, 512);   // PLAN-R1 PD16
        f.load (0, noiseBuf (2, 48000, 0.3f, 121), kFs);
        for (int n : { 48, 60 })
            f.noteOn (64, n);
        f.run (48000, 512);
        auto& ov = f.eng().getTelemetry().overview;
        int valid = 0;
        float lo = 0.0f, hi = 0.0f;
        for (int b = 0; b < 94; ++b)
            valid += ov.read (0, b, lo, hi) ? 1 : 0;
        const auto ep = ov.epoch (0);
        f.p->resetSlotSync (0);
        f.run (4800, 64);   // the take (after the head parks, <= 20 ms + 1 cell) bumps the epoch
        for (int b = 0; b < 94; ++b) { float l2, h2; (void) ov.read (0, b, l2, h2); }
        int stale = 0;
        for (int b = 0; b < 94; ++b)
            stale += ov.read (0, b, lo, hi) ? 0 : 1;
        row (valid > 0 && ov.epoch (0) > ep && stale > 0, "TL3 PD15 overview bins epoch-tagged; a new tape makes them stale",
             "meas " + juce::String (valid) + " of 94 bins rewritten by the head, epoch " + juce::String ((int) ep) + " -> " + juce::String ((int) ov.epoch (0))
                 + ", stale after the take " + juce::String (stale) + " | thr > 0, bumped, stale | ctrl -");
    }

    //==========================================================================
    // TB7 — true bypass with the SC bus present and the recorder idle
    void rowTB7()
    {
        const auto now  = eg24::toMap (rh::computeAll());
        const int a = eg24::sameLines (now, eg24::readTable ("renders-2.4.sha256"));
        juce::StringArray legacy;
        {
            rh::Overrides o = rh::overrides;
            o.legacy = true;
            rh::ScopedOverrides so (o);
            legacy = rh::computeAll();
        }
        const int b = eg24::sameLines (eg24::toMap (legacy), eg24::readTable ("renders-2.2.sha256"));
        row (a == 14 && b == 14, "TB7 true bypass via the shipping path (SC bus present, recorder idle)",
             "meas renders-2.4 " + juce::String (a) + "/14, legacy " + juce::String (b) + "/14 | thr 14 / 14 | ctrl TB0 (legacyWetTanh)");
    }

    //==========================================================================
    // GD1 — the real Recordings folder is never touched by the harness
    struct DirSnap { bool exists = false; int count = 0; juce::int64 newest = 0; };
    DirSnap snapReal()
    {
        DirSnap d;
        const auto dir = realMusic()   // PLAN-4 PD15: the REAL home (the processor sees the temp one)
                             .getChildFile ("Ouaricon").getChildFile ("O-GranulatorInstrument").getChildFile ("Recordings");
        d.exists = dir.isDirectory();
        if (d.exists)
            for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*", juce::File::findFiles))
            {
                ++d.count;
                d.newest = std::max (d.newest, e.getModificationTime().toMilliseconds());
            }
        return d;
    }
    DirSnap gStart;
    void guardStart() { gStart = snapReal(); }

    void rowGD1()
    {
        const auto e = snapReal();
        row (e.exists == gStart.exists && e.count == gStart.count && e.newest == gStart.newest, "GD1 P17 the real ~/Music/.../Recordings unchanged",
             juce::String ("meas start ") + (gStart.exists ? juce::String (gStart.count) + " files" : "absent") + ", end "
                 + (e.exists ? juce::String (e.count) + " files" : "absent") + " | thr unchanged | ctrl -");
    }

    //==========================================================================
    void runAll()
    {
        gTmp26 = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ogi-eg26b-" + juce::String (juce::Time::currentTimeMillis()));
        gTmp26.createDirectory();
        struct Guard { ~Guard() { gTmp26.deleteRecursively(); } } guard;

        section ("2.6b  state / resolve / factory / hash / alloc / clicks / block size / telemetry / bypass (PLAN-2.6 T13)");
        if (want ("SS"))  { rowST1(); rowST2(); rowST3(); rowST4(); rowST5(); rowsST67(); }
        if (want ("RL"))  rowsRS();
        if (want ("FA"))  rowsFA();
        if (want ("HS"))  rowsHS();
        if (want ("AL26"))  rowAL1();
        if (want ("CK"))  rowsCK();
        if (want ("BS26"))  rowBS1();
        if (want ("TL"))  rowsTL();
        if (want ("TB"))  rowTB7();
        if (want ("GD"))  rowGD1();
    }

    //==========================================================================
    // --cpu26: the --cpu25a rows + C26rec INFO (C25m with a take in flight).
    int cpu26()
    {
        const int f = eg25b::cpu25a();
        constexpr int kBlock = 128;
        const double refNs = cal::readRef (gGoldenDir);
        std::vector<int> n16;
        for (int k = 0; k < 16; ++k) n16.push_back (40 + k);
        std::uint64_t starts = 0;
        auto mk = [&starts]
        {
            auto s = eg24::c25Session (kFs, kBlock, true, eg23::worstFx(), cat ({ eg24::c25Extra(), eg25b::c25mRoutes(),
                                                                                  { { "rec_trigger", 0.0f }, { "rec_length", 30.0f } } }));
            setP (*s->p, "rec_arm", 1.0f);   // the edge lands at the first block: Recording for 30 s
            starts += 1;
            return s;
        };
        const auto m = eg24::measure25 (mk, n16, kFs, kBlock, refNs, true);
        std::printf ("  INFO %s (C26rec: C25m with a Manual take in flight, 30 s cap; %d sessions)\n",
                     eg23::cpuLine ("C26rec", m, refNs).toRawUTF8(), (int) starts);
        std::fflush (stdout);
        // PLAN-R1 PD33: C25m with Reverb Pre on all four streams (eight tanks). INFO, not a row; above 40 % = stop and ask.
        const auto mp = eg24::measure25 ([] { return eg24::c25Session (kFs, kBlock, true, eg23::worstFx(),
                                                  cat ({ eg24::c25Extra(), eg25b::c25mRoutes(), eg24::all4 ("reverb_pre", 1.0f) })); },
                                        n16, kFs, kBlock, refNs, true);
        std::printf ("  INFO %s (C25mPre: C25m with sN_reverb_pre = Pre on 4 streams, 8 tanks; delta vs C26rec %+.2f points)\n",
                     eg23::cpuLine ("C25mPre", mp, refNs).toRawUTF8(), mp.pct - m.pct);
        std::fflush (stdout);
        return f;
    }
} }
