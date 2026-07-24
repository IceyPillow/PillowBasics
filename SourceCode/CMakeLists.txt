# Requirements.
cmake_minimum_required(VERSION 4.3)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Solution-wide & platform-wide settings.
# Toolchain files are applied before cmake run.
#if(WIN32)
#elseif(ANDROID)
#else()
#   message(FATAL_ERROR "Invalid target platform.")
#endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "Available configuration types" FORCE)
add_compile_definitions($<$<CONFIG:Debug>:PILLOW_DEBUG>) # A customized macro.
set(LIBRARIES)
set(RUNTIMES)
if(WIN32)
   set(D3D12_RUNTIMES)
endif()

# Projects.
project(PillowBasics LANGUAGES CXX C)
add_subdirectory(3rdParty)
add_subdirectory(Pillow)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Pillow)

# Enumerate shaders in Visual Studio
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
# RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/../Resources/Shaders"
   "${CMAKE_CURRENT_SOURCE_DIR}/../Resources/Shaders/*.hlsl"
)
set_source_files_properties(${SOURCES} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")
add_library(Shaders INTERFACE ${SOURCES})

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    get_filename_component(INSTALL_PATH "${CMAKE_SOURCE_DIR}/../Cmake/Packed" ABSOLUTE)
    set(CMAKE_INSTALL_PREFIX "${INSTALL_PATH}" CACHE PATH "Default install path." FORCE)
endif()

install(TARGETS Pillow RUNTIME DESTINATION "./")
install(FILES ${RUNTIMES} DESTINATION "./")
if(WIN32)
   install(FILES ${D3D12_RUNTIMES} DESTINATION "./D3D12/")
endif()
