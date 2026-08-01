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

    TuningExporter.cpp
    scala-tuning-engine module v2.0.0

  ==============================================================================
*/

#include "TuningExporter.h"
#include "TuningEngine.h"
#include <cmath>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════
// Common Ratios Database
// ═══════════════════════════════════════════════════════════════════

const std::vector<TuningExporter::RatioEntry>& TuningExporter::getCommonRatios()
{
    static const std::vector<RatioEntry> ratios = {
        {0.0, "1/1", "Unison"},
        {21.5, "81/80", "Syntonic comma"},
        {70.7, "25/24", "Small semitone"},
        {84.5, "21/20", "Minor semitone"},
        {90.2, "256/243", "Pythagorean limma"},
        {100.0, "2^(1/12)", "ET semitone"},
        {111.7, "16/15", "Just semitone"},
        {119.4, "15/14", "Major diatonic semitone"},
        {133.2, "27/25", "Large limma"},
        {150.6, "12/11", "Undecimal neutral second"},
        {165.0, "11/10", "Greater undecimal neutral second"},
        {182.4, "10/9", "Minor whole tone"},
        {200.0, "2^(2/12)", "ET whole tone"},
        {203.9, "9/8", "Major whole tone"},
        {231.2, "8/7", "Septimal whole tone"},
        {266.9, "7/6", "Septimal minor third"},
        {294.1, "32/27", "Pythagorean minor third"},
        {300.0, "2^(3/12)", "ET minor third"},
        {315.6, "6/5", "Just minor third"},
        {347.4, "11/9", "Undecimal neutral third"},
        {386.3, "5/4", "Just major third"},
        {400.0, "2^(4/12)", "ET major third"},
        {407.8, "81/64", "Pythagorean major third"},
        {435.1, "9/7", "Septimal major third"},
        {470.8, "21/16", "Narrow fourth"},
        {498.0, "4/3", "Perfect fourth"},
        {500.0, "2^(5/12)", "ET fourth"},
        {551.3, "11/8", "Undecimal tritone"},
        {582.5, "7/5", "Septimal tritone"},
        {600.0, "2^(6/12)", "ET tritone"},
        {617.5, "10/7", "Euler's tritone"},
        {648.7, "16/11", "Undecimal diminished fifth"},
        {700.0, "2^(7/12)", "ET fifth"},
        {702.0, "3/2", "Perfect fifth"},
        {729.2, "32/21", "Wide fifth"},
        {764.9, "14/9", "Septimal minor sixth"},
        {782.5, "11/7", "Undecimal minor sixth"},
        {800.0, "2^(8/12)", "ET minor sixth"},
        {813.7, "8/5", "Just minor sixth"},
        {852.6, "18/11", "Undecimal neutral sixth"},
        {884.4, "5/3", "Just major sixth"},
        {900.0, "2^(9/12)", "ET major sixth"},
        {905.9, "27/16", "Pythagorean major sixth"},
        {933.1, "12/7", "Septimal major sixth"},
        {968.8, "7/4", "Harmonic seventh"},
        {996.1, "16/9", "Pythagorean minor seventh"},
        {1000.0, "2^(10/12)", "ET minor seventh"},
        {1017.6, "9/5", "Just minor seventh"},
        {1049.4, "11/6", "Undecimal neutral seventh"},
        {1088.3, "15/8", "Just major seventh"},
        {1100.0, "2^(11/12)", "ET major seventh"},
        {1109.8, "243/128", "Pythagorean major seventh"},
        {1200.0, "2/1", "Octave"},
        {1901.96, "3/1", "Tritave (BP)"}
    };
    return ratios;
}

// ═══════════════════════════════════════════════════════════════════
// Ratio Approximation
// ═══════════════════════════════════════════════════════════════════

juce::String TuningExporter::approximateRatio(double cents)
{
    const auto& ratios = getCommonRatios();

    // Find closest ratio within 5 cents tolerance
    const RatioEntry* closest = nullptr;
    double minDiff = 5.0;

    for (const auto& entry : ratios)
    {
        double diff = std::abs(cents - entry.cents);
        if (diff < minDiff)
        {
            minDiff = diff;
            closest = &entry;
        }
    }

    if (closest != nullptr)
        return juce::String(closest->ratio);

    // No close match - return "~" with cents
    return "~" + juce::String(static_cast<int>(std::round(cents))) + "c";
}

// ═══════════════════════════════════════════════════════════════════
// ET Deviation Calculation
// ═══════════════════════════════════════════════════════════════════

double TuningExporter::calculateETDeviation(double cents, int degree, int totalDegrees, double period)
{
    if (degree == 0)
        return 0.0;

    double etCents = (static_cast<double>(degree) / static_cast<double>(totalDegrees)) * period;
    return cents - etCents;
}

// ═══════════════════════════════════════════════════════════════════
// CSS Styles
// ═══════════════════════════════════════════════════════════════════

juce::String TuningExporter::getStyles()
{
    return R"(
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Georgia', 'Times New Roman', serif;
            background: linear-gradient(135deg, #f5f0e8 0%, #e8dfd3 100%);
            color: #3d3325;
            padding: 40px;
            max-width: 900px;
            margin: 0 auto;
        }
        .container {
            background: #fffcf5;
            border: 1px solid #c4b89a;
            border-radius: 8px;
            padding: 40px;
            box-shadow: 0 4px 20px rgba(61, 51, 37, 0.1);
        }
        h1 {
            font-size: 2.2em;
            color: #5c4d3a;
            border-bottom: 2px solid #8b7355;
            padding-bottom: 15px;
            margin-bottom: 10px;
        }
        .subtitle {
            color: #7a6a52;
            font-style: italic;
            margin-bottom: 30px;
        }
        .metadata {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
            padding: 20px;
            background: #f9f6f0;
            border-radius: 6px;
            border-left: 4px solid #8b7355;
        }
        .metadata-item {
            display: flex;
            flex-direction: column;
        }
        .metadata-label {
            font-size: 0.75em;
            text-transform: uppercase;
            letter-spacing: 0.1em;
            color: #7a6a52;
            margin-bottom: 4px;
        }
        .metadata-value {
            font-size: 1.1em;
            font-weight: 600;
            color: #5c4d3a;
        }
        h2 {
            font-size: 1.4em;
            color: #5c4d3a;
            margin: 30px 0 15px 0;
            border-bottom: 1px solid #d4c9b5;
            padding-bottom: 8px;
        }
        .interval-table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
            font-size: 0.95em;
        }
        .interval-table th {
            background: #8b7355;
            color: #fff;
            padding: 12px 15px;
            text-align: left;
            font-weight: 600;
        }
        .interval-table td {
            padding: 10px 15px;
            border-bottom: 1px solid #e5ddd0;
        }
        .interval-table tr:nth-child(even) {
            background: #faf7f2;
        }
        .interval-table tr:hover {
            background: #f0ebe0;
        }
        .degree { font-weight: 600; color: #5c4d3a; }
        .cents { font-family: 'Courier New', monospace; }
        .ratio { color: #6b5a42; }
        .deviation {
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
        }
        .deviation.pure { color: #4a7c59; }
        .deviation.sharp { color: #c67b38; }
        .deviation.flat { color: #4a6fa5; }
        .pitch-circle {
            display: flex;
            justify-content: center;
            margin: 30px 0;
        }
        .pitch-circle svg {
            max-width: 400px;
            height: auto;
        }
        .footer {
            margin-top: 40px;
            padding-top: 20px;
            border-top: 1px solid #d4c9b5;
            font-size: 0.85em;
            color: #7a6a52;
            text-align: center;
        }
        .footer a {
            color: #8b7355;
            text-decoration: none;
        }
        @media print {
            body { background: white; padding: 20px; }
            .container { box-shadow: none; border: none; }
        }
    )";
}

// ═══════════════════════════════════════════════════════════════════
// Interval Table Row
// ═══════════════════════════════════════════════════════════════════

juce::String TuningExporter::generateIntervalRow(int degree, double cents, int totalDegrees, double period)
{
    juce::String ratio = approximateRatio(cents);
    double deviation = calculateETDeviation(cents, degree, totalDegrees, period);

    juce::String devClass = "pure";
    juce::String devSign = "";
    if (std::abs(deviation) > 0.5)
    {
        devClass = deviation > 0 ? "sharp" : "flat";
        devSign = deviation > 0 ? "+" : "";
    }

    return "<tr>"
           "<td class=\"degree\">" + juce::String(degree) + "</td>"
           "<td class=\"cents\">" + juce::String(cents, 2) + "</td>"
           "<td class=\"ratio\">" + ratio + "</td>"
           "<td class=\"deviation " + devClass + "\">" + devSign + juce::String(deviation, 2) + "</td>"
           "</tr>\n";
}

// ═══════════════════════════════════════════════════════════════════
// SVG Pitch Circle
// ═══════════════════════════════════════════════════════════════════

juce::String TuningExporter::generatePitchCircleSVG(const std::vector<double>& intervals,
                                                     double period,
                                                     const juce::String& scaleName)
{
    juce::ignoreUnused(scaleName);

    const int size = 400;
    const int cx = size / 2;
    const int cy = size / 2;
    const int radius = 160;
    const int innerRadius = 100;

    juce::String svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 "
                      + juce::String(size) + " " + juce::String(size) + "\">\n";

    // Background
    svg += "  <rect width=\"100%\" height=\"100%\" fill=\"#fffcf5\"/>\n";

    // Outer circle
    svg += "  <circle cx=\"" + juce::String(cx) + "\" cy=\"" + juce::String(cy)
         + "\" r=\"" + juce::String(radius) + "\" fill=\"none\" stroke=\"#d4c9b5\" stroke-width=\"2\"/>\n";

    // Inner reference circle
    svg += "  <circle cx=\"" + juce::String(cx) + "\" cy=\"" + juce::String(cy)
         + "\" r=\"" + juce::String(innerRadius) + "\" fill=\"none\" stroke=\"#e5ddd0\" stroke-width=\"1\" stroke-dasharray=\"4,4\"/>\n";

    // Center point
    svg += "  <circle cx=\"" + juce::String(cx) + "\" cy=\"" + juce::String(cy)
         + "\" r=\"4\" fill=\"#8b7355\"/>\n";

    const int total = static_cast<int>(intervals.size());
    const double twoPi = 6.283185307179586;

    // Draw ET reference lines (faint)
    for (int i = 0; i < total; ++i)
    {
        double etAngle = (static_cast<double>(i) / static_cast<double>(total)) * twoPi - (twoPi / 4.0);
        int etX = cx + static_cast<int>(std::cos(etAngle) * radius);
        int etY = cy + static_cast<int>(std::sin(etAngle) * radius);

        svg += "  <line x1=\"" + juce::String(cx) + "\" y1=\"" + juce::String(cy)
             + "\" x2=\"" + juce::String(etX) + "\" y2=\"" + juce::String(etY)
             + "\" stroke=\"#e5ddd0\" stroke-width=\"1\"/>\n";
    }

    // Draw actual interval positions
    for (int i = 0; i < total; ++i)
    {
        double cents = (i < static_cast<int>(intervals.size())) ? intervals[static_cast<size_t>(i)] : 0.0;
        double angle = (cents / period) * twoPi - (twoPi / 4.0);

        int x = cx + static_cast<int>(std::cos(angle) * radius);
        int y = cy + static_cast<int>(std::sin(angle) * radius);

        // Line from center to point
        svg += "  <line x1=\"" + juce::String(cx) + "\" y1=\"" + juce::String(cy)
             + "\" x2=\"" + juce::String(x) + "\" y2=\"" + juce::String(y)
             + "\" stroke=\"#8b7355\" stroke-width=\"2\"/>\n";

        // Node circle
        svg += "  <circle cx=\"" + juce::String(x) + "\" cy=\"" + juce::String(y)
             + "\" r=\"8\" fill=\"#8b7355\" stroke=\"#5c4d3a\" stroke-width=\"1\"/>\n";

        // Degree label
        int labelX = cx + static_cast<int>(std::cos(angle) * (radius + 20));
        int labelY = cy + static_cast<int>(std::sin(angle) * (radius + 20));
        svg += "  <text x=\"" + juce::String(labelX) + "\" y=\"" + juce::String(labelY)
             + "\" text-anchor=\"middle\" dominant-baseline=\"middle\" "
             + "font-family=\"Georgia\" font-size=\"12\" fill=\"#5c4d3a\">"
             + juce::String(i) + "</text>\n";
    }

    // Scale note count in center
    svg += "  <text x=\"" + juce::String(cx) + "\" y=\"" + juce::String(cy + innerRadius / 2)
         + "\" text-anchor=\"middle\" font-family=\"Georgia\" font-size=\"11\" fill=\"#7a6a52\">"
         + juce::String(total) + " notes</text>\n";

    svg += "</svg>";
    return svg;
}

// ═══════════════════════════════════════════════════════════════════
// Main HTML Export
// ═══════════════════════════════════════════════════════════════════

juce::String TuningExporter::toHTML(const TuningEngine& engine, const juce::String& pluginName)
{
    juce::String scaleName = engine.getActiveTuningName();
    std::vector<double> intervals = engine.getIntervals();
    int noteCount = engine.getScaleDegrees();
    double masterTune = engine.getMasterTune();
    float octaveStretch = engine.getOctaveStretch();

    // Determine period
    double period = 1200.0;
    if (!intervals.empty() && intervals.back() > 1200.0)
    {
        period = intervals.back();
    }
    else if (!intervals.empty() && noteCount > 0)
    {
        period = 1200.0;
    }

    // Current date
    auto now = juce::Time::getCurrentTime();
    juce::String dateStr = now.formatted("%Y-%m-%d %H:%M");

    // Build HTML
    juce::String html = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
                        "  <meta charset=\"UTF-8\">\n"
                        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                        "  <title>" + scaleName + " - Tuning Documentation</title>\n"
                        "  <style>\n" + getStyles() + "\n  </style>\n"
                        "</head>\n<body>\n"
                        "<div class=\"container\">\n";

    // Header
    html += "  <h1>" + scaleName + "</h1>\n";
    html += "  <p class=\"subtitle\">Microtonal Scale Documentation</p>\n";

    // Metadata
    html += "  <div class=\"metadata\">\n";
    html += "    <div class=\"metadata-item\"><span class=\"metadata-label\">Notes</span>"
            "<span class=\"metadata-value\">" + juce::String(noteCount) + "</span></div>\n";
    html += "    <div class=\"metadata-item\"><span class=\"metadata-label\">Period</span>"
            "<span class=\"metadata-value\">" + juce::String(period, 1) + " cents</span></div>\n";
    html += "    <div class=\"metadata-item\"><span class=\"metadata-label\">Reference (A4)</span>"
            "<span class=\"metadata-value\">" + juce::String(masterTune, 1) + " Hz</span></div>\n";
    if (std::abs(octaveStretch - 1.0f) > 0.001f)
    {
        html += "    <div class=\"metadata-item\"><span class=\"metadata-label\">Octave Stretch</span>"
                "<span class=\"metadata-value\">" + juce::String(octaveStretch, 3) + "</span></div>\n";
    }
    html += "  </div>\n";

    // Pitch Circle
    html += "  <h2>Pitch Circle</h2>\n";
    html += "  <div class=\"pitch-circle\">\n";
    html += generatePitchCircleSVG(intervals, period, scaleName);
    html += "  </div>\n";

    // Interval Table
    html += "  <h2>Interval Table</h2>\n";
    html += "  <table class=\"interval-table\">\n";
    html += "    <thead><tr>"
            "<th>Degree</th>"
            "<th>Cents</th>"
            "<th>Ratio</th>"
            "<th>ET Deviation</th>"
            "</tr></thead>\n";
    html += "    <tbody>\n";

    for (int i = 0; i < noteCount; ++i)
    {
        double cents = (i < static_cast<int>(intervals.size())) ? intervals[static_cast<size_t>(i)] : 0.0;
        html += "      " + generateIntervalRow(i, cents, noteCount, period);
    }

    html += "    </tbody>\n";
    html += "  </table>\n";

    // Footer
    html += "  <div class=\"footer\">\n";
    html += "    <p>Generated by <strong>" + pluginName + "</strong> on " + dateStr + "</p>\n";
    html += "    <p><a href=\"https://ouaricon.com\">Ouaricon Audio</a></p>\n";
    html += "  </div>\n";

    html += "</div>\n</body>\n</html>";

    return html;
}
