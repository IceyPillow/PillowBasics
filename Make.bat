@echo off
setlocal

echo.
echo [Arguments Help]
echo.
echo 26: Choose Visual Studio 2026. (VS 2022 is default)
echo -c: Delete the CMake-generated files.
echo.
echo.

set CLEAN=false
set Generator="Visual Studio 17 2022"
set SolutionEntry="PillowBasics.sln"
for %%i in (%*) do (
    if "%%i"=="-c" set CLEAN=true
    if "%%i"=="26" (
      set Generator = "Visual Studio 18 2026"
      set SolutionEntry="PillowBasics.slnx"
    )
)
:: Delete stale CMake files.
if "%CLEAN%"=="true" (
    echo Deleting stale CMake files...
    rd /s/q Cmake
)
echo.

:: Generate.
cmake -G %Generator% -T ClangCL -DCMAKE_TOOLCHAIN_FILE=./ToolchainWin64.cmake -S ./SourceCode -B ./Cmake/Win64

:: Open the solution.
echo.
set /p input=Open the solution? (y/n)
if /i "%input%"=="y" start ./Cmake/Win64/%SolutionEntry%