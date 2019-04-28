#include <vui/dat.hpp>
#include <vui/gui.hpp>

#include <rvg/font.hpp>
#include <dlg/dlg.hpp>
#include <nytl/rectOps.hpp>
#include <nytl/scope.hpp>

namespace vui::dat {
namespace colors {

const auto name = rvg::Color {255u, 255u, 255u};
// const auto line = rvg::Color {15u, 15u, 15u};
const auto line = rvg::Color {44u, 44u, 44u}; // #2c2c2c, dat.gui
// const auto button = rvg::Color {200u, 20u, 20u};
const auto button = rvg::Color {230u, 29u, 95u};
// const auto text = rvg::Color {20u, 120u, 20u};
const auto text = rvg::Color {47u, 161u, 214u};
const auto label = rvg::Color {36u, 220u, 198u};
const auto range = rvg::Color {20u, 20u, 120u};
const auto checkbox = rvg::Color {120u, 20u, 120u};

const auto fg = rvg::Color {221, 221, 221};
const auto bg = rvg::Color {26u, 26u, 26u}; // @1a1a1a, dat.gui
const auto bgHover = rvg::Color {10u, 10u, 10u};
const auto bgActive = rvg::Color {10u, 10u, 10u};

const auto bgWidget = rvg::Color {48u, 48u, 48u}; // #303030, dat.gui
const auto bgWidgetHover = rvg::Color {60u, 60u, 60u}; // #3c3c3c, dat.gui
const auto bgWidgetFocus = rvg::Color {73u, 73u, 73u}; // #494949, dat.gui

const auto metaButtonBg = rvg::Color {0u, 0u, 0u}; // #000 dat.gui
const auto metaButtonBgHover = bgHover;
const auto metaButtonBgPressed = metaButtonBgHover;
// const auto folderLine = rvg::Color {255u, 255u, 255u, 8u};
const auto folderLine = rvg::Color {255u, 255u, 255u, 5u};

} // namespace colors

// TODO: assert expected sizes (e.g. in constructors/bounds)
// assert(bounds.size.y == panel().rowHeight());
// don't expose bounds publicly?
// TODO: currently controllers/folders can't be accessed out of the
// hierachy at all, they can't be created without parent.
// change that? decouple panel and controller/folder at least so far
// that this is possible (manually passing row height and name width
// to contoller/folder?) We could decouple them completely when we
// store all the paints and styles in gui (or some other, with gui
// connected central instance) instead of in the panel (which is
// redundant/resource-wasting to do for every panel anyways)

constexpr auto classifierWidth = 3.f; // width of color classifiers
constexpr auto lineHeight = 1.f; // bottomLine (separation) height
constexpr auto folderOffset = 4.f; // x position offset for folder children
constexpr auto namePadding = 10.f;

// Container
void Container::relayout() {
	if(relayouting_) {
		return;
	}

	// we change children
	// Those in turn might signal us to relayout
	relayouting_ = true;
	auto guad = nytl::ScopeGuard {[&]{ relayouting_ = false; }};

	// currently width of container is never changed so we only
	// have to correct position
	auto y = position().y;
	for(auto& w : widgets_) {
		if(y != w->position().y) {
			w->position({w->position().x, y});
		}

		y += w->size().y;
	}

	if(y - position().y != size().y) {
		height(y - position().y - size().y); // passing delta
	}
}

std::unique_ptr<Widget> Container::remove(const Widget& w) {
	auto ptr = ContainerWidget::remove(w);
	if(!ptr) {
		return {};
	}

	height(-w.size().y);
	relayout();
	return ptr;
}

Widget& Container::add(std::unique_ptr<Widget> w) {
	// TODO: pass on the panels row height and name width to
	// the given widget

	dlg_assert(w);
	auto nb = nextBounds();
	nb.size.y = w->size().y;
	auto& ret = ContainerWidget::add(std::move(w));
	ret.bounds(nb);
	height(ret.size().y);
	return ret;
}

void Container::height(float delta) {
	auto s = size();
	s.y += delta;
	dlg_assert(s.y > 0);
	Container::size(s); // will refresh all child scissors
	// parent()->relayout();
}

void Container::hide(bool h) {
	if(open_ || h) {
		ContainerWidget::hide(h);
	}
}

void Container::open(bool open) {
	if(open_ == open) {
		return;
	}

	open_ = open;
	auto hidden = this->hidden();
	auto size = this->size();

	if(open) {
		size.y = 0.f;
		for(auto& w : widgets_) {
			size.y += w->size().y;
		}
	} else {
		// not too well designed but works
		size.y = closedHeight();
	}

	for(auto& w : widgets_) {
		w->hide(hidden || !open);
	}

	if(size != this->size()) {
		Container::size(size);
		// parent()->relayout();
	}
}

Rect2f Container::nextBounds() const {
	auto pos = position();
	if(!widgets_.empty()) {
		pos = widgets_.back()->position();
		pos.y += widgets_.back()->size().y;
	}

	return {pos, {size().x, panel().rowHeight()}};
}

float Container::closedHeight() const {
	return panel().rowHeight();
}

bool Container::moveBefore(const Widget& move, const Widget& before, bool e) {
	auto ret = ContainerWidget::moveBefore(move, before, e);
	if(ret) {
		relayout();
	}
	return ret;
}

bool Container::moveAfter(const Widget& move, const Widget& after, bool e) {
	auto ret = ContainerWidget::moveAfter(move, after, e);
	if(ret) {
		relayout();
	}
	return ret;
}

// Panel
Panel::Panel(Gui& gui, ContainerWidget* p, const Vec2f& pos, float width,
	float nameWidth, float rowHeight) :
		Container(gui, p), rowHeight_(rowHeight), nameWidth_(nameWidth) {

	if(rowHeight_ == autoSize) {
		rowHeight_ = 5 + 1.5 * gui.font().height();
	}

	if(nameWidth_ == autoSize) {
		nameWidth_ = gui.font().width("Rather long name");
	}

	if(width == autoSize) {
		width = nameWidth_ * 3;
	}

	Widget::bounds({pos, {width, rowHeight_}});

	// paints
	using namespace colors;
	paints_.name = {context(), rvg::colorPaint(name)};
	paints_.line = {context(), rvg::colorPaint(line)};
	paints_.folderLine = {context(), rvg::colorPaint(folderLine)};
	paints_.buttonClass = {context(), rvg::colorPaint(button)};
	paints_.labelClass = {context(), rvg::colorPaint(label)};
	paints_.textClass = {context(), rvg::colorPaint(text)};
	paints_.rangeClass = {context(), rvg::colorPaint(range)};
	paints_.checkboxClass = {context(), rvg::colorPaint(checkbox)};

	paints_.bg = {context(), rvg::colorPaint(bg)};
	paints_.bgHover = {context(), rvg::colorPaint(bgHover)};
	paints_.bgActive = {context(), rvg::colorPaint(bgActive)};

	paints_.bgWidget = {context(), rvg::colorPaint(bgWidget)};

	// styles
	// TODO: parameterize constants
	auto cp = [&](auto& col) { return rvg::colorPaint(col); };

	auto yoff = ((rowHeight_ - 4.f) - gui.font().height()) / 2.f;
	auto xoff = std::max(yoff * 1.5f, 2.f);

	styles_.textfield = gui.styles().textfield;
	styles_.textfield.normal = {cp(bgWidget), cp(fg), {}};
	styles_.textfield.hovered = {cp(bgWidgetHover), cp(fg), {}};
	styles_.textfield.focused = {cp(bgWidgetFocus), cp(fg), {}};
	styles_.textfield.padding = {xoff, yoff};

	styles_.button = gui.styles().basicButton;
	styles_.button.normal = {cp(metaButtonBg), {}, cp(fg)};
	styles_.button.hovered = {cp(metaButtonBgHover), {}, cp(fg)};
	styles_.button.pressed = {cp(metaButtonBgPressed), {}, cp(fg)};
	styles_.button.rounding = {};

	styles_.metaButton = gui.styles().labeledButton;
	styles_.metaButton.basic = &styles_.button;

	// toggle button
	// auto buttonHeight = 10 + gui.font().height();
	auto buttonHeight = rowHeight_;
	auto btnBounds = Rect2f {position(), {width, buttonHeight}};
	auto btn = std::make_unique<LabeledButton>(gui, this, btnBounds,
		"Toggle Controls", panel().styles().metaButton);
	toggleButton_ = btn.get();
	widgets_.push_back(std::move(btn));
	toggleButton_->onClick = [&](auto&){ this->toggle(); };
}

void Panel::hide(bool hide) {
	// toggleButton_ will be hidden by this since it's in widgets_
	Container::hide(hide);
}

bool Panel::hidden() const {
	return toggleButton_->hidden();
}

Widget& Panel::add(std::unique_ptr<Widget> w) {
	// since toggleButton_ is also a member of widgets_ we make
	// sure that we add the new widget before it
	dlg_assert(!widgets_.empty() && widgets_.back().get() == toggleButton_);
	auto& ret = Container::add(std::move(w));
	std::swap(widgets_.back(), widgets_[widgets_.size() - 2]);

	auto y = position().y + size().y - rowHeight_;
	toggleButton_->position({position().x, y});
	toggleButton_->updateScissor();

	return ret;
}

void Panel::open(bool o) {
	if(o == open_) {
		return;
	}

	auto before = toggleButton_->hidden();
	Container::open(o);
	toggleButton_->hide(before); // Container::open changes it

	auto y = position().y + size().y - toggleButton_->size().y;
	toggleButton_->position({position().x, y});
	toggleButton_->updateScissor();
}

Rect2f Panel::nextBounds() const {
	dlg_assert(!widgets_.empty() && widgets_.back().get() == toggleButton_);
	auto pos = position();
	if(widgets_.size() > 1) {
		auto& last = *widgets_[widgets_.size() - 2];
		pos = last.position();
		pos.y += last.size().y;
	}

	return {pos, {size().x, panel().rowHeight()}};
}

// Folder
Folder::Folder(Container& parent, const Rect2f& bounds, std::string_view name) :
		Container(parent.gui(), &parent) {

	bottomLine_ = {context(), {}, {false, lineHeight}};

	auto btnBounds = Rect2f{position(), {bounds.size.x, panel().rowHeight()}};
	auto btn = std::make_unique<LabeledButton>(gui(), this, btnBounds,
		name, panel().styles().metaButton);
	toggleButton_ = btn.get();
	widgets_.push_back(std::move(btn));
	toggleButton_->onClick = [&](auto&){ this->toggle(); };

	this->bounds(bounds);
}

void Folder::bounds(const Rect2f& b) {
	{
		auto blc = bottomLine_.change();
		blc->points = {
			b.position + Vec {0.f, panel().rowHeight()},
			b.position + Vec {b.size.x, panel().rowHeight()}
		};
	}

	Container::bounds(b);
	requestRedraw();
}

void Folder::draw(vk::CommandBuffer cb) const {
	Container::ContainerWidget::draw(cb);
	Container::bindScissor(cb);
	panel().paints().folderLine.bind(cb);
	bottomLine_.stroke(cb);
}

void Folder::open(bool o) {
	if(o == open_) {
		return;
	}

	dlg_assert(!widgets_.empty() && widgets_.front().get() == toggleButton_);
	auto before = toggleButton_->hidden();
	Container::open(o);
	container().relayout();
	toggleButton_->hide(before); // Container::open changes it
}

void Folder::hide(bool h) {
	ContainerWidget::hide(h);
	bottomLine_.disable(h);

	// to show it even when we are closed
	toggleButton_->hide(h);
	requestRedraw();
}

bool Folder::hidden() const {
	return toggleButton_->hidden();
}

Container& Folder::container() const {
	dlg_assert(dynamic_cast<Container*>(Widget::parent()));
	return *static_cast<Container*>(Widget::parent());
}

Rect2f Folder::nextBounds() const {
	auto b = Container::nextBounds();
	b.position.x = position().x + folderOffset;
	b.size.x -= folderOffset;
	return b;
}

void Folder::height(float h) {
	Container::height(h);
	container().relayout();
}

// Controller
Controller::Controller(Container& p, std::string_view name)
		: ContainerWidget(p.gui(), &p) {
	name_ = {context(), name, gui().font(), {}};
	classifier_ = {context(), {}, {false, 3.f}};
	bg_ = {context(), {}, {}, {true, 0.f}};
	bottomLine_ = {context(), {}, {false, lineHeight}};
}

void Controller::reset(const Rect2f& bounds,
		std::optional<std::string_view> oname) {
	auto bc = !(bounds == this->bounds());
	if(!bc && !oname) {
		return;
	}

	auto& font = gui().font();
	auto pos = bounds.position;
	auto size = bounds.size;
	dlg_assert(size.x != autoSize && size.y != autoSize);

	// change
	auto ny = (size.y - font.height()) / 2;
	auto nameOff = Vec {classifierWidth + std::max(ny, classifierWidth), ny};
	auto nc = name_.change();
	nc->font = &font;
	nc->position = pos + nameOff;
	if(oname) {
		nc->utf8(*oname);
	}

	if(bc) {
		auto start = Vec {classifierWidth / 2.f, 0.f};
		auto end = Vec {start.x, size.y};
		auto cc = classifier_.change();
		cc->points = {pos + start, pos + end};

		start = Vec {0.f, size.y};
		end = Vec {size.x, size.y};
		auto blc = bottomLine_.change();
		blc->points = {pos + start, pos + end};
		blc->drawMode = {false, lineHeight};

		auto bc = bg_.change();
		bc->size = size;
		bc->position = pos;

		ContainerWidget::bounds(bounds);
	}

	requestRedraw();
}


void Controller::draw(vk::CommandBuffer cb) const {
	ContainerWidget::bindScissor(cb);

	bgPaint().bind(cb);
	bg_.fill(cb);

	classPaint().bind(cb);
	classifier_.stroke(cb);

	panel().paints().name.bind(cb);
	name_.draw(cb);

	panel().paints().line.bind(cb);
	bottomLine_.stroke(cb);

	ContainerWidget::draw(cb);
}

const rvg::Paint& Controller::bgPaint() const {
	return panel().paints().bg;
}

void Controller::hide(bool hide) {
	ContainerWidget::hide(hide);
	classifier_.disable(hide);
	bottomLine_.disable(hide);
	name_.disable(hide);
	bg_.disable(hide);
	requestRedraw();
}

bool Controller::hidden() const {
	return name_.disabled();
}

void Controller::bounds(const Rect2f& bounds) {
	reset(bounds);
}

void Controller::name(std::string_view name) {
	reset(bounds(), name);
}

Container& Controller::container() const {
	dlg_assert(dynamic_cast<Container*>(Widget::parent()));
	return *static_cast<Container*>(Widget::parent());
}

// Button
// NOTE: we currently reimplement full button functionality instead
// of using BasicButton. Is it possible to reuse code here in any way?
// If not deriving from BasicButton, create some class they both
// use like ButtonBehavior or something? worth it?
// also the same for checkbox
Button::Button(Container& parent, const Rect2f& b, std::string_view name) :
		Controller(parent, "") {
	bgColor_ = {context(), rvg::colorPaint(colors::bg)};
	Controller::reset(b, name);
}

const rvg::Paint& Button::classPaint() const {
	return panel().paints().buttonClass;
}

void Button::mouseOver(bool mouseOver) {
	Controller::mouseOver(mouseOver);
	hovered_ = mouseOver;
	if(mouseOver) {
		bgColor_.paint(rvg::colorPaint(colors::bgHover));
		requestRedraw();
	} else {
		bgColor_.paint(rvg::colorPaint(colors::bg));
		requestRedraw();
	}
}

Widget* Button::mouseButton(const MouseButtonEvent& ev) {
	Controller::mouseButton(ev);
	if(ev.button == MouseButton::left) {
		if(ev.pressed) {
			pressed_ = true;
			bgColor_.paint(rvg::colorPaint(colors::bgActive));
			requestRedraw();
		} else if(pressed_) {
			auto col = hovered_ ? colors::bgHover : colors::bgActive;
			bgColor_.paint(rvg::colorPaint(col));
			pressed_ = false;
			if(hovered_ && onClick) {
				onClick();
			}
			requestRedraw();
		}
	}

	return this;
}

Cursor Button::cursor() const {
	return Cursor::hand;
}

// Textfield
Textfield::Textfield(Container& c, const Rect2f& b, std::string_view name,
		std::string_view start) : Controller(c, name) {
	create<vui::Textfield>(Rect2f {}, start, panel().styles().textfield);
	bounds(b);
}

const rvg::Paint& Textfield::classPaint() const {
	return panel().paints().textClass;
}

void Textfield::bounds(const Rect2f& b) {
	auto height = panel().rowHeight() - 4;
	auto width = b.size.x - panel().nameWidth() - 2 * namePadding;
	auto tpos = position() + Vec{panel().nameWidth() + namePadding, 2};
	auto bounds = Rect2f {tpos, Vec{width, height}};

	textfield().bounds(bounds);
	Controller::bounds(b);
}

vui::Textfield& Textfield::textfield() const {
	dlg_assert(widgets_.size() == 1);
	dlg_assert(dynamic_cast<vui::Textfield*>(widgets_[0].get()));
	return *static_cast<vui::Textfield*>(widgets_[0].get());
}

// Checkbox
// See dat::Button::Button implementation code duplicattion comment
Checkbox::Checkbox(Container& c, const Rect2f& b, std::string_view name)
		: Controller(c, name) {
	bgColor_ = {context(), rvg::colorPaint(colors::bg)};
	create<vui::Checkbox>(Rect2f {}); // no custom style needed?
	bounds(b);
}

const rvg::Paint& Checkbox::classPaint() const {
	return panel().paints().checkboxClass;
}

void Checkbox::bounds(const Rect2f& b) {
	auto height = b.size.y / 2;
	auto tpos = position() + Vec{panel().nameWidth() + namePadding,
		(b.size.y - height) / 2};
	auto bounds = Rect2f {tpos, Vec{height, height}};

	checkbox().bounds(bounds);
	Controller::bounds(b);
}

vui::Checkbox& Checkbox::checkbox() const {
	dlg_assert(widgets_.size() == 1);
	dlg_assert(dynamic_cast<vui::Checkbox*>(widgets_[0].get()));
	return *static_cast<vui::Checkbox*>(widgets_[0].get());
}

void Checkbox::mouseOver(bool mouseOver) {
	Controller::mouseOver(mouseOver);
	hovered_ = mouseOver;
	if(mouseOver) {
		bgColor_.paint(rvg::colorPaint(colors::bgHover));
		requestRedraw();
	} else {
		bgColor_.paint(rvg::colorPaint(colors::bg));
		requestRedraw();
	}
}

Widget* Checkbox::mouseButton(const MouseButtonEvent& ev) {
	if(Controller::mouseButton(ev) == &checkbox()) {
		return &checkbox();
	}

	if(ev.button == MouseButton::left) {
		if(ev.pressed) {
			pressed_ = true;
			bgColor_.paint(rvg::colorPaint(colors::bgActive));
			requestRedraw();
		} else if(pressed_) {
			auto col = hovered_ ? colors::bgHover : colors::bgActive;
			bgColor_.paint(rvg::colorPaint(col));
			pressed_ = false;
			if(hovered_) {
				checkbox().toggle();
				checkbox().onToggle(checkbox());
			}
			requestRedraw();
		}
	}

	return this;
}

Cursor Checkbox::cursor() const {
	return Cursor::hand;
}

// Label
Label::Label(Container& p, const Rect2f& b, std::string_view name,
		std::string_view label) : Controller(p, name) {
	label_ = {context(), label, gui().font(), {}};
	bounds(b);
}

const rvg::Paint& Label::classPaint() const {
	return panel().paints().labelClass;
}

void Label::bounds(const Rect2f& b) {
	auto y = (b.size.y - label_.font()->height() - 1) / 2;
	auto lc = label_.change();
	lc->position = b.position + Vec2f {panel().nameWidth() + 4, y};
	Controller::bounds(b);
}

void Label::hide(bool hide) {
	label_.disable(hide);
}

void Label::label(std::string_view label) {
	auto lc = label_.change();
	lc->utf8(label);
}

void Label::draw(vk::CommandBuffer cb) const {
	Controller::draw(cb);
	panel().paints().name.bind(cb);
	label_.draw(cb);
}


/*
// Text
Textfield::Textfield(Panel& panel, Vec2f pos, std::string_view
		name, std::string_view start) : Controller(panel, pos, name) {

	auto height = panel.rowHeight() - 5;
	auto width = size().x - panel.nameWidth() - 8;
	auto tpos = position() + Vec{panel.nameWidth() + 4, 2};
	auto bounds = Rect2f {tpos, Vec{width, height}};
	textfield_.emplace(panel.gui(), bounds, start, panel.styles().textfield);
}

void Textfield::size(Vec2f) {
	// TODO
}

const rvg::Paint& Textfield::classPaint() const {
	return panel().paints().textClass;
}

void Textfield::nameWidth(float width) {
	auto pos = textfield_->position();
	textfield_->position({position().x + width, pos.y});
}

void Textfield::hide(bool hide) {
	textfield_->hide(hide);
	Controller::hide(hide);
}

void Textfield::draw(vk::CommandBuffer cb) const {
	Controller::draw(cb);
	textfield_->draw(cb);
}

void Textfield::refreshTransform() {
	textfield_->refreshTransform();
	Controller::refreshTransform();
}

Widget* Textfield::key(const KeyEvent& ev) {
	if(focus_) {
		return textfield_->key(ev);
	}

	return nullptr;
}

Widget* Textfield::textInput(const TextInputEvent& ev) {
	if(focus_) {
		return textfield_->textInput(ev);
	}

	return nullptr;
}

Widget* Textfield::mouseButton(const MouseButtonEvent& ev) {
	if(mouseOver_) {
		if(!focus_) {
			textfield_->focus(true);
			focus_ = true;
		}

		return textfield_->mouseButton(ev);
	} else if(focus_) {
		textfield_->focus(false);
		focus_ = false;
	}

	return this;
}

Widget* Textfield::mouseMove(const MouseMoveEvent& ev) {
	if(textfield_->contains(ev.position)) {
		if(!mouseOver_) {
			textfield_->mouseOver(true);
			mouseOver_ = true;
		}

		textfield_->mouseMove(ev);
	} else if(mouseOver_) {
		mouseOver_ = false;
		textfield_->mouseOver(false);
	}

	return this;
}

void Textfield::mouseOver(bool gained) {
	if(!gained && mouseOver_) {
		mouseOver_ = false;
		textfield_->mouseOver(false);
	}
}

void Textfield::focus(bool gained) {
	if(!gained && focus_) {
		focus_ = false;
		textfield_->focus(false);
	}
}

void Textfield::position(Vec2f position) {
	auto off = textfield_->position() - this->position();
	textfield_->position(position + off);
	Controller::position(position);
	textfield_->intersectScissor(scissor());
}

void Textfield::intersectScissor(const Rect2f& scissor) {
	Controller::intersectScissor(scissor);
	textfield_->intersectScissor(this->scissor());
}

// Label
Label::Label(Panel& panel, Vec2f pos, std::string_view name,
		std::string_view label) : Controller(panel, pos, name) {

	auto y = ((panel.rowHeight() - gui().font().height() - 1) / 2);
	label_ = {context(), label, gui().font(), {panel.nameWidth() + 4, y}};
}

const rvg::Paint& Label::classPaint() const {
	return panel().paints().labelClass;
}

void Label::label(std::string_view label) {
	label_.change()->utf8(label);
}

void Label::hide(bool hide) {
	label_.disable(hide);
	Controller::hide(hide);
}

void Label::draw(vk::CommandBuffer cb) const {
	Controller::draw(cb);

	panel_.paints().name.bind(cb);
	label_.draw(cb);
}

// Checkbox
Checkbox::Checkbox(Panel& panel, Vec2f pos, std::string_view name) :
		Controller(panel, pos, name) {

	auto size = panel.rowHeight() / 2;
	auto off = panel.rowHeight() / 4;
	auto cpos = position() + Vec{panel.nameWidth() + 4, off};
	auto bounds = Rect2f{cpos, Vec{size, size}};
	checkbox_.emplace(panel.gui(), bounds);
	bgColor_ = {context(), rvg::colorPaint(colors::bg)};
}

void Checkbox::mouseOver(bool gained) {
	if(gained) {
		gui().listener().cursor(Cursor::hand);
		bgColor_.paint(rvg::colorPaint(colors::bgHover));
	} else {
		bgColor_.paint(rvg::colorPaint(colors::bg));

		// TODO
		gui().listener().cursor(Cursor::pointer);
	}
}

const rvg::Paint& Checkbox::classPaint() const {
	return panel().paints().checkboxClass;
}

void Checkbox::hide(bool hide) {
	checkbox_->hide(hide);
	Controller::hide(hide);
}

Widget* Checkbox::mouseButton(const MouseButtonEvent& ev) {
	if(ev.pressed && ev.button == MouseButton::left) {
		checkbox_->mouseButton(ev);
	}

	return this;
}

void Checkbox::draw(vk::CommandBuffer cb) const {
	Controller::draw(cb);
	checkbox_->draw(cb);
}
void Checkbox::refreshTransform() {
	Controller::refreshTransform();
	checkbox_->refreshTransform();
}

void Checkbox::position(Vec2f position) {
	auto off = checkbox_->position() - this->position();
	Controller::position(position);
	checkbox_->position(position + off);
	checkbox_->intersectScissor(scissor());
}

void Checkbox::intersectScissor(const Rect2f& scissor) {
	Controller::intersectScissor(scissor);
	checkbox_->intersectScissor(this->scissor());
}

// Panel
Panel::Panel(Gui& gui, const nytl::Rect2f& bounds, float rowHeight) :
		ContainerWidget(gui, bounds), rowHeight_(rowHeight) {

	if(rowHeight_ == autoSize) {
		rowHeight_ = gui.font().height() + 10;
	}

	auto size = bounds.size;
	if(size.x == autoSize) {
		size.x = 350;
	}

	if(size.y == autoSize) {
		size.y = rowHeight_;
	}

	if(size != bounds.size) {
		Widget::size(size);
	}

	// paints
	using namespace colors;
	paints_.name = {context(), rvg::colorPaint(name)};
	paints_.line = {context(), rvg::colorPaint(line)};
	paints_.folderLine = {context(), rvg::colorPaint(folderLine)};
	paints_.buttonClass = {context(), rvg::colorPaint(button)};
	paints_.labelClass = {context(), rvg::colorPaint(label)};
	paints_.textClass = {context(), rvg::colorPaint(text)};
	paints_.rangeClass = {context(), rvg::colorPaint(range)};
	paints_.checkboxClass = {context(), rvg::colorPaint(checkbox)};

	paints_.bg = {context(), rvg::colorPaint(bg)};
	paints_.bgHover = {context(), rvg::colorPaint(bgHover)};
	paints_.bgActive = {context(), rvg::colorPaint(bgActive)};

	paints_.bgWidget = {context(), rvg::colorPaint(bgWidget)};

	// styles
	auto yoff = ((rowHeight_ - 4.f) - gui.font().height()) / 2.f;
	auto xoff = std::max(yoff * 1.5f, 2.f);
	styles_.textfield = gui.styles().textfield;
	styles_.textfield.bg = &paints_.bgWidget;
	styles_.textfield.padding = {xoff, yoff};

	styles_.button = gui.styles().basicButton;
	styles_.button.normal = {rvg::colorPaint(metaButtonBg)};
	styles_.button.hovered = {rvg::colorPaint(metaButtonBgHover)};
	styles_.button.pressed = {rvg::colorPaint(metaButtonBgPressed)};
	styles_.button.rounding = {};

	styles_.metaButton = gui.styles().labeledButton;
	styles_.metaButton.basic = &styles_.button;

	// toggle button
	auto btnBounds = Rect2f{position(), {width(), rowHeight_}};
	auto btn = std::make_unique<LabeledButton>(gui, btnBounds,
		"Toggle Controls", styles_.metaButton);
	toggleButton_ = btn.get();
	widgets_.push_back(std::move(btn));
	toggleButton_->onClick = [&](auto&) { this->toggle(); };
}

void Panel::hide(bool hide) {
	ContainerWidget::hide(hide);
}

bool Panel::hidden() const {
	return toggleButton_->hidden();
}

void Panel::size(Vec2f size) {
	// TODO
	ContainerWidget::size(size);
}

Widget& Panel::add(std::unique_ptr<Widget> ctrl) {
	dlg_assert(!widgets_.empty());

	auto& ret = *ctrl;
	widgets_.emplace(widgets_.end() - 1, std::move(ctrl));

	auto s = size();
	s.y += ret.size().y;
	Widget::size(s);

	auto p = position();
	toggleButton_->position({p.x, p.y + s.y - rowHeight_});

	// TODO: should prob not be here
	gui().rerecord();

	return ret;
}

bool Panel::remove(const Widget& w) {
	auto it = std::find_if(widgets_.begin(), widgets_.end(),
		[&](auto& ww){ return ww.get() == &w; });
	if(it == widgets_.end()) {
		return true;
	}

	// TODO! children
	// if(focus_ == &w) {
		focus_ = nullptr;
	// }

	// if(mouseOver_ == &w) {
		mouseOver_ = nullptr;
	// }

	gui().moveDestroyWidget(std::move(*it));
	widgets_.erase(it);
	refreshLayout();

	return true;
}

bool Panel::disable(const Widget& w) {
	auto it = std::find_if(widgets_.begin(), widgets_.end(),
		[&](auto& ww){ return ww.get() == &w; });
	if(it == widgets_.end()) {
		return false;
	}

	// TODO! children
	// if(focus_ == &w) {
		focus_ = nullptr;
	// }

	// if(mouseOver_ == &w) {
		mouseOver_ = nullptr;
	// }

	disabled_.push_back(std::move(*it));
	widgets_.erase(it);
	refreshLayout();
	gui().rerecord(); // TODO: might be possible with hide

	return true;
}

bool Panel::enable(const Widget& w) {
	auto it = std::find_if(disabled_.begin(), disabled_.end(),
		[&](auto& ww){ return ww.get() == &w; });
	if(it == disabled_.end()) {
		return false;
	}

	widgets_.emplace(widgets_.end() - 1, std::move(*it));
	disabled_.erase(it);
	refreshLayout();
	gui().rerecord();

	return true;
}

void Panel::open(bool open) {
	if(open_ == open) {
		return;
	}

	open_ = open;
	auto size = this->size();

	if(open_) {
		size.y = 0.f;
		for(auto& w : widgets_) {
			size.y += w->size().y;
		}
	} else {
		size.y = rowHeight_;
	}

	for(auto& w :widgets_) {
		if(w.get() != toggleButton_) {
			w->hide(!open);
		}
	}

	auto y = size.y - rowHeight_;
	toggleButton_->position({position().x, y});
	Widget::size(size);
}

void Panel::refreshLayout() {
	auto y = position().y;
	for(auto& w : widgets_) {
		w->position({w->position().x, y});

		// NOTE: this is a hack. Should be done better
		auto folder = dynamic_cast<Folder*>(w.get());
		if(folder) {
			folder->refreshLayout();
		}
		// /hack

		y += w->size().y;
	}

	if(y != size().y) {
		Widget::size({size().x, y - position().y});
	}
}

// Folder
Folder::Folder(Panel& panel, Vec2f pos, std::string_view name) :
	ContainerWidget(panel.gui(), {pos,
			{panel.position().x + panel.width() - pos.x, 0}}),
		panel_(panel) {

	auto btnBounds = Rect2f{position(), {size().x, panel.rowHeight()}};
	auto btn = std::make_unique<LabeledButton>(gui(), btnBounds, name,
		panel_.styles().metaButton);
	button_ = btn.get();
	add(std::move(btn));
	button_->onClick = [&](auto&) { this->toggle(); };

	auto points = {{0.f, size().y}, size()};
	bottomLine_ = {context(), points, {false, lineHeight}};
}

void Folder::hide(bool hide) {
	bottomLine_.disable(hide);
	if(!hide && !open_) {
		button_->hide(false);
	} else {
		ContainerWidget::hide(hide);
	}
}

bool Folder::hidden() const {
	return button_->hidden();
}

Widget& Folder::add(std::unique_ptr<Widget> widget) {
	auto& ret = *widget;
	widgets_.emplace_back(std::move(widget));

	auto s = size();
	s.y += ret.size().y;
	Widget::size(s);

	// TODO will call this->refreshLayout and size again
	panel().refreshLayout();

	return ret;
}

void Folder::open(bool open) {
	if(open_ == open) {
		return;
	}

	open_ = open;
	auto size = this->size();

	if(open_) {
		size.y = 0.f;
		for(auto& w : widgets_) {
			size.y += w->size().y;
		}
	} else {
		size.y = panel().rowHeight();
	}

	for(auto& w : widgets_) {
		if(w.get() != button_) {
			w->hide(!open);
		}
	}

	// TODO: somewhat redundant
	Widget::size(size);
	panel().refreshLayout();
}

void Folder::refreshLayout() {
	auto y = position().y;
	for(auto& w : widgets_) {
		w->position({w->position().x, y});

		// NOTE: this is a hack. Should be done better
		auto folder = dynamic_cast<Folder*>(w.get());
		if(folder) {
			folder->refreshLayout();
		}
		// /hack

		y += w->size().y;
	}

	if(open_ && y != size().y) {
		Widget::size({size().x, y - position().y});
	}
}

void Folder::draw(vk::CommandBuffer cb) const {
	ContainerWidget::draw(cb);

	bindState(cb);
	panel().paints().folderLine.bind(cb);
	bottomLine_.stroke(cb);
}
*/

} // namespace vui::dat
