@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo   PROJECT 03: FILE COMPRESSION TOOL (HUFFMAN ENGINE)
echo   Language: C++20 (g++) - MinGW-w64 64-bit
echo =======================================================

if not exist bin mkdir bin
if not exist test_out mkdir test_out

set CXX=g++
set CXXFLAGS=-std=c++20 -O3 -Wall -Wextra -Wconversion -Wno-sign-conversion -Iinclude

set CORE_SRCS=src/ArchiveHeader.cpp src/BatchProcessor.cpp src/BenchmarkSuite.cpp src/BitBuffer.cpp src/CLI.cpp src/Common.cpp src/Compressor.cpp src/Decompressor.cpp src/FileIO.cpp src/FrequencyCounter.cpp src/HuffmanTree.cpp src/PathSanitizer.cpp src/TreeVisualizer.cpp

if "%1"=="" goto build_all
if "%1"=="test" goto run_tests
if "%1"=="clean" goto clean
if "%1"=="run" goto run_app

:build_all
echo --- Phase 1: Compiling Application and Test Harness ---
%CXX% %CXXFLAGS% %CORE_SRCS% tests/test_all.cpp -o bin/test_all.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Test compilation failed!
    exit /b 1
)

%CXX% %CXXFLAGS% %CORE_SRCS% src/main.cpp -o bin/huffman.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Application compilation failed!
    exit /b 1
)
echo [SUCCESS] Build succeeded! Executables generated in bin\
goto run_tests

:run_tests
echo --- Phase 2: Running Automated Test Suite ---
if not exist bin\test_all.exe (
    %CXX% %CXXFLAGS% %CORE_SRCS% tests/test_all.cpp -o bin/test_all.exe
    if %ERRORLEVEL% NEQ 0 exit /b 1
)
bin\test_all.exe
goto end

:run_app
if not exist bin\huffman.exe (
    %CXX% %CXXFLAGS% src/*.cpp -o bin/huffman.exe
    if %ERRORLEVEL% NEQ 0 exit /b 1
)
bin\huffman.exe %2 %3 %4 %5 %6
goto end

:clean
echo Cleaning build artifacts...
if exist bin rmdir /s /q bin
if exist test_out rmdir /s /q test_out
echo Clean finished.
goto end

:end
