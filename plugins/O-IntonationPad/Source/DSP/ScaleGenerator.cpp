/*
  ==============================================================================

    ScaleGenerator.cpp
    scala-tuning-engine module v2.0.0

  ==============================================================================
*/

#include "ScaleGenerator.h"
#include <algorithm>
#include <cmath>

std::vector<double> ScaleGenerator::generateEDO(int divisions, double period)
{
    // Clamp to reasonable range
    divisions = std::max(2, std::min(72, divisions));
    period = std::max(100.0, std::min(2400.0, period));

    std::vector<double> intervals;
    intervals.reserve(static_cast<size_t>(divisions) + 1);

    double step = period / static_cast<double>(divisions);

    for (int i = 0; i <= divisions; ++i)
    {
        intervals.push_back(static_cast<double>(i) * step);
    }

    return intervals;
}

std::vector<double> ScaleGenerator::generateHarmonicSeries(int startHarmonic, int endHarmonic)
{
    // Validate inputs
    startHarmonic = std::max(1, std::min(32, startHarmonic));
    endHarmonic = std::max(startHarmonic + 1, std::min(64, endHarmonic));

    std::vector<double> intervals;
    intervals.push_back(0.0);  // Unison (1/1)

    // Generate cents for each harmonic ratio
    for (int h = startHarmonic + 1; h <= endHarmonic; ++h)
    {
        double ratio = static_cast<double>(h) / static_cast<double>(startHarmonic);
        double cents = 1200.0 * std::log2(ratio);
        intervals.push_back(cents);
    }

    // Reduce to one octave, sort, and add period
    return reduceAndSort(intervals, 1200.0);
}

std::vector<double> ScaleGenerator::generateRank2(double generatorCents, double periodCents, int count)
{
    // Validate inputs
    generatorCents = std::max(1.0, std::min(periodCents - 1.0, generatorCents));
    periodCents = std::max(100.0, std::min(2400.0, periodCents));
    count = std::max(3, std::min(31, count));

    std::vector<double> intervals;
    intervals.push_back(0.0);  // Unison

    // Generate by stacking the generator interval
    // Use "spiral of fifths" approach: 0, +1g, -1g, +2g, -2g, ...
    for (int i = 1; i < count; ++i)
    {
        int step = (i + 1) / 2;
        if (i % 2 == 0)
            step = -step;

        double cents = static_cast<double>(step) * generatorCents;
        intervals.push_back(cents);
    }

    // Reduce to one period, sort, and add period
    return reduceAndSort(intervals, periodCents);
}

std::vector<double> ScaleGenerator::reduceAndSort(std::vector<double>& cents, double period)
{
    // Reduce all values to within [0, period)
    for (auto& c : cents)
    {
        while (c < 0.0)
            c += period;
        while (c >= period)
            c -= period;
    }

    // Sort in ascending order
    std::sort(cents.begin(), cents.end());

    // Remove near-duplicates (within 0.1 cent tolerance)
    auto last = std::unique(cents.begin(), cents.end(),
        [](double a, double b) { return std::abs(a - b) < 0.1; });
    cents.erase(last, cents.end());

    // Add period at end
    cents.push_back(period);

    return cents;
}

