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
                                                double period,
                                                const juce::String& scaleName);
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
