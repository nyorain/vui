#include <vui/gui.hpp>
#include <vui/widget.hpp>
#include <rvg/context.hpp>
#include <dlg/dlg.hpp>
#include <nytl/rectOps.hpp>
#include <nytl/matOps.hpp>
#include <cmath>

namespace vui {
namespace {

/// Returns whether desc is in subtree of root
bool inSubtree(const Widget& desc, const Widget& root) {
	return &desc == &root || desc.isDescendant(root);
}

} // anon namespace

// GuiListener
GuiListener& GuiListener::nop() {
	static GuiListener listener;
	return listener;
}

// Gui
Gui::Gui(Context& ctx, const Font& font, GuiListener& listener)
		: ContainerWidget(*this, nullptr),  context_(ctx), font_(font),
			listener_(listener) {
	transform_ = {ctx};
	defaultStyles_.emplace(ctx);
	styles_ = defaultStyles_->styles();
}

Gui::Gui(Context& ctx, const Font& font, Styles&& s, GuiListener& listener)
		: ContainerWidget(*this, nullptr),  context_(ctx), font_(font),
			listener_(listener), styles_(std::move(s)) {
}

void Gui::transform(const nytl::Mat4f& mat) {
	transform_.matrix(mat);
}

Widget* Gui::mouseMove(const MouseMoveEvent& ev) {
	if(buttonGrab_.first) {
		return buttonGrab_.first->mouseMove(ev);
	}

	auto w = ContainerWidget::mouseMove(ev);
	if(mouseOver() != w) {
		listener().mouseOver(globalMouseOver_, w);
		globalMouseOver_ = w;
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

	auto w = ContainerWidget::mouseButton(ev);
	if(focus() != w) {
		listener().focus(focus(), w);
		globalFocus_ = w;
	}

	if(ev.pressed && w) {
		// NOTE: we simply cancel the old button grab.
		// we could also support multiple button grabs but things probably
		// get more complicated then
		if(buttonGrab_.first) {
			auto r = MouseButtonEvent {false, buttonGrab_.second, ev.position};
			buttonGrab_.first->mouseButton(r);
		}
		buttonGrab_ = {w, ev.button};
	}

	return w;
}

void Gui::focus(bool gained) {
	ContainerWidget::focus(gained);
	if(!gained && focus()) {
		listener().focus(focus(), nullptr);
		globalFocus_ = nullptr;
	}
}

void Gui::mouseOver(bool gained) {
	ContainerWidget::mouseOver(gained);
	if(!gained && mouseOver()) {
		listener().focus(mouseOver(), nullptr);
		globalMouseOver_ = nullptr;
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

void Gui::draw(vk::CommandBuffer cb) const {
	context().bindDefaults(cb);
	transform_.bind(cb);
	ContainerWidget::draw(cb);
}

// informs the gui object that this widget has been removed from the hierachy
void Gui::removed(Widget& widget) {
	if(globalFocus_ && inSubtree(widget, *globalFocus_)) {
		listener().focus(globalFocus_, nullptr);
		globalFocus_ = nullptr;
	}

	if(globalMouseOver_ && inSubtree(widget, *globalMouseOver_)) {
		listener().mouseOver(globalMouseOver_, nullptr);
		globalMouseOver_ = nullptr;
	}

	if(buttonGrab_.first && inSubtree(widget, *buttonGrab_.first)) {
		buttonGrab_ = {};
	}
}

void Gui::moveDestroyWidget(std::unique_ptr<Widget> w) {
	dlg_assert(w);
	destroyWidgets_.emplace_back(std::move(w));
}

void Gui::addUpdate(Widget& widget) {
	update_.insert(&widget);
}

void Gui::addUpdateDevice(Widget& widget) {
	updateDevice_.insert(&widget);
}

} // namespace vui
