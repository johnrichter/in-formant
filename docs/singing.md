# Singing

This page shows how to use InFormant to practice singing. The pitch track helps with tuning and steadiness. The spectrogram helps with tone and vowels.

**CAUTION.** Warm up before you sing. Stop when you feel pain, tightness, or hoarseness. A singing teacher can check your technique, and a doctor can check a voice that stays hoarse. This page does not replace that help.

## Sing in tune

A semitone is the step between two neighboring keys of a piano. A cent is one hundredth of a semitone. The Note statistic shows the nearest note of the mean pitch, and the error in cents. For example, "A4 +12c" means 12 cents above A4 (440 Hz).

Many listeners accept an error of up to about 20 cents on a sung note. A good listener can hear a difference of 5 to 10 cents when two pure tones sound one after the other.

How to practice:

1. Show the statistics boxes. Choose the lines Mean, SD and Note in the Readouts tab of the settings.
2. Play a reference note on an instrument.
3. Sing the note for 4 to 5 seconds. Keep the voice steady.
4. Read the Note line in the pitch box. A value near 0 cents is in tune.
5. Click on the graph at the start of the note, and again near the end. Compare the values. A fall of the pitch over time is a common error.

## Keep a note steady

The CV statistic is the standard deviation divided by the mean, in percent. It gives a quick check of steadiness. The error in cents is about 17 times the CV in percent. A CV of 0.5 percent is an error of about 9 cents.

Sing the same note on different vowels. A note that is steady on "ah" and shaky on "ee" shows a place where the technique needs work.

## Vibrato

Vibrato is a regular small change of the pitch. In classical singing it often has a rate of 5 to 7 cycles each second. Its width is often 50 to 100 cents above and below the center. A rate that is much slower sounds like a wobble. A rate that is much faster sounds like a bleat.

How to see vibrato:

- Set the view time to 2 or 3 seconds in the View field below the graph.
- Choose the line style for tracks in the Tracks tab of the settings. Set the smoothness of the line low, so the line does not hide the movement.
- Count the peaks in one second. Measure the width of the movement against the frequency axis. Select the notes or the octaves mode of the axis to read the width in semitones.

## Resonance and tone

The spectrogram shows two things. The horizontal lines are harmonics. The harmonics are multiples of the pitch: F0, 2 times F0, 3 times F0, and so on. The wide bright bands are formants. A formant makes the harmonics near it louder.

- **The singer's formant.** Trained male classical singers often have a bright band between 2500 and 3500 Hz. Here the formants F3, F4 and F5 move close together. This band makes the voice carry over an orchestra. Look for it on a sustained vowel.
- **Vowel tuning at a high pitch.** When the pitch rises above the usual F1 of a vowel, the vowel can lose its color and its loudness. Many trained sopranos change the jaw and the lips so that F1 stays near the pitch. The vowel then changes a little, and the voice stays loud.
- **Clean tone.** A clean tone shows clear harmonics with little noise between them. A breathy tone shows noise between the harmonics.

At a high pitch the harmonics are far apart. The formant tracker then has little data. Above about 400 Hz, trust the spectrogram more than the formant lines.

## Find your range

The Min and the Max lines give the lowest and the highest pitch of the last seconds. The Note line shows the note of the mean. To find the lowest and the highest note of your range:

1. Set the statistics window in the Readouts tab to a long time, for example 30 seconds.
2. Slide from your lowest comfortable note to your highest comfortable note.
3. Read Min and Max. Click on the graph at the ends of the slide to read the exact values.
4. Use the formula in "The math" to count the semitones between the two values.

Stay inside the range that you sing with ease. Never push to a note that hurts.

## A practice plan

1. Warm up for five minutes with easy sounds.
2. Practice one skill at a time: tuning, steadiness, vibrato, or vowels.
3. Record your practice with Record. The report gives the statistics of each part.
4. Listen to the recording. Then compare it with the graph.
5. Rest your voice between sessions.

## Limits

- The pitch tracker can jump by an octave on a note with strong overtones. A jump shows as a dot far from the line.
- A fast run of notes can look rough, because the pitch track needs a short time to settle on a new note.
- The tracks need one voice and a quiet room. A piano or a second singer in the room spoils them.
