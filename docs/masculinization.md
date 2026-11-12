# Voice masculinization

This page helps you to practise a lower, darker voice with InFormant. A voice goal is personal. You decide what sounds right for you.

**CAUTION.** Do not push your voice down. Stop when you feel pain, tightness, or hoarseness. Rest your voice, and drink water. A speech-language pathologist (SLP) with experience in voice work for trans and gender-diverse people can give you a safe plan. This page does not replace that help.

Testosterone therapy often lowers the pitch over several months. The length of the vocal tract does not change. Resonance and intonation therefore need practice, even after the pitch drops.

## What InFormant can measure

Three parts of the voice change how listeners hear gender. InFormant shows all three.

- **Pitch** is how high the voice is. InFormant shows it as the pitch track and as the Mean, Median and Note statistics.
- **Resonance** is the colour of the voice. It depends on the length and the shape of the vocal tract. InFormant shows it as the formant tracks F1, F2 and F3.
- **Intonation** is how much the pitch moves in speech. InFormant shows it as the Range and SD statistics of the pitch.

No single part decides how a listener hears your voice. The three parts work together. Words, speed, loudness, and body language also have an effect.

## Reference values

These are average values for adult speakers of American English. They come from the study of Peterson and Barney (1952). Your values will differ. Use them as a rough guide, not as a goal.

- "ee", men: F1 270 Hz, F2 2290 Hz, F3 3010 Hz.
- "ee", women: F1 310 Hz, F2 2790 Hz, F3 3310 Hz.
- "ah", men: F1 730 Hz, F2 1090 Hz, F3 2440 Hz.
- "ah", women: F1 850 Hz, F2 1220 Hz, F3 2810 Hz.
- "oo", men: F1 300 Hz, F2 870 Hz, F3 2240 Hz.
- "oo", women: F1 370 Hz, F2 950 Hz, F3 2670 Hz.

The formants of men are about 10 to 15 percent lower than the formants of women.

## Pitch

Studies often report an average speaking pitch of about 110 to 120 Hz for men. Studies of listeners often place the border between a male and a female voice near 150 to 180 Hz. A voice with an average pitch below this zone is usually heard as male.

How to practise:

1. Make a baseline. Read a short text in your normal voice for one minute.
2. Read the Mean and the Median of the pitch. Write them down.
3. Pick a goal that is a little lower than your baseline. Do not jump to a low value.
4. Read the text again at the new pitch. Keep the voice relaxed and easy.
5. Check the pitch track. The line must stay near your goal.

Stay inside the range that your voice makes with ease. A pitch that is too low for your voice often turns into vocal fry. Vocal fry is a creaky, rattling sound. The pitch track often has gaps when the voice fries, because the tracker cannot find a clear pitch. Long use of vocal fry can tire the voice.

## Resonance

A longer vocal tract gives lower formants. You can make the vocal tract longer and the space larger in four ways:

- **Round the lips.** Push the lips forward a little, as for the vowel "oo". This lowers F2 and F3.
- **Move the tongue back and down.** This lowers F2.
- **Open the jaw a little more.** Keep the throat relaxed.
- **Lower the larynx a small amount.** Think of the start of a yawn. Never push the larynx down by force.

How to practise:

1. Choose the task "Vowels", and press Practice. Hold the vowel "ee" for 3 seconds in your normal voice. Press M in the middle of the vowel.
2. Hold the vowel again with the changes above. Press M in the middle of the vowel. The marks save the pitch and the formants F1 to F4. Compare the two marks in the file `marks.csv` of the session.
3. Do the same for "ah" and "oo".
4. A fall of 10 to 15 percent in all three formants is a useful first goal.

Keep the pitch the same during this exercise. Then you hear the effect of resonance alone. Also check that the spectrogram looks clean. A strained voice often shows noise between the harmonics.

## Intonation

Intonation is the movement of the pitch while you speak. Two numbers describe it: the pitch range and the pitch variation.

- **Pitch range** is the distance between a low pitch and a high pitch. InFormant uses the 5th and the 95th percentile (P5 and P95), so a few extreme values do not count.
- **Pitch variation** is how far the pitch moves around its mean. InFormant shows it as SD and as CV.

Hertz is a poor unit for a range. The same step in hertz is a large step for a low voice and a small step for a high voice. Use semitones. One semitone is one key on a piano, and 12 semitones are one octave. The formula is 12 times the base 2 logarithm of P95 divided by P5.

A worked example:

- A baseline text has P5 of 100 Hz and P95 of 150 Hz. The range is 12 times log2 of 150 / 100, which is 7.0 semitones.
- A calm reading of the same text has P5 of 105 Hz and P95 of 135 Hz. The range is 12 times log2 of 135 / 105, which is 4.4 semitones.
- The calm reading is 2.7 semitones narrower.

How to read the numbers in InFormant:

- Switch on the statistics in the Readouts tab. Choose Range, and choose "Show the Range in semitones". The Range line then shows a value like `4.4st`.
- The report of a Record and the summary of a practice session also give the range in semitones.
- Watch the SD and the CV of the pitch. A lower CV means less movement.
- Look at the shape of the pitch track at the end of each phrase. A phrase can end with a rise, a fall, or a level line.

What to practise:

- Many listeners hear less pitch movement as more masculine. They also hear a fall at the end of a statement.
- Read a text in a calm voice, as if you give a short report. Compare its range with your baseline text. Compare it again each week.
- Lower is not always better. A voice with no movement at all sounds tired or bored. Aim for a calm and steady movement.
- Style differs between cultures, languages, and people. Use the numbers to track your own change, and not to match a number. The listener has the last word.

## A practice plan

1. Record a baseline of three tasks: a sustained "ee", "ah" and "oo", a short text, and a conversation of one minute. Use the Practice button, and choose the task for each part.
2. Practise one skill at a time. Start with the skill that is easiest for you.
3. Practise in short sessions, and rest between them. Warm up first. The page "Warm-ups and vocal health" shows how.
4. Press M to mark a moment that you want to remember. A mark saves the pitch and the formants at that time.
5. Open the Progress window each week. Choose a task and a measure. The chart shows your change over days, weeks, or months.
6. Ask people that you trust how the voice sounds. Numbers are a guide. The listener has the last word.

## Limits

- The formant tracker can make errors. Check the tracks against the spectrogram.
- The pitch tracker can jump by an octave, and it loses the pitch in vocal fry.
- The reference values come from one study of one language. Other languages and groups have other values.
