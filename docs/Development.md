# Files

I have tried to sensibly split up everything into files with sensible
names. But there is a lot of overlap at this stage so these rules
should be taken with a grain of salt.

- meson.build 
Meson control file. It's not particularly good.
If somebody could figure out how to make it find
dependencies better that would be great.

- mace.h 
Declares global functions and structures. 

- sequence.c
Impliments the sequence functions.

- font.c
Functions for handling fonts and font related things. This needs a
lot of improvements.

- mace.c
For managing a mace instance. Forwards things like user events to the
appropriout textbox.

- pane.c
Creation, management and drawing of panes.

- tab.c
Creation, management and drawing of tabs.

- textbox.c, textboxdraw.c
Functions for creating, managing and drawing text box's. 

- selection.c
Creation and management of selections. This may be unnecessary and
could probably be put into textbox.c.

- utf8.h
A small library for utf8 functions.

- utf8.c
Implimenting above library.

- xmain.c
All X11 related things. In future there will probably be one of
these for quarts, windows, wayland, etc.

# User Input

When xmain.c (or whatever is iterfacing with the windowing system)
gets an event it passes it to the appropriate portable handle 
function. From here things get tricky. The mace structure has a textbox
for the mouse focus and a textbox for the keyboard focus. Key events get
given to the keyboard focus. Mouse releases and motions get sent to the 
mouse focus. For mouse presses, handlebuttonpress finds the pane that
was clicked on and sends the event to the panes focused tab. tabbuttonpress
then sets its mace's mousefocus to the textbox that was clicked on.

Yes, this is horrible. But for whatever reason I voted against C++ and 
going object oriented and I have not come up with a nice way to interface
everything.

Key presses and releases are different depending on the key
pressed/released. Most of the time keys can be represented as a utf8
encoded string and length such as "a" if the A key is pressed. These
are given to handletyping. In other cases such as the enter key or the
escape key handlekey[press|release] is called with a key_t enum
given. This allows mace's key representation to be seperated from X11,
allowing it to be portable in future.

# Fonts

Mace uses fontconfig to find fonts to use.  Once mace has found a
font to use it uses freetype2 to do the heavy lifting and get
anti-aliased bitmaps that it can draw. 

This is not the best way to do things. For a start it forces mace to 
have to figure out all the layouts, so multi codepoint glyphs will 
almost definatly not render properly in mace, and any language that 
is not left to right will be wrong. 

In future I think we should convert to using harfbuzz. I have had
an attempt at using this but it was not fully successful so I reverted
back to doing the positioning in Mace. This is sufficient for now.

# Text Box's and Sequences

Textbox's and sequences handle most of the text rendering and
hard stuff. The idea originally was to give each textbox a sequence,
have the sequence manage text insertions, deletion, undo and
redo, then have textbox's manage the rendering of said text.
However it seemed easier to have sequences also manage text
positioning and let textbox's do the drawing based of that. Now
that I think about it this could easily be seperated again. I need
to do some profiling and figure out if it is worth it.

Anyway, sequences use a piece chain to store text in a way that
makes adding, deleting, undoing and redoing easy. That being
said we have not yet implimented undo or redo. 

Read [1](https://github.com/martanne/vis/wiki/Text-management-using-a-piece-chain)
for a go run down.

The idea is that you put all your text in one append only buffer.
Then you have a doubly linked list of structures that point 
to different parts of the text buffer. Then when you add text
you append it to the buffer and add another element to the
linked list. When you delete you remove some elements and 
maybe add some that point to already added text. This way text
is never forgotten and you can undo and redo with relative ease.
It also makes managing things easy as you can put the linked 
list in another append only buffer and then you have two buffers
to free when you are done.

However because I decied to merge glyph placing with sequences
the text pieces also have an allocated array of glyphs that are used
for drawing. The idea is that only text pieces that are currently
in the chain need this so any that get removed have their glyph array
freed. My aim for this was to make updating glyph positions efficient
as the sequence knows which ones need updated and how to update
them. But now I am thinking it would be easier to have one big glyph
array allocated to store the current glyphs and reallocate it as the
glyph count changes. This would be less efficient but easier to handle.
As I said above some profiling needs to be done.

# Reading

- [Harfbuzz, pango, freetype, fontconfig](https://behdad.org/text/)
- [Text Piece Chain](https://github.com/martanne/vis/wiki/Text-management-using-a-piece-chain)
- [Some interesting text management documents](https://github.com/google/xi-editor/blob/master/doc/rope_science/intro.md) 
- [A Paper on Text Management](https://www.cs.unm.edu/~crowley/papers/sds.pdf),
