// v1.18.3 pitched-down repeat click probe: one RepeatLane, stereo sine, one trigger,
// output = lane wet only. usage: probe <pitchSt> <freqHz> <subdiv> <repeats> <reverse> <swing> <out.f32>
#include <JuceHeader.h>
#include "RepeatLane.h"
#include <cmath>
#include <cstdio>
int main(int argc, char** argv)
{
    const float pitch = std::atof(argv[1]); const double f = std::atof(argv[2]);
    const int subdiv = std::atoi(argv[3]), reps = std::atoi(argv[4]);
    const bool rev = std::atoi(argv[5]); const float swing = std::atof(argv[6]);
    const double fs = 48000; const int len = 96000, trig = 48000, B = 256;
    RepeatLane lane; lane.prepare({ fs, (juce::uint32) B, 2 });
    lane.setRandomSeed(1, 0); lane.updateTempo(120.0, fs);
    lane.setSubdivision(subdiv); lane.setRepeats(reps); lane.setDecay(100); lane.setVolume(100);
    lane.setPan(0); lane.setProbability(100); lane.setSwing(swing); lane.setReverse(rev); lane.setPitch(pitch);
    std::vector<float> out(len);
    juce::AudioBuffer<float> buf(2, B);
    for (int s0 = 0; s0 < len; s0 += B)
    {
        for (int i = 0; i < B; ++i) { float x = 0.5f * (float) std::sin(2 * M_PI * f * (s0 + i) / fs); buf.setSample(0, i, x); buf.setSample(1, i, x); }
        if (s0 <= trig && trig < s0 + B)
        {
            const int k = trig - s0;
            if (k > 0) lane.processBlock(buf, 0, k);
            lane.trigger(); lane.processBlock(buf, k, B - k);
        }
        else lane.processBlock(buf, 0, B);
        for (int i = 0; i < B; ++i) out[s0 + i] = buf.getSample(0, i);
    }
    FILE* fo = std::fopen(argv[7], "wb"); std::fwrite(out.data(), 4, len, fo); std::fclose(fo);
}
namespace juce { extern const char* const juce_compilationDate; extern const char* const juce_compilationTime; const char* const juce_compilationDate = __DATE__; const char* const juce_compilationTime = __TIME__; }
