# scala-tuning-engine module tests — standalone console apps.
#
#   cmake -S modules/tuning/scala-tuning-engine/tests -B build-scala-tuning-tests -G Ninja \
#         -DCMAKE_BUILD_TYPE=Release
#   ninja -C build-scala-tuning-tests scala-tuning-tests
#   build-scala-tuning-tests/scala-tuning-tests_artefacts/Release/scala-tuning-tests
#
# Exit 0 iff every check passes.
#
# Parity gate (module vs fork copies vs a baseline tree): pass name=dir pairs and
# run parity_gate.py over the resulting parity-<name> binaries.
#
#   -DTUNING_PARITY_TREES="baseline=/path/to/old/cpp;prism=/.../plugins/O-Prism/Source"
#   -DTUNING_TEST_SRC=/path/to/old/cpp    builds the unit tests against another
#                                         tree (negative control)

cmake_minimum_required(VERSION 3.22)
project(scala_tuning_tests VERSION 3.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT COMMAND juce_add_console_app)
    if(DEFINED ENV{JUCE_DIR})
        add_subdirectory($ENV{JUCE_DIR} JUCE)
    elseif(WIN32)
        add_subdirectory(C:/JUCE JUCE)
    else()
        add_subdirectory(/Users/taylorbrook/JUCE JUCE)
    endif()
endif()

function(add_tuning_app target main_source tree)
    juce_add_console_app(${target} PRODUCT_NAME "${target}")
    target_sources(${target} PRIVATE
        ${main_source}
        ${tree}/TuningEngine.cpp
        ${tree}/ScaleGenerator.cpp
        ${tree}/EmbeddedTunings.cpp
        ${tree}/TuningExporter.cpp)
    # The stub JuceHeader.h comes first; the tree supplies the module headers.
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${tree})
    target_compile_definitions(${target} PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0)
    target_link_libraries(${target}
        PRIVATE juce::juce_core juce::juce_data_structures
        PUBLIC juce::juce_recommended_config_flags)
endfunction()

set(TUNING_TEST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../cpp" CACHE PATH "Tree the unit tests build against")
add_tuning_app(scala-tuning-tests scala_tuning_tests.cpp "${TUNING_TEST_SRC}")

add_tuning_app(parity-module parity_dump.cpp "${CMAKE_CURRENT_SOURCE_DIR}/../cpp")
foreach(entry IN LISTS TUNING_PARITY_TREES)
    string(REPLACE "=" ";" pair "${entry}")
    list(GET pair 0 name)
    list(GET pair 1 dir)
    add_tuning_app(parity-${name} parity_dump.cpp "${dir}")
endforeach()
