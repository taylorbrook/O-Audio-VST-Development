/*
   This file is part of O-IntonationPad, an Ouaricon Audio plugin.
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

private:
    static juce::String generatePitchCircleSVG(const std::vector<double>& intervals,
                                                double period);
    static juce::String approximateRatio(double cents);
    static double calculateETDeviation(double cents, int degree, int totalDegrees, double period);
    static juce::String getStyles();
    static juce::String generateIntervalRow(int degree, double cents, int totalDegrees, double period);

    struct RatioEntry {
        double cents;
        const char* ratio;
        const char* name;
    };
    static const std::vector<RatioEntry>& getCommonRatios();
};
