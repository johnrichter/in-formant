---
paths:
  - "src/context/config.cpp"
  - "src/context/config.h"
  - "src/context/configmigration.h"
  - "src/gui/renderoptions.h"
  - "src/qml/SettingsWindow.qml"
workspace:
  id: rule:in-formant:config-migration
  tags:
    - type:knowledge
    - topic:configuration
    - status:complete
    - privacy:public
    - owner:in-formant
  links:
    - rule:in-formant:test-plan-coverage
  updated: 2026-11-14T15:40:00Z
---

# Change a default only together with a migration

**Invariant.** A change to a stored default, a renamed key, or a removed choice comes with a new config version and a one-time migration.

**Seam.**

- The default in `src/context/config.cpp` and in `src/gui/renderoptions.h`.
- `kConfigVersion` and the migration chain in `src/context/configmigration.h`.
- The choices in `src/qml/SettingsWindow.qml`.
- The cases in `tests/config_migration_test.cpp` and `tests/config_test.cpp`, and their IDs in `docs/test-plan.md`.

**Failure mode.** The app writes defaults to `informant.conf`. A new default in code therefore does not reach an existing user, and nothing reports it. Real instances:

- Version 7 moved the readout boxes from "bottom" to "left". Without the migration, old users kept "bottom".
- Version 8 replaced one formant switch with four. A file with the old switch off must turn all four off one time.
- The removed Deep formant method stays in the `FormantAlgorithm` enum. The user-facing getter maps a stored Deep value to Filtered. Without that mapping, `makeFormantSolver` throws.

**Detection.** Run `git diff with-qt -- src/context/config.cpp src/gui/renderoptions.h`. When a default changed, `kConfigVersion` must change in the same diff. Run `./run-tests.sh -R Config`.

**Remediation.** Enumerate together. Raise `kConfigVersion` by one. Add a migration step that changes a stored value only when it still equals the old default, and only when the file version is lower. A later choice of the user then stays. Never delete an enum value that a stored file can hold. Add a migration test and a test-plan case.

**Scope.** Path-scoped to the config code, the render options, and the Settings window.
