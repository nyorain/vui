#include <vui/gui.hpp>
#include <vui/widget.hpp>
#include <dlg/dlg.hpp>
#include <nytl/rectOps.hpp>
#include <nytl/matOps.hpp>
#include <cmath>

namespace vui {

// GuiListener
GuiListener& GuiListener::nop() {
	static GuiListener listener;
	return listener;
}

// Gui
Gui::Gui(Context& ctx, const Font& font, Styles&& s, GuiListener& listener)
		: WidgetContainer(*this),  context_(ctx), font_(font),
			listener_(listener), styles_(std::move(s)) {
	nytl::identity(transform_);
}

Gui::Gui(Context& ctx, const Font& font, GuiListener& listener)
		: WidgetContainer(*this),  context_(ctx), font_(font),
			listener_(listener) {
	defaultStyles_.emplace(ctx);
	styles_ = defaultStyles_->styles();
}

void Gui::transform(const nytl::Mat4f& mat) {
	transform_ = mat;
	refreshTransform();
}

Widget* Gui::mouseMove(const MouseMoveEvent& ev) {
	if(buttonGrab_.first) {
		return buttonGrab_.first->mouseMove(ev);
	}

	auto w = WidgetContainer::mouseMove(ev);
	if(mouseOver() != w) {
		listener().mouseOver(mouseOverWidget_, w);
		mouseOverWidget_ = w;
	}

	return w;
}

Widget* Gui::mouseButton(const MouseButtonEvent& ev) {
	if(!ev.pressed && buttonGrab_.first && ev.button == buttonGrab_.second) {
		auto w = buttonGrab_.first;
		buttonGrab_.first->mouseButton(ev);
		buttonGrab_ = {};
		this->mouseMove({ev.position});
		return w;
	}

	auto w = WidgetContainer::mouseButton(ev);
	if(focus() != w) {
		listener().focus(focus(), w);
		focusWidget_ = w;
	}

	if(ev.pressed && w) {
		// TODO: send release to old buttonGrab_?
		//   or use multiple button grabs?
		buttonGrab_ = {w, ev.button};
	}

	return w;
}

void Gui::focus(bool gained) {
	WidgetContainer::focus(gained);
	if(!gained && focus()) {
		listener().focus(focus(), nullptr);
		focusWidget_ = nullptr;
	}
}

void Gui::mouseOver(bool gained) {
	WidgetContainer::mouseOver(gained);
	if(!gained && mouseOver()) {
		listener().focus(mouseOver(), nullptr);
		mouseOverWidget_ = nullptr;
	}
}

void Gui::update(double delta) {
	auto moved = std::move(update_);
	for(auto& widget : moved) {
		dlg_assert(widget);
		widget->update(delta);
	}
}

bool Gui::updateDevice() {
	auto moved = std::move(updateDevice_);
	bool rerecord = rerecord_;

	for(auto& widget : moved) {
		dlg_assert(widget);
		rerecord |= widget->updateDevice();
	}

	if(!destroyWidgets_.empty()) {
		destroyWidgets_.clear();
		rerecord = true;
	}

	rerecord_ = false;
	return rerecord;
}

void Gui::moveDestroyWidget(std::unique_ptr<Widget> w) {
	dlg_assert(w);

	// TODO: hack atm
	// we want to remove all this stuff for ALL (sub-)children of w

	// if(focusWidget_ == w.get()) {
		focusWidget_ = nullptr;
	// }

	// if(mouseOverWidget_ == w.get()) {
		mouseOverWidget_ = nullptr;
	// }

	// if(buttonGrab_.first == w.get()) {
		buttonGrab_ = {};
	// }

	destroyWidgets_.emplace_back(std::move(w));
}
void Gui::addUpdate(Widget& widget) {
	update_.insert(&widget);
}
void Gui::addUpdateDevice(Widget& widget) {
	updateDevice_.insert(&widget);
}


} // namespace vui
