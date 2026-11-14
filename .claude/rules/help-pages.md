---
paths:
  - "docs/**"
  - "README.md"
  - "How-does-it-work.md"
  - "resources/qml.qrc"
  - "src/qml/HelpWindow.qml"
workspace:
  id: rule:in-formant:help-pages
  tags:
    - type:knowledge
    - topic:documentation
    - status:complete
    - privacy:public
    - owner:in-formant
  links:
    - rule:in-formant:design-tokens
  updated: 2026-11-14T15:40:00Z
---

# Register each help page in all four places, and keep it true

**Invariant.** Each help page in `docs/` is in `resources/qml.qrc`, in the page list of `HelpWindow.qml`, and linked from `README.md`. Its text agrees with the code and with the other pages.

**Seam.**

- The Markdown file in `docs/`.
- The `help/<name>.md` alias in `resources/qml.qrc`.
- The `{ title, file }` entry in `src/qml/HelpWindow.qml`. Other pages name a page by this title.
- The link in the "Help and guides" item of `README.md`.
- The tests `TP_DOC_05`, `TP_DOC_11`, `TP_DOC_13`, and `TP_DOC_20` in `tests/docs_test.cpp`.

**Failure mode.** When one of these places does not list a page, the app builds and runs, but the Help window does not show the page. Text can also drift from the code with no error. Real instances:

- `How-does-it-work.md` named Laguerre root finding, but `find_roots.cpp` calls Aberth.
- The feminization and masculinization pages said opposite things about how the formants move when the jaw opens.
- The README gave a config path without "Application Support".
- The docs still described Linux, Windows, Android, and Torch after their code was gone.
- The style checker printed CLEAN while pages still had phrasal verbs, passive verbs, and a broken sentence.

**Detection.** Run `./run-tests.sh -R TP_DOC`. Set `INFORMANT_STE_CHECK` to the path of the style script to also run `TP_DOC_01`. For each number or method that a page names, find it in the code.

**Remediation.** Enumerate together. Change the page, the resource, the window list, and the README in one change. Write in Simplified Technical English with American spelling. Read each page yourself after the checker. Give the source of each health or voice-science claim, and say when the evidence is small.

**Scope.** Path-scoped to the documents, the README, the resource file, and the Help window.
