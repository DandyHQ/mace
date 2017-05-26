# Mace [![Travis build Status](https://travis-ci.org/DandyHQ/mace.svg?branch=master)](https://travis-ci.org/DandyHQ/mace)

A modern Unix text editor taking inspiration from Acme, Emacs and
Atom.

## Building

Mace requires the following dependencies:

 - [meson](https://github.com/mesonbuild/meson)
 - xlib
 - fontconfig
 - freetype2
 - cairo
 - lua 5.3
 
### Ubuntu 16.04 Dependencies

To install the dependencies in Ubuntu 16.04 execute the next command:

```
sudo apt install meson libx11-dev libcairo2-dev lua5.3-dev
```

Then continue with the steps below to build Mace.

### Compiling

Once you have the dependencies install, Mace can be compiled as follows:

```
mkdir build
meson build
ninja -C build
./build/mace
```

The last command launches Mace.

If you'd like to run the test suite to ensure Mace is functioning correctly. They can be run from ninja using.

```
ninja -C build test
```
