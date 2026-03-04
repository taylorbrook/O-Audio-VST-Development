/*
  ==============================================================================

    GlissandoController.cpp
    Phase 2.9: Glissando Controller Implementation

  ==============================================================================
*/

#include "GlissandoController.h"

GlissandoController::GlissandoController()
{
    currentFrequency = 440.0;
}

void GlissandoController::prepare(double sr)
{
    sampleRate = sr;

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

void GlissandoController::setHumanize(float amount)
{
    humanizeAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void GlissandoController::setRampTime(float seconds)
{
    rampTimeSeconds = juce::jlimit(0.01f, 10.0f, seconds); // v1.31.0: expanded max for tempo sync
}

void GlissandoController::setVelocityProfile(float startVel, float endVel)
{
    startVelocity = juce::jlimit(0.0f, 1.0f, startVel);
    endVelocity = juce::jlimit(0.0f, 1.0f, endVel);
}

float GlissandoController::getNextVelocity() const
{
    if (mode != GlissandoMode::ScaleLocked || initialScaleDegree == targetScaleDegree)
        return 1.0f;

    int totalSteps = std::abs(targetScaleDegree - initialScaleDegree);
    int stepsDone = std::abs(currentScaleDegree - initialScaleDegree);
    float t = static_cast<float>(stepsDone) / static_cast<float>(totalSteps);

    return startVelocity + (endVelocity - startVelocity) * t;
}

GlissandoDirection GlissandoController::getDirection() const
{
    return direction;
}

void GlissandoController::startGlissando(double startFreq, double endFreq)
{
    // v1.28.0: Auto-detect sweep direction from pitch relationship
    direction = (endFreq > startFreq) ? GlissandoDirection::Ascending : GlissandoDirection::Descending;

    if (mode == GlissandoMode::Off)
    {
        // No glissando - just set to end frequency
        currentFrequency = endFreq;
        active = false;
        return;
    }

    if (mode == GlissandoMode::Free)
    {
        // Free mode: Logarithmic interpolation for perceptually uniform pitch sweep
        currentFrequency = startFreq;
        targetFrequency = endFreq;
        startFreqLog = std::log2(startFreq);
        endFreqLog = std::log2(endFreq);
        freeProgress = 0.0;

        // v1.25.0: Use configurable ramp time (default 50ms preserves pre-v1.25.0 behavior)
        freeProgressIncrement = 1.0 / (static_cast<double>(rampTimeSeconds) * sampleRate);
        active = true;
    }
    else if (mode == GlissandoMode::ScaleLocked)
    {
        // Scale-Locked mode: Find scale degrees for start and end frequencies
        if (scaleSize == 0)
        {
            // No scale loaded - fall back to direct frequency
            currentFrequency = endFreq;
            active = false;
            return;
        }

        currentScaleDegree = findClosestScaleDegree(startFreq);
        targetScaleDegree = findClosestScaleDegree(endFreq);
        initialScaleDegree = currentScaleDegree;
        currentVelocity = startVelocity;

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

            // v1.24.0: Apply per-step timing humanization (jitter)
            if (humanizeAmount > 0.0f)
            {
                // maxJitterMs scales inversely with speed: slow = more jitter, fast = less
                float maxJitterMs = juce::jmap(speed, 4.0f, 30.0f, 10.0f, 2.0f);

                for (int i = 0; i < numSteps; ++i)
                {
                    float jitterMs = humanizeAmount * maxJitterMs * (humanizeRandom.nextFloat() * 2.0f - 1.0f);
                    int jitterSamples = static_cast<int>(jitterMs * 0.001f * static_cast<float>(sampleRate));
                    stepDurations[i] = std::max(1, stepDurations[i] + jitterSamples);
                }
            }
        }
    }
}

double GlissandoController::getNextFrequency()
{
    if (!active || mode == GlissandoMode::Off)
    {
        return currentFrequency;
    }

    if (mode == GlissandoMode::Free)
    {
        // Free mode: Logarithmic interpolation (equal time per octave)
        freeProgress += freeProgressIncrement;

        if (freeProgress >= 1.0)
        {
            currentFrequency = targetFrequency;
            active = false;
        }
        else
        {
            // v1.29.2: Apply shape curve to Free mode progress
            double shapedProgress = freeProgress;
            switch (shape)
            {
                case GlissandoShape::Accelerate:
                    shapedProgress = freeProgress * freeProgress;
                    break;
                case GlissandoShape::Decelerate:
                    shapedProgress = std::sqrt(freeProgress);
                    break;
                case GlissandoShape::SCurve:
                    shapedProgress = freeProgress * freeProgress * (3.0 - 2.0 * freeProgress);
                    break;
                default: // Linear
                    break;
            }
            currentFrequency = std::pow(2.0, startFreqLog + shapedProgress * (endFreqLog - startFreqLog));
        }

        return currentFrequency;
    }
    else if (mode == GlissandoMode::ScaleLocked)
    {
        // Scale-Locked mode: Step through scale degrees
        updateScaleLocked();
        return currentScaleFrequency;
    }

    return currentFrequency;
}

bool GlissandoController::isActive() const
{
    return active;
}

void GlissandoController::reset()
{
    active = false;
    freeProgress = 0.0;
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

        // v1.27.0: Update interpolated velocity for this step
        {
            int totalSteps = std::abs(targetScaleDegree - initialScaleDegree);
            if (totalSteps > 0)
            {
                int stepsDone = std::abs(currentScaleDegree - initialScaleDegree);
                float t = static_cast<float>(stepsDone) / static_cast<float>(totalSteps);
                currentVelocity = startVelocity + (endVelocity - startVelocity) * t;
            }
        }

        // Check if we've reached the target
        if (currentScaleDegree == targetScaleDegree)
        {
            active = false;
        }
    }
}
