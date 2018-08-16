#include <vui/hint.hpp>
#include <vui/gui.hpp>
#include <rvg/font.hpp>
#include <nytl/rectOps.hpp>

namespace vui {

Hint::Hint(Gui& gui, Vec2f pos, std::string_view text) :
	Hint(gui, {pos, {autoSize, autoSize}}, text) {
}

Hint::Hint(Gui& gui, const Rect2f& bounds, std::string_view text) :
	Hint(gui, bounds, text, gui.styles().hint) {
}

Hint::Hint(Gui& gui, const Rect2f& bounds, std::string_view text,
		const HintStyle& xstyle) : MovableWidget(gui) {

	bg_ = {context()};
	bg_.disable(true);
	text_ = {context(), text, gui.font(), {}};
	text_.disable(true);

	style(xstyle);
	relayout(bounds);
}

void Hint::draw(vk::CommandBuffer cb) const {
	bindScissor(cb);
	bindTransform(cb);

	if(style().bg) {
		style().bg->bind(cb);
		bg_.fill(cb);
	}

	if(style().bgStroke) {
		style().bgStroke->bind(cb);
		bg_.stroke(cb);
	}

	if(style().text) {
		style().text->bind(cb);
		text_.draw(cb);
	}
}

void Hint::hide(bool hide) {
	bg_.disable(hide);
	text_.disable(hide);
}

bool Hint::hidden() const {
	return bg_.disabled();
}

void Hint::reset(const HintStyle& style, const Rect2f& bounds, bool force) {
	auto sc = force || &style != &this->style(); // style change
	auto bc = !(bounds == this->bounds()); // bounds change

	if(!sc && !bc) {
		return;
	}

	// analyze
	auto pos = bounds.position;
	auto size = bounds.size;
	auto& font = style.font ? *style.font : gui().font();
	auto textSize = nytl::Vec2f {font.width(text_.utf8()), font.height()};
	auto textPos = pos + style.padding;

	if(size.x != autoSize) {
		textPos.x = pos.x + (size.x - textSize.x) / 2;
	} else {
		size.x = textSize.x + 2 * style.padding.x;
	}

	if(size.y != autoSize) {
		textPos.y = pos.y + (size.y - textSize.y) / 2;
	} else {
		size.y = textSize.y + 2 * style.padding.y;
	}

	// change
	auto tc = text_.change();
	tc->font = &font;
	tc->position = textPos;

	auto bgc = bg_.change();
	bgc->drawMode = {true, style.bgStroke ? 2.f : 0.f};
	bgc->size = size;
	bgc->rounding = style.rounding;
	bgc->position = pos;

	style_ = &style;
	if(bc) {
		Widget::relayout({pos, size});
	}
}

void Hint::style(const HintStyle& style, bool force) {
	reset(style, {{}, size()}, force);
}

void Hint::size(Vec2f size) {
	reset(style(), {{}, size}, false);
}

void Hint::label(std::string_view label) {
	text_.change()->utf8(label);
}

// DelayedHint
void DelayedHint::hovered(bool hovered) {
	if(hovered && !hovered_) {
		accum_ = 0;
		hovered_ = true;
		registerUpdate();
	} else if(!hovered) {
		accum_ = 0;
		hide(true);
	}

	hovered_ = hovered;
}

void DelayedHint::update(double delta) {
	if(!hovered_) {
		return;
	}

	accum_ += delta;
	if(accum_ >= gui().hintDelay) {
		hide(false);
	} else {
		registerUpdate();
	}
}

} // namespace vui
