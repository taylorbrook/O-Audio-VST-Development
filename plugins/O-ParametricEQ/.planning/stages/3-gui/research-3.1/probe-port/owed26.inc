    //==========================================================================
    //==========================================================================
    // Stage 3, Phase 3.1: the rows VERIFICATION-2.6 Issue 2 owes (CONTEXT U8).
    // Ported from stages/2-dsp/verify-2.6/probe.cpp (E1, T1, N1, R1, X1: the
    // probe-only kills of LS10, LS11, LS12, LS15 / PX26, PX27, PX38, PX39),
    // plus PX22, DM2 and AN1 (real by read, ungated in 2.6). The renders
    // follow the probe's: a fresh instance prepared, then set up; a block is
    // cut at every event, so an event lands on a block start.
    //==========================================================================
    //==========================================================================
    using OwedFn = std::function<void (Processor&)>;

    struct OwedEvent
    {
        int at;
        OwedFn apply;
    };

    struct OwedOptions
    {
        int block = 64;
        bool randomBlocks = false;      // 1 ... maxBlock, seeded
        juce::int64 seed = 1;
        int maxBlock = 2048;
        std::function<bool (int)> viaBypassed;                  // per block start
        std::function<void (Processor&, int, int)> after;       // after each block (start, length)
    };

    // Band indices here are 0-based, as listenBand's (bandId is 1-based).
    OwedFn owedBand (int index, int type, float freq, float gain, float q = 1.0f, int slope = 1, int placement = kStereo)
    {
        return [=] (Processor& p) { setBand (p, index + 1, makeSpec (type, freq, gain, q, slope, placement)); };
    }

    OwedFn owedSet (const juce::String& id, float raw)
    {
        return [=] (Processor& p) { setRaw (p, id, raw); };
    }

    OwedFn owedListen (int band)
    {
        return [=] (Processor& p) { p.listenBand.store (band); };
    }

    OwedFn owedAll (std::vector<OwedFn> fns)
    {
        return [=] (Processor& p) { for (const auto& f : fns) if (f) f (p); };
    }

    int owedLength (double seconds, double rate)  { return juce::jmax (1, (int) std::lround (seconds * rate)); }
    int owedCeil (int sample, int grid)           { return ((sample + grid - 1) / grid) * grid; }

    juce::AudioBuffer<float> owedRender (double rate, const juce::AudioBuffer<float>& input, const OwedFn& setup,
                                         const std::vector<OwedEvent>& events, const OwedOptions& o = {})
    {
        const int length = input.getNumSamples(), numChannels = input.getNumChannels();
        const int prepared = o.randomBlocks ? o.maxBlock : o.block;
        auto p = make();

        if (! setLayout (*p, numChannels))
            row (false, "harness: layout refused (" + juce::String (numChannels) + " channels)");

        p->setRateAndBufferSizeDetails (rate, prepared);
        p->prepareToPlay (rate, prepared);

        if (setup)
            setup (*p);

        juce::AudioBuffer<float> buffer;
        buffer.makeCopyOf (input);
        juce::MidiBuffer midi;
        juce::Random random (o.seed);

        for (int start = 0; start < length;)
        {
            int n = juce::jmin (o.randomBlocks ? 1 + random.nextInt (o.maxBlock) : o.block, length - start);

            for (const auto& e : events)
                if (e.at > start && e.at < start + n)
                    n = e.at - start;

            for (const auto& e : events)
                if (e.at == start)
                    e.apply (*p);

            juce::AudioBuffer<float> view (buffer.getArrayOfWritePointers(), numChannels, start, n);

            if (o.viaBypassed && o.viaBypassed (start))
                p->processBlockBypassed (view, midi);
            else
                p->processBlock (view, midi);

            if (o.after)
                o.after (*p, start, n);

            start += n;
        }

        p->releaseResources();
        return buffer;
    }

    bool owedSame (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        return a.getNumChannels() == b.getNumChannels() && a.getNumSamples() == b.getNumSamples()
            && sameFrom (a, 0, b, 0, a.getNumSamples());
    }

    /** First sample index at which a and b differ (any channel), or -1. */
    int owedFirstDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        int first = -1;

        for (int channel = 0; channel < a.getNumChannels(); ++channel)
            for (int i = 0; i < a.getNumSamples(); ++i)
                if (! sameBits (a.getSample (channel, i), b.getSample (channel, i)))
                {
                    if (first < 0 || i < first)
                        first = i;

                    break;
                }

        return first;
    }

    void owed26RowsXr()
    {
        section ("XR-ls10 / XR-ls11 / XR-ls15 / XR-px38 / XR-px39 / XR-px22 / XR-dm2 / XR-an1. the rows VERIFICATION-2.6 Issue 2 owes (Phase 3.1: CONTEXT U8; probe E1, T1, N1, R1, X1)");

        //======================================================================
        // XR-ls10 (probe E1; kills LS10, PX27): an f / Q edit of the listened
        // band inside the incoming voice's input ramp reaches that voice on its
        // tick (PLAN-2.6 P6, applyPending), so the output differs from the
        // unedited render from move sample R on (the first sample the voice is
        // heard), within one grid; and an edit of a settled listen moves the
        // listen output (the voices' control tick runs).
        //======================================================================
        {
            Tally tally;
            juce::String figures;

            for (const double rate : { 48000.0, 96000.0 })
                for (const int kind : { 0, 1 })         // 0 engage band 3; 1 change 3 -> 7, the edit on 7
                    for (const int what : { 0, 1 })     // 0 freq, 1 Q
                    {
                        const int grid = harnessGrid (rate), ramp = owedLength (0.030, rate), fade = owedLength (0.020, rate), move = ramp + fade;
                        const int s1 = 1001, t1 = owedCeil (s1, grid);
                        const int s2 = t1 + move + 500, t2 = owedCeil (s2, grid);
                        const int moveStart = kind == 0 ? t1 : t2, band = kind == 0 ? 3 : 7;
                        const int edit = moveStart + ramp / 3;
                        const int length = moveStart + move + 4000;
                        const auto input = makeNoiseAt (2, length, 0x31000095, 0.5f);
                        const auto setup = owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.0f), owedSet (bandId (4, "enabled"), 0.0f),
                                                      owedBand (7, kBandPass, 3000.0f, 0.0f, 4.0f), owedSet (bandId (8, "enabled"), 0.0f) });
                        std::vector<OwedEvent> base = { { s1, owedListen (3) } };

                        if (kind == 1)
                            base.push_back ({ s2, owedListen (7) });

                        auto edited = base;
                        edited.push_back ({ edit, what == 0 ? owedSet (bandId (band + 1, "freq"), band == 3 ? 1500.0f : 4500.0f)
                                                            : owedSet (bandId (band + 1, "q"), 0.5f) });
                        OwedOptions o;
                        o.block = 64;
                        const int first = owedFirstDifference (owedRender (rate, input, setup, base, o), owedRender (rate, input, setup, edited, o));
                        const bool ok = first >= moveStart + ramp && first < moveStart + ramp + grid;
                        const juce::String where = fmtG (rate / 1000.0, 3) + "k " + (kind ? "change" : "engage") + (what ? " Q" : " f");
                        tally.check (ok, where + " first difference " + juce::String (first) + " (want " + juce::String (moveStart + ramp) + " + < " + juce::String (grid) + ")");
                        figures += (figures.isEmpty() ? "" : ", ") + where + " " + juce::String (first) + " / " + juce::String (moveStart + ramp);
                    }

            // A settled listen (from the first block) on band 3: an f / Q edit at mid render moves the output.
            for (const int what : { 0, 1 })
            {
                const double rate = 48000.0;
                const int length = owedLength (0.25, rate);
                const auto input = makeNoiseAt (2, length, 0x31000096, 0.5f);
                const auto setup = owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.5f), owedListen (3) });
                OwedOptions o;
                o.block = 128;
                const auto base = owedRender (rate, input, setup, {}, o);
                const auto edited = owedRender (rate, input, setup, { { length / 2, what == 0 ? owedSet (bandId (4, "freq"), 1300.0f) : owedSet (bandId (4, "q"), 0.7f) } }, o);
                const int first = owedFirstDifference (base, edited);
                tally.check (first >= length / 2, juce::String ("settled listen, ") + (what ? "Q" : "f") + " edit at " + juce::String (length / 2) + ": first difference " + juce::String (first));
                figures += juce::String (", settled ") + (what ? "Q" : "f") + " edit " + juce::String (first) + " / " + juce::String (length / 2);
            }

            row (tally.bad == 0 && tally.total == 10,
                 "XR-ls10 VERIFICATION-2.6 LS10 / PX27 (probe E1): an f / Q edit of the listened band inside the incoming voice's input ramp (engage, and change 3 -> 7), 48 / 96 kHz, block 64: the output first differs from the unedited render at move sample R (within one grid); a settled listen's f / Q edit moves the output",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-ls11 (probe T1; kills LS11, LS12): the listen timeline at block 1
        // against the re-stated P6 state machine. A latched change, release
        // and engage; a settled frequency edit of the listened band ramps (no
        // move); a gain edit does nothing; a type-family edit runs a change
        // move to the same band; a latched change after it.
        //======================================================================
        {
            struct Segment { int start; eq::ListenMove kind; int length; int target; bool latched; };
            struct Cfg { double rate; int channels; };
            Tally tally;
            juce::String figures;

            for (const Cfg c : { Cfg { 48000.0, 2 }, Cfg { 96000.0, 2 }, Cfg { 44100.0, 1 }, Cfg { 192000.0, 2 } })
            {
                const int grid = harnessGrid (c.rate), ramp = owedLength (0.030, c.rate), fade = owedLength (0.020, c.rate), move = ramp + fade;
                const int s1 = 1001;            const int t1 = owedCeil (s1, grid);
                const int s2 = t1 + 500;        const int e1 = t1 + move;       // latched change to 7
                const int s3 = e1 + 700;        const int e2 = e1 + move;       // latched release
                const int s4 = e2 + 100;        const int e3 = e2 + fade;       // latched engage to 5
                const int s5 = e3 + move + 301;                                  // band 5 freq edit: a ramp, no move
                const int s6 = s5 + 403;                                         // band 5 gain edit: nothing
                const int s7 = s6 + 405;        const int t7 = owedCeil (s7, grid);   // band 5 type family: a change, target 5
                const int s8 = t7 + 37;         const int e7 = t7 + move;       // latched change to 9
                const int length = e7 + move + 500;

                const std::vector<Segment> segments = {
                    { t1, eq::ListenMove::engage,  move, 3,  false },
                    { e1, eq::ListenMove::change,  move, 7,  true  },
                    { e2, eq::ListenMove::release, fade, -1, true  },
                    { e3, eq::ListenMove::engage,  move, 5,  true  },
                    { t7, eq::ListenMove::change,  move, 5,  false },
                    { e7, eq::ListenMove::change,  move, 9,  true  } };

                const auto targetWithNoMove = [&] (int i)
                {
                    if (i < t1)
                        return -1;

                    for (int k = (int) segments.size() - 1; k >= 0; --k)
                        if (i >= segments[(size_t) k].start)
                            return segments[(size_t) k].target;

                    return -1;
                };

                const auto expect = [&] (int i, eq::ListenMove& kind, int& position, int& target)
                {
                    for (size_t k = 0; k < segments.size(); ++k)
                    {
                        const Segment& s = segments[k];

                        if (i >= s.start && i < s.start + s.length)
                        {
                            if (i < s.start + s.length - 1)
                            {
                                kind = s.kind; position = i - s.start + 1; target = s.target;
                                return;
                            }

                            if (k + 1 < segments.size() && segments[k + 1].latched && segments[k + 1].start == i + 1)
                            {
                                kind = segments[k + 1].kind; position = 0; target = segments[k + 1].target;
                                return;
                            }

                            kind = eq::ListenMove::none; position = 0; target = s.target;
                            return;
                        }
                    }

                    kind = eq::ListenMove::none; position = 0; target = targetWithNoMove (i);
                };

                int mismatched = 0, firstBad = -1, rampSeen = 0, fadeSeen = 0;
                OwedOptions o;
                o.block = 1;
                o.after = [&] (Processor& p, int start, int)
                {
                    const eq::ListenState state = p.getListenState();
                    rampSeen = state.rampLength;
                    fadeSeen = state.fadeLength;
                    eq::ListenMove kind;
                    int position = 0, target = 0;
                    expect (start, kind, position, target);

                    if (state.move != kind || state.movePosition != position || state.target != target)
                    {
                        if (firstBad < 0)
                            firstBad = start;

                        ++mismatched;
                    }
                };

                const auto input = makeNoiseAt (c.channels, length, 0x31000021, 0.5f);
                const auto setup = owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.0f), owedBand (7, kLowShelf, 200.0f, -4.0f),
                                              owedBand (5, kBell, 3000.0f, 3.0f, 4.0f), owedBand (9, kHighCut, 6000.0f, 0.0f, 1.0f, 3) });
                owedRender (c.rate, input, setup,
                            { { s1, owedListen (3) }, { s2, owedListen (7) }, { s3, owedListen (-1) }, { s4, owedListen (5) },
                              { s5, owedSet (bandId (6, "freq"), 2500.0f) }, { s6, owedSet (bandId (6, "gain"), -9.0f) },
                              { s7, owedSet (bandId (6, "type"), (float) kHighShelf) }, { s8, owedListen (9) } }, o);

                const juce::String where = fmtG (c.rate / 1000.0, 4) + "k/" + juce::String (c.channels) + "ch";
                tally.check (mismatched == 0 && rampSeen == ramp && fadeSeen == fade,
                             where + ": " + juce::String (mismatched) + " samples off the state machine" + (firstBad >= 0 ? " (first " + juce::String (firstBad) + ")" : juce::String())
                                 + ", R " + juce::String (rampSeen) + " / " + juce::String (ramp) + ", N " + juce::String (fadeSeen) + " / " + juce::String (fade));
                figures += (figures.isEmpty() ? "" : ", ") + where + " " + juce::String (length) + " samples, " + juce::String (mismatched) + " off";
            }

            row (tally.bad == 0 && tally.total == 4,
                 "XR-ls11 VERIFICATION-2.6 LS11 / LS12 (probe T1): the listen state (move, move position, target) after every sample at block 1, 48 / 96 / 44.1 (mono) / 192 kHz, equals the re-stated P6 machine: latched change, release and engage; a settled f edit of the listened band ramps with no move; a gain edit does nothing; a type-family edit runs a change move to the same band; R and N as prepared",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-ls15 (probe N1; kills LS15, PX26): a non-finite input sample,
        // listen settled on band 3 and no listen: the output is finite from
        // the first block that starts after it (PLAN-2.6 P10 step 8 clears the
        // listen voices).
        //======================================================================
        {
            Tally tally;

            for (const int channels : { 1, 2 })
                for (const bool listening : { true, false })
                    for (const int block : { 64, 512 })
                    {
                        const double rate = 48000.0;
                        const int length = 48000, nanAt = 20000 + 37;
                        auto input = makeNoiseAt (channels, length, 0x31000090 + channels, 0.5f);

                        for (int channel = 0; channel < channels; ++channel)
                            input.setSample (channel, nanAt, std::numeric_limits<float>::quiet_NaN());

                        OwedOptions o;
                        o.block = block;
                        const auto y = owedRender (rate, input, owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.0f), owedListen (listening ? 3 : -1) }), {}, o);
                        const int from = (nanAt / block + 1) * block;
                        int nonFinite = 0;

                        for (int channel = 0; channel < channels; ++channel)
                            for (int i = from; i < length; ++i)
                                if (! std::isfinite (y.getSample (channel, i)))
                                    ++nonFinite;

                        tally.check (nonFinite == 0, juce::String (channels) + "ch " + (listening ? "listen" : "no listen") + " block " + juce::String (block) + ": "
                                                         + juce::String (nonFinite) + " non-finite samples from " + juce::String (from));
                    }

            row (tally.bad == 0 && tally.total == 8,
                 "XR-ls15 VERIFICATION-2.6 LS15 / PX26 (probe N1): a NaN input sample at 20 037, listen settled on band 3 and no listen, mono / stereo, blocks 64 / 512, 48 kHz: the output is finite from the first block that starts after it",
                 tally.detail());
        }

        //======================================================================
        // XR-px38 (probe R1): an out-of-range listenBand reads as off
        //======================================================================
        {
            Tally tally;

            for (const int channels : { 1, 2 })
            {
                const double rate = 48000.0;
                const int length = owedLength (0.4, rate);
                const auto input = makeNoiseAt (channels, length, 0x31000060, 0.5f);
                const auto setup = owedAll ({ owedBand (0, kBell, 500.0f, 6.0f), owedBand (11, kLowCut, 60.0f, 0.0f, 1.0f, 5), owedBand (23, kHighShelf, 5000.0f, -4.0f) });
                OwedOptions o;
                o.randomBlocks = true;
                o.maxBlock = 1000;
                o.seed = 3;
                const auto base = owedRender (rate, input, setup, {}, o);
                const auto y = owedRender (rate, input, setup,
                                           { { 777, owedListen (Processor::kNumBands) }, { 5000, owedListen (1000) }, { 9001, owedListen (-2) },
                                             { 13000, owedListen (std::numeric_limits<int>::min()) }, { 15555, owedListen (std::numeric_limits<int>::max()) },
                                             { 17000, owedListen (-1) } }, o);
                const int first = owedFirstDifference (y, base);
                tally.check (first < 0, juce::String (channels) + "ch: first difference " + juce::String (first));
            }

            row (tally.bad == 0 && tally.total == 2,
                 "XR-px38 VERIFICATION-2.6 PX38 (probe R1): listenBand 24, 1000, -2, INT_MIN, INT_MAX, then -1, three bands on, random blocks 1 - 1000, mono / stereo, 48 kHz: memcmp-equal to the render with no listen",
                 tally.detail());
        }

        //======================================================================
        // XR-px39 (probe X1): a latched un-bypass with listen engaged, no band
        // running and the stage at unity fades back in (P10 step 7 jumps only
        // with listen idle). Bypass on at b1, off again 128 samples later, by
        // the parameter and by processBlockBypassed; the fade back starts at
        // r = b1 + N from the dry signal.
        //======================================================================
        {
            Tally tally;
            juce::String figures;

            for (const double rate : { 48000.0, 96000.0 })
                for (const int channels : { 1, 2 })
                    for (const bool viaEntry : { false, true })
                    {
                        const int fade = owedLength (0.020, rate), b1 = 6400, b2 = b1 + 128, length = b1 + 6 * fade;
                        const auto input = makeNoiseAt (channels, length, 0x31000101, 0.5f);
                        OwedOptions o;
                        o.block = 64;
                        std::vector<OwedEvent> events = { { 0, owedListen (3) } };

                        if (viaEntry)
                        {
                            o.viaBypassed = [b1, b2] (int start) { return start >= b1 && start < b2; };
                        }
                        else
                        {
                            events.push_back ({ b1, owedSet ("bypass", 1.0f) });
                            events.push_back ({ b2, owedSet ("bypass", 0.0f) });
                        }

                        const auto y = owedRender (rate, input, owedAll ({ owedBand (3, kBandPass, 1000.0f, 0.0f, 2.0f), owedSet (bandId (4, "enabled"), 0.0f) }), events, o);
                        const int r = b1 + fade;
                        const auto meanError = [&] (int from, int to)
                        {
                            double sum = 0.0;
                            int count = 0;

                            for (int channel = 0; channel < channels; ++channel)
                                for (int i = from; i < to; ++i, ++count)
                                    sum += std::abs ((double) y.getSample (channel, i) - (double) input.getSample (channel, i));

                            return sum / juce::jmax (1, count);
                        };

                        const double ratio = meanError (r - 4, r + 12) / meanError (r + 2 * fade, r + 2 * fade + 512);
                        const juce::String where = fmtG (rate / 1000.0, 3) + "k/" + juce::String (channels) + "ch/" + (viaEntry ? "bypassed" : "param");
                        tally.check (ratio < 0.05, where + " " + fmtG (ratio, 3));
                        figures += (figures.isEmpty() ? "" : ", ") + where + " " + fmtG (ratio, 2);
                    }

            row (tally.bad == 0 && tally.total == 8,
                 "XR-px39 VERIFICATION-2.6 PX39 (probe X1): listen on band 3 (disabled, no band running, unity stage), bypass on at 6400 and off 128 samples later, by the parameter and by processBlockBypassed, 48 / 96 kHz, mono / stereo: the fade back from the dry signal, mean |y - x| over [r - 4, r + 12) under 0.05 x the settled one (r = 6400 + N; a jump reads ~1)",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-px22: on a mono layout every placement is effectively Stereo, so
        // a placement edit of the listened band while listening changes
        // nothing: no move starts and the output is memcmp-equal to the
        // unedited render (makeInput applies effectiveBandInput). Control: the
        // same edits on a stereo layout start change moves and move the output.
        //======================================================================
        {
            Tally tally;
            juce::String figures;

            for (const int channels : { 1, 2 })
            {
                const double rate = 48000.0;
                const int length = owedLength (0.5, rate);
                const auto input = makeNoiseAt (channels, length, 0x31000022, 0.5f);
                const auto setup = owedAll ({ owedBand (3, kBell, 1000.0f, 6.0f, 2.0f, 1, kLeft), owedListen (3) });
                const std::vector<OwedEvent> edits = { { 4001, owedSet (bandId (4, "placement"), (float) kRight) },
                                                       { 9007, owedSet (bandId (4, "placement"), (float) kMid) },
                                                       { 14011, owedSet (bandId (4, "placement"), (float) kSide) },
                                                       { 19013, owedSet (bandId (4, "placement"), (float) kStereo) } };
                int moveBlocks = 0, offTarget = 0;
                OwedOptions o;
                o.block = 64;
                const auto base = owedRender (rate, input, setup, {}, o);
                o.after = [&] (Processor& p, int, int)
                {
                    const eq::ListenState state = p.getListenState();
                    moveBlocks += state.move != eq::ListenMove::none ? 1 : 0;
                    offTarget += state.target != 3 ? 1 : 0;
                };
                const auto y = owedRender (rate, input, setup, edits, o);
                const int first = owedFirstDifference (y, base);

                if (channels == 1)
                    tally.check (moveBlocks == 0 && offTarget == 0 && first < 0,
                                 "mono: " + juce::String (moveBlocks) + " blocks in a move, " + juce::String (offTarget) + " off target 3, first difference " + juce::String (first));
                else
                    tally.check (moveBlocks > 0 && first >= 4001,
                                 "control, stereo: " + juce::String (moveBlocks) + " blocks in a move, first difference " + juce::String (first));

                figures += (figures.isEmpty() ? "" : ", ") + juce::String (channels == 1 ? "mono" : "stereo control") + ": " + juce::String (moveBlocks)
                         + " move blocks, first difference " + juce::String (first);
            }

            row (tally.bad == 0 && tally.total == 2,
                 "XR-px22 VERIFICATION-2.6 PX22: mono layout, listen settled on a Left bell, placement edits Left -> Right -> Mid -> Side -> Stereo, block 64, 48 kHz: no listen move starts and the output is memcmp-equal to the unedited render; control: the same edits on stereo start moves and move the output",
                 tally.detail() + "  " + figures);
        }

        //======================================================================
        // XR-dm2: DisplayModel::poll re-designs every band on a layout change
        // at the same rate (a host renegotiating mono / stereo): all 24 are
        // reported, the header's stereo flag follows, and the packets carry
        // the effective placement (Stereo on mono, the parameter's on stereo).
        //======================================================================
        {
            Tally tally;
            auto p = make();
            setLayout (*p, 2);
            const int bands[4] = { 2, 4, 6, 8 };
            const int placements[4] = { kLeft, kMid, kSide, kRight };
            setBand (*p, bands[0] + 1, makeSpec (kBell, 500.0f, 6.0f, 2.0f, 1, placements[0]));
            setBand (*p, bands[1] + 1, makeSpec (kHighShelf, 4000.0f, -3.0f, 1.0f, 1, placements[1]));
            setBand (*p, bands[2] + 1, makeSpec (kBell, 2000.0f, 4.0f, 1.0f, 1, placements[2]));
            setBand (*p, bands[3] + 1, makeSpec (kLowCut, 80.0f, 0.0f, 1.0f, 3, placements[3]));
            auto model = std::make_unique<DisplayModel> (*p);

            const auto all24 = [] (const juce::Array<int>& a)
            {
                if (a.size() != Processor::kNumBands)
                    return false;

                for (int b = 0; b < Processor::kNumBands; ++b)
                    if (a[b] != b)
                        return false;

                return true;
            };

            const auto header = [&model] (double rate, bool stereo)
            {
                const juce::var head = model->headerVar();
                return (double) head.getProperty ("sampleRate", 0.0) == rate && (bool) head.getProperty ("rateKnown", false)
                    && (bool) head.getProperty ("stereo", ! stereo) == stereo;
            };

            const auto placementsAre = [&] (bool stereo)
            {
                for (int i = 0; i < 4; ++i)
                    if ((int) model->packetFor (bands[i]).getProperty ("placement", -1) != (stereo ? placements[i] : kStereo))
                        return false;

                return true;
            };

            const auto relayout = [&p] (int channels, double rate)
            {
                p->releaseResources();
                const bool ok = setLayout (*p, channels);
                p->prepareToPlay (rate, 512);
                return ok;
            };

            for (const double rate : { 48000.0, 96000.0 })
            {
                const juce::String r = fmtG (rate / 1000.0, 3) + "k";
                bool ok = relayout (2, rate);
                auto changed = model->poll();
                tally.check (ok && all24 (changed) && header (rate, true) && placementsAre (true), r + " stereo prepare: " + juce::String (changed.size()) + " bands");

                changed = model->poll();
                tally.check (changed.isEmpty(), r + " no edit: " + juce::String (changed.size()));

                ok = relayout (1, rate);
                changed = model->poll();
                tally.check (ok && all24 (changed) && header (rate, false) && placementsAre (false),
                             r + " stereo -> mono at the same rate: " + juce::String (changed.size()) + " bands, header stereo "
                                 + (model->headerVar().getProperty ("stereo", true) ? "true" : "false") + ", placements " + (placementsAre (false) ? "Stereo" : "NOT EFFECTIVE"));

                changed = model->poll();
                tally.check (changed.isEmpty(), r + " no edit on mono: " + juce::String (changed.size()));

                ok = relayout (2, rate);
                changed = model->poll();
                tally.check (ok && all24 (changed) && header (rate, true) && placementsAre (true),
                             r + " mono -> stereo at the same rate: " + juce::String (changed.size()) + " bands, placements " + (placementsAre (true) ? "the parameters'" : "NOT RESTORED"));
            }

            model.reset();
            p->releaseResources();

            row (tally.bad == 0 && tally.total == 10,
                 "XR-dm2 VERIFICATION-2.6 DM2: DisplayModel::poll on a layout change at the same rate (stereo -> mono -> stereo at 48 and 96 kHz, re-prepared): all 24 bands re-designed, the header's stereo flag follows, Left / Mid / Side / Right bands carry Stereo on mono and their own placement on stereo; no edit -> empty",
                 tally.detail());
        }

        //======================================================================
        // XR-an1: no analyzer frame straddles a drop. The harness re-states
        // which input samples each drain holds (the processor offers each
        // block's last min (n, C) samples; a push that does not fit keeps its
        // newest `space` samples; the analyzer drains everything per call),
        // checks that against the analyzer's drained count, and requires that
        // whenever a frame is computed the newest N drained samples are
        // consecutive input samples. A 2 s stall (no process call) overflows
        // both rings; frames must resume after it (vacuity guard).
        //======================================================================
        {
            Tally tally;
            juce::String figures;
            const double rate = 48000.0;
            const int fftN = SpectrumAnalyzer::fftSizeForRate (rate);
            constexpr int blockSize = 512;
            const int stall = (int) (2.0 * rate);

            for (const int cadence : { 512, 2048 })
            {
                auto p = makeBands (2, 0, mixedSpec);
                const Handles h = handlesOf (*p);
                resetAllRaw (*p, h);
                p->analyzerMode.store (3);
                p->prepareToPlay (rate, blockSize);
                auto analyzer = std::make_unique<SpectrumAnalyzer> (*p);

                std::vector<int> pending;           // input indices in the rings since the last drain (both taps alike)
                std::vector<int> history;           // every drained index, in order
                juce::AudioBuffer<float> block (2, blockSize);
                juce::MidiBuffer midi;
                int position = 0, straddles = 0, modelOff = 0, framesBefore = 0, framesAfter = 0, callsToResume = -1, callsAfter = 0;
                bool afterStall = false;
                std::uint64_t frames[2] = { 0, 0 }, drained[2] = { 0, 0 };

                const auto play = [&] (int samples)
                {
                    for (int done = 0; done < samples;)
                    {
                        const int n = juce::jmin (blockSize, samples - done);
                        block.setSize (2, n, false, false, true);

                        for (int i = 0; i < n; ++i)
                        {
                            const auto v = (float) (0.25 * std::sin (2.0 * eq::pi * 1000.0 * (double) (position + i) / rate));
                            block.setSample (0, i, v);
                            block.setSample (1, i, v);
                        }

                        p->processBlock (block, midi);

                        const int offered = juce::jmin (n, eq::SpectrumTap::capacity);
                        const int space = eq::SpectrumTap::capacity - (int) pending.size();
                        const int kept = juce::jmin (offered, space);

                        for (int i = n - kept; i < n; ++i)
                            pending.push_back (position + i);

                        position += n;
                        done += n;
                    }
                };

                const auto call = [&]
                {
                    analyzer->process ((double) cadence / rate);
                    history.insert (history.end(), pending.begin(), pending.end());

                    bool framed = false;

                    for (int tap = 0; tap < 2; ++tap)
                    {
                        const auto d = analyzer->getSamplesDrained (tap) - drained[tap];
                        drained[tap] = analyzer->getSamplesDrained (tap);

                        if (d != (std::uint64_t) pending.size())
                            ++modelOff;

                        if (analyzer->getFramesComputed (tap) > frames[tap])
                        {
                            framed = true;
                            frames[tap] = analyzer->getFramesComputed (tap);

                            bool consecutive = (int) history.size() >= fftN;

                            for (size_t k = history.size() - (size_t) juce::jmin ((int) history.size(), fftN) + 1; k < history.size() && consecutive; ++k)
                                consecutive = history[k] == history[k - 1] + 1;

                            if (! consecutive)
                                ++straddles;
                        }
                    }

                    pending.clear();

                    if (afterStall)
                    {
                        ++callsAfter;

                        if (framed)
                        {
                            ++framesAfter;

                            if (callsToResume < 0)
                                callsToResume = callsAfter;
                        }
                    }
                    else if (framed)
                    {
                        ++framesBefore;
                    }
                };

                for (int k = 0; k < 16; ++k)
                {
                    play (cadence);
                    call();
                }

                const std::uint32_t dropsBefore = p->getPreTap().getDropCount() + p->getPostTap().getDropCount();
                play (stall);
                const std::uint32_t dropsStall = p->getPreTap().getDropCount() + p->getPostTap().getDropCount() - dropsBefore;
                afterStall = true;
                call();                                             // the stale, overflowed run

                for (int k = 0; k < 16; ++k)
                {
                    play (cadence);
                    call();
                }

                analyzer.reset();
                p->releaseResources();

                const bool resumed = callsToResume > 0 && framesAfter > 0;
                tally.check (straddles == 0 && modelOff == 0 && framesBefore > 0 && dropsStall > 0 && resumed,
                             "cadence " + juce::String (cadence) + ": " + juce::String (straddles) + " frames straddle a gap, model off " + juce::String (modelOff)
                                 + ", frames before / after " + juce::String (framesBefore) + " / " + juce::String (framesAfter) + ", drops " + juce::String ((juce::int64) dropsStall)
                                 + ", first frame at call " + juce::String (callsToResume) + " after the stall");
                figures += (figures.isEmpty() ? "" : ", ") + juce::String ("cadence ") + juce::String (cadence) + ": first frame at call " + juce::String (callsToResume)
                         + " after the stall, drops " + juce::String ((juce::int64) dropsStall);
            }

            row (tally.bad == 0 && tally.total == 2,
                 "XR-an1 VERIFICATION-2.6 AN1: analyzer with both taps, 48 kHz, blocks 512, a process call every 512 / 2048 samples, a 2 s stall (both rings overflow): every frame's newest N drained samples are consecutive input samples (the harness's re-stated drain contents, checked against the analyzer's drained count); frames resume after the stall",
                 tally.detail() + "  " + figures);
        }
    }

