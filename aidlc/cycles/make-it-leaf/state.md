---
type: state
description: Leaf gets its own identity, becomes the default Markdown app, and opens every file at the top.
status: closed
closed: 2026-09-27
opened: 2026-09-27
mode: run
---

# State: make-it-leaf

## Where we are

Closed 2026-09-27. All four slices done; the spec's verification performed; the
foundation documents reconciled.

## Next

Nothing. This cycle is closed.

## Done

- 2026-09-27 1-1 rename-to-leaf
- 2026-09-27 1-2 leaf-icon
- 2026-09-27 2-1 open-at-top
- 2026-09-27 3-1 make-leaf-default

## For a cold session

- Delivered: Leaf's own identity and icon, every load starting at the first line, and
  `bin/install` making Leaf the Markdown default; Super+Shift+W opens Leaf through
  the owner's own Hyprland bindings.
- Two spec checks were settled by the owner's choice, recorded in `scope-changes.md`:
  `xdg-open` still hands `.md` files to Neovim (now in the design's *Not in scope*),
  and the check with Omawrite uninstalled was not performed because the owner keeps
  Omawrite for now.
- The spec's open question, the icon's look, was answered in slice 1-2.
- Left for later, as the owner noted during the brainstorm: keeping the reading place
  on reload, and the prompt that appears on every outside change even with no local
  edits.
- One unreproduced observation: an unedited file once opened marked "Unsaved" (slice
  3-1 notes). Watch for it.
