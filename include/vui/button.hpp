#pragma once

#include <vui/fwd.hpp>
#include <vui/widget.hpp>
#include <vui/style.hpp>

#include <rvg/shapes.hpp>
#include <rvg/text.hpp>

#include <functional>
#include <string_view>

namespace vui {

/// Base class for all kinds of buttons.
/// Does not expose click events publicly but implements all required
/// functionality: styles, hint and click detection.
class BasicButton : public Widget {
public:
	/// Sets/updates the hint for this button.
	/// When an empty string view is passed, the hint
	/// will be disabled.
	void hint(std::string_view hint);

	void reset(const BasicButtonStyle&, const Rect2f&, bool force = false);
	void style(const BasicButtonStyle&, bool reload = false);

	void bounds(const nytl::Rect2f&) override;
	void hide(bool hide) override;
	bool hidden() const override;

	Widget* mouseButton(const MouseButtonEvent&) override;
	Widget* mouseMove(const MouseMoveEvent&) override;
	void mouseOver(bool gained) override;
	void draw(vk::CommandBuffer) const override;

	DelayedHint* hint() const { return hint_; }
	const auto& style() const { return *style_; }
	bool hovered() const { return hovered_; }
	bool pressed() const { return pressed_; }

protected:
	BasicButton(Gui&, ContainerWidget*);

	/// This method will be called when the button was clicked.
	/// Can be overriden to trigger an effect.
	virtual void clicked(const MouseButtonEvent&) {}
	virtual void updatePaints();
	const ButtonDraw& drawStyle() const;

	Cursor cursor() const override;

protected:
	const BasicButtonStyle* style_;
	RectShape bg_;
	Paint bgFill_;
	Paint bgStroke_;
	bool hovered_ {};
	bool pressed_ {};
	DelayedHint* hint_ {};
};

/// BasicButton with a label and publicly exposed click event.
class LabeledButton : public BasicButton {
public:
	std::function<void(LabeledButton&)> onClick;

public:
	LabeledButton(Gui&, Vec2f pos, std::string_view label);
	LabeledButton(Gui&, const Rect2f& bounds, std::string_view label);
	LabeledButton(Gui&, const Rect2f& bounds, std::string_view label,
		const LabeledButtonStyle&);

	/// Changes the buttons label.
	void label(std::string_view);
	void reset(const LabeledButtonStyle&, const Rect2f&, bool force = false,
		std::optional<std::string_view> label = std::nullopt);
	void style(const LabeledButtonStyle&, bool reload = false);

	void bounds(const nytl::Rect2f& rect) override;
	void hide(bool hide) override;
	void draw(vk::CommandBuffer) const override;

	const auto& style() const { return *style_; }

protected:
	LabeledButton(Gui&, ContainerWidget*, std::string_view label);

	void clicked(const MouseButtonEvent&) override;
	void updatePaints() override;

protected:
	const LabeledButtonStyle* style_;
	Text label_;
	Paint fgPaint_;
};

} // namespace vui

