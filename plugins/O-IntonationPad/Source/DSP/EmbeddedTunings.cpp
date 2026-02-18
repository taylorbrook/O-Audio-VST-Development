/*
  ==============================================================================

    EmbeddedTunings.cpp
    scala-tuning-engine module v2.0.0

    Contains static tuning data for 24 embedded tunings across 5 categories.
    All intervals are in cents from unison.

  ==============================================================================
*/

#include "EmbeddedTunings.h"
#include <algorithm>

// Static member definitions
std::vector<EmbeddedTuning> EmbeddedTunings::tunings;
bool EmbeddedTunings::initialized = false;

void EmbeddedTunings::initializeTunings()
{
    if (initialized) return;

    tunings.clear();

    // ═══════════════════════════════════════════════════════════════════
    // HISTORICAL TEMPERAMENTS (Well-temperaments for Baroque/Classical)
    // ═══════════════════════════════════════════════════════════════════

    tunings.push_back({
        "historical/young1799",
        "Young 1799",
        "Historical",
        "Thomas Young's well-temperament from 1799. Balanced for all keys.",
        {0, 93.9, 195.8, 297.8, 391.7, 499.6, 591.7, 697.6, 795.8, 893.7, 999.6, 1091.7},
        1200.0
    });

    tunings.push_back({
        "historical/neidhardt",
        "Neidhardt III",
        "Historical",
        "Johann Neidhardt's circulating temperament (1724). Popular in Germany.",
        {0, 94.1, 196.1, 296.1, 392.2, 498.0, 592.2, 698.0, 796.1, 894.1, 996.1, 1092.2},
        1200.0
    });

    tunings.push_back({
        "historical/kellner",
        "Kellner Bach",
        "Historical",
        "Herbert Kellner's reconstruction of Bach's tuning (1975).",
        {0, 90.2, 194.5, 294.1, 386.3, 498.0, 588.3, 696.1, 792.2, 890.2, 996.1, 1088.3},
        1200.0
    });

    tunings.push_back({
        "historical/lehman",
        "Bach/Lehman",
        "Historical",
        "Bradley Lehman's interpretation of Bach's WTC tuning (2005).",
        {0, 98.0, 196.1, 298.0, 392.2, 502.0, 596.1, 698.0, 796.1, 894.1, 1000.0, 1094.1},
        1200.0
    });

    tunings.push_back({
        "historical/valotti",
        "Valotti",
        "Historical",
        "Francesco Valotti's temperament (c.1754). Smooth modulations.",
        {0, 94.1, 196.1, 298.0, 392.2, 502.0, 592.2, 698.0, 796.1, 894.1, 1000.0, 1090.2},
        1200.0
    });

    // ═══════════════════════════════════════════════════════════════════
    // JUST INTONATION (Pure intervals, limited transposition)
    // ═══════════════════════════════════════════════════════════════════

    tunings.push_back({
        "just/ptolemy",
        "Ptolemy Intense Diatonic",
        "Just Intonation",
        "Ancient Greek tuning by Ptolemy. Pure 5ths and 3rds on white keys.",
        {0, 111.7, 203.9, 315.6, 386.3, 498.0, 609.8, 702.0, 813.7, 884.4, 1017.6, 1088.3},
        1200.0
    });

    tunings.push_back({
        "just/5limit",
        "5-Limit JI",
        "Just Intonation",
        "5-limit just intonation. Pure major and minor triads in C.",
        {0, 111.7, 203.9, 315.6, 386.3, 498.0, 582.5, 702.0, 813.7, 884.4, 1017.6, 1088.3},
        1200.0
    });

    tunings.push_back({
        "just/7limit",
        "7-Limit JI",
        "Just Intonation",
        "7-limit just intonation. Includes septimal minor 7th (7/4).",
        {0, 111.7, 203.9, 315.6, 386.3, 498.0, 582.5, 702.0, 813.7, 884.4, 968.8, 1088.3},
        1200.0
    });

    tunings.push_back({
        "just/partch43",
        "Partch 43-Tone",
        "Just Intonation",
        "Harry Partch's 43-tone 11-limit scale. Microtonal masterwork.",
        {0, 21.5, 53.3, 84.5, 111.7, 150.6, 182.4, 203.9, 231.2, 266.9, 294.1, 315.6,
         347.4, 386.3, 417.5, 435.1, 470.8, 498.0, 519.5, 551.3, 582.5, 617.5, 648.7,
         680.5, 702.0, 729.2, 764.9, 782.5, 813.7, 852.6, 884.4, 905.9, 933.1, 968.8,
         996.1, 1017.6, 1049.4, 1088.3, 1115.5, 1146.7, 1178.5},
        1200.0
    });

    // ═══════════════════════════════════════════════════════════════════
    // EQUAL DIVISIONS (EDO - Equal temperaments beyond 12)
    // ═══════════════════════════════════════════════════════════════════

    // Helper lambda to generate EDO intervals
    auto generateEDO = [](int divisions) -> std::vector<double> {
        std::vector<double> intervals;
        double step = 1200.0 / divisions;
        for (int i = 0; i < divisions; ++i)
            intervals.push_back(i * step);
        return intervals;
    };

    tunings.push_back({
        "edo/17",
        "17-EDO",
        "Equal Divisions",
        "17 equal divisions. Good approximation of Pythagorean tuning.",
        generateEDO(17),
        1200.0
    });

    tunings.push_back({
        "edo/19",
        "19-EDO",
        "Equal Divisions",
        "19 equal divisions. Excellent 1/3-comma meantone approximation.",
        generateEDO(19),
        1200.0
    });

    tunings.push_back({
        "edo/22",
        "22-EDO",
        "Equal Divisions",
        "22 equal divisions. Good septimal intervals (7-limit).",
        generateEDO(22),
        1200.0
    });

    tunings.push_back({
        "edo/31",
        "31-EDO",
        "Equal Divisions",
        "31 equal divisions. Nearly pure 5ths and 3rds. Popular for xenharmonic.",
        generateEDO(31),
        1200.0
    });

    tunings.push_back({
        "edo/41",
        "41-EDO",
        "Equal Divisions",
        "41 equal divisions. Excellent 5-limit and 7-limit approximations.",
        generateEDO(41),
        1200.0
    });

    tunings.push_back({
        "edo/53",
        "53-EDO",
        "Equal Divisions",
        "53 equal divisions. Nearly perfect 5ths, used in Turkish music theory.",
        generateEDO(53),
        1200.0
    });

    // ═══════════════════════════════════════════════════════════════════
    // NON-OCTAVE (Scales that don't repeat at the octave)
    // ═══════════════════════════════════════════════════════════════════

    // Bohlen-Pierce: 13 equal divisions of 3:1 (tritave = 1902 cents)
    {
        std::vector<double> bp;
        double step = 1901.955 / 13.0;
        for (int i = 0; i < 13; ++i)
            bp.push_back(i * step);
        tunings.push_back({
            "nonoctave/bohlenpierceET",
            "Bohlen-Pierce (Equal)",
            "Non-Octave",
            "13 equal divisions of 3:1 tritave. No octaves, rich in 3:5:7 harmonies.",
            bp,
            1901.955
        });
    }

    tunings.push_back({
        "nonoctave/alpha",
        "Carlos Alpha",
        "Non-Octave",
        "Wendy Carlos' Alpha scale (78c steps). 15.385 steps/octave.",
        {0, 78.0, 156.0, 234.0, 312.0, 390.0, 468.0, 546.0, 624.0, 702.0, 780.0, 858.0, 936.0, 1014.0, 1092.0, 1170.0},
        1248.0
    });

    tunings.push_back({
        "nonoctave/beta",
        "Carlos Beta",
        "Non-Octave",
        "Wendy Carlos' Beta scale (63.8c steps). 18.809 steps/octave.",
        {0, 63.8, 127.6, 191.4, 255.2, 319.0, 382.8, 446.6, 510.4, 574.2, 638.0, 701.8, 765.6, 829.4, 893.2, 957.0, 1020.8, 1084.6, 1148.4},
        1212.2
    });

    tunings.push_back({
        "nonoctave/gamma",
        "Carlos Gamma",
        "Non-Octave",
        "Wendy Carlos' Gamma scale (35.1c steps). 34.188 steps/octave.",
        {0, 35.1, 70.2, 105.3, 140.4, 175.5, 210.6, 245.7, 280.8, 315.9, 351.0, 386.1, 421.2, 456.3, 491.4, 526.5, 561.6, 596.7, 631.8, 666.9, 702.0},
        737.1
    });

    // ═══════════════════════════════════════════════════════════════════
    // WORLD TUNINGS (Traditional tuning systems)
    // ═══════════════════════════════════════════════════════════════════

    tunings.push_back({
        "world/arabic24",
        "Arabic 24-TET",
        "World",
        "24-tone equal temperament. Includes quarter tones for Arabic maqamat.",
        generateEDO(24),
        1200.0
    });

    tunings.push_back({
        "world/turkish",
        "Turkish Makam",
        "World",
        "53-comma based Turkish tuning. Traditional Makam intervals.",
        {0, 90.2, 180.5, 203.9, 294.1, 384.4, 407.8, 498.0, 588.3, 678.5, 701.9, 792.2, 882.4, 905.9, 996.1, 1086.3, 1109.8},
        1200.0
    });

    tunings.push_back({
        "world/shruti22",
        "Indian 22-Shruti",
        "World",
        "Traditional Indian 22-shruti system. Basis for raga intonation.",
        {0, 90.2, 111.7, 182.4, 203.9, 294.1, 315.6, 386.3, 407.8, 498.0, 519.6, 590.2, 609.8, 701.9, 792.2, 813.7, 884.4, 905.9, 996.1, 1017.6, 1088.3, 1109.8},
        1200.0
    });

    tunings.push_back({
        "world/slendro",
        "Gamelan Slendro",
        "World",
        "Javanese 5-tone slendro. Approximately equal pentatonic.",
        {0, 240.0, 480.0, 720.0, 960.0},
        1200.0
    });

    tunings.push_back({
        "world/pelog",
        "Gamelan Pelog",
        "World",
        "Javanese 7-tone pelog. Unequal, with characteristic narrow and wide steps.",
        {0, 120.0, 270.0, 540.0, 670.0, 780.0, 950.0},
        1200.0
    });

    initialized = true;
}

const std::vector<EmbeddedTuning>& EmbeddedTunings::getAllTunings()
{
    initializeTunings();
    return tunings;
}

const EmbeddedTuning* EmbeddedTunings::getTuningById(const std::string& id)
{
    initializeTunings();
    for (const auto& tuning : tunings)
    {
        if (tuning.id == id)
            return &tuning;
    }
    return nullptr;
}

std::vector<const EmbeddedTuning*> EmbeddedTunings::getTuningsByCategory(const std::string& category)
{
    initializeTunings();
    std::vector<const EmbeddedTuning*> result;
    for (const auto& tuning : tunings)
    {
        if (tuning.category == category)
            result.push_back(&tuning);
    }
    return result;
}

std::vector<std::string> EmbeddedTunings::getCategories()
{
    return {"Historical", "Just Intonation", "Equal Divisions", "Non-Octave", "World"};
}

size_t EmbeddedTunings::getTuningCount()
{
    initializeTunings();
    return tunings.size();
}
