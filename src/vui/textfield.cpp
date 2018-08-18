#include <vui/textfield.hpp>
#include <vui/gui.hpp>

#include <rvg/font.hpp>
#include <nytl/utf.hpp>
#include <nytl/rectOps.hpp>
#include <dlg/dlg.hpp>

// TODO:
// - double click selects all
// - ctrl a for selecting/unselecting everything
// - copy/paste integration
// - allow to extend selections with shift?

namespace vui {

Textfield::Textfield(Gui& gui, Vec2f pos, std::string_view start) :
	Textfield(gui, {pos, {autoSize, autoSize}}, start) {
}

Textfield::Textfield(Gui& gui, const Rect2f& bounds, std::string_view start) :
	Textfield(gui, bounds, start, gui.styles().textfield) {
}

Textfield::Textfield(Gui& gui) : Widget(gui) {
	bg_ = {context()};
	selection_.bg = {context(), {}, {}, {true, 0.f}};
	selection_.bg.disable(true);

	text_ = {context(), U"", gui.font(), {}};
	selection_.text = {context(), U"", gui.font(), {}};
	selection_.text.disable(true);

	cursor_ = {context(), {}, {}, {true, 0.f}};
	cursor_.disable(true);

	bgPaint_ = {context(), {}};
	fgPaint_ = {context(), {}};
	bgStroke_ = {context(), {}};
}

Textfield::Textfield(Gui& gui, const Rect2f& bounds, std::string_view start,
		const TextfieldStyle& style) : Textfield(gui) {
	reset(style, bounds, false, start);
}

void Textfield::reset(const TextfieldStyle& style, const Rect2f& bounds,
		bool force, std::optional<std::string_view> ostring) {
	auto sc = force || &style != &this->style();
	auto bc = !(bounds == this->bounds());

	if(!bc && !sc) {
		return;
	}

	// analyze
	auto pos = bounds.position;
	auto size = bounds.size;
	const auto& string = ostring ? nytl::toUtf32(*ostring) : text_.utf32();
	auto& font = style.font ? *style.font : gui().font();
	auto textSize = nytl::Vec2f {font.width(string), font.height()};
	auto textPos = pos + style.padding;

	if(size.x != autoSize) {
		size.x = font.width(U".:This is the default textfield length:.");
	}

	if(size.y != autoSize) {
		textPos.y = pos.y + (size.y - textSize.y) / 2;
	} else {
		size.y = textSize.y + 2 * style.padding.y;
	}

	bool stroke = false;
	for(auto& draw : {style.hovered, style.normal, style.focused}) {
		if(draw.bgStroke) {
			stroke = true;
			break;
		}
	}

	// change
	auto bgc = bg_.change();
	bgc->position = pos;
	bgc->drawMode.stroke = stroke ? 2.f : 0.f;
	bgc->size = size;
	bgc->position = pos;
	bgc->rounding = style.rounding;

	auto tc = text_.change();
	tc->position = textPos;
	tc->utf32 = string;

	updateSelectionDraw();
	updateCursorPosition();

	// propagate
	if(bc) {
		Widget::relayout(bounds);
	}

	if(sc) {
		dlg_assert(style.selectedText || style.selected);
		dlg_assert(style.cursor);
		style_ = &style;
		updatePaints();
		gui().rerecord();
	}
}

void Textfield::relayout(const Rect2f& bounds) {
	reset(style(), bounds);
}

void Textfield::style(const TextfieldStyle& style, bool force) {
	reset(style, bounds(), force);
}

void Textfield::utf8(std::string_view str) {
	utf32(nytl::toUtf32(str));
}

void Textfield::utf32(std::u32string_view str) {
	endSelection();
	cursorPos_ = 0;
	text_.change()->utf32 = str;
	updateCursorPosition();
}

void Textfield::hide(bool hide) {
	// Textfield has many elements to hide/show and so this method
	// is easy to get wrong. Should work in all possible states
	bg_.disable(hide);
	bg_.disable(drawStyle().bgStroke.has_value(), DrawType::stroke);
	text_.disable(hide);

	if(hide) {
		cursor_.disable(true);
	} // otherwise on focus it will show itself again due to blinking

	if(hide) {
		selection_.bg.disable(true);
		selection_.text.disable(true);
	} else if(selection_.count) { // only unhide selection stuff if active
		selection_.bg.disable(false);
		selection_.text.disable(false);
	}
}

bool Textfield::hidden() const {
	return bg_.disabled(DrawType::fill);
}

Widget* Textfield::mouseButton(const MouseButtonEvent& ev) {
	if(ev.button != MouseButton::left) {
		return nullptr;
	}

	// we have to call this again since we might still have focus from
	// gui pov (so we it doesn't get called) but lost it internally, e.g.
	// due to a submit/cancel
	focus(true);

	// remember that the selecting mouse button is pressed
	// used in mouseMove
	selecting_ = ev.pressed;
	if(ev.pressed) {
		endSelection();
		auto ex = ev.position.x - (position().x + text_.position().x);
		cursorPos_ = boundaryAt(ex);

		// clicking somewhere immediately shows the cursor there
		showCursor(true);
		resetBlinkTime();
		updateCursorPosition();
	}

	return this;
}

void Textfield::mouseOver(bool gained) {
	Widget::mouseOver(gained);
	mouseOver_ = gained;
	updatePaints();
}

Widget* Textfield::mouseMove(const MouseMoveEvent& ev) {
	if(selecting_) {
		auto ps = selection_.start;
		auto pc = selection_.count;

		auto textx = position().x + text_.position().x;
		auto c1 = cursorPos_;
		auto c2 = boundaryAt(ev.position.x - textx);
		selection_.start = std::min(c1, c2);
		selection_.count = std::abs(int(c1) - int(c2));

		// if selection has changed:
		if(ps != selection_.start || pc != selection_.count) {
			gui().listener().selection(utf8Selected());
			updateSelectionDraw();
		}
	}

	return this;
}

void Textfield::updateSelectionDraw() {
	// When there is no selection, we have nothing to do
	if(!selection_.count) {
		return;
	}

	auto end = selection_.start + selection_.count - 1;

	auto b1 = text_.ithBounds(selection_.start);
	auto b2 = text_.ithBounds(end);

	b1.position += text_.position();
	b2.position += text_.position();

	auto sc = selection_.bg.change();
	sc->position.x = b1.position.x;
	sc->position.y = text_.position().y - 1.f;
	sc->size.x = b2.position.x + b2.size.x - b1.position.x;
	sc->size.y = text_.font()->height() + 2.f;
	selection_.bg.disable(false);

	auto tc = selection_.text.change();
	tc->position.x = b1.position.x;
	tc->position.y = text_.position().y;
	tc->utf32 = {
		text_.utf32().begin() + selection_.start,
		text_.utf32().begin() + selection_.start + selection_.count};
	selection_.text.disable(false);
}

void Textfield::focus(bool gained) {
	if(gained == focus_) { // see mouseButton(ev)
		return;
	}

	if(!gained) {
		cursorPos_ = 0;
		endSelection();
		updateCursorPosition();
	}

	focus_ = gained;
	showCursor(focus_);
	blinkCursor(focus_);
	resetBlinkTime();
	updatePaints();
}

Widget* Textfield::textInput(const TextInputEvent& ev) {
	// When e.g. enter/escape was pressed we still receive events
	// (since we still have focus from the guis perspective)
	// although we don't want them anymore. So we ignore them.
	if(!focus_) {
		return nullptr;
	}

	auto utf32 = toUtf32(ev.utf8);

	{
		auto tc = text_.change();
		tc->utf32.insert(cursorPos_, utf32);
		dlg_assert(cursorPos_ <= tc->utf32.length());
	}

	endSelection();
	showCursor(true);
	resetBlinkTime();

	cursorPos_ += utf32.length();
	updateCursorPosition();
	if(onChange) {
		onChange(*this);
	}

	return this;
}

Widget* Textfield::key(const KeyEvent& ev) {
	// When e.g. enter/escape was pressed we still receive events
	// (since we still have focus from the guis perspective)
	// although we don't want them anymore.
	if(!focus_ || !ev.pressed) {
		return nullptr;
	}

	bool changed = false;
	bool updateCursor = false;
	if(ev.key == Key::backspace && cursorPos_ > 0) {
		auto tc = text_.change();
		changed = true;
		if(selection_.count) {
			tc->utf32.erase(selection_.start, selection_.count);
			cursorPos_ = selection_.start;
			endSelection();
		} else {
			cursorPos_ -= 1;
			tc->utf32.erase(cursorPos_, 1);
		}
		updateCursor = true;
	} else if(ev.key == Key::left) {
		if(selection_.count) {
			cursorPos_ = selection_.start;
			endSelection();
		} else if(cursorPos_ > 0) {
			cursorPos_ -= 1;
		}

		updateCursor = true;
		showCursor(true);
		resetBlinkTime();
	} else if(ev.key == Key::right) {
		if(selection_.count) {
			cursorPos_ = selection_.start + selection_.count;
			endSelection();
			updateCursor = true;
		} else if(cursorPos_ < text_.utf32().length()) {
			cursorPos_ += 1;
			updateCursor = true;
			showCursor(true);
			resetBlinkTime();
		}
	} else if(ev.key == Key::del) {
		auto tc = text_.change();
		if(selection_.count) {
			tc->utf32.erase(selection_.start, selection_.count);
			cursorPos_ = selection_.start;
			updateCursor = true;
			endSelection();
			changed = true;
		} else if(cursorPos_ < text_.utf32().length()) {
			tc->utf32.erase(cursorPos_, 1);
			changed = true;
		}
	} else if(ev.key == Key::escape) {
		focus(false);
		if(onCancel) {
			onCancel(*this);
		}
	} else if(ev.key == Key::enter) {
		focus(false);
		if(onSubmit) {
			onSubmit(*this);
		}
	}

	if(updateCursor) {
		updateCursorPosition();
	}

	if(changed && onChange) {
		onChange(*this);
	}

	dlg_assert(cursorPos_ <= text_.utf32().length());
	return this;
}

void Textfield::draw(vk::CommandBuffer cb) const {
	Widget::bindScissor(cb);

	bgPaint_.bind(cb);
	bg_.fill(cb);
	bgStroke_.bind(cb);
	bg_.stroke(cb);

	if(style().selected) {
		style().selected->bind(cb);
		selection_.bg.fill(cb);
	}

	fgPaint_.bind(cb);
	text_.draw(cb);

	if(style().selectedText) {
		style().selectedText->bind(cb);
		selection_.text.draw(cb);
	}

	dlg_assert(style().cursor);
	style().cursor->bind(cb);
	cursor_.fill(cb);
}

void Textfield::update(double delta) {
	if(!focus_ || !blink_) {
		return;
	}

	blinkAccum_ = blinkAccum_ + delta;

	// when the textfield is hidden we can't just show the cursor
	if(!hidden() && blinkAccum_ > Gui::blinkTime) {
		int quo;
		blinkAccum_ = std::remquo(blinkAccum_, Gui::blinkTime, &quo);
		if(quo % 2) { // only uneven amount of toggles results in real toggle
			cursor_.disable(!cursor_.disabled());
		}
	}

	registerUpdate();
}

void Textfield::updateCursorPosition() {
	dlg_assert(cursorPos_ <= text_.utf32().length());
	auto x = text_.position().x;
	if(cursorPos_ > 0) {
		auto b = text_.ithBounds(cursorPos_ - 1);
		x += b.position.x + b.size.x;
	}

	// scrolling
	auto xbeg = style().padding.x;
	auto xend = size().x - style().padding.x;

	// the cursor position clamped into visible range
	auto clamped = std::clamp(x, xbeg, xend);

	// if the original cursor position is out of visible range we
	// have to scroll the text at least so far to get it into range
	if(clamped != x) {
		auto tc = text_.change();
		tc->position.x += clamped - x;
		x += clamped - x;
	}

	auto cc = cursor_.change();
	cc->size.x = style().cursorWidth;
	cc->size.y = text_.font()->height();
	cc->position.x = x;
	cc->position.y = text_.position().y;
	cc->drawMode = {true, 0.f};

	// Since we have changed the texts position we must refresh the
	// bounds of the selection as well
	updateSelectionDraw();
}

void Textfield::showCursor(bool s) {
	cursor_.disable(!s);
}

void Textfield::blinkCursor(bool b) {
	blink_ = b;
	if(b) {
		registerUpdate();
	}
}

void Textfield::resetBlinkTime() {
	blinkAccum_ = 0.f;
}

unsigned Textfield::boundaryAt(float x) {
	auto ca = text_.charAt(x);
	if(ca < text_.utf32().length()) {
		auto bounds = text_.ithBounds(ca);
		auto into = (x - bounds.position.x) / bounds.size.x;
		if(into >= 0.5f) {
			++ca;
		}
	}

	return ca;
}

std::u32string_view Textfield::utf32() const {
	return text_.utf32();
}

std::string Textfield::utf8() const {
	return text_.utf8();
}

std::u32string_view Textfield::utf32Selected() const {
	return {text_.utf32().data() + selection_.start, selection_.count};
}

std::string Textfield::utf8Selected() const {
	return toUtf8(utf32Selected());
}

void Textfield::endSelection() {
	if(!selection_.count) {
		return;
	}

	selection_.count = selection_.start = {};
	selection_.text.disable(true);
	selection_.bg.disable(true);

	if(focus_) {
		showCursor(true);
		blinkCursor(true);
		resetBlinkTime();
	}
}

Cursor Textfield::cursor() const {
	return Cursor::beam;
}

const TextfieldDraw& Textfield::drawStyle() const {
	return focus_ ? style().focused :
		mouseOver_ ? style().hovered : style().normal;
}

void Textfield::updatePaints() {
	auto& draw = drawStyle();
	bgPaint_.paint(draw.bg);
	fgPaint_.paint(draw.text);

	bg_.disable(!draw.bgStroke, DrawType::stroke);
	if(draw.bgStroke) {
		dlg_assert(bgStroke_.valid());
		bgStroke_.paint(*draw.bgStroke);
	}
}

} // namespace vui
