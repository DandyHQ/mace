# Introduction

Mace is in a state of active development so a lot of the documentation
may be out of date and no longer relevant.

At a basic level, Mace functions like any other text editor. 
When you type, text is added at the position of the cursor. 
You can scroll with the scroll wheel, or by clicking on the scroll bar
at the point you want to scroll to. 

What you should see when you first open Mace is one pane, with 
one tab, with two text box's, and a scroll bar.

 Every tab has two text box's. The large one at the bottom is for
the files contents, it is called the main text box. The small one 
above is called the action text box. You can type whatever you want
there but generally it will be used for commands. The string of at the 
start followed by a colon is the file name of the tab.

However with Mace there can be an arbitrary number of cursors
and not every text box has one. You can place a cursor by left clicking.
If you type text will go to it. If you run commands or use key bindings
they will probably act on all the cursors (some act on where the command
was clicked, ie: save). If you middle click (this will probably change in
future) another cursor will be placed without removing any other cursors.
This allows you to have multiple cursors at once, each receiving input
from the keyboard, and each being acted on my commands.

There are commands in the action bar. You can
right click them to select, then when you release they are
run. If you move your cursor away from the selection before
releasing they will not be run. 

You can type the commands anythere you want and they
will work the same. 

What the different commands do should be self explantory.
