---
type: slice
description: The core exposes the outline from its recount, and a fixed-width pane on the left lists it and jumps to a heading on click, with the text column fitted beside it.
status: done
effort: high
---

# Slice 2-1: outline-pane

## Context

Serves the spec's *The outline pane* (all but the reading mark), *Jumping to a
section* with the mouse and *The jump*, and *Layout*, in phase 2, "Find your way". The
phase leaves behind the outline as the core exposes it -- entries with level, display
text and position -- and the pane's jump. Slice 2-2 adds the mark that follows
reading; 2-3 the keyboard route. Slice 1-2's notes: keep the headings from the same
recount rather than scanning again, and re-anchor the cards and the text column to the
space beside the pane.

## Intent

Today the core's recount runs the structure scan and keeps only the number of
headings. This slice keeps the headings themselves as the outline: for each, its
level, the text it shows and the position of its line. The display text drops the
closing run of `#` after a space and the markers for bold, italic, inline code and
links, found by the highlighter's own inline rules; inline code's rule, today private
to the highlighter's styling, is shared for this. The window gains a pane of a set
width down the left, below the cards, listing the entries indented by level, long
ones ending in "…", an empty one shown as a faint "Untitled heading", and a faint "No
headings" when there are none. Clicking an entry places the cursor at the start of
the heading's line, scrolls so that line is at the top of the view, or as near as the
end of the file allows, and gives the text focus. The text column's reading width is
worked out from the space beside the pane, and the cards line up above the text
column. The pane takes no keyboard focus yet.

## Done when

Tests check the outline's entries -- levels, display text, positions -- after a load
and after an edit, the pane's list and its "No headings", and where a jump lands,
including a heading near the end; on screen, clicking entries in a long file lands
each heading at the top, in dark and light themes, and the owner has approved the
pane's look.

## Tasks

- [x] Share the highlighter's inline code rule as a static function beside the inline markup, used by its own styling -- files: `src/markdownhighlighter.h`, `src/markdownhighlighter.cpp`. Why: the outline strips inline code markers by the same rule the styling uses.
- [x] Keep the outline from the recount: a display-text function for a heading, and an outline property of entries (level, title, position) with its change signal, set on load and after typing -- files: `src/backend.h`, `src/backend.cpp`. Why: the pane shows what the core computes, from the same scan as the figures.
- [x] Add the outline pane: fixed width on the left below the cards, entries indented by level, elided, "Untitled heading" and "No headings", its own scroll bar, clicking an entry jumps -- files: `src/Main.qml`. Why: the pane itself.
- [x] Add the jump: cursor at the heading line's start, the line scrolled to the top of the view (clamped at the end), focus to the text -- files: `src/Main.qml`. Why: the spec's *The jump*, and the cursor-follow scrolling must not pull the view off the heading.
- [x] Fit the text column and the cards to the space beside the pane -- files: `src/Main.qml`. Why: the reading width is worked out from the whole window today.
- [x] Write the tests -- files: `tests/tst_leaf.cpp`. Why: the display text rules and the jump's landing are where this breaks.
- [x] Check on screen in dark and light themes with a long LLM file, and get the owner's approval of the pane's look.

## Tests

In the existing Qt Test suite, against real objects:

- The display text rules, directly: plain text unchanged; a closing `##` after a space
  dropped; `C#` kept; bold, italic, inline code and link markers dropped; an empty
  heading and a heading of only closing `#`s giving empty text.
- A real `Backend` opening a real file: the outline's levels, titles and positions,
  front matter and fenced `#` lines left out; after an edit in the real window, the
  outline gains the new heading.
- The real window, headless: the pane lists one row per heading, shows "No headings"
  for a file without any; a jump to a heading in the middle of a long file leaves its
  line at the top of the view and the cursor at its start; a jump to the last heading
  of a long file scrolls as far as it can.

## Documents this could invalidate

- `docs/architecture.md` -- *Reading and jumping* says picking an entry scrolls the
  heading to the top; it should still hold. The layout description may need the
  cards' placement over the text column.

## Notes for the next slice

- The outline is `Backend::outline`, a list of maps {level, title, position} set by
  the same `recount` as the figures, with `outlineChanged`. `position` is the heading
  line's first character. `Backend::outlineTitle` makes the display text; it marks
  characters to drop rather than removing spans in turn, because markers overlap (an
  `_x_` inside a link's address is also italic). The review caught this.
- `MarkdownHighlighter::inlineCodeSpans` is the shared inline code rule, used by the
  styling and the outline.
- The jump is `win.jumpToHeading(position)` in `Main.qml`: cursor first, then the
  scroll, then focus. Reversed, `onCursorRectangleChanged` -> `ensureCursorVisible`
  scrolls two lines of margin off the heading. The view snaps to whole pixels, so
  tests allow one pixel.
- `editorWidth` is also capped at `editorFlick.width`: on the laptop screen at 1.5
  scale a tiled Leaf window is narrower than the pane plus the 360 minimum, and the
  text was cut off on its left. A test sets a 600 wide window.
- Outside the plan's tasks: the card values now shrink to fit their card
  (`fontSizeMode: Text.HorizontalFit`), since the cards sit over the narrower text
  column. The look is otherwise as the owner approved in 1-2. The cards are centred
  over `editorFlick`, not the window.
- ListView delegates, like Repeater delegates, are not found by `findChild`; the test
  helper `outlineEntry` walks the visual items from the window's content item.
- When the outline changes (a heading typed or edited), the ListView gets a new model
  and scrolls its list back to the top. 2-2's "keep the mark in view" will cover
  this; check it there.
- The pane takes no keyboard focus; 2-3 adds it. The ListView's `currentIndex` is
  unused so far and is the natural hook for both the mark and keyboard selection --
  but they are different things (the mark follows reading; the selection moves with
  the arrows while the text stays still), so 2-3 must not let one overwrite the other.
- The owner tried the clicks on 2026-09-27 and said it behaves well. They asked for
  smaller body text: `editorFontPixelSize` went from 20 to 17 at text scale 1 (tests
  updated). They then asked about showing the file as rendered Markdown, calling the
  look inherited from Omawrite "ugly" and unpolished; that question reaches the design
  (which rules out a separate reading view) and is raised with the owner before 2-2.
