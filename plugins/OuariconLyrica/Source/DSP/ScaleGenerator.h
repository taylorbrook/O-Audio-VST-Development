/*
  ==============================================================================

    ScaleGenerator.h
    v1.14.0: Scale Generation Utilities

    Provides mathematical scale generators for creating custom tunings:
    - EDO (Equal Division of Octave/Period)
    - Harmonic Series (from overtone ratios)
    - Rank-2 Temperaments (stacked generator intervals)

  ==============================================================================
*/

#pragma once
#include <vector>
#include <string>

/**
 * ScaleGenerator: Static utility class for generating microtonal scales
 *
 * All methods return vectors of cents values, starting with 0.0 (unison)
 * and ending with the period (typically 1200.0 for octave-repeating scales).
 */
class ScaleGenerator
{
public:
    /**
     * Generate Equal Division of a period (EDO/ED2, etc.)
     *
     * Creates N equal steps within the specified period.
     * Common uses:
     *   - 12-EDO (standard): generateEDO(12, 1200.0)
     *   - 19-EDO: generateEDO(19, 1200.0)
     *   - Bohlen-Pierce: generateEDO(13, 1902.0) (tritave)
     *
     * @param divisions Number of equal divisions (2-72)
     * @param period Period in cents (default 1200.0 = octave)
     * @return Vector of cents [0, step, 2*step, ..., period]
     */
    static std::vector<double> generateEDO(int divisions, double period = 1200.0);

    /**
     * Generate scale from harmonic series
     *
     * Creates a scale using ratios from consecutive harmonics.
     * Example: generateHarmonicSeries(8, 16) produces harmonics 8:9:10:11:12:13:14:15:16
     * reduced to one octave.
     *
     * @param startHarmonic First harmonic number (1-32, typically 4, 8, or 16)
     * @param endHarmonic Last harmonic number (must be > startHarmonic)
     * @return Vector of cents values (reduced to one octave, sorted, with period)
     */
    static std::vector<double> generateHarmonicSeries(int startHarmonic, int endHarmonic);

    /**
     * Generate Rank-2 temperament (meantone-like)
     *
     * Creates a scale by stacking a generator interval within a period.
     * The generator is applied alternatingly above and below the starting pitch.
     *
     * Common generators:
     *   - 700.0: Pythagorean (pure fifths)
     *   - 696.578: 1/4-comma meantone
     *   - 697.654: 1/6-comma meantone
     *   - 701.955: 12-TET fifth
     *
     * @param generatorCents Generator interval in cents
     * @param periodCents Period in cents (typically 1200.0)
     * @param count Number of notes to generate (3-31)
     * @return Vector of cents values (reduced to one period, sorted, with period)
     */
    static std::vector<double> generateRank2(double generatorCents, double periodCents, int count);

    /**
     * Get description string for generated scale
     */
    static std::string getEDODescription(int divisions, double period);
    static std::string getHarmonicDescription(int start, int end);
    static std::string getRank2Description(double generator, double period, int count);

private:
    /**
     * Reduce cents values to within [0, period) and sort
     * Removes near-duplicates (within 0.1 cent tolerance)
     * Appends the period at the end
     */
    static std::vector<double> reduceAndSort(std::vector<double>& cents, double period);
};
