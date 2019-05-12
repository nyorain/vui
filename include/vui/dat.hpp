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

namespace vui::dat {

class Panel;

/// Contains only Folders and Controllers.
/// Also holds reference to its panel.
class Container : public ContainerWidget {
public:
	// TODO: createAfter/createBefore
	//   also addAfter/addBefore

	virtual const Panel& panel() const = 0;
	void relayout();

	/// (Re-)adds a widget that is currently orphaned.
	/// Must have been created outside the hierachy or previously
	/// been removed from it. The widget must be a controller or
	/// subfolder.
	Widget& add(std::unique_ptr<Widget>) override;

	/// See ContainerWidget::remove.
	/// Removed Controllers and folders must not be accessed in any way
	/// but can be readded later. Currently they have to readded on
	/// a container of the same panel.
	std::unique_ptr<Widget> remove(const Widget&) override;
	using ContainerWidget::destroy;

	virtual void hide(bool h) override;
	virtual void open(bool open);
	virtual bool open() const { return open_; }
	virtual void toggle() { open(!open_); }

	/// Creates and insert a new controller of type T.
	/// T must have a contructor (Container&, const Rect2f&, Args...).
	/// Returns a reference to the created widget.
	template<typename T, typename... Args>
	T& create(Args&&... args) {
		auto ctrl = std::make_unique<T>(*this, nextBounds(),
			std::forward<Args>(args)...);
		auto& ret = *ctrl;
		add(std::move(ctrl));
		return ret;
	}

	bool moveBefore(const Widget& move, const Widget& before,
		bool exactly = false) override;
	bool moveAfter(const Widget& move, const Widget& after,
		bool exactly = false) override;

protected:
	using ContainerWidget::ContainerWidget;
	using ContainerWidget::bounds;
	using ContainerWidget::size;

	virtual Rect2f nextBounds() const; // return bounds of new controller
	virtual void height(float delta); // called when controller added/removed
	virtual float closedHeight() const;

protected:
	bool open_ {true};
	bool relayouting_ {};
};

/// Root of a dat gui tree.
class Panel : public Container {
public:
	/// When rowHeight is autoSize, will automatically choose a (fixed!)
	/// row height based on the used font size.
	/// nameWidth is the uniform width of the naming label in controllers.
	/// bounds.size.y should be set to autoSize.
	Panel(Gui&, ContainerWidget*, const Vec2f& pos, float width,
		float nameWidth = autoSize, float rowHeight = autoSize);

	Widget& add(std::unique_ptr<Widget>) override;
	const Panel& panel() const override { return *this; }

	void open(bool) override;
	void hide(bool) override;
	bool hidden() const override;

	float rowHeight() const { return rowHeight_; }
	float nameWidth() const { return nameWidth_; }
	float fontHeight() const { return std::floor(rowHeight() * 0.6f); }

	// for controllers
	const auto& paints() const { return paints_; }
	const auto& styles() const { return styles_; }

protected:
	Rect2f nextBounds() const override;
	float closedHeight() const override { return toggleButton_->size().y; }

protected:
	float rowHeight_ {};
	float nameWidth_ {100.f};
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
};

class Folder : public Container {
public:
	Folder(Container& parent, const Rect2f& bounds, std::string_view name);

	void bounds(const Rect2f&) override;
	void open(bool) override;
	void hide(bool) override;
	bool hidden() const override;
	void draw(vk::CommandBuffer cb) const override;

	Container& container() const;
	const Panel& panel() const override { return container().panel(); }

protected:
	Rect2f nextBounds() const override;
	void height(float) override;

protected:
	LabeledButton* toggleButton_;
	rvg::Shape bottomLine_;
};

class Controller : public ContainerWidget {
public:
	virtual const rvg::Paint& classPaint() const = 0;
	virtual const rvg::Paint& bgPaint() const;

	void name(std::string_view name);
	void reset(const Rect2f&, std::optional<std::string_view> name = {});

	void hide(bool) override;
	bool hidden() const override;
	void draw(vk::CommandBuffer) const override;
	void bounds(const Rect2f&) override;
	using Widget::bounds;

	Container& container() const;
	const Panel& panel() const { return container().panel(); }

protected:
	Controller(Container&, std::string_view name);

	rvg::RectShape bg_;
	rvg::Shape classifier_;
	rvg::Shape bottomLine_;
	rvg::Text name_;
};

class Button : public Controller {
public:
	std::function<void()> onClick;

public:
	Button(Container&, const Rect2f&, std::string_view name);

	const rvg::Paint& classPaint() const override;
	const rvg::Paint& bgPaint() const override { return bgColor_; }

	void mouseOver(bool) override;
	Widget* mouseButton(const MouseButtonEvent&) override;

protected:
	Cursor cursor() const override;

protected:
	rvg::Paint bgColor_;
	bool hovered_ {};
	bool pressed_ {};
};

class Textfield : public Controller {
public:
	Textfield(Container&, const Rect2f&, std::string_view name,
		std::string_view start = "");

	const rvg::Paint& classPaint() const override;
	using Controller::bounds;

	vui::Textfield& textfield() const;

protected:
	void bounds(const Rect2f&) override;
};

class Checkbox : public Controller {
public:
	Checkbox(Container&, const Rect2f&, std::string_view name);

	const Paint& bgPaint() const override { return bgColor_; }
	const rvg::Paint& classPaint() const override;
	using Controller::bounds;

	void mouseOver(bool) override;
	Widget* mouseButton(const MouseButtonEvent&) override;
	vui::Checkbox& checkbox() const;

protected:
	void bounds(const Rect2f&) override;
	Cursor cursor() const override;

protected:
	rvg::Paint bgColor_;
	bool hovered_ {};
	bool pressed_ {};
};

class Label : public Controller {
public:
	Label(Container&, const Rect2f&, std::string_view name,
		std::string_view label);

	const rvg::Paint& classPaint() const override;
	void bounds(const Rect2f&) override;
	void hide(bool hide) override;
	void label(std::string_view label);
	void draw(vk::CommandBuffer) const override;

protected:
	rvg::Text label_;
};

/*

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
*/

} // namespace vui::dat
