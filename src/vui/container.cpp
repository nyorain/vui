#include <vui/container.hpp>
#include <vui/gui.hpp>
#include <dlg/dlg.hpp>
#include <algorithm>

namespace vui {
namespace {

template<typename C>
auto findWidget(C& widgets, const Widget& tofind) {
	return std::find_if(widgets.begin(), widgets.end(),
			[&](const std::unique_ptr<Widget>& ptr){
				return ptr.get() == &tofind;
			});
}

} // anon namespace

// WidgetContainer
Widget* ContainerWidget::widgetAt(Vec2f pos) {
	// since widgets are ordered by z order (lower to higher) we
	// have to traverse them in reverse
	for(auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
		auto& w = *it;
		if(!w->hidden() && w->contains(pos)) {
			return w.get();
		}
	}

	return nullptr;
}

void ContainerWidget::refreshMouseOver(Vec2f pos) {
	if(!invalidated_ && mouseOver_ && mouseOver_->contains(pos)) {
		return;
	}

	auto over = widgetAt(pos);
	if(over != mouseOver_) {
		if(mouseOver_) {
			mouseOver_->mouseOver(false);
		}
		mouseOver_ = over;
		if(over) {
			over->mouseOver(true);
		}
	}
}

void ContainerWidget::refreshFocus() {
	if(focus_ && focus_->hidden()) {
		focus_->focus(false);
		focus_ = nullptr;
	}
}

Widget* ContainerWidget::mouseMove(const MouseMoveEvent& ev) {
	refreshMouseOver(ev.position);
	if(invalidated_) {
		refreshFocus();
		invalidated_ = false;
	}

	return mouseOver_ ? mouseOver_->mouseMove(ev) : nullptr;
}

Widget* ContainerWidget::mouseButton(const MouseButtonEvent& ev) {
	if(invalidated_) {
		refreshMouseOver(ev.position);
		refreshFocus();
		invalidated_ = false;
	}

	if(mouseOver_ != focus_) {
		if(focus_) {
			focus_->focus(false);
		}
		focus_ = mouseOver_;
		if(focus_) {
			focus_->focus(true);
		}
	}

	return mouseOver_? mouseOver_->mouseButton(ev) : nullptr;
}

Widget* ContainerWidget::mouseWheel(const MouseWheelEvent& ev) {
	if(invalidated_) {
		refreshMouseOver(ev.position);
		refreshFocus();
		invalidated_ = false;
	}

	return mouseOver_? mouseOver_->mouseWheel(ev) : nullptr;
}

Widget* ContainerWidget::key(const KeyEvent& ev) {
	if(invalidated_) {
		refreshFocus();
	}

	return focus_ ? focus_->key(ev) : nullptr;
}

Widget* ContainerWidget::textInput(const TextInputEvent& ev) {
	if(invalidated_) {
		refreshFocus();
	}

	return focus_ ? focus_->textInput(ev) : nullptr;
}

void ContainerWidget::focus(bool gained) {
	if(!gained && focus_) {
		focus_->focus(false);
		focus_ = nullptr;
	}
}

void ContainerWidget::mouseOver(bool gained) {
	if(!gained && mouseOver_) {
		mouseOver_->mouseOver(false);
		mouseOver_ = nullptr;
	}
}

void ContainerWidget::draw(vk::CommandBuffer cb) const {
	for(auto& widget : widgets_) {
		dlg_assert(widget);
		widget->draw(cb);
	}
}

Widget& ContainerWidget::add(std::unique_ptr<Widget> widget) {
	dlg_assert(widget && findWidget(widgets_, *widget) == widgets_.end());
	auto& ret = *widget;
	widgets_.emplace_back(std::move(widget));
	if(ret.parent() != this) {
		dlg_assertm(!ret.parent(), "ContainerWidget::add: "
			"given widget already has a parent");
		Widget::parent(ret, this);
	}

	ret.updateScissor();
	return ret;
}

std::unique_ptr<Widget> ContainerWidget::remove(const Widget& widget) {
	auto it = findWidget(widgets_, widget);
	if(it == widgets_.end()) {
		return {};
	}

	// not sure if focus/mouseOver calls are needed/good idea
	// but since the returned widget might not be destroyed and re-added
	// later on it's probably cleaner/safer this way
	if(focus_ == &widget) {
		focus_->focus(false);
		focus_ = nullptr;
	}

	if(mouseOver_ == &widget) {
		mouseOver_->mouseOver(false);
		mouseOver_ = nullptr;
	}


	auto ret = std::move(*it);
	widgets_.erase(it);
	gui().removed(*ret);
	return ret;
}

bool ContainerWidget::destroy(const Widget& widget) {
	auto moved = remove(widget);
	if(!moved) {
		return false;
	}

	gui().moveDestroyWidget(std::move(moved));
	return true;
}

bool ContainerWidget::raiseAbove(const Widget& raise, const Widget& above) {
	auto r = findWidget(widgets_, raise);
	auto a = findWidget(widgets_, above);
	if(r == widgets_.end() || a == widgets_.end() || r == a) {
		return false;
	}

	if(r >= a) {
		return true;
	}

	// basically (sketches help): move r after a
	std::rotate(r, r + 1, a + 1);
	gui().rerecord();
	return true;
}

bool ContainerWidget::lowerBelow(const Widget& lower, const Widget& below) {
	auto l = findWidget(widgets_, lower);
	auto b = findWidget(widgets_, below);
	if(l == widgets_.end() || b == widgets_.end() || l == b) {
		return false;
	}

	if(l <= b) {
		return true;
	}

	// basically (sketches help): move l before b
	std::rotate(b, l, l);
	gui().rerecord();
	return true;
}

const Widget* ContainerWidget::highestWidget() const {
	return widgets_.empty() ? nullptr : widgets_.back().get();
}

const Widget* ContainerWidget::lowestWidget() const {
	return widgets_.empty() ? nullptr : widgets_.front().get();
}

bool ContainerWidget::hasChild(const Widget& w) const {
	// faster than iterating through all children
	return w.parent() == this;
}

bool ContainerWidget::hasDescendant(const Widget& w) const {
	// faster than iterating through all children for each generation
	return w.isDescendant(*this);
}

void ContainerWidget::childChanged() {
	invalidated_ = true;
}

void ContainerWidget::updateScissor() {
	Widget::updateScissor();
	for(auto& w : widgets_) {
		dlg_assert(w);
		w->updateScissor();
	}
}

void ContainerWidget::bounds(const Rect2f& b) {
	// NOTE: all widgets will currently call updateScissor twice,
	// once triggered by position and the other by our own updateScissor
	// call that is called in Widget::bounds

	// we just move all widgets by the offset
	if(b.position != position()) {
		auto off = b.position - position();
		for(auto& w : widgets_) {
			dlg_assert(w);
			w->position(w->position() + off);
		}
	}

	Widget::bounds(b);
}

} // namespace vui
