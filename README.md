# InFormant™ - a real-time pitch and formant tracking software

## About this repository

This repository is a fork of [in-formant/in-formant](https://github.com/in-formant/in-formant), the upstream project. It keeps the upstream history and adds the changes that the sections below describe.

The repository left the GitHub fork network, for two reasons:

* GitHub refuses new Git LFS objects on a public fork, unless the fork network already has LFS objects.
* The upstream repository does not use Git LFS. It is about 220 MB, because it stores binary files as plain Git objects. This fork stores all binary files in Git LFS, so the repository is much smaller.

The move to Git LFS rewrote the history of this repository. Its commit IDs differ from the commit IDs of upstream. The content of each file is the same. The `upstream` remote still works for `git fetch`, but a merge or a rebase on upstream needs more care than before. To give a change to upstream, send a patch (`git format-patch`) or a branch that has no LFS files. Submodules keep their own files and are not in Git LFS.

Before you clone this repository, install Git LFS (for example with `brew install git-lfs`) and run `git lfs install` one time. Without Git LFS, the binary files are small text pointers, and the build fails. To get the real files in an existing clone, run `git lfs pull`.

## Table of Contents

* [Prebuilt binary](#prebuilt-binary)
* [How to build for macOS on Apple silicon](#how-to-build-for-macos-on-apple-silicon)

## Prebuilt binary

Each release has one DMG image, for macOS 27 or newer. It runs on Apple silicon only. There are no Intel binaries and no builds for other systems.

## How to build for macOS on Apple silicon

The build runs natively on a Mac. It makes an arm64 application that does not need Rosetta 2. Intel Macs are not supported.

These changes live in the `johnrichter/in-formant` fork. The `in-formant/in-formant` repository is the `upstream` remote.

### Requirements

* An Apple silicon Mac with macOS 27 or newer.
* Xcode, or the Xcode command line tools, with the macOS 27 SDK.
* Python 3.
* Git LFS, which fetches the fonts and the icons.
* `pkg-config`. For example, install it with `brew install pkgconf`.
* An internet connection and 10 GB of free disk space for the first build.

### Build steps

1. Run the build script from the project root. The first build downloads Qt and builds five libraries, so it takes longer than the next builds.

```sh
./build-macos.sh
```

2. Check the result. The disk image is `dist/InFormant-<version>-macOS-arm64.dmg`. A good disk image is about 66 MB.

To check the content, mount the disk image.

```sh
hdiutil attach -readonly dist/InFormant-*-macOS-arm64.dmg
```

The script writes its downloads and build files in `build/native`. It reads these settings from the environment:

* `BUILD_TYPE`: `Release` (default), `RelWithDebInfo`, or `Debug`.
* `QT_VERSION`: The Qt version that `aqtinstall` installs. The default is 6.12.0.
* `WORK_DIR`: The folder for downloads and build files. The default is `build/native`.
* `JOBS`: The number of parallel jobs. The default is the number of CPU cores.

The `macdeployqt` tool prints `ERROR: no file at ".../libiodbc.2.dylib"`, `ERROR: no file at ".../libpq.5.dylib"`, and `ERROR: no file at ".../libmimerapi.dylib"`. These messages come from the optional Qt SQL plugins. The application does not use them. Ignore these messages.

### Run the unit tests

The unit tests use GoogleTest and GoogleMock. The file `CONTRIBUTING.md` has the full guide. Run `build-macos.sh` one time first. Then run this command:

```sh
./run-tests.sh
```

The script builds the tests in `build/native/build-tests` and runs them with CTest. The application build does not contain any test code. These options help:

* `--sanitize`: Build the tests with the address and undefined behavior sanitizers.
* `-R TP_LAY`: Run only the tests whose name has `TP_LAY`. The name of each test starts with the ID of its case in `docs/test-plan.md`.
* `-L script`: Run only the script tests.
* `-L app`: Run only the app tests. They start the built application, so run `build-macos.sh` first.

The script `tests/check_plan_coverage.py` checks that each case of type Unit in the test plan has a test. A case with no test needs a line in `tests/deferred.txt` with a reason.

### Changes in this fork

Build system:

* **Native build:** The script `build-macos.sh` installs Qt with `aqtinstall` in a Python virtual environment. It builds FFTW 3.3.10, PortAudio 19.7, FreeType 2.15.0, and libsamplerate 0.2.2, and it installs Eigen 5.0.1 and toml++ 3.4.0, in a local folder. Then it builds the application with CMake and Ninja.
* **Pinned downloads:** The script checks a SHA-256 checksum for each of the six libraries, and it downloads over https only. It pins the versions of `aqtinstall`, CMake, and Ninja. It deletes the build folder when the Qt version changes. It needs the macOS 27 SDK.
* **Minimum version:** The deployment target is macOS 27.0. The CMake file sets it, and `LSMinimumSystemVersion` in `dist-res/Info.plist` states it.
* **AGL framework:** The macOS 27 SDK has no AGL framework, but Qt 6.12.0 links it. The CMake file replaces it with the OpenGL framework.
* **Libraries:** The source tree has no copy of FreeType, libsamplerate, or toml++. The script gives the `macdeployqt` tool the library folder of its local prefix, so the bundle has the libraries. To change a version, change the version, the address, and the checksum together. The search for packages ignores `/opt/homebrew`, so the build never mixes in other versions.
* **Signature:** The `macdeployqt` tool changes library paths, and this invalidates the signatures. Apple silicon refuses to run an arm64 file with an invalid signature. The script signs the whole bundle again and verifies it. The script always signs, with the hardened runtime and the entitlements in `dist-res/InFormant.entitlements`. It uses the first valid Developer ID identity of the keychain, and then the first valid Apple Development identity. With neither, the script stops before the build. Set `SIGN_IDENTITY` to choose another identity. Set `NOTARY_PROFILE` to a profile of `notarytool` to notarize the application and the disk image.
* **Disk image:** The script makes a compressed disk image with `hdiutil`.
* **One platform:** The Docker images, the x86_64 build, and the builds for Linux, Windows, and Android are gone. The code for these systems and the optional libraries (Armadillo, Oboe, ALSA, PulseAudio, Torch) are gone too. PortAudio with CoreAudio is the only audio module.

Application:

* **Black spectrogram:** The patch of upstream pull request 40 moves the shaders from GLSL 120 to GLSL 150. It requests an OpenGL 3.2 Core profile with 4x multisampling. It anchors the canvas to the top of the window.
* **Toolbar icons:** The CMake file links the Qt SVG module. The `macdeployqt` tool then deploys `libqsvg.dylib`. Without this plugin, Qt prints `Unsupported image format` and the icons stay invisible.
* **Retina resolution:** `NSHighResolutionCapable` is `true` in `dist-res/Info.plist`. The application renders at the full resolution of the display.
* **Frame rate:** The renderer built all fonts again at each frame, and it updated a vertex buffer for each glyph, line, and point. On Apple, each buffer update sends the command buffer to the GPU. The frame rate was about 7 frames per second. The renderer now builds the fonts one time, and the shaders take the corners of each quad from a uniform. The frame rate follows the display refresh rate.
* **Exit crash:** `BaseProcessor` and `FormantSolver` had no virtual destructor, and the pipeline deleted derived objects through them. Clang compiled this undefined behavior to a trap. `main` also destroys the context manager before the static destructors run. Also, the configuration is saved at exit, so the settings were never saved before.
* **Logger process:** The logger child process did not stop at the end of the pipe, so each launch from a terminal left one process alive. It now stops.
* **Logger exit:** The logger child process now ends without the static destructors. Before, Qt threw an error at exit, and macOS reported a crash.
* **Window size:** The main window opens at 1600 by 900 pixels.
* **Input device:** The Input tab of the settings has an "Input device" list. The setting `captureDevice` in the `[audio]` table of the configuration file stores the name. An empty name selects the default input device of macOS. Before this change, the application always used the default device. This device can be a webcam microphone with a low sample rate.
* **First launch:** The spectrogram, the pitch track, and the formant tracks are on by default. Before this change, a new installation showed a black window.
* **Spectrogram jump:** The picture of the spectrogram jumped back and forth. The right edge of the texture used the column before the new slices. It now uses the column after them.
* **CPU use:** The application used 65 to 88 percent of one core. It now uses 16 to 23 percent. The window of the charts that are not visible no longer updates its series. The renderer draws all glyphs of a string, and all points of a track, in one draw call. The spectrogram code no longer copies the visible slices for each frame. The audio queue no longer fills the log, and the synthesizer sends no sound while it is off.
* **Freeze:** A quit signal could freeze the application. The signal handler called Qt in the middle of a chart update, and the update held a lock. The handler now writes one byte to a pipe, and the event loop quits.
* **Spectrogram quality:** The Display tab of the settings has the list "Quality" (Fast or Smooth) and the slider "Blur" (0 to 100 percent). The keys `spectrogramQuality` and `spectrogramBlur` in the `[view]` table store them. The Smooth filter is a cubic filter on the square root of the power. A blur of 0 gives the sharp Catmull-Rom shape, and a blur of 100 gives the soft B-spline shape. The renderer also averages the bins and the columns that fall in one pixel. It adds noise before the color lookup to hide steps between colors.
* **Time step:** The spectrogram has a slice each 10 ms. Before, it had a slice each 20 ms. The key `granularity` in the `[analysis]` table sets the step. A configuration file that stores 20 keeps the old step.
* **Text:** The renderer starts each string on a whole pixel, so the texture filter does not blur the glyphs.
* **Time alignment:** The pitch track, the formant tracks, and the spectrogram use the time of the end of each frame. Before this change, the tracks were up to 60 ms early.
* **Settings window:** The Settings button opens one window for all settings, and the key combination Command and comma opens it too. The tabs are General, Display, Input, Axes, Tracks, Readouts, and Theme. The app has no side drawer. The `[options]` table of the configuration file stores each setting. The `[meta]` table stores a version number, and a new version applies new defaults one time.
* **Axes:** Both axes have a title, and a switch hides the titles or the numbers. The frequency axis has the modes automatic, custom step, notes, and octaves. The time axis has the modes automatic and custom step. The automatic modes label the roundest values first, and the labels never overlap. Both axes lie outside the graph. New sound appears at the left edge and the axis counts back to the right (a switch restores the old direction).
* **Tracks:** Each track draws as dots, as a line, or as both, and the Tracks tab sets this for each track. A slider sets the smoothness of the lines, and a line breaks at a gap or at a large jump.
* **Readouts:** One statistics box for each track shows by default. The Readouts tab sets the order of the boxes, a switch for each box, and the place of the boxes (at the left of the graph by default, or below it). At the left, the column has the order of the tracks in the graph: F4 at the top and the pitch at the bottom. A box has the live value in its header and one line for each chosen statistic. Each box has a fixed size, so it does not move when the numbers change. A bar above the graph holds the legend and the frame times side by side. The frame times are the render time, the update time, and the frames each second. The bar keeps its height during a recording, and the app hides the frame times in the recording. A switch in the Readouts tab shows the Range in semitones (for example `7.0st`) and not in Hz.
* **Color themes:** The application has 6 scientific themes, 16 dark pride themes, and a custom theme. A theme sets the colors of the spectrogram, the tracks, the oscilloscope, and the synthesizer.
* **Color maps:** The colors of a map change in the OKLab color space, so a gradient has no gray patch. Each pride theme goes from near black to a light tint, and no theme ends in white. The tracks of a pride theme take hues that differ from the loud part of the map, and each track has a dark outline. The Theme tab has a saturation slider from 50 to 150 percent. The spectrogram keeps 16 dB of headroom, so the loud harmonics of speech do not fill the map with its last color. The Gain normalization slider still sets the brightness.
* **Pitch zones:** The Display tab has a switch for three bands in the graph: masculine (blue), androgynous (yellow-green), and feminine (pink). The default edges are 60, 150, 180, and 300 Hz, and a text field sets them. The bands follow the frequency scale. They show ranges of pitch that many listeners hear in a certain way. The ranges are a rough stereotype, they differ between languages and voices, and a zone is not a goal. The recorder grabs the whole window, so a video shows the zones unless the setting "Keep the zones in recordings" is off.
* **Grid lines:** Two switches in the Axes tab draw thin horizontal lines across the graph at the ticks of the frequency axis. The major lines are at the labeled ticks, and the minor lines are at the ticks between them. Both are off by default.
* **Time direction:** The newest sound is now at the left edge from the first moment of a capture, and older sound flows to the right. The part before the start is empty. The time axis counts back from the left. The switch "Newest sound at the left" in the Axes tab restores the old direction. The mouse wheel and the trackpad follow the new direction. The map below the graph stays unflipped.
* **Formant switches:** The Display tab has one switch for each formant track (F1 to F4). Each switch has a short hint. The hint names the part of the voice that the track shows. The old single switch is gone. A file that had it off turns all four off one time (config version 8).
* **Axes:** The time axis labels whole seconds, and the smallest step is 1 second. The label step follows the length of the view. A view of 15 s has 1 s, 1 minute has 5 s, and 2 minutes have 10 s. A longer view has larger steps. The automatic frequency axis labels only 1, 2, 3, and 5 times a power of ten. A value has a label in all decades or in none, so the pattern is the same along the axis. A setting chooses Few, Normal, or Many labels. A linear axis uses equal steps, and all axis labels have one text size.
* **Grid lines:** A slider sets the strength of the grid lines. Their color is a tint of the quiet color of the theme, so they show on every theme. The default strength is 60 percent.
* **Quiet color:** The switch "Minimum volume is black" in the Theme tab is off by default. The quiet end of each theme then has a dark tint of the theme hue, as in Viridis. The graph behind the spectrogram has the same color. The upper colors do not change.
* **Reset graph:** The top bar has a red trash icon, next to the play button. It clears the tracks and the recording of the capture and starts again at time zero. A long capture asks first.
* **Settings window:** Each pane has more space at the top and at the bottom. The values are saved when the window closes. The synthesizer turns off when its window closes. A "?" button next to many settings opens a box with a longer explanation.
* **Help pages:** A new page "Settings" explains the frequency scale, the FFT size, and four methods (pitch, formants, LPC, and glottal flow). It gives advice for each goal. The pages use a new typography. The column has 640 pixels. The body has 16 pixels on a line of 28. A heading has more space above it than below it.
* **Reset:** The General tab of Settings has the button "Reset all settings to defaults". A dialog asks for a confirmation. The reset returns the options, the view, the layers, the four algorithms, and the input device to their defaults. The folder of the practice sessions stays, and so do your sessions and files. A test of the reset found a crash in the setter of an old key. All setters in `config.cpp` now work when the file has no key for them.
* **Tab buttons:** The choice "Full" and "Pitch only" is now a group of two large buttons, and the chosen button has a solid purple fill. The tabs in Settings and Help use the same fill for the chosen tab.
* **Pitch-only graph:** Two buttons in the toolbar, "Full" and "Pitch only", set what the graph shows. In the pitch-only mode, the graph hides the spectrogram and the formant tracks, and it draws the pitch track larger. The vertical axis is linear and goes from 0 to 500 Hz by default, so the pitch zones show in full. The Display tab sets the two edges. A switch fits the axis to the pitch in view instead. The fit uses a logarithmic scale and a minimum of 12 semitones, and the 5th and the 95th percentile set its edges. The pitch zones also show in this mode.
* **Practice mode:** The Practice button in the bar below the graph starts a session on the live capture, and the capture starts if it is paused. A list next to the button sets the task: Free, Vowels, Reading, or Conversation. The M key, or the Mark button, saves a mark with the median pitch and F1 to F4 of the last half second. A note field next to the button adds a note to a mark. A click on Stop writes one folder for the session. A window then shows a summary. The summary compares the session with the last session of the same task. A session shorter than 2 seconds is not saved. When the program closes during a session, it saves the session first.
* **Practice question:** Stop practice ends the session and opens a window with the question "Keep this practice session?". The window shows the task, the length, the marks, and the comparison with the last session of the task. Keep writes the folder. Discard writes nothing. The window stays until you choose. A quit while the window is open keeps the session. A session shorter than the minimum time (5 seconds by default, a slider in the General tab) is dropped with no question.
* **Design system:** All windows share one type scale (12, 14, 17, 20, and 24 pixels), one spacing scale (multiples of 4 pixels), and one set of text colors. Roboto is the font of the text. Roboto Mono is the font of numbers and of labels that line up, in the windows and in the graph. Gray hints under the controls say what each control does. The page [design system](docs/design-system.md) has the rules.
* **Practice folder:** The folder of the sessions is Documents/InFormant/Practice, and the General tab can change it. Each session has a folder with the date in its path (year, month, day, and the id of the session). The file `session.json` has the statistics, the marks, and a timeline of medians. The folder also has `report.md`, `stats.csv`, `marks.csv`, and `audio.wav`. A switch in the General tab turns off the sound file. The statistics always stay. Export all copies all sessions and a `history.csv` to a folder that you choose. The practice mode has no video yet, and the Record button stays off during a session.
* **Progress window:** The Progress button opens a window with the trend of the practice sessions. Four boxes set the measure (the pitch or F1 to F4), the statistic, the task, and the period (day, week, month, or year). The statistics are the median, the mean, the standard deviation, and the variation. Others are the minimum, the maximum, and the 5th, 25th, 75th and 95th percentiles. The range shows in Hz and in semitones. The last two are the voiced share and the number of values. The chart shows the median of the sessions in each period as a line. A bar shows the lowest and the highest session of the period. A table below the chart has the number of sessions, the minutes of practice, and the three values. The task filter is on by default, because a formant median over mixed tasks says little. Below the table, a list shows each saved session, newest first. The Delete button of a session asks a question first. After you confirm, the program deletes the folder of the session with its statistics, marks, report, and sound. Files that you exported before stay.
* **Defaults:** The time step is 10 ms and the view is 15 seconds. A configuration file with an older version number gets these values one time.
* **Oscilloscope:** The window opens at 1100 by 620 pixels. It shows the newest 30 ms of the sound at the rate of the display. A slider sets the time from 5 to 100 ms. The view starts at a rising zero crossing, like the trigger of an oscilloscope. The estimate of the glottal flow has a new value each 20 ms (key `oscilloscopeSpacing`). Before, it had a new value each 160 ms, and the view changed about 6 times each second.
* **Plots:** The plots of the oscilloscope and the synthesizer use one new item. It draws the grid one time and the lines at each frame. A plot makes data only while it is on the screen. Qt Charts is no longer a dependency.
* **Synthesizer:** The window takes the size of its contents when it opens for the first time. The synthesizer starts in constant mode. The settings "Follow pitch estimates" and "Follow formant estimates" are off by default. Sliders set the frequency and the bandwidth of F1 to F4, and the amount of pitch jitter (0 by default). The loudness is fixed and does not follow the sound. A warning shows when a follow mode is on, because the microphone then hears the synthesizer. The box "Quick start" sets F1 to F4 and the pitch for ten vowels, for a lower voice or a higher voice. Number boxes take typed values for the pitch and each formant. A button opens the help page "Synthesizer", with recipes for six tasks.
* **Synthesizer follow mode:** The follow mode smooths the pitch on a logarithmic scale, and it switches the voice off after 80 ms with no pitch. Each formant keeps its slot for 300 ms after the last estimate. Then the slot returns to the value of its slider. The pitch goes to the synthesizer from one thread, because a signal between threads could replace a newer value. The synthesizer makes audio in blocks of 1024 samples. Each noise generator has its own state, so two threads do not share it.
* **Long views:** A field at the right end of the bar below the graph sets the view time. It accepts any time from 0.5 s to 24 hours. The application keeps the sound of the capture for 24 hours on the disk (a temporary file, 64 KB for each second), and it keeps the pitch and formant tracks in memory. A view longer than about 35 s, and each view of a past time, takes its spectrogram from the stored sound in a second thread. The thread keeps the columns that did not change, so the cost of a long live view is close to the cost of a short one. The graph reduces the tracks to at most 6000 points.
* **Picked moment:** A click or a drag on the graph picks a moment. A bar shows the moment from the top to the bottom of the graph. A label next to the bar gives the time. A label with a dot on the bar gives the exact value of the pitch and of each formant, in the color of its track. The labels go to the side of the bar that has the room. A double click or a right click removes the bar.
* **Help and guides:** The Help button opens a window with eight pages. The pages cover the display, your settings, voice feminization, voice masculinization, singing, warm-ups and vocal health, the synthesizer, and the math. The pages are in plain language, and the last page has the formulas. The same pages are Markdown files in the `docs` folder. Links: [the display](docs/reading-the-display.md), [your settings](docs/choosing-settings.md), [feminization](docs/feminization.md), [masculinization](docs/masculinization.md), [singing](docs/singing.md), [warm-ups and vocal health](docs/warmups-and-vocal-health.md), [the synthesizer](docs/synthesizer.md), and [the math](docs/the-math.md). The warm-up page ranks six exercises for four goals, and it says that the ranks are a judgment from small studies. The guides on feminization and masculinization explain the pitch range and the pitch variation in semitones, with a worked example.
* **Session map:** A map below the graph shows the loudness and the pitch of the session, and a frame around the part that the graph shows. The map shows 2.5 minutes at most (the Readouts tab sets this time), and a shorter session shows whole. A click or a drag in the map moves the graph to another time. A drag of an edge of the frame changes the time of the graph. A strip under the map, and a sideways scroll over the map, move the map in a longer session. The mouse wheel moves the graph in time. The Live button returns to the newest sound. A switch in the Readouts tab of the settings hides the map.
* **Map bins:** The bins of the map start at fixed times, so old columns keep their look while new sound arrives. The loudness scale and the pitch scale change slowly.
* **Playback and export:** Play window plays the sound of the graph, and the graph follows the sound. The capture pauses during the playback, and it continues at the end. Export window and Export all write the sound as a mono 16-bit WAV file at 32 kHz.
* **Audio files:** Open file loads a file of any format that macOS reads (WAV, MP3, AAC, FLAC, AIFF, and others). Other systems read WAV files. The application analyzes the whole file, about 10 times faster than the length of the sound, and the map shows the result. The Live button deletes the file and starts a new capture.
* **Recording:** Record writes three files with one name: a video (`.mp4`, H.264 and AAC), the sound (`.wav`), and a report (`.md`). The video shows the graph at the left. Open windows (the oscilloscope and the synthesizer) go in a column at the right, scaled so that the picture has no empty space. The Record button opens a sheet before the file dialog. During a recording the graph shows the time that the sheet sets (2 to 60 s, 10 s by default). The sheet also sets the frame rate (10 to 30) and the choice of a video. The app stores the choices of the sheet. The report has the date, the length, and for the pitch and each formant the mean, the median, the standard deviation, the minimum, the maximum, the 5th, 25th, 75th and 95th percentiles, and the share of frames with a value. It also has a table with the median of each part of the recording.

### Troubleshooting

* **The script prints `this script needs macOS 27 or newer`.** Update macOS. The script checks the version, because the build uses the macOS 27 SDK.
* **The script prints `the Xcode command line tools are missing`.** Run `xcode-select --install`.
* **The script prints `pkg-config is missing`.** Install it, for example with `brew install pkgconf`.
* **The link step fails with `framework 'AGL' not found`.** Keep the `WrapOpenGL_AGL` setting in the CMake file. Delete the folder `build/native/build`, and run the script again.
* **The application closes at once, and the shell shows `Killed: 9`.** The signature of the bundle is invalid. Keep the `codesign` step in `build-macos.sh`.
* **The toolbar has no icons.** Check that the bundle has `Contents/PlugIns/imageformats/libqsvg.dylib`.
* **The spectrogram area is black.** The area stays black when the microphone sends no sound. Make sure that macOS gives microphone access to InFormant. To ask again, run `tccutil reset Microphone fr.cloyunhee.InFormant`, and then start the application. Choose the input device in the drawer.

### Known limits

* The checks launched the application, saved a picture of the window, and confirmed the toolbar, the axes, and the text. Speech from a loudspeaker, picked up by the microphone, drew the spectrogram, the pitch track, and the formant tracks.
* The checks used the built-in input path of one Mac with a USB webcam microphone. Other input devices are not tested.
* The native build is tested on one Mac with macOS 27.0.1 and Qt 6.12.0. The application starts, shows the window, and quits with no crash report.
* The disk image of this build has an Apple Development signature and no notarization. This signature works on this Mac only. The script can sign and notarize with a Developer ID, but nobody tested this path, because this Mac has no such identity.
* The log file is `~/Library/Logs/InFormant/InFormant.log`. A file over 5 MB moves to `InFormant.1.log` at the next start.
* The application asks for the microphone when you press play for the first time. When you deny it, the application stays paused and shows a message.
* The application reads its configuration from `~/Library/Application/informant.conf`.
* The checks of the playback confirmed the data flow, the position, and the view that follows the sound. Nobody listened to the sound.
* The checks covered views up to 5 minutes, a 20 second speech file in two formats (AIFF and AAC), and recordings of 13 seconds. A view of many hours uses one short analysis for each column. This path has no test with hours of sound.
