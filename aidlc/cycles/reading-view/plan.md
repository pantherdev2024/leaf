---
type: plan
description: Build the reading renderer and show it as the view every file opens in, with tables and code drawn, then make the outline, switching, find and printing work there.
---

# Plan: reading-view

## Approach

Phase 1 makes the reading view real: a renderer in the C++ core turns the text into a
restyled, throwaway document, the window shows it read-only where every file opens,
and Ctrl+E swaps it with today's editor; tables and code blocks are then drawn as the
spec says. Phase 2 makes the rest of Leaf work in it: the outline, the jump and the
mark matched to the rendered headings, switching views at the same place, find, and
printing through the same renderer, ending with the spec's verification. The order
puts the unknowns first -- how Qt's importer copes with real files, how fast it is,
and how far its document can be restyled -- so a shortfall is raised before anything
is built on it. Reused: Qt's Markdown importer (printing already uses it), the
structure scan's front-matter and fence rules, the backend's theme colours, dark mode
and text size, `openExternalUrl` for links, and the window's outline, mark, find bar
and smooth scrolling. New: the reading renderer, the reading view in the window, and
view-aware versions of the jump, the mark and find.

## Phases

### Phase 1: A page to read

Every file opens as a finished page, tables and code included, and Ctrl+E switches
to the editing view and back. Done when: opening `docs/architecture.md`,
`docs/design.md` and the aidlc plugin's `workflow/brainstorm.md` shows them rendered
and restyled in both themes, with nothing lost after tag lines, tables as grids and
code as labelled boxes, and Ctrl+E switching both ways. Effort: high.
Leaves behind: the **reading renderer** in the core -- text and a style (theme
colours, dark or light, text size, column width) in; a restyled document and the
list of its rendered headings (level, plain text, where each sits in the document)
out; the display-copy rule (front matter and tag lines left out as blank lines)
testable on its own. In the window, the **reading view** showing that document
read-only, and one property saying which view is shown, which every later feature
reads.

- [x] 1-1 reading-renderer -- Build the renderer: the display copy, Qt's import, and
  restyling of headings, paragraphs, lists, task lists, quotes, rules, inline code,
  links and images (local only) from a style; measure its speed on a 5,000-line file
  and check nested lists in tables and other HTML on real files. Done when: tests
  show the display copy and restyled output as the spec describes, including a list
  and a table after `<context>` coming through whole, and the speed and the real-file
  findings are recorded, with any shortfall raised. Depends on: none.
- [x] 1-2 reading-view-window -- Show the renderer's document in a read-only view
  where every file opens, and add Ctrl+E: opening, reloading and recovery show the
  reading view at its start, a new empty window opens in editing, returning to
  reading shows unsaved edits, typing and editing keys do nothing there, text can be
  selected and copied, web and mail links open, and a theme, mode or text-size change
  restyles it live. Until phase 2, picking an outline entry, Ctrl+J and Ctrl+F from
  the reading view switch to the editing view and work there as today, so nothing
  breaks between slices. Done when: in Leaf, in both themes, those behaviours are seen on
  screen and the suite covers opening, switching and rebuilding. Depends on: 1-1.
- [ ] 1-3 tables-and-code -- Draw tables as grids with a shaded header row, the file's
  column alignment, wrapping cells and empty cells for short rows, and code blocks as
  shaded monospaced bands with kept spacing, wrapped lines and a small language
  label. Done when: tests check borders, header shading, alignment and code shading
  and labels in the rendered document, and on screen a wide table wraps, a text
  diagram keeps its shape and a labelled code block shows its label. Depends on: 1-2.

### Phase 2: Find your way on the page

The outline, switching at the same place, find and printing all work in the reading
view. Done when: the spec's Verification holds in full. Effort: medium.
Leaves behind: one way to ask for a heading's place in whichever view is shown, used
by the jump, the mark, switching and find.

- [ ] 2-1 outline-in-reading -- Match outline entries to rendered headings in order by
  level and plain text, and make clicking, Ctrl+J with the arrows, Enter and Escape,
  and the reading mark work in the reading view; unmatched entries jump nowhere and
  are never marked. Done when: tests cover matching, including a heading with marks
  and an underlined heading that does not shift it, and on screen the click, Ctrl+J
  and the mark work in the reading view. Depends on: 1-3. Effort: high.
- [ ] 2-2 switch-in-place -- Keep the place across Ctrl+E: the same marked section
  and the same share of it, the editor's cursor at the first line in view, its undo
  history kept, and Ctrl+E added to the Ctrl+? list. Done when: tests show switching
  keeps the marked section and leaves the text and undo history untouched, and on
  screen a switch mid-section lands at the same place both ways. Depends on: 2-1.
- [ ] 2-3 find-in-reading -- Find in the reading view: Ctrl+F matches the text as
  shown and highlights it there without touching the editor, Ctrl+G and the buttons
  step and scroll, the bar keeps its query across Ctrl+E and searches again, and
  Ctrl+H switches to editing with replace open. Done when: tests cover matching and
  highlighting in the rendered document and the query surviving a switch, and on
  screen find works in the reading view. Depends on: 2-2.
- [ ] 2-4 print-and-verify -- Print through the renderer in light colours from either
  view, unsaved edits included, and run the spec's verification. Done when: a page
  printed to a file shows the reading view's styling without front matter or tag
  lines, and every check in the spec's Verification passes, including a file opened,
  read in both views, searched and closed being byte-for-byte unchanged. Depends on:
  2-3.

## Notes

- **Biggest risk: how far Qt's document can be styled.** Header shading, cell
  borders, code bands and heading spacing are ordinary text formats, but the
  language label has no format and has to be drawn over the view, and block quotes
  cannot have a side bar. Slice 1-1 finds out early; anything that falls short is
  raised with the owner rather than worked around.
- **Second risk: speed.** The reading view is rebuilt on every return from editing
  and on every theme change. Slice 1-1 measures it against the spec's aim; if it is
  slow, the owner decides before 1-2 builds on it.
- **Tag lines with attributes** such as `<example type="x">` are in the spec's rule
  and are tested in 1-1, since Qt swallows content after them too.
- A QML text view owns its own document, so the renderer's output is attached to it
  or built into it; which is decided when 1-2 is detailed. Find today highlights
  through the editor's highlighter and must not in the reading view; printing today
  imports the raw text and must go through the renderer.
- Assumed, not said in the spec: prose in the reading view is set in a proportional
  font (iA Writer Quattro S, installed beside the editor's iA Writer Mono S) and code
  in iA Writer Mono S. The owner confirms the look on screen in 1-2.
- Assumed: the renderer lives in the C++ core so the suite can test it, as the
  outline and stats do; the window only shows its result.
- Foundation documents this plan expects to touch: `docs/architecture.md` if the
  renderer's shape or the label drawing differs from what it says, and at close the
  design's reconciling list in the spec. New source files go in `leaf.pro` and
  `tests/tests.pro`; any new QML goes in `src/resources.qrc`.
