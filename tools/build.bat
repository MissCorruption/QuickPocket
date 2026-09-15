@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0.."

if not exist "lib\CommonLibSSE-NG\xmake.lua" goto :init_clib
if not exist "lib\CommonLibSSE-NG\extern\openvr\headers" goto :init_clib
goto :have_clib
:init_clib
git submodule update --init --recursive lib/CommonLibSSE-NG
:have_clib
if not exist "lib\CommonLibSSE-NG\xmake.lua" (
	echo CommonLibSSE-NG submodule is missing.
	echo Clone with: git clone --recurse-submodules ^<url^>
	exit /b 1
)

if "%MODE%"=="" set MODE=releasedbg
if /I "%~1"=="debug" (
	set MODE=debug
	shift
) else if /I "%~1"=="release" (
	set MODE=release
	shift
) else if /I "%~1"=="releasedbg" (
	set MODE=releasedbg
	shift
)

xmake f -y -m %MODE% --ccache=y

set "ARGS="
:args
if "%~1"=="" goto :build
set ARGS=!ARGS! %1
shift
goto :args
:build
xmake build -y !ARGS!
