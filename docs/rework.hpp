#pragma once

#include <vui/input.hpp>
#include <nytl/nonCopyable.hpp>
#include <nytl/rect.hpp>
#include <nytl/vec.hpp>
#include <vkpp/fwd.hpp>
#include <rvg/state.hpp>
#include <functional>
#include <memory>

namespace vui {

using namespace nytl;
class ContainerWidget;
class Widget;
class Gui;

/// Can be passed to a Widget for size to let it choose its own size.
/// Allowed to just set this in one component and choose the other one
/// fixed.
constexpr auto autoSize = -1.f;

/// A widget with fixed bounds.
/// Must not be something visible, may be a layouting or container widget.
/// There are two types of coordinate spaces relevant for widgets:
/// - local: does not depend on widgets transform/position. In these
///   cordinates, the widget always has an axis aligned bounding box with
///   the top-left corner at (0,0)
/// - global: coordinates relative to the widgets global frame of reference.
///   Isn't necessarily the gui space, there can be widgets that create
///   their own frame of reference.
class Widget {
public:
	virtual ~Widget() = default;

	/// Hides/unhides this node.
	/// When hidden, a node should make sure that itself and its children
	/// don't render anything.
	/// Hidden nodes will not receive input from their parents.
	virtual void hide(bool) = 0;

	/// Returns whether the widget is hidden.
	virtual bool hidden() const = 0;

	/// Returns whether the node contains the given point in global gui
	/// coordinates. Used e.g. to determine whether the cursor is over
	/// the node or not.
	virtual bool contains(Vec2f) const;

	/// Resizes this widget. Note that not all widgets are resizable equally.
	/// Some might throw when an invalid size is given or just display
	/// their content incorrectly.
	/// Implementations must call Widget::bounds to update the bounds.
	virtual void size(Vec2f size) { bounds({position(), size}); };

	/// Changes the widgets position. Can be seen as offset of
	/// the local coordinate space to the global one.
	/// Implementations must call Widget::bounds to update the bounds.
	virtual void position(Vec2f pos) { bounds({pos, size()}); }

	/// Completely moves and resizes the widget.
	/// Like the size method, this is able to resize the widget which isn't
	/// supported by all widget types equally.
	/// Implementations must call Widget::bounds to update the widgets bounds.
	virtual void bounds(const Rect2f& bounds) = 0;

	/// Records all needed command for drawing itself into the given
	/// CommandBuffer. The CommandBuffer must be in recording state.
	virtual void draw(vk::CommandBuffer) const {}

	/// Transforms a given global position into local coordinates.
	/// The default implementation just subtracts the own position.
	virtual Vec2f toLocal(Vec2f pos) const;

	// - input processing -
	/// All positions are given in local coordinates.
	/// Must return the Widget that processed the event which might be itself
	/// or a child widget or none (nullptr).
	virtual Widget* mouseMove(const MouseMoveEvent&) { return nullptr; }
	virtual Widget* mouseButton(const MouseButtonEvent&) { return nullptr; }
	virtual Widget* mouseWheel(const MouseWheelEvent&) { return nullptr; }
	virtual Widget* key(const KeyEvent&) { return nullptr; }
	virtual Widget* textInput(const TextInputEvent&) { return nullptr; }

	/// Called when the node receives or loses focus.
	/// Sent before the event that triggered it if any (e.g. a button click
	/// on this node).
	virtual void focus(bool gained) { (void)(gained); }

	/// Called when the mouse enters or leaves this node.
	/// Followed by a MouseMoveEvent indicating the cursor position.
	virtual void mouseOver(bool gained) { (void)(gained); }

	/// Returns the effective area in which this node will render.
	/// Must be smaller than the scissor of its parent.
	virtual nytl::Rect2f scissor() const;

	/// Returns the transform of the frame of reference this widget uses.
	/// Note that this transform is absolute and already contains any
	/// outer frame of references. It transform local coordinates
	/// into normalized window space (vulkan).
	virtual nytl::Mat4f transform() const;

	/// Called when the Widget has registered itself for update.
	/// Gets the delta time since the last frame in seconds.
	/// Must not touch resources used for rendering.
	virtual void update(double) {}

	/// Called when the Widget has registered itself for updateDevice.
	/// Called when no rendering is currently done, so the widget might
	/// update rendering resources.
	virtual void updateDevice() {}

	/// Returns the parent of this widget
	/// A widget may not have a parent.
	ContainerWidget* parent() const { return parent_; }

	/// All values are given in global space.
	const Rect2f& bounds() const { return bounds_; }
	Vec2f position() const { return bounds_.position; }
	Vec2f size() const { return bounds_.size; }

	/// Returns the associated gui object.
	Gui& gui() const { return gui_; }
	Context& context() const;

protected:
	Widget(Gui& gui, ContainerWidget* parent) :
		gui_(gui), parent_(parent) {}

	/// Registers this widget for an update/updateDevice callback as soon
	/// as possible.
	void registerUpdate();
	void registerUpdateDevice();

	/// Makes sure all state for rendering is bound.
	/// Will bind the scissor.
	void bindScissor(vk::CommandBuffer cb) const;

	/// Returns the logical scissor used by this widget.
	/// Will be intersected with intersectScissor to result
	/// in the effective scissor.
	virtual Rect2f ownScissor() const { return bounds_; }

	/// Sets the parent of this widget.
	/// Can be null. Allows derived classes to manage (especially add/remove)
	/// their children.
	static void parent(Widget& widget, ContainerWidget* newParent);

	/// The cursor that should be used when the cursor hovers over this
	/// widget. Normal pointer by default.
	/// Will be set everytime the cursor enters this widget. Widgets can
	/// stil change it dynamically.
	virtual Cursor cursor() const;

private:
	Gui& gui_; // associated ui
	Rect2f bounds_; // global bounds
	ContainerWidget* parent_ {}; // optional parent
	mutable rvg::Scissor scissor_; // mutable since only created when needed
};

/// Owns widgets and manages them, i.e. forwards input and rendering calls.
class ContainerWidget : public Widget {
public:
	/// Raises/lowers the first widget above/below the second one.
	virtual void raiseAbove(const Widget& raise, const Widget& above);
	virtual void lowerBelow(const Widget& lower, const Widget& below);

	/// Return the highest/lowest widgets ordering-wise.
	virtual const Widget* highestWidget() const;
	virtual const Widget* lowestWidget() const;

	/// Returns whether the given widget is a direct child of this one.
	/// Returns false for itself.
	virtual bool isChild(const Widget&) const;

	/// Returns whether the given widget is a transitive child of this one.
	/// Returns false for itself.
	virtual bool isDescendant(const Widget&) const;

	/// To be called when a child changes properties relevant for this
	/// such as position, size or visibility.
	/// Will invalidate cached information, e.g. query the matching
	/// child from scratch on the next mouse event.
	virtual void childChanged();

	/// Overridden widget functions. Propagated to matching children.
	Widget* mouseMove(const MouseMoveEvent&) override;
	Widget* mouseButton(const MouseButtonEvent&) override;
	Widget* mouseWheel(const MouseWheelEvent&) override;
	Widget* key(const KeyEvent&) override;
	Widget* textInput(const TextInputEvent&) override;
	void focus(bool gained) override;
	void mouseOver(bool gained) override;
	void draw(vk::CommandBuffer) const override;

	/// Returns the child widget with focus/over which the mouse hovers.
	/// Returns nullptr if there is no such child.
	Widget* childMouseOver() const { return mouseOver_; }
	Widget* childFocus() const { return focus_; }

protected:
	using Widget::Widget;

	/// Returns the first widget at this position or nullptr
	/// if there is none. Will never return this.
	virtual Widget* widgetAt(Vec2f pos);
	virtual void refreshMouseOver(Vec2f pos);
	virtual void refreshFocus();

	virtual std::unique_ptr<Widget> remove(const Widget&);
	virtual Widget& add(std::unique_ptr<Widget>);

	/// Creates a widget of the given type with the given arguments and
	/// adds it to this container. The widget must have a
	/// constructor(Gui&, Args&&...).
	/// Returns a reference to the created widget.
	template<typename W, typename... Args>
	W& create(Args&&... args) {
		static_assert(std::is_base_of_v<Widget, W>, "Can only create widgets");
		auto widget = std::make_unique<W>(gui(), std::forward<Args>(args)...);
		auto& ret = *widget;
		add(std::move(widget));
		return ret;
	}

protected:
	// sorted by z order (if there is any)
	std::vector<std::unique_ptr<Widget>> children_;
	Widget* mouseOver_ {};
	Widget* focus_ {};
	bool invalidated_ {}; // whether to query mouse over from scratch
};

class Pane : public ContainerWidget {
public:
protected:
	Widget* child_;
};

class Window : public Pane {
public:
protected:
	rvg::Transform transform_;
};

class ScrollArea : public Pane {
public:
protected:
	rvg::Transform transform_;
	Vec2f offset_; // positive
	Vec2f scrollSize_;
};

/// ContainerWidget that exposes functionality for creating new widgets.
class DynamicLayoutWidget : public ContainerWidget {
public:
	using ContainerWidget::ContainerWidget;

	/// Creates a widget with the given arguments and adds it to the window.
	/// Will automatically position and size it, see createSized if you
	/// explicitly want to specify the size.
	/// The Widget must be constructible with (Gui&, Rect2f, Args...) as
	/// arguments.
	/// Returns a reference to it.
	template<typename W, typename... Args>
	W& create(Args&&... args) {
		return createSized<W>(nextSize(), std::forward<Args>(args)...);
	}

	/// Like create but lets the caller explicitly specify the size passed
	/// to the widget.
	template<typename W, typename... Args>
	W& createSized(Vec2f size, Args&&... args) {
		return ContainerWidget::create<W>(Rect2f {nextPosition(), size},
			std::forward<Args>(args)...);
	}

	/// Since in a layout widget no children should overlap,
	/// these don't have an effect. Instead rather preserve children order.
	void raiseAbove(const Widget&, const Widget&) override {}
	void lowerBelow(const Widget&, const Widget&) override {}

protected:
	virtual Vec2f nextSize() const = 0;
	virtual Vec2f nextPosition() const = 0;
};

class RowLayout : public DynamicLayoutWidget {
public:
	RowLayout(Gui&, ContainerWidget&, const Rect2f& bounds,
		Vec2f childSize = {autoSize, autoSize});
	Widget& add(std::unique_ptr<Widget>) override;

protected:
	Vec2f childSize_ {};
};

class ColumnLayout : public DynamicLayoutWidget {
public:
	ColumnLayout(Gui&, ContainerWidget&, const Rect2f& bounds,
		Vec2f childSize = {autoSize, autoSize});
	Widget& add(std::unique_ptr<Widget>) override;

protected:
	Vec2f childSize_;
};






// dat
namespace dat {

class Controller : public ContainerWidget {
public:
protected:
};

class Button : public Controller {
};

class Textfield : public Controller {
};

} // dat


// idea to add to ContainerWidget
// not sure if needed, we have moveBefore/moveAfter
using Iterator = std::vector<std::unique_ptr<Widget>>::iterator;
using ConstIterator = std::vector<std::unique_ptr<Widget>>::const_iterator;

/// Tries to find the given widget in the list of children.
/// Returns widgets_.end() if it could not be found.
Iterator find(const Widget&);
ConstIterator find(const Widget&) const;

virtual Widget& addBefore(Widget&, std::unique_ptr<Widget>);
virtual Widget& addAfter(Widget&, std::unique_ptr<Widget>);





// taken from containerWidget
// maybe rather name it sth like 'childBoundsChanged'? Because
// the container widget has to decide themself how to handle it,
// only _some_ may respond to it with a relayout

/// Abstract way to tell the widget that a child has changed
/// (e.g. size) and the whole layout needs to be recomputed.
/// In the general case not called automatically.
virtual void relayout() {}



// idea  for textfield undo
struct Action {
	// Whether something was erased
	// When this is true, str of the union is active, otherwise count
	bool erased;

	// the start position of the action
	unsigned position;

	union {
		unsigned count;
		std::string str;
	};
};

std::vector<Action> actions_;
// then add insert(start, str) and erase(start, count) functions and
// call them instead of manual manipulation

} // namepsace vui
