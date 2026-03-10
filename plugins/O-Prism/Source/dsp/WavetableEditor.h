/*
  ==============================================================================

    WavetableEditor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    Working copy manager for wavetable harmonic editing.
    Provides FFT analysis, harmonic editing, frame operations, and save.

  ==============================================================================
*/

#pragma once
#include "WavetableData.h"
#include "WavetableGenerator.h"
#include "UserWavetableManager.h"
#include <JuceHeader.h>
#include <memory>
#include <vector>

class WavetableEditor
{
public:
    WavetableEditor();

    /** Deep-copy source table into working copy and generate mipmaps. */
    void loadTable (const WavetableData* sourceTable);

    /** Release working copy. */
    void clearWorkingTable();

    /** Get harmonic magnitudes for a frame (bins 1..numBins). */
    std::vector<float> getFrameHarmonics (int frameIndex, int numBins) const;

    /** Set harmonic magnitudes for a frame (triggers iFFT + mipmap regen). */
    void setFrameHarmonics (int frameIndex, const std::vector<float>& magnitudes);

    /** Get time-domain waveform for a frame (2048 samples). */
    std::vector<float> getFrameWaveform (int frameIndex) const;

    /** Get downsampled waveforms for all frames (for strip display). */
    std::vector<std::vector<float>> getAllFrameWaveforms (int samplesPerFrame) const;

    /** Normalize selected frames to peak ±1.0 (per-frame or global). */
    void normalizeFrames (const std::vector<int>& frames, bool perFrame);

    /** Apply linear fade-in/out at frame boundaries. */
    void fadeEdges (const std::vector<int>& frames, float fadePercent);

    /** Reverse audio within each selected frame. */
    void reverseFrames (const std::vector<int>& frames);

    /** Reverse the ordering of selected frames. */
    void reverseOrder (const std::vector<int>& frames);

    /** Spectral smoothing: 6dB/oct rolloff above cutoff harmonic. */
    void smoothFrames (const std::vector<int>& frames, float strength);

    /** Save working table as user wavetable. Returns true on success. */
    bool saveAsUserWavetable (const juce::String& name, UserWavetableManager& manager);

    /** Get mutable pointer to working table (for oscillator preview). */
    WavetableData* getWorkingTable() { return workingTable.get(); }

    /** Get const pointer to working table. */
    const WavetableData* getWorkingTable() const { return workingTable.get(); }

    int getNumFrames() const;
    bool hasWorkingTable() const { return workingTable != nullptr; }

private:
    std::unique_ptr<WavetableData> workingTable;
    juce::dsp::FFT fft { 11 }; // 2048-point

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableEditor)
};
