cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-Prism
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuPr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Prism${OUARICON_DEV_SUFFIX}"
    VERSION 1.28.1
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(O-Prism
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/FactoryPresets.cpp
        Source/PrismVoice.cpp
        Source/TuningEngine.cpp
        Source/ScaleGenerator.cpp
        Source/EmbeddedTunings.cpp
        Source/TuningExporter.cpp
        Source/dsp/WavetableGenerator.cpp
        Source/dsp/WavetableFactory.cpp
        Source/dsp/WavetableOscillator.cpp
        Source/dsp/WavetableImporter.cpp
        Source/dsp/UserWavetableManager.cpp
        Source/dsp/SubOscillator.cpp
        Source/dsp/NoiseGenerator.cpp
        Source/dsp/SVFFilter.cpp
        Source/dsp/DistortionProcessor.cpp
        Source/dsp/DelayProcessor.cpp
        Source/dsp/ReverbProcessor.cpp
        Source/dsp/EQProcessor.cpp
        Source/dsp/EnsembleChorus.cpp
        Source/dsp/LFO.cpp
        Source/dsp/ModulationMatrix.cpp
        Source/dsp/WavetableEditor.cpp
)

# Include paths
target_include_directories(O-Prism
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-Prism
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

# Generate JuceHeader.h (JUCE 8 requirement — after target_link_libraries)
juce_generate_juce_header(O-Prism)

# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-Prism note-expression)

# WebView UI Resources
juce_add_binary_data(O-Prism_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/js/wavetable-editor.js
        Source/ui/public/css/wavetable-editor.css
)

target_link_libraries(O-Prism
    PRIVATE
        O-Prism_UIResources
)

# Compile definitions
target_compile_definitions(O-Prism
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-Prism test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory. O-Prism's 81 parameter IDs are
    # string-concatenated (Source/PluginProcessor.cpp:76-87) and appear nowhere
    # in the source as literals — the runtime walk is the only way to enumerate
    # them.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(O-Prism ${CMAKE_CURRENT_SOURCE_DIR}/Source)

    # v1.25.0 — Geometry bank smoke + preset-migration gate. Constructs the
    # REAL processor (same derived-identity console as param-dump), then:
    # 36 factory tables with 28-35 in "Geometry"; each embedded table finite,
    # DC-free, global peak 1.0, level-0 int16 re-encoding SHA-256 == the
    # baker's manifest.json (proves the embed round-trip); a C4 note on each
    # table renders finite, non-silent, at 261.63 Hz ± 1 cent with harmonic 1
    # within 6 dB of the strongest partial; a preset saved under the 0-27
    # range (version "1.0.0" or absent) re-decodes to the SAME table name
    # through the migration hook; every factory preset loads with the table
    # they were authored with.
    #     ninja O-Prism-geometry-check && ./build/plugins/O-Prism/O-Prism-geometry-check_artefacts/O-Prism-geometry-check scripts/geometry-wavetables/manifest.json
    # Exit code = number of failed checks.
    ouaricon_add_processor_console(O-Prism ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/geometry_check.cpp geometry-check)

    # v1.26.1 — pitch-bend / state-restore / zero-channel gate. Covers the
    # v1.26.0 review findings that are host-interaction sequences and so had
    # no automated gate: CR-03 (a released note re-striking detuned), the
    # startNote pitch-wheel seed, WR-09 (zero-channel block dereferencing
    # channel 0) and WR-03 (setStateInformation inheriting the previous
    # session's user wavetable).
    #     ninja O-Prism-bend-state-check && ./build/plugins/O-Prism/O-Prism-bend-state-check_artefacts/O-Prism-bend-state-check
    # Exit code = number of failed checks.
    ouaricon_add_processor_console(O-Prism ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/bend_state_check.cpp bend-state-check)

    # v1.27.0 — wavetable copy-on-write gate (CR-01). startEditing() publishes
    # the editor's working table to the oscillator, so every editor operation
    # used to write into the buffer the voices were reading. Each of the seven
    # mutating ops must now publish a NEW table, leave the displaced buffer
    # byte-identical, and still actually change something. Plus a render-thread
    # smoke with a chord held while edits hammer the table.
    #     ninja O-Prism-wavetable-cow-check && ./build/plugins/O-Prism/O-Prism-wavetable-cow-check_artefacts/O-Prism-wavetable-cow-check
    # Exit code = number of failed checks.
    ouaricon_add_processor_console(O-Prism ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/wavetable_cow_check.cpp wavetable-cow-check)

    # v1.27.0 — regression gate for the findings the CR-01/CR-02/WR-07 fixes
    # introduced. REG-01: 200 edits with the audio thread idle must publish
    # from at most 2 addresses (pre-fix: 201, none of them ever freed), and the
    # dirty-frame resync that makes the rotation cheap must lose neither a
    # frame nor a mipmap level. REG-02: setStateInformation must close the
    # editing session, so a stale editor op cannot republish over the restored
    # state. REG-03: the routing predicate the per-sample exp2 hoist relies on
    # must mirror evaluate()'s skip conditions exactly.
    #     ninja O-Prism-edit-rotation-check && ./build/plugins/O-Prism/O-Prism-edit-rotation-check_artefacts/Release/O-Prism-edit-rotation-check
    # Exit code = number of failed checks.
    ouaricon_add_processor_console(O-Prism ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/edit_rotation_check.cpp edit-rotation-check)

    # v1.27.1 — modulation finiteness gate. getSampleRate() is read in exactly
    # one place in the plugin (processBlock -> advanceGlobalLfoPhases) and is
    # set by setPlayConfigDetails, NOT prepareToPlay; a gate that omits it
    # renders at rate 0, where rateHz/0 is inf and inf - floor(inf) is a NaN
    # that never clears. That poisoned the global LFO phases, hence the five
    # processor-level mod destinations, and produced the bogus "FX mod NaN"
    # finding in the v1.27.0 CHANGELOG. Sweeps all 25 destinations and all 10
    # sources under the host contract, and carries its own negative control:
    # an instance that never declares its rate must still render finite.
    #     ninja O-Prism-fx-mod-nan-check && ./build/plugins/O-Prism/O-Prism-fx-mod-nan-check_artefacts/Release/O-Prism-fx-mod-nan-check
    # Exit code = number of failed checks.
    ouaricon_add_processor_console(O-Prism ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fx_mod_nan_check.cpp fx-mod-nan-check)

    # v1.27.2 — free-run LFO sub-block gate (WR-01). juce::Synthesiser splits
    # the block at MIDI events and calls renderVoices() per sub-block; the
    # voices re-seed their free-running LFOs from globalLfoPhase on every one
    # of those calls, so advancing the phase once per processBlock rewound the
    # LFO k times and advanced it through only the last sub-block. The
    # observable is that MIDI carrying no modulation (CC#20 — inert on both the
    # processor scan and PrismVoice::controllerMoved) must not change the
    # sound: a held note with a free-running LFO renders identically whether
    # the block goes through whole or in 16 pieces.
    #     ninja O-Prism-lfo-subblock-check && ./build/plugins/O-Prism/O-Prism-lfo-subblock-check_artefacts/Release/O-Prism-lfo-subblock-check
    # Exit code = number of failed checks.
    ouaricon_add_processor_console(O-Prism ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/lfo_subblock_check.cpp lfo-subblock-check)

    # v1.28.0 — DSP-quality gate for the five remaining Warning-tier findings.
    # WR-06 (two clock-seeded juce::Random streams plus a `this`-seeded phase
    # LCG made any render non-reproducible) is asserted at processor level: two
    # instances, held alive at once so they cannot share a heap address, must
    # render bit-identically with noise, S&H and random start phases all
    # engaged. WR-02 pins getLevelSelectFrequency() — the quantity the mipmap
    # fix introduces — for every warp mode and both sides of the 0.001 Sync
    # floor. WR-04, WR-05 and WR-08 each carry a BUILT-IN negative control: the
    # pre-fix arithmetic is reproduced inside the gate (right-line-only mono
    # delay, the unwarped 44.1 kHz pink coefficients at 96 kHz, the linear-Hz
    # glide one-pole) and must fail the same assertion the fixed code passes,
    # so none of the three needs a source revert to be falsifiable.
    #     ninja O-Prism-dsp-quality-check && ./build/plugins/O-Prism/O-Prism-dsp-quality-check_artefacts/Release/O-Prism-dsp-quality-check
    # Exit code = number of failed checks.
    ouaricon_add_processor_console(O-Prism ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/dsp_quality_check.cpp dsp-quality-check)
endif()
