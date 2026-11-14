# InFormant: guide for Claude

InFormant shows the pitch and the formants of a voice in real time, for voice training and singing. This repo is the fork `johnrichter/in-formant`. Read this page first. The rules in `.claude/rules/` load when you touch their files.

## Facts to know first

- **Platform:** macOS 27 or newer on Apple silicon (arm64) only. The code for Linux, Windows, Android, x86_64, Docker, and Torch is gone. Do not add it back.
- **Branches:** `with-qt` is the main branch. `origin` is the fork. `upstream` is `in-formant/in-formant`, for `git fetch` only. The history differs from upstream, because the fork moved all binary files to Git LFS.
- **Stack:** C++17, Qt 6.12 (Qt Quick and QML), PortAudio with CoreAudio, FFTW 3.3.11, libsamplerate, Eigen, toml++, FreeType. `build-macos.sh` pins each version with a SHA-256 checksum.
- **Name and identity:** The bundle ID stays the upstream ID, because a new ID resets the microphone permission of each user. The build has an Apple Development signature only and no notarization. A new bundle ID waits for a new product name.
- **Successor:** A new app in Flutter and Dart is under study. Keep the work in this repo to fixes and features of the Qt app.

## Commands

- **Build the app and the DMG:** `./build-macos.sh`. The first run downloads Qt and takes a long time.
- **Run all tests:** `./run-tests.sh`. It needs one `build-macos.sh` run in the same tree.
- **Run the sanitizer tests:** `./run-tests.sh --sanitize`.
- **Run one group of tests:** `./run-tests.sh -R TP_LAY`.
- **Check the test plan coverage:** `python3 tests/check_plan_coverage.py --report`, with the options in `CONTRIBUTING.md`.

`CONTRIBUTING.md` has the full test, commit, and version rules. `README.md` has the build settings and the list of changes in this fork. Do not copy them here.

## Map of the code

- **`src/analysis/`:** Signal processing. It has the FFT, filters, LPC (Burg, autocorrelation, covariance), formants (`FilteredLP`), pitch (YIN, MPM, RAPT), and glottal inverse filtering (IAIF, GFM-IAIF). `wavelet/` and `amgif.cpp` are dead code.
- **`src/synthesis/` and `src/modules/app/synthesizer/`:** The LF glottal source, the noise, and the formant filter of the synthesizer.
- **`src/modules/audio/`:** PortAudio capture and playback, the lock-free `Buffer`, and the resampler.
- **`src/modules/app/pipeline/`:** The analysis pipeline. Four processors (spectrogram, pitch, formants, oscilloscope) share one sliding window.
- **`src/context/`:** `ContextManager` (threads and the audio life cycle), `Config` and its migrations, and `DataStore` and `Session` (stored sound and tracks). It also holds the practice library, the recorder, and the Qt wrappers.
- **`src/gui/`:** The graph renderer (OpenGL and QPainter), axes, themes, color maps, and layout math. `uiscale.h` holds the design tokens.
- **`src/qml/`:** All windows: main, Settings, Synthesizer, Oscilloscope, Progress, and Help.
- **`docs/`:** The help pages, `design-system.md`, and `test-plan.md`. The app loads the help pages through `resources/qml.qrc`.
- **`tests/`:** GoogleTest suites, one for each area, and `app_smoke.py` for the built app.

The sound goes from the PortAudio capture callback to `Buffer`, and then to `Pipeline` with its processors. The results go to `DataStore` and `Session`. The renderer reads them on the GUI thread. These threads run all the time:

- The PortAudio callbacks for capture and playback.
- The analysis loop of `ContextManager`, which pulls from `Buffer` into `Pipeline::processAll`.
- The processing thread of `Pipeline`, which runs the processors.
- The datavis loop, which feeds the oscilloscope and sends the pitch to the synthesizer.
- The synthesis loop, which makes sound only while the synthesizer is on.
- The GUI thread.

Short-lived threads load files, export sound, and build the spectrogram of long views (`ReviewSpectrogram`, `FileAnalyzer`). The rule `threads-and-locks.md` says how threads share data.

## How to work in this repo

- **Worktrees:** Make each change in `.claude/worktrees/<slug>` on a branch named `type/slug`, for example `fix/render-gap`. Never write in the primary checkout. Land with the `git-tools merge --push` verb from the primary checkout. Then remove the worktree and the branch.
- **Signing:** Each commit has a signature from the 1Password SSH agent. When signing fails, stop and ask the user to unlock 1Password. Never turn off signing, and never commit without a signature.
- **Commits:** Write release-note messages in Simplified Technical English. Stage each file by name. Store each binary file in Git LFS, and run `git lfs status` before a commit.
- **Versions:** A bug fix raises the patch number. A feature raises the minor number. Change `version` and `CUR_VERSION` in `CMakeLists.txt` in the same commit. Test code and documents do not change the version.
- **Tests:** A new feature or a fixed bug gets cases in `docs/test-plan.md` and tests in the same change. A failing test is a bug candidate. Fix the program, and never weaken the check.
- **Review:** Before you land a large change, run a read-only Opus reviewer at low or medium effort. Apply its findings, or tell the user why you reject one.
- **Release:** A release is a signed DMG from `build-macos.sh`. Copy it to `~/Downloads`. The DMG must not contain a temporary test hook (see `test-hook-never-ships.md`).
- **Proof:** For a change to drawing or layout, launch the app and look at a screenshot. Unit tests alone do not prove a visual change. When you did not listen to a sound change, say so.

## Design and engineering direction

- **macOS native first.** Use the newest macOS APIs where they help. Support only operating systems that the vendor supports, and no system older than 3 years.
- **Low idle cost.** Do no work when nothing changes. Do not redraw a still graph, run a stopped synthesizer, or wait in a busy loop. CPU use fell from about 80 percent of one core to about 20 percent. Keep it there.
- **One clock.** The count of stored samples is the clock of all live data. The wall clock only smooths the live view, by 0.1 seconds at most.
- **One design system.** Take every size, space, radius, and color from `src/gui/uiscale.h` (`ui.*` in QML). All controls have a radius of 8 pixels. `docs/design-system.md` has the rules.
- **Every control explains itself.** A control has a gray hint under it. A `HelpButton` ("?") opens a longer explanation. The help pages explain the voice science in plain language.
- **Respectful defaults.** The pitch zones are a rough stereotype, not a goal, and the text says so. Never delete a file that the user saved. The setting `keepRecordings` controls only the scratch sound file.
- **Plain language.** Write the help pages, the hints, the comments, and the commit messages in Simplified Technical English with American spelling. The style checker misses phrasal verbs, passive verbs, and broken sentences, so also read each page yourself.

## Traps that cost time before

- Startup can wait about 12 seconds in CoreAudio while it opens the input device. The wait is in the macOS audio server, not in this code.
- A screenshot read can fail with `decoder_unavailable`. Take a new capture in PNG at a smaller size, and do not guess the content.
- The macOS microphone permission belongs to the bundle ID. To ask again, run `tccutil reset Microphone fr.cloyunhee.InFormant`.
- The config file is `~/Library/Application Support/informant.conf`. Write the "Application Support" part in full. A broken file starts the app with defaults and moves to `informant.conf.broken`.
- `macdeployqt` prints three errors about SQL plugins. Ignore them.
- The ASan build can report an overflow inside GoogleTest registration, with no InFormant frame on the stack. Check the stack before you change InFormant code.
