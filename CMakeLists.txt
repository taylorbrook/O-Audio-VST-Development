cmake_minimum_required(VERSION 3.22)

# Set deployment target before project
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")

project(JUCEPlugins VERSION 1.0.0)

# Licensing: OFF for local dev, ON in GitHub Actions publish builds
option(OUARICON_LICENSING "Enable license checking in plugins" OFF)

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

# Add JUCE once at root
add_subdirectory(/Users/taylorbrook/JUCE JUCE)

# Auto-discover plugins
file(GLOB PLUGIN_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*")
foreach(PLUGIN_DIR ${PLUGIN_DIRS})
    if(IS_DIRECTORY ${PLUGIN_DIR} AND EXISTS "${PLUGIN_DIR}/CMakeLists.txt")
        add_subdirectory(${PLUGIN_DIR})
    endif()
endforeach()
