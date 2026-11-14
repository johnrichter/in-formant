---
paths:
  - "src/qml/**"
  - "src/gui/**"
  - "docs/design-system.md"
workspace:
  id: rule:in-formant:design-tokens
  tags:
    - type:knowledge
    - topic:design-system
    - status:complete
    - privacy:public
    - owner:in-formant
  links:
    - rule:in-formant:help-pages
  updated: 2026-11-14T15:40:00Z
---

# Take each size from the design tokens

**Invariant.** Each size, space, radius, and color comes from `src/gui/uiscale.h`. This covers text widths, the windows, and the graph. In QML it comes from `ui.type`, `ui.space`, `ui.radius`, `ui.color`, `ui.font`, or `ui.measure`.

**Seam.**

- The tokens in `src/gui/uiscale.h`.
- The numbers in `docs/design-system.md` (test `TP_DOC_20`).
- The shared buttons and tabs in `src/qml/`: `AppButton`, `AppToolButton`, `AppTabBar`, `AppTabButton`, and `SegmentedControl`.
- The shared text and option controls: `Heading`, `Hint`, `HelpButton`, `OptionSwitch`, and `OptionSlider`.
- Each window that uses them, and the graph renderer in `src/gui/`.

**Failure mode.** A literal number looks right in one window and drifts from the others. Real instances:

- The reset button had its own size and was much too big next to the play button.
- Several wrapped labels used a fixed pixel width, not `ui.measure`, so their lines grew past 560 pixels.
- A plain Qt button has fully round ends. Only `AppButton` and `AppToolButton` set the radius of 8 pixels.

**Detection.**

```
git diff with-qt -- src/qml | grep -E "^\+.*(pixelSize|spacing|radius|Margin|width):\s*[0-9]"
```

Check each line. An added line that is not a mark (a tick, a line width, a dot) is a defect. The windows still have about 50 old literal numbers. Replace one when you touch its line. Run `./run-tests.sh -L ui_scale` and `-R TP_UI_12`. For a visual change, launch the app and look at a screenshot.

**Remediation.** Reduce to one source of truth. Use a token, or a shared control. When no token fits, add one to `uiscale.h` and to `docs/design-system.md` in the same change. Keep the chosen tab and segment as a solid accent fill. Give each new control a gray `Hint`, and a `HelpButton` when the hint is not enough.

**Scope.** Path-scoped to the QML windows, the GUI code, and the design-system page. The graph still uses its own font sizes of 10, 12, and 14 pixels. Do not copy them into a window.
