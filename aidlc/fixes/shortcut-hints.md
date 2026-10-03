---
type: slice
description: A faint, always-on line of the main shortcuts in the bottom-right corner, and a complete Ctrl+? list.
status: done
effort: light
---

# Fix: shortcut hints

## Context

The owner forgets the keys, Ctrl+J above all. Leaf has a list of them behind Ctrl+?
(`shortcutsDialog` in `src/Main.qml`), but nothing on screen says so, so that is
forgotten too. The list is also stale: it leaves out Undo, Redo and Ctrl+G.

To see it: open any file. Nothing on screen names a key.

Right looks like: a faint line in the bottom-right corner, always there, reading
`Ctrl+J outline · Ctrl+E read/edit · Ctrl+F find · Ctrl+? all keys`. Ctrl+? opens the
full list, which now names every shortcut Leaf has.

Approaches put to the owner: a corner card that opens on click (recommended), this
always-on line, or a footer button for the existing popup. The owner chose the
always-on line.

## Intent

Add a `Label` to the footer area of `src/Main.qml`, anchored bottom-right, matching
`footerStatus` on the left: same muted colour, mono font, 11 px scaled, and the same
0.55 opacity and margins. It is fixed text, the same in both views. It hides when the
window is too narrow for it to clear `footerStatus`, rather than overlapping it or
being cut. The `shortcutsDialog` text gains Undo (Ctrl+Z), Redo (Ctrl+Shift+Z /
Ctrl+Y) and Next match (Ctrl+G).

No file text changes; this is only drawn.

## Done when

- The hint line shows in the bottom-right corner in both the reading and the editing
  view, in both themes, and does not overlap the save/open buttons or the status text.
- In a narrow window it is hidden rather than overlapping.
- Ctrl+? lists every `Shortcut` in `src/Main.qml`.
- The suite passes.

## Tasks

- [x] Window test: the hint line exists (`objectName: "shortcutHints"`), is visible at
  the default size, names Ctrl+J, Ctrl+E, Ctrl+F and Ctrl+?, and lies right of
  `footerStatus` without overlap; hidden at a narrow width. Fails first -- files:
  `tests/tst_leaf.cpp`. Why: the regression test comes first.
- [x] Test that the shortcuts dialog names Ctrl+Z, Ctrl+Y and Ctrl+G -- files:
  `tests/tst_leaf.cpp`. Why: keeps the full list from going stale again.
- [x] Add the hint `Label` bottom-right, styled like `footerStatus`, with a `visible`
  rule against the footer's right edge -- files: `src/Main.qml`. Why: the line itself.
- [x] Add Undo, Redo and Next match to `shortcutsDialog` -- files: `src/Main.qml`.
  Why: the line points there.
- [x] Check on screen, both views, light and dark, wide and narrow, with the
  temporary-test `grabWindow()` route from AGENTS.md. Why: it is visual.

## Tests

Window tests in `tests/tst_leaf.cpp` against the real `Main.qml` loaded with a real
`Backend`: find `shortcutHints` and `footerStatus`, compare their geometry in window
coordinates, and resize the window to test the narrow case. The dialog test reads the
real dialog's label text.

## Documents this could invalidate

- None. Checked `docs/design.md` and `docs/architecture.md`: neither describes the
  footer closely enough to be made wrong by this. A design line about the corner hint
  was left out as optional.

## Notes for the next slice

- Window tests that check positions must set the window width themselves: an earlier
  test leaves a saved size behind, and the corner test passed alone but failed in the
  suite until it set 1280 itself.
- At the minimum width (720) with a file open, the hint hides; at 1100 and wider it
  shows. Checked on screen in both views and both themes.
