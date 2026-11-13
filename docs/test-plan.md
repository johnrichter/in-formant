# Test plan: use cases for later tests

This page lists every use case that the project did not test during development. Each case is a test to write later. The list is long on purpose.

## How to read this page

- Each case has an ID, a short setup, and the result that a test must check.
- The type tells how to automate the case. **Unit** is a plain C++ test in `tests/`. **Qt** is a Qt Quick Test or a Qt Test with a window. **Image** compares a screenshot with a stored picture. **Audio** analyses rendered samples. **Manual** needs a person or a device.
- The priority is **P1** (a failure harms a user), **P2** (a failure is visible), or **P3** (a small risk).
- A case that says "not built yet" tests a feature that does not exist. Write the test when the feature exists.

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
- **The warm-up ranks are a judgement from search summaries.** TP-DOC-07.
- **The formant tracker is not reliable above about 400 Hz.** TP-ANA-01.
- **The Qt 6.8.3 build was verified on one Mac.** TP-REL-02, TP-REL-03.
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
- **The old setting `viewShowFormants` stays in the code.** TP-TRACK-03.
- **The reset button got one script test, with one set of odd values.** TP-RST-01 to TP-RST-12.
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

## 2. Playback of a session

- **TP-PLY-01** (Audio, P1). Play a loaded file. Expect the sound to match the file and to stay in time with the cursor.
- **TP-PLY-02** (Audio, P2). Play a selected window. Expect the sound to start and stop at the window edges.
- **TP-PLY-03** (Qt, P2). Press Play while capture runs. Expect capture to pause or to continue as the design says, and expect no echo.
- **TP-PLY-04** (Audio, P3). Play a file with a sample rate that differs from the output device. Expect the right speed.
- **TP-PLY-05** (Manual, P2). Change the output device while the sound plays. Expect no crash.

## 3. Layout of the bar and the boxes

- **TP-LAY-01** (Unit, P1). Compute the layout with the legend and the frame times on. Expect the two boxes side by side and centred over the graph.
- **TP-LAY-02** (Unit, P1). Compute the layout with the frame times on and `hideDebug` on (a recording). Expect the same top margin and the same graph size.
- **TP-LAY-03** (Unit, P2). Compute the layout with both boxes off. Expect a top margin of zero.
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
- **TP-MAP-15** (Image, P3). Check the vertical centre of the text in the field "View".

## 6. The right edge and the live spectrogram

- **TP-EDGE-01** (Image, P1). Capture the right 100 pixels at FFT sizes 512, 1024, 2048, and 4096 in live mode. Expect no black bar.
- **TP-EDGE-02** (Image, P2). Repeat in review mode past the end of the session.
- **TP-EDGE-03** (Image, P2). Repeat with a session shorter than the view.
- **TP-EDGE-04** (Image, P2). Repeat while the canvas width changes.

## 6b. Time direction, grid lines, and track switches

- **TP-FLIP-01** (Image, P1). Load a file and capture the graph with the default settings. Expect time 0 at the right edge and the newest time at the left edge.
- **TP-FLIP-02** (Image, P1). Turn off "Newest sound at the left". Expect time 0 at the left edge.
- **TP-FLIP-03** (Qt, P1). In live mode with the flip on, speak. Expect the new sound at the left edge and the old sound to move to the right, with no black bar at the left edge.
- **TP-FLIP-04** (Qt, P1). Click in the graph in both directions. Expect the cursor line and the time of the cursor to match the click.
- **TP-FLIP-05** (Qt, P2). Scroll the wheel and the trackpad sideways in both directions. Expect the content to follow the fingers.
- **TP-FLIP-06** (Image, P2). Check the pitch-only graph and a recording in both directions.
- **TP-FLIP-07** (Unit, P1). A test cannot reach the mapping yet, because it is a member of the painter. Move it to a free function. Then test that `timeAtX(mapTimeToX(t))` returns `t` in both directions. Use widths of 1, 2, and 4000 pixels.
- **TP-FLIP-08** (Image, P2). Compare the session map with the graph. The map keeps old sound at the left. Decide with the user whether the map must flip too, then test the choice.
- **TP-FLIP-09** (Image, P1). In live mode with the flip on, check the left 100 pixels at FFT sizes 512, 1024, 2048, and 4096. Expect no black bar, as in TP-EDGE-01.
- **TP-FLIP-10** (Image, P2). Check the labels of the cursor and the boxes of the values. Expect each label to stay inside the graph when the cursor is at the left edge and at the right edge, in both directions.
- **TP-FLIP-11** (Qt, P2). Play a window with the flip on. Expect the playback line to move from the right to the left, and expect the sound to match the line.
- **TP-FLIP-12** (Qt, P2). Drag in the graph from the left edge to the right edge in both directions. Expect the cursor to follow the pointer with no jump.
- **TP-FLIP-13** (Unit, P2). Check the time ticks with a decreasing pixel mapping. Expect the same labels as the increasing mapping, mirrored, and no overlap.
- **TP-FLIP-14** (Qt, P2). Change the switch "Newest sound at the left" while capture runs. Expect the picture to redraw at once with no stale column and no crash.
- **TP-FLIP-15** (Manual, P3). Check the wheel with the system setting "Natural scrolling" on and off. Expect the content to follow the fingers in both cases.
- **TP-FLIP-16** (Unit, P2). Check that an old file with no `flipTime` value starts with the flip on, and that a stored `false` stays.
- **TP-GRID-01** (Image, P1). Set each grid switch alone, then both. Expect lines at the labelled ticks for the major switch and at the other ticks for the minor switch.
- **TP-GRID-02** (Image, P2). Change the frequency scale (linear, logarithmic, Mel, ERB) and the tick mode. Expect each line to stay on its tick.
- **TP-GRID-03** (Image, P2). Check that the lines stay behind the tracks and that the lines are not in the minimap.
- **TP-GRID-04** (Image, P2). Check the brightness of the grid lines on each of the 16 pride themes and on Magma, with a loud sound and with silence. Expect the lines to show, and expect them not to hide the spectrogram.
- **TP-GRID-05** (Image, P2). Check the grid lines at device pixel ratios 1 and 2. Expect a line of about the same visual width.
- **TP-GRID-06** (Image, P2). Check the grid lines in the pitch-only graph with the fixed axis and with the automatic fit. Expect each line at a tick of the axis.
- **TP-GRID-07** (Qt, P2). Record with the grid on. Extract a frame from the video. Expect the lines in the frame.
- **TP-GRID-08** (Unit, P2). Test the grid with a custom tick step, with the note mode, and with the octave mode. Expect the major lines at the labelled ticks only.
- **TP-GRID-09** (Unit, P3). Test a graph so short that the ticks are closer than 2 pixels. Expect no more than one line for each pixel row.
- **TP-TRACK-01** (Qt, P1). Turn off each formant switch in turn. Expect only that track, its box, and its legend entry to go.
- **TP-TRACK-02** (Unit, P1). Migrate a file with the old switch off. Expect all four new switches off, and a second run to change nothing.
- **TP-TRACK-03** (Unit, P3). Remove the old setting `viewShowFormants` and its property in `config.h`, or test that nothing reads it.
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
- **TP-GRD-10** (Image, P1). Check the grid lines on every theme at strengths of 10, 60, and 100 percent. Expect lines that show, and expect no line to hide the loud sound.
- **TP-GRD-11** (Unit, P2). Check the grid colour for a dark quiet colour and a light quiet colour. Expect a light tint for the first and a dark tint for the second.
- **TP-QUI-01** (Unit, P1). Lift the quiet end of a map that starts at black. Expect a lightness of 0.22 to 0.32 at the first entry. Expect the same colours above 45 percent of the map, and no fall of the lightness. The unit test `oklab` has this.
- **TP-QUI-02** (Unit, P2). Lift the quiet end of Viridis and of a grey map. Expect no change for Viridis and no chroma for the grey map.
- **TP-QUI-03** (Image, P1). Screenshot each of the 22 themes with the switch on and off. Expect the quiet parts to differ from the black window when the switch is off.
- **TP-QUI-04** (Image, P2). Check the graph before the first sound arrives and in the empty part of a flipped graph. Expect the quiet colour and no black block.
- **TP-QUI-05** (Qt, P2). Turn the switch on and off while capture runs. Expect the strip in the Theme tab, the graph, the map, and the plots to follow at once.
- **TP-LFT-01** (Qt, P1). Start a live capture with the flip on. Expect the new sound at the left edge from the first second, and the old sound to flow to the right.
- **TP-LFT-02** (Qt, P2). Check the empty part at the right of a young capture. Expect the quiet colour and no tick before time zero.
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
- **TP-TYP-01** (Image, P1). Check each Help page at widths of 480, 760, and 1400 pixels. Expect a column of at most 640 pixels, centred, with no sideways scroll.
- **TP-TYP-02** (Unit, P1). Check the prose scale. Lines and gaps must be on the 4-pixel grid. A body line must be at least 1.6 times the size. A heading must have more space above it than below it. The unit test `ui_scale` has this.
- **TP-TYP-03** (Image, P2). Check a table, a code block, a list, a block quote, a link, and bold and code text in one paragraph. Expect the right size, colour, and gap for each.
- **TP-TYP-04** (Qt, P3). Select text with the mouse and copy it. Expect the text without style marks.
- **TP-TYP-05** (Manual, P2). Read each page for five minutes. Compare the comfort with the old pages.
- **TP-GDE-01** (Manual, P2). A speech-language pathologist reads the page "Settings". Check each claim about a method, and check the advice for each goal.
- **TP-GDE-02** (Unit, P2). Check the numbers of the FFT table on the page (window time and line distance) against the formula for a view up to 5500 Hz.
- **TP-STP-01** (Manual, P1). Measure the time from the start of the app to the line "Input device" in the log. The time must be under 3 seconds on each input device. A first build of the help styling took 12 seconds, and one edit block fixed it. The audio device was not the cause.

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

## 9. Pitch zones

- **TP-ZONE-01** (Image, P2). Draw the zones on the linear, logarithmic, Mel, and ERB scales. Expect the edges at the right heights on each scale. The Mel scale was never captured.
- **TP-ZONE-02** (Unit, P2). Test edge values that are not in order, equal, zero, negative, or not numbers. Expect a safe fallback.
- **TP-ZONE-03** (Qt, P2). Record with the zones on and "Keep the zones in recordings" on. Extract a frame from the saved video. Expect the zones to show.
- **TP-ZONE-04** (Qt, P2). Record with "Keep the zones in recordings" off. Extract a frame. Expect no zones, and expect the zones to come back on the screen after the recording.
- **TP-ZONE-05** (Image, P3). Check the labels of the zones at a small window height.
- **TP-ZONE-06** (Unit, P3). Check that the zone colours differ for a person with red-green colour blindness (simulate it).

## 10. Colour themes

- **TP-COL-01** (Unit, P2). For each of the 16 pride themes, check that the lightness rises in order and that no stop is white.
- **TP-COL-02** (Unit, P1). For "Trans (Dark)", check the top three stops. Expect an OKLab chroma of at least 0.12 after gamut mapping, and a hue between 340 and 360 degrees.
- **TP-COL-03** (Unit, P2). Check that the colour table of each theme has 256 entries and that each entry is in the sRGB gamut.
- **TP-COL-04** (Unit, P2). Check the OKLab distance between each track colour and every entry of its table. Expect a minimum above a set value.
- **TP-COL-05** (Image, P2). Screenshot each of the 16 pride themes with a real speech file. Only six themes and a strip picture of the others were checked.
- **TP-COL-06** (Unit, P2). Change the saturation from 50 to 150 percent. Expect the chroma to scale and the lightness to stay.
- **TP-COL-07** (Qt, P2). Move the Saturation slider. Expect the stored value to change in steps of 5 percent.
- **TP-COL-08** (Image, P2). Check that the 16 dB headroom does not make the default Magma theme too dark. Compare with a stored picture.
- **TP-COL-09** (Unit, P3). Parse a custom theme with a wrong colour, a missing colour, and a very long list. Expect a safe fallback.
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
- **TP-SET-14** (Qt, P3). Check the grey hint under each control of each tab. Expect the text to wrap, to stay inside the window, and not to overlap.
- **TP-SET-16** (Unit, P2). Check that every setter in `config.cpp` works when the file has no key for it. A setter crashed the app once for this reason (`showFormants`).
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
- **TP-UI-04** (Unit, P2). Check the contrast of the grey hint colour against the dark background. Expect at least 4.5 to 1.
- **TP-UI-05** (Qt, P3). Resize each window to its minimum size. Expect no overlap and a working scrollbar.
- **TP-UI-06** (Manual, P3). Use a large system font. Expect the layout to hold.
- **TP-UI-07** (Qt, P3). Check the keyboard focus order in each window and each dialog.
- **TP-UI-08** (Manual, P3). Check each control with VoiceOver. Expect a useful name for each control.
- **TP-FONT-01** (Qt, P2). Check that the families "Roboto" and "Roboto Mono" load from the resources on a Mac with no Roboto fonts installed.
- **TP-FONT-02** (Image, P2). Check that a heading shows in bold and a field label shows in medium weight.
- **TP-FONT-03** (Image, P2). Check that the digits in the boxes of the tracks, in the frame times, and on the axes keep their width when the values change.
- **TP-FONT-04** (Unit, P3). Check that the frame-times text has the same width for 0.00 ms and for 99.99 ms.
- **TP-FONT-05** (Image, P3). Check the text of the graph at device pixel ratios 1 and 2, and at the zoom steps of the toolbar.
- **TP-FONT-06** (Unit, P3). Check that each font file is in Git LFS and that each licence file is in `resources`.
- **TP-UI-15** (Image, P2). Check the gap between each switch and its hint. Expect at least 8 pixels from the visual bottom of the switch, in the Settings, Record, and Synthesizer windows.
- **TP-UI-10** (Qt, P2). Check that each heading uses `Heading`, each hint uses `Hint`, and no window has a separator line between groups.
- **TP-UI-11** (Image, P2). Check the spacing of one window with an overlay of the 4-pixel grid. Expect each gap to be a step of the scale.
- **TP-UI-12** (Unit, P3). Check that no paragraph in a window is wider than 560 pixels.
- **TP-UI-13** (Unit, P2). Check the contrast of the three text colours against the window and the surface. The unit test `ui_scale` does this. Repeat it when the Material theme changes.
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
- **TP-PRA-10** (Unit, P2). Load a library of 5000 sessions. Expect the Progress window to open in under 2 seconds.
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
- **TP-PRG-10** (Qt, P2). Check the colour of the chart for the pitch and for each formant.
- **TP-PRG-11** (Image, P3). Check the chart with 1 point, 2 points, 200 points, and all values equal.
- **TP-PRG-12** (Qt, P2). Save a new session while the window is open. Expect the chart to refresh.
- **TP-PRG-13** (Unit, P3). Check the old API (`TrendMeasure`) against the new API (`TrendStat`). Expect the same numbers.

## 16. Statistics and reports

- **TP-STA-01** (Unit, P1). Check that the report and the live table use the standard deviation with N minus 1.
- **TP-STA-02** (Unit, P2). Check the percentiles for 1, 2, 3, and 1000 values, and for equal values.
- **TP-STA-03** (Unit, P2). Check CV, Voiced share, and N for a window with no values.
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

- **TP-DOC-01** (Unit, P2). Run the style check on every file in `docs/` and on the README. Expect no new violation. The README has 13 known violations.
- **TP-DOC-02** (Unit, P2). Check each link in the pages. Expect an answer of 200 for each source URL.
- **TP-DOC-03** (Qt, P2). Open each of the seven tabs. Expect the page text, and expect the tab order Display, Feminization, Masculinization, Singing, Warm-ups, Synthesizer, The math.
- **TP-DOC-04** (Qt, P3). Call `showPage` with a known and an unknown file. Expect the tab to change for the known file and no error for the unknown file.
- **TP-DOC-05** (Unit, P2). Check that each file of `docs/` is in `resources/qml.qrc` and in the list of the Help window.
- **TP-DOC-06** (Unit, P2). Check that the page "Warm-ups and vocal health" has the CAUTION text and the section "Sources".
- **TP-DOC-07** (Manual, P2). A speech-language pathologist reads the page "Warm-ups and vocal health". The ranks are a judgement from search summaries and not from full papers. Check each rank against the full papers.
- **TP-DOC-08** (Unit, P3). Check that the worked examples in the guides (5.0 to 9.7 and 7.0 to 4.4 semitones) match the formula.

## 19. Analysis quality

- **TP-ANA-01** (Audio, P1). Run the formant tracker on synthetic vowels with a pitch of 100 to 1000 Hz. The tracker is not reliable above about 400 Hz. Record the error against the pitch.
- **TP-ANA-02** (Audio, P2). Run the pitch tracker on a signal with an octave jump, on vocal fry, and on whispered speech. Expect the documented behaviour.
- **TP-ANA-03** (Audio, P2). Run each pitch method (YIN, McLeod, RAPT) on the same file. Compare the tracks.
- **TP-ANA-04** (Audio, P2). Run each formant method and each LPC method on the same file. Compare the tracks.
- **TP-ANA-05** (Audio, P2). Test the formant limit of 5200 Hz and the view range of 5500 Hz. Expect the tracker not to follow the view range.
- **TP-ANA-06** (Audio, P3). Test a very quiet file, a clipped file, and a file with a DC offset.

## 20. Build, release, and platform

- **TP-REL-01** (Manual, P1). Build the Release app and the disk image from a clean clone. Expect no error.
- **TP-REL-02** (Manual, P1). Start the app from the disk image on another Mac. The build was verified only on one Mac with macOS 27 and Qt 6.8.3.
- **TP-REL-03** (Manual, P2). Build and start on an Intel Mac and on an older macOS version.
- **TP-REL-04** (Manual, P2). Check that each binary file is in Git LFS and that `git lfs status` shows no screenshot, helper script, or test hook.
- **TP-REL-05** (Unit, P1). Check that the test hook is not in `src/context/guicontext.cpp` of a release commit.
- **TP-REL-06** (Manual, P3). Clone the old history and read the notes. Expect a clear message that old clones must clone again.
- **TP-REL-07** (Manual, P3). Run the app with the Sound settings of the system set to a sample rate of 8000, 48000, and 192000 Hz.

## 21. Load and long sessions

- **TP-PERF-01** (Manual, P2). Run live capture for 2 hours. Expect stable memory and a CPU use of 16 to 23 percent of one core.
- **TP-PERF-02** (Manual, P2). Load a file of 10 minutes and a file of 2 hours. Expect the map and the view to stay responsive.
- **TP-PERF-03** (Manual, P2). Check the frame time with the bar, the boxes, the zones, and the minimap all on.
- **TP-PERF-04** (Manual, P3). Check the energy use on battery with the window in the background.

## 22. Things that no test can cover

These items need a person.

- Whether the synthesizer sounds natural to a listener.
- Whether the colours look good to the user. The user approved the palette in one session.
- Whether the help pages are clear for a first-time reader.
- Whether the new flipped direction of time feels right to a person who reads the graph every day.
- Whether the purple fill of the chosen button is obvious to a person with low vision or a colour deficiency.

## 23. Code that needs a change before a test can reach it

Some cases need a small change in the code. Do these changes first, because they make many cases cheap.

- **A free function for the time mapping.** `mapTimeToX` and `timeAtX` are members of the painter, and they read the options. A free function with the plot size, the time range, and the flip as arguments makes TP-FLIP-07 and TP-FLIP-13 plain unit tests.
- **A `Config` on a table in memory.** The class reads a file in its constructor and writes a file in its destructor. A constructor that takes a table makes TP-RST-02 to TP-RST-04, TP-SET-16, and TP-SET-17 plain unit tests.
- **The layout of the graph without a window.** The functions `updateLayout` and `drawFrequencyGrid` need a renderer. A function that returns the list of lines (a plain list of numbers) makes TP-GRID-01 to TP-GRID-09 unit tests.
- **A hook for tests that stays out of release builds.** The project used a temporary hook in `src/context/guicontext.cpp` for each screenshot. A test-only target with its own entry point replaces it, so no test depends on a hook that must never be committed.
- **Names for objects in QML.** Give an `objectName` to each control that a test must press (the reset button, the reset dialog, the segmented control, and the tabs).
- Whether the warm-up ranks are right for a given voice.
