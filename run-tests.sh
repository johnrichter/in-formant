#!/bin/bash
#
# Build and run the unit tests on an Apple silicon Mac. The tests use their own build folder,
# build/native/build-tests, so the application build does not contain any test code.
#
# Run build-macos.sh one time first. The tests use the same Qt and the same libraries.
#
# Usage: ./run-tests.sh [--sanitize] [ctest arguments]
#   --sanitize   Build the tests with the address and undefined behavior sanitizers.
#   Examples of ctest arguments: -R TP_LAY (run the layout tests), -L script (run the script tests).
#
# Settings (environment variables):
#   BUILD_TYPE   The build type (default Debug)
#   QT_VERSION   The Qt version of build-macos.sh (default 6.12.0)
#   WORK_DIR     The folder of build-macos.sh (default build/native)
#   JOBS         The number of parallel jobs (default: the number of CPU cores)

set -euo pipefail

BUILD_TYPE=${BUILD_TYPE:-Debug}
QT_VERSION=${QT_VERSION:-6.12.0}
MIN_MACOS=27.0

root=$(cd "$(dirname "$0")" && pwd)
work=${WORK_DIR:-$root/build/native}
jobs=${JOBS:-$(sysctl -n hw.ncpu)}
prefix=$work/prefix
qt_dir=$work/qt/$QT_VERSION/macos
build=$work/build-tests

die() {
    echo "run-tests.sh: $*" >&2
    exit 1
}

[ -x "$work/venv/bin/cmake" ] || die "run build-macos.sh one time first, so that the tools and the libraries exist"
[ -d "$qt_dir" ] || die "Qt is missing in $qt_dir. Run build-macos.sh first"

sanitize=OFF
if [ "${1:-}" = "--sanitize" ]; then
    sanitize=ON
    shift
fi

export PATH="$work/venv/bin:$PATH"
export MACOSX_DEPLOYMENT_TARGET=$MIN_MACOS
export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
export PKG_CONFIG_LIBDIR=$prefix/lib/pkgconfig
unset PKG_CONFIG_PATH

# CMake keeps the path of Qt in its cache. A build folder that used another Qt version links the wrong libraries.
if [ -f "$build/CMakeCache.txt" ] && ! grep -q "^Qt6_DIR:PATH=$qt_dir/" "$build/CMakeCache.txt"; then
    rm -rf "$build"
fi

cmake -S "$root" -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_MACOS" \
    -DCMAKE_PREFIX_PATH="$qt_dir;$prefix" \
    -DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew \
    -DCUR_VERSION="$(cat "$root/version")" \
    -DINFORMANT_BUILD_TESTS=ON \
    -DINFORMANT_APP_PATH="$work/stage/InFormant.app" \
    -DINFORMANT_SANITIZE_TESTS="$sanitize"

cmake --build "$build" --target unit_tests --parallel "$jobs"

# The prebuilt Qt has no instrumentation, so the sanitizer reports false overflows in containers of Qt.
export ASAN_OPTIONS=${ASAN_OPTIONS:-detect_container_overflow=0}
ctest --test-dir "$build" --output-on-failure "$@"
