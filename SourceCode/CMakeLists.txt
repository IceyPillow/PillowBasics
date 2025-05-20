# Requirements.
cmake_minimum_required(VERSION 4.0)
set(CMAKE_GENERATOR "Visual Studio 17 2022")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

# Solution-wide settings.
set(TARGET_WINDOWS_SDK 10.0.18362.0)
set(CMAKE_SYSTEM_VERSION ${TARGET_WINDOWS_SDK}) # Where CMake runs.
set(CMAKE_GENERATOR_PLATFORM x64,version=${TARGET_WINDOWS_SDK}) # Target machine.
message("\nCMAKE_GENERATOR_PLATFORM = ${CMAKE_GENERATOR_PLATFORM}\n")
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Available configuration types" FORCE)
add_compile_definitions(_UNICODE)

# Projects.
project(PillowBasics LANGUAGES CXX C)
add_subdirectory(Pillow)
add_subdirectory(3rdParty)
target_link_libraries(Pillow PRIVATE DirectXMath LodePNG OpenAL)
target_include_directories(Pillow PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Pillow)