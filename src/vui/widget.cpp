#include <vui/widget.hpp>
#include <vui/gui.hpp>
#include <dlg/dlg.hpp>

#include <nytl/matOps.hpp>
#include <nytl/rectOps.hpp>

namespace vui {

// Widget
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

void Widget::relayout(const Rect2f& b) {
	dlg_assert(b.size.x >= 0 && b.size.y >= 0);
	if(!(b == bounds_)) {
		bounds_ = b;
		updateScissor();
	}
}

void Widget::updateScissor() {
	if(scissor_.valid()) {
		dlg_assert(bounds().size.x >= 0 && bounds().size.y >= 0);
		scissor_.rect(nytl::intersection(ownScissor(), intersectScissor()));
	}
}

void Widget::intersectScissor(const Rect2f& rect) {
	dlg_assert(rect.size.x >= 0 && rect.size.y >= 0);
	intersectScissor_ = rect;
	updateScissor();
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
		auto s = nytl::intersection(ownScissor(), intersectScissor());
		scissor_ = {context(), s};
	}

	scissor_.bind(cb);
}

// MovableWidget
MovableWidget::MovableWidget(Gui& gui) : Widget(gui) {
	transform_ = {context()};
}

void MovableWidget::updateTransform(const nytl::Mat4f& t) {
	auto mat = nytl::identity<4, float>();
	mat[0][3] = bounds().position.x;
	mat[1][3] = bounds().position.y;
	transform_.matrix(t * mat);
}

void MovableWidget::position(Vec2f n) {
	if(n == bounds().position) {
		return;
	}

	// we cheat here a little bit so we don't have to store
	// the transform from updateTransform
	// note that this is safe because only the translation
	// effect of the matrix is ever changed.
	auto mat = transform_.matrix();
	auto o = bounds().position;
	mat[0][3] -= (o.x / mat[0][0]) + (o.y / mat[0][1]);
	mat[1][3] -= (o.y / mat[1][1]) + (o.x / mat[1][0]);
	mat[0][3] += (n.x * mat[0][0]) + (n.y * mat[0][1]);
	mat[1][3] += (n.y * mat[1][1]) + (n.x * mat[1][0]);
	transform_.matrix(mat);
}

void MovableWidget::relayout(const Rect2f& b) {
	position(b.position);
	size(b.size);
	Widget::relayout(b);
}

void MovableWidget::bindTransform(vk::CommandBuffer cb) const {
	transform_.bind(cb);
}

} // namespace vui
