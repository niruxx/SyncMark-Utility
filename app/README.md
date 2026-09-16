# SyncMark Desktop Client

A Qt6 (Widgets) desktop client for [SyncMark](https://github.com/niruxx/SyncMark), a
self-hosted bookmark manager with contacts, calendar, file storage and a
password vault. Builds natively on Linux and Windows.

The client talks to your existing SyncMark server over its REST API
(`/api/...`) — it does not include or run the server itself. Start your
SyncMark server first, then point this app at it.

A modern Fusion-based theme (light/dark, following the OS setting), a fixed
top bar with branding and account controls, a branded sidebar, consistently
styled tables/forms, subtle fade-in motion (page switches, every dialog),
and system tray support (minimize/close to tray, restore, log out, quit) are
applied app-wide — see [Theme.cpp](src/ui/Theme.cpp), [MainWindow.cpp](src/ui/MainWindow.cpp)
and [Animations.cpp](src/ui/common/Animations.cpp).

## Features

- First-run server setup wizard and login (session-cookie auth, with an
  optional "stay signed in" that persists the session across restarts)
- **Bookmarks**: folder tree, search/sort/favorites, add/edit/delete,
  import (browser/Pocket/etc. exports) and export (HTML/JSON)
- **Contacts**: full field editing (emails, phones, addresses, social
  profiles, messaging handles, custom fields, key dates, relationships to
  other contacts), tags, favorites, photo; manual and smart groups; duplicate
  detection with merge/dismiss; multi-select bulk favorite/unfavorite/delete;
  import/export (CSV/vCard)
- **Calendar**: month view, add/edit/delete events with recurrence
  (daily/weekly/monthly + until date) expanded into individual occurrences
  in both the day list and the month highlight, import/export (ICS)
- **Password vault**: add/edit/delete, reveal/copy password, generate
  password, import/export (CSV)
- **Files**: manage sandboxed locations, browse/upload/download/rename/
  delete, trash with restore, inline text file viewing/editing, Unix
  permission (chmod) editor
- **Admin**: user management, module (feature) toggles, on-demand and
  scheduled backups, restore from backup
- Account settings: username/password, avatar, session duration

### Known simplifications

- CardDAV/CalDAV are intentionally out of scope — those exist on the server
  for native phone sync, and add nothing for this REST-based desktop client
- Editing a recurring event edits the whole series (there's no per-occurrence
  override), matching the server's single RRULE-per-event model
- Smart contact group rule *values* are free-text (the field and common
  operators are pickable from a list; the exact operator set the server
  accepts isn't published, so unrecognized operators are sent through as-is)

## Requirements

- Qt 6.2+ (Widgets, Network, Core, Gui modules)
- CMake 3.16+
- A C++17 compiler: GCC or Clang on Linux, MSVC or MinGW on Windows

## Building on Linux

```bash
# Debian/Ubuntu example — adjust for your distro
sudo apt install qt6-base-dev cmake build-essential

cd app
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/SyncMarkClient
```

## Building on Windows

1. Install Qt 6 (via the [Qt Online Installer](https://www.qt.io/download-qt-installer)),
   selecting the MSVC or MinGW kit that matches your compiler.
2. Install [CMake](https://cmake.org/download/) and a compiler (Visual Studio
   2022 with the "Desktop development with C++" workload, or MinGW).
3. Configure and build, pointing CMake at your Qt install:

```powershell
cd app
cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\msvc2019_64" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

4. Run `build\Release\SyncMarkClient.exe`. The build automatically runs
   `windeployqt` after linking (see `CMakeLists.txt`), copying the required
   Qt DLLs and plugins next to the exe, so it runs as-is straight out of
   `build\Release\` — no separate deployment step needed.

## Usage

1. Launch the app. Enter your SyncMark server's URL (e.g.
   `http://localhost:3000` or `https://syncmark.example.com`) and connect.
2. If this is a brand-new server, you'll be walked through the first-run
   setup wizard; otherwise, log in with your existing account.
3. Use the left-hand navigation to switch between Bookmarks, Contacts,
   Calendar, Passwords and Files — only modules enabled on the server
   (see **Account → Enabled Modules**, admin only) are shown.

## Project layout

```
app/
  CMakeLists.txt
  src/
    main.cpp              Startup: connect/login flow, then MainWindow
    core/                 ApiClient (REST + cookies), Session, AppSettings
    ui/                   MainWindow, Theme (styling), dialogs shared across modules
    ui/bookmarks/
    ui/contacts/           Contacts, groups, duplicate merging
    ui/calendar/
    ui/passwords/
    ui/files/               Browser, locations, trash, permissions
    ui/admin/              Users + backups (admin-only)
    ui/common/             Shared helpers (PageWidget base, Notify, RepeatingTableEditor)
```
