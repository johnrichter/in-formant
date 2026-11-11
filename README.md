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

The prebuilt Docker images support x86_64 hosts. This section documents how to build the macOS image and the macOS application on an Apple M-series Mac.

The build produces an x86_64 application. The application runs on arm64 through Rosetta 2. A native arm64 build does not exist.

These changes live in the `johnrichter/in-formant` fork. The `in-formant/in-formant` repository is the `upstream` remote.

### Requirements

* Rosetta 2 on the Mac.
* The Docker CLI, Docker buildx, and Colima with the `vz` virtual machine type.
* A Colima virtual machine with Rosetta, 6 CPU cores, and 8 GiB of memory.
* The project and the ccache directory inside your home directory. Colima shares only the home directory with the virtual machine.
* 5.5 GB of disk space for the Docker image.

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

5. Check the result. The disk image is `dist/InFormant-<version>-macOS-x86_64.dmg`.

The `build.sh` script exits with code 0 even when `macdeployqt` fails to fill the disk image. Search the build output for `Bundle creation error`. A disk image smaller than 1 MB is empty. A good disk image is about 55 MB. Mount the disk image to check the content:

```sh
hdiutil attach -readonly dist/InFormant-*-macOS-x86_64.dmg
```

### Changes in this fork

* **Colima settings:** Enable Rosetta. Use 6 CPU cores and 8 GiB of memory. QEMU emulation makes `clang` crash with a segmentation fault. The default 2 GiB of memory is too small.
* **Platform in `docker/Dockerfile.macos`:** `FROM --platform=linux/amd64`. The toolchain, Qt, and the libraries exist for x86_64 only.
* **Hard link in `docker/Dockerfile.macos`:** `clang++` is a hard link to `clang`. The osxcross wrapper follows symlinks. Then `clang` links in C mode, without `-lc++`.
* **Root user in `docker/Dockerfile.macos`:** `USER swu` is disabled, so the build runs as root. The packaging step writes `/InFormant.app` and `/InFormant.dmg` in `/`. Docker does not keep a `chown` of `/`.
* **Disk image margin in `docker/Dockerfile.macos`:** A `sed` step raises the margin from 32 MB to 256 MB. The `macdeployqt` tool sizes the image as the bundle size plus 32 MB. This is too small, and `hfsplus` fails with `error: allocate`.
* **Submodule `docker/macdeployqt`:** The pointer moves to commit `59fb62a`. The old commit has `load(qt_app)` in its project file. Qt 6.1.2 rejects this line.

### Troubleshooting

* **`make: *** ... Segmentation fault` in the osxcross step.** QEMU emulates x86_64. Enable Rosetta in Colima.
* **`ld: symbol(s) not found` with `std::__1::` symbols at the end of the osxcross step.** The `clang++` command runs as `clang`. Keep the hard link step in the Dockerfile.
* **`qmake` prints its usage text in the `macdeployqt` step.** The `docker/macdeployqt` submodule is empty. Run `git submodule update --init docker/macdeployqt`.
* **`Project has no top-level .qmake.conf file`.** The `docker/macdeployqt` submodule uses an old commit. Use commit `59fb62a` or newer.
* **`mkdir: cannot create directory '/InFormant.app': Permission denied`.** The container user cannot write in `/`. Run the build as root.
* **`Bundle creation error: (hfsplus) "error: allocate"` and a tiny `.dmg` file.** The disk image is too small. Keep the `sed` step in the Dockerfile.
* **Docker prints `requested image's platform (linux/amd64) does not match the detected host platform`.** This warning is expected on arm64. Ignore it, because Rosetta runs the image.

### Known limits

* The application is x86_64 only. A native arm64 build needs an arm64 toolchain, Qt, and libraries.
* The checks mounted the `.dmg` and read its content. The checks did not launch the application.
* The `hfsplus` tool copies each symlink as a full file. The application bundle uses 143 MB in the image, instead of 60 MB.
* The build runs as root in the container. On a Linux host, the files in `build` and `dist` belong to root.
* The Dockerfile clones the osxcross `master` branch with no pin. A future osxcross change can break the image.

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

