# Containing two platforms in single solution is not supported by CMake. e.g. x64+Win32
#set(TARGET_WINDOWS_SDK 10.0.18362.0) #1903, 19H1
set(TARGET_WINDOWS_SDK 10.0.19041.0) #2004, 20H1, updated for XeSS 2
set(CMAKE_SYSTEM_VERSION ${TARGET_WINDOWS_SDK}) # Where CMake runs
set(CMAKE_GENERATOR_PLATFORM x64,version=${TARGET_WINDOWS_SDK}) # Target machine

add_compile_definitions(_UNICODE)
add_compile_options(/arch:AVX /Zc:__cplusplus) # Support AVX and force __cplusplus to update
# Indicates /MT or /MTd (multithreaded, static link) to CRT (C/C++ runtime library).
set(CMAKE_MSVC_RUNTIME_LIBRARY "$<IF:$<CONFIG:Debug>,MultiThreadedDebug,MultiThreaded>")