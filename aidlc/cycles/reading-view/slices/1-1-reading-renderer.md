---
type: slice
description: A reading renderer in the core turns the text into a restyled throwaway document, from a display copy with front matter and tag lines left out, and reports its rendered headings.
status: done
effort: high
---

# Slice 1-1: reading-renderer

## Context

Phase 1, "A page to read". Serves the spec's *What the reading view shows* (headings,
paragraphs, lists, task lists, quotes, rules, inline code, links, images, front
matter and tag lines), *Theme* (colours, mode, text size) and the open questions on
how Qt's importer copes with real files and how fast it is. Tables and code blocks
are drawn in 1-3; showing the result in the window is 1-2.

## Intent

Add a reading renderer beside the structure scan: pure C++, no QML, testable
directly. It has three parts. The **display copy** takes the text and blanks out
front matter (by the structure scan's rule) and tag lines outside code blocks (fences
by the scan's rule), line for line, so nothing on either side joins. The **render**
fills a given document from the display copy with Qt's GitHub-dialect importer and
then walks it to restyle it from a style: body and heading fonts and sizes, spacing,
list indents, quote shading, rule colour, inline code, links, and text colour; images
resolve against the file's folder and anything not local shows its alternative text.
It fills a document it is given, rather than returning one, because the window's text
view owns its document (1-2). The **rendered headings** list the document's headings
in order with level, plain text and block number, for the outline matching in 2-1.

## Done when

The suite passes with new tests showing the display copy and the restyled output as
the spec describes, including a list and a table after `<context>` coming through
whole; the speed on a 5,000-line file and the findings on real files (nested lists in
tables, other HTML, the brainstorm workflow file) are recorded below, and any
shortfall is raised with the owner.

## Tasks

- [x] Add the renderer with the display copy: front matter and tag lines (a single
  opening or closing tag, attributes allowed, alone on its line apart from spaces,
  outside a code block) become empty lines -- files: `src/readingrenderer.h`,
  `src/readingrenderer.cpp`, `leaf.pro`, `tests/tests.pro`. Why: Qt's importer
  swallows the Markdown after a tag line; blank lines keep neighbours apart.
- [x] Render into a given document from the display copy and a style (colours,
  dark or light, body pixel size, prose and code font families, the file's folder),
  restyling headings (sizes stepping down by level, bold, space above and below),
  paragraphs and list items (line height, space between), nested list indents,
  task-list checkboxes, block quotes (indent and a light shade), horizontal rules,
  inline code (code font, light shade), links (accent colour, underline) and body
  text colour -- files: `src/readingrenderer.*`. Why: Qt's default look is cramped
  and ignores the theme.
- [x] Resolve images against the file's folder; replace any image whose source is
  not a local file with its alternative text -- files: `src/readingrenderer.*`. Why:
  nothing is fetched from the network, and a missing image still says what it was.
- [x] Report the rendered headings: level, plain text with runs of spaces collapsed,
  block number -- files: `src/readingrenderer.*`. Why: 2-1 matches outline entries to
  them.
- [x] Tests for the display copy, the render and the headings -- files:
  `tests/tst_leaf.cpp`. Why: the display-copy rules and restyling are what later
  slices build on.
- [x] Measure a render of a 5,000-line file and render real files to images outside
  the repo (the aidlc plugin's `workflow/brainstorm.md`, `docs/architecture.md`, a
  file with a table holding lists, a file with other HTML) and look at them -- files:
  none in the repo. Why: the spec's open questions on speed and Qt's handling.

## Tests

Against the real renderer and real `QTextDocument`s, in the existing Qt Test suite:

- The display copy blanks front matter and keeps its line count; a first line of
  `---` with no closing line is left alone.
- The display copy blanks `<task>`, `</output_format>` and `<example type="bad">`,
  keeps a tag line inside a fenced code block, keeps a line with a tag and other text,
  and keeps every other line unchanged.
- Rendering brainstorm-like text (front matter, `<context>`, a list and a table
  inside it) gives a document with the list items and table cells present and no
  front matter text.
- A tag line between two paragraphs leaves two paragraphs.
- Headings render at sizes that step down by level and in the style's text colour;
  links take the accent colour; inline code takes the code font.
- A web image renders as its alternative text; a local image keeps its image.
- The rendered headings list levels and plain text in order, with `**bold**` and
  `` `code` `` shown without marks, and an underlined heading included.

## Documents this could invalidate

`docs/architecture.md`: the *Reading renderer* component says it "returns a
document"; it fills one it is given. Updated in review if so.

## Notes for the next slice

Findings so far (2026-09-27), from the suite and from rendering real files to images
in `/tmp/mdreal` (a throwaway harness linking the renderer):

- **Drift, resolved:** an unclosed void HTML tag -- `<br>`, `<hr>`, `<img ...>`,
  `<input>`, in any case -- made Qt's importer drop everything after it in the file.
  `<br/>`, `<br />` and `<br></br>` are fine, as are `<details>`, `<summary>`,
  `<kbd>`, `<em>` and comments. The owner agreed a second display-copy rule (scope
  change 2026-09-27): such tags are written closed, outside code blocks and inline
  code, in text and heading lines. A `<br>` in a table cell now draws a line break
  in the cell.
- Speed: render alone, 5,130 lines, 26 ms in the suite; render plus layout at 700 px,
  6,840 lines, 78 ms. Well inside the spec's aim before on-screen drawing; 1-2
  measures in the window.
- Qt quirks corrected: a task item's checkbox marker leaks onto later blocks; a
  heading's size step is dropped on its bold words; code gets a point size; every
  code line is its own block (so spacing goes only on a run's outer lines); every
  code block, fenced or indented, carries `BlockCodeLanguage`, which is the reliable
  test for code (`nonBreakableLines` is only set on fenced blocks).
- Font: the installed iA Writer Quattro S files all claim the same weight, so Qt
  picks its Bold face for regular text. Prose is set in iA Writer Duo S instead
  (weights correct); code in iA Writer Mono S. The plan note assumed Quattro.
- For 1-3: the architecture diagram is about 78 columns and slightly overflows a
  700 px column at the code size; wrapping it would break its shape. Table header
  bolding from Qt is inconsistent (some header cells bold, some not), so 1-3 should
  set it itself. Block quotes are shaded; a side bar is not possible.
- Other HTML is passed to Qt's own HTML handling; comments vanish, as the spec says.
  An HTML `<img>` becomes an image format like a Markdown one, so the local-only
  image rule covers it too.
- For 1-2: the renderer fills a document it is given (the text view's own), so no
  document hand-over is needed. The style carries colours rather than a dark flag:
  the caller picks text, accent and a shade that suits the mode. Column width is the
  view's, not the renderer's. Headings only get a top margin when not the first
  block. Text after a table needs more space (1-3).

