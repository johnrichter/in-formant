# The math

This page gives the formulas behind the numbers in InFormant. You do not need it to use the program. It helps when you want to check a number or to plan a practice goal.

## Pitch, notes, semitones and cents

The ear hears pitch on a logarithmic scale. Two notes one octave apart have a frequency ratio of 2.

```
semitones between f1 and f2 = 12 * log2(f2 / f1)
cents between f1 and f2     = 1200 * log2(f2 / f1)
MIDI note number            = 69 + 12 * log2(f / 440)
frequency of a MIDI note    = 440 * 2^((n - 69) / 12)
```

The Note statistic uses the MIDI formula. It rounds to the nearest note, with A4 at 440 Hz, and shows the rest in cents.

Examples:

- A pitch range from 150 Hz to 300 Hz is 12 * log2(2) = 12 semitones.
- A pitch range from 180 Hz to 240 Hz is 12 * log2(240 / 180) = 5.0 semitones.

The Mean statistic is the arithmetic mean in Hz. The ear follows the geometric mean better. The geometric mean is exp(mean(ln f)). For a voice that moves over a wide range, the two values differ by a few Hz.

A small error in cents follows from the CV statistic.

```
error in cents = 1200 * log2(1 + SD / Mean)
               ≈ 17.3 * CV in percent        (for CV below about 5 percent)
```

## The vocal tract and the formants

A simple model treats the vocal tract as a tube. The tube is closed at the vocal folds and open at the lips. The length is L. This formula gives the resonances of this tube:

```
Fn = (2n - 1) * c / (4 * L)        n = 1, 2, 3, ...
c  = about 35 000 cm/s             (speed of sound in warm, moist air)
```

Examples:

- L = 17.5 cm gives F1 = 500 Hz, F2 = 1500 Hz and F3 = 2500 Hz.
- L = 14.5 cm gives F1 = 603 Hz, F2 = 1810 Hz and F3 = 3017 Hz.

The model holds for a relaxed, neutral vowel. Other vowels change the shape of the tube, and the formants then move away from these values. The model still shows an important fact. Every formant is proportional to 1 / L.

```
new formant = old formant * (old length / new length)
```

- A vocal tract that is 10 percent shorter raises all formants by 11 percent (1 / 0.9 = 1.11).
- A vocal tract that is 10 percent longer lowers all formants by 9 percent (1 / 1.1 = 0.91).

Many men have a vocal tract of about 17 cm, and many women have one of about 14 to 15 cm. The ratio is about 1.15 to 1.2. This agrees with the difference of 10 to 20 percent between the average formants of men and women.

Lip rounding and a low larynx make the tube longer. Lip spreading and a high larynx make it shorter.

## Harmonics

The voice source makes a series of harmonics.

```
harmonic k = k * F0        k = 1, 2, 3, ...
```

The vocal tract does not make new frequencies. It makes the harmonics near a formant louder. When F0 is 200 Hz, the harmonics are 200 Hz apart. The tracker then cannot see a formant between two harmonics well. This is why formant tracking is less accurate at a high pitch.

## Frequency scales

The vertical axis has four scales. Each scale converts a frequency f in Hz to a position v.

```
linear:   v = f
log:      v = log2(f)                         (one octave is one step)
mel:      v = 2595 * log10(1 + f / 700)
ERB rate: v = 21.33 * log10(1 + 0.00437 * f)
```

The mel scale and the ERB-rate scale follow how the ear resolves frequency. Both give more room to low frequencies. The log scale gives equal room to each octave.

## The spectrogram

The spectrogram cuts the sound into short windows of N samples. It makes a Fourier transform of each window. N is the FFT size.

```
frequency step = fs / N
window length  = N / fs        (in seconds)
```

fs is the sample rate. A larger N gives a smaller frequency step and a longer window. You cannot have both. The product of the frequency step and the window length is about 1.

Example: fs = 11 000 Hz and N = 1024 give a frequency step of 10.7 Hz and a window of 93 ms. The harmonics of a voice at 100 Hz are then well separated, but a fast change of the pitch looks blurred.

For a long view, and for a view of a past time, InFormant takes the sample rate as twice the highest frequency of the view.

## Formant tracking

The formant tracker uses linear prediction (LPC). LPC models the vocal tract as an all-pole filter of order p.

```
x[n] is about a1 * x[n-1] + a2 * x[n-2] + ... + ap * x[n-p]
```

Each pair of complex poles at radius r and angle θ gives a formant.

```
frequency F = θ * fs / (2π)
bandwidth B = -ln(r) * fs / π
```

A common rule for the order is p = fs / 1000 + 2. This gives 2 poles for each expected formant, and 2 more for the slope of the spectrum. How-does-it-work.md in the root folder of the program describes the algorithms in detail.

## Statistics

N is the number of values in the window. For the percentiles, the program sorts the values x from the smallest to the largest.

```
Mean      = (x1 + x2 + ... + xN) / N
SD        = sqrt( sum((xi - Mean)^2) / (N - 1) )
CV        = 100 * SD / Mean                          (in percent)
P(q)      = the value at position q * (N - 1) in the sorted list,
            with a straight line between the two nearest values
Range     = P(0.95) - P(0.05)
Voiced    = 100 * (frames with a value) / (all frames)
```

The median is P(0.5). The statistics use the values of the chosen window, not the points that the graph draws.
