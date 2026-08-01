/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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

    BowedStringVoice.cpp
    O-Bowed - Physical Modeling Bowed String Voice (MPE)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "BowedStringVoice.h"
#include "TuningEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>

BowedStringVoice::BowedStringVoice (juce::AudioProcessorValueTreeState* apvts)
    : parameters (apvts)
{
}

//==============================================================================
// MPE Voice Callbacks
//==============================================================================

void BowedStringVoice::noteStarted()
{
    auto note = getCurrentlyPlayingNote();
    int midiNote = note.initialNote;
    float velocity = note.noteOnVelocity.asUnsignedFloat();

    // Get base frequency from tuning engine (Scala/MTS-ESP/12-TET)
    currentFrequency = getBaseFrequencyFromTuning (midiNote);

    // Apply initial MPE pitch bend on top of tuning engine frequency
    float bendSemitones = static_cast<float> (note.totalPitchbendInSemitones);
    if (std::abs (bendSemitones) > 0.001f)
        currentFrequency *= std::pow (2.0f, bendSemitones / 12.0f);

    waveguideString.trigger (currentFrequency);
    bowModel.startBow (velocity);
    oversampling.reset();
    bowNoiseGen.reset();

    // Clear the bristle displacement state so a new note can't inherit a stale (or
    // non-finite) z from the previous note now that the bristle path is live. IN-03,
    // required companion to WR-02.
    bristleFriction.resetState();

    // Reset reversed-friction smoother so the reversal formula can't pin rho
    // to a non-zero value before the bow physically engages (v1.1.2 thump fix).
    smoothedReversedAmount = 0.0f;

    // WR-04: re-prime the geometry/brightness smoothers to this note's first block
    // (done in updateParametersFromAPVTS) so the attack doesn't sweep.
    smoothersPrimed = false;
}

void BowedStringVoice::noteStopped (bool allowTailOff)
{
    if (allowTailOff)
    {
        // Bow lifts -- release ramp begins, voice stays active until energy decays
        bowModel.stopBow();
    }
    else
    {
        // Hard stop -- immediate silence
        clearCurrentNote();
        waveguideString.reset();
        bowModel.reset();
    }
}

void BowedStringVoice::notePitchbendChanged()
{
    auto note = getCurrentlyPlayingNote();
    int midiNote = note.initialNote;

    // Recompute from tuning engine base + new MPE bend
    currentFrequency = getBaseFrequencyFromTuning (midiNote);
    float bendSemitones = static_cast<float> (note.totalPitchbendInSemitones);
    if (std::abs (bendSemitones) > 0.001f)
        currentFrequency *= std::pow (2.0f, bendSemitones / 12.0f);

    waveguideString.trigger (currentFrequency);
}

void BowedStringVoice::notePressureChanged()
{
    // Pressure modulation applied in updateParametersFromAPVTS()
}

void BowedStringVoice::noteTimbreChanged()
{
    // Timbre modulation applied in updateParametersFromAPVTS()
}

void BowedStringVoice::noteKeyStateChanged()
{
    // No special handling needed for sustain/sostenuto
}

//==============================================================================
// Prepare and Render
//==============================================================================

void BowedStringVoice::prepareToPlay (double sampleRate, int maxBlockSize)
{
    // Prepare waveguide and bow at 2x oversampled rate
    waveguideString.prepare (sampleRate * 2.0, maxBlockSize * 2);
    bowModel.prepare (sampleRate * 2.0);

    // dt at oversampled rate
    dt = 1.0f / static_cast<float> (sampleRate * 2.0);

    // Initialize oversampling (mono)
    oversampling.initProcessing (static_cast<size_t> (maxBlockSize));

    // Allocate mono voice buffer
    voiceBuffer.setSize (1, maxBlockSize);
    voiceBuffer.clear();

    // Prepare bow noise at native rate (not oversampled)
    bowNoiseGen.prepare (sampleRate, voiceIndex);

    // Reversed-friction smoother coefficient: ~25 ms one-pole at oversampled rate
    reversedRampCoeff = 1.0f - std::exp (-1.0f / (0.025f * static_cast<float> (sampleRate * 2.0)));

    // WR-04: bow-position / brightness smoothers run in the oversampled inner loop.
    // ~15 ms ramp removes block-boundary steps without smearing intentional gestures.
    bowPositionSmoothed.reset (sampleRate * 2.0, 0.015);
    brightnessSmoothed.reset (sampleRate * 2.0, 0.015);
    smoothersPrimed = false;
}

void BowedStringVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                         int startSample, int numSamples)
{
    // WR-03: belt-and-suspenders denormal flush for the voice in isolation (bridge
    // loss-filter state has a ~15 s tail that decays through the denormal range).
    // The processor sets ScopedNoDenormals on the master path, but the render harness
    // drives voices directly and feedback DSP warrants its own guard.
    const juce::ScopedNoDenormals noDenormals;

    updateParametersFromAPVTS();

    // Check if voice should be cleared (bow released and string decayed)
    if (! bowModel.isActive() && ! waveguideString.isActive())
    {
        clearCurrentNote();
        return;
    }

    // --- Block-based oversampling pathway ---

    // 1. Prepare mono input block of silence
    voiceBuffer.clear (0, 0, numSamples);
    juce::dsp::AudioBlock<float> inputBlock (voiceBuffer);
    auto block = inputBlock.getSubBlock (0, static_cast<size_t> (numSamples));

    // 2. Upsample (silence in -> 2x oversampled buffer)
    auto oversampledBlock = oversampling.processSamplesUp (block);
    auto numOversampledSamples = static_cast<int> (oversampledBlock.getNumSamples());
    auto* oversampledData = oversampledBlock.getChannelPointer (0);

    // 3. Per-sample loop at 2x rate (friction + waveguide inner loop)
    for (int i = 0; i < numOversampledSamples; ++i)
    {
        // WR-04: advance the smoothed bow position / brightness once per sample and
        // push into the waveguide before reading the junction, so continuous changes
        // move the delay lengths and bridge-filter corner smoothly instead of stepping
        // at block boundaries.
        waveguideString.setBowPosition (bowPositionSmoothed.getNextValue());
        waveguideString.setBrightness  (brightnessSmoothed.getNextValue());

        // Step 1: Update bow envelope
        bowModel.updateEnvelope();
        float v_bow = bowModel.getBowVelocity();
        float F_bow = bowModel.getBowForce();

        // Steps 2-4: Read junction (pop from delays, compute incoming)
        auto jState = waveguideString.readJunction (v_bow);
        float v_delta = v_bow - jState.v_string_incoming;

        // Step 5: Friction — Hyperbolic Stribeck curve
        float rho = frictionModel.computeReflectionCoefficient (v_delta, F_bow);

        // WR-02: blend the elasto-plastic bristle model in via bowHairStiffness.
        // 0 = pure Core (Hyperbolic), 1 = full bristle. Previously bristleBlend was
        // computed every block but never applied, so bowHairStiffness was inert.
        if (bristleBlend > 0.001f)
        {
            float bristleRho = bristleFriction.computeReflectionCoefficient (v_delta, F_bow, dt);
            rho = (1.0f - bristleBlend) * rho + bristleBlend * bristleRho;
        }

        // Reversed friction: interpolate rho toward (1 - rho).
        // The smoother ramps from 0 on note-on over ~25 ms so the reversal
        // formula can't pin rho to a non-zero value while F_bow is still
        // ramping — which previously caused the waveguide to latch onto the
        // bow velocity attack and emit a thump at note-on.
        smoothedReversedAmount += (reversedAmount - smoothedReversedAmount) * reversedRampCoeff;
        if (smoothedReversedAmount >= 0.001f)
            rho = rho + smoothedReversedAmount * (1.0f - 2.0f * rho);

        // Clamp rho to prevent excessive velocity injection
        rho = std::min (rho, 0.85f);

        // Energy-aware excitation limiting for high-sustain modes.
        // Reduces excitation as waveguide energy builds — physically motivated:
        // bow loses grip on strongly oscillating string.
        if (cachedInfSustain > 0.05f)
        {
            float energy = waveguideString.getEnergyEstimate();
            float targetEnergy = 0.5f * (1.0f - 0.75f * cachedInfSustain);
            if (energy > targetEnergy)
                rho *= targetEnergy / energy;
        }

        // Steps 6-8: Write junction (inject velocity, push to delays, output)
        float sample = waveguideString.writeJunction (rho, v_delta, jState);

        // WR-01: NaN/Inf guard at the write boundary. std::min-based rho clamps do NOT
        // filter NaN (min(NaN,x)==NaN) and tanh preserves it, so a single non-finite
        // excitation would poison the delay line, drive energyEstimate to NaN, and
        // silence the note mid-sustain. Reset the *source* (waveguide + both friction
        // models), not just the sample, so the note can recover cleanly.
        if (! std::isfinite (sample))
        {
            waveguideString.reset();
            bowModel.reset();
            bristleFriction.resetState();
            sample = 0.0f;
        }

        // Sub-harmonics waveshaper (post-waveguide, pre-body)
        if (subHarmonicsAmount >= 0.001f)
            sample = subHarmonicsGen.process (sample, subHarmonicsAmount);

        oversampledData[i] = sample;
    }

    // 4. Downsample back to native rate
    oversampling.processSamplesDown (block);

    // 5. Mix voiceBuffer into outputBuffer with panning + bow noise (at native rate)
    for (int i = 0; i < numSamples; ++i)
    {
        // Normalize waveguide velocity units to audio range
        float sample = voiceBuffer.getSample (0, i) * 0.35f;

        // Add bow noise (post-body, post-downsample)
        float noise = bowNoiseGen.processSample (effectivePressure, effectiveSpeed, bowNoiseAmount);
        sample += noise;

        // Safety hard-clip to prevent runaway
        sample = juce::jlimit (-1.0f, 1.0f, sample);

        // Write to stereo output with per-voice panning
        outputBuffer.addSample (0, startSample + i, sample * panL);
        if (outputBuffer.getNumChannels() >= 2)
            outputBuffer.addSample (1, startSample + i, sample * panR);
    }
}

//==============================================================================
// Parameter Reading
//==============================================================================

void BowedStringVoice::updateParametersFromAPVTS()
{
    if (parameters == nullptr)
        return;

    // Read APVTS parameters atomically (once per block)
    float bowSpeed       = parameters->getRawParameterValue ("bowSpeed")->load();
    float bowPressure    = parameters->getRawParameterValue ("bowPressure")->load();
    float bowPos         = parameters->getRawParameterValue ("bowPosition")->load();
    float rosin          = parameters->getRawParameterValue ("rosin")->load();

    // Humanize: add random-walk offsets scaled to each parameter's musical range.
    // Offsets are in [-1..+1] normalised scale; per-param deviations below were
    // chosen so a full "humanize" knob (1.0) gives noticeable but never
    // destructive variation. The engine is processor-shared, so all active
    // voices breathe coherently.
    if (humanizeEngine != nullptr)
    {
        constexpr float speedDev    = 0.30f;  // m/s
        constexpr float pressureDev = 0.60f;  // N
        constexpr float positionDev = 0.05f;  // fraction of string length
        constexpr float rosinDev    = 0.20f;  // 0-1 friction shape

        bowSpeed    = juce::jlimit (0.02f, 2.0f,
                                    bowSpeed    + humanizeEngine->getOffset (HumanizeEngine::Speed)    * speedDev);
        bowPressure = juce::jlimit (0.01f, 5.0f,
                                    bowPressure + humanizeEngine->getOffset (HumanizeEngine::Pressure) * pressureDev);
        bowPos      = juce::jlimit (0.02f, 0.30f,
                                    bowPos      + humanizeEngine->getOffset (HumanizeEngine::Position) * positionDev);
        rosin       = juce::jlimit (0.0f, 1.0f,
                                    rosin       + humanizeEngine->getOffset (HumanizeEngine::Rosin)    * rosinDev);
    }
    float brightness     = parameters->getRawParameterValue ("brightness")->load();
    float infSustain     = parameters->getRawParameterValue ("infiniteSustain")->load();
    float outputLevel    = parameters->getRawParameterValue ("outputLevel")->load();
    bowNoiseAmount       = parameters->getRawParameterValue ("bowNoise")->load();

    // Read friction parameters
    reversedAmount       = parameters->getRawParameterValue ("reversedFriction")->load();
    subHarmonicsAmount   = parameters->getRawParameterValue ("subHarmonics")->load();
    float stringGauge    = parameters->getRawParameterValue ("stringGauge")->load();
    float bowHairStiff   = parameters->getRawParameterValue ("bowHairStiffness")->load();
    bristleBlend         = bowHairStiff;

    // Apply MPE modulation on top of knob base values
    auto note = getCurrentlyPlayingNote();
    float mpePressure = note.pressure.asUnsignedFloat();
    float mpeTimbre   = note.timbre.asSignedFloat();

    effectivePressure = bowPressure * (0.5f + mpePressure * 1.5f);
    effectivePosition = bowPos + mpeTimbre * 0.1f;
    effectivePosition = juce::jlimit (0.02f, 0.30f, effectivePosition);
    effectiveSpeed    = bowSpeed * mpeExpression;

    // Update DSP components with effective values
    bowModel.setBowSpeed (effectiveSpeed);
    bowModel.setBowPressure (effectivePressure);
    frictionModel.setRosin (rosin);
    waveguideString.setInfiniteSustain (infSustain);

    // WR-04: bow position and brightness are advanced per sample from smoothers in the
    // render loop rather than stepped here. Prime to the real value on the first block
    // of a note so the attack starts in place; ramp on subsequent blocks.
    if (! smoothersPrimed)
    {
        bowPositionSmoothed.setCurrentAndTargetValue (effectivePosition);
        brightnessSmoothed.setCurrentAndTargetValue (brightness);
        smoothersPrimed = true;
    }
    else
    {
        bowPositionSmoothed.setTargetValue (effectivePosition);
        brightnessSmoothed.setTargetValue (brightness);
    }
    cachedInfSustain = infSustain;
    outputGainLinear = juce::Decibels::decibelsToGain (outputLevel);

    // Update friction models with matching parameters
    frictionModel.setStringImpedance (stringGauge);
    bristleFriction.setRosin (rosin);
    bristleFriction.setStringImpedance (stringGauge);
    bristleFriction.setBristleStiffness (bowHairStiff);
}

float BowedStringVoice::getBaseFrequencyFromTuning (int midiNote) const
{
    double freq = (tuningEngine != nullptr)
        ? tuningEngine->getFrequency (midiNote)
        : juce::MidiMessage::getMidiNoteInHertz (midiNote);

    // VST3 Note Expression tuning delta (Dorico microtonal). Phase 24.
    // Single source of truth for noteStarted() (line 32) and notePitchbendChanged()
    // (line 71) — both call this helper before waveguideString.trigger() (lines
    // 39, 76). exchange(0.0) consume — first call (in noteStarted) tunes; held-note
    // pitch-bend updates return base unchanged (correct: NE applies once per
    // noteStarted, MPE pitch-bend updates compose multiplicatively per-block).
    if (pendingTuningSource != nullptr)
        freq = Ouaricon::NoteExpression::applyPendingTuning (*pendingTuningSource, midiNote, freq);

    return static_cast<float> (freq);
}
