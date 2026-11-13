# Contributing to InFormant

This page tells you how to build the application, run and write the tests, and prepare a change. Read it before you send a change.

## Build the application

- Run `./build-macos.sh` on an Apple silicon Mac with macOS 27 or newer. The script installs Qt and the libraries in `build/native`, builds the application, and makes a disk image in `dist`.
- The README has the full list of requirements and settings.

## Run the tests

The unit tests use GoogleTest and GoogleMock. Run `./build-macos.sh` one time first, because the tests use the same Qt and the same libraries.

- Run `./run-tests.sh` to build and run all tests. The script uses its own build folder, `build/native/build-tests`. The application build contains no test code.
- Run `./run-tests.sh --sanitize` to build the tests with the address and undefined behavior sanitizers. Run this before you send a change that touches memory or numbers.
- Run `./run-tests.sh -R TP_LAY` to run the tests whose name has `TP_LAY`. Any argument after the options goes to CTest.
- Run `./run-tests.sh -L script` to run only the script tests.
- Run `./run-tests.sh -V -R Ticks` to see the full output of the tests of one suite.
- Run `./run-tests.sh -L app` to run only the app tests. These tests start the built application in `build/native/stage/InFormant.app`, and they take about 2 minutes.

The app tests (labels `app`, names that start with `App.`) run `tests/app_smoke.py`. Run `./build-macos.sh` first. When the bundle is missing, CTest skips the app tests. To skip them on purpose, set `INFORMANT_NO_APP_TESTS=1`. The tests start the application as Finder does, from the root folder. They write to `~/Library/Logs/InFormant` and remove `informant.conf` at the end. Do not use the application during the tests.

All tests must pass before you send a change. A failing test is a bug candidate. Fix the program in the same change, and never weaken a check to make a test pass.

The first run downloads GoogleTest 1.18.0. The download is pinned with a hash. To work with no network, set `-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=<folder>` in the CMake call of `run-tests.sh`.

## Write a test

- Put the test in `tests/`, in a file that ends in `_test.cpp`. Add the file to `tests/CMakeLists.txt` with `informant_add_test`.
- Test pure logic. Move logic out of a class that needs a window, a file, or a sound device. The project keeps this logic in header files, for example `src/gui/maplayout.h` and `src/gui/oklab.h`.
- Start the name of a test with the ID of its case in `docs/test-plan.md`, for example `TEST(Layout, TP_LAY_01_BoxesCentered)`. A test with no case in the plan has a plain name.
- Use `EXPECT_*` for a check that lets the test go on, and `ASSERT_*` for a check that makes the rest of the test pointless.
- Use `QTemporaryDir` for files. Never write to the folder of the user.
- A test with a time limit allows three times the limit of the plan, because a busy machine is slower.
- Two tests need an opt-in. The test `TP_DOC_01` needs the variable `INFORMANT_STE_CHECK` with the path of the style script. The test `TP_DOC_02_EachSourceAddressAnswersWith200` needs the variable `INFORMANT_CHECK_LINKS` and the network.
- Never leave a process running after a test. Never touch an installed `InFormant` application. Remove `informant.conf` after a test that starts the application.

## The test plan and the coverage check

`docs/test-plan.md` lists the test cases. Each case has an ID, a type (Unit, Qt, Image, Audio, App, or Manual), and a priority.

- Each case of type Unit needs a test or a line in `tests/deferred.txt`. The format of the line is `ID | package | reason`.
- The script test `plan_coverage` runs `tests/check_plan_coverage.py`. It fails when a Unit case has neither, when a deferred line names an unknown case, and when a deferred case already has a test.
- Run `python3 tests/check_plan_coverage.py --plan docs/test-plan.md --tests tests --deferred tests/deferred.txt --report` to see the open cases for each package.
- When you add a feature or find a bug, add its cases to the plan in the same change.

## Versions

The version is in the file `version` and in the default of `CUR_VERSION` in `CMakeLists.txt`. Change both. Use semantic versioning:

- A bug fix raises the patch number. One release can hold several fixes.
- A new feature raises the minor number and sets the patch number to 0.
- The test code and the documents do not change the version.

## Commits

- Write the message in the style of a release note. The subject is a command of about 50 characters, with no period at the end.
- Put the details in the sections `Added:`, `Changed:`, `Fixed:`, and `Notes:`. Each item has one line.
- Sign each commit.
- Stage each file by name. Do not use `git add -A`.
- Store each binary file (images, fonts, archives) in Git LFS. Run `git lfs status` before you commit.
- Never commit a temporary test hook in `src/context/guicontext.cpp`.

## Writing

Write the documents, the messages in the application, the comments, and the commit messages in Simplified Technical English (ASD-STE100):

- Use sentences of 20 words or fewer for an instruction, and 25 words or fewer for other text.
- Use the active voice and the simple tenses. Do not use contractions.
- Use American spelling.
- Use one name for one thing.

## Spacing and type

All windows and the graph use the scales in `src/gui/uiscale.h`. Use a step of the spacing scale for each space. A tick mark, a line width, or a dot size is a mark and has no step. The test `ui_scale` checks the scales.
