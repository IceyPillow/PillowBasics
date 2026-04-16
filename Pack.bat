@echo off
setlocal

rd /s/q Packed
cmake --build ./Cmake/Win64 --config Release
cmake --install ./Cmake/Win64