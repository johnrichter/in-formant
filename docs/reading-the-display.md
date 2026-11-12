# Read the display

This page explains what you see in InFormant. The other pages use these terms.

## What the graph shows

- **Spectrogram.** Time goes from left to right. Frequency goes from bottom to top. The colour shows the loudness at that time and frequency.
- **Pitch track.** The dots and the line show the fundamental frequency (F0). F0 is the rate at which the vocal folds open and close. You hear F0 as the pitch of the voice.
- **Formant tracks.** F1, F2, F3 and the higher formants are resonances of the vocal tract. The vocal tract is the space from the vocal folds to the lips. The shape of this space sets the formants.
- **Axes.** Both axes lie outside the graph. Time zero is at the lower left. A new capture fills the graph from the left to the right. Then the graph scrolls.

## What a formant tells you

A formant is a frequency where the vocal tract amplifies the sound. Each formant follows a movement of the mouth:

- **F1** goes up when the jaw opens and the tongue goes down. Say "ee", then "ah". F1 goes up.
- **F2** goes up when the tongue moves to the front. Say "oo", then "ee". F2 goes up.
- **F3** goes up when the lips spread, and it goes down when the lips round.

A shorter vocal tract gives higher formants. A longer vocal tract gives lower formants. This is the main reason why many voices of men and women sound different. The page "The math" gives the formula.

## Pick a moment

Click on the graph, or drag across it. A bar shows the moment from the top to the bottom of the graph. A label next to the bar gives the time. A label with a dot on the bar gives the exact value of the pitch and of each formant at that moment. The colour of each label is the colour of its track.

- A double click or a right click removes the bar.
- The labels go to the side of the bar that has room.
- A track that has no value at that moment has no label.

## Read the statistics

The row below the graph can show a table for the last seconds of the capture. Each row is one track. Each column is one statistic. The Readouts tab of the settings chooses the columns.

| Column | Meaning |
| --- | --- |
| Now | The newest value. |
| Min, Max | The smallest and the largest value. |
| P5, P95 | The value that 5% or 95% of the values do not exceed. |
| Mean, Median | The average value, and the middle value. |
| SD | The standard deviation. It shows how much the value changes. |
| Range | P95 minus P5. It ignores a few extreme values. |
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

- The map below the graph shows the whole session. Click or drag in the map to go to another time.
- The mouse wheel moves the graph in time.
- Live returns to the newest sound.
- Play window plays the sound of the graph. Export window and Export all save the sound as a WAV file.
- Open file analyzes an audio file.
- Record saves a video, the sound, and a report with the statistics.

## Settings that change what you see

- **View time.** A short view shows fast changes. A long view shows a trend.
- **Frequency scale.** The log, mel and ERB scales give more room to low frequencies. The ear works the same way.
- **View frequency range.** This range only sets what you see. It does not change the formant tracks.
- **Formant limit.** The formant tracker uses its own limit. The default is 5200 Hz. Phoneticians often use 5000 Hz for male voices and 5500 Hz for female voices. The `maxFrequency` value in the `[analysis]` table of the configuration file sets this limit.
- **FFT size.** A larger size separates close harmonics better. It shows fast changes less well.

## Know the limits

The tracks are estimates. Check them against the spectrogram.

- The pitch track can jump by an octave. It can lose the pitch in a creaky voice or in a whisper.
- The formant tracks are less reliable when the pitch is high. At a high pitch the harmonics are far apart, and the tracker has little data between them. Above about 400 Hz, trust the spectrogram more than the formant lines.
- Noise, echo, and a second voice make all tracks worse. Use a quiet room and keep the microphone at a fixed distance.
- InFormant does not tell you how a voice sounds to a listener. Use the numbers as a guide, and ask listeners that you trust.
