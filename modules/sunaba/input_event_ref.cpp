/*************************************************************************/
/*  input_event_ref.cpp                                                  */
/*************************************************************************/

#include "input_event_ref.h"

#include "input_map.h"
#include "os/keyboard.h"

/* Code conversion */

// Godot 2 special keys are SPKEY (1 << 24) | code, Godot 4's are
// (1 << 22) | code. Codes up to 0x2B and the keypad block are the same;
// this table covers the ones that moved.
static const int GODOT2_SPKEY = 1 << 24;
static const int GODOT4_SPECIAL = 1 << 22;
static const int GODOT4_KEY_UNKNOWN = 8388607;

static const struct {
	int godot2;
	int godot4;
} key_remap[] = {
	{ 0x2C, 0x17 }, // SUPER_L -> META
	{ 0x2D, 0x17 }, // SUPER_R -> META
	{ 0x2E, 0x42 }, // MENU
	{ 0x2F, 0x43 }, // HYPER_L -> HYPER
	{ 0x30, 0x43 }, // HYPER_R -> HYPER
	{ 0x31, 0x45 }, // HELP
	{ 0x40, 0x48 }, // BACK
	{ 0x41, 0x49 }, // FORWARD
	{ 0x42, 0x4A }, // STOP
	{ 0x43, 0x4B }, // REFRESH
	{ 0x44, 0x4C }, // VOLUMEDOWN
	{ 0x45, 0x4D }, // VOLUMEMUTE
	{ 0x46, 0x4E }, // VOLUMEUP
	{ 0x4C, 0x54 }, // MEDIAPLAY
	{ 0x4D, 0x55 }, // MEDIASTOP
	{ 0x4E, 0x56 }, // MEDIAPREVIOUS
	{ 0x4F, 0x57 }, // MEDIANEXT
	{ 0x50, 0x58 }, // MEDIARECORD
	{ 0x51, 0x59 }, // HOMEPAGE
	{ 0x52, 0x5A }, // FAVORITES
	{ 0x53, 0x5B }, // SEARCH
	{ 0x54, 0x5C }, // STANDBY
	{ 0x55, 0x5D }, // OPENURL
	{ 0x56, 0x5E }, // LAUNCHMAIL
	{ 0x57, 0x5F }, // LAUNCHMEDIA
	{ 0x58, 0x60 }, // LAUNCH0
	{ 0x59, 0x61 }, // LAUNCH1
	{ 0x5A, 0x62 }, // LAUNCH2
	{ 0x5B, 0x63 }, // LAUNCH3
	{ 0x5C, 0x64 }, // LAUNCH4
	{ 0x5D, 0x65 }, // LAUNCH5
	{ 0x5E, 0x66 }, // LAUNCH6
	{ 0x5F, 0x67 }, // LAUNCH7
	{ 0x60, 0x68 }, // LAUNCH8
	{ 0x61, 0x69 }, // LAUNCH9
	{ 0x62, 0x6A }, // LAUNCHA
	{ 0x63, 0x6B }, // LAUNCHB
	{ 0x64, 0x6C }, // LAUNCHC
	{ 0x65, 0x6D }, // LAUNCHD
	{ 0x66, 0x6E }, // LAUNCHE
	{ 0x67, 0x6F }, // LAUNCHF
	{ 0x80, 0x06 }, // KP_ENTER
	{ -1, -1 }
};

// Godot 2 has no equivalent for these Godot 4 special keys' low codes
// above the shared range unless listed in key_remap.
static bool godot2_special_is_shared(int p_low) {

	return (p_low >= 0x01 && p_low <= 0x2B) || (p_low >= 0x81 && p_low <= 0x8F);
}

int InputEventRef::key_to_godot4(uint32_t p_scancode) {

	if (!(p_scancode & GODOT2_SPKEY))
		return p_scancode;
	int low = p_scancode & ~GODOT2_SPKEY;
	if (godot2_special_is_shared(low))
		return GODOT4_SPECIAL | low;
	for (int i = 0; key_remap[i].godot2 >= 0; i++) {
		if (key_remap[i].godot2 == low)
			return GODOT4_SPECIAL | key_remap[i].godot4;
	}
	return GODOT4_KEY_UNKNOWN;
}

uint32_t InputEventRef::key_from_godot4(int p_keycode) {

	if (p_keycode == GODOT4_KEY_UNKNOWN)
		return KEY_UNKNOWN;
	if (!(p_keycode & GODOT4_SPECIAL))
		return p_keycode;
	int low = p_keycode & ~GODOT4_SPECIAL;
	if (godot2_special_is_shared(low))
		return GODOT2_SPKEY | low;
	for (int i = 0; key_remap[i].godot2 >= 0; i++) {
		if (key_remap[i].godot4 == low)
			return GODOT2_SPKEY | key_remap[i].godot2;
	}
	return KEY_UNKNOWN;
}

// Godot 2 JOY_* buttons (0 A, 1 B, 2 X, 3 Y, 4 L, 5 R, 6 L2, 7 R2, 8 L3,
// 9 R3, 10 SELECT, 11 START, 12-15 DPAD) -> Godot 4 JoyButton (SDL
// layout). Godot 4 reports L2/R2 as axes; as buttons they map to
// PADDLE1/PADDLE2.
static const int joy_button_map[16] = { 0, 1, 2, 3, 9, 10, 16, 17, 7, 8, 4, 6, 11, 12, 13, 14 };

int InputEventRef::joy_button_to_godot4(int p_button) {

	if (p_button >= 0 && p_button < 16)
		return joy_button_map[p_button];
	return p_button;
}

int InputEventRef::joy_button_from_godot4(int p_button) {

	for (int i = 0; i < 16; i++) {
		if (joy_button_map[i] == p_button)
			return i;
	}
	return p_button;
}

// Godot 2 analog L2/R2 are axes 6/7; Godot 4's triggers are 4/5.
int InputEventRef::joy_axis_to_godot4(int p_axis) {

	switch (p_axis) {
		case 4: return 6;
		case 5: return 7;
		case 6: return 4;
		case 7: return 5;
		default: return p_axis;
	}
}

int InputEventRef::joy_axis_from_godot4(int p_axis) {

	return joy_axis_to_godot4(p_axis); // the mapping is its own inverse
}

/* Class emulation */

static const char *class_for_type(int p_type) {

	switch (p_type) {
		case InputEvent::KEY: return "InputEventKey";
		case InputEvent::MOUSE_MOTION: return "InputEventMouseMotion";
		case InputEvent::MOUSE_BUTTON: return "InputEventMouseButton";
		case InputEvent::JOYSTICK_MOTION: return "InputEventJoypadMotion";
		case InputEvent::JOYSTICK_BUTTON: return "InputEventJoypadButton";
		case InputEvent::SCREEN_TOUCH: return "InputEventScreenTouch";
		case InputEvent::SCREEN_DRAG: return "InputEventScreenDrag";
		case InputEvent::ACTION: return "InputEventAction";
		default: return "InputEvent";
	}
}

String InputEventRef::get_godot4_class() const {

	return class_for_type(event.type);
}

bool InputEventRef::is_godot4_class(const String &p_class) const {

	if (p_class == "InputEvent" || p_class == "Reference" || p_class == "RefCounted" ||
			p_class == "Resource" || p_class == "Object" || p_class == "InputEventRef")
		return true;
	if (p_class == class_for_type(event.type))
		return true;
	bool key = event.type == InputEvent::KEY;
	bool mouse = event.type == InputEvent::MOUSE_MOTION || event.type == InputEvent::MOUSE_BUTTON;
	bool touch = event.type == InputEvent::SCREEN_TOUCH || event.type == InputEvent::SCREEN_DRAG;
	if (p_class == "InputEventFromWindow")
		return key || mouse || touch;
	if (p_class == "InputEventWithModifiers")
		return key || mouse;
	if (p_class == "InputEventMouse")
		return mouse;
	return false;
}

void InputEventRef::init_type(const String &p_godot4_class) {

	InputEvent e;
	e.device = event.device;
	for (int t = InputEvent::NONE; t < InputEvent::TYPE_MAX; t++) {
		if (p_godot4_class == class_for_type(t)) {
			e.type = t;
			break;
		}
	}
	event = e;
}

/* Properties (Godot 4 names) */

static InputModifierState *modifiers(InputEvent &e) {

	switch (e.type) {
		case InputEvent::KEY: return &e.key.mod;
		case InputEvent::MOUSE_MOTION: return &e.mouse_motion.mod;
		case InputEvent::MOUSE_BUTTON: return &e.mouse_button.mod;
		default: return NULL;
	}
}

bool InputEventRef::_get(const StringName &p_name, Variant &r_ret) const {

	String n = p_name;
	InputEvent &e = const_cast<InputEvent &>(event);

	if (n == "device") {
		r_ret = e.device;
		return true;
	}

	InputModifierState *mod = modifiers(e);
	if (mod) {
		if (n == "alt_pressed") {
			r_ret = mod->alt;
			return true;
		} else if (n == "shift_pressed") {
			r_ret = mod->shift;
			return true;
		} else if (n == "ctrl_pressed") {
			r_ret = mod->control;
			return true;
		} else if (n == "meta_pressed") {
			r_ret = mod->meta;
			return true;
		} else if (n == "command_or_control_autoremap") {
			r_ret = false;
			return true;
		}
	}

	switch (e.type) {
		case InputEvent::KEY:
			if (n == "pressed") r_ret = e.key.pressed;
			else if (n == "keycode" || n == "physical_keycode" || n == "key_label") r_ret = key_to_godot4(e.key.scancode);
			else if (n == "unicode") r_ret = (int)e.key.unicode;
			else if (n == "echo") r_ret = e.key.echo;
			else if (n == "location") r_ret = 0;
			else return false;
			return true;

		case InputEvent::MOUSE_MOTION:
		case InputEvent::MOUSE_BUTTON: {
			InputEventMouse &m = e.type == InputEvent::MOUSE_MOTION ? (InputEventMouse &)e.mouse_motion : (InputEventMouse &)e.mouse_button;
			if (n == "button_mask") {
				r_ret = m.button_mask;
				return true;
			} else if (n == "position") {
				r_ret = Vector2(m.x, m.y);
				return true;
			} else if (n == "global_position") {
				r_ret = Vector2(m.global_x, m.global_y);
				return true;
			}
			if (e.type == InputEvent::MOUSE_BUTTON) {
				if (n == "factor") r_ret = e.mouse_button.factor;
				else if (n == "button_index") r_ret = e.mouse_button.button_index;
				else if (n == "pressed") r_ret = e.mouse_button.pressed;
				else if (n == "double_click") r_ret = e.mouse_button.doubleclick;
				else if (n == "canceled") r_ret = false;
				else return false;
				return true;
			}
			if (n == "relative" || n == "screen_relative") r_ret = Vector2(e.mouse_motion.relative_x, e.mouse_motion.relative_y);
			else if (n == "velocity" || n == "screen_velocity") r_ret = Vector2(e.mouse_motion.speed_x, e.mouse_motion.speed_y);
			else if (n == "pressure") r_ret = 0.0;
			else if (n == "tilt") r_ret = Vector2();
			else if (n == "pen_inverted") r_ret = false;
			else return false;
			return true;
		}

		case InputEvent::JOYSTICK_MOTION:
			if (n == "axis") r_ret = joy_axis_to_godot4(e.joy_motion.axis);
			else if (n == "axis_value") r_ret = e.joy_motion.axis_value;
			else return false;
			return true;

		case InputEvent::JOYSTICK_BUTTON:
			if (n == "button_index") r_ret = joy_button_to_godot4(e.joy_button.button_index);
			else if (n == "pressed") r_ret = e.joy_button.pressed;
			else if (n == "pressure") r_ret = e.joy_button.pressure;
			else return false;
			return true;

		case InputEvent::SCREEN_TOUCH:
			if (n == "index") r_ret = e.screen_touch.index;
			else if (n == "position") r_ret = Vector2(e.screen_touch.x, e.screen_touch.y);
			else if (n == "pressed") r_ret = e.screen_touch.pressed;
			else if (n == "canceled" || n == "double_tap") r_ret = false;
			else return false;
			return true;

		case InputEvent::SCREEN_DRAG:
			if (n == "index") r_ret = e.screen_drag.index;
			else if (n == "position") r_ret = Vector2(e.screen_drag.x, e.screen_drag.y);
			else if (n == "relative" || n == "screen_relative") r_ret = Vector2(e.screen_drag.relative_x, e.screen_drag.relative_y);
			else if (n == "velocity" || n == "screen_velocity") r_ret = Vector2(e.screen_drag.speed_x, e.screen_drag.speed_y);
			else if (n == "pressure") r_ret = 0.0;
			else if (n == "tilt") r_ret = Vector2();
			else if (n == "pen_inverted") r_ret = false;
			else return false;
			return true;

		case InputEvent::ACTION:
			if (n == "action") r_ret = String(InputMap::get_singleton()->get_action_from_id(e.action.action));
			else if (n == "pressed") r_ret = e.action.pressed;
			else if (n == "strength") r_ret = e.action.pressed ? 1.0 : 0.0;
			else if (n == "event_index") r_ret = -1;
			else return false;
			return true;
	}
	return false;
}

bool InputEventRef::_set(const StringName &p_name, const Variant &p_value) {

	String n = p_name;
	InputEvent &e = event;

	if (n == "device") {
		e.device = p_value;
		return true;
	}

	InputModifierState *mod = modifiers(e);
	if (mod) {
		if (n == "alt_pressed") {
			mod->alt = p_value;
			return true;
		} else if (n == "shift_pressed") {
			mod->shift = p_value;
			return true;
		} else if (n == "ctrl_pressed") {
			mod->control = p_value;
			return true;
		} else if (n == "meta_pressed") {
			mod->meta = p_value;
			return true;
		}
	}

	switch (e.type) {
		case InputEvent::KEY:
			if (n == "pressed") e.key.pressed = p_value;
			else if (n == "keycode" || n == "physical_keycode") e.key.scancode = key_from_godot4(p_value);
			else if (n == "unicode") e.key.unicode = (int)p_value;
			else if (n == "echo") e.key.echo = p_value;
			else return false;
			return true;

		case InputEvent::MOUSE_MOTION:
		case InputEvent::MOUSE_BUTTON: {
			InputEventMouse &m = e.type == InputEvent::MOUSE_MOTION ? (InputEventMouse &)e.mouse_motion : (InputEventMouse &)e.mouse_button;
			if (n == "button_mask") {
				m.button_mask = p_value;
				return true;
			} else if (n == "position") {
				Vector2 v = p_value;
				m.x = v.x;
				m.y = v.y;
				return true;
			} else if (n == "global_position") {
				Vector2 v = p_value;
				m.global_x = v.x;
				m.global_y = v.y;
				return true;
			}
			if (e.type == InputEvent::MOUSE_BUTTON) {
				if (n == "factor") e.mouse_button.factor = p_value;
				else if (n == "button_index") e.mouse_button.button_index = p_value;
				else if (n == "pressed") e.mouse_button.pressed = p_value;
				else if (n == "double_click") e.mouse_button.doubleclick = p_value;
				else return false;
				return true;
			}
			if (n == "relative") {
				Vector2 v = p_value;
				e.mouse_motion.relative_x = v.x;
				e.mouse_motion.relative_y = v.y;
			} else if (n == "velocity") {
				Vector2 v = p_value;
				e.mouse_motion.speed_x = v.x;
				e.mouse_motion.speed_y = v.y;
			} else {
				return false;
			}
			return true;
		}

		case InputEvent::JOYSTICK_MOTION:
			if (n == "axis") e.joy_motion.axis = joy_axis_from_godot4(p_value);
			else if (n == "axis_value") e.joy_motion.axis_value = p_value;
			else return false;
			return true;

		case InputEvent::JOYSTICK_BUTTON:
			if (n == "button_index") e.joy_button.button_index = joy_button_from_godot4(p_value);
			else if (n == "pressed") e.joy_button.pressed = p_value;
			else if (n == "pressure") e.joy_button.pressure = p_value;
			else return false;
			return true;

		case InputEvent::SCREEN_TOUCH:
			if (n == "index") {
				e.screen_touch.index = p_value;
			} else if (n == "position") {
				Vector2 v = p_value;
				e.screen_touch.x = v.x;
				e.screen_touch.y = v.y;
			} else if (n == "pressed") {
				e.screen_touch.pressed = p_value;
			} else {
				return false;
			}
			return true;

		case InputEvent::SCREEN_DRAG:
			if (n == "index") {
				e.screen_drag.index = p_value;
			} else if (n == "position") {
				Vector2 v = p_value;
				e.screen_drag.x = v.x;
				e.screen_drag.y = v.y;
			} else if (n == "relative") {
				Vector2 v = p_value;
				e.screen_drag.relative_x = v.x;
				e.screen_drag.relative_y = v.y;
			} else if (n == "velocity") {
				Vector2 v = p_value;
				e.screen_drag.speed_x = v.x;
				e.screen_drag.speed_y = v.y;
			} else {
				return false;
			}
			return true;

		case InputEvent::ACTION:
			if (n == "action") e.action.action = InputMap::get_singleton()->get_action_id(String(p_value));
			else if (n == "pressed") e.action.pressed = p_value;
			else return false;
			return true;
	}
	return false;
}

/* Methods */

bool InputEventRef::is_action(const String &p_action, bool p_exact) const {

	return InputMap::get_singleton()->has_action(p_action) && event.is_action(p_action);
}

bool InputEventRef::is_action_pressed(const String &p_action, bool p_allow_echo, bool p_exact) const {

	if (!p_allow_echo && event.is_echo())
		return false;
	return is_action(p_action) && event.is_pressed();
}

bool InputEventRef::is_action_released(const String &p_action, bool p_exact) const {

	return is_action(p_action) && !event.is_pressed();
}

bool InputEventRef::is_action_type() const {

	return event.type != InputEvent::MOUSE_MOTION && event.type != InputEvent::SCREEN_DRAG && event.type != InputEvent::NONE;
}

float InputEventRef::get_action_strength(const String &p_action, bool p_exact) const {

	if (!is_action(p_action))
		return 0.0;
	if (event.type == InputEvent::JOYSTICK_MOTION)
		return Math::abs(event.joy_motion.axis_value);
	if (event.type == InputEvent::JOYSTICK_BUTTON && event.joy_button.pressed && event.joy_button.pressure > 0)
		return event.joy_button.pressure;
	return event.is_pressed() ? 1.0 : 0.0;
}

bool InputEventRef::is_echo() const {

	return event.is_echo();
}

bool InputEventRef::is_pressed() const {

	return event.is_pressed();
}

bool InputEventRef::is_released() const {

	return is_action_type() && !event.is_pressed();
}

bool InputEventRef::is_match(const Variant &p_event, bool p_exact) const {

	if (p_event.get_type() == Variant::INPUT_EVENT)
		return event == (InputEvent)p_event;
	Object *o = p_event;
	InputEventRef *other = o ? o->cast_to<InputEventRef>() : NULL;
	return other && event == other->event;
}

String InputEventRef::as_text() const {

	if (event.type == InputEvent::KEY)
		return keycode_get_string(event.key.scancode);
	return String(event);
}

Ref<InputEventRef> InputEventRef::xformed_by(const Matrix32 &p_xform, const Vector2 &p_local_ofs) const {

	Matrix32 xform = p_xform;
	xform.elements[2] += p_local_ofs;
	return wrap(event.xform_by(xform));
}

Ref<InputEventRef> InputEventRef::wrap(const InputEvent &p_event) {

	Ref<InputEventRef> ref;
	ref.instance();
	ref->event = p_event;
	return ref;
}

void InputEventRef::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("get_event"), &InputEventRef::_get_event_variant);
	ObjectTypeDB::bind_method(_MD("set_event", "event"), &InputEventRef::_set_event_variant);
	ObjectTypeDB::bind_method(_MD("get_godot4_class"), &InputEventRef::get_godot4_class);
	ObjectTypeDB::bind_method(_MD("is_godot4_class", "class"), &InputEventRef::is_godot4_class);
	ObjectTypeDB::bind_method(_MD("init_type", "godot4_class"), &InputEventRef::init_type);

	ObjectTypeDB::bind_method(_MD("is_action", "action", "exact_match"), &InputEventRef::is_action, DEFVAL(false));
	ObjectTypeDB::bind_method(_MD("is_action_pressed", "action", "allow_echo", "exact_match"), &InputEventRef::is_action_pressed, DEFVAL(false), DEFVAL(false));
	ObjectTypeDB::bind_method(_MD("is_action_released", "action", "exact_match"), &InputEventRef::is_action_released, DEFVAL(false));
	ObjectTypeDB::bind_method(_MD("is_action_type"), &InputEventRef::is_action_type);
	ObjectTypeDB::bind_method(_MD("get_action_strength", "action", "exact_match"), &InputEventRef::get_action_strength, DEFVAL(false));
	ObjectTypeDB::bind_method(_MD("is_echo"), &InputEventRef::is_echo);
	ObjectTypeDB::bind_method(_MD("is_pressed"), &InputEventRef::is_pressed);
	ObjectTypeDB::bind_method(_MD("is_released"), &InputEventRef::is_released);
	ObjectTypeDB::bind_method(_MD("is_canceled"), &InputEventRef::is_canceled);
	ObjectTypeDB::bind_method(_MD("accumulate", "with_event"), &InputEventRef::accumulate);
	ObjectTypeDB::bind_method(_MD("is_match", "event", "exact_match"), &InputEventRef::is_match, DEFVAL(true));
	ObjectTypeDB::bind_method(_MD("as_text"), &InputEventRef::as_text);
	ObjectTypeDB::bind_method(_MD("xformed_by:InputEventRef", "xform", "local_ofs"), &InputEventRef::xformed_by, DEFVAL(Vector2()));
}

InputEventRef::InputEventRef() {
}
