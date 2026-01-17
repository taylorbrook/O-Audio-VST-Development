/*
  ==============================================================================

    DelayLine.cpp
    Efficient delay line with Lagrange interpolation for pitch-accurate waveguide
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "DelayLine.h"

DelayLine::DelayLine()
{
}

DelayLine::~DelayLine()
{
}

void DelayLine::prepare(double sampleRate, int maxDelayInSamples)
{
    currentSampleRate = sampleRate;

    // Round up to power of 2 for efficient modulo operations
    bufferSize = nextPowerOfTwo(maxDelayInSamples + 4); // +4 for interpolation safety

    // Allocate buffer
    buffer.setSize(1, bufferSize);
    clear();

    writePosition = 0;
    delayInSamples = 0.0f;
}

void DelayLine::setDelay(float delayInSamples)
{
    // Clamp to valid range
    this->delayInSamples = juce::jlimit(1.0f, static_cast<float>(bufferSize - 4), delayInSamples);
}

void DelayLine::pushSample(float sample)
{
    auto* bufferData = buffer.getWritePointer(0);

    // Write sample to current position
    bufferData[writePosition] = sample;

    // Advance write position (power-of-2 wrap)
    writePosition = (writePosition + 1) & (bufferSize - 1);
}

float DelayLine::popSample()
{
    auto* bufferData = buffer.getReadPointer(0);

    // Calculate read position (fractional)
    float readPosition = static_cast<float>(writePosition) - delayInSamples;

    // Wrap read position if negative
    while (readPosition < 0.0f)
        readPosition += static_cast<float>(bufferSize);

    // Lagrange interpolation for accurate pitch
    return lagrangeInterpolation(bufferData, readPosition);
}

void DelayLine::clear()
{
    buffer.clear();
    writePosition = 0;
}

float DelayLine::lagrangeInterpolation(const float* bufferData, float readPosition)
{
    // Get integer and fractional parts
    int index = static_cast<int>(readPosition);
    float frac = readPosition - static_cast<float>(index);

    // Get 4 surrounding samples for 3rd order interpolation
    int i0 = (index - 1) & (bufferSize - 1);
    int i1 = index & (bufferSize - 1);
    int i2 = (index + 1) & (bufferSize - 1);
    int i3 = (index + 2) & (bufferSize - 1);

    float y0 = bufferData[i0];
    float y1 = bufferData[i1];
    float y2 = bufferData[i2];
    float y3 = bufferData[i3];

    // Lagrange 3rd order polynomial interpolation
    // https://ccrma.stanford.edu/~jos/pasp/Lagrange_Interpolation.html
    float c0 = y1;
    float c1 = y2 - y0 / 3.0f - y1 / 2.0f - y3 / 6.0f;
    float c2 = y0 / 2.0f - y1 + y2 / 2.0f;
    float c3 = y3 / 6.0f - y2 / 2.0f + y1 / 2.0f - y0 / 6.0f;

    return c0 + c1 * frac + c2 * frac * frac + c3 * frac * frac * frac;
}

int DelayLine::nextPowerOfTwo(int value)
{
    // Find next power of 2
    int powerOfTwo = 1;
    while (powerOfTwo < value)
        powerOfTwo *= 2;
    return powerOfTwo;
}
