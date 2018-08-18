#pragma once

#include <vui/widget.hpp>
#include <vui/style.hpp>

#include <rvg/shapes.hpp>
#include <rvg/text.hpp>

namespace vui {

/// Small popup hint that displays text and processes no input.
/// Not shown by default.
class Hint : public Widget {
public:
	Hint(Gui&, ContainerWidget*, Vec2f pos, std::string_view text);
	Hint(Gui&, ContainerWidget*, const Rect2f& bounds, std::string_view text);
	Hint(Gui&, ContainerWidget*, const Rect2f& bounds, std::string_view text,
		const HintStyle&);

	void label(std::string_view, bool resize);
	void reset(const HintStyle&, const Rect2f&, bool force = false,
		std::optional<std::string_view> label = std::nullopt);
	void style(const HintStyle&, bool force = false);
	void bounds(const Rect2f& bounds) override;
	using Widget::bounds;

	void hide(bool hide) override;
	void draw(vk::CommandBuffer) const override;
	bool hidden() const override;

	const auto& style() const { return *style_; }

protected:
	const HintStyle* style_;
	RectShape bg_;
	Text text_;
};

/// Hint that automatically shows itself after a certain amount of time.
class DelayedHint : public Hint {
public:
	using Hint::Hint;

	/// Whether the mouse is currently hovering the area for which a hint
	/// exists. The widget still has to update the Hints position.
	void hovered(bool);
	void update(double delta);

protected:
	bool hovered_ {};
	double accum_ {};
};

} // namespace vui
