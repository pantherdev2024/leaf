# Leaf

A Markdown viewer for Omarchy, for reading, scanning and jumping around the long
Markdown files that LLM sessions produce, with light editing when needed. It opens
files at the top, shows an outline of the headings to jump through, and shows word,
line, token and section counts at a glance. Forked from Omawrite, it follows the
system's dark or light mode, the live Omarchy theme and the desktop text size.

## Running it

- `bin/build` builds the app into `build/leaf`. Run it with a file:
  `build/leaf notes.md`.
- `bin/test` runs the test suite, headless.
- `bin/install` builds and installs it as an Arch package (asks for sudo), and
  makes Leaf your default app for Markdown files.

Needs Qt 6 (`qt6-base`, `qt6-declarative`), `xdg-desktop-portal` with a backend,
and qmake and make to build. The reading view's text font, iA Writer Duo S, comes
from the `ttf-ia-writer` package; without it Qt picks another font.

## Read more

- [docs/design.md](docs/design.md): what Leaf is and why.
- [docs/architecture.md](docs/architecture.md): how it is built.

## License

MIT; see `LICENSE`. The bundled iA Writer Mono font is under the SIL Open Font
License 1.1; see `fonts/OFL.txt`. The font is copyright Information Architects Inc.
and based on IBM Plex, copyright IBM Corp.
