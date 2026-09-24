/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    ScaleGenerator.cpp
    scala-tuning-engine module v2.0.0

  ==============================================================================
*/

#include "ScaleGenerator.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

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
    // Validate inputs. CR-09: period FIRST — the generator used to be clamped
    // against the raw, unbounded period, so a typed 1e12 reached reduceAndSort.
    if (! std::isfinite(periodCents))    periodCents = 1200.0;
    if (! std::isfinite(generatorCents)) generatorCents = 700.0;
    periodCents = std::max(100.0, std::min(2400.0, periodCents));
    generatorCents = std::max(1.0, std::min(periodCents - 1.0, generatorCents));
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
    // CR-09: a non-positive or non-finite period cannot reduce anything.
    if (! std::isfinite(period) || period <= 0.0)
        period = 1200.0;

    // Reduce all values to within [0, period). fmod, not a subtract-one-period
    // loop: that was ~6e9 iterations for a 1e13 input, and never terminated
    // once c - period == c in double precision.
    for (auto& c : cents)
    {
        if (! std::isfinite(c))
            c = 0.0;
        c = std::fmod(c, period);
        if (c < 0.0)
            c += period;
        if (c >= period) // -tiny + period can round up to period
            c = 0.0;
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

std::string ScaleGenerator::getEDODescription(int divisions, double period)
{
    std::ostringstream oss;
    oss << divisions << "-EDO";

    if (std::abs(period - 1200.0) > 0.1)
    {
        oss << " (" << std::fixed << std::setprecision(0) << period << " period)";
    }

    return oss.str();
}

std::string ScaleGenerator::getHarmonicDescription(int start, int end)
{
    std::ostringstream oss;
    oss << "Harmonics " << start << "-" << end;
    return oss.str();
}

std::string ScaleGenerator::getRank2Description(double generator, double /*period*/, int count)
{
    std::ostringstream oss;
    oss << "Rank-2 (" << std::fixed << std::setprecision(1) << generator << "c";

    // Add common name hints
    if (std::abs(generator - 700.0) < 1.0)
        oss << " Pythagorean";
    else if (std::abs(generator - 696.578) < 1.0)
        oss << " 1/4-comma";
    else if (std::abs(generator - 697.654) < 1.0)
        oss << " 1/6-comma";

    oss << ", " << count << " notes)";

    return oss.str();
}
