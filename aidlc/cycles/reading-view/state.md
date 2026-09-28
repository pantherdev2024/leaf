---
type: state
description: Leaf opens every file in a rendered reading view, built from Qt's own Markdown support and restyled, with real table grids and shaded code boxes, and Ctrl+E to switch to the editable text.
status: closed
opened: 2026-09-27
closed: 2026-09-27
mode: run
---

# State: reading-view

## Where we are

Closed 2026-09-27. All eight slices done and the spec's verification run; the
foundation documents reconciled.

## Next

Nothing. The cycle is closed.

## Done

- 2026-09-27 1-1 reading-renderer
- 2026-09-27 1-2 reading-view-window
- 2026-09-27 1-3 tables-and-code
- 2026-09-27 2-1 outline-in-reading
- 2026-09-27 2-2 switch-in-place
- 2026-09-27 2-3 find-in-reading
- 2026-09-27 2-4 print-and-verify

## For a cold session

- What was built: every file opens in a rendered reading view (Qt's Markdown importer,
  restyled by `src/readingrenderer.*`) with table grids, shaded and labelled code
  boxes, shaded quotes and round bullets; Ctrl+E switches to the editable text at the
  same place; the outline, Ctrl+J, the mark and find work on the page; printing goes
  through the same renderer in light colours; a file opens with the keyboard in the
  outline.
- Two scope changes (`scope-changes.md`): void HTML tags such as `<br>` closed in the
  display copy; the keyboard focus starts in the outline on open.
- The spec's open questions: a label for the view shown -- dropped by the owner
  (2026-09-27), the two views look different enough; the exact look -- the owner kept
  today's style over the T3 Code look; Qt's importer on real files -- fine, apart from
  unclosed void tags (scope change); speed -- a switch into reading on a 5,000-line
  file with tables and code draws in about 190 ms.
- Verified by a session: the suite (103 tests), headless captures of the real window
  in both themes, a PDF print, a live theme change, the file unchanged byte for byte.
  Not checked by the owner at close: a web link opening in the browser, and copying
  text from the page.
- Follow-up for the owner to place: the package does not depend on `ttf-ia-writer`,
  which provides the page's text font; the README now says so.
- Qt Quick's text view limits shaped the design: no block or cell backgrounds (frames,
  and window-drawn rectangles for header shade and find), and a crash on bordered
  non-table frames. See 1-3's notes and AGENTS.md.
