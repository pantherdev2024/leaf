---
type: slice
description: Ctrl+J moves focus between the text and the outline, where the arrows move a selection without moving the text, Enter jumps and Escape returns.
status: done
effort: medium
---

# Slice 2-3: outline-keyboard

## Context

Serves the spec's *Jumping to a section* from the keyboard, and the Ctrl+J line in the
shortcuts list, in phase 2, "Find your way". It is the cycle's last slice, so the
phase's done-when -- the spec's verification -- is checked when it closes. Slice 2-2
left the mark as `win.markedHeading`, the jump as `win.jumpToHeading(index)`, and the
ListView's `currentIndex` free for a keyboard selection that must look different from
the mark.

## Intent

A window shortcut, Ctrl+J, works from the text or the find and replace bar: it puts
focus in the outline with the marked entry selected, or the first entry when nothing
is marked. It is off when the file has no headings or while one of Leaf's dialogs is
open. In the outline, the list's own key navigation moves the selection with the up
and down arrows and scrolls the list to keep it in view; the text does not move and
the mark does not change. Enter jumps to the selected heading, which returns focus to
the text. Escape, or Ctrl+J again, returns focus to the text with the cursor and view
as they were. The selection is drawn as an accent outline around the entry, only
while the outline has focus, so it reads apart from the mark's tint. Tab keeps its
meaning in the text. The shortcuts list gains Ctrl+J.

## Done when

Tests drive each key in the real window -- Ctrl+J in and out, from the text and the
find bar, the arrows, Enter, Escape, no headings, a dialog open -- and the full spec
verification holds; on screen, the owner reaches a section of a long file without the
mouse and approves how the selection looks.

## Tasks

- [x] Add the Ctrl+J shortcut, enabled only with headings and no dialog open, entering the outline with the marked or first entry selected, and leaving it when pressed there -- files: `src/Main.qml`. Why: the spec's one key into the outline and back.
- [x] Handle Enter and Escape in the outline, and draw the selection while the outline has focus -- files: `src/Main.qml`. Why: jumping and returning from the keyboard, and a selection that is not confused with the mark.
- [x] Add Ctrl+J to the shortcuts list -- files: `src/Main.qml`. Why: the spec lists it there.
- [x] Write the keyboard tests -- files: `tests/tst_leaf.cpp`. Why: each key has a rule, and focus is easy to lose.
- [x] Check on screen and get the owner's approval; then run the spec's verification for the phase.

## Tests

In the existing Qt Test suite, with key presses sent to the real window, headless:

- Ctrl+J from the text selects the marked entry, or the first entry when nothing is
  marked, and gives the outline focus.
- The arrows move the selection; the view's scroll and the mark stay as they were.
- Enter jumps to the selected heading and gives the text focus.
- Escape and a second Ctrl+J give the text focus with the cursor and scroll
  unchanged.
- Ctrl+J from the find bar enters the outline.
- Ctrl+J does nothing in a file without headings, or while the shortcuts dialog is
  open.

## Documents this could invalidate

- `docs/architecture.md` -- *Reading and jumping* says which keys move focus into the
  outline is decided in this cycle; it can now name Ctrl+J. Left for close-work's
  reconcile, with the design's open question on focus.

## Notes for the next slice

- Ctrl+J is a window `Shortcut` enabled only while `backend.outline` has entries. It
  was first also disabled while any of Leaf's dialogs was open; a test showed the
  modal dialogs (all three are `modal: true`) already keep it from reaching the
  outline, so that guard was removed. The test proves the key works once the dialog
  closes, so the check is not vacuous.
- The outline's selection is the ListView's `currentIndex`, moved by its own key
  navigation. Enter and Escape are `Keys` handlers on the list. The selection is
  drawn as a 1px accent ring only while the list has focus; the mark stays the tint.
- If the outline has focus and a file without headings is loaded (Ctrl+O, or Reload
  from the outside-change dialog), `onCountChanged` returns focus to the text. The
  review found Enter then threw on `backend.outline[-1]`.
- Key tests use `createActiveWindow`, which fails the test if the window never becomes
  active, and `press`.
- Spec verification, as of this slice: suite 53 passing with every check the spec
  lists; on screen in both themes, a long plan file with its outline and cards, front
  matter kept out of the outline, "No headings", and a fenced `#` line plain and not
  listed; clicking and the mark tried by the owner in 2-1 and 2-2; a file opened and
  closed was byte-for-byte unchanged. The keyboard route on screen is left to the
  owner: a session cannot send keys to Leaf without taking focus from the owner's
  window.
- Scope change, 2026-09-27, after the owner tried it: Enter jumps but keeps focus in
  the outline. `jumpToHeading` no longer moves focus; the entry's click handler moves
  it to the text itself. Ctrl+J then returns to the text at the last heading jumped
  to, since the jump placed the cursor there.
- The owner tried the keyboard route on 2026-09-27 and approved it. Escape was left
  returning to the text, as specified; the owner did not ask to change it.
