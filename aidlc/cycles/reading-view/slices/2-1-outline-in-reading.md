---
type: slice
description: Outline entries are matched to the rendered headings, so clicking, Ctrl+J and the reading mark work in the reading view without switching to editing.
status: done
effort: high
---

# Slice 2-1: outline-in-reading

## Context

Phase 2, "Find your way on the page", first slice. Serves the spec's *The outline,
the jump and the mark in the reading view*. Today (1-2's stopgap) picking an entry,
Enter after Ctrl+J, and Ctrl+J itself switch to the editing view first, and nothing
is marked while reading; the owner found Ctrl+J leaving reading surprising. The
phase leaves behind one way to ask for a heading's place in whichever view is
shown, which 2-2 (switching in place) and 2-3 (find) build on.

## Intent

The core matches the outline, which is listed from the text, to the rendered
headings (`ReadingRenderer::headings`) in order: for each entry, the first rendered
heading not yet used, at or after the last match, with the same level and the same
text once runs of spaces count as one. Rendered headings the outline does not list
(underlined `===` / `---` headings) are passed over; an entry with no match gets
none and does not move the search on. The backend publishes, for each outline entry,
where its heading starts in the reading view's document, or -1, and republishes after
every render and every outline change.

The window gets one way to ask where outline entry *i* sits in the view shown
(`headingY(i)`, undefined when it has no place there), used by the jump and by the
mark. In the reading view the jump scrolls the page to the matched heading and
marks it; an unmatched entry does nothing. The mark follows scrolling by the same
rule as editing, over matched entries only. Ctrl+J opens the outline without leaving
reading, and Escape, Ctrl+J and a click return focus to the page. The stopgap that
switched to editing goes.

## Done when

Tests cover the matching -- in order, a heading with bold and inline code matched,
an underlined heading not shifting later matches, an unmatched entry getting none
while later ones still match -- and, in the window, a click and Enter in the reading
view scrolling to the heading and marking it while staying in reading, an unmatched
entry doing nothing, Ctrl+J and Escape keeping the reading view, and the mark
following scrolling there. On screen, the click, Ctrl+J and the mark work in the
reading view.

## Tasks

- [x] Match outline entries to rendered headings -- files: `src/readingrenderer.h`,
  `src/readingrenderer.cpp`. Why: the rule belongs with the rendered headings, in
  the core, where the suite can test it without a window.
- [x] Publish each outline entry's place in the reading document, recomputed after a
  render and after the outline changes -- files: `src/backend.h`, `src/backend.cpp`.
  Why: a reload renders before the recount updates the outline, so both must
  refresh it.
- [x] One `headingY(i)` for the view shown; the jump and the mark use it, in reading
  over matched entries only; the mark is refreshed after a render and on the
  reader's width change -- files: `src/Main.qml`. Why: the phase's *leaves behind*,
  used by 2-2 and 2-3.
- [x] Remove the stopgap: the jump and Ctrl+J no longer switch to editing, and the
  mark is no longer cleared while reading -- files: `src/Main.qml`. Why: the spec;
  the owner's feedback on Ctrl+J.
- [x] Tests, then check on screen in the reading view -- files: `tests/tst_leaf.cpp`.
  Why: the done-when.

## Tests

- Core, on a real `QTextDocument` rendered by the renderer and the outline titles
  from `Backend::outlineTitle`: in-order matching; a heading `Use **bold** and
  `code`` matched; `Under\n===` between two headings not shifting the second; a
  heading `A &amp; B` (rendered as `A & B`) unmatched while the next heading matches.
- Window, headless with a real backend, on a long file: clicking an entry while
  reading keeps `reading` true, scrolls the heading to the top and marks it; Enter in
  the outline does the same; an unmatched entry leaves the scroll and the mark as
  they were; Ctrl+J focuses the outline and Escape returns focus to the reader, still
  reading; scrolling the page to a heading marks it. Existing editing-view tests
  still pass unchanged apart from the stopgap they relied on.

## Documents this could invalidate

`docs/architecture.md`: the *Window* component or data flow if they describe the
jump and mark as editing-only. `AGENTS.md`: none expected.

## Notes for the next slice

- **The one question** is `headingY(index)` in `Main.qml`: where outline entry
  `index` starts in the view shown, in the scrolling area's coordinates, or
  `undefined` when the reading view has no heading matched to it. The jump and the
  mark use it; 2-2 should note the marked entry and its share of the section with it
  in the view being left and restore with it in the view shown.
- The backend's `readingHeadingPositions` holds each entry's character position in
  the reading document (or -1), re-matched after every render and every outline
  change. It changes before `outlineChanged` is emitted, so the window's handlers see
  both together.
- **Bug found and fixed:** while reading, the hidden editor still scrolled the view to
  its own cursor whenever that cursor's rectangle changed (seen after a focus change
  in a narrow window). `ensureCursorVisible` now does nothing while reading. Tests
  that scrolled by moving the editor's cursor now scroll the view (`scrollToEnd`).
- **The page is rebuilt once the reader's width settles** after the window lays out,
  150 ms later, which moves the headings. Window tests that measure places on the page
  wait for it with `waitForPageToSettle` (it watches the `readerWidthSettled` timer).
- Eight editing-view tests had relied on the stopgap to switch them into editing; they
  now call `showEditingView` like the others. In the reading view the first heading is
  marked at the top of a file with front matter, since the front matter is not on the
  page; in editing the lines above it still leave nothing marked.
- An entry the page shows differently -- such as `A &amp; B`, drawn as `A & B` -- stays
  unmatched: it jumps nowhere and is never marked. The spec asks exactly for that; if
  the owner finds it in practice, entities and backslash escapes could be decoded
  before comparing.
- **Added after closing, at the owner's ask (scope change 2026-09-27):** a file opens
  with the focus in the outline (`focusOutline`, shared with Ctrl+J). Tests that
  pressed Ctrl+J straight after opening now press Escape first or expect the outline.
- Ctrl+F and Ctrl+H still switch to editing (2-3). Switching with Ctrl+E still keeps
  only the scroll share (2-2).
- Seen on screen (headless capture of the real window): a jump in the architecture
  document while reading puts the heading at the top and marks its entry. The owner
  has not tried it in the running app yet.
