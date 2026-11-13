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
#   QT_VERSION     The Qt version that aqtinstall installs (default 6.12.0)
#   WORK_DIR       The folder for downloads and build files (default build/native)
#   JOBS           The number of parallel jobs (default: the number of CPU cores)
#   SIGN_IDENTITY  The name or the hash of a code signing identity. The script always signs. With no value, it
#                  uses the first valid "Developer ID Application" identity of the keychain, and then the first
#                  valid "Apple Development" identity. With no such identity, the script stops. It never makes
#                  an unsigned or an ad hoc build.
#   NOTARY_PROFILE The keychain profile of notarytool. With a profile, the script notarizes the application
#                  and the disk image. It needs a Developer ID identity.

set -euo pipefail

BUILD_TYPE=${BUILD_TYPE:-Release}
SIGN_IDENTITY=${SIGN_IDENTITY:-}
NOTARY_PROFILE=${NOTARY_PROFILE:-}
QT_VERSION=${QT_VERSION:-6.12.0}
FFTW_VERSION=3.3.10
PORTAUDIO_URL=https://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz
EIGEN_VERSION=5.0.1
FREETYPE_VERSION=2.15.0
SAMPLERATE_VERSION=0.2.2
TOMLPP_VERSION=3.4.0

# Each download has a pinned SHA-256 checksum. A file with another checksum stops the build. To change a
# version, change the version, the address, and the checksum together.
FFTW_SHA256=56c932549852cddcfafdab3820b0200c7742675be92179e59e6215b340e26467
PORTAUDIO_SHA256=47efbf42c77c19a05d22e627d42873e991ec0c1357219c0d74ce6a2948cb2def
EIGEN_SHA256=e9c326dc8c05cd1e044c71f30f1b2e34a6161a3b6ecf445d56b53ff1669e3dec
FREETYPE_SHA256=93cd47eb719c325107f2d3cf087d06299e944c3b48a40b55537302304dea1dd5
SAMPLERATE_SHA256=3258da280511d24b49d6b08615bbe824d0cacc9842b0e4caf11c52cf2b043893
TOMLPP_SHA256=8517f65938a4faae9ccf8ebb36631a38c1cadfb5efa85d9a72e15b9e97d25155

# The versions of the Python tools. Some libraries can need an older CMake than the newest one.
AQT_VERSION=3.3.0
CMAKE_VERSION=3.31.10
NINJA_VERSION=1.13.2
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
sdk_version=$(xcrun --sdk macosx --show-sdk-version 2>/dev/null) || die "the Xcode command line tools are missing. Run: xcode-select --install"
[ "${sdk_version%%.*}" -ge 27 ] || die "this script needs the macOS 27 SDK (Xcode 27 or newer). The SDK is $sdk_version"
command -v pkg-config > /dev/null || die "pkg-config is missing. Install it, for example with: brew install pkgconf"

# The build always signs, so find the identity now and not after a long build.
find_identity() {
    # find_identity KIND: print the hash of the first valid identity whose name starts with KIND
    security find-identity -v -p codesigning | grep "\"$1:" | head -n 1 | awk '{print $2}' || true
}
if [ -z "$SIGN_IDENTITY" ]; then
    SIGN_IDENTITY=$(find_identity "Developer ID Application")
fi
if [ -z "$SIGN_IDENTITY" ]; then
    SIGN_IDENTITY=$(find_identity "Apple Development")
fi
[ -n "$SIGN_IDENTITY" ] && [ "$SIGN_IDENTITY" != "-" ] || die "no code signing identity found. Create an Apple Development certificate in Xcode (Settings, Accounts, Manage Certificates), or set SIGN_IDENTITY. The script does not make an unsigned build"

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
tools="aqtinstall==$AQT_VERSION cmake==$CMAKE_VERSION ninja==$NINJA_VERSION"
if [ ! -x "$venv/bin/aqt" ] || [ "$(cat "$venv/tools.txt" 2>/dev/null)" != "$tools" ]; then
    step "Install aqtinstall, CMake, and Ninja in a virtual environment"
    rm -rf "$venv"
    python3 -m venv "$venv"
    "$venv/bin/pip" install --quiet --upgrade pip
    # shellcheck disable=SC2086
    "$venv/bin/pip" install --quiet $tools
    echo "$tools" > "$venv/tools.txt"
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
    # fetch URL FILE SHA256: download one time, and check the checksum each time
    if [ ! -f "$2" ]; then
        curl -fL --retry 3 -o "$2.part" "$1"
        mv "$2.part" "$2"
    fi
    local actual
    actual=$(shasum -a 256 "$2" | cut -d' ' -f1)
    if [ "$actual" != "$3" ]; then
        rm -f "$2"
        die "the checksum of $1 is wrong. It is $actual. It must be $3"
    fi
}

src=$work/src
mkdir -p "$src"

if [ ! -f "$prefix/lib/libfftw3.dylib" ]; then
    step "Build FFTW $FFTW_VERSION"
    fetch "https://www.fftw.org/fftw-$FFTW_VERSION.tar.gz" "$src/fftw.tar.gz" "$FFTW_SHA256"
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
    fetch "$PORTAUDIO_URL" "$src/portaudio.tgz" "$PORTAUDIO_SHA256"
    rm -rf "$src/portaudio" && mkdir "$src/portaudio"
    tar -xzf "$src/portaudio.tgz" -C "$src/portaudio" --strip-components=1
    cmake -S "$src/portaudio" -B "$src/portaudio/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_MACOS" \
        -DBUILD_SHARED_LIBS=ON -DPA_BUILD_SHARED=ON -DPA_BUILD_STATIC=OFF
    cmake --build "$src/portaudio/build" --parallel "$jobs"
    cmake --install "$src/portaudio/build"
fi

# A stamp file in the prefix records each library that has a version. A change of the version makes a new stamp
# name, so the script builds the library again.
stamps=$prefix/stamps
mkdir -p "$stamps"

if [ ! -f "$stamps/eigen-$EIGEN_VERSION" ]; then
    step "Install Eigen $EIGEN_VERSION"
    fetch "https://gitlab.com/libeigen/eigen/-/archive/$EIGEN_VERSION/eigen-$EIGEN_VERSION.tar.gz" "$src/eigen.tar.gz" "$EIGEN_SHA256"
    rm -rf "$src/eigen" && mkdir "$src/eigen"
    tar -xzf "$src/eigen.tar.gz" -C "$src/eigen" --strip-components=1
    # An old version leaves headers that the new version does not own, so delete the old install first.
    rm -rf "$prefix/include/eigen3" "$prefix/share/eigen3"
    cmake -S "$src/eigen" -B "$src/eigen/build" -G Ninja -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DBUILD_TESTING=OFF -DEIGEN_BUILD_DOC=OFF -DEIGEN_BUILD_DEMOS=OFF \
        -DEIGEN_BUILD_BLAS=OFF -DEIGEN_BUILD_LAPACK=OFF
    cmake --install "$src/eigen/build"
    touch "$stamps/eigen-$EIGEN_VERSION"
fi

# FreeType draws the text of the graph. The program needs no PNG, zlib, bzip2, Brotli, or HarfBuzz support.
if [ ! -f "$stamps/freetype-$FREETYPE_VERSION" ]; then
    step "Build FreeType $FREETYPE_VERSION"
    fetch "https://download.savannah.gnu.org/releases/freetype/freetype-$FREETYPE_VERSION.tar.xz" "$src/freetype.tar.xz" "$FREETYPE_SHA256"
    rm -rf "$src/freetype" && mkdir "$src/freetype"
    tar -xJf "$src/freetype.tar.xz" -C "$src/freetype" --strip-components=1
    cmake -S "$src/freetype" -B "$src/freetype/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_MACOS" \
        -DBUILD_SHARED_LIBS=ON -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON \
        -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON
    cmake --build "$src/freetype/build" --parallel "$jobs"
    cmake --install "$src/freetype/build"
    touch "$stamps/freetype-$FREETYPE_VERSION"
fi

# libsamplerate changes the sample rate of the sound.
if [ ! -f "$stamps/samplerate-$SAMPLERATE_VERSION" ]; then
    step "Build libsamplerate $SAMPLERATE_VERSION"
    fetch "https://github.com/libsndfile/libsamplerate/releases/download/$SAMPLERATE_VERSION/libsamplerate-$SAMPLERATE_VERSION.tar.xz" "$src/samplerate.tar.xz" "$SAMPLERATE_SHA256"
    rm -rf "$src/samplerate" && mkdir "$src/samplerate"
    tar -xJf "$src/samplerate.tar.xz" -C "$src/samplerate" --strip-components=1
    cmake -S "$src/samplerate" -B "$src/samplerate/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_MACOS" \
        -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF -DLIBSAMPLERATE_EXAMPLES=OFF
    cmake --build "$src/samplerate/build" --parallel "$jobs"
    cmake --install "$src/samplerate/build"
    touch "$stamps/samplerate-$SAMPLERATE_VERSION"
fi

# toml++ reads the settings file. The library is a set of headers, and the program compiles the code one time.
if [ ! -f "$stamps/tomlplusplus-$TOMLPP_VERSION" ]; then
    step "Install toml++ $TOMLPP_VERSION"
    fetch "https://github.com/marzer/tomlplusplus/archive/refs/tags/v$TOMLPP_VERSION.tar.gz" "$src/tomlplusplus.tar.gz" "$TOMLPP_SHA256"
    rm -rf "$src/tomlplusplus" && mkdir "$src/tomlplusplus"
    tar -xzf "$src/tomlplusplus.tar.gz" -C "$src/tomlplusplus" --strip-components=1
    cmake -S "$src/tomlplusplus" -B "$src/tomlplusplus/build" -G Ninja \
        -DCMAKE_INSTALL_PREFIX="$prefix" -Dtomlplusplus_BUILD_TESTS=OFF -Dtomlplusplus_BUILD_EXAMPLES=OFF
    cmake --install "$src/tomlplusplus/build"
    touch "$stamps/tomlplusplus-$TOMLPP_VERSION"
fi

# --- InFormant -----------------------------------------------------------------------------------

build=$work/build
# CMake keeps the path of Qt in its cache. A build folder that used another Qt version links the wrong libraries.
if [ -f "$build/CMakeCache.txt" ] && ! grep -q "^Qt6_DIR:PATH=$qt_dir/" "$build/CMakeCache.txt"; then
    step "Delete the build folder of another Qt version"
    rm -rf "$build"
fi
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

# macdeployqt copies Qt and the other libraries into the bundle. The script made FFTW, PortAudio, FreeType, and
# libsamplerate in the prefix, so give the library folder of the prefix as a search path.
"$qt_dir/bin/macdeployqt" "$app" -qmldir="$root/src/qml" "-libpath=$prefix/lib"

# The deployment step changes library paths, and that breaks the signatures. Apple silicon refuses to
# run an arm64 file with an invalid signature, so sign the whole bundle again. Each build is signed with the
# hardened runtime and a secure timestamp. A stable identity also keeps the microphone choice of the user
# between builds, because macOS ties that choice to the signature. The entitlements give the application
# the microphone and the Qt Quick compiler.
step "Sign the bundle"
codesign --force --deep --options runtime --timestamp \
    --entitlements "$root/dist-res/InFormant.entitlements" --sign "$SIGN_IDENTITY" "$app"
codesign --verify --deep --strict "$app"

# Notarization sends the application to Apple, and it staples the ticket to the file. It needs a Developer ID
# identity and a profile of notarytool in the keychain (xcrun notarytool store-credentials).
notarize() {
    # notarize FILE: submit one file, and wait for the result
    xcrun notarytool submit "$1" --keychain-profile "$NOTARY_PROFILE" --wait
}
if [ -n "$NOTARY_PROFILE" ]; then
    step "Notarize the application"
    ditto -c -k --keepParent "$app" "$stage/InFormant.zip"
    notarize "$stage/InFormant.zip"
    rm -f "$stage/InFormant.zip"
    xcrun stapler staple "$app"
fi

step "Make the disk image"
ln -s /Applications "$stage/Applications"
dmg=$root/dist/InFormant-$version-macOS-arm64.dmg
rm -f "$dmg"
hdiutil create -volname InFormant -srcfolder "$stage" -ov -format UDZO "$dmg"

codesign --force --timestamp --sign "$SIGN_IDENTITY" "$dmg"
if [ -n "$NOTARY_PROFILE" ]; then
    step "Notarize the disk image"
    notarize "$dmg"
    xcrun stapler staple "$dmg"
fi

echo
echo "Done: $dmg"
