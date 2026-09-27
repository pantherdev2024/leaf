---
type: state
description: Leaf opens every file in a rendered reading view, built from Qt's own Markdown support and restyled, with real table grids and shaded code boxes, and Ctrl+E to switch to the editable text.
status: active
opened: 2026-09-27
mode: run
---

# State: reading-view

## Where we are

Phase 1, slice 1-1 done. 1-2 not yet detailed.

## Next

Detail and execute slice 1-2 reading-view-window: `execute-slice` on `reading-view`.
Effort high.

## Done

- 2026-09-27 1-1 reading-renderer

## For a cold session

- Brainstorm decisions (2026-09-27): always opens in the reading view; Ctrl+E toggles
  to editing and back; front matter hidden in the reading view; tables as grids with
  a shaded header, column alignment, wide tables wrap in cells; code blocks shaded,
  monospace, wrapped, small language label; outline, jump, reading mark, Ctrl+J work
  in both views and switching keeps the place; stat cards unchanged; editing view
  stays as today; Ctrl+F finds in the reading view, replace only in editing; links
  clickable; theme followed; printing prints the reading view. Approach 1 (Qt's
  importer, restyled); drawing blocks by hand stays open for anything that falls short.
- One scope change so far: the display copy also closes void HTML tags such as
  `<br>` (`scope-changes.md`), because Qt drops the rest of the file otherwise.
- Prose is set in iA Writer Duo S, not the Quattro S the plan assumed: the installed
  Quattro files are mislabelled and Qt draws its Bold face for regular text. The owner
  has not seen it on screen yet; show them in 1-2.
- For 1-2, read 1-1's notes: the renderer fills the view's own document, the caller
  chooses colours per mode, and still to check on screen is drawing time in the real
  window (render plus layout of 6,840 lines was 78 ms offscreen).
- For the planner notes still standing: find in the reading view needs its own
  highlighting (today's goes through the editor's highlighter); printing today
  imports the raw text and must go through the renderer (2-4).
- A throwaway harness in `/tmp/mdreal` renders a file to a PNG with the renderer
  (`./p <file> <out.png> <max height> light|dark`); rebuild it with qmake6 if /tmp
  was cleared. It is not part of the repo.
- A Leaf window launched from a session may take keyboard focus from the owner's
  window; capture straight after launch, close at once, and send keys only when Leaf
  is the active window.
