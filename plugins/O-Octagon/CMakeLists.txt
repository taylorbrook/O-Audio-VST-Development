cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# NOTE: unlike sibling O-Orbit, O-Octagon does NOT use SAF. There is deliberately no
# add_subdirectory(libs/SAF/framework) here — DBAP is implemented in-tree (ARCHITECTURE §3).
# Copying O-Orbit's CMakeLists wholesale would silently import SAF.

juce_add_plugin(OuariconOctagon
    COMPANY_NAME             "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE              OuOc            # verified unused across all 39 existing plugins
    FORMATS                  VST3 AU Standalone
    PRODUCT_NAME             "O-Octagon${OUARICON_DEV_SUFFIX}"
    VERSION                  1.6.0           # NOT PLUGIN_VERSION — that keyword is not a JUCE
                                             # keyword, is silently ignored, and ships 1.0.0 anyway
    NEEDS_WEB_BROWSER        TRUE
    NEEDS_WEBVIEW2           TRUE
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    IS_MIDI_EFFECT           FALSE
    VST3_CATEGORIES          "Fx" "Spatial"  # JUCE hoists Fx to index 0 regardless and emits
                                             # "Fx|Spatial"; writing it first makes the source
                                             # match what actually ships (RESEARCH F6)
    AU_MAIN_TYPE             "kAudioUnitType_Effect"
    # DO NOT set PLUGIN_CHANNEL_CONFIGURATIONS — it understands channel COUNTS only, not channel
    # TYPES, and breaks surround layout detection. It is also redundant: JUCE derives the entire
    # AU channel-config set from isBusesLayoutSupported() (RESEARCH F2).
)

target_sources(OuariconOctagon
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp         # Phase 3.1 — the WebView editor.
                                        # MUST NEVER be added to tests/render-harness/CMakeLists.txt:
                                        # that target compiles PluginProcessor.cpp with
                                        # JUCE_WEB_BROWSER=0, under which WebBrowserComponent's
                                        # types do not exist
                                        # (pattern_render_harness_breaks_on_webview_editor).
        Source/Data/VenueModel.cpp
        Source/Data/VenueFile.cpp       # Phase 3.2 — .venue I/O. No processor reference, which is
                                        # what lets it join the FAST unit target too.
        Source/Data/SceneModel.cpp      # Phase 3.3 — FUNC-06. The six named predicates and the four
                                        # persisted user slots. No processor reference either, so it
                                        # joins the unit target and FUNC-06/2's PERMUTATION probe
                                        # runs in seconds rather than behind a plugin.
        Source/DSP/ChannelMap.cpp
        Source/DSP/ConvexHull2D.cpp
        Source/DSP/DbapSolver.cpp       # Phase 2.2
        Source/DSP/SourceShaper.cpp     # Phase 2.2
        Source/DSP/FieldSampler.cpp     # Phase 3.3 — UI-04. Message-thread only, and it reaches
                                        # DbapSolver / HullProcessor / ConvexHull2D and nothing
                                        # else, so it joins the unit target too (gate 11).
        Source/DSP/GainStage.cpp        # Phase 2.2
        Source/DSP/VerifyPing.cpp       # Phase 3.2 — FUNC-04. Needs juce_dsp, so it joins the
                                        # render harness but NEVER the unit target (gate 11).
        # Source/DSP/Vec.h, Source/Data/VenueSnapshot.h and Source/Data/VenueGeometry.h are
        # header-only and deliberately absent.
)

# ── Phase 3.1: the WebView UI resources ──────────────────────────────────────
#
# NAMESPACE is NOT optional. A second juce_add_binary_data target in the same
# build collides with the default `BinaryData` namespace and the link fails —
# or worse, resolves to the wrong table (critical_dual_binary_data_namespace_collision).
# O-Octagon has only one today, but the namespace is set now rather than at the
# moment a second appears.
#
# NO FILENAME HERE CONTAINS A HYPHEN. juce_add_binary_data STRIPS hyphens rather
# than converting them to underscores, so `room-plan.js` would have to be reached
# as the symbol `roomplan_js` (critical_binary_data_strips_hyphens). The files are
# authored hyphen-free so there is no transform to remember.
#
# check_native_interop.js is embedded and SERVED. Omitting it can hang the page
# (juce8-critical-patterns section 13) — js/juce/index.js imports it directly.
# Both JUCE files are byte-exact copies of
# ${JUCE_DIR}/modules/juce_gui_extra/native/javascript/.
juce_add_binary_data(OuariconOctagon_UIResources
    NAMESPACE   UIBinaryData
    HEADER_NAME UIBinaryData.h
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/i18n.js      # v1.6.0. The hover-help copy table, English + French.
                                         # No hyphen in the name: a hyphenated file would have to
                                         # be reached as a symbol with the hyphen STRIPPED.
        Source/ui/public/js/roomplan.js
        Source/ui/public/js/venue.js     # Phase 3.2. Section 21 of ui_frontend_check.js DERIVES the
                                         # page-module list by READING THAT DIRECTORY and asserts it
                                         # equals this set, so a module added on disk and forgotten
                                         # here fails a gate rather than 404-ing at runtime.
                                         # (The glob is deliberately not written out here: section 9
                                         # scans this block for embedded paths and would harvest it.)
        # Phase 3.3. Four modules, and NOT ONE OF THEM HAD TO BE REMEMBERED IN SEVEN PLACES: P51's
        # derived registry picks them up automatically and §21 asserts set-equality against exactly
        # this list. That vacuity class has been caught six times in this project and this is the
        # first phase where closing it cost nothing.
        Source/ui/public/js/scenes.js
        Source/ui/public/js/meters.js
        Source/ui/public/js/field.js
        Source/ui/public/js/elevation.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

target_link_libraries(OuariconOctagon PRIVATE OuariconOctagon_UIResources)

target_include_directories(OuariconOctagon
    PRIVATE
        Source
        # Phase 3.2 — the preset-manager module's C++ side ONLY (PLAN-3.2 P58). Its js/ directory is
        # deliberately NOT vendored, and section 27 of the static gate asserts that absence together
        # with the absence of any setCustomStateCallbacks registration.
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
)

target_link_libraries(OuariconOctagon
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Order is load-bearing: juce_generate_juce_header() AFTER target_link_libraries and
# BEFORE target_compile_definitions (juce8-critical-patterns §1).
juce_generate_juce_header(OuariconOctagon)

target_compile_definitions(OuariconOctagon
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        # Set now although Stage 1 has no WebView. The flags are inert with a generic editor, and
        # having JUCE_WEB_BROWSER defined from the start is what lets the Phase 2.2 render harness
        # guard createEditor with #if JUCE_WEB_BROWSER BEFORE the Stage 3 swap rather than after it
        # (pattern_render_harness_breaks_on_webview_editor).
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# ── Test targets (Phase 2.1) ──────────────────────────────────────────────────
# One option gates both, matching the repo convention across twelve existing harnesses.
#
#   O-Octagon-geometry-test  — links ONLY the three geometry TUs (no PluginProcessor.cpp)
#   O-Octagon-render-test    — links the processor; bus layouts constructed programmatically
#
# DO NOT add these to .github/workflows/build-and-release.yml: it is tag-triggered and
# secrets-bearing, and carries a standing rule against widening its trigger surface (PLAN-2.1 P13).
# That prohibition still stands and is still the reason the file below exists.
#
# The residual it left open — a JUCE bump performed without running the test target ships silently
# — is CLOSED as of Phase 4.1 (PLAN-4.1 P86), and not by overturning P13. Both targets are built
# and run per-commit by .github/workflows/ci-tests.yml, a SECRETLESS workflow with
# `permissions: contents: read`, which is orthogonal to the trigger rule rather than an exception
# to it. It also catches the stated failure mode where a tag-time gate could not: a JUCE bump is a
# COMMIT, not a tag.
#
# That job's gate is the `cmake --build` step, not the run step — tests/unit's Layer-2 golden is
# compared by static_assert during the compile, so a JUCE that reorders AudioChannelSet fails to
# BUILD. Do not "optimise" it into executing a cached binary.
option(OUARICON_BUILD_TESTS "Build O-Octagon test targets" OFF)

if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/unit)
    add_subdirectory(tests/render-harness)
endif()
