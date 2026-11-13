# The design system of InFormant

This page describes the type scale, the spacing scale, and the rules that make the windows easy to read. The numbers are in `src/gui/uiscale.h`. A unit test (`tests/ui_scale_test.cpp`) checks the rules.

## Why InFormant has its own scale

InFormant is a dense tool with dark windows. A person reads numbers, small labels, and short hints for a long time. A scale for a web page does not fit this use. These sources shaped the design:

- A 4-point grid for all spaces and line heights: [The ultimate spacing guide for UI designers](https://www.uxlab.academy/blogs/the-ultimate-spacing-guide-for-ui-designers).
- A modular type scale, where each step is a fixed ratio of the step below: [Type scale](https://interfaces.show/type-scale/) and [typescale.com](https://typescale.com/).

Those pages use a major third (1.25) from a base of 16 pixels. InFormant uses a minor third (1.2) from a base of 14 pixels. The steps are closer, so a window can show many levels of text and stay calm.

## The type scale

- **Caption, 12 pixels, line 16.** Hints, units, and small notes. Nothing in the app is smaller than 12 pixels.
- **Body, 14 pixels, line 20.** Labels, controls, and paragraphs.
- **Subtitle, 17 pixels, line 24.** The headings of the groups in a window. Bold.
- **Title, 20 pixels, line 28.** The title of a window or a dialog.
- **Headline, 24 pixels, line 32.** A number or a short phrase that stands alone.

The steps above the body follow the ratio 1.2 (16.8, 20.2, and 24.2, rounded to 17, 20, and 24). The step below the body would be 11.7, so the floor of 12 pixels sets the caption.

## The fonts

- **Roboto** (regular, medium, and bold) is the font of the text. The windows and the graph use the same family.
- **Roboto Mono** is the font of numbers and of labels that line up. Each character has the same width, so digits stay in columns and a value does not move the text next to it.
- In the graph, Roboto Mono shows the boxes of the tracks, the numbers on the axes, the frame times, and the labels of the cursor.
- In the windows, Roboto Mono shows the values of sliders, the number boxes, and the number fields. It also shows the paths and the tables of the Progress window.
- The font files are in the resources of the app, so the windows look the same on each Mac. The licence of both families is Apache 2.0. The files `LICENSE-Roboto.txt` and `LICENSE-RobotoMono.txt` are in the `resources` folder.

## The spacing scale

All spaces are multiples of 4 pixels.

- **xs, 4.** Between a label or a slider and its hint.
- **sm, 8.** Between a label and its control, and between a switch and its hint.
- **md, 12.** Between two controls of one group.
- **lg, 16.** The margin of a small window, and the space above the first heading.
- **xl, 24.** The margin of a large window, and the space between groups.
- **xxl, 32.** The space between large parts.
- **xxxl, 48.** The space around an empty state.

The radii are 4, 8, and 12 pixels.

## The rules

1. **Three levels in each window.** A heading, a field label, and a hint. Each level differs in size, in weight, and in colour. The heading is 17 pixels and bold. The field label is 14 pixels and medium. The hint is 12 pixels in the secondary colour.
2. **Close means related.** A hint is 4 pixels from its label or slider, and 8 pixels from a switch (a switch is a tall control). Two controls of one group are 12 pixels apart. Two groups are 24 pixels apart, or more. A person sees the groups without lines.
3. **Space instead of lines.** A heading with a large space above it separates the groups. The windows have no separator lines between groups.
4. **Short lines.** A paragraph has at most 560 pixels, which is about 70 characters. A longer line tires the eye.
5. **Enough contrast.** Each text colour has a contrast of at least 4.5 to 1 against the window and the surface. The primary colour is for headings, labels, and values. The secondary colour is for hints and units. The tertiary colour is for the least important text.
6. **Numbers in columns.** A value that changes uses the monospace font, so the digits keep their width.
7. **One place for the numbers.** A window reads the scale through the context property `ui` (`ui.type`, `ui.space`, `ui.radius`, `ui.color`, `ui.font`, `ui.measure`). A window does not use its own number for a size or a space.
8. **Every control explains itself.** A control has a grey hint under it when its name does not say what it does.

## How to use the scale in a window

- Use the component `Heading` for a heading, and set `first: true` for the first heading of a page.
- Use the component `Hint` for a line of explanation. The option controls `OptionSwitch` and `OptionSlider` and the lists `ConfigCombo` and `ConfigSlider` take a `hint` property.
- Set the spacing of a column to `ui.space.md`. Set the margin of a window to `ui.space.lg` or `ui.space.xl`.
- Set `font.pixelSize: ui.type.body` on the window. Use `ui.type.caption` for dense toolbars.
- Set `font.family: ui.font.mono` on a number, a number field, or a label that lines up.

## Limits

- The drawing of the graph (the boxes, the axes, the tick labels) has its own font sizes of 10, 12, and 14 pixels. They are not part of this scale yet.
- A screenshot and a unit test of the numbers checked the scale. No user test measured the reading speed.
