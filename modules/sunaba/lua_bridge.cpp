/*************************************************************************/
/*  lua_bridge.cpp                                                       */
/*************************************************************************/

#include "lua_bridge.h"

#include "image_ref.h"
#include "input_event_ref.h"
#include "lua_runtime.h"
#include "script_object.h"

namespace sunaba {

static const char *RUNTIME_KEY = "sunaba.runtime";

Object *ObjectBox::get() const {

	if (id == 0)
		return NULL;
	return ObjectDB::get_instance(id);
}

/* Registry */

Runtime *get_runtime(lua_State *L) {

	lua_getfield(L, LUA_REGISTRYINDEX, RUNTIME_KEY);
	Runtime *rt = (Runtime *)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return rt;
}

void set_runtime(lua_State *L, Runtime *p_runtime) {

	lua_pushlightuserdata(L, p_runtime);
	lua_setfield(L, LUA_REGISTRYINDEX, RUNTIME_KEY);
}

/* Userdata */

int get_kind(lua_State *L, int p_idx) {

	if (lua_type(L, p_idx) != LUA_TUSERDATA)
		return KIND_NONE;
	if (!lua_getmetatable(L, p_idx))
		return KIND_NONE;
	lua_pushstring(L, SUNABA_KIND_KEY);
	lua_rawget(L, -2);
	int kind = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	return kind;
}

void *test_kind(lua_State *L, int p_idx, int p_kind) {

	if (get_kind(L, p_idx) != p_kind)
		return NULL;
	return lua_touserdata(L, p_idx);
}

void new_type(lua_State *L, const char *p_name, int p_kind, const luaL_Reg *p_methods, const luaL_Reg *p_meta) {

	luaL_newmetatable(L, p_name);
	int mt = lua_gettop(L);

	lua_pushinteger(L, p_kind);
	lua_setfield(L, mt, SUNABA_KIND_KEY);

	lua_newtable(L);
	if (p_methods)
		luaL_setfuncs(L, p_methods, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, mt, SUNABA_METHODS_KEY);
	lua_setglobal(L, p_name);

	lua_newtable(L);
	lua_setfield(L, mt, "__cache");

	if (p_meta)
		luaL_setfuncs(L, p_meta, 0);

	lua_getfield(L, mt, "__index");
	bool has_index = !lua_isnil(L, -1);
	lua_pop(L, 1);
	if (!has_index) {
		lua_getfield(L, mt, SUNABA_METHODS_KEY);
		lua_setfield(L, mt, "__index");
	}

	lua_pop(L, 1);
}

void push_methods(lua_State *L, const char *p_name) {

	luaL_getmetatable(L, p_name);
	lua_getfield(L, -1, SUNABA_METHODS_KEY);
	lua_remove(L, -2);
}

/* Variant boxes */

const char *metatable_for(Variant::Type p_type) {

	switch (p_type) {
		case Variant::VECTOR2: return SUNABA_MT_VECTOR2;
		case Variant::RECT2: return SUNABA_MT_RECT2;
		case Variant::VECTOR3: return SUNABA_MT_VECTOR3;
		case Variant::MATRIX32: return SUNABA_MT_TRANSFORM2D;
		case Variant::PLANE: return SUNABA_MT_PLANE;
		case Variant::QUAT: return SUNABA_MT_QUATERNION;
		case Variant::_AABB: return SUNABA_MT_AABB;
		case Variant::MATRIX3: return SUNABA_MT_BASIS;
		case Variant::TRANSFORM: return SUNABA_MT_TRANSFORM3D;
		case Variant::COLOR: return SUNABA_MT_COLOR;
		case Variant::ARRAY: return SUNABA_MT_ARRAYLIST;
		case Variant::DICTIONARY: return SUNABA_MT_DICTIONARY;
		default: return NULL;
	}
}

VariantBox *test_box(lua_State *L, int p_idx) {

	return (VariantBox *)test_kind(L, p_idx, KIND_VARIANT_BOX);
}

// Boxed containers must be shared so mutations made through the box are
// visible to everything else holding the same container (Godot 4
// semantics). Godot 2 copies non-shared containers on write.
static Variant make_shared(const Variant &p_value) {

	if (p_value.get_type() == Variant::ARRAY) {
		const Array src = p_value;
		if (src.is_shared())
			return p_value;
		Array dst(true);
		dst.resize(src.size());
		for (int i = 0; i < src.size(); i++)
			dst[i] = src[i];
		return dst;
	}
	if (p_value.get_type() == Variant::DICTIONARY) {
		const Dictionary src = p_value;
		if (src.is_shared())
			return p_value;
		Dictionary dst(true);
		const Variant *k = NULL;
		while ((k = src.next(k)))
			dst[*k] = src[*k]; // const access: no copy-on-write
		return dst;
	}
	return p_value;
}

void push_box(lua_State *L, const Variant &p_value, const char *p_metatable) {

	VariantBox *b = (VariantBox *)lua_newuserdatauv(L, sizeof(VariantBox), 0);
	memnew_placement(b, VariantBox);
	b->value = make_shared(p_value);
	luaL_setmetatable(L, p_metatable);
}

void push_variant(lua_State *L, const Variant &p_value) {

	push_box(L, p_value, SUNABA_MT_VARIANT);
}

void push_object(lua_State *L, Object *p_object) {

	if (!p_object) {
		lua_pushnil(L);
		return;
	}
	ObjectBox *b = (ObjectBox *)lua_newuserdatauv(L, sizeof(ObjectBox), 0);
	memnew_placement(b, ObjectBox);
	b->id = p_object->get_instance_ID();
	Reference *r = p_object->cast_to<Reference>();
	if (r) {
		b->ref = Ref<Reference>(r);
		luaL_setmetatable(L, SUNABA_MT_NATIVEREFERENCE);
	} else {
		luaL_setmetatable(L, SUNABA_MT_NATIVEOBJECT);
	}
}

static void push_string(lua_State *L, const String &p_string) {

	CharString cs = p_string.utf8();
	lua_pushlstring(L, cs.get_data(), cs.length());
}

void push_typed(lua_State *L, const Variant &p_value) {

	switch (p_value.get_type()) {
		case Variant::NIL: lua_pushnil(L); return;
		case Variant::BOOL: lua_pushboolean(L, (bool)p_value); return;
		case Variant::INT: lua_pushinteger(L, (int)p_value); return;
		case Variant::REAL: lua_pushnumber(L, (double)p_value); return;
		case Variant::STRING: push_string(L, p_value); return;
		case Variant::OBJECT: {
			Object *o = p_value;
			push_object(L, o);
			return;
		}
		case Variant::INPUT_EVENT: {
			Ref<InputEventRef> ev = InputEventRef::wrap(p_value);
			push_object(L, ev.ptr());
			return;
		}
		case Variant::IMAGE: {
			Ref<ImageRef> img = ImageRef::wrap(p_value);
			push_object(L, img.ptr());
			return;
		}
		default: break;
	}

	const char *mt = metatable_for(p_value.get_type());
	push_box(L, p_value, mt ? mt : SUNABA_MT_VARIANT);
}

void push_script_value(lua_State *L, const Variant &p_value) {

	switch (p_value.get_type()) {
		case Variant::NIL:
		case Variant::BOOL:
		case Variant::INT:
		case Variant::REAL:
		case Variant::STRING:
			push_typed(L, p_value);
			return;
		case Variant::ARRAY: {
			Array arr = p_value;
			lua_createtable(L, arr.size(), 0);
			for (int i = 0; i < arr.size(); i++) {
				push_script_value(L, arr[i]);
				lua_rawseti(L, -2, i + 1);
			}
			return;
		}
		case Variant::DICTIONARY: {
			Dictionary dict = p_value;
			lua_createtable(L, 0, dict.size());
			const Variant *k = NULL;
			while ((k = dict.next(k))) {
				push_script_value(L, *k);
				push_script_value(L, dict[*k]);
				if (lua_isnil(L, -2)) {
					lua_pop(L, 2);
					continue;
				}
				lua_rawset(L, -3);
			}
			return;
		}
		case Variant::OBJECT: {
			Object *o = p_value;
			if (o) {
				ScriptObject *so = o->cast_to<ScriptObject>();
				if (so) {
					if (!so->push_table())
						lua_pushnil(L);
					return;
				}
			}
			push_object(L, o);
			return;
		}
		case Variant::INPUT_EVENT:
		case Variant::IMAGE:
			// Godot 4 events and images are objects; present Godot 2's the
			// same way.
			push_typed(L, p_value);
			return;
		default:
			// libsunaba pushed everything else as a raw godot::Variant.
			push_variant(L, p_value);
			return;
	}
}

/* Lua -> Variant */

static Variant table_to_script_object(lua_State *L, int p_idx) {

	Ref<ScriptObject> so;
	so.instance();
	so->set_table(L, p_idx);
	return so;
}

static Variant function_to_script_function(lua_State *L, int p_idx) {

	Ref<ScriptFunction> sf;
	sf.instance();
	sf->set_function(L, p_idx);
	return sf;
}

static Variant object_box_to_variant(const ObjectBox *p_box) {

	// Godot 2 APIs take InputEvent values, not the wrapper.
	const InputEventRef *ev = p_box->ref.is_valid() ? p_box->ref->cast_to<InputEventRef>() : NULL;
	if (ev)
		return ev->get_event();
	// Likewise for Image values.
	const ImageRef *img = p_box->ref.is_valid() ? p_box->ref->cast_to<ImageRef>() : NULL;
	if (img)
		return img->get_image();
	if (p_box->ref.is_valid())
		return Variant(p_box->ref);
	return Variant(p_box->get());
}

static Variant byte_to_variant(const ByteBox *p_byte) {

	int v;
	memcpy(&v, p_byte->data, sizeof(int));
	return v;
}

static Variant userdata_to_variant(lua_State *L, int p_idx) {

	switch (get_kind(L, p_idx)) {
		case KIND_VARIANT_BOX:
			return ((VariantBox *)lua_touserdata(L, p_idx))->value;
		case KIND_OBJECT:
			return object_box_to_variant((ObjectBox *)lua_touserdata(L, p_idx));
		case KIND_CALLABLE: {
			CallableBox *c = (CallableBox *)lua_touserdata(L, p_idx);
			if (c->ref.is_valid())
				return Variant(c->ref);
			return Variant(c->get_object());
		}
		case KIND_SIGNAL:
			return String(((SignalBox *)lua_touserdata(L, p_idx))->name);
		case KIND_BYTE:
			return byte_to_variant((ByteBox *)lua_touserdata(L, p_idx));
		case KIND_BYTEARRAY: {
			ByteArrayBox *ba = (ByteArrayBox *)lua_touserdata(L, p_idx);
			ByteArray raw;
			raw.resize(ba->cells.size());
			{
				ByteArray::Write w = raw.write();
				for (int i = 0; i < ba->cells.size(); i++)
					w[i] = ba->cells[i].data[0];
			}
			return raw;
		}
		default:
			return Variant();
	}
}

Variant to_variant(lua_State *L, int p_idx) {

	switch (lua_type(L, p_idx)) {
		case LUA_TBOOLEAN:
			return (bool)lua_toboolean(L, p_idx);
		case LUA_TNUMBER:
			if (lua_isinteger(L, p_idx))
				return (int)lua_tointeger(L, p_idx);
			return (double)lua_tonumber(L, p_idx);
		case LUA_TSTRING:
			return to_string(L, p_idx);
		case LUA_TUSERDATA:
			return userdata_to_variant(L, p_idx);
		case LUA_TTABLE:
			return table_to_script_object(L, p_idx);
		case LUA_TFUNCTION:
			return function_to_script_function(L, p_idx);
		default:
			return Variant();
	}
}

Variant to_script_variant(lua_State *L, int p_idx) {

	if (lua_type(L, p_idx) == LUA_TNUMBER)
		return (double)lua_tonumber(L, p_idx);
	return to_variant(L, p_idx);
}

bool to_array(lua_State *L, int p_idx, Array &r_array) {

	switch (lua_type(L, p_idx)) {
		case LUA_TNONE:
		case LUA_TNIL:
			r_array = Array(true);
			return true;
		case LUA_TUSERDATA: {
			VariantBox *b = test_box(L, p_idx);
			if (!b)
				return false;
			switch (b->value.get_type()) {
				case Variant::ARRAY:
				case Variant::RAW_ARRAY:
				case Variant::INT_ARRAY:
				case Variant::REAL_ARRAY:
				case Variant::STRING_ARRAY:
				case Variant::VECTOR2_ARRAY:
				case Variant::VECTOR3_ARRAY:
				case Variant::COLOR_ARRAY:
					r_array = b->value;
					return true;
				default:
					return false;
			}
		}
		case LUA_TTABLE: {
			int abs = lua_absindex(L, p_idx);
			int n = (int)lua_rawlen(L, abs);
			r_array = Array(true);
			r_array.resize(n);
			for (int i = 0; i < n; i++) {
				lua_rawgeti(L, abs, i + 1);
				r_array[i] = to_variant(L, -1);
				lua_pop(L, 1);
			}
			return true;
		}
		default:
			return false;
	}
}

String to_string(lua_State *L, int p_idx) {

	int t = lua_type(L, p_idx);
	if (t != LUA_TSTRING && t != LUA_TNUMBER)
		return String(luaL_typename(L, p_idx));
	// Convert a copy: lua_tolstring changes numbers on the stack in place.
	lua_pushvalue(L, p_idx);
	size_t len = 0;
	const char *s = lua_tolstring(L, -1, &len);
	String ret = String::utf8(s, (int)len);
	lua_pop(L, 1);
	return ret;
}

String call_error_text(const Variant::CallError &p_error, const String &p_method) {

	switch (p_error.error) {
		case Variant::CallError::CALL_OK:
			return String();
		case Variant::CallError::CALL_ERROR_INVALID_METHOD:
			return "Invalid method '" + p_method + "'";
		case Variant::CallError::CALL_ERROR_INVALID_ARGUMENT:
			return "Invalid type for argument " + itos(p_error.argument + 1) + " of '" + p_method + "', expected " + Variant::get_type_name(p_error.expected);
		case Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS:
			return "Too many arguments for '" + p_method + "'";
		case Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS:
			return "Too few arguments for '" + p_method + "'";
		case Variant::CallError::CALL_ERROR_INSTANCE_IS_NULL:
			return "Instance is null calling '" + p_method + "'";
	}
	return "Error calling '" + p_method + "'";
}

/* Godot 4 type numbering */

int to_godot4_type(Variant::Type p_type) {

	switch (p_type) {
		case Variant::NIL: return 0;
		case Variant::BOOL: return 1;
		case Variant::INT: return 2;
		case Variant::REAL: return 3;
		case Variant::STRING: return 4;
		case Variant::VECTOR2: return 5;
		case Variant::RECT2: return 7;
		case Variant::VECTOR3: return 9;
		case Variant::MATRIX32: return 11;
		case Variant::PLANE: return 14;
		case Variant::QUAT: return 15;
		case Variant::_AABB: return 16;
		case Variant::MATRIX3: return 17;
		case Variant::TRANSFORM: return 18;
		case Variant::COLOR: return 20;
		case Variant::NODE_PATH: return 22;
		case Variant::_RID: return 23;
		// Image and InputEvent are objects in Godot 4.
		case Variant::IMAGE:
		case Variant::INPUT_EVENT:
		case Variant::OBJECT: return 24;
		case Variant::DICTIONARY: return 27;
		case Variant::ARRAY: return 28;
		case Variant::RAW_ARRAY: return 29;
		case Variant::INT_ARRAY: return 30;
		case Variant::REAL_ARRAY: return 32;
		case Variant::STRING_ARRAY: return 34;
		case Variant::VECTOR2_ARRAY: return 35;
		case Variant::VECTOR3_ARRAY: return 36;
		case Variant::COLOR_ARRAY: return 37;
		default: return 0;
	}
}

String godot4_type_name(int p_godot4_type) {

	static const char *names[] = {
		"Nil", "bool", "int", "float", "String", "Vector2", "Vector2i", "Rect2", "Rect2i",
		"Vector3", "Vector3i", "Transform2D", "Vector4", "Vector4i", "Plane", "Quaternion",
		"AABB", "Basis", "Transform3D", "Projection", "Color", "StringName", "NodePath", "RID",
		"Object", "Callable", "Signal", "Dictionary", "Array", "PackedByteArray",
		"PackedInt32Array", "PackedInt64Array", "PackedFloat32Array", "PackedFloat64Array",
		"PackedStringArray", "PackedVector2Array", "PackedVector3Array", "PackedColorArray",
		"PackedVector4Array"
	};
	if (p_godot4_type < 0 || p_godot4_type >= (int)(sizeof(names) / sizeof(names[0])))
		return String();
	return names[p_godot4_type];
}

/* Protected access and calls */

static int getfield_k(lua_State *L) {

	lua_gettable(L, 1);
	return 1;
}

static int setfield_k(lua_State *L) {

	lua_settable(L, 1);
	return 0;
}

bool safe_getfield(lua_State *L, int p_idx, const char *p_key) {

	int t = lua_absindex(L, p_idx);
	lua_pushcfunction(L, getfield_k);
	lua_pushvalue(L, t);
	lua_pushstring(L, p_key);
	if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
		report_error(L, "Script Error");
		lua_pushnil(L);
		return false;
	}
	return true;
}

bool safe_setfield(lua_State *L, int p_idx, const char *p_key) {

	int t = lua_absindex(L, p_idx);
	int v = lua_gettop(L);
	lua_pushcfunction(L, setfield_k);
	lua_pushvalue(L, t);
	lua_pushstring(L, p_key);
	lua_pushvalue(L, v);
	lua_remove(L, v);
	if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
		report_error(L, "Script Error");
		return false;
	}
	return true;
}

static int traceback_handler(lua_State *L) {

	const char *msg = lua_tostring(L, 1);
	if (!msg) {
		if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
			return 1;
		msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
	}
	luaL_traceback(L, L, msg, 1);
	return 1;
}

int pcall_traceback(lua_State *L, int p_nargs, int p_nresults) {

	int base = lua_gettop(L) - p_nargs;
	lua_pushcfunction(L, traceback_handler);
	lua_insert(L, base);
	int status = lua_pcall(L, p_nargs, p_nresults, base);
	lua_remove(L, base);
	return status;
}

void report_error(lua_State *L, const String &p_title) {

	String msg = to_string(L, -1);
	lua_pop(L, 1);
	Runtime *rt = get_runtime(L);
	if (rt) {
		rt->errord(msg, p_title);
	} else {
		ERR_PRINT(String(p_title + ": " + msg).utf8().get_data());
	}
}

} // namespace sunaba
