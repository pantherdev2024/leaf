---
type: slice
description: Replace the inherited Omawrite artwork with a leaf icon in the same flat style, approved by the owner.
status: done
effort: medium
---

# Slice 1-2: leaf-icon

## Context

Spec, *Leaf's identity*: the icon is new -- a simple leaf shape in the same flat style
as Omawrite's, a dark rounded square with a light figure on it. Open question in the
spec: its exact look, resolved by the owner approving it. Phase 1, *It's called Leaf*.

## Intent

Slice 1-1 renamed the icon file to `leaf.svg` and the package and desktop entry
already install and name it `leaf`, but the file still holds Omawrite's page-and-lines
artwork. This slice draws a leaf in its place, keeping the canvas, the dark rounded
square and the light foreground colour so it sits among Omarchy's icons, and gets the
owner's approval of the look.

## Done when

The owner has approved the icon, `pkgbuild/leaf.svg` holds it, and the package's
install step still installs it under the name `leaf`.

## Tasks

- [x] Draw two or three leaf variants on Omawrite's canvas and colours, render them to
  images, and show the owner -- files: scratch images outside the repo. Why: the look
  is the owner's call.
- [x] Write the approved variant into the icon file -- files: `pkgbuild/leaf.svg`. Why:
  the package installs this file as the `leaf` icon.
- [x] Confirm the icon renders at launcher sizes and the package still references it
  -- files: `pkgbuild/PKGBUILD` (read only). Why: a small icon must still read as a
  leaf.

## Tests

No automated test: an icon's correctness is its look, which the owner approves. The
render at small sizes is checked by eye.

## Documents this could invalidate

None.

## Notes for the next slice

- The owner chose variant A of three: a leaf tilted from bottom left to top right,
  with one dark centre line, on Omawrite's dark rounded square and light colour.
  The rejected variants were an upright leaf with a stem (its centre line nearly
  split it at small sizes) and a veined leaf (busy and blurry when small).
- Rendered at 16, 24, 32 and 48 pixels it still reads as a leaf. The installed icon
  is only seen once the package is installed in 3-1.
