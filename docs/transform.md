How to handle transform and position of widgets
===============================================

### A: only ever use global coordinates, only one central transform

pro: 0 overhead, really easy to understand, maintain, write, document
contra: moving stuff around (or things like collapsing folders) will have
	to upload every single drawn vertex of the whole widget hierachy again.

### B: give every widget its own transform

pro: moving stuff around gets cheaper (only transform change for every child)
contra: code more complicates (two coordinate systems), quite some
	memory overhead (4x4 matrix on device and that plus additional
	managing resources on cpu) per widget

Old documentation try in Widget:

```
/// Global coordinates space is the same for all widgets and fixed
/// for a gui object.
/// Local coordinates mean coordinates relative to the top-left corner of
/// the widget.
/// The only difference between local and global coordinates is the
/// position() offset, aside from that widgets always only use the
/// central gui transformation. Rendering resources as well as input events
/// always use local coordinates so widgets don't have to offset
/// position() everytime.
```

### C: only give often-moving widgets their own transform

pro: moving stuff around is (usually) super cheap (only one transform change),
	neglectable memory overhead (there will probably be not too
	much movable/top-level widgets)
contra: in some cases (e.g. layout changes) moving still has to upload
	every vertex of every child; introduces many coordinate systems and
	is really hard to document; get across. Arguing over frame of references
	vs global vs local coordinates can get confusing.


__We use method A for now. While B and C are optimizations they make
the code more complex. Might be added in later version if performance
while moving really becomes a problem__


## Widget transforms

There won't be fully custom transformable widgets since different
features _need_ scissors and when we e.g. rotate a Widget, we can't
define intersected scissors as rectangles anymore.

