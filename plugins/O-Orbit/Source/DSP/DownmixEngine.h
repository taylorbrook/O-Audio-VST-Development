#pragma once

#include "../Data/SpeakerLayout.h"
#include <vector>
#include <cmath>
#include <algorithm>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

class DownmixEngine
{
public:
    void prepare (const SpeakerLayout& layout, int availableChannels)
    {
        sourceChannels = layout.getChannelCount();
        targetChannels = availableChannels;
        active = (targetChannels < sourceChannels && targetChannels >= 2);

        if (! active)
            return;

        // Build downmix matrix: targetChannels x sourceChannels
        downmixMatrix.resize ((size_t) targetChannels);
        for (auto& row : downmixMatrix)
        {
            row.assign ((size_t) sourceChannels, 0.0f);
        }

        if (targetChannels == 2)
        {
            // Stereo downmix: pan each speaker to L/R based on azimuth
            for (int src = 0; src < sourceChannels; ++src)
            {
                if (layout.speakers[(size_t) src].isLFE)
                {
                    // LFE equally to both channels
                    downmixMatrix[0][(size_t) src] = 0.5f;
                    downmixMatrix[1][(size_t) src] = 0.5f;
                }
                else
                {
                    float az = layout.speakers[(size_t) src].azimuth;
                    float panAngle = (az + 90.0f) / 180.0f;
                    panAngle = std::clamp (panAngle, 0.0f, 1.0f);
                    downmixMatrix[0][(size_t) src] = std::cos (panAngle * (float) M_PI * 0.5f);
                    downmixMatrix[1][(size_t) src] = std::sin (panAngle * (float) M_PI * 0.5f);
                }
            }

            // Energy-preserving normalization per target channel
            for (int t = 0; t < targetChannels; ++t)
            {
                float energy = 0.0f;
                for (int s = 0; s < sourceChannels; ++s)
                    energy += downmixMatrix[(size_t) t][(size_t) s] * downmixMatrix[(size_t) t][(size_t) s];

                if (energy > 1.0f)
                {
                    float scale = 1.0f / std::sqrt (energy);
                    for (int s = 0; s < sourceChannels; ++s)
                        downmixMatrix[(size_t) t][(size_t) s] *= scale;
                }
            }
        }
        else
        {
            // N-channel downmix: fold height speakers to nearest ear-level
            for (int src = 0; src < sourceChannels; ++src)
            {
                if (src < targetChannels)
                {
                    downmixMatrix[(size_t) src][(size_t) src] = 1.0f;
                }
                else
                {
                    // Find nearest target channel by azimuth
                    float srcAz = layout.speakers[(size_t) src].azimuth;
                    int bestTarget = 0;
                    float bestDist = 999.0f;

                    for (int t = 0; t < targetChannels; ++t)
                    {
                        if (t < (int) layout.speakers.size())
                        {
                            float diff = std::abs (std::fmod (layout.speakers[(size_t) t].azimuth - srcAz + 540.0f, 360.0f) - 180.0f);
                            if (diff < bestDist)
                            {
                                bestDist = diff;
                                bestTarget = t;
                            }
                        }
                    }

                    downmixMatrix[(size_t) bestTarget][(size_t) src] += 0.707f; // -3dB
                }
            }
        }
    }

    bool isActive() const { return active; }
    int getSourceChannels() const { return sourceChannels; }
    int getTargetChannels() const { return targetChannels; }

    void process (const float* const* sourceData, float** destData, int numSamples)
    {
        if (! active)
            return;

        for (int t = 0; t < targetChannels; ++t)
        {
            for (int s = 0; s < numSamples; ++s)
                destData[t][s] = 0.0f;

            for (int src = 0; src < sourceChannels; ++src)
            {
                float gain = downmixMatrix[(size_t) t][(size_t) src];
                if (gain == 0.0f) continue;

                for (int s = 0; s < numSamples; ++s)
                    destData[t][s] += sourceData[src][s] * gain;
            }
        }
    }

private:
    bool active = false;
    int sourceChannels = 0;
    int targetChannels = 0;
    std::vector<std::vector<float>> downmixMatrix;
};
