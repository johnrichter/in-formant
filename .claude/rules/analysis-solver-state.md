---
paths:
  - "src/analysis/**"
  - "src/synthesis/**"
  - "src/context/solvermakers.cpp"
  - "src/context/solvermakers.h"
workspace:
  id: rule:in-formant:analysis-solver-state
  tags:
    - type:knowledge
    - topic:signal-processing
    - status:complete
    - privacy:public
    - owner:in-formant
  links:
    - rule:in-formant:threads-and-locks
  updated: 2026-11-14T15:40:00Z
---

# Keep the state of a solver in the solver object

**Invariant.** A solver keeps its buffers, FFT plans, windows, and filters as members of its own object. A cache rebuilds when any input that shaped it changes (the sample rate, the frame length, the order).

**Seam.**

- The solver classes in `src/analysis/` (pitch, LPC, formants, glottal inverse filtering).
- The factories in `src/context/solvermakers.cpp`, which make one object for each user of a solver.
- The callers: the pipeline thread, the datavis thread, `FileAnalyzer`, and `ReviewSpectrogram`. Two of them can run the same solver type at the same time.

**Failure mode.** All objects and all threads share a function-local `static`, and it does not see a new sample rate. Nothing errors. The output is only wrong. Real instances:

- `acorr_r` in `mpm.cpp` kept a `static` FFT object.
- `iaif.cpp` kept a `static` window and a `static` high-pass filter, built for the first sample rate only. Its window also divided by `length - 1` where it needed `lpW - 1`.
- `yin.cpp` used `r[0] + r[1]` where the YIN difference needs the energy of the two windows.

**Detection.**

```
grep -rn "^\s*static [^(]*;" src/analysis src/synthesis
```

Check each hit. A hit that is not `const` or `constexpr` is a defect. Also test each solver at two sample rates and two frame lengths, called in turn.

**Remediation.** Reduce to one owner. Move the state into the object. Key each cache on all of its inputs. Check a new formula against a synthetic signal with a known answer. For example, use a sine of 220 Hz at 16 kHz and at 48 kHz. Keep the documents in `How-does-it-work.md` and `docs/the-math.md` true to the code.

**Scope.** Path-scoped to the analysis and synthesis code and the solver factories. `src/analysis/wavelet/` and `amgif.cpp` are dead code. Do not spend time on them.
