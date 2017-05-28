# Design

## Platforms

To make it in portable C11. OpenBSD, ubuntu, and fedora are targets. 
In future Windows and OSX support will probably be added. 

## Layouting

Mace will have to sit on top of the OS and manage its own sub-windows 
in a way similar to what a Window Manager does. There's three benefits 
to this

1. More control
2. Cross platform
3. Remote windows that don't suck

The current decision is to launch new windows as tabs. These tabs 
then can be dragged into vertical or horizontal tiles. See Atom as a 
way this works

If a new window tiles the same way as existing ones. They should be 
equal width instead of performing a new split. (this would require a 
structure other than a binary tree). Atom does this. Stumpwm (and 
likely ratpoison), and terminix don't


## Lua

Mace can be configured and controlled with lua. Lua code can be run
and added during runtime. This should allow for a large amount of
flexibilty and power as Mace can be extended to do anything that uses
text. 


