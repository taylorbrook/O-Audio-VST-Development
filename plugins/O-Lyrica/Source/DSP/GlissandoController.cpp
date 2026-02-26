/*
  ==============================================================================

    GlissandoController.cpp
    Phase 2.9: Glissando Controller Implementation

  ==============================================================================
*/

#include "GlissandoController.h"

GlissandoController::GlissandoController()
{
    // Initialize frequency ramp with reasonable default ramp time
    // 50ms ramp time for smooth transitions
    frequencyRamp.reset(44100.0, 0.05);
    frequencyRamp.setCurrentAndTargetValue(440.0);
}

void GlissandoController::prepare(double sr)
{
    sampleRate = sr;

    // Reset smoothed value with new sample rate
    // 50ms ramp time for smooth, musical glissando
    frequencyRamp.reset(sampleRate, 0.05);

    // Recalculate samples per step based on new sample rate
    if (speed > 0.0f)
    {
        samplesPerStep = static_cast<int>(sampleRate / speed);
    }
}

void GlissandoController::setMode(GlissandoMode newMode)
{
    mode = newMode;

    // Reset state when changing modes
    if (mode == GlissandoMode::Off)
    {
        active = false;
        reset();
    }
}

void GlissandoController::setScale(const std::vector<double>& scaleFrequencies)
{
    // v1.3.2: Copy to fixed-size array instead of vector assignment (no allocation)
    scaleSize = std::min(static_cast<int>(scaleFrequencies.size()), MAX_SCALE_SIZE);
    for (int i = 0; i < scaleSize; ++i)
    {
        scale[i] = scaleFrequencies[i];
    }
}

void GlissandoController::setSpeed(float newSpeed)
{
    // Clamp to reasonable range (0.1 to 100 notes per second)
    speed = juce::jlimit(0.1f, 100.0f, newSpeed);

    // Update samples per step (used as baseline for Linear shape)
    if (speed > 0.0f)
    {
        samplesPerStep = static_cast<int>(sampleRate / speed);
    }
}

void GlissandoController::setShape(GlissandoShape newShape)
{
    shape = newShape;
}

void GlissandoController::startGlissando(double startFreq, double endFreq)
{
    if (mode == GlissandoMode::Off)
    {
        // No glissando - just set to end frequency
        frequencyRamp.setCurrentAndTargetValue(endFreq);
        active = false;
        return;
    }

    if (mode == GlissandoMode::Free)
    {
        // Free mode: Continuous sweep from start to end
        frequencyRamp.setCurrentAndTargetValue(startFreq);
        frequencyRamp.setTargetValue(endFreq);
        targetFrequency = endFreq;
        active = true;
    }
    else if (mode == GlissandoMode::ScaleLocked)
    {
        // Scale-Locked mode: Find scale degrees for start and end frequencies
        if (scaleSize == 0)
        {
            // No scale loaded - fall back to direct frequency
            frequencyRamp.setCurrentAndTargetValue(endFreq);
            active = false;
            return;
        }

        currentScaleDegree = findClosestScaleDegree(startFreq);
        targetScaleDegree = findClosestScaleDegree(endFreq);

        // Set initial frequency
        if (currentScaleDegree >= 0 && currentScaleDegree < scaleSize)
        {
            currentScaleFrequency = scale[currentScaleDegree];
        }

        sampleCounter = 0;
        currentStepIndex = 0;

        int numSteps = std::abs(targetScaleDegree - currentScaleDegree);
        active = (numSteps > 0);

        // v1.22.0: Pre-compute per-step sample durations from shape curve
        if (active && numSteps > 0)
        {
            // Total glissando duration in samples (numSteps * samplesPerStep preserves overall speed)
            int totalSamples = numSteps * samplesPerStep;

            if (shape == GlissandoShape::Linear || numSteps == 1)
            {
                // Constant spacing — same as pre-v1.22.0 behavior
                for (int i = 0; i < numSteps; ++i)
                    stepDurations[i] = samplesPerStep;
            }
            else
            {
                // Compute cumulative curve positions for each step boundary
                // t goes from 0.0 (start) to 1.0 (end), mapped through shape function
                // The derivative of the shape function determines note density
                int assigned = 0;
                for (int i = 0; i < numSteps; ++i)
                {
                    double t0 = static_cast<double>(i) / numSteps;
                    double t1 = static_cast<double>(i + 1) / numSteps;
                    double mapped0, mapped1;

                    switch (shape)
                    {
                        case GlissandoShape::Accelerate:
                            // pow(t, 2.0) — slow start, fast end
                            mapped0 = t0 * t0;
                            mapped1 = t1 * t1;
                            break;
                        case GlissandoShape::Decelerate:
                            // pow(t, 0.5) — fast start, slow end
                            mapped0 = std::sqrt(t0);
                            mapped1 = std::sqrt(t1);
                            break;
                        case GlissandoShape::SCurve:
                            // smoothstep: t*t*(3-2t) — slow start, fast middle, slow end
                            mapped0 = t0 * t0 * (3.0 - 2.0 * t0);
                            mapped1 = t1 * t1 * (3.0 - 2.0 * t1);
                            break;
                        default:
                            mapped0 = t0;
                            mapped1 = t1;
                            break;
                    }

                    // Duration for this step proportional to the curve segment width
                    double segmentWidth = mapped1 - mapped0;
                    int stepSamples = std::max(1, static_cast<int>(segmentWidth * totalSamples));
                    stepDurations[i] = stepSamples;
                    assigned += stepSamples;
                }

                // Distribute rounding remainder to the last step
                if (assigned != totalSamples && numSteps > 0)
                    stepDurations[numSteps - 1] += (totalSamples - assigned);
            }
        }
    }
}

double GlissandoController::getNextFrequency()
{
    if (!active || mode == GlissandoMode::Off)
    {
        // No glissando - return current value
        return frequencyRamp.getCurrentValue();
    }

    if (mode == GlissandoMode::Free)
    {
        // Free mode: Get next smoothed value
        double nextFreq = frequencyRamp.getNextValue();

        // Check if we've reached the target
        if (std::abs(nextFreq - targetFrequency) < 0.01)
        {
            active = false;
        }

        return nextFreq;
    }
    else if (mode == GlissandoMode::ScaleLocked)
    {
        // Scale-Locked mode: Step through scale degrees
        updateScaleLocked();
        return currentScaleFrequency;
    }

    return frequencyRamp.getCurrentValue();
}

bool GlissandoController::isActive() const
{
    return active;
}

void GlissandoController::reset()
{
    active = false;
    frequencyRamp.setCurrentAndTargetValue(frequencyRamp.getCurrentValue());
    sampleCounter = 0;
}

int GlissandoController::findClosestScaleDegree(double freq) const
{
    if (scaleSize == 0)
        return 0;

    int closestIndex = 0;
    double smallestDiff = std::abs(scale[0] - freq);

    for (int i = 1; i < scaleSize; ++i)
    {
        double diff = std::abs(scale[i] - freq);
        if (diff < smallestDiff)
        {
            smallestDiff = diff;
            closestIndex = i;
        }
    }

    return closestIndex;
}

void GlissandoController::updateScaleLocked()
{
    if (scaleSize == 0 || currentScaleDegree == targetScaleDegree)
    {
        active = false;
        return;
    }

    // Increment sample counter
    sampleCounter++;

    // v1.22.0: Use pre-computed per-step duration from shape curve
    int currentStepDuration = stepDurations[currentStepIndex];

    // Check if it's time to step to next scale degree
    if (sampleCounter >= currentStepDuration)
    {
        sampleCounter = 0;
        currentStepIndex++;

        // Move one step toward target
        if (currentScaleDegree < targetScaleDegree)
        {
            currentScaleDegree++;
        }
        else if (currentScaleDegree > targetScaleDegree)
        {
            currentScaleDegree--;
        }

        // Update frequency to new scale degree
        if (currentScaleDegree >= 0 && currentScaleDegree < scaleSize)
        {
            currentScaleFrequency = scale[currentScaleDegree];
        }

        // Check if we've reached the target
        if (currentScaleDegree == targetScaleDegree)
        {
            active = false;
        }
    }
}
