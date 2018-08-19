#pragma once

#include <vui/fwd.hpp>
#include <nytl/vec.hpp>
#include <nytl/flags.hpp>

namespace vui {

enum class MouseButton : unsigned {
	left = 2,
	right,
	middle,
	custom1,
	custom2
};

// modeled after <linux/input-event-codes.h>
enum class Key : unsigned {
	escape = 1,
	backspace = 14,
	enter = 28,
	up = 103,
	left = 105,
	right = 106,
	del = 111,
	down = 108,

	a = 30,
	c = 46,
	v = 47,
	x = 45,
};

enum class KeyboardModifier : unsigned {
	shift = (1 << 1),
	ctrl = (1 << 2),
	alt = (1 << 3),
	super = (1 << 4),
	capsLock = (1 << 5),
	numLock = (1 << 6)
};

NYTL_FLAG_OPS(KeyboardModifier)
using KeyboardModifiers = nytl::Flags<KeyboardModifier>;

// events
struct MouseMoveEvent {
	Vec2f position;
};

struct MouseButtonEvent {
	bool pressed;
	MouseButton button;
	Vec2f position;
};

struct MouseWheelEvent {
	Vec2f distance;
	Vec2f position;
};

struct KeyEvent {
	Key key;
	bool pressed;
	KeyboardModifiers modifiers;
};

struct TextInputEvent {
	const char* utf8;
};

} // namespace vui
