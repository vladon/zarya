#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${ZARYA_BUILD_DIR:-build}"
with_plasma_shell=0
inside_plasma_shell=0

while (($#)); do
    case "$1" in
        --with-plasma-shell)
            with_plasma_shell=1
            ;;
        --inside-plasma-shell)
            inside_plasma_shell=1
            ;;
        -h|--help)
            echo "Usage: $0 [build-dir] [--with-plasma-shell]"
            exit 0
            ;;
        -*)
            echo "Unknown option: $1" >&2
            exit 2
            ;;
        *)
            build_dir="$1"
            ;;
    esac
    shift
done

if [[ "$build_dir" != /* ]]; then
    build_dir="$repo_root/$build_dir"
fi
test_binary="$build_dir/zarya_linux_proxy_test"

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Required command is unavailable: $1" >&2
        exit 1
    fi
}

require_command dbus-run-session
require_command dbus-send
require_command kreadconfig6
require_command kwriteconfig6

if [[ ! -x "$test_binary" ]]; then
    echo "Build zarya_linux_proxy_test first: cmake --build \"$build_dir\" --target zarya_linux_proxy_test" >&2
    exit 1
fi

if ((with_plasma_shell && !inside_plasma_shell)); then
    require_command xvfb-run
    require_command plasmashell
    require_command rg
    exec dbus-run-session -- xvfb-run -a "$0" "$build_dir" --inside-plasma-shell
fi

if ((inside_plasma_shell)); then
    plasma_config="$(mktemp -d "${TMPDIR:-/tmp}/zarya-plasma-config-XXXXXX")"
    plasma_cache="$(mktemp -d "${TMPDIR:-/tmp}/zarya-plasma-cache-XXXXXX")"
    plasma_pid=""
    # shellcheck disable=SC2329 # Invoked by the EXIT trap below.
    cleanup() {
        if [[ -n "$plasma_pid" ]]; then
            kill "$plasma_pid" 2>/dev/null || true
            wait "$plasma_pid" 2>/dev/null || true
        fi
        rm -rf -- "$plasma_config" "$plasma_cache"
    }
    trap cleanup EXIT

    export XDG_CONFIG_HOME="$plasma_config"
    export XDG_CACHE_HOME="$plasma_cache"
    export XDG_CURRENT_DESKTOP=KDE
    export KDE_FULL_SESSION=true
    export QT_QPA_PLATFORM=xcb

    plasmashell --no-respawn >"$plasma_cache/plasmashell.log" 2>&1 &
    plasma_pid=$!
    plasma_ready=0
    for _ in $(seq 1 20); do
        if dbus-send --session --print-reply=literal \
            --dest=org.freedesktop.DBus / org.freedesktop.DBus.ListNames \
            | rg -q org.kde.plasmashell; then
            plasma_ready=1
            break
        fi
        sleep 1
    done
    if ((plasma_ready == 0)); then
        echo "Plasma shell did not register on session D-Bus." >&2
        sed -n '1,160p' "$plasma_cache/plasmashell.log" >&2
        exit 1
    fi
    plasmashell --version
    env ZARYA_RUN_NATIVE_KDE_PROXY_SMOKE=1 "$test_binary"
    exit $?
fi

exec dbus-run-session -- env \
    XDG_CURRENT_DESKTOP=KDE \
    KDE_FULL_SESSION=true \
    ZARYA_RUN_NATIVE_KDE_PROXY_SMOKE=1 \
    "$test_binary"
