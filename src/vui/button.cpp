#include <vui/button.hpp>
#include <vui/gui.hpp>
#include <vui/hint.hpp>

#include <rvg/font.hpp>
#include <dlg/dlg.hpp>
#include <nytl/rectOps.hpp>
#include <nytl/utf.hpp>

namespace vui {
namespace {

bool bgStrokeNeeded(const BasicButtonStyle& style) {
	for(auto& draw : {style.hovered, style.normal, style.pressed}) {
		if(draw.bgStroke) {
			return true;
		}
	}

	return false;
}

} // anon namespace

// Basicbutton
BasicButton::BasicButton(Gui& gui, ContainerWidget* p) : Widget(gui, p) {
	bg_ = {context(), {}, {}, {}};
	bgFill_ = {context(), {}};
	bgStroke_ = {context(), {}};
}

void BasicButton::reset(const BasicButtonStyle& style, const Rect2f& bounds,
		bool force) {
	auto sc = force || &style != &this->style(); // style change
	auto bc = !(bounds == this->bounds()); // bounds change

	if(!sc && !bc) {
		return;
	}

	// analyze
	auto pos = bounds.position;
	auto size = bounds.size;
	auto stroke = bgStrokeNeeded(style);
	size.x = (size.x == autoSize) ? 130 : size.x;
	size.y = (size.y == autoSize) ? 30 : size.y;

	// change
	auto bgc = bg_.change();
	bgc->size = size;
	bgc->position = pos;
	bgc->drawMode = {true, stroke ? 2.f : 0.f};
	bgc->rounding = style.rounding;

	if(bc) {
		Widget::bounds(bounds);
	}

	if(sc) {
		style_ = &style;
		updatePaints();
	}

	requestRedraw();
}

void BasicButton::style(const BasicButtonStyle& style, bool force) {
	reset(style, bounds(), force);
}

void BasicButton::bounds(const nytl::Rect2f& bounds) {
	reset(style(), bounds, false);
}

void BasicButton::hint(std::string_view text) {
	if(text.empty() && hint_) {
		dlg_assert(gui().destroy(*hint_));
		hint_ = {};
	} else {
		hint_ = &gui().create<DelayedHint>(position(), text);
	}
}

const ButtonDraw& BasicButton::drawStyle() const {
	return pressed_ ? style().pressed :
		hovered_ ? style().hovered : style().normal;
}

void BasicButton::updatePaints() {
	auto& draw = drawStyle();
	bgFill_.paint(draw.bg);
	bg_.disable(hidden() || !draw.bgStroke, DrawType::stroke);
	if(draw.bgStroke.has_value()) {
		bgStroke_.paint(*draw.bgStroke);
	}
	requestRedraw();
}

Widget* BasicButton::mouseButton(const MouseButtonEvent& event) {
	if(event.button != MouseButton::left) {
		return nullptr;
	}

	if(event.pressed) {
		pressed_ = true;
		updatePaints();
	} else if(pressed_) {
		pressed_ = false;
		updatePaints();
		if(hovered_) {
			clicked(event);
		}
	}

	return this;
}

void BasicButton::hide(bool hide) {
	bg_.disable(hide);
	bg_.disable(hide || !drawStyle().bgStroke.has_value(), DrawType::stroke);
	requestRedraw();
}

bool BasicButton::hidden() const {
	return bg_.disabled(DrawType::fill);
}

Widget* BasicButton::mouseMove(const MouseMoveEvent& ev) {
	if(this->contains(ev.position) && hint_) {
		hint_->hovered(true);
		hint_->position(ev.position + gui().hintOffset);
	} else if(hint_) {
		hint_->hovered(false);
	}

	return this;
}

void BasicButton::mouseOver(bool gained) {
	Widget::mouseOver(gained);
	hovered_ = gained;
	if(hint_) {
		hint_->hovered(hovered_);
	}
	updatePaints();
}

void BasicButton::draw(vk::CommandBuffer cb) const {
	Widget::bindScissor(cb);
	bgFill_.bind(cb);
	bg_.fill(cb);

	if(bgStrokeNeeded(style())) {
		bgStroke_.bind(cb);
		bg_.stroke(cb);
	}
}

Cursor BasicButton::cursor() const {
	return Cursor::hand;
}

// Button
LabeledButton::LabeledButton(Gui& gui, ContainerWidget* p, Vec2f pos,
		std::string_view text) :
	LabeledButton(gui, p, {pos, {autoSize, autoSize}}, text) {
}

LabeledButton::LabeledButton(Gui& gui, ContainerWidget* p, const Rect2f& b,
		std::string_view txt) :
	LabeledButton(gui, p, b, txt, gui.styles().labeledButton) {
}

LabeledButton::LabeledButton(Gui& gui, ContainerWidget* p,
		std::string_view label) : BasicButton(gui, p) {
	label_ = {context(), label, gui.font(), {}};
	fgPaint_ = {context(), {}};
}

LabeledButton::LabeledButton(Gui& gui, ContainerWidget* p, const Rect2f& bounds,
	std::string_view text, const LabeledButtonStyle& xstyle) :
		LabeledButton(gui, p, "") {
	reset(xstyle, bounds, false, text);
}

void LabeledButton::reset(const LabeledButtonStyle& style,
		const Rect2f& bounds, bool force, std::optional<std::string_view> ostr) {
	auto sc = force || &style != &this->style(); // style change
	auto bc = !(bounds == this->bounds()); // bounds change

	if(!sc && !bc && !ostr) {
		return;
	}

	// analyze
	auto pos = bounds.position;
	auto size = bounds.size;
	auto str = ostr ? nytl::toUtf32(*ostr) : label_.utf32();
	auto& font = style.font ? *style.font : gui().font();
	auto textSize = nytl::Vec2f {font.width(str), font.height()};
	auto textPos = style.padding; // local

	if(size.x != autoSize) {
		textPos.x = (size.x - textSize.x) / 2;
	} else {
		size.x = textSize.x + 2 * style.padding.x;
	}

	if(size.y != autoSize) {
		textPos.y = (size.y - textSize.y) / 2;
	} else {
		size.y = textSize.y + 2 * style.padding.y;
	}

	// change
	auto tc = label_.change();
	tc->position = pos + textPos;
	tc->font = &font;
	tc->utf32 = str;

	// propagate
	style_ = &style;
	auto& s = style.basic ? *style.basic : gui().styles().basicButton;
	BasicButton::reset(s, {pos, size}, force);
	requestRedraw();
}

void LabeledButton::clicked(const MouseButtonEvent&) {
	if(onClick) {
		onClick(*this);
	}
}

void LabeledButton::style(const LabeledButtonStyle& style, bool force) {
	reset(style, bounds(), force);
}

void LabeledButton::bounds(const Rect2f& bounds) {
	reset(style(), bounds, false);
}

void LabeledButton::hide(bool hide) {
	BasicButton::hide(hide);
	label_.disable(hide);
}

void LabeledButton::draw(vk::CommandBuffer cb) const {
	BasicButton::draw(cb);
	fgPaint_.bind(cb);
	label_.draw(cb);
}

void LabeledButton::updatePaints() {
	BasicButton::updatePaints();
	auto& draw = drawStyle();
	dlg_assert(draw.fg);
	fgPaint_.paint(*draw.fg);
}

void LabeledButton::label(std::string_view label, bool resize) {
	auto b = resize ? bounds() : Rect2f {position(), {autoSize, autoSize}};
	reset(style(), b, false, label);
}

} // namespace vui
