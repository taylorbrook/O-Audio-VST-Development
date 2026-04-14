/*
  ==============================================================================

    FluteSynthVoice.h
    O-Wind - Physical Modeling Flute Synthesiser Voice
    Ouaricon Audio
    Developer: Taylor Brook

    SynthesiserVoice subclass orchestrating the jet-drive waveguide flute model.
    Per-sample pipeline at 2x oversampled rate:
    JetExciter -> Jet Delay -> JetNonlinearity -> DCBlocker ->
    BoreWaveguide -> output + feedback loop.
    Reads APVTS parameters once per block.

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/JetExciter.h"
#include "DSP/JetNonlinearity.h"
#include "DSP/DCBlocker.h"
#include "DSP/BoreWaveguide.h"
#include "DSP/InstrumentPresets.h"

class TuningEngine;

class FluteSynthVoice : public juce::SynthesiserVoice
{
public:
    explicit FluteSynthVoice (juce::AudioProcessorValueTreeState* apvts,
                              TuningEngine* tuning);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void prepareToPlay (double sampleRate, int maxBlockSize);
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    // Query oversampling latency (called by processor for setLatencySamples)
    float getOversamplingLatency() const;

private:
    void updateParametersFromAPVTS();

    juce::AudioProcessorValueTreeState* parameters = nullptr;
    TuningEngine* tuningEngine = nullptr;

    // Current preset internal coefficients (applied on startNote or param change)
    InstrumentPreset currentPreset = InstrumentPresets::concertFlute;
    void applyPresetCoefficients();

    // DSP components (all prepared at internal oversampled rate)
    JetExciter jetExciter;
    JetNonlinearity jetNonlinearity;
    DCBlocker dcBlocker;
    BoreWaveguide boreWaveguide;

    // Jet delay line (Lagrange3rd for real-time embouchure modulation)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> jetDelay { 1024 };

    // Per-voice 2x oversampling (1 channel, polyphase IIR)
    juce::dsp::Oversampling<float> oversampling {
        1, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, false
    };
    juce::AudioBuffer<float> tempBuffer;  // native-rate temp for up/downsample

    // SmoothedValue for total loop delay (bore + jet, split in render loop)
    // Reset at internal (oversampled) rate so getNextValue advances correctly
    juce::SmoothedValue<float> totalDelaySmoothed;
    juce::SmoothedValue<float> embouchureSmoothed;

    // Pitch vibrato (modulates bore delay, not breath pressure)
    float vibratoPhase = 0.0f;
    float vibratoPhaseInc = 0.0f;
    float vibratoDepthParam = 0.0f;
    float vibratoTremoloDepthParam = 0.0f;  // amplitude mod locked to vibrato phase
    float vibratoDriftDepthParam = 0.5f;   // scales drift oscillator amplitude (0=static, 1=full)
    float vibratoDriftSpeedParam = 0.4f;   // base frequency for drift oscillators (Hz)

    // Vibrato humanization
    float vibratoOnsetMs = 300.0f;
    int vibratoOnsetSamples = 0;
    int samplesSinceNoteOn = 0;
    float vibratoRateDriftPhase = 0.0f;
    float vibratoDepthDriftPhase = 0.0f;
    float vibratoRateDriftInc = 0.0f;
    float vibratoDepthDriftInc = 0.0f;

    // Dynamic filter delay compensation (computed once per block)
    float filterDelayCompensation = 2.0f;

    // Register transition state (stored per-block for per-sample use)
    float breathPressureParam = 0.5f;
    float currentOverblowEase = 0.6f;

    // Voice state
    float outputGainLinear = 1.0f;
    float currentFrequency = 440.0f;
    int currentMidiNote = 60;
    double internalSampleRate = 88200.0;  // 2x oversampled
    double nativeSampleRate = 44100.0;

    // Preset change tracking
    int lastPresetIndex = -1;

    // Energy tracking for voice cleanup
    int silentSampleCount = 0;
    static constexpr int silentThreshold = 512;  // ~6ms at 88.2kHz internal rate
    static constexpr float energyThreshold = 0.0001f;

    // Release tail fade: ensures voice clearing even if waveguide has residual energy
    float releaseFade = 1.0f;
    float releaseFadeInc = 0.0f;
    bool releaseFading = false;

    // Deferred jet exciter release: when ADSR is enabled, breath excitation
    // continues through the ADSR release so the waveguide has energy to shape
    bool pendingJetRelease = false;

    // CC state for MPE
    float ccBreathPressure = 0.0f;
    float ccEmbouchure = 0.0f;
    float ccVibratoDepth = 0.0f;
    int pitchWheelValue = 8192;

    // Pitch bend state
    float pitchBendSemitones = 0.0f;
    static constexpr float pitchBendRange = 2.0f;  // +/- 2 semitones

    // Chiff: pitch overshoot state (bore delay starts short, settles to target)
    float pitchOvershootFactor = 1.0f;   // current multiplier on total delay (< 1.0 = sharper)
    float pitchOvershootTarget = 1.0f;   // always 1.0 (steady-state = no overshoot)
    float pitchOvershootCoeff = 0.0f;    // one-pole smoothing coefficient
    float attackChiffParam = 0.5f;       // cached from APVTS

    // Per-note humanization (drawn at noteOn, scaled by humanize param)
    juce::Random voiceRng;
    float humanizeParam = 0.3f;
    float attackTimeScale = 1.0f;         // +/-20% of base attack time
    float noiseBurstScale = 1.0f;         // +/-30% of chiff noise amplitude
    float embouchureDelayOffset = 0.0f;   // +/-1% of bore delay (timbre shift)
    float strouhalFreqScale = 1.0f;       // +/-10% of Strouhal center freq
    float vibratoOnsetOffsetMs = 0.0f;    // +/-50ms of vibrato onset delay

    // Growl: secondary low-frequency sawtooth oscillator modulating bore feedback
    float growlParam = 0.0f;              // cached from APVTS (0 = off, 1 = full)
    float growlPhase = 0.0f;              // sawtooth phase [0, 1)
    float growlPhaseInc = 0.0f;           // phase increment per oversampled sample
    float growlFreq = 90.0f;             // randomized 70-120 Hz per note

    // ADSR envelope (optional amplitude shaping, non-physical)
    enum class ADSRStage { Idle, Attack, Decay, Sustain, Release };
    ADSRStage adsrStage = ADSRStage::Idle;
    float adsrLevel = 0.0f;               // current envelope level [0, 1]
    float adsrIncrement = 0.0f;           // per-sample increment for current stage
    float adsrSustainLevel = 0.8f;        // cached sustain target
    float adsrReleaseInc = 0.0f;          // cached release increment (computed at noteOff)
    bool adsrEnabled = false;             // cached from APVTS
    float adsrAttackSeconds = 0.01f;
    float adsrDecaySeconds = 0.1f;
    float adsrReleaseSeconds = 0.2f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FluteSynthVoice)
};
