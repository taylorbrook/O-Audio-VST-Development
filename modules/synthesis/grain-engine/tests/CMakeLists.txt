# grain-engine module tests — standalone console app.
#
#   cmake -S modules/synthesis/grain-engine/tests -B build-grain-engine-tests -G Ninja \
#         -DCMAKE_BUILD_TYPE=Release
#   ninja -C build-grain-engine-tests && build-grain-engine-tests/grain-engine-tests_artefacts/Release/grain-engine-tests
#
# Exit 0 iff every check passes. JUCE is found from $JUCE_DIR, else the root
# CMakeLists' platform fallbacks; only juce_core is linked (WindowLut).

cmake_minimum_required(VERSION 3.22)
project(grain_engine_tests VERSION 1.1.0 LANGUAGES C CXX)

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

juce_add_console_app(grain-engine-tests PRODUCT_NAME "grain-engine-tests")

target_sources(grain-engine-tests PRIVATE grain_engine_tests.cpp)
target_include_directories(grain-engine-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cpp)

target_compile_definitions(grain-engine-tests PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

target_link_libraries(grain-engine-tests
    PRIVATE
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
