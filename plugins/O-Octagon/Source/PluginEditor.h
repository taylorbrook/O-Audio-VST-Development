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

    O-Octagon — Plugin Editor (Stage 3 Phase 3.1)

    The performance surface. A WebView editor replaces the Stage-1 generic one:
    two screens (Room built, Venue a placeholder until 3.2), the Room plan drawn
    from the plugin's own geometry, and all 17 musical parameters bound two-way.

    ── ONE RELAY LIST ────────────────────────────────────────────────────────
    All 17 parameters are juce::AudioParameterFloat (RESEARCH-3.1 F1), so there
    is one relay vector and one attachment vector, and no combo or toggle
    machinery at all. The relay-TYPE split that bit O-ReverseDelay three
    separate times — grainShape, freeze, sourceMode, each a relay that attaches
    without error and then produces a control that never updates — is
    STRUCTURALLY ABSENT here and stays absent while the set stays all-float.

    ── NATIVE-FUNCTION SURFACE IS EXACTLY THIRTEEN (PLAN-3.2 P65) ───────────
      read:
        getParameterDefaults   once at init          (dblclick reset)
        getVenueGeometry       init, then on venueGen change. ONE call, and at
                               3.2 it carries all 42 venue values: the eight
                               trims ride INSIDE their speaker object and the
                               two rake heights are their own
        getStatus              polled at 2 Hz  (SAFE + MAP banners, venueGen)
        getPresetList · getCurrentPreset · getPingState
      write:
        setVenue               all 42 in ONE call, VALIDATED BEFORE APPLIED
        saveVenue · loadVenue  FileChooser::launchAsync -> oo::venuefile
        savePreset · loadPreset
        startPing · stopPing

    THE COUNT IS LOAD-BEARING, and tests/ui_frontend_check.js section 3
    grep-diffs it against the DERIVED page-module registry in BOTH directions,
    plus the ui-stub whitelist as a third set. Its literal moved 3 -> 13 at this
    phase and FAILED LOUDLY until all thirteen existed in all three places,
    which is the only way a count assertion is worth anything.

    An unregistered fn is a silently dead control that passes build, auval AND
    pluginval (pattern_webview_native_fn_bridge_gap) — and in JUCE 8 it is worse
    than that name suggests: an invocation naming an unregistered function hits
    `jassertfalse; return;` with NO completion, so in Release the promise never
    settles at all (juce_WebBrowserComponent.cpp:306-312, RESEARCH-3.2 N4).

    Keeping the surface small per phase is the whole argument for splitting
    Stage 3 into three: a grep-diff gate that runs against 3, then 13, catches
    things a gate against 70 does not.

    ── NO UI STATE DEPENDS SOLELY ON A COMPLETION (N4 / P64) ────────────────
    emitCompletionEvent does jassert(owner.isVisible()) and then calls
    emitEventIfBrowserIsVisible, which DROPS the event when the component is
    hidden (:336-344, :607-611). No error, no rejection, no log. And because the
    options below include withKeepPageLoadedWhenBrowserIsHidden(), the page
    survives being hidden and is still running when the promise it abandoned
    would have resolved. Every write here therefore treats its completion as
    ADVISORY; the authoritative effect arrives on the venueGen poll.

    getVenueGeometry is ONE call and not three because three admit a TORN READ —
    an envelope from venue A composited with glyphs from venue B. That is the
    identical hazard ARCHITECTURE section 7.2 guards on the audio thread by
    acquiring one snapshot per control block, and which P16 fixed at Phase 2.2
    by stamping the generation inside the payload.

    ── THIS HEADER IS INCLUDED ONLY FROM INSIDE #if JUCE_WEB_BROWSER ────────
    tests/render-harness builds PluginProcessor.cpp with JUCE_WEB_BROWSER=0 and
    no editor sources, under which WebBrowserComponent's types do not exist. 32
    probes die silently without that guard
    (pattern_render_harness_breaks_on_webview_editor). PluginEditor.cpp is in
    target_sources(OuariconOctagon) and MUST NEVER be added to
    tests/render-harness/CMakeLists.txt; section 11 of the static gate asserts
    the absence.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Data/VenueFile.h"
#include "DSP/FieldSampler.h"   // Phase 3.3 — UI-04's grid, sampled on the message thread

// THE HEADER ONLY. preset-manager.js is deliberately NOT vendored (PLAN-3.2 P58):
// its constructor wires TEN native functions unconditionally, any one of which
// C++ does not register becomes an N4-class never-settling promise, and
// createPresetBar() writes container.innerHTML and then queries its own injected
// markup (pattern_js_state_updater_overwrites_html_labels). The FUNC-05
// guarantee lives entirely in the C++: applyPresetJson iterates
// processor.getParameters() and resolves via parameters.getParameter(id), so it
// can NEVER walk apvts.state's children and can never reach the VENUE node.
//
// DO NOT EDIT modules/persistence/preset-manager/. Four other plugins depend on
// it and this boundary is not the place to move a shared module — N5's fix lives
// at O-Octagon's call site instead.
#include "OuariconPresetManager.h"

class OctagonEditor : public juce::AudioProcessorEditor
{
public:
    explicit OctagonEditor (OOctagonProcessor&);
    ~OctagonEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Serves the embedded UI files. The callback receives a BARE PATH — there is
    // no scheme or host to strip, and the scheme differs per platform
    // (critical_webview_resource_provider_and_schemes).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OOctagonProcessor& processorRef;

    /** FUNC-05. The module's C++ side ONLY.

        ── §27's ASSERTION CHANGED SHAPE AT PHASE 3.3, AND THAT WAS PLANNED (D17 / P80) ──────────
        `setCustomStateCallbacks` is the ONE path by which a preset can reach non-parameter state.
        Through 3.2 the strongest available form of FUNC-05 criterion 1 was that the symbol appeared
        in ZERO of O-Octagon's source files — a single grep. At 3.3 EXACTLY ONE registration becomes
        legitimate, for `SCENES`, so the gate now asserts "exactly one, and its body touches only
        SCENES" instead of "none". That is why FUNC-06 criterion 5 RE-RUNS the 42-value venue
        bit-compare (probe CL) rather than inheriting 3.2's result: the tree shape the guarantee
        holds over has changed.

        A preset WITHOUT scenes leaves the slots untouched rather than clearing them —
        `applyPresetJson` calls `customLoad` only when the "customState" property exists
        (OuariconPresetManager.h:346-349), verified in module source.
    */
    OuariconPresetManager presetManager;

    // FUNC-06's WRITE lives on the PROCESSOR, as OOctagonProcessor::applySceneWeights — see the
    // recorded deviation from P78 in its declaration. In one C++ function with one call site (the
    // `applyScene` native function below), which is what D18's argument actually asks for; on the
    // processor rather than here so that PLAN-3.3's own probe table can keep CI a HARNESS probe,
    // because the render harness never compiles this file.
    //
    // WHAT REMAINS HERE IS THE REFUSAL, and it belongs here: emptiness is a property of the SCENE,
    // which is resolved in `applyScene`, whereas the write only ever sees eight numbers.

    /** MUST OUTLIVE launchAsync. A FileChooser destroyed while its native modal is open takes the
        completion with it; a stack-local here would be a use-after-free with a dialog still on
        screen. One member, reused by both operations — they cannot be open at once. */
    std::unique_ptr<juce::FileChooser> venueChooser;

    /** UI-04's field, sampled on the MESSAGE THREAD (Q1: `dbap::solve` is a free function with no
        state, no allocation and no JUCE, so there is no second solver instance to need).

        A MEMBER RATHER THAN A LOCAL, because it carries the recompute counter UI-04 criterion 2
        asserts against — a counter that reset on every call would report 1 forever and the
        criterion would be satisfied by a function that recomputed every frame. */
    oo::FieldSampler fieldSampler;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView still alive)
    // Wrong order = release-build crash on plugin reload, because an attachment
    // would outlive the WebView and call into a freed component. This is a
    // destruction-order requirement, not a style preference
    // (juce8-critical-patterns section 3).
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS — 17, one per APVTS parameter, built by looping
    //    oo::params::id(i) rather than from a second transcribed list. That
    //    table beside the enum in GainStage.h is already THE single mapping
    //    between the parameter order and the APVTS ids; a hand-written copy here
    //    would be pattern_test_fixture_mirrors_drift_silently with a
    //    static_assert two files away that could not fire.
    std::vector<std::unique_ptr<juce::WebSliderRelay>> sliderRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctagonEditor)
};
