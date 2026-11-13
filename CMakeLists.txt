cmake_minimum_required(VERSION 3.24)

if(NOT "x$ENV{MACOSX_DEPLOYMENT_TARGET}" STREQUAL "x")
    set(CMAKE_OSX_DEPLOYMENT_TARGET $ENV{MACOSX_DEPLOYMENT_TARGET})
elseif(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    # The oldest macOS version that the application supports.
    set(CMAKE_OSX_DEPLOYMENT_TARGET 27.0)
endif()

if("x${CUR_VERSION}" STREQUAL "x")
    set(CUR_VERSION "3.4.0")
endif()

string(FIND "${CUR_VERSION}" "-" dash_pos)
string(SUBSTRING "${CUR_VERSION}" 0 ${dash_pos} parsed_version)
project(in-formant VERSION ${parsed_version})

if(NOT APPLE)
    message(FATAL_ERROR "InFormant builds on macOS only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_FLAGS "-Wall -Wextra -fPIC -ftree-vectorize -fno-omit-frame-pointer -fno-math-errno -ffast-math -fno-finite-math-only -fvisibility=hidden")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -fPIC -ftree-vectorize -fno-omit-frame-pointer -fno-math-errno -ffast-math -fno-finite-math-only -fvisibility=hidden -fvisibility-inlines-hidden")
set(CMAKE_C_FLAGS_DEBUG           "-Og -g")
set(CMAKE_C_FLAGS_RELEASE         "-O3 -g0 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO  "-O3 -g")
set(CMAKE_C_FLAGS_MINSIZEREL      "-Os -g0 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG           "-Og -g")
set(CMAKE_CXX_FLAGS_RELEASE         "-O3 -g0 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O3 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL      "-Os -g0 -DNDEBUG")

set(CMAKE_ENABLE_EXPORTS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)

set(SOURCES
    src/main.cpp
    src/file_logger.cpp
    src/file_logger.h
    src/timetrack.ipp
    src/timetrack.h
    src/context/timings.cpp
    src/context/timings.h
    src/context/solvermakers.cpp
    src/context/solvermakers.h
    src/context/synthwrapper.cpp
    src/context/synthwrapper.h
    src/context/dataviswrapper.cpp
    src/context/dataviswrapper.h
    src/context/config.cpp
    src/context/config.h
    src/context/contextmanager.cpp
    src/context/contextmanager.h
    src/context/audiocontext.cpp
    src/context/audiocontext_streams.cpp
    src/context/audiocontext.h
    src/context/rendercontext.cpp
    src/context/rendercontext.h
    src/context/guicontext.cpp
    src/context/guicontext.h
    src/context/datastore.cpp
    src/context/audiofile.cpp
    src/context/audiofile.h
    src/context/fileanalyzer.cpp
    src/context/fileanalyzer.h
    src/context/devicewatcher.h
    src/context/recorder.cpp
    src/context/recorder.h
    src/context/recordstats.cpp
    src/context/practicelib.cpp
    src/context/practicelib.h
    src/context/practicewrapper.cpp
    src/context/practicewrapper.h
    src/context/summary.h
    src/context/recordstats.h
    src/context/session.cpp
    src/context/videowriter.h
    src/context/sessionplayer.cpp
    src/context/sessionplayer.h
    src/context/sessionwrapper.cpp
    src/context/sessionwrapper.h
    src/context/session.h
    src/context/reviewspectrogram.cpp
    src/context/reviewspectrogram.h
    src/context/datastore.h
    src/context/views/views.h
    src/context/views/spectrogram.cpp
    src/context/views/spectrogram.h
    src/gui/qpainterwrapper.cpp
    src/gui/qpainterwrapper.h
    src/gui/qpainterwrapper_static.cpp
    src/gui/plotitem.cpp
    src/gui/overviewitem.cpp
    src/gui/overviewitem.h
    src/gui/qpainterwrapper_overlay.cpp
    src/gui/theme.cpp
    src/gui/ticks.cpp
    src/gui/helpstyle.cpp
    src/gui/helpstyle.h
    src/gui/cmap.cpp
    src/gui/shaders/text.h
    src/gui/font.cpp
    src/gui/font.h
    src/gui/canvas_item.cpp
    src/gui/canvas_renderer.cpp
    src/gui/canvas.h
    src/modules/audio/base/base.cpp
    src/modules/audio/base/base.h
    src/modules/audio/buffer/buffer.cpp
    src/modules/audio/buffer/buffer.h
    src/modules/audio/queue/queue.cpp
    src/modules/audio/queue/queue.h
    src/modules/audio/resampler/resampler.cpp
    src/modules/audio/resampler/resampler.h
    src/modules/audio/audio.h
    src/modules/app/pipeline/processors/base.cpp
    src/modules/app/pipeline/processors/base.h
    src/modules/app/pipeline/processors/formants.cpp
    src/modules/app/pipeline/processors/formants.h
    src/modules/app/pipeline/processors/oscilloscope.cpp
    src/modules/app/pipeline/processors/oscilloscope.h
    src/modules/app/pipeline/processors/pitch.cpp
    src/modules/app/pipeline/processors/pitch.h
    src/modules/app/pipeline/processors/spectrogram.cpp
    src/modules/app/pipeline/processors/spectrogram.h
    src/modules/app/pipeline/pipeline.cpp
    src/modules/app/pipeline/pipeline.h
    src/modules/app/synthesizer/synthesizer.cpp
    src/modules/app/synthesizer/synthesizer.h
    src/modules/app/app.h
    src/modules/modules.h
    src/analysis/fft/realfft.cpp
    src/analysis/fft/complexfft.cpp
    src/analysis/fft/realrealfft.cpp
    src/analysis/fft/wisdom.cpp
    src/analysis/fft/fft_n.cpp
    src/analysis/fft/fft.h
    src/analysis/freqz/sosfreqz.cpp
    src/analysis/freqz/freqz.h
    src/analysis/filterbanks/linear.cpp
    src/analysis/filterbanks/log.cpp
    src/analysis/filterbanks/mel.cpp
    src/analysis/filterbanks/erb.cpp
    src/analysis/filterbanks/filterbanks.h
    src/analysis/filter/butterworth.cpp
    src/analysis/filter/zpk2sos.cpp
    src/analysis/filter/sosfilter.cpp
    src/analysis/filter/filter.cpp
    src/analysis/filter/filter.h
    #src/analysis/pitch/amdf_m.cpp
    src/analysis/pitch/yin.cpp
    src/analysis/pitch/mpm.cpp
    src/analysis/pitch/rapt.cpp
    src/analysis/pitch/rapt.h
    src/analysis/pitch/pitch.h
    src/analysis/linpred/autocorr.cpp
    src/analysis/linpred/covar.cpp
    src/analysis/linpred/burg.cpp
    src/analysis/linpred/linpred.h
    src/analysis/formant/simplelp.cpp
    src/analysis/formant/filteredlp.cpp
    src/analysis/formant/formant.h
    src/analysis/invglot/iaif.cpp
    src/analysis/invglot/gfm_iaif.cpp
    #src/analysis/invglot/amgif.cpp
    src/analysis/invglot/invglot.h
    src/analysis/wavelet/wavelet.h
    src/analysis/wavelet/wavelets.cpp
    src/analysis/wavelet/wavelets.h
    src/analysis/wavelet/common.cpp
    src/analysis/wavelet/common.h
    src/analysis/wavelet/coeffs.cpp
    src/analysis/wavelet/coeffs.h
    src/analysis/wavelet/convolution.cpp
    src/analysis/wavelet/convolution.h
    src/analysis/wavelet/wt.cpp
    src/analysis/wavelet/wt.h
    src/analysis/util/find_peaks.cpp
    src/analysis/util/find_roots.cpp
    src/analysis/util/aberth.cpp
    src/analysis/util/aberth.h
    src/analysis/util/laguerre.cpp
    src/analysis/util/laguerre.h
    src/analysis/util/polish_root.cpp
    src/analysis/util/eval_polynomial.cpp
    src/analysis/util/calc_formant.cpp
    src/analysis/util/sort_formants.cpp
    src/analysis/util/parabolic_interpolation.cpp
    src/analysis/util/zerocros.cpp
    src/analysis/util/util.h
    src/analysis/analysis.h
    src/synthesis/noise.cpp
    src/synthesis/voice.cpp
    src/synthesis/filter.cpp
    src/synthesis/sosfilter.cpp
    src/synthesis/glotLf.cpp
    src/synthesis/shiftFilter.cpp
    src/synthesis/polyFromRoots.cpp
    src/synthesis/synthesis.h
    src/tomlplusplus.cpp
)

# The video file needs the encoder of macOS.
list(APPEND SOURCES src/context/videowriter_mac.mm)
set_source_files_properties(src/context/videowriter_mac.mm PROPERTIES LANGUAGE CXX COMPILE_FLAGS "-x objective-c++ -fobjc-arc")

# macOS tells the application when the default input device changes.
list(APPEND SOURCES src/context/devicewatcher_mac.mm)
set_source_files_properties(src/context/devicewatcher_mac.mm PROPERTIES LANGUAGE CXX COMPILE_FLAGS "-x objective-c++ -fobjc-arc")

# The permission to use the microphone. The program asks AVFoundation directly.
list(APPEND SOURCES src/context/micpermission_mac.mm)
set_source_files_properties(src/context/micpermission_mac.mm PROPERTIES LANGUAGE CXX COMPILE_FLAGS "-x objective-c++ -fobjc-arc")

### REQUIRED MODULES

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW REQUIRED fftw3)
pkg_check_modules(portaudio REQUIRED portaudio-2.0)

find_package(Eigen3 REQUIRED NO_MODULE)

# The finder of Qt for OpenGL also links the AGL framework, and the macOS SDK 27 has no AGL. The
# finder skips its search when this value exists, so give it a framework that exists.
set(WrapOpenGL_AGL "-framework OpenGL" CACHE STRING "Replacement for the AGL framework" FORCE)

find_package(Qt6 COMPONENTS Quick QuickControls2 QuickTemplates2 Qml Widgets OpenGL Svg Gui Core REQUIRED)
foreach(c Quick QuickControls2 QuickTemplates2 Qml Widgets OpenGL Svg Gui Core)
    if(${Qt6${c}_FOUND})
        get_target_property(Qt6${c}_LIBRARY_FILE ${Qt6${c}_LIBRARIES} LOCATION)
        message(STATUS "Found Qt6${c}: ${Qt6${c}_LIBRARY_FILE}")
    else()
        message(FATAL_ERROR "Qt6${c} not found!")
    endif()
endforeach()
qt6_add_resources(RESOURCES_OBJ resources/qml.qrc)
qt6_add_big_resources(RESOURCES_OBJ resources/other.qrc)
set_property(SOURCE "${RESOURCES_OBJ}" PROPERTY SKIP_AUTOMOC ON)

set(TOMLPP_INCLUDE_DIR external/tomlplusplus/include)

### AUDIO MODULE

# PortAudio is the only audio module. It uses CoreAudio on macOS.
list(APPEND SOURCES
    src/modules/audio/portaudio/portaudio.cpp
    src/modules/audio/portaudio/portaudio.h
)

### CREATE CMAKE TARGET

add_executable(in-formant ${SOURCES} ${RESOURCES_OBJ})

### TESTS

# The tests cover the logic that has no window or sound device. Run them with ctest.
option(INFORMANT_BUILD_TESTS "Build the unit tests" OFF)
if(INFORMANT_BUILD_TESTS)
    enable_testing()
    # One target builds all tests. The script run-tests.sh builds this target and not the application.
    add_custom_target(unit_tests)
    # The tests with GoogleTest. See tests/CMakeLists.txt.
    add_subdirectory(tests)
endif()

target_include_directories(in-formant SYSTEM PRIVATE ${FFTW_INCLUDE_DIRS} ${portaudio_INCLUDE_DIRS} ${TOMLPP_INCLUDE_DIR})
target_link_libraries(in-formant PRIVATE Eigen3::Eigen ${FFTW_LIBRARIES} ${portaudio_LIBRARIES} Qt6::Quick Qt6::QuickControls2 Qt6::QuickTemplates2 Qt6::Qml Qt6::Widgets Qt6::OpenGL Qt6::Svg Qt6::Gui Qt6::Core)
target_link_directories(in-formant PRIVATE ${FFTW_LIBRARY_DIRS} ${portaudio_LIBRARY_DIRS})
target_link_libraries(in-formant PRIVATE "-framework AudioToolbox" "-framework CoreAudio" "-framework CoreFoundation" "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo" "-framework Foundation")


set(BUILD_SHARED_LIBS TRUE)
add_subdirectory(external/freetype EXCLUDE_FROM_ALL)
target_link_libraries(in-formant PRIVATE freetype)
set(BUILD_SHARED_LIBS)

add_subdirectory(external/rpmalloc EXCLUDE_FROM_ALL)
target_link_libraries(in-formant PRIVATE rpcxx_only)

add_subdirectory(external/libsamplerate)
target_link_libraries(in-formant PRIVATE lsr)
target_include_directories(in-formant PRIVATE external/libsamplerate/src)

target_compile_definitions(in-formant PRIVATE
    -DINFORMANT_VERSION=${CUR_VERSION}
    -DTOML_HEADER_ONLY=0 -DEIGEN_DONT_PARALLELIZE)

if(CMAKE_BUILD_TYPE STREQUAL Release)
    set_property(SOURCE "${SOURCES}" PROPERTY COMPILE_FLAGS "-flto")
endif()

target_compile_options(in-formant PRIVATE -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
