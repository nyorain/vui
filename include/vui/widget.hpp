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

/// Represents an abstract node in the widget hierachy.
/// Must not be something visible, may be a layouting or container widget.
/// Every Widget has defined bounds outside which it should not render
/// and inside which it might receive input events.
class Widget : public nytl::NonMovable {
public:
	virtual ~Widget() = default;

	/// Hides/unhides this widget.
	/// A hidden widget should not render anything.
	/// Hidden widgets will not receive input.
	virtual void hide(bool hide) = 0;

	/// Returns whether the widget is hidden.
	virtual bool hidden() const = 0;

	/// Resizes this widget. Note that not all widgets are resizable equally.
	/// Some might throw when an invalid size is given or just display
	/// their content incorrectly.
	/// Implementations must call Widget::relayout to update them.
	virtual void size(Vec2f size) { relayout({position(), size}); };

	/// Changes the widgets global position.
	/// Implementations must call Widget::relayout to update them.
	virtual void position(Vec2f pos) { relayout({pos, size()}); }

	/// Completely moves and resizes the widget.
	/// Like the size method, this is able to resize the widget which isn't
	/// supported by all widget types equally.
	/// Implementations must call Widget::relayout to update the widgets bounds.
	virtual void relayout(const Rect2f& bounds) = 0;

	/// Returns whether the widget contains the given point in global gui
	/// coordinates. Used e.g. to determine whether the cursor is over
	/// the widget or not. Must return false for positions outside
	/// of it bounds.
	virtual bool contains(Vec2f point) const;

	/// Instructs the widget to make sure that no rendering happens
	/// outside the given rect (additionally to its own bounds).
	/// Resets any previous scissor intersections. Given in gui coordinates.
	virtual void intersectScissor(const Rect2f&);

	/// Called when the Widget has registered itself for update.
	/// Gets the delta time since the last frame in seconds.
	/// Must not touch resources used for rendering.
	virtual void update(double) {}

	/// Called when the Widget has registered itself for updateDevice.
	/// Called when no rendering is currently done, so the widget might
	/// update rendering resources.
	virtual void updateDevice() {}

	/// Records all needed command for drawing itself into the given
	/// CommandBuffer. The CommandBuffer must be in recording state.
	virtual void draw(vk::CommandBuffer) const {}

	/// The z order of this widget.
	/// Widgets	with a lower z order are drawn first.
	/// Note that the zOrder only matters for widgets with the same parent.
	/// zOrder 0 should be the default.
	virtual int zOrder() const { return 0; }

	// - input processing -
	/// All positions are given in gui coordinates.
	/// Must return the Widget that processed the event which might be itself
	/// or a child widget or none (nullptr).
	/// If e.g. the widget is transparent at the given position and therefore
	/// does not interact with a mouse button press it should return nullptr.
	virtual Widget* mouseButton(const MouseButtonEvent&) { return nullptr; }
	virtual Widget* mouseMove(const MouseMoveEvent&) { return nullptr; }
	virtual Widget* mouseWheel(const MouseWheelEvent&) { return nullptr; }
	virtual Widget* key(const KeyEvent&) { return nullptr; }
	virtual Widget* textInput(const TextInputEvent&) { return nullptr; }

	/// Called when the widget receives or loses focus.
	virtual void focus(bool) {}

	/// Called when the mouse enters or leaves the bounds widget.
	/// Overriding implementations should always make sure to call the
	/// base implementation to correctly (re-)set the cursor.
	virtual void mouseOver(bool);

	/// Returns the associated gui object.
	Gui& gui() const { return gui_; }
	Context& context() const;

	/// All values are given in gui space.
	const Rect2f& bounds() const { return bounds_; }
	Vec2f position() const { return bounds_.position; }
	Vec2f size() const { return bounds_.size; }

	/// Notifies this widget that the gui transform was changed.
	/// Only relevant for widgets that use custom transform (they
	/// must pre-multiply this transform).
	/// NOTE: usually not required to call as widget user directly.
	virtual void updateTransform(const nytl::Mat4f&) {}

protected:
	Widget(Gui& gui) : gui_(gui) {}

	/// Registers this widget for an update/updateDevice callback as soon
	/// as possible.
	void registerUpdate();
	void registerUpdateDevice();

	/// Makes sure all state for rendering is bound.
	/// Will bind the scissor.
	void bindScissor(vk::CommandBuffer cb) const;

	/// Can be changed by implementations to make this widget use
	/// another cursor.
	virtual Cursor cursor() const;
	virtual Rect2f ownScissor() const { return bounds_; }

	void updateScissor();
	const Rect2f& intersectScissor() const { return intersectScissor_; }

private:
	Gui& gui_; // associated gui
	Rect2f bounds_; // global bounds

	// Required even in purely logical (not drawing) nodes to correctly
	// propagate it to (new) children
	// Needed in all (!) drawing widgets to realize e.g. scrolling
	// in windows
	Rect2f intersectScissor_;
	mutable rvg::Scissor scissor_; // mutable since only created when needed
};

/// Widget that offsets its position using a custom transform.
/// Mainly used for widgets that change position often (like windows/hints)
/// so only the transform has to be recalculated on change.
/// Deriving classes only have to implement the size method, the position
/// part of relayout is handled by this widget. They also have to specify
/// everything that is rendered in local coordinates.
class MovableWidget : public Widget {
public:
	void updateTransform(const nytl::Mat4f&) override;
	void position(Vec2f) override;
	void relayout(const Rect2f& bounds) override;
	void size(Vec2f) override = 0;

protected:
	MovableWidget(Gui& gui);
	void bindTransform(vk::CommandBuffer cb) const;

private:
	rvg::Transform transform_;
};

} // namespace vui
