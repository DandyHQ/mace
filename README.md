# Mace

A modern Unix text editor taking inspiration from Acme, Emacs and
Atom.

## Build

You will need:

 - [meson](https://github.com/mesonbuild/meson)
 - xlib
 - freetype2
 - [utf8proc](https://github.com/JuliaLang/utf8proc)
 - lua 5.2+ (?)

```
$EDITOR meson.build     # Point meson to the correct libraries.
meson build
ninja -C build
$EDITOR init.lua        # Give loadfont a valid font path.
./build/mace
```

## Design

Please see the [design document](https://github.com/DandyHQ/mace/wiki/Design).

Mace is a text editor, but it is also a development environment. You
should be able to edit text as you please in a simple and intuitive
way that also allows you to do powerful operations. Mace aims to
achieve this by allowing you to use the lua extension language to
add features to the base system. The base system will allow you to
manage a number of panes each of which holds a number of tabs.

Tabs are where you text goes. Each tab has an action bar and a main
area. The main area stores you text while the action bar gives you a
separate area for writing functions and anything else you want to
temporarily store by your text. 

Mace then allows you to run commands by right clicking on them or by
selecting a section of lua code and right clicking on it. You can pass
code to commands by selecting the code then right clicking on the
command. You can also write the command in the tabs action bar and
right click to run a command with the tabs main area as its argument.

This in conjunction with multiple selections should provide a very
powerful environment for manipulating text. 

## Development

Please see the [development document](https://github.com/DandyHQ/mace/wiki/Development).

