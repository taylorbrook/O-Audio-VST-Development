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
/*
  ==============================================================================

    O-Octagon — Plugin Editor (implementation)

    18 WebSliderRelay bindings, THREE native functions, one WebBrowserComponent
    at a fixed 1100 x 720.

    ── MSVC HABITS ARE WRITTEN NOW, NOT FIXED AT PORT TIME ──────────────────
    Neither hazard has a call site in this file — the FileChooser work is 3.2 —
    but both are write-time habits: no non-static constexpr inside a lambda
    (C3493, critical_msvc_constexpr_lambda_capture), and no SafePointer(this)
    init-capture in a nested lambda
    (critical_msvc_safepointer_init_capture_nested_lambda). The two envelope
    constants below therefore live at namespace scope rather than inside the
    getVenueGeometry lambda that uses them.

    ── juce::String (const char*) IS ASCII-ONLY ─────────────────────────────
    It converts through CharPointer_ASCII (juce_String.cpp:307-308) and mangles
    any byte above 127 with no compiler warning; += / + / << go through
    CharPointer_UTF8 and are safe (juce_String.cpp:773-777). Every literal in
    this file is pure ASCII, and any text that is not must be built with << or
    wrapped in juce::CharPointer_UTF8
    (critical_juce_string_char_ctor_is_ascii_only).

  ==============================================================================
*/

#include "PluginEditor.h"
#include "UIBinaryData.h"   // distinct NAMESPACE — see CMakeLists juce_add_binary_data

// Phase 4.1 (P92, P93). The six factory definitions and the preserving load live in a header the
// RENDER HARNESS can reach — this TU is permanently excluded from that target, and a preset rule
// written here would be unreachable by any probe.
#include "Data/PresetPolicy.h"

// v1.1.0 — the measured CoreAudio 7.1 device order, for the speaker→output UI. Header-only and
// keyed on ChannelType; nothing in it touches a buffer index (R1 unchanged).
#include "Data/OutputOrder.h"

#include <array>
#include <cmath>

namespace
{
    // ── The room envelope (ARCHITECTURE section 6.2 / AD-14) ────────────────
    // DERIVED, not stored: speaker bounding box + 15 % margin per axis, with a
    // 1.0 m floor. That was an explicit decision not to add two more values to
    // the 42-value venue, and it is the reason the plan's proportions follow the
    // rig automatically.
    //
    // At namespace scope, not inside the lambda: a non-static constexpr declared
    // inside a lambda is MSVC error C3493.
    constexpr float kEnvelopeMarginFraction = 0.15f;
    constexpr float kEnvelopeMarginMinM     = 1.0f;

    // ConvexHull2D::classify() -> the string the UI puts on the glyph. Made a
    // first-class return value at Phase 2.1 (P11) precisely so the overlay and
    // the solver cannot disagree: this is the same hull the DBAP solve runs
    // against, not a second one drawn for display.
    const char* classificationName (oo::ConvexHull2D::Classification c) noexcept
    {
        switch (c)
        {
            case oo::ConvexHull2D::Classification::VERTEX:   return "VERTEX";
            case oo::ConvexHull2D::Classification::ON_EDGE:  return "ON_EDGE";
            case oo::ConvexHull2D::Classification::INTERIOR: return "INTERIOR";
        }

        return "INTERIOR";
    }

    // ochan::MapFailure -> the token the page keys its banner copy off (P54).
    //
    // ASCII IDENTIFIERS, deliberately: the human sentence is built in JS, where a
    // non-ASCII character costs nothing, rather than here, where
    // juce::String (const char*) converts through CharPointer_ASCII and mangles
    // every byte above 127 with no compiler warning at all
    // (critical_juce_string_char_ctor_is_ascii_only — a real defect at 3.1).
    const char* mapFailureName (ochan::MapFailure f) noexcept
    {
        switch (f)
        {
            case ochan::MapFailure::none:             return "none";
            case ochan::MapFailure::notEightChannels: return "notEightChannels";
            case ochan::MapFailure::labelNotInSet:    return "labelNotInSet";
            case ochan::MapFailure::duplicateLabel:   return "duplicateLabel";
        }

        return "none";
    }

    /** A finite float out of a var property, or the fallback.

        The page has already parsed and rejected non-numeric text (D12), so this is the SECOND
        line rather than the first — but publishSnapshot()'s sanitiser is downstream of the venue
        model, and a NaN that reached VenueModel would be stored before it was ever clamped.
    */
    float finiteOr (const juce::var& source, const char* key, float fallback) noexcept
    {
        auto* obj = source.getDynamicObject();

        if (obj == nullptr || ! obj->hasProperty (key))
            return fallback;

        const auto v = static_cast<float> (static_cast<double> (obj->getProperty (key)));
        return std::isfinite (v) ? v : fallback;
    }

    /** `{ ok, reason, speaker }` — the shape every write on this surface answers with. `speaker` is
        0-BASED, matching ochan::MapDiagnosis; the page adds one when it renders a row number. */
    juce::var makeResult (bool ok, const char* reason, int speakerIndex)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("ok",      ok);
        obj->setProperty ("reason",  juce::String (reason));
        obj->setProperty ("speaker", speakerIndex);
        return juce::var (obj);
    }

    /** base64, for the field grid's 8-bit payload (P73).

        THE PAYLOAD IS THE CONSTRAINT, NOT THE MATHS. 32 x 40 floats as JSON is 61 kB per recompute
        through a bridge that serialises every value; the same grid quantised to 8 bits and base64'd
        is 1.7 kB. juce::Base64::toBase64 is used rather than a hand-rolled encoder — the page
        decodes with the browser's own atob, and two hand-written codecs are two things that can
        disagree about padding.
    */
    juce::String toBase64 (const std::vector<std::uint8_t>& bytes)
    {
        juce::MemoryOutputStream out;

        if (! bytes.empty())
            juce::Base64::convertToBase64 (out, bytes.data(), bytes.size());

        return out.toString();
    }

    auto makeBinaryResource (const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*> (data);
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (bytes, bytes + size),
            juce::String (mimeType)
        };
    }
}

// ── Resource provider ───────────────────────────────────────────────────────
// The WKWebView / WebView2 resource callback receives a BARE PATH ("/",
// "/js/app.js", ...) — there is no scheme or host to strip, and stripping one
// would collapse every lookup to an empty string. Match by direct equality and
// never hard-code juce:// vs https://juce.backend
// (critical_webview_resource_provider_and_schemes).
//
// Every path here must appear in the juce_add_binary_data SOURCES list and in
// the HTML/JS that asks for it; section 9 of tests/ui_frontend_check.js closes
// that loop three ways, because a file embedded but not served, or served but
// not embedded, is a 404 that shows up as a missing panel and nothing else.
std::optional<juce::WebBrowserComponent::Resource>
OctagonEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on every text resource — index.html carries UTF-8 entities
    // (en-dashes, hair spaces, the multiplication sign in the metres readout)
    // that mojibake without it on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize,
                                   "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (UIBinaryData::styles_css, UIBinaryData::styles_cssSize,
                                   "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (UIBinaryData::app_js, UIBinaryData::app_jsSize,
                                   "application/javascript; charset=utf-8");

    // v1.6.0 — the hover-help copy table, English + French. Embedded in the
    // UIResources target AND served here: a file that is one but not the other
    // is a 404 that presents as a page with no tooltips and nothing else.
    // Sections 9 and 21 close that loop, and scripts/check-i18n.js assertion 8
    // checks both ends independently.
    if (url == "/js/i18n.js")
        return makeBinaryResource (UIBinaryData::i18n_js, UIBinaryData::i18n_jsSize,
                                   "application/javascript; charset=utf-8");

    // NOT room-plan.js. juce_add_binary_data STRIPS a hyphen rather than
    // converting it to an underscore, so a hyphenated name would have to be
    // reached as `roomplan_js` anyway; the file is authored hyphen-free so there
    // is no transform to remember (critical_binary_data_strips_hyphens).
    if (url == "/js/roomplan.js")
        return makeBinaryResource (UIBinaryData::roomplan_js, UIBinaryData::roomplan_jsSize,
                                   "application/javascript; charset=utf-8");

    // Phase 3.2's page module. Section 21 of the static gate now DERIVES the module list from
    // Source/ui/public/js/*.js and asserts it equals the juce_add_binary_data SOURCES set, so a
    // file added on disk and forgotten here fails a gate instead of 404-ing as a missing panel.
    if (url == "/js/venue.js")
        return makeBinaryResource (UIBinaryData::venue_js, UIBinaryData::venue_jsSize,
                                   "application/javascript; charset=utf-8");

    // Phase 3.3's four page modules. NO FILENAME CONTAINS A HYPHEN, here or on disk:
    // juce_add_binary_data STRIPS a hyphen rather than converting it to an underscore, so
    // `plan-field.js` would have to be reached as the symbol `planfield_js`
    // (critical_binary_data_strips_hyphens). Authored hyphen-free so there is no transform to
    // remember.
    if (url == "/js/scenes.js")
        return makeBinaryResource (UIBinaryData::scenes_js, UIBinaryData::scenes_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/meters.js")
        return makeBinaryResource (UIBinaryData::meters_js, UIBinaryData::meters_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/field.js")
        return makeBinaryResource (UIBinaryData::field_js, UIBinaryData::field_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/elevation.js")
        return makeBinaryResource (UIBinaryData::elevation_js, UIBinaryData::elevation_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (UIBinaryData::index_js, UIBinaryData::index_jsSize,
                                   "application/javascript; charset=utf-8");

    // Embedded AND served, or the page can hang (juce8-critical-patterns
    // section 13). js/juce/index.js imports it directly.
    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (UIBinaryData::check_native_interop_js,
                                   UIBinaryData::check_native_interop_jsSize,
                                   "application/javascript; charset=utf-8");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
OctagonEditor::OctagonEditor (OOctagonProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      // The C++ side only. Initialiser order matches declaration order, which is
      // what keeps -Wreorder quiet under the zero-warning gate.
      presetManager (p.getAPVTS(), "O-Octagon")
{
    // 1. RELAYS (must exist before the WebView) ------------------------------
    //
    // Built from oo::params::id(i), which is THE single mapping between the
    // parameter order and the APVTS ids (GainStage.h:75). There is deliberately
    // no kSliderIds literal in this file: a second transcribed list is exactly
    // the mirrored fixture that has drifted five times in the precedent, and it
    // would drift silently — a relay for an id that no longer exists attaches to
    // nothing and produces a dead control.
    for (int i = 0; i < static_cast<int> (oo::params::kCount); ++i)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (oo::params::id (i)));

    // 2. WEBVIEW options + relay registration --------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);

    // ── NATIVE FUNCTIONS — EXACTLY THREE, AND NO FOURTH ────────────────────

    // (1) Dblclick-reset needs each parameter's default in ENGINEERING units:
    // the properties payload pushed to the page carries start/end/skew but no
    // default, and a hardcoded JS default table would drift from the C++
    // NormalisableRange (pattern_webview_knob_readout_scaled_value). The trap
    // that table would hit here is specific and quiet: w1..w8 default to 1.0,
    // NOT to their range minimum.
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();

            for (int i = 0; i < static_cast<int> (oo::params::kCount); ++i)
            {
                const juce::String id { oo::params::id (i) };   // ASCII ids

                if (auto* param = processorRef.getAPVTS().getParameter (id))
                    obj->setProperty (id, param->convertFrom0to1 (param->getDefaultValue()));
            }

            complete (juce::var (obj));
        });

    // (2) The room, in ONE payload. Envelope, speaker bbox, centroid, rigScale,
    // the eight positions with their labels and classifications, and the hull —
    // all read from the live objects the processor already owns, in one pass, so
    // the page cannot composite an envelope from venue A with glyphs from venue
    // B.
    //
    // TWO BOXES TRAVEL, AND THEY ARE NOT THE SAME BOX:
    //   envelope  bbox + margin. What the plan DRAWS, so proportions follow the
    //             venue (UI-02 criterion 1).
    //   bbox      the raw speaker bounding box. What VenueModel::normToMetres()
    //             denormalises srcX / srcY against, so it is what the puck's
    //             position and the metres readout resolve through.
    // Sending only the envelope would force the page to invert the margin rule
    // to recover the bbox — a second derivation, free to drift, and wrong by
    // 1.80 m and 2.25 m on the default venue in a way that looks plausible.
    //
    // degenerateX / degenerateY are decided HERE, against oo::plane::kMinSpan —
    // the single definition, referenced and not transcribed. VenueModel pins a
    // degenerate axis to its minimum rather than dividing by zero
    // (VenueModel.h:185-192); a naive min + n * (max - min) in JS diverges from
    // the C++ on exactly the degenerate venues Phase 2.1 spent a whole matrix
    // on. The page branches on the flag, never on a threshold.
    //
    // NOTE for section 13 of the static gate: there is no numeric literal for
    // any bbox bound anywhere below. Every bound is an accessor call.
    options = options.withNativeFunction ("getVenueGeometry",
        [this] (auto&, auto complete)
        {
            const auto& venue = processorRef.getVenue();
            const auto& hull  = processorRef.getHull();

            const float spanX = venue.bbMaxX() - venue.bbMinX();
            const float spanY = venue.bbMaxY() - venue.bbMinY();

            const float marginX = juce::jmax (kEnvelopeMarginMinM, kEnvelopeMarginFraction * spanX);
            const float marginY = juce::jmax (kEnvelopeMarginMinM, kEnvelopeMarginFraction * spanY);

            auto* envelope = new juce::DynamicObject();
            envelope->setProperty ("minX", venue.bbMinX() - marginX);
            envelope->setProperty ("maxX", venue.bbMaxX() + marginX);
            envelope->setProperty ("minY", venue.bbMinY() - marginY);
            envelope->setProperty ("maxY", venue.bbMaxY() + marginY);

            auto* bbox = new juce::DynamicObject();
            bbox->setProperty ("minX", venue.bbMinX());
            bbox->setProperty ("maxX", venue.bbMaxX());
            bbox->setProperty ("minY", venue.bbMinY());
            bbox->setProperty ("maxY", venue.bbMaxY());
            bbox->setProperty ("degenerateX", spanX < oo::plane::kMinSpan);
            bbox->setProperty ("degenerateY", spanY < oo::plane::kMinSpan);

            const auto c = venue.centroid();
            auto* centroid = new juce::DynamicObject();
            centroid->setProperty ("x", c.x);
            centroid->setProperty ("y", c.y);

            juce::Array<juce::var> speakers;
            speakers.ensureStorageAllocated (oo::VenueModel::kNumSpeakers);

            for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
            {
                const auto pos = venue.speaker (i);

                auto* s = new juce::DynamicObject();
                s->setProperty ("n",     i + 1);
                s->setProperty ("x",     pos.x);
                s->setProperty ("y",     pos.y);
                s->setProperty ("z",     pos.z);
                s->setProperty ("label", venue.labelAbbreviation (i));
                s->setProperty ("class", juce::String (classificationName (hull.classify (i))));

                // Phase 3.2 (P55). trimDb rides INSIDE the speaker object rather than in a
                // parallel trims[8] array, so a consumer cannot index the two out of step. With
                // the rake below, 8 x 6 + 2 = 50 is now fully representable from THIS ONE CALL —
                // which is what lets the Venue table be a rendering job and setVenue its exact
                // inverse. Still one call: P38's torn-read argument is untouched.
                s->setProperty ("trimDb", venue.trimDb (i));

                // v1.4.0. INSIDE the speaker object for the identical reason trimDb is, one line
                // up: a parallel delays[8] array is a second thing to index, and a consumer that
                // indexes the two out of step produces a room that is measured and misaligned with
                // nothing on screen to distinguish it. 8 x 6 + 2 = 50 is now fully representable
                // from THIS ONE CALL, so the Venue table stays a rendering job and setVenue stays
                // its exact inverse. Still one call: P38's torn-read argument is untouched.
                s->setProperty ("delayMs", venue.delayMs (i));

                // v1.1.0. The 1-based PHYSICAL output this speaker's label reaches under the
                // measured CoreAudio 7.1 device order, or 0 for a label outside the 7.1 set.
                // COMPUTED HERE AND RETURNED WHOLE (D19): the page renders this number and never
                // owns a device-order table of its own — a JS copy would be a mirrored fixture
                // over R1, free to drift from the one in Data/OutputOrder.h.
                s->setProperty ("output", oo::outorder::outputNumberForLabel (venue.labelAbbreviation (i)));

                speakers.add (juce::var (s));
            }

            // Venue-scoped, not per-speaker, so its own object.
            auto* rake = new juce::DynamicObject();
            rake->setProperty ("front", venue.rakeFront());
            rake->setProperty ("rear",  venue.rakeRear());

            juce::Array<juce::var> hullPoints;
            const int hullCount = hull.getNumHullPoints();
            hullPoints.ensureStorageAllocated (hullCount);

            for (int i = 0; i < hullCount; ++i)
            {
                const auto pt = hull.getHullPoint (i);

                auto* h = new juce::DynamicObject();
                h->setProperty ("x", pt.x);
                h->setProperty ("y", pt.y);

                hullPoints.add (juce::var (h));
            }

            // ── Phase 3.3 (P79 / Q10): NAMED-SCENE MEMBERSHIP RIDES THIS PAYLOAD ───────────
            //
            // It is a PURE FUNCTION OF THE VENUE, so it belongs in the call that already refreshes
            // on venueGen — which removes a whole staleness class and costs no nineteenth native
            // function. The four USER slots are NOT a venue function and have their own read.
            //
            // COMPUTED IN C++ AND RETURNED WHOLE (D19). The page performs no speaker arithmetic:
            // a JS re-derivation would be a mirrored fixture over R1, and it is exactly what
            // FUNC-06/2's permutation probe exists to catch — a fixed-index implementation must
            // FAIL, and it cannot fail a test the page never runs. §32 asserts the JS absence.
            juce::Array<juce::var> sceneSets;
            sceneSets.ensureStorageAllocated (oo::scenes::kNumNamed);

            for (int s = 0; s < oo::scenes::kNumNamed; ++s)
            {
                const auto which = static_cast<oo::scenes::Named> (s);

                // THE SAME FUNCTION applyScene CONSULTS, and the same one probes CF/CG/CH drive.
                // One implementation, three consumers.
                const auto m = oo::scenes::resolve (which, venue.speakerPositions(), hull);

                juce::Array<juce::var> indices;

                for (int i = 0; i < oo::scenes::kNumSpeakers; ++i)
                    if (m.in[static_cast<std::size_t> (i)])
                        indices.add (i + 1);   // 1-based, matching every human-facing surface

                auto* entry = new juce::DynamicObject();
                entry->setProperty ("id",      juce::String (oo::scenes::name (which)));
                entry->setProperty ("indices", juce::var (indices));
                entry->setProperty ("empty",   m.isEmpty());

                sceneSets.add (juce::var (entry));
            }

            auto* obj = new juce::DynamicObject();
            obj->setProperty ("envelope",   juce::var (envelope));
            obj->setProperty ("bbox",       juce::var (bbox));
            obj->setProperty ("scenes",     juce::var (sceneSets));
            obj->setProperty ("centroid",   juce::var (centroid));
            obj->setProperty ("rigScale",   venue.rigScale());

            // v1.4.0 — the Venue table's metres/ms toggle converts with this, rather than holding
            // its own 343. A JS literal would be a mirrored fixture over oo::plane::kSpeedOfSoundMps
            // (pattern_test_fixture_mirrors_drift_silently) and free to drift from the constant the
            // suggestion is actually computed with — so the column would read metres the derive
            // button disagrees with. Same D19 rule the output-order arithmetic follows.
            obj->setProperty ("speedOfSound", oo::plane::kSpeedOfSoundMps);

            // The rail the column enforces, also sent rather than transcribed.
            obj->setProperty ("maxDelayMs",   OOctagonProcessor::kVenueDelayClampMs);
            obj->setProperty ("venueName",  venue.getName());
            obj->setProperty ("speakers",   juce::var (speakers));
            obj->setProperty ("rake",       juce::var (rake));
            obj->setProperty ("hull",       juce::var (hullPoints));
            obj->setProperty ("hullCount",  hullCount);
            obj->setProperty ("generation", static_cast<juce::int64> (processorRef.getVenueGeneration()));

            complete (juce::var (obj));
        });

    // (3) The only POLLED function, at 2 Hz from a JS interval — deliberately
    // not a juce::Timer. Keeping the pull on the page's side is what lets the
    // ui-stub render the whole UI without modelling backend.addEventListener,
    // and it keeps this class Timer-free.
    //
    // THE NEGOTIATED-SET NAME IS NEVER AN ATOMIC. A juce::String written on one
    // thread and read on another is a race; it is resolved HERE, on the message
    // thread, from the bus that owns it. safeMode by contrast IS an atomic,
    // written in prepareToPlay() beside the isBusesLayoutSupported() rule it
    // mirrors, so the "which sets count as SAFE" decision exists in exactly one
    // place (PLAN-3.1 P43).
    options = options.withNativeFunction ("getStatus",
        [this] (auto&, auto complete)
        {
            juce::String outputSetName;

            if (auto* bus = processorRef.getBus (false, 0))
                outputSetName = bus->getCurrentLayout().getDescription();

            auto* obj = new juce::DynamicObject();
            obj->setProperty ("safeMode",          processorRef.isSafeMode());
            obj->setProperty ("outputSetName",     outputSetName);
            obj->setProperty ("numOutputChannels", processorRef.getTotalNumOutputChannels());
            obj->setProperty ("mapInvalid",        processorRef.isChannelMapInvalid());

            // ── P54 — WHY, AND WHICH ROW ─────────────────────────────────────
            // After N8 this banner is the only thing telling an operator why
            // seven speakers just went mono, and in a hall WHICH ROW is the
            // actionable half. buildSpeakerToBuffer already separated the three
            // failure modes and threw the distinction away in a bool; 3.2 keeps
            // it. NO NEW ATOMIC: rebuildChannelMap() and this lambda are both
            // the message thread, which is P43's rule reused rather than
            // re-argued. mapInvalid itself stays the atomic it is, because that
            // one really is read by the audio thread.
            const auto diagnosis = processorRef.lastMapDiagnosis();
            obj->setProperty ("mapInvalidReason",  juce::String (mapFailureName (diagnosis.reason)));
            obj->setProperty ("mapInvalidSpeaker", diagnosis.speakerIndex);

            obj->setProperty ("venueGen",
                              static_cast<juce::int64> (processorRef.getVenueGeneration()));

            // Phase 3.3 (P79). scenesGen MIRRORS venueGen for the four USER slots, which are the
            // one part of FUNC-06 that is not a function of the venue and therefore cannot ride
            // getVenueGeometry. Same poll, same shape, no new native function.
            obj->setProperty ("scenesGen",
                              static_cast<juce::int64> (processorRef.getScenesGeneration()));

            // ── v1.7.0 — THE MONITOR FOLD ────────────────────────────────────
            // Rides the EXISTING poll rather than adding a native function, the
            // same argument scenesGen makes. The banner has to react to state
            // the page never set — a bypass, a SAFE-mode flip, a ping start and
            // an offline render all clear or suppress the arm behind its back —
            // so a one-shot push at init would go stale within seconds
            // (pattern_webview_one_shot_state_push_stale_on_preset_load).
            obj->setProperty ("monitorArmed",      processorRef.isMonitorArmed());
            obj->setProperty ("monitorSuppressed", processorRef.isMonitorSuppressed());

            // Whether the ARM WOULD BE ACCEPTED, so the page can disable the
            // control instead of offering a button that silently refuses. The
            // predicate mirrors setMonitorArmed()'s preconditions.
            obj->setProperty ("monitorAvailable",  ! processorRef.isSafeMode()
                                                && ! processorRef.isChannelMapInvalid());

            complete (juce::var (obj));
        });

    // ══════════════════════════════════════════════════════════════════════
    // PHASE 3.2 — the ten that turn a renderer of state into an editor of it.
    // ══════════════════════════════════════════════════════════════════════

    // (4) setVenue — ALL 42 VALUES IN ONE CALL, VALIDATED BEFORE APPLIED.
    //
    // ONE call and not 42 for the same reason getVenueGeometry is one and not
    // three, arriving on the write side: 42 async round trips whose promises may
    // resolve out of order, against a model that recomputes bbox, centroid,
    // rigScale and the convex hull on every one of them.
    //
    // It calls applyVenueEditChecked() and NEVER applyVenueEdit() — section 22
    // of the static gate asserts this file contains no `applyVenueEdit (` call
    // site at all. The guard is what keeps a half-typed label swap off the PA:
    // an invalid map is AUDIBLE, and it collapses speakers 2-8 onto the right
    // input at unity (N8).
    options = options.withNativeFunction ("setVenue",
        [this] (auto& args, auto complete)
        {
            if (args.size() < 1)
            {
                complete (makeResult (false, "notEightChannels", -1));
                return;
            }

            const auto& payload  = args[0];
            auto*       payloadO = payload.getDynamicObject();

            if (payloadO == nullptr)
            {
                complete (makeResult (false, "notEightChannels", -1));
                return;
            }

            const auto  speakersVar = payloadO->getProperty ("speakers");
            const auto* speakerArr  = speakersVar.getArray();

            if (speakerArr == nullptr || speakerArr->size() != oo::VenueModel::kNumSpeakers)
            {
                complete (makeResult (false, "notEightChannels", -1));
                return;
            }

            // Starts from the LIVE venue, so anything not on the wire — the venue
            // name — survives a table commit untouched.
            oo::VenueModel edited = processorRef.getVenue();

            for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
            {
                const auto& row = (*speakerArr)[i];
                const auto  pos = edited.speaker (i);

                edited.setSpeakerPosition (i, { finiteOr (row, "x", pos.x),
                                                finiteOr (row, "y", pos.y),
                                                finiteOr (row, "z", pos.z) });

                edited.setSpeakerTrimDb (i, finiteOr (row, "trimDb", edited.trimDb (i)));

                // v1.4.0. finiteOr falls back to the LIVE value, so a page that predates this
                // field — or one whose delay column failed to parse — leaves the stored delays
                // exactly as they were rather than zeroing eight measured values.
                edited.setSpeakerDelayMs (i, finiteOr (row, "delayMs", edited.delayMs (i)));

                if (auto* rowO = row.getDynamicObject(); rowO != nullptr && rowO->hasProperty ("label"))
                    edited.setSpeakerLabel (i, rowO->getProperty ("label").toString());
            }

            const auto rakeVar = payloadO->getProperty ("rake");
            edited.setRake (finiteOr (rakeVar, "front", edited.rakeFront()),
                            finiteOr (rakeVar, "rear",  edited.rakeRear()));

            ochan::MapDiagnosis whyNot {};
            const bool ok = processorRef.applyVenueEditChecked (edited, &whyNot);

            complete (makeResult (ok, mapFailureName (whyNot.reason), whyNot.speakerIndex));
        });

    // ══════════════════════════════════════════════════════════════════════
    // v1.1.0 — the speaker→output surface (18 -> 20).
    //
    // Both are LABEL EDITS through the same guard setVenue uses: clone the live venue, rewrite
    // the eight labels, applyVenueEditChecked(). Nothing new can reach the audio thread that the
    // existing path could not already send it, and section 22's "one setVenue call site" is
    // untouched — these are their own registrations, not a second write surface for the 42.
    //
    // THE ARITHMETIC LIVES HERE, NOT IN JS (D19). "Assign speaker n to output k" needs the
    // measured device order and a permutation repair; a JS copy of either would be a mirrored
    // fixture over R1. The page sends two integers and renders what getVenueGeometry returns.
    // ══════════════════════════════════════════════════════════════════════

    // (4b) assignSpeakerOutput (speakerN 1..8, outputK 1..8) — the Room plan's double-click
    // popover. SWAP SEMANTICS: speaker n takes output k; the previous holder of k takes n's old
    // output. A speaker whose label is outside the 7.1 set (output 0) is repaired from the pool
    // of unclaimed outputs, so the result is a permutation BY CONSTRUCTION — the guard still
    // validates it, but "every route passes through a duplicate" (the label column's reachability
    // problem) does not arise here at all.
    options = options.withNativeFunction ("assignSpeakerOutput",
        [this] (auto& args, auto complete)
        {
            const int speakerN = args.size() > 0 ? static_cast<int> (args[0]) : 0;
            const int outputK  = args.size() > 1 ? static_cast<int> (args[1]) : 0;

            if (speakerN < 1 || speakerN > oo::VenueModel::kNumSpeakers
                || outputK < 1 || outputK > oo::outorder::kNumOutputs)
            {
                complete (makeResult (false, "labelNotInSet", -1));
                return;
            }

            oo::VenueModel edited = processorRef.getVenue();

            // Current 1-based output per speaker; 0 marks a label the device-order table cannot
            // place. Repairs below draw from the outputs no speaker currently claims.
            std::array<int, oo::VenueModel::kNumSpeakers> want {};

            for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
                want[static_cast<std::size_t> (i)] =
                    oo::outorder::outputNumberForLabel (edited.labelAbbreviation (i));

            const auto sp   = static_cast<std::size_t> (speakerN - 1);
            const int  prev = want[sp];

            for (int j = 0; j < oo::VenueModel::kNumSpeakers; ++j)
                if (j != speakerN - 1 && want[static_cast<std::size_t> (j)] == outputK)
                    want[static_cast<std::size_t> (j)] = prev;   // may be 0 — repaired below

            want[sp] = outputK;

            // Permutation repair: hand each unplaced speaker an unclaimed output, in order. This
            // is reachable only from labels the popover did not write (custom sets, duplicates
            // typed in the table) — a clean swap never enters it.
            std::array<bool, oo::outorder::kNumOutputs> claimed {};

            for (const int w : want)
                if (w >= 1 && w <= oo::outorder::kNumOutputs)
                    claimed[static_cast<std::size_t> (w - 1)] = true;

            for (auto& w : want)
                if (w < 1 || w > oo::outorder::kNumOutputs)
                    for (int k = 0; k < oo::outorder::kNumOutputs; ++k)
                        if (! claimed[static_cast<std::size_t> (k)])
                        {
                            claimed[static_cast<std::size_t> (k)] = true;
                            w = k + 1;
                            break;
                        }

            for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
                edited.setSpeakerLabel (i, oo::outorder::abbreviationForOutput (
                                               want[static_cast<std::size_t> (i)]));

            ochan::MapDiagnosis whyNot {};
            const bool ok = processorRef.applyVenueEditChecked (edited, &whyNot);

            complete (makeResult (ok, mapFailureName (whyNot.reason), whyNot.speakerIndex));
        });

    // (4c) applyOutputOrderPreset ("direct" | "roles") — the Venue screen's two one-click sets.
    //
    //   direct  speaker n → physical output n under the measured CoreAudio device order. The
    //           one-click fix for a rig wired 1..8 in a CoreAudio host.
    //   roles   the factory label assignment — surround ROLE order, exactly what
    //           VenueModel::resetToDefaults() writes. Read from a default-constructed model
    //           rather than transcribed, so this cannot drift from the factory table.
    options = options.withNativeFunction ("applyOutputOrderPreset",
        [this] (auto& args, auto complete)
        {
            const auto id = args.size() > 0 ? args[0].toString() : juce::String {};

            oo::VenueModel edited = processorRef.getVenue();

            if (id == "direct")
            {
                for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
                    edited.setSpeakerLabel (i, oo::outorder::abbreviationForOutput (i + 1));
            }
            else if (id == "roles")
            {
                const oo::VenueModel factory;

                for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
                    edited.setSpeakerLabel (i, factory.labelAbbreviation (i));
            }
            else
            {
                complete (makeResult (false, "labelNotInSet", -1));
                return;
            }

            ochan::MapDiagnosis whyNot {};
            const bool ok = processorRef.applyVenueEditChecked (edited, &whyNot);

            complete (makeResult (ok, mapFailureName (whyNot.reason), whyNot.speakerIndex));
        });

    // (4d) applySuggestedDelays — v1.4.0's Derive button, and the LAST of the four label/value
    // one-click sets that share applyOutputOrderPreset's shape: clone the live venue, overwrite one
    // family of values, push it through applyVenueEditChecked().
    //
    // THE ARITHMETIC LIVES IN C++ (D19), and here that is load-bearing twice over. The law needs
    // the centroid, the audience plane and eight 3-D distances; a JS re-derivation would be a
    // mirrored fixture over VenueModel::suggestedDelaysMs() with nothing tying the two together,
    // and it is exactly the kind of copy v1.3.5's own `blur`-fallback bug was — a default stated in
    // two places, stale for two minor versions with nothing on screen to show for it.
    //
    // A ONE-SHOT FILL, NOT A MODE. It writes the eight stored values once and returns; nothing
    // recomputes them afterwards. That is the whole of "auto-derive with manual override" — the
    // operator edits any of the eight freely from that point, and moving a speaker later does not
    // silently rewrite a delay they typed by hand.
    options = options.withNativeFunction ("applySuggestedDelays",
        [this] (auto&, auto complete)
        {
            oo::VenueModel edited = processorRef.getVenue();

            // Computed from the LIVE geometry, before this edit — the delays are not an input to
            // the law, so ordering here cannot feed back on itself.
            const auto suggested = edited.suggestedDelaysMs();

            for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
                edited.setSpeakerDelayMs (i, suggested[static_cast<std::size_t> (i)]);

            // Through the SAME guard, even though a delay edit cannot break the channel map. Not
            // because this call needs validating, but because section 22's "every venue write goes
            // through applyVenueEditChecked" is only true if it has no exceptions — and an
            // exception here is how the next value that CAN break the map arrives unguarded.
            ochan::MapDiagnosis whyNot {};
            const bool ok = processorRef.applyVenueEditChecked (edited, &whyNot);

            complete (makeResult (ok, mapFailureName (whyNot.reason), whyNot.speakerIndex));
        });

    // (5) saveVenue — FileChooser::launchAsync, then oo::venuefile::save.
    //
    // THE SafePointer IS HOISTED TO A LOCAL and captured by copy. Writing
    // `[safeThis = juce::Component::SafePointer<OctagonEditor> (this)]` inside a
    // nested lambda is MSVC-specific breakage: it resolves `this` to the CLOSURE
    // rather than to the editor
    // (critical_msvc_safepointer_init_capture_nested_lambda). Silent on Apple
    // Clang, a hard error on the first Windows CI build — authored out now, not
    // fixed at port time. Section 20 of the static gate asserts the form.
    //
    // ON A DEAD POINTER THE COMPLETION RETURNS BARE. Never complete(false),
    // which is itself a use-after-free
    // (pattern_webview_launchasync_safepointer_no_complete). N4 adds the other
    // half: even on a LIVE pointer the completion may be dropped, so nothing in
    // the UI waits on it.
    options = options.withNativeFunction ("saveVenue",
        [this] (auto&, auto complete)
        {
            juce::Component::SafePointer<OctagonEditor> safeThis { this };

            venueChooser = std::make_unique<juce::FileChooser> (
                "Save venue", juce::File(), juce::String (oo::venuefile::kFileWildcard));

            venueChooser->launchAsync (
                juce::FileBrowserComponent::saveMode
                    | juce::FileBrowserComponent::canSelectFiles
                    | juce::FileBrowserComponent::warnAboutOverwriting,
                [safeThis, complete] (const juce::FileChooser& chooser)
                {
                    if (safeThis == nullptr)
                        return;                            // BARE. Never complete(false).

                    const auto file = chooser.getResult();

                    if (file == juce::File())
                    {
                        complete (makeResult (false, "cancelled", -1));
                        return;
                    }

                    const auto target = file.withFileExtension (
                        juce::String (oo::venuefile::kFileExtension));

                    const bool ok = oo::venuefile::save (safeThis->processorRef.getVenue(), target);
                    complete (makeResult (ok, ok ? "none" : "unreadable", -1));
                });
        });

    // (6) loadVenue — into a FRESH model, then through the SAME guard a typed
    // label goes through.
    //
    // A .venue written by a future build must not half-apply into the live room:
    // VenueModel::readFromState falls back PER ATTRIBUTE to whatever the model
    // already held, which is right for session state and wrong for a file
    // (RESEARCH-3.2 Q6). oo::venuefile::load enforces the fresh model, surfaces
    // a forward @schemaVersion and rejects a malformed root without touching
    // anything.
    options = options.withNativeFunction ("loadVenue",
        [this] (auto&, auto complete)
        {
            juce::Component::SafePointer<OctagonEditor> safeThis { this };

            venueChooser = std::make_unique<juce::FileChooser> (
                "Load venue", juce::File(), juce::String (oo::venuefile::kFileWildcard));

            venueChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete] (const juce::FileChooser& chooser)
                {
                    if (safeThis == nullptr)
                        return;                            // BARE. Never complete(false).

                    const auto file = chooser.getResult();

                    if (file == juce::File())
                    {
                        complete (makeResult (false, "cancelled", -1));
                        return;
                    }

                    // FRESH, default-constructed. Never the live venue.
                    oo::VenueModel loaded;
                    int fileVersion = oo::VenueModel::kSchemaVersion;

                    const auto result = oo::venuefile::load (file, loaded, &fileVersion);

                    if (result == oo::venuefile::LoadResult::malformedRoot
                        || result == oo::venuefile::LoadResult::unreadable)
                    {
                        complete (makeResult (false,
                                              result == oo::venuefile::LoadResult::malformedRoot
                                                  ? "malformedRoot" : "unreadable", -1));
                        return;
                    }

                    // The SAME guard, so a file whose labels do not resolve against the negotiated
                    // set is rejected exactly as a typed duplicate is — and 3.2 still adds no
                    // second venue-apply path.
                    ochan::MapDiagnosis whyNot {};

                    if (! safeThis->processorRef.applyVenueEditChecked (loaded, &whyNot))
                    {
                        complete (makeResult (false, mapFailureName (whyNot.reason), whyNot.speakerIndex));
                        return;
                    }

                    complete (makeResult (true,
                                          result == oo::venuefile::LoadResult::forwardVersion
                                              ? "forwardVersion" : "none", -1));
                });
        });

    // (7) savePreset — the musical store. FUNC-05 holds BY CONSTRUCTION:
    // applyPresetJson iterates processor.getParameters() only and can never walk
    // apvts.state's children, where VENUE lives.
    options = options.withNativeFunction ("savePreset",
        [this] (auto& args, auto complete)
        {
            const auto name = args.size() > 0 ? args[0].toString() : juce::String();

            if (name.trim().isEmpty())
            {
                complete (makeResult (false, "emptyName", -1));
                return;
            }

            const bool ok = presetManager.savePreset (name.trim());
            complete (makeResult (ok, ok ? "none" : "writeFailed", -1));
        });

    // (8) loadPreset — AND THE SEVENTEEN GESTURE BRACKETS (N5 / P59).
    //
    // OuariconPresetManager::applyPresetJson calls setValueNotifyingHost DIRECTLY
    // on the parameter object, not through a ParameterAttachment. Two consequences,
    // both verified through the chain rather than assumed:
    //
    //   * F3's unchanged-write skip DOES NOT APPLY. callIfParameterValueChanged is
    //     a member of the ATTACHMENT (juce_ParameterAttachments.cpp:88-95) and this
    //     path never goes through one, so a load from an untouched default patch
    //     still emits 18 reset writes of values the parameters already hold.
    //   * THERE ARE NO BRACKETS. setValueNotifyingHost is setValue +
    //     sendValueChangedMessageToListeners and nothing else. The wrappers turn
    //     that into a bare kAudioUnitEvent_ParameterValueChange (AU_1.mm:1341-1360)
    //     and a bare paramChanged (VST3.cpp:1498-1501) — up to 34 UNBRACKETED host
    //     writes per load, which in Logic with a lane in Latch or Touch is a
    //     recorded sweep the operator did not perform.
    //
    // THE FIX IS HERE, AT O-OCTAGON'S CALL SITE, and never in the shared module:
    // four other plugins depend on it. This also GENERALISES P39's rule — open a
    // gesture on every parameter you will write, and close every one of them —
    // from the puck to the preset store.
    //
    // ── PHASE 4.1 (N5 / P93): A SECOND DEFECT ON THE SAME PATH ───────────────
    //
    // applyPresetJson does not merely apply the keys a preset carries. WR-01
    // RESETS ALL 18 TO THEIR DEFAULTS FIRST, so a factory preset scoped to the
    // six room-character parameters does not LEAVE the other eleven alone — it
    // re-centres srcX/srcY, drops srcZ to 0 m and returns all eight weights to
    // 1.0, wiping whatever scene is applied. Mid-cue, in a hall.
    //
    // oo::presets::loadPreserving is the fix, and it lives in a header the
    // RENDER HARNESS can reach so probe CP can assert the eleven BIT-unchanged.
    // The eighteen brackets below are unchanged and still enclose everything:
    // the restore's writes land inside a bracket established at Phase 3.2, so
    // no new bracketing obligation is created here.
    options = options.withNativeFunction ("loadPreset",
        [this] (auto& args, auto complete)
        {
            const auto name = args.size() > 0 ? args[0].toString() : juce::String();

            std::array<juce::RangedAudioParameter*, oo::params::kCount> params {};

            for (int i = 0; i < static_cast<int> (oo::params::kCount); ++i)
                params[static_cast<std::size_t> (i)] =
                    processorRef.getAPVTS().getParameter (juce::String (oo::params::id (i)));

            for (auto* param : params)
                if (param != nullptr)
                    param->beginChangeGesture();

            const bool ok = oo::presets::loadPreserving (presetManager,
                                                         processorRef.getAPVTS(),
                                                         name);

            // Closed on BOTH paths. An interrupted load that never closed would
            // leave the host in an open automation-write region on all 17.
            for (auto* param : params)
                if (param != nullptr)
                    param->endChangeGesture();

            complete (makeResult (ok, ok ? "none" : "notFound", -1));
        });

    // (9) getPresetList / (10) getCurrentPreset — the two reads the rail needs.
    // FOUR functions total, not the module's TEN: any one of those ten that C++
    // did not register would be an N4-class never-settling promise the moment the
    // page called it.
    options = options.withNativeFunction ("getPresetList",
        [this] (auto&, auto complete)
        {
            juce::Array<juce::var> names;

            for (const auto& name : presetManager.getPresetList())
                names.add (juce::var (name));

            auto* obj = new juce::DynamicObject();
            obj->setProperty ("presets", juce::var (names));
            obj->setProperty ("current", presetManager.getCurrentPresetName());

            complete (juce::var (obj));
        });

    options = options.withNativeFunction ("getCurrentPreset",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("name", presetManager.getCurrentPresetName());
            complete (juce::var (obj));
        });

    // (11) startPing — AND IT REFUSES ON AN INVALID MAP (Q5 / P60).
    //
    // Pinging "speaker 5" on a stereo fold names a speaker that does not exist,
    // during the one procedure whose entire purpose is confirming that speaker N
    // IS speaker N. That is R1 reproduced inside its own diagnostic tool, and the
    // UI says why rather than doing nothing.
    options = options.withNativeFunction ("startPing",
        [this] (auto& args, auto complete)
        {
            int target = oo::VerifyPing::kAuto;

            if (args.size() > 0 && ! args[0].toString().equalsIgnoreCase ("auto"))
                target = static_cast<int> (args[0]);

            const bool ok = processorRef.startVerifyPing (target);

            const auto state = processorRef.verifyPingState();

            auto* obj = new juce::DynamicObject();
            obj->setProperty ("ok",          ok);
            obj->setProperty ("reason",      juce::String (ok ? "none" : "mapInvalid"));
            obj->setProperty ("active",      state.active);
            obj->setProperty ("mode",        state.mode);
            obj->setProperty ("speaker",     state.speaker);
            obj->setProperty ("elapsedMs",   state.elapsedMs);
            obj->setProperty ("remainingMs", state.remainingMs);

            complete (juce::var (obj));
        });

    // (12) stopPing — D11's explicit Stop. Graceful: the audio thread runs the
    // 20 ms release rather than cutting, which is why there is no reset() here or
    // anywhere else in that class.
    options = options.withNativeFunction ("stopPing",
        [this] (auto&, auto complete)
        {
            processorRef.stopVerifyPing();
            complete (makeResult (true, "none", -1));
        });

    // (13) getPingState — D14's authority, polled at 100 ms WHILE PINGING ONLY.
    //
    // The 2 Hz status poll is too slow to resolve a 400 ms gap, and a push
    // transport was rejected because emitEvent IS emitEventIfBrowserIsVisible: a
    // dropped push never retries where a poll self-heals on its next tick (P61).
    // The page renders `speaker` and never re-derives it.
    options = options.withNativeFunction ("getPingState",
        [this] (auto&, auto complete)
        {
            const auto state = processorRef.verifyPingState();

            auto* obj = new juce::DynamicObject();
            obj->setProperty ("active",      state.active);
            obj->setProperty ("mode",        state.mode);
            obj->setProperty ("speaker",     state.speaker);
            obj->setProperty ("elapsedMs",   state.elapsedMs);
            obj->setProperty ("remainingMs", state.remainingMs);

            complete (juce::var (obj));
        });

    // ══════════════════════════════════════════════════════════════════════
    // PHASE 3.3 — THE FIVE THAT CLOSE STAGE 3. The surface goes 13 -> 18.
    //
    // §3 of the static gate diffs this set against the derived JS call sites AND the ui-stub
    // whitelist, in BOTH directions, and its count literal moved with it. It failed loudly until
    // all eighteen existed in all three places, exactly as the 3 -> 13 move did at 3.2 — a count
    // that silently tracked whatever was registered would assert nothing at all.
    //
    // UI-05 NEEDS NO FUNCTION HERE. getVenueGeometry already carries per-speaker z, rake.front /
    // rake.rear, the bbox and the centroid, all landed by 3.2's P55, so the elevation strip is a
    // rendering job over a payload that already exists — which is half of D15's affordability
    // argument, confirmed in source rather than assumed.
    // ══════════════════════════════════════════════════════════════════════

    // (14) getMeters — polled at ~30 Hz, EIGHT LINEAR PEAKS, READ-AND-ZEROED HERE.
    //
    // A SECOND POLL RATHER THAN A FIELD ON getStatus, and the reason is measurable: getStatus
    // builds a juce::String from getBus(false,0)->getCurrentLayout().getDescription() on EVERY
    // call (see (3) above). Folding the meters in would run that at 30 Hz — thirty string
    // constructions a second on the message thread — for a value that changes only when the host
    // renegotiates. This payload is eight floats and nothing else.
    //
    // exchange(0) HAPPENS IN THE PROCESSOR, not on the page, so a DROPPED COMPLETION WIDENS THE
    // MEASUREMENT WINDOW INSTEAD OF LOSING THE PEAK — and completions really are dropped, silently,
    // whenever the editor is hidden (N4).
    options = options.withNativeFunction ("getMeters",
        [this] (auto&, auto complete)
        {
            const auto peaks = processorRef.readAndZeroMeters();

            juce::Array<juce::var> arr;
            arr.ensureStorageAllocated (ochan::kNumSpeakers);

            for (int i = 0; i < ochan::kNumSpeakers; ++i)
                arr.add (peaks[static_cast<std::size_t> (i)]);

            auto* obj = new juce::DynamicObject();
            obj->setProperty ("peaks", juce::var (arr));

            complete (juce::var (obj));
        });

    // (15) getScenes — THE FOUR USER SLOTS ONLY.
    //
    // The six NAMED scenes are not here: they are derived on demand and ride getVenueGeometry
    // (P79), so there is nothing about them to go stale when the venue moves. These four are
    // stored state and need their own read and their own generation.
    options = options.withNativeFunction ("getScenes",
        [this] (auto&, auto complete)
        {
            const auto& store = processorRef.getScenes();

            juce::Array<juce::var> slots;
            slots.ensureStorageAllocated (oo::SceneStore::kNumSlots);

            for (int s = 0; s < oo::SceneStore::kNumSlots; ++s)
            {
                const auto w = store.weights (s);

                juce::Array<juce::var> weights;
                weights.ensureStorageAllocated (oo::SceneStore::kNumSpeakers);

                for (int i = 0; i < oo::SceneStore::kNumSpeakers; ++i)
                    weights.add (w[static_cast<std::size_t> (i)]);

                auto* entry = new juce::DynamicObject();
                entry->setProperty ("slot",     s + 1);
                entry->setProperty ("occupied", store.isOccupied (s));
                entry->setProperty ("w",        juce::var (weights));

                slots.add (juce::var (entry));
            }

            auto* obj = new juce::DynamicObject();
            obj->setProperty ("slots",      juce::var (slots));
            obj->setProperty ("generation",
                              static_cast<juce::int64> (processorRef.getScenesGeneration()));

            complete (juce::var (obj));
        });

    // (16) applyScene — THE PHASE'S ONLY WRITE PATH, AND EIGHT GESTURE BRACKETS (D18 / P78).
    //
    // Takes a wire id: one of the six named scenes, or "slot1".."slot4". Named membership is
    // resolved through oo::scenes::resolve — THE SAME FUNCTION getVenueGeometry above returned to
    // the page and the same one probes CF/CG/CH drive — so the set the operator previewed and the
    // set that is written cannot differ.
    //
    // D20'S REFUSAL LIVES HERE, NOT ONLY IN THE DISABLED CONTROL. All-zero weights are DSP-05's
    // silence path; reaching it by a mis-derived scene click mid-concert is unrecoverable. This is
    // the same defence-in-depth startPing's mapInvalid refusal established at 3.2.
    options = options.withNativeFunction ("applyScene",
        [this] (auto& args, auto complete)
        {
            const auto id = args.size() > 0 ? args[0].toString() : juce::String();

            std::array<float, oo::SceneStore::kNumSpeakers> weights {};
            bool resolved = false;

            if (id.startsWith ("slot"))
            {
                const int slot = id.substring (4).getIntValue() - 1;

                if (! processorRef.getScenes().isOccupied (slot))
                {
                    complete (makeResult (false, "emptySlot", -1));
                    return;
                }

                weights  = processorRef.getScenes().weights (slot);
                resolved = true;
            }
            else if (oo::scenes::Named which {}; oo::scenes::parse (id, which))
            {
                const auto m = oo::scenes::resolve (which,
                                                    processorRef.getVenue().speakerPositions(),
                                                    processorRef.getHull());

                if (m.isEmpty())
                {
                    complete (makeResult (false, "emptyScene", -1));
                    return;
                }

                weights  = oo::scenes::weightsFor (m);
                resolved = true;
            }

            if (! resolved)
            {
                complete (makeResult (false, "unknownScene", -1));
                return;
            }

            complete (makeResult (processorRef.applySceneWeights (weights), "none", -1));
        });

    // (17) storeScene — D22's capture into one of the four slots.
    //
    // The capture itself is in the PROCESSOR: it owns apvts.state, and the slot must survive an
    // editor closed the instant after the click.
    options = options.withNativeFunction ("storeScene",
        [this] (auto& args, auto complete)
        {
            const int slot = args.size() > 0 ? static_cast<int> (args[0]) : 0;

            if (slot < 1 || slot > oo::SceneStore::kNumSlots)
            {
                complete (makeResult (false, "range", -1));
                return;
            }

            processorRef.captureScene (slot - 1);
            complete (makeResult (true, "none", -1));
        });

    // (18) getFieldGrid — UI-04's backdrop, 32 x 40, 8-bit, base64.
    //
    // MESSAGE-THREAD SAFE WITH NO SECOND SOLVER INSTANCE (Q1): dbap::solve is a free function with
    // no state, no allocation, no JUCE and `noexcept`. 183 us for the whole grid, measured.
    //
    // THE QUANTITY IS 1/k = sqrt(denom) AND NOT max_i v_i^2 (P69 / N10). The latter is identically
    // 1.0000 everywhere whenever exactly one weight is non-zero, because DBAP normalises to
    // sum v^2 = 1 and it therefore measures CONCENTRATION rather than level — the picture would go
    // blank precisely when the spatial situation is most extreme.
    //
    // minDb/maxDb are the PER-RECOMPUTE OBSERVED span, because the field over a raked audience
    // plane is genuinely flat and an absolute map renders an information-free wash that looks
    // exactly like a working one.
    options = options.withNativeFunction ("getFieldGrid",
        [this] (auto&, auto complete)
        {
            const auto& snapshot = processorRef.getVenueSnapshot();
            auto&       apvts    = processorRef.getAPVTS();

            std::array<float, oo::dbap::kNumSpeakers> w {};

            // NAMED so as not to shadow the lambda's own locals below. The repo builds with
            // juce_recommended_warning_flags and a HARD ZERO-WARNING GATE, and
            // -Wshadow-uncaptured-local fires on the obvious `p` in both scopes.
            for (int i = 0; i < oo::dbap::kNumSpeakers; ++i)
                if (auto* weightParam = apvts.getRawParameterValue (oo::params::id (oo::params::w1 + i)))
                    w[static_cast<std::size_t> (i)] = weightParam->load (std::memory_order_relaxed);

            // THE FALLBACK IS DERIVED, NOT TRANSCRIBED (SIMPLIFICATION-AUDIT MEDIUM-03, v1.3.5).
            // Three literals used to sit at the call sites below — 4.0f / 0.1f / 1.0f — and
            // blur's had ALREADY drifted: the live default moved 0.10 -> 0.03 in v1.3.0 when
            // kBlurScale tripled (PluginProcessor.cpp), and the copy here did not follow. Reading
            // convertFrom0to1 (getDefaultValue()) is exactly what getParameterDefaults does at
            // line 284, so the default is stated ONCE, in the APVTS layout, and the drift class
            // leaves with the literals (pattern_test_fixture_mirrors_drift_silently).
            //
            // The fallback is unreachable in practice: getRawParameterValue is non-null for every
            // valid id and the atomic is only non-finite if the host writes NaN. This is therefore
            // a correction on a dead path, not a behaviour change on a live one.
            const auto readParam = [&apvts] (const char* id)
            {
                auto* param       = apvts.getParameter (id);
                auto* atomicValue = apvts.getRawParameterValue (id);

                const float fallback = param != nullptr
                                         ? param->convertFrom0to1 (param->getDefaultValue())
                                         : 0.0f;

                if (atomicValue == nullptr)
                    return fallback;

                const float raw = atomicValue->load (std::memory_order_relaxed);
                return std::isfinite (raw) ? raw : fallback;
            };

            const float rolloff   = readParam (oo::params::id (oo::params::rolloff));
            const float blur      = readParam (oo::params::id (oo::params::blur));
            const float hullAtten = readParam (oo::params::id (oo::params::hullAtten));

            const auto field = fieldSampler.sample (
                snapshot, w.data(),
                oo::dbap::rolloffToAlpha (rolloff),
                oo::dbap::blurToRadius (blur, snapshot.rigScale),
                hullAtten);

            auto* obj = new juce::DynamicObject();
            obj->setProperty ("cols",         oo::FieldSampler::kCols);
            obj->setProperty ("rows",         oo::FieldSampler::kRows);
            obj->setProperty ("minDb",        0.0f);
            obj->setProperty ("maxDb",        field.spanDb);
            obj->setProperty ("silent",       field.isSilent);
            obj->setProperty ("data",         toBase64 (oo::FieldSampler::quantise (field)));
            obj->setProperty ("computeCount",
                              static_cast<juce::int64> (fieldSampler.recomputeCount()));

            complete (juce::var (obj));
        });

    // ── HOVER HELP (v1.2.0) — the "?" toggle's persistence pair ────────────────────────────
    //
    // UI state, not a parameter: no automation lane, no preset membership. The set completes
    // with the stored value either way so the page could re-sync from the reply if it ever
    // wanted to.
    options = options.withNativeFunction ("setTooltipsEnabled",
        [this] (auto& args, auto complete)
        {
            if (args.size() > 0)
                processorRef.tooltipsEnabled.store ((bool) args[0], std::memory_order_release);

            complete (juce::var (processorRef.tooltipsEnabled.load (std::memory_order_acquire)));
        });

    // PULLED by the page at init, never pushed — a push from the constructor or a poll tick
    // fires before the page module has evaluated, so the preference would silently never
    // arrive and the toggle would read OFF on every reopen
    // (pattern_webview_one_shot_state_push_stale_on_preset_load).
    options = options.withNativeFunction ("getTooltipsEnabled",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.tooltipsEnabled.load (std::memory_order_acquire)));
        });

    // ── HOVER-HELP LANGUAGE (v1.6.0) — the language pair ──────────────────────────────────────
    //
    // Same shape and same discipline as the toggle above: plain withNativeFunction, no relay,
    // PULLED once by the page at init. No push from this constructor, no timer, no
    // poll().then(poll) and no revision counter — the language is not preset content, and
    // OuariconPresetManager::loadPreset walks preset["parameters"] only, so no preset path can
    // change it behind the page's back.
    options = options.withNativeFunction ("getUiLanguage",
        [this] (auto&, auto complete)
        {
            complete (juce::var (OOctagonProcessor::languageCode (
                processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    options = options.withNativeFunction ("setUiLanguage",
        [this] (auto& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" to 0, so an unexpected argument
            // from the page degrades to English rather than being stored unvalidated.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    OOctagonProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);

            complete (juce::var (OOctagonProcessor::languageCode (
                processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    // ══════════════════════════════════════════════════════════════════════
    // v1.7.0 — THE MONITOR FOLD-DOWN
    //
    // Plain withNativeFunction, no relay and no parameter attachment, and that
    // is the point rather than a shortcut: an APVTS parameter would give the
    // arm an automation lane, and an automation lane is a recording of the arm
    // that a bounce could replay. See PluginProcessor.h's four guards.
    // ══════════════════════════════════════════════════════════════════════
    options = options.withNativeFunction ("setMonitorArmed",
        [this] (auto& args, auto complete)
        {
            const bool want = args.size() > 0 && static_cast<bool> (args[0]);

            // The RESULT is what the page renders, never the request. An arm is
            // refused in SAFE mode and on an unresolved monitor pair, and a
            // banner that lit because the page ASKED would be claiming the rig
            // lanes are muted while eight speakers carry on playing.
            processorRef.setMonitorArmed (want);

            complete (juce::var (processorRef.isMonitorArmed()));
        });

    // THERE IS DELIBERATELY NO getMonitorArmed. getStatus already carries
    // monitorArmed on the 2 Hz poll the banner needs anyway, and a second
    // reader would be a native function nothing calls — a dead bridge, which
    // section 3 of ui_frontend_check.js fails the build over precisely because
    // a half-wired bridge fails SILENTLY (pattern_webview_native_fn_bridge_gap).

   #if JUCE_WINDOWS
    // WebView2's default user-data folder is denied in most DAW hosts; a failed
    // construction falls back to the IE backend with no resource provider, which
    // presents as a BLANK PAGE and no error
    // (critical_webview2_runtime_gotchas_windows).
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OOctagon_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    // ── THE FIRST AND ONLY setCustomStateCallbacks REGISTRATION (D17 / P80) ────────────────
    //
    // Registered AFTER the native functions and BEFORE the WebView, beside the preset manager it
    // belongs to. It reaches `SCENES` and nothing else: the payload is four slots of eight weights,
    // built by the processor, and `VENUE` is not representable through it at all. FUNC-05's
    // guarantee therefore still holds — a musical preset cannot touch the 42 measured values — but
    // it now holds for a stated reason instead of by the absence of any registration, which is
    // exactly why FUNC-06 criterion 5 RE-MEASURES it (probe CL) rather than inheriting 3.2's pass.
    //
    // A PRESET WITHOUT SCENES LEAVES THE SLOTS UNTOUCHED. `applyPresetJson` calls `customLoad` only
    // when the "customState" property exists (OuariconPresetManager.h:346-349) — verified in module
    // source, not assumed — so every preset written before 3.3 is inert here rather than clearing
    // four slots the operator measured.
    //
    // AND `setStateFromXml` IS NEVER CALLED. That path (`:585-604`) invokes `customLoad` on a
    // DIFFERENT condition and, above it, does `parameters.replaceState(...)` — which would replace
    // the WHOLE tree, VENUE included. O-Octagon does not use it and must not start; §35 is a
    // one-line gate over both this file and PluginProcessor.cpp.
    presetManager.setCustomStateCallbacks (
        [this] { return processorRef.scenesToVar(); },
        [this] (const juce::var& payload) { processorRef.scenesFromVar (payload); });

    // ── v1.3.0 PRESET MIGRATION (preset-manager v1.0.6 hook) ────────────────────────────────
    //
    // Presets store NORMALISED 0..1 fractions, and v1.3.0 moved three encodings:
    //   rolloff  range 3-6  → 3-12 dB/2x   (same engineering value ⇒ old fraction ÷ 3)
    //   width    range 0-6  → 0-12 m       (same metres            ⇒ old fraction ÷ 2)
    //   blur     kBlurScale 0.5 → 1.5      (same RADIUS            ⇒ old fraction ÷ 3;
    //                                       the 0-1 range itself is unchanged)
    // Sessions are unaffected — the APVTS stores denormalised values
    // (critical_apvts_denormalised_vs_preset_normalised) — so only this JSON path migrates.
    //
    // The gate is the MAJOR.MINOR pair, parsed leniently: any preset stamped < 1.3 migrates, and
    // an unparseable version string is treated as pre-1.3 (every shipped version that wrote
    // presets predates 1.3, so the failure mode of a mangled stamp is the migration running —
    // correct for all presets that exist today).
    presetManager.setMigrationCallback (
        [] (juce::DynamicObject& parameters, const juce::String& presetVersion)
        {
            const auto tokens = juce::StringArray::fromTokens (presetVersion, ".", {});
            const int  major  = tokens.size() > 0 ? tokens[0].getIntValue() : 0;
            const int  minor  = tokens.size() > 1 ? tokens[1].getIntValue() : 0;

            if (major > 1 || (major == 1 && minor >= 3))
                return;

            const auto rescale = [&parameters] (const char* id, float factor)
            {
                const juce::Identifier key { id };

                if (parameters.hasProperty (key))
                    parameters.setProperty (key,
                        juce::jlimit (0.0f, 1.0f,
                                      static_cast<float> (parameters.getProperty (key)) * factor));
            };

            rescale ("rolloff", 1.0f / 3.0f);
            rescale ("width",   1.0f / 2.0f);
            rescale ("blur",    1.0f / 3.0f);
        });

    // ── THE SIX FACTORY PRESETS (Phase 4.1, P92) ───────────────────────────────────────────
    //
    // FROM THE EDITOR, NEVER THE PROCESSOR. Every one of ~20 precedent plugins calls this from the
    // processor constructor with a processor-owned manager; O-Octagon's manager is editor-owned, so
    // there is nothing to copy — and editor-side is STRICTLY BETTER than the precedent here. The
    // module's own constructor defers directory creation "to avoid file I/O during AU validation",
    // and an editor-side call keeps EVERY write off the headless scan path: auval and pluginval
    // construct processors, not editors.
    //
    // It is cheap to call every time. WR-04's .factory-version sentinel returns early once the
    // files match JucePlugin_VersionString — which is also an AUTHORING TRAP worth knowing about:
    // the second and every later edit to the definitions writes NOTHING until
    // ~/Library/O-Octagon/Presets/Factory/ is deleted. The symptom reads as a code bug and is not
    // one. Probe CP deletes that directory and calls this itself for the same reason.
    //
    // Factory/ only. User/ is never touched by anything on this path.
    presetManager.initializeFactoryPresets (oo::presets::factoryDefs (processorRef.getAPVTS()));

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView; THREE-arg ctor, nullptr undoManager) --
    for (size_t i = 0; i < sliderRelays.size(); ++i)
    {
        const juce::String id { oo::params::id (static_cast<int> (i)) };

        auto* param = processorRef.getAPVTS().getParameter (id);
        jassert (param != nullptr);   // ID drift -> silently dead control

        if (param != nullptr)
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (
                    *param, *sliderRelays[i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Fixed 1100 x 720, non-resizable (CONTEXT-3.1 D7). Reflow is where a UI
    // this size breaks, and section R7 names this the largest UI in the repo.
    //
    // THE SINGLE setSize CALL, and css/styles.css must agree with it — html,
    // body and .frame all read 1100 x 720, and section 17 of
    // tests/ui_frontend_check.js diffs the three against this line. The
    // precedent records that pair diverging across three separate resizes.
    //
    // The consequence the page cannot argue with: the derived envelope for the
    // default venue is 15.60 m x 19.50 m, aspect 0.800 — PORTRAIT. Inside 1100
    // x 720 the plan is HEIGHT-bound at 448 x 560 px, which is why the layout is
    // plan-left / controls-right and why widening the window would not enlarge
    // the plan. That figure is measured on the rendered page by
    // tests/ui_layout_check.js, not asserted here.
    setSize (1100, 720);
}

// ── Destruction ─────────────────────────────────────────────────────────────
// NO LONGER `= default`. D11's first stop: closing the editor stops the ping.
//
// Without this a latched ping outlives the window that started it and keeps
// sounding for up to 120 s with NO VISIBLE CONTROL ANYWHERE — the operator has
// closed the only surface that could stop it. Graceful rather than abort(): the
// audio thread is still running, so it takes the 20 ms release and the last
// thing the hall hears is a fade rather than an edge.
OctagonEditor::~OctagonEditor()
{
    processorRef.stopVerifyPing();

    // ── v1.7.0 — CLOSING THE WINDOW DISARMS THE MONITOR, AND THAT IS A SAFETY RULE ────────────
    //
    // THE INVARIANT IS: THE FOLD CAN ONLY BE ACTIVE WHILE ITS WARNING IS VISIBLE.
    //
    // The MONITOR banner is the only thing standing between an armed session and a REALTIME
    // bounce — isNonRealtime() catches the offline path, and not persisting catches a reload, but
    // nothing catches "armed, bounced in real time, same sitting" except a human seeing the
    // banner. A closed editor has no banner, so an armed monitor behind a closed window is the
    // one configuration where the fold is running with every warning switched off.
    //
    // The cost is that a composer who closes the window while listening has to reopen it and
    // re-arm. That is the correct side to err on: the failure this prevents is a delivered file
    // with six silent channels, and the failure it causes is one extra click.
    processorRef.setMonitorArmed (false);
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OctagonEditor::paint (juce::Graphics&)
{
    // The WebView fills the editor — nothing to paint.
}

void OctagonEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
