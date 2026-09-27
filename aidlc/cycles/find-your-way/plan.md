---
type: plan
description: Build the shared structure scan and the stat cards first, then the outline pane, its reading mark and its keyboard route.
---

# Plan: find-your-way

## Approach

Build the scan first, inside the first slice that needs it, so the styling uses it
from day one and nothing ever disagrees about what a heading is. The stat cards come
next: they are the smaller change to the window, and they put the scan's results on
screen with the recount the word count already has. The outline follows in three
steps -- the pane and clicking to jump, then the mark that follows reading, then the
keyboard route -- each usable on its own. Reused: the word count's 120 ms recount
after typing, the highlighter's inline rules for stripping markers from outline
entries, the load path that already puts every document at its first line, and the
test suite that drives the real window headless. New: the line scan, the four
figures, the outline, the cards and the pane.

## Phases

### Phase 1: One scan, and the figures at a glance

The file's shape is read once and shown across the top. Done when: in Leaf, the four
cards show correct words, lines, tokens and sections for a real LLM file and update
after an edit; the corner word count is gone; a `#` line inside a code block or front
matter is drawn plain; the test suite passes. Effort: medium.
Leaves behind: the structure scan -- text in, each line's kind and the headings (level,
text, position) and front matter extent out -- used by both the core and the
highlighter; the core's single recount on the typing delay, which produces the
figures and the headings together; the band across the top of the window that the
text sits below.

- [x] 1-1 structure-scan -- write the line scan to the spec's rules (headings, fences, front matter, unclosed fences), and make the highlighter use it so `#` lines inside code or front matter are not drawn as headings, restyling when a fence or the front matter's closing line changes. Done when: new scan and styling tests pass, and a file with a `#` line in a code block shows it plain on screen. Depends on: none. Effort: high.
- [x] 1-2 stat-cards -- compute words, lines, tokens and sections in the core from one recount, show them as four cards across the top, and remove the corner word count. Done when: tests check each figure, including an empty file, a trailing line break and front matter; the cards read correctly on screen in dark and light themes and update after an edit. Depends on: 1-1.

### Phase 2: Find your way

The owner reaches any section from the outline, by mouse or keyboard. Done when: the
spec's verification holds -- the test suite passes with checks for headings, figures,
styling and updates; on screen, in dark and light themes, a long LLM file shows its
outline and correct cards with no corner count, front matter stays out of the
outline, a file with no headings shows "No headings", a `#` line in a code block is
plain and left out, Ctrl+J with the arrows, Enter and Escape work as specified and a
picked heading lands at the top, clicking an entry jumps to it, scrolling moves the
mark and a long outline keeps it in view; and a file opened, read, jumped around in
and closed without edits is byte-for-byte unchanged. Effort: medium.
Leaves behind: the outline as the core exposes it (entries with level, display text
and position), and the pane's jump, which later cycles do not change.

- [x] 2-1 outline-pane -- expose the outline from the core, show it in a fixed-width pane on the left with indents, "…", "Untitled heading" and "No headings", fit the text column beside it, and jump on click with the heading at the top and the cursor at its start. Done when: tests check the outline's entries and a jump's landing; clicking entries in a long file on screen lands each heading at the top. Depends on: 1-2. Effort: high.
- [x] 2-2 reading-mark -- mark the heading being read as the view scrolls, mark a picked heading after a jump even when it cannot reach the top, and keep the mark in view in a long outline. Done when: tests check the mark after scrolling and after a jump near the end; on screen, the mark follows scrolling in a long file. Depends on: 2-1.
- [x] 2-3 outline-keyboard -- Ctrl+J into and out of the outline from the text or the find bar, arrows to move without moving the text, Enter to jump, Escape to return, nothing with no headings or with a dialog open, and Ctrl+J in the shortcuts list. Done when: tests drive each key; on screen, a section of a long file is reached without the mouse. Depends on: 2-2.

## Notes

- Risk: the highlighter styles one line at a time, top to bottom, but whether a
  leading `---` is front matter depends on a closing line further down. The expected
  shape is for the core to tell the highlighter the front matter's extent after each
  recount and restyle when it changes; 1-1 settles it. Hence effort high.
- Risk: placing the cursor on a heading makes the editor scroll to keep the cursor in
  view, which may fight the jump's own scrolling. 2-1 settles it against the wheel
  and cursor-follow code inherited from Omawrite.
- Risk: the reading width of the text column is worked out from the whole window
  width today, and the scroll bar is inset to clear the corner word count. Both change
  in 1-2 and 2-1.
- Assumed: the stats and the outline come from the same recount, so they cannot
  disagree about the number of headings. The spec requires the agreement; the single
  recount is how.
- Assumed: the look of the cards and the pane is settled on screen in 1-2 and 2-1
  with the owner, as the spec's open question says.
- Documents this plan expects to touch at close: `docs/design.md`, which takes the
  spec's additions and loses the open questions they answer; `docs/architecture.md`,
  whose *Reading and jumping* and *Unusual documents* leave the focus keys and the
  no-headings and front matter behaviour to this cycle.
