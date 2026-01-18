/*
  ==============================================================================

    TuningEngine.cpp
    Phase 2.8: Tuning Engine Implementation

  ==============================================================================
*/

#include "TuningEngine.h"

TuningEngine::TuningEngine()
{
    // Initialize all pitch bends to sentinel value (no bend)
    for (auto& bend : notePitchBends)
        bend.store(NO_BEND, std::memory_order_relaxed);
}

void TuningEngine::setMasterTune(double freqHz)
{
    // Clamp to reasonable range
    a4Frequency = juce::jlimit(400.0, 480.0, freqHz);
}

void TuningEngine::setPitchBendRange(float semitones)
{
    // Clamp to reasonable range (1-48 semitones)
    pitchBendRange = juce::jlimit(1.0f, 48.0f, semitones);
}

double TuningEngine::getFrequency(int midiNote, int midiChannel)
{
    juce::ignoreUnused(midiChannel); // Reserved for future MTS-ESP per-channel tuning

    // Clamp MIDI note to valid range
    midiNote = juce::jlimit(0, 127, midiNote);

    // Calculate base 12-TET frequency
    double baseFreq = calculate12TETFrequency(midiNote);

    // Thread-safe read of pitch bend (lock-free atomic access)
    // NO_BEND sentinel is 2.0f (outside valid range -1.0 to 1.0)
    float bendAmount = notePitchBends[static_cast<size_t>(midiNote)].load(std::memory_order_relaxed);
    if (bendAmount <= 1.0f && bendAmount >= -1.0f)  // Check if within valid range (not sentinel)
    {
        return applyPitchBend(baseFreq, bendAmount);
    }

    return baseFreq;
}

void TuningEngine::setPitchBend(int midiNote, float bendAmount)
{
    // Clamp MIDI note and bend amount
    midiNote = juce::jlimit(0, 127, midiNote);
    bendAmount = juce::jlimit(-1.0f, 1.0f, bendAmount);

    // Thread-safe write (lock-free atomic access)
    notePitchBends[static_cast<size_t>(midiNote)].store(bendAmount, std::memory_order_relaxed);
}

void TuningEngine::clearPitchBend(int midiNote)
{
    midiNote = juce::jlimit(0, 127, midiNote);
    // Thread-safe clear (set to sentinel value)
    notePitchBends[static_cast<size_t>(midiNote)].store(NO_BEND, std::memory_order_relaxed);
}

void TuningEngine::clearAllPitchBends()
{
    // Thread-safe clear all (set all to sentinel value)
    for (auto& bend : notePitchBends)
        bend.store(NO_BEND, std::memory_order_relaxed);
}

std::vector<double> TuningEngine::getScaleFrequencies(int rootNote, int numNotes)
{
    std::vector<double> frequencies;
    frequencies.reserve(numNotes);

    // 12-TET chromatic scale (all semitones)
    // For scale-locked glissando, this provides all available pitches
    // The GlissandoController can filter to specific scale degrees
    for (int i = 0; i < numNotes; ++i)
    {
        int midiNote = rootNote + i;
        if (midiNote >= 0 && midiNote <= 127)
        {
            frequencies.push_back(calculate12TETFrequency(midiNote));
        }
    }

    return frequencies;
}

bool TuningEngine::loadScalaFile(const juce::File& scl, const juce::File& kbm)
{
    juce::ignoreUnused(scl, kbm);

    // PLACEHOLDER: Not implemented in Phase 2.8
    // Future implementation would:
    // 1. Parse .scl file for scale degrees
    // 2. Parse .kbm file for keyboard mapping
    // 3. Store tuning table
    // 4. Set scalaFileLoaded = true

    DBG("TuningEngine::loadScalaFile() - Not implemented (optional feature)");
    return false;
}

bool TuningEngine::connectMTSClient()
{
    // PLACEHOLDER: Not implemented in Phase 2.8
    // Future implementation would:
    // 1. Initialize libMTSClient
    // 2. Register as MTS-ESP client
    // 3. Set mtsSynthClientConnected = true

    DBG("TuningEngine::connectMTSClient() - Not implemented (optional feature)");
    return false;
}

double TuningEngine::calculate12TETFrequency(int midiNote) const
{
    // 12-TET formula: f = A4 * 2^((n - 69) / 12)
    // where n is MIDI note number, A4 = a4Frequency (default 440 Hz)

    const double semitonesFromA4 = static_cast<double>(midiNote - 69);
    const double frequency = a4Frequency * std::pow(2.0, semitonesFromA4 / 12.0);

    return frequency;
}

double TuningEngine::applyPitchBend(double baseFreq, float bendAmount) const
{
    // Convert bend amount (-1.0 to +1.0) to semitones
    const double bendSemitones = static_cast<double>(bendAmount * pitchBendRange);

    // Apply pitch bend using equal temperament formula
    // f_bent = f_base * 2^(bend_semitones / 12)
    const double bentFreq = baseFreq * std::pow(2.0, bendSemitones / 12.0);

    return bentFreq;
}
