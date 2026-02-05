/*
  ==============================================================================

    O-SpectralShaper - STFT Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "STFTProcessor.h"
#include <cmath>

// ============================================================================
// Constructor
// ============================================================================

STFTProcessor::STFTProcessor()
{
    // Initialize curves to neutral (no shaping)
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        attackCurve[0][i] = 0.0f;
        attackCurve[1][i] = 0.0f;
        sustainCurve[0][i] = 0.0f;
        sustainCurve[1][i] = 0.0f;
    }
}

// ============================================================================
// Lifecycle
// ============================================================================

void STFTProcessor::prepare(double sr)
{
    this->sampleRate = static_cast<float>(sr);
    hopTime = HOP_SIZE / static_cast<float>(sr);

    // Create Hann window table
    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        windowTable.data(),
        FFT_SIZE,
        juce::dsp::WindowingFunction<float>::hann,
        false);  // No normalization

    // Setup band boundaries (logarithmic spacing 20Hz-Nyquist)
    setupBandBoundaries(sr);

    // Calculate envelope coefficients
    auto srFloat = static_cast<float>(sr);
    fastCoeff = calculateEnvelopeCoefficient(1.0f, srFloat, HOP_SIZE);      // 1ms fast attack
    slowCoeff = calculateEnvelopeCoefficient(15.0f, srFloat, HOP_SIZE);     // 15ms slow attack
    releaseCoeff = calculateEnvelopeCoefficient(50.0f, srFloat, HOP_SIZE);  // 50ms release

    // Prepare smoothed gain values (50ms ramp time)
    // SmoothedValue is called once per FFT frame (at hop rate), NOT per sample.
    // Use frame rate (sr/HOP_SIZE) so 50ms ramp takes the correct number of frames.
    double frameRate = sr / static_cast<double>(HOP_SIZE);
    for (auto& band : bands)
    {
        band.gainSmoothed.reset(frameRate, 0.05);  // 50ms ramp at frame rate
        band.gainSmoothed.setCurrentAndTargetValue(1.0f);
    }

    reset();
}

void STFTProcessor::reset()
{
    // Clear FIFOs and FFT data
    std::fill(inputFIFO.begin(), inputFIFO.end(), 0.0f);
    std::fill(outputFIFO.begin(), outputFIFO.end(), 0.0f);
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    fifoIndex = 0;

    // Reset band states
    for (auto& band : bands)
    {
        band.prevMagnitude = 0.0f;
        band.fastEnvelope = 0.0f;
        band.slowEnvelope = 0.0f;
        band.transientActivity = 0.0f;
        band.gainSmoothed.setCurrentAndTargetValue(1.0f);
    }

    std::fill(transientActivity.begin(), transientActivity.end(), 0.0f);
}

// ============================================================================
// Sample-by-sample Processing
// ============================================================================

float STFTProcessor::processSample(float input)
{
    // Bypass mode: pass-through for null-test verification
    if (bypass)
        return input;

    // Write new sample to SECOND HALF of input FIFO (current frame's samples)
    // The first half contains the previous frame's samples (from the shift)
    inputFIFO[static_cast<size_t>(HOP_SIZE + fifoIndex)] = input;

    // Read from output FIFO (completed overlap-added samples)
    float output = outputFIFO[static_cast<size_t>(fifoIndex)];

    // Increment FIFO index
    if (++fifoIndex >= HOP_SIZE)
    {
        // CRITICAL: Shift output FIFO BEFORE processFrame()
        // The [0..255] samples have been read during this hop.
        // Move [256..511] (previous frame's tail) to [0..255] for overlap-add.
        std::copy(outputFIFO.begin() + HOP_SIZE, outputFIFO.end(), outputFIFO.begin());
        // Clear second half for new IFFT output
        std::fill(outputFIFO.begin() + HOP_SIZE, outputFIFO.end(), 0.0f);

        // Process frame: adds IFFT to the shifted output FIFO
        // outputFIFO[0..255] gets: previous_tail + current_first_half (overlap!)
        // outputFIFO[256..511] gets: current_second_half (tail for next frame)
        processFrame();

        // Shift input FIFO: [256..511] → [0..255] for next frame's overlap
        std::copy(inputFIFO.begin() + HOP_SIZE, inputFIFO.end(), inputFIFO.begin());

        fifoIndex = 0;
    }

    return output;
}

// ============================================================================
// Frame Processing (Overlap-Add STFT)
// ============================================================================

void STFTProcessor::processFrame()
{
    // Copy input FIFO to FFT buffer and apply analysis window
    // JUCE's performRealOnlyForwardTransform expects REAL samples in fftData[0..FFT_SIZE-1]
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        fftData[static_cast<size_t>(i)] = inputFIFO[static_cast<size_t>(i)] * windowTable[static_cast<size_t>(i)];
    }

    // Forward FFT: transforms real input to interleaved complex output
    // Output: fftData[0..NUM_BINS*2-1] as (real, imag) pairs for each bin
    forwardFFT.performRealOnlyForwardTransform(fftData.data(), true);

    // Phase 2.2: Per-band transient detection (reads complex magnitudes)
    detectTransients();

    // Phase 2.3: Per-band envelope shaping (modifies complex magnitudes)
    applyEnvelopeShaping();

    // Inverse FFT: transforms interleaved complex back to real
    // Output: fftData[0..FFT_SIZE-1] as real samples
    inverseFFT.performRealOnlyInverseTransform(fftData.data());

    // Overlap-add: Add IFFT output directly to output FIFO (no synthesis window).
    // Analysis-only Hann windowing satisfies COLA at 50% overlap: w(n) + w(n+N/2) = 1.0
    // Applying a second (synthesis) window breaks COLA — Hann^2 sum is NOT constant,
    // creating amplitude modulation at the hop rate (~172Hz) which introduces low-freq artifacts.
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        outputFIFO[static_cast<size_t>(i)] += fftData[static_cast<size_t>(i)];
    }
}

// ============================================================================
// Band Boundaries Setup (Logarithmic Spacing)
// ============================================================================

void STFTProcessor::setupBandBoundaries(double sampleRate)
{
    const float nyquist = static_cast<float>(sampleRate) * 0.5f;
    const float minFreq = 20.0f;
    const float maxFreq = nyquist;

    // Logarithmic frequency spacing
    const float logMin = std::log(minFreq);
    const float logMax = std::log(maxFreq);
    const float logStep = (logMax - logMin) / NUM_BANDS;

    for (int band = 0; band < NUM_BANDS; ++band)
    {
        // Calculate band frequency range
        float freqStart = std::exp(logMin + band * logStep);
        float freqEnd = std::exp(logMin + (band + 1) * logStep);

        // Convert to FFT bin indices
        int binStart = static_cast<int>(freqStart * FFT_SIZE / sampleRate);
        int binEnd = static_cast<int>(freqEnd * FFT_SIZE / sampleRate);

        // Clamp to valid range
        binStart = juce::jlimit(0, NUM_BINS - 1, binStart);
        binEnd = juce::jlimit(binStart + 1, NUM_BINS, binEnd);

        bandBoundaries[band].startBin = binStart;
        bandBoundaries[band].endBin = binEnd;
    }
}

// ============================================================================
// Transient Detection (Phase 2.2)
// ============================================================================

void STFTProcessor::detectTransients()
{
    // Phase 3.3: Store FFT magnitudes for visualization
    for (int bin = 0; bin < NUM_BINS; ++bin)
    {
        float real = fftData[bin * 2];
        float imag = fftData[bin * 2 + 1];
        lastMagnitudes[bin] = std::sqrt(real * real + imag * imag);
    }

    // Process each frequency band independently
    for (int band = 0; band < NUM_BANDS; ++band)
    {
        const int startBin = bandBoundaries[band].startBin;
        const int endBin = bandBoundaries[band].endBin;

        // Calculate band magnitude (RMS of all bins in this band)
        float bandMagnitude = 0.0f;
        for (int bin = startBin; bin < endBin; ++bin)
        {
            float real = fftData[bin * 2];
            float imag = fftData[bin * 2 + 1];
            float binMagnitude = std::sqrt(real * real + imag * imag);
            bandMagnitude += binMagnitude * binMagnitude;
        }
        bandMagnitude = std::sqrt(bandMagnitude / (endBin - startBin));

        // Spectral flux: positive-only magnitude difference
        // (Only increases in magnitude indicate transients)
        float spectralFlux = juce::jmax(0.0f, bandMagnitude - bands[band].prevMagnitude);
        bands[band].prevMagnitude = bandMagnitude;

        // Apply sensitivity scaling
        spectralFlux *= sensitivity * 10.0f;  // Scale to reasonable range

        // Dual envelope followers:
        // - Fast attack (1ms): Captures transient onsets
        // - Slow attack (15ms): Follows overall energy
        float fastTarget = spectralFlux;
        float slowTarget = spectralFlux;

        // One-pole lowpass filters
        bands[band].fastEnvelope = fastTarget + fastCoeff * (bands[band].fastEnvelope - fastTarget);
        bands[band].slowEnvelope = slowTarget + slowCoeff * (bands[band].slowEnvelope - slowTarget);

        // Transient activity: Difference between fast and slow envelopes
        // Normalized to 0.0-1.0 range
        float difference = bands[band].fastEnvelope - bands[band].slowEnvelope;
        float transient = juce::jlimit(0.0f, 1.0f, difference * 2.0f);

        // Apply release envelope (smooth decay)
        bands[band].transientActivity = transient + releaseCoeff * (bands[band].transientActivity - transient);

        // Store for UI visualization
        transientActivity[band] = bands[band].transientActivity;
    }
}

// ============================================================================
// Envelope Shaping (Phase 2.3)
// ============================================================================

void STFTProcessor::applyEnvelopeShaping()
{
    // Read active curve buffers (atomic, lock-free — separate indices)
    int activeAttack = activeAttackBuffer.load(std::memory_order_acquire);
    int activeSustain = activeSustainBuffer.load(std::memory_order_acquire);
    const auto& attackCurveData = attackCurve[activeAttack];
    const auto& sustainCurveData = sustainCurve[activeSustain];

    // Build per-bin gain array FIRST, then apply once.
    // This prevents low-frequency bands that share the same FFT bin from
    // multiplying their gains together (bands 0-6 all map to bin 0 at 44.1kHz
    // with 512-point FFT, causing up to 7× multiplicative gain stacking).
    std::array<float, NUM_BINS> binGains;
    binGains.fill(1.0f);

    for (int band = 0; band < NUM_BANDS; ++band)
    {
        const int startBin = bandBoundaries[band].startBin;
        const int endBin = bandBoundaries[band].endBin;

        // Get transient activity for this band (0.0 = sustain, 1.0 = transient)
        float transient = bands[band].transientActivity;
        float sustain = 1.0f - transient;

        // Attack shaping: curve value maps directly to gain range
        // Curve -1.0 → -18dB, 0.0 → 0dB, +1.0 → +18dB (weighted by transient activity)
        float attackDB = attackCurveData[band] * MAX_SHAPE_DB;
        float attackGain = juce::Decibels::decibelsToGain(attackDB * transient);

        // Sustain shaping: same range, weighted by sustain (1 - transient)
        float sustainDB = sustainCurveData[band] * MAX_SHAPE_DB;
        float sustainGain = juce::Decibels::decibelsToGain(sustainDB * sustain);

        // Combined target gain (multiplicative)
        float targetGain = attackGain * sustainGain;

        // Smooth gain changes (50ms ramp to avoid clicks)
        bands[band].gainSmoothed.setTargetValue(targetGain);
        float smoothedGain = bands[band].gainSmoothed.getNextValue();

        // ASSIGN gain per bin (last band wins for shared bins — highest freq takes priority)
        for (int bin = startBin; bin < endBin; ++bin)
        {
            binGains[static_cast<size_t>(bin)] = smoothedGain;
        }
    }

    // Apply per-bin gains to FFT data (each bin processed exactly once)
    for (int bin = 0; bin < NUM_BINS; ++bin)
    {
        fftData[bin * 2] *= binGains[static_cast<size_t>(bin)];
        fftData[bin * 2 + 1] *= binGains[static_cast<size_t>(bin)];
    }
}

// ============================================================================
// Parameter Setters
// ============================================================================

void STFTProcessor::setAttackCurve(const std::array<float, NUM_BANDS>& curve)
{
    // Write to inactive buffer, then swap (separate index from sustain)
    int inactive = 1 - activeAttackBuffer.load(std::memory_order_relaxed);
    std::copy(curve.begin(), curve.end(), attackCurve[inactive].begin());
    activeAttackBuffer.store(inactive, std::memory_order_release);
}

void STFTProcessor::setSustainCurve(const std::array<float, NUM_BANDS>& curve)
{
    // Write to inactive buffer, then swap (separate index from attack)
    int inactive = 1 - activeSustainBuffer.load(std::memory_order_relaxed);
    std::copy(curve.begin(), curve.end(), sustainCurve[inactive].begin());
    activeSustainBuffer.store(inactive, std::memory_order_release);
}

void STFTProcessor::setAttackTime(float ms)
{
    attackTimeMs = ms;
}

void STFTProcessor::setSustainTime(float ms)
{
    sustainTimeMs = ms;
}

void STFTProcessor::setSensitivity(float value)
{
    sensitivity = juce::jlimit(0.0f, 1.0f, value);
}

// ============================================================================
// Helper Methods
// ============================================================================

float STFTProcessor::calculateEnvelopeCoefficient(float timeMs, float sr, float hopSize)
{
    // Calculate one-pole filter coefficient for envelope follower
    // Time constant adjusted for hop rate (not per-sample)
    float timeSeconds = timeMs / 1000.0f;
    float hopRate = sr / hopSize;
    return std::exp(-1.0f / (timeSeconds * hopRate));
}
