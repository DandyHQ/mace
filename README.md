# Mace [![Travis build Status](https://travis-ci.org/DandyHQ/mace.svg?branch=master)](https://travis-ci.org/DandyHQ/mace)

A modern Unix text editor taking inspiration from Acme, Emacs and
Atom.

## Screenshot
![Screenshot](https://github.com/DandyHQ/mace/blob/master/docs/screenshot.png)

## About

Mace is a programer's text editor that supports the following features:

* Executing any text in any window as a command
* Customisable hot bar for regularly used commands
* Tabbed interface

## To Do

There are many key features left to add to Mace for it to make Alpha:

* Tiling interface (development delayed)
* Support Undoing edits
* Unicode input from X11 / some way to input unicode.
* Syntax highlighting
* Smart indenting
* Keybord shortcuts
* Session management
* Multiple selections
* Lots of small things.

## Documentation

Mace documentation is in the [docs](docs) folder.

You should read [docs/Introduction.md](the introduction) for a run down
of how Mace is intended to work.

[docs/Development.md](docs/Development.md) gives is intended to help
explain how Mace is structured.

## Building

Mace requires the following dependencies:

 - [meson](https://github.com/mesonbuild/meson)
 - xlib
 - fontconfig
 - freetype2
 - cairo

#### Ubuntu 16.04

```
# apt install meson libx11-dev libcairo2-dev
```

#### Fedora 25

```
# dnf install meson libX11-devel cairo-devel
```

#### OpenBSD 6.1

```
# pkg_add cairo
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

## Issues & Support

Please [file](https://github.com/DandyHQ/mace/issues) any bugs or feature requests.

## Contributing

The Mace developers welcomes contributions in the form of Pull Requests.
