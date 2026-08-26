/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>

#include "Data/SceneModel.h"
#include "Data/VenueModel.h"
#include "Data/VenueSnapshot.h"
#include "DSP/ChannelMap.h"
#include "DSP/ConvexHull2D.h"
#include "DSP/GainStage.h"
#include "DSP/VerifyPing.h"

//==============================================================================
/**
    O-Octagon — Stage 2 Phase 2.2 (DBAP Solve + Gain Application).

    An 8-channel DBAP spatialiser for an irregular, non-flat concert array.

    Phase 2.1 gave the plugin the ROOM — the 42-value VENUE tree, the derived geometry, the convex
    hull and the speaker→buffer channel map — while every output still carried the same mono sum.
    PHASE 2.2 IS WHERE THE EIGHT LANES BECOME DIFFERENT: the DBAP solver, the 64-sample
    absolute-sample-aligned control grid, the 17 smoothed gains and the per-sample inner loop, all
    written through speakerToBuffer.

    Still absent, by plan: the hull gain trim and the air-absorption LPF (§5 step 6), the FUNC-07
    venue-trim fold, any read of `width` — all Phase 2.3, each marked by a greppable marker token in
    GainStage.cpp. Also absent: VerifyPing, metering, any WebView editor.

    Deliberately NOT an AsyncUpdater. The one thing that could have needed deferral —
    setStateInformation() arriving before prepareToPlay() — is handled by a plain `preparedYet` flag
    instead, so there is no queued apply that could stomp restored state and therefore no
    cancelPendingUpdate() obligation (pattern_asyncupdater_guard_flag_needs_cancel).
*/
class OOctagonProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    OOctagonProcessor();
    ~OOctagonProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    /** OVERRIDDEN, and it is not free — JUCE's default already passes through silently, but it
        would leave the ping's STATE running and the 120 s clock ticking (D11, RESEARCH-3.2).

        A bypassed plugin that keeps emitting noise is a genuinely confusing thing to debug on a
        stage: the first instinct is to bypass, and if that does not silence it the diagnosis goes
        somewhere wrong. */
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** The bound `publishSnapshot()` clamps every venue trim to, in dB, before converting to a
        linear factor (PLAN-2.3 P29 / RESEARCH-2.3 H5).

        PUBLIC so probe BH can assert against THIS SYMBOL rather than a transcribed 24.0f
        (`pattern_test_fixture_mirrors_drift_silently`). ±24 dB matches the hull trim's own −24 dB
        floor and comfortably contains FUNC-07's criteria, which require −12 dB and +6 dB to be
        reachable from the venue store.
    */
    static constexpr float kVenueTrimClampDb = 24.0f;

    /** v1.4.0 — the bound `publishSnapshot()` clamps every venue alignment delay to, in ms.

        PUBLIC for the same reason `kVenueTrimClampDb` is: the probes and `getVenueGeometry`'s
        payload both name THIS SYMBOL rather than a transcribed 50.0f
        (`pattern_test_fixture_mirrors_drift_silently`).

        AN ALIAS of `oo::plane::kMaxAlignDelayMs` — which the derived suggestion clamps to and the
        eight delay lines are sized for — so the three cannot drift. See that constant for why one
        definition replaced three literals and the static_asserts that policed them.
    */
    static constexpr float kVenueDelayClampMs = oo::plane::kMaxAlignDelayMs;

    //==============================================================================
    /** Read-only access to the room, for the editor and the tests. Message thread. */
    const oo::VenueModel&   getVenue() const noexcept  { return venue; }
    const oo::ConvexHull2D& getHull()  const noexcept  { return hull; }

    /** The parameter tree, for the Stage-3 editor's relays and for the render harness. Message
        thread. Matches the accessor name used across the rest of this repo. */
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    /** True when the last map build failed and the previous valid map is still in force. Drives the
        persistent FUNC-03 UI warning; never a reason to route audio anyway. */
    bool isChannelMapInvalid() const noexcept { return mapInvalid.load (std::memory_order_acquire); }

    /** True when the host negotiated a mono or stereo output — the defined, non-destructive SAFE
        fold rather than the 8-channel rig. Drives the Phase 3.1 SAFE banner (PLAN-3.1 P43).

        Written in prepareToPlay(), which is already the single site that knows the negotiated
        layout, so the derivation stays ADJACENT to the isBusesLayoutSupported() rule it mirrors.
        Deriving it a second time inside the editor's native function would have put a second copy
        of "which sets count as SAFE" a long way from the first.
    */
    bool isSafeMode() const noexcept { return safeMode.load (std::memory_order_acquire); }

    /** The venue publication counter, for the editor's geometry cache: the page refetches
        getVenueGeometry when this moves, which is how a venue change from ANY source — a session
        restore, a .venue load, a host preset — invalidates a cached envelope (PLAN-3.1 P42).

        Message thread / diagnostics ONLY. THE DIRTY CHECK MUST NOT READ THIS: it reads
        `snapshot.generation`, which arrives with the geometry it belongs to, and that is the H1
        bug P16 made unreachable at Phase 2.2.
    */
    std::uint32_t getVenueGeneration() const noexcept { return venuePublisher.getGeneration(); }

    /** Applies a venue edit: re-derives geometry, rebuilds the hull and the map, publishes a new
        snapshot, and writes the venue back into apvts.state. Message thread only.

        THE ONLY VENUE-APPLY PATH IN THE PLUGIN, and Phase 3.2 adds no second one — what it adds is
        the guard below, in FRONT of this. Still public because render-harness probe BL calls it
        directly; ui_frontend_check.js section 22 asserts that PluginEditor.cpp does NOT, which is
        what keeps "public" from becoming a hole. */
    void applyVenueEdit (const oo::VenueModel& newVenue);

    /** VALIDATES THE LABEL SET, THEN APPLIES. The editor's only route to the venue (PLAN-3.2 P52).

        ── WHY A GUARD IN FRONT OF applyVenueEdit() RATHER THAN A CHECK INSIDE IT ────────────────
        An invalid map is not a quiet retention. `mappedOutputAvailable()` false sends GainStage to
        its else arm, which writes `out[ch][n] = ch == 0 ? sL : sR` with `numWrite 8` — SPEAKER 1
        GETS THE LEFT INPUT AND SPEAKERS 2 THROUGH 8 ALL GET THE RIGHT ONE, AT UNITY
        (GainStage.cpp:408, :461; RESEARCH-3.2 N8). Under D8's commit-on-blur a label swap would
        hold that state on the PA for as long as the operator takes to type the second label.

        ── THE PREDICATE IS ochan::buildSpeakerToBuffer() ITSELF ────────────────────────────────
        It builds into a SCRATCH array, so this is not a second implementation of "is this label set
        valid" and the guard cannot drift from the audio path's backstop. One extra map build per
        commit, on the message thread, into a std::array<int,8>: free.

        The backstop stays exactly where it is. A session restored from a foreign venue, or a host
        renegotiating to a set that no longer contains a stored label, still fails in
        rebuildChannelMap() and still raises mapInvalid. This removes the TRANSIENT, not the net.

        @param whyNot  optional; on failure receives the reason and the 0-based row
        @returns       false and NOTHING APPLIED, or true and applyVenueEdit() has run
    */
    bool applyVenueEditChecked (const oo::VenueModel& newVenue, ochan::MapDiagnosis* whyNot = nullptr);

    /** The diagnosis from the last rebuildChannelMap(). Message thread both ends, so a plain member
        rather than an atomic (P43 reused, not re-argued): rebuildChannelMap() runs on the message
        thread and getStatus reads it there. `mapInvalid` itself STAYS the atomic it is, because
        that one really is read by the audio thread through mappedOutputAvailable(). */
    ochan::MapDiagnosis lastMapDiagnosis() const noexcept { return mapDiagnosis; }

    //==============================================================================
    // ── FUNC-04 — the verify ping ───────────────────────────────────────────────────────────────

    /** @param speakerOrAuto  1..8, or oo::VerifyPing::kAuto for the 1->8 cycle.
        @returns false, having started nothing, when the map is not usable.

        REFUSING IS THE POINT (RESEARCH-3.2 Q5). Pinging "speaker 5" on a stereo fold names a
        speaker that does not exist, during the one procedure whose entire purpose is confirming
        that speaker N is speaker N — R1 reproduced inside its own diagnostic tool. */
    bool startVerifyPing (int speakerOrAuto);

    /** D11's explicit Stop, and the editor destructor's. Graceful: the audio thread runs the 20 ms
        release. Message thread. */
    void stopVerifyPing();

    /** For the editor's 100 ms poll. Message thread; reads atomics the audio thread publishes. */
    oo::VerifyPing::State verifyPingState() const noexcept { return verifyPing.getState(); }

    //==============================================================================
    // ── UI-03 — the eight meters (Phase 3.3) ────────────────────────────────────────────────────

    /** The eight peaks since the last read, LINEAR, and ZEROED BY THE READ. Message thread.

        ── `exchange(0)` AND NOT A PLAIN LOAD (ARCHITECTURE §4.3 amendment 2 / PLAN-3.3 P77) ──────
        Read-and-zero means a DROPPED FRAME WIDENS THE MEASUREMENT WINDOW INSTEAD OF LOSING THE
        PEAK. That matters more here than the phrase suggests: a WebView completion is silently
        dropped when the editor is hidden (RESEARCH-3.2 N4), so frames really are lost in normal
        operation, and a plain load would show the operator the level from whichever 33 ms window
        happened to survive rather than the loudest thing that has happened since they last looked.

        LINEAR, not dB. The −60..0 dBFS mapping and the ballistics both live in js/meters.js, and
        sending the raw measurement keeps that transform in exactly one place.
    */
    std::array<float, ochan::kNumSpeakers> readAndZeroMeters() noexcept;

    //==============================================================================
    // ── FUNC-06 — weight scenes (Phase 3.3) ─────────────────────────────────────────────────────

    /** The four USER slots. Message thread; the editor reads them for `getScenes`. */
    const oo::SceneStore& getScenes() const noexcept { return sceneStore; }

    /** FUNC-06's write, and THE THIRD AND FINAL SITE OF THE GESTURE-BRACKET OBLIGATION (D18/P78).

        `beginChangeGesture()` → `setValueNotifyingHost()` → `endChangeGesture()` on EACH of
        `w1..w8`, closed on BOTH paths. `setValueNotifyingHost` is `setValue` +
        `sendValueChangedMessageToListeners` AND NOTHING ELSE — the wrappers turn that into a bare
        `kAudioUnitEvent_ParameterValueChange` (AU_1.mm:1341-1360) and a bare `paramChanged`
        (VST3.cpp:1498-1501), so in Logic with a lane in Latch or Touch the eight weights MOVE THE
        SOUND AND ARE NOT RECORDED. Nothing in build, `auval` or `pluginval` can see the omission.

        Closes `gesture_bracket_obligation` after the 3.1 puck (two parameters) and the 3.2 preset
        load (seventeen).

        ── WHY IT IS HERE AND NOT AT THE EDITOR'S CALL SITE — A RECORDED DEVIATION FROM P78 ──────
        P78 sites this beside `loadPreset` in PluginEditor.cpp. Its actual argument is C++-vs-JS:
        eight `SliderState` writes would scatter the obligation across 24 bridge messages where no
        single grep can confirm it. That argument is fully honoured by ONE C++ function with ONE
        call site — and putting it on the PROCESSOR buys something the editor cannot:
        **PLAN-3.3's own probe table makes CI a HARNESS probe**, and the render harness never
        compiles PluginEditor.cpp. On the editor the brackets could only ever have been grepped;
        here they are MEASURED, through real parameter listeners, by probe CI.

        @returns true. The empty-set refusal happens UPSTREAM, in the editor's `applyScene`, because
                 emptiness is a property of the SCENE and this function only sees eight numbers.
    */
    bool applySceneWeights (const std::array<float, oo::SceneStore::kNumSpeakers>& weights);

    /** D22's capture: reads the eight live `w` parameters into a slot and normalises the tree.

        In the PROCESSOR and not the editor because the processor owns `apvts.state`, and because
        the slot must survive an editor that is closed the instant after the click. The APPLY
        direction lives in the editor instead — it is eight bracketed host gestures, and brackets
        belong beside the other two sites of that obligation (P78).
    */
    void captureScene (int slot);

    /** Bumps on every scene-store write. The page mirrors `venueGen` with this so it knows when to
        refetch the four slots — they are NOT a function of the venue and so cannot ride
        `getVenueGeometry` the way named-scene membership does (P79 / Q10). */
    std::uint32_t getScenesGeneration() const noexcept { return scenesGeneration; }

    /** The four slots as a `var`, for the ONE `setCustomStateCallbacks` registration (D17 / P80).

        THIS IS THE ONLY ROUTE FROM A PRESET TO NON-PARAMETER STATE, and it reaches `SCENES` and
        nothing else — `VENUE` is not representable through it. That is what keeps FUNC-05's
        guarantee true while letting a musical preset carry its weight scenes, and it is why §27's
        assertion changed from "the symbol appears nowhere" to "exactly one registration, touching
        only SCENES". */
    juce::var    scenesToVar() const;
    void         scenesFromVar (const juce::var& payload);

    /** The published room, for the editor's field sampler. Message thread.

        `dbap::solve` is a free function with no state and no allocation (Q1), so sampling the field
        from here is not a threading question — but the GEOMETRY still has to be the one the audio
        thread is using, and that is this. */
    const oo::VenueSnapshot& getVenueSnapshot() const noexcept { return venuePublisher.read(); }

    /** v1.2.0 — hover-help ("?" toggle) preference. UI state, not a parameter: no automation, no
        preset membership. Written by the editor's setTooltipsEnabled native fn (message thread),
        persisted as a root XML ATTRIBUTE in get/setStateInformation — not a ValueTree property,
        whose XML round-trip rebuilds every property as a string so an isBool() guard on restore
        would never fire (critical_valuetree_xml_roundtrip_loses_type). */
    std::atomic<bool> tooltipsEnabled { false };

    /** v1.6.0 — the hover-help LANGUAGE. 0 = en, 1 = fr.

        Held as an int index rather than the string it persists as, because
        std::atomic<juce::String> does not compile — juce::String is not
        trivially copyable — so the audio-safe form is an index behind the
        two-function codec below while the PERSISTED form stays a language code.

        Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
        automation lane, and a preset must not be able to change which language
        somebody reads their help in. Like tooltipsEnabled it rides the session
        as a root XML ATTRIBUTE (idiom 2), which is the idiom this plugin
        already uses — a ValueTree property's XML round-trip rebuilds every
        property as a string, so a type predicate on restore never fires
        (critical_valuetree_xml_roundtrip_loses_type).

        Written by the editor's setUiLanguage native fn on the message thread;
        PULLED once by the page at init through getUiLanguage. */
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected bridge argument degrades to English
        rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

   #if OOCTAGON_INSTRUMENT
    /** The 17 smoothers' current values — test targets only (PLAN-2.2 P20). */
    std::array<float, oo::GainStage::kNumSmoothers> currentSmoothedGains() const noexcept
    {
        return gainStage.currentSmoothedValues();
    }
   #endif

private:
    //==============================================================================
    /** Write the four slots back into `apvts.state` AND bump the generation the page polls.

        The two lines are ONE invariant, not two statements: `scenesGeneration` is the only signal
        that tells the page its cached slots are stale, so a write that forgets the bump leaves the
        UI showing scenes the plugin no longer holds. It was three hand-kept copies (captureScene,
        scenesFromVar, setStateInformation) until v1.3.4 — a fourth scene-mutating path would have
        had to remember both lines.

        THE CONSTRUCTOR'S WRITE IS DELIBERATELY NOT A CALL TO THIS. It seeds the `SCENES` node at
        birth (N13) so a session saved before prepareToPlay() carries a complete tree; there is no
        cache to invalidate yet and `scenesGeneration` starts at 1, so bumping there would be a
        change in behaviour rather than a dedup. */
    void commitScenes();

    //==============================================================================
    // ─────────────────────────────────────────────────────────────────────────────
    // VENUE STORE — the slot claimed at Stage 1 (PLAN P2), now occupied.
    //
    // Declaration order matters and is why the slot was reserved above apvts rather than below it.
    // ─────────────────────────────────────────────────────────────────────────────

    oo::VenueModel   venue;
    oo::ConvexHull2D hull;

    /** FUNC-06's four user slots. A SIBLING of VENUE under apvts.state, never a child (D17), which
        is what makes FUNC-06/4's session round-trip STRUCTURAL: `getStateInformation` is
        `apvts.copyState()` → XML and the child rides along with no new code.

        Declared here, above apvts, for the same reason venue and hull are: the constructor writes
        it into `apvts.state` and declaration order decides what exists when. */
    oo::SceneStore   sceneStore;

    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    // ── The control-grid snapshot source ─────────────────────────────────────────────────────────
    //
    // Indexed by oo::params::Index rather than seventeen named members, because the control block
    // wants them as a flat array it can sanitise in one loop and memcmp in one call. The names live
    // in oo::params::id(), beside the enum, so there is one ordering rather than two.

    std::array<std::atomic<float>*, oo::params::kCount> paramPtr {};

    /** Each parameter's DECLARED default, in the same order.

        Used as the fallback when an atomic reads back non-finite (PLAN-2.2 P17). A NaN carries no
        information about which end to clamp to, so the default is the only honest substitute.

        DERIVED FROM THE PARAMETER OBJECTS in the constructor, never hand-transcribed: a written-out
        table of 17 defaults is exactly pattern_test_fixture_mirrors_drift_silently, and the
        parameter-spec gate would not catch the drift because it compares the PARAMETERS, not this
        array.
    */
    std::array<float, oo::params::kCount> paramDefaults {};

    //==============================================================================
    oo::GainStage gainStage;

    /** FUNC-04. A POST-WRITE OVERWRITE of the eight mapped output pointers, applied at the end of
        GainStage's REAL arm and indexed through the same speakerToBuffer — which is what makes it
        test the MAP rather than the chain (§7.2 / §OQ2). GainStage does not own it and does not
        decide when it runs; the processor passes it in (P24). */
    oo::VerifyPing verifyPing;

    //==============================================================================
    /** UI-03. Per-speaker peaks of THE WRITTEN OUTPUT BUFFER — max-held by the audio thread,
        consumed and zeroed by the message thread.

        ── THE INVARIANT IS A static_assert, NOT A COMMENT ──────────────────────────────────────
        A lock inside `std::atomic<float>` would be a lock on the audio thread, and it would be
        completely invisible: every probe would pass and the RT violation would only ever surface
        as a dropout in a hall. This project does not leave that class of invariant in prose
        (`pattern_ring_invariant_needs_static_assert`).

        ── THE LOAD/COMPARE/STORE RACE IS BENIGN, AND IS DOCUMENTED RATHER THAN HARDENED ────────
        One audio-thread writer does `if (pk > load) store (pk)`; one message-thread reader does
        `exchange (0)`. The only interleaving that differs from the sequential case is a store
        landing just after an exchange, which RE-PUBLISHES A PEAK THAT WAS ALREADY REPORTED — a
        duplicate on a max-hold display, never a lost peak. A CAS loop would buy nothing and would
        put a retry loop on the audio thread.
    */
    std::array<std::atomic<float>, ochan::kNumSpeakers> meterPeak {};

    static_assert (std::atomic<float>::is_always_lock_free,
                   "the meter array is written from the audio thread. A locking std::atomic<float> "
                   "would put a mutex in processBlock, PERF-01 would be violated with every probe "
                   "still green, and the only symptom would be a dropout in a hall.");

    /** Mirrors venueGeneration for the four user slots. Plain, not atomic: both ends are the
        message thread, which is P43's rule reused rather than re-argued. */
    std::uint32_t scenesGeneration { 1 };

    //==============================================================================
    // ── Channel map state (R1) ───────────────────────────────────────────────────────────────

    /** Speaker n → output buffer index. The LAST VALID map: on a failed rebuild this is retained
        unchanged and mapInvalid is raised, so the plugin never silently routes to a half-applied
        assignment. Initialised to identity so a pre-prepareToPlay read is defined. */
    std::array<int, ochan::kNumSpeakers> speakerToBuffer { 0, 1, 2, 3, 4, 5, 6, 7 };

    std::atomic<bool> mapInvalid { false };

    /** Why the last build failed, for the D13 banner. NOT an atomic and it does not need to be —
        both ends are the message thread. The same reasoning that keeps `outputSetName` off an
        atomic keeps this off one (P43). */
    ochan::MapDiagnosis mapDiagnosis {};

    /** Phase 3.1 (P43). Written in prepareToPlay() from the NEGOTIATED output layout, beside
        preparedYet, and read from the editor's message thread. An atomic because those are two
        different threads; and it is a bool rather than the set's NAME because a cross-thread
        juce::String is a race — the name is resolved inside the native function, on the message
        thread, from the bus that owns it. */
    std::atomic<bool> safeMode { false };

    oo::VenueSnapshotPublisher venuePublisher;

    /** setStateInformation() can arrive before prepareToPlay(). The map must be built from the
        NEGOTIATED layout, so in that window the rebuild is deferred to prepareToPlay() rather than
        adding a second construction site. A plain flag, not an AsyncUpdater — there is no queued
        apply to cancel (pattern_asyncupdater_guard_flag_needs_cancel). */
    bool preparedYet { false };

    //==============================================================================
    /** THE SINGLE CHANNEL-MAP CONSTRUCTION SITE.

        The only caller of ochan::buildSpeakerToBuffer() in the plugin, the only writer of
        mapInvalid, and the only publisher of speakerToBuffer into the snapshot. Called from
        prepareToPlay() and on a label-map edit — nowhere else.
    */
    void rebuildChannelMap();

    /** Reads the VENUE child of apvts.state (defaults if missing or partial), re-derives geometry,
        rebuilds the hull, and publishes a new snapshot. Message thread only.

        @param publish  Pass FALSE when a rebuildChannelMap() call follows immediately. That call
                        publishes unconditionally, so leaving this true makes TWO publishes land
                        microseconds apart on one message-thread call — and the 2-slot publisher's
                        second write lands in the very slot a live processBlock is still holding
                        (CODE_REVIEW WR-01). The suppression must itself be conditional wherever
                        the following rebuildChannelMap() is: see setStateInformation().
    */
    void readVenueFromState (bool publish = true);

    /** Copies the current venue + hull + map into a snapshot and publishes it. */
    void publishSnapshot();

    /** True iff the channel map may be used to index THIS block's buffer.

        BOTH conditions are load-bearing and neither implies the other (RESEARCH-2.1 G1):

          - a VALID map is not evidence of an 8-channel BUFFER. Under the F3 hazard — Standalone on
            a 3-7 output device — canonicalChannelSet(n) is rejected, Release KEEPS the 7.1 layout,
            and the buffer arrives with n < 8 channels. mapInvalid stays false the whole time while
            speakerToBuffer still holds indices up to 7. The map is derived from
            getTotalNumOutputChannels(), which is the accessor that lies in exactly this state;
          - an 8-channel buffer is not evidence of a valid map.

        Phase 2.2's GainStage inner loop CALLS THIS. It does not re-derive it.

        @param numOutputChannels  the count the caller already read from buffer.getNumChannels()
    */
    bool mappedOutputAvailable (int numOutputChannels) const noexcept;

    /** Loads the 17 atomics into a flat array, substituting the declared default for any value that
        reads back non-finite (PLAN-2.2 P17 / RESEARCH-2.2 H2).

        ── This guard closes a reachable, PERMANENT failure ──────────────────────────────────────
        A host really can write NaN into a parameter: juce::jlimit passes it straight through
        (juce_MathsFunctions.h:520-527) and the jassert inside clampTo0To1 is Debug-only. §3.3.4's
        all-zero-weight guard does not catch it either, because `NaN < kDenomEpsilon` is FALSE. It
        then reaches SmoothedValue::setTargetValue(NaN), which sets step = NaN, after which
        currentValue is NaN for the life of the object WITH NO SELF-HEALING PATH.

        ARCHITECTURE §3.5.2's claim that the TPT filter is "the only recursive element" is wrong —
        the 17 SmoothedValues are recursive too. Probe AR drives a parameter NaN, not only an input
        NaN, precisely because the input path self-heals and this one does not.

        Sanitising here also keeps NaN and denormal anomalies out of the dirty check.
    */
    oo::ParamSnapshot snapshotParameters() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OOctagonProcessor)
};
