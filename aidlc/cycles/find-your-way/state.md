---
type: state
description: Leaf gets an outline pane to jump between sections from the keyboard, and four stat cards across the top, both fed by one scan of the text.
status: active
opened: 2026-09-27
mode: run
---

# State: find-your-way

## Where we are

Phase 1 done: slices 1-1 and 1-2 closed. Phase 2 not started; 2-1 outline-pane not yet detailed.

## Next

Detail and implement slice 2-1 outline-pane (execute-slice). Effort: high.

## Done

- 2026-09-27 1-1 structure-scan
- 2026-09-27 1-2 stat-cards

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
- Phase 1 is committed on the `foundation-docs` branch, as the owner asked. Later
  work goes on the same branch unless the owner says otherwise.
