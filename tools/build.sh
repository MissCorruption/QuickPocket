#!/usr/bin/env bash
# Build the plugin. On Linux this uses msvc-wine; on Windows, native MSVC.
# Usage: ./tools/build.sh [debug|release|releasedbg] [xmake build args...]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "$ROOT"

if [[ ! -f "$ROOT/lib/CommonLibSSE-NG/xmake.lua" || ! -d "$ROOT/lib/CommonLibSSE-NG/extern/openvr/headers" ]]; then
	if command -v git >/dev/null && [[ -d "$ROOT/.git" ]]; then
		git submodule update --init --recursive lib/CommonLibSSE-NG
	fi
fi

if [[ ! -f "$ROOT/lib/CommonLibSSE-NG/xmake.lua" ]]; then
	echo "CommonLibSSE-NG submodule is missing." >&2
	echo "Clone with: git clone --recurse-submodules <url>" >&2
	exit 1
fi

MODE="${MODE:-releasedbg}"
if [[ "${1:-}" == debug || "${1:-}" == release || "${1:-}" == releasedbg ]]; then
	MODE="$1"
	shift
fi

if [[ "$(uname -s)" == Linux ]]; then
	MSVC_BINS="${MSVC_BINS:-$HOME/msvc-bins}"
	if [[ ! -x "$MSVC_BINS/bin/x64/cl" ]]; then
		echo "msvc-wine SDK not found at $MSVC_BINS; running setup (accepts Microsoft VS/WinSDK terms)..."
		"$ROOT/tools/setup-msvc-wine.sh"
	fi
	export MSVC_BINS
	source "$ROOT/tools/wine-msvc.sh"
	wine_msvc_env
	wine_msvc_guard
	trap wine_msvc_cleanup EXIT
	trap 'exit 130' INT
	trap 'exit 143' TERM
	wine_msvc_shutdown
	echo "Configuring (first Wine compiler probe can sit silent for about a minute)..."
fi

xmake f -y -m "$MODE" --ccache=y
xmake build -y "$@"
