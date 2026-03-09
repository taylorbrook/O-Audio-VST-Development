#pragma once

#include <JuceHeader.h>

struct SyncInfo
{
    double bpm = 120.0;
    double ppqPosition = 0.0;
    double ppqPerSample = 0.0;
    bool isPlaying = false;
};

class TempoTracker
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        manualPpq = 0.0;
        lastPpq = 0.0;
    }

    SyncInfo update (juce::AudioPlayHead* playHead, int numSamples)
    {
        SyncInfo info;

        bool gotPosition = false;

        if (playHead != nullptr)
        {
            auto pos = playHead->getPosition();
            if (pos.hasValue())
            {
                auto bpmOpt = pos->getBpm();
                auto ppqOpt = pos->getPpqPosition();
                auto playing = pos->getIsPlaying();

                if (bpmOpt.hasValue() && ppqOpt.hasValue())
                {
                    info.bpm = *bpmOpt;
                    info.ppqPosition = *ppqOpt;
                    info.isPlaying = playing;
                    info.ppqPerSample = info.bpm / (60.0 * sampleRate);

                    lastPpq = info.ppqPosition;
                    gotPosition = true;
                }
            }
        }

        if (!gotPosition)
        {
            // Standalone fallback: 120 BPM manual counter
            info.bpm = 120.0;
            info.ppqPerSample = 120.0 / (60.0 * sampleRate);
            info.ppqPosition = manualPpq;
            info.isPlaying = true;

            manualPpq += info.ppqPerSample * numSamples;
            lastPpq = manualPpq;
        }

        return info;
    }

private:
    double sampleRate = 44100.0;
    double manualPpq = 0.0;
    double lastPpq = 0.0;
};
