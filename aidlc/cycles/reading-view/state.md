---
type: state
description: Leaf opens every file in a rendered reading view, built from Qt's own Markdown support and restyled, with real table grids and shaded code boxes, and Ctrl+E to switch to the editable text.
status: active
opened: 2026-09-27
mode: run
---

# State: reading-view

## Where we are

Phase 2, slice 2-1 closed and docs/architecture.md reconciled. 2-2 switch-in-place not
yet detailed. 1-3 and 2-1 are not committed yet.

## Next

Detail and execute slice 2-2 switch-in-place: `execute-slice` on `reading-view`.
Effort medium (the phase default), lower than 2-1's high, so the owner may switch
model first.

## Done

- 2026-09-27 1-1 reading-renderer
- 2026-09-27 1-2 reading-view-window
- 2026-09-27 1-3 tables-and-code
- 2026-09-27 2-1 outline-in-reading

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
- Still stopgaps until 2-2 and 2-3: Ctrl+F and Ctrl+H switch to editing first, and
  Ctrl+E keeps only the scroll share. The outline, Ctrl+J and the mark work in the
  reading view since 2-1.
- Read 2-1's notes before 2-2: `headingY(index)` is the one way to ask where a
  heading is in the view shown. Read 1-3's notes too: block numbers in the rendered
  document include the empty blocks frames add; the code label is document text that
  find (2-3) will match; the header shade is drawn by the window, so printing (2-4)
  must draw it itself.
- Judge the look only from the real window, never from the offscreen harness in
  `/tmp/mdreal`. AGENTS.md says how to capture it headless, light theme included.
