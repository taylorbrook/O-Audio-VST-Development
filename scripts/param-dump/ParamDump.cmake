# ─────────────────────────────────────────────────────────────────────────────
# ouaricon_add_param_dump(<PluginTarget> <PluginSourceDir>)
#
# Adds a console target `<folder-name>-param-dump` that constructs the plugin
# processor via createPluginFilter() and prints a TSV inventory of
# AudioProcessor::getParameters() to stdout.
#
# Call it from plugins/<Name>/CMakeLists.txt, guarded by OUARICON_BUILD_TESTS:
#
#     option(OUARICON_BUILD_TESTS "Build O-Foo test targets" OFF)
#     if(OUARICON_BUILD_TESTS)
#         include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
#         ouaricon_add_param_dump(OFoo ${CMAKE_CURRENT_SOURCE_DIR}/Source)
#     endif()
#
# The target name is derived from the PLUGIN FOLDER, not the CMake target, so
# O-Emulator/ (target OEmulator) yields O-Emulator-param-dump — matching the
# existing render-harness convention (O-Bitrot-render-test on target OBitrot).
#
# EVERY JucePlugin_* value below is DERIVED from the properties juce_add_plugin
# set on the plugin target. None is mirrored as a literal: a mirrored fixture
# constant has drifted silently twice in this repo
# (pattern_test_fixture_mirrors_drift_silently). A property that reads empty is
# a FATAL_ERROR, never a guess.
# ─────────────────────────────────────────────────────────────────────────────

include_guard(GLOBAL)

# Read a required property off the plugin target, or fail the configure.
function(_ouaricon_pd_require_property out_var target prop)
    get_target_property(_value ${target} ${prop})

    if(NOT _value OR _value STREQUAL "${prop}-NOTFOUND")
        message(FATAL_ERROR
            "ouaricon_add_param_dump: could not read ${prop} from target '${target}'. "
            "Refusing to stamp the param-dump with a guessed value — deriving from the "
            "plugin target is the whole point of this block.")
    endif()

    set(${out_var} "${_value}" PARENT_SCOPE)
endfunction()

# A four-character JUCE code ("OuDv") -> the 0x-prefixed integer literal the
# JucePlugin_*Code macros expect. JUCE's own _juce_to_char_literal is private
# to JUCEUtils.cmake, so string(HEX) does the same job without depending on an
# upstream internal that a rename would break.
function(_ouaricon_pd_fourcc out_var code)
    string(LENGTH "${code}" _len)

    if(NOT _len EQUAL 4)
        message(FATAL_ERROR
            "ouaricon_add_param_dump: '${code}' is not a 4-character JUCE code.")
    endif()

    string(HEX "${code}" _hex)
    set(${out_var} "0x${_hex}" PARENT_SCOPE)
endfunction()

function(ouaricon_add_param_dump plugin_target plugin_source_dir)

    if(NOT TARGET ${plugin_target})
        message(FATAL_ERROR
            "ouaricon_add_param_dump: '${plugin_target}' is not a target. Call this AFTER "
            "juce_add_plugin() and juce_generate_juce_header().")
    endif()

    get_filename_component(_pd_folder "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    set(_pd_target "${_pd_folder}-param-dump")

    if(TARGET ${_pd_target})
        return()
    endif()

    # ── Identity, DERIVED ────────────────────────────────────────────────────
    _ouaricon_pd_require_property(_pd_version      ${plugin_target} JUCE_VERSION)
    _ouaricon_pd_require_property(_pd_product      ${plugin_target} JUCE_PRODUCT_NAME)
    _ouaricon_pd_require_property(_pd_company      ${plugin_target} JUCE_COMPANY_NAME)
    _ouaricon_pd_require_property(_pd_plugin_code  ${plugin_target} JUCE_PLUGIN_CODE)
    _ouaricon_pd_require_property(_pd_manu_code    ${plugin_target} JUCE_PLUGIN_MANUFACTURER_CODE)

    _ouaricon_pd_fourcc(_pd_plugin_code_hex "${_pd_plugin_code}")
    _ouaricon_pd_fourcc(_pd_manu_code_hex   "${_pd_manu_code}")

    # The hex math mirrors JUCE's own _juce_version_code()
    # ((major << 16) + (minor << 8) + patch) rather than calling it — that
    # function is private and an upstream rename would break configure.
    string(REPLACE "." ";" _pd_version_parts "${_pd_version}")
    list(LENGTH _pd_version_parts _pd_version_part_count)

    if(_pd_version_part_count LESS 3)
        message(FATAL_ERROR
            "ouaricon_add_param_dump: JUCE_VERSION '${_pd_version}' on '${plugin_target}' is "
            "not major.minor.patch.")
    endif()

    list(GET _pd_version_parts 0 _pd_ver_major)
    list(GET _pd_version_parts 1 _pd_ver_minor)
    list(GET _pd_version_parts 2 _pd_ver_patch)
    math(EXPR _pd_version_code
        "(${_pd_ver_major} << 16) + (${_pd_ver_minor} << 8) + ${_pd_ver_patch}"
        OUTPUT_FORMAT HEXADECIMAL)

    # Booleans arrive as TRUE/FALSE; the macros need 1/0. Absent means 0, which
    # is JUCE's own default, so these are NOT routed through the require helper.
    foreach(_pd_pair "JUCE_IS_SYNTH:_pd_is_synth"
                     "JUCE_NEEDS_MIDI_INPUT:_pd_midi_in"
                     "JUCE_NEEDS_MIDI_OUTPUT:_pd_midi_out"
                     "JUCE_IS_MIDI_EFFECT:_pd_midi_fx")
        string(REPLACE ":" ";" _pd_pair_parts "${_pd_pair}")
        list(GET _pd_pair_parts 0 _pd_prop)
        list(GET _pd_pair_parts 1 _pd_out)

        get_target_property(_pd_raw ${plugin_target} ${_pd_prop})

        if(_pd_raw)
            set(${_pd_out} 1)
        else()
            set(${_pd_out} 0)
        endif()
    endforeach()

    # ── Sources, DERIVED from the plugin target ──────────────────────────────
    # Same rule as the render harnesses: compile the plugin's own TUs, never a
    # copy of the list, and never the editor TU (this target sets
    # JUCE_WEB_BROWSER=0, and createEditor() is guarded on it).
    get_target_property(_pd_plugin_sources ${plugin_target} SOURCES)

    if(NOT _pd_plugin_sources)
        message(FATAL_ERROR
            "ouaricon_add_param_dump: target '${plugin_target}' has no SOURCES.")
    endif()

    get_target_property(_pd_plugin_dir ${plugin_target} SOURCE_DIR)

    set(_pd_sources "")
    set(_pd_saw_processor FALSE)

    foreach(_pd_src IN LISTS _pd_plugin_sources)
        if(NOT _pd_src MATCHES "\\.(cpp|cc|cxx|mm)$")
            continue()
        endif()

        # Editor TU: excluded by design, not by accident.
        if(_pd_src MATCHES "PluginEditor")
            continue()
        endif()

        if(NOT IS_ABSOLUTE "${_pd_src}")
            set(_pd_src "${_pd_plugin_dir}/${_pd_src}")
        endif()

        if(_pd_src MATCHES "PluginProcessor\\.cpp$")
            set(_pd_saw_processor TRUE)
        endif()

        list(APPEND _pd_sources "${_pd_src}")
    endforeach()

    if(NOT _pd_saw_processor)
        message(FATAL_ERROR
            "ouaricon_add_param_dump: no PluginProcessor.cpp in '${plugin_target}' SOURCES — "
            "createPluginFilter() would not link.")
    endif()

    # ── The target ───────────────────────────────────────────────────────────
    juce_add_console_app(${_pd_target}
        PRODUCT_NAME "${_pd_target}"
    )

    target_sources(${_pd_target} PRIVATE
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/main.cpp
        ${_pd_sources}
    )

    target_include_directories(${_pd_target} PRIVATE
        ${plugin_source_dir}
        $<TARGET_PROPERTY:${plugin_target},INCLUDE_DIRECTORIES>
    )

    # JuceHeader.h is generated by the plugin target; depend on it so it exists
    # before this target compiles.
    add_dependencies(${_pd_target} ${plugin_target})

    target_compile_definitions(${_pd_target} PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JucePlugin_Build_Standalone=1

        JucePlugin_Name="${_pd_product}"
        JucePlugin_Desc="${_pd_product}"
        JucePlugin_Manufacturer="${_pd_company}"
        JucePlugin_ManufacturerCode=${_pd_manu_code_hex}
        JucePlugin_PluginCode=${_pd_plugin_code_hex}
        JucePlugin_VersionString="${_pd_version}"
        JucePlugin_VersionCode=${_pd_version_code}
        JucePlugin_IsSynth=${_pd_is_synth}
        JucePlugin_WantsMidiInput=${_pd_midi_in}
        JucePlugin_ProducesMidiOutput=${_pd_midi_out}
        JucePlugin_IsMidiEffect=${_pd_midi_fx}
        JucePlugin_EditorRequiresKeyboardFocus=0
    )

    # The plugin's own non-JUCE PRIVATE links (vendored codecs and the like)
    # are carried across; juce_audio_plugin_client is deliberately excluded —
    # a console app must not pull in the format wrappers — and so are the
    # binary-data targets, which only the editor reads.
    get_target_property(_pd_plugin_links ${plugin_target} LINK_LIBRARIES)
    set(_pd_extra_links "")

    if(_pd_plugin_links)
        foreach(_pd_link IN LISTS _pd_plugin_links)
            if(_pd_link MATCHES "^juce::" OR _pd_link MATCHES "UIResources$")
                continue()
            endif()

            if(TARGET ${_pd_link})
                list(APPEND _pd_extra_links ${_pd_link})
            endif()
        endforeach()
    endif()

    target_link_libraries(${_pd_target}
        PRIVATE
            ${_pd_extra_links}
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_formats
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
            juce::juce_recommended_warning_flags
    )

    message(STATUS
        "${_pd_target}: version ${_pd_version} (${_pd_version_code}), code ${_pd_plugin_code} "
        "(${_pd_plugin_code_hex}) — all derived from target '${plugin_target}'")

endfunction()
