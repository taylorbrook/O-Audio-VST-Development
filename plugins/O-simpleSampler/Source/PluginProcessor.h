/*
   This file is part of O-simpleSampler, an Ouaricon Audio plugin.
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

    O-simpleSampler - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical keyboard sampler.

    Stage 2.1 (Core Playable Sampler): a polyphonic, MIDI-playable sampler — the
    single embedded piano.wav read through a fractional-read varispeed ("Repitch") head,
    isolated by start/end region, anti-aliased, shaped by a per-voice amp ADSR +
    VCA + velocity sensitivity, tuned relative to the live Root Key. Decode /
    resample / atomic-publish of the source happens OFF the audio thread
    (prepareToPlay / AsyncUpdater). Loop / reverse / Stretch / Vintage / filter
    land Phase 2.2; viz taps + render-harness land Phase 2.3. WebView UI Stage 3.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <vector>
#include "dsp/WindowLuts.h"     // Hann LUT for Stretch SOLA — built once, shared read-only
#include "SamplerVizAnalyzer.h" // VizRing (lock-free audio-writer / msg-reader scope/spectrum tap)

// Forward declarations — the custom voice/sound are pulled in only by the .cpp.
// NB: named Sample{Voice,Sound}, NOT Sampler{Voice,Sound} — juce::SamplerVoice /
// juce::SamplerSound exist and JuceHeader.h does `using namespace juce;`, so the
// `Sampler`-prefixed names would be ambiguous (cf. regionStart/regionEnd vs juce::end).
class SampleVoice;
class SampleSound;

//==============================================================================
// Parameter identifiers — single source of truth for APVTS IDs.
// Referenced by the processor now; by the sampler voices / param-push (Stage 2)
// and WebView relays/attachments (Stage 3) later. IDs/ranges/defaults are
// authoritative per .planning/parameter-spec.md (20 params).
//
// v1.1.0: source selection is no longer parameterised — the plugin starts on its
// one embedded source and Load… / drag-drop is the only way to change it.
namespace OSimpleSampler::ParamIDs
{
    // Region (start/end · loop · reverse)
    // NB: identifiers are regionStart/regionEnd (not start/end) — a bare `end`
    // collides with juce::end (RangedDirectoryIterator) under `using namespace`.
    // The APVTS string IDs stay "start"/"end" (parameter-spec contract unchanged).
    inline constexpr auto regionStart     = "start";           // 0–100 % of source
    inline constexpr auto regionEnd       = "end";             // 0–100 % of source
    inline constexpr auto loopMode        = "loopMode";        // choice: off/forward/ping-pong
    inline constexpr auto loopStart       = "loopStart";       // 0–100 % of region
    inline constexpr auto loopEnd         = "loopEnd";         // 0–100 % of region
    inline constexpr auto loopCrossfade   = "loopCrossfade";   // 0–500 ms
    inline constexpr auto reverse         = "reverse";         // bool

    // Pitch (root · Repitch/Stretch · tune/fine)
    inline constexpr auto rootKey         = "rootKey";         // int MIDI 0–127
    inline constexpr auto pitchMode       = "pitchMode";       // choice: Repitch/Stretch
    inline constexpr auto tune            = "tune";            // int −24–+24 st
    inline constexpr auto fine            = "fine";            // −100–+100 cents

    // Vintage
    inline constexpr auto vintage         = "vintage";         // 0–100 % (bypass at 0)

    // Filter (resonant low-pass)
    inline constexpr auto filterCutoff    = "filterCutoff";    // 20–20000 Hz (log)
    inline constexpr auto filterResonance = "filterResonance"; // 0–100 %

    // Amplitude envelope (per-voice ADSR)
    inline constexpr auto ampAttack       = "ampAttack";       // 0–5 s
    inline constexpr auto ampDecay        = "ampDecay";        // 0–5 s
    inline constexpr auto ampSustain      = "ampSustain";      // 0–1 (0–100 %)
    inline constexpr auto ampRelease      = "ampRelease";      // 0–5 s

    // Voice / Output
    inline constexpr auto velToAmp        = "velToAmp";        // 0–100 %
    inline constexpr auto outputLevel     = "outputLevel";     // master trim (dB)
}

//==============================================================================
class OSimpleSamplerAudioProcessor : public juce::AudioProcessor,
                                     private juce::AudioProcessorValueTreeState::Listener,
                                     private juce::AsyncUpdater
{
public:
    OSimpleSamplerAudioProcessor();
    ~OSimpleSamplerAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simpleSampler"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; } // max amp release

    //==========================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Public access to APVTS for the editor (Stage 3 relays/attachments).
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    // Loaded-source identity (custom, non-APVTS state). "embedded:piano" by
    // default, an "embedded:<name>" built-in, or a user file path once Stage 2.3
    // wires real loading. Persisted alongside the APVTS tree so a session restores
    // the same source.
    const juce::String& getSourceIdentity() const noexcept { return currentSourceIdentity; }
    void setSourceIdentity (const juce::String& id) { currentSourceIdentity = id; }

    //==========================================================================
    // Source loading (Stage 3.1 / FUNC-03). The drag-drop streaming handlers are
    // the C++ side of the WebView bridge — the Stage-3 editor registers JS that
    // CALLS them via withNativeFunction. Single source: Start -> Chunk -> Commit.
    // All decode/resample/publish runs on the MESSAGE thread (native-fn callbacks
    // run there); the audio thread only ever sees a fully-built buffer published
    // via the atomic shared_ptr swap. Decode uses juce::Base64::convertFromBase64
    // (standard btoa() output — NOT MemoryBlock::fromBase64Encoding). Returns the
    // bool "ok" the JS awaits (toasts on false).
    bool dropSampleStart  (const juce::String& sessionId, const juce::String& name = {});
    bool dropSampleChunk  (const juce::String& sessionId, const juce::String& name,
                           const juce::String& base64);
    bool dropSampleCommit (const juce::String& sessionId, const juce::String& name,
                           const juce::String& base64);

    // File-picker fallback (always works where drag-drop doesn't). Async, message
    // thread. The Stage-3 "Load…" button calls this. Same decode path as drop.
    void loadSourceFromFileChooser();

    // Whether the last loaded source was truncated to the kMaxSourceSeconds (30 s)
    // cap (the Stage-3 UI surfaces a notice). Set by every decode that resamples.
    bool wasLastLoadTruncated() const noexcept { return lastLoadTruncated.load (std::memory_order_relaxed); }

    //==========================================================================
    // On-screen keyboard: the editor injects note on/off from the WebView (any
    // thread). Queued via a MidiMessageCollector and merged into processBlock's
    // MIDI stream so UI notes drive the sampler identically to host MIDI.
    void handleUiMidi (int noteNumber, bool noteOn, float velocity);

    // Lead-voice display atomics (Phase 2.2a; consumed by the Stage-3 filter curve).
    // displayK = 1/Q = JUCE's R2 — feeds SubFilterCurve::magnitudeDb directly.
    float getDisplayCutoffHz() const noexcept { return displayCutoffHz.load (std::memory_order_relaxed); }
    float getDisplayK()        const noexcept { return displayK.load (std::memory_order_relaxed); }

    //==========================================================================
    // Stage-2.3 visualization accessors. The Stage-3 editor reads these on its
    // message-thread Timer (30 Hz). The audio thread is copy-only / lock-free.
    //   getVizRing()         -> output scope/spectrum tap (the editor runs the FFT)
    //   getDisplayPlayhead() -> lead (loudest-active) voice playhead, normalized
    //                           [0,1] over the live [startSamp,endSamp) region.
    //                           0 when no voice is sounding (playhead at region start).
    //   getCurrentSampleRate()-> for the FFT frequency axis + ms<->samples.
    // Curve / scope DRAWING is Stage 3; Stage 2.3 only lands the ring + atomics.
    VizRing& getVizRing()        noexcept { return vizRing; }
    float    getDisplayPlayhead() const noexcept { return displayPlayhead.load (std::memory_order_relaxed); }
    double   getCurrentSampleRate() const noexcept { return currentSampleRate; }

    // Min/max envelope of the currently-published source (Stage-3 waveform editor
    // background). Message-thread read: snapshots the atomic shared_ptr (held for
    // the call), then reduces channel 0 to `numPairs` min/max pairs in [-1,1]. The
    // audio thread is untouched. Returns a flat [min,max,…] vector (empty if no
    // source). The Stage-3 editor fetches it on load + at boot (NOT per frame).
    std::vector<float> getSourceThumbnail (int numPairs) const;

    //==========================================================================
    // Concept-preset tour (Stage 3.3 / FUNC-07). Each named preset is one factory
    // snapshot that isolates a single sampler concept. Applied as a full APVTS write
    // via setValueNotifyingHost so (a) the host records automation + state-save, and
    // (b) the WebView relays/attachments resync every knob/combo/toggle back to the
    // page automatically — no DOM poking. The editor registers an "applyFactoryPreset"
    // native fn that forwards the button label here. Message thread (native-fn
    // callback); the audio thread sees the change via the param atomics like any host
    // automation. Mirrors O-simpleGrain::applyFactoryPreset.
    //   STAGE 4: all 7 branch bodies authored (param values that isolate each concept),
    //   with a central post-reset root re-seed so no preset plays octave-flat.
    void applyFactoryPreset (const juce::String& name);

    //==========================================================================
    // Engine constants (declared NOW; consumed by the sampler engine in Stage 2).
    static constexpr int kMaxVoices         = 16;   // polyphony
    static constexpr int kMaxGrainsPerVoice = 4;    // Stretch (synchronous-granular) grain pool
    static constexpr int kRootNote          = 60;   // C3 — key-track reference / default rootKey
    static constexpr int kMaxSourceSeconds  = 30;   // source-length cap
    static constexpr int kStretchGrainMs    = 60;   // Stretch fixed grain size (ms)
    static constexpr int kNumBuiltIns       = 1;    // embedded built-in source count

private:
    //==========================================================================
    // Source decode/resample/publish (Phase 2.1) — ALL off the audio thread.

    // Decode one embedded BinaryData .wav (by built-in index), resample to the
    // engine rate, cap at kMaxSourceSeconds, and atomic-publish. Returns true on
    // success. OFF the audio thread (prepareToPlay / setStateInformation).
    bool loadBuiltInSource (int builtInIndex, double engineRate);

    // Map an "embedded:<name>" identity to its built-in index (0..kNumBuiltIns-1).
    // Falls back to 0 (piano) if unknown.
    int builtInIndexForIdentity (const juce::String& identity) const;

    // Decode a raw byte block (a complete .wav/.aiff/.flac in memory) through the
    // format manager, resample, cap, atomic-publish. Invalid reader → keep the
    // previous source (returns false). OFF the audio thread.
    bool decodeAndPublish (const void* data, size_t numBytes, double engineRate,
                           const juce::String& identity);

    // Resample a decoded buffer (srcRate) to engineRate, capped at kMaxSourceSeconds.
    // Returns a fully-built shared_ptr ready to atomic-publish. Sets `truncated`
    // when the source exceeded the cap.
    std::shared_ptr<juce::AudioBuffer<float>>
        resampleToEngineRate (const juce::AudioBuffer<float>& src, double srcRate,
                              double engineRate, bool& truncated) const;

    // Seed the LIVE rootKey param to the per-source recorded-pitch root (e.g.
    // piano = 48). The APVTS rootKey DEFAULT stays 60 (frozen contract); this
    // overwrites the live value. OFF the audio thread.
    void seedRootForSource (int builtInIndex);

    // APVTS listener: fires on the message thread when a region/loop marker changes.
    // We do NOT scan here (the host may call it from any thread) — we
    // triggerAsyncUpdate() so the work always runs on the message thread.
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // AsyncUpdater: message-thread callback that performs the deferred fresh-instance
    // root seed and the deferred zero-cross marker snap (Phase 2.2a).
    void handleAsyncUpdate() override;

    // Clear the active drag-drop session state (message thread).
    void endDropSession() noexcept;

    // Zero-cross marker snap (Phase 2.2a, RESEARCH P3.4) — OFF the audio thread.
    // Scans ±W source samples around each nominal start/end/loopStart/loopEnd marker
    // for the nearest sign change (minimizing |src[i]|) and publishes the snapped ABS
    // source-frame indices to the snap* atomics the audio thread reads once per block.
    // Secondary seam defense (the equal-power crossfade is primary); keeps even a 0 ms
    // crossfade reasonable. Runs from prepareToPlay / handleAsyncUpdate / setState —
    // never processBlock.
    void computeZeroCrossSnaps();

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // Stretch SOLA Hann window LUT (Phase 2.2b). Built ONCE at construction (the
    // ctor allocates its tables off the audio thread) and shared read-only by every
    // SampleVoice via a const WindowLuts* wired in prepareToPlay. Declared BEFORE the
    // synth so it outlives the voices. RT-CRITICAL: a single processor-owned instance —
    // never per-voice, never rebuilt in the render path (WindowLuts::build allocates).
    WindowLuts windowLuts { 2048 };

    //==========================================================================
    // The sampler synth: kMaxVoices custom SampleVoice + one shared SampleSound
    // (built in the ctor). The resampled source buffer is published to the audio
    // thread via the atomic shared_ptr swap and snapshotted once per block.
    juce::Synthesiser synth;

    // RT-safe source-swap handoff (RESEARCH §3, mirrors O-TextureForge). The AUDIO
    // thread reads ONLY this raw atomic pointer once per block (acquire) — it never
    // touches a shared_ptr, so it can never become the last owner and free a buffer in
    // the render path (RT-safety item 1). The buffer it points to is kept alive by
    // currentSource; the previous generation is held one swap longer by retiredSource,
    // so any in-flight audio block finishes reading the old buffer before it is freed
    // (depth-1 is safe: swaps are user-paced, one per message-thread turn).
    std::atomic<juce::AudioBuffer<float>*> sourceForAudio { nullptr };

    // Owning shared_ptrs — touched ONLY off the audio thread, guarded by sourceMutex
    // (NOT the deprecated std::atomic_load/store(shared_ptr) — RT-safety item 2). The
    // writer is decodeAndPublish (prepareToPlay = a host/audio-setup thread, the
    // AsyncUpdater + setStateInformation = the message thread); the readers are
    // getSourceThumbnail + computeZeroCrossSnaps. The mutex makes the prepareToPlay-
    // writer vs editor-reader boundary race-free. The audio thread NEVER locks it.
    std::shared_ptr<juce::AudioBuffer<float>> currentSource;     // live owner
    std::shared_ptr<juce::AudioBuffer<float>> retiredSource;     // previous generation (depth 1)
    juce::CriticalSection sourceMutex;                           // guards currentSource/retiredSource — off-audio only

    // On-screen-keyboard MIDI (Stage 3.1): thread-safe queue drained into
    // processBlock so the WebView keyboard drives the synth identically to host
    // MIDI. reset() in prepareToPlay; removeNextBlockOfMessages() at block start.
    juce::MidiMessageCollector midiCollector;

    // Master output trim (dB->lin, 20 ms smoothing) — final stage in processBlock.
    juce::SmoothedValue<float> outputGain { 1.0f };

    // Filter cutoff/Q smoothing lives in the PROCESSOR (one SmoothedValue each — NOT
    // 16× per-voice; the filter is global, no per-note mod in v1). Per block the
    // current smoothed scalar is pushed to every voice (one tan recompute) and drives
    // the lead-voice display atomics. 20 ms ramp = zipper-free (RESEARCH P1.1 / P5).
    juce::SmoothedValue<float> filterCutoffSm { 20000.0f };
    juce::SmoothedValue<float> filterQSm      { 0.707f };

    // Phase 2.2a — deferred zero-cross snap request (set by parameterChanged on a
    // start/end/loopStart/loopEnd change; consumed in handleAsyncUpdate).
    std::atomic<bool> pendingSnap { false };

    // Phase 2.2a — snapped ABS source-frame markers published by computeZeroCrossSnaps
    // (message thread), read once per block by the audio thread. -1 = not yet snapped
    // (audio thread falls back to the live %-computed bounds).
    std::atomic<int> snapRegionStart { -1 };
    std::atomic<int> snapRegionEnd   { -1 };
    std::atomic<int> snapLoopStart   { -1 };
    std::atomic<int> snapLoopEnd     { -1 };

    // Phase 2.2a — lead-voice (loudest-active) display atomics, published once per
    // block (RESEARCH P5.3). The filter is global in v1 so these mirror the processor-
    // smoothed cutoff and k=1/Q (= JUCE's R2); Stage 3 draws SubFilterCurve from them
    // so the curve matches the running filter by construction (QUAL-02).
    std::atomic<float> displayCutoffHz { 20000.0f };
    std::atomic<float> displayK        { 1.0f / 0.707f };

    // Phase 2.3 — lead-voice playhead, published once per block from the loudest-
    // active voice's live read axis (readPos for Repitch / timePos for Stretch),
    // normalized to [0,1] over [startSamp,endSamp). 0 when nothing sounds. Read by
    // the Stage-3 editor Timer to overlay the playhead on the waveform (RESEARCH §12).
    std::atomic<float> displayPlayhead { 0.0f };

    // Phase 2.3 — output scope/spectrum tap. Fully allocated at construction (lock-
    // free overwrite ring); the audio thread COPY-ONLY writes the post-gain output
    // at the processBlock tail (no alloc / no lock / no FFT on the audio thread — the
    // FFT runs on the Stage-3 editor Timer). RESEARCH §12 / PERF-01.
    VizRing vizRing;

    // Root-seed guards (RESEARCH §6). A FRESH instance seeds the per-source root
    // once on the first prepareToPlay (rootSeeded); a RESTORED session keeps its
    // saved rootKey (stateWasRestored gates the prepare-time seed off).
    bool stateWasRestored = false;
    bool rootSeeded       = false;

    // RT-safety item 3 (RESEARCH §3): a fresh instance's one-time root seed is
    // DEFERRED off prepareToPlay (where setValueNotifyingHost is discouraged — prepare
    // can run off the message thread / during scans) onto the existing AsyncUpdater.
    // prepareToPlay sets this flag + triggerAsyncUpdate(); handleAsyncUpdate (message
    // thread) consumes it and calls seedRootForSource.
    std::atomic<bool> pendingRootSeed { false };

    //==========================================================================
    // Drag-drop streaming session state (Stage 3.1). For a single sampler source
    // we keep ONE active session's accumulated base64; commit decodes + publishes.
    // All on the message thread (NativeFunction callbacks run there).
    juce::String dropSessionId;       // empty = no active session
    juce::String dropAccumBase64;     // last Chunk payload (single source)
    juce::String dropAccumFilename;

    std::unique_ptr<juce::FileChooser> fileChooser;   // held alive across the async picker

    // Whether the most recent load was truncated to kMaxSourceSeconds.
    std::atomic<bool> lastLoadTruncated { false };

    //==========================================================================
    // Custom non-APVTS state: which source is loaded (built-in name or file path).
    juce::String currentSourceIdentity { "embedded:piano" };

    // Built-in names. v1.1.0 ships ONE embedded source: piano. The identity scheme is
    // "embedded:<name>", so this string is the contract-adjacent surface (it must match
    // the parameter-spec built-in table). Kept as an array rather than hard-coded so a
    // future cleared built-in is a one-line addition here + in kBuiltInRoot.
    static constexpr const char* kBuiltInNames[kNumBuiltIns] = { "piano" };

    // Per-source recorded-pitch root (engine metadata), probed via YIN f0 → nearest
    // MIDI. piano = 48 (f0 ≈ 131.6 Hz / C3). The APVTS rootKey DEFAULT stays 60
    // (frozen contract); seedRootForSource overwrites the live value per source.
    static constexpr int kBuiltInRoot[kNumBuiltIns] = { 48 };

    //==========================================================================
    // Cached raw-param atomic pointers (assigned in the ctor). Established now;
    // read once per block by the sampler engine in Stage 2. Unused while silent.
    std::atomic<float>* startParam           = nullptr;
    std::atomic<float>* endParam             = nullptr;
    std::atomic<float>* loopModeParam        = nullptr;
    std::atomic<float>* loopStartParam       = nullptr;
    std::atomic<float>* loopEndParam         = nullptr;
    std::atomic<float>* loopCrossfadeParam   = nullptr;
    std::atomic<float>* reverseParam         = nullptr;
    std::atomic<float>* rootKeyParam         = nullptr;
    std::atomic<float>* pitchModeParam       = nullptr;
    std::atomic<float>* tuneParam            = nullptr;
    std::atomic<float>* fineParam            = nullptr;
    std::atomic<float>* vintageParam         = nullptr;
    std::atomic<float>* filterCutoffParam    = nullptr;
    std::atomic<float>* filterResonanceParam = nullptr;
    std::atomic<float>* ampAttackParam       = nullptr;
    std::atomic<float>* ampDecayParam        = nullptr;
    std::atomic<float>* ampSustainParam      = nullptr;
    std::atomic<float>* ampReleaseParam      = nullptr;
    std::atomic<float>* velToAmpParam        = nullptr;
    std::atomic<float>* outputLevelParam     = nullptr;

    //==========================================================================
    double currentSampleRate = 44100.0;

    // Custom-state element names for get/setStateInformation.
    static constexpr const char* kSourceStateTag = "SOURCE";
    static constexpr const char* kSourceIdProp   = "identity";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleSamplerAudioProcessor)
};
