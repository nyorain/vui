### vui

- [ ] more advanced textfield
  - [x] scrolling, clipping
  - [x] enter/escape
  - [x] selection
  - [ ] some basic shortcuts like ctrl-a (might need ny fixes)
- [ ] add hint to widget? only one pointer, will only be created when set
	- [ ] otherwise: add it to all major classes, Textfield, Controller etc
- [ ] vui: row
- [ ] maybe not a good idea to make widgets use a transform by default.
      cleaner implementation without? And better performance
	  maybe add universal 'layout(position, size)' method to every widget
	  that is also just called from the constructor?
- [ ] destruction/removal of widgets
	- [ ] make sure to rerecord
- [ ] vui: non-drawing widgets (like row/column) should not create
      transform/scissor objects
- [ ] vui: don't make windows manage layouting. Make them (like panes) manage
      exactly one embedded widget which may be a layout widget
	  - [ ] implement real layout-only container widgets
- [ ] idea: vui::WidgetWrapper: widget container that contains exactly
      one child widget (but can have additional stuff). Automatically
	  handles events and stuff. Abstraction over Panel, multiple vui::dat
	  Controllers
- [ ] work on paddings and margins. Should not be hardcoded but depend
      on font size (or overall scale, something like this)
	  - [ ] e.g. for checkbox, the padding should really depend on the
	        checkbox size. That in turn will usually dependent on the font size
- [ ] vui: validate styles passed to widgets (assert valid)
- [ ] readd vui::Slider (with (optional?) different style)
- [ ] implement vui cursor image callback in gui listener (e.g. textfield hover)
- [ ] vui: window scrolling
- [ ] document stuff
  - [ ] intro tutorial, getting started
- [ ] vui: label
- [ ] vui: window names
- [ ] vui: horizontal splitting line
- [ ] clipboard support (probably over Gui/GuiListener)
- [x] z widget ordering
  - [ ] temporary raise on one layer (reorder in vector)
  - [ ] allow widgets to change it? needed?
- [ ] vui: radio button
- [ ] don't use that much paints and descriptors for widgets
  -> optional dynamic new ext descriptor support
  -> advanced styling/themes
- [ ] widget styles, also spacings/paddings/margins/borders etc
- [ ] popups (needed for dropdown menu, tooltip)
- [ ] dropdown menu
- [ ] vui: performance optimziations #2
      - [ ] Especially don't call the size() function so often (in
	        deriving hirachies: every constructor again)
	  - [ ] ColorButon pane hides/unhides too often
	  - [ ] Eliminate redundant construct/change calls of rvg shapes as
	        possible
- [ ] vui: runtime style changing
	- [ ] every widget has to implement support
	- [ ] also change the gui style, all widgets (that use those styles)
	      should update/be updated
- [ ] tabs (vui::TabbedPane or something as class)
- [ ] better mouse/keyboard grabs
  - [ ] currently bugs when multiple button grabs
  - [ ] key grabs (needed?)
- [ ] beautiful demos with screenshots
- [ ] window decorations/integrate with tabs
- [ ] animations
- [ ] advanced widget sizing hints, min/max size (needed?)
- [ ] textfields/slider combos for ints/floats
- [ ] better,easier custom navigation (e.g. tab-based)
- [ ] custom grabbing slider
- [ ] window operations (move, resize) (?)
- [ ] graph widget, e.g. for frametimes
- [ ] drag and drop stuff (not sure if needed at all)


- [ ] mechanism to allow optimization in widget container?

```
/// Must be called when a child widget changed its position.
/// Will start from scratch when determining the widget under
/// cursor next time.
virtual void invalidateMouseOver();
```
