#include "MotionEngine.h"

void MotionEngine::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    phaseAccumulator = 0.0f;
    noiseTime = 0.0f;
    perlin.seed (42);
    currentState = { 0.0f, 0.0f, 1.0f };
}

float MotionEngine::getEffectiveSpeed() const
{
    if (tempoSyncIndex > 0 && tempoSyncIndex < 15)
        return (float) (hostBpm / 60.0) * tempoMultipliers[tempoSyncIndex];

    return speed;
}

void MotionEngine::advance (int numSamples)
{
    float effSpeed = getEffectiveSpeed();
    float phaseOffset = phase * (float) M_PI / 180.0f;
    float halfWidth = width * 0.5f;
    float baseDist = 1.0f; // Reference distance
    float depthFactor = depth * 0.01f;

    switch (pathIndex)
    {
        case 0: // Orbit (Elliptical)
        {
            float t = phaseAccumulator + phaseOffset;
            currentState.azimuth = halfWidth * std::cos (t);
            currentState.elevation = elevationEnabled
                ? elevationRange * std::sin (t)
                : tilt;
            currentState.distance = baseDist + depthFactor * baseDist * 0.5f * (std::sin (t) + 1.0f);

            phaseAccumulator += 2.0f * (float) M_PI * effSpeed * (float) numSamples / (float) sampleRate;
            if (phaseAccumulator >= 2.0f * (float) M_PI)
                phaseAccumulator -= 2.0f * (float) M_PI;
            break;
        }

        case 1: // Pendulum (Single-Axis Swing)
        {
            currentState.azimuth = halfWidth * std::sin (phaseAccumulator + phaseOffset);
            currentState.elevation = elevationEnabled ? tilt : 0.0f;
            currentState.distance = baseDist + depthFactor * baseDist;

            phaseAccumulator += 2.0f * (float) M_PI * effSpeed * (float) numSamples / (float) sampleRate;
            if (phaseAccumulator >= 2.0f * (float) M_PI)
                phaseAccumulator -= 2.0f * (float) M_PI;
            break;
        }

        case 2: // Linear (Constant Velocity Sweep)
        {
            currentState.azimuth = width * (phaseAccumulator / (2.0f * (float) M_PI)) - halfWidth;
            currentState.elevation = elevationEnabled ? tilt : 0.0f;
            currentState.distance = baseDist + depthFactor * baseDist;

            phaseAccumulator += 2.0f * (float) M_PI * effSpeed * (float) numSamples / (float) sampleRate;
            if (phaseAccumulator >= 2.0f * (float) M_PI)
                phaseAccumulator -= 2.0f * (float) M_PI;
            break;
        }

        case 3: // Drift (Perlin fBm)
        {
            float azNoise = perlin.fbm (noiseTime, 4);
            float elNoise = perlin.fbm (noiseTime + 1000.0f, 4);
            float distNoise = perlin.fbm (noiseTime + 2000.0f, 3);

            currentState.azimuth = azNoise * halfWidth;
            currentState.elevation = elevationEnabled
                ? elNoise * elevationRange
                : tilt;
            currentState.distance = baseDist + distNoise * depthFactor * baseDist;

            noiseTime += effSpeed * (float) numSamples / (float) sampleRate;
            break;
        }

        default:
            break;
    }

    // Clamp distance to safe range
    if (currentState.distance < 0.1f)
        currentState.distance = 0.1f;
}

void MotionEngine::setPath (int index)               { pathIndex = index; }
void MotionEngine::setSpeed (float hz)               { speed = hz; }
void MotionEngine::setWidth (float degrees)           { width = degrees; }
void MotionEngine::setDepth (float percent)           { depth = percent; }
void MotionEngine::setTilt (float degrees)            { tilt = degrees; }
void MotionEngine::setPhase (float degrees)           { phase = degrees; }
void MotionEngine::setElevationEnabled (bool enabled) { elevationEnabled = enabled; }
void MotionEngine::setElevationRange (float degrees)  { elevationRange = degrees; }
void MotionEngine::setTempoSync (int divisionIndex)   { tempoSyncIndex = divisionIndex; }
void MotionEngine::setHostBpm (double bpm)            { hostBpm = bpm; }
