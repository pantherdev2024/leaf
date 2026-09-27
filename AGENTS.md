# Leaf

<!-- aidlc:begin -->
## aidlc

This repo is developed with the aidlc plugin. Its state lives in files, not in any
session.

- Read the active cycle's `aidlc/cycles/*/state.md` before touching code. Without it,
  a session redoes finished slices or undoes decisions still in flight. A directory
  holding a `state.md` is a cycle; lone bug fixes are in `aidlc/fixes/`.
- Prefer starting new work, a feature or a bug, with `start-work` over editing code
  directly. A change made outside a slice has no spec, no tests tied to it, and no
  record for the next session.
- A slice is done when its tests pass and `update-state` has recorded it, not when the
  code works. The plan's checkbox and `state.md` are what the next session trusts.
- When the plan turns out to be wrong, stop and raise it rather than route around it.
  An agreed change that affects later slices goes in `scope-changes.md`; otherwise
  they get detailed against a plan that no longer holds.
- Prefer running the skill that owns a step over hand-editing files under `aidlc/`.
  The skills keep the plan, the slice files and `state.md` in step; a hand edit to one
  leaves the others disagreeing.
- Read `docs/design.md` (what and why) before changing behaviour, and
  `docs/architecture.md` (how) before changing structure. A change that contradicts
  either one updates it first.
<!-- aidlc:end -->

## How to read this file

These are defaults, not law. Where the code in front of you disagrees with one, the
code usually wins: say so and proceed, rather than silently obeying or silently routing
around it. A line marked as an invariant stays binding; if it looks wrong, argue it,
do not act past it.

## Working norms

- If an approach looks wrong, say so once, with the reason, before building it. If the
  decision stands, build it fully.
- Read your own diff as a reviewer looking for the bug before calling it done, and say
  what you checked and what you did not.
- Working is evidence, not completion.
- For anything visual, check the real result on screen, in the states people will
  see, before calling it done. A passing build or review does not show what it looks
  like or how it behaves.

## This repo

- Invariant: Leaf never changes a file's text except when the owner saves an edit.
  No reformatting, padding or normalising to make something display better; that is
  done with styling. See docs/architecture.md, "Why formatting only changes how text
  is drawn".
- A new source file that is missing from a build list fails late and confusingly.
  C++ files go in `leaf.pro`, and in `tests/tests.pro` if the tests use them;
  QML and JS files go in `src/resources.qrc`, or the window fails to load at startup.
- `bin/test` prints QML "Binding loop detected for property implicitWidth" warnings
  from the Material dialog. They were there at the fork and are not caused by your
  change; the pass/fail totals at the end are what count.
- Do not run `bin/install` from a session: it runs `makepkg -fsi`, which asks for
  sudo and installs a system package. Use `bin/build` and `build/leaf <file>`
  to try the app.
- To check the app on screen from a session: launch `build/leaf <file>`, find its
  window in `hyprctl clients -j`, and capture it with `grim -g`. Close it with
  `hyprctl dispatch 'hl.dsp.window.close({ window = "pid:<pid>" })'`; killing the
  process skips saving settings and leaves a recovery lock behind. Keys sent with
  `wtype` reach the app but not Hyprland's own shortcuts.
- `xdg-open file.md` opens Neovim, not Leaf: it guesses types from contents. Test the
  Markdown default with `gio open`, which goes by the file name, as the file manager
  does.
