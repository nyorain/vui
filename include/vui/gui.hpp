#pragma once

#include <vui/fwd.hpp>
#include <vui/input.hpp>
#include <vui/style.hpp>
#include <vui/container.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>
#include <nytl/rect.hpp>
#include <nytl/mat.hpp>
#include <nytl/stringParam.hpp>

#include <unordered_set>
#include <optional>

// TODO
// Make Gui::update return whether the gui has changed visually in any way and
// needs to be re-rendered.

namespace vui {

/// Native cursor types.
enum class Cursor : unsigned {
	pointer = 3,
	load,
	loadPtr,
	rightPtr,
	hand,
	grab,
	crosshair,
	help,
	beam,
	forbidden,
	size,
	sizeLeft,
	sizeRight,
	sizeTop,
	sizeBottom,
	sizeBottomRight,
	sizeBottomLeft,
	sizeTopRight,
	sizeTopLeft,
};

/// Can be implemented and passed to a Gui object to get notified about
/// certain events.
class GuiListener {
public:
	/// Static GuiListener object using the default (no op) implementation.
	static GuiListener& nop();

public:
	/// Called when a widget wants to copy a string to the clipboard.
	virtual void copy(std::string_view) {}

	/// Called when a widget wants to change the cursor.
	virtual void cursor(Cursor) {}

	/// Called when a widget wants to read a string from the clipboard.
	/// When the GuiListener can deliver such a string, it should call
	/// Gui::paste with that string and the given widget.
	/// The gui will make sure that widget wasn't deleted in the mean time.
	/// When unable to fulfill, should return false or (if an error
	/// occurs later) call Gui::paste with an empty string.
	virtual bool pasteRequest(const Widget&) { return false; }

	/// Called every time the selection changes (e.g. of a textfield).
	/// On linux, the GuiListener might forward this selection
	/// to the system (x11/wayland primary).
	virtual void selection(std::string_view) {}

	/// Called every time the focus changes.
	/// First parameter is the old focused widget, second parameter
	/// the new one. Any of those parameters can be nullptr, indicating
	/// no focused widget.
	virtual void focus(Widget*, Widget*) {}

	/// Called every time widget over which the mouse hovers changes.
	/// First parameter is the old hovered widget, second parameter
	/// the new one. Any of those parameters can be nullptr, indicating
	/// no hovered widget.
	virtual void mouseOver(Widget*, Widget*) {}
};

/// Central gui object. Manages all widgets and is entry point for
/// input and rendering.
class Gui : public ContainerWidget {
public:
	/// Constants to be parameterized
	static constexpr auto hintDelay = 1.f; // seconds
	static constexpr auto hintOffset = Vec {20.f, 5.f}; // seconds
	static constexpr auto blinkTime = 0.5f; // seconds

public:
	Gui(Context& context, const Font& font,
		GuiListener& listener = GuiListener::nop());
	Gui(Context& context, const Font& font, Styles&& s,
		GuiListener& listener = GuiListener::nop());

	/// Makes the Gui process the given input.
	Widget* mouseMove(const MouseMoveEvent&) override;
	Widget* mouseButton(const MouseButtonEvent&) override;
	void focus(bool gained) override;
	void mouseOver(bool gained) override;

	/// Update should be called every frame (or otherwise as often as
	/// possible) with the delta frame time in seconds.
	/// Needed for time-sensitive stuff like animations or cusor blinking.
	void update(double delta) override;

	/// Should be called once every frame when the device is not currently
	/// using the rendering resources.
	/// Will update device resources.
	/// Returns whether a rerecord is needed.
	bool updateDevice() override;

	/// Renders all widgets.
	void draw(vk::CommandBuffer) const override;

	/// Changes the transform to use for all widgets.
	void transform(const nytl::Mat4f&);

	/// Returns a descendent (so e.g. the child of a child) which currently
	/// has focus/over which the mouse hovers.
	/// Effectively traverses the line of focused/mouseOver children.
	/// Returns nullptr if there is currently none.
	Widget* mouseOver() const { return globalMouseOver_; }
	Widget* focus() const { return globalFocus_; }

	/// See ContainerWidget.
	using ContainerWidget::create;
	using ContainerWidget::add;
	using ContainerWidget::remove;
	using ContainerWidget::destroy;

	/// Can be used by a GuiListener implementation to answer a pasteRequest
	/// as soon as the data is available.
	/// The widget parameter must be the same from the pasteRequest.
	/// Returns false when the widget didn't submit a paste request
	/// or was removed.
	bool paste(const Widget& widget, std::string_view);

	Context& context() const override { return context_; }
	const Font& font() const { return font_; }
	const nytl::Mat4f transform() const { return transform_.matrix(); }
	const auto& styles() const { return styles_; }

	GuiListener& listener() { return listener_.get(); }
	void rerecord() { rerecord_ = true; }

	/// Internal widget helpers
	void addUpdate(Widget&);
	void addUpdateDevice(Widget&);
	void removed(Widget&);
	void moveDestroyWidget(std::unique_ptr<Widget>);
	void pasteRequest(Widget&);

protected:
	using Widget::gui;
	using Widget::contains;
	using Widget::updateScissor;

	// TODO: implement them
	void bounds(const Rect2f& r) override { Widget::bounds(r); }
	void hide(bool) override { /* Widget::hide(h); */ }
	bool hidden() const override { return false; }
	Rect2f scissor() const override { return rvg::Scissor::reset; }

protected:
	Context& context_;
	const Font& font_;
	std::reference_wrapper<GuiListener> listener_;
	std::unordered_set<Widget*> update_;
	std::unordered_set<Widget*> updateDevice_;
	std::pair<Widget*, MouseButton> buttonGrab_ {};
	bool rerecord_ {};
	rvg::Transform transform_ {};

	std::vector<std::unique_ptr<Widget>> destroyWidgets_;
	std::vector<Widget*> pasteRequests_;

	std::optional<DefaultStyles> defaultStyles_;
	Styles styles_;

	Widget* globalFocus_ {};
	Widget* globalMouseOver_ {};
};

} // namespace vui
