#include <vui/pane.hpp>
#include <vui/gui.hpp>
#include <rvg/context.hpp>
#include <dlg/dlg.hpp>
#include <nytl/rectOps.hpp>

namespace vui {

Pane::Pane(Gui& gui, ContainerWidget* p, const Rect2f& bounds,
	std::unique_ptr<Widget> child) :
		Pane(gui, p, bounds, gui.styles().pane, std::move(child)) {
}

Pane::Pane(Gui& gui, ContainerWidget* p, const Rect2f& bounds,
	const PaneStyle& style, std::unique_ptr<Widget> child) :
		Pane(gui, p) {
	reset(style, bounds, false, std::move(child));
}

Pane::Pane(Gui& gui, ContainerWidget* p) : ContainerWidget(gui, p) {
	bg_ = {context(), {}, {}, {}};
}


// TODO: introduce something like vui::autoPosition?
// to allow the pane to set it's position just relative to the
// widgets position?
void Pane::reset(const PaneStyle& style, const Rect2f& bounds,
		bool force, std::optional<std::unique_ptr<Widget>> owidget) {
	auto sc = force || &style != &this->style();
	auto bc = !(bounds == this->bounds());

	if(!sc && !bc && !owidget) {
		return;
	}

	// analyze
	auto pos = bounds.position;
	auto size = bounds.size;
	auto* nextWidget = owidget ? owidget->get() : this->widget();

	if(size.x == autoSize) {
		size.x = nextWidget ?
			nextWidget->size().x + 2 * style.padding.x :
			fallbackSize.x;
	}
	if(size.y == autoSize) {
		size.y = nextWidget ?
			nextWidget->size().y + 2 * style.padding.y :
			fallbackSize.y;
	}

	// change
	auto bgc = bg_.change();
	bgc->position = pos;
	bgc->size = size;
	bgc->rounding = style.rounding;
	bgc->drawMode = {true, style.bgStroke ? 2.f : 0.f};

	// propagate
	if(sc) {
		dlg_assert(style.bg);
		style_ = &style;
		requestRerecord(); // NOTE: could be optimized, not always needed
	}

	if(owidget) {
		this->widget(std::move(*owidget));
	}

	if(bc) {
		ContainerWidget::bounds({pos, size});
	}

	if(nextWidget) {
		nextWidget->bounds(childBounds());
	}

	requestRedraw();
}

void Pane::bounds(const Rect2f& b) {
	reset(style(), b);
}

void Pane::style(const PaneStyle& s, bool force) {
	reset(s, bounds(), force);
}

void Pane::hide(bool hide) {
	bg_.disable(hide);
	ContainerWidget::hide(hide);
	requestRedraw();
}

bool Pane::hidden() const {
	return bg_.disabled(DrawType::fill);
}

void Pane::draw(vk::CommandBuffer cb) const {
	ContainerWidget::bindScissor(cb);
	style().bg->bind(cb);
	bg_.fill(cb);

	if(style().bgStroke) {
		style().bgStroke->bind(cb);
		bg_.stroke(cb);
	}

	ContainerWidget::draw(cb);
}

Widget* Pane::widget() const {
	dlg_assert(widgets_.size() <= 1);
	return widgets_.empty() ? nullptr : widgets_.front().get();
}

void Pane::widget(std::unique_ptr<Widget> w, bool resizeSelf) {
	if(resizeSelf) {
		auto bounds = Rect2f {position(), {autoSize, autoSize}};
		reset(style(), bounds, false, std::move(w));
	} else {
		if(w) {
			w->bounds(childBounds());
		}
		widget(std::move(w));
	}
}

Rect2f Pane::childBounds() const {
	auto p = style().padding;
	return {position() + p, size() - 2 * p};
}

void Pane::widget(std::unique_ptr<Widget> w) {
	dlg_assert(widgets_.size() <= 1);
	if(!widgets_.empty()) {
		dlg_assert(widgets_.front());
		ContainerWidget::destroy(*widgets_.front());
	}

	if(w) {
		ContainerWidget::add(std::move(w));
	}
}

Widget& Pane::add(std::unique_ptr<Widget> w) {
	dlg_warn("Pane::add has replacing semantics and shouldn't be called");
	dlg_assert(w);
	auto& ret = *w;
	widget(std::move(w));
	return ret;
}

std::unique_ptr<Widget> Pane::remove() {
	if(!widget()) {
		return {};
	}

	auto ret = ContainerWidget::remove(*widget());
	dlg_assert(ret && !widget());
	return ret;
}

} // namespace vui
