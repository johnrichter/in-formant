---
paths:
  - "src/context/contextmanager.cpp"
  - "src/context/contextmanager.h"
  - "src/context/datastore.cpp"
  - "src/context/datastore.h"
  - "src/context/session.cpp"
  - "src/context/session.h"
  - "src/context/sessionwrapper.cpp"
  - "src/context/viewrange.h"
  - "src/modules/app/pipeline/**"
  - "src/modules/audio/**"
  - "src/gui/plotitem.cpp"
workspace:
  id: rule:in-formant:live-clock
  tags:
    - type:knowledge
    - topic:audio
    - status:complete
    - privacy:public
    - owner:in-formant
  links:
    - rule:in-formant:threads-and-locks
  updated: 2026-11-14T15:40:00Z
---

# Use the count of stored samples as the only live clock

**Invariant.** All live times come from the count of samples in the session. This covers the end of the live view, the time of a track point, and the time of a spectrogram slice. `LiveClock` can add the wall time since the last stored block, for smooth motion, but never more than 0.1 seconds.

**Seam.**

- `LiveClock` in `src/context/viewrange.h`, which gives the end of the live view.
- The time counter in `Pipeline::callbackProcessing`, which advances by the samples of each block.
- `BaseProcessor::process`, which stamps each frame at its newest sample.
- `Session::append` and `Session::duration`, which store the sound.
- The capture `Buffer`, which can drop samples when it is full.

**Failure mode.** The live spectrogram showed an empty strip at its edge after each pause and play. The wall clock ran ahead of the stored sound. Each start added one input latency, and dropped capture samples added more. Nothing reported the drift.

In a second instance, the pipeline added the configured granularity for each block. A block has a whole number of samples, so the slices ran ahead of the sound. Commits `8e66ca12` and `7f7cc28c` fixed both.

**Detection.** Search for a wall-clock source in the live path:

```
grep -rn -e "steady_clock" -e "system_clock" -e "QElapsedTimer" -e "QDateTime::current" src/context src/modules src/gui
```

Check each hit. A hit that sets a live time is a defect, except the capped step inside `LiveClock`. Run the tests `TP_EDGE_07` to `TP_EDGE_10` (`./run-tests.sh -R TP_EDGE`).

**Remediation.** Reduce to one source of truth. Compute a time as `samples / sampleRate`. Use the wall clock only for the capped step of `LiveClock`, a frame rate, a timeout, or a log line.

**Scope.** Path-scoped to the capture, pipeline, store, and live view code.
