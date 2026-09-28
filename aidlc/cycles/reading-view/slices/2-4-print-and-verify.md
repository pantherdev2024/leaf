---
type: slice
description: Ctrl+P prints the text through the reading renderer in light colours, and the spec's verification is run in full.
status: done
effort: medium
---

# Slice 2-4: print-and-verify

## Context

Phase 2, last slice of the cycle. Serves the spec's *Printing* and its whole
*Verification*. Today `Backend::printDocument` imports the raw text with Qt's
Markdown support and prints it with the editor's font: front matter and tag lines
print, anything after an unclosed `<br>` is lost, and nothing is styled. 1-3 left the
table header shade to the window, which printing does not have; Qt's printing
painter, unlike the text view, does draw a table cell's background.

## Intent

The core builds the printed page with the reading renderer: the current text (unsaved
edits included), a light style whatever the theme (the light theme's default colours,
shading and lines worked out as the reading view does), the file's URL for images,
and the page's width as the column, so code is fitted to the paper as it is to the
window. The page size is set on the document from the printer's printable area, so
the column and the printed page agree. After rendering, each table's header cells get
the shade as their own background, which the printing painter draws. The print dialog
and printing itself are unchanged. Then the spec's verification is run: the suite,
the on-screen checks in both themes, and the byte-for-byte check.

## Done when

A page printed to a PDF file shows the reading view's styling -- shaded header and
code boxes, no front matter or tag lines, dark text on white from a dark theme -- and
every check in the spec's *Verification* passes, including a file opened, read in
both views, searched and closed being byte-for-byte unchanged. Checks a session cannot
make (a link opening in the browser, the running app in the light theme) are listed
for the owner.

## Tasks

- [x] Build the printed document through the renderer, light-styled, at the page's
  width, with header cells shaded; print it -- files: `src/backend.h`,
  `src/backend.cpp`, `src/readingrenderer.h`, `src/readingrenderer.cpp`. Why: the
  spec's *Printing*; the header shade must be in the document for paper.
- [x] Tests: the printed document's styling from a dark theme, a PDF written, and a
  file unchanged byte for byte after being read, switched, searched and closed --
  files: `tests/tst_leaf.cpp`. Why: the done-when and the spec's last check.
- [x] Run the verification: map each suite check to its test, capture the on-screen
  checks in both themes, look at the PDF -- files: none (temporary capture test,
  removed). Why: the spec's *Verification*.

## Tests

- With the backend in dark mode, the printed document of a file with front matter, a
  `<task>` line, a table and a labelled code block has no front matter or tag line
  text, text in the light foreground, header cells with the shade as background, and
  the code in a shaded frame with its label.
- Printing to a PDF file writes a non-empty PDF.
- A file opened, switched both ways, searched and its window closed is unchanged on
  disk, byte for byte.

## Documents this could invalidate

`docs/architecture.md`: printing (component or flow) if it still says print imports
the raw text. `README.md`: if it describes printing. `AGENTS.md`: none expected.

## Notes for the next slice

- **Printing** goes through `Backend::printTo`: `buildPrintDocument` renders the current
  text with the light theme's default colours (white, `#222324`, `#2077b2`) at the
  standard body size (not the desktop text scale), sets the document's page size to
  the printer's printable area at 96 pixels to the inch so Qt prints it scaled from
  screen pixels, fits code to that width, and shades header cells in the document
  (`ReadingRenderer::shadeHeaderCells`), which paper draws.
- **Margins:** Qt adds 2 cm margins only to a document without a page size; with one
  it adds none, and a PDF printer has none, so `printTo` sets 2 cm when the printer
  gives under 1 cm on every side.
- A code box can split across two pages.
- **Verification run (2026-09-27):**
  - Suite: 103 tests pass. Each suite check in the spec maps to a test: display copy
    (`displayCopy*`), a list and table after `<context>`, a tag line between
    paragraphs, `<br>` in a cell and in code, tables' borders, header and alignment,
    code shade and label, matching in order with underlined and marked headings,
    unmatched entries, switching keeping the section and the undo history, edits shown
    on return, rebuilt after reload, web images not fetched.
  - On screen, headless captures of the real window: brainstorm.md (front matter and
    `<task>` lines gone, table whole, dark and light), architecture.md (diagram keeps
    its shape, dark and light), a wide table wrapping, a labelled code block, Ctrl+E
    at the same place both ways, the outline click, Ctrl+J and the mark, find
    highlighting on the page. design.md has no tables or code; it was seen in 1-2.
  - Printed to PDF: the sample and brainstorm.md show shaded headers and code boxes,
    no front matter or tag lines, dark text on white from a dark theme.
  - A file opened, switched, searched and closed is byte-for-byte unchanged (test).
  - **Left to the owner:** a web link opening in the browser, copying text from the
    page, a live theme change while a file is open, and the whole thing in the
    running app. The spec's open question on showing which view is active is also
    the owner's to settle.
