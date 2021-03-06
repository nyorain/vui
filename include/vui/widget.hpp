#pragma once

#include <vui/fwd.hpp>
#include <vui/input.hpp>
#include <rvg/state.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>
#include <nytl/rect.hpp>

namespace vui {

/// Can be passed to a Widget for size to let it choose its own size.
/// Allowed to just set this in one component and choose the other one
/// fixed.
constexpr auto autoSize = -1.f;

/// A Widget with fixed bounds.
/// Must not be something visible, may be a layouting or container widget.
/// All coordinates for the Widget are always in global space.
/// The Widget is defined through its axis-aligned bounding box.
class Widget : public nytl::NonMovable {
public:
	virtual ~Widget();

	/// Hides/unhides this widget.
	/// A hidden widget should not render anything.
	/// Hidden widgets will not receive input.
	virtual void hide(bool hide) = 0;

	/// Returns whether the widget is hidden.
	virtual bool hidden() const = 0;

	/// Returns whether the widget contains the given point
	/// Used e.g. to determine whether the cursor is over it.
	/// Default implementation returns true for all positions inside the bounds.
	virtual bool contains(Vec2f) const;

	/// Resizes this widget. Note that not all widgets are resizable equally.
	/// Some might throw when an invalid size is given or just display
	/// their content incorrectly.
	/// Can also mess up the parents layout so should only be called
	/// when parent explicitly allows it or caller can handle
	/// all unwanted effects.
	/// Implementations must call Widget::bounds to update the bounds.
	virtual void size(Vec2f size) { bounds({position(), size}); };

	/// Changes the widgets position in the global space.
	/// Can mess up the parents layout so should only be called
	/// when parent explicitly allows it or caller can handle
	/// all unwanted effects.
	/// Implementations must call Widget::bounds to update the bounds.
	virtual void position(Vec2f pos) { bounds({pos, size()}); }

	/// Completely moves and resizes the widget.
	/// Like the size method, this is able to resize the widget which isn't
	/// supported by all widget types equally.
	/// Can also mess up the parents layout so should only be called
	/// when parent explicitly allows it or caller can handle
	/// all unwanted effects.
	/// Implementations must call Widget::bounds to update the widgets bounds.
	virtual void bounds(const Rect2f& bounds) = 0;

	/// Records all needed command for drawing itself into the given
	/// CommandBuffer. The CommandBuffer must be in recording state.
	virtual void draw(vk::CommandBuffer) const {}

	/// Called when the Widget has registered itself for update.
	/// Gets the delta time since the last frame in seconds.
	/// Must not touch resources used for rendering.
	/// Can return true to signal that it needs to be redrawn
	/// Default implementation just warns (since it was explicitly added
	/// to the guis update list without implementation) and returns false.
	virtual bool update(double);

	/// Called when the Widget has registered itself for updateDevice.
	/// Called when no rendering is currently done, so the widget might
	/// update rendering resources.
	/// Can return true to signal that a rerecord is needed.
	/// Default implementation just warns (since it was explicitly added
	/// to the guis updateDevice list without implementation) and returns false.
	virtual bool updateDevice();

	/// Returns the effective area outside which this widget and
	/// all its children must not render.
	/// Should never be larger than the parents scissor.
	virtual Rect2f scissor() const;

	// - input processing -
	/// All positions are given global coordinates.
	/// Must return the Widget that processed the event which might be itself
	/// or a child widget or none (nullptr).
	virtual Widget* mouseButton(const MouseButtonEvent&) { return nullptr; }
	virtual Widget* mouseMove(const MouseMoveEvent&) { return nullptr; }
	virtual Widget* mouseWheel(const MouseWheelEvent&) { return nullptr; }
	virtual Widget* key(const KeyEvent&) { return nullptr; }
	virtual Widget* textInput(const TextInputEvent&) { return nullptr; }

	/// Called when the widget receives or loses focus.
	/// Sent before the event that triggered it if any (e.g. a button click
	/// on this widget).
	virtual void focus(bool gained) { (void)(gained); }

	/// Called when the mouse enters or leaves this widget.
	/// Followed by a MouseMoveEvent indicating the cursor position.
	/// Overriding implementations should always make sure to call the
	/// base implementation to correctly (re-)set the cursor.
	virtual void mouseOver(bool gained);

	/// Called from parent. Update internal rendering state.
	virtual void updateScissor();

	/// Returns whether this widget is descendent of the given widget.
	/// Returns false for itself.
	virtual bool isDescendant(const Widget&) const;

	/// Returns whether this widget is in the gui hierachy.
	/// This is true if and only if it is descendant of its associated
	/// gui object. Note that widgets outside the hierachy will probably
	/// not function correctly.
	virtual bool inHierachy() const;

	/// Returns the parent of this widget
	/// A widget may not have a parent.
	virtual ContainerWidget* parent() const { return parent_; }

	/// Returns the associated gui object.
	Gui& gui() const { return gui_; }
	virtual Context& context() const;

	/// All values are given in gui space.
	const Rect2f& bounds() const { return bounds_; }
	Vec2f position() const { return bounds_.position; }
	Vec2f size() const { return bounds_.size; }

protected:
	Widget(Gui& gui, ContainerWidget* parent);

	/// Registers this widget for an update/updateDevice callback as soon
	/// as possible.
	void registerUpdate();
	void registerUpdateDevice();

	/// Returns the logical scissor used by this widget in external coordinates.
	/// Will be intersected with intersectScissor to result
	/// in the effective scissor.
	virtual Rect2f ownScissor() const { return bounds_; }

	/// Binds the scissor for this widget. Should only be called
	/// when this widget renders something itself. The scissor
	/// must always be bound, no matter if the Widget only renders
	/// inside its bounds since it might be additionally restriced
	/// by its parent.
	void bindScissor(vk::CommandBuffer cb) const;

	/// The cursor that should be used when the cursor hovers over this
	/// widget. Normal pointer by default.
	/// Will be set everytime the cursor enters this widget. Widgets can
	/// stil change it dynamically.
	virtual Cursor cursor() const;

	/// Must be called by widgets when they changed something visually
	/// and rendering their content now might look different than the last
	/// frame. Ignored if widget is not in hierachy.
	virtual void requestRedraw();

	/// Must be called by widgets when they invalidated or changed
	/// renderering resources in a way that requires a command buffer
	/// rerecord. Will automatically also request a redraw.
	/// Ignored if widget is not in hierachy.
	virtual void requestRerecord();

	/// May be called by gui after this widget submitted a paste request.
	virtual void pasteResponse(std::string_view) {}

	// - little bit of a hack that allows to call protected stuff on others -
	/// Sets the parent of this widget.
	/// Can be null. Allows derived classes to manage (especially add/remove)
	/// their children.
	static void parent(Widget& widget, ContainerWidget* newParent);

	/// Allows gui to call pasteResponse on widget.
	static void callPasteResponse(Widget&, std::string_view);

private:
	Gui& gui_; // associated gui
	Rect2f bounds_; // global bounds
	ContainerWidget* parent_ {}; // optional parent
	mutable rvg::Scissor scissor_; // mutable since only created when needed
};

} // namespace vui
