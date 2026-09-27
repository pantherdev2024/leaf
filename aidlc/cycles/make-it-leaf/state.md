---
type: state
description: Leaf gets its own identity, becomes the default Markdown app, and opens every file at the top.
status: active
opened: 2026-09-27
mode: run
---

# State: make-it-leaf

## Where we are

All three phases done: slices 1-1, 1-2, 2-1 and 3-1 closed. The cycle is not yet
closed; the design's additions are not yet reconciled.

## Next

Close the cycle with `close-work`.

## Done

- 2026-09-27 1-1 rename-to-leaf
- 2026-09-27 1-2 leaf-icon
- 2026-09-27 2-1 open-at-top
- 2026-09-27 3-1 make-leaf-default

## For a cold session

- Two spec checks were not met, both by the owner's choice, recorded in slice 3-1's
  notes: `xdg-open` still hands `.md` files to Neovim (as it did with Omawrite), and
  the check with Omawrite uninstalled was skipped because the owner keeps Omawrite
  for now. Close-work must reconcile the spec's "desktop's open command" line and the
  design with these.
- The design takes the spec's three additions at close: reload starts at the top,
  Super+Shift+W opens Leaf, and the install script sets the Markdown default.
- One unreproduced observation to watch: an unedited file once opened marked
  "Unsaved" (slice 3-1 notes).
- Leaf 0.1.0 is installed on the owner's machine from this working tree. Nothing
  from this cycle is committed yet; the work sits on branch `foundation-docs` on top
  of the committed foundation documents. Commit only when the owner asks.
- The owner prefers plain, non-technical language in conversation.
