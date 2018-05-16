#pragma once

#include <vui/fwd.hpp>
#include <vui/widget.hpp>
#include <vui/style.hpp>

#include <rvg/shapes.hpp>
#include <rvg/text.hpp>

#include <functional>
#include <string_view>

namespace vui {

class Textfield : public Widget {
public:
	std::function<void(Textfield&)> onChange;
	std::function<void(Textfield&)> onCancel;
	std::function<void(Textfield&)> onSubmit;

public:
	Textfield(Gui&, Vec2f pos, std::string_view start = "");
	Textfield(Gui&, const Rect2f& bounds, std::string_view start = "");
	Textfield(Gui&, const Rect2f& bounds, std::string_view start,
		const TextfieldStyle&);

	std::u32string_view utf32() const;
	std::string utf8() const;

	std::u32string_view utf32Selected() const;
	std::string utf8Selected() const;

	void utf8(std::string_view);
	void utf32(std::u32string_view);

	void size(Vec2f size) override;
	using Widget::size;

	void hide(bool hide) override;
	bool hidden() const override;

	Widget* mouseButton(const MouseButtonEvent&) override;
	Widget* mouseMove(const MouseMoveEvent&) override;
	Widget* textInput(const TextInputEvent&) override;
	Widget* key(const KeyEvent&) override;
	void focus(bool gained) override;

	void update(double delta) override;
	void draw(vk::CommandBuffer) const override;

	const auto& style() const { return style_.get(); }

protected:
	void updateCursorPosition();
	void updateSelectionDraw();
	void cursor(bool show, bool resetBlink = true, bool blink = true);
	void endSelection();
	unsigned charAt(float x);

protected:
	std::reference_wrapper<const TextfieldStyle> style_;

	RectShape bg_;
	RectShape cursor_;

	Text text_;

	unsigned cursorPos_ {};
	bool focus_ {false};
	double blinkAccum_ {};
	bool selecting_ {};
	bool blink_ {true};

	struct {
		RectShape bg;
		Text text;

		unsigned start;
		unsigned count;
	} selection_ {};
};

} // namespace vui
