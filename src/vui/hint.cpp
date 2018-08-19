#include <vui/hint.hpp>
#include <vui/gui.hpp>
#include <rvg/font.hpp>
#include <nytl/rectOps.hpp>
#include <nytl/utf.hpp>
#include <dlg/dlg.hpp>

namespace vui {

Hint::Hint(Gui& gui, ContainerWidget* p, Vec2f pos, std::string_view text) :
	Hint(gui, p, {pos, {autoSize, autoSize}}, text) {
}

Hint::Hint(Gui& gui, ContainerWidget* p, const Rect2f& bounds,
		std::string_view text) : Hint(gui, p, bounds, text, gui.styles().hint) {
}

Hint::Hint(Gui& gui, ContainerWidget* p, const Rect2f& bounds,
		std::string_view text, const HintStyle& xstyle) : Widget(gui, p) {

	bg_ = {context()};
	bg_.disable(true);
	text_ = {context(), U"", gui.font(), {}};
	text_.disable(true);

	reset(xstyle, bounds, false, text);
}

void Hint::reset(const HintStyle& style, const Rect2f& bounds, bool force,
		std::optional<std::string_view> ostr) {
	auto sc = force || &style != &this->style(); // style change
	auto bc = !(bounds == this->bounds()); // bounds change

	if(!sc && !bc && !ostr) {
		return;
	}

	// analyze
	auto str = ostr ? nytl::toUtf32(*ostr) : text_.utf32();
	auto pos = bounds.position;
	auto size = bounds.size;
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
	auto tc = text_.change();
	tc->position = pos + textPos;
	tc->font = &font;
	tc->utf32 = str;

	auto bgc = bg_.change();
	bgc->drawMode = {true, style.bgStroke ? 2.f : 0.f};
	bgc->size = size;
	bgc->rounding = style.rounding;
	bgc->position = pos;

	if(bc) {
		Widget::bounds({pos, size});
	}

	if(sc) {
		style_ = &style;
		gui().rerecord();
	}
}

void Hint::style(const HintStyle& style, bool force) {
	reset(style, {{}, size()}, force);
}

void Hint::bounds(const Rect2f& bounds) {
	reset(style(), bounds, false);
}

void Hint::draw(vk::CommandBuffer cb) const {
	bindScissor(cb);

	if(style().bg) {
		style().bg->bind(cb);
		bg_.fill(cb);
	}

	if(style().bgStroke) {
		style().bgStroke->bind(cb);
		bg_.stroke(cb);
	}

	dlg_assert(style().text->valid());
	style().text->bind(cb);
	text_.draw(cb);
}

void Hint::hide(bool hide) {
	bg_.disable(hide);
	text_.disable(hide);
}

bool Hint::hidden() const {
	return bg_.disabled(DrawType::fill);
}

void Hint::label(std::string_view label, bool resize) {
	auto b = resize ? bounds() : Rect2f {position(), {autoSize, autoSize}};
	reset(style(), b, false, label);
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
