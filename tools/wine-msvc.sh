wine_msvc_prefix() {
	echo "${MSVC_WINEPREFIX:-$ROOT/build/wineprefix}"
}

wine_msvc_env() {
	export WINEPREFIX
	WINEPREFIX="$(wine_msvc_prefix)"
	export WINEDEBUG="${WINEDEBUG:--all}"
	# Skip Gecko/Mono prompts (those spawn MicrosoftEdgeUpdate holding the lock).
	export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:-mscoree,mshtml=}"
	mkdir -p "$WINEPREFIX"
}

wine_msvc_shutdown() {
	if command -v wineserver >/dev/null 2>&1; then
		wineserver -k 2>/dev/null || true
	fi
	if command -v fuser >/dev/null 2>&1; then
		local lock
		for lock in "$ROOT"/.xmake/linux/*/project.lock; do
			[[ -e "$lock" ]] || continue
			fuser -k -KILL "$lock" >/dev/null 2>&1 || true
		done
	fi
}

wine_msvc_guard() {
	local lockdir="$ROOT/build/build.lockdir"
	mkdir -p "$ROOT/build"
	if ! mkdir "$lockdir" 2>/dev/null; then
		local old
		old=$(cat "$lockdir/pid" 2>/dev/null || true)
		if [[ -n "$old" ]] && kill -0 "$old" 2>/dev/null; then
			echo "A build is already running (pid $old)." >&2
			echo "Stop it, or wait; two msvc-wine builds in this tree corrupt .obj files." >&2
			exit 1
		fi
		rm -rf "$lockdir"
		mkdir "$lockdir"
	fi
	echo $$ >"$lockdir/pid"
}

wine_msvc_unguard() {
	rm -rf "$ROOT/build/build.lockdir"
}

wine_msvc_cleanup() {
	wine_msvc_unguard
	wine_msvc_shutdown
}
