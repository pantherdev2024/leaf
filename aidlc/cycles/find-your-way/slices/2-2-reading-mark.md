---
type: slice
description: The outline marks the heading being read as the view scrolls, marks a picked heading after a jump, and keeps the mark in view in a long outline.
status: done
effort: medium
---

# Slice 2-2: reading-mark

## Context

Serves the spec's *The outline pane* rules for the mark and for scrolling a long list,
and the jump's rule that the picked entry becomes the marked one, in phase 2, "Find
your way". Slice 2-1 left the outline as `backend.outline` entries and the jump as
`win.jumpToHeading(position)`. Its notes: the list scrolls back to the top whenever
the outline changes, which this slice must cover; and the ListView's `currentIndex` is
left for 2-3's keyboard selection, which is not the same thing as the mark.

## Intent

The window works out which heading is being read: the last heading whose line's top
is at or above the top of the text as the view shows it, that is the view's top edge
plus the space above the first line, so a file opening on a heading marks it. When the
view is above the first heading, nothing is marked. Since heading lines only move
down the text, the search is a binary search on line positions from the editor's
layout. It runs as the view scrolls, when the outline changes and when the text column
is resized. A jump now takes the entry's index; it marks that entry and holds the mark
there, even when the heading could not reach the top, until the view next scrolls by
some other means. The marked entry is drawn with a tint and the theme's accent, and
the pane scrolls itself to keep it in view, including after the outline changes. The
mark is its own property, not the ListView's current item.

## Done when

Tests check the mark at the top of a file, after scrolling to and just short of a
heading, with the view above the first heading, after a jump near the end and after
the next scroll, and that a long outline keeps the marked entry in view; on screen,
the mark follows scrolling in a long file, and the owner approves how it is drawn.

## Tasks

- [x] Work out the heading at the top by binary search over the outline's line positions, and keep it in a window property, updated on scroll, on outline change and on a resize of the text column -- files: `src/Main.qml`. Why: the spec's marking rule.
- [x] Make the jump take an entry's index, mark it, and hold the mark until the view next scrolls another way -- files: `src/Main.qml`. Why: a heading near the end cannot reach the top, and the spec still marks it.
- [x] Draw the marked entry, and keep it in view in the pane, also after the outline changes -- files: `src/Main.qml`. Why: the mark must be seen, and a long list must follow reading.
- [x] Update the jump tests for the index, and write the mark tests -- files: `tests/tst_leaf.cpp`. Why: the rules have edges -- the first heading, the end of the file, the held mark.
- [x] Check on screen with a long LLM file, and get the owner's approval of how the mark looks.

## Tests

In the existing Qt Test suite, against the real window headless and real files:

- A file opening on a heading marks it; a file whose first heading is further down
  marks nothing at the top.
- Scrolling the view so a heading's line is at the top marks it; a little short of
  it marks the one before.
- A jump to the last heading of a long file marks it though it cannot reach the top;
  scrolling afterwards marks by the usual rule.
- In a file with many headings, scrolling to the end leaves the marked entry inside
  the pane's visible part of the list.

## Documents this could invalidate

- `docs/architecture.md` -- *Reading and jumping* says the window works out which
  heading is at the top and marks it; it should still hold. None expected.

## Notes for the next slice

- The mark is `win.markedHeading` (-1 for none), worked out by `win.headingAtTop()`:
  a binary search over `backend.outline` positions, the reading line being
  `max(contentY, editor.y) + 1` -- the view's top edge, except at the very top of
  the file where the text starts below it. It updates on `editorFlick`'s
  `onContentYChanged`, on the editor's `onWidthChanged` (after rewrapping; a
  window-level handler sees the old layout, per the review), and on
  `outlineChanged`.
- `win.jumpToHeading(index)` now takes the entry's index. It marks the entry after
  its scroll and sets `markHeld`, which `onContentYChanged` clears. The hold matters
  only for an edit: the recount's `outlineChanged` would otherwise re-mark by the
  usual rule without the view moving. It was removed once as redundant and restored
  after the review found that case; `holdsAJumpsMarkThroughAnEdit` guards it.
- The pane keeps the mark in view with `positionViewAtIndex(..., ListView.Contain)`
  on every mark change, and again via `Qt.callLater` after `outlineChanged`, because
  a new model resets the list to its top.
- The mark is drawn as a 10% text-colour tint behind the entry and the entry's text
  in the theme accent. It is not the ListView's `currentIndex`, which stays free for
  2-3's keyboard selection. 2-3 must decide how the selection is drawn so it reads
  differently from the mark.
- `remarksAfterTheTextRewraps` checks the mark after a resize, but in the headless
  window it also passes with the old window-level handler: something else refreshes
  the mark there too. It checks the outcome, not the ordering.
- Tests now call `jumpToHeading` with an index.
- The owner tried it and approved the mark's look and behaviour on 2026-09-27; they
  noted the arrow keys do nothing in the pane yet, which is 2-3.
