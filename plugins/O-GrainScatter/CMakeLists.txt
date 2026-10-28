cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OuariconGrainScatter
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuGS
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-GrainScatter${OUARICON_DEV_SUFFIX}"
    VERSION 3.4.0
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

target_sources(OuariconGrainScatter
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(OuariconGrainScatter
    PRIVATE
        Source
)

target_link_libraries(OuariconGrainScatter
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

juce_generate_juce_header(OuariconGrainScatter)

# v3.3.0: grain-engine 1.1.0 for the band-limited reader (BandlimitedReader.h).
# Only that header is included; the local CounterRng / GrainScheduler / GrainPool
# copies in Source/dsp stay (global namespace, no clash with ouaricon::grain).
ouaricon_add_module(OuariconGrainScatter grain-engine)

# WebView UI Resources
juce_add_binary_data(OuariconGrainScatter_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/app.js
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        # v2.9.0 (R5) - bundled EB Garamond, direct embed from the shared
        # module; served by four getResource() branches in PluginEditor.cpp.
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/css/eb-garamond.css
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2
)

target_link_libraries(OuariconGrainScatter
    PRIVATE
        OuariconGrainScatter_UIResources
)

target_compile_definitions(OuariconGrainScatter
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-GrainScatter test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory (scripts/param-dump). A regex over
    # createParameterLayout() is not authoritative: IDs can be concatenated and
    # parameters can come from a factory lambda. Only a walk of
    # AudioProcessor::getParameters() on a constructed processor sees the real set.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(OuariconGrainScatter ${CMAKE_CURRENT_SOURCE_DIR}/Source)

    # v3.1.0 block-size null gate: 32 / 512 / variable blocks null to <= -120 dBFS
    # across free, sync (repeats + stutter gate), spatial, freeze and deep pitch.
    # The scheduler negative control compiles the v3.0.5 scheduler from the backup.
    ouaricon_add_processor_console(OuariconGrainScatter ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/blocksize-null/main.cpp blocksize-null)
    set(_gs_baseline ${CMAKE_SOURCE_DIR}/backups/O-GrainScatter/v3.0.5/Source/dsp)
    if(EXISTS ${_gs_baseline}/GrainScheduler.h)
        target_compile_definitions(O-GrainScatter-blocksize-null PRIVATE
            GS_BASELINE_DSP_DIR=1
            "GS_BASELINE_SCHEDULER_H=\"${_gs_baseline}/GrainScheduler.h\"")
    endif()

    # v3.2.0 density compensation + freeze jitter gate. The -baseline twin compiles the
    # same driver against the v3.1.0 backup (wet tanh, plain freeze spread) as the
    # negative control and dumps the sparse render the current build nulls against:
    #   O-GrainScatter-density-gain-baseline <dir> && O-GrainScatter-density-gain <dir>
    ouaricon_add_processor_console(OuariconGrainScatter ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/density-gain/main.cpp density-gain)
    set(_gs_density_base ${CMAKE_SOURCE_DIR}/backups/O-GrainScatter/v3.1.0/Source)
    if(EXISTS ${_gs_density_base}/PluginProcessor.cpp)
        ouaricon_add_processor_console(OuariconGrainScatter ${_gs_density_base}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/density-gain/main.cpp density-gain-baseline)
        # The helper compiles the plugin target's own sources; point them at the backup.
        get_target_property(_gs_db_srcs O-GrainScatter-density-gain-baseline SOURCES)
        list(TRANSFORM _gs_db_srcs REPLACE "^${CMAKE_CURRENT_SOURCE_DIR}/Source/" "${_gs_density_base}/")
        set_property(TARGET O-GrainScatter-density-gain-baseline PROPERTY SOURCES ${_gs_db_srcs})
        target_compile_definitions(O-GrainScatter-density-gain-baseline PRIVATE GS_DENSITY_BASELINE=1)
    endif()

    # v3.3.0: the sparse row's reference is this build with density compensation forced
    # off (the band-limited reader changed every grain's samples, so v3.1.0 no longer
    # nulls). Same driver, reference mode (dumps, no verdicts):
    #   O-GrainScatter-density-gain-compoff <dir> && O-GrainScatter-density-gain <dir>
    ouaricon_add_processor_console(OuariconGrainScatter ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/density-gain/main.cpp density-gain-compoff)
    target_compile_definitions(O-GrainScatter-density-gain-compoff PRIVATE
        GS_DENSITY_BASELINE=1 GS_TEST_DENSITY_COMP_OFF=1)

    # v3.3.0 alias gate (band-limited reader). The -v320 twin compiles the same driver
    # against v3.2.0 (commit 9c6c22d0, extracted with git archive at configure time)
    # as the negative control: it must FAIL the alias rows.
    ouaricon_add_processor_console(OuariconGrainScatter ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/alias-gate/main.cpp alias-gate)
    set(_gs_v320_root ${CMAKE_BINARY_DIR}/gs-v320-9c6c22d0)
    set(_gs_v320_src ${_gs_v320_root}/plugins/O-GrainScatter/Source)
    if(NOT EXISTS ${_gs_v320_src}/PluginProcessor.cpp)
        file(MAKE_DIRECTORY ${_gs_v320_root})
        execute_process(
            COMMAND git -C ${CMAKE_SOURCE_DIR} archive 9c6c22d0 plugins/O-GrainScatter/Source
            COMMAND tar -x -C ${_gs_v320_root}
            RESULT_VARIABLE _gs_v320_rc)
    endif()
    if(EXISTS ${_gs_v320_src}/PluginProcessor.cpp)
        ouaricon_add_processor_console(OuariconGrainScatter ${_gs_v320_src}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/alias-gate/main.cpp alias-gate-v320)
        get_target_property(_gs_ag_srcs O-GrainScatter-alias-gate-v320 SOURCES)
        list(TRANSFORM _gs_ag_srcs REPLACE "^${CMAKE_CURRENT_SOURCE_DIR}/Source/" "${_gs_v320_src}/")
        set_property(TARGET O-GrainScatter-alias-gate-v320 PROPERTY SOURCES ${_gs_ag_srcs})
        target_compile_definitions(O-GrainScatter-alias-gate-v320 PRIVATE GS_ALIAS_V320=1)
    endif()

    # v3.4.0 Grain Skip + Shape Tilt gate. The -v330 twin compiles the same driver
    # against v3.3.0 (commit d920bedd, extracted with git archive at configure time)
    # and dumps the reference renders + a v3.3.0 session that the defaults must match
    # bit for bit:
    #   O-GrainScatter-skip-tilt-v330 <dir> && O-GrainScatter-skip-tilt <dir>
    ouaricon_add_processor_console(OuariconGrainScatter ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/skip-tilt/main.cpp skip-tilt)
    set(_gs_v330_root ${CMAKE_BINARY_DIR}/gs-v330-d920bedd)
    set(_gs_v330_src ${_gs_v330_root}/plugins/O-GrainScatter/Source)
    if(NOT EXISTS ${_gs_v330_src}/PluginProcessor.cpp)
        file(MAKE_DIRECTORY ${_gs_v330_root})
        execute_process(
            COMMAND git -C ${CMAKE_SOURCE_DIR} archive d920bedd plugins/O-GrainScatter/Source
            COMMAND tar -x -C ${_gs_v330_root}
            RESULT_VARIABLE _gs_v330_rc)
    endif()
    if(EXISTS ${_gs_v330_src}/PluginProcessor.cpp)
        ouaricon_add_processor_console(OuariconGrainScatter ${_gs_v330_src}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/skip-tilt/main.cpp skip-tilt-v330)
        get_target_property(_gs_st_srcs O-GrainScatter-skip-tilt-v330 SOURCES)
        list(TRANSFORM _gs_st_srcs REPLACE "^${CMAKE_CURRENT_SOURCE_DIR}/Source/" "${_gs_v330_src}/")
        set_property(TARGET O-GrainScatter-skip-tilt-v330 PROPERTY SOURCES ${_gs_st_srcs})
        target_compile_definitions(O-GrainScatter-skip-tilt-v330 PRIVATE GS_SKIPTILT_V330=1)
    endif()
endif()
