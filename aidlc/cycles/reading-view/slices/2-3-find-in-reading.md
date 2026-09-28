---
type: slice
description: Ctrl+F finds in the reading view as the page shows the text, highlighting there without touching the editor, and the find bar keeps its query across Ctrl+E.
status: done
effort: medium
---

# Slice 2-3: find-in-reading

## Context

Phase 2, "Find your way on the page". Serves the spec's *Find and other keys in the
reading view*. Today (1-2's stopgap) Ctrl+F and Ctrl+H switch to editing first, and
the find bar closes when the reading view is shown. Find in editing matches the
editor's text case-insensitively and highlights through the editor's highlighter,
which must not be used for the page. 2-2 left `currentPlace()` / `restorePlace()`.

## Intent

The core finds a query in the reading view's document as shown -- case-insensitive,
within blocks, by Qt's own document search -- and returns each match's start and end
there. The window keeps one list of matches for the view shown: the editor's text in
editing, as today, and the page's in reading. In reading, matches are highlighted by
boxes drawn over the page in the find colours, a little transparent so the text shows
through, for the matches in and near the view only (a common letter can match tens of
thousands of times), rebuilt as the view scrolls or the page is laid out again; the
current match is drawn stronger. Stepping (Ctrl+G, Enter, the bar's arrows) moves the
current match and scrolls it into view by the same margins the editor uses. The
editor's text, selection and highlighter are not touched while reading.

Ctrl+F opens the bar in the view shown. Ctrl+H switches to editing and opens replace.
When Ctrl+E switches views with the bar open, the bar stays open with its query, the
place is kept as 2-2 does, the matches are found again in the view now shown, and the
current match becomes the first one at or below the top of the view, so the switch
does not jump away. Replace is closed on entering reading; it works only in editing.
The code label is text the page shows, so it is found like any other text.

## Done when

Tests cover finding the text as shown (a query across bold marks matches in reading
and not in editing), highlighting in the page without touching the editor, Ctrl+G
stepping and scrolling to a match further down, the query surviving Ctrl+E both ways
with the matches found again, and Ctrl+H switching to editing with replace open. On
screen, find works in the reading view.

## Tasks

- [x] Find in the reading document -- files: `src/backend.h`, `src/backend.cpp`. Why:
  positions must be the page's own, which Qt's document search gives; the page's plain
  text does not line up with them around frames.
- [x] One match list for the view shown; stepping and showing the current match in
  either view; the reading highlight boxes, limited to the view -- files:
  `src/Main.qml`. Why: the spec's matching and highlighting, without the editor's
  highlighter.
- [x] Ctrl+F in reading, Ctrl+H to editing with replace; the bar kept across Ctrl+E
  with the matches found again from the place; the close-on-reading stopgap removed
  -- files: `src/Main.qml`. Why: the spec's keys.
- [x] Tests, then check on screen in both themes -- files: `tests/tst_leaf.cpp`. Why:
  the done-when.

## Tests

Against the real window, headless, with a real backend:

- Reading `Some **bold** text`, the query `bold text` has one match; after Ctrl+E it
  has none (the editor's text has the marks), and after Ctrl+E again one.
- While reading, finding leaves the editor's text and selection as they were, and the
  page shows a highlight box for the current match.
- In a long file with a match far down, Ctrl+G brings it into view.
- Ctrl+H while reading switches to editing with replace open.

## Documents this could invalidate

`docs/architecture.md`: the find flow, if it says find works through the
highlighter only. `AGENTS.md`: none expected.

## Notes for the next slice

- **One match list for the view shown:** `searchMatches` now holds `{start, end}`
  objects in both views (replace uses them too). `findMatches()` searches the editor's
  text or, while reading, `backend.findInReading`, which uses `QTextDocument::find`
  so positions are the page's own.
- **Highlights on the page** are boxes drawn over the reader at half opacity
  (`readingMatchBoxes`), only for matches from a screen above the view to a screen
  below, found by binary search, and rebuilt on scroll and relayout. Drawn over, not
  under, because the code boxes' backgrounds would hide them. A match wrapped across
  two lines gets a box on each; one across three leaves the middle line unboxed.
- **Across Ctrl+E** the bar stays open; `restorePlace` finds again with
  `refindFromView()`, picking the first match at or below the top of the view, so
  the switch does not jump. Entering reading closes replace and clears the editor's
  highlight and selection. Ctrl+F and Ctrl+H reopening with an old query also find
  again from the view (they used to keep stale matches).
- `ensureVisible(top, bottom)` on the scrolling area now serves both the editor's
  cursor and the page's current match.
- The code label is found like any other text on the page.
- Printing (2-4) must not print find's boxes; they are window items, not part of the
  document, so going through the renderer leaves them out anyway.
- Seen on screen (headless capture of the real window, dark and light): "reading" in
  the architecture document highlighted in prose and inside the diagram's box, the
  current match stronger. The owner has not tried it in the running app.
