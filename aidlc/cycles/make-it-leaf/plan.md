---
type: plan
description: Rename the fork to Leaf, then make every file open at its first line, then make Leaf the owner's default Markdown app.
---

# Plan: make-it-leaf

## Approach

Rename first, so every later slice and every test already speaks of Leaf. Then fix
opening at the top, which is independent of the rename but the most valuable change
day to day. Last, make Leaf the default, which needs the renamed package installed.
Nearly everything is reuse: the rename is mostly replacing names and renaming files,
the install script already builds and installs the package, and the test suite
already drives the real window headless.

## Phases

### Phase 1: It's called Leaf

Leaf has its own name and icon everywhere. Done when: the build produces `leaf`, its
window title ends in "- Leaf", the test suite passes, and no code, build, test or
packaging file mentions Omawrite. Effort: light.
Leaves behind: the `leaf` names -- program, application identity, desktop entry,
package -- that later slices refer to.

- [x] 1-1 rename-to-leaf -- rename everything that says Omawrite: program, application identity and settings location, window title, messages, project, test and packaging files, and the build and test scripts. Done when: the phase's done-when holds. Depends on: none.
- [x] 1-2 leaf-icon -- draw the new leaf icon and ship it in the package. Done when: the owner has approved the icon and the package installs it. Depends on: 1-1. Effort: medium.

### Phase 2: Opens at the top

Every way a document is loaded starts at its first line. Done when: opening a long
file from the command line and through the open dialog, reloading it after an outside
change, and recovering it after a crash all show the first line at the top with the
cursor at the start, proven by new tests and checked by hand. Effort: high.
Leaves behind: tests that check where the view lands after a document is loaded,
which the outline cycle can extend.

- [x] 2-1 open-at-top -- confirm why documents open at the bottom, write failing tests for each load path, then fix it. Done when: the new tests pass, and a long file opened by hand starts at the top with scrolling otherwise unchanged. Depends on: 1-1.

### Phase 3: Leaf is the default

Leaf is what opens Markdown on the owner's machine. Done when: the spec's verification
holds -- after the install script runs, the desktop reports Leaf as the default for
Markdown and it is written in the owner's own settings; a long `.md` file opened from
the file manager opens in Leaf at its first line; the launcher shows Leaf with its
icon; Super+Shift+W opens Leaf; all of this still works with Omawrite uninstalled;
scrolling feels as before; and a file opened and closed without edits is byte-for-byte
unchanged. Effort: light.
Leaves behind: an install script that sets the Markdown default.

- [x] 3-1 make-leaf-default -- the install script sets Leaf as the owner's Markdown default after installing and reports plainly if it cannot; the owner runs it; then, with the owner's go-ahead, Super+Shift+W is pointed at Leaf in the owner's personal key bindings. Done when: a `.md` file from the file manager and Super+Shift+W both open Leaf. Depends on: 1-2, 2-1.

## Notes

- Risk: the cause of opening at the bottom is suspected, not confirmed -- the caret
  likely lands at the end of the loaded text and the view follows it. Hence effort
  high on 2-1.
- Risk: renaming the application identity moves the settings location, so Leaf's
  first launch uses the default window size. Agreed in the spec.
- Assumed: the owner runs the install script, since it needs sudo; agents do not run
  it. The key binding edit follows Omarchy's rules for personal overrides. The owner
  uninstalls Omawrite for the final check.
- Documents this plan expects to touch: `README.md` and `AGENTS.md`, which mention
  `omawrite` names and paths; `docs/design.md`, which takes the spec's three additions
  when the cycle closes.
