# Mace [![Travis build Status](https://travis-ci.org/DandyHQ/mace.svg?branch=master)](https://travis-ci.org/DandyHQ/mace)

A modern Unix text editor taking inspiration from Acme, Emacs and
Atom.

## Screenshot
![Screenshot](https://github.com/DandyHQ/mace/blob/master/docs/screenshot.png)

## About

Mace is a programer's text editor that supports the following features:

* Executing any text in any window as a command
* Customisable hot bar for regularly used commands
* Support for writing custom commands and adding them to the current session
* Tabbed interface

## To Do

There are many key features left to add to Mace for it to make Alpha:

* Tiling interface (development delayed)
* Support Undoing edits
* Remove Lua runtime
* Utf8
* Syntax highlighting
* Smart indenting
* Keybord shortcuts
* Session management
* Multiple selections
* Lots of small things.

## Building

Mace requires the following dependencies:

 - [meson](https://github.com/mesonbuild/meson)
 - xlib
 - fontconfig
 - freetype2
 - harfbuzz
 - cairo
 - lua 5.3

#### Ubuntu 16.04

```
# apt install meson libx11-dev libharfbuzz-dev libharfbuzz-icu0 libcairo2-dev lua5.3-dev
```

#### Fedora 25

```
# dnf install meson libX11-devel harfbuzz-devel harfbuzz-icu cairo-devel lua-devel
```

#### OpenBSD 6.1

```
# pkg_add harfbuzz cairo lua%5.3
```

### Compiling

Once you have the dependencies installed, Mace can be compiled as
follows:

```
$ mkdir build
$ meson build
$ ninja -C build

# To run
$ ./build/mace

# or to install

# ninja -C build install
$ mace

```

If you'd like to run the test suite to ensure Mace is functioning correctly, they can be run from ninja using:

```
$ ninja -C build test
```

## Documentation

Mace documentation is in the [docs](docs) folder.

## Issues & Support

Please [file](https://github.com/DandyHQ/mace/issues) any bugs or feature requests.

## Contributing

The Mace developers welcomes contributions in the form of Pull Requests.
