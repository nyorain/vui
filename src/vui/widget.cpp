#include <vui/widget.hpp>
#include <vui/gui.hpp>

#include <nytl/matOps.hpp>
#include <nytl/rectOps.hpp>

namespace vui {

Widget::Widget(Gui& gui, const Rect2f& bounds) :
		gui_(gui), bounds_(bounds), intersectScissor_(Scissor::reset) {

	auto mat = nytl::identity<4, float>();
	mat[0][3] = bounds_.position.x;
	mat[1][3] = bounds_.position.y;
	transform_ = {context(), gui.transform() * mat};

	auto b = bounds;
	b.position = {};
	scissor_ = {context(), b};
}

void Widget::bindState(vk::CommandBuffer cb) const {
	scissor_.bind(cb);
	transform_.bind(cb);
}

void Widget::size(Vec2f size) {
	bounds_.size = size;
	updateScissor();
}

void Widget::refreshTransform() {
	auto mat = nytl::identity<4, float>();
	mat[0][3] = bounds_.position.x;
	mat[1][3] = bounds_.position.y;
	transform_.matrix(gui().transform() * mat);
}

void Widget::position(Vec2f position) {
	bounds_.position = position;
	refreshTransform();
	updateScissor();
}

bool Widget::contains(Vec2f point) const {
	return nytl::contains(bounds_, point);
}

void Widget::intersectScissor(const Rect2f& rect) {
	intersectScissor_ = rect;
	updateScissor();
}

void Widget::updateScissor() {
	auto cpy = intersectScissor_;
	cpy.position -= bounds_.position;
	scissor_.rect(nytl::intersection(cpy, ownScissor()));
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

Rect2f Widget::ownScissor() const {
	return {{}, size()};
}

Rect2f Widget::scissor() const {
	auto rect = scissor_.rect();
	rect.position += position();
	return rect;
}

} // namespace vui
