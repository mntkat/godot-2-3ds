/*************************************************************************/
/*  lua_bridge.h                                                         */
/*************************************************************************/
/*  Sunaba Lua runtime for Godot 2.x.                                    */
/*                                                                       */
/*  Port of libsunaba's sol2/godot-cpp bindings to the raw Lua C API so  */
/*  the module builds with the 3DS toolchain flags (gnu++11, no RTTI, no */
/*  exceptions).                                                         */
/*                                                                       */
/*  Lua raises errors with longjmp, which skips C++ destructors. Every   */
/*  binding is therefore split into an _impl function that owns all C++  */
/*  temporaries and reports failure through a LuaError, and a thin       */
/*  wrapper that raises the Lua error after the _impl frame has unwound. */
/*  Use SUNABA_LUA_FUNC to declare bindings.                             */
/*************************************************************************/

#ifndef SUNABA_LUA_BRIDGE_H
#define SUNABA_LUA_BRIDGE_H

#include "array.h"
#include "dictionary.h"
#include "reference.h"
#include "variant.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

class Runtime;

namespace sunaba {

// Lua-visible type names. These keep the Godot 4 names used by libsunaba's
// Haxe API so the same Lua output runs on both engines.
#define SUNABA_MT_VARIANT "Variant"
#define SUNABA_MT_VECTOR2 "Vector2"
#define SUNABA_MT_RECT2 "Rect2"
#define SUNABA_MT_VECTOR3 "Vector3"
#define SUNABA_MT_TRANSFORM2D "Transform2D"
#define SUNABA_MT_PLANE "Plane"
#define SUNABA_MT_QUATERNION "Quaternion"
#define SUNABA_MT_AABB "AABB"
#define SUNABA_MT_BASIS "Basis"
#define SUNABA_MT_TRANSFORM3D "Transform3D"
#define SUNABA_MT_COLOR "Color"
#define SUNABA_MT_ARRAYLIST "ArrayList"
#define SUNABA_MT_DICTIONARY "Dictionary"
#define SUNABA_MT_NATIVEOBJECT "NativeObject"
#define SUNABA_MT_NATIVEREFERENCE "NativeReference"
#define SUNABA_MT_CALLABLE "Callable"
#define SUNABA_MT_SIGNAL "Signal"
#define SUNABA_MT_BYTE "Byte"
#define SUNABA_MT_BYTEARRAY "BinaryData"

// Every sunaba userdata metatable stores its kind under this key.
#define SUNABA_KIND_KEY "__sunaba"
// The table of explicit methods (also the global type table).
#define SUNABA_METHODS_KEY "__methods"

enum UserdataKind {
	KIND_NONE = 0,
	KIND_VARIANT_BOX = 1, // any boxed Variant (Variant, Vector2, ArrayList, ...)
	KIND_OBJECT = 2, // NativeObject / NativeReference
	KIND_BYTE = 3,
	KIND_BYTEARRAY = 4,
	KIND_CALLABLE = 5,
	KIND_SIGNAL = 6,
};

struct LuaError {
	bool set;
	char msg[512];

	LuaError() {
		set = false;
		msg[0] = 0;
	}

	void format(const char *p_format, ...) {
		va_list args;
		va_start(args, p_format);
		vsnprintf(msg, sizeof(msg), p_format, args);
		va_end(args);
		set = true;
	}
};

// LuaError is trivially destructible, so it is safe to longjmp over it.
#define SUNABA_LUA_FUNC(m_name)                                    \
	static int m_name##_impl(lua_State *L, sunaba::LuaError &err); \
	static int m_name(lua_State *L) {                              \
		sunaba::LuaError err;                                      \
		int ret = m_name##_impl(L, err);                           \
		if (err.set)                                               \
			return luaL_error(L, "%s", err.msg);                   \
		return ret;                                                \
	}                                                              \
	static int m_name##_impl(lua_State *L, sunaba::LuaError &err)

// Boxed Variant stored in a full userdata.
struct VariantBox {
	Variant value;
};

// NativeObject and NativeReference share this layout. `ref` is set for
// Reference-derived objects so they stay alive while Lua holds them.
struct ObjectBox {
	ObjectID id;
	Ref<Reference> ref;

	Object *get() const;
};

// Godot 2 has no Callable. A Callable is a (target, method, binds) triple.
// Lua functions are wrapped in a ScriptFunction whose `invoke` method is
// the target, so they can be connected to signals.
struct CallableBox {
	ObjectID target;
	Ref<Reference> ref;
	StringName method;
	Array binds;

	Object *get_object() const;
	bool is_valid() const;
	Variant call(const Array &p_args, Variant::CallError &r_error) const;
};

// Godot 2 has no Signal type either; a Signal is an (object, name) pair.
struct SignalBox {
	ObjectID object;
	StringName name;
};

// A Byte is an 8-byte cell holding an int or double, as in libsunaba.
struct ByteBox {
	uint8_t data[8];
};

struct ByteArrayBox {
	Vector<ByteBox> cells;
};

/* Registry helpers */

Runtime *get_runtime(lua_State *L);
void set_runtime(lua_State *L, Runtime *p_runtime);

/* Userdata helpers */

int get_kind(lua_State *L, int p_idx);
void *test_kind(lua_State *L, int p_idx, int p_kind);

// Creates a metatable named p_name with the given kind and a methods table
// that is also published as the global p_name. Leaves nothing on the stack.
void new_type(lua_State *L, const char *p_name, int p_kind, const luaL_Reg *p_methods, const luaL_Reg *p_meta);
// Pushes the methods table of an existing type.
void push_methods(lua_State *L, const char *p_name);

/* Variant <-> Lua */

const char *metatable_for(Variant::Type p_type);

VariantBox *test_box(lua_State *L, int p_idx);
void push_box(lua_State *L, const Variant &p_value, const char *p_metatable);
// Always a "Variant" box (sol2 returned godot::Variant as its usertype).
void push_variant(lua_State *L, const Variant &p_value);
// Primitives become Lua values, math types typed boxes, objects
// NativeObject/NativeReference, everything else a Variant box.
void push_typed(lua_State *L, const Variant &p_value);
// libsunaba's gdToSol(): like push_typed but arrays and dictionaries become
// Lua tables and ScriptObjects unwrap to their Lua table.
void push_script_value(lua_State *L, const Variant &p_value);

void push_object(lua_State *L, Object *p_object);

// Typed conversion used for call arguments and Variant.new().
Variant to_variant(lua_State *L, int p_idx);
// libsunaba's solToGd(): numbers become floats, tables ScriptObjects.
Variant to_script_variant(lua_State *L, int p_idx);
// Accepts an ArrayList box, a Variant box holding an Array, a Lua sequence,
// or nil/none (empty array). Returns false for anything else.
bool to_array(lua_State *L, int p_idx, Array &r_array);

String to_string(lua_State *L, int p_idx);

// Protected t[k] (honours metatables). Always pushes exactly one value
// (nil on error) and returns false if a metamethod raised an error.
bool safe_getfield(lua_State *L, int p_idx, const char *p_key);
// Protected t[k] = v, where v is popped from the top of the stack.
bool safe_setfield(lua_State *L, int p_idx, const char *p_key);

// lua_pcall with a traceback message handler. On error the message is left
// on the stack, as with lua_pcall.
int pcall_traceback(lua_State *L, int p_nargs, int p_nresults);
// Pops the error message left by a failed pcall and reports it through the
// Runtime's _errord (falls back to ERR_PRINT without a Runtime).
void report_error(lua_State *L, const String &p_title);
String call_error_text(const Variant::CallError &p_error, const String &p_method);

// Godot 4 Variant.Type numbering, which the Haxe VariantType enum uses.
int to_godot4_type(Variant::Type p_type);
String godot4_type_name(int p_godot4_type);

// Metamethods shared by every boxed-Variant type (defined in lua_math.cpp).
int box_gc_func(lua_State *L);
int box_tostring_func(lua_State *L);
int box_eq_func(lua_State *L);

/* Per-type registration */

void open_variant(lua_State *L, bool p_sandboxed, const Array &p_classnames);
void open_math(lua_State *L);
void open_containers(lua_State *L);
void open_native(lua_State *L, bool p_sandboxed, const Array &p_classnames);
void open_callable(lua_State *L);
void open_io(lua_State *L);
void open_fs(lua_State *L);
void run_prelude(lua_State *L);

} // namespace sunaba

#endif // SUNABA_LUA_BRIDGE_H
