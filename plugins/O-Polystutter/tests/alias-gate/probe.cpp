// v1.19.0 alias / bit-exact / block-size probe: one RepeatLane, offline.
// usage: probe <fs> <bpm> <subdiv> <reps> <pitchSt> <randSt|none|lo:hi> <reverse> <swing>
//              <input: sine:<Hz> | noise> <block: N | var> <triggers: a,b,c> <len> <out.f32>
// Output: the lane's wet stereo, interleaved L R (float32).
// randSt: pitch random enabled with min = max = randSt, unquantized (adds randSt to
// pitch; the only way to reach > +12 st). lo:hi: random in [lo, hi], unquantized.
#include <JuceHeader.h>
#include "RepeatLane.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
int main(int argc, char** argv)
{
    if (argc != 14) { std::fprintf(stderr, "bad args\n"); return 2; }
    const double fs = std::atof(argv[1]), bpm = std::atof(argv[2]);
    const int subdiv = std::atoi(argv[3]), reps = std::atoi(argv[4]);
    const float pitch = (float) std::atof(argv[5]);
    const std::string rnd = argv[6];
    const bool rev = std::atoi(argv[7]) != 0; const float swing = (float) std::atof(argv[8]);
    const std::string in = argv[9], blk = argv[10];
    std::vector<int> trig; { std::string t = argv[11]; size_t p = 0; while (p < t.size()) { size_t q = t.find(',', p); if (q == std::string::npos) q = t.size(); trig.push_back(std::atoi(t.substr(p, q - p).c_str())); p = q + 1; } }
    const int len = std::atoi(argv[12]);
    RepeatLane lane; lane.prepare({ fs, 4096u, 2 });
    lane.setRandomSeed(7, 0); lane.updateTempo(bpm, fs);
    lane.setSubdivision(subdiv); lane.setRepeats(reps); lane.setDecay(100); lane.setVolume(100);
    lane.setPan(0); lane.setProbability(100); lane.setSwing(swing); lane.setReverse(rev);
    if (rnd != "none")
    {
        const size_t c = rnd.find(':');
        const float lo = (float) std::atof(rnd.substr(0, c).c_str());
        const float hi = c == std::string::npos ? lo : (float) std::atof(rnd.substr(c + 1).c_str());
        lane.setPitchRandEnabled(true); lane.setPitchRandMin(lo); lane.setPitchRandMax(hi); lane.setPitchRandQuantize(false);
    }
    lane.setPitch(pitch);
    const bool sine = in.rfind("sine:", 0) == 0; const double f = sine ? std::atof(in.c_str() + 5) : 0.0;
    uint32_t lcg = 12345;
    std::vector<float> out((size_t) len * 2);
    juce::AudioBuffer<float> buf(2, 4096);
    uint32_t vb = 999; int s0 = 0; size_t ti = 0;
    const auto t0 = std::chrono::steady_clock::now();
    while (s0 < len)
    {
        int B = blk == "var" ? 1 + (int) ((vb = vb * 1664525u + 1013904223u) >> 20) % 700 : std::atoi(blk.c_str());
        B = std::min(B, len - s0);
        for (int i = 0; i < B; ++i)
        {
            float l, r;
            if (sine) { l = r = 0.5f * (float) std::sin(2 * M_PI * f * (double) (s0 + i) / fs); }
            else { lcg = lcg * 1664525u + 1013904223u; l = ((lcg >> 8) / 16777216.0f - 0.5f); lcg = lcg * 1664525u + 1013904223u; r = ((lcg >> 8) / 16777216.0f - 0.5f); }
            buf.setSample(0, i, l); buf.setSample(1, i, r);
        }
        int k = 0;
        while (ti < trig.size() && trig[ti] < s0 + B)
        {
            const int at = trig[ti] - s0;
            if (at > k) lane.processBlock(buf, k, at - k);
            lane.trigger(); k = std::max(k, at); ++ti;
        }
        if (k < B) lane.processBlock(buf, k, B - k);
        for (int i = 0; i < B; ++i) { out[2 * (size_t) (s0 + i)] = buf.getSample(0, i); out[2 * (size_t) (s0 + i) + 1] = buf.getSample(1, i); }
        s0 += B;
    }
    const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
    std::fprintf(stderr, "ns/sample %.1f\n", 1000.0 * us / len);
    FILE* fo = std::fopen(argv[13], "wb"); std::fwrite(out.data(), 4, out.size(), fo); std::fclose(fo);
}
namespace juce { extern const char* const juce_compilationDate; extern const char* const juce_compilationTime; const char* const juce_compilationDate = __DATE__; const char* const juce_compilationTime = __TIME__; }
