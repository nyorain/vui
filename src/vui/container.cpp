#include <vui/container.hpp>
#include <dlg/dlg.hpp>

namespace vui {

// WidgetContainer
Widget* WidgetContainer::widgetAt(Vec2f pos) {
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

void WidgetContainer::refreshMouseOver(Vec2f pos) {
	// NOTE: although it would be a neat optimization, we can't
	// start with checking if it is still over the current mouseOver_
	// and have to start from scratch every time since position
	// from higher, overlapping widgets could have changed.
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

void WidgetContainer::refreshFocus() {
	if(focus_ && focus_->hidden()) {
		focus_->focus(false);
		focus_ = nullptr;
	}
}

void WidgetContainer::reorder() {
	std::sort(widgets_.begin(), widgets_.end(), zWidgetOrder);
}

// we always call the refresh(mouseOver/focus) since the widgets
// properties might have changed from the time they got focus/mouseOver
// to now (e.g. they might have gone hidden)
Widget* WidgetContainer::mouseMove(const MouseMoveEvent& ev) {
	refreshMouseOver(ev.position);
	return mouseOver_ ? mouseOver_->mouseMove(ev) : nullptr;
}

Widget* WidgetContainer::mouseButton(const MouseButtonEvent& ev) {
	refreshMouseOver(ev.position);
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

Widget* WidgetContainer::mouseWheel(const MouseWheelEvent& ev) {
	refreshMouseOver(ev.position);
	return mouseOver_? mouseOver_->mouseWheel(ev) : nullptr;
}

Widget* WidgetContainer::key(const KeyEvent& ev) {
	refreshFocus();
	return focus_ ? focus_->key(ev) : nullptr;
}

Widget* WidgetContainer::textInput(const TextInputEvent& ev) {
	refreshFocus();
	return focus_ ? focus_->textInput(ev) : nullptr;
}

void WidgetContainer::focus(bool gained) {
	if(!gained && focus_) {
		focus_->focus(false);
		focus_ = nullptr;
	}
}

void WidgetContainer::mouseOver(bool gained) {
	if(!gained && mouseOver_) {
		mouseOver_->mouseOver(false);
		mouseOver_ = nullptr;
	}
}

void WidgetContainer::draw(vk::CommandBuffer cb) const {
	for(auto& widget : widgets_) {
		dlg_assert(widget);
		widget->draw(cb);
	}
}

Widget& WidgetContainer::add(std::unique_ptr<Widget> widget) {
	dlg_assert(widget);
	auto ub = std::upper_bound(widgets_.begin(), widgets_.end(),
		widget, zWidgetOrder);
	widgets_.insert(ub, std::move(widget));
	return *widgets_.back();
}

void WidgetContainer::updateTransform(const nytl::Mat4f& mat) {
	for(auto& w : widgets_) {
		dlg_assert(w);
		w->updateTransform(mat);
	}
}

// ContainerWidget
ContainerWidget::ContainerWidget(Gui& gui)
	: Widget(gui), WidgetContainer() {
}

void ContainerWidget::hide(bool hide) {
	for(auto& w : widgets_) {
		dlg_assert(w);
		w->hide(hide);
	}
}

void ContainerWidget::position(Vec2f pos) {
	auto old = position();
	Widget::position(pos);

	// we just move all widgets by the offset
	for(auto& w : widgets_) {
		dlg_assert(w);
		auto rel = w->position() - old;
		w->position(pos + rel);
		w->intersectScissor(scissor());
	}
}

void ContainerWidget::size(Vec2f size) {
	Widget::size(size);
	for(auto& w : widgets_) {
		dlg_assert(w);
		w->intersectScissor(scissor());
	}
}

void ContainerWidget::refreshTransform() {
	Widget::refreshTransform();
	for(auto& w : widgets_) {
		dlg_assert(w);
		w->refreshTransform();
	}
}

Widget& ContainerWidget::add(std::unique_ptr<Widget> w) {
	w->intersectScissor(scissor());
	return WidgetContainer::add(std::move(w));
}

Widget* ContainerWidget::mouseMove(const MouseMoveEvent& ev) {
	return WidgetContainer::mouseMove(ev);
}

Widget* ContainerWidget::mouseButton(const MouseButtonEvent& ev) {
	return WidgetContainer::mouseButton(ev);
}

Widget* ContainerWidget::mouseWheel(const MouseWheelEvent& ev) {
	return WidgetContainer::mouseWheel(ev);
}

Widget* ContainerWidget::key(const KeyEvent& ev) {
	return WidgetContainer::key(ev);
}

Widget* ContainerWidget::textInput(const TextInputEvent& ev) {
	return WidgetContainer::textInput(ev);
}

void ContainerWidget::focus(bool gained) {
	return WidgetContainer::focus(gained);
}

void ContainerWidget::mouseOver(bool gained) {
	return WidgetContainer::mouseOver(gained);
}

void ContainerWidget::draw(vk::CommandBuffer cb) const {
	WidgetContainer::draw(cb);
}

} // namespace vui
