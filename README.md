# Mace

A modern Unix text editor taking inspiration from Acme, Emacs and Atom.

## Build

You will need libraries and headers for:

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

## Development
Please see the [development document](https://github.com/DandyHQ/mace/wiki/Development).
