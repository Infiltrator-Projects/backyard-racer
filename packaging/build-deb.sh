#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${1:-$root/build-package}"
dist_dir="${2:-$root/dist}"
version="$(sed -n 's/^project(BackyardRacer VERSION \([^ ]*\) LANGUAGES C CXX)$/\1/p' "$root/CMakeLists.txt")"
arch="$(dpkg --print-architecture)"

[[ -n "$version" ]] || { echo "Unable to determine Backyard Racer version" >&2; exit 1; }
command -v cmake >/dev/null
command -v dpkg-deb >/dev/null

rm -rf "$build_dir"
mkdir -p "$build_dir" "$dist_dir"

cmake -S "$root" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBACKYARD_BUILD_PROFILE=generic
cmake --build "$build_dir" --parallel
ctest --test-dir "$build_dir" --output-on-failure

stage="$build_dir/package-root"
rm -rf "$stage"
DESTDIR="$stage" cmake --install "$build_dir" \
    --component Runtime --prefix /usr

mkdir -p "$stage/DEBIAN"
cat >"$stage/DEBIAN/control" <<EOF
Package: backyard-racer
Version: $version
Section: games
Priority: optional
Architecture: $arch
Maintainer: Shannon Smith <noreply@github.com>
Depends: libx11-6, zlib1g
Homepage: https://github.com/Infiltrator-Projects/backyard-racer
Description: native street-rod garage and racing game
 Backyard Racer is a native C/C++ street-rod garage and racing game.
 This package is the generic CPU build; hardware-native builds are made
 directly from the same source tree with BACKYARD_BUILD_PROFILE=native.
EOF

output="$dist_dir/backyard-racer_${version}_${arch}.deb"
dpkg-deb --root-owner-group --build "$stage" "$output"
printf '%s\n' "$output"
