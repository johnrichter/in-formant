# Read the display

This page explains what you see in InFormant. The other pages use these terms.

## What the graph shows

- **Spectrogram.** Frequency goes from bottom to top. The color shows the loudness at that time and frequency. By default, the newest sound is at the left edge, and older sound moves to the right.
- **Pitch track.** The dots and the line show the fundamental frequency (F0). F0 is the rate at which the vocal folds open and close. You hear F0 as the pitch of the voice.
- **Formant tracks.** F1, F2, F3 and the higher formants are resonances of the vocal tract. The vocal tract is the space from the vocal folds to the lips. The shape of this space sets the formants.
- **Axes.** Both axes lie outside the graph. By default, new sound appears at the left edge, next to the frequency axis, from the first moment of a capture. Old sound flows to the right. The part of the graph before the start of the capture is empty. To stop this, clear the switch "Newest sound at the left" in the Axes tab. The graph then fills from the left to the right, and time zero is at the lower left.
- **Frequency labels.** The automatic mode labels only round values, all with one text size. The setting "Number of labels" chooses Few (100, 200, 500), Normal (100, 200, 300, 500), or Many (100, 150, 200, 300, 400, 500). A linear axis uses equal steps.
- **Time labels.** The labels show whole seconds, and the smallest step is 1 second. A view of 15 seconds has a label each second. A view of 1 minute has a label every 5 seconds, and a view of 2 minutes every 10 seconds. A longer view has larger steps.
- **Grid lines.** Two switches in the Axes tab draw thin horizontal lines across the graph. The major lines are at the numbers of the frequency axis. The minor lines are at the ticks between the numbers. Both are off by default. A slider sets the strength of the lines. The line color comes from the quiet color of the theme, so it fits every theme.

## What a formant tells you

A formant is a frequency where the vocal tract amplifies the sound. Each formant follows a movement of the mouth:

- **F1** goes up when the jaw opens and the tongue goes down. Say "ee", then "ah". F1 goes up.
- **F2** goes up when the tongue moves to the front. Say "oo", then "ee". F2 goes up.
- **F3** goes up when the lips spread, and it goes down when the lips round.
- **F4** goes up when the vocal tract is shorter.

The Display tab has one switch for each formant track (F1 to F4). Clear the switch of a track that you do not need. The box of that track in the readouts has its own switch in the Readouts tab.

A shorter vocal tract gives higher formants. A longer vocal tract gives lower formants. This is the main reason why many voices of men and women sound different. The page "The math" gives the formula.

## Pick a moment

Click on the graph, or drag across it. A bar shows the moment from the top to the bottom of the graph. A label next to the bar gives the time. A label with a dot on the bar gives the exact value of the pitch and of each formant at that moment. The color of each label is the color of its track.

- A double click or a right click removes the bar.
- The labels go to the side of the bar that has room.
- A track that has no value at that moment has no label.

## Read the statistics

The statistics show in boxes. They are on by default. Each track has its own box with the name of the track, the live value, and one line for each statistic. A box keeps its size when the numbers change.

The Readouts tab of the settings has these choices:

- The place of the boxes: at the left of the graph (the default), or below the graph. A column at the left wraps into a second column when it is too high.
- The order of the boxes, and a switch for each box.
- The statistics that have a line in the boxes.

At the left of the graph, the column has the same order as the tracks in the graph. The highest formant (F4) is at the top, and the pitch is at the bottom.

A bar above the graph holds the legend and the frame times. The two boxes stand side by side at the center of the bar. The bar keeps its height during a recording, so the graph does not move. The app hides the frame times during a recording.

| Line | Meaning |
| --- | --- |
| Now | The newest value. It is in the header of the box. |
| Min, Max | The smallest and the largest value. |
| P5, P95 | The value that 5% or 95% of the values do not exceed. |
| Mean, Median | The average value, and the middle value. |
| SD | The standard deviation. It shows how much the value changes. |
| Range | P95 minus P5. It ignores a few extreme values. A switch in the Readouts tab shows it in semitones (st). |
| CV | SD divided by the mean, in percent. |
| Voiced | The share of frames that have a value. |
| N | The number of values. |
| Note | The musical note of the mean pitch, and the error in cents. |

### What SD and CV tell you

SD is the standard deviation. It measures how far the values spread around the mean. SD has the unit of the track, which is hertz for pitch and for each formant.

CV is the coefficient of variation. CV is SD divided by the mean, in percent. CV has no unit, so you can compare tracks that have different means.

- A low SD or CV means that the value stays close to the mean. The sound is steady.
- A high SD or CV means that the value moves a lot.
- A pitch SD of 20 Hz with a mean of 200 Hz gives a CV of 10%.
- The same SD of 20 Hz with a mean of 100 Hz gives a CV of 20%. The same step in hertz is a larger step for a low voice.

How to use the two values:

- **Pitch.** The ear hears pitch changes as ratios. CV compares the movement of a low voice and a high voice better than SD does. A higher CV means more intonation.
- **Formants.** In a sustained vowel, a low SD means a stable formant. A high SD means that the vowel moved, or that the tracker made errors.
- **Errors.** One jump of the tracker to a wrong octave raises SD and CV a lot. Look at the track before you trust a high value.

The report that Record writes uses the same definition as the table. Both divide the sum of squares by N - 1.

## Move in time

- The map below the graph shows the loudness and the pitch of the session. It shows 2.5 minutes at most. A shorter session shows whole.
- Click or drag in the map to go to another time.
- Drag the left or the right edge of the frame to change the time that the graph shows. The View field shows the new time.
- For a longer session, a strip under the map scrolls the map. A sideways scroll over the map does the same.
- The mouse wheel moves the graph in time.
- Live returns to the newest sound.
- Play window plays the sound of the graph. Export window and Export all save the sound as a WAV file.
- Open file analyzes an audio file.
- Record saves a video, the sound, and a report with the statistics.

## Pitch zones

The Display tab has a switch for pitch zones. The zones are three colored bands behind the tracks:

- **Masculine** is blue. The default range is 60 to 150 Hz.
- **Androgynous** is yellow-green. The default range is 150 to 180 Hz.
- **Feminine** is pink. The default range is 180 to 300 Hz.

A text field sets the four edges. The bands follow the frequency scale. A thin band has no label.

The zones show ranges of pitch that many listeners hear in a certain way. This is a rough stereotype. It differs between languages, voices, and listeners. A zone is not a goal.

The recorder grabs the whole window, so a video shows the zones. To remove the zones from a video, clear the switch "Keep the zones in recordings".

## Pitch-only graph

The two buttons "Full" and "Pitch only" in the toolbar set what the graph shows. The pitch-only mode hides the spectrogram and the formant tracks. It draws the pitch track larger.

- The vertical axis goes from 0 to 500 Hz. The Display tab sets the two edges.
- A switch fits the axis to the pitch in view instead. This uses a logarithmic scale and a minimum range of 12 semitones.
- The pitch zones show in this mode too.

Use this mode for a pitch slide, for a warm-up, and for a talk where you only watch the pitch.

## Colors

The Theme tab sets the colors of the spectrogram, the tracks, and the plots.

- A color map changes from near black to a light tint. The colors change in a way that looks even, so no gray patch shows between two colors.
- The saturation slider makes the colors weaker or stronger.
- The brightness has its own slider. It is the Gain normalization slider in the Display tab. A loud sound must not fill the map with its last color.
- Each track has a dark outline, so it stays visible on every color.
- The switch "Minimum volume is black" sets the quiet end. When it is off (the default), the quietest sound has a dark tint of the theme, as in the Viridis theme. A quiet sound then stands out from the black window. When it is on, the quietest sound is black.

## Help in the Settings window

A "?" button next to a setting opens a box with a longer explanation. The box scrolls when the text is long. A press outside the box, or the Escape key, closes it. The short hint under the setting stays. The page "Settings" in the Help window explains the frequency scale, the FFT size, and the four methods of the Input tab for each goal.

## Clear the graph

The red trash icon, next to the play button, is the button "Reset graph". It deletes the tracks and the recording of the capture and starts again at time zero. A capture of more than 10 seconds asks first. The button does nothing during a recording or a practice session. The settings and the saved files stay.

## Practice mode and progress

The Practice button below the graph starts a session on the live capture. Choose the task in the list next to the button: Free, Vowels, Reading, or Conversation.

- The M key, or the Mark button, saves a mark. A mark has the time, a note if you type one, and the pitch and F1 to F4 of the last half second.
- Click Stop practice to end the session. A window asks if you want to keep it. It shows the length, the marks, and a comparison with your last session of the same task. Keep saves the session. Discard saves nothing.
- The app discards a session that is shorter than the minimum time, with no question. The minimum is 5 seconds by default, and the General tab of the settings changes it.
- A folder for the session holds the statistics, a report, the marks, and the sound. The General tab of the settings sets the folder. The default is Documents/InFormant/Practice.
- The Progress button opens a chart of your change. Choose a measure, a task, and a period. Compare like with like. A formant median over mixed tasks says little.

## The synthesizer

The Synthesizer window makes a steady test voice. It starts in constant mode, so its sound does not change by itself.

- The box "Quick start" sets F1 to F4 and the pitch for ten vowels, for a lower voice or for a higher voice. The page "Synthesizer" in the help shows how to use it, with recipes.
- Sliders and number boxes set the pitch, F1 to F4, the bandwidths, and the amount of pitch jitter.
- The switches "Follow pitch estimates" and "Follow formant estimates" make the synthesizer follow your voice. Use headphones when you use these switches. Without headphones, the microphone hears the synthesizer, and the sound changes by itself.

## Settings that change what you see

- **View time.** A short view shows fast changes. A long view shows a trend.
- **Frequency scale.** The log, mel and ERB scales give more room to low frequencies. The ear works the same way.
- **View frequency range.** This range only sets what you see. It does not change the formant tracks.
- **Formant limit.** The formant tracker uses its own limit. The default is 5200 Hz. Phoneticians often use 5000 Hz for male voices and 5500 Hz for female voices. The `maxFrequency` value in the `[analysis]` table of the configuration file sets this limit.
- **FFT size.** A larger size separates close harmonics better. It shows fast changes less well.
- **Gain normalization.** This slider sets the brightness of the spectrogram.

## Return to the defaults

The Settings window saves its values when you close it. The General tab has the button "Reset all settings to defaults". A dialog asks you to confirm. You cannot undo the reset. It returns every setting to its default value.

The folder of the practice sessions stays. Your sessions, recordings, and files stay.

## Know the limits

The tracks are estimates. Check them against the spectrogram.

- The pitch track can jump by an octave. It can lose the pitch in a creaky voice or in a whisper.
- The formant tracks are less reliable when the pitch is high. At a high pitch the harmonics are far apart, and the tracker has little data between them. Above about 400 Hz, trust the spectrogram more than the formant lines.
- Noise, echo, and a second voice make all tracks worse. Use a quiet room and keep the microphone at a fixed distance.
- InFormant does not tell you how a voice sounds to a listener. Use the numbers as a guide, and ask listeners that you trust.
