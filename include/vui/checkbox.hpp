#pragma once

#include <vui/fwd.hpp>
#include <vui/widget.hpp>
#include <vui/style.hpp>

#include <rvg/shapes.hpp>
#include <functional>

namespace vui {

class Checkbox : public Widget {
public:
	std::function<void(Checkbox&)> onToggle;

public:
	Checkbox(Gui&, ContainerWidget*, const Rect2f& bounds);
	Checkbox(Gui&, ContainerWidget*, const Rect2f& bounds,
		const CheckboxStyle& style);

	/// If true, checks the checkbox, otherwise unchecks it.
	/// Will not trigger the onToggle callbabck.
	void set(bool);
	auto checked() const { return checked_; }
	void toggle() { set(!checked()); }

	void reset(const CheckboxStyle&, const Rect2f&, bool forceReload = false);
	void style(const CheckboxStyle&, bool forceReload = false);
	void bounds(const Rect2f&) override;
	using Widget::bounds;

	void hide(bool hide) override;
	bool hidden() const override;

	Widget* mouseButton(const MouseButtonEvent&) override;
	void draw(vk::CommandBuffer) const override;

	const auto& style() const { return *style_; }

protected:
	Checkbox(Gui&, ContainerWidget*);
	Cursor cursor() const override;

protected:
	const CheckboxStyle* style_ {};
	rvg::RectShape bg_;
	rvg::RectShape fg_;
	bool checked_ {};
};

} // namespace vui

