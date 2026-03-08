/*
  ==============================================================================

    WavetableImporter.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    FFT-based wavetable import from audio files (Serum-style FFT 2048).

  ==============================================================================
*/

#pragma once
#include "WavetableData.h"
#include "WavetableGenerator.h"
#include <JuceHeader.h>
#include <memory>

class WavetableImporter
{
public:
    struct ImportResult
    {
        std::unique_ptr<WavetableData> table;
        juce::String error;
        bool success = false;
    };

    /** Import from audio file using FFT analysis (Serum-style FFT 2048).
        Supports WAV, AIFF, FLAC via JUCE AudioFormatManager. */
    static ImportResult importFromFile (const juce::File& file);

    /** Import from memory block (for drag-and-drop via WebView). */
    static ImportResult importFromMemory (const void* data, size_t sizeInBytes);

private:
    static ImportResult processAudioBuffer (juce::AudioBuffer<float>& buffer);
};
