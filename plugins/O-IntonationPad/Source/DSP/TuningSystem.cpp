/*
  ==============================================================================

    TuningSystem.cpp
    Implementation of tuning system

  ==============================================================================
*/

#include "TuningSystem.h"

TuningSystem::TuningSystem()
    : currentMode(Mode::JustIntonation)
    , tonicNote(0)  // C
{
}

void TuningSystem::setMode(Mode mode)
{
    currentMode.store(mode, std::memory_order_relaxed);
}

void TuningSystem::setTonicNote(int tonicIndex)
{
    tonicNote.store(tonicIndex, std::memory_order_relaxed);
}

double TuningSystem::getFrequency(int midiNote) const
{
    return getFrequencyWithOffset(midiNote, 0.0);
}

double TuningSystem::getFrequencyWithOffset(int midiNote, double centOffset) const
{
    // Calculate base frequency using 12-TET as reference
    // A4 (MIDI note 69) = 440 Hz
    double baseFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);

    // Get tuning-specific cent offset
    int pitchClass = midiNote % 12;
    int tonic = tonicNote.load(std::memory_order_relaxed);

    // Rotate pitch class relative to tonic
    int relativePitch = (pitchClass - tonic + 12) % 12;

    double tuningCents = getCentOffset(relativePitch);
    double totalCents = tuningCents + centOffset;

    // Apply cent offset
    return baseFreq * centsToRatio(totalCents);
}

double TuningSystem::getCentOffset(int pitchClass) const
{
    Mode mode = currentMode.load(std::memory_order_relaxed);

    switch (mode)
    {
        case Mode::TwelveTET:
            return twelveTETIntervals[pitchClass];

        case Mode::JustIntonation:
            return justIntonationIntervals[pitchClass];

        case Mode::Pythagorean:
            return pythagoreanIntervals[pitchClass];

        case Mode::Historical:
            // Use Pythagorean as fallback for now
            return pythagoreanIntervals[pitchClass];

        case Mode::Scala:
            // Use 12-TET as fallback for now
            return twelveTETIntervals[pitchClass];

        default:
            return twelveTETIntervals[pitchClass];
    }
}

double TuningSystem::centsToRatio(double cents)
{
    return std::pow(2.0, cents / 1200.0);
}
