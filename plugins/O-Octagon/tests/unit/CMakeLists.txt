# O-Octagon geometry unit target — Phase 2.1's correctness gate for R1 (the channel map),
# DSP-03 (the hull) and DSP-04 (the audience plane).
#
# juce_add_console_app + check()/exit-code, matching all twelve existing harnesses in this repo.
# THERE IS NO UNIT-TEST FRAMEWORK HERE: no Catch2, no GoogleTest, no doctest, no juce::UnitTest.
# The Catch2 references in docs/codebase/TESTING.md describe an intent that was never implemented —
# do not follow them, and do not resolve a missing symbol by adding a framework (RESEARCH-2.1 Q5).
#
# CRITICAL: the plugin CMake target is OuariconOctagon, NOT the folder name O-Octagon
# (memory pattern build_script_target_name_vs_folder — 11 of 37 plugins differ, this is one).
#
# This target deliberately links ONLY the three geometry TUs. It does NOT link PluginProcessor.cpp,
# so it needs no JucePlugin_* block, compiles in seconds, and is structurally immune to the Stage-3
# WebView editor swap.
# Off by default; enable with -DOUARICON_BUILD_TESTS=ON.

juce_add_console_app(O-Octagon-geometry-test
    PRODUCT_NAME "O-Octagon-geometry-test"
)

# ── Layer 2 codegen (COMPAT-03) ───────────────────────────────────────────────
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# JUCE_MODULES_DIR is set CACHE INTERNAL by JUCE/CMakeLists.txt:41, so it is visible here and
# resolves identically in CI (which sets JUCE_DIR at build-and-release.yml:131 and lets the root
# CMakeLists resolve it the same way). Never a path literal.
if(NOT JUCE_MODULES_DIR)
    message(FATAL_ERROR
        "JUCE_MODULES_DIR is empty. Layer 2's golden would be generated from nothing, the "
        "generated header would carry no channel orders, and the COMPAT-03 gate would pass "
        "VACUOUSLY — which is worse than not running it at all.")
endif()

set(_OCT_GOLDEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(_OCT_GOLDEN     ${_OCT_GOLDEN_DIR}/JuceChannelOrderGolden.h)
set(_OCT_GENERATOR  ${CMAKE_CURRENT_SOURCE_DIR}/../tools/gen_juce_channel_order.py)

add_custom_command(
    OUTPUT  ${_OCT_GOLDEN}
    COMMAND ${Python3_EXECUTABLE} ${_OCT_GENERATOR}
            --juce-modules ${JUCE_MODULES_DIR}
            --output       ${_OCT_GOLDEN}
    # DEPENDS the generator AND the two JUCE files it parses, so a JUCE bump regenerates the golden
    # rather than reusing a stale one — which is the whole point of deriving it from source.
    DEPENDS ${_OCT_GENERATOR}
            ${JUCE_MODULES_DIR}/juce_audio_basics/buffers/juce_AudioChannelSet.h
            ${JUCE_MODULES_DIR}/juce_audio_basics/buffers/juce_AudioChannelSet.cpp
    COMMENT "O-Octagon Layer 2: deriving the JUCE channel-order golden from parsed JUCE source"
    VERBATIM
)

target_sources(O-Octagon-geometry-test PRIVATE
    main.cpp
    ${_OCT_GOLDEN}

    # Reach into the plugin source tree — share the code, not a static lib. PluginProcessor.cpp is
    # deliberately absent and must stay absent.
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Data/VenueModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/DSP/ChannelMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/DSP/ConvexHull2D.cpp

    # Phase 3.2 — .venue file I/O. juce::File, ValueTree and createXml() ALL live inside the three
    # modules linked below, so this TU joins the FAST target and the narrow link line survives
    # unchanged. That is what puts UI-01 criterion 3(a) — probe BN's 42-value bit-compare through
    # the same functions the chooser completions call — in the seconds-to-build target rather than
    # behind a plugin. Gate 11 re-verifies the link line rather than trusting this comment.
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Data/VenueFile.cpp

    # Phase 2.2. Both compile against <cmath> and Vec.h only — DbapSolver takes RAW inputs rather
    # than a VenueModel (PLAN-2.2 P19) and SourceShaper takes a POD VenueSnapshot, so neither drags
    # juce_audio_processors in and the narrow link line below survives intact. GainStage.cpp is
    # deliberately ABSENT: it needs SmoothedValue and AudioBuffer, and it is exercised by the render
    # harness instead.
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/DSP/DbapSolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/DSP/SourceShaper.cpp

    # Phase 3.3 — TWO TUs JOIN AT ONCE, WHICH IS WHEN A NARROW LINK LINE QUIETLY WIDENS.
    #
    # FieldSampler.cpp reaches DbapSolver.h, HullProcessor.h, ConvexHull2D.h, VenueGeometry.h,
    # Vec.h and VenueSnapshot.h — every one of which is already in this target or is header-only,
    # so UI-04 criterion 1's twenty-point comparison against a direct solve runs in the
    # SECONDS-TO-BUILD target rather than behind a plugin. That is the same move P56 made for
    # VenueFile.cpp at 3.2.
    #
    # SceneModel.cpp reaches ConvexHull2D.h, Vec.h and juce_data_structures — the last of which
    # VenueModel.cpp already pulls in. It carries no processor reference, which is what lets
    # FUNC-06/2's PERMUTATION probe be eight coordinates rather than a constructed plugin.
    #
    # NEITHER pulls juce_dsp or juce_audio_processors, and GATE 11 RE-VERIFIES THE LINK LINE
    # BELOW rather than trusting these two paragraphs.
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/DSP/FieldSampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Data/SceneModel.cpp
)

target_include_directories(O-Octagon-geometry-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source
    ${CMAKE_CURRENT_SOURCE_DIR}/../fixtures      # DbapReferenceFixture.h (committed, Phase 2.2)
    ${_OCT_GOLDEN_DIR}
)

target_compile_definitions(O-Octagon-geometry-test PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0

    # Phase 2.2 instrumentation (PLAN-2.2 P20). The plugin target NEVER defines this, so in the
    # shipping binary the four counter objects do not exist and countedPow() collapses to std::pow.
    # The lever is the same one already used for JUCE_WEB_BROWSER=0 above.
    OOCTAGON_INSTRUMENT=1
)

# No juce_audio_processors, no juce_gui_*: the geometry TUs include only juce_audio_basics (for
# AudioChannelSet) and juce_data_structures (for ValueTree). Keeping the link line this narrow is
# what makes the "no PluginProcessor.cpp" property structural rather than a convention.
target_link_libraries(O-Octagon-geometry-test
    PRIVATE
        juce::juce_audio_basics
        juce::juce_core
        juce::juce_data_structures
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# ── The DBAP fixture's own gate (PLAN-2.2 P22) ────────────────────────────────
# tests/fixtures/DbapReferenceFixture.h is COMMITTED, not generated at build time: unlike the Layer-2
# golden it tracks a fixed published equation set (the 2011-04-14 DBAP revision) rather than a moving
# one (JUCE), and regenerating it every build would make a solver change plus a matching generator
# change agree silently with nothing in the diff.
#
# This target proves the committed file still matches its generator. EXCLUDE_FROM_ALL, because it is
# a review aid rather than a build step — running it is a deliberate act.
add_custom_target(O-Octagon-dbap-fixture-check
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/../tools/gen_dbap_reference.py
            --output ${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/DbapReferenceFixture.h
            --check
    COMMENT "O-Octagon: checking the committed DBAP reference fixture against its generator"
    VERBATIM
)

set_target_properties(O-Octagon-dbap-fixture-check PROPERTIES EXCLUDE_FROM_ALL TRUE)
