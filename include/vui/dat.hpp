#pragma once

/// Simple widgets classes modeled after googles dat.gui
/// https://github.com/dataarts/dat.gui/blob/master/API.md

#include <vui/widget.hpp>
#include <vui/textfield.hpp>
#include <vui/checkbox.hpp>
#include <vui/container.hpp>
#include <vui/button.hpp>

#include <rvg/context.hpp>
#include <rvg/text.hpp>
#include <rvg/shapes.hpp>

// NOTES
// - make color classifiers optional. dat.gui has them but they
//   are not really needed.
// - currently code duplication between folder and panel. Furthermore
//   ugly refreshLayout hack
// - missing intersectScissors. Needed here at all? test it
// - reenable todo in ContainerWidget::position. Breaks nested folders
// - still some clipping issues (probably related to scissor)

namespace vui::dat {

class Panel;

class Controller : public Widget {
public:
	Controller(Panel&, Vec2f, std::string_view name);

	virtual const rvg::Paint& classPaint() const = 0;
	virtual const rvg::Paint& bgPaint() const;

	virtual void nameWidth(float) {}
	auto& panel() const { return panel_; }

	void hide(bool) override;
	bool hidden() const override;
	void draw(vk::CommandBuffer) const override;
	void size(Vec2f) override;
	using Widget::size;

protected:
	Panel& panel_;
	rvg::RectShape bg_;
	rvg::Shape classifier_;
	rvg::Shape bottomLine_;
	rvg::Text name_;
};

class Button : public Controller {
public:
	std::function<void()> onClick;

public:
	Button(Panel&, Vec2f, std::string_view name);

	const rvg::Paint& classPaint() const override;
	const rvg::Paint& bgPaint() const override { return bgColor_; }

	void mouseOver(bool) override;
	Widget* mouseButton(const MouseButtonEvent&) override;

protected:
	rvg::Paint bgColor_;
	bool hovered_ {};
	bool pressed_ {};
};


class Textfield : public Controller {
public:
	Textfield(Panel&, Vec2f, std::string_view name,
		std::string_view start = "");

	void size(Vec2f) override;
	using Widget::size;

	void position(Vec2f position) override;
	void intersectScissor(const Rect2f& scissor) override;
	using Controller::position;

	const rvg::Paint& classPaint() const override;
	void nameWidth(float) override;
	void hide(bool hide) override;

	Widget* key(const KeyEvent&) override;
	Widget* textInput(const TextInputEvent&) override;
	Widget* mouseButton(const MouseButtonEvent&) override;
	Widget* mouseMove(const MouseMoveEvent&) override;
	void mouseOver(bool) override;
	void focus(bool) override;

	void draw(vk::CommandBuffer) const override;
	void refreshTransform() override;

	auto& textfield() const { return *textfield_; }
	auto& textfield() { return *textfield_; }

protected:
	std::optional<vui::Textfield> textfield_;
	bool focus_ {};
	bool mouseOver_ {};
};

class Label : public Controller {
public:
	Label(Panel&, Vec2f, std::string_view name,
		std::string_view label);

	const rvg::Paint& classPaint() const override;
	void hide(bool hide) override;
	void label(std::string_view label);
	void draw(vk::CommandBuffer) const override;

protected:
	rvg::Text label_;
};

class Checkbox : public Controller {
public:
	Checkbox(Panel&, Vec2f, std::string_view name);

	void hide(bool hide) override;
	const rvg::Paint& classPaint() const override;
	Widget* mouseButton(const MouseButtonEvent&) override;
	void draw(vk::CommandBuffer) const override;
	void mouseOver(bool) override;
	const rvg::Paint& bgPaint() const override { return bgColor_; }

	void refreshTransform() override;
	void position(Vec2f position) override;
	void intersectScissor(const Rect2f& scissor) override;
	using Controller::position;

	auto& checkbox() const { return *checkbox_; }
	auto& checkbox() { return *checkbox_; }

protected:
	std::optional<vui::Checkbox> checkbox_;
	rvg::Paint bgColor_;
};

class Panel : public ContainerWidget {
public:
	Panel(Gui& gui, const nytl::Rect2f& bounds, float rowHeight = autoSize);

	template<typename T, typename... Args>
	T& create(Args&&... args) {
		auto pos = position();
		pos.y += size().y - rowHeight();
		auto ctrl = std::make_unique<T>(*this, pos,
			std::forward<Args>(args)...);
		auto& ret = *ctrl;
		add(std::move(ctrl));
		return ret;
	}

	void hide(bool) override;
	bool hidden() const override;
	void size(Vec2f) override;

	using Widget::size;

	float rowHeight() const { return rowHeight_; }
	float width() const { return size().x; }

	float nameWidth() const { return nameWidth_; }
	void nameWidth(float);

	void open(bool open);
	bool open() const { return open_; }
	void toggle() { open(!open_); }

	void refreshLayout();

	bool remove(const Widget&);

	bool disable(const Widget&);
	bool enable(const Widget&);

	// for controllers
	const auto& paints() const { return paints_; }
	const auto& styles() const { return styles_; }

protected:
	Widget& add(std::unique_ptr<Widget>) override;

protected:
	float rowHeight_ {};
	float nameWidth_ {100.f};
	bool open_ {true};
	LabeledButton* toggleButton_ {};

	struct {
		rvg::Paint bg;
		rvg::Paint bgHover;
		rvg::Paint bgActive;

		rvg::Paint name;
		rvg::Paint line;
		rvg::Paint folderLine;
		rvg::Paint buttonClass;
		rvg::Paint textClass;
		rvg::Paint labelClass;
		rvg::Paint rangeClass;
		rvg::Paint checkboxClass;

		rvg::Paint bgWidget;
	} paints_;

	struct {
		BasicButtonStyle button;
		LabeledButtonStyle metaButton;
		TextfieldStyle textfield;
	} styles_;

	std::vector<std::unique_ptr<Widget>> disabled_;
};

class Folder : public ContainerWidget {
public:
	static constexpr auto xoff = 5.f;

public:
	Folder(Panel& panel, Vec2f, std::string_view name);

	void open(bool);
	bool open() const { return open_; }
	void toggle() { open(!open_); }

	void hide(bool) override;
	bool hidden() const override;
	void size(Vec2f) override {} // TODO
	using Widget::size;

	void draw(vk::CommandBuffer cb) const override;

	template<typename T, typename... Args>
	T& create(Args&&... args) {
		auto pos = position();
		pos.x += xoff;
		pos.y += size().y;
		auto ctrl = std::make_unique<T>(panel_, pos,
			std::forward<Args>(args)...);
		auto& ret = *ctrl;
		add(std::move(ctrl));
		return ret;
	}

	void refreshLayout();
	auto& panel() const { return panel_; }

protected:
	Widget& add(std::unique_ptr<Widget> widget) override;

protected:
	Panel& panel_;
	LabeledButton* button_;
	rvg::Shape bottomLine_;
	bool open_ {true};
};

// TODO
class RangeController : public Controller {
protected:
	rvg::RectShape bg_;
	rvg::RectShape fg_;
	rvg::Paint mainPaint_;

	rvg::RectShape valueBg_;
	rvg::Text value_;
	rvg::Paint valuePaint_;
};

} // namespace vui::dat
