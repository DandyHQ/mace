# Mace
A modern Unix text editor

## Design
Please see the [design document](https://github.com/DandyHQ/mace/wiki/Design).

## Build

You will need libraries and headers: 

    - Xlib
    - freetype2
    - [libutf8proc](https://github.com/JuliaLang/utf8proc/releases/tag/v2.1.0)

You will also need to edit font.c to give freetype2 a valid font.

```
make
./mace
```

Or you can use meson. We need to decide which should be used. Meson is
not well suported but it seems interesting. Its advantages should be more
obvious once the project gets large:

```
meson . build
ninja -C build
```

## Files

I have tried to sensibly split up everything into files with sensible names.
But there is a lot of overlap at this stage so these rules should be taken
with a grain of salt.

 - Makefile   - The Makefile. Not a particularly good makefile.
 - mace.h     - Declares any global variables or functions.
 - main.c     - Where all global variables should be stored. Shouldn't be
                used for much else.
 - draw.c     - Functions for assisting with drawing.
 - font.c     - For handling fonts and font related things.
 - focus.c    - Deals with mouse and key events for the currently focused tab.
 - piece.c    - Managing text pieces.
 - mouse.c    - Handles mouse events such as tab re-arrangements.
 - pane.c     - Creation, management and drawing of panes.
 - tab.c      - Creation and drawing of tabs.
 - xmain.c    - All X11 related things.

## Structure

So far only the UI has been (partially) implemented. I works on X11 but I have
tried to separate everything from X11 to make porting to Wayland, Mac and
Windows easy.

All drawing is done to a buffer in bgra format. Each colour channel gets a byte
and for the most part the alpha channel is ignored once a colour has been drawn
to a buffer.

---Keys and button management will need to be changed. At the moment xmain passes
the button codes untouched which will probably work everywhere but isn't very
clean. The same is done for key code which is more worrying. In future I think
key codes should be translated into utf8 before being sent to the rest of the
program. An exception will have to be made for keys that do not represent
characters such as Return, Delete etc. However these are not so numerous as to
become overwhelming so we can probably just define them somewhere and translate
X11 keysyms into our own codes to make it portable.---

X11 translates key events into either utf8 encoded strings and passes it to
handletyping or into a keycode_t and passes it to handlekeypress/handlerelease.

The internal structure of the UI is similar to the visual structure of the UI
(at least as I see it). It consists of a binary tree of panes with linked lists
of tabs in each leaf pane. Non leaf panes are either vertical splits (into top
and bottom) or horizontal splits (left and right).

