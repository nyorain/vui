#include <vui/checkbox.hpp>
#include <vui/gui.hpp>
#include <nytl/rectOps.hpp>
#include <dlg/dlg.hpp>

namespace vui {

Checkbox::Checkbox(Gui& gui, ContainerWidget* p, const Rect2f& bounds) :
	Checkbox(gui, p, bounds, gui.styles().checkbox) {
}

Checkbox::Checkbox(Gui& gui, ContainerWidget* p, const Rect2f& bounds,
		const CheckboxStyle& style) : Checkbox(gui, p) {
	reset(style, bounds);
}

Checkbox::Checkbox(Gui& gui, ContainerWidget* p) : Widget(gui, p) {
	bg_ = {context(), {}, {}, {}};
	fg_ = {context(), {}, {}, {true, 0.f}};
	fg_.disable(true); // we always start unchcked
}

void Checkbox::reset(const CheckboxStyle& style, const Rect2f& bounds,
		bool force) {
	auto bc = !(bounds == this->bounds());
	auto sc = force || &style != &this->style();
	if(!bc && !sc) {
		return;
	}

	auto pos = bounds.position;
	auto size = bounds.size;
	if(size.x == autoSize && size.y == autoSize) {
		size = {15.f, 15.f};
	} else if(size.x == autoSize) {
		size.x = size.y;
	} else if(size.y == autoSize) {
		size.y = size.x;
	}

	// change
	using namespace nytl::vec::cw;
	auto bgc = bg_.change();
	bgc->size = size;
	bgc->position = pos;
	bgc->rounding = style.bgRounding;
	bgc->drawMode = {true, style.bgStroke ? 2.f : 0.f};

	auto fgc = fg_.change();
	fgc->position = pos + style.padding;
	fgc->size = max(size - 2 * style.padding, nytl::Vec {0.f, 0.f});
	fgc->rounding = style.fgRounding;

	if(bc) {
		Widget::bounds({pos, size});
	}

	if(sc) {
		dlg_assert(style.bg && style.fg);
		requestRerecord(); // NOTE: could be optimized, not always needed
		style_ = &style;
	}

	requestRedraw();
}

void Checkbox::set(bool checked) {
	if(checked == checked_) {
		return;
	}

	checked_ = checked;
	fg_.disable(!checked);
	requestRedraw();
}

void Checkbox::bounds(const Rect2f& b) {
	reset(style(), b, false);
}

void Checkbox::style(const CheckboxStyle& style, bool force) {
	reset(style, bounds(), force);
}

void Checkbox::hide(bool hide) {
	bg_.disable(hide);
	if(hide) {
		fg_.disable(true);
	} else if(checked_) {
		fg_.disable(false);
	}
	requestRedraw();
}

bool Checkbox::hidden() const {
	return bg_.disabled();
}

Widget* Checkbox::mouseButton(const MouseButtonEvent& ev) {
	if(ev.button == MouseButton::left && ev.pressed) {
		toggle();
		if(onToggle) {
			onToggle(*this);
		}
	}

	return this;
}

void Checkbox::draw(vk::CommandBuffer cb) const {
	Widget::bindScissor(cb);

	style().bg->bind(cb);
	bg_.fill(cb);

	if(style().bgStroke) {
		style().bgStroke->bind(cb);
		bg_.stroke(cb);
	}

	style().fg->bind(cb);
	fg_.fill(cb);
}

Cursor Checkbox::cursor() const {
	return Cursor::hand;
}

} // namespace vui
