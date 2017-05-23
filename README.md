# Mace

A modern Unix text editor taking inspiration from Acme, Emacs and
Atom.

## Build

You will need:

 - [meson](https://github.com/mesonbuild/meson)
 - xlib
 - fontconfig
 - freetype2
 - cairo
 - lua 5.2+ (?)

```
meson build
ninja -C build
./build/mace
```
