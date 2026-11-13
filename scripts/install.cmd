@echo off
REM @Author: AxlumLab
REM @LastEditors: AxlumLab
REM Copyright (c) 2026 by AxlumLab, All Rights Reserved.

setlocal

set "BUILD_TYPE=%BUILD_TYPE%"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"

set "VCPKG_TRIPLET=%VCPKG_TARGET_TRIPLET%"
if "%VCPKG_TRIPLET%"=="" set "VCPKG_TRIPLET=x64-windows"

set "BUILD_DIR=%BUILD_DIR%"
if "%BUILD_DIR%"=="" set "BUILD_DIR=out\build\ninja-vcpkg-%VCPKG_TRIPLET%-%BUILD_TYPE%"

set "INSTALL_PREFIX=%INSTALL_PREFIX%"
if "%INSTALL_PREFIX%"=="" set "INSTALL_PREFIX=out\install\rhodium"

pushd "%~dp0.."
if errorlevel 1 exit /b 1

call "%~dp0build.cmd"
if errorlevel 1 (
    popd
    exit /b 1
)

cmake --install "%BUILD_DIR%" --config "%BUILD_TYPE%" --prefix "%INSTALL_PREFIX%"
set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%
