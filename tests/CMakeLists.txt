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

informant_add_test(NAME framework_tests SOURCES framework_test.cpp LABELS framework)

# The check that each case of type Unit in docs/test-plan.md has a test or a recorded reason.
add_test(NAME plan_coverage
    COMMAND python3 -I ${CMAKE_CURRENT_SOURCE_DIR}/check_plan_coverage.py
        --plan ${PROJECT_SOURCE_DIR}/docs/test-plan.md
        --tests ${CMAKE_CURRENT_SOURCE_DIR}
        --deferred ${CMAKE_CURRENT_SOURCE_DIR}/deferred.txt)
set_tests_properties(plan_coverage PROPERTIES LABELS "script")
