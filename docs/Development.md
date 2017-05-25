# Files

I have tried to sensibly split up everything into files with sensible
names. But there is a lot of overlap at this stage so these rules
should be taken with a grain of salt.

- meson.build 
Meson control file. It's not particularly good.
If somebody could figure out how to make it find
dependencies better that would be great.

- mace.h 
Declares global functions and structures. I am trying to break this
up into more sepecific header files but mace is pretty tightly
coupled. 

- sequence.h
Declares structures and functions of sequences.

- lib.h
Decleares some helper functions.

- sequence.c
Impliments the functions in sequence.h

- font.c
Functions for handling fonts and font related things. This needs a
lot of improvements.

- mace.c
For managing a mace instance. Forwards things like user events to the
appropriout textbox.

- tab.c
Creation, management and drawing of tabs.

- textbox.c, textboxdraw.c
Functions for creating, managing and drawing text box's. textboxdraw.c
is paricually aweful and needs a lot of work.

- selection.c
Creation and management of selections. This may be unnecessary and
could probably be put into textbox.c.

- utf8.c
Small collection of utf8/text functions.

- lua.c
Lua api mappings.

- init.lua
A basic initialization script.

- xmain.c
All X11 related things. In future there will probably be one of
these for quarts, windows, wayland, etc.

# Events

When X11 gets an event it passes it to the appropriate handle 
function. This then gives it to the appropriate tab which give it to
the appropriate textbox. 

Button events have a location and a button integer. Motion has the new
location of the pointer. 

Key presses and releases are different depending on the key
pressed/released. Most of the time keys can be represented as a utf8
encoded string and length such as "a" if the A key is pressed. These
are given to handletyping. In other cases such as the enter key or the
escape key handlekey[press|release] is called with a key_t enum
given. This allows mace's key representation to be seperated from X11,
allowing it to be portable in future.

# Program Structure

Currently mace is one tab and that is all there is. It should be easy
to extend it to have multiple tabs but for now this is simpler.

The tab has two text box's. Each of the text box's has a piece chain for
its text, a cursor location, a buffer for pre rendering and some other
things. 

The piece chain is a doubly linked list of slices into a utf8 buffer. 
Read [1](https://github.com/martanne/vis/wiki/Text-management-using-a-piece-chain)
for an explination. The last added piece is growable the rest have 
fixed sizes. The idea with this is that it makes undo's and redos easy
as the text never gets removed. It also makes managing the resources
easy. It is just a free for the slices and a free for the data.

Text box's have fixed widths and variable heights. They have support
to be scrollable with the scroll bar drawn by their containing tab.

I don't like how I have done text box's and tabs. It is not very
functional and is very stateful. It works for now though.

Each textbox has a linked lists of selections. In future you will be
able to make multiple selections but for now you can only have one.

# Commands

If you right click on a selection (or a word, in which case
the word gets selected) mace gets the global lua object with the name
of the selection and calls it as a funciton. This function can then
use the global mace object to find the focus's textbox or tab, go
through selections, evaluate code, open new tabs, etc. What ever needs
to be done. If you look in `init.lua` you will see a few functions
that have been created so far such as `open`, `save`, `test`,
`eval`. `eval` is a particually intersting one as it allows you to
evaluate lua code during run time and add new functions.

# Lua

Lua is integrated into mace by providing bindings for various data
types. 

Tab's, Textbox's, and sequences are represented as user data 
objects that contain pointers to the c data. The user data 
structures are stored in lua's registry and are managed by lua's 
garbage collector. Before data is free'd on the C side it must call
luaremove(L, addr) to remove the object from the lua registry. 
Should a lua script try to use a object that has been free'd the 
lua wrapper functions will check if the address is in the registry,
if it is not then they fail. Thereby making the objects safe to 
use. There can be no dangling pointers and we don't have to 
constantly talk to lua to get data when working only in the C side 
of life.

# Fonts

Mace uses fontconfig to find fonts to use. So on startup mace asks
fontconfig for the default font for size 15px. Once mace has found a
font to use it gets freetype2 to do the heavy lifting and get
anti-aliased bitmaps that it can draw. 

This is not the best way to do things. For a start it forces mace to 
have to figure out all the layouts, so multi codepoint glyphs will 
almost definatly not render properly in mace, and any language that 
is not left to right will be wrong. 

In future I think we should convert to using pango, and having it
manage all the font picking and font rendering, or use harfbuzz to do
the text layouts. However I have not figured out any way to integrate
them nicely with text pieces/sequence structures. Both expect to have
an uninterupted string of of utf8 and a rectangle to draw to. 

So font handling will have to change at some point.

# Reading

- [Harfbuzz, pango, freetype, fontconfig](https://behdad.org/text/)
- [Text Piece Chain](https://github.com/martanne/vis/wiki/Text-management-using-a-piece-chain)
- [Some interesting text management documents](https://github.com/google/xi-editor/blob/master/doc/rope_science/intro.md) 
- [A Paper on Text Management](https://www.cs.unm.edu/~crowley/papers/sds.pdf),
it explains pieces chains well.
