cmake_minimum_required(VERSION 3.22)

# Set deployment target before project (macOS only)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")
endif()

project(JUCEPlugins VERSION 1.0.0)

# Licensing: OFF for local dev, ON in GitHub Actions publish builds
option(OUARICON_LICENSING "Enable license checking in plugins" OFF)

# Release branding: OFF for local dev, ON in GitHub Actions publish builds
option(OUARICON_RELEASE "Use production branding (no -dev suffix)" OFF)

if(OUARICON_RELEASE)
    set(OUARICON_COMPANY_NAME "Ouaricon Audio")
    set(OUARICON_MANUFACTURER_CODE OuAu)
    set(OUARICON_DEV_SUFFIX "")
    message(STATUS "[Ouaricon] Release branding — COMPANY: Ouaricon Audio, CODE: OuAu")
else()
    set(OUARICON_COMPANY_NAME "Ouaricon Audio Development")
    set(OUARICON_MANUFACTURER_CODE OuDv)
    set(OUARICON_DEV_SUFFIX "-dev")
    message(STATUS "[Ouaricon] Dev branding — COMPANY: Ouaricon Audio Development, CODE: OuDv, SUFFIX: -dev")
endif()

if(OUARICON_LICENSING)
    message(STATUS "[Ouaricon] Licensing ENABLED — plugins will require activation")
    add_compile_definitions(OUARICON_LICENSING_ENABLED=1)

    # These must be passed via -D flags (set in GitHub Actions from repo secrets)
    # Escaped quotes so the values become C string literals: "https://..."
    if(DEFINED OUARICON_SUPABASE_URL)
        add_compile_definitions("OUARICON_SUPABASE_URL=\"${OUARICON_SUPABASE_URL}\"")
    endif()

    if(DEFINED OUARICON_SUPABASE_ANON_KEY)
        add_compile_definitions("OUARICON_SUPABASE_ANON_KEY=\"${OUARICON_SUPABASE_ANON_KEY}\"")
    endif()
else()
    message(STATUS "[Ouaricon] Licensing DISABLED — plugins run without activation")
endif()

# Add JUCE once at root (env var → platform fallback)
if(DEFINED ENV{JUCE_DIR})
    add_subdirectory($ENV{JUCE_DIR} JUCE)
elseif(WIN32)
    add_subdirectory(C:/JUCE JUCE)
else()
    add_subdirectory(/Users/taylorbrook/JUCE JUCE)
endif()

# Plugins to skip (e.g. missing platform-specific dependencies)
set(SKIP_PLUGINS "" CACHE STRING "Semicolon-separated list of plugin directory names to skip")

# Auto-discover plugins
file(GLOB PLUGIN_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*")
foreach(PLUGIN_DIR ${PLUGIN_DIRS})
    get_filename_component(PLUGIN_NAME ${PLUGIN_DIR} NAME)
    if(IS_DIRECTORY ${PLUGIN_DIR} AND EXISTS "${PLUGIN_DIR}/CMakeLists.txt")
        if(PLUGIN_NAME IN_LIST SKIP_PLUGINS)
            message(STATUS "[Ouaricon] Skipping plugin: ${PLUGIN_NAME}")
        else()
            add_subdirectory(${PLUGIN_DIR})
        endif()
    endif()
endforeach()
