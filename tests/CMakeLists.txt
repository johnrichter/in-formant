# The unit tests with GoogleTest and GoogleMock. The root CMake file includes this folder when
# INFORMANT_BUILD_TESTS is on. A release build never downloads or builds any test code.

include(FetchContent)

option(INFORMANT_SANITIZE_TESTS "Build the tests with the address and undefined behavior sanitizers" OFF)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# The release archive is pinned with its hash. A Git submodule is not used, because a submodule stops
# "git worktree remove". With CMake 3.24 or newer, an installed GoogleTest wins over the download.
set(GTEST_FETCH_ARGS)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    list(APPEND GTEST_FETCH_ARGS DOWNLOAD_EXTRACT_TIMESTAMP TRUE FIND_PACKAGE_ARGS NAMES GTest)
endif()
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/releases/download/v1.18.0/googletest-1.18.0.tar.gz
    URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
    ${GTEST_FETCH_ARGS})
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# informant_add_test(NAME name SOURCES a.cpp ... [LIBS lib ...] [LABELS label ...])
# Make a test executable, register each test of it with CTest, and add it to the target "unit_tests".
# The sanitizers apply to the test targets only. They never change the flags of the application.
function(informant_add_test)
    cmake_parse_arguments(T "" "NAME" "SOURCES;LIBS;LABELS" ${ARGN})
    add_executable(${T_NAME} ${T_SOURCES})
    target_link_libraries(${T_NAME} PRIVATE GTest::gmock GTest::gtest_main ${T_LIBS})
    target_include_directories(${T_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/src)
    if(INFORMANT_SANITIZE_TESTS)
        target_compile_options(${T_NAME} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${T_NAME} PRIVATE -fsanitize=address,undefined)
    endif()
    gtest_discover_tests(${T_NAME}
        DISCOVERY_TIMEOUT 60
        PROPERTIES LABELS "${T_LABELS}")
    add_dependencies(unit_tests ${T_NAME})
endfunction()

set(SRC ${PROJECT_SOURCE_DIR}/src)

informant_add_test(NAME framework_tests SOURCES framework_test.cpp LABELS framework)

# The logic that has no window and no sound device. Most of it is in header files.
informant_add_test(NAME bands_tests SOURCES bands_test.cpp LABELS bands)
informant_add_test(NAME pitch_axis_tests SOURCES pitch_axis_test.cpp LABELS pitch_axis)
informant_add_test(NAME readout_layout_tests SOURCES readout_layout_test.cpp LABELS readout_layout)
informant_add_test(NAME oklab_tests SOURCES oklab_test.cpp LABELS oklab)
informant_add_test(NAME map_layout_tests SOURCES map_layout_test.cpp LABELS map_layout)
informant_add_test(NAME ui_scale_tests SOURCES ui_scale_test.cpp LABELS ui_scale)
informant_add_test(NAME ticks_tests SOURCES ticks_test.cpp ${SRC}/gui/ticks.cpp LABELS ticks)

# The migration of the settings file needs the TOML library.
add_library(tomlpp_for_tests STATIC ${SRC}/tomlplusplus.cpp)
target_include_directories(tomlpp_for_tests SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/${TOMLPP_INCLUDE_DIR})
informant_add_test(NAME config_migration_tests SOURCES config_migration_test.cpp LIBS tomlpp_for_tests LABELS config_migration)

# The settings. The configuration class needs the theme code and the TOML library. The audio backend
# lookup lives in the audio code, so the test program supplies a stand-in for it.
informant_add_test(NAME config_tests
    SOURCES config_test.cpp ${SRC}/context/config.cpp ${SRC}/gui/theme.cpp
    LIBS tomlpp_for_tests Eigen3::Eigen rpcxx_only Qt6::Quick Qt6::QuickControls2 Qt6::QuickTemplates2 Qt6::Qml Qt6::Widgets Qt6::OpenGL Qt6::Svg Qt6::Gui Qt6::Core freetype lsr
    LABELS config)
target_include_directories(config_tests SYSTEM PRIVATE ${ARMADILLO_INCLUDE_DIR} ${FFTW_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/external/libsamplerate/src)

# The practice library uses QDir, QFile, and QDateTime, and the tests use QTemporaryDir for the files.
informant_add_test(NAME practice_tests SOURCES practice_test.cpp ${SRC}/context/practicelib.cpp LIBS Qt6::Core LABELS practice)

# The offline test of the synthesizer voice. It needs the filter code and the allocator only.
informant_add_test(NAME voice_tests
    SOURCES voice_test.cpp
        ${SRC}/synthesis/noise.cpp ${SRC}/synthesis/filter.cpp ${SRC}/synthesis/sosfilter.cpp
        ${SRC}/synthesis/glotLf.cpp ${SRC}/synthesis/shiftFilter.cpp ${SRC}/synthesis/polyFromRoots.cpp
        ${SRC}/synthesis/voice.cpp
        ${SRC}/analysis/filter/butterworth.cpp ${SRC}/analysis/filter/zpk2sos.cpp
        ${SRC}/analysis/filter/sosfilter.cpp ${SRC}/analysis/filter/filter.cpp
        ${SRC}/analysis/freqz/sosfreqz.cpp ${SRC}/analysis/util/eval_polynomial.cpp
    LIBS Eigen3::Eigen rpcxx_only Qt6::Core
    LABELS voice)
target_include_directories(voice_tests SYSTEM PRIVATE ${ARMADILLO_INCLUDE_DIR} ${FFTW_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/external/libsamplerate/src)

# The check that each case of type Unit in docs/test-plan.md has a test or a recorded reason.
add_test(NAME plan_coverage
    COMMAND python3 -I ${CMAKE_CURRENT_SOURCE_DIR}/check_plan_coverage.py
        --plan ${PROJECT_SOURCE_DIR}/docs/test-plan.md
        --tests ${CMAKE_CURRENT_SOURCE_DIR}
        --deferred ${CMAKE_CURRENT_SOURCE_DIR}/deferred.txt)
set_tests_properties(plan_coverage PROPERTIES LABELS "script")
