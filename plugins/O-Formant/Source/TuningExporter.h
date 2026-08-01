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

    TuningExporter.h
    scala-tuning-engine module v2.0.0

    Generates formatted HTML documents for tuning scales including:
    - Scale metadata (name, description, note count, period)
    - Interval table with cents, ratio approximation, and ET deviation
    - SVG pitch circle visualization
    - Generation metadata (date, source)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <string>

class TuningEngine;

/**
 * TuningExporter: Static utility class for exporting tunings to various formats
 */
class TuningExporter
{
public:
    /**
     * Generate HTML document for current tuning
     * @param engine Reference to TuningEngine for scale data
     * @param pluginName Name of the plugin generating the export (for branding)
     * @return Complete HTML document as string
     */
    static juce::String toHTML(const TuningEngine& engine, const juce::String& pluginName = "Ouaricon Audio");

    /**
     * Generate SVG pitch circle for current tuning
     * @param intervals Vector of intervals in cents
     * @param period Period in cents (1200.0 for octave)
     * @param scaleName Name of the scale
     * @return SVG markup as string
     */
    static juce::String generatePitchCircleSVG(const std::vector<double>& intervals,
                                                double period,
                                                const juce::String& scaleName);

    /**
     * Approximate a cents value as a simple ratio
     * @param cents Interval in cents
     * @return String representation of ratio (e.g., "3/2", "5/4")
     */
    static juce::String approximateRatio(double cents);

    /**
     * Calculate deviation from equal temperament
     * @param cents Actual interval in cents
     * @param degree Scale degree (0-based)
     * @param totalDegrees Total degrees in scale
     * @param period Period in cents
     * @return Deviation in cents (positive = sharp, negative = flat)
     */
    static double calculateETDeviation(double cents, int degree, int totalDegrees, double period);

private:
    // CSS styles for HTML export
    static juce::String getStyles();

    // HTML table row for interval
    static juce::String generateIntervalRow(int degree, double cents, int totalDegrees, double period);

    // Common simple ratios for approximation lookup
    struct RatioEntry {
        double cents;
        const char* ratio;
        const char* name;
    };
    static const std::vector<RatioEntry>& getCommonRatios();
};
