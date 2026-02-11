#pragma once

#include <JuceHeader.h>
#include "../Data/SpeakerLayout.h"
#include <vector>
#include <memory>

struct VBAPData
{
    std::vector<float> gainTable;
    std::vector<int> speakerToChannelMap;
    int numVBAPSpeakers = 0;
    int numOutputChannels = 0;
    int aziRes = 1;
    int elevRes = 1;
    bool is3D = false;
};

class VBAPDataExchange : public juce::Timer
{
public:
    VBAPDataExchange() { startTimerHz (10); }
    ~VBAPDataExchange() override { stopTimer(); }

    void setNewData (std::unique_ptr<VBAPData> newData)
    {
        const juce::SpinLock::ScopedLockType lock (spinLock);
        pending = std::move (newData);
    }

    void updateAudioThreadData()
    {
        const juce::SpinLock::ScopedTryLockType lock (spinLock);

        if (lock.isLocked() && pending != nullptr)
        {
            old = std::move (active);
            active = std::move (pending);
        }
    }

    VBAPData* getActiveData() { return active.get(); }

private:
    void timerCallback() override
    {
        // Clean up old data on message thread (not audio thread)
        old.reset();
    }

    juce::SpinLock spinLock;
    std::unique_ptr<VBAPData> pending;
    std::unique_ptr<VBAPData> active;
    std::unique_ptr<VBAPData> old;
};

class VBAPComputeThread : public juce::Thread
{
public:
    VBAPComputeThread (VBAPDataExchange& exchange)
        : juce::Thread ("VBAPCompute"), dataExchange (exchange) {}

    ~VBAPComputeThread() override
    {
        signalThreadShouldExit();
        notify();
        stopThread (2000);
    }

    void requestRecomputation (const SpeakerLayout& layout)
    {
        const juce::ScopedLock lock (requestLock);
        pendingLayout = layout;
        hasRequest = true;
        notify();
    }

    void run() override;

private:
    void computeGainTable (const SpeakerLayout& layout);

    VBAPDataExchange& dataExchange;
    juce::CriticalSection requestLock;
    SpeakerLayout pendingLayout;
    bool hasRequest = false;
};
