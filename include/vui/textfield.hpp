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
	/// Called everytime when the content is changed.
	std::function<void(Textfield&)> onChange;

	/// Called everytime when the textfield is unfocused
	/// without being sumitted (i.e. enter pressed or somewhere
	/// else clicked)
	std::function<void(Textfield&)> onCancel;

	/// Called everytime when the textfield is submitted (i.e. enter
	/// pressed)
	std::function<void(Textfield&)> onSubmit;

	// Add onSelection callback?

public:
	Textfield(Gui&, ContainerWidget*, Vec2f pos, std::string_view start = "");
	Textfield(Gui&, ContainerWidget*, const Rect2f& bounds,
		std::string_view start = "");
	Textfield(Gui&, ContainerWidget*, const Rect2f& bounds,
		std::string_view start, const TextfieldStyle&);

	/// Return the current textfield content.
	std::string_view utf8() const;
	std::u32string_view utf32() const;

	/// Returns the current selected string.
	/// If none is selected, an empty string is returned.
	std::string_view utf8Selected() const;
	std::u32string_view utf32Selected() const;

	/// Sets the content of this textfield.
	/// Note that this should not be done while it is editable since
	/// it will reset the current selection and the cursor position to 0.
	void utf8(std::string_view);
	void utf32(std::u32string_view);

	void reset(const TextfieldStyle&, const Rect2f&, bool force = false,
		std::optional<std::string_view> = std::nullopt);
	void style(const TextfieldStyle&, bool force = false);

	void hide(bool hide) override;
	bool hidden() const override;
	void bounds(const Rect2f& size) override;
	using Widget::bounds;

	Widget* mouseButton(const MouseButtonEvent&) override;
	Widget* mouseMove(const MouseMoveEvent&) override;
	Widget* textInput(const TextInputEvent&) override;
	Widget* key(const KeyEvent&) override;
	Widget* mouseWheel(const MouseWheelEvent&) override;
	void focus(bool gained) override;
	void mouseOver(bool gained) override;

	bool update(double delta) override;
	void draw(vk::CommandBuffer) const override;

	const auto& style() const { return *style_; }

protected:
	Textfield(Gui&, ContainerWidget*);

	void pasteResponse(std::string_view) override;

	/// Updates the selection render state based on the logical state.
	void updateSelectionDraw();

	/// Updates the cursor render state based on the logical state.
	void updateCursorPosition();

	void showCursor(bool);
	void blinkCursor(bool);
	void resetBlinkTime();

	/// Resets the selection if there is any.
	/// Otherwise has no effect.
	void endSelection();

	/// Returns the character id the cursor should take when it
	/// clicks on the given position. Uses a different measurement
	/// than rvg::Text::charAt in that it does not return the character
	/// under the given position but the character starting at the
	/// nearest boundary (since that is how textfields conventionally work).
	/// x ist the x coordinate in text-local coordinates.
	unsigned boundaryAt(float x);

	const TextfieldDraw& drawStyle() const;
	void updatePaints();
	Cursor cursor() const override;

protected:
	const TextfieldStyle* style_ {};

	RectShape bg_;
	RectShape cursor_;

	Paint bgPaint_;
	Paint bgStroke_;
	Paint fgPaint_;

	Text text_;
	std::u32string content_;

	unsigned cursorPos_ {}; // the character before which it rests
	bool focus_ {false};
	bool mouseOver_ {false};
	double blinkAccum_ {}; // in seconds
	std::optional<unsigned> selectionStart_ {}; // where mouse got down
	bool blink_ {true}; // whether cursor is blinking

	struct {
		RectShape bg;
		Text text;

		unsigned start {}; // character start
		unsigned count {}; // count of characters
	} selection_ {};
};

} // namespace vui
