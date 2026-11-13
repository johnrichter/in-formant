# The synthesizer: how to use it

The synthesizer makes a test voice from numbers. You choose the pitch and the formants, and you hear the result. This page shows how to reach a sound that you have in mind.

**CAUTION.** Wear headphones before you start the synthesizer. Set a low volume on your computer first, and raise it slowly. Without headphones, the microphone hears the speakers, and the sound can change by itself.

## What the synthesizer is for

- It teaches you how each number changes a voice. You hear the change as you make it.
- It gives you a target. You can hear a vowel at a pitch and with formants that you read from your own practice.
- It makes a steady, simple voice. It has no words, no consonants, and no nasal sound. It does not record or save a sound.

## Start in six steps

1. Wear headphones. Set a low volume.
2. Press the Synthesizer button in the main window.
3. In the box "Quick start", choose a vowel. Choose "ah, as in father" for the first test.
4. Choose a voice. "Lower voice" sets 120 Hz and the formants of an average man. "Higher voice" sets 210 Hz and the formants of an average woman.
5. Press Apply. The sliders and the number boxes change to the new values.
6. Start the sound with the switch "Turn on/off" at the top left. You hear a steady "ah".

Press "Reset all" to return every control to its value at the start.

## How a voice is made

A voice has two parts. You can change each part alone.

- **The source** is the buzz of the vocal folds. It sets the pitch, and it sets how soft or how pressed the voice sounds. The controls for the source are the pitch, Rd, Tc, the volume, the jitter, and the breath noise.
- **The filter** is the shape of the throat, the mouth, and the lips. It sets the formants F1 to F4. The formants make the vowel and the color of the voice.

A vowel is a pattern of formants. The same pattern at a low pitch or at a high pitch is the same vowel. A voice that sounds low, dark, or male usually has a low pitch and low formants. A voice that sounds high, bright, or female usually has a high pitch and high formants.

## What each control does

- **The main switch.** The switch at the top left starts and stops the sound.
- **Aspirate noise volume.** This is the noise of breath. At 0% the voice is clean. At 30% (the start value) it is a little breathy. At 100% it is very breathy.
- **Glottal source volume.** This is the loudness of the buzz. Keep it at 100% for a normal voice. Use the volume of your computer to change the loudness.
- **Glottal source pitch.** This is the pitch in hertz. A box under the slider takes a typed value. About 120 Hz is a low voice, 170 Hz is in the middle, and 210 Hz is a high voice.
- **Follow pitch estimates.** The synthesizer copies the pitch that the microphone hears. Use headphones. Clear it to hear a steady pitch.
- **Rd.** This is the softness of the buzz. A low value (0.7) gives a pressed, bright, and loud voice. A high value (2.6) gives a soft, breathy, and dark voice. The start value is 1.7.
- **Tc.** This is the share of each cycle in which the vocal folds move. At 1.0 the folds never rest. A lower value adds a short rest. The range is small, so change it in small steps.
- **Vocal tract filter shift.** This moves all formants up or down at one time. At 100% it changes nothing. Keep it at 100% when you type the formants yourself.
- **Voicing.** Off removes the buzz, and only the breath noise stays. The sound is a whisper.
- **Pitch jitter.** This adds small random changes to the pitch. At 0% the voice is a perfect machine. A few percent sounds more like a person.
- **Follow formant estimates.** The synthesizer copies the formants that the microphone hears. Use headphones. Clear it to set the formants yourself.
- **F1 to F4, frequency.** A slider and a number box set each formant in hertz. Click a number and type a new value, or use the arrows.
- **F1 to F4, bandwidth.** The bandwidth sets how sharp a formant is. A small value gives a clear, ringing peak. A large value gives a soft peak. The start values are 60, 90, 120, and 150 Hz.

## The three plots

- **Synthesis filter.** It shows the formants as peaks. A peak is at the frequency of a formant. Move a formant, and the peak moves.
- **Synthesized source.** It shows the shape of one buzz of the vocal folds. Move Rd, and the shape changes.
- **Synthesized source spectrum.** It shows the harmonics of the buzz. A high pitch gives harmonics that are far apart. A low pitch gives harmonics that are close together.

## What the formants mean in the mouth

You can reach most vowels with F1 and F2 alone.

- **F1 goes with the opening of the jaw.** A low F1 (300 Hz) is a closed mouth, as in "ee" and "oo". A high F1 (700 to 850 Hz) is an open mouth, as in "ah".
- **F2 goes with the place of the tongue.** A high F2 (2300 Hz and above) is a tongue at the front, as in "ee". A low F2 (900 Hz) is a tongue at the back and rounded lips, as in "oo".
- **F3 and F4 give color.** They change little between vowels. They make the voice sound smaller or larger.
- **All formants together** go down when the vocal tract is longer, and they go up when it is shorter. Rounded lips and a low larynx make the tract longer.

## Recipe 1: hear a vowel

1. Choose a vowel in the Quick start box.
2. Press Apply with the same voice, and listen.
3. Choose another vowel, and press Apply again. Listen to the change.
4. Open the plot "Synthesis filter" while you do this. Watch the first two peaks move.

## Recipe 2: hear resonance alone

Pitch and resonance together decide how a voice sounds. This recipe removes the pitch, so that you hear the resonance.

1. Clear the check box "Also set the pitch".
2. Set the pitch to 170 Hz with the number box.
3. Choose "ah, as in father" and the voice "Lower voice". Press Apply. Listen.
4. Choose the voice "Higher voice", and press Apply. Listen again.
5. The pitch is the same in both sounds. The change that you hear is the resonance.

## Recipe 3: hear pitch alone

1. Press Apply for one vowel and one voice.
2. Move the pitch slider up and down. The vowel stays the same.
3. Find the pitch at which the voice sounds natural for the formants that you set.

## Recipe 4: copy your own vowel

Use this recipe when you want to hear what your practice sounds like as a steady sound.

1. In the main window, choose the task "Vowels" and press Practice.
2. Hold the vowel for 3 seconds. Press M in the middle of the vowel.
3. Stop the practice. Open the file `marks.csv` in the session folder.
4. Read the pitch and the formants F1 to F4 in the row of the mark.
5. In the synthesizer, type the pitch in the box under the pitch slider.
6. Type F1 to F4 in the number boxes. Press the Enter key after each number.
7. Start the synthesizer, and listen to your own vowel.

The formant tracker can make errors. When the sound is not close to your voice, check the formant tracks in the main window.

## Recipe 5: reach a target value

1. Write down the target. For example: pitch 200 Hz, F1 600 Hz, F2 1500 Hz.
2. Type the values in the boxes.
3. Change one value at a time. Listen after each change.
4. When the vowel sounds wrong, check F1 and F2 first. Then check the bandwidths.

## Recipe 6: a whisper and a breathy voice

- **A whisper.** Clear the switch Voicing. Set the noise volume to 100%.
- **A breathy voice.** Keep Voicing on. Set the noise volume to 60%. Set Rd to 2.2.
- **A pressed voice.** Set the noise volume to 0%. Set Rd to 0.9.

## If something is wrong

- **You hear nothing.** Check the switch "Turn on/off". Check the volume of your computer. Check that the glottal source volume is above 0%. When Voicing is off, check that the noise volume is above 0%.
- **The sound squeals or changes by itself.** A follow switch is on, and the microphone hears the speakers. Wear headphones, or clear the follow switches.
- **The vowel sounds wrong.** Check that the filter shift is at 100%. Check F1 and F2 against the list of vowels in the Quick start box.
- **The sound is too buzzy.** Raise the jitter a little. Raise Rd a little. Raise the bandwidths.
- **The sound is too weak.** Lower the bandwidths. Lower the noise volume.
- **The vowel is not clear at a high pitch.** At a high pitch the harmonics are far apart. They do not show the formants well. This is normal, and a real voice has the same problem.

## Limits

- The vowel values are averages of adult speakers of American English from one study (Peterson and Barney, 1952). Your voice, your language, and your age give other values.
- The value of F4 is a typical value and not a measured one. The study has no F4.
- The descriptions of the controls come from the theory of the voice model. They are not a result of a listening test. Trust your ears.
- The synthesizer has no consonants, no nasal sound, and no change of the formants in time.
- The synthesizer does not save a sound. It plays through your headphones only.
