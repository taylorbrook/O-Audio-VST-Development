/*
  ==============================================================================

    EmbeddedTunings.h
    scala-tuning-engine module v2.0.0

    Provides 24+ embedded tunings organized by category:
    - Historical (Well-temperaments)
    - Just Intonation
    - Equal Divisions (EDO)
    - Non-Octave (Bohlen-Pierce, Carlos)
    - World (Arabic, Turkish, Indian, Gamelan)

  ==============================================================================
*/

#pragma once
#include <vector>
#include <string>

/**
 * EmbeddedTuning: Single tuning definition
 */
struct EmbeddedTuning
{
    const char* id;           // "historical/young1799"
    const char* name;         // "Young 1799"
    const char* category;     // "Historical"
    const char* description;  // Brief description
    std::vector<double> intervals;  // Cents from unison (excluding period)
    double period;            // Period in cents (1200.0 for octave, 1902.0 for tritave)
};

/**
 * EmbeddedTunings: Static library of factory tunings
 */
class EmbeddedTunings
{
public:
    /**
     * Get all available tunings
     * @return Vector of all embedded tuning definitions
     */
    static const std::vector<EmbeddedTuning>& getAllTunings();

    /**
     * Get tuning by ID
     * @param id Tuning ID (e.g., "historical/young1799")
     * @return Pointer to tuning if found, nullptr otherwise
     */
    static const EmbeddedTuning* getTuningById(const std::string& id);

    /**
     * Get tunings by category
     * @param category Category name (e.g., "Historical")
     * @return Vector of tunings in that category
     */
    static std::vector<const EmbeddedTuning*> getTuningsByCategory(const std::string& category);

    /**
     * Get list of all category names
     * @return Vector of category names in display order
     */
    static std::vector<std::string> getCategories();

    /**
     * Get tuning count
     */
    static size_t getTuningCount();

private:
    static std::vector<EmbeddedTuning> tunings;
    static bool initialized;
    static void initializeTunings();
};
