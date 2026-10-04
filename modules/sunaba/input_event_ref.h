/*************************************************************************/
/*  input_event_ref.h                                                    */
/*************************************************************************/
/*  Godot 2's InputEvent is a builtin value type; Godot 4's is a class   */
/*  hierarchy (InputEventKey, InputEventMouseMotion, ...). InputEventRef */
/*  wraps a Godot 2 InputEvent and presents it the Godot 4 way: class    */
/*  names, property names (position, relative, keycode, ...), key and    */
/*  joypad codes, and methods (is_action_pressed, as_text, ...), so      */
/*  libsunaba's Haxe input classes work unchanged.                       */
/*                                                                       */
/*  The Lua runtime wraps InputEvent values crossing into Lua            */
/*  automatically, and unwraps them when they are passed back.           */
/*************************************************************************/

#ifndef SUNABA_INPUT_EVENT_REF_H
#define SUNABA_INPUT_EVENT_REF_H

#include "os/input_event.h"
#include "reference.h"

class InputEventRef : public Reference {
	OBJ_TYPE(InputEventRef, Reference);

	InputEvent event;

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	static void _bind_methods();

public:
	static Ref<InputEventRef> wrap(const InputEvent &p_event);

	// Godot 2 <-> Godot 4 code conversion.
	static int key_to_godot4(uint32_t p_scancode);
	static uint32_t key_from_godot4(int p_keycode);
	static int joy_button_to_godot4(int p_button);
	static int joy_button_from_godot4(int p_button);
	static int joy_axis_to_godot4(int p_axis);
	static int joy_axis_from_godot4(int p_axis);

	const InputEvent &get_event() const { return event; }
	void set_event(const InputEvent &p_event) { event = p_event; }

	String get_godot4_class() const;
	bool is_godot4_class(const String &p_class) const;
	void init_type(const String &p_godot4_class);

	bool is_action(const String &p_action, bool p_exact = false) const;
	bool is_action_pressed(const String &p_action, bool p_allow_echo = false, bool p_exact = false) const;
	bool is_action_released(const String &p_action, bool p_exact = false) const;
	bool is_action_type() const;
	float get_action_strength(const String &p_action, bool p_exact = false) const;
	bool is_echo() const;
	bool is_pressed() const;
	bool is_released() const;
	bool is_canceled() const { return false; }
	bool accumulate(const Variant &p_event) { return false; }
	bool is_match(const Variant &p_event, bool p_exact = true) const;
	String as_text() const;
	Ref<InputEventRef> xformed_by(const Matrix32 &p_xform, const Vector2 &p_local_ofs = Vector2()) const;

	Variant _get_event_variant() const { return event; }
	void _set_event_variant(const InputEvent &p_event) { event = p_event; }

	InputEventRef();
};

#endif // SUNABA_INPUT_EVENT_REF_H
