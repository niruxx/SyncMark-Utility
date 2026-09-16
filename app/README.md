# SyncMark Desktop Client

A Qt6 (Widgets) desktop client for [SyncMark](https://github.com/niruxx/SyncMark), a
self-hosted bookmark manager with contacts, calendar, file storage and a
password vault. Builds natively on Linux and Windows.

The client talks to your existing SyncMark server over its REST API
(`/api/...`) — it does not include or run the server itself. Start your
SyncMark server first, then point this app at it.

## Features

- First-run server setup wizard and login (session-cookie auth, with an
  optional "stay signed in" that persists the session across restarts)
- **Bookmarks**: folder tree, search/sort/favorites, add/edit/delete,
  import (browser/Pocket/etc. exports) and export (HTML/JSON)
- **Contacts**: search, add/edit/delete, favorites, photo, tags,
  import/export (CSV/vCard)
- **Calendar**: month view, add/edit/delete events with simple recurrence
  (daily/weekly/monthly + until date), import/export (ICS)
- **Password vault**: add/edit/delete, reveal/copy password, generate
  password, import/export (CSV)
- **Files**: manage sandboxed locations, browse/upload/download/rename/
  delete, trash with restore, inline text file viewing/editing
- **Admin**: user management, module (feature) toggles, on-demand and
  scheduled backups, restore from backup
- Account settings: username/password, avatar, session duration

### Known simplifications

A few advanced, secondary parts of the server's data model are not fully
exposed in the UI (existing values round-trip untouched when you edit a
contact, they're just not editable from this client yet):

- Contact addresses, social profiles, messaging handles, custom fields, key
  dates, relationships, smart contact groups, and duplicate-contact merging
- Calendar recurrence is edited/saved but not expanded into individual
  occurrences in the day list (each event shows its base start/end time)
- File permission (chmod) editing
- CardDAV/CalDAV (those are for native phone sync, not needed for this
  REST-based desktop client)

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

4. Run `build\Release\SyncMarkClient.exe`. For a standalone/distributable
   build, run Qt's deployment tool from the same directory as the built exe:

```powershell
C:\Qt\6.7.0\msvc2019_64\bin\windeployqt.exe build\Release\SyncMarkClient.exe
```

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
    ui/                   MainWindow and dialogs shared across modules
    ui/bookmarks/
    ui/contacts/
    ui/calendar/
    ui/passwords/
    ui/files/
    ui/admin/              Users + backups (admin-only)
    ui/common/             Small shared helpers (PageWidget base, Notify)
```
