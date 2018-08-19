// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include "render.hpp"
#include "window.hpp"

#include "vui/gui.hpp"
#include "vui/button.hpp"
// #include "vui/window.hpp"
#include "vui/colorPicker.hpp"
#include "vui/textfield.hpp"
// #include "vui/checkbox.hpp"
#include "vui/dat.hpp"

#include <rvg/context.hpp>
#include <rvg/shapes.hpp>
#include <rvg/paint.hpp>
#include <rvg/state.hpp>
#include <rvg/font.hpp>
#include <rvg/text.hpp>

#include <katachi/path.hpp>
#include <katachi/svg.hpp>

#include <ny/backend.hpp>
#include <ny/appContext.hpp>
#include <ny/asyncRequest.hpp>
#include <ny/key.hpp>
#include <ny/mouseButton.hpp>
#include <ny/event.hpp>
#include <ny/appContext.hpp>
#include <ny/windowContext.hpp>
#include <ny/cursor.hpp>
#include <ny/dataExchange.hpp>

#include <vpp/instance.hpp>
#include <vpp/debug.hpp>
#include <vpp/formats.hpp>
#include <vpp/physicalDevice.hpp>

#include <nytl/vecOps.hpp>
#include <nytl/matOps.hpp>

#include <dlg/dlg.hpp>

#include <chrono>
#include <array>

// static const std::string baseResPath = "../";
static const std::string baseResPath = "../subprojects/vui/";

// using namespace vui;
using namespace nytl::vec::operators;
using namespace nytl::vec::cw::operators;

// settings
constexpr auto appName = "rvg-example";
constexpr auto engineName = "vpp,rvg";
constexpr auto useValidation = true;
constexpr auto startMsaa = vk::SampleCountBits::e1;
constexpr auto layerName = "VK_LAYER_LUNARG_standard_validation";
constexpr auto printFrames = true;
constexpr auto vsync = true;
constexpr auto clearColor = std::array<float, 4>{{0.f, 0.f, 0.f, 1.f}};

// TODO: move to nytl
template<typename T>
void scale(nytl::Mat4<T>& mat, nytl::Vec3<T> fac) {
	for(auto i = 0; i < 3; ++i) {
		mat[i][i] *= fac[i];
	}
}

template<typename T>
void translate(nytl::Mat4<T>& mat, nytl::Vec3<T> move) {
	for(auto i = 0; i < 3; ++i) {
		mat[i][3] += move[i];
	}
}

class TextDataSource : public ny::DataSource {
public:
	std::vector<ny::DataFormat> formats() const override {
		return {ny::DataFormat::text};
	}

	std::any data(const ny::DataFormat& format) const override {
		if(format != ny::DataFormat::text) {
			return {};
		}

		return {text};
	}

	std::string text;
};

class GuiListener : public vui::GuiListener {
public:
	void copy(std::string_view view) override {
		auto source = std::make_unique<TextDataSource>();
		source->text = view;
		ac->clipboard(std::move(source));
	}

	void cursor(vui::Cursor cursor) override {
		if(cursor == currentCursor) {
			return;
		}

		currentCursor = cursor;
		auto c = static_cast<ny::CursorType>(cursor);
		wc->cursor({c});
	}

	bool pasteRequest(const vui::Widget& widget) override {
		auto& gui = widget.gui();
		auto offer = ac->clipboard();
		if(!offer) { // nothing in clipboard
			return false;
		}

		auto req = offer->data(ny::DataFormat::text);
		if(req->ready()) {
			std::any any = req.get();
			auto* pstr = std::any_cast<std::string>(&any);
			if(!pstr) {
				return false;
			}

			gui.paste(widget, *pstr);
			return true;
		}

		req->callback([this, &gui](auto& req){ dataHandler(gui, req); });
		reqs.push_back({std::move(req), &widget});
		return true;
	}

	void dataHandler(vui::Gui& gui, ny::AsyncRequest<std::any>& req) {
		auto it = std::find_if(reqs.begin(), reqs.end(),
			[&](auto& r) { return r.request.get() == &req; });
		if(it == reqs.end()) {
			dlg_error("dataHandler: invalid request");
			return;
		}

		std::any any = req.get();
		auto* pstr = std::any_cast<std::string>(&any);
		auto str = pstr ? *pstr : "";
		gui.paste(*it->widget, str);
		reqs.erase(it);
	}

	struct Request {
		ny::DataOffer::DataRequest request;
		const vui::Widget* widget;
	};

	std::vector<Request> reqs;
	ny::AppContext* ac;
	ny::WindowContext* wc;
	vui::Cursor currentCursor {};
};

int main() {
	// - initialization -
	auto& backend = ny::Backend::choose();
	if(!backend.vulkan()) {
		throw std::runtime_error("ny backend has no vulkan support!");
	}

	auto appContext = backend.createAppContext();

	// vulkan init: instance
	auto iniExtensions = appContext->vulkanExtensions();
	iniExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	vk::ApplicationInfo appInfo (appName, 1, engineName, 1, VK_API_VERSION_1_0);
	vk::InstanceCreateInfo instanceInfo;
	instanceInfo.pApplicationInfo = &appInfo;

	instanceInfo.enabledExtensionCount = iniExtensions.size();
	instanceInfo.ppEnabledExtensionNames = iniExtensions.data();

	if(useValidation) {
		auto layers = {
			layerName,
			// "VK_LAYER_RENDERDOC_Capture",
		};

		instanceInfo.enabledLayerCount = layers.size();
		instanceInfo.ppEnabledLayerNames = layers.begin();
	}

	vpp::Instance instance {};
	try {
		instance = {instanceInfo};
		if(!instance.vkInstance()) {
			throw std::runtime_error("vkCreateInstance returned a nullptr");
		}
	} catch(const std::exception& error) {
		dlg_error("Vulkan instance creation failed: {}", error.what());
		dlg_error("\tYour system may not support vulkan");
		dlg_error("\tThis application requires vulkan to work");
		throw;
	}

	// debug callback
	std::unique_ptr<vpp::DebugCallback> debugCallback;
	if(useValidation) {
		debugCallback = std::make_unique<vpp::DebugCallback>(instance);
	}

	// init ny window
	MainWindow window(*appContext, instance);
	auto vkSurf = window.vkSurface();

	// create device
	// enable some extra features
	float priorities[1] = {0.0};

	auto phdevs = vk::enumeratePhysicalDevices(instance);
	auto phdev = vpp::choose(phdevs, instance, vkSurf);

	auto queueFlags = vk::QueueBits::compute | vk::QueueBits::graphics;
	int queueFam = vpp::findQueueFamily(phdev, instance, vkSurf, queueFlags);

	vk::DeviceCreateInfo devInfo;
	vk::DeviceQueueCreateInfo queueInfo({}, queueFam, 1, priorities);

	auto exts = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	devInfo.pQueueCreateInfos = &queueInfo;
	devInfo.queueCreateInfoCount = 1u;
	devInfo.ppEnabledExtensionNames = exts.begin();
	devInfo.enabledExtensionCount = 1u;

	auto features = vk::PhysicalDeviceFeatures {};
	features.shaderClipDistance = true;
	devInfo.pEnabledFeatures = &features;

	auto device = vpp::Device(instance, phdev, devInfo);
	auto presentQueue = device.queue(queueFam);

	auto renderInfo = RendererCreateInfo {
		device, vkSurf, window.size(), *presentQueue,
		startMsaa, vsync, clearColor
	};

	auto renderer = Renderer(renderInfo);

	// rvg
	rvg::Context ctx(device, {renderer.renderPass(), 0, true});

	rvg::Transform transform(ctx);

	auto drawMode = rvg::DrawMode {false, 1.f};
	drawMode.aaStroke = true;
	drawMode.deviceLocal = true;
	rvg::Shape shape(ctx, {}, drawMode);
	rvg::Paint paint(ctx, rvg::colorPaint({rvg::norm, 0.1f, .6f, .3f}));

	auto fontHeight = 14;
	rvg::FontAtlas atlas(ctx);
	rvg::Font lsFont(atlas, baseResPath + "example/LiberationSans-Regular.ttf",
		fontHeight);
	atlas.bake(ctx);

	auto string = "yo, whaddup";
	rvg::Text text(ctx, string, lsFont, {0, 0});
	auto textWidth = lsFont.width(string);

	// svg path
	// auto svgSubpath = rvg::parseSvgSubpath({300, 200},
		// "h -150 a150 150 0 1 0 150 -150 z");
	auto svgSubpath = ktc::parseSvgSubpath("h 1920 v 1080 h -1920 z");

	rvg::Shape svgShape(ctx, ktc::flatten(svgSubpath), {true, 0.f});
	rvg::Paint svgPaint {ctx, rvg::colorPaint({150, 230, 200})};

	auto bgPaintData = rvg::colorPaint({5, 5, 5});
	auto labelPaintData = rvg::colorPaint({240, 240, 240});

	auto hintBgPaint = rvg::Paint(ctx, rvg::colorPaint({5, 5, 5, 200}));
	auto hintTextPaint = rvg::Paint(ctx, labelPaintData);

	auto bgPaint = rvg::Paint(ctx, bgPaintData);

	// gui
	// vui::Gui gui(ctx, lsFont, std::move(styles));
	GuiListener listener;
	listener.ac = appContext.get();
	listener.wc = &window.windowContext();

	vui::Gui gui(ctx, lsFont, listener);

	auto bounds = nytl::Rect2f {100, 100, vui::autoSize, vui::autoSize};
	auto& cp = gui.create<vui::ColorPicker>(bounds);
	cp.onChange = [&](auto& cp){
		svgPaint.paint(rvg::colorPaint(cp.picked()));
	};

	bounds.position = {400, 100};
	auto& tf = gui.create<vui::Textfield>(bounds);
	tf.onSubmit = [&](auto& tf) {
		dlg_info("submitted: {}", tf.utf8());
	};

	tf.onCancel = [&](auto& tf) {
		dlg_info("cancelled: {}", tf.utf8());
	};

	bounds.position = {100, 500};
	auto& btn = gui.create<vui::LabeledButton>(bounds, "Waddup my man");
	dlg_info("{}", btn.size());
	btn.onClick = [&](auto&) {
		dlg_info("button pressed");
	};

	auto pos = nytl::Vec2f {520, 0};
	auto& panel = gui.create<vui::dat::Panel>(pos, 200.f);

	auto& f1 = panel.create<vui::dat::Folder>("folder 1");
	auto& b1 = f1.create<vui::dat::Button>("button 1");
	b1.onClick = [&](){ dlg_info("click 1"); };
	f1.create<vui::dat::Button>("button 4");
	f1.create<vui::dat::Button>("button 5");

	auto& f2 = panel.create<vui::dat::Folder>("folder 2");
	auto& b2 = f2.create<vui::dat::Button>("button 2");
	b2.onClick = [&](){ dlg_info("click 2"); };

	auto& nf1 = f2.create<vui::dat::Folder>("nested folder 1");
	auto& b3 = nf1.create<vui::dat::Button>("button 3");
	b3.onClick = [&](){ dlg_info("click 3"); };

	nf1.create<vui::dat::Button>("button 6");
	nf1.create<vui::dat::Button>("button 7");
	nf1.create<vui::dat::Button>("button 8");

	/*
	auto& win = gui.create<vui::Window>(nytl::Rect2f {100, 100, 500, 880});
	auto& button = win.create<vui::LabeledButton>("button, waddup");
	button.onClick = [&](auto&) { dlg_info("Clicked!"); };
	auto& cp = win.create<vui::ColorButton>(
		nytl::Vec2f{vui::autoSize, vui::autoSize}, rvg::Color {40, 40, 40});
	cp.onChange = [&](auto& cp){
		svgPaint.paint(rvg::colorPaint(cp.picked()));
	};

	win.create<vui::LabeledButton>("b#2");
	win.create<vui::Checkbox>();

	auto& tf = win.createSized<vui::Textfield>(nytl::Vec {400.f, vui::autoSize});
	tf.onChange = [&](auto& tf) {
		dlg_info("changed: {}", tf.utf8());
	};
	*/

	svgPaint = {ctx, rvg::colorPaint(cp.picked())};

	/*
	// dat
	// https://www.reddit.com/r/leagueoflegends/comments/3nnm36
	auto& panel = gui.create<vui::dat::Panel>(
		nytl::Rect2f {800.f, 0.f, vui::autoSize, vui::autoSize});
	panel.create<vui::dat::Button>("Unload the toad");
	panel.create<vui::dat::Textfield>("Some textfield");
	panel.create<vui::dat::Button>("Unclog the frog");

	auto& folder = panel.create<vui::dat::Folder>("Misc");
	folder.create<vui::dat::Button>("Eyooo");
	folder.create<vui::dat::Button>("Waddupp");
	folder.create<vui::dat::Textfield>("Thiccness");
	folder.create<vui::dat::Textfield>("Amount of parrots needed");
	folder.create<vui::dat::Button>("Ayy");

	auto& folder2 = panel.create<vui::dat::Folder>("Specifics");
	folder2.create<vui::dat::Button>("Permit the kermit");
	folder2.create<vui::dat::Label>("Days since a linux install", "42");
	folder2.create<vui::dat::Checkbox>("I want an extra llama");
	folder2.create<vui::dat::Button>("Unstick the lick");

	auto& nested = folder2.create<vui::dat::Folder>("Nested");
	nested.create<vui::dat::Label>("Does this work?", "Hopefully");

	panel.create<vui::dat::Textfield>("Awesomeness", "Over 9000");
	panel.create<vui::dat::Button>("Unclog the frog");
	*/

	// render recoreding
	renderer.onRender += [&](vk::CommandBuffer buf){
		ctx.bindDefaults(buf);

		transform.bind(buf);
		svgPaint.bind(buf);
		svgShape.fill(buf);

		paint.bind(buf);
		shape.stroke(buf);
		text.draw(buf);

		gui.draw(buf);
	};

	ctx.updateDevice();
	renderer.invalidate();

	// connect window & renderer
	auto run = true;
	window.onClose = [&](const auto&) { run = false; };
	window.onKey = [&](const auto& ev) {
		auto processed = false;

		// send key event to gui
		auto vev = vui::KeyEvent {};
		vev.key = static_cast<vui::Key>(ev.keycode); // both modeled after linux
		vev.modifiers = {static_cast<vui::KeyboardModifier>(ev.modifiers.value())};
		vev.pressed = ev.pressed;
		processed |= (gui.key(vev) != nullptr);

		// send text event to gui
		auto textable = ev.pressed && !ev.utf8.empty();
		textable &= !ny::specialKey(ev.keycode);
		textable &= !(ev.modifiers & ny::KeyboardModifier::ctrl);
		if(textable) {
			processed |= (gui.textInput({ev.utf8.c_str()}) != nullptr);
		}

		if(ev.pressed && !processed) {
			if(ev.keycode == ny::Keycode::escape) {
				dlg_info("Escape pressed, exiting");
				run = false;
			} else if(ev.keycode == ny::Keycode::b) {
				*paint.change() = rvg::colorPaint({rvg::norm, 0.2, 0.2, 0.8});
			} else if(ev.keycode == ny::Keycode::g) {
				*paint.change() = rvg::colorPaint({rvg::norm, 0.1, 0.6, 0.3});
			} else if(ev.keycode == ny::Keycode::r) {
				*paint.change() = rvg::colorPaint({rvg::norm, 0.8, 0.2, 0.3});
			} else if(ev.keycode == ny::Keycode::d) {
				*paint.change() = rvg::colorPaint({rvg::norm, 0.1, 0.1, 0.1});
			} else if(ev.keycode == ny::Keycode::w) {
				*paint.change() = rvg::colorPaint(rvg::Color::white);
			} else if(ev.keycode == ny::Keycode::p) {
				*paint.change() = rvg::linearGradient({0, 0}, {2000, 1000},
					{255, 0, 0}, {255, 255, 0});
			} else if(ev.keycode == ny::Keycode::c) {
				*paint.change() = rvg::radialGradient({1000, 500}, 0, 1000,
					{255, 0, 0}, {255, 255, 0});
			}
		}
	};
	window.onResize = [&](const auto& ev) {
		renderer.resize(ev.size);

		auto tchange = text.change();
		tchange->position.x = (ev.size[0] - textWidth) / 2;
		tchange->position.y = ev.size[1] - fontHeight - 20;

		auto mat = nytl::identity<4, float>();
		auto s = nytl::Vec {2.f / window.size().x, 2.f / window.size().y, 1};
		scale(mat, s);
		translate(mat, {-1, -1, 0});
		*transform.change() = mat;
		gui.transform(mat);
	};

	ktc::Subpath subpath;
	bool first = true;

	window.onMouseButton = [&](const auto& ev) {
		auto p = static_cast<nytl::Vec2f>(ev.position);
		if(gui.mouseButton({ev.pressed,
				static_cast<vui::MouseButton>(ev.button), p})) {
			return;
		}

		if(!ev.pressed) {
			return;
		}

		if(ev.button == ny::MouseButton::left) {
			if(first) {
				first = false;
				subpath.start = p;
			} else {
				subpath.sqBezier(p);
				shape.change()->points = ktc::flatten(subpath);
			}
		} else if(ev.button == ny::MouseButton::right) {
			// win.position(p);
		}
	};

	window.onMouseMove = [&](const auto& ev) {
		gui.mouseMove({static_cast<nytl::Vec2f>(ev.position)});
	};

	// - main loop -
	using Clock = std::chrono::high_resolution_clock;
	using Secf = std::chrono::duration<float, std::ratio<1, 1>>;

	auto lastFrame = Clock::now();
	auto fpsCounter = 0u;
	auto secCounter = 0.f;

	while(run) {
		auto now = Clock::now();
		auto diff = now - lastFrame;
		auto deltaCount = std::chrono::duration_cast<Secf>(diff).count();
		lastFrame = now;

		if(!appContext->pollEvents()) {
			dlg_info("pollEvents returned false");
			return 0;
		}

		gui.update(deltaCount);

		if(gui.updateDevice()) {
			dlg_info("gui rerecord");
			renderer.invalidate();
		}

		auto [rec, seph] = ctx.upload();

		if(rec) {
			dlg_info("ctx rerecord");
			renderer.invalidate();
		}

		auto wait = {
			vpp::StageSemaphore {
				seph,
				vk::PipelineStageBits::allGraphics,
			}
		};

		vpp::RenderInfo info;
		if(seph) {
			info.wait = wait;
		}

		renderer.renderSync(info);

		if(printFrames) {
			++fpsCounter;
			secCounter += deltaCount;
			if(secCounter >= 1.f) {
				dlg_info("{} fps", fpsCounter);
				secCounter = 0.f;
				fpsCounter = 0;
			}
		}
	}
}
