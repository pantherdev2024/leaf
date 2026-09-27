---
type: state
description: Leaf opens every file in a rendered reading view, built from Qt's own Markdown support and restyled, with real table grids and shaded code boxes, and Ctrl+E to switch to the editable text.
status: active
opened: 2026-09-27
mode: run
---

# State: reading-view

## Where we are

Opened. No spec yet. The brainstorm found this cycle contradicts the foundation:
`docs/design.md` rules out a reading mode and chose approach A, and
`docs/architecture.md` says "No rendered view and no separate reading mode". Those
change first, agreed with the owner, then the spec.

## Next

Agree the design and architecture changes with the owner, then write the spec
(`start-work`, write-spec step) from the brainstorm findings.

## Done

## For a cold session

- Came from the find-your-way close, where the owner called the styled raw text
  "ugly". Options A (polish), B (Qt rendered view), C (web renderer) were offered;
  the owner chose B and folded the planned tables-and-code-blocks cycle into this one.
- Brainstorm decisions (2026-09-27): always opens in the reading view; Ctrl+E toggles
  to editing and back; front matter hidden in the reading view; tables as grids with
  a shaded header, column alignment, wide tables wrap in cells; code blocks shaded,
  monospace, wrapped, small language label; outline, jump, reading mark, Ctrl+J work
  in both views and switching keeps the place; stat cards unchanged; editing view
  stays as today; Ctrl+F finds in the reading view, replace only in editing; links
  clickable; theme, dark/light and text size followed; printing prints the reading
  view; XML-like tag lines (`<task>`) hidden; outside changes refresh the view.
- Approach chosen: 1, Qt's Markdown importer into a throwaway document, restyled by
  walking it. Rejected: 2 (Leaf draws each block itself; possible later for parts
  that fall short) and 3 (web engine).
- Evidence gathered: a throwaway renderer (/tmp/mdproto) showed Qt 6.11's importer gets
  structure, tables and front matter right, but lines like `<task>` are read as HTML
  and swallow the lists and tables after them, and the default look is cramped with
  unshaded code. So a display-only cleaned copy is fed to the importer; the owner
  agreed that does not break the invariant, since it is never saved.
