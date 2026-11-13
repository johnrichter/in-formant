#!/bin/bash
#
# Build InFormant natively on an Apple silicon Mac. The result is a signed arm64 application in a
# disk image: dist/InFormant-<version>-macOS-arm64.dmg
#
# The script needs macOS 27 or newer, the Xcode command line tools (or Xcode) with the macOS 27 SDK,
# Python 3, and pkg-config. It downloads Qt, FFTW, PortAudio, and Eigen, and it keeps them and the
# build files in build/native.
#
# Settings (environment variables):
#   BUILD_TYPE     Release (default), RelWithDebInfo, or Debug
#   QT_VERSION     The Qt version that aqtinstall installs (default 6.8.3)
#   WORK_DIR       The folder for downloads and build files (default build/native)
#   JOBS           The number of parallel jobs (default: the number of CPU cores)

set -euo pipefail

BUILD_TYPE=${BUILD_TYPE:-Release}
QT_VERSION=${QT_VERSION:-6.8.3}
FFTW_VERSION=${FFTW_VERSION:-3.3.10}
PORTAUDIO_URL=${PORTAUDIO_URL:-http://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz}
EIGEN_VERSION=${EIGEN_VERSION:-3.4.0}
MIN_MACOS=27.0

root=$(cd "$(dirname "$0")" && pwd)
work=${WORK_DIR:-$root/build/native}
jobs=${JOBS:-$(sysctl -n hw.ncpu)}
version=$(cat "$root/version")

die() {
    echo "build-macos.sh: $*" >&2
    exit 1
}

step() {
    echo
    echo "==> $*"
}

# --- Check the machine ---------------------------------------------------------------------------

[ "$(uname -s)" = Darwin ] || die "this script builds for macOS and runs on macOS only"
[ "$(uname -m)" = arm64 ] || die "this script needs an Apple silicon Mac"
[ "$(sw_vers -productVersion | cut -d. -f1)" -ge 27 ] || die "this script needs macOS 27 or newer"
sdk=$(xcrun --sdk macosx --show-sdk-path 2>/dev/null) || die "the Xcode command line tools are missing. Run: xcode-select --install"
command -v pkg-config > /dev/null || die "pkg-config is missing. Install it, for example with: brew install pkgconf"

mkdir -p "$work" "$root/dist"
prefix=$work/prefix
mkdir -p "$prefix"

export MACOSX_DEPLOYMENT_TARGET=$MIN_MACOS
export SDKROOT=$sdk

# Use only the libraries of the prefix, and not the ones that another package manager installed.
export PKG_CONFIG_LIBDIR=$prefix/lib/pkgconfig
unset PKG_CONFIG_PATH

# --- Python tools --------------------------------------------------------------------------------

venv=$work/venv
if [ ! -x "$venv/bin/aqt" ]; then
    step "Install aqtinstall, CMake, and Ninja in a virtual environment"
    python3 -m venv "$venv"
    "$venv/bin/pip" install --quiet --upgrade pip
    "$venv/bin/pip" install --quiet aqtinstall "cmake<4" ninja
fi
export PATH=$venv/bin:$PATH

# --- Qt ------------------------------------------------------------------------------------------

qt_dir=$work/qt/$QT_VERSION/macos
if [ ! -x "$qt_dir/bin/macdeployqt" ]; then
    step "Install Qt $QT_VERSION"
    aqt install-qt mac desktop "$QT_VERSION" clang_64 --outputdir "$work/qt"
fi

# --- Libraries -----------------------------------------------------------------------------------

fetch() {
    # fetch URL FILE: download one time
    if [ ! -f "$2" ]; then
        curl -fL --retry 3 -o "$2.part" "$1"
        mv "$2.part" "$2"
    fi
}

src=$work/src
mkdir -p "$src"

if [ ! -f "$prefix/lib/libfftw3.dylib" ]; then
    step "Build FFTW $FFTW_VERSION"
    fetch "https://www.fftw.org/fftw-$FFTW_VERSION.tar.gz" "$src/fftw.tar.gz"
    rm -rf "$src/fftw" && mkdir "$src/fftw"
    tar -xzf "$src/fftw.tar.gz" -C "$src/fftw" --strip-components=1
    (
        cd "$src/fftw"
        CFLAGS="-arch arm64 -O3" ./configure --prefix="$prefix" --enable-shared --disable-static \
            --disable-fortran --disable-dependency-tracking
        make -j"$jobs"
        make install
    )
fi

if [ ! -f "$prefix/lib/libportaudio.dylib" ]; then
    step "Build PortAudio"
    fetch "$PORTAUDIO_URL" "$src/portaudio.tgz"
    rm -rf "$src/portaudio" && mkdir "$src/portaudio"
    tar -xzf "$src/portaudio.tgz" -C "$src/portaudio" --strip-components=1
    cmake -S "$src/portaudio" -B "$src/portaudio/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_MACOS" \
        -DBUILD_SHARED_LIBS=ON -DPA_BUILD_SHARED=ON -DPA_BUILD_STATIC=OFF
    cmake --build "$src/portaudio/build" --parallel "$jobs"
    cmake --install "$src/portaudio/build"
fi

if [ ! -d "$prefix/include/eigen3" ]; then
    step "Install Eigen $EIGEN_VERSION"
    fetch "https://gitlab.com/libeigen/eigen/-/archive/$EIGEN_VERSION/eigen-$EIGEN_VERSION.tar.gz" "$src/eigen.tar.gz"
    rm -rf "$src/eigen" && mkdir "$src/eigen"
    tar -xzf "$src/eigen.tar.gz" -C "$src/eigen" --strip-components=1
    cmake -S "$src/eigen" -B "$src/eigen/build" -G Ninja -DCMAKE_INSTALL_PREFIX="$prefix"
    cmake --install "$src/eigen/build"
fi

# --- InFormant -----------------------------------------------------------------------------------

build=$work/build
step "Configure InFormant $version ($BUILD_TYPE)"
cmake -S "$root" -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_MACOS" \
    -DCMAKE_PREFIX_PATH="$qt_dir;$prefix" \
    -DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew \
    -DCUR_VERSION="$version"

step "Build InFormant"
cmake --build "$build" --parallel "$jobs"

# --- Package -------------------------------------------------------------------------------------

step "Make the application bundle"
stage=$work/stage
app=$stage/InFormant.app
rm -rf "$stage"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources"
sed "s/@VERSION@/$version/g" "$root/dist-res/Info.plist" > "$app/Contents/Info.plist"
cp "$build/in-formant" "$app/Contents/MacOS/InFormant"
cp "$root/resources/icons/in-formant.icns" "$app/Contents/Resources/"

# macdeployqt copies Qt and the other libraries into the bundle. The libraries that the build made
# (FreeType and libsamplerate) are in the build folder, so give their folders as search paths.
libpaths=()
while IFS= read -r dir; do
    libpaths+=("-libpath=$dir")
done < <(find "$build" -name '*.dylib' -exec dirname {} \; | sort -u)
"$qt_dir/bin/macdeployqt" "$app" -qmldir="$root/src/qml" "${libpaths[@]}"

# The deployment step changes library paths, and that breaks the signatures. Apple silicon refuses to
# run an arm64 file with an invalid signature, so sign the whole bundle again (ad hoc).
step "Sign the bundle"
codesign --force --deep --sign - "$app"
codesign --verify --deep --strict "$app"

step "Make the disk image"
ln -s /Applications "$stage/Applications"
dmg=$root/dist/InFormant-$version-macOS-arm64.dmg
rm -f "$dmg"
hdiutil create -volname InFormant -srcfolder "$stage" -ov -format UDZO "$dmg"

echo
echo "Done: $dmg"
