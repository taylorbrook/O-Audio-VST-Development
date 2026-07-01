/*
  ==============================================================================

    SampleMap.h
    Microtonal Sample Engine - Sample-map POD storage
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1: findCell implemented as a linear scan over cells.

    Lifetime model: held by PluginProcessor as std::shared_ptr<SampleMap>.
    Background loader produces a new shared_ptr; message thread atomic-stores
    it into the processor's slot. Voices snapshot the shared_ptr in startNote
    (lock-free refcount inc) and hold for the note's duration.

    v1.8.0 — Round-Robin Samples
    ----------------------------
    The structure was refactored to support multiple SAMPLE VARIANTS per
    (midiNote, velocityLayer) coordinate ("cell"). Detection happens at load:

      - Filenames with explicit `rr[N]`, `take[N]`, or `tk[N]` tokens grouping
        the same (midi, layer) become silent variants of one cell.
      - Bare duplicates (same (midi, layer) without RR tokens) trigger a
        WebView confirmation modal — see PluginProcessor::pendingAmbiguousDuplicates.

    Voices select a variant per startNote via PluginProcessor's RR mode +
    per-cell counter (RT-safe atomic ops; no allocation in the audio path).

    The single-variant case is bit-identical to v1.7.1 — render-harness
    identity test (single-variant library) verifies zero overhead.

    Phase 3.1 invariant carried forward (per-variant):
      - SampleVariant::audio is std::shared_ptr<juce::AudioBuffer<float>> so
        per-cell replace can deep-copy SampleMap's cell vector cheaply.
        Voices read via variant->audio->getReadPointer(ch). Active-note voices
        keep their snapshot map alive (transitive ref) so a cell replacement
        mid-note does not invalidate the held buffer (Stage 2 EC-3).
      - SampleVariant::filename is the basename (File::getFileName()) populated
        by the loader. Used for Stage 3 UI cell tooltip / loop editor header.
      - SampleVariant::loopMode defaults to Auto with whole-file loop points
        (loopStart=0, loopEnd=N-2) since v1.4.0. Falls back to OneShot only
        when the buffer is too short for the cubic-context headroom (< 18
        samples). Manual mode is set when the user overrides loop points via
        the Stage 3 loop editor — now per-variant (each variant has its own
        loop region).
      - SampleMap::version is monotonic; bumped by the processor on every
        atomic-store. Stage 3 JS uses it for diff detection / stale-data
        guards in the async cell-replace queue (EC3-5).

  ==============================================================================
*/

#pragma once
// Specific JUCE module headers (instead of the build-system-generated
// <JuceHeader.h>) so this header is consumable by standalone test executables
// that don't go through juce_add_plugin (e.g. merge_rr_check.cpp).
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <limits>
#include <memory>
#include <vector>

enum class LoopMode
{
    OneShot = 0,    // Buffer too short for whole-file loop (< 18 samples)
    Auto    = 1,    // Default whole-file loop (loopStart=0, loopEnd=N-2)
    Manual  = 2     // User-overridden via Stage 3 loop editor
};

// v1.6.0: per-folder-load configuration. Passed to SampleLoader::loadFolder so
// the loader can either honour FilenameParser-derived velocities or force
// every produced slot onto a caller-specified target layer.
//
//   targetLayer     0..3 — which velocity row the new slots populate when
//                   overrideTokens=true (or when the parser couldn't infer
//                   a velocity, though current parser default is layer 0
//                   which we preserve).
//   overrideTokens  true  → ignore filename velocity tokens; force layer
//                          = targetLayer for every parsed slot.
//                   false → filename token wins (legacy v1.5.x behavior),
//                          targetLayer is ignored by the loader.
//
// v1.14.0: technique axis (engine + KS) — same idea as targetLayer/
// overrideTokens but for the technique dimension. Lets the user assign-folder-
// to-technique manually from the load modal even when filenames carry no
// technique token.
//
//   targetTechnique     0..7 — destination technique slot when override = true
//                       (or when the parser couldn't infer a technique, default
//                       behaviour preserved as 0 / "ord").
//   overrideTechnique   true  → ignore filename technique tokens; force tech
//                              = targetTechnique for every parsed slot.
//                       false → filename token wins (back-compat: pre-1.14.0
//                              folders with no technique tokens land on tech 0).
//
// The processor-level `LoadMode` (Append / ReplaceLayer / ReplaceAll) is
// orthogonal to this and never reaches the loader — merge semantics are a
// processor concern, not a loader concern.
struct LoadOptions
{
    int  targetLayer       = 0;
    bool overrideTokens    = false;
    int  targetTechnique   = 0;     // v1.14.0
    bool overrideTechnique = false; // v1.14.0
};

// v1.8.0: a single audio "take" within a cell. A cell holds one variant in the
// common (single-variant) case — bit-identical to v1.7.1. Multiple variants
// surface from explicit rr[N]/take[N]/tk[N] filename tokens or from
// user-confirmed bare duplicates (modal flow).
struct SampleVariant
{
    std::shared_ptr<juce::AudioBuffer<float>> audio;
    juce::String filename;                   // Basename only (File::getFileName())
    double       sourceSampleRate = 0.0;
    int          loopStart        = 0;
    int          loopEnd          = 0;       // 0 = no loop (one-shot)
    LoopMode     loopMode         = LoopMode::OneShot;
};

// v1.8.0: a cell is the (midiNote, velocityLayer) coordinate. It holds one or
// more SampleVariants. variants is never empty when the cell is present in
// SampleMap::cells — the loader filters out failed loads.
//
// v1.14.0: third axis — `technique`. (midi, layer) is no longer unique within
// a SampleMap; the cell key is the triplet (midiNote, velocityLayer,
// technique). technique is 0..(kMaxTechniques-1). Pre-1.14.0 presets and any
// load that didn't see a technique token default to technique=0 ("ord"),
// preserving bit-identity of single-technique libraries.
struct SampleCell
{
    int                        midiNote      = -1;
    int                        velocityLayer = 0;       // 0..3
    int                        technique     = 0;       // v1.14.0: 0..7
    std::vector<SampleVariant> variants;                // size >= 1 when populated

    // Convenience: the primary (first) variant. Used in legacy code paths
    // that haven't been variant-aware updated. RT-safe (no alloc, no throw).
    const SampleVariant& primary() const noexcept { return variants.front(); }
};

// v1.14.0: maximum number of technique slots per SampleMap. The plan ships
// 10 default technique names (ord/sp/st/sv/cs/pizz/harm/mart/trem/flaut) but
// only 8 fit the keyswitch range comfortably and the RR counter is sized
// accordingly. The user can rename / reduce; addTechniqueSlot caps growth.
inline constexpr int kMaxTechniques = 8;

// v1.23.0: maximum number of velocity (dynamic) layers per cell. Mirrors
// SampleMap::numVelocityLayers's documented 1..4 range and the literal `4`
// used throughout the voice's layer math. Named here so the trim table below
// stays in lockstep with the layer count.
inline constexpr int kMaxVelocityLayers = 4;

// v1.23.0: symmetric trim range in dB for the per-technique / per-layer trims.
// ±12 dB cut/boost, 0 dB = unity. Used to clamp the setters and to scale the
// WebView sliders.
inline constexpr float kTrimMaxDb = 12.0f;

// v1.23.0 — per-technique + per-(technique,layer) loudness trim table (dB).
//
// Library-balancing metadata ("make just staccato quieter", "pull down the mf
// layer of ord"), NOT performance automation — so it is deliberately kept OUT
// of the APVTS and round-trips through the state ValueTree like techniqueNames
// (see PluginProcessor capture/restoreStateValueTree). Owned by the processor;
// each voice holds a `const TrimTable*` and snapshots the relevant scalars at
// note-on (RT-safe atomic loads).
//
// Two independent dB axes combine additively, which is exactly multiplicative
// in linear gain: dbToGain(techDb + layerDb) == dbToGain(techDb)·dbToGain(layerDb).
// So a single dbToGain per (technique,layer) at note-on yields both the
// per-technique master trim AND the per-layer trim.
//
// Default 0 dB on every slot → gainFor() returns exactly 1.0f → bit-identical
// to v1.22.0 when the user never touches a trim.
struct TrimTable
{
    std::atomic<float> techniqueDb[kMaxTechniques];                     // master per technique
    std::atomic<float> layerDb     [kMaxTechniques][kMaxVelocityLayers]; // per (technique,layer)

    TrimTable() noexcept
    {
        for (int t = 0; t < kMaxTechniques; ++t)
        {
            techniqueDb[t].store (0.0f, std::memory_order_relaxed);
            for (int l = 0; l < kMaxVelocityLayers; ++l)
                layerDb[t][l].store (0.0f, std::memory_order_relaxed);
        }
    }

    // RT-safe combined linear gain for (technique, layer). One pow() per call —
    // invoked at note-on only (folded into layer weights / DynLayer trims), never
    // per-sample. Out-of-range indices return unity so the audio path is never
    // perturbed by a bad lookup.
    float gainFor (int technique, int layer) const noexcept
    {
        if (technique < 0 || technique >= kMaxTechniques
            || layer < 0 || layer >= kMaxVelocityLayers)
            return 1.0f;

        const float db = techniqueDb[technique].load (std::memory_order_acquire)
                       + layerDb[technique][layer].load (std::memory_order_acquire);
        return juce::Decibels::decibelsToGain (db);
    }
};

struct SampleMap
{
    std::vector<SampleCell> cells;
    int                     lowestNote        = 127;
    int                     highestNote       = 0;
    int                     numVelocityLayers = 1;  // 1..4
    int                     version           = 0;  // Phase 3.1: monotonic counter

    // Linear scan returning the cell in `velocityLayer`+`technique` whose
    // midiNote is closest to the requested `midiNote`. Implements
    // REQUIREMENTS §FUNC-04 ("or nearest if N is unsampled") — the voice's
    // repitch path (computePlayRateForVariant) handles the pitch shift.
    //
    // v1.14.0: when `technique` is non-zero and no cell exists for the
    // requested technique slot, the search falls back to `technique=0`
    // ("ord") — the user-facing default — instead of returning nullptr,
    // matching the plan's RT-safety contract. Returns nullptr only when both
    // the requested technique AND the fallback contain no cells in
    // `velocityLayer`.
    //
    // Tie-breaking: when two cells are equidistant, prefer the lower midiNote
    // so the equidistant choice transposes UP — preserves more of the
    // sample's low-frequency content than a downshift would.
    const SampleCell* findCell (int midiNote, int velocityLayer,
                                int technique) const noexcept
    {
        if (auto* hit = findCellExact (midiNote, velocityLayer, technique))
            return hit;

        if (technique != 0)
            return findCellExact (midiNote, velocityLayer, 0);

        return nullptr;
    }

    // Back-compat overload — the v1.13.0 two-argument form. Defaults to the
    // "ord" technique slot (0). Kept so legacy single-technique callers (UI
    // snapshots, waveform peeks) continue to work without churning every
    // call site.
    const SampleCell* findCell (int midiNote, int velocityLayer) const noexcept
    {
        return findCell (midiNote, velocityLayer, 0);
    }

    // v1.17.2: layer-tolerant lookup for the voice's PRIMARY cell selection.
    // Tries the requested `velocityLayer` first (with the technique→0 fallback
    // of findCell). If that layer has no cell, it expands OUTWARD by layer
    // distance and returns the cell in the nearest POPULATED layer instead of
    // nullptr.
    //
    // Why: filename parsing can land a whole library on a single non-zero
    // velocity layer (e.g. an all-`mf` library → layer 2). Without this
    // fallback the voice goes silent at any note-on velocity that buckets to
    // an empty layer. This is the safety net that lets the parser honour
    // pre-note dynamics (v1.17.2 PARSE-DYN) without re-introducing that
    // silence. A fully-populated multi-layer library never reaches the
    // expansion loop (the exact layer always hits), so existing libraries are
    // unaffected.
    //
    // Tie-break: equidistant layers prefer the LOWER (quieter) layer. RT-safe:
    // no allocation, no throw — pure linear scans over `cells`.
    //
    // NOTE: deliberately NOT used for the crossfade-partner lookup in the
    // voice — that path must stay exact (an empty adjacent layer means "no
    // crossfade", not "borrow a distant layer").
    //
    // v1.18.3: TECHNIQUE fidelity now wins over velocity-layer fidelity. We
    // search the REQUESTED technique across ALL velocity layers first, and only
    // fall back to technique 0 ("ord") once we have proven the requested slot is
    // entirely empty on every layer. Previously this called findCell(), whose
    // per-layer technique→0 fallback fired at the requested layer BEFORE the
    // cross-layer expansion ran — so an articulation populated on fewer velocity
    // layers than ord (e.g. a single-dynamic pizz vs a 4-layer ord) was shadowed
    // by ord for any note velocity that bucketed to a layer it didn't cover. The
    // technique cursor stayed on pizz but the note sounded ord. Now pizz is
    // always pizz, relayered/repitched as needed.
    const SampleCell* findCellNearestLayer (int midiNote, int velocityLayer,
                                            int technique) const noexcept
    {
        // Phase 1 — requested technique, requested layer then expand outward
        // (distance 1,2,3,…; prefer the lower layer at each distance). No ord
        // fallback here: findCellExact never substitutes technique 0.
        if (auto* hit = findCellExact (midiNote, velocityLayer, technique))
            return hit;

        for (int delta = 1; delta <= 4; ++delta)
        {
            const int lo = velocityLayer - delta;
            if (lo >= 0)
                if (auto* hit = findCellExact (midiNote, lo, technique))
                    return hit;

            const int hi = velocityLayer + delta;
            if (hi <= 3)
                if (auto* hit = findCellExact (midiNote, hi, technique))
                    return hit;
        }

        // Phase 2 — requested technique slot is empty on every layer; fall back
        // to technique 0 ("ord") with the same layer-tolerant outward search.
        // Preserves the existing "empty technique slot plays ord" contract,
        // deferred until the slot is proven empty.
        if (technique != 0)
        {
            if (auto* hit = findCellExact (midiNote, velocityLayer, 0))
                return hit;

            for (int delta = 1; delta <= 4; ++delta)
            {
                const int lo = velocityLayer - delta;
                if (lo >= 0)
                    if (auto* hit = findCellExact (midiNote, lo, 0))
                        return hit;

                const int hi = velocityLayer + delta;
                if (hi <= 3)
                    if (auto* hit = findCellExact (midiNote, hi, 0))
                        return hit;
            }
        }

        return nullptr;
    }

    // v1.21.0: gather, in ASCENDING velocity-layer order (= ascending loudness),
    // the nearest-midi populated cell in each layer for the given technique.
    // Used by the voice's CC-crossfade dynamics mode to build the layer stack
    // it morphs across.
    //
    // Deliberately uses findCellExact (NO technique→0 fallback) so every cell
    // written to `outCells` shares ONE technique — mixing articulations across
    // the loudness axis would be audibly wrong. The caller handles the ord
    // fallback at the technique level (gather requested technique; if empty,
    // gather technique 0).
    //
    // Writes at most `maxOut` pointers and returns the count written. Empty
    // layers are skipped (no null gaps in the output). RT-safe: pure linear
    // scans over `cells`, no allocation, no throw.
    int gatherLayerCells (int midiNote, int technique,
                          const SampleCell** outCells, int maxOut) const noexcept
    {
        int count = 0;
        // velocityLayer is 0..3 (numVelocityLayers <= 4); iterate the full
        // range and let findCellExact return null for unpopulated layers.
        for (int layer = 0; layer < 4 && count < maxOut; ++layer)
            if (auto* hit = findCellExact (midiNote, layer, technique))
                outCells[count++] = hit;

        return count;
    }

private:
    const SampleCell* findCellExact (int midiNote, int velocityLayer,
                                     int technique) const noexcept
    {
        const SampleCell* best     = nullptr;
        int               bestDist = std::numeric_limits<int>::max();

        for (const auto& c : cells)
        {
            if (c.velocityLayer != velocityLayer)
                continue;
            if (c.technique != technique)
                continue;

            const int d = std::abs (c.midiNote - midiNote);
            if (d < bestDist
                || (d == bestDist && best != nullptr && c.midiNote < best->midiNote))
            {
                best     = &c;
                bestDist = d;
            }
        }

        return best;
    }
};

//==============================================================================
// v1.9.0 — round-robin merge helper.
//
// Inserts `newCell` into `targetCells`. If a cell with matching (midiNote,
// velocityLayer, technique) already exists, the new cell's variants are
// appended onto the existing cell's variants vector instead of replacing
// the cell — used by LoadMode::MergeRR and the per-cell single-file merge
// path.
//
// v1.14.0: the match key gained the technique axis. Two cells with matching
// (midi, layer) but DIFFERENT technique slots no longer collide — they
// coexist in `targetCells` as two distinct cells.
//
// Variant cap: each cell holds at most `maxVariantsPerCell` (64). Excess
// variants from `newCell.variants` past the cap are skipped; their filenames
// are appended to `outSkipped` (prefixed with "variant cap reached: ") so
// the UI can surface them in the existing skipped-files toast.
//
// Returns true if the merge produced any visible change (cell added or
// variants appended), false if the cap rejected every variant.
//
// This helper is pure: no atomic ops, no callbacks, no RR-counter access. The
// processor wraps it with the atomic-store + counter reset for live cells.
inline bool applyMergeRrCell (std::vector<SampleCell>& targetCells,
                              const SampleCell&        newCell,
                              int                      maxVariantsPerCell,
                              juce::StringArray&       outSkipped)
{
    auto it = std::find_if (targetCells.begin(), targetCells.end(),
        [&newCell] (const SampleCell& c)
        {
            return c.midiNote      == newCell.midiNote
                && c.velocityLayer == newCell.velocityLayer
                && c.technique     == newCell.technique;
        });

    if (it == targetCells.end())
    {
        targetCells.push_back (newCell);
        return true;
    }

    const int existing = (int) it->variants.size();
    const int room     = std::max (0, maxVariantsPerCell - existing);
    const int incoming = (int) newCell.variants.size();
    const int toAdd    = std::min (room, incoming);

    for (int i = toAdd; i < incoming; ++i)
        outSkipped.add ("variant cap reached: "
            + newCell.variants[(size_t) i].filename);

    if (toAdd <= 0)
        return false;

    it->variants.reserve ((size_t) (existing + toAdd));
    for (int i = 0; i < toAdd; ++i)
        it->variants.push_back (newCell.variants[(size_t) i]);
    return true;
}
