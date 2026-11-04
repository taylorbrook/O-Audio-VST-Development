# pitch-collections module tests — standalone console app.
#
#   cmake -S modules/tuning/pitch-collections/tests -B build-pitch-collections-tests -G Ninja \
#         -DCMAKE_BUILD_TYPE=Release
#   ninja -C build-pitch-collections-tests && build-pitch-collections-tests/pitch-collections-tests_artefacts/Release/pitch-collections-tests
#
# Exit 0 iff every check passes. Built at C++17: the lowest consumer standard
# (O-GrainScatter and every processor-console target compile at gnu++17).

cmake_minimum_required(VERSION 3.22)
project(pitch_collections_tests VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
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

juce_add_console_app(pitch-collections-tests PRODUCT_NAME "pitch-collections-tests")

target_sources(pitch-collections-tests PRIVATE pitch_collections_tests.cpp)
target_include_directories(pitch-collections-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cpp)

target_compile_definitions(pitch-collections-tests PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

target_link_libraries(pitch-collections-tests
    PRIVATE
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
