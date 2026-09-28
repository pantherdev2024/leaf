---
type: spec
description: Every file opens in a rendered, restyled reading view with real table grids and shaded code boxes, and Ctrl+E switches to the editable text and back, keeping the place.
status: approved
---

# Spec: reading-view

## Goal

Give the owner a finished page to read. Every file opens in a reading view: the
Markdown drawn as a page, with no `#` marks, `---` lines or fences showing,
hard-wrapped lines joined back into paragraphs, tables as grids and code as shaded
boxes. Ctrl+E switches to the editable text, as Leaf shows it today, and back, keeping
the place. The outline, the jump, the reading mark and find work in both. This comes
now because the owner reads these files daily and finds the styled raw text
unpleasant to read; the last two cycles fixed finding one's way, and the look is what
is left. It takes in the planned tables-and-code-blocks cycle, since in the reading
view tables and code are part of the same page.

## Design fit

Uses from `docs/design.md`: the **Reading view**, **Editing view**, **Formatted
block**, **Heading**, **Outline** and **Stats** concepts; the **Open and read**,
**Jump to a section** and **Make a small edit** flows; approach D; and the constraint
that Leaf never rewrites a file. The design and architecture were changed for this
cycle, with the owner's agreement, before this spec (2026-09-27).

Adds to the design, for reconciling at close:

- What counts as a tag line, and that such lines are left out of the reading view.
- A new, empty window opens in the editing view, since it has nothing to read.
- In the reading view, the editing shortcuts (bold, italic, link, undo, redo) do
  nothing, and Ctrl+H switches to the editing view to replace.
- Text can be selected and copied in the reading view.
- Printing prints the reading view's page.

Nothing here contradicts the design or the architecture as changed.

## Behaviour

Nothing in this cycle writes to the file. The reading view is built from the text,
shown, and rebuilt or thrown away; nothing typed or clicked there changes the text.

### Opening

- Opening a file -- from the file manager, a terminal or inside Leaf -- shows it in
  the reading view, scrolled to its start. So do reloading after an outside change and
  recovering after a crash.
- The keyboard focus then starts in the outline, on the first entry (or the marked
  one), as after Ctrl+J; a file with no headings leaves it on the page (scope change,
  2026-09-27).
- The stat cards and the outline are as they are today: counted and listed from the
  file's text.
- A new, empty window (Ctrl+N, or Leaf started with no file) opens in the editing
  view, ready to type. Ctrl+E there works as anywhere else, once there is text to
  read.
- A recovered document opens in the reading view like any other, marked unsaved as
  today; Ctrl+E reaches the recovered text to save or change it.

### Switching views

- Ctrl+E switches from the reading view to the editing view, and back. It works from
  the text, the outline and the find bar, and does nothing while a dialog is open.
- The place is kept both ways. The section marked in the outline before the switch is
  still the marked section after it, and the view is the same distance through that
  section, as a share of its length, as the two layouts allow. Before the first
  heading, the same holds for the part of the file above it.
- On switching to the editing view, focus goes to the text, with the cursor at the
  start of the first line in view. The editor keeps its undo history and unsaved edits
  across switches.
- On switching back to the reading view, it shows the text as it now is, unsaved
  edits included.
- Ctrl+E is added to the shortcuts list the window shows with Ctrl+?.

### What the reading view shows

- Headings at sizes that step down by level, with no `#` marks; paragraphs with space
  between them, with hard-wrapped lines joined; bulleted, numbered and nested lists;
  task-list checkboxes; block quotes set apart from the prose; bold, italic, inline
  code and links styled; horizontal rules as a line.
- The text column keeps the width and the body text size the editing view uses.
- **Tables.** A grid with thin lines around every cell, a bold header row on a light
  shade, and each column aligned as the file marks it (left, centre or right; left
  when not marked). A table wider than the column wraps the text inside its cells;
  it is never cut off and never scrolls sideways on its own. A row with fewer cells
  than the header is shown with the missing cells empty.
- **Code blocks.** A shaded band the width of the column, with space above and below
  its text, in a monospaced font. Long lines wrap inside it. Spacing inside the block
  is kept exactly, so diagrams drawn in text keep their shape. When the opening fence
  names a language (`` ```python ``), the name shows as a small, dim label at the
  top right of the block. The code is not coloured by language.
- **Front matter**, found by the same rule the outline uses, is not shown.
- **Tag lines** are not shown. A tag line is a line, outside a code block, whose only
  content, apart from spaces, is a single opening or closing tag: `<` or `</`, a name
  that starts with a letter and holds letters, digits, `_` or `-`, then optionally
  attributes, then `>`. For example `<task>`, `</output_format>` and
  `<example type="bad">`. The lines between an opening and a closing tag are shown as
  ordinary Markdown. Inside a code block, tag lines are shown as code.
- **Void HTML tags** written without a closing slash -- `<br>`, `<hr>`, `<img ...>`,
  `<input>` and the other HTML elements that never close, in any case -- are shown as
  if written with one (`<br/>`), outside code blocks and inline code. Qt otherwise
  drops everything after them. The file is not touched (scope change, 2026-09-27).
- Each left-out line counts as a blank line, so the paragraphs, lists and tables on
  either side of it stay apart rather than joining.
- Anything else Leaf does not style specially is shown as Qt reads it. Every line of
  ordinary Markdown text reaches the page. Other HTML is left to Qt: an HTML comment
  (`<!-- -->`) does not show, and HTML that is not a tag line may be drawn as Qt
  chooses.
- Images from a path on this machine, relative to the file's folder, are shown if
  they load; anything else, web images included, shows the image's alternative text.
  Nothing is fetched from the network.

### Theme

- The reading view uses the theme's colours for text, headings, links, shading and
  lines, follows dark and light mode, and uses the desktop text size, as the editing
  view does.
- When the theme, the mode or the text size changes while a file is open, the reading
  view restyles at once and keeps the place.

### The outline, the jump and the mark in the reading view

- The outline is listed from the text, as today. Each entry is matched to its heading
  in the reading view in order, by level and by its text as the outline shows it
  (Markdown marks removed), compared with runs of spaces treated as one.
- Clicking an entry, or Enter on it after Ctrl+J, brings that heading to the top of
  the reading view, or as near as scrolling allows, and marks it, as in the editing
  view. Ctrl+J, the arrows, Enter and Escape behave as they do there; Escape and Ctrl+J
  return focus to the reading view.
- The reading mark follows scrolling by the same rule as the editing view: the last
  heading whose top edge is at or above the top of the view.
- A heading the reading view shows that the outline does not list -- one underlined
  with `===` or `---` -- is shown as a heading but is not in the outline. An outline
  entry with no match in the reading view does nothing when picked, rather than jump
  to the wrong place, and is never marked; the mark stays on the last matched heading
  at or above the top of the view.

### Find and other keys in the reading view

- Ctrl+F opens the find bar. Matches are found in the text as the reading view shows
  it, without Markdown marks, highlighted in the reading view; Ctrl+G and the bar's
  buttons step through them, scrolling each into view. Finding in the reading view
  never touches the editing view's text or styling.
- When the find bar is open and Ctrl+E switches views, the bar stays open with its
  query, and the matches are found again in the view now shown.
- Ctrl+H switches to the editing view and opens find and replace there.
- Text can be selected with the mouse and copied with Ctrl+C, as the text it shows.
- Typing, and Ctrl+B, Ctrl+I, Ctrl+K, Ctrl+Z and Ctrl+Y, do nothing in the reading
  view.
- Ctrl+S, Ctrl+Shift+S, Ctrl+O, Ctrl+N, Ctrl+P, fullscreen and Ctrl+? work as in the
  editing view. The warnings about unsaved work and outside changes are unchanged.

### Links

- Clicking a link in the reading view opens it when it is a web or mail link, handed
  to the desktop as today. Any other link does nothing. The pointer changes over a
  link.

### Printing

- Ctrl+P, from either view, prints the current text, unsaved edits included, rendered
  as the reading view renders it: with its tables and code blocks, in light colours
  whatever the theme, and without front matter or tag lines.

## Not in scope

- Editing in the reading view, or remembering a view per file.
- Colouring code by language.
- Rounded code boxes, and tables that scroll sideways. They are beyond what the chosen
  approach draws; a later cycle may draw such blocks by hand.
- Headings made by underlining in the outline.
- Showing front matter in the reading view, as text or as a table.
- Changes to the editing view other than Ctrl+E.
- Fetching anything from the network, images included.
- Anything that writes to the file other than the owner's own save.

## Verification

- The test suite passes, with new checks that:
  - the display copy leaves out front matter and tag lines, keeps tag lines inside
    code blocks, and keeps everything else, with the lines between tags rendered
    (a list and a table after `<context>` come through whole);
  - the rendered page has tables with borders, a shaded header and the file's column
    alignment, and code blocks shaded, monospaced and labelled with their language;
  - outline entries are matched to rendered headings in order, an underlined heading
    does not shift the matching, and an unmatched entry jumps nowhere;
  - switching views keeps the marked section and does not touch the text or its undo
    history;
  - the reading view shows unsaved edits after switching back, and is rebuilt after a
    reload;
  - a left-out tag line between two paragraphs leaves them as two paragraphs;
  - a `<br>` in a table cell leaves the rest of the file on the page, and a `<br>`
    inside inline code or a code block is shown as written;
  - a heading with bold or inline code in it is matched to its rendered heading;
  - a web image is not fetched.
- On screen, in Leaf, in dark and light themes:
  - `docs/architecture.md`, `docs/design.md` and
    `~/Projects/talos/plugins/aidlc/workflow/brainstorm.md`, which has `<task>` lines,
    front matter and a table, read as finished pages, with no Markdown marks
    showing, and nothing missing after the tag lines;
  - a file with a wide table wraps inside its cells; a code block shows its label and
    keeps a text diagram's shape;
  - Ctrl+E switches both ways at the same place; the outline click, Ctrl+J and the
    mark work in the reading view;
  - Ctrl+F finds and highlights in the reading view; a web link opens in the browser;
  - a theme change restyles the reading view while it is open;
  - a printed page (to a file) shows the reading view's styling.
- A file opened, read in both views, searched and closed without edits is
  byte-for-byte unchanged on disk.

## Open questions

- Whether the window should show which view is active -- for example a faint word in
  the footer -- so typing into the reading view is not a surprise. Resolved on screen,
  with the owner.
- The exact look: heading sizes, spacing, shading strength, the label's placement.
  Resolved on screen during the cycle, with the owner's approval.
- How well Qt's importer handles nested lists inside tables, HTML other than tag
  lines, and very large files. Checked on real files early in the cycle; anything
  that falls short is raised rather than worked around.
- How fast the reading view is built. Aim: switching views on a 5,000-line file
  feels immediate, well under a quarter of a second. Measured in the first phase; if
  it is slower, raised with the owner.
