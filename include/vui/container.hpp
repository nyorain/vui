#pragma once

#include <vui/fwd.hpp>
#include <vui/input.hpp>
#include <vui/widget.hpp>

namespace vui {

/// Abstract widget that owns and manages other widgets.
/// Can be somethin abstract like a layout, some kind of widget group
/// or Gui as root node itself.
/// Propagates input and drawing to all its children.
class ContainerWidget : public Widget {
public:
	// TODO: implement hide
	void hide(bool) override {}

	/// Raises/lowers the first widget above/below the second one.
	/// Returns false if any of the widgets isn't a direct child,
	/// both widgets are the same or the operation isn't supported.
	/// Will have no effect if the conditions are already met.
	virtual bool raiseAbove(const Widget& raise, const Widget& above);
	virtual bool lowerBelow(const Widget& lower, const Widget& below);

	/// Return the highest/lowest widgets ordering-wise.
	virtual const Widget* highestWidget() const;
	virtual const Widget* lowestWidget() const;

	/// Returns whether the given widget is a direct child of this one.
	/// Returns false for itself.
	virtual bool hasChild(const Widget&) const;

	/// Returns whether the given widget is a transitive child of this one.
	/// Returns false for itself.
	virtual bool hasDescendant(const Widget&) const;

	// TODO: expose this here, publicly? probably not good idea
	// not all container widgets have a layout concept

	/// Abstract way to tell the widget that a child has changed
	/// (e.g. size) and the whole layout needs to be recomputed.
	/// In the general case not called automatically.
	virtual void relayout() {}

	Widget* mouseMove(const MouseMoveEvent&) override;
	Widget* mouseButton(const MouseButtonEvent&) override;
	Widget* mouseWheel(const MouseWheelEvent&) override;
	Widget* key(const KeyEvent&) override;
	Widget* textInput(const TextInputEvent&) override;
	void focus(bool gained) override;
	void mouseOver(bool gained) override;
	void draw(vk::CommandBuffer) const override;
	void updateScissor() override;
	void bounds(const Rect2f&) override;

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

	/// Adds the given widget to the container.
	/// Derived classes may adjust its position/size before adding it.
	virtual Widget& add(std::unique_ptr<Widget>);

	/// If the given widget isn't a child, returns nullptr.
	/// Otherwise removes it locally and transfers ownership to the caller.
	/// Will always trigger a rerecord. When called during a frame,
	/// the caller must make sure that the returned Widget stays valid
	/// at least until the frame finishes (the next updateDevice turn), that's
	/// also the reason for the nodiscard.
	/// If you just want to destroy the widget, call destroy which
	/// manages the keeping alive autoomatically.
	/// The removed widget will be a orphan without parent and can generally
	/// not be used in any way until it's readded to the hierachy.
	/// Only call/expose this method if you know what you are doing.
	[[nodiscard]] virtual std::unique_ptr<Widget> remove(const Widget&);

	/// If the given widget isn't a child returns nullptr.
	/// Otherwise destroys the given widget (as soon as possible).
	/// It must not accessed in any way after this call.
	/// See also the remove method if you e.g. may want to readd
	/// the widget later on.
	virtual bool destroy(const Widget&);

	/// Creates a widget of the given type with the given arguments and
	/// adds it to this container. The widget must have a
	/// constructor(Gui&, ContainerWidget*, Args&&...).
	/// Returns a reference to the created widget.
	template<typename W, typename... Args>
	W& create(Args&&... args) {
		static_assert(std::is_base_of_v<Widget, W>, "Can only create widgets");
		auto widget = std::make_unique<W>(gui(), this,
			std::forward<Args>(args)...);
		auto& ret = *widget;
		add(std::move(widget));
		return ret;
	}

protected:
	// sorted by order in which they should be drawn and otherwise
	// by add order
	std::vector<std::unique_ptr<Widget>> widgets_;

	// both always direct children
	Widget* focus_ {};
	Widget* mouseOver_ {};
};

} // namespace vui
