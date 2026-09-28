---
type: slice
description: Ctrl+E keeps the place -- the same marked section and the same share through it -- with the editor's cursor at the first line in view and its undo history kept.
status: done
effort: medium
---

# Slice 2-2: switch-in-place

## Context

Phase 2, "Find your way on the page". Serves the spec's *Switching views*: the place
kept both ways, focus and cursor on entering editing, undo history kept, Ctrl+E in
the Ctrl+? list; and *Theme*'s "restyles at once and keeps the place". Today (1-2's
stopgap) a switch keeps only the scroll position as a share of the whole page, so
the two layouts' different heights put the view in a different section. 2-1 left
`headingY(index)`, where an outline entry's heading is in the view shown.

## Intent

The window gets a place: the section at the top of the view (the outline entry
`headingAtTop` gives, -1 above the first heading) and how far through it the view
is, as a share of the section's length -- from its heading to the next heading the
view shows, or to the end of the page. It notes the place in the view being left
and brings the view shown to the same section and share once the new layout exists,
marking that section. Entering editing, it puts the cursor at the start of the line
at the top of the view before scrolling, as the jump does, so the cursor's own
scrolling does not pull the view away. A section the reading view has no heading for
falls back to the nearest matched section before it. The same place replaces the
scroll share wherever the page is rebuilt while shown (theme, mode, text size,
width) and in the stopgaps for Ctrl+F and Ctrl+H.

## Done when

Tests show a switch each way keeps the marked section and the share through it,
also above the first heading, puts the editor's cursor at the first line in view,
and leaves the text and its undo history untouched; the Ctrl+? list shows Ctrl+E.
On screen, a switch mid-section lands at the same place both ways.

## Tasks

- [x] A place in the view shown -- section and share -- and restoring it, with the
  section marked -- files: `src/Main.qml`. Why: the spec's *Switching views*; built on
  `headingY`, so both views answer the same way.
- [x] Ctrl+E, Ctrl+F and Ctrl+H use the place instead of the scroll share; entering
  editing puts the cursor at the first line in view -- files: `src/Main.qml`. Why:
  the spec; the scroll share lands in the wrong section.
- [x] Rebuilding the page while shown keeps the place -- files: `src/Main.qml`. Why:
  the spec's *Theme*, and the width re-render from 1-3.
- [x] Ctrl+E in the Ctrl+? list -- files: `src/Main.qml`. Why: the spec.
- [x] Tests, then check on screen -- files: `tests/tst_leaf.cpp`. Why: the done-when.

## Tests

Against the real window, headless, with a real backend and a long headed file:

- Reading, scrolled halfway through a section, Ctrl+E: editing marks the same
  section and its view is halfway through that section there; the cursor is at the
  start of the line at the top of the view. Back again: the same.
- Above the first heading, in a file with a long preamble: the share through the
  preamble is kept and nothing is marked.
- An edit, then two switches: the text is as edited and undo takes the edit back.
- The shortcuts dialog's text lists Ctrl+E.

## Documents this could invalidate

`docs/architecture.md`: *Switching views* already describes the place kept by
section; check it matches (share of the section, the fallback, rebuilds). `AGENTS.md`:
none expected.

## Notes for the next slice

- **The place** is `currentPlace()` / `restorePlace(place)` in `Main.qml`, built on
  2-1's `headingY`. It replaced the scroll share everywhere: Ctrl+E, the Ctrl+F and
  Ctrl+H stopgaps, and rebuilding the page for a theme, text size or width change.
  2-3 can use it to keep the place when the find bar switches views.
- **The very top is its own place.** Above the first line the view sits in the page's
  top padding, above the first heading; restoring that as "the start of section 0"
  snapped the view down onto the heading, so a view at contentY 0 stays at 0.
- The restored section is marked and held, like a jump, so it stays marked even
  where the layout cannot scroll that far (near the end); the next scroll lets the
  usual rule take over.
- Entering editing puts the cursor at the start of the line at the top of the view
  before scrolling, as the jump does; the cursor's own scrolling would otherwise pull
  the view up by its two-line margin.
- `showEditing(false)`, used by tests, still switches without moving anything.
- Tests: halfway through a section each way (within 2 px in editing, 2% of the
  section back in reading), halfway through a long preamble, an edit and its undo
  surviving two switches, Ctrl+E in the Ctrl+? list. The first two were checked to
  fail with the share broken.
- Seen on screen (headless capture of the real window): the architecture document at
  40% through "Data flow" in reading, then editing, then reading again, lands on the
  same paragraph each time with "Data flow" marked. The owner has not tried it in the
  running app.
