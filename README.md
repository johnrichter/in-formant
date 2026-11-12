[![Build Status](https://travis-ci.com/clo-yunhee/in-formant.svg?branch=with-qt)](https://travis-ci.com/clo-yunhee/in-formant)

# InFormant™ - a real-time pitch and formant tracking software

## Table of Contents

* [Prebuilt binaries](#prebuilt-binaries)
* [Compiling from source](#how-to-compile-from-source-using-docker)
  * [Using Docker](#how-to-compile-from-source-using-docker)
  * [On Apple silicon (arm64)](#how-to-compile-for-macos-on-apple-silicon-arm64)
  * [Locally](#how-to-compile-from-source-locally)

## Prebuilt binaries

There are several prebuilt binaries included with each new release:

* an AppImage file, for Linux (simply set the executable flag and run it)
* two ZIP archives, for 32-bit and 64-bit Windows (the 32-bit will run on both editions of Windows)
* a DMG image, for macOS 10.14 Mojave or newer (only 64-bit PC architecture supported, no ARM binaries yet)
* ~~an APK file, for Android 9 Pie or newer (**prefer [the Play Store version](https://play.google.com/store/apps/details?id=fr.cloyunhee.speechanalysis) for smaller downloads**)~~ 

Android is not yet supported for 3.x

## How to compile from source (using Docker)

### Preparing the build environment

The build system uses Docker images in order to have a consistent build environment regardless of the actual machine that is being used for building.

You can pull prebuilt Docker images from the Docker Hub:

```sh
for arch in linux win32 win64 macos android ; do docker pull clorika/sabuilder:$arch ; done
```

However these are not always kept up to date with the various Dockerfiles included in the repository so you can always build them yourself:

```sh
cd docker
# Replace ARCH with one of [ linux, win32, win64, macos, android ]
docker build -t clorika/sabuilder:ARCH . -f Dockerfile.ARCH
```

**Note:** The Linux Dockerfile uses the Qt online installer so the build arguments `QT_EMAIL` and `QT_PW` must be provided. Do keep in mind that build arguments are kept in the final image. I will look for an alternative way to install Qt in the future.

### Building the project

Run the `build.sh` script from the project root.

The script takes two arguments:
- The first one is the target platform: must be one of `linux`, `win32`, `win64`, `macos`, `android`.
- The second one is the build configuration: must be one of `Release`, `RelWithDebInfo`, `Debug`.

There is an additional third argument for the `android` target to specify the target architecture.

Valid values are `arm`, `arm64`, `x86` (default if not given), and `x86_64`.

Multi-arch Android builds are already supported by the underlying build system but are not possible as of now.

**Note:** This script immediately creates a native package for the given target platform after a successful build. If you wish to build the project to contribute to it, you should use a local build environment. (see [How to compile from source locally](#how-to-compile-from-source-locally))

## How to compile for macOS on Apple silicon (arm64)

The prebuilt Docker images support x86_64 hosts. This section documents how to build the macOS image and a native arm64 macOS application on an Apple M-series Mac.

The build produces a native arm64 application. The application does not need Rosetta 2. Only the Docker image needs Rosetta 2, because the toolchain exists for x86_64 Linux only.

These changes live in the `johnrichter/in-formant` fork. The `in-formant/in-formant` repository is the `upstream` remote.

### Requirements

* Rosetta 2 on the Mac.
* The Docker CLI, Docker buildx, and Colima with the `vz` virtual machine type.
* A Colima virtual machine with Rosetta, 6 CPU cores, and 8 GiB of memory.
* The project and the ccache directory inside your home directory. Colima shares only the home directory with the virtual machine.
* 8 GB of disk space for the Docker image.

### Build steps

1. Configure Colima. Colima saves these values for the next start.

```sh
colima stop
colima start --vm-type vz --vz-rosetta --cpu 6 --memory 8
```

2. Initialize the submodules that the macOS build needs.

```sh
git submodule update --init docker/macdeployqt external/tomlplusplus
```

3. Build the Docker image. The first build takes more than 15 minutes.

```sh
cd docker
docker build -t clorika/sabuilder:macos . -f Dockerfile.macos
cd ..
```

4. Build the application from the project root.

```sh
mkdir -p build/ccache
CCACHE_DIR="$PWD/build/ccache" script -q /dev/null ./build.sh macos Release
```

The `build.sh` script runs `docker run -it`, so it needs a terminal. The `script` command provides a terminal when none exists. In a normal terminal window, you can run `./build.sh macos Release` directly. You must set `CCACHE_DIR`, because `build.sh` mounts it into the container. An empty value breaks the `docker run` command.

5. Check the result. The disk image is `dist/InFormant-<version>-macOS-arm64.dmg`. A good disk image is about 55 MB.

The build stops with an error when a step fails. A disk image smaller than 1 MB is empty. Mount the disk image to check the content:

```sh
hdiutil attach -readonly dist/InFormant-*-macOS-arm64.dmg
```

The `macdeployqt` tool prints `ERROR: no file at ".../libiodbc.2.dylib"` and `ERROR: no file at ".../libpq.5.dylib"`. These messages come from the optional Qt SQL plugins. The application does not use them. Ignore these messages.

### Build for x86_64

The `TARGET_ARCH` build argument selects the architecture. The default value is `arm64`. To build an x86_64 application, build the image with `--build-arg TARGET_ARCH=x86_64`. The disk image then has the name `InFormant-<version>-macOS-x86_64.dmg`. The maintainers of this fork did not test this path after the arm64 changes.

### Changes in this fork

Build system:

* **Colima settings:** Enable Rosetta. Use 6 CPU cores and 8 GiB of memory. QEMU emulation makes `clang` crash with a segmentation fault. The default 2 GiB of memory is too small.
* **Platform in `docker/Dockerfile.macos`:** `FROM --platform=linux/amd64`. The toolchain, Qt, and the libraries exist for x86_64 Linux only.
* **Hard link in `docker/Dockerfile.macos`:** `clang++` is a hard link to `clang`. The osxcross wrapper follows symlinks. Then `clang` links in C mode, without `-lc++`.
* **Root user in `docker/Dockerfile.macos`:** `USER swu` is disabled, so the build runs as root. The packaging step writes `/InFormant.app` and `/InFormant.dmg` in `/`. Docker does not keep a `chown` of `/`.
* **Target architecture in `docker/Dockerfile.macos`:** The `TARGET_ARCH` argument sets the compiler host `arm64-apple-darwin20.4`, the `-arch` flag of PortAudio and FFTW, and the deployment target 11.0.
* **Qt 6.2.4:** Qt 6.1.2 has x86_64 macOS binaries only. Qt 6.2.4 is the first version with universal binaries. The `aqtinstall` tool 1.2.2 installs it.
* **Thin Qt files:** The Dockerfile runs `lipo -thin` on each universal Qt file. The `macdeployqt` tool cannot read the `otool` output of a universal file.
* **`qmlimportscanner` link:** Qt 6.2 moved the tool to `libexec`. The Dockerfile links it into `bin`, where `macdeployqt` looks for it.
* **Submodule `docker/macdeployqt`:** The pointer moves to commit `59fb62a`. The old commit has `load(qt_app)` in its project file. Qt 6.1.2 rejects this line.
* **Signature in `docker/deploy-macos.sh`:** The `install_name_tool` and `strip` commands invalidate the signature of each file. Apple silicon refuses to run an arm64 file with an invalid signature. The script signs the bundle again with `rcodesign` (ad hoc).
* **Disk image in `docker/deploy-macos.sh`:** The script makes the disk image itself, after the signing step. The image has 256 MB of free space, because `hfsplus` copies each symlink as a full file. The free space compresses to almost nothing.

Application:

* **Black spectrogram:** The patch of upstream pull request 40 moves the shaders from GLSL 120 to GLSL 150. It requests an OpenGL 3.2 Core profile with 4x multisampling. It anchors the canvas to the top of the window.
* **Toolbar icons:** The CMake file links the Qt SVG module. The `macdeployqt` tool then deploys `libqsvg.dylib`. Without this plugin, Qt prints `Unsupported image format` and the icons stay invisible.
* **Retina resolution:** `NSHighResolutionCapable` is `true` in `dist-res/Info.plist`. The application renders at the full resolution of the display.
* **Frame rate:** The renderer built all fonts again at each frame, and it updated a vertex buffer for each glyph, line, and point. On Apple, each buffer update sends the command buffer to the GPU. The frame rate was about 7 frames per second. The renderer now builds the fonts one time, and the shaders take the corners of each quad from a uniform. The frame rate follows the display refresh rate.
* **Exit crash:** `BaseProcessor` and `FormantSolver` had no virtual destructor, and the pipeline deleted derived objects through them. Clang compiled this undefined behavior to a trap. `main` also destroys the context manager before the static destructors run. Also, the configuration is saved at exit, so the settings were never saved before.
* **Logger process:** The logger child process did not stop at the end of the pipe, so each launch from a terminal left one process alive. It now stops.
* **Input device:** The drawer has an "Input device" list. The setting `captureDevice` in the `[audio]` table of the configuration file stores the name. An empty name selects the default input device of macOS. Before this change, the application always used the default device. This device can be a webcam microphone with a low sample rate.
* **First launch:** The spectrogram, the pitch track, and the formant tracks are on by default. Before this change, a new installation showed a black window.
* **Spectrogram jump:** The picture of the spectrogram jumped back and forth. The right edge of the texture used the column before the new slices. It now uses the column after them.
* **CPU use:** The application used 65 to 88 percent of one core. It now uses 16 to 23 percent. The window of the charts that are not visible no longer updates its series. The renderer draws all glyphs of a string, and all points of a track, in one draw call. The spectrogram code no longer copies the visible slices for each frame. The audio queue no longer fills the log, and the synthesizer sends no sound while it is off.
* **Freeze:** A quit signal could freeze the application. The signal handler called Qt in the middle of a chart update, and the update held a lock. The handler now writes one byte to a pipe, and the event loop quits.
* **Spectrogram quality:** The drawer has the list "Spectrogram quality" (Fast or Smooth) and the slider "Spectrogram blur" (0 to 100 percent). The keys `spectrogramQuality` and `spectrogramBlur` in the `[view]` table store them. The Smooth filter is a cubic filter on the square root of the power. A blur of 0 gives the sharp Catmull-Rom shape, and a blur of 100 gives the soft B-spline shape. The renderer also averages the bins and the columns that fall in one pixel. It adds noise before the colour lookup to hide steps between colours.
* **Time step:** The spectrogram has a slice each 10 ms. Before, it had a slice each 20 ms. The key `granularity` in the `[analysis]` table sets the step. A configuration file that stores 20 keeps the old step.
* **Text:** The renderer starts each string on a whole pixel, so the texture filter does not blur the glyphs.

### Troubleshooting

* **`make: *** ... Segmentation fault` in the osxcross step.** QEMU emulates x86_64. Enable Rosetta in Colima.
* **`ld: symbol(s) not found` with `std::__1::` symbols at the end of the osxcross step.** The `clang++` command runs as `clang`. Keep the hard link step in the Dockerfile.
* **`qmake` prints its usage text in the `macdeployqt` step.** The `docker/macdeployqt` submodule is empty. Run `git submodule update --init docker/macdeployqt`.
* **`Project has no top-level .qmake.conf file`.** The `docker/macdeployqt` submodule uses an old commit. Use commit `59fb62a` or newer.
* **`mkdir: cannot create directory '/InFormant.app': Permission denied`.** The container user cannot write in `/`. Run the build as root.
* **`Bundle creation error: (hfsplus) "error: allocate"` and a tiny `.dmg` file.** The disk image is too small. Keep the 256 MB margin in `docker/deploy-macos.sh`.
* **`macdeployqt` cannot parse the `otool` output.** A universal Qt file is in the bundle. Keep the `lipo -thin` step in the Dockerfile.
* **`qmlimportscanner` is not found.** Keep the link step in the Dockerfile.
* **The application closes at once, and the shell shows `Killed: 9` or exit code 137.** The signature of the bundle is invalid. Keep the `rcodesign` step in `docker/deploy-macos.sh`.
* **macOS says that the application does not run on this version of macOS.** The application is x86_64 only. Build with the default `TARGET_ARCH=arm64`.
* **The toolbar has no icons.** Check that the bundle has `Contents/PlugIns/imageformats/libqsvg.dylib`.
* **The spectrogram area is black.** The area stays black when the microphone sends no sound. Make sure that macOS gives microphone access to InFormant. To ask again, run `tccutil reset Microphone fr.cloyunhee.InFormant`, and then start the application. Choose the input device in the drawer.
* **Docker prints `requested image's platform (linux/amd64) does not match the detected host platform`.** This warning is expected on arm64. Ignore it, because Rosetta runs the image.

### Known limits

* The checks launched the application, saved a picture of the window, and confirmed the toolbar, the axes, and the text. Speech from a loudspeaker, picked up by the microphone, drew the spectrogram, the pitch track, and the formant tracks.
* The checks used the built-in input path of one Mac with a USB webcam microphone. Other input devices are not tested.
* The application has an ad hoc signature and no notarization.
* The application reads its configuration from `~/Library/Application/informant.conf`.
* The `hfsplus` tool copies each symlink as a full file. The application bundle uses 143 MB in the image, instead of 60 MB.
* The build runs as root in the container. On a Linux host, the files in `build` and `dist` belong to root.
* The Dockerfile clones the osxcross `master` branch with no pin. A future osxcross change can break the image.
* The repository tracks the Apple SDK archive `docker/MacOSX11.3.sdk.tar.xz` (52 MB). A review tool that blocks binary files can block a merge for this reason.

## How to compile from source locally

As of now, development is only really supported on Linux hosts, but is likely feasible on other Unix-like systems with some work.

Before anything, you must install the various dependencies. Specifics will vary depending on your distribution.

* Qt 5.15.2 (the Quick Controls 2 module and the Charts module are required in addition to the base package)
* FFTW3
* Eigen3
* libtorch built with C++11 ABI (from PyTorch)
* PortAudio v19 
* PulseAudio (optional, unstable due to race condition)
* ALSA (optional, causes inexplicably high CPU usage)

You will also need CMake and a compiler with C++17 support.

This should be all, you're ready to start building:

```sh
mkdir local
cd local
cmake ..
make -j$(nproc)
```

You may need to edit the CMake command line if some of those dependencies are installed to non-standard paths.

```sh
cmake .. -DCMAKE_PREFIX_PATH="/opt/Qt/5.15.2/gcc_64;/usr/local/libtorch"
```

You can also specify a build configuration between `Debug`, `RelWithDebInfo`, `MinSizeRel`, and `Release`.

```sh
cmake .. -DCMAKE_BUILD_TYPE=Release
```

**Note:** the `Debug` build configuration enables ASan and UBSan by default.

