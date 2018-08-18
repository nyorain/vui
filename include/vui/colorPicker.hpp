#pragma once

#include <vui/fwd.hpp>
#include <vui/widget.hpp>
#include <vui/style.hpp>
#include <vui/button.hpp>

#include <rvg/shapes.hpp>
#include <rvg/paint.hpp>
#include <rvg/text.hpp>

#include <functional>

namespace vui {

/// Basic hsv (hue, saturation, value) color picker.
/// Has a hue selector and a larger field with combined saturation/value
/// gradients.
class ColorPicker : public Widget {
public:
	/// Called everytime the selected color changes.
	/// Will be called rather often e.g. when sliding the selector.
	std::function<void(ColorPicker&)> onChange;

public:
	ColorPicker(Gui&, const Rect2f& bounds, const Color& start = {20, 20, 20});
	ColorPicker(Gui&, const Rect2f& bounds, const Color& start,
			const ColorPickerStyle& style);

	/// Picks the given color. Will not trigger an onChange callback.
	void pick(const Color& rgba);
	void pick(const Vec3f& hsv);

	/// Returns the currently selected color.
	Color picked() const;
	Vec3f currentHsv() const;
	float currentHue() const;
	Vec2f currentSV() const;

	void reset(const ColorPickerStyle&, const Rect2f&, bool force = false,
		std::optional<nytl::Vec3f> hsv = std::nullopt);
	void style(const ColorPickerStyle&, bool force = false);
	void relayout(const Rect2f&) override;

	void hide(bool hide) override;
	bool hidden() const override;

	Widget* mouseButton(const MouseButtonEvent&) override;
	Widget* mouseMove(const MouseMoveEvent&) override;
	void draw(vk::CommandBuffer) const override;

	const auto& style() const { return *style_; }

protected:
	ColorPicker(Gui&);

	Rect2f ownScissor() const override;
	void click(Vec2f pos, bool real);

protected:
	const ColorPickerStyle* style_;

	Shape hue_;
	RectShape hueMarker_;

	RectShape selector_;
	CircleShape colorMarker_;

	Paint basePaint_ {};
	Paint sGrad_ {}; // saturation gradient
	Paint vGrad_ {}; // value gradient

	bool slidingSV_ {};
	bool slidingHue_ {};
};

/// A button that shows a ColorPicker when pressed.
/// Displays the selected color as button "label".
class ColorButton : public BasicButton {
public:
	/// Called every time the selected color changes.
	/// Will be called rather often e.g. when sliding the selector.
	std::function<void(ColorButton&)> onChange;

public:
	ColorButton(Gui&, const Rect2f& bounds,
		const Vec2f& pickerSize = {autoSize, autoSize},
		const Color& start = {20, 20, 20});
	ColorButton(Gui&, const Rect2f& bounds, const Vec2f& pickerSize,
		const Color& start, const ColorButtonStyle& style);

	using Widget::size;
	using Widget::position;

	void size(Vec2f size) override;
	void position(Vec2f pos) override;

	void hide(bool hide) override;

	void focus(bool gained) override;
	void draw(vk::CommandBuffer) const override;

	const auto& style() const { return style_.get(); }
	const auto& cp() const { return *cp_; }
	auto picked() const { return cp().picked(); }

protected:
	void clicked(const MouseButtonEvent&) override;

protected:
	std::reference_wrapper<const ColorButtonStyle> style_;

	Paint colorPaint_;
	RectShape color_;
	Pane* pane_;
	ColorPicker* cp_;
};

} // namespace vui
