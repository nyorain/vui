#pragma once

#include "fwd.hpp"
#include "container.hpp"

// TODO: outerPadding needed? default it to 0?

namespace vui {

/// Layout that just puts each widget in its own row.
class RowLayout : public ContainerWidget {
public:
	RowLayout(ContainerWidget& parent, Vec2f outerPadding,
		float innerPadding, float childWidth = autoSize);

	Widget& add(std::unique_ptr<Widget>);
	Widget& add(unsigned pos, std::unique_ptr<Widget>);

	template<typename T, typename... Args>
	T& create(unsigned pos, Args&&... args) {
		auto ctrl = std::make_unique<T>(*this, boundsOf(pos),
			std::forward<Args>(args)...);
		auto& ret = *ctrl;
		add(std::move(ctrl));
		return ret;
	}

	template<typename T, typename... Args>
	T& create(Args&&... args) {
		return create(children().size(), std::forward<Args>(args)...);
	}

	using ContainerWidget::moveBefore;
	using ContainerWidget::moveAfter;
	bool move(Widget&, unsigned pos);
	std::optional<unsigned> indexOf(Widget&) const;

	/// Return the bounds the widget at the given position should have.
	/// Does not query its actual size, returns autoSize without static child
	/// width.
	Rect2f boundsOf(unsigned pos) const;

protected:
	Vec2f outerPadding_;
	float innerPadding_;
	float childWidth_; // might be autoSize
};

/// Layout that puts each widget in its own column.
class ColumnLayout : public ContainerWidget {
public:
	ColumnLayout(ContainerWidget& parent, Vec2f outerPadding,
		float innerPadding, float childHeight = autoSize);

	Widget& add(std::unique_ptr<Widget>);
	Widget& add(unsigned pos, std::unique_ptr<Widget>);

	template<typename T, typename... Args>
	T& create(unsigned pos, Args&&... args) {
		auto ctrl = std::make_unique<T>(*this, boundsOf(pos),
			std::forward<Args>(args)...);
		auto& ret = *ctrl;
		add(std::move(ctrl));
		return ret;
	}

	template<typename T, typename... Args>
	T& create(Args&&... args) {
		return create(children().size(), std::forward<Args>(args)...);
	}

	using ContainerWidget::moveBefore;
	using ContainerWidget::moveAfter;
	bool move(Widget&, unsigned pos);
	std::optional<unsigned> indexOf(Widget&) const;

	/// Return the bounds the widget at the given position should have.
	/// Does not query its actual size, returns autoSize without static child
	/// height.
	Rect2f boundsOf(unsigned pos) const;

protected:
	Vec2f outerPadding_;
	float innerPadding_;
	float childHeight_; // might be autoSize
};


/// Layout that has a fixed AxB table/matrix in which it puts widgets.
/// Width of columns and heights of rows can be variable.
class TableLayout : public ContainerWidget {
public:
	TableLayout(ContainerWidget& parent, Vec2f outerPadding,
		float innerPadding, Vec2f childSize = {autoSize, autoSize});

	/// Return the bounds the widget at the given position should have.
	/// Does not query its actual size, returns autoSize without static child
	/// size.
	Rect2f boundsOf(Vec2ui pos) const;

protected:
	Vec2f outerPadding_;
	float innerPadding_;
	Vec2f childSize_;
};

} // namespace vui
