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
- **TP-GRID-01** (Image, P1). Set each grid switch alone, then both. Expect lines at the labelled ticks for the major switch and at the other ticks for the minor switch.
- **TP-GRID-02** (Image, P2). Change the frequency scale (linear, logarithmic, Mel, ERB) and the tick mode. Expect each line to stay on its tick.
- **TP-GRID-03** (Image, P2). Check that the lines stay behind the tracks and that the lines are not in the minimap.
- **TP-TRACK-01** (Qt, P1). Turn off each formant switch in turn. Expect only that track, its box, and its legend entry to go.
- **TP-TRACK-02** (Unit, P1). Migrate a file with the old switch off. Expect all four new switches off, and a second run to change nothing.
- **TP-TAB-01** (Image, P1). Look at the Full and Pitch only buttons. Expect the chosen button to have a solid fill and the other to have an outline.

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
- **TP-SET-04** (Unit, P1). Migrate a file of each version from 0 to 5. Expect the chain to end at version 6 with the right defaults.
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
- Whether the warm-up ranks are right for a given voice.
