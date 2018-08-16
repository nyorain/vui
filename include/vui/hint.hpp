#pragma once

#include <vui/widget.hpp>
#include <vui/style.hpp>

#include <rvg/shapes.hpp>
#include <rvg/text.hpp>

namespace vui {

/// Small popup hint that displays text and processes no input.
/// Not shown by default.
class Hint : public MovableWidget {
public:
	Hint(Gui&, Vec2f pos, std::string_view text);
	Hint(Gui&, const Rect2f& bounds, std::string_view text);
	Hint(Gui&, const Rect2f& bounds, std::string_view text, const HintStyle&);

	void label(std::string_view);
	void reset(const HintStyle&, const Rect2f&, bool force = false);
	void style(const HintStyle&, bool force = false);

	void size(Vec2f) override;
	using Widget::size;

	void hide(bool hide) override;
	void draw(vk::CommandBuffer) const override;
	bool hidden() const override;
	int zOrder() const override { return 1000; }

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
