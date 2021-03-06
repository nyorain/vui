#include <vui/widget.hpp>
#include <vui/gui.hpp>
#include <dlg/dlg.hpp>

#include <nytl/matOps.hpp>
#include <nytl/rectOps.hpp>

namespace vui {

// Widget
Widget::Widget(Gui& gui, ContainerWidget* p) : gui_(gui), parent_(p) {
	requestRerecord();
}

Widget::~Widget() {
	gui().removed(*this);
}

bool Widget::contains(Vec2f point) const {
	dlg_assert(bounds().size.x >= 0 && bounds().size.y >= 0);
	return nytl::contains(bounds_, point);
}

Context& Widget::context() const {
	return gui().context();
}

void Widget::registerUpdate() {
	gui().addUpdate(*this);
}

void Widget::registerUpdateDevice() {
	gui().addUpdateDevice(*this);
}

void Widget::bounds(const Rect2f& b) {
	dlg_assertm(b.size.x >= 0 && b.size.y >= 0, "{}", b);
	if(b == bounds_) {
		return;
	}

	bounds_ = b;
	updateScissor();

	// We could call parent()->relayout() here (at least when size
	// changes) but that leads to many weird recursion problems.
	// When someone else than the parent changes the bounds of a widget,
	// the caller must know what they are doing and fix everything
	// manually
}

void Widget::updateScissor() {
	if(scissor_.valid()) {
		auto s = scissor();
		dlg_assert(s.size.x >= 0 && s.size.y >= 0);
		scissor_.rect(s);
	}
}

void Widget::mouseOver(bool over) {
	if(over) {
		gui().listener().cursor(cursor());
	}
}

Cursor Widget::cursor() const {
	// default cursor
 	return Cursor::pointer;
}

void Widget::bindScissor(vk::CommandBuffer cb) const {
	// only create it when really needed
	// bindScissor will only be called from widgets that actually draw
	// stuff
	if(!scissor_.valid()) {
		dlg_assert(bounds().size.x >= 0 && bounds().size.y >= 0);
		scissor_ = {context(), scissor()};
	}

	scissor_.bind(cb);
}

Rect2f Widget::scissor() const {
	if(parent()) {
		return intersection(ownScissor(), parent()->scissor());
	} else {
		dlg_warn("Widget::scissor called on orphaned widget");
		return ownScissor();
	}
}

bool Widget::isDescendant(const Widget& up) const {
	return parent() && (&up == parent() || parent()->isDescendant(up));
}

void Widget::parent(Widget& widget, ContainerWidget* newParent) {
	if(widget.inHierachy() && (!newParent || !newParent->inHierachy())) {
		widget.gui().removed(widget);
	}
	widget.parent_ = newParent;
	if(widget.inHierachy()) {
		widget.gui().rerecord();
	}
}

void Widget::callPasteResponse(Widget& w, std::string_view str) {
	w.pasteResponse(str);
}

bool Widget::update(double) {
	// see header file function doc for reason of warning
	dlg_warn("Widget::update: default implementation called");
	return false;
}

bool Widget::updateDevice() {
	// see header file function doc for reason of warning
	dlg_warn("Widget::updateDevice: default implementation called");
	return false;
}

bool Widget::inHierachy() const {
	return isDescendant(gui());
}

void Widget::requestRedraw() {
	if(inHierachy()) {
		gui().redraw();
	}
}

void Widget::requestRerecord() {
	if(inHierachy()) {
		gui().rerecord();
	}
}

} // namespace vui
