/*
  ==============================================================================

    TechniqueDefaults.h
    O-MicrotonalSampler — canonical defaults for the technique axis.

    Single source of truth for the fresh-instance technique configuration so
    the APVTS parameter layout, resetTechniqueNames(), processBlock's keyswitch
    scan, and the unit tests can never drift apart again. Introduced in v1.23.3
    to close the 2026-06-30 review findings:

      * WR-03 — keyswitches are now OPT-IN. A fresh instance shipped
        ks_enabled=true over MIDI 0..9, so processBlock silently absorbed every
        note-on in that range and never forwarded it to the synth; any library
        mapped into the low register lost those notes with no visible cause. The
        default is now ks_enabled=false — nothing is absorbed until the user
        turns keyswitching on.
      * IN-04 — the default KS range is now exactly kMaxTech slots wide
        (MIDI 0..7), so when a user does enable keyswitching no two candidate
        notes collapse onto the same technique (the old 0..9 range saturated
        notes 8 and 9 onto the last slot).
      * IN-03 — the default technique vocabulary lived in three places that had
        drifted apart (resetTechniqueNames vs. two header docstrings vs. a test
        fixture). It now lives here once.

    Header-only, pure data — no JuceHeader.h, no audio processor — so the audio
    thread, the message thread, and the test executables all share identical
    logic (same pattern as TriggerMapping.h).

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include "TriggerMapping.h"   // OMtsTrigger::kMaxTech

namespace OMtsTechnique
{
    // ---- Fresh-instance keyswitch defaults (review WR-03 / IN-04) ----
    //
    // Keyswitches are OPT-IN. With ks_enabled=false and no CC/PC trigger the
    // active technique cursor stays at slot 0, so playback reproduces v1.13.0
    // behaviour (every cell read at technique=0) and no note-on is ever
    // absorbed. When the user enables keyswitching, the default range spans
    // exactly kMaxTech slots (MIDI 0..7 == ksLow .. ksLow + kMaxTech - 1) so
    // one semitone maps to one technique with no saturation.
    static constexpr bool kDefaultKsEnabled  = false;
    static constexpr int  kDefaultKsLowNote  = 0;
    static constexpr int  kDefaultKsHighNote = OMtsTrigger::kMaxTech - 1;   // 7

    // Number of technique slots seeded on a fresh instance. Eight named slots
    // are visible in the UI even with keyswitches off; they simply are not
    // reachable until a trigger (KS/CC/PC) or a UI click selects them.
    static constexpr int  kDefaultTechniqueCount = OMtsTrigger::kMaxTech;    // 8

    // Curated default technique vocabulary. LOAD-BEARING: slots 0..7 align with
    // the Dorico Strings expression map shipped in
    //   Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib
    // (natural, sulPont, sulTasto, staccato, muted, pizzicato, harm, trem),
    // made canonical in v1.16.3. Keep this in sync with that map.
    inline juce::StringArray defaultTechniqueVocabulary()
    {
        return juce::StringArray { "ord", "sp", "st", "stacc",
                                   "cs",  "pizz", "harm", "trem" };
    }

    // Map a keyswitch note-on to a technique slot. Returns the technique index
    // (0 .. min(kMaxTech-1, techCount-1)) for a note inside [ksLow..ksHigh], or
    // -1 for a note outside the range (which processBlock forwards to the synth
    // instead of absorbing). Mirrors — and is the single source for — the
    // candidate math in PluginProcessor::processBlock, so the absorption
    // contract is unit-testable without instantiating the audio processor.
    inline int keyswitchTechnique (int noteNumber, int ksLow, int ksHigh,
                                   int techCount) noexcept
    {
        if (noteNumber < ksLow || noteNumber > ksHigh)
            return -1;
        return juce::jlimit (0, juce::jmin (OMtsTrigger::kMaxTech - 1, techCount - 1),
                             noteNumber - ksLow);
    }
}
