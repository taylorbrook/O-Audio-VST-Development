/*
  ==============================================================================

    merge_rr_check.cpp
    O-MicrotonalSampler — v1.9.0 round-robin merge unit tests.

    Manual run: build with `ninja O-MicrotonalSampler_MergeRrCheck` and
    execute the produced binary. Returns exit code = number of failed cases
    (0 = all pass).

    What this exercises
    -------------------
    The pure helper `applyMergeRrCell` (declared in SampleMap.h) is used by
    both LoadMode::MergeRR (folder load) and the per-cell single-file merge
    path. These tests assert the four invariants that callers rely on:

      1. **No-collision insert** — a new cell with a unique (midi, layer)
         is appended to `targetCells` unchanged.
      2. **Collision merge** — a new cell whose (midi, layer) matches an
         existing one has its variants APPENDED in load order; the existing
         cell stays in place (no reordering, no replacement).
      3. **Variant cap (64)** — when an existing cell already has K variants,
         only `64 - K` of the incoming variants are appended; the rest are
         pushed to `outSkipped` with a "variant cap reached: <filename>"
         prefix so the UI can surface them in the existing skipped-files
         toast. The helper returns false if every incoming variant is
         dropped, true otherwise.
      4. **Multi-cell input** — each call merges one cell, so callers that
         iterate over a producer's cells get deterministic ordering: the
         first call may grow an existing cell; the second call appends a
         fresh new cell.

    Audio buffers are intentionally null `shared_ptr`s — the helper never
    touches the audio payload, only the variants vector + filenames. Real
    loads carry populated buffers, but this test validates the algorithm.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "../SampleMap.h"
#include <iostream>
#include <string>

namespace
{
    SampleVariant makeVariant (const juce::String& filename)
    {
        SampleVariant v;
        v.filename = filename;
        // audio left as nullptr — helper doesn't dereference it.
        return v;
    }

    SampleCell makeCell (int midi, int layer, std::initializer_list<const char*> filenames)
    {
        SampleCell c;
        c.midiNote      = midi;
        c.velocityLayer = layer;
        for (const char* f : filenames)
            c.variants.push_back (makeVariant (juce::String (f)));
        return c;
    }

    int failed = 0;

    void check (bool cond, const std::string& desc)
    {
        if (cond)
        {
            std::cout << "  PASS: " << desc << "\n";
        }
        else
        {
            std::cout << "  FAIL: " << desc << "\n";
            ++failed;
        }
    }

    // -------------------- Test 1: no-collision insert --------------------
    void testNoCollisionInsert()
    {
        std::cout << "[Test 1] No-collision insert\n";
        std::vector<SampleCell> target;
        target.push_back (makeCell (60, 0, { "c4_v1.wav" }));

        juce::StringArray skipped;
        const bool changed = applyMergeRrCell (target,
                                                makeCell (62, 0, { "d4_v1.wav" }),
                                                64, skipped);

        check (changed,                       "returns true on insert");
        check (target.size() == 2,            "target now has 2 cells");
        check (skipped.isEmpty(),             "no skipped variants");
        check (target[0].midiNote == 60
            && target[0].variants.size() == 1, "existing cell unchanged");
        check (target[1].midiNote == 62
            && target[1].variants.size() == 1, "new cell appended");
    }

    // -------------------- Test 2: collision merge --------------------
    void testCollisionMerge()
    {
        std::cout << "[Test 2] Collision merge — variants appended in load order\n";
        std::vector<SampleCell> target;
        target.push_back (makeCell (60, 0, { "c4_take1.wav", "c4_take2.wav" }));

        juce::StringArray skipped;
        const bool changed = applyMergeRrCell (target,
                                                makeCell (60, 0, { "c4_take3.wav", "c4_take4.wav" }),
                                                64, skipped);

        check (changed,                       "returns true on merge");
        check (target.size() == 1,            "still 1 cell — no duplicate added");
        check (skipped.isEmpty(),             "no skipped variants");
        check (target[0].variants.size() == 4, "cell now holds 4 variants");
        check (target[0].variants[0].filename == juce::String ("c4_take1.wav"),
               "variant 0 preserved");
        check (target[0].variants[1].filename == juce::String ("c4_take2.wav"),
               "variant 1 preserved");
        check (target[0].variants[2].filename == juce::String ("c4_take3.wav"),
               "variant 2 = first new in load order");
        check (target[0].variants[3].filename == juce::String ("c4_take4.wav"),
               "variant 3 = second new in load order");
    }

    // -------------------- Test 3: variant cap (64) --------------------
    void testVariantCap()
    {
        std::cout << "[Test 3] Variant cap at 64 — overflow goes to outSkipped\n";
        std::vector<SampleCell> target;

        // Existing cell with 60 variants.
        SampleCell base;
        base.midiNote      = 60;
        base.velocityLayer = 0;
        for (int i = 1; i <= 60; ++i)
            base.variants.push_back (makeVariant ("c4_take" + juce::String (i) + ".wav"));
        target.push_back (std::move (base));

        // Incoming cell with 10 new variants — only 4 should fit, 6 skipped.
        SampleCell incoming;
        incoming.midiNote      = 60;
        incoming.velocityLayer = 0;
        for (int i = 61; i <= 70; ++i)
            incoming.variants.push_back (makeVariant ("c4_take" + juce::String (i) + ".wav"));

        juce::StringArray skipped;
        const bool changed = applyMergeRrCell (target, incoming, 64, skipped);

        check (changed,                       "returns true (4 added)");
        check (target[0].variants.size() == 64, "cell capped at 64 variants");
        check (skipped.size() == 6,           "6 overflow filenames recorded");
        check (skipped[0].startsWith ("variant cap reached: "),
               "skipped entries carry the expected prefix");
        check (skipped[0].contains ("c4_take65.wav"),
               "first overflow is take65 (61..64 fit, 65..70 skipped)");
        check (skipped[5].contains ("c4_take70.wav"),
               "last overflow is take70");
        check (target[0].variants[63].filename == juce::String ("c4_take64.wav"),
               "last fitted variant is take64");
    }

    // -------------------- Test 4: cap already reached → no-op --------------------
    void testCapAlreadyReached()
    {
        std::cout << "[Test 4] Cap already reached — incoming entirely skipped\n";
        std::vector<SampleCell> target;

        SampleCell base;
        base.midiNote      = 60;
        base.velocityLayer = 0;
        for (int i = 1; i <= 64; ++i)
            base.variants.push_back (makeVariant ("c4_take" + juce::String (i) + ".wav"));
        target.push_back (std::move (base));

        juce::StringArray skipped;
        const bool changed = applyMergeRrCell (target,
                                                makeCell (60, 0, { "c4_take65.wav", "c4_take66.wav" }),
                                                64, skipped);

        check (! changed,                     "returns false when nothing fit");
        check (target[0].variants.size() == 64, "cell still at 64 (unchanged)");
        check (skipped.size() == 2,           "both incoming variants recorded as skipped");
    }

    // -------------------- Test 5: layer-aware collision detection --------------------
    void testLayerAwareCollision()
    {
        std::cout << "[Test 5] (midi, layer) is the collision key — same midi, different layer\n";
        std::vector<SampleCell> target;
        target.push_back (makeCell (60, 0, { "c4_v1.wav" }));

        juce::StringArray skipped;
        applyMergeRrCell (target,
                          makeCell (60, 1, { "c4_v2.wav" }),
                          64, skipped);

        check (target.size() == 2,            "C4 L0 and C4 L1 are distinct cells");
        check (target[0].velocityLayer == 0,  "L0 cell preserved");
        check (target[1].velocityLayer == 1,  "L1 cell appended (no merge)");
        check (target[0].variants.size() == 1, "L0 still has 1 variant");
        check (target[1].variants.size() == 1, "L1 has 1 variant");
    }

    // -------------------- Test 6: multi-call ordering (folder-load shape) --------------------
    void testMultiCallOrdering()
    {
        std::cout << "[Test 6] Multi-call: producer iterates cells, helper handles each\n";
        std::vector<SampleCell> target;
        target.push_back (makeCell (60, 0, { "c4_a.wav" }));
        target.push_back (makeCell (62, 0, { "d4_a.wav" }));

        // Simulate a folder-load with two cells: one collides (60,0), one fresh (64,0).
        std::vector<SampleCell> incoming;
        incoming.push_back (makeCell (60, 0, { "c4_b.wav" }));
        incoming.push_back (makeCell (64, 0, { "e4_a.wav" }));

        juce::StringArray skipped;
        for (const auto& c : incoming)
            applyMergeRrCell (target, c, 64, skipped);

        check (target.size() == 3,            "3 cells after multi-call merge");
        check (target[0].variants.size() == 2, "60,0 grew to 2 variants");
        check (target[0].variants[1].filename == juce::String ("c4_b.wav"),
               "incoming variant landed at end");
        check (target[1].variants.size() == 1 && target[1].midiNote == 62,
               "62,0 untouched");
        check (target[2].variants.size() == 1 && target[2].midiNote == 64,
               "64,0 appended fresh");
    }
}

int main (int /*argc*/, char** /*argv*/)
{
    std::cout << "=== O-MicrotonalSampler v1.9.0 merge-RR helper tests ===\n";
    testNoCollisionInsert();
    testCollisionMerge();
    testVariantCap();
    testCapAlreadyReached();
    testLayerAwareCollision();
    testMultiCallOrdering();
    std::cout << "\n=== Result: " << (failed == 0 ? "ALL PASSED" : "FAILED") << " ("
              << failed << " failure(s)) ===\n";
    return failed;
}
