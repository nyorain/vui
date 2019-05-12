#pragma once

#include <vui/fwd.hpp>
#include <vui/container.hpp>
#include <vui/style.hpp>
#include <rvg/shapes.hpp>

namespace vui {

constexpr struct ResizeWidgetTag {} resizeWidget = {};

/// Basic widget that can have one child.
/// Just displays iteself as background under the child widget.
/// Building block for e.g. windows and popups.
class Pane : public ContainerWidget {
public:
	/// Standard size, when auto size and no widget is given
	/// Only fallback, you usually want to avoid this
	static constexpr auto fallbackSize = nytl::Vec2f {200.f, 200.f};

public:
	/// When size of bounds is autoSize and a widget is given will simply
	/// take the widgets size, otherewise will resize the widget
	/// (if not null) to its own size.
	Pane(Gui& gui, ContainerWidget* parent, const Rect2f& bounds,
		std::unique_ptr<Widget> = {});
	Pane(Gui& gui, ContainerWidget* parent, const Rect2f& bounds,
		const PaneStyle& style, std::unique_ptr<Widget> = {});

	/// Changes the widget in this pane.
	/// Will destroy any previous widget stores in the pane.
	/// The passed widget can be empty (to empty the pane).
	/// When `resizeSelf` is true, will resize itself to fit the
	/// size of the widget, otherwise resize the widget to fit
	/// the panels bounds. Will always move the given widget
	/// to the correct position.
	void widget(std::unique_ptr<Widget>, bool resizeSelf);

	/// Creates a new widget of type W and sets it as child.
	/// Destroys any previous child.
	/// The bounds the new widget should take (matching the panes bounds)
	/// will automatically be passed to its constructor.
	/// Therefore there has to be constructor of W
	/// W(Gui&, ContainerWidget*, const Rect2f&, Args...).
	/// Returns a reference to the newly created widget.
	template<typename W, typename... Args>
	W& create(Args&&... args) {
		static_assert(std::is_base_of_v<Widget, W>, "Can only create widgets");
		auto widget = std::make_unique<W>(gui(), this, childBounds(),
			std::forward<Args>(args)...);
		auto& ret = *widget;
		this->widget(std::move(widget));
		return ret;
	}

	/// Like create but the user has to manually pass the widgets size
	/// and the pane will then adopt to the newly created widgets size.
	template<typename W, typename... Args>
	W& createResize(Vec2f size, Args&&... args) {
		static_assert(std::is_base_of_v<Widget, W>, "Can only create widgets");
		Rect2f bounds {childBounds().position, size};
		auto widget = std::make_unique<W>(gui(), this, bounds,
			std::forward<Args>(args)...);
		auto& ret = *widget;
		this->widget(std::move(widget), true);
		return ret;
	}

	/// Removes the current child widget and passes its ownership to the caller.
	/// If there was none, returns null.
	/// Note that the returned widget must be kept alive at least until
	/// the next frame, see ContainerWidget::remove for more information.
	std::unique_ptr<Widget> remove();

	/// Returns its single (optional) child.
	Widget* widget() const;

	virtual void reset(const PaneStyle& style, const Rect2f& bounds,
		bool forceReload = false, std::optional<std::unique_ptr<Widget>> = {});
	void style(const PaneStyle&, bool forceReload = false);
	void bounds(const Rect2f&) override;
	using ContainerWidget::bounds;

	void hide(bool hide) override;
	bool hidden() const override;
	void draw(vk::CommandBuffer) const override;

	const auto& style() const { return *style_; }

protected:
	Pane(Gui&, ContainerWidget*);

	using ContainerWidget::remove;

	/// Returns the bounds a child should have.
	virtual Rect2f childBounds() const;

	/// Just sets the child widget to the given one and destroys
	/// the previous (if there was one). Does not care about
	/// bounds/redrawing/rerecording. The passed widget can be null.
	void widget(std::unique_ptr<Widget>);

	/// Internally just calls widget.
	/// Since it's replacing instead of adding, this method
	/// should ne called/exposed.
	Widget& add(std::unique_ptr<Widget>) override;

protected:
	const PaneStyle* style_ {};
	RectShape bg_ {};
};

} // namespace vui
