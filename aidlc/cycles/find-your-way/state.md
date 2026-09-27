---
type: state
description: Leaf gets an outline pane to jump between sections from the keyboard, and four stat cards across the top, both fed by one scan of the text.
status: active
opened: 2026-09-27
mode: run
---

# State: find-your-way

## Where we are

Phase 2 done: all five slices closed. The spec's verification holds (see 2-3's notes). Not yet closed as a cycle or reconciled.

## Next

Close the cycle with close-work, and raise the rendered-Markdown question with the owner there.

## Done

- 2026-09-27 1-1 structure-scan
- 2026-09-27 1-2 stat-cards
- 2026-09-27 2-1 outline-pane
- 2026-09-27 2-2 reading-mark
- 2026-09-27 2-3 outline-keyboard

## For a cold session

- The findings came from a brainstorm with the owner on 2026-09-27, run by start-work.
  The owner chose approach B: one shared line scan that the outline, the stats and the
  highlighter all use, as the architecture describes. The smaller option, a heading
  finder for the outline and stats only, was turned down because it would leave the
  highlighter drawing `#` lines inside code as headings.
- The owner asked for simple shortcuts: two keys, not three. Ctrl+J was agreed.
- The owner is wary of anything touching files beyond their own edits. Nothing in
  this cycle writes to the file; say so plainly whenever it comes up.
- A newly launched Leaf window takes keyboard focus, and the owner's typing can land
  in it (seen in 1-1). Capture on-screen checks straight after launch and close the
  window; do not leave test windows open.
- Phases 1 and 2 are committed on the `foundation-docs` branch, as the owner asked.
  Later work goes on the same branch unless the owner says otherwise.
- After 2-1 the owner asked whether Leaf can display files as rendered Markdown,
  finding the styled raw text inherited from Omawrite "ugly" and unpolished. The
  design rules out a separate reading view and chose styled raw text (approach A),
  so this is a design question, not a slice. Options were put to the owner: A polish
  the styled text, B add a rendered reading view with Qt's Markdown support and a key
  to switch to editing (recommended), C a web renderer. The owner chose to finish this
  cycle first and decide after it closes; B would replace the planned "tables and
  code blocks" cycle. Raise it again at close-work.
- Scope change 2026-09-27: Enter in the outline jumps but keeps focus in the outline
  (see `scope-changes.md`). Asked whether Escape should also stop returning to the
  text, the owner approved the slice without asking for that; Escape stays.
