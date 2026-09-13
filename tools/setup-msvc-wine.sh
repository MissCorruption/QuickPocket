#!/usr/bin/env bash
# One-time Linux host: download MSVC + WinSDK and wrap them for Wine.
# Requires wine, python3, msitools (msiextract), and winbind/samba.
# License: you must accept Microsoft's VS/WinSDK terms (vsdownload.py prompts).
set -euo pipefail

DEST="${MSVC_BINS:-$HOME/msvc-bins}"
SRC="${MSVC_WINE_SRC:-$HOME/src/msvc-wine}"

if [[ -x "$DEST/bin/x64/cl" ]]; then
	echo "msvc-wine SDK already present at $DEST"
	exit 0
fi

if ! command -v wine >/dev/null; then
	echo "wine is not installed." >&2
	exit 1
fi
if ! command -v msiextract >/dev/null; then
	echo "msiextract not found (package: msitools)." >&2
	exit 1
fi

mkdir -p "$(dirname "$SRC")"
if [[ ! -d "$SRC/.git" ]]; then
	git clone --depth 1 https://github.com/mstorsjo/msvc-wine.git "$SRC"
fi

echo "Downloading MSVC/WinSDK into $DEST (this is large)..."
python3 "$SRC/vsdownload.py" --accept-license --dest "$DEST"
"$SRC/install.sh" "$DEST"

if [[ ! -x "$DEST/bin/x64/cl" ]]; then
	echo "install finished but $DEST/bin/x64/cl is missing" >&2
	exit 1
fi

echo "OK. SDK is at $DEST"
echo "Build with: ./tools/build.sh"
