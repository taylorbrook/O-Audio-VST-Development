// O-Polystutter scheduler harness: drives four RepeatLanes + TapeDegrader the way
// PluginProcessor::processBlock does, on a synthetic stereo input with a scripted
// transport and MIDI list, chopped into blocks of a chosen pattern.
// Built twice: against the working tree (-DNEW_ENGINE: TriggerScheduler, segmented
// lanes, counter RNG) and against the v1.16.0 DSP (TriggerRouter, block-start
// triggers; the driver below is a verbatim copy of v1.16.0's processBlock order).
//
// usage: psh <config> <block 32|480|512|var> <seed> <out.f32> [onsets.txt]
#include <JuceHeader.h>
#include "RepeatLane.h"
#include "TapeDegrader.h"
#ifdef NEW_ENGINE
 #include "TriggerScheduler.h"
#else
 #include "TriggerRouter.h"
#endif
#include <cstdio>
#include <string>
#include <vector>

static constexpr double kFs = 48000.0;
static constexpr int kLen = 480000;   // 10 s
static constexpr int kMaxBlock = 1024;

struct Lcg { uint64_t s; uint32_t next() { s = s * 6364136223846793005ull + 1442695040888963407ull; return (uint32_t) (s >> 33); }
             float uni() { return (float) next() / 2147483648.0f; } };

struct LaneP
{
    bool enabled = true; int subdiv = 2, repeats = 4;
    float decay = 90, volume = 100, pan = 0, prob = 100, swing = 0, pitch = 0, filter = 0;
    bool prEn = false; float prMin = 0, prMax = 0; bool prQ = true;
    bool pingPong = false, reverse = false;
    bool euc = false; int eucP = 4, eucS = 16;
    bool steps[16] = { true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true };
};

struct Config
{
    LaneP lane[4];
    bool midiEnabled = false, sequencer = false;
    float dry = 50, wet = 100;
    bool tapeBypass = true;
    float sat = 0, wow = 0, flutter = 0, hiss = 0, rolloff = 0, dropout = 0;
    double bpm = 120.0;
    int startSample = 0;          // transport position of render sample 0
    int loopLen = 0;              // > 0: transport loops [0, loopLen) samples (block split at the wrap)
    std::vector<std::pair<int, int>> notes;   // (render sample, note)
};

static void stepsOff(LaneP& l, std::initializer_list<int> off) { for (int s : off) l.steps[s] = false; }

static Config makeConfig(const std::string& name)
{
    Config c;
    auto beatLanes = [&]
    {
        c.bpm = 128.0; c.sequencer = true;
        c.lane[0] = {}; c.lane[0].subdiv = 2; c.lane[0].repeats = 4; c.lane[0].decay = 80; c.lane[0].pan = -30;
        stepsOff(c.lane[0], { 1, 2, 3, 5, 6, 9, 10, 11, 13 });
        c.lane[1].subdiv = 4; c.lane[1].repeats = 3; c.lane[1].reverse = true; c.lane[1].pitch = 5;
        stepsOff(c.lane[1], { 1, 2, 4, 5, 7, 8, 10, 11, 13, 14 });
        c.lane[2].subdiv = 3; c.lane[2].repeats = 6; c.lane[2].swing = 40; c.lane[2].pingPong = true;
        stepsOff(c.lane[2], { 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15 });
        c.lane[3].subdiv = 0; c.lane[3].repeats = 2; c.lane[3].pitch = -7; c.lane[3].filter = -50;
        stepsOff(c.lane[3], { 1, 3 });
        c.startSample = 1234;
    };
    auto noisyTape = [&] { c.tapeBypass = false; c.sat = 30; c.wow = 40; c.flutter = 30; c.hiss = 60; c.rolloff = 20; c.dropout = 50; };

    if (name == "beat128") beatLanes();
    else if (name == "beatprob")
    {
        beatLanes(); noisyTape();
        c.lane[0].prob = 50; c.lane[1].prob = 70; c.lane[3].prob = 40;
        c.lane[0].prEn = true; c.lane[0].prMin = -5; c.lane[0].prMax = 5;
        c.lane[2].prEn = true; c.lane[2].prMin = -2; c.lane[2].prMax = 2; c.lane[2].prQ = false;
        c.lane[3].euc = true; c.lane[3].eucP = 5; c.lane[3].eucS = 8;
    }
    else if (name == "loop") { beatLanes(); c.startSample = 0; c.loopLen = 180000; }   // 2 bars at 128 BPM
    else if (name == "midi" || name == "exact_midi" || name == "exact_midi_r4" || name == "onsets_midi")
    {
        c.midiEnabled = true;
        c.lane[0].subdiv = 2; c.lane[1].subdiv = 1; c.lane[1].reverse = true; c.lane[2].subdiv = 3; c.lane[2].pingPong = true;
        c.lane[3].subdiv = 4; c.lane[3].pitch = -5; c.lane[3].filter = 35;
        c.startSample = 4800;
        Lcg r { 777 };
        const int notes[5] = { 60, 61, 62, 63, 67 };
        if (name == "midi")
        {
            noisyTape();
            c.bpm = 128.0; c.sequencer = true; stepsOff(c.lane[2], { 2, 3, 7 });
            c.lane[1].prob = 70; c.lane[0].prEn = true; c.lane[0].prMin = -3; c.lane[0].prMax = 7;
            for (int k = 0; k < 120; ++k) c.notes.push_back({ (int) (r.next() % (kLen - 2000)), notes[r.next() % 5] });
            for (int k = 0; k < 15; ++k)   // chords: 2-3 lane notes on one sample
            {
                const int at = (int) (r.next() % (kLen - 2000));
                const int n = 2 + (int) (r.next() % 2);
                for (int j = 0; j < n; ++j) c.notes.push_back({ at, 60 + (int) ((r.next() + j) % 4) });
            }
        }
        else if (name == "exact_midi" || name == "exact_midi_r4")
        {
            // Offsets on multiples of 960 = lcm(32, 480): sample 0 of a block at both sizes.
            // Every cycle (subdivision x repeats) must also end on a block start:
            // v1.16.0 began the fade-out there. 1/8T x 3 = 24000; x 4 = 32000 does not
            // divide by 480 (the _r4 control).
            c.lane[3].repeats = name == "exact_midi" ? 3 : 4;
            c.tapeBypass = false; c.sat = 25; c.wow = 30; c.rolloff = 15;
            for (int at = 9600; at < kLen - 4000; at += 960 * (3 + (int) (r.next() % 20)))
                c.notes.push_back({ at, notes[r.next() % 5] });
        }
        else   // onsets_midi: sparse single notes at random offsets, lanes idle again before the next
        {
            c.lane[0].repeats = c.lane[1].repeats = c.lane[2].repeats = c.lane[3].repeats = 2;
            for (int at = 7001; at < kLen - 48000; at += 48000 + (int) (r.next() % 9000))
                c.notes.push_back({ at, notes[r.next() % 5] });
        }
    }
    else if (name == "exact_beat" || name == "onsets_beat")
    {
        // 120 BPM at 48 kHz: 1/8 = 12000 and 1/4 = 24000 samples, multiples of 32 and 480
        c.sequencer = true;
        c.lane[0].subdiv = 1; c.lane[0].repeats = 4; c.lane[0].decay = 85; c.lane[0].pan = -40;
        stepsOff(c.lane[0], { 1, 2, 3, 5, 6, 7, 9, 13, 14 });
        c.lane[1].subdiv = 0; c.lane[1].repeats = 2; c.lane[1].reverse = true;
        stepsOff(c.lane[1], { 1, 3, 4, 6, 7, 9 });
        c.lane[2].subdiv = 1; c.lane[2].pingPong = true; c.lane[2].pitch = 7; c.lane[2].filter = 40;
        stepsOff(c.lane[2], { 0, 2, 3, 4, 6, 8, 10, 11 });
        c.lane[3].subdiv = 0; c.lane[3].pitch = -5; c.lane[3].repeats = 3; c.lane[3].filter = -30;
        stepsOff(c.lane[3], { 1, 2, 5, 6, 9 });
        c.tapeBypass = false; c.sat = 25; c.wow = 30; c.rolloff = 15;
        c.startSample = 4800;   // first block holds no boundary: v1.16.0 skips the first playing block
        if (name == "onsets_beat") { c.bpm = 128.0; c.sequencer = false; c.tapeBypass = true; c.startSample = 1234; }
    }
    else { std::fprintf(stderr, "unknown config %s\n", name.c_str()); std::exit(2); }
    return c;
}

static float inputSample(int ch, int i)
{
    static std::vector<float> data[2];
    if (data[0].empty())
    {
        Lcg r { 12345 };
        for (auto& d : data) d.resize(kLen);
        for (int n = 0; n < kLen; ++n)
        {
            const double t = n / kFs;
            const double click = std::exp(-(double) (n % 17760) / 300.0);   // decaying click every 0.37 s
            data[0][n] = (float) (0.3 * std::sin(2 * M_PI * 220.0 * t) * (0.5 + 0.5 * std::sin(2 * M_PI * 0.7 * t))
                                + 0.25 * click * std::sin(2 * M_PI * 1800.0 * t) + 0.08 * (r.uni() * 2.0f - 1.0f));
            data[1][n] = (float) (0.25 * std::sin(2 * M_PI * (330.0 + 90.0 * std::sin(0.4 * t)) * t)
                                + 0.2 * click * std::sin(2 * M_PI * 2500.0 * t) + 0.08 * (r.uni() * 2.0f - 1.0f));
        }
    }
    return data[ch][(size_t) i];
}

int main(int argc, char** argv)
{
    if (argc < 5) { std::fprintf(stderr, "bad args\n"); return 2; }
    const Config c = makeConfig(argv[1]);
    const std::string block = argv[2];
    const uint64_t seed = std::strtoull(argv[3], nullptr, 10);
    FILE* onsets = argc > 5 ? std::fopen(argv[5], "w") : nullptr;

    juce::dsp::ProcessSpec spec { kFs, (juce::uint32) kMaxBlock, 2 };
    RepeatLane lanes[4];
    for (auto& l : lanes) l.prepare(spec);
    TapeDegrader tape; tape.prepare(spec);
    juce::AudioBuffer<float> buf(2, kMaxBlock), dry(2, kMaxBlock), laneBuf[4];
    for (auto& b : laneBuf) b.setSize(2, kMaxBlock);
#ifdef NEW_ENGINE
    TriggerScheduler sched; sched.reset();
#else
    TriggerRouter router; router.reset();
    double lastPPQPosition = 0.0; bool wasPlaying = false;
    juce::ignoreUnused(seed);
#endif

    std::vector<float> out((size_t) kLen * 2);
    Lcg br { 4242 };
    juce::MidiBuffer midi;
    size_t ni = 0;
    auto notes = c.notes;
    std::stable_sort(notes.begin(), notes.end(), [](auto& a, auto& b) { return a.first < b.first; });
    const double ppqPerSample = c.bpm / (60.0 * kFs);

    for (int pos = 0; pos < kLen;)
    {
        int n = block == "var" ? 1 + (int) (br.next() % kMaxBlock) : std::atoi(block.c_str());
        if (block == "var" && br.next() % 13 == 0) n = 1;
        n = std::min(n, kLen - pos);
        long long tpos = (long long) c.startSample + pos;   // transport sample
        if (c.loopLen > 0)
        {
            tpos %= c.loopLen;
            n = (int) std::min<long long>(n, c.loopLen - tpos);   // hosts split the block at the loop end
        }
        const double ppq = (double) tpos * c.bpm / (60.0 * kFs);

        buf.setSize(2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i) buf.setSample(ch, i, inputSample(ch, pos + i));
        midi.clear();
        while (ni < notes.size() && notes[ni].first < pos + n)
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, notes[ni].second, (juce::uint8) 100), notes[ni].first - pos);
            ++ni;
        }

        // ========== Lane parameters (PluginProcessor::processBlock order) ==========
        for (int li = 0; li < 4; ++li)
        {
            const LaneP& p = c.lane[li]; RepeatLane& l = lanes[li];
            l.setEnabled(p.enabled); l.setSubdivision(p.subdiv); l.setRepeats(p.repeats); l.setDecay(p.decay);
            l.setVolume(p.volume); l.setPan(p.pan); l.setProbability(p.prob); l.setSwing(p.swing);
            l.setPitch(p.pitch); l.setFilter(p.filter);
            l.setPitchRandEnabled(p.prEn); l.setPitchRandMin(p.prMin); l.setPitchRandMax(p.prMax); l.setPitchRandQuantize(p.prQ);
            l.setPingPong(p.pingPong); l.setReverse(p.reverse); l.setManualTimeEnabled(false);
            l.setEuclideanEnabled(p.euc); l.setEuclideanPulses(p.eucP); l.setEuclideanSteps(p.eucS);
            if (!p.euc)
                for (int s = 0; s < 16; ++s) l.setPatternStep(s, c.sequencer ? p.steps[s] : true);
        }

        auto logOnset = [&](int li, int offset, const char* kind)
        { if (onsets) std::fprintf(onsets, "%d %d %s\n", li, pos + offset, kind); };

#ifdef NEW_ENGINE
        // ========== v1.17.0: tempo, pattern position, scheduling ==========
        for (int li = 0; li < 4; ++li) lanes[li].setRandomSeed(seed, (uint64_t) li);
        tape.setRandomSeed(seed);
        const double bpm = juce::jlimit(20.0, 999.0, c.bpm);
        for (auto& l : lanes) l.updateTempo(bpm, kFs);
        for (int li = 0; li < 4; ++li) lanes[li].updatePatternPosition(ppq, c.lane[li].subdiv);

        TriggerScheduler::BlockInfo info;
        info.numSamples = n; info.sampleRate = kFs; info.bpm = bpm;
        info.hasPosition = true; info.hasPpq = true; info.ppq = ppq; info.isPlaying = true;
        info.midiEnabled = c.midiEnabled; info.manualEdge = false;
        for (int li = 0; li < 4; ++li) { info.laneEnabled[li] = c.lane[li].enabled; info.laneSubdiv[li] = c.lane[li].subdiv; }
        sched.schedule(info, midi);
        for (int li = 0; li < 4; ++li)
            for (int e = 0; e < sched.getNumEvents(li); ++e)
                logOnset(li, sched.getEvent(li, e).offset, sched.getEvent(li, e).kind == TriggerScheduler::Kind::Midi ? "midi" : "beat");

        for (int ch = 0; ch < 2; ++ch) juce::FloatVectorOperations::copy(dry.getWritePointer(ch), buf.getReadPointer(ch), n);
        for (int li = 0; li < 4; ++li)
        {
            laneBuf[li].setSize(2, n, false, false, true);
            laneBuf[li].clear();
            if (c.lane[li].enabled)
            {
                for (int ch = 0; ch < 2; ++ch) juce::FloatVectorOperations::copy(laneBuf[li].getWritePointer(ch), dry.getReadPointer(ch), n);
                sched.renderLane(lanes[li], li, c.lane[li].subdiv, laneBuf[li], n);
            }
        }
#else
        // ========== v1.16.0: MIDI triggers at block start (TriggerRouter) ==========
        router.setMidiEnabled(c.midiEnabled);
        router.processMidiTriggerDetection(midi);
        const int midiLane = router.getMidiTriggeredLane();
        if (midiLane >= 0)
        {
            if (midiLane == 100)
            {
                for (int li = 0; li < 4; ++li)
                    if (c.lane[li].enabled && !lanes[li].isRepeating()) { lanes[li].trigger(); logOnset(li, 0, "midi"); }
            }
            else if (c.lane[midiLane].enabled && !lanes[midiLane].isRepeating())
            {
                lanes[midiLane].trigger(); logOnset(midiLane, 0, "midi");
            }
        }
        for (int li = 0; li < 4; ++li) lanes[li].updatePatternPosition(ppq, c.lane[li].subdiv);

        // updateBeatSync (v1.16.0)
        {
            const double bpm = juce::jlimit(20.0, 999.0, c.bpm);
            for (auto& l : lanes) l.updateTempo(bpm, kFs);
            const bool isPlaying = true;
            if (c.midiEnabled) { lastPPQPosition = ppq; wasPlaying = isPlaying; }
            else
            {
                auto getSubdivisionPPQ = [](int i) { switch (i) { case 0: return 1.0; case 1: return 0.5; case 2: return 0.25;
                                                                  case 3: return 0.125; case 4: return 1.0 / 3.0; case 5: return 1.0 / 6.0;
                                                                  default: return 0.25; } };
                if (wasPlaying)
                    for (int li = 0; li < 4; ++li)
                        if (c.lane[li].enabled)
                        {
                            const double s = getSubdivisionPPQ(c.lane[li].subdiv);
                            if (static_cast<int>(ppq / s) > static_cast<int>(lastPPQPosition / s)) { lanes[li].trigger(); logOnset(li, 0, "beat"); }
                        }
                lastPPQPosition = ppq; wasPlaying = isPlaying;
            }
        }

        for (int ch = 0; ch < 2; ++ch) juce::FloatVectorOperations::copy(dry.getWritePointer(ch), buf.getReadPointer(ch), n);
        for (int li = 0; li < 4; ++li)
        {
            laneBuf[li].setSize(2, n, false, false, true);
            laneBuf[li].clear();
            if (c.lane[li].enabled)
            {
                for (int ch = 0; ch < 2; ++ch) juce::FloatVectorOperations::copy(laneBuf[li].getWritePointer(ch), dry.getReadPointer(ch), n);
                lanes[li].processBlock(laneBuf[li], n);
            }
        }
#endif
        juce::ignoreUnused(ppqPerSample);

        // ========== Mix (unchanged) ==========
        int active = 0;
        for (auto& l : c.lane) active += l.enabled ? 1 : 0;
        const float dryGain = c.dry / 100.0f, wetPerLane = active > 0 ? (c.wet / 100.0f) / (float) active : 0.0f;
        for (int ch = 0; ch < 2; ++ch)
        {
            juce::FloatVectorOperations::multiply(buf.getWritePointer(ch), dry.getReadPointer(ch), dryGain, n);
            for (auto& lb : laneBuf) juce::FloatVectorOperations::addWithMultiply(buf.getWritePointer(ch), lb.getReadPointer(ch), wetPerLane, n);
        }
        if (!c.tapeBypass)
        {
            tape.setSaturation(c.sat); tape.setWow(c.wow); tape.setFlutter(c.flutter);
            tape.setHiss(c.hiss); tape.setRolloff(c.rolloff); tape.setDropout(c.dropout);
            tape.processBlock(buf, n);
        }
        for (int i = 0; i < n; ++i)
        {
            out[(size_t) (pos + i)] = buf.getSample(0, i);
            out[(size_t) (kLen + pos + i)] = buf.getSample(1, i);
        }
        pos += n;
    }
    if (onsets) std::fclose(onsets);
    FILE* f = std::fopen(argv[4], "wb");
    std::fwrite(out.data(), sizeof(float), out.size(), f);
    std::fclose(f);
    return 0;
}

namespace juce { extern const char* const juce_compilationDate; extern const char* const juce_compilationTime; const char* const juce_compilationDate = __DATE__; const char* const juce_compilationTime = __TIME__; }
