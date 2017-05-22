# Design

## Platforms

To make it portable ANSI C. OSX, Linux and BSDs are all targets. 
With Linux being first citizen. Windows is a non-target with the 
architecture of Mace being Unix inspired.

## Network

We should aim (though it might be a stretch) at a client server 
architecture.

#### The benefits

1. Multiple GUIs
2. Remote file editing
3. Collaborative editing
4. Modularity

#### The drawbacks

1. Complexity
2. Code duplication
3. Extra failure cases
4. Performance


## Hackability

Ideally Mace will be hackable in multiple languages. There are some 
drawbacks to this including complexity and performance. However this 
is also an area where other text editors fall down.

The complexity issues mainly resolve around handling package 
installation for users, and the dependencies of those packages. We'd 
need to find a way where we can have a powerful configuration for the 
user and still inject the packages in from our side (how does Emacs or 
Vim handle this?).

The simplest way of extending the editor is through IPC which poses 
a performance problem. We'd need to investigate this before 
proceeding.

Other ways of extending could be through "blessed" languages which 
run in process.

Ideally the editor can be hacked on through shell languages. This 
would give greatest flexibility and such extensions can be replaced 
in performance sensitive cases with faster languages. (See 
Herbsluftwm, Uzbl and Acme as examples)

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
