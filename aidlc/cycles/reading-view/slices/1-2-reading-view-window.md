---
type: slice
description: The window shows the renderer's page read-only where every file opens, and Ctrl+E switches between it and the editor.
status: done
effort: high
---

# Slice 1-2: reading-view-window

## Context

Phase 1, "A page to read". Serves the spec's *Opening*, the switching part of
*Switching views* (keeping the exact place is 2-2), *What the reading view shows* on
screen, *Theme*, the keys part of *Find and other keys*, and *Links*. Builds on 1-1's
renderer, which fills a document it is given from text and a style.

## Intent

Add a read-only text view, the reader, beside the editor inside the same scrolling
area, so the wheel physics, scroll bar and layout are shared, with one of the two
shown at a time by a window property saying which view is shown. The backend gets
the reader's document once, and renders the current text into it on request, with a
style it builds from its theme colours, text scale and the file's URL. The window
asks for a render when a document is loaded (and then shows the reading view at its
start), when Ctrl+E switches to reading, and when the theme, the mode or the text
size changes while reading. A new window with no file stays in editing. In the
reading view the editing shortcuts are off, links open through the existing
web-and-mail-only hand-off, and the pointer changes over a link. Until phase 2, the
outline, Ctrl+J, Ctrl+F and Ctrl+H switch to the editing view first and work there as
today, and nothing is marked in the outline while reading; switching keeps the
scroll position as a share of the page, which 2-2 replaces with the exact place.

## Done when

In Leaf, in both themes, a file opens as a rendered page at its start, Ctrl+E
switches both ways, unsaved edits show on return, links open, typing and editing keys
do nothing while reading, text can be selected and copied, and a theme change
restyles the page; the suite covers opening, switching, rebuilding after edits and
reloads, and the inert editing keys.

## Tasks

- [x] Give the backend the reader's document and a render request, building the
  style from the theme foreground and accent, a shade mixed from foreground and
  background, the scaled body size, the prose and code fonts and the file's URL --
  files: `src/backend.h`, `src/backend.cpp`. Why: the core owns the text and the
  theme; the window only asks.
- [x] Add the reader to the window beside the editor, the property for which view is
  shown, and the scroll area's height following whichever is shown -- files:
  `src/Main.qml`. Why: one scrolling area keeps Leaf's scrolling feel in both views.
- [x] Show the reading view at the start of every loaded document, keep a new empty
  window in editing, and re-render on theme, mode and text-size changes while
  reading -- files: `src/Main.qml`. Why: the spec's *Opening* and *Theme*.
- [x] Add Ctrl+E: into editing with focus in the text; into reading with a fresh
  render; enabled once there is text; the scroll kept as a share of the page -- files:
  `src/Main.qml`. Why: the spec's *Switching views*, short of the exact place.
- [x] In the reading view: turn off the editing shortcuts; make the outline, Ctrl+J,
  Ctrl+F and Ctrl+H switch to editing first; clear the mark; open links through the
  backend and show the link pointer -- files: `src/Main.qml`. Why: nothing may break
  between slices, and the spec's *Links* and keys.
- [x] Tests, then check on screen in both themes, and measure how long a switch into
  reading takes on a 5,000-line file in the real window -- files:
  `tests/tst_leaf.cpp`. Why: the done-when, and the spec's speed aim.

## Tests

Against the real window loaded headless with a real backend, as the existing window
tests do:

- Opening a file shows the reader and hides the editor, and the reader's document
  holds the rendered text without `#` marks or front matter.
- A window with no file opens in editing.
- Ctrl+E switches to editing and back; an edit made in editing shows in the reader
  after switching back, and the editor's text is unchanged by the switches.
- Reloading after an outside change re-renders the reader with the new text.
- While reading, Ctrl+B and Ctrl+Z leave the editor's text unchanged.
- A text-size change while reading re-renders at the new size.

## Documents this could invalidate

`docs/architecture.md`: the *Window* component and *Switching views* flow, if the
reader and editor sharing one scrolling area, or the render being asked for by the
window, needs saying. `AGENTS.md`: none expected.

## Notes for the next slice

- **Speed:** the first render in the window took 3.5 s on a 5,000-line file, because
  the text view re-laid the page after each of the renderer's many format changes.
  The renderer now applies its restyle inside one edit block: open and render
  204 ms, a switch into reading 144 ms, in the headless window. A suite test counts
  the document's changes (fails with 163 without the edit block) so it cannot
  quietly come back. Keep any later restyling (1-3) inside that edit block.
- **Block backgrounds are not drawn by Qt Quick's text view.** Character backgrounds
  (inline code) and table borders show; a block's background (the quote shade set in
  1-1) does not. For 1-3's code bands, and quotes, use a frame around the run
  (frame background, padding, border), which the text view does draw, as it does
  table frames. Until then quotes are set apart only by their indent.
- Existing editing-view tests (mark, Ctrl+J, jump) now switch to editing first with a
  `showEditingView` helper, since a loaded document opens in reading.
- Interim behaviour, replaced in phase 2: nothing is marked while reading; the
  outline, Ctrl+J, Ctrl+F and Ctrl+H switch to editing first; opening the reading
  view closes the find bar; switching keeps the scroll as a share of the page, and
  the editor's cursor is not moved to the view (2-2). Keyboard scrolling (arrows,
  Page Up and Down) does nothing in the reading view; the spec does not ask for it,
  but the owner may notice.
- Focus goes back to whichever view is shown through one window function,
  `focusText`; use it rather than focusing the editor directly.
- On screen (dark theme, Hyprland, half-width window): brainstorm.md, the
  architecture and an edge-case file render as pages; Ctrl+E switches both ways.
  The owner then tried it themselves and was good with it (2026-09-27); the light
  theme, links and copying were theirs to check, as a session cannot safely switch
  the desktop theme, open the browser or drag-select.
- **Judge the look only from the real window.** The throwaway offscreen harness
  (`/tmp/mdreal`) draws differently from Qt Quick's text view (block backgrounds,
  colours, width), and a picture from it misled the owner once. To capture the real
  window without taking focus, a temporary test that loads the window headless,
  resizes it, opens a file and saves `grabWindow()` works (removed after use).
- **Owner feedback on the look (2026-09-27):** the real window's list bullets are
  tiny square dots that sit low and float between margin and text, and the owner
  is not seeing the polish they expected. A T3 Code style mock-up was shown (system
  sans, semi-bold headings at 1.1-1.4x, 150% lines, links without underline, tables
  with row rules only, bordered code boxes); the owner has not chosen it yet.
- **Owner feedback on the stopgap:** Ctrl+J leaving the reading view surprised the
  owner. Bringing 2-1 forward, before 1-3, was offered.

