/*
  ==============================================================================

    TuningSystem.h
    Tuning system for just intonation and alternative temperaments
    Phase 2.3: Simple implementation (5-limit JI, Pythagorean, 12-TET)

  ==============================================================================
*/

#pragma once
#include <array>
#include <atomic>
#include <cmath>

class TuningSystem
{
public:
    enum class Mode
    {
        TwelveTET = 0,
        JustIntonation = 1,
        Pythagorean = 2,
        Historical = 3,
        Scala = 4
    };

    TuningSystem();

    // Set tuning mode
    void setMode(Mode mode);

    // Set tonic note (0=C, 1=C#, ..., 11=B)
    void setTonicNote(int tonicIndex);

    // Get frequency for MIDI note (thread-safe)
    double getFrequency(int midiNote) const;

    // Get frequency with cent offset (used for randomization)
    double getFrequencyWithOffset(int midiNote, double centOffset) const;

private:
    std::atomic<Mode> currentMode;
    std::atomic<int> tonicNote;

    // Pre-calculated cent tables for each tuning system
    static constexpr std::array<double, 12> twelveTETIntervals = {
        0.0, 100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0
    };

    // 5-limit Just Intonation (C major scale, simplified)
    static constexpr std::array<double, 12> justIntonationIntervals = {
        0.0,    // C (1:1)
        111.73, // C# (16:15)
        203.91, // D (9:8)
        315.64, // Eb (6:5)
        386.31, // E (5:4)
        498.04, // F (4:3)
        582.51, // F# (45:32)
        701.96, // G (3:2)
        813.69, // Ab (8:5)
        884.36, // A (5:3)
        1017.60, // Bb (16:9)
        1088.27  // B (15:8)
    };

    // Pythagorean tuning (3-limit)
    static constexpr std::array<double, 12> pythagoreanIntervals = {
        0.0,    // C
        90.22,  // C# (Pythagorean chromatic semitone)
        203.91, // D
        294.13, // Eb
        407.82, // E (Pythagorean major third - sharp!)
        498.04, // F
        611.73, // F#
        701.96, // G
        792.18, // Ab
        905.87, // A
        996.09, // Bb
        1109.78 // B
    };

    // Get cent offset for pitch class in current tuning
    double getCentOffset(int pitchClass) const;

    // Convert cents to frequency ratio
    static double centsToRatio(double cents);
};
