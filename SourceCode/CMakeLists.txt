# Requirements.
cmake_minimum_required(VERSION 4.0)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

# Solution-wide & platform-wide settings.
if(WIN32)
   # Containing two platforms in single solution is not supported by CMake. e.g. x64+Win32
   set(TARGET_WINDOWS_SDK 10.0.18362.0)
   set(CMAKE_SYSTEM_VERSION ${TARGET_WINDOWS_SDK}) # Target OS version.
   add_compile_definitions(_UNICODE)
   add_compile_options(/arch:SSE2)
   set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded") # Equal to /MD, using static C/C++ runtime library
elseif(ANDROID)
   if(CMAKE_ANDROID_ARCH_ABI NOT STREQUAL "arm64-v8a")
      message(FATAL_ERROR "Only support arm64-v8a in Android, but CMAKE_ANDROID_ARCH_ABI=${CMAKE_ANDROID_ARCH_ABI}.")
   endif()
   # TODO
else()
   message(FATAL_ERROR "Invalid target platform.")
endif()
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Available configuration types" FORCE)
# Using a customized debug macro. _DEBUG affects the C++ STL headers, but we wanna use /MT whenever
add_compile_definitions($<$<CONFIG:Debug>:PILLOW_DEBUG>)
# Projects.
project(PillowBasics LANGUAGES CXX C)
add_subdirectory(Pillow)
add_subdirectory(3rdParty)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Pillow)