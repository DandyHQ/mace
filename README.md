# Mace
A modern Unix text editor

## Build

You will need libraries and headers for xlib, xft, and freetype2.
You will also need to edit font.c to give freetype2 a valid font.

```
make
./mace
```

## Design
Please see the [design document](https://github.com/DandyHQ/mace/wiki/Design).

## Development
Please see the [development document](https://github.com/DandyHQ/mace/wiki/Development).

## Essential UI Features

See [this list](https://dandyhq.github.io/features.html) for potential features beyond the scope of v1.0.

* Moveable panes and tabs
  * Tabs sit inside panes (not the other way around)
  * Files can be dropped into Mace as new panes/tabs
  * Tab bar on each pane does not overflow, and is navigated by scrolling (or clicking arrow buttons?)
  * New tabs are opened in a pane by middle-clicking the tab bar
  * New panes are made by dragging tabs towards the sides of pre-existing panes
* Command Bar
  * One command bar (rather than one per pane)
  * Editable (can add/remove commands by clicking and typing)
  * Commands are executed by right-clicking
  * Command bar overflows (no scrolling)
  * Custom commands can be added via an extension language
  * Navigate to other directories by clicking on parts of the path to the current file
* Directory Browsing
  * New folders are opened in a new tab. Browsing a directory in a tab does not automatically open new tabs.
* Shell
  * A tab can be used as a terminal

