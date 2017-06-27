# Introduction

## Platforms

To make it in portable C11. OpenBSD, ubuntu, and fedora are targets. 
In future support for Windows and OSX will probably be added but it
is not a priority.

## Basics

At a basic level, Mace functions like any other text editor. 
When you type, text is added at the position of the cursor. 

The cursor is moved by clicking positions in the text. 
Sections of text can be selected by clicking and dragging.
You can then delete the selected text with the backspace,
delete key, or by typing a replacement.

You can scroll up and down by using the scroll wheel, 
or by clicking positions on the scroll bar. 
You can move between tabs by clicking on them in the tab
bar at the top of the screen.

## Layouting

Mace will have to sit on top of the OS and manage its own sub-windows 
in a way similar to what a tiling window manager does. Mace will allow
you to split your window verticley and horizontally into multiple panes.
Each pane will contain an arbitary number of tabs. 
Each tab will have two text box's. One is called the action bar. It a kind
of scratch area where you can type whatever you want and it will not
be saved with the file. It is intended to give you a place to write commands
that you can then run.

## Commands

There are commands in the action bar (the textbox above 
the main textbox where your files contents go). You can
right click them to select, then when you release they are
run. If you move your cursor away from the selection before
releasing they will not be run. 

You can type the commands anythere you want and they
will work the same. 

What the different commands do should be self explantory.

## Further

A text editor with multiple panes, tabs and some hard coded commands is
not very useful in itself so Mace will need more. Originally we had planned
to have Lua integrated so users could extend Mace with their own commands.
However we have decided to ditch this for ease of development as we felt
the benefits did not outweight the drawbacks.

If you have any ideas for features that would make a simple editor
a great one please tell us somewhere.
