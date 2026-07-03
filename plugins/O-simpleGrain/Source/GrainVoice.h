/*
  ==============================================================================

    O-simpleGrain - Grain Voice (one polyphonic granular voice)

    A juce::SynthesiserVoice that owns a preallocated bounded grain pool
    (std::array<Grain, kMaxGrainsPerVoice>), a per-sample scheduler, a per-voice
    juce::Random, and a juce::ADSR amp envelope. Each held MIDI note transposes
    the grain cloud (key-tracked resample); grains read windowed fragments from
    the shared static source buffer and are overlap-added into the output.

    Phase 2.1: core grain engine + overlap-add + amp ADSR + key resample.
    Phase 2.2 (this file): per-voice spray & scatter via the per-voice juce::Random
    (position/pitch/period/pan), velToDensity into the effective density, and the
    anti-aliasing one-pole filled in the grain read loop. The processor advances a
    moving/freezable global playhead and pushes its block-start value via
    setPlayhead — the voice spawn signature is unchanged from 2.1.

    Real-time safety (every line of renderNextBlock): NO allocation / lock / I/O.
    Preallocated pool; steal-oldest when full (never resize). Per-voice Random.
    ADSR setSampleRate BEFORE setParameters in the (non-virtual) prepareToPlay.

    JUCE 8: SynthesiserVoice has NO virtual prepareToPlay — this declares a
    NON-VIRTUAL prepareToPlay(double,int) dispatched by the processor via
    dynamic_cast (O-simpleFM / O-Bassoon pattern).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include "PluginProcessor.h"            // engine constants (kMaxGrainsPerVoice, kRootNote)
#include "GrainSound.h"
#include "dsp/Grain.h"
#include "dsp/WindowLuts.h"
#include "dsp/LagrangeInterpolation.h"
#include "dsp/GrainCloudFrame.h"        // Phase 2.3 — grain-event viz tap

//==============================================================================
// Block-pushed parameter bundle. Defined generously so Phase 2.2 can add
// spray/scatter/velToDensity fields with low churn (the voice ignores any field
// it does not yet use). Voices never touch the APVTS — the processor reads it
// once per block and calls setParams(...).
struct GrainVoiceParams
{
    float grainSizeMs = 30.0f;   // 2..200 ms
    float density     = 40.0f;   // grains/sec (clamped >= 1 by the scheduler)
    int   windowShape = 4;       // 0=rect .. 4=Hann
    float grainPitch  = 0.0f;    // global transposition in semitones

    // --- Phase 2.2 spray & scatter (per-grain RNG; processor pushes these) ---
    float positionSpray = 0.0f;  // 0..100 % of source length — read-start jitter
    float pitchSpray    = 0.0f;  // 0..12 st — ± per-grain rate jitter
    float scatter       = 0.0f;  // 0..100 % — scheduler period jitter (sync↔async)
    float panSpray      = 0.0f;  // 0..100 % — per-grain stereo spread
    float velToDensity  = 0.0f;  // 0..100 % (stored 0..1) — velocity -> density depth

    // false = bypass the amp ADSR: amplitude is a flat velocity-scaled gate and, on
    // note-off, the voice stops spawning grains and lets the active grains drain
    // through their own Window envelopes (no declick envelope needed).
    bool adsrEnabled = true;
    juce::ADSR::Parameters amp { 0.01f, 0.3f, 0.8f, 0.4f };
};

//==============================================================================
class GrainVoice : public juce::SynthesiserVoice
{
public:
    GrainVoice() = default;
    ~GrainVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<GrainSound*> (sound) != nullptr;
    }

    //==========================================================================
    // Non-virtual JUCE-8 prepare. setSampleRate MUST precede setParameters.
    void prepareToPlay (double sr, int /*maxBlockSize*/)
    {
        setCurrentPlaybackSampleRate (sr);
        sampleRate = sr;

        ampEnv.setSampleRate (sr);              // BEFORE setParameters (RT-safety gate 8)
        ampEnv.setParameters (ampParams);

        // Clear the pool so a sample-rate change never leaves stale grains running.
        for (auto& g : grains) g.active = false;
        nextGrain = 0;
        samplesUntilNextGrain = 0;

        // Drop our viz-count contribution (the processor zeroes the shared atomic
        // in its prepareToPlay; we forget our last contribution so the next
        // publishActiveCount() recomputes a clean delta from 0).
        myActiveCount = 0;
    }

    // Shared window-LUT table set (owned by the processor). Set once after
    // construction (a const pointer the read loop dereferences — no copy).
    void setWindowLuts (const WindowLuts* luts) noexcept { windowLuts = luts; }

    //==========================================================================
    // Block param-push from the processor (once per block). Carries the 2.1 core
    // params + the 2.2 spray/scatter/velToDensity fields. Voices never touch APVTS.
    void setParams (const GrainVoiceParams& p) noexcept
    {
        params = p;
        ampParams = p.amp;
        ampEnv.setParameters (ampParams);
    }

    // Source snapshot the processor sets once per block (it holds the shared_ptr
    // alive for the whole block; the voice only reads the raw pointer/len). A
    // null source -> the voice renders silence (no grains read).
    void setSource (const float* src, int len) noexcept
    {
        sourcePtr = src;
        sourceLen = len;
    }

    // Abstract "current playhead value" (in source samples) the processor sets
    // per block — the block-start position of the moving/freezable global read
    // head. Grains spawned this block read from here (± position spray). The
    // processor advances/freezes the playhead per sample; the voice only reads
    // this per-block snapshot, so the spawn signature is unchanged from 2.1.
    void setPlayhead (float posSamples) noexcept { playheadPos = posSamples; }

    //==========================================================================
    // Phase 2.3 viz taps. The processor sets these once per block (like the
    // playhead seam) so the voice spawn signature stays untouched:
    //   setGrainCloudFrame : the block's GrainCloudFrame the voice appends a
    //                        GrainEvent to at each spawn (bounded by kMaxEvents —
    //                        extras dropped, never grown). Null between blocks.
    //   setActiveGrainCount: the processor-owned atomic the voice increments on
    //                        spawn and decrements when a grain finishes (UI-05).
    //   setBlockStartSample: the global sample offset of this block's render start
    //                        (for GrainEvent::spawnSample). 0 here since the synth
    //                        renders the whole block at once (startSample passed to
    //                        renderNextBlock is the buffer offset).
    void setGrainCloudFrame  (GrainCloudFrame* frame) noexcept { cloudFrame = frame; }
    void setActiveGrainCount (std::atomic<int>* counter) noexcept { activeGrainCount = counter; }

    //==========================================================================
    void startNote (int midiNote, float velocity,
                    juce::SynthesiserSound*, int /*pitchWheel*/) override
    {
        // Key-tracked resample: held note transposes the cloud (C3 = recorded pitch).
        voiceRate = std::pow (2.0f, (float) (midiNote - OSimpleGrainAudioProcessor::kRootNote) / 12.0f);

        // Clear the pool — a new note starts a fresh cloud.
        for (auto& g : grains) g.active = false;
        nextGrain = 0;
        samplesUntilNextGrain = 0;               // fire a grain on the first sample

        velLevel = juce::jlimit (0.0f, 1.0f, velocity);
        gateHeld = true;
        ampEnv.noteOn();                         // always driven — keeps ampEnv coherent for click-free ADSR live-toggle
    }

    void stopNote (float, bool allowTailOff) override
    {
        gateHeld = false;                   // note released (both paths) — stops grain spawning in ADSR-off mode
        if (allowTailOff)
        {
            ampEnv.noteOff();
            // ADSR off: do NOT clear grains — let the active grains finish their
            // Window envelopes (natural, click-free drain). Voice frees in
            // renderNextBlock once gateHeld is false and the pool empties.
        }
        else
        {
            clearCurrentNote();
            ampEnv.reset();
            for (auto& g : grains) g.active = false;
            publishActiveCount();           // hard stop -> our grains drop to 0 (UI-05)
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        // Voice is live while: (ADSR on) the envelope is active; (ADSR off) the note
        // is held OR grains from a released note are still draining through their
        // Window envelopes. That drain is what lets the bypass be click-free without a
        // declick envelope — each tail grain tapers to zero via its own window.
        const bool voiceLive = params.adsrEnabled ? ampEnv.isActive()
                                                  : (gateHeld || myActiveCount > 0);
        if (! voiceLive)
        {
            // If we were contributing grains last block, drop our contribution to
            // the shared count so a silent voice doesn't pin the readout (UI-05).
            if (myActiveCount != 0)
            {
                for (auto& g : grains) g.active = false;
                publishActiveCount();
            }
            return;
        }

        const int numCh = out.getNumChannels();

        // Effective density -> base inter-grain interval (samples). velToDensity
        // (0..1 depth) makes harder notes spawn a denser cloud: a note above the
        // mid-velocity (0.5) thickens, below thins. velLevel is fixed at note-on,
        // so the base interval is constant across the block; scatter jitters the
        // ACTUAL interval per spawn (below) — the sync↔async axis (DSP-05).
        const float effectiveDensity = juce::jlimit (1.0f, 200.0f,
            params.density * (1.0f + params.velToDensity * (velLevel - 0.5f) * 2.0f));
        const float baseInterval = (float) sampleRate / effectiveDensity;

        // Grain length in samples (clamp >= 2 so phaseInc is finite).
        const float lenSamp = juce::jmax (2.0f, (params.grainSizeMs * 0.001f) * (float) sampleRate);
        const float phaseInc = 1.0f / lenSamp;

        // ADSR off + note released = draining: keep rendering the active grains but
        // spawn no new ones, so the cloud fades out over one grain length via the
        // windows. ADSR on always spawns (the release tail stays audible).
        const bool allowSpawn = params.adsrEnabled || gateHeld;

        for (int i = 0; i < numSamples; ++i)
        {
            // --- Scheduler: fire a grain when the countdown elapses -----------
            if (--samplesUntilNextGrain <= 0)
            {
                if (allowSpawn)
                    spawnGrain (lenSamp, phaseInc, startSample + i);
                // Scatter (RESEARCH §2.6): jitter the period ± scatter% of the
                // base interval. 0% = constant period (synchronous → discrete
                // sidebands); 100% = ±full random (asynchronous → noise).
                const float jitter = (params.scatter * 0.01f) * baseInterval
                                   * (rng.nextFloat() * 2.0f - 1.0f);
                samplesUntilNextGrain = juce::jmax (1, (int) (baseInterval + jitter));
            }

            // --- Overlap-add the active grains -------------------------------
            float outL = 0.0f, outR = 0.0f;

            for (auto& g : grains)
            {
                if (! g.active)
                    continue;

                const float env = (windowLuts != nullptr) ? windowLuts->read (g.shape, g.phase) : 0.0f;
                float src = readSourceLagrange (sourcePtr, sourceLen, g.readPos);
                src = aaOnePole (src, g.aaCoeff, g.aaEngaged, g.aaState);   // band-limit if rate > 1 (DSP-08)

                const float s = src * env;
                // Equal-power pan gains were precomputed on spawn (constant per grain).
                outL += s * g.panL;
                outR += s * g.panR;

                g.readPos += g.rate;
                g.phase   += g.phaseInc;
                ++g.age;
                if (g.phase >= 1.0f)
                    g.active = false;             // grain done
            }

            // --- Voice amplitude (amp ADSR * velocity) -----------------------
            // Always advance the envelope so ampEnv stays coherent for click-free
            // live toggling of the ADSR. When bypassed the amp is a flat velocity-
            // scaled gate — the per-grain Window envelope does all the shaping.
            const float envSample = ampEnv.getNextSample();
            const float ampVal = (params.adsrEnabled ? envSample : 1.0f) * velLevel;
            outL *= ampVal;
            outR *= ampVal;

            // Add into the output buffer (mono: sum L+R for a centred image).
            if (numCh >= 2)
            {
                out.addSample (0, startSample + i, outL);
                out.addSample (1, startSample + i, outR);
            }
            else if (numCh == 1)
            {
                out.addSample (0, startSample + i, 0.5f * (outL + outR));
            }
        }

        // Publish this voice's live grain count to the shared atomic (UI-05).
        // Recomputed by scanning the pool (O(24)) so steal-oldest / grain-done /
        // note-clear all stay consistent — robust vs. fragile inc/dec.
        publishActiveCount();

        // Voice lifetime: (ADSR on) the envelope finished its release; (ADSR off) the
        // note was released AND every drained grain has finished its window. Either
        // way the voice frees so the synth can reuse it (no stuck voices).
        const bool ended = params.adsrEnabled ? (! ampEnv.isActive())
                                              : (! gateHeld && myActiveCount == 0);
        if (ended)
        {
            for (auto& g : grains) g.active = false;
            publishActiveCount();           // release tail / drain finished -> drop to 0
            clearCurrentNote();
        }
    }

private:
    //==========================================================================
    // Spawn a grain: find an inactive slot, else steal the oldest (max age).
    // Bounded by kMaxGrainsPerVoice -> never allocates, never xruns (PERF-02).
    void spawnGrain (float lenSamp, float phaseInc, int spawnSample) noexcept
    {
        constexpr int N = OSimpleGrainAudioProcessor::kMaxGrainsPerVoice;

        int target = -1, oldest = 0, maxAge = -1;
        for (int k = 0; k < N; ++k)
        {
            const int idx = (nextGrain + k) % N;
            if (! grains[(size_t) idx].active) { target = idx; break; }
            if (grains[(size_t) idx].age > maxAge) { maxAge = grains[(size_t) idx].age; oldest = idx; }
        }
        if (target < 0)
            target = oldest;                      // steal oldest

        auto& g = grains[(size_t) target];
        g.active        = true;
        g.age           = 0;
        g.phase         = 0.0f;
        g.phaseInc      = phaseInc;
        g.lengthSamples = lenSamp;
        g.shape         = params.windowShape;

        // --- Position spray: scatter the read start ± positionSpray% of the
        // source length around the current playhead. Under freeze the playhead is
        // pinned, so spray still scatters reads WITHIN the frozen region → a
        // frozen pad shimmers rather than buzzing on one grain (FUNC-03).
        const float posSprayRand = (rng.nextFloat() * 2.0f - 1.0f)
                                 * (params.positionSpray * 0.01f) * (float) sourceLen;
        g.readPos = playheadPos + posSprayRand;

        // --- Pitch spray: fresh ± pitchSpray semitones per grain, combined with
        // the key rate and the global grainPitch offset (decision #2 — all three
        // multiply into the read increment). rate > 1 = up-transposed (the AA
        // one-pole engages on the read; see aaOnePole).
        const float pitchSprayRand = (rng.nextFloat() * 2.0f - 1.0f) * params.pitchSpray;
        g.rate = voiceRate * std::pow (2.0f, (params.grainPitch + pitchSprayRand) / 12.0f);

        // --- Pan spray: equal-power pan scattered ± panSpray% around centre.
        g.pan = juce::jlimit (0.0f, 1.0f,
            0.5f + (rng.nextFloat() * 2.0f - 1.0f) * (params.panSpray * 0.01f) * 0.5f);
        // Equal-power gains evaluated ONCE here (constant for the grain's life) —
        // the render loop just multiplies, no per-sample cos/sin.
        g.panL = std::cos (g.pan * juce::MathConstants<float>::halfPi);
        g.panR = std::sin (g.pan * juce::MathConstants<float>::halfPi);

        // --- AA one-pole: engage only for up-transposed grains (rate > 1). The
        // cutoff fc = 0.5*fs/rate and its smoothing coefficient depend only on rate
        // (constant per grain), so both are computed ONCE here — no per-sample
        // std::exp in the render loop (RESEARCH §5 / DSP-08).
        g.aaEngaged = (g.rate > 1.0f);
        g.aaCoeff   = g.aaEngaged
                    ? 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                             * (0.5f * (float) sampleRate / g.rate) / (float) sampleRate)
                    : 0.0f;

        // AA one-pole state primed to the first read sample so the filter starts
        // settled (no spurious attack transient on up-transposed grains).
        g.aaState = readSourceLagrange (sourcePtr, sourceLen, g.readPos);

        nextGrain = (target + 1) % N;

        // --- Viz tap (Phase 2.3): append a GrainEvent to the block's cloud frame
        // (UI-01 scatter + UI-02 playheads). Bounded by kMaxEvents — extras are
        // DROPPED, never grown (RT-safe, copy-only). The processor reset count=0
        // at block start and publish()es once per block.
        if (cloudFrame != nullptr && cloudFrame->count < GrainCloudFrame::kMaxEvents)
        {
            auto& ev = cloudFrame->events[(size_t) cloudFrame->count++];
            ev.readPosNorm = (sourceLen > 0)
                           ? juce::jlimit (0.0f, 1.0f, g.readPos / (float) sourceLen) : 0.0f;
            ev.sizeMs      = (float) (lenSamp / juce::jmax (1.0, sampleRate) * 1000.0);
            // Pitch shown relative to the source's recorded pitch: voice key +
            // global offset + this grain's spray (semitones).
            const float keySemis = 12.0f * std::log2 (juce::jmax (1.0e-6f, voiceRate));
            ev.pitchSemis  = keySemis + params.grainPitch + pitchSprayRand;
            ev.pan         = g.pan;
            ev.spawnSample = spawnSample;
        }
    }

    // Random-access fractional read into the static source (clamp at bounds —
    // a grain that runs off the end tapers out via its window, RESEARCH §2.5).
    static float readSourceLagrange (const float* src, int len, float pos) noexcept
    {
        if (src == nullptr || len <= 0)
            return 0.0f;

        const int   i0   = (int) pos;
        const float frac = pos - (float) i0;
        const int   im1  = juce::jlimit (0, len - 1, i0 - 1);
        const int   ip0  = juce::jlimit (0, len - 1, i0);
        const int   ip1  = juce::jlimit (0, len - 1, i0 + 1);
        const int   ip2  = juce::jlimit (0, len - 1, i0 + 2);
        return lagrangeInterpolate (src[im1], src[ip0], src[ip1], src[ip2], frac);
    }

    // Anti-aliasing one-pole (RESEARCH §5 / DSP-08). Band-limits up-transposed
    // grains before the fractional read crosses Nyquist. The cutoff (fc =
    // 0.5*fs/rate), its smoothing coefficient, and the engage decision (rate > 1)
    // are all precomputed ON SPAWN (constant per grain — see spawnGrain), so this
    // hot-path helper is one branch + one multiply-add, NO transcendental. When not
    // engaged the read passes through and `state` is kept coherent for a later
    // engage edge. `state` is the per-grain aaState (primed on spawn). Keeps high
    // pitch-spray grains clean without global oversampling — zero added latency.
    static float aaOnePole (float x, float coeff, bool engaged, float& state) noexcept
    {
        if (! engaged)
        {
            state = x;                          // keep state coherent for the bypass→engage edge
            return x;
        }

        state += coeff * (x - state);           // one-pole y += g*(x - y), g precomputed
        return state;
    }

    // Recompute this voice's active grain count (scan the bounded pool, O(24))
    // and apply the delta to the shared atomic. Robust vs. steal-oldest /
    // grain-done / note-clear (which a fragile inc/dec would mishandle). The
    // atomic is a relaxed running sum across all voices (UI-05).
    void publishActiveCount() noexcept
    {
        int live = 0;
        for (const auto& g : grains)
            if (g.active) ++live;

        if (activeGrainCount != nullptr && live != myActiveCount)
            activeGrainCount->fetch_add (live - myActiveCount, std::memory_order_relaxed);

        myActiveCount = live;
    }

    //==========================================================================
    double sampleRate = 44100.0;
    float  voiceRate  = 1.0f;            // key-tracked read-increment ratio
    float  velLevel   = 1.0f;
    bool   gateHeld   = false;           // note currently held (drives spawning/lifetime when the ADSR is bypassed)

    // Source snapshot (raw pointer set per block; processor holds the shared_ptr).
    const float* sourcePtr = nullptr;
    int          sourceLen = 0;

    // Current playhead (resting point in 2.1; moving/freezable in 2.2).
    float playheadPos = 0.0f;

    // Phase 2.3 viz-tap seams (processor sets per block; null between blocks).
    GrainCloudFrame*  cloudFrame       = nullptr;   // grain events appended at spawn
    std::atomic<int>* activeGrainCount = nullptr;   // shared running grain count (UI-05)
    int               myActiveCount    = 0;         // this voice's last-published contribution

    // Preallocated bounded grain pool + scheduler state.
    std::array<Grain, OSimpleGrainAudioProcessor::kMaxGrainsPerVoice> grains {};
    int nextGrain = 0;
    int samplesUntilNextGrain = 0;

    // Per-voice RNG (no shared/global RNG — RT-safety gate 4). Unused in 2.1;
    // present so spray/scatter in 2.2 needs no new member.
    juce::Random rng;

    juce::ADSR ampEnv;
    juce::ADSR::Parameters ampParams { 0.01f, 0.3f, 0.8f, 0.4f };

    GrainVoiceParams params;
    const WindowLuts* windowLuts = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainVoice)
};
