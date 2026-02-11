#pragma once

#include "../Data/SpeakerLayout.h"
#include <vector>
#include <cmath>
#include <algorithm>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

class VBAPRenderer
{
public:
    void prepare (const SpeakerLayout& layout);

    void computeGains (float azimuth, float elevation, float* gains, int numSpeakers);

    void setCenterDiverge (float amount) { centerDiverge = amount; }

    int getNumSpeakers() const { return currentNumSpeakers; }
    bool isSAFActive() const { return useSAF; }

    // Set external gain table from VBAPDataExchange (Phase 2.2+)
    void setExternalGainTable (const float* table, int tableSize, int numSpkrs,
                               int aziResolution, int elevResolution, bool is3DTable,
                               const int* spkrToChannelMap, int numVBAPSpkrs);

private:
    void computeStereoGains (float azimuth, float* gains);
    void computePairwiseGains (float azimuth, float* gains, int numSpeakers);
    void computeSAFGains (float azimuth, float elevation, float* gains, int numSpeakers);
    void applyCenterDiverge (float* gains, int numSpeakers);

    int currentNumSpeakers = 0;
    bool useSAF = false;
    float centerDiverge = 0.0f;

    std::vector<float> speakerAzimuths;

    // External gain table (set by VBAPDataExchange)
    const float* externalGainTable = nullptr;
    int externalTableSize = 0;
    int externalNumVBAPSpeakers = 0;
    int externalAziRes = 1;
    int externalElevRes = 1;
    bool externalIs3D = false;
    const int* externalSpeakerToChannel = nullptr;
    int externalNumMappedSpeakers = 0;

    // LFE channel indices (excluded from VBAP)
    std::vector<int> lfeChannelIndices;
};
