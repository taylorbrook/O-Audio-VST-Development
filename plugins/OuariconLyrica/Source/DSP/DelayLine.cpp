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

    // OPTIMIZED: Single addition instead of while loop for negative wrap
    // Since bufferSize is power-of-2, we can add it once and use bitwise AND
    float readPosition = static_cast<float>(writePosition) - delayInSamples + static_cast<float>(bufferSize);

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

    // OPTIMIZED: Cache bufferSize mask for reuse
    const int mask = bufferSize - 1;

    // Get 4 surrounding samples for 3rd order interpolation
    float y0 = bufferData[(index - 1) & mask];
    float y1 = bufferData[index & mask];
    float y2 = bufferData[(index + 1) & mask];
    float y3 = bufferData[(index + 2) & mask];

    // OPTIMIZED: Precompute reciprocals (constants, no division at runtime)
    constexpr float oneThird = 1.0f / 3.0f;
    constexpr float oneHalf = 0.5f;
    constexpr float oneSixth = 1.0f / 6.0f;

    // Lagrange 3rd order polynomial coefficients
    float c0 = y1;
    float c1 = y2 - y0 * oneThird - y1 * oneHalf - y3 * oneSixth;
    float c2 = (y0 - y1 - y1 + y2) * oneHalf;
    float c3 = (y3 - y2 - y2 - y2 + y1 + y1 + y1 - y0) * oneSixth;

    // OPTIMIZED: Horner's method for polynomial evaluation
    // c0 + frac * (c1 + frac * (c2 + frac * c3))
    return c0 + frac * (c1 + frac * (c2 + frac * c3));
}

int DelayLine::nextPowerOfTwo(int value)
{
    // OPTIMIZED: Bit manipulation instead of while loop
    // Rounds up to next power of 2 in constant time
    if (value <= 1) return 1;
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return value + 1;
}
