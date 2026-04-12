/*
  ==============================================================================

    WaveguideString.h
    Bidirectional Digital Waveguide String Model - Phase 2.2-2.5
    Ouaricon Audio
    Developer: Taylor Brook

    Implements true waveguide synthesis with separate upper/lower rails,
    bridge filter, nut filter, loop damping, stiffness filter, and
    material system for realistic string behavior with piano-like
    inharmonicity and material-specific timbres.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "PluckExciter.h"
#include "StiffnessFilter.h"
#include "StringMaterial.h"

/**
 * Bidirectional Digital Waveguide String Model
 *
 * Models physical wave propagation in a string using two delay lines:
 * - Upper Rail: Right-traveling waves (toward bridge)
 * - Lower Rail: Left-traveling waves (toward nut)
 *
 * Filters:
 * - Bridge Filter: Frequency-dependent reflection at bridge
 * - Nut Filter: Inverted reflection at nut (sign inversion)
 * - Loop Damping: Material-based energy loss per cycle
 * - Stiffness Filter: Allpass cascade for inharmonicity/dispersion (Phase 2.4)
 */
class WaveguideString
{
public:
    WaveguideString();
    ~WaveguideString();

    /**
     * Prepare for playback
     * @param sampleRate Current sample rate
     * @param maxBlockSize Maximum expected block size
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Trigger string excitation
     * @param frequency Fundamental frequency in Hz
     * @param velocity Note velocity (0.0 - 1.0)
     * @param position Pluck position (0.0 = nut, 1.0 = bridge)
     * @param hardness Finger hardness (0.0 = soft, 1.0 = hard)
     */
    void trigger(double frequency, float velocity, float position, float hardness);

    /**
     * Process one sample through the waveguide
     * @return Output sample
     */
    float processSample();

    /**
     * Check if string is still sounding
     */
    bool isActive() const;

    /**
     * Reset string to silent state
     */
    void reset();

    /**
     * Set tonal damping (affects high-frequency decay rate / timbre)
     * @param damping 0.0 = bright, 1.0 = dark/muted
     */
    void setDamping(float damping);

    /**
     * Set decay time (affects overall sustain duration via feedback coefficient)
     * @param decayTimeSeconds Time in seconds for signal to decay to -60dB
     */
    void setDecayTime(float decayTimeSeconds);

    /**
     * Set brightness (affects tone color via bridge filter)
     * @param brightness 0.0 = dark/muted, 1.0 = bright
     */
    void setBrightness(float brightness);

    /**
     * Set bridge brightness (v1.3.0) - direct control of bridge filter character
     * @param bridgeBrightness 0.0 = very dark/damped reflection, 1.0 = bright/open reflection
     * This provides more direct waveguide control than the general brightness parameter
     */
    void setBridgeBrightness(float bridgeBrightness);

    /**
     * Set attack noise amount (v1.3.0) - independent of material noise setting
     * @param noiseAmount 0.0 = clean attack, 1.0 = noisy/scratchy attack
     * Overrides the material's default noise content for user control
     */
    void setAttackNoise(float noiseAmount);

    /**
     * Set pluck position along string (0.0 = nut, 1.0 = bridge)
     * @param position Normalized position 0.0-1.0
     */
    void setPluckPosition(float position);

    /**
     * Set glissando excitation amount (v1.26.0 - Brush profile)
     * @param amount 0.0 = full deliberate pluck, 1.0 = lightest brush sweep
     */
    void setGlissandoExcitation(float amount);

    /**
     * Set playing technique
     * @param technique Technique to apply (Normal, Harmonic, Muted, PresDeLaTable)
     */
    void setTechnique(PlayingTechnique technique);

    /**
     * Set string stiffness (affects inharmonicity/dispersion)
     * @param stiffness 0.0 = flexible/harmonic, 1.0 = stiff/piano-like
     */
    void setStiffness(float stiffness);

    /**
     * Set string tension (affects brightness and resonance)
     * @param tension 0.0 = loose/dark, 1.0 = tight/bright/resonant
     * Higher tension increases bridge filter Q and brightness
     * Does NOT affect pitch (compensated in delay line)
     */
    void setTension(float tension);

    /**
     * Set string gauge (affects mass and damping characteristics)
     * @param gauge 0.0 = thin/bright/quick, 1.0 = thick/dark/heavy
     * Thicker gauge increases damping and slows attack transient
     * Does NOT affect pitch
     */
    void setGauge(float gauge);

    /**
     * Set string length scaling (affects harmonic decay character)
     * @param length 0.0 = short/tight decay, 1.0 = long/diffuse decay
     * Affects feedback characteristics without changing pitch
     * Does NOT affect pitch (only decay envelope shape)
     */
    void setLength(float length);

    /**
     * Set string material (Phase 2.5)
     * @param material StringMaterial with physical properties
     */
    void setMaterial(const StringMaterial& material);

    /**
     * v1.33.0: Set dampening state (étouffé)
     * When active, rapidly reduces feedback over ~50ms to simulate string muting
     * @param active true = start dampening, false = stop dampening
     */
    void setDampening(bool active);

    /**
     * Update frequency in real-time (for pitch bend)
     * @param frequency New frequency in Hz
     */
    void setFrequency(double frequency);

    /**
     * Get current material settings
     * @return Current StringMaterial
     */
    const StringMaterial& getMaterial() const { return currentMaterial; }

private:
    // Delay lines for bidirectional propagation
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> upperRail;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> lowerRail;

    // v2.1.8: Minimal first-order lowpass with directly accessible state.
    // Coefficient math matches juce::dsp::IIR::Coefficients::makeFirstOrderLowPass
    // (bilinear transform, TDF-II state update) so audible behavior is identical
    // to the previous juce::dsp::IIR::Filter<float> implementation. The plain-POD
    // state layout lets us trivially snapshot a filter into a shadow copy for
    // crossfading coefficient changes (see applyPendingFilterUpdates).
    struct OnePoleLPF
    {
        float b0 = 1.0f;
        float b1 = 0.0f;
        float a1 = 0.0f;
        float state = 0.0f;

        inline float processSample(float x) noexcept
        {
            const float y = b0 * x + state;
            state = b1 * x - a1 * y;
            return y;
        }

        void reset() noexcept { state = 0.0f; }

        void setCutoff(float cutoffHz, double sampleRate) noexcept
        {
            const double n = std::tan(juce::MathConstants<double>::pi * (static_cast<double>(cutoffHz) / sampleRate));
            const double a0 = n + 1.0;
            const double invA0 = 1.0 / a0;
            b0 = static_cast<float>(n * invA0);
            b1 = static_cast<float>(n * invA0);
            a1 = static_cast<float>((n - 1.0) * invA0);
        }
    };

    // Filters
    OnePoleLPF bridgeFilter;   // Frequency-dependent reflection
    OnePoleLPF nutFilter;      // Inverted reflection
    OnePoleLPF loopDamping;    // Material-based damping
    StiffnessFilter stiffnessFilter;              // Inharmonicity/dispersion (Phase 2.4)

    // v2.1.8: Shadow filter copies for crossfading coefficient transitions.
    // When a parameter update lands we snapshot the active filters into these
    // shadows (keeping their old coefficients + state) and run them in parallel
    // for FILTER_CROSSFADE_LENGTH samples, linearly blending the old and new
    // filter outputs to eliminate audible clicks on fast parameter sweeps.
    OnePoleLPF bridgeFilterShadow;
    OnePoleLPF nutFilterShadow;
    OnePoleLPF loopDampingShadow;
    int filterCrossfadeRemaining = 0;
    static constexpr int FILTER_CROSSFADE_LENGTH = 64;

    // Pluck excitation generator (Phase 2.3)
    PluckExciter exciter;

    // State
    double currentSampleRate = 44100.0;
    double currentFrequency = 440.0;
    float pluckPosition = 0.5f;  // 0.0 = nut, 1.0 = bridge

    // Energy tracking for voice stealing
    float currentEnergy = 0.0f;
    float energyDecayRate = 0.9999f;

    // Parameters
    float dampingAmount = 0.7f;    // Material-based damping
    float brightnessAmount = 0.5f; // Tone brightness
    float stiffnessAmount = 0.2f;  // Final computed stiffness (Phase 2.4)

    // Material-aware stiffness system (v1.0.3 fix)
    float materialStiffness = 0.2f;      // Base stiffness from material
    float userStiffnessModifier = 0.5f;  // User slider value (0-1, 0.5 = neutral)

    // Material-aware damping system (v1.0.4 fix, renamed to timbre in v1.1.0)
    float materialDamping = 0.3f;        // Base damping from material
    float userDampingModifier = 0.5f;    // User timbre slider value (0-1, 0.5 = neutral)

    // Decay time system (v1.1.0) - feedback coefficient for true sustain control
    float decayTimeSeconds = 5.0f;       // Time to decay to -60dB
    float feedbackCoefficient = 0.9999f; // Computed from decayTime and frequency

    // Advanced string parameters (v1.2.0) - timbre modifiers without pitch effect
    float tensionAmount = 0.5f;          // String tension (0=loose, 1=tight) - affects resonance/Q
    float gaugeAmount = 0.5f;            // String gauge (0=thin, 1=thick) - affects mass/damping
    float lengthAmount = 0.5f;           // String length (0=short, 1=long) - affects decay character
    float bridgeBrightnessAmount = 0.5f; // v1.3.0: Bridge brightness (0=dark, 1=bright) - direct bridge control

    // v1.33.0: Étouffé dampening system
    bool dampening = false;
    float dampeningMultiplier = 1.0f;
    float dampeningDecayRate = 0.999f; // Recomputed in prepare() from sample rate

    // v2.2.1: Pedal buzz — filtered noise burst during étouffé to simulate palm/felt friction
    OnePoleLPF buzzLowCut;
    OnePoleLPF buzzHighCut;
    float buzzEnvelope = 0.0f;
    float buzzCapturedEnergy = 0.0f;
    juce::Random buzzRandom;

    // Material system (Phase 2.5)
    StringMaterial currentMaterial;

    // v1.7.10: Thread-safe filter coefficient updates
    // Coefficients are computed on parameter-change thread but applied on audio thread
    std::atomic<bool> filterUpdatePending { false };
    std::atomic<float> pendingBridgeCutoff { 1000.0f };
    std::atomic<float> pendingNutCutoff { 2000.0f };
    std::atomic<float> pendingDampingCutoff { 5000.0f };

    /**
     * Calculate final stiffness from material base and user modifier
     * User modifier at 0.5 = material value unchanged
     * User modifier at 0.0 = half material value
     * User modifier at 1.0 = 1.5x material value
     */
    float calculateFinalStiffness() const
    {
        return materialStiffness * (0.5f + userStiffnessModifier * 1.0f);
    }

    /**
     * Calculate final damping from material base and user modifier (v1.0.4)
     * userDampingModifier comes from: 1.0 - timbre slider
     * So timbre=1.0 → modifier=0.0 → less damping (brighter)
     *    timbre=0.0 → modifier=1.0 → more damping (darker)
     */
    float calculateFinalDamping() const
    {
        // Modifier range: 0.5x to 1.5x of material damping
        return materialDamping * (0.5f + userDampingModifier);
    }

    /**
     * Calculate feedback coefficient from decay time and frequency (v1.1.0)
     * For a signal to decay to -60dB in T seconds at frequency f:
     * coefficient = 10^(-3 / (T * f))
     * This gives the per-sample decay needed for the waveguide loop.
     * v1.2.0: Length parameter affects decay character - longer strings decay slower
     */
    float calculateFeedbackCoefficient() const
    {
        // Number of waveguide cycles per second = frequency
        // Samples per cycle = sampleRate / frequency
        // Total samples for decay = decayTimeSeconds * sampleRate
        // Cycles for decay = decayTimeSeconds * frequency
        // Per-cycle decay = 10^(-3 / cycles) = 10^(-3 / (T * f))

        // v1.2.0: Length modifier - longer strings (high length) have slower decay
        // Length=0 → 0.7x decay time (faster), Length=0.5 → 1.0x, Length=1.0 → 1.6x (slower)
        // This creates audible difference: short strings = punchy/quick, long strings = sustained/diffuse
        float lengthDecayModifier = 0.7f + lengthAmount * 0.9f;

        float effectiveDecayTime = decayTimeSeconds * lengthDecayModifier;
        float cyclesForDecay = effectiveDecayTime * static_cast<float>(currentFrequency);
        float perCycleDecay = std::pow(10.0f, -3.0f / cyclesForDecay);
        return juce::jlimit(0.9f, 0.99999f, perCycleDecay);
    }

    /**
     * v1.3.1: Struct to hold computed filter cutoff frequencies
     * Used by both updateFilters() and calculateFilterGroupDelay() to avoid duplication
     */
    struct FilterCutoffs
    {
        float bridgeCutoffHz;
        float nutCutoffHz;
        float dampingCutoffHz;
    };

    /**
     * v1.3.1: Calculate all filter cutoff frequencies based on current parameters
     * This centralizes the calculation that was previously duplicated
     */
    FilterCutoffs calculateFilterCutoffs() const;

    /**
     * Update all filter coefficients based on current parameters
     * v1.7.10: Now stores cutoffs atomically for deferred application
     */
    void updateFilters();

    /**
     * v1.7.10: Apply pending filter coefficient updates on audio thread
     * Called at start of processSample() to ensure thread-safe updates
     */
    void applyPendingFilterUpdates();

    /**
     * Calculate delay length for half the string (one rail)
     */
    float calculateRailDelay() const;

    /**
     * Calculate total group delay from all filters in the feedback loop (v1.1.1)
     * This varies with brightness/damping settings and must be compensated
     * in the delay line length to maintain correct pitch.
     */
    float calculateFilterGroupDelay() const;

    /**
     * Energy threshold for considering voice inactive
     */
    static constexpr float ENERGY_THRESHOLD = 0.0001f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveguideString)
};
