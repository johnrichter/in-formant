---
paths:
  - "src/context/guicontext.cpp"
  - "src/context/guicontext.h"
  - "build-macos.sh"
workspace:
  id: rule:in-formant:test-hook-never-ships
  tags:
    - type:knowledge
    - topic:release
    - status:complete
    - privacy:public
    - owner:in-formant
  links:
    - rule:in-formant:test-plan-coverage
  updated: 2026-11-14T15:40:00Z
---

# Never commit or ship the temporary test hook

**Invariant.** No commit and no DMG contains the temporary test hook. The hook is the block between `TEMPORARY TEST HOOK BEGIN` and `TEMPORARY TEST HOOK END`, with its `INFORMANT_*` triggers.

**Seam.**

- `src/context/guicontext.cpp`, where a script inserts the hook for a GUI check.
- The hook text, which lives in the session scratchpad and never in the repo.
- The release DMG from `build-macos.sh`.
- The test `TP_REL_05` in `tests/docs_test.cpp`.

**Failure mode.** The hook drives the app from about two dozen `INFORMANT_*` environment variables. Examples are `INFORMANT_GRAB`, `INFORMANT_PLAY`, `INFORMANT_EXPORT`, and `INFORMANT_PRACTICE`. A build with the hook works normally until one of these variables has a value. A screenshot or a smoke test therefore does not show the hook. For this reason, release 3.1.2 needed a second build without the hook.

**Detection.**

```
grep -rn -e "TEMPORARY TEST HOOK" -e "INFORMANT_GRAB" src
```

The output must be empty. Run this check before each commit and before each release DMG. `./run-tests.sh -R TP_REL_05` runs the same check.

**Remediation.** Verify end to end. Insert the hook in a worktree, run the check, and remove the hook. Then rebuild before you make the DMG. A better long-term fix is a test-only target with its own entry point, so that the release source never needs the hook.

**Scope.** Path-scoped to the GUI context and the build script.
