/*
  ==============================================================================

    WavetableImporter.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    FFT-based wavetable import from audio files.
    Slices audio into 2048-sample frames, performs spectral analysis,
    resynthesizes clean single-cycle waveforms, and builds mipmap hierarchy.

  ==============================================================================
*/

#include "WavetableImporter.h"
#include <cmath>

// Absolute import ceiling: 30 s at 192 kHz is ~5.8M samples; 8M gives headroom
// while bounding a hostile header's allocation to ~64 MB after the 2-channel
// clamp (WR-13).
static constexpr juce::int64 kAbsoluteMaxImportSamples = 8 * 1024 * 1024;

WavetableImporter::ImportResult WavetableImporter::importFromFile (const juce::File& file)
{
    ImportResult result;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (! reader)
    {
        result.error = "Could not read audio file: " + file.getFileName();
        return result;
    }

    // Never trust header-claimed sizes (WR-13): a hostile header can claim a
    // huge length/sample-rate/channel-count. Compute the 30 s cap entirely in
    // int64 with an absolute ceiling, and only allocate the channels we use.
    if (! std::isfinite (reader->sampleRate) || reader->sampleRate <= 0.0)
    {
        result.error = "Audio file has an invalid sample rate";
        return result;
    }

    const auto maxSamples64 = juce::jmin<juce::int64> (
        reader->lengthInSamples,
        static_cast<juce::int64> (reader->sampleRate * 30.0),
        kAbsoluteMaxImportSamples);
    if (maxSamples64 <= 0)
    {
        result.error = "Audio file is empty";
        return result;
    }

    const int numSamples = static_cast<int> (maxSamples64);
    const int numChannels = juce::jlimit (1, 2, static_cast<int> (reader->numChannels));
    juce::AudioBuffer<float> buffer (numChannels, numSamples);
    reader->read (&buffer, 0, numSamples, 0, true, numChannels > 1);

    return processAudioBuffer (buffer);
}

WavetableImporter::ImportResult WavetableImporter::importFromMemory (const void* data, size_t sizeInBytes)
{
    ImportResult result;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    auto inputStream = std::make_unique<juce::MemoryInputStream> (data, sizeInBytes, false);
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (std::move (inputStream)));
    if (! reader)
    {
        result.error = "Could not decode audio data";
        return result;
    }

    // Same header-hardening as importFromFile (WR-13)
    if (! std::isfinite (reader->sampleRate) || reader->sampleRate <= 0.0)
    {
        result.error = "Audio data has an invalid sample rate";
        return result;
    }

    const auto maxSamples64 = juce::jmin<juce::int64> (
        reader->lengthInSamples,
        static_cast<juce::int64> (reader->sampleRate * 30.0),
        kAbsoluteMaxImportSamples);
    if (maxSamples64 <= 0)
    {
        result.error = "Audio data is empty";
        return result;
    }

    const int numSamples = static_cast<int> (maxSamples64);
    const int numChannels = juce::jlimit (1, 2, static_cast<int> (reader->numChannels));
    juce::AudioBuffer<float> buffer (numChannels, numSamples);
    reader->read (&buffer, 0, numSamples, 0, true, numChannels > 1);

    return processAudioBuffer (buffer);
}

WavetableImporter::ImportResult WavetableImporter::processAudioBuffer (juce::AudioBuffer<float>& buffer)
{
    ImportResult result;

    constexpr int frameSize = WavetableData::kTableSize; // 2048
    constexpr int maxFrames = WavetableData::kMaxFrames; // 256

    // Mix down to mono
    int numSamples = buffer.getNumSamples();
    std::vector<float> mono (static_cast<size_t> (numSamples));

    if (buffer.getNumChannels() >= 2)
    {
        for (int i = 0; i < numSamples; ++i)
            mono[static_cast<size_t> (i)] = (buffer.getSample (0, i) + buffer.getSample (1, i)) * 0.5f;
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
            mono[static_cast<size_t> (i)] = buffer.getSample (0, i);
    }

    // Calculate number of frames (Serum-style: divide by frame size, short files = fewer frames)
    int numFrames = numSamples / frameSize;
    if (numFrames == 0)
        numFrames = 1; // Single-cycle waveform (zero-padded)
    numFrames = juce::jmin (numFrames, maxFrames);

    // Allocate wavetable
    auto table = std::make_unique<WavetableData>();
    table->allocate (numFrames);

    // FFT setup
    static constexpr int fftOrder = 11; // log2(2048)
    juce::dsp::FFT fft (fftOrder);
    std::vector<float> fftBuffer (static_cast<size_t> (frameSize) * 2, 0.0f);

    for (int frame = 0; frame < numFrames; ++frame)
    {
        int startSample = frame * frameSize;

        // Copy frame data (zero-pad if short)
        std::fill (fftBuffer.begin(), fftBuffer.end(), 0.0f);
        int samplesToRead = juce::jmin (frameSize, numSamples - startSample);
        for (int i = 0; i < samplesToRead; ++i)
            fftBuffer[static_cast<size_t> (i)] = mono[static_cast<size_t> (startSample + i)];

        // Forward FFT
        fft.performRealOnlyForwardTransform (fftBuffer.data(), false);

        // Zero DC bin (remove offset)
        fftBuffer[0] = 0.0f;
        fftBuffer[1] = 0.0f;

        // Zero Nyquist bin (prevent artifacts)
        fftBuffer[static_cast<size_t> (frameSize)] = 0.0f;
        fftBuffer[static_cast<size_t> (frameSize) + 1] = 0.0f;

        // Inverse FFT to get clean single-cycle waveform
        fft.performRealOnlyInverseTransform (fftBuffer.data());

        // Store in level 0 of wavetable
        float* destFrame = table->getFrameData (0, frame);
        std::copy (fftBuffer.begin(), fftBuffer.begin() + frameSize, destFrame);
    }

    // Normalize all frames to consistent amplitude
    float globalMax = 0.0f;
    for (int frame = 0; frame < numFrames; ++frame)
    {
        const float* frameData = table->getFrameData (0, frame);
        for (int i = 0; i < frameSize; ++i)
            globalMax = std::max (globalMax, std::abs (frameData[i]));
    }

    if (globalMax > 0.0f)
    {
        float normalizeGain = 1.0f / globalMax;
        for (int frame = 0; frame < numFrames; ++frame)
        {
            float* destFrame = table->getFrameData (0, frame);
            for (int i = 0; i < frameSize; ++i)
                destFrame[i] *= normalizeGain;
        }
    }

    // Generate mipmaps from level 0 (reuse existing band-limiting algorithm)
    WavetableGenerator::generateMipmaps (*table);

    result.table = std::move (table);
    result.success = true;
    return result;
}
