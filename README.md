# fd-gui

A fast Qt 6/QML file and directory search UI backed by [fd](https://github.com/sharkdp/fd).

## Features

- Results appear as `fd` finds them.
- Literal, case-smart search with whitespace-separated AND terms: `report draft` matches `Report-Draft.md` and `report_draft.pdf`.
- Extension filtering and fd-compatible ignore globs, including paths such as `~/Downloads/`.
- Live sorting by modification time, name, size, or type, with an optional folders-first grouping.
- Optional searching inside hidden directories.
- Image, plain-text, PDF, DOCX, and rendered Markdown previews.
- Double-click to open with the default application.
- Drag one or multiple results to other applications using standard `text/uri-list` MIME data.
- Material UI that follows DMS colors when `~/.cache/dms/dms-colors.json` is available.
- Persistent extension, hidden-directory, and ignore settings.

## Requirements

Build requirements:

- CMake 3.25 or newer.
- A C++20 compiler.
- Qt 6.5 or newer, including Core, Gui, Qml, Quick, Quick Controls 2, and Test.
- Ninja is recommended but not required.

Runtime requirements:

- `fd` must be available as `fd` in `PATH`.
- The Qt 6 QML modules for Qt Quick, Qt Quick Controls, and Qt Quick Layouts.

Optional preview tools:

| Tool | Purpose |
| --- | --- |
| `pdftotext` | Preferred PDF text extraction; provided by Poppler. |
| `pdf2txt` | Fallback PDF text extraction; provided by pdfminer.six. |
| `pandoc` | DOCX to Markdown conversion. |

DMS is optional. Without its generated color file, the app uses the standard Material light/dark palette.

> Older Debian and Ubuntu releases may provide Qt older than 6.5. Install a newer Qt before building on those releases.

## Distribution packages

### Debian and Ubuntu

```sh
sudo apt update
sudo apt install \
  cmake g++ ninja-build \
  qt6-base-dev qt6-declarative-dev \
  qml6-module-qtqml \
  qml6-module-qtquick \
  qml6-module-qtquick-controls \
  qml6-module-qtquick-layouts \
  fd-find poppler-utils pandoc python3-pdfminer
```

Debian and Ubuntu install the fd binary as `fdfind`, while this app runs `fd`. If `fd --version` does not work, create a user-local symlink:

```sh
mkdir -p "$HOME/.local/bin"
ln -s /usr/bin/fdfind "$HOME/.local/bin/fd"
```

Make sure `~/.local/bin` is in `PATH`:

```sh
export PATH="$HOME/.local/bin:$PATH"
```

Add that export to your shell configuration if it is not already present. A shell alias is not sufficient because the app starts `fd` directly.

### Fedora

```sh
sudo dnf install \
  cmake gcc-c++ ninja-build \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  fd-find poppler-utils pandoc python3-pdfminer
```

Fedora's `fd-find` package provides `fd` directly.

### Arch Linux

```sh
sudo pacman -S --needed \
  cmake base-devel ninja \
  qt6-base qt6-declarative \
  fd poppler pandoc python-pdfminer
```

## Build and run

From the project root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/fd-gui
```

The search root is the directory from which the app is launched. For example:

```sh
cd ~/Documents
/path/to/fd-gui/build/fd-gui
```

With empty search and extension fields, the app lists only immediate subdirectories for a fast startup. Typing search terms or an extension starts a recursive search; clearing both returns to the shallow subdirectory list.

## Install

Install for the current user:

```sh
cmake --install build --prefix "$HOME/.local"
```

This installs `~/.local/bin/fd-gui` and `~/.local/share/applications/fd-gui.desktop`, so the app also appears in your application launcher with the system search icon. Ensure `~/.local/bin` is in `PATH`, then run:

```sh
fd-gui
```

Install system-wide:

```sh
sudo cmake --install build --prefix /usr/local
```

This installs `/usr/local/bin/fd-gui`.

## Usage

- **Search:** type literal terms separated by spaces. Every term must match the filename; order does not matter.
- **Case:** lowercase terms are case-insensitive. An uppercase letter enables fd's smart-case behavior.
- **Extension:** enter one or more extensions, such as `pdf, md` or `.pdf .md`.
- **Ignore:** enter one glob per line or separate globs with commas, for example `node_modules`, `.cache`, or `~/Downloads/`.
- **Preview:** single-click a result.
- **Open:** double-click a result or use the preview's Open button.
- **Drag:** drag a result into another application. Use Ctrl-click or Shift-click for multiple selections.
- **Resize:** drag the single vertical handle between the result list and preview pane.

The extension, hidden-directory, and ignore fields are saved in:

```text
$XDG_CONFIG_HOME/fd-gui/fd-gui.conf
```

If `XDG_CONFIG_HOME` is unset, this is normally:

```text
~/.config/fd-gui/fd-gui.conf
```

On first run, settings from the previous `fd-file-search` configuration are migrated automatically.

## Tests

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

## License

MIT. See [LICENSE](LICENSE).
