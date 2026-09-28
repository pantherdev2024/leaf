---
type: state
description: Leaf opens every file in a rendered reading view, built from Qt's own Markdown support and restyled, with real table grids and shaded code boxes, and Ctrl+E to switch to the editable text.
status: active
opened: 2026-09-27
mode: run
---

# State: reading-view

## Where we are

All slices done: phase 2 closed with 2-4, and the spec's verification run (see 2-4's
notes). docs/architecture.md reconciled. Everything is committed.

## Next

The owner checks what a session cannot -- a web link opening, copying from the page,
a live theme change, the running app -- and settles the spec's open question on
showing which view is active. Then close the cycle: `close-work` on `reading-view`.

## Done

- 2026-09-27 1-1 reading-renderer
- 2026-09-27 1-2 reading-view-window
- 2026-09-27 1-3 tables-and-code
- 2026-09-27 2-1 outline-in-reading
- 2026-09-27 2-2 switch-in-place
- 2026-09-27 2-3 find-in-reading
- 2026-09-27 2-4 print-and-verify

## For a cold session

- Brainstorm decisions (2026-09-27): always opens in the reading view; Ctrl+E toggles
  to editing and back; front matter hidden in the reading view; tables as grids with
  a shaded header, column alignment, wide tables wrap in cells; code blocks shaded,
  monospace, wrapped, small language label; outline, jump, reading mark, Ctrl+J work
  in both views and switching keeps the place; stat cards unchanged; editing view
  stays as today; Ctrl+F finds in the reading view, replace only in editing; links
  clickable; theme followed; printing prints the reading view. Approach 1 (Qt's
  importer, restyled).
- The look: the owner chose on 2026-09-27 to keep today's style (iA Writer Duo S,
  today's sizes) rather than the T3 Code look, and to fix the bullets; 1-3 did so.
- Two scope changes (`scope-changes.md`): the display copy closes void HTML tags such
  as `<br>`; a file opens with the keyboard focus in the outline.
- No stopgaps left: the outline, Ctrl+J, the mark, Ctrl+E's place and find all work in
  the reading view; Ctrl+H switches to editing by design.
- Printing goes through the renderer in light colours since 2-4; the header shade is
  drawn by the window on screen and set on the cells for paper.
- Judge the look only from the real window, never from the offscreen harness in
  `/tmp/mdreal`. AGENTS.md says how to capture it headless, light theme included.
