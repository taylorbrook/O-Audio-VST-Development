/*
  ==============================================================================

    WavetableGenerator.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include "WavetableData.h"
#include <memory>

enum class WaveShape
{
    Saw = 0,
    Square,
    Triangle,
    Sine
};

class WavetableGenerator
{
public:
    static std::unique_ptr<WavetableData> generateProceduralTable (WaveShape shape);
    static void generateMipmaps (WavetableData& table);
    static void generateMipmapsForFrame (WavetableData& table, int frameIndex);

private:
    static void generateSaw (float* buffer, int size);
    static void generateSquare (float* buffer, int size);
    static void generateTriangle (float* buffer, int size);
    static void generateSine (float* buffer, int size);
};
