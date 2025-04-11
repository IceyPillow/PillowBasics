@echo off
setlocal

:: Delete the old CMake files.
set CLEAN=false
for %%i in (%*) do (
    if "%%i"=="-c" set CLEAN=true
)
echo.
if "%CLEAN%"=="true" (
    echo Deleting the old CMake files...
    rd /s/q Cmake
)
else (
    echo Tip: You can use "-c" to forcely clean the old CMake files.
)
echo.

:: Generate.
cmake -S ./SourceCode -B ./Cmake -G "Visual Studio 17 2022"

:: Open the solution.
set /p input=Open the solution? (y/n)
if /i "%input%"=="y" start ./Cmake/CrucibleSolution.sln