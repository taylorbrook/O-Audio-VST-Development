#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct Speaker
{
    float azimuth   = 0.0f;   // degrees, counter-clockwise: 0=front, +90=left
    float elevation = 0.0f;   // degrees, 0=horizon, +90=up
    float distance  = 1.0f;   // meters from center
    juce::String label;
    bool isLFE      = false;
};

struct SpeakerLayout
{
    juce::String name;
    std::vector<Speaker> speakers;
    bool is3D = false;

    int getChannelCount() const { return (int) speakers.size(); }
};
