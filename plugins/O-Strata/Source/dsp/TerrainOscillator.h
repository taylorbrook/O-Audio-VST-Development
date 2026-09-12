/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    TerrainOscillator.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    Live wave-terrain oscillator (ARCHITECTURE Core 1, 4, 10). Replaces the
    O-Prism wavetable oscillator in place: the public interface is the old one's
    minus the table setter, so the voice's inherited call sites (Sync / Bend /
    Window / FM warps, unison, Phase) compile unchanged. θ = 2π · phase is the
    orbit parameter; scan (phase, partial) = orbit (θ) → affine → feedback
    displacement → clamp → terrain → clamp → saturation.

    Per-sample setters store floats and do no computation (Core 1). The only
    pow / exp2 calls live in prepare / setUnison (cached) / updateBlockRate
    (DSP-05).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Orbits.h"
#include "Terrains.h"
#include "HalfbandDecimator.h"
#include "ChebyshevSet.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

// Moved from the wavetable oscillator header unchanged (Phase 2.1).
enum class WarpType
{
    Off = 0,
    Sync,
    Bend,
    FM,
    Window
};

// = the osc?Quality choice list. `Bandlimited` evaluates the published Chebyshev
// set (ARCH Core 6) at 1× when one matches the terrain, and falls back to the
// analytic terrain at 1× otherwise (never silence — plan Decision 30).
enum class Quality
{
    Bandlimited = 0,
    X2,
    X4
};

// = the osc?TerEdge choice list (0–1), read by the PNG path (TerrainImage::sample).
// HarnessWrap (100) is harness-only — periodic tiling, the H11 negative control on
// the edge branch — reachable only through harnessEdgeOverride[], never from the
// parameter (the voice maps the choice through jlimit (0, 1, …)).
enum class EdgeMode
{
    Mirror = 0,
    Window,
    HarnessWrap = 100
};

/** Core 10: view data for the ≋ waveform and the feedback trail. One ring per
    oscillator, owned by the processor; the display voice's partial 0 writes
    (θ radians, p.x, p.y, y pre-saturation) per SUB-sample (fs · OS — the write
    sits inside scan(), which runs OS times per base sample). Read by
    TerrainViewFeed on the message thread (Stage 3): 8192 slots hold a full
    cycle down to 23.4 Hz at 2× / 48 k (RESEARCH §2.3). */
struct CycleCapture
{
    static constexpr int kPoints = 8192;
    static_assert ((kPoints & (kPoints - 1)) == 0, "CycleCapture::kPoints must be a power of two (the writer masks the index)");
    std::array<float, kPoints * 4> ring {};
    std::atomic<uint32_t> writeIndex { 0 };
};

struct TerrainImage;

class TerrainOscillator
{
public:
    static constexpr int kMaxUnison = 4;   // was 8 in the wavetable oscillator (ARCH must-have 6, PERF-02)

    TerrainOscillator() = default;

    // ─── Inherited interface (the wavetable oscillator's, minus the table setter) ───
    void prepare (double sampleRate);
    void setFrequency (double freq);
    void setPosition (float pos);               // Orbit Size: r = 0.05 + 0.95 · pos
    void reset();
    void resetWithPhase (double phase);
    /** seed == 0 → the production address hash (unchanged behaviour); otherwise the
        LCG runs from `seed` so harness renders are deterministic (plan Decision 8). */
    void resetWithRandomPhases (uint32_t seed = 0);

    void getNextSampleStereo (double& outL, double& outR);

    void setUnison (int count, float detune, float width);   // cached on (count, detune, width) change
    void setWarpType (WarpType type);
    void setWarpAmount (float amount);
    void setFMInput (double value);
    double getFrequency() const { return frequency; }

    // ─── Block-rate setters (once per renderNextBlock) ───
    void setTerrain (TerrainKind k)     { terrainKind = k; }
    void setOrbit (OrbitKind k)         { orbitKind = k; }
    void setQuality (Quality q);
    void setPitchTrack (float t)        { pitchTrack = t; }
    void setFeedbackDamp (float d)      { feedbackDamp = d; }
    void setEdgeMode (EdgeMode e)       { edgeMode = e; }
    /** Round B (plan Decisions 28–31, 35): the published set for this oscillator
        (nullptr = none). Stored only; the tapered copy is rebuilt in updateBlockRate. */
    void setChebyshevSet (const ChebyshevSet* s) { chebSet = s; }
    /** The published image (Terrain = Imported); nullptr = none → silent. */
    void setImage (const TerrainImage* i)       { image = i; }
    /** Harness: force the analytic 1× path in Bandlimited (H6 "1x" column, H1). */
    void setChebyshevBypass (bool b)            { chebBypass = b; }
    bool isChebyshevActive() const              { return chebActive; }

    /** Orbit re-normalisation, Squarcle 1/tanh k, pitch-track ratio, damp
        coefficient, the Chebyshev taper / set crossfade — computed from constants
        and the block-start values, never from the block length (block-size
        invariance). `noteHz` = glide TARGET × pitch ratio (pitch tracking, ARCH
        Decision 2); `dmaxHz` = max (target, current) × ratio (the truncation law,
        plan Decision 27 — a downward glide never leaves a diagonal above Nyquist). */
    void updateBlockRate (double noteHz, double dmaxHz);

    // Harness-only switches (ARCH "Harness design"); copied from the processor
    // atomics by the voice at block start. Never parameters.
    void setKernelBypass (bool b)          { kernelBypass = b; }
    /** Phase 2.3 saving (plan Decision 17): false ⇒ the displacement arithmetic is
        skipped for the block (state held); the voice sets it from the block's Feedback
        row + isDestinationRouted. */
    void setFeedbackActive (bool b)        { feedbackActive = b; }
    void setFeedbackPathEnabled (bool b)   { feedbackPathEnabled = b; }
    void setSingleSampleFeedback (bool b)  { singleSampleFeedback = b; }
    void setSaturationBypass (bool b)      { saturationBypass = b; }
    void setDcBlockerBypass (bool b)       { dcBlockerBypass = b; }

    /** Core 10 ring; nullptr = not the display voice. */
    void setCaptureTarget (CycleCapture* c) { capture = c; }

    // ─── Per-sample setters (values already smoothed + modulated by the voice) ───
    void setAspect (float a)              { aspect = a; }
    void setRotation (float radians)      { rotation = radians; }
    void setCentre (float x, float y)     { centreX = x; centreY = y; }
    void setOrbitMod (float m)            { orbitMod = m; }
    /** Already exp2'd and clamped to [0.25, 8] by the voice (plan Decision 12). */
    void setTerrainFreq (float f)         { terrainFreq = f; }
    void setTerrainMod (float x, float y) { terrainModX = x; terrainModY = y; }
    void setFeedback (float fb)           { feedback = fb; }
    void setSaturation (float s)          { saturation = s; }

    /** Designed halfband latencies (base samples) — SUMMARY / --print-only. */
    static double decimatorLatency2() { return HalfbandCoeffs::get().latency2; }
    static double decimatorLatency4() { return HalfbandCoeffs::get().latency4; }

private:
    enum class XfadeKind { Quality, Set };
    void buildChebWeights (int idx, double dc) noexcept;
    float scan (double phase, int partial, bool shadow) noexcept;
    float saturate (float y) const noexcept;
    double applyWarp (double phase) const noexcept;
    /** One sub-sample of partial i: the inherited Sync / Window or Bend / FM branch
        with phase advance `inc` (= phaseIncrement / OS). `shadow` = crossfade's old
        path: local phase copies, no state writes. */
    float subSample (int i, double& phase, double& master, double inc, bool isSyncMode, bool shadow) noexcept;
    float decimate (int i, int set, int os, const float* sub) noexcept;
    static int osFor (Quality q) noexcept { return q == Quality::X4 ? 4 : q == Quality::X2 ? 2 : 1; }

    double currentSampleRate = 44100.0;
    double frequency = 440.0;
    double phaseIncrement = 0.0;
    float position = 0.5f;

    // Per-partial state (copied from the wavetable oscillator minus the table pointer)
    double phaseAccumulators[kMaxUnison] = {};
    double masterPhases[kMaxUnison] = {};
    double unisonDetuneFactors[kMaxUnison] = { 1.0, 1.0, 1.0, 1.0 };
    double unisonPanL[kMaxUnison] = { 1.0, 1.0, 1.0, 1.0 };
    double unisonPanR[kMaxUnison] = { 1.0, 1.0, 1.0, 1.0 };
    double unisonGain = 1.0;
    int unisonCount = 1;
    float cachedDetune = -1.0f, cachedWidth = -1.0f;   // setUnison cache key (plan Decision 10)

    // Warp state
    WarpType warpType = WarpType::Off;
    float warpAmount = 0.0f;
    double fmInput = 0.0;

    // Terrain / orbit selection and block-rate values
    TerrainKind terrainKind = TerrainKind::SineProduct;
    OrbitKind orbitKind = OrbitKind::Ellipse;
    Quality quality = Quality::X2;
    EdgeMode edgeMode = EdgeMode::Mirror;
    float pitchTrack = 1.0f;
    float feedbackDamp = 0.5f;
    float rTrack = 1.0f;      // min (1, C4 / f_note)^track — block-rate (ARCH Algorithm "Pitch tracking")
    float aEff = 0.978f;      // damp coefficient a^(48000 / (fs · OS)) — block-rate (ARCH Core 4)
    int   oversampling = 1;   // OS ∈ {1, 2, 4}; Phase 2.3 drives it from Quality
    OrbitScratch scratch;
    float lastNormalisedOrbitMod = -10.0f;
    OrbitKind lastNormalisedOrbit = OrbitKind::Ellipse;
    const float* superLUT = nullptr;

    // Per-sample values
    float aspect = 0.7f, rotation = 0.0f, centreX = 0.13f, centreY = 0.21f, orbitMod = 0.5f;
    float terrainFreq = 1.0f, terrainModX = 0.5f, terrainModY = 0.5f, feedback = 0.0f, saturation = 0.0f;
    float lastRotation = 1.0e9f, sinRot = 0.0f, cosRot = 1.0f;

    // Trajectory feedback per partial (Core 4; zeroed in reset*)
    float fbD[kMaxUnison] = {}, fbY1[kMaxUnison] = {}, fbY2[kMaxUnison] = {};
    bool feedbackActive = true;

    // Per-partial decimators, two sets so a Quality switch can run the old path as a
    // shadow while the new one fades in (ARCH Core 5, plan Decision 18)
    HalfbandStage2 hb2[2][kMaxUnison];
    HalfbandStage4 hb4[2][kMaxUnison];
    const HalfbandCoeffs* hbCoeffs = nullptr;
    int currentSet = 0;
    Quality oldQuality = Quality::X2;
    int xfadeRemaining = 0;          // base samples left in the 64-sample equal-gain crossfade
    XfadeKind xfadeKind = XfadeKind::Quality;
    bool fresh = true;               // no sample rendered since the last reset → Quality / set switches instantly

    // ─── Bandlimited mode (ARCH Core 6; plan Decisions 27–31, 35) ───
    // chebSet / image: the block's published pointers (dereferenced at block start only).
    // chebW[2]: the per-oscillator TAPERED coefficient copies — the sample loop reads
    // chebW[chebCur] (or chebW[shadowIdx] on the crossfade's old path), never the set.
    const ChebyshevSet* chebSet = nullptr;
    const ChebyshevSet* chebBuilt = nullptr;   // the set chebW[chebCur] was built from
    const TerrainImage* image = nullptr;
    float chebW[2][kChebCoeffs] = {};
    int chebCur = 0;
    bool chebActive = false;                    // this block evaluates clenshaw2D (chebW[chebCur])
    bool chebBypass = false;                    // harness
    double lastDc = -1.0;
    // The crossfade's old path: which evaluator (Chebyshev or analytic) and which copy
    bool shadowIsCheb = false;
    int shadowIdx = 1;

    // DC blocker (per oscillator, L and R): y = x − x1 + R · y1, R = 1 − 2π·5/fs
    double dcR = 1.0;
    double dcX1L = 0.0, dcY1L = 0.0, dcX1R = 0.0, dcY1R = 0.0;

    // Harness switches
    bool kernelBypass = false;
    bool feedbackPathEnabled = true;
    bool singleSampleFeedback = false;
    bool saturationBypass = false;
    bool dcBlockerBypass = false;

    CycleCapture* capture = nullptr;
};
