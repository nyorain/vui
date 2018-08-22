// TODO: resizable? close, minimize buttons?
/// Extends Pane by a top bar that allows to move the window.
class Window : public Pane {
public:
	Window(Gui& gui, ContainerWidget* parent, const Rect2f& bounds,
		std::unique_ptr<Widget> = {});
	Window(Gui& gui, ContainerWidget* parent, const Rect2f& bounds,
		const PaneStyle& style, std::unique_ptr<Widget> = {});

	void hide(bool hide) override;
	void draw(vk::CommandBuffer) const override;
	void reset(const PaneStyle& style, const Rect2f& bounds,
		bool forceReload = false,
		std::optional<std::unique_ptr<Widget>> = {}) override;

	const auto& style() const { return *style_; }

protected:
	Window(Gui& gui, ContainerWidget*);
	Rect2f childBounds() const override;

protected:
	const WindowStyle* sytle_;
	RectShape topbar_;
};


struct WindowStyle {
	PaneStyle* pane;
	rvg::Paint* topbar;
};

