# Mace [![Travis build Status](https://travis-ci.org/DandyHQ/mace.svg?branch=master)](https://travis-ci.org/DandyHQ/mace)

A modern Unix text editor taking inspiration from Acme, Emacs and
Atom.

## Screenshot
![Screenshot](https://github.com/DandyHQ/mace-design/blob/master/screenshot.png)

## Building

Mace requires the following dependencies:

 - [meson](https://github.com/mesonbuild/meson)
 - xlib
 - fontconfig
 - freetype2
 - cairo
 - lua 5.3
 
#### Ubuntu 16.04 

```
# apt install meson libx11-dev libcairo2-dev lua5.3-dev
```

Then go to [Compiling](#compiling)

#### Fedora 25 

```
# dnf install meson libX11-devel cairo-devel lua-devel
```

Then go to [Compiling](#compiling)

#### OpenBSD 6.1

```
# pkg_add cairo lua%5.3
```

Then go to [Compiling](#compiling)

### Compiling

Once you have the dependencies install, Mace can be compiled as follows:

```
$ mkdir build
$ meson build
$ ninja -C build

$ ./build/mace

# or to install

# ninja -C build install
$ mace

```

The last command launches Mace.

If you'd like to run the test suite to ensure Mace is functioning correctly. They can be run from ninja using:

```
$ ninja -C build test
```
