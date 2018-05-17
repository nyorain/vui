#include <vui/style.hpp>

namespace vui {
namespace colors {

const auto text = rvg::Color {255u, 255u, 255u};
const auto bg = rvg::Color {10u, 10u, 10u};
const auto bgAlpha = rvg::Color {40u, 40u, 40u, 200u};
const auto bgHover = rvg::Color {5u, 5u, 5u};
const auto bgActive = rvg::Color {2u, 2u, 2u};
const auto accent = rvg::Color {150u, 230u, 200};

} // namespace colors

DefaultStyles::DefaultStyles(rvg::Context& ctx) {
	auto textData = rvg::colorPaint(colors::text);
	auto bgData = rvg::colorPaint(colors::bg);
	auto bgAlphaData = rvg::colorPaint(colors::bgAlpha);
	auto bgHoverData = rvg::colorPaint(colors::bgHover);
	auto bgActiveData = rvg::colorPaint(colors::bgActive);
	auto accentData = rvg::colorPaint(colors::accent);

	paints_.text = {ctx, textData};
	paints_.bg = {ctx, bgData};
	paints_.bgHover = {ctx, bgHoverData};
	paints_.bgActive = {ctx, bgActiveData};
	paints_.accent = {ctx, accentData};
	paints_.bgAlpha = {ctx, bgAlphaData};

	styles_.basicButton.normal.bg = bgData;
	styles_.basicButton.hovered.bg = bgHoverData;
	styles_.basicButton.pressed.bg = bgActiveData;
	styles_.labeledButton.label = &paints_.text;

	styles_.hint.bg = &paints_.bg;
	styles_.hint.text = &paints_.text;

	styles_.window.bg = &paints_.bgAlpha;
	styles_.window.outerPadding = {10.f, 10.f};
	styles_.window.innerPadding = 5.f;

	styles_.pane.bg = &paints_.bg;
	styles_.colorPicker.marker = &paints_.bg;

	styles_.checkbox.bg = &paints_.bgAlpha;
	styles_.checkbox.fg = &paints_.accent;

	styles_.textfield.bg = &paints_.bg;
	styles_.textfield.text = &paints_.text;
	styles_.textfield.selected = &paints_.accent;
	styles_.textfield.cursor = &paints_.text;
}

} // namespace vui
