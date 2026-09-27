---
type: state
description: Leaf gets an outline pane to jump between sections from the keyboard, and four stat cards across the top, both fed by one scan of the text.
status: closed
closed: 2026-09-27
opened: 2026-09-27
mode: run
---

# State: find-your-way

## Where we are

Closed 2026-09-27. All five slices done; the spec's verification performed; the
design and architecture reconciled.

## Next

Nothing. This cycle is closed.

## Done

- 2026-09-27 1-1 structure-scan
- 2026-09-27 1-2 stat-cards
- 2026-09-27 2-1 outline-pane
- 2026-09-27 2-2 reading-mark
- 2026-09-27 2-3 outline-keyboard

## For a cold session

- Delivered: one structure scan shared by the highlighter, the stat cards and the
  outline; four stat cards across the top in place of the corner word count; the
  outline pane with click-to-jump, a mark that follows reading, and Ctrl+J with the
  arrows, Enter and Escape. The body text went from 20 to 17 px at the owner's
  request.
- One scope change: Enter in the outline jumps but keeps focus there
  (`scope-changes.md`).
- Verification: the suite (53) covers every check the spec lists; on screen, both
  themes were captured for the cards, the pane, the mark, "No headings" and a fenced
  `#` line; the owner tried clicking, the mark and the keyboard; a file opened and
  closed was byte-for-byte unchanged.
- Left open at close: the owner finds the styled raw text "ugly" and asked for
  rendered Markdown. Options put to them: A polish the styled text, B a rendered
  reading view with Qt's Markdown support and a key to edit (recommended; it would
  replace the planned tables-and-code cycle), C a web renderer. The design rules out a
  reading view, so B starts by changing the design.
- A Leaf window launched from a session may take keyboard focus from the owner's
  window; their typing once landed in a test file. Capture straight after launch,
  close at once, and send keys only when Leaf is the active window.
