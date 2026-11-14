# Test plan: use cases for later tests

This page lists every use case of the project. A test names the ID of its case. The command `tests/check_plan_coverage.py --report` lists the cases that have no test yet. The list is long on purpose.

## How to read this page

- Each case has an ID, a short setup, and the result that a test must check.
- The type tells how to automate the case. **Unit** is a C++ test in `tests/` with GoogleTest. The name of the test starts with the ID of the case, for example `TEST(Layout, TP_LAY_01_BoxesCentered)`. **Perf** is a timed measure on a real session. **Qt** is a Qt Quick Test or a Qt Test with a window. **Image** compares a screenshot with a stored picture. **Audio** analyses rendered samples. **App** starts the built application bundle, and a script in `tests/app_smoke.py` checks the result. **Manual** needs a person or a device.
- The priority is **P1** (a failure harms a user), **P2** (a failure is visible), or **P3** (a small risk).
- A case that says "not built yet" tests a feature that does not exist. Write the test when the feature exists.

## How to run the unit tests

The file `CONTRIBUTING.md` has the full guide.

- Run `./run-tests.sh` to build and run all unit tests. Add `--sanitize` to build with the address and undefined behavior sanitizers.
- Run `./run-tests.sh -R TP_LAY` to run the tests of one area.
- The file `tests/deferred.txt` lists each Unit case that has no test yet, with the package that owns it. The script test `plan_coverage` fails when a case has neither a test nor a line in that file.

## What the tests must never do

- Never leave an `InFormant` process running after a test.
- Never touch the installed application in `/Applications`.
- Remove `informant.conf` after each test that starts the app.
- Restore the system input device after each test that changes it.
- Detach each disk image that a test mounts.

## Index of the limits that were reported

Each "Limits" block in a report named things that the project did not test. This index links each limit to the cases below. A limit with no case is a gap in this page.

- **The synthesizer sound was never heard.** TP-SYN-01 to TP-SYN-30, and the section "Things that no test can cover".
- **The frame-times box was not checked in a real recording.** TP-LAY-17.
- **The stability of the live map got an eye check only.** TP-MAP-01 to TP-MAP-04.
- **The left edge of the map was not clicked by hand.** TP-MAP-06, TP-MAP-07.
- **The sideways scroll of the map was not tried by hand.** TP-MAP-10.
- **The resize cursor was not seen by hand.** TP-MAP-09.
- **The key combination for Settings was not tried by hand.** TP-SET-01.
- **The toolbar buttons were not all clicked by hand.** TP-MODE-06.
- **The Saturation slider was not moved by hand.** TP-COL-07.
- **The pitch-axis fields were not used by hand.** TP-PITCH-04.
- **The practice settings fields were not used by hand.** TP-SET-09.
- **The switch between Full and Pitch only during live capture.** TP-MODE-01 to TP-MODE-03.
- **The pitch-only axis has Hz only.** TP-PITCH-05 (not built yet).
- **The Mel scale was never captured with zones.** TP-ZONE-01.
- **A recording with zones got a window grab only.** TP-ZONE-03, TP-ZONE-04.
- **No practice video and no zip export.** TP-PRA-12, TP-PRA-13 (not built yet).
- **The practice comparison got unit tests and a check of the dialog text only.** TP-PRA-14.
- **The Progress window was checked on the screen for the default measure only.** TP-PRG-08.
- **Only six pride themes got an eye check.** TP-COL-05.
- **The 16 dB headroom darkens the default Magma theme.** TP-COL-08.
- **The warm-up ranks are a judgment from search summaries.** TP-DOC-07.
- **The formant tracker is not reliable above about 400 Hz.** TP-ANA-01.
- **The Qt 6.12.0 build was verified on one Mac.** TP-REL-02, TP-REL-03.
- **Old clones must clone again.** TP-REL-06.
- **The Escape key and a quit with the question open were not tried by hand.** TP-PRA-33, TP-PRA-34.
- **The text in the graph is not on the type scale.** TP-UI-16.
- **No person measured the reading speed.** TP-UI-17.
- **The shortest practice time was read as "too short".** TP-PRA-39.
- **The flipped time axis was checked with a saved file only.** TP-FLIP-03 to TP-FLIP-09.
- **The wheel direction, the click pick, and the formant switches were not clicked by hand.** TP-FLIP-04, TP-FLIP-05, TP-TRACK-01.
- **The session map stays in the old direction.** TP-FLIP-08.
- **The Record sheet and the keep dialog were not checked for the new corner radius.** TP-SHAPE-03.
- **The grid lines have a fixed brightness.** TP-GRID-04.
- **The reset button got one script test, with one set of odd values.** TP-RST-01 to TP-RST-12.
- **Only the Pitch "?" box got a screenshot check.** TP-HLP-05 to TP-HLP-07.
- **The start time was checked with one default device.** TP-STP-02 to TP-STP-04.
- **"Reset graph" got one live test and no stress test.** TP-RGR-07 to TP-RGR-09.
- **The synthesizer stop on close got no test with each way to close.** TP-WIN-03, TP-WIN-04.
- **The scroll rate got a check with synthetic events only, and no check with a real trackpad or a real mouse.** TP-SCR-08, TP-SCR-09.
- **The lifted quiet color got an eye check on a few themes.** TP-QUI-06, TP-QUI-07.
- **The edits of the peer review got no expert check.** TP-DOC-11 to TP-DOC-13.
- **The map got a timing test in one live run, one scroll, and no drag of a long session.** TP-MAP-16 to TP-MAP-22.
- **The main graph stalls for 50 to 66 ms during a drag in file mode, and the threaded render loop stops the app.** TP-MAP-23, TP-MAP-24.
- **The spaces around the graph got a check by screenshot at one window size and one pixel ratio.** TP-LAY-21 to TP-LAY-32.
- **The scratch folders `mig` and `fresh` in the working area of the session cannot be removed by the shell gate.** This is not a product test. Remove them by hand.

## 1. Playback and synthesizer sound

The project never played the sound through a speaker. These cases use rendered samples as a proxy.

- **TP-SYN-01** (Audio, P1). Render 5 seconds with fixed settings and jitter 0. Expect the level of each 1024-sample block to change by less than 1 percent.
- **TP-SYN-02** (Audio, P1). Render a constant pitch of 120, 170, and 210 Hz. Expect a pitch error below 1 cent.
- **TP-SYN-03** (Audio, P2). Render with jitter 100 percent. Expect a pitch spread that matches the jitter size, and no block with a level jump above 3 dB.
- **TP-SYN-04** (Audio, P1). Render each of the ten vowel presets for both voices. Expect spectral peaks within 5 percent of the formants that the preset sets.
- **TP-SYN-05** (Audio, P2). Render with Voicing off and noise 100 percent. Expect no harmonic peaks, and a flat spectrum.
- **TP-SYN-06** (Audio, P2). Render with noise 0 and Voicing on. Expect no energy between the harmonics above the noise floor of the filter.
- **TP-SYN-07** (Audio, P2). Sweep Rd from 0.7 to 2.6. Expect the level of the first harmonic to rise against the second harmonic.
- **TP-SYN-08** (Audio, P3). Sweep Tc from 0.9 to 1.0. Expect no clicks and no change of the pitch.
- **TP-SYN-09** (Audio, P2). Set the filter shift to 50, 100, and 200 percent. Expect the formant peaks at the shifted frequencies, and the same Q factor.
- **TP-SYN-10** (Audio, P1). Run two noise generators on two threads for 60 seconds. Expect no data race (run under a thread sanitizer).
- **TP-SYN-11** (Audio, P2). Change a formant while the sound plays. Expect no click and no jump larger than the smoothing allows.
- **TP-SYN-12** (Audio, P2). Start the sound with formants far from the first values. Expect the formants to start at the target and not to glide from zero.
- **TP-SYN-13** (Audio, P2). Set the pitch to 60 and to 1500 Hz. Expect a stable sound at both limits.
- **TP-SYN-14** (Audio, P2). Set a bandwidth of 20 and of 300 Hz. Expect a stable filter and no overflow.
- **TP-SYN-15** (Audio, P1). Set four formants close together (for example 500, 520, 540, 560 Hz). Expect no instability and no output above the clip level.
- **TP-SYN-16** (Audio, P2). Set a formant above half of the sample rate. Expect the program to drop the formant and to keep the sound stable.
- **TP-SYN-17** (Manual, P2). A person listens to each description in the help page "Synthesizer". Expect the description to match the sound. Change the text when it does not.
- **TP-SYN-18** (Manual, P1). Start a follow mode with speakers. Expect the feedback warning to show. Expect the sound to stop when the person clears the mode.
- **TP-SYN-19** (Audio, P2). Follow mode: send a pitch estimate, then nothing for 100 ms. Expect the voice to switch off after 80 ms.
- **TP-SYN-20** (Audio, P2). Follow mode: send a formant estimate, then nothing for 400 ms. Expect the slot to return to the slider value after 300 ms.
- **TP-SYN-21** (Unit, P2). Switch the synthesizer off. Expect the program to send no samples to the playback stream.
- **TP-SYN-22** (Unit, P2). Check the preset table. Expect each value to equal the table of Peterson and Barney (1952). Expect F4 to be 3500 Hz for the lower voice and 4000 Hz for the higher voice.
- **TP-SYN-23** (Qt, P2). Press Apply with the check box "Also set the pitch" clear. Expect the pitch to stay and the formants to change.
- **TP-SYN-24** (Qt, P2). Press Apply with a follow mode on. Expect the follow modes to switch off.
- **TP-SYN-25** (Qt, P2). Press "Reset all". Expect each control to return to its start value. The values are noise 30 percent, gain 100 percent, pitch 170 Hz, Rd 1.7, Tc 1.0, shift 100 percent, and jitter 0. The formants are 500, 1500, 2500, and 3500 Hz.
- **TP-SYN-26** (Qt, P2). Type a value in a formant number box and press Enter. Expect the slider and the plot to follow. Test a value below 150 and above 5500 Hz. Expect the box to clamp it.
- **TP-SYN-27** (Qt, P3). Type text that is not a number in a number box. Expect the box to keep the old value.
- **TP-SYN-28** (Qt, P3). Press "How to use". Expect the Help window to open on the tab "Synthesizer", also when the window is open on another tab.
- **TP-SYN-29** (Qt, P3). Resize the window to the minimum. Expect no overlap of the hints and the controls.
- **TP-SYN-30** (Audio, P2). Start and stop the synthesizer 100 times. Expect no click at the start, no leak, and no stuck thread.
- **TP-SYN-31** (Unit, P2). The synthesis loop and the loop of the plot data wait 200 ms when they have no work, and 20 ms when they have work. The work is the synthesizer on, a follow mode on, or a plot on the screen.
- **TP-SYN-32** (Unit, P2). An idle playback queue keeps its block length when it runs empty. A queue that is not idle makes its blocks longer after an underrun.
- **TP-SYN-33** (Audio, P2). Leave the synthesizer off for a minute. Switch it on, and then press play on a stored sound. Expect the sound within 200 ms each time.
- **TP-IDLE-01** (Manual, P2). Pause the capture, close the synthesizer window, and measure the wake-ups of the process for 10 s with `sample` or Activity Monitor. Expect fewer than 40 wake-ups in a second.
- **TP-IDLE-02** (Unit, P1). The canvas draws the next frame at once only while the capture or a playback runs. It does the same for 1 s after an input of the user. Else it draws one frame for each 200 ms. An input in any window counts.
- **TP-IDLE-03** (App, P1). Start the app and leave it paused. Measure the CPU use of the process for 20 s. Expect less than 6 percent of one core.
- **TP-IDLE-05** (Manual, P1). Pause the capture, and move the mouse over the graph. Expect the cursor to follow with no lag, and the spectrogram to show the change of a setting at once.
- **TP-IDLE-04** (Image, P2). Pause the capture. Change the theme, the scale, and the view range. Expect the graph to show each change at once. Press play, and then pause. Expect the last frame to stay on the screen.

## 2. Playback of a session

- **TP-PLY-01** (Audio, P1). Play a loaded file. Expect the sound to match the file and to stay in time with the cursor.
- **TP-PLY-02** (Audio, P2). Play a selected window. Expect the sound to start and stop at the window edges.
- **TP-PLY-03** (Qt, P2). Press Play while capture runs. Expect capture to pause or to continue as the design says, and expect no echo.
- **TP-PLY-04** (Audio, P3). Play a file with a sample rate that differs from the output device. Expect the right speed.
- **TP-PLY-05** (Manual, P2). Change the output device while the sound plays. Expect no crash.

## 3. Layout of the bar and the boxes

- **TP-LAY-01** (Unit, P1). Compute the layout with the legend and the frame times on. Expect the two boxes side by side and centered over the graph.
- **TP-LAY-02** (Unit, P1). Compute the layout with the frame times on and `hideDebug` on (a recording). Expect the same top margin and the same graph size.
- **TP-LAY-03** (Unit, P2). Compute the layout with both boxes off. Expect a top margin of the graph padding and nothing more.
- **TP-LAY-04** (Unit, P2). Make the bar wider than the graph. Expect the bar to start at the left edge of the graph and to stay inside the window.
- **TP-LAY-05** (Unit, P2). Set the side to "left". Expect the column order to be F4, F3, F2, F1, Pitch from the top to the bottom.
- **TP-LAY-06** (Unit, P2). Set the side to "left" with a custom order (for example F2, Pitch, F4). Expect the reverse of that order in the column.
- **TP-LAY-07** (Unit, P2). Make the column taller than the graph. Expect a second column, and expect the order to continue from the top of the second column.
- **TP-LAY-08** (Unit, P2). Hide one track box. Expect the other boxes to close the gap.
- **TP-LAY-09** (Unit, P2). Hide a track in the graph. Expect the box of that track to disappear.
- **TP-LAY-10** (Unit, P1). Sweep a live value from 9.9 to 100.0 and from 999.9 to 1000. Expect no box to change its size or its place.
- **TP-LAY-11** (Unit, P2). Test the width function with a proportional font. Expect the widest digit to set the width, for each digit from 0 to 9.
- **TP-LAY-12** (Unit, P2). Test the text pattern of each statistic (Now, Min, P5, Mean, Median, SD, P95, Max, Range, CV, Voiced, N, Note). Expect the pattern to be at least as wide as the widest real value.
- **TP-LAY-13** (Image, P2). Compare screenshots of both layouts at device pixel ratios 1 and 2.
- **TP-LAY-14** (Unit, P2). Test the key list. Expect the legend key in the stored order to be ignored in the strip and in the column.
- **TP-LAY-15** (Qt, P2). Move a box up and down in the Readouts tab. Expect the stored order to change, and expect the legend not to be in the list.
- **TP-LAY-16** (Unit, P3). Compute the layout in a window of 300 pixels width. Expect no negative sizes.
- **TP-LAY-17** (Qt, P2). The frame-times box hides during a real recording. Start a recording, grab a frame from the saved video, and check that the box is not in it.
- **TP-LAY-18** (Image, P1). With the bar above the graph, click on the graph. Expect the right time, and expect the labels of the cursor at the right height.
- **TP-LAY-19** (Unit, P1). Click above the graph (in the bar). Expect no pick.
- **TP-LAY-20** (Unit, P1). Click at the first and the last row of the graph. Expect a pick.
- **TP-LAY-21** (Image, P1). Grab the window with the boxes in a row and then in a column. Expect a space of 24 points at the top, the right, and the bottom edge of the graph. Expect the same space at the left edge of the boxes, and between a column of boxes and the title of the frequency axis.
- **TP-LAY-22** (Image, P2). Grab the window at device pixel ratios 1 and 2. Expect each space to scale with the ratio.
- **TP-LAY-23** (Unit, P1). Click on the space at the right of the graph. Expect no pick. Click on the last column of the graph. Expect a pick.
- **TP-LAY-24** (Image, P2). Set a time window where the last time label is near the right edge. Expect the label not to be cut off, and not to overlap the space of the window edge.
- **TP-LAY-25** (Image, P2). Show the cursor at the last column of the graph. Expect the labels of the cursor to stay inside the graph.
- **TP-LAY-26** (Image, P2). Start a recording and grab one frame of the video. Expect the same spaces around the graph in the video.
- **TP-LAY-27** (Image, P3). Resize the window to 480 points of width. Expect a graph with a positive width and the right space intact.
- **TP-LAY-28** (Image, P2). Show the legend and the frame times in a window that is 600 points high. Expect the bar to start 24 points below the toolbar, and a graph with a positive height.
- **TP-LAY-29** (Unit, P1). Compute the layout for each combination of axis labels and axis titles. Expect each margin of the axes to be a sum of the ticks and the smallest steps.
- **TP-LAY-30** (Unit, P2). Compute the layout at each pixel ratio from 1 to 3. Expect the row height of the boxes to be a multiple of 4 times the ratio.
- **TP-LAY-31** (Image, P2). Hide the axis titles and the axis labels. Expect the same padding at the left edge of the graph.
- **TP-LAY-32** (Image, P3). Compare the box heights with the heights before this change. Expect a one-row box of 32 points and a six-row box of 112 points.

## 4. Drawing with an origin below the bar

The renderer shifts all drawing by the height of the bar. The fragment shaders compare window coordinates with the coordinates of the drawing, so they need the same shift.

- **TP-GL-01** (Image, P1). Draw a segment, a circle, and a rounded rectangle with the origin at 0 and at 40 pixels. Expect the same picture, shifted by 40 pixels.
- **TP-GL-02** (Image, P1). Draw the spectrogram with a top margin. Expect the picture to fill the graph, with no gap at the top or at the bottom.
- **TP-GL-03** (Image, P1). Draw text with a top margin. Expect the text to be at the shifted place.
- **TP-GL-04** (Image, P1). Set a clip rectangle with a top margin. Expect the clip to cover the graph and nothing else.
- **TP-GL-05** (Image, P2). Draw the bands, the ticks, the grid, the tracks, and the cursor with a top margin. Expect all of them in the graph.
- **TP-GL-06** (Image, P2). Reset the origin after the boxes. Expect the next frame to start at the origin 0.
- **TP-GL-07** (Image, P2). Resize the window while a frame draws. Expect no stale picture.

## 5. The minimap and the live view

- **TP-MAP-01** (Image, P2). Capture two frames one second apart from a 10-minute session. Expect the old columns to stay the same (no shimmer).
- **TP-MAP-02** (Unit, P2). Check that the bin edges are multiples of the bin width in absolute time.
- **TP-MAP-03** (Unit, P2). Feed a loud burst, then quiet sound. Expect the normalization to fall slowly (hysteresis) and not at each rebuild.
- **TP-MAP-04** (Unit, P2). Test the gap rule with the real bin width at several zoom levels. Expect a silence to stay a gap.
- **TP-MAP-05** (Unit, P3). Test the pitch range of the map for data below 60 Hz and above 600 Hz. Expect a visible marker or the range of the data.
- **TP-MAP-06** (Qt, P1). Drag the left edge of the frame. Expect the right end to stay and the span to change.
- **TP-MAP-07** (Qt, P1). Drag the right edge of the frame. Expect the left end to stay and the span to change.
- **TP-MAP-08** (Unit, P2). Drag an edge to a span below 0.5 seconds and above 24 hours. Expect the span to clamp.
- **TP-MAP-09** (Qt, P2). Move the mouse over each edge zone. Expect the resize cursor, and expect the normal cursor elsewhere.
- **TP-MAP-10** (Qt, P2). Scroll sideways over the map with a trackpad and with a wheel. Expect the map to move.
- **TP-MAP-11** (Qt, P2). Drag the scrollbar strip under the map. Expect the map to move.
- **TP-MAP-12** (Qt, P2). Let the frame leave the map in live mode. Expect the map to scroll by itself.
- **TP-MAP-13** (Unit, P2). Make the view span longer than the map window. Expect the map to widen to the span, so the handles work.
- **TP-MAP-14** (Qt, P2). Type a value in the field "View" (below 1, above 3600, not a number). Expect a clamp or a rejection.
- **TP-MAP-15** (Image, P3). Check the vertical center of the text in the field "View".
- **TP-MAP-16** (Qt, P1). Run a live capture for 60 seconds with a map window of 8 seconds. Record the time of each map drawing. Expect at least 50 drawings in a second and no gap above 40 ms. The first run had 62 drawings in a second and a longest gap of 29 ms.
- **TP-MAP-17** (Image, P2). Record the map while a capture grows from 0 to 150 seconds. Compare two frames at each change of the bin size. Expect no jump in the shape of the sound.
- **TP-MAP-18** (Qt, P1). Drag the frame with a mouse that sends a move each 16 ms. Expect the frame in the map under the pointer in each drawing, with a lag of at most one drawing.
- **TP-MAP-19** (Qt, P2). Drag the scrollbar strip over a session of 10 minutes. Expect a smooth move and no empty part inside the map.
- **TP-MAP-20** (Qt, P2). Stop the capture in the middle of a run. Expect the map to stop within 0.25 seconds and not to run ahead of the sound.
- **TP-MAP-21** (Unit, P2). The bin size stays between one and two and a half pixels. A growth by a factor of 15 needs between three and five new sizes. The unit test `map_layout` has this.
- **TP-MAP-22** (Perf, P2). Measure the time of one drawing of the map on a session of 24 hours. Expect less than 4 ms.
- **TP-MAP-23** (Qt, P3). In file mode, drag the frame with a long file and count the drawings of the main graph. The main graph had stalls of 50 to 66 ms during a drag, and the cause is not known.
- **TP-MAP-24** (Manual, P3). Start the app with the render loop `threaded` (the variable `QSG_RENDER_LOOP`). The app stopped with a trap. Find the cause before a change to the render loop.
- **TP-MAP-25** (Qt, P1). During a live capture, record the left and right edge of the frame in each drawing. Expect an edge that never moves back and a left edge that stays still while the map scrolls. The run had no step back after the fix.
- **TP-MAP-26** (Unit, P2). The length of the map never goes back while the capture runs. It can go back after a reset of the graph or after a pause. Check the function with a fake clock.
- **TP-MAP-27** (Unit, P2). The map checks at the rate of the screen while the sound runs, during a drag, and for 0.5 s after a change. Then it checks 10 times in a second. A hidden map checks 4 times in a second.
- **TP-RGR-10** (Qt, P2). Find the button "Reset graph" in the top bar next to the play button. Expect a trash icon of the same visual size as the play icon and a tip on a hover. Expect a disabled state during a recording and a practice session.
- **TP-RGR-11** (Image, P3). Compare the trash icon with the play icon and the stop icon at a scale of 100 and 200 percent. Expect the same visual size and the same weight. Check that the red is muted on the bar.

## 6. The right edge and the live spectrogram

- **TP-EDGE-01** (Image, P1). Capture the right 100 pixels at FFT sizes 512, 1024, 2048, and 4096 in live mode. Expect no black bar.
- **TP-EDGE-02** (Image, P2). Repeat in review mode past the end of the session.
- **TP-EDGE-03** (Image, P2). Repeat with a session shorter than the view.
- **TP-EDGE-04** (Image, P2). Repeat while the canvas width changes.
- **TP-EDGE-05** (Unit, P1). Give a picture that is older than the view by 0.1 seconds. Expect the newest column to fill the span up to the view. Give a lag of more than 0.25 seconds, a newer picture, and a past view. Expect no hold.
- **TP-EDGE-06** (Image, P1). Capture a live view with a span of 60 seconds. Expect no empty strip between the axis and the newest sound, in 10 frames that follow each other.
- **TP-EDGE-07** (Unit, P1). Give the clock of the live view a steady stream of sound in steps of 0.02 seconds. Expect an end that is never behind the stored sound, never more than 0.1 seconds beyond it, and never goes back. Read the source of the view. Expect the clock to take the stored sound, and expect no use of the wall clock. Expect the time of the slices to count the samples of each block.
- **TP-EDGE-08** (Unit, P1). Stop the sound for 3 seconds and restart it after 0.17 seconds. Expect the end to stay within 0.1 seconds of the stored sound. Stall the sound for a long time. Expect the end to stop at the cap.
- **TP-EDGE-09** (Unit, P2). Clear the stored sound, as the reset of the graph does. Expect the clock to start again, with no old end.
- **TP-EDGE-10** (Image, P1). Record live with a span of 77 seconds. Open the oscilloscope and the synthesizer, switch the synthesizer on, toggle the voicing, then pause and restart three times. Expect no empty strip next to the axis after each restart.

## 6b. Time direction, grid lines, and track switches

- **TP-FLIP-01** (Image, P1). Load a file and capture the graph with the default settings. Expect time 0 at the right edge and the newest time at the left edge.
- **TP-FLIP-02** (Image, P1). Turn off "Newest sound at the left". Expect time 0 at the left edge.
- **TP-FLIP-03** (Qt, P1). In live mode with the flip on, speak. Expect the new sound at the left edge and the old sound to move to the right, with no black bar at the left edge.
- **TP-FLIP-04** (Qt, P1). Click in the graph in both directions. Expect the cursor line and the time of the cursor to match the click.
- **TP-FLIP-05** (Qt, P2). Scroll the wheel and the trackpad sideways in both directions. Expect the content to follow the fingers.
- **TP-FLIP-06** (Image, P2). Check the pitch-only graph and a recording in both directions.
- **TP-FLIP-07** (Unit, P1). Test that `plotXToTime(plotTimeToX(t))` returns `t` in both directions. Use the free functions of `src/gui/plotmap.h`. Use widths of 1, 2, and 4000 pixels.
- **TP-FLIP-08** (Image, P2). Compare the session map with the graph. The map keeps old sound at the left. Decide with the user whether the map must flip too, then test the choice.
- **TP-FLIP-09** (Image, P1). In live mode with the flip on, check the left 100 pixels at FFT sizes 512, 1024, 2048, and 4096. Expect no black bar, as in TP-EDGE-01.
- **TP-FLIP-10** (Image, P2). Check the labels of the cursor and the boxes of the values. Expect each label to stay inside the graph when the cursor is at the left edge and at the right edge, in both directions.
- **TP-FLIP-11** (Qt, P2). Play a window with the flip on. Expect the playback line to move from the right to the left, and expect the sound to match the line.
- **TP-FLIP-12** (Qt, P2). Drag in the graph from the left edge to the right edge in both directions. Expect the cursor to follow the pointer with no jump.
- **TP-FLIP-13** (Unit, P2). Check the time ticks with a decreasing pixel mapping. Expect the same labels as the increasing mapping, mirrored, and no overlap.
- **TP-FLIP-14** (Qt, P2). Change the switch "Newest sound at the left" while capture runs. Expect the picture to redraw at once with no stale column and no crash.
- **TP-FLIP-15** (Manual, P3). Check the wheel with the system setting "Natural scrolling" on and off. Expect the content to follow the fingers in both cases.
- **TP-FLIP-16** (Unit, P2). Check that an old file with no `flipTime` value starts with the flip on, and that a stored `false` stays.
- **TP-GRID-01** (Image, P1). Set each grid switch alone, then both. Expect lines at the labeled ticks for the major switch and at the other ticks for the minor switch.
- **TP-GRID-02** (Image, P2). Change the frequency scale (linear, logarithmic, Mel, ERB) and the tick mode. Expect each line to stay on its tick.
- **TP-GRID-03** (Image, P2). Check that the lines stay behind the tracks and that the lines are not in the minimap.
- **TP-GRID-04** (Image, P2). Check the brightness of the grid lines on each of the 16 pride themes and on Magma, with a loud sound and with silence. Expect the lines to show, and expect them not to hide the spectrogram.
- **TP-GRID-05** (Image, P2). Check the grid lines at device pixel ratios 1 and 2. Expect a line of about the same visual width.
- **TP-GRID-06** (Image, P2). Check the grid lines in the pitch-only graph with the fixed axis and with the automatic fit. Expect each line at a tick of the axis.
- **TP-GRID-07** (Qt, P2). Record with the grid on. Extract a frame from the video. Expect the lines in the frame.
- **TP-GRID-08** (Unit, P2). Test the grid with a custom tick step, with the note mode, and with the octave mode. Expect the major lines at the labeled ticks only.
- **TP-GRID-09** (Unit, P3). Test a graph so short that the ticks are closer than 2 pixels. Expect no more than one line for each pixel row.
- **TP-TRACK-01** (Qt, P1). Turn off each formant switch in turn. Expect only that track, its box, and its legend entry to go.
- **TP-TRACK-02** (Unit, P1). Migrate a file with the old switch off. Expect all four new switches off, and a second run to change nothing.
- **TP-TRACK-03** (Unit, P3). Remove the old setting `viewShowFormants` and its property in `config.h`, or test that nothing reads it. Done in 3.2.2: the property is gone and only the migration reads the old key.
- **TP-TRACK-04** (Qt, P1). Turn off one formant track in the pitch-only graph and turn the graph back to Full. Expect the choice to stay.
- **TP-TRACK-05** (Qt, P2). Turn off all four formant tracks. Expect the legend and the boxes to show the pitch only, and expect no empty gap in the left column.
- **TP-TRACK-06** (Qt, P2). Turn off F2 during a practice session. Expect the statistics of the session to keep F2, because the analysis does not stop.
- **TP-TRACK-07** (Qt, P2). Turn off F3 and check the cursor labels. Expect no label for F3.
- **TP-TRACK-08** (Unit, P2). Test a formant count of 3 and of 5 in the file (`formantCount`). Expect no read past the four switches.
- **TP-TRACK-09** (Qt, P3). Check the hint text of each formant switch with a speech-language pathologist. Expect the text to be correct and short.
- **TP-TAB-01** (Image, P1). Look at the Full and Pitch only buttons. Expect the chosen button to have a solid fill and the other to have an outline.
- **TP-TAB-02** (Qt, P1). Click each segment of the group with the mouse and with the keyboard. Expect the choice to change and the fill to follow.
- **TP-TAB-03** (Manual, P2). Show the group to five people for three seconds. Ask which mode is on. Expect a correct answer from all five.
- **TP-TAB-04** (Qt, P2). Check the tab bars of Settings and Help at the minimum window width. Expect all tabs to stay reachable.
- **TP-TAB-05** (Manual, P3). Check the group with VoiceOver. Expect the name, the role, and the chosen state.
- **TP-TAB-06** (Image, P3). Check the hover fill of a segment and of a tab on the dark window. Expect the fill to show, and expect the text to keep a contrast of at least 4.5 to 1.

## 6c. One shape for all controls

- **TP-SHAPE-01** (Unit, P2). Check that `cornerRadius` equals the medium radius. The unit test `ui_scale` does this.
- **TP-SHAPE-02** (Qt, P2). Scan all QML files. Expect each `Button` and `ToolButton` to be an `AppButton` or an `AppToolButton`, and expect each window and dialog to set `Material.roundedScale`.
- **TP-SHAPE-03** (Image, P2). Screenshot the Record sheet, the keep dialog, the reset dialog, and each menu and list popup. Expect a radius of 8 pixels. The Record sheet and the keep dialog were not checked.
- **TP-SHAPE-04** (Image, P2). Screenshot the boxes, the labels, and the legend in the graph at device pixel ratios 1 and 2. Expect a radius of 8 device-independent pixels.
- **TP-SHAPE-05** (Image, P3). Check the frame of the session map with a narrow frame (under 8 pixels). Expect no broken outline.
- **TP-SHAPE-06** (Image, P3). Check the combo boxes, the text fields, and the sliders. Expect the radius to match the buttons.

## 6d. Axes, grid, themes, and help (round 5)

- **TP-AXT-01** (Unit, P1). Check the time label step for views of 15, 30, 60, 120, 300, 600, and 3600 seconds. Expect 1, 2, 5, 10, 30, 60, and 300 seconds. The unit test `ticks` has these.
- **TP-AXT-02** (Unit, P1). Check that no time tick falls between whole seconds, in the automatic mode and in the custom mode with a step of 0.5.
- **TP-AXT-03** (Image, P2). Check the time labels at window widths of 600, 1000, and 2000 pixels. Expect no overlap and a larger step in a narrow window.
- **TP-AXT-04** (Unit, P1). Check that a mantissa gets a label in all decades or in none, for each density and each scale.
- **TP-AXT-05** (Image, P1). Check the frequency axis on each scale (linear, logarithmic, Mel, ERB) and at three window heights. Expect labels of the set 1, 2, 3, 5 only, one text size, and no overlap.
- **TP-AXT-06** (Unit, P2). Check the linear axis from 0 to 500 Hz and from 0 to 8000 Hz. Expect equal steps.
- **TP-AXT-07** (Qt, P2). Change "Number of labels" (Few, Normal, Many). Expect the graph to follow at once, and expect the grid lines to follow the same ticks.
- **TP-AXT-08** (Image, P2). Check that the label text of the frequency axis has one size in the custom, the note, and the octave modes.
- **TP-AXT-09** (Unit, P1). Check a logarithmic axis of less than two octaves, for example 95 to 235 Hz. Expect equal steps of 1, 2, or 5 times a power of ten. Expect three labels or more, for a ratio up to 3 and an axis of 300 pixels or more. Expect no overlap and no label outside the range, for 245 combinations of range and height. Expect a range of two octaves or more to keep the powers of ten.
- **TP-AXT-10** (Unit, P2). Give a label size of 0 to the linear axis and to the logarithmic axis. Expect the call to end with a short list of ticks.
- **TP-GRD-10** (Image, P1). Check the grid lines on every theme at strengths of 10, 60, and 100 percent. Expect lines that show, and expect no line to hide the loud sound.
- **TP-GRD-11** (Unit, P2). Check the grid color for a dark quiet color and a light quiet color. Expect a light tint for the first and a dark tint for the second.
- **TP-QUI-01** (Unit, P1). Lift the quiet end of a map that starts at black. Expect a lightness of 0.22 to 0.32 at the first entry. Expect the same colors above 45 percent of the map, and no fall of the lightness. The unit test `oklab` has this.
- **TP-QUI-02** (Unit, P2). Lift the quiet end of Viridis and of a gray map. Expect no change for Viridis and no chroma for the gray map.
- **TP-QUI-03** (Image, P1). Screenshot each of the 22 themes with the switch on and off. Expect the quiet parts to differ from the black window when the switch is off.
- **TP-QUI-04** (Image, P2). Check the graph before the first sound arrives and in the empty part of a flipped graph. Expect the quiet color and no black block.
- **TP-QUI-05** (Qt, P2). Turn the switch on and off while capture runs. Expect the strip in the Theme tab, the graph, the map, and the plots to follow at once.
- **TP-LFT-01** (Qt, P1). Start a live capture with the flip on. Expect the new sound at the left edge from the first second, and the old sound to flow to the right.
- **TP-LFT-02** (Qt, P2). Check the empty part at the right of a young capture. Expect the quiet color and no tick before time zero.
- **TP-LFT-03** (Qt, P2). Click in the empty part. Expect no pick, or a pick at time zero.
- **TP-LFT-04** (Qt, P2). Pause, resume, and press Reset graph with the flip on. Expect the new sound at the left edge again.
- **TP-LFT-05** (Unit, P2). Check the view end for a young capture (time 0 to 15 s) with the flip on and off. Expect a negative start with the flip on and a start of zero with the flip off.
- **TP-RGR-01** (Qt, P1). Press "Reset graph" after a capture of 5 seconds. Expect a clear graph with no question, and a new capture at time zero.
- **TP-RGR-02** (Qt, P1). Press it after 30 seconds. Expect the question. Cancel keeps the data. Clear deletes the tracks and the recording.
- **TP-RGR-03** (Qt, P1). Press it during a recording and during a practice session. Expect the button to be off.
- **TP-RGR-04** (Qt, P2). Press it with a loaded file. Expect the file to go, and expect a live capture at time zero.
- **TP-RGR-05** (Qt, P2). Press it during playback. Expect the playback to stop.
- **TP-RGR-06** (Qt, P2). Press it twice in a row. Expect no crash and no stale data. The data race between the reset and the analysis thread was not tested.
- **TP-PAD-01** (Image, P2). Check the top and the bottom of each Settings tab at the end of the scroll. Expect at least 24 pixels above the first heading and 32 pixels below the last control.
- **TP-WIN-01** (Qt, P1). Turn the synthesizer on and close its window. Expect the synthesizer to be off and the sound to stop.
- **TP-WIN-02** (Qt, P1). Change a setting, close the Settings window, and kill the app with SIGKILL. Start again. Expect the setting to stay.
- **TP-SEG-01** (Image, P2). Check the Full and Pitch only group in each state (chosen, hover, hover on the chosen one, pressed). Expect square inner corners and no gap between the segments.
- **TP-HLP-01** (Qt, P1). Press the "?" button of each setting. Expect a box with the text, and expect a scroll bar for a long text.
- **TP-HLP-02** (Qt, P2). Close the box with a press outside, with Escape, and with a second press on the button.
- **TP-HLP-03** (Qt, P2). Open the box near the right edge and the bottom edge of the window. Expect the box to stay inside the window.
- **TP-HLP-04** (Manual, P2). Check each text of the boxes with a speech-language pathologist.
- **TP-TYP-01** (Image, P1). Check each Help page at widths of 480, 760, and 1400 pixels. Expect a column of at most 640 pixels, centered, with no sideways scroll.
- **TP-TYP-02** (Unit, P1). Check the prose scale. Lines and gaps must be on the 4-pixel grid. A body line must be at least 1.6 times the size. A heading must have more space above it than below it. The unit test `ui_scale` has this.
- **TP-TYP-03** (Image, P2). Check a table, a code block, a list, a block quote, a link, and bold and code text in one paragraph. Expect the right size, color, and gap for each.
- **TP-TYP-04** (Qt, P3). Select text with the mouse and copy it. Expect the text without style marks.
- **TP-TYP-05** (Manual, P2). Read each page for five minutes. Compare the comfort with the old pages.
- **TP-GDE-01** (Manual, P2). A speech-language pathologist reads the page "Settings". Check each claim about a method, and check the advice for each goal.
- **TP-GDE-02** (Unit, P2). Check the numbers of the FFT table on the page (window time and line distance) against the formula for a view up to 5500 Hz.
- **TP-STP-01** (Manual, P1). Measure the time from the start of the app to the line "Input device" in the log. The time must be under 3 seconds on each input device. A first build of the help styling took 12 seconds, and one edit block fixed it. The audio device was not the cause.
- **TP-HLP-05** (Qt, P1). Press each of the 37 "?" buttons in a window of 700 by 900 pixels. Expect a box fully inside the window and a scroll bar for a long text. Expect a close on Escape, on an outside press, and on a second press. Only the Pitch button got a screenshot check.
- **TP-HLP-06** (Qt, P2). Press a "?" button near the bottom edge and near the right edge of a narrow window of 420 pixels. Expect a box that stays inside the window.
- **TP-HLP-07** (Qt, P2). A test that presses a "?" button must press a visible button of the open tab. The test hook of the first run pressed a button of a hidden tab, and the box showed in a wrong place.
- **TP-STP-02** (Unit, P1). Time `HelpStyle::apply` on all help pages. Expect less than 200 ms in total. The test allows 600 ms, because a busy machine is slower. Without one edit block, the start of the app took 12 seconds.
- **TP-STP-03** (Manual, P2). Start the app for the first time, with the microphone permission not yet given. Expect the window in less than 3 seconds and the permission box without a freeze.
- **TP-STP-04** (Qt, P2). Start the app with the default input device on a camera microphone, on a USB device, and on the built-in microphone. Expect less than 3 seconds to the line "Input device" on each.
- **TP-LFT-06** (Qt, P1). Start a live capture with a view of 2 seconds and an FFT size of 4096. Expect the newest sound at the left edge from the first column. Expect no jump when the sound fills the view.
- **TP-RGR-07** (Qt, P1). Press "Reset graph" 100 times during a live capture, with a pause of 50 ms. Expect no crash, no stale track, and no frozen analysis.
- **TP-RGR-08** (Qt, P2). Press "Reset graph" with more than 10 seconds of data. Cancel the question and expect the data to stay. Accept it and expect an empty graph and the status "The graph is clear".
- **TP-RGR-09** (Qt, P2). Press "Reset graph", and then use "Export window" and "Play window". Expect a clear message and no file of old data.
- **TP-WIN-03** (Qt, P1). Close the synthesizer window with the red button, with the key combination for the window, and with Escape. Expect `synth.enabled` false each time.
- **TP-WIN-04** (Qt, P2). Change a setting, press quit with the key combination for the app, and start the app again. Expect the new setting.
- **TP-QUI-06** (Unit, P2). For each theme with the lift, compute the contrast of the grid color and of each track color against the quiet color. Expect a contrast ratio of at least 3 to 1.
- **TP-QUI-07** (Image, P3). Compare the lifted and the black quiet color on a bright monitor in a bright room. Expect that the lifted color is not too light.
- **TP-DOC-11** (Unit, P3). A script lists British spellings (for example "colour", "centre", "practise") in `docs`, in `README.md`, and in the strings of `src/qml`. Expect none. The check keeps the UI and the help pages in one spelling.
- **TP-DOC-12** (Manual, P2). A speech-language pathologist checks the edits of the peer review. Check the F1 statement in the guides for feminization and masculinization, and check the range of 8 to 18 percent.
- **TP-DOC-13** (Unit, P3). Check that each page name in the text of a page matches the title that the Help window shows. The page "Settings" has the title "Choosing your settings".

## 6e. Scrolling at the rate of the system

The scroll views of the settings, the help pages, the help boxes, the Synthesizer window, and the Progress window use `NativeWheel.qml`. It follows the distance that the system reports.

- **TP-SCR-01** (Qt, P1). Send 20 wheel events of 10 pixels with a pixel distance (a trackpad) to a scroll view. Expect the content to move by 200 pixels. Version 3.2.0 moved it by 1 pixel.
- **TP-SCR-02** (Qt, P1). Send one notch (120 units, no pixel distance) to a scroll view. Expect a move of the system lines times 24 pixels (72 pixels with 3 lines).
- **TP-SCR-03** (Qt, P1). Scroll past the top and past the end. Expect the content to stop at the edges, with no blank space.
- **TP-SCR-04** (Qt, P2). Send the events with the direction of natural scrolling on and off. Expect the content to follow the direction of the system.
- **TP-SCR-05** (Qt, P2). Scroll each of the seven tabs of the Settings window, the eight help pages, a help box, the Synthesizer window, and the Progress table. Expect the same rate in each.
- **TP-SCR-06** (Qt, P2). Scroll with the pointer over a slider, a switch, a field, and a list box. Expect the content to move, and expect no change of a value.
- **TP-SCR-07** (Qt, P2). Scroll a view that is shorter than its window. Expect no movement.
- **TP-SCR-08** (Manual, P1). Scroll the Settings window and a help page with a real trackpad and with a real mouse. Expect the feel of other apps on the same Mac, including the momentum at the end of a swipe.
- **TP-SCR-09** (Manual, P2). Change the scroll speed and the direction in System Settings. Expect the app to follow without a restart.
- **TP-SCR-10** (Unit, P2). Check that the scroll line is a multiple of the grid unit of the scale. The unit test `ui_scale` does this.

## 7. Switching between modes while capture runs

- **TP-MODE-01** (Qt, P1). Press "Pitch only" during live capture. Expect no stale spectrogram on the screen, and expect the pitch track to continue.
- **TP-MODE-02** (Qt, P1). Press "Full" during live capture. Expect the spectrogram to come back with the right time range.
- **TP-MODE-03** (Qt, P2). Switch ten times in one second. Expect no crash and no leak.
- **TP-MODE-04** (Qt, P2). Switch while a practice session runs. Expect the session to continue and its statistics to stay complete.
- **TP-MODE-05** (Qt, P2). Switch while a recording runs. Expect the video to follow the mode.
- **TP-MODE-06** (Qt, P2). Click each toolbar button (Full, Pitch only, Help, Progress, Settings, Oscilloscope, Synthesizer). Expect the right window or mode.

## 8. The pitch-only graph

- **TP-PITCH-01** (Unit, P2). Test the fixed axis from 0 to 500 Hz. Expect a linear scale and the zones in full.
- **TP-PITCH-02** (Unit, P2). Test the automatic fit with the 5th and the 95th percentile, a margin, and a minimum of 12 semitones.
- **TP-PITCH-03** (Unit, P2). Feed octave jumps to the fit. Expect the fit to ignore them.
- **TP-PITCH-04** (Qt, P2). Type values in the two axis fields: from above to, negative, above 2000, not a number. Expect a safe result.
- **TP-PITCH-05** (Unit, P2). Not built yet: an axis in semitones with note names. Expect labels such as `A3`, `C4`, and the right spacing.
- **TP-PITCH-06** (Unit, P3). Not built yet: pause the formant analysis in this mode. Expect lower CPU use and no loss of the pitch.
- **TP-PITCH-07** (Unit, P1). Test the jump limit of the line of a track on the pitch-only axis of 12 semitones. Expect a glide of 3 semitones in one frame to stay in one run. Expect a jump of 12 semitones to split the run, with the border at 8 semitones. Expect a frame with no value to split a run. Expect a time gap of more than three median gaps to split a run. Test a linear axis from 60 to 500 Hz. Expect an octave error from 100 to 200 Hz to split the run, because the limit comes from the low end of the range. The old limit of a tenth of the height split the line at each normal step.
- **TP-PITCH-08** (Image, P1). Choose Pitch only and the style Line for the pitch track. Speak a sentence with rising and falling words. Expect one connected line for each voiced part, and a break only at a silence or at an octave error.

## 9. Pitch zones

- **TP-ZONE-01** (Image, P2). Draw the zones on the linear, logarithmic, Mel, and ERB scales. Expect the edges at the right heights on each scale. The Mel scale was never captured.
- **TP-ZONE-02** (Unit, P2). Test edge values that are not in order, equal, zero, negative, or not numbers. Expect a safe fallback.
- **TP-ZONE-03** (Qt, P2). Record with the zones on and "Keep the zones in recordings" on. Extract a frame from the saved video. Expect the zones to show.
- **TP-ZONE-04** (Qt, P2). Record with "Keep the zones in recordings" off. Extract a frame. Expect no zones, and expect the zones to come back on the screen after the recording.
- **TP-ZONE-05** (Image, P3). Check the labels of the zones at a small window height.
- **TP-ZONE-06** (Unit, P3). Check that the zone colors differ for a person with red-green color blindness (simulate it).

## 10. Color themes

- **TP-COL-01** (Unit, P2). For each of the 16 pride themes, check that the lightness rises in order and that no stop is white.
- **TP-COL-02** (Unit, P1). For "Trans (Dark)", check the top three stops. Expect an OKLab chroma of at least 0.12 after gamut mapping, and a hue between 340 and 360 degrees.
- **TP-COL-03** (Unit, P2). Check that the color table of each theme has 256 entries and that each entry is in the sRGB gamut.
- **TP-COL-04** (Unit, P2). Check the OKLab distance between each track color and every entry of its table. Expect a minimum above a set value.
- **TP-COL-05** (Image, P2). Screenshot each of the 16 pride themes with a real speech file. Only six themes and a strip picture of the others were checked.
- **TP-COL-06** (Unit, P2). Change the saturation from 50 to 150 percent. Expect the chroma to scale and the lightness to stay.
- **TP-COL-07** (Qt, P2). Move the Saturation slider. Expect the stored value to change in steps of 5 percent.
- **TP-COL-08** (Image, P2). Check that the 16 dB headroom does not make the default Magma theme too dark. Compare with a stored picture.
- **TP-COL-09** (Unit, P3). Parse a custom theme with a wrong color, a missing color, and a very long list. Expect a safe fallback.
- **TP-COL-10** (Image, P3). Check the dark outline of dots and lines on each theme.

## 11. Capture lifecycle and devices

- **TP-CAP-01** (Manual, P1). Start the app. Expect no capture and no CPU load until the person presses play.
- **TP-CAP-02** (Manual, P1). Change the default input device while capture runs. Expect the app to switch to the new device.
- **TP-CAP-03** (Manual, P1). Change the default input device while capture is paused. Expect the app to use the new device at the next start.
- **TP-CAP-04** (Manual, P1). Change the default device before the first press of play. Expect the new device.
- **TP-CAP-05** (Manual, P2). Unplug the input device while capture runs. Expect a visible message and no crash.
- **TP-CAP-06** (Manual, P2). Start with no input device. Expect a visible message and no crash.
- **TP-CAP-07** (Manual, P2). Choose a device that is not the default. Unplug it. Expect a fall back to the default device or a message.
- **TP-CAP-08** (Unit, P2). Check the name of the session scratch file. Expect the process ID in the name.
- **TP-CAP-09** (Manual, P1). Kill the app with SIGKILL, then start it again. Expect the app to delete the orphan scratch file and to keep files of running processes.
- **TP-CAP-10** (Manual, P1). Quit with SIGTERM. Expect the scratch file to go and the practice session to be saved.
- **TP-CAP-11** (Manual, P2). Quit with "keep recordings" on. Expect a WAV file in the library folder. Quit with it off. Expect no file.
- **TP-CAP-12** (Manual, P2). Change the sample rate of the input device while capture runs. Expect the app to follow.
- **TP-CAP-13** (Manual, P2). Use a Bluetooth microphone that changes its sample rate when the output opens. Expect no crash and no wrong pitch.
- **TP-CAP-14** (Manual, P3). Deny the microphone permission. Expect a clear message.
- **TP-CAP-15** (Unit, P2). Stop the synthesizer and the playback stream before the audio system restarts, and start them after. Expect no write to a closed stream.
- **TP-CAP-16** (Unit, P1). The capture buffer reserves 1048576 samples at the start, and a push never allocates memory. A push into a full buffer drops the samples and counts them. The samples keep their order. The buffer takes samples again after the reader catches up.

- **TP-CAP-17** (Unit, P1). Press play with the permission for the microphone in each state. Granted starts the capture. Undetermined asks the system. Denied stays paused and shows a message that names the System Settings.
- **TP-CAP-20** (Unit, P1). Check that the program asks for the microphone through AVFoundation and not through the permission plugin of Qt. Check that the source file of the call is in the list before the target, and that no file includes the Qt permission header. Check that a denied answer opens a dialog with a button for the System Settings.
- **TP-CAP-21** (Manual, P1). Start an app that has no microphone access, for example a new build with a new signature. Expect the window at once, with no question of the system. Press play. Expect the question of the system, and the capture after you allow it.
- **TP-CAP-18** (Manual, P1). Reset the permission with `tccutil reset Microphone fr.cloyunhee.InFormant`, and start the app. Expect no question at the start. Press play. Expect the question of the system, and the capture after you allow it.
- **TP-CAP-19** (Manual, P1). Deny the microphone, and press play. Expect the paused state and the message with the System Settings. Allow it in the System Settings, and press play. Expect the capture.

## 12. Settings and persistence

- **TP-SET-01** (Qt, P2). Open the Settings window with the key combination Command and comma. Expect the window to open, and expect a second press not to open a second window.
- **TP-SET-02** (Qt, P2). Click each control in each of the seven tabs. Expect the stored option to change and the display to follow.
- **TP-SET-03** (Qt, P2). Set each option, quit, and start again. Expect each option to keep its value.
- **TP-SET-04** (Unit, P1). Migrate a file of each version from 0 to 7. Expect the chain to end at version 8 with the right defaults.
- **TP-SET-05** (Unit, P1). Migrate a version 5 file with `showStats = false`. Expect `true`. Migrate it again. Expect no change.
- **TP-SET-06** (Unit, P2). Migrate a version 5 file with no `showStats`. Expect no value, so the default `true` applies.
- **TP-SET-15** (Unit, P2). Start with no file. Expect the boxes at the left of the graph. Migrate a version 6 file with `readoutSide = "bottom"`. Expect `left`, and expect no second change.
- **TP-SET-07** (Unit, P2). Start with no file. Expect `showStats` on, and expect the boxes of the five tracks with the default statistics.
- **TP-SET-08** (Unit, P2). Load a file with an unknown key, a wrong type, and a broken line. Expect the app to start with defaults for the broken parts.
- **TP-SET-09** (Qt, P2). Change the practice settings fields (folder, task, keep audio). Expect the practice mode to use them at the next session.
- **TP-SET-10** (Qt, P2). Click a statistic check box. Expect the box to gain or lose the line at once.
- **TP-SET-11** (Qt, P2). Change the statistics window. Expect the statistics to cover that time.
- **TP-SET-12** (Qt, P3). Select "Show the Range in semitones". Expect a value such as `7.0st` in the Range line.
- **TP-SET-13** (Unit, P2). Check that the Range in semitones is 12 times log2(P95 / P5). Test P5 of 0 or negative. Expect a safe text.
- **TP-SET-14** (Qt, P3). Check the gray hint under each control of each tab. Expect the text to wrap, to stay inside the window, and not to overlap.
- **TP-SET-16** (Unit, P2). Check that every setter in `config.cpp` works when the file has no key for it. A setter crashed the app once for this reason (`showFormants`).
- **TP-SET-18** (Unit, P1). Start with a file that has a syntax error. Expect the defaults and a copy of the file named `informant.conf.broken`. Before 3.2.2 the program stopped at launch.
- **TP-SET-17** (Unit, P2). Check that each default value outside the option map has one definition (the `kDefault` constants). The getter and the reset must use the same one.

## 12b. The reset button

- **TP-RST-01** (Qt, P1). Change one option in each of the seven tabs. Press "Reset all settings to defaults" and confirm. Expect every option to return to its default.
- **TP-RST-02** (Unit, P1). Compare every key of `defaultOptions()` after the reset with its default value. Expect no difference, except `practiceFolder`.
- **TP-RST-03** (Unit, P1). Check the view values after the reset. These are the zoom, the two frequency limits, the FFT size, the gain, the quality, the blur, the time span, the scale, and the layers. Expect the defaults.
- **TP-RST-04** (Unit, P1). Check the four algorithms after the reset. Expect RAPT, Burg, Filtered, and GFM-IAIF.
- **TP-RST-05** (Qt, P1). Choose an input device that is not the default. Reset. Expect the system default device, and expect capture to continue on it.
- **TP-RST-06** (Qt, P1). Press Cancel in the dialog, and press Escape. Expect no change.
- **TP-RST-07** (Qt, P1). Set a practice folder. Reset. Expect the folder to stay, and expect the Progress window to still show the sessions.
- **TP-RST-08** (Qt, P1). Reset during live capture. Expect the capture to continue, the graph to redraw with the default view, and no crash.
- **TP-RST-09** (Qt, P1). Reset during a practice session and during a recording. Expect the session or the recording to continue and to finish with no loss.
- **TP-RST-10** (Unit, P2). Reset twice in a row. Expect no change and no error the second time.
- **TP-RST-11** (Qt, P2). Reset, quit, and start the app. Expect the default values, and expect no key that the reset left behind.
- **TP-RST-12** (Qt, P2). Check that the reset does not remove sessions, recordings, exports, or the scratch recording.
- **TP-RST-13** (Qt, P3). Check the text of the button, the dialog, and the hint. Expect the text to say that the action cannot be undone.
- **TP-RST-14** (Not built yet, P3). A reset for one tab, and a reset for the view only (the zoom button at 100 percent). Ask the user which one they want, then test it.
- **TP-RST-15** (Unit, P2). Make the reset testable without a window. `Config` needs a file and Qt objects today. Add a `Config` that works on a table in memory, then write TP-RST-02 to TP-RST-04 as plain unit tests.

## 13. Type scale and spacing scale

- **TP-UI-01** (Unit, P2). Check the scale values. The type scale is 12, 14, 17, 20, and 24 pixels, with the line heights 16, 20, 24, 28, and 32. The spacing scale is 4, 8, 12, 16, 24, 32, and 48 pixels. The unit test `ui_scale` has the rules.
- **TP-UI-02** (Qt, P2). Scan all QML files. Expect no hard-coded font size or margin outside the two scales, except where a comment says why.
- **TP-UI-03** (Image, P2). Screenshot each window at device pixel ratios 1 and 2. Expect no clipped text.
- **TP-UI-04** (Unit, P2). Check the contrast of the gray hint color against the dark background. Expect at least 4.5 to 1.
- **TP-UI-05** (Qt, P3). Resize each window to its minimum size. Expect no overlap and a working scrollbar.
- **TP-UI-06** (Manual, P3). Use a large system font. Expect the layout to hold.
- **TP-UI-07** (Qt, P3). Check the keyboard focus order in each window and each dialog.
- **TP-UI-08** (Manual, P3). Check each control with VoiceOver. Expect a useful name for each control.
- **TP-FONT-01** (Qt, P2). Check that the families "Roboto" and "Roboto Mono" load from the resources on a Mac with no Roboto fonts installed.
- **TP-FONT-02** (Image, P2). Check that a heading shows in bold and a field label shows in medium weight.
- **TP-FONT-03** (Image, P2). Check that the digits in the boxes of the tracks, in the frame times, and on the axes keep their width when the values change.
- **TP-FONT-04** (Unit, P3). Check that the frame-times text has the same width for 0.00 ms and for 99.99 ms.
- **TP-FONT-05** (Image, P3). Check the text of the graph at device pixel ratios 1 and 2, and at the zoom steps of the toolbar.
- **TP-FONT-06** (Unit, P3). Check that each font file is in Git LFS and that each license file is in `resources`.
- **TP-UI-15** (Image, P2). Check the gap between each switch and its hint. Expect at least 8 pixels from the visual bottom of the switch, in the Settings, Record, and Synthesizer windows.
- **TP-UI-10** (Qt, P2). Check that each heading uses `Heading`, each hint uses `Hint`, and no window has a separator line between groups.
- **TP-UI-11** (Image, P2). Check the spacing of one window with an overlay of the 4-pixel grid. Expect each gap to be a step of the scale.
- **TP-UI-12** (Unit, P3). Check that no paragraph in a window is wider than 560 pixels.
- **TP-UI-13** (Unit, P2). Check the contrast of the three text colors against the window and the surface. The unit test `ui_scale` does this. Repeat it when the Material theme changes.
- **TP-UI-14** (Image, P3). Check that numbers in a column keep their width (tabular figures) when the values change.
- **TP-UI-09** (Qt, P2). The task box of the practice mode must show the word "Conversation" in full. Test every task name and a long custom name.
- **TP-UI-16** (Image, P2). The text in the graph (the boxes, the axes, the cursor labels, the frame times) uses sizes of 10, 12, and 14 pixels. Decide which steps of the type scale they use, change them, and compare screenshots.
- **TP-UI-17** (Manual, P3). Measure the reading speed. Ask five people to find three values in the Settings window and in the boxes of the graph. Compare the time before and after the type scale.

## 14. Practice mode

- **TP-PRA-30** (Qt, P1). Stop a practice session of at least the minimum time. Expect the dialog "Keep this practice session?" with the task, the length, the marks, and the comparison. Expect no files before the answer.
- **TP-PRA-31** (Qt, P1). Answer Keep. Expect the session folder with all files, and expect the status text "Practice saved".
- **TP-PRA-32** (Qt, P1). Answer Discard. Expect no folder, no marks left, and the status text "The practice is discarded".
- **TP-PRA-33** (Qt, P2). Press the key Escape and click outside the dialog. Expect the dialog to stay open until the person chooses.
- **TP-PRA-34** (Manual, P1). Quit while the dialog is open. Expect the session to be saved, because the person cannot answer.
- **TP-PRA-35** (Unit, P1). Stop a session that is shorter than `practiceMinSeconds` (default 5). Expect no dialog and no files. Test 4.9, 5.0, and 5.1 seconds.
- **TP-PRA-36** (Qt, P2). Change `practiceMinSeconds` from 2 to 30 in the General tab. Expect the new limit at the next session.
- **TP-PRA-37** (Unit, P3). Set `practiceMinSeconds` to 0, to a negative number, and to text. Expect a minimum of 1 second.
- **TP-PRA-38** (Qt, P2). Start a new practice while the dialog is open. Expect that the window blocks this action.
- **TP-PRA-39** (Manual, P2). The user asked for a minimum time of "5 to 10 seconds". The app reads this as a limit with a default of 5 seconds and a range of 2 to 30 seconds. Ask the user if this is right, then fix the text or the default.

- **TP-PRA-01** (Qt, P1). Start a practice session. Expect one click to start and one click to stop.
- **TP-PRA-02** (Qt, P1). Press the key M and the button Mark. Expect a mark with the pitch and F1 to F4 at that time.
- **TP-PRA-03** (Qt, P2). Type a note in the note field and press Enter. Expect a mark with the note.
- **TP-PRA-04** (Unit, P1). Check `session.json`, `report.md`, `stats.csv`, `marks.csv`, and `audio.wav` for one session. Expect all fields.
- **TP-PRA-05** (Unit, P2). Run a session with `practiceKeepAudio` off. Expect no `audio.wav` and all statistics.
- **TP-PRA-06** (Manual, P1). Quit during a session. Expect the session to be saved in the library.
- **TP-PRA-07** (Manual, P2). Quit during a session, with the folder not writable. Expect a clear result and no data loss.
- **TP-PRA-08** (Manual, P2). Start with the default folder in Documents on a new account. Expect the privacy question of macOS, and expect a clear message when the person denies it.
- **TP-PRA-09** (Unit, P2). Load a library with one broken `session.json`. Expect the program to skip it and to list the others.
- **TP-PRA-10** (Unit, P2). Load a library of 5000 sessions. Expect the Progress window to open in under 2 seconds. The test allows 6 seconds, because a busy machine is slower.
- **TP-PRA-11** (Unit, P2). Export all sessions. Expect each folder, a `history.csv`, and a status text. Test a target folder that exists and a read-only target.
- **TP-PRA-12** (Not built yet, P3). A video in a practice session. Expect the same files as Record.
- **TP-PRA-13** (Not built yet, P3). A zip export with `ditto`. Expect one file with all sessions.
- **TP-PRA-14** (Qt, P1). Record two sessions of the same task. Expect the summary dialog to show the comparison with the first. Check the text on the screen, not only in the unit test.
- **TP-PRA-15** (Unit, P2). Compare a session with a previous session that has no formants. Expect lines only for the common values.
- **TP-PRA-16** (Unit, P2). Check the first session of a task. Expect the text "first session".
- **TP-PRA-17** (Unit, P2). Test a session of 0 seconds, of 1 second, and of 12 hours. Expect safe statistics.
- **TP-PRA-18** (Unit, P2). Test a session with no voiced frames. Expect zero counts and no division by zero.
- **TP-PRA-19** (Qt, P2). The Record button stays off during a practice session, and the Practice button stays off during a recording.
- **TP-PRA-20** (Qt, P3). Change the task during a session. Expect the old session to keep its task.
- **TP-PRA-21** (Unit, P2). Check the timeline of medians for a session of 5 seconds and of 3 hours. Expect a limited number of points.

## 15. The Progress window and its trends

- **TP-PRG-01** (Unit, P1). For each of the 14 statistics and each of the 5 tracks, check the value of one known session. The statistics are Median, Mean, SD, CV, Min, P5, P25, P75, P95, Max, Range in Hz, Range in semitones, Voiced share, and Count.
- **TP-PRG-02** (Unit, P2). Check the minimum counts: SD needs 2 values, CV needs 2 values and a mean above 0, and the ranges need 5 values.
- **TP-PRG-03** (Unit, P2). Check a track with frames and no values. Expect a Voiced share of 0 and a Count of 0, and no value for the other statistics.
- **TP-PRG-04** (Unit, P2). Check the grouping by day, week, month, and year across a year end, a leap day, and a change of the ISO week year.
- **TP-PRG-05** (Unit, P2). Check the grouping with sessions at 23:59 and 00:01 in a time zone that changes for summer time.
- **TP-PRG-06** (Unit, P2). Check the median of an even and an odd number of session values.
- **TP-PRG-07** (Unit, P2). Check the filter by task with different letter case, and with the empty filter.
- **TP-PRG-08** (Qt, P2). Choose each measure and each statistic in the window. Expect the chart, the units, and the table to update. Only the default measure was checked on the screen.
- **TP-PRG-09** (Qt, P2). Choose a statistic with no value in any session. Expect the message "No session has a value".
- **TP-PRG-10** (Qt, P2). Check the color of the chart for the pitch and for each formant.
- **TP-PRG-11** (Image, P3). Check the chart with 1 point, 2 points, 200 points, and all values equal.
- **TP-PRG-12** (Qt, P2). Save a new session while the window is open. Expect the chart to refresh.
- **TP-PRG-13** (Unit, P3). Check the old API (`TrendMeasure`) against the new API (`TrendStat`). Expect the same numbers.
- **TP-PRG-14** (Unit, P1). Delete one session of the library. Expect the folder and all files to go, and the other sessions to stay. Expect the empty day, month, and year folders to go. Expect the root, and any folder with a file of the person, to stay. Expect false for a session that is not there. Expect false for a folder with no `session.json`. Expect false for an id with path parts, for an empty id, and for a link out of the library. Expect no file outside the library to change. Copy a session into another folder of the library, so that two rows have the same id. Expect the delete of the original to leave the copy, and the delete of the copy to be refused.
- **TP-PRG-15** (Qt, P1). Open the Progress window. Expect a list of the sessions, newest first. Expect the date, the task, the minutes, the pitch, the marks, and a Delete button in each row. Press Delete. Expect a question with the date, the task, and the length. Press Delete in the question. Expect the session to leave the list. Expect the chart and the table to refresh, and the status "The practice session is deleted".
- **TP-PRG-16** (Qt, P2). Press Delete and then Cancel in the question, and close the question with the key Escape. Expect the session to stay in the list and on the disk.
- **TP-PRG-17** (Qt, P2). Stop a practice, answer Keep, and delete the new session at once, while the sound is still in the write. Expect no crash, no file left in the library, and no error text.
- **TP-PRG-18** (Unit, P2). Check that the Progress window has the list, the Delete button, and the question with a Cancel button. Check that the program has the call `deleteSession`. Check that the question says that exported files stay.

## 16. Statistics and reports

- **TP-STA-01** (Unit, P1). Check that the report and the live table use the standard deviation with N minus 1.
- **TP-STA-02** (Unit, P2). Check the percentiles for 1, 2, 3, and 1000 values, and for equal values.
- **TP-STA-03** (Unit, P2). Check CV, Voiced share, and N for a window with no values. When the window has frames, expect a dash for the CV, 0% for the share, and 0 for N. With no frame, expect a dash for all three.
- **TP-STA-04** (Unit, P2). Check the statistics window (visible, 5, 10, 30, and 60 seconds) at the start of a session, when less data exists than the window.
- **TP-STA-05** (Unit, P2). Check the note name and the cents for 440 Hz, for 435 Hz, for the lowest and the highest pitch, and for 0.
- **TP-STA-06** (Unit, P3). Check the number format for values of 999.95 and 1000.

## 17. Recording and export

- **TP-REC-01** (Manual, P1). Record 10 seconds with the video on. Expect an `.mp4`, a `.wav`, and an `.md` with one name.
- **TP-REC-02** (Manual, P2). Record with the video off. Expect no `.mp4`.
- **TP-REC-03** (Manual, P2). Record with the side windows on, with the oscilloscope and the synthesizer open. Expect a column at the right of the video.
- **TP-REC-04** (Manual, P2). Record at 10, 20, and 30 frames each second and at a graph time of 2, 10, and 60 seconds.
- **TP-REC-05** (Manual, P2). Cancel the file dialog. Expect no recording and no leftover file.
- **TP-REC-06** (Manual, P2). Record to a read-only folder, and to a full disk. Expect a visible message.
- **TP-REC-07** (Qt, P2). Open the Record sheet. Expect the last choices of the previous recording.
- **TP-REC-08** (Manual, P2). The graph keeps the time of the sheet during a recording and returns to the old time after.
- **TP-REC-09** (Manual, P2). Export a window and export all. Expect the same choices and files as the sheet says.

## 18. Help pages and documents

- **TP-DOC-20** (Unit, P2). Check that `docs/design-system.md` lists the same numbers as `src/gui/uiscale.h`.

- **TP-DOC-01** (Unit, P2). Run the style check on every file in `docs/` and on the README. Expect no new violation. The README has 12 known violations. The test runs only when the variable `INFORMANT_STE_CHECK` gives the path of the style script.
- **TP-DOC-02** (Unit, P2). Check each link in the pages. Expect an answer of 200 for each source URL. A site can refuse an automated client (401 or 403). A site can fail for a short time (5xx). The test only reports these answers. The test needs the network. It runs only when the variable `INFORMANT_CHECK_LINKS` is set.
- **TP-DOC-03** (Qt, P2). Open each of the eight tabs. Expect the page text, and expect the tab order Display, Settings, Feminization, Masculinization, Singing, Warm-ups, Synthesizer, The math.
- **TP-DOC-04** (Qt, P3). Call `showPage` with a known and an unknown file. Expect the tab to change for the known file and no error for the unknown file.
- **TP-DOC-05** (Unit, P2). Check that each file of `docs/` is in `resources/qml.qrc` and in the list of the Help window.
- **TP-DOC-06** (Unit, P2). Check that the page "Warm-ups and vocal health" has the CAUTION text and the section "Sources".
- **TP-DOC-07** (Manual, P2). A speech-language pathologist reads the page "Warm-ups and vocal health". The ranks are a judgment from search summaries and not from full papers. Check each rank against the full papers.
- **TP-DOC-08** (Unit, P3). Check that the worked examples in the guides (5.0 to 9.7 and 7.0 to 4.4 semitones) match the formula.

## 19. Analysis quality

- **TP-ANA-01** (Audio, P1). Run the formant tracker on synthetic vowels with a pitch of 100 to 1000 Hz. The tracker is not reliable above about 400 Hz. Record the error against the pitch.
- **TP-ANA-02** (Audio, P2). Run the pitch tracker on a signal with an octave jump, on vocal fry, and on whispered speech. Expect the documented behavior.
- **TP-ANA-03** (Audio, P2). Run each pitch method (YIN, McLeod, RAPT) on the same file. Compare the tracks.
- **TP-ANA-04** (Audio, P2). Run each formant method and each LPC method on the same file. Compare the tracks.
- **TP-ANA-05** (Audio, P2). Test the formant limit of 5200 Hz and the view range of 5500 Hz. Expect the tracker not to follow the view range.
- **TP-ANA-06** (Audio, P3). Test a very quiet file, a clipped file, and a file with a DC offset.
- **TP-ANA-07** (Unit, P1). Run YIN on pure sines and on harmonic sums at 100, 220, and 440 Hz, at 16 kHz and at 48 kHz. Expect the pitch within 1 percent.
- **TP-ANA-08** (Unit, P1). Run YIN on a harmonic sum whose level rises over the frame. Expect the pitch within 1 percent. The difference function uses a fixed window, so the rise does not move the minimum.
- **TP-ANA-09** (Unit, P1). Run YIN on silence and on white noise. Expect no voiced result.
- **TP-ANA-10** (Unit, P2). Run YIN on 440 Hz at 16 kHz. The period is 36.36 samples. Expect a result within 2 Hz, which needs the parabolic interpolation of the lag.
- **TP-ANA-11** (Unit, P3). Run YIN on a frame, then on a frame of another length. Expect the same result as a new solver gives.
- **TP-ANA-12** (Unit, P1). Run MPM on the signals of TP-ANA-07. Expect the pitch within 1 percent.
- **TP-ANA-13** (Unit, P1). Run MPM on silence and on white noise. Expect no voiced result.
- **TP-ANA-14** (Unit, P1). Run two MPM solvers in turn, with frames of different lengths and rates. Expect the right pitch from each.
- **TP-ANA-15** (Unit, P1). Run two MPM solvers on two threads, with frames of different lengths. Expect the right pitch for every call. The solvers must not share an FFT.
- **TP-ANA-16** (Unit, P2). Run RAPT at 16 kHz, then at 48 kHz, then at 16 kHz. Expect the LPC order to follow the sample rate each time.
- **TP-ANA-17** (Unit, P2). Run two RAPT solvers on two threads. Expect the path of each to equal the path of a run on one thread.
- **TP-ANA-18** (Unit, P1). Run IAIF at 16 kHz and at 48 kHz, in turn. Expect a finite output with a peak of 1 each time.
- **TP-ANA-19** (Unit, P1). Run IAIF at 16 kHz, 48 kHz, 44.1 kHz, and 16 kHz again with one solver. Expect the output of each call to equal the output of a reference that holds no state. The filter and the window must follow the sample rate.
- **TP-ANA-20** (Unit, P1). Run IAIF with frames of 1024, 512, 4096, 700, and 1024 samples. Expect no crash, a finite output of the frame length, and the output of the reference. The Hann window must have `lpW` points and divide by `lpW - 1`.
- **TP-ANA-21** (Unit, P2). Run two IAIF solvers on two threads, at two rates. Expect the output of the reference from each.
- **TP-ANA-22** (Unit, P1). Run GFM-IAIF with the case of TP-ANA-18.
- **TP-ANA-23** (Unit, P1). Run GFM-IAIF with the case of TP-ANA-19.
- **TP-ANA-24** (Unit, P1). Run GFM-IAIF with the case of TP-ANA-20.
- **TP-ANA-25** (Unit, P2). Run two GFM-IAIF solvers with the case of TP-ANA-21.
- **TP-ANA-26** (Unit, P2). Run two covariance LPC solvers on two threads, with different orders and lengths. Expect the result of each to equal the result of a run on one thread.
- **TP-ANA-27** (Unit, P3). Find the roots of a polynomial with small roots and of one with large roots, in turn. Expect all roots each time. The start values must come from the polynomial at hand.
- **TP-ANA-28** (Unit, P3). Find the roots of two polynomials on two threads. Expect all roots on every call.
- **TP-ANA-29** (Unit, P3). Call the root finder with start points near 0.4 rad and near 2.0 rad, in turn. Expect the same cluster center each time.

## 20. Build, release, and platform

- **TP-REL-01** (Manual, P1). Build the Release app and the disk image from a clean clone. Expect no error.
- **TP-REL-02** (Manual, P1). Start the app from the disk image on another Mac. The build was verified only on one Mac with macOS 27 and Qt 6.12.0.
- **TP-REL-03** (Manual, P2). Build and start the app on macOS 27.0, the oldest version that the fork supports. Expect a start with no error.
- **TP-REL-04** (Manual, P2). Check that each binary file is in Git LFS and that `git lfs status` shows no screenshot, helper script, or test hook.
- **TP-REL-05** (Unit, P1). Check that the test hook is not in `src/context/guicontext.cpp` of a release commit.
- **TP-REL-06** (Manual, P3). Clone the old history and read the notes. Expect a clear message that old clones must clone again.
- **TP-REL-07** (Manual, P3). Run the app with the Sound settings of the system set to a sample rate of 8000, 48000, and 192000 Hz.
- **TP-REL-09** (Unit, P1). Check that the build script downloads over https only and checks a pinned SHA-256 checksum for each of its six libraries. Check that it pins the Python tools and that both scripts use the same Qt version. Check that the source tree has no copy of FreeType, libsamplerate, or toml++.
- **TP-REL-10** (Manual, P1). Change `QT_VERSION` and run `build-macos.sh` on a used build folder. Expect the script to delete the old build folder, and the app to link the new Qt.
- **TP-REL-11** (Unit, P1). Check that the build script always signs with the hardened runtime, the entitlements, and a timestamp. Check that it looks for an identity before the build, and that it stops when it finds none. Check that it has no ad hoc path. Check the entitlements for the microphone and the Qt Quick compiler, and the microphone text in the Info.plist.
- **TP-REL-12** (Manual, P1). Run `SIGN_IDENTITY` with a Developer ID and `NOTARY_PROFILE`. Expect the status Accepted from Apple, a stapled ticket, and a start of the app on another Mac with no warning of Gatekeeper.
- **TP-REL-13** (App, P2). Check that the bundle has a valid signature. Start the signed app. Expect a start with no QML error and a quit at SIGTERM. The build script signs each build, so the bundle always has a signature.
- **TP-REL-08** (Unit, P1). Check that the code of the app has no condition for another system, no choice of audio backend, and no optional library. Check that the build file stops on any other system.

## 21. Load and long sessions

- **TP-PERF-01** (Manual, P2). Run live capture for 2 hours. Expect stable memory and a CPU use of 16 to 23 percent of one core.
- **TP-PERF-02** (Manual, P2). Load a file of 10 minutes and a file of 2 hours. Expect the map and the view to stay responsive.
- **TP-PERF-03** (Manual, P2). Check the frame time with the bar, the boxes, the zones, and the minimap all on.
- **TP-PERF-04** (Manual, P3). Check the energy use on battery with the window in the background.

## 22. Things that no test can cover

These items need a person.

- Whether the synthesizer sounds natural to a listener.
- Whether the colors look good to the user. The user approved the palette in one session.
- Whether the help pages are clear for a first-time reader.
- Whether the new flipped direction of time feels right to a person who reads the graph every day.
- Whether the purple fill of the chosen button is obvious to a person with low vision or a color deficiency.
- **Some track colors are close to their own map.** The test TP-COL-04 sets a floor of 0.025 in OKLab. The themes Gay MLM (0.030), Rainbow (0.035), and Ember (0.039) are close to that floor. The dark outline of the tracks keeps them visible.

## 23. Code that needs a change before a test can reach it

Some cases need a small change in the code. Do these changes first, because they make many cases cheap.

- **A free function for the time mapping.** Done. `plotTimeToX` and `plotXToTime` in `src/gui/plotmap.h` take the graph, the time range, and the flip. TP-FLIP-07 uses them.
- **A `Config` on a table in memory.** Done. The constructor `Config(toml::table)` never reads or writes the file. TP-RST-02 to TP-RST-04, TP-SET-16, and TP-SET-17 use it.
- **The layout of the graph without a window.** Done for the layout and for the grid lines. `computeGraphLayout` in `src/gui/graphlayout.h` takes a text measure, and `makeGridLines` in `src/gui/gridlines.h` returns the rows. The cases TP-LAY and TP-GRID-08 to TP-GRID-09 use them.
- **Color and grid rules without a window.** Done. `gridLineColor` in `src/gui/gridcolor.h` returns the grid color, and `defaultColorMap` lives in `src/gui/cmap.cpp`. The target `color_tests` builds the themes and tests them without Qt Quick. TP-COL, TP-GRD-11, and TP-QUI use them.
- **The map rules without a window.** Done. `src/gui/maplayout.h` holds the clock, the loudness scale, the pitch scale, and the gap rule of the map. A fake clock replaces the system clock in TP-MAP-26.
- **The audio streams without a sound device.** Done. `AudioContext` takes a backend from the caller, and `src/context/audiocontext_streams.cpp` holds the code that does not depend on a backend. TP-CAP-15 uses a stand-in backend.
- **The playback source and the preset table without a window.** Done. `src/context/playbackfeed.h` chooses the source of the playback stream, and `src/context/vowelpresets.h` holds the vowel table that the synthesizer window shows. TP-SYN-21 and TP-SYN-22 use them.
- **A hook for tests that stays out of release builds.** The project used a temporary hook in `src/context/guicontext.cpp` for each screenshot. A test-only target with its own entry point replaces it, so no test depends on a hook that must never be committed.
- **Names for objects in QML.** Give an `objectName` to each control that a test must press (the reset button, the reset dialog, the segmented control, and the tabs).
- Whether the warm-up ranks are right for a given voice.

## 27. Start and quit of the application

- **TP-APP-01** (App, P1). Start the app from the root folder with no hook. Expect the window to stay open for 8 s, and no QML error in the log. Expect the log to show that the start got past the capture step, with no wait for a system question. Expect a quit within 15 s at SIGTERM.

## 26. The log file

- **TP-LOG-01** (Unit, P1). The log file is `~/Library/Logs/InFormant/InFormant.log`, and the file of the last run is `InFormant.1.log`.
- **TP-LOG-02** (Unit, P2). A file over 5 MB moves to the second file when the program starts.
- **TP-LOG-03** (Unit, P2). Each line gets one stamp. A line can span two chunks of text, and an open line waits for its end.
- **TP-LOG-04** (Unit, P2). A line longer than 4096 bytes breaks into two lines and loses no byte.
- **TP-LOG-05** (App, P1). Start the app with the root folder as the working folder, as Finder does. Expect new lines with stamps in `~/Library/Logs/InFormant/InFormant.log`. Expect no file in the folder `/`.
- **TP-LOG-06** (Unit, P2). Give the log writer text that ends with no end of line. Expect the finish call to write the text as one line with one stamp, and a second call to write nothing.

## 25. The oscilloscope

- **TP-SCP-01** (Unit, P2). Compute the span for 30 ms at 32000 Hz, for 0 ms, for 5000 ms, and for a time that is not a number. Expect a view of 960 samples, then 32 samples, then half of the ring, then 32 samples. Expect no negative search distance.
- **TP-SCP-02** (Unit, P2). Give the trace fewer samples than it needs. Expect no trace.
- **TP-SCP-03** (Unit, P1). Give the trace a steady tone. Expect a start at a rising zero crossing, and the same picture for two frames that differ by a part of a period.
- **TP-SCP-04** (Unit, P2). Give the trace a quiet tone with an offset. Expect no offset and a peak of 1.
- **TP-SCP-05** (Unit, P2). Give the trace silence. Expect a flat line and no value that is not a number.
- **TP-SCP-06** (Image, P1). Start the app and open the Oscilloscope window before the first press of play. Expect the axes and the text "Press play to see the sound of the microphone."
- **TP-SCP-07** (Image, P1). Load an audio file and open the Oscilloscope window. Expect the sound at the end of the view of the main graph, and the title with its time. Pick a moment with a click, and expect the sound at that moment.
- **TP-SCP-08** (Image, P2). Press play, and open the Oscilloscope window. Expect the live sound and the glottal flow estimate.

## 24. Bugs found by the unit tests

Each entry names the case, the bug, the version of the fix, and the test that guards it.

- **A file with a syntax error stopped the program at launch (3.2.2).** The parser threw an error that no code caught, so the user could not start the program until the file was removed. Now the program starts with the defaults and keeps the broken file as `informant.conf.broken`. TP-SET-08 and TP-SET-18.
- **An export to a folder that cannot be written gave no reason (3.2.3).** The status said "Exported 0 of N sessions". Now the status says that the target folder is not writable and names it. TP-PRA-11.
- **A window that was narrower than the margins gave a graph of negative width (3.2.4).** The margins now leave one pixel for the graph, and the boxes go away. TP-LAY-16.
- **A click on the first pixel of the margin counted as a click on the graph (3.2.4).** The test of the graph now ends before the last pixel row and the last pixel column. TP-LAY-19, TP-LAY-20, and TP-LAY-23.
- **The column N was too narrow for a count above 999999 (3.2.4).** A window of 24 hours has about 8.6 million frames, so the pattern now has 7 digits. TP-LAY-12.
- **Two grid lines on one pixel row looked brighter than one line (3.2.4).** The grid now draws one line for each pixel row, and a major line wins over a minor line. TP-GRID-09.
- **A stream call after a failed restart reached a stopped audio system (3.2.5).** When the restart failed, a later press of play still called the sound backend. Now the call throws an error, the status says that the input device did not start, and the backend sees no call. TP-CAP-15.
- **British spellings in the guides and in two windows (3.2.6).** The design page, the test plan, and the README used "colour", "centred", "grey", and "licence". The Settings window used "labelled" and "neighbouring". Now all use American spelling. TP-DOC-11.
- **The README listed seven help pages, and the app has eight (3.2.6).** The page about the settings had no link. Now the README lists all eight pages. TP-DOC-05.
- **Wrapped text in the Progress and Synthesizer windows had no limit on its width (3.2.6).** In a wide window, a paragraph went over 560 pixels. Now each paragraph has a maximum width of 560 pixels. TP-UI-12.
- **Three links did not work (3.2.6).** The README had a build badge and a Play Store link. Both services no longer exist for this fork. The warm-up page had a link to a removed poster. Now these links are gone. TP-DOC-02.
- **A failed backup of a broken settings file lost the file (3.2.7).** The program wrote the defaults over the only copy. Now the program does not write the settings when the backup fails, and each save goes through a temporary file. TP-SET-18.
- **A failed write of the practice files gave no message (3.2.7).** Now the status says that the practice files were not saved. TP-PRA-04.
- **A note that started with a formula character ran as a formula in a spreadsheet (3.2.7).** Now the CSV file puts a quote before such a note. TP-PRA-04.
- **The map code could loop for ever or divide by zero (3.2.7).** A span that was not finite, a bin width of 0, and a time range of 0 now give a safe result. TP-MAP-02 and TP-LAY-19.
- **The program stopped at launch when no input device existed (3.2.7).** The window now opens with the status "No input device available". TP-CAP-06.
- **The compiler removed the checks for a number that is not a number (3.2.7).** The flag `-ffast-math` makes these checks undefined. The flag `-fno-finite-math-only` now keeps them. TP-PRA-37.
- **The oscilloscope showed nothing after the start and with a loaded file (3.2.8).** The plot had data only during a live capture, and it drew no axes and no text. Now the plot shows the axes and a text before the first press of play. When the capture is paused or a file is loaded, it shows the stored sound. The sound is at the picked moment, or at the end of the view. TP-SCP-06 and TP-SCP-07.
- **A change of the audio backend could free the audio system while it ran (3.2.9).** The handler replaced the audio context while the synthesizer thread still used it. The program now has one backend, PortAudio, and no such option. TP-REL-08.
- **The play button did nothing in an app that Finder started (3.3.1).** The permission plugin of Qt refused to ask for the microphone. The program then paused again at once, and only a hidden status line told the reason. Now the program asks AVFoundation directly, and a denied answer opens a dialog with a button for the System Settings. The program also opened the capture stream at launch. The system then showed its question at launch, and the program waited for the answer with no window. Now the stream opens after the user allows the microphone. TP-CAP-20, TP-CAP-21, and TP-APP-01.
- **The line of the pitch track broke into pieces in the pitch-only graph (3.4.0).** A line joins two frames only when their jump is below a limit. The limit was a tenth of the height of the graph. The pitch-only axis spans about 12 semitones, so a normal glide of 3 semitones in one frame was above the limit. The track then showed dots only, whatever the style said. Now the limit is the larger of a tenth of the height and the height of 8 semitones. A jump of an octave still breaks the line. TP-PITCH-07.
- **The automatic axis of the pitch-only graph had only two labels (3.4.1).** The logarithmic axis labels powers of ten with the digits 1, 2, 3, and 5. A range under two octaves, for example 95 to 235 Hz, has only the values 100 and 200 in view. Now such a range uses equal steps of 1, 2, or 5 times a power of ten at the positions of the log axis. A wider range keeps the old labels. TP-AXT-09.
- **The libraries are new (3.4.1).** FreeType is 2.15.0 (was 2.10.4), libsamplerate is 0.2.2 (was 0.1.9), Eigen is 5.0.1 (was 3.4.0), and toml++ is 3.4.0 (was 2.x). The build script now downloads each one with a pinned checksum, and the source tree holds no copy. TP-REL-09.
- **The picture of a long live view lagged behind the sound next to the axis (3.4.2).** A view of more than about 35 seconds draws a picture that a thread makes. The program asked for a new picture every 400 milliseconds, so the picture ended up to 0.34 seconds before the newest sound. In a view of 60 seconds, that was an empty strip of up to 8 pixels. Now the program asks every 100 milliseconds, and the newest column fills a lag of up to 0.25 seconds. A view of a shorter span had a smaller gap of up to 0.03 seconds. It now uses the same rule. TP-EDGE-05, TP-EDGE-06.
- **The timeline of a practice session could have a thirteenth part of no length (3.4.2).** The loop added the step to a start time with a fraction, and the rounding error gave one more part. Now each part starts at a multiple of the step. TP-PRA-21.
- **Export all copied nothing for a session that the person renamed (3.4.2).** The export built the folder from the date and the id, and not from the folder that the list found. Now it copies the listed folder. TP-PRA-11.
- **Delete accepted a folder such as backup/11/13/id (3.4.2).** The check counted four parts of the path only. Now the year has four digits, and the month and the day have two digits. TP-PRG-14.
- **The script for the build did not build FFTW or PortAudio again after a change of version (3.4.2).** It looked for the library file only. Now each of the six libraries has a stamp with its version. TP-REL-09.
- **The log lost its last text in some cases (3.4.2).** The process of the log stopped at an interrupted read, and it dropped a last line with no end of line. Now it reads again after an interrupt, and it writes the last line. TP-LOG-06.
- **A label size of 0 could make the linear axis loop for ever (3.4.2).** Now a label has at least one pixel. TP-AXT-10.
- **The answer to the microphone question used a guarded pointer on a thread of the system (3.4.2).** Now the call posts the work to the application object, and the Qt thread reads the pointer. No test can reach this code. The change has a review only.
- **A full disk could replace the settings file with a cut file (3.4.2).** The program checked the stream before the close, and the close writes the rest of the buffer. Now it checks after the close. No test can reach this code. The change has a review only.
- **FFTW is 3.3.11 (3.4.2).** The version was 3.3.10. The checksum of the new file matches the checksum in the data of Homebrew. The file `.gitmodules` is gone, because the program has no submodule. TP-REL-09.
- **A cached download stopped the build after a change of version (3.4.2).** The cache file had no version in its name, so the old file met the new checksum and failed. Now each cache file has the version in its name. TP-REL-09.
- **The end of the live graph lagged more after each pause (3.4.3).** The view followed the wall clock, and the picture holds only the sound that the program stored. A pause, a restart of the input, or a stall each left a lead of about 0.17 seconds for ever. An empty strip then grew next to the axis. Now the view follows the stored sound, plus the wall time since the last step, up to 0.1 seconds. The strip stays below 0.06 seconds after three pauses. TP-EDGE-07, TP-EDGE-08, TP-EDGE-09, TP-EDGE-10.
- **The time of the slices could run ahead of the stored sound (3.4.3).** The pipeline added the granularity to the time of each block, but a block has a whole number of samples. A rate of 22.05 kHz with 10 milliseconds gives a block of 220 samples, half a sample short. Now the time adds the samples of the block divided by the rate. TP-EDGE-07.
- **YIN used the wrong difference function (3.4.4).** It computed `r(0) + r(1) - 2 r(tau)` from the autocorrelation of the whole frame. The correct difference is `e(0..W) + e(tau..tau+W) - 2 r_W(tau)` over a fixed window. A signal with a rising level or a low pitch then moved the minimum. YIN also gave whole lags only, so 440 Hz at 16 kHz was 1 percent high. Now YIN uses a cumulative sum of squares for the energy terms and the FFT for the cross term, and it interpolates the lag. TP-ANA-07 to TP-ANA-10.
- **Solvers shared buffers, an FFT, filters, and windows through `static` variables (3.4.4).** MPM, IAIF, GFM-IAIF, covariance LPC, and RAPT kept them in functions. Two solvers or two threads shared them. IAIF and GFM-IAIF also kept the high-pass filter of the first sample rate for ever. RAPT kept the LPC order of the first sample rate. Now each solver holds its own state, and the filter, the window, and the order follow the sample rate. TP-ANA-14 to TP-ANA-17, TP-ANA-19, TP-ANA-21, TP-ANA-23, TP-ANA-25, and TP-ANA-26.
- **The Hann window of IAIF and GFM-IAIF divided by the frame length (3.4.4).** The window has `lpW` points, so it did not reach 0 at its end. Now it divides by `lpW - 1`. TP-ANA-20 and TP-ANA-24.
- **The root finder kept the start values of its first call, and all threads shared its random engine (3.4.4).** The static distributions took their limits from the first polynomial. Now each call builds its own distributions, and each thread has its own engine. TP-ANA-27 to TP-ANA-29.
