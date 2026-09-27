---
type: spec
description: Leaf gets an outline pane for jumping between sections from the keyboard, and four stat cards across the top, both fed by one scan of the text that the styling shares.
status: approved
---

# Spec: find-your-way

## Goal

Give the owner a way to move around a long file and size it up at a glance. An
outline of the file's headings sits to the left of the text; picking one, by keyboard
or mouse, brings that heading to the top. Four cards across the top show words,
lines, estimated tokens and sections. Both are fed by one scan of the text, which the
styling also uses, so the outline, the figures and what looks like a heading in the
text always agree. This comes now because the daily cost is highest here after
opening at the top, which the last cycle fixed: every trip to a section of a long LLM
file is a scroll or a guess at a search term. Both pieces need the same scan and both
change the window layout, so they are built together.

## Design fit

Uses from `docs/design.md`: the **Heading**, **Section**, **Outline** and **Stats**
concepts; the **Open and read**, **Jump to a section** and **Make a small edit** flows;
and the constraint that Leaf never rewrites a file.

Adds to the design, for reconciling at close:

- Ctrl+J moves between the text and the outline. This answers the design's open
  question on how the keyboard, the outline and the text share focus.
- Every heading level is shown and nothing folds. This answers the open question on
  deep heading levels.
- A file with no headings shows a faint "No headings" in the outline pane, and
  front matter is never part of the outline but is counted in words, lines and
  estimated tokens. This answers the open question on unusual documents.
- Only `#` headings count. Headings made by underlining a line with `===` or `---`
  are not recognised.
- The faint word count in the corner is replaced by the stat cards.

Nothing here contradicts the design or the architecture. The architecture already
describes the structure scan, the outline and stats derived in the core, and the
highlighter sharing the scan's rules.

## Behaviour

Nothing in this cycle writes to the file. The outline, the cards and the styling are
drawn from the text; the file on disk changes only when the owner saves an edit.

### What counts as a heading

- A heading is a line that starts, at its very first character, with one to six `#`
  characters followed by at least one space or tab. Its level is the number of `#`
  characters. This is the rule the text styling uses today, kept as it is so nothing
  that looks like a heading now stops looking like one.
- A line is not a heading when it is inside a fenced code block or inside front
  matter.
- A fenced code block opens with a line of three or more backticks or three or more
  tildes, with up to three spaces before them, and closes with a line of the same
  character, at least as long, with nothing after it but spaces. A fence that is never
  closed makes the rest of the file code, so no headings are found past it.
- Front matter is present only when the file's very first line is `---`. It runs to
  the next line that is `---` or `...`; trailing spaces on either line are allowed. If
  no such line follows, the file has no front matter and is read as ordinary
  Markdown, in the outline and in the styling alike.
- Headings made by underlining a line with `===` or `---` are not headings in Leaf.

### Styling in the text

- A `#` line inside a code block or front matter is no longer drawn as a heading. It
  is drawn as plain text, as the lines around it are.
- Typing or deleting a fence line restyles the lines below it at once, so opening a
  code block drops the heading styling of any `#` lines inside it, and closing it
  brings back the styling of headings after it.
- Typing or deleting the line that closes front matter restyles the lines it affects
  no later than the outline updates.
- Nothing else about styling changes in this cycle. Bold, italic, links and inline
  code inside code blocks are styled as they are today; code block shading is the next
  cycle.

### The outline pane

- A pane down the left side of the window, beside the text, with a set width. It is
  always shown. It follows the theme's colours and the desktop text size, as the rest
  of the window does.
- It lists every heading in the file, in order, indented by level. All levels are
  shown and nothing folds.
- Each entry shows the heading's text as it reads, without the leading `#`
  characters, a closing run of `#` characters after a space (so `# C#` shows "C#"), or
  the Markdown markers for bold, italic, inline code and links. The markers are
  recognised by the same inline rules the text styling uses, not a second copy. A heading too long for the pane ends in "…". A heading with
  no text is listed as a faint "Untitled heading", so it can still be reached.
- The heading the owner is reading is marked: the last heading whose line's top edge
  is at or above the top edge of the view. When the view is above the first heading, nothing is
  marked. The mark moves as the owner scrolls.
- When the list is longer than the pane, the pane scrolls on its own, and keeps the
  marked entry in view as the owner reads.
- A file with no headings shows a faint "No headings" in the pane. The pane keeps its
  place and width, so the text does not shift sideways between files.

### Jumping to a section

- **From the keyboard.** Ctrl+J, pressed in the text or in the find and replace bar,
  moves focus into the outline
  with the marked entry selected, or the first entry when nothing is marked.
  - Up and down arrows move the selection. The text does not move while they do.
  - Enter jumps to the selected heading and returns focus to the text.
  - Escape, or Ctrl+J again, returns focus to the text without jumping. The text
    cursor is where it was.
  - In a file with no headings, or while a dialog is open, Ctrl+J does nothing.
- **With the mouse.** Clicking an entry jumps to that heading and puts focus in the
  text.
- **The jump.** The heading's line is brought to the top of the view, or as near as
  scrolling allows for a heading close to the end of the file. The text cursor is
  placed at the start of the heading line, and that entry becomes the marked one,
  even when the heading could not reach the top. The next scroll marks by the usual
  rule.
- Tab keeps its current meaning in the text. It is not used to move into or through
  the outline.
- Ctrl+J is added to the shortcuts list the window shows with Ctrl+?.

### Layout

- The text column fits the space beside the outline pane. It keeps today's reading
  width wherever the window leaves room for it, and narrows when it does not.
- The footer, the find and replace bar and the scroll bar keep their places; the
  scroll bar still stays clear of the footer.

### The stat cards

- A row of four cards across the top of the window, above the outline and the text:
  **Words**, **Lines**, **Tokens** and **Sections**, in that order. Each shows a large
  number with its label beneath, in the theme's colours.
- Numbers are written with commas between thousands: "12,480". The token figure is
  shown with "≈" in front, "≈ 3,112", because it is a rough guide.
- The cards cannot take focus and do nothing when clicked.
- All four figures describe the text in the window, unsaved edits included, not the
  file as last saved.
- **Words** is counted exactly as the corner word count counts today, over the whole
  text, front matter and code included.
- **Lines** is the number of lines in the text. A line break at the very end does
  not start another line. An empty document has 0 lines.
- **Tokens** is the number of characters in the text divided by four, with halves
  rounded up. Characters are counted as Leaf's text holds them: each line break is
  one, and some emoji count as two, which is well within a rough guide.
- **Sections** is the number of headings, the same as the number of entries in the
  outline.
- The faint word count in the corner of the footer is removed. The rest of the footer
  is unchanged.

### Keeping up with the text

- When a document is loaded -- by opening, reloading after an outside change, or
  recovery after a crash -- the outline and cards show it straight away.
- While the owner types, the outline and cards update 120 milliseconds after typing
  pauses, the same delay the word count uses today. During that moment they may be a
  step behind the text, and a jump uses the outline as last updated.

## Not in scope

- Shading code blocks, lining up table columns, and any other styling of code blocks
  or tables. That is the next cycle.
- Headings made by underlining with `===` or `---`.
- Folding sections, hiding heading levels, hiding the outline pane, or resizing it.
- Stepping through headings with Tab, or a live preview that moves the text as the
  outline selection moves.
- Printing the outline or the cards. Printing is unchanged.
- An exact token count for any model.
- Anything that writes to the file other than the owner's own save.

## Verification

- The test suite passes, with new checks that:
  - headings are found at the right levels, text and positions, and not inside fenced
    code blocks, front matter, or after an unclosed fence;
  - a file whose first line is `---` with no closing line has no front matter;
  - words, lines, tokens and sections come out as described, including for an empty
    file, a file ending with a line break, and a file with front matter;
  - the styling does not draw a `#` line inside a code block or front matter as a
    heading, and opening or closing a fence restyles the lines below;
  - the outline and stats update after an edit, and show a loaded document at once.
- On screen, in Leaf, with dark and light themes:
  - a long LLM plan file shows its outline, the cards read correctly, and the corner
    word count is gone;
  - a file under `aidlc/` with front matter shows no front matter in its outline;
  - a file with no headings shows "No headings";
  - a file with a `#` line inside a code block shows that line plain and leaves it out
    of the outline;
  - Ctrl+J, the arrow keys, Enter and Escape work as described, and a picked heading
    lands at the top of the view;
  - clicking an entry jumps to it;
  - scrolling moves the mark, and in a long outline the mark stays in view.
- A file opened, read, jumped around in and closed without edits is byte-for-byte
  unchanged on disk.

## Open questions

- The exact look of the cards and the pane: widths, spacing, how the marked entry is
  shown. Resolved on screen during the cycle, with the owner's approval.
