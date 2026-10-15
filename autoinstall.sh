#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: ./autoinstall.sh [--dry-run | --help]

Install dependencies, build, and install KWin Saturation under /usr.
Supported: Arch/CachyOS, Debian/Ubuntu, and Fedora with KDE Plasma 6.
Run as your normal user; sudo is used only for system changes.

  --dry-run  Print commands without changing the system.
  --help     Show this help.
EOF
}

die() { printf 'Error: %s\n' "$*" >&2; exit 1; }

dry_run=false
case "${1:-}" in
    --help|-h) usage; exit 0 ;;
    --dry-run) dry_run=true ;;
    '') ;;
    *) usage >&2; exit 1 ;;
esac
[[ $# -le 1 ]] || die 'Use --help to see supported options.'

run() {
    printf '  '
    printf '%q ' "$@"
    printf '\n'
    if [[ "$dry_run" == false ]]; then
        "$@"
    fi
}

trap 'printf "Installation stopped at line %s. Fix the error above and rerun the installer.\n" "$LINENO" >&2' ERR

source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir="$source_dir/build-autoinstall"
[[ -f "$source_dir/CMakeLists.txt" ]] || die 'Keep this script in the project source directory.'
[[ -r /etc/os-release ]] || die 'Cannot detect this distribution: /etc/os-release is missing.'
# shellcheck source=/etc/os-release
. /etc/os-release

family=''
for distro in "${ID:-}" ${ID_LIKE:-}; do
    case "$distro" in
        arch|cachyos|manjaro|endeavouros) family=arch; break ;;
        debian|ubuntu) family=debian; break ;;
        fedora) family=fedora; break ;;
    esac
done
[[ -n "$family" ]] || die "Unsupported distribution: ${PRETTY_NAME:-unknown}. See README.md for manual installation."
[[ ! -e /run/ostree-booted ]] || die 'Immutable/OSTree systems need a distribution-specific installation method.'

if [[ "$dry_run" == false ]]; then
    [[ $EUID -ne 0 ]] || die 'Run ./autoinstall.sh as your normal desktop user, without sudo.'
    command -v sudo >/dev/null || die 'sudo is required for package and system installation.'
    if [[ ${KDE_SESSION_VERSION:-6} != 6 ]]; then
        die 'KDE Plasma 6 is required. Plasma 5 is not supported.'
    fi
fi

printf 'KWin Saturation installer — %s\n' "${PRETTY_NAME:-$family}"
printf 'KDE Plasma 6 is required. Installation prefix: /usr\n'
if [[ "$dry_run" == true ]]; then
    printf 'Preview only; no commands will be executed.\n'
fi

case "$family" in
    arch)
        # A full upgrade avoids unsupported partial upgrades on Arch systems.
        run sudo pacman -Syu --needed base-devel cmake extra-cmake-modules \
            kwin qt6-base qt6-svg qt6-tools kconfig vulkan-headers
        ;;
    debian)
        run sudo apt-get update
        if [[ "$dry_run" == false ]]; then
            candidate=$(LC_ALL=C apt-cache policy kwin-dev | awk '/Candidate:/ {print $2; exit}')
            [[ -n "$candidate" && "$candidate" != '(none)' ]] || die 'kwin-dev is not available in your configured repositories.'
            dpkg --compare-versions "${candidate#*:}" ge 6.0 || die 'Your repositories provide Plasma 5. Use a distribution release with Plasma 6 development packages.'
        fi
        run sudo apt-get install build-essential cmake extra-cmake-modules \
            kwin-dev qt6-base-dev qt6-svg-dev qdbus-qt6 libkf6config-dev \
            libvulkan-dev libwayland-dev libdrm-dev
        ;;
    fedora)
        run sudo dnf install gcc-c++ make cmake extra-cmake-modules \
            kwin-devel qt6-qtbase-devel qt6-qtsvg-devel qt6-qttools kf6-kconfig-devel \
            vulkan-headers vulkan-loader-devel
        ;;
esac

jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '2')
run cmake -S "$source_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
run cmake --build "$build_dir" --parallel "$jobs"
run sudo cmake --install "$build_dir"

if [[ "$dry_run" == true ]]; then
    printf '\nPreview complete. Run ./autoinstall.sh to install.\n'
else
    printf '\nInstallation complete.\n'
    printf 'Log out of your KDE Plasma session, then log back in to load the effect.\n'
    printf 'Then open Digital Vibrance from the application menu or run kwin-saturation-gui.\n'
    printf 'If needed, enable Saturation Boost in System Settings > Desktop Effects.\n'
fi
