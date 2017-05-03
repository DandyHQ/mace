# Mace

A modern Unix text editor taking inspiration from Acme, Emacs and
Atom.

## Build

You will need:

 - [meson](https://github.com/mesonbuild/meson)
 - xlib
 - freetype2
 - cairo
 - lua 5.2+ (?)
 - [utf8proc](https://github.com/JuliaLang/utf8proc)

```
$EDITOR meson.build     # Point meson to the correct libraries.
meson build
ninja -C build
$EDITOR init.lua        # Give loadfont a valid font path.
./build/mace
```

## Development

Check the wiki.
