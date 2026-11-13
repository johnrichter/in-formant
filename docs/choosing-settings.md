# Choosing your settings

This page helps you set the frequency scale, the FFT size, and the four methods in the Input tab. The first section lists a choice for each goal. The other sections explain each setting.

If you are not sure, keep the defaults. They work for most voices. Change one setting at a time, and look at the graph after each change.

## Quick answers by goal

- **Raise or lower your pitch.** Frequency scale: Logarithmic. FFT size: 1024. Pitch method: RAPT. Formant method: Filtered LPC. LPC method: Burg.
- **Change your resonance (F1, F2, F3).** Frequency scale: ERB. FFT size: 2048. Formant method: Filtered LPC. LPC method: Burg.
- **Practice intonation and fast speech.** Frequency scale: Logarithmic. FFT size: 512 or 1024. Pitch method: RAPT.
- **Sing or hold a steady note.** Frequency scale: Logarithmic. FFT size: 4096. Pitch method: McLeod or YIN.
- **Check breathiness or brightness above 4 kHz.** Frequency scale: Linear. Raise the highest frequency of the view to 8000 Hz. FFT size: 2048.

## Frequency scale

The scale sets how the vertical axis uses the space. All four scales show the same sound. They differ in how much room each part of the range gets.

### Linear

Each hertz has the same room. Harmonics (the multiples of your pitch) stand at equal distances. The high range has plenty of room, and the low range is small.

- **Choose it to** count harmonics, to measure the distance between harmonics, and to see noise and breath above 4 kHz.
- **Avoid it for** pitch work. A voice at 100 to 250 Hz fills a small part of the axis.

### Logarithmic

Each doubling of the frequency has the same room. This is how music works. An octave has the same height at 100 Hz and at 1000 Hz. A change of one semitone looks the same at any pitch.

- **Choose it to** follow your pitch, your intonation, and the size of your pitch changes. This is the best scale for pitch training.
- **Avoid it for** the high formants. They are close together near the top.

### Mel

The Mel scale follows how people judge the height of a tone. It is nearly linear below 1000 Hz and nearly logarithmic above 1000 Hz. It gives the middle range more room than the logarithmic scale.

- **Choose it as** a compromise. The pitch and the formants F1 to F3 are all easy to see.
- **Avoid it when** you need exact musical steps. Use the logarithmic scale then.

### ERB

The ERB scale follows how the ear separates frequencies. It gives low frequencies more room than the Mel scale. The range from 100 to 3500 Hz, where the formants F1 to F3 are, takes most of the axis.

- **Choose it to** work on resonance. The three formants are well apart, and F1 and F2 have room to move.
- **Avoid it for** a high range above 6 kHz. This range is small on the ERB scale.

## FFT size

The FFT size sets how much sound each column of the picture uses. A large size shows fine detail in frequency. A small size shows fine detail in time. You cannot have both.

The window time is the FFT size divided by twice the highest frequency of the view. The distance between two lines of the picture is the highest frequency of the view, times two, divided by the FFT size. These are the values for a view up to 5500 Hz:

- **512.** Window 47 ms. Lines 21.5 Hz apart. The picture follows fast changes. Low harmonics of a low voice start to blur together.
- **1024.** Window 93 ms. Lines 10.7 Hz apart. A good balance for speech and pitch work.
- **2048.** Window 186 ms. Lines 5.4 Hz apart. This is the default. It shows each harmonic and the formants of a steady vowel.
- **4096.** Window 372 ms. Lines 2.7 Hz apart. This shows very fine detail of a held note, such as vibrato. A fast glide looks smeared.

A rule: choose the smallest size that still shows each harmonic as a separate line. For a voice with a pitch of 100 Hz, the lines must be closer than 30 Hz. All four sizes meet this rule. A smaller size is then better for fast speech.

A large size also delays the newest column of the picture by half a window.

## Pitch method

The pitch method finds F0, the rate at which your vocal folds open and close. Try another method when the pitch track jumps by an octave, or when it breaks in a quiet voice.

- **YIN.** A fast method with few errors. It works well on a clean voice. It is a good first choice.
- **McLeod (MPM).** A fast method that picks the first strong peak. It is stable on a clean, steady voice, and it suits singing. It can pick a harmonic instead of your pitch when the harmonic is stronger.
- **RAPT.** This method looks at many frames and chooses the best path through them. The track is smoother and has fewer jumps. It also decides which parts are voiced. It usually needs more computer power. This is the default.

## Formant method

The formant method finds F1 to F4 from the sound. A formant track is a good guess and not an exact value. Check it with the spectrogram.

- **Simple LPC.** It takes every resonance that the LPC model finds. It is fast. It can show a false formant or miss a real one.
- **Filtered LPC.** It removes weak resonances, and it checks large gaps for a formant that the first step missed. The tracks are steadier. This is the default.

## Linear prediction (LPC) method

The formant methods above use a model of the vocal tract. The LPC method is the way that the program fits this model. Most people keep the default.

- **Autocorrelation.** Always stable. It smooths the peaks a little, so a formant can look wide and a little off, especially for a high pitch.
- **Covariance.** It fits a short piece of sound more closely. It can become unstable, and then a formant jumps.
- **Burg.** Stable like autocorrelation, and it keeps sharp peaks in a short window. It is the method that the Praat program uses by default. This is the default of InFormant.

## Glottal inverse filtering

This setting changes the glottal flow shape in the oscilloscope window. The glottal flow is the sound of the vocal folds, before the vocal tract changes it. It does not change the main graph.

- **IAIF.** A classic method. It removes the effect of the vocal tract in several steps. It works well on a normal voice.
- **GFM-IAIF.** A newer method that models the glottal flow with a third-order filter. Its authors found it close to IAIF for the glottal formant, and closer to the true spectral tilt. The tilt relates to vocal effort. This is the default.

Try both methods and keep the one with the smoother, more stable shape.

## Sources

- de Cheveigné and Kawahara (2002). YIN, a fundamental frequency estimator for speech and music. The Journal of the Acoustical Society of America.
- McLeod and Wyvill (2005). A smarter way to find pitch. International Computer Music Conference.
- Talkin (1995). A robust algorithm for pitch tracking (RAPT). In Speech Coding and Synthesis.
- Burg (1975). Maximum entropy spectral analysis. Doctoral thesis, Stanford University.
- Alku (1992). Glottal wave analysis with pitch synchronous iterative adaptive inverse filtering. Speech Communication.
- Perrotin and McLoughlin (2019). [A spectral glottal flow model for source-filter separation of speech](https://hal.archives-ouvertes.fr/hal-01971707). ICASSP 2019.
- Dissen, Goldberger and Keshet (2019). Formant estimation and tracking: a deep learning approach. The Journal of the Acoustical Society of America.
- Glasberg and Moore (1990). Derivation of auditory filter shapes from notched-noise data. Hearing Research.
- Stevens, Volkmann and Newman (1937). A scale for the measurement of the psychological magnitude of pitch. The Journal of the Acoustical Society of America.

The advice on this page is a guide and not a rule. The descriptions of the methods come from their papers and from general experience. We did not test the program against each claim. When a method gives a better track for your voice, use it.
