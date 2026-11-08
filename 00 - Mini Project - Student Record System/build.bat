@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo   Building Student Record Management System (BST)
echo   East West University - CSE 207 Lab Project
echo ============================================================

set CXX=g++
set CXXFLAGS=-std=c++17 -O2 -Wall -Wextra

if not exist bin mkdir bin

echo Compiling Student Record Management System...
%CXX% %CXXFLAGS% main.cpp Node.cpp Student.cpp StudentBST.cpp -o bin/StudentRecordSystem.exe

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo [SUCCESS] Build completed: bin/StudentRecordSystem.exe

