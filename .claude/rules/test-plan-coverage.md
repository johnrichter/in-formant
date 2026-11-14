---
paths:
  - "tests/**"
  - "docs/test-plan.md"
workspace:
  id: rule:in-formant:test-plan-coverage
  tags:
    - type:knowledge
    - topic:testing
    - status:complete
    - privacy:public
    - owner:in-formant
  links:
    - rule:in-formant:config-migration
  updated: 2026-11-14T15:40:00Z
---

# Keep each test and its test-plan case in step

**Invariant.** Each Unit case in `docs/test-plan.md` has a unique ID. It also has exactly one of these: a GoogleTest test whose name starts with the ID, or a line in `tests/deferred.txt`.

**Seam.**

- The cases and their IDs in `docs/test-plan.md` (for example `TP-EDGE-07`).
- The test names in `tests/*_test.cpp` (for example `TP_EDGE_07_...`).
- The deferred lines in `tests/deferred.txt` (`ID | package | reason`).
- The registration with `informant_add_test` in `tests/CMakeLists.txt`.
- The script `tests/check_plan_coverage.py`, which CTest runs as `plan_coverage`.

**Failure mode.** A test file that is not in `tests/CMakeLists.txt` never builds, and the suite still passes. A case ID that two sections use makes one test look like coverage for two cases. A review found a duplicate ID in the pause-lag cases of release 3.4.3.

**Detection.**

```
python3 tests/check_plan_coverage.py --plan docs/test-plan.md --tests tests --deferred tests/deferred.txt --report
grep -oE "^- \*\*TP-[A-Z]+-[0-9]+" docs/test-plan.md | sort | uniq -d
```

Read both outputs. The second command finds a case ID that two definitions use, and it must print nothing. Then run `./run-tests.sh`.

**Remediation.** Enumerate together. Add the case, the test, and the CMake line in one change. Take the next free number of the area. A failing test is a bug candidate: list it in section 24 of the plan, fix the program, and never weaken the check. A test writes only in a `QTemporaryDir`, and never leaves a process or an `informant.conf` behind.

**Scope.** Path-scoped to the tests and the test plan.
