---
type: slice
description: A single line scan defines headings, code fences and front matter, and the highlighter uses it so `#` lines inside code or front matter are no longer drawn as headings.
status: done
effort: high
---

# Slice 1-1: structure-scan

## Context

Serves the spec's *What counts as a heading* and *Styling in the text*, in phase 1,
"One scan, and the figures at a glance". The phase leaves behind the structure scan:
text in, each line's kind and the headings (level, text, position) and front matter
extent out, used by both the core and the highlighter. Slice 1-2 builds the stat cards
and the single recount on it; phase 2 builds the outline from its headings.

## Intent

Today the highlighter decides alone, line by line, that any line starting with one to
six `#` and whitespace is a heading, with no idea of code blocks or front matter. This
slice adds a pure structure scan holding the spec's block rules: the heading rule
exactly as the highlighter has it now, fenced code blocks (backticks or tildes, up to
three spaces of indent, closed by the same character at least as long), and front
matter (a first line `---` closed by `---` or `...`, trailing spaces allowed, and none
at all when there is no closing line). It classifies one line given the state carried
from the line before, and scans a whole text for its headings (level, raw text, line,
position) and front matter end. The highlighter carries the fence state from block to
block through its block state, so a typed or deleted fence restyles the lines below at
once, and skips heading styling on code and front matter lines. Front matter needs a
look ahead the line-by-line highlighter cannot make: the highlighter works out the
front matter's extent when it styles the first line, and the core asks it to check
again on its existing 120 ms recount after typing, restyling only when the extent
changed. Nothing else about styling changes.

## Done when

The new scan and styling tests pass with the rest of the suite, and a file with a `#`
line in a code block and in front matter shows those lines plain in Leaf on screen,
while real headings stay bold.

## Tasks

- [x] Add the structure scan: the per-line rules (heading, fence open and close, front matter open and close), the line classifier that carries fence state as an int the highlighter can store, and the whole-text scan returning headings and the front matter end -- files: `src/structurescan.h`, `src/structurescan.cpp`. Why: the one definition of block-level Markdown the architecture calls for.
- [x] Add the new files to both builds -- files: `leaf.pro`, `tests/tests.pro`. Why: a file missing from a build list fails late.
- [x] Make the highlighter use the scan: carry fence state through the block state, compute the front matter extent at the first line, and apply heading styling only to lines the scan calls headings -- files: `src/markdownhighlighter.h`, `src/markdownhighlighter.cpp`. Why: `#` lines in code or front matter must not be drawn as headings, and fences must restyle the lines below.
- [x] Have the core ask the highlighter to recheck the front matter on the existing recount after typing -- files: `src/backend.cpp`. Why: typing or deleting the closing line changes lines far from the edit, which the highlighter cannot see line by line.
- [x] Write the scan and styling tests -- files: `tests/tst_leaf.cpp`. Why: the rules have many edge cases and the styling change touches every file.
- [x] Check on screen with a file holding front matter, a fenced `#` line and real headings, in the running app.

## Tests

In the existing Qt Test suite, against the real scan, the real highlighter on a real
`QTextDocument`, and the real window driven headless for the recount path:

- Headings: levels one to six with their text, line and position; a tab after the
  `#`s counts; `#x`, seven `#`s and a leading space are not headings, as today.
- Fences: `#` lines inside backtick and tilde fences are not headings; a closing fence
  must use the same character and be at least as long; an indent of up to three
  spaces still makes a fence and four does not; an unclosed fence hides every heading
  after it.
- Front matter: `#` lines between a first-line `---` and a closing `---` or `...` are
  not headings, trailing spaces allowed; a first-line `---` with no closing line means
  no front matter, and headings after it are found.
- Styling: in a highlighted document, a heading line is bold and a `#` line in a
  fence or front matter is not; inserting an opening fence above a heading unbolds it,
  and removing the fence bolds it again.
- Recount: in the real window, typing the closing line of front matter unbolds a `#`
  line inside it after the recount.

## Documents this could invalidate

- `docs/architecture.md` -- *Structure scan* and *Highlighter* describe this shape. The
  highlighter working out the front matter's extent itself, with the core asking it to
  recheck, is a detail the architecture does not state; add it only if it contradicts
  what is written.

## Notes for the next slice

- The scan lives in `src/structurescan.{h,cpp}` as free functions: `headingLine`,
  `classifyLine` (fence state carried as an int), `opensFrontMatter`,
  `closesFrontMatter`, and `scan`, which returns the headings (level, raw text, line,
  position of the line start) and the front matter's closing line. Heading text is raw:
  closing `#`s and inline markers are still in it, for 2-1 to strip for display.
- The core does not call `scan` yet. The only recount is still
  `Backend::refreshWordCount` on the 120 ms timer, which now also calls
  `MarkdownHighlighter::refreshFrontMatter()`. 1-2 should turn it into the single
  recount that runs `scan` once for the figures and, later, the outline; the name
  `refreshWordCount` no longer fits.
- The highlighter works out the front matter's extent itself when it styles line 0,
  and `refreshFrontMatter()` restyles the whole document when the extent moved. A
  review found that an edit to line 0 could hide a move from the recheck; a flag now
  catches it, with a regression test.
- Block states: 0 plain, 1 front matter, 6 or more an open fence. Anything else that
  needs a block state later must not collide with these.
- Tests on a bare `QTextDocument` must create its layout (`documentLayout()`) before
  editing, and let the new highlighter's queued first pass run with
  `processEvents()`; without either, nothing is restyled. The editor's document
  always has a layout.
- Trailing whitespace after a closing fence or a front matter line is spaces only, as
  the spec says; a tab keeps a fence open.
- `LineKind::Fence` and `LineKind::Code` are not told apart by anyone yet; the next
  cycle's code shading needs them.
- On screen, while this slice was checked, keystrokes meant for another window landed
  in the newly opened Leaf window ("ck " at the start of the file), because Hyprland
  focuses a new window. This may be the "Unsaved on open" seen in make-it-leaf 3-1.
  When checking on screen, capture straight after launch and do not leave the window
  open.
- Documents: `docs/architecture.md` does not contradict this; its *Highlighter*
  paragraph does not mention front matter, which can be added when the cycle closes.
