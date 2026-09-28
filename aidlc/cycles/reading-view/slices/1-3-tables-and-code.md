---
type: slice
description: Tables become shaded-header grids with the file's alignment and whole rows, code blocks become shaded, labelled boxes that wrap and keep diagrams whole, quotes get their shade, and list bullets are drawn at the text's size.
status: done
effort: high
---

# Slice 1-3: tables-and-code

## Context

Phase 1, "A page to read", last slice. Serves the spec's *What the reading view
shows*: **Tables** and **Code blocks**, and the block quotes being set apart. Builds
on 1-1's renderer and 1-2's reading view. The owner chose on 2026-09-27 to keep
today's style (iA Writer Duo S, today's sizes and spacing) rather than move to the
T3 Code look, and to fix the list bullets, which are tiny and sit low.

What the real window shows today (captured headless at 1898x1032): tables are
already grids with bold headers and aligned body cells, but the header row is not
shaded, header cells are all left-aligned, the lines are a mid grey, and a short row's
one cell is stretched across the whole row with no inner lines. Code blocks have no
shade, no label, and fenced lines never wrap, so the architecture diagram runs past
the column. Quotes have no shade (a block's own background is not drawn).

Found in Qt's source (6.11.2) for how the text view draws:

- A frame's background is drawn over its whole box, padding included. A border on a
  frame that is not a table crashes the view (it assumes a table), so code and quote
  boxes get no border.
- A block's *character* format background is drawn behind the block's lines; a
  table cell's own background is not drawn.
- A list bullet is drawn in the block's character format font, which the renderer
  never sets, so it falls back to the application's small default font.
- The importer spans a short row's cells into one, and leaves header cells
  left-aligned whatever the column says.

## Intent

The renderer, still in its one edit block, also: bolds each table's header row, aligns each header cell as its column, splits a short row back into its
cells so the missing ones show empty, draws the grid in a faint line colour, and
spaces the table from the prose around it. It gathers each run of code lines into
a frame with the shade as background and some padding, the full column wide, lets
long lines wrap, and puts the fence's language, when named, as a small dim
right-aligned line at the top of the box. A text diagram keeps its shape by fitting
rather than wrapping: when a code block's longest line would not fit the column at
the code size, the block's font is made just small enough to fit, down to a floor
(about three quarters of the code size); only past the floor do lines wrap. For that
the style carries the column width, and the window re-renders when the column width
changes. Quote runs get the same shaded frame as code. Every block's character
format is set to the prose font, size and colour, so bullets are drawn at the
text's size and line up with it.

The label is part of the rendered document rather than drawn over the view: the
spec asks only for a small dim label at the top right, and in the document it
scrolls, lays out and prints with the box for free. It is copied with a selection.

## Done when

Tests check, in the rendered document: header shading, header bold and alignment,
a short row's empty cells, code blocks inside a shaded frame, a language label on a
labelled block and none on an unlabelled one, a wide block shrunk to fit and a
normal one not, quotes in a shaded frame, and list blocks carrying the prose font;
the restyle still reaches a view as one change. On screen, in the real window: a
wide table wraps in its cells, the architecture diagram keeps its shape, a labelled
code block shows its label, and bullets sit at the text's size.

## Tasks

- [x] Tables: header row bold, aligned as the column's body cells; short rows split
  into single cells; grid in a faint line colour, cell padding, and space above and
  below the table; the header row's place reported for the window to shade (changed
  during the slice: a block's shade covers only its text, not the cell) -- files:
  `src/readingrenderer.cpp`, `src/readingrenderer.h`, `src/backend.*`,
  `src/Main.qml`. Why: the spec's *Tables*; the importer leaves the header plain and
  merges short rows.
- [x] Code: each run of code lines moved into a shaded, padded, full-width frame;
  lines allowed to wrap; the language label added as the frame's first line, small,
  dim and right-aligned -- files: `src/readingrenderer.cpp`. Why: the spec's *Code
  blocks*; block backgrounds are not drawn, frame backgrounds are.
- [x] Diagrams keep their shape: the style gets the column width; a code block whose
  longest line is wider than the box at the code size gets a smaller font, just
  enough to fit, not below the floor -- files: `src/readingrenderer.h`,
  `src/readingrenderer.cpp`. Why: wrapping a diagram breaks it, and the spec says it
  keeps its shape and is never cut off.
- [x] The backend takes the column width with the render request, and the window
  passes the reader's width and re-renders, after a short pause, when that width
  changes while reading -- files: `src/backend.h`, `src/backend.cpp`, `src/Main.qml`.
  Why: fitting depends on the width, which a window resize or text size changes.
- [x] Quotes in a shaded frame like code (replacing the block background the view
  never draws) and every block's character format set to the prose font, size and
  colour so bullets are drawn at the text's size -- files: `src/readingrenderer.cpp`.
  Why: quotes are to be set apart, and the owner asked for the bullets fixed.
- [x] Tests, then check on screen in the real window (headless capture) with
  `docs/architecture.md`, `docs/design.md` and a table-and-code sample, in dark and
  light -- files: `tests/tst_leaf.cpp`. Why: the done-when.

## Tests

Renderer tests on a real `QTextDocument` with the real fonts, as 1-1's are:

- A table's header cells are bold and body cells are not; the renderer places the
  header row around the header's lines and above the body's.
- A header cell over a right-aligned column is right-aligned.
- A row with one cell under a three-column header has three cells, the last two
  empty.
- A code block's lines sit in a frame with a background; a prose block does not.
- A `python` fenced block has a first line reading `python`, smaller than the code;
  an unlabelled block has none, and the code lines are unchanged.
- With a column width given, a block with an 80-column line gets a smaller font than
  a block with a short line, and no line wider than the box; without one, no
  fitting.
- A quote sits in a shaded frame.
- A list item's block character format has the prose font at the body size.
- The existing one-change test covers the new restyling, and the 5,000-line speed
  test still passes.

Window test: the reader re-renders when its width changes (fitted font follows).

## Documents this could invalidate

`docs/architecture.md`: the *Reading renderer* component (the style now carries the
column width, frames for code and quotes, the label in the document) and the note
that the window rebuilds the page on a width change. `docs/design.md`: none expected.
`AGENTS.md`: a gotcha worth adding -- a border on a non-table frame crashes the text
view, and block backgrounds go through the character format.

## Notes for the next slice

- **Drawing, as found in Qt 6.11's source and on screen.** The text view draws a
  frame's background over its whole box, margins included, so boxes are spaced by
  the empty blocks the document keeps around every frame (set to a fixed height), not
  by frame margins. A frame inserted around blocks takes the first block's format for
  the empty block in front of it; the renderer puts it back. A border on a frame that
  is not a table crashes the view. A block's *character* background is drawn, but only
  behind its lines, not the cell padding, which is why the header shade is a
  rectangle drawn by the window (`readingHeaderRows`) rather than a format. A frame
  put inside a table cell brings empty blocks with it and spoils the column widths.
- **The header shade is outside the document.** The window refreshes it after every
  render and when the page's height changes. 2-4's printing goes through the
  renderer only and will print headers bold but unshaded unless it draws the rows
  too (or uses Qt's widget-side painter, which does draw cell backgrounds -- set a
  cell background there only for printing).
- **The label is text in the document**, on the box's first line. Copying a
  selection copies it, and 2-3's find will match it unless find skips it (it is the
  only block in a code frame with no `BlockCodeLanguage`). It does not affect
  `headings()`.
- **Block numbers moved.** Frames add empty blocks, so a heading's block number is
  only good in the document it came from. 2-1 should match with `headings()` on the
  rendered document, as planned, not with numbers worked out from the text.
- **Fitting.** A code block shrinks to fit the column only if it fits at no less than
  three quarters of the code size; otherwise it wraps at the code size. The
  architecture diagram and the plugin's Markdown example both shrink about two
  pixels. The window re-renders 150 ms after the reader's width settles.
- **Bullets** come from the desktop's `sans-serif` font (Liberation Sans here), sized
  so their baseline meets the prose; iA Writer Duo S's own bullet is a small square.
  Numbers stay in the prose font and, being wide in Duo S, hang a few pixels left of
  the column; they are still drawn whole. iA Writer Duo S itself is not bundled with
  Leaf (only Mono S is); it comes from the system.
- **Boundaries between code blocks.** Qt marks none, so two code blocks in a row are
  split only when their language or fencing differs. Two unlabelled fenced blocks
  separated only by a blank line share one box.
- A code block inside a quote gets no box of its own; it is monospaced inside the
  quote's box. A table with only a header row shows a thin empty row under it (Qt's
  import).
- **Speed:** switching into reading and drawing a 5,130-line file with 270 tables and
  270 code blocks took 190 ms in the headless window (1-2 measured 144 ms without the
  draw). The one-change test now includes tables, code, quotes and lists.
- **Seen on screen** (headless capture of the real window, 1898x1032): a sample with
  aligned, short-row and wide tables, labelled code, the diagram and a quote; the
  architecture document in dark and light; the plugin's `schema/slice.md`; a
  5,000-line file; lists, tasks and code in lists and quotes. The owner has not yet
  seen this slice's look in the running app.
