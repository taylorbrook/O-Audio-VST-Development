/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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

    O-simpleGrain - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical granular synthesizer.

    Stage 1 (Foundation): silent 8-voice synth shell. Full 19-parameter APVTS +
    state persistence (incl. a custom, non-APVTS loaded-source identity). No audio
    rendering yet (grain engine / voices / ADSR / window LUTs land Stage 2), no
    WebView UI yet (minimal placeholder editor for now — Stage 3 brings the UI).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>
#include "dsp/WindowLuts.h"
#include "dsp/TripleBuffer.h"
#include "dsp/GrainCloudFrame.h"
#include "VizAnalyzer.h"

// Forward declarations — the voice/sound are pulled in only by the .cpp
// (GrainVoice.h itself includes this header for the engine constants).
class GrainVoice;
class GrainSound;

//==============================================================================
// Parameter identifiers — single source of truth for APVTS IDs.
// Referenced by the processor now; by the grain engine / voice param-push
// (Stage 2) and WebView relays/attachments (Stage 3) later. IDs/ranges/defaults
// are authoritative per .planning/parameter-spec.md (19 params).
namespace OSimpleGrain::ParamIDs
{
    // Source
    inline constexpr auto sourceSample   = "sourceSample";   // choice: fire/voice/water/piano

    // Grain
    inline constexpr auto grainSize      = "grainSize";      // 2–500 ms (v1.4.0: was 2–200)
    inline constexpr auto density        = "density";        // 1–200 grains/s (log skew)
    inline constexpr auto position       = "position";       // 0–100 %
    inline constexpr auto scan           = "scan";           // −200–+200 % (bipolar)
    inline constexpr auto freeze         = "freeze";         // bool

    // Window
    inline constexpr auto windowShape    = "windowShape";    // choice: rect/tri/Welch/Gauss/Hann/Tukey
    inline constexpr auto windowTaper    = "windowTaper";    // 0–100 % — Tukey taper (α), v1.4.0

    // Spray & Scatter
    inline constexpr auto pitchSpray     = "pitchSpray";     // 0–12 st
    inline constexpr auto positionSpray  = "positionSpray";  // 0–100 %
    inline constexpr auto scatter        = "scatter";        // 0–100 %
    inline constexpr auto grainPitch     = "grainPitch";     // −24–+24 st
    inline constexpr auto panSpray       = "panSpray";       // 0–100 %
    inline constexpr auto velToDensity   = "velToDensity";   // 0–100 %

    // Amplitude envelope (per-voice ADSR)
    inline constexpr auto ampAttack      = "ampAttack";      // 0–5 s
    inline constexpr auto ampDecay       = "ampDecay";       // 0–5 s
    inline constexpr auto ampSustain     = "ampSustain";     // 0–1 (0–100 %)
    inline constexpr auto ampRelease     = "ampRelease";     // 0–5 s
    inline constexpr auto adsrEnabled    = "adsrEnabled";    // bool — envelope on/off (v1.1.0)

    // Output
    inline constexpr auto outputLevel    = "outputLevel";    // master trim (dB)
}

//==============================================================================
class OSimpleGrainAudioProcessor : public juce::AudioProcessor,
                                   private juce::AudioProcessorValueTreeState::Listener,
                                   private juce::AsyncUpdater
{
public:
    OSimpleGrainAudioProcessor();
    ~OSimpleGrainAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simpleGrain"; }
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

    // Loaded-source identity (custom, non-APVTS state). "embedded:fire" by
    // default or a user file path once Stage 2.3 wires real loading. Persisted
    // alongside the APVTS tree so a session restores the same source.
    // Thread-safe: the identity is a COW juce::String touched from both host-
    // controlled threads (prepareToPlay / get/setStateInformation) and the
    // message thread (drop/picker/preset callbacks) — every access goes through
    // sourceStateLock (returned BY VALUE, never by reference).
    juce::String getSourceIdentity() const
    {
        const juce::ScopedLock sl (sourceStateLock);
        return currentSourceIdentity;
    }
    void setSourceIdentity (const juce::String& id)
    {
        const juce::ScopedLock sl (sourceStateLock);
        currentSourceIdentity = id;
    }

    //==========================================================================
    // On-screen keyboard: the editor injects note on/off from the WebView (any
    // thread). Queued via a MidiMessageCollector and merged into processBlock's
    // MIDI stream so UI notes drive the synth identically to host MIDI.
    void handleUiMidi (int noteNumber, bool noteOn, float velocity);

    //==========================================================================
    // INTERFACE LANGUAGE (v1.3.0) — the WebView UI's own language preference.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a concept preset must not be able to change which
    // language somebody reads their interface in. It rides the APVTS tree as a
    // plain PROPERTY, which is the same shape this processor already uses for
    // the loaded-source identity it persists as custom ValueTree state.
    // applyFactoryPreset() sets parameters only, so no preset path can touch it
    // behind the page's back.
    //
    // The RUNTIME form is an index; the PERSISTED form is the language code.
    // The editor PULLS it once at page init; nothing pushes.
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

    //==========================================================================
    // Stage-3 visualization accessors. The editor reads these on its
    // message-thread Timer (30 Hz). Audio thread is copy-only / lock-free.
    //   getVizRing()         -> output scope/spectrum (UI-04; editor runs the FFT)
    //   getGrainCloudBuffer()-> grain events (cloud scatter UI-01 + playheads UI-02)
    //   getActiveGrainCount()-> grain-count / CPU readout (UI-05)
    //   getCurrentSampleRate()-> for the FFT frequency axis + ms<->samples
    VizRing&                       getVizRing()          noexcept { return vizRing; }
    TripleBuffer<GrainCloudFrame>& getGrainCloudBuffer() noexcept { return grainCloudBuffer; }
    int    getActiveGrainCount() const noexcept { return activeGrainCount.load (std::memory_order_relaxed); }
    double getCurrentSampleRate() const noexcept { return currentSampleRate.load (std::memory_order_relaxed); }

    //==========================================================================
    // Source loading (Stage 2.3). The drag-drop streaming handlers below are the
    // C++ side of the shared webview-drop-streaming.js bridge — Stage 3 wires the
    // JS that CALLS them (via WebBrowserComponent::Options::withNativeFunction).
    // The names are FIXED by the shared module — do NOT rename. For a single
    // source the single-file path suffices, but the full set is registered so the
    // shared JS module binds cleanly. All decode/resample/publish happens here on
    // the message thread; the audio thread only ever sees a fully-built buffer
    // published via the atomic shared_ptr swap.
    //
    // Return value semantics (mirrors O-MicrotonalSampler): bool "ok" — the JS
    // awaits it and toasts on false.
    bool dropSessionStart       (const juce::String& sessionId, const juce::String& folderName = {});
    bool dropSessionAddFile     (const juce::String& sessionId, const juce::String& filename,
                                 const juce::String& base64);
    bool dropSessionCommitFile  (const juce::String& sessionId, const juce::String& filename,
                                 const juce::String& base64);
    bool dropSessionCommitFolder (const juce::String& sessionId);

    // File-picker fallback (always works where drag-drop doesn't). Async, message
    // thread. Stage 3 calls this from a "Load…" button. Same decode path.
    void loadSourceFromFileChooser();

    // Whether the last loaded source was truncated to the 10 s cap (UI surfaces a
    // notice in Stage 3). Cleared on each successful load that did not truncate.
    bool wasLastLoadTruncated() const noexcept { return lastLoadTruncated.load (std::memory_order_relaxed); }

    // Bumped on every successful decodeAndPublish (IN-08, v1.1.2). The editor's
    // 30 Hz timer polls it and emits a "sourceChanged" WebView event on change —
    // the JS drives its thumbnail/status refresh from that instead of fixed
    // timers racing the decode. (Polling because decodeAndPublish can run on
    // host-controlled threads where emitting WebView events is not allowed.)
    juce::uint32 getSourceVersion() const noexcept { return sourceVersion.load (std::memory_order_relaxed); }

    //==========================================================================
    // Concept-preset tour (Stage 3.3 / FUNC-06). Eight factory snapshots, each
    // isolating ONE granular concept. Applied as a full APVTS snapshot via
    // setValueNotifyingHost so (a) the host records automation + state-save, and
    // (b) the WebView relays/attachments sync every knob/combo/toggle back to the
    // page automatically — no DOM poking. The editor registers an
    // "applyFactoryPreset" native fn that forwards the button label here. Message
    // thread (native-fn callback); the audio thread sees the change via the
    // parameter atomics like any host automation. Mirrors O-simpleAdditive.
    void applyFactoryPreset (const juce::String& name);

    // A min/max envelope (~`numPairs` pairs) of the currently-published source,
    // for the UI-02 source-waveform background. Returns a flat vector
    // [min,max,min,max,…] in [-1,1]; empty if no source is loaded. Read-only
    // snapshot OFF the audio thread (message thread; the editor calls it on load
    // + at boot). The audio thread is untouched (PERF-01).
    std::vector<float> getSourceThumbnail (int numPairs = 512) const;

    //==========================================================================
    // Engine constants (declared NOW; consumed by the grain engine in Stage 2).
    static constexpr int kMaxVoices        = 8;     // polyphony
    static constexpr int kMaxGrainsPerVoice = 24;   // per-voice active grain cap
    static constexpr int kGlobalGrainCap   = 192;   // global cap (steal-oldest)
    static constexpr int kRootNote         = 60;    // C3 — key-track reference
    static constexpr int kMaxSourceSeconds = 10;    // source-length cap
    static constexpr int kWindowLutSize    = 2048;  // precomputed window LUT length

private:
    //==========================================================================
    // Source publish/read — genuinely lock-free on the audio thread (v1.1.1,
    // CODE_REVIEW WR-05). The previous std::atomic_load/store(shared_ptr&) free
    // functions are NOT lock-free (libc++ backs them with a global mutex pool),
    // which put a real lock on the RT path. Now:
    //
    //   - shared_ptr OWNERSHIP lives on the message/host side only, mutated
    //     exclusively under sourceStateLock (never on the audio thread).
    //   - the audio thread reads ONE std::atomic<AudioBuffer*> raw-pointer view
    //     per block (wait-free; buffers are immutable once published).
    //   - a publish parks the outgoing shared_ptr in `retiredSources` instead of
    //     freeing it; an in-flight audio block may still be reading the old raw
    //     pointer. Entries are reaped (on later publishes / prepareToPlay /
    //     destruction) only once `blocksRendered` has advanced ≥2 past the value
    //     recorded at parking — by then any block that could have observed the
    //     old pointer has completed. Pattern per O-MicrotonalSampler v1.24.0.
    //
    // Memory bound: retired entries only exist between a publish and the second
    // audio block after it (≤ one 10 s buffer, ~7 MB @96k, if audio is stopped).
    struct RetiredSource
    {
        std::shared_ptr<juce::AudioBuffer<float>> buffer;
        juce::uint64 parkedAtBlock = 0;
    };

    // Publish a fully-built buffer to the audio thread and park the old one.
    // Caller MUST hold sourceStateLock. Message/host threads only.
    void publishSource (std::shared_ptr<juce::AudioBuffer<float>> newBuf);

    // Free retired buffers that no in-flight audio block can still reference.
    // Caller MUST hold sourceStateLock.
    void reapRetiredSources();

    //==========================================================================
    // Source decode/resample/publish (Stage 2.3) — ALL off the audio thread.
    //
    // Built-in index order MUST match the sourceSample AudioParameterChoice:
    // 0=fire, 1=voice, 2=water, 3=piano (PluginProcessor.cpp createParameterLayout).
    static constexpr int kNumBuiltIns = 4;

    // Decode one embedded BinaryData .wav (by built-in index), resample to the
    // engine rate, cap at kMaxSourceSeconds, and atomic-publish. Returns true on
    // success. OFF the audio thread (prepareToPlay / sourceSample change).
    bool loadBuiltInSource (int builtInIndex, double engineRate);

    // Map an "embedded:<name>" identity to its built-in index (0..kNumBuiltIns-1).
    // Falls back to the live sourceSample choice, then 0 (fire), if unknown.
    int builtInIndexForIdentity (const juce::String& identity) const;

    // Decode a raw byte block (already in a memory buffer) through the format
    // manager, resample, cap, atomic-publish. Shared by the built-in path, the
    // drag-drop base64 path, and the file-picker path. OFF the audio thread.
    bool decodeAndPublish (const void* data, size_t numBytes, double engineRate,
                           const juce::String& identity);

    // Resample a decoded buffer (at srcRate) to engineRate, capped at the source
    // length cap. Returns a fully-built shared_ptr ready to atomic-publish. Sets
    // `truncated` when the source exceeded the cap.
    std::shared_ptr<juce::AudioBuffer<float>>
        resampleToEngineRate (const juce::AudioBuffer<float>& src, double srcRate,
                              double engineRate, bool& truncated) const;

    // sourceSample-change detection (off the audio thread). processBlock only
    // reads the choice atomic and stores it; the actual decode happens on an
    // AsyncUpdater the parameter listener triggers (never on the audio thread).
    void rebuildSourceFromChoice();

    // Clear the active drag-drop session state (message thread).
    void endDropSession() noexcept;

    // APVTS listener: fires on the message thread when `sourceSample` changes.
    // We do NOT decode here directly (the host may call it from any thread) —
    // we triggerAsyncUpdate() so the decode always runs on the message thread.
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // AsyncUpdater: message-thread callback that performs the actual built-in
    // source decode/resample/publish for the pending sourceSample selection.
    void handleAsyncUpdate() override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // Grain engine. Window LUTs built once at construction (declared BEFORE the
    // synth so they exist when the voices receive their LUT pointer in the ctor).
    WindowLuts        windowLuts { kWindowLutSize };
    juce::Synthesiser synth;

    // Typed view of the synth's voices, cached at construction (the voices are
    // owned by `synth` for the processor's lifetime). Avoids 2×kMaxVoices RTTI
    // dynamic_casts per block on the audio thread (IN-02, v1.1.2).
    std::array<GrainVoice*, kMaxVoices> grainVoices {};

    // On-screen-keyboard MIDI: thread-safe queue drained into processBlock so the
    // WebView keyboard drives the synth identically to host MIDI.
    juce::MidiMessageCollector midiCollector;

    //==========================================================================
    // Source ownership + audio-thread view (see the publish/reap note above).
    //
    // sourceStateLock guards ALL of: currentSource (the shared_ptr itself),
    // currentSourceIdentity, retiredSources, userSourceBytes(+Identity), and
    // publishedSourceRate. Taken ONLY off the audio thread (message + host-
    // controlled threads: prepareToPlay, get/setStateInformation, native-fn
    // callbacks). juce::CriticalSection is re-entrant, so decodeAndPublish may
    // be called with the lock already held.
    juce::CriticalSection sourceStateLock;

    // Owning ref of the published source (message/host side, under the lock).
    std::shared_ptr<juce::AudioBuffer<float>> currentSource;

    // Wait-free audio-thread view of currentSource (raw pointer; the pointee is
    // kept alive by currentSource / retiredSources until provably unreferenced).
    std::atomic<juce::AudioBuffer<float>*> audioSourceView { nullptr };

    // Outgoing sources parked until no in-flight audio block can reference them.
    std::vector<RetiredSource> retiredSources;

    // Monotonic count of COMPLETED audio blocks (incremented at the end of
    // processBlock, release order) — the reap fence for retiredSources.
    std::atomic<juce::uint64> blocksRendered { 0 };

    // Engine rate the published source was resampled to (under the lock) — lets
    // prepareToPlay skip a pointless re-decode when the rate did not change.
    double publishedSourceRate = 0.0;

    // Raw file bytes of the last "dropped:" source (CR-01). A dropped file has
    // no re-readable disk path (WKWebView strips it), so prepareToPlay retains
    // the bytes to re-decode at a NEW engine rate instead of clobbering the
    // user's live sound with a built-in. Capped so a huge drop can't pin RAM;
    // over the cap the live buffer is kept as-is on a rate change (transposed).
    static constexpr size_t kMaxRetainedSourceBytes = 32 * 1024 * 1024;
    juce::MemoryBlock userSourceBytes;
    juce::String      userSourceBytesIdentity;   // which "dropped:" identity the bytes belong to

    // Master output trim (dB->lin, smoothed) folded together with an overlap-aware
    // normalization factor so sparse/single grains play at full level while dense
    // overlapping clouds stay tamed below clip (see processBlock).
    juce::SmoothedValue<float> outputGain { 1.0f };

    //==========================================================================
    // Global read head (processor-owned, Phase 2.2). One playhead shared by all
    // voices/grains: `position` sets its resting point, `scan` its velocity,
    // `freeze` pins it. Advanced PER SAMPLE in processBlock (wrapped to
    // [0, srcLen) for both directions — negative scan = reverse). Voices read the
    // block-start playhead at spawn (setPlayhead, once per block) so the 2.1 voice
    // spawn signature is untouched (Sequencing Note 3).
    //
    // Click-free (QUAL-01): SmoothedValue ramps on scan / position / playhead
    // velocity. Freeze targets velocity -> 0 (and disengage ramps back to the
    // scan-derived velocity) via the velocity SmoothedValue — the playhead is
    // NEVER hard-jumped (RESEARCH §4.2 "simplest robust approach").
    double playheadPos = 0.0;                        // current global read point (source samples)

    juce::SmoothedValue<float> scanSmoothed     { 0.0f };   // scan % -> velocity ramp
    juce::SmoothedValue<float> positionSmoothed { 0.0f };   // resting point % -> samples ramp
    juce::SmoothedValue<float> playheadVelocity { 0.0f };   // samples/sample, freeze-pinnable

    // Per-sample rest-ease coefficient, derived from a time constant in
    // prepareToPlay (IN-03, v1.1.2): the old fixed 0.0008/sample glided ~2×
    // faster at 96 kHz than at 44.1 kHz. τ ≈ 28.3 ms reproduces the shipped
    // 44.1 kHz feel at every rate. Audio thread reads it; written only in
    // prepareToPlay (never concurrent with processBlock).
    static constexpr double kRestEaseTauSeconds = 0.0283;
    double restEaseCoeff = 0.0008;

    //==========================================================================
    // Custom non-APVTS state: which source is loaded (built-in name, file path,
    // or "dropped:<name>"). GUARDED by sourceStateLock (CR-02): a COW
    // juce::String written from host-controlled threads (prepareToPlay,
    // set/getStateInformation) AND the message thread (drop/picker/preset) —
    // unsynchronized ref-count ops on the same String can double-release.
    juce::String currentSourceIdentity { "embedded:fire" };

    // Built-in names, indexed to match the sourceSample choice order.
    static constexpr const char* kBuiltInNames[kNumBuiltIns] = { "fire", "voice", "water", "piano" };

    //==========================================================================
    // Visualization taps (Stage 2.3) — lock-free, audio-thread copy-only.
    //   vizRing          : output samples -> scope/spectrum (UI-04). The audio
    //                      thread WRITES the post-gain mono sum; the editor Timer
    //                      runs the FFT (never the audio thread).
    //   grainCloudBuffer : grain events -> cloud scatter (UI-01) + playheads
    //                      (UI-02). Filled per block (each voice appends a
    //                      GrainEvent at spawn), published once per block.
    //   activeGrainCount : live grain count -> grain-count / CPU readout (UI-05).
    VizRing                       vizRing;
    TripleBuffer<GrainCloudFrame> grainCloudBuffer;
    std::atomic<int>              activeGrainCount { 0 };

    //==========================================================================
    // Drag-drop streaming session state (Stage 2.3). The shared
    // webview-drop-streaming.js module streams base64 chunks through
    // dropSessionStart/AddFile/CommitFile. For a single granular source we keep
    // ONE active session's accumulated base64; commit decodes + publishes. All on
    // the message thread (NativeFunction callbacks run there).
    juce::String dropSessionId;                 // empty = no active session
    juce::String dropSessionFolderName;         // best-effort, for the restore notice
    juce::String dropAccumBase64;               // last AddFile payload (single source)
    juce::String dropAccumFilename;

    std::unique_ptr<juce::FileChooser> fileChooser;   // held alive across the async picker

    // Whether the most recent load was truncated to kMaxSourceSeconds.
    std::atomic<bool> lastLoadTruncated { false };

    // Bumped by decodeAndPublish; polled by the editor timer (IN-08).
    std::atomic<juce::uint32> sourceVersion { 0 };

    // Pending built-in index for the AsyncUpdater (set by the parameter listener,
    // consumed on the message thread). -1 = nothing pending.
    std::atomic<int> pendingBuiltInIndex { -1 };

    //==========================================================================
    // Cached raw-param atomic pointers (assigned in the ctor). Established now;
    // read once per block by the grain engine in Stage 2. Unused while silent.
    std::atomic<float>* sourceSampleParam  = nullptr;
    std::atomic<float>* grainSizeParam     = nullptr;
    std::atomic<float>* densityParam       = nullptr;
    std::atomic<float>* positionParam      = nullptr;
    std::atomic<float>* scanParam          = nullptr;
    std::atomic<float>* freezeParam        = nullptr;
    std::atomic<float>* windowShapeParam   = nullptr;
    std::atomic<float>* windowTaperParam   = nullptr;
    std::atomic<float>* pitchSprayParam    = nullptr;
    std::atomic<float>* positionSprayParam = nullptr;
    std::atomic<float>* scatterParam       = nullptr;
    std::atomic<float>* grainPitchParam    = nullptr;
    std::atomic<float>* panSprayParam      = nullptr;
    std::atomic<float>* velToDensityParam  = nullptr;
    std::atomic<float>* ampAttackParam     = nullptr;
    std::atomic<float>* ampDecayParam      = nullptr;
    std::atomic<float>* ampSustainParam    = nullptr;
    std::atomic<float>* ampReleaseParam    = nullptr;
    std::atomic<float>* adsrEnabledParam   = nullptr;
    std::atomic<float>* outputLevelParam   = nullptr;

    //==========================================================================
    // Atomic (CR-02): written by prepareToPlay (host-controlled thread), read
    // lock-free by message-thread native fns and the editor timer.
    std::atomic<double> currentSampleRate { 44100.0 };

    // Custom-state element names for get/setStateInformation.
    static constexpr const char* kSourceStateTag = "SOURCE";
    static constexpr const char* kSourceIdProp   = "identity";
    // v1.3.0 — a ROOT property, not a child of the SOURCE node: the interface
    // language has nothing to do with which sound is loaded.
    static constexpr const char* kUiLanguageProp = "uiLanguage";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleGrainAudioProcessor)
};
