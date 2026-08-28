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
#include "PluginProcessor.h"

// Phase 4.1 (P91). The SAFE-mode partition, extracted so probe CO can present sets that
// isBusesLayoutSupported() rejects. Header-only, juce_audio_basics only.
#include "Data/RigPolicy.h"

// Phase 3.1. Included ONLY from inside the guard: the render harness builds this TU with
// JUCE_WEB_BROWSER=0 and no editor sources, and WebBrowserComponent's types do not exist there
// (pattern_render_harness_breaks_on_webview_editor).
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

#include <cmath>

//==============================================================================
namespace
{
    // Two-argument NormalisableRange<float>(min, max) yields interval 0, skew 1. The 4-argument
    // form with an explicit 1.0f skew is deliberately NOT used: it reads as though a skew were
    // intended and invites a future "fix". All 17 ranges are linear by design (parameter-spec.md).
    juce::NormalisableRange<float> linearRange (float lo, float hi)
    {
        return juce::NormalisableRange<float> (lo, hi);
    }

    // Hint generations. NEVER renumber — append a new constant for the next release that adds a
    // parameter. The render harness's DN probe holds every id to its generation.
    constexpr int kHintV1_0 = 1;   // the 17 originals (v1.0.0 – v1.4.0)
    constexpr int kHintV1_5 = 2;   // decorr
    constexpr int kHintV1_8 = 3;   // the ten motion* parameters

    std::unique_ptr<juce::AudioParameterFloat> makeFloat (int versionHint,
                                                          const juce::String& id,
                                                          const juce::String& name,
                                                          juce::NormalisableRange<float> range,
                                                          float defaultValue,
                                                          const juce::String& label = {})
    {
        auto attributes = juce::AudioParameterFloatAttributes();

        if (label.isNotEmpty())
            attributes = attributes.withLabel (label);

        // The version hint (second ParameterID argument) is mandatory in JUCE 8 — and it is NOT a
        // formality. The AU wrapper sorts the parameter list by masked ID hash and then STABLE-sorts
        // by version hint; Logic keys automation lanes by INDEX in that list. Every parameter added
        // after a release MUST carry a hint HIGHER than everything that shipped before it, or the
        // new hashes interleave with the old and every lane past the first collision retargets.
        // (v1.10.0 / WR-02: v1.5–v1.8 shipped all-1, which moved outputGain -> motionHeight.)
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, versionHint },
                                                            name,
                                                            range,
                                                            defaultValue,
                                                            attributes);
    }

    // v1.8.0 — THE FIRST NON-FLOAT PARAMETERS. A Bool and two Choices, because `motionPath` in a
    // host automation lane must read "Figure-8", not 0.2. Choice strings are ASCII: juce::String's
    // const char* constructor is ASCII-only (critical_juce_string_char_ctor_is_ascii_only).
    std::unique_ptr<juce::AudioParameterBool> makeBool (int versionHint,
                                                        const juce::String& id,
                                                        const juce::String& name,
                                                        bool defaultValue)
    {
        return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, versionHint },
                                                           name, defaultValue);
    }

    std::unique_ptr<juce::AudioParameterChoice> makeChoice (int versionHint,
                                                            const juce::String& id,
                                                            const juce::String& name,
                                                            const juce::StringArray& choices,
                                                            int defaultIndex)
    {
        return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, versionHint },
                                                             name, choices, defaultIndex);
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OOctagonProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Five groups (PLAN P1). The headline gesture of this plugin is automating eight weights;
    // a flat 17-entry menu buries them. Group membership does not participate in parameter
    // identity, so this stays reversible if Stage 4 host testing disagrees.

    // ── Position ────────────────────────────────────────────────────────────────
    // srcX/srcY display NORMALISED 0.00–1.00 in the host lane and take no label. Metres are a
    // Stage 3.1 UI-side conversion: the metre readout depends on the live venue bounding box, and
    // a host value→text lambda is captured at construction and therefore cannot read a live venue
    // (pattern_webview_knob_readout_scaled_value). Do not add a this-capturing lambda here.
    layout.add (std::make_unique<juce::AudioProcessorParameterGroup> (
        "position", "Position", "|",
        makeFloat (kHintV1_0, "srcX",  "Source X", linearRange (0.0f, 1.0f), 0.5f),
        makeFloat (kHintV1_0, "srcY",  "Source Y", linearRange (0.0f, 1.0f), 0.5f),
        makeFloat (kHintV1_0, "srcZ",  "Source Z", linearRange (-2.0f, 8.0f), 0.0f, "m"),
        // v1.3.0: width max 6 → 12 m. At 6 m the two sub-points' gain vectors differed by ≤ 2.5 dB
        // per channel in the geometrically flat default field — barely audible. 12 m puts them near
        // opposite walls of the default room at full width. Presets saved under < 1.3.0 carry the
        // old normalised encoding and are re-mapped (÷2) by the editor's migration hook.
        makeFloat (kHintV1_0, "width", "Width",    linearRange (0.0f, 12.0f), 0.0f, "m"),
        // v1.5.0. THE 18th PARAMETER, AND IT DEFAULTS TO 0 FOR A COMPATIBILITY REASON, NOT A
        // TASTE ONE: at 0 GainStage bypasses the decorrelation network entirely, so every session
        // and every preset written before v1.5.0 renders bit-identically. Probe CU holds that
        // against the v1.4.0 binary's own digest.
        //
        // Unit-less rather than "ms" although it scales a dispersion time: the time it produces
        // also depends on the effective width (GainStage scales depth by wEff), so a millisecond
        // label would be a number the control does not actually deliver.
        //
        // IN "Position" AND NOT "Space" BECAUSE IT IS PART OF WIDTH. It is gated on wEff and does
        // nothing without it — a user who widens a mono stem, hears combing, and goes looking for
        // the cure should find it in the group they are already in.
        makeFloat (kHintV1_5, "decorr", "Decorrelate", linearRange (0.0f, 1.0f), 0.0f)));

    // ── Solve ───────────────────────────────────────────────────────────────────
    // "dB/2x" rather than "dB/doubling": Logic truncates the unit field hard and the prose form
    // does not survive it (RESEARCH §3.2).
    layout.add (std::make_unique<juce::AudioProcessorParameterGroup> (
        "solve", "Solve", "|",
        // v1.3.0: rolloff max 6 → 12 dB/2x. The paper's 3-6 range maps to exponents a = 0.5-1.0 —
        // the gentle half of DBAP's useful range; over the default rig the max-to-min channel
        // spread only reached ~13 dB at R = 6. R = 12 (a ≈ 2) reaches ~25 dB: a real focus
        // control. Default 4.0 unchanged; < 1.3.0 presets re-mapped (÷3 in normalised terms) by
        // the editor's migration hook.
        makeFloat (kHintV1_0, "rolloff", "Rolloff", linearRange (3.0f, 12.0f), 4.0f, "dB/2x"),
        // v1.3.0: default 0.10 → 0.03. kBlurScale tripled (0.5 → 1.5, DbapSolver.h) so blur = 1 is
        // a true wash; 0.03 keeps the shipped default radius at ~0.36 m (was 0.40 m) — audibly the
        // same starting point. Concert Default (PresetPolicy.h) moves with it: that preset must
        // stay exactly the shipped defaults.
        makeFloat (kHintV1_0, "blur",    "Blur",    linearRange (0.0f, 1.0f), 0.03f)));

    // ── Weights ─────────────────────────────────────────────────────────────────
    // No label, deliberately: these are DBAP weights, not percentages of anything. Inventing
    // "norm" or "%" would misrepresent them.
    auto weights = std::make_unique<juce::AudioProcessorParameterGroup> ("weights", "Weights", "|");

    for (int i = 1; i <= ochan::kNumSpeakers; ++i)
        weights->addChild (makeFloat (kHintV1_0, "w" + juce::String (i),
                                      "Weight " + juce::String (i),
                                      linearRange (0.0f, 1.0f),
                                      1.0f));

    layout.add (std::move (weights));

    // ── Space ───────────────────────────────────────────────────────────────────
    layout.add (std::make_unique<juce::AudioProcessorParameterGroup> (
        "space", "Space", "|",
        makeFloat (kHintV1_0, "hullAtten", "Hull Atten", linearRange (0.0f, 3.0f), 1.0f, "dB/m"),
        makeFloat (kHintV1_0, "airAmount", "Air",        linearRange (0.0f, 1.0f), 0.35f)));

    // ── Output ──────────────────────────────────────────────────────────────────
    layout.add (std::make_unique<juce::AudioProcessorParameterGroup> (
        "output", "Output", "|",
        makeFloat (kHintV1_0, "outputGain", "Output", linearRange (-24.0f, 12.0f), 0.0f, "dB")));

    // ── Motion (v1.8.0) ──────────────────────────────────────────────────────────
    // Ten parameters. motionOn defaults OFF for the same reason decorr defaults 0: at 0 GainStage
    // takes the v1.7.0 branch verbatim and probe DC holds the render against the v1.7.0 digest.
    //
    // motionRate is THE FIRST NON-LINEAR RANGE (parameter-spec.md's "all skews linear" ends here):
    // 0.01-4 Hz with the centre at 0.3 Hz, because a slow orbit is the musical default and a
    // linear lane would spend 90% of its travel above 0.4 Hz. Sync choices are Free + O-Orbit's
    // fourteen divisions in oo::motion::kSyncMultipliers order. Seed steps by 1.
    juce::NormalisableRange<float> rateRange (0.01f, 4.0f);
    rateRange.setSkewForCentre (0.3f);

    layout.add (std::make_unique<juce::AudioProcessorParameterGroup> (
        "motion", "Motion", "|",
        makeBool   (kHintV1_8, "motionOn",     "Motion On",    false),
        makeChoice (kHintV1_8, "motionPath",   "Motion Path",
                    juce::StringArray { "Orbit", "Figure-8", "Sweep", "Drift", "Pendulum", "Spiral" }, 0),
        makeChoice (kHintV1_8, "motionSync",   "Motion Sync",
                    juce::StringArray { "Free", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D",
                                        "1/4T", "1/4", "1/4D", "1/2", "1/2D", "1 Bar", "2 Bars",
                                        "4 Bars" }, 0),
        makeFloat  (kHintV1_8, "motionRate",   "Motion Rate",   rateRange,                          0.1f, "Hz"),
        makeFloat  (kHintV1_8, "motionSize",   "Motion Size",   linearRange (0.0f, 24.0f),          6.0f, "m"),
        makeFloat  (kHintV1_8, "motionRatio",  "Motion Ratio",  linearRange (0.0f, 1.0f),           1.0f),
        makeFloat  (kHintV1_8, "motionAngle",  "Motion Angle",  linearRange (0.0f, 360.0f),         0.0f, "deg"),
        makeFloat  (kHintV1_8, "motionHeight", "Motion Height", linearRange (0.0f, 8.0f),           0.0f, "m"),
        makeFloat  (kHintV1_8, "motionPhase",  "Motion Phase",  linearRange (0.0f, 360.0f),         0.0f, "deg"),
        makeFloat  (kHintV1_8, "motionSeed",   "Motion Seed",   juce::NormalisableRange<float> (1.0f, 64.0f, 1.0f), 1.0f)));

    return layout;
}

//==============================================================================
OOctagonProcessor::OOctagonProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::mono(),          true)
                          .withOutput ("Output", juce::AudioChannelSet::create7point1(), true)),
      // State root is "OOctagon" — the architecture's identifier, NOT the sibling O-Orbit's
      // "OOrbitParams" idiom. Phase 2.1 attaches the VENUE child to this exact node; changing it
      // later orphans every saved session and every .venue file written in between.
      // THIS IDENTIFIER MUST NEVER CHANGE.
      apvts (*this, nullptr, juce::Identifier ("OOctagon"), createParameterLayout())
{
    // The atomics AND their declared defaults, in oo::params order, from one loop over one ID table.
    //
    // paramDefaults is READ OUT OF THE PARAMETER OBJECTS, never written out here (P17). A literal
    // table of 17 numbers would be a mirrored fixture that drifts silently the first time a range or
    // a default is edited, and the parameter-spec gate compares the parameters rather than this
    // array, so it would not notice.
    for (std::size_t k = 0; k < oo::params::kCount; ++k)
    {
        const auto* id = oo::params::id (static_cast<int> (k));

        paramPtr[k] = apvts.getRawParameterValue (id);

        auto* parameter = apvts.getParameter (id);

        jassert (paramPtr[k] != nullptr && parameter != nullptr);

        if (parameter != nullptr)
            paramDefaults[k] = parameter->convertFrom0to1 (parameter->getDefaultValue());
    }

    // Give apvts.state a VENUE child from birth, on the message thread, so that:
    //   - a session saved before prepareToPlay() still carries a complete room;
    //   - readVenueFromState() and writeToState() are never the ones mutating the ValueTree from
    //     whatever thread a host chooses to call prepareToPlay() on.
    // The venue is already at its §OQ4 defaults — VenueModel's constructor put it there.
    venue.writeToState (apvts.state);

    // FUNC-06 / N13 — THE SAME TREATMENT AT THE SAME SITE. `SCENES` gets its node from birth for
    // the identical reason `VENUE` does: a session saved before prepareToPlay() carries a complete,
    // self-describing tree. Without it the four slots would read as ABSENT rather than EMPTY, and
    // absent and empty are different things to a page that has to decide whether to disable a
    // control.
    sceneStore.writeToState (apvts.state);

    hull.build (venue.speakerPositions());

    publishSnapshot();
}

OOctagonProcessor::~OOctagonProcessor() = default;

//==============================================================================
bool OOctagonProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // This function is load-bearing well beyond its length: JUCE DERIVES the entire AU
    // channel-config set from it (RESEARCH F2) —
    //     AUChannelInfo = {(1,1),(1,2),(1,8),(2,1),(2,2),(2,8)}
    // and auval exercises all six. Widening or narrowing this predicate silently changes what
    // auval tests and what Logic offers.

    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in.isDisabled() || out.isDisabled())
        return false;

    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
        return false;

    // Real mode — the three 8-channel containers Logic exposes.
    //
    // Phase 4.1 (P91): THE SAME FUNCTION prepareToPlay() derives safeMode from. These two rules
    // were three literal comparisons here and three more there, and they are the same partition
    // stated twice — which is exactly the drift the extraction exists to make impossible. Route
    // both through oo::rig::isRealRig and the SAFE banner cannot fall out of step with what this
    // predicate admits. Behaviour is identical; the AUChannelInfo set JUCE derives is unchanged.
    if (oo::rig::isRealRig (out))
        return true;

    // SAFE mode — defined, non-destructive, clearly signposted. Load-bearing for AU (the (n,1) and
    // (n,2) configs above), not only for Standalone on a 2-channel interface (COMPAT-04).
    if (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        return true;

    // Everything else is rejected — including octagonal(), which JUCE offers as an 8-channel
    // candidate and which Logic ignores.
    return false;
}

//==============================================================================
void OOctagonProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // samplesPerBlock is NAMED AND FORWARDED as of Phase 2.3 (RESEARCH-2.3 Q4). The control grid is
    // still decoupled from the host's block size — nothing here keys off it — but
    // juce::dsp::FirstOrderTPTFilter::prepare takes a ProcessSpec, and fabricating a
    // maximumBlockSize would be a small lie in a codebase that has been careful not to tell any.
    // (The filter never reads that field; passing the real one costs an identifier and removes the
    // question.) The -Wunused-parameter workaround that used to live here is gone with it.
    //
    // NEVER call setLatencySamples() here — latency is zero and getLatencySamples() is
    // non-virtual in JUCE 8.
    //
    // Order is fixed: the venue defines the labels, and the map is built from the labels against
    // the layout the host has just negotiated.
    //
    // publish=false: rebuildChannelMap() on the next line publishes unconditionally, and two
    // publishes inside one processBlock read window overwrite the slot it is holding (WR-01).
    // Audio is suspended here so this pair was never the reachable one — suppressed for symmetry
    // with setStateInformation(), which is.
    readVenueFromState (false);
    rebuildChannelMap();

    preparedYet = true;

    // Phase 3.1 (P43). SAFE mode is the mono/stereo fold — defined, non-destructive, and signposted
    // by a banner in the editor. Derived HERE and nowhere else, because prepareToPlay() is already
    // the single site that knows what the host negotiated, and that keeps the derivation ADJACENT
    // to the isBusesLayoutSupported() rule above that admits those two sets in the first place.
    //
    // THE PREDICATE ITSELF now lives in Data/RigPolicy.h (Phase 4.1, P91) — unchanged, and moved
    // for one reason: reached only from here it can never be presented with a set
    // isBusesLayoutSupported() rejects, so this site can prove the WIRING (probe BM) but never the
    // FORM. Probe CO calls the free function directly and proves the form; neither alone is
    // non-vacuous. See the table in that header.
    //
    // It is written as the complement of the three REAL containers rather than as
    // "== mono || == stereo", so that a future fourth 8-channel container admitted by
    // isBusesLayoutSupported() DOES raise the banner — an unmapped rig folds and says so. The
    // rejected spelling is the one that would silently STOP raising it. (P90: this sentence stated
    // that outcome backwards until Phase 4.1, in the same inversion COMPAT-04 criterion 3 carried.)
    {
        const auto outSet = getBusesLayout().getMainOutputChannelSet();

        safeMode.store (! oo::rig::isRealRig (outSet), std::memory_order_release);
    }

    // FUNC-04. Its single initialisation site, beside the others, and it needs only the rate: both
    // of its clocks are counted in SAMPLES derived from it (P60), which is the only form probes BS
    // and BT can measure offline. A prepare() that arrives mid-ping restarts the ping's own state
    // and nothing else — the seventeen smoothers are untouched by anything in that class.
    verifyPing.prepare (sampleRate);

    // LAST, and that is the point (P23): the gain stage's initial solve must run against a PUBLISHED
    // snapshot and a BUILT map, or sample 0 of every render is solved against the wrong room.
    gainStage.prepare (sampleRate, samplesPerBlock, venuePublisher.read(), snapshotParameters());
}

void OOctagonProcessor::releaseResources()
{
    // Deliberately does NOT touch the gain stage. absoluteSampleCounter is reset in prepareToPlay()
    // and nowhere else (P23), and SmoothedValue::reset() is a STATE reset that would teleport all 17
    // gains — a second call site would be invisible to QUAL-03, because both block-size renders
    // would teleport identically (RESEARCH-2.2 H9).
}

//==============================================================================
oo::ParamSnapshot OOctagonProcessor::snapshotParameters() const noexcept
{
    oo::ParamSnapshot p {};

    for (std::size_t k = 0; k < oo::params::kCount; ++k)
    {
        const float raw = paramPtr[k]->load (std::memory_order_relaxed);

        // 17 branches per block. The cost is nil; the failure it prevents is permanent silence.
        p[k] = std::isfinite (raw) ? raw : paramDefaults[k];
    }

    return p;
}

//==============================================================================
bool OOctagonProcessor::mappedOutputAvailable (int numOutputChannels) const noexcept
{
    return numOutputChannels == ochan::kNumSpeakers
        && ! mapInvalid.load (std::memory_order_acquire);
}

//==============================================================================
void OOctagonProcessor::rebuildChannelMap()
{
    const auto outSet = getBusesLayout().getMainOutputChannelSet();

   #if JUCE_DEBUG
    // Layer 1 of the three-layer strategy, called here as well as from the unit target so that a
    // developer who never builds the tests still trips it on the first prepareToPlay().
    {
        juce::String whyNot;

        if (! ochan::verifyEnumBitOrder (outSet, &whyNot))
        {
            DBG ("O-Octagon channel-map Layer 1 FAILED: " << whyNot);
            jassertfalse;
        }
    }
   #endif

    // buildSpeakerToBuffer() leaves speakerToBuffer untouched unless it fully succeeds, so the
    // failure path below really does retain the LAST VALID map rather than half of a rejected one.
    //
    // The diagnosis is captured into a PLAIN MEMBER: this function and getStatus are both message
    // thread (P43 reused). It is stored on SUCCESS too — buildSpeakerToBuffer clears it up front —
    // so a resolved map cannot leave a stale reason behind the banner.
    const bool ok = ochan::buildSpeakerToBuffer (outSet, venue.labelTypes(), speakerToBuffer,
                                                 &mapDiagnosis);

    mapInvalid.store (! ok, std::memory_order_release);

    // Published either way: on failure the snapshot carries the retained map, and the audio thread
    // is stopped from using it by mappedOutputAvailable(), not by stale data.
    publishSnapshot();
}

void OOctagonProcessor::readVenueFromState (bool publish)
{
    // A missing OR partial VENUE node yields the §OQ4 defaults per attribute — never zeros, never
    // an error. Every session saved during Stage 1 takes exactly that path.
    venue.readFromState (apvts.state);

    hull.build (venue.speakerPositions());

    // ── WHY THE PUBLISH IS OPTIONAL (CODE_REVIEW WR-01) ───────────────────────────────────────
    //
    // VenueSnapshotPublisher is a 2-slot double buffer: publish() always writes `1 - activeSlot`
    // and nothing tracks which slot a READER currently holds. processBlock() binds the active slot
    // BY REFERENCE once per block and holds it for the whole callback. ONE publish inside that
    // window is safe — it writes the other slot. TWO are not: the second computes
    // `1 - (the slot the first just activated)` and lands squarely in the slot the audio thread is
    // reading. A genuine data race on ~276 bytes of non-atomic floats and ints.
    //
    // There were exactly two back-to-back publish pairs in the plugin, and both were
    // readVenueFromState() immediately followed by rebuildChannelMap():
    //
    //   prepareToPlay()        — host-guaranteed non-concurrent with processBlock. Harmless, and
    //                            suppressed anyway so the two sites read the same.
    //   setStateInformation()  — REACHABLE. A host preset switch or session restore with the
    //                            transport rolling runs this concurrently with processBlock, and
    //                            the two publishes are microseconds apart inside a ~10.7 ms block.
    //
    // Every other publisher publishes exactly once: the constructor, and applyVenueEdit(), whose
    // `if (preparedYet) rebuildChannelMap(); else publishSnapshot();` is an either/or by
    // construction. The UI cannot drive a double publish — venue.js has ONE setVenue call site and
    // it commits on blur/Enter, not on drag; the Room-plan puck writes APVTS parameters and never
    // the venue.
    //
    // So suppressing the first of the pair closes every reachable path. It does NOT make the
    // publisher itself safe against a future second caller. The durable fix is a seqlock (writer
    // bumps an odd/even version around the slot copy; the audio thread copies the trivially-
    // copyable snapshot into a local and retries on a changed or odd version) or a 3-slot buffer
    // where the audio thread publishes its claimed index. Both are hardening, deliberately NOT
    // taken here — they change the audio thread's read path, and no second caller justifies that
    // risk today. If one is ever added, add the seqlock with it.
    if (publish)
        publishSnapshot();
}

void OOctagonProcessor::publishSnapshot()
{
    // ── P29 / RESEARCH-2.3 H5 — THE VENUE VALUES ARE SANITISED HERE, AND ONLY HERE ────────────
    //
    // The 17 musical parameters have been guarded at ingestion since 2.2 (snapshotParameters, P17).
    // The 42 VENUE values were not: readFloat() has no clamp, no jlimit and no isfinite anywhere in
    // VenueModel.cpp, and venue values arrive from setStateInformation (host session data) and,
    // from Stage 3.2, from a UI where a user types coordinates.
    //
    // publishSnapshot() is the single funnel for everything the audio thread ever reads about the
    // room, which makes it the exact analogue of snapshotParameters(): one site, stated once,
    // structurally impossible to bypass.
    //
    // ── TWO GUARDS WITH DIFFERENT PROVENANCE, AND THE SPLIT IS STATED RATHER THAN BLURRED ─────
    //
    //   THE TRIM GUARD IS 2.3's. FUNC-07's multiply is what arms it. trimDb = 1e30 converts to
    //   +inf, and v_i is EXACTLY 0.0f whenever w_i == 0 (DSP-05/1), so `v_i * trimLin` is
    //   0.0f * inf = NaN → setTargetValue(NaN) → the SmoothedValue latches → PERMANENT SILENCE.
    //   That is RESEARCH-2.2's H2 latch reached through a new door. (A NaN trimDb happens to be
    //   benign — NaN > -100.0f is false, so decibelsToGain returns 0.0f — but that is luck, one
    //   refactor from changing, and it is not relied on: NaN is replaced before conversion.)
    //
    //   THE POSITION GUARD CLOSES A PRE-EXISTING 2.2 HAZARD and is a recorded scope addition
    //   (SUMMARY-2.3). A NaN speaker coordinate already reaches dbap::solve today, where
    //   `dRaw < kMinDistance` is FALSE for NaN and `denom < kDenomEpsilon` is FALSE for NaN, so it
    //   falls straight through to the same setTargetValue. Same loop, same site, no extra cost.
    //
    // Fallbacks are READ FROM A DEFAULT-CONSTRUCTED MODEL, not transcribed as literals — a hand-
    // written table of §OQ4 defaults here is pattern_test_fixture_mirrors_drift_silently with the
    // drift pointing at the audio thread.
    static const oo::VenueModel defaults;

    const auto sane = [] (float v, float fallback) noexcept
    {
        return std::isfinite (v) ? v : fallback;
    };

    const auto saneVec3 = [&sane] (oo::Vec3 v, oo::Vec3 fallback) noexcept
    {
        return oo::Vec3 { sane (v.x, fallback.x), sane (v.y, fallback.y), sane (v.z, fallback.z) };
    };

    oo::VenueSnapshot snapshot;

    snapshot.speakerToBuffer = speakerToBuffer;

    const auto positions = venue.speakerPositions();

    for (int i = 0; i < ochan::kNumSpeakers; ++i)
    {
        snapshot.spk[(size_t) i] = saneVec3 (positions[(size_t) i], defaults.speaker (i));

        // Clamped BEFORE the conversion, so the +inf can never be constructed in the first place.
        // trimDb == 0 still yields decibelsToGain(0.0f) == 1.0f exactly, so the default patch and
        // FUNC-07/4's preset-load bit-identity are untouched.
        const float trimDb = juce::jlimit (-kVenueTrimClampDb, kVenueTrimClampDb,
                                           sane (venue.trimDb (i), 0.0f));

        snapshot.trimLin[(size_t) i] = juce::Decibels::decibelsToGain (trimDb);

        // v1.4.0 — THE DELAY RAIL, AT THE SAME SITE AND FOR THE SAME REASON.
        //
        // The lower bound is not decoration. GainStage sizes its lines for kVenueDelayClampMs and
        // pops at the smoothed value; a NEGATIVE delay would index BEFORE the write head, which
        // juce::dsp::DelayLine does not range-check, and a 1e30 would exceed the allocated line.
        // Both are reachable from a hand-edited .venue or a session written by a build that railed
        // differently, so neither is guarded by "the UI would not send it".
        //
        // sane() FIRST, exactly as the trim does it: NaN survives jlimit unchanged (both of
        // jlimit's comparisons are false for NaN, so it returns the NaN), and a NaN delay would
        // reach setTargetValue and latch the smoother — RESEARCH-2.2's H2 latch reached through a
        // third door.
        snapshot.delayMs[(size_t) i] = juce::jlimit (0.0f, kVenueDelayClampMs,
                                                     sane (venue.delayMs (i), 0.0f));

        // The hull is DERIVED from the positions, so it is downstream of the same poison and is
        // guarded at the same site. A collapsed hull is handled by ConvexHull2D's degeneracy paths
        // (probe L); a NaN one is not handled anywhere.
        const auto hp = hull.getHullPoint (i);

        snapshot.hullPts[(size_t) i] = { sane (hp.x, 0.0f), sane (hp.y, 0.0f) };
    }

    snapshot.hullCount    = hull.getNumHullPoints();
    snapshot.hullEpsCross = sane (hull.getCrossEpsilon(), 0.0f);

    snapshot.centroid  = saneVec3 (venue.centroid(),  defaults.centroid());
    snapshot.rigScale  = sane (venue.rigScale(),      defaults.rigScale());
    snapshot.bbMinX    = sane (venue.bbMinX(),        defaults.bbMinX());
    snapshot.bbMaxX    = sane (venue.bbMaxX(),        defaults.bbMaxX());
    snapshot.bbMinY    = sane (venue.bbMinY(),        defaults.bbMinY());
    snapshot.bbMaxY    = sane (venue.bbMaxY(),        defaults.bbMaxY());
    snapshot.rakeFront = sane (venue.rakeFront(),     defaults.rakeFront());
    snapshot.rakeRear  = sane (venue.rakeRear(),      defaults.rakeRear());

    // ── v1.7.0 — THE MONITOR PAIR, RESOLVED HERE AND ONLY HERE ────────────────────────────────
    //
    // On the MESSAGE THREAD, beside the speaker map it inverts, so the audio thread performs no
    // channel lookup of its own and there remains exactly one expression in this plugin that turns
    // a speaker into an output channel.
    //
    // Left { -1, -1 } on failure, which GainStage treats as REFUSE. Never a fallback to slots 0
    // and 1: that would be correct for the shipped container and silently wrong for a re-wired
    // rig, which is the precise failure mode ChannelMap.h exists to make impossible.
    if (! ochan::resolveMonitorSlots (getBusesLayout().getMainOutputChannelSet(),
                                      speakerToBuffer, snapshot.monitorSlot))
        snapshot.monitorSlot = { -1, -1 };

    venuePublisher.publish (snapshot);
}

void OOctagonProcessor::applyVenueEdit (const oo::VenueModel& newVenue)
{
    venue = newVenue;
    venue.writeToState (apvts.state);

    hull.build (venue.speakerPositions());

    // A label edit can change the map, so the single construction site runs — and publishes.
    if (preparedYet)
        rebuildChannelMap();
    else
        publishSnapshot();
}

bool OOctagonProcessor::applyVenueEditChecked (const oo::VenueModel& newVenue,
                                               ochan::MapDiagnosis* whyNot)
{
    if (whyNot != nullptr)
        *whyNot = {};

    // ── VALIDATE INTO A SCRATCH ARRAY, THROUGH THE BACKSTOP'S OWN PREDICATE (P52) ────────────────
    //
    // Not a second implementation of "is this label set valid" — literally the function the audio
    // path's backstop calls. Guard and backstop therefore cannot drift, which matters because the
    // failure they both exist to prevent is AUDIBLE: mappedOutputAvailable() false collapses seven
    // speakers onto the right input at unity (N8).
    //
    // `scratch` is discarded either way. On success applyVenueEdit() -> rebuildChannelMap() builds
    // into the REAL array, so there is still exactly one construction site for the live map.
    std::array<int, ochan::kNumSpeakers> scratch {};
    ochan::MapDiagnosis diagnosis {};

    const auto outSet = getBusesLayout().getMainOutputChannelSet();

    if (! ochan::buildSpeakerToBuffer (outSet, newVenue.labelTypes(), scratch, &diagnosis))
    {
        // NOTHING is applied: not the coordinates, not the trims, not the rake. A partial apply is
        // exactly the "venue that half-applied" this guard exists to make impossible.
        if (whyNot != nullptr)
            *whyNot = diagnosis;

        return false;
    }

    applyVenueEdit (newVenue);
    return true;
}

//==============================================================================
bool OOctagonProcessor::startVerifyPing (int speakerOrAuto)
{
    // THE PRECONDITION (Q5 / P60). getTotalNumOutputChannels() is the accessor that LIES under the
    // F3 hazard — a 3-7 channel device leaves it reporting 8 while the buffer is narrower — so this
    // check alone is not sufficient, and it is not asked to be: the audio thread carries the second
    // half, aborting a running ping the moment `mapped` goes false with the real buffer width in
    // hand. The two arrive at the same rule from opposite directions, which is why both exist.
    if (! mappedOutputAvailable (getTotalNumOutputChannels()))
        return false;

    // MUTUALLY EXCLUSIVE WITH THE MONITOR, and setMonitorArmed() carries the reciprocal. Not
    // tidiness: the ping names a PHYSICAL speaker, and folding it to headphones would answer a
    // question about wiring with a signal that has left the wiring. Whichever the operator asked
    // for last wins, which is the least surprising rule available.
    monitorArmed.store (false, std::memory_order_release);

    verifyPing.start (speakerOrAuto);
    return true;
}

void OOctagonProcessor::stopVerifyPing()
{
    verifyPing.stop();
}

//==============================================================================
bool OOctagonProcessor::setMonitorArmed (bool shouldArm)
{
    // Disarming ALWAYS succeeds and is never gated on a precondition. A monitor that could refuse
    // to switch off because the rig had meanwhile gone unmapped would be exactly backwards.
    if (! shouldArm)
    {
        monitorArmed.store (false, std::memory_order_release);
        return true;
    }

    // SAFE mode is refused, and the reason is not that it would misbehave — it is that it would be
    // MEANINGLESS. SAFE mode already writes a stereo fold of the dry input; there are no eight
    // solved speaker feeds to fold FROM, and there is no monitor pair to fold INTO. Same shape as
    // startVerifyPing()'s precondition, and the audio thread re-checks it with the true buffer
    // width in hand because F3 can flip the mode between blocks.
    if (! mappedOutputAvailable (getTotalNumOutputChannels()))
        return false;

    // An unresolved pair means the negotiated set has no left/right, or the map does not reach
    // them. Refuse rather than guess — see resolveMonitorSlots().
    if (venuePublisher.read().monitorSlot[0] < 0)
        return false;

    // See startVerifyPing() for why these two cannot both be up.
    verifyPing.abort();

    monitorArmed.store (true, std::memory_order_release);
    return true;
}

//==============================================================================
void OOctagonProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    // D11's second stop. JUCE's default passthrough would silence the ping INCIDENTALLY — it simply
    // never calls processBlock — but it would leave the state machine running and the 120 s latch
    // counting, so un-bypassing would resume a ping mid-cycle from a clock the operator cannot see.
    // abort() touches only atomics, which is what makes it safe to call from here.
    verifyPing.abort();

    // v1.7.0. D11's rule extended to the monitor, and the argument transfers exactly: a bypassed
    // plugin that still folds is confusing to debug on a stage, because the first instinct IS to
    // bypass. Guard 4 of 4.
    monitorArmed.store (false, std::memory_order_release);

    juce::AudioProcessor::processBlockBypassed (buffer, midiMessages);
}

void OOctagonProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Phase 2.2: the eight lanes finally carry DIFFERENT signal. Everything that decides what goes
    // where lives in GainStage; this function's whole job is to hand it four facts and get out of
    // the way. No allocation, no lock, no file I/O below this line (PERF-01).

    // Bound by buffer.getNumChannels(), NOT 8 and NOT getTotalNumOutputChannels(). On a 3-7 output
    // device canonicalChannelSet(n) is rejected, Debug asserts, and Release KEEPS the 7.1 layout
    // while the buffer holds only n channels (RESEARCH F3). getTotalNumOutputChannels() returns 8
    // in exactly that state — it is the accessor that lies.
    const int numOut = buffer.getNumChannels();
    const int numIn  = juce::jmin (getTotalNumInputChannels(), numOut);

    // The G1 branch, stated in exactly one place (mappedOutputAvailable) and PASSED IN. GainStage
    // calls neither this helper's inputs nor getTotalNumOutputChannels() — it is handed the answer
    // so that G1 exists in one place rather than two (P24).
    //
    // The count is fixed for the duration of a block, so the mode cannot change mid-block. It CAN
    // change between blocks (probe S), which is why the inner loop advances all 17 smoothers in both
    // modes.
    const bool mapped = mappedOutputAvailable (numOut);

    // ── D11's FIFTH STOP (Q5 / P60) ───────────────────────────────────────────────────────────
    //
    // The precondition in startVerifyPing() arrives at this rule from the message thread, where
    // getTotalNumOutputChannels() can lie under F3. THIS is the same rule with the real buffer width
    // in hand, and the flip it catches can happen BETWEEN BLOCKS with no intervening
    // prepareToPlay(). Pinging "speaker 5" on a stereo fold names a speaker that does not exist
    // during the one procedure whose purpose is confirming that it does — R1 reproduced inside its
    // own diagnostic tool. abort() is atomics-only and therefore RT-safe.
    if (! mapped && verifyPing.isActive())
        verifyPing.abort();

    // Acquired ONCE per block and held. Re-reading mid-block would let a venue edit land between two
    // speakers of one gain vector.
    const auto& snapshot = venuePublisher.read();

    // The §3.4.3 convention (0.5·L / 0.5·R) supersedes Phase 2.1's 1/numIn averaging. For both mono
    // and stereo input the result is identical to what 2.1 produced — this is not a level change.
    //
    // The ping is PASSED IN, never reached for: GainStage does not ask the processor anything (P24),
    // and it does not decide when the ping runs. Null in every render-harness call site that does
    // not want one, which is all of them except probes BQ-BU.
    // ══ v1.7.0 — THE PRIMARY CONSTRAINT: THE MONITOR MUST NOT CONTAMINATE A RENDER ═══════════
    //
    // GUARD 3 OF 4. isNonRealtime() is set by the wrapper for an offline bounce — Logic's Bounce
    // and Bounce in Place both take that path. The fold is bypassed STRUCTURALLY there: not
    // attenuated, not faded out, never ENGAGED at all, so MonitorFold::isRunning() stays false and
    // GainStage does not clock one sample of it. Probe CZ asserts the offline render is
    // BIT-IDENTICAL to a never-armed one, with a realtime arm as its negative control.
    //
    // THIS IS NOT THE STRONGEST GUARD AND MUST NOT BE READ AS SUFFICIENT. It does not fire for a
    // REALTIME bounce, where the host is genuinely running in real time and is right to say so.
    // What covers that case is guard 2 — the arm is neither a parameter nor persisted, so no
    // session can come back armed (see getStateInformation) — backed by the banner, which is the
    // only defence left once someone arms and realtime-bounces inside one sitting.
    //
    // `mapped` is in the conjunction for the same reason the ping's abort is: F3 can flip the mode
    // BETWEEN blocks with no intervening prepareToPlay(), and a monitor pair resolved against the
    // eight-channel map means nothing on a stereo fold.
    const bool armed = monitorArmed.load (std::memory_order_acquire);

    const bool monitorOn = armed && mapped && ! isNonRealtime();

    // Published for the banner: armed, but not folding. The operator is told WHY.
    monitorSuppressed.store (armed && ! monitorOn, std::memory_order_release);

    // ══ v1.8.0 — THE HOST CLOCK, READ ONCE PER BLOCK AND HANDED IN (P24) ════════════════════
    //
    // Exactly O-Orbit's read (PluginProcessor.cpp:561-574): getPosition() -> bpm, ppq, playing.
    // ppqValid means the host SUPPLIED a position, which is a different fact from "playing" — a
    // stopped host with a PPQ holds motion at that PPQ (where playback resumes); one with no PPQ
    // free-runs (MotionClock.h Q7). Every accessor here is a value read; nothing allocates.
    //
    // NO isSafeMode() GATE (RESEARCH Q3): the unmapped renderChunk branch never reads a position,
    // so motion is inaudible in SAFE mode by construction and the map still animates.
    oo::motion::HostClock clock;

    if (auto* playHead = getPlayHead())
    {
        if (const auto pos = playHead->getPosition())
        {
            if (const auto bpm = pos->getBpm())
                clock.bpm = *bpm;

            if (const auto ppq = pos->getPpqPosition())
            {
                clock.ppq      = *ppq;
                clock.ppqValid = true;
            }

            clock.playing = pos->getIsPlaying();
        }
    }

    gainStage.process (buffer, numIn, numOut, mapped, snapshot, snapshotParameters(), &verifyPing,
                       monitorOn, &clock);

    // ══ UI-03 — THE METERS. THE LAST STATEMENT IN processBlock, AND THAT IS THE POINT ═════════
    //
    // ── WHAT IS MEASURED: THE WRITTEN BUFFER, POST-MAP AND POST-TRIM ─────────────────────────
    // Not `v_i`, and the distinction is the whole requirement. §R7 names UI-03 a second human line
    // of defence on R1 — the channel map — so a meter driven by the SOLVE would light correctly
    // under a bypassed map and report a rig that is working while the hall hears the wrong
    // speakers. That is the exact NC3 failure caught at Phase 2.2, and NC4 reproduces it here.
    //
    // ── WHY IT SITS AFTER THE PING, NOT BEFORE ───────────────────────────────────────────────
    // VerifyPing is a POST-WRITE OVERWRITE of the eight mapped pointers. Metering before it would
    // show the programme material while the hall hears the ping, and UI-03 criterion 2's
    // cross-check — step the ping 1 → 8 and watch the matching indicator light — would be
    // impossible to make at all. Probe CM is that cross-check, on a NON-IDENTITY map.
    //
    // ── snapshot.speakerToBuffer, NEVER THE PROCESSOR MEMBER ─────────────────────────────────
    // The block was RENDERED against this snapshot, and a venue edit can publish a new one between
    // the render and this loop. Reading the member here would attribute this block's audio to next
    // block's map — a one-block mis-lighting on every venue edit, which looks exactly like a
    // flicker and is actually the bug UI-03 exists to detect.
    //
    // ── IDENTITY ATTRIBUTION WHEN UNMAPPED IS CORRECT AND MUST NOT BE "FIXED" ────────────────
    // Under mapInvalid, GainStage's else arm writes `out[ch][n] = ch == 0 ? sL : sR`, so the meters
    // show speaker 1 lit from L and speakers 2-8 all lit from R. THAT IS N8's FOLD BEING VISIBLE,
    // which is the entire point of metering the output rather than the intent. Any 3.3 assertion
    // about what an invalid map RETAINS is made against the snapshot, never against this buffer.
    //
    // getMagnitude resolves to FloatVectorOperations::findMinAndMax on a raw pointer — no
    // allocation, no lock. Probe CN re-runs AO with this loop live and still reads 0 allocations.
    for (int i = 0; i < ochan::kNumSpeakers; ++i)
    {
        const int ch = mapped ? snapshot.speakerToBuffer[static_cast<std::size_t> (i)] : i;

        if (ch >= numOut)
            continue;

        const float pk = buffer.getMagnitude (ch, 0, buffer.getNumSamples());
        auto& slot = meterPeak[static_cast<std::size_t> (i)];

        if (pk > slot.load (std::memory_order_relaxed))
            slot.store (pk, std::memory_order_relaxed);
    }
}

//==============================================================================
std::array<float, ochan::kNumSpeakers> OOctagonProcessor::readAndZeroMeters() noexcept
{
    std::array<float, ochan::kNumSpeakers> out {};

    for (int i = 0; i < ochan::kNumSpeakers; ++i)
        out[static_cast<std::size_t> (i)] =
            meterPeak[static_cast<std::size_t> (i)].exchange (0.0f, std::memory_order_relaxed);

    return out;
}

//==============================================================================
void OOctagonProcessor::captureScene (int slot)
{
    if (slot < 0 || slot >= oo::SceneStore::kNumSlots)
        return;

    // The LIVE parameter values, read through the same paramPtr table the control block reads —
    // not through a second lookup, and not from the editor's cached echo. What is captured is what
    // the plugin is currently doing.
    std::array<float, oo::SceneStore::kNumSpeakers> w {};

    for (int i = 0; i < oo::SceneStore::kNumSpeakers; ++i)
    {
        const float raw = paramPtr[static_cast<std::size_t> (oo::params::w1 + i)]
                              ->load (std::memory_order_relaxed);

        // SceneStore::capture sanitises again; doing it here too is not redundancy for its own
        // sake — a NaN that reached the store would be written into apvts.state and would then
        // arrive back through setStateInformation on every future session load.
        w[static_cast<std::size_t> (i)] = std::isfinite (raw) ? raw : 0.0f;
    }

    sceneStore.capture (slot, w);
    commitScenes();
}

//==============================================================================
void OOctagonProcessor::commitScenes()
{
    sceneStore.writeToState (apvts.state);
    ++scenesGeneration;
}

//==============================================================================
bool OOctagonProcessor::applySceneWeights (
    const std::array<float, oo::SceneStore::kNumSpeakers>& weights)
{
    // ══ THE THIRD AND FINAL GESTURE-BRACKET SITE (D18 / P78) ══════════════════════════════════
    //
    // begin -> setValueNotifyingHost -> end, on EACH of w1..w8. setValueNotifyingHost is setValue
    // + sendValueChangedMessageToListeners AND NOTHING ELSE; the wrappers turn that into a bare
    // kAudioUnitEvent_ParameterValueChange (AU_1.mm:1341-1360) and a bare paramChanged
    // (VST3.cpp:1498-1501). Without the brackets, Logic with a lane in Latch or Touch MOVES THE
    // SOUND AND DOES NOT RECORD IT — and nothing in build, auval or pluginval can see it.
    //
    // ONE FUNCTION, ONE CALL SITE. The alternative — eight SliderState writes from JS — would work
    // and would scatter D18's obligation across 24 bridge messages, where no single grep confirms
    // it. This is the same shape PluginEditor.cpp's loadPreset uses for seventeen parameters.
    //
    // THE PARAMETER ECHO REPAINTS THE PAGE FOR FREE: WebSliderParameterAttachment listens to the
    // PARAMETER, so this write moves the eight in-plan weight cells with no extra plumbing.
    std::array<juce::RangedAudioParameter*, oo::SceneStore::kNumSpeakers> params {};

    for (int i = 0; i < oo::SceneStore::kNumSpeakers; ++i)
        params[static_cast<std::size_t> (i)] =
            apvts.getParameter (juce::String (oo::params::id (oo::params::w1 + i)));

    for (auto* param : params)
        if (param != nullptr)
            param->beginChangeGesture();

    for (int i = 0; i < oo::SceneStore::kNumSpeakers; ++i)
        if (auto* param = params[static_cast<std::size_t> (i)])
            param->setValueNotifyingHost (
                param->convertTo0to1 (weights[static_cast<std::size_t> (i)]));

    // CLOSED ON BOTH PATHS, the discipline loadPreset established at 3.2: an interrupted write that
    // never closed would leave the host in an open automation-write region on all eight.
    for (auto* param : params)
        if (param != nullptr)
            param->endChangeGesture();

    return true;
}

//==============================================================================
// ── THE PRESET ROUTE TO `SCENES`, AND TO NOTHING ELSE (D17 / P80) ────────────────────────────
//
// `applyPresetJson` iterates `processor.getParameters()` only and can never walk `apvts.state`'s
// children, which is why FUNC-05 holds BY CONSTRUCTION. The single exception is
// `setCustomStateCallbacks`, and these two functions are the whole of what it can reach: four
// slots of eight weights. `VENUE` is not representable through them, so a preset still cannot
// touch the 42 measured values however it is authored — and probe CL re-measures that rather than
// inheriting 3.2's result, because the tree shape has changed underneath the guarantee.

juce::var OOctagonProcessor::scenesToVar() const
{
    juce::Array<juce::var> slots;
    slots.ensureStorageAllocated (oo::SceneStore::kNumSlots);

    for (int s = 0; s < oo::SceneStore::kNumSlots; ++s)
    {
        const auto w = sceneStore.weights (s);

        juce::Array<juce::var> weights;
        weights.ensureStorageAllocated (oo::SceneStore::kNumSpeakers);

        for (int i = 0; i < oo::SceneStore::kNumSpeakers; ++i)
            weights.add (w[static_cast<std::size_t> (i)]);

        auto* entry = new juce::DynamicObject();
        entry->setProperty ("occupied", sceneStore.isOccupied (s));
        entry->setProperty ("w",        juce::var (weights));

        slots.add (juce::var (entry));
    }

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("slots", juce::var (slots));

    return juce::var (obj);
}

void OOctagonProcessor::scenesFromVar (const juce::var& payload)
{
    auto* obj = payload.getDynamicObject();

    if (obj == nullptr)
        return;

    const auto  slotsVar = obj->getProperty ("slots");
    const auto* arr      = slotsVar.getArray();

    if (arr == nullptr)
        return;

    for (int s = 0; s < oo::SceneStore::kNumSlots && s < arr->size(); ++s)
    {
        auto* entry = (*arr)[s].getDynamicObject();

        if (entry == nullptr || ! static_cast<bool> (entry->getProperty ("occupied")))
            continue;

        const auto  wVar = entry->getProperty ("w");
        const auto* wArr = wVar.getArray();

        if (wArr == nullptr || wArr->size() != oo::SceneStore::kNumSpeakers)
            continue;

        std::array<float, oo::SceneStore::kNumSpeakers> w {};

        for (int i = 0; i < oo::SceneStore::kNumSpeakers; ++i)
            w[static_cast<std::size_t> (i)] =
                static_cast<float> (static_cast<double> ((*wArr)[i]));

        // capture() sanitises and clamps. A preset authored by hand, or written by an older build,
        // is untrusted input on exactly the same footing as a typed venue coordinate.
        sceneStore.capture (s, w);
    }

    commitScenes();
}

//==============================================================================
bool OOctagonProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OOctagonProcessor::createEditor()
{
    // ── Phase 3.1: THE ARMS NOW DIVERGE. This is the phase the #if was written for ─────────────
    // It went in at Stage 1 while provably inert (G8) precisely so that the swap could happen here
    // without touching the render-harness target: that target compiles this TU with
    // JUCE_WEB_BROWSER=0, under which WebBrowserComponent's types do not exist. Added AFTER the
    // swap instead, the guard would have been a build break in a target nobody is looking at
    // (pattern_render_harness_breaks_on_webview_editor).
    //
    // The Stage-1 comment said of the generic editor "It is deleted at Phase 3.1; nothing may come
    // to depend on it." That is honoured for the plugin path and CORRECTED for the harness path:
    // it is not deleted, it is DEMOTED to the #else arm, where 32 harness probes need this function
    // to return something. COMPAT-04's Standalone eyeball no longer needs it either — the
    // Standalone build takes the #if arm and gets the real WebView UI.
   #if JUCE_WEB_BROWSER
    return new OctagonEditor (*this);
   #else
    return new juce::GenericAudioProcessorEditor (*this);
   #endif
}

//==============================================================================
const juce::String OOctagonProcessor::getName() const       { return "O-Octagon"; }

bool OOctagonProcessor::acceptsMidi() const                 { return false; }
bool OOctagonProcessor::producesMidi() const                { return false; }
bool OOctagonProcessor::isMidiEffect() const                { return false; }
double OOctagonProcessor::getTailLengthSeconds() const      { return 0.0; }

int OOctagonProcessor::getNumPrograms()                     { return 1; }
int OOctagonProcessor::getCurrentProgram()                  { return 0; }
void OOctagonProcessor::setCurrentProgram (int)             {}
const juce::String OOctagonProcessor::getProgramName (int)  { return {}; }
void OOctagonProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void OOctagonProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // This code is written once and does NOT change when Phase 2.1 adds the VENUE child — the
    // child rides along inside copyState() automatically. That is the whole reason it is written
    // now rather than at 2.1.
    auto state = apvts.copyState();

    if (auto xml = state.createXml())
    {
        // v1.2.0 — hover-help preference rides the session as a root XML attribute, NOT a
        // ValueTree property: the ValueTree XML round-trip rebuilds properties as strings, so an
        // isBool() guard on restore would never fire (critical_valuetree_xml_roundtrip_loses_type).
        // getBoolAttribute below sidesteps that class of bug entirely.
        xml->setAttribute ("tooltipsEnabled", tooltipsEnabled.load (std::memory_order_acquire));

        // v1.6.0 — the hover-help LANGUAGE rides the same root attribute idiom, and for the same
        // reason. Persisted as the language CODE, not the index: the index is an implementation
        // detail of the audio-safe std::atomic<int>, and a session written today has to still
        // mean "French" if the codec ever gains a third entry.
        xml->setAttribute ("uiLanguage", languageCode (uiLanguage.load (std::memory_order_acquire)));

        // ── v1.7.0 — monitorArmed IS DELIBERATELY ABSENT FROM THIS FUNCTION ───────────────────
        //
        // GUARD 2 OF 4, and the only one that covers the case isNonRealtime() cannot: a REALTIME
        // bounce. The two attributes directly above are the contrast that makes this readable —
        // tooltipsEnabled and uiLanguage are non-parameter UI booleans that SHOULD ride the
        // session, and the monitor arm is a non-parameter UI boolean that must NOT.
        //
        // If it were persisted here, a session could reopen ARMED. A realtime bounce would then
        // carry a headphone fold into the delivered file with six of eight channels silent, and
        // nothing in the render would say so. Not writing it makes that state unreachable across
        // a reload, which is a structural property rather than a warning anyone has to read.
        //
        // IF YOU ARE ADDING PERSISTENCE HERE BECAUSE RE-ARMING EACH SESSION IS ANNOYING, YOU ARE
        // DELETING THE FEATURE'S PRIMARY SAFETY PROPERTY. It is annoying on purpose. SpatGRIS,
        // L-ISA and SPAT Revolution all treat monitor mode as transport state for this reason.

        copyXmlToBinary (*xml, destData);
    }
}

void OOctagonProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

        // v1.2.0 — pre-1.2.0 sessions have no attribute, so the default (OFF) stands. The editor
        // PULLS this via the getTooltipsEnabled native fn at page init rather than being pushed —
        // a push from here fires before the page module has evaluated, so the preference would
        // silently never arrive (pattern_webview_one_shot_state_push_stale_on_preset_load).
        if (xml->hasAttribute ("tooltipsEnabled"))
            tooltipsEnabled.store (xml->getBoolAttribute ("tooltipsEnabled"),
                                   std::memory_order_release);

        // v1.6.0 — same shape for the language. Pre-1.6.0 sessions have no attribute, so the
        // default (English) stands. languageIndex() clamps anything that is not "fr" to 0, so a
        // hand-edited or corrupt value degrades to English rather than reaching storage
        // unvalidated. The editor PULLS this via getUiLanguage at page init, never a push.
        if (xml->hasAttribute ("uiLanguage"))
            uiLanguage.store (languageIndex (xml->getStringAttribute ("uiLanguage")),
                              std::memory_order_release);
    }

    // ── ORDERING HAZARD (ARCHITECTURE §4.1). This sequence is not interchangeable ───────────────
    //
    // 1. replaceState() above swaps in the restored tree, VENUE child and all.
    // 2. readVenueFromState() re-derives the geometry and the hull from whatever that tree carries.
    //    A session written during Stage 1 has NO VENUE child; it restores to the §OQ4 defaults
    //    silently, per attribute, and that is the common case for every project saved so far.
    // 3. Only then can the map be rebuilt, because the labels it resolves come from the venue.
    //
    // THE PUBLISH IS SUPPRESSED ONLY WHEN THE REBUILD BELOW WILL RUN (WR-01). This call can be
    // concurrent with processBlock — a host preset switch or session restore while the transport
    // rolls — and the rebuildChannelMap() at the bottom of this function publishes too. Two
    // publishes inside one held read window race the audio thread. The condition MUST be the same
    // `preparedYet` that rebuild is gated on: a host that calls setStateInformation() before
    // prepareToPlay() skips the rebuild, and an unconditional `false` would then leave the
    // restored geometry unpublished until something else happened to publish it.
    readVenueFromState (! preparedYet);

    // Normalise the tree: a missing or partial VENUE node is written back complete, so the next
    // getStateInformation() is self-describing and a Stage-1 session is upgraded exactly once.
    venue.writeToState (apvts.state);

    // ── THE SECOND OF `VENUE`'s TWO POINTS, AND `SCENES` NEEDS BOTH (RESEARCH-3.3 N13) ─────────
    //
    // Restore, then normalise. Without the write-back, EVERY SESSION SAVED BEFORE PHASE 3.3
    // restores with no SCENES node at all and the four slots read as ABSENT rather than EMPTY —
    // silent, and only on upgrade, which is the failure mode this project keeps catching. With it,
    // an older session is upgraded exactly once and the next getStateInformation() is
    // self-describing. Probe CK drives a pre-3.3 session explicitly.
    //
    // Slots that were not in the restored tree come back EMPTY rather than defaulted. That is the
    // deliberate opposite of VenueModel's per-attribute §OQ4 fallback: a venue has right answers
    // for a missing value, whereas an invented scene would put an unmeasured gain vector one click
    // from the PA.
    sceneStore.readFromState (apvts.state);
    commitScenes();

    // The map is built from the NEGOTIATED layout, so it can only be built once the host has
    // negotiated one. A host that calls setStateInformation() before prepareToPlay() gets the
    // rebuild deferred to prepareToPlay() rather than a second construction site appearing here.
    //
    // This is a plain flag, not an AsyncUpdater: there is no queued apply that could stomp the
    // state we have just restored, and therefore no cancelPendingUpdate() obligation
    // (pattern_asyncupdater_guard_flag_needs_cancel).
    if (preparedYet)
        rebuildChannelMap();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OOctagonProcessor();
}
