#pragma once

#include <nytl/nonCopyable.hpp>
#include <nytl/rect.hpp>
#include <nytl/vec.hpp>
#include <vkpp/fwd.hpp>

/// Abstraction over Wdiget and WidgetContainer (and therefore Gui).

namespace vui {

using namespace nytl;

/// Abstract node in the widget hierachy.
/// Can e.g. be a Widget, a WidgetContainer or the Gui object itself.
/// Basically handles and propagates input, drawing, transforms etc.
class Node : public nytl::NonMovable {
public:
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
	virtual bool contains(Vec2f) const = 0;

	/// Instructs the widget to make sure that no rendering happens
	/// outside the given rect (additionally to its own bounds).
	/// Resets any previous scissor intersections. Given in gui coordinates.
	/// Needed to make things like scrolling in the parent work.
	virtual void intersectScissor(const Rect2f&) = 0;

	/// Records all needed command for drawing itself into the given
	/// CommandBuffer. The CommandBuffer must be in recording state.
	virtual void draw(vk::CommandBuffer) const {}


	// - input processing -
	/// All positions are given in gui coordinates.
	/// Must return the Widget that processed the event which might be itself
	/// or a child widget or none (nullptr).
	/// If e.g. the widget is transparent at the given position and therefore
	/// does not interact with a mouse button press it should return nullptr.
	virtual Widget* mouseMove(const MouseMoveEvent&) { return nullptr; }
	virtual Widget* mouseButton(const MouseButtonEvent&) { return nullptr; }
	virtual Widget* mouseWheel(const MouseWheelEvent&) { return nullptr; }
	virtual Widget* key(const KeyEvent&) { return nullptr; }
	virtual Widget* textInput(const TextInputEvent&) { return nullptr; }

	/// Called when the node receives or loses focus.
	/// Sent before the event that triggered it if any (e.g. a button click
	/// on this node).
	virtual void focus(bool) {}

	/// Called when the mouse enters or leaves this node.
	/// Followed by a MouseMoveEvent indicating the cursor position.
	virtual void mouseOver(bool gained) { (void)(gained); }


	/// Notifies this Node that the gui transform was changed.
	/// Should make sure that all childrens use this transform when
	/// rendering.
	virtual void updateTransform(const nytl::Mat4f&) {}

	/// Returns the associated gui object.
	virtual Gui& gui() const = 0;
	Context& context() const;
};

} // namepsace vui
