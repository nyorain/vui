The widget hierarchy
====================

ideas:

- don't make gui a ContainerWidget, gui itself isn't a widget. Instead
	- A: make Node overclass (Widget and gui), leads to the diamond of death
	- B: make gui have one toplevel managing container widget (probably better)
- force all widgets to be constructed with parent. Disallow the pointer
  constructor. With that we can remove the gui parameter and simply query
  the gui from parent
