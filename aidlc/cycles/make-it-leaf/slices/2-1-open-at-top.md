---
type: slice
description: Every way a document is loaded leaves the view at its first line and the cursor at the start.
status: done
effort: high
---

# Slice 2-1: open-at-top

## Context

Spec, *Opening at the top*: whenever a document is loaded into a window -- from the
command line or file manager, through the open dialog (including into a window
scrolled down in another document), by reloading after an outside change, or by
recovering after a crash -- the view shows its first line at the top and the cursor
sits at the very start. Nothing else about scrolling changes. Phase 2, *Opens at the
top*; it leaves behind tests of where the view lands after a load, for the outline
cycle to extend.

## Intent

All four load paths end in the core replacing the editor document's text. The
suspected cause: replacing the text leaves the editor's cursor at the end of the new
text, and the window's rule that keeps the cursor in view then scrolls to the bottom.
Confirm this with failing tests, then make a load put the cursor at the start and the
view at the top, in one place every load path passes through, without changing how
the view follows the cursor while typing.

## Done when

New tests for opening, opening into a scrolled window, reloading and recovering pass;
the whole suite passes; and a long file opened by hand in `build/leaf` starts at its
first line, with wheel, keys and typing scrolling as before.

## Tasks

- [x] Point the test suite's data folder at a temporary directory so recovery files written by tests land
  in a test directory, not the owner's -- files: `tests/tst_leaf.cpp`. Why: the
  recovery test writes a real snapshot.
- [x] Name the editor's scroll area so tests can read its position -- files:
  `src/Main.qml`. Why: the tests assert the view's position.
- [x] Write the failing tests: open a long file; scroll down and open another; reload
  after the file changes on disk; recover a snapshot of a long document -- files:
  `tests/tst_leaf.cpp`. Why: pin down the behaviour and confirm the cause.
- [x] Fix it where every load passes through, so a load leaves the cursor at the start
  and the view at the top -- files: `src/backend.cpp` and/or `src/Main.qml`, decided
  by what the tests show. Why: the spec's behaviour, in one place rather than four.
- [x] Check by hand in the real app on screen -- files: none. Why: the working norm
  for anything visual.

## Tests

Four tests in the existing Qt Test suite, each loading the real interface with a real
backend and real files in a temporary directory: the view's scroll position is zero
and the editor's cursor is at position zero after each kind of load. The recovery test
writes a real snapshot file where the backend looks for one. The suite's standard
data folder is pointed at a temporary directory for the run, so nothing touches the
owner's own data and a failed test cannot leave a snapshot for the next run.

## Documents this could invalidate

None expected. If the fix changes where the view's position is decided, the
architecture's *Data flow* section on opening may need a factual correction. Checked
at close: it already says the window places the view at the first line; no change.

## Notes for the next slice

- Cause confirmed: replacing the document's text left the editor's cursor at the end
  (position 11,500 of 11,500 in the test), and the rule that keeps the cursor in view
  scrolled there. Fixed by the core emitting a "document loaded" signal from the one
  function every load passes through; the window answers by putting the cursor at
  the start and scrolling to the top. Five load paths reach it: open, the open dialog,
  reload, recovery at startup, and the command line.
- Recovery restores inside the editor's creation. The window's signal handler is
  already connected by then, because Qt connects every `Connections` before running
  any creation handler. Reliable, not luck.
- Setting the cursor alone left the view 2 pixels down (the cursor-follow rule keeps a
  margin); the explicit scroll to the top is needed. A mutation run showed all four
  tests catch that.
- Test helpers now exist for loading the real window, writing long documents,
  scrolling to the end and asserting the first line is at the top -- reuse them in
  the outline cycle.
- The test suite's data folder (`XDG_DATA_HOME`) is a temporary directory for the
  run. Earlier runs had created empty `tst_*` folders in the owner's home; they were
  removed.
- Checked by hand: a 60-section file opened at its first line; Ctrl+End and Page Up
  still move the view with the cursor; closing left the file byte-for-byte unchanged.
