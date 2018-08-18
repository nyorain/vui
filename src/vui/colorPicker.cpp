#include <vui/colorPicker.hpp>
#include <vui/gui.hpp>

#include <rvg/context.hpp>
#include <dlg/dlg.hpp>
#include <nytl/rectOps.hpp>
#include <nytl/vecOps.hpp>

namespace vui {
namespace {

template<size_t D, typename T>
Vec<D, T> clamp(const Vec<D, T>& v, Rect<D, T> b) {
	return nytl::vec::cw::clamp(v, b.position, b.position + b.size);
}

} // anon namespace

ColorPicker::ColorPicker(Gui& gui, ContainerWidget* p, const Rect2f& bounds,
	const Color& start) : ColorPicker(gui, p, bounds, start,
		gui.styles().colorPicker) {
}

ColorPicker::ColorPicker(Gui& gui, ContainerWidget* p) : Widget(gui, p) {
	selector_ = {context()};
	hueMarker_ = {context()};
	colorMarker_ = {context()};
	basePaint_ = {context(), {}};

	hue_ = {context(), {}, {}};
	sGrad_ = {context(), {}};
	vGrad_ = {context(), {}};
}

ColorPicker::ColorPicker(Gui& gui, ContainerWidget* p, const Rect2f& bounds,
	const Color& start, const ColorPickerStyle& style)
		: ColorPicker(gui, p) {

	auto hsv = hsvNorm(start);
	reset(style, bounds, false, hsv);
}

void ColorPicker::reset(const ColorPickerStyle& style, const Rect2f& bounds,
		bool force, std::optional<nytl::Vec3f> ohsv) {

	auto bc = !(bounds == this->bounds());
	auto sc = force || &style != &this->style();

	if(!bc && !sc && !ohsv) {
		return;
	}

	// == analyze ==
	auto size = bounds.size;
	auto pos = bounds.position;
	auto hsv = ohsv ? *ohsv : currentHsv();

	if(size == Vec {autoSize, autoSize}) {
		size = {230, 200};
	} else if(size.x == autoSize) {
		size.x = std::min(0.87f * size.y, 150.f);
	} else if(size.y == autoSize) {
		size.y = std::min(1.15f * size.x, 120.f);
	}

	// == change ==
	if(ohsv) {
		basePaint_.paint(colorPaint(hsvNorm(hsv[0], 1.f, 1.f)));
	}

	auto slc = selector_.change();
	slc->position = pos;
	slc->size = size;
	slc->size.x -= style.hueWidth + style.huePadding;
	slc->drawMode = {true, style.strokeWidth};

	// selector gradients
	sGrad_.paint(linearGradient(
		selector_.position(),
		selector_.position() + Vec {selector_.size().x, 0.f},
		::rvg::hsv(0, 0, 255), ::rvg::hsv(0, 0, 255, 0)));
	vGrad_.paint(linearGradient(
		selector_.position(),
		selector_.position() + Vec {0.f, selector_.size().y},
		::rvg::hsv(0, 255, 0, 0), ::rvg::hsv(0, 255, 0)));

	// selector marker
	using namespace nytl::vec::cw::operators;
	auto cmc = colorMarker_.change();
	cmc->center = pos + Vec {hsv[1], 1.f - hsv[2]} * selector_.size();
	cmc->radius = {style.colorMarkerRadius, style.colorMarkerRadius};
	cmc->drawMode = {false, style.colorMarkerThickness};
	cmc->pointCount = 6u;

	// hue
	auto hc = hue_.change();
	hc->points.clear();
	hc->drawMode.stroke = style.hueWidth;
	hc->drawMode.color.stroke = true;

	auto ystep = size.y / 6.f;
	auto x = pos.x + size.x - style.hueWidth / 2;
	for(auto i = 0u; i < 7; ++i) {
		auto col = hsvNorm(i / 6.f, 1.f, 1.f);
		hc->drawMode.color.points.push_back(col.rgba());
		hc->points.push_back({x, pos.y + ystep * i});
	}

	// hue marker
	auto hmc = hueMarker_.change();
	hmc->position.x = pos.x + size.x - style.hueWidth;
	hmc->position.y = pos.y + hsv[0] * size.y - style.hueMarkerHeight / 2.f;
	hmc->size = {style.hueWidth, style.hueMarkerHeight};
	hmc->drawMode = {false, style.hueMarkerThickness};

	// == propagate & delegate ==
	if(bc) {
		Widget::bounds({pos, size});
	}

	if(sc) {
		dlg_assert(style.marker);
		style_ = &style;
		gui().rerecord();
	}
}

void ColorPicker::bounds(const Rect2f& bounds) {
	reset(style(), bounds);
}

void ColorPicker::style(const ColorPickerStyle& style, bool force) {
	reset(style, bounds(), force);
}

void ColorPicker::hide(bool hide) {
	hue_.disable(hide);
	hueMarker_.disable(hide);
	selector_.disable(hide);
	colorMarker_.disable(hide);
}

bool ColorPicker::hidden() const {
	return hue_.disabled(DrawType::fill);
}

Widget* ColorPicker::mouseButton(const MouseButtonEvent& ev) {
	if(ev.button != MouseButton::left) {
		return nullptr;
	}

	if(!ev.pressed) {
		slidingSV_ = slidingHue_ = false;
		return this;
	}

	click(ev.position, true);
	return this;
}

Widget* ColorPicker::mouseMove(const MouseMoveEvent& ev) {
	dlg_info("move: {}, pos: {}", ev.position, position());
	click(ev.position, false);
	return this;
}

void ColorPicker::pick(const Color& color) {
	auto hsv = hsvNorm(color);

	auto hue = hsv[0];
	auto sv = Vec {hsv[1], hsv[2]};

	// update paints/shapes
	basePaint_.paint(colorPaint(hsvNorm(hue, 1.f, 1.f)));

	auto hmc = hueMarker_.change();
	hmc->position.y = position().y + hue * selector_.size().y -
		style().hueMarkerHeight / 2.f;

	using namespace nytl::vec::cw::operators;
	auto cmc = colorMarker_.change();
	cmc->center = sv * selector_.size();
	cmc->radius = {style().colorMarkerRadius, style().colorMarkerRadius};
}

void ColorPicker::draw(vk::CommandBuffer cb) const {
	Widget::bindScissor(cb);

	for(auto* p : {&basePaint_, &sGrad_, &vGrad_}) {
		p->bind(cb);
		selector_.fill(cb);
	}

	context().pointColorPaint().bind(cb);
	hue_.stroke(cb);

	if(style().stroke) {
		style().stroke->bind(cb);
		selector_.stroke(cb);
	}

	dlg_assert(style().marker);
	style().marker->bind(cb);
	hueMarker_.stroke(cb);
	colorMarker_.stroke(cb);
}

void ColorPicker::click(Vec2f pos, bool real) {
	pos = clamp(pos, bounds());
	auto hue = Rect2f {
		position() + Vec {selector_.size().x + style().huePadding, 0.f},
		{style().hueWidth, selector_.size().y}
	};

	if(slidingSV_ || (real && nytl::contains(selector_.bounds(), pos))) {
		slidingSV_ = true;
		pos = clamp(pos, selector_.bounds());
		colorMarker_.change()->center = pos;
		if(onChange) {
			onChange(*this);
		}
	} else if(slidingHue_ || (real && nytl::contains(hue, pos))) {
		slidingHue_ = true;
		pos = clamp(pos, hue);
		auto norm = (pos.y - position().y) / selector_.size().y;
		hueMarker_.change()->position.y = pos.y;
		hueMarker_.change()->position.y -= style().hueMarkerHeight / 2.f;
		basePaint_.paint(colorPaint(hsvNorm(norm, 1.f, 1.f)));
		if(onChange) {
			onChange(*this);
		}
	}
}

float ColorPicker::currentHue() const {
	auto pos = hueMarker_.position().y + style().hueMarkerHeight / 2.f;
	return (pos - position().y) / selector_.size().y;
}

Vec2f ColorPicker::currentSV() const {
	using namespace nytl::vec::cw::operators;
	auto sv = (colorMarker_.center() - position()) / selector_.size();
	sv.y = 1 - sv.y;
	return sv;
}

Vec3f ColorPicker::currentHsv() const {
	auto sv = currentSV();
	return {currentHue(), sv.x, sv.y};
}

Color ColorPicker::picked() const {
	auto sv = currentSV();
	return hsvNorm(currentHue(), sv.x, sv.y);
}

Rect2f ColorPicker::ownScissor() const {
	// Except with the marker (which sort of lays on the widget) we
	// never draw outside of our bounds so this should not be a problem.
	auto r = Widget::ownScissor();
	auto y = std::max(style().colorMarkerRadius,
		style().hueMarkerHeight / 2.f + style().hueMarkerThickness);

	r.position.x -= style().colorMarkerRadius;
	r.position.y -= y;
	r.size.x += style().colorMarkerRadius + 1.f;
	r.size.y += 2 * y;

	return r;
}

/*
// ColorButton
ColorButton::ColorButton(Gui& gui, const Rect2f& bounds,
		const Vec2f& pickerSize, const Color& start) :
	ColorButton(gui, bounds, pickerSize, start, gui.styles().colorButton) {
}

ColorButton::ColorButton(Gui& gui, const Rect2f& bounds, const Vec2f& pickerSize,
	const Color& start, const ColorButtonStyle& style) :
		BasicButton(gui, bounds, style.button ? *style.button :
			gui.styles().basicButton), style_(style) {

	// TODO: currently hiding & unhiding on every focus
	// change (triggering an updateDevice)
	class PopupPane : public Pane {
	public:
		using Pane::Pane;
		void focus(bool gained) override {
			hide(!gained);
		}
	};

	// NOTE: makes this non movable!
	auto updateColorPaint = [this](const auto& cp){
		this->colorPaint_.paint(rvg::colorPaint(cp.picked()));
		if(onChange) {
			onChange(*this);
		}
	};

	color_ = {context()};

	auto cpBounds = Rect2f{{}, pickerSize};
	auto cp = std::make_unique<ColorPicker>(gui, cpBounds, start);
	cp_ = cp.get();
	cp_->onChange = updateColorPaint;
	colorPaint_ = {context(), rvg::colorPaint(cp_->picked())};

	pane_ = &gui.create<PopupPane>(Rect2f{{}, {autoSize, autoSize}},
		std::move(cp));
	pane_->hide(true);

	this->size(bounds.size);
}

void ColorButton::size(Vec2f size) {
	if(size.x == autoSize && size.y == autoSize) {
		size = {100.f, 25.f};
		size += 2 * style().padding;
	} else if(size.x == autoSize) {
		size.x = size.y * 4;
	} else if(size.y == autoSize) {
		size.y = size.x / 4;
	}

	auto cc = color_.change();
	cc->position = style().padding;
	cc->size = size - 2 * style().padding;
	cc->drawMode.fill = true;

	pane_->position(position() + Vec2f{0.f, size.y});
	BasicButton::size(size);
}

void ColorButton::position(Vec2f pos) {
	Widget::position(pos);
	pane_->position(position() + Vec2f{0.f, size().y});
}

void ColorButton::hide(bool hide) {
	if(hide) {
		pane_->hide(hide);
	}

	color_.disable(hide);
	BasicButton::hide(hide);
}

void ColorButton::clicked(const MouseButtonEvent&) {
	pane_->hide(false);
}

void ColorButton::focus(bool gained) {
	BasicButton::focus(gained);
	if(!gained) {
		pane_->hide(true);
	}
}

void ColorButton::draw(vk::CommandBuffer cb) const {
	BasicButton::draw(cb);
	colorPaint_.bind(cb);
	color_.fill(cb);
}
*/

} // namespace vui
