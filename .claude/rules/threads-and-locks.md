---
paths:
  - "src/context/contextmanager.cpp"
  - "src/context/dataviswrapper.cpp"
  - "src/context/synthwrapper.cpp"
  - "src/context/sessionwrapper.cpp"
  - "src/context/practicewrapper.cpp"
  - "src/context/guicontext.cpp"
  - "src/main.cpp"
  - "src/modules/**"
workspace:
  id: rule:in-formant:threads-and-locks
  tags:
    - type:knowledge
    - topic:concurrency
    - status:complete
    - privacy:public
    - owner:in-formant
  links:
    - rule:in-formant:live-clock
    - rule:in-formant:analysis-solver-state
  updated: 2026-11-14T15:40:00Z
---

# Hand data between threads without a lock across a call out

**Invariant.** A thread never holds a mutex while it emits a Qt signal or calls into another subsystem. A value that only the newest copy matters for (pitch, voicing, formants) goes to its user through one thread, not through a queued signal.

**Seam.**

- The worker loops in `ContextManager`: analysis, datavis, and synthesis.
- The Qt wrappers that the GUI thread reads: `DataVisWrapper`, `SynthWrapper`, `SessionWrapper`, `PracticeWrapper`.
- The audio thread of the synthesizer, which reads the synthesizer parameters.
- The signal handler in `src/main.cpp`.

**Failure mode.** Real instances, each with no error message:

- `DataVisWrapper::setSound` held two mutexes while it emitted a signal. The GUI update took the same mutexes, and the main thread stalled.
- The GUI thread wrote formant values while the audio thread read them. The sound broke up.
- A queued signal delivered an old pitch after a newer one, so the synthesizer followed late values.
- A quit signal called Qt in the middle of a chart update, which held a lock. The app froze.
- `PracticeWrapper::deleteSession` joins the sound thread on the GUI thread. A long write freezes the window.

**Detection.** In a changed function, look for `emit` or a call to another wrapper between a `QMutexLocker` or `std::lock_guard` and the end of its scope. Run `./run-tests.sh --sanitize` for a change in this code.

**Remediation.**

- Copy the data under the lock, release the lock, then emit.
- Send the newest value through the datavis loop, as the pitch to the synthesizer does now.
- In a signal handler, only write one byte to the pipe. The event loop does the rest.
- `Buffer::cancelPulls` sets one static flag for all buffers. A restart of capture must clear it.

**Scope.** Path-scoped to the thread loops, the Qt wrappers, `main.cpp`, and the audio and pipeline modules.
