---
type: slice
description: The core computes words, lines, estimated tokens and sections in one recount, and the window shows them as four cards across the top in place of the corner word count.
status: done
effort: medium
---

# Slice 1-2: stat-cards

## Context

Serves the spec's *The stat cards*, *Layout* (the scroll bar and footer keep their
places) and *Keeping up with the text*, in phase 1, "One scan, and the figures at a
glance". The phase leaves behind the core's single recount on the typing delay, which
produces the figures and the headings together, and the band across the top of the
window that the text sits below. Phase 2 builds the outline pane under that band and
reads its headings from the same recount.

## Intent

Today the core keeps one figure, the word count, recounted 120 ms after typing and
set at once on load, and the window shows it faintly in the footer's right corner.
This slice turns that into one recount that runs the structure scan once and sets
four figures together: words (counted as now), lines, estimated tokens (characters
divided by four, halves up) and sections (the number of headings the scan finds). The
same recount keeps asking the highlighter to recheck front matter, as 1-1 left it. The
window shows the figures as four cards -- Words, Lines, Tokens, Sections -- in a row
across the top, numbers with commas and the token figure behind "≈", and the text
area sits below them. The corner word count is removed; the scroll bar keeps clear of
the footer. The cards cannot take focus. Their exact look is settled on screen with
the owner, as the spec's open question says.

## Done when

Tests check each figure -- including an empty document, a trailing line break and
front matter -- straight after a load and after an edit, and the formatting the cards
show; on screen, in dark and light themes, the cards read correctly for a real LLM
file and update after an edit, the corner word count is gone, and the owner has
approved the look.

## Tasks

- [x] Replace the word count with the four figures from one recount: rename the timer and its slots to say recount, run the scan once per recount and once on load, add line and token counting, and expose the four figures to the window with one change signal -- files: `src/backend.h`, `src/backend.cpp`. Why: one recount keeps the figures and, later, the outline in agreement.
- [x] Show the four cards across the top, move the text area below them, format the numbers, remove the corner word count, and update the scroll bar's comment now that the corner count is gone -- files: `src/Main.qml`. Why: the figures readable at a glance, replacing the faint count.
- [x] Write the tests for the figures, their update after an edit, and the cards' text -- files: `tests/tst_leaf.cpp`. Why: the spec names the edge cases, and the cards' formatting is what the owner reads.
- [x] Check on screen in dark and light themes with a real LLM file, and get the owner's approval of the look.

## Tests

In the existing Qt Test suite:

- The counting rules, directly: lines for an empty text, one line with and without a
  final line break, and blank lines in between; tokens rounding halves up and zero for
  an empty text.
- Against a real `Backend` opening a real file in a temporary directory: all four
  figures straight after the load, for a file with front matter, a fenced `#` line
  and headings, so words and lines include the front matter and sections leave out
  the fenced line.
- Against the real window, headless: after typing a heading into the editor, the
  figures and the cards' text update; the cards show commas and the "≈".

## Documents this could invalidate

- `docs/architecture.md` -- the diagram and *Components* already place the stat cards
  at the top and say the stats are recomputed after typing pauses "as the word count
  is today"; that phrase goes stale once the word count is no longer separate.
- `README.md` -- if it mentions the word count in the corner.

## Notes for the next slice

- The owner approved the cards' look on 2026-09-27, after trying the build on their
  own files.
- The recount is `Backend::recount(text)`: one `StructureScan::scan`, then words,
  lines, tokens and sections, set together with one `statsChanged` signal. It runs at
  once on load (on the editor's text, not the file's) and 120 ms after typing via
  `m_recountTimer`. The timer's handler also calls the highlighter's
  `refreshFrontMatter()`; loading does not, because loading restyles everything.
  2-1 should keep the scan's headings from this same recount for the outline rather
  than scanning again.
- `countLines` and `estimateTokens` are static on `Backend`, beside `countWords`.
- The cards are an inline `StatCard` component in `Main.qml`, four declared
  instances in the `statCards` Row. A Repeater was tried first; its delegates are not
  found by `findChild`, so tests could not read them. Each value label has the
  objectName `<name>Value`, which the tests use through `cardText`.
- The row is centred on the window and as wide as `editorWidth`, and the Flickable is
  anchored below it. When 2-1 adds the outline pane on the left, the row and the text
  column both need re-anchoring to the space beside the pane; the spec puts the cards
  above both the outline and the text.
- The card labels use the text colour at 60% rather than `mutedColor`, which was too
  faint on the card in light themes.
- To check a light theme on screen without touching the owner's desktop, launch with
  `HOME` pointing at a temporary directory holding
  `.local/state/omarchy/current/theme/colors.toml` with `mode = "light"`.
- `docs/architecture.md` *Outline and Stats* now describes the single recount instead
  of "as the word count is today".
