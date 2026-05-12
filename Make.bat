@echo off
setlocal

:: Delete stale CMake files.
set CLEAN=false
for %%i in (%*) do (
    if "%%i"=="-c" set CLEAN=true
)
echo.
if "%CLEAN%"=="true" (
    echo Deleting stale CMake files...
    rd /s/q Cmake
) else (
    echo Tip: Add "-c" to clean stale CMake files forcely.
)
echo.

:: Generate.
cmake -G "Visual Studio 18 2026" -DCMAKE_TOOLCHAIN_FILE=./ToolchainWin64.cmake -S ./SourceCode -B ./Cmake/Win64

:: Open the solution.
echo.
set /p input=Open the solution? (y/n)
if /i "%input%"=="y" start ./Cmake/Win64/PillowBasics.slnx