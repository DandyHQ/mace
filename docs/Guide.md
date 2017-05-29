# User Guide

### Basics

At a basic level, Mace functions like any other text editor. 
When you type, text is added at the position of the cursor. 

The cursor is moved by clicking positions in the text. 
Sections of text can be selected by clicking and dragging, or deleted with the backspace or delete key.

You can scroll up and down by using the scroll wheel, with the Page-Up/Page-Down keys, or by clicking positions on the scroll bar.
You can move between tabs by clicking on them in the tab bar at the top of the screen.

### Commands

By default, several commands are available in an action bar, just below the tab bar at the top of the screen.
These commands can be executed by right-clicking on them. For example, right-clicking 'save' in the action bar
saves the current tab as a file. Since this function is defined, you can also execute the command by right-clicking
the word 'save' wherever it appears in the text you are editing.

Some functions (e.g. cut, copy, open) take input. For example, if you select some text and right-click 'copy',
the selected text will be copied to a buffer, ready to be pasted with the 'paste' command. Similarly,
the 'open' command will open (in a new tab) a file at the path specified by a selected section of text.

Custom commands can be added with the 'eval' command, which can be used to evaluate Lua code. Write your own function
in Lua, select the text, and right click eval. Now, whenever you right-click the name of that function, in the action bar
or the body of the text editor, the function will execute. The contents of the action bar can be edited simply by clicking
on it and typing.

mace.lua provides some examples for scripting with Mace. 
