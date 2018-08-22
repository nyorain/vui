#pragma once

#include "fwd.hpp"
#include <rvg/paint.hpp>
#include <optional>
#include <array>

namespace vui {

struct ButtonDraw {
	rvg::PaintData bg;
	std::optional<rvg::PaintData> bgStroke {};

	// must be set for buttons types that have a foreground, e.g. label
	std::optional<rvg::PaintData> fg {};
};

struct BasicButtonStyle {
	ButtonDraw normal;
	ButtonDraw hovered;
	ButtonDraw pressed;
	std::array<float, 4> rounding = {};
};

struct LabeledButtonStyle {
	BasicButtonStyle* basic {};
	Vec2f padding = Vec {20.f, 10.f};
	const Font* font {};
};

struct TextfieldDraw {
	rvg::PaintData bg;
	rvg::PaintData text;
	std::optional<rvg::PaintData> bgStroke;
};

struct TextfieldStyle {
	TextfieldDraw normal;
	TextfieldDraw hovered;
	TextfieldDraw focused;
	rvg::Paint* selected; // only visible when focused
	rvg::Paint* cursor; // only visible when focused
	rvg::Paint* selectedText {}; // only visible when focused
	float cursorWidth = 1.f;
	Vec2f padding = Vec {10.f, 10.f};
	std::array<float, 4> rounding = {};
	const Font* font {};
};

struct PaneStyle {
	rvg::Paint* bg;
	rvg::Paint* bgStroke {};
	std::array<float, 4> rounding {0.f, 0.f, 0.f, 0.f};
	Vec2f padding = Vec {10.f, 10.f};
};

// TODO
struct SliderStyle {
	rvg::PaintData left;
	rvg::PaintData right;
};

struct HintStyle {
	rvg::Paint* bg; /// Background paint
	rvg::Paint* text; /// Label/text paint
	rvg::Paint* bgStroke {}; /// (optional) background stroke (border)
	Vec2f padding {5.f, 5.f}; /// padding, distance from label to border
	std::array<float, 4> rounding {3.f, 3.f, 3.f, 3.f};
	const Font* font {}; /// Font to use, falls back to guis default font
};

struct ColorPickerStyle {
	rvg::Paint* marker; // marker stroking
	rvg::Paint* stroke {}; // (optional) hue + selector field stroke
	float huePadding = 10.f;
	float strokeWidth = 1.5f;
	float colorMarkerRadius = 3.f;
	float colorMarkerThickness = 1.5f;
	float hueMarkerHeight = 8.f;
	float hueMarkerThickness = 4.f;
	float hueWidth = 20.f;
};

struct ColorButtonStyle {
	BasicButtonStyle* button {};
	Vec2f padding {5.f, 5.f};
};

struct CheckboxStyle {
	rvg::Paint* bg;
	rvg::Paint* fg;
	rvg::Paint* bgStroke {};
	std::array<float, 4> bgRounding {0.f, 0.f, 0.f, 0.f};
	std::array<float, 4> fgRounding {0.f, 0.f, 0.f, 0.f};
	Vec2f padding = Vec {3.f, 3.f};
};

struct Styles {
	BasicButtonStyle basicButton {};
	LabeledButtonStyle labeledButton {};
	TextfieldStyle textfield {};
	// SliderStyle slider {};
	HintStyle hint {};
	ColorPickerStyle colorPicker {};
	ColorButtonStyle colorButton {};
	PaneStyle pane {};
	CheckboxStyle checkbox {};
};

class DefaultStyles {
public:
	DefaultStyles(rvg::Context&);

	auto& paints() const { return paints_; }
	auto& paints() { return paints_; }

	const Styles& styles() const { return styles_; }
	Styles& styles() { return styles_; }

protected:
	struct {
		rvg::Paint text;
		rvg::Paint bg;
		rvg::Paint bgAlpha;
		rvg::Paint bgHover;
		rvg::Paint bgActive;
		rvg::Paint border;
		rvg::Paint accent; // e.g. checkbox active
		rvg::Paint selection; // textfield selection
	} paints_;

	Styles styles_;
};

} // namespace vui
