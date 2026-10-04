/*************************************************************************/
/*  lua_variant.cpp                                                      */
/*************************************************************************/
/*  The `Variant` Lua type: a boxed godot Variant with from/as           */
/*  conversions, matching libsunaba's VariantNative Haxe extern.         */
/*************************************************************************/

#include "lua_bridge.h"

#include "lua_runtime.h"
#include "script_object.h"

namespace sunaba {

static VariantBox *self_variant(lua_State *L) {

	return test_box(L, 1);
}

#define VARIANT_SELF(m_name)                                  \
	VariantBox *self_box = self_variant(L);                   \
	if (!self_box) {                                          \
		err.format("Variant.%s: expected a Variant", m_name); \
		return 0;                                             \
	}

static bool class_allowed(lua_State *L, Object *p_object) {

	Runtime *rt = get_runtime(L);
	if (!rt || !p_object || !rt->is_sandboxed())
		return true;
	const Array &allowed = rt->get_sandbox_classes();
	if (allowed.size() == 0)
		return true;
	return allowed.has(p_object->get_type());
}

SUNABA_LUA_FUNC(variant_new) {

	{
		push_variant(L, lua_gettop(L) == 0 ? Variant() : to_variant(L, 1));
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_eq) {

	bool eq;
	{
		eq = to_variant(L, 1) == to_variant(L, 2);
	}
	lua_pushboolean(L, eq);
	return 1;
}

SUNABA_LUA_FUNC(variant_get_type) {

	VARIANT_SELF("getType");
	lua_pushinteger(L, to_godot4_type(self_box->value.get_type()));
	return 1;
}

// Static in Haxe (Variant.getTypeName(type)), but also accepts a Variant.
SUNABA_LUA_FUNC(variant_get_type_name) {

	int type;
	VariantBox *b = test_box(L, 1);
	if (b)
		type = to_godot4_type(b->value.get_type());
	else
		type = (int)lua_tointeger(L, 1);
	{
		CharString cs = godot4_type_name(type).utf8();
		lua_pushlstring(L, cs.get_data(), cs.length());
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_as_string) {

	VARIANT_SELF("asString");
	{
		CharString cs = String(self_box->value).utf8();
		lua_pushlstring(L, cs.get_data(), cs.length());
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_as_int) {

	VARIANT_SELF("asInt");
	lua_pushinteger(L, (int)self_box->value);
	return 1;
}

SUNABA_LUA_FUNC(variant_as_float) {

	VARIANT_SELF("asFloat");
	lua_pushnumber(L, (double)self_box->value);
	return 1;
}

SUNABA_LUA_FUNC(variant_as_bool) {

	VARIANT_SELF("asBool");
	lua_pushboolean(L, (bool)self_box->value);
	return 1;
}

// as<MathType>: upvalues are the target Variant::Type, truncate flag and
// metatable name.
SUNABA_LUA_FUNC(variant_as_math) {

	VARIANT_SELF("as*");
	Variant::Type type = (Variant::Type)lua_tointeger(L, lua_upvalueindex(1));
	bool truncate = lua_toboolean(L, lua_upvalueindex(2));
	const char *mt = lua_tostring(L, lua_upvalueindex(3));
	{
		const Variant *arg = &self_box->value;
		Variant::CallError ce;
		Variant ret = Variant::construct(type, &arg, 1, ce, false);
		if (ce.error != Variant::CallError::CALL_OK)
			ret = Variant::construct(type, NULL, 0, ce);
		if (truncate) {
			switch (type) {
				case Variant::VECTOR2: {
					Vector2 v = ret;
					ret = Vector2((int)v.x, (int)v.y);
				} break;
				case Variant::VECTOR3: {
					Vector3 v = ret;
					ret = Vector3((int)v.x, (int)v.y, (int)v.z);
				} break;
				case Variant::RECT2: {
					Rect2 r = ret;
					ret = Rect2((int)r.pos.x, (int)r.pos.y, (int)r.size.x, (int)r.size.y);
				} break;
				case Variant::QUAT: {
					Quat q = ret;
					ret = Quat((int)q.x, (int)q.y, (int)q.z, (int)q.w);
				} break;
				default: break;
			}
		}
		push_box(L, ret, mt);
	}
	return 1;
}

static int variant_unsupported(lua_State *L) {

	lua_pushnil(L);
	return 1;
}

SUNABA_LUA_FUNC(variant_as_object) {

	VARIANT_SELF("asObject");
	{
		if (self_box->value.get_type() != Variant::OBJECT) {
			lua_pushnil(L);
			return 1;
		}
		Object *o = self_box->value;
		if (!o || !class_allowed(L, o)) {
			lua_pushnil(L);
			return 1;
		}
		push_object(L, o);
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_as_reference) {

	VARIANT_SELF("asReference");
	{
		if (self_box->value.get_type() != Variant::OBJECT) {
			lua_pushnil(L);
			return 1;
		}
		Object *o = self_box->value;
		if (!o || !o->cast_to<Reference>() || !class_allowed(L, o)) {
			lua_pushnil(L);
			return 1;
		}
		push_object(L, o);
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_as_table) {

	VARIANT_SELF("asTable");
	{
		Object *o = self_box->value.get_type() == Variant::OBJECT ? (Object *)self_box->value : NULL;
		ScriptObject *so = o ? o->cast_to<ScriptObject>() : NULL;
		if (!so || !so->push_table())
			lua_pushnil(L);
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_as_function) {

	VARIANT_SELF("asFunction");
	{
		Object *o = self_box->value.get_type() == Variant::OBJECT ? (Object *)self_box->value : NULL;
		ScriptFunction *sf = o ? o->cast_to<ScriptFunction>() : NULL;
		if (!sf || !sf->push_function())
			lua_pushnil(L);
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_as_callable) {

	VARIANT_SELF("asCallable");
	{
		Object *o = self_box->value.get_type() == Variant::OBJECT ? (Object *)self_box->value : NULL;
		ScriptFunction *sf = o ? o->cast_to<ScriptFunction>() : NULL;
		if (!sf) {
			lua_pushnil(L);
			return 1;
		}
		CallableBox *c = (CallableBox *)lua_newuserdatauv(L, sizeof(CallableBox), 0);
		memnew_placement(c, CallableBox);
		c->target = sf->get_instance_ID();
		c->ref = Ref<Reference>(sf);
		c->method = "invoke";
		luaL_setmetatable(L, SUNABA_MT_CALLABLE);
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_as_byte_array) {

	VARIANT_SELF("asByteArray");
	{
		ByteArray raw;
		if (self_box->value.get_type() == Variant::RAW_ARRAY)
			raw = self_box->value;
		ByteArrayBox *ba = (ByteArrayBox *)lua_newuserdatauv(L, sizeof(ByteArrayBox), 0);
		memnew_placement(ba, ByteArrayBox);
		ba->cells.resize(raw.size());
		ByteArray::Read r = raw.read();
		for (int i = 0; i < raw.size(); i++) {
			int v = r[i];
			memset(ba->cells[i].data, 0, sizeof(ba->cells[i].data));
			memcpy(ba->cells[i].data, &v, sizeof(int));
		}
		luaL_setmetatable(L, SUNABA_MT_BYTEARRAY);
	}
	return 1;
}

// as*Array: upvalue is the accepted Variant::Type. Returns a 1-based Lua
// table (libsunaba's TypedArray indexes the table directly).
SUNABA_LUA_FUNC(variant_as_packed) {

	VARIANT_SELF("as*Array");
	Variant::Type type = (Variant::Type)lua_tointeger(L, lua_upvalueindex(1));
	{
		if (self_box->value.get_type() != type) {
			lua_newtable(L);
			return 1;
		}
		Array a = self_box->value;
		lua_createtable(L, a.size(), 0);
		for (int i = 0; i < a.size(); i++) {
			push_typed(L, a[i]);
			lua_rawseti(L, -2, i + 1);
		}
	}
	return 1;
}

static void push_as_packed(lua_State *L, const char *p_name, Variant::Type p_type) {

	lua_pushinteger(L, p_type);
	lua_pushcclosure(L, variant_as_packed, 1);
	lua_setfield(L, -2, p_name);
}

static void push_as_math(lua_State *L, const char *p_name, Variant::Type p_type, bool p_truncate, const char *p_mt) {

	lua_pushinteger(L, p_type);
	lua_pushboolean(L, p_truncate);
	lua_pushstring(L, p_mt);
	lua_pushcclosure(L, variant_as_math, 3);
	lua_setfield(L, -2, p_name);
}

/* from* (static) */

SUNABA_LUA_FUNC(variant_from_array_list) {

	{
		Array a;
		if (!to_array(L, 1, a)) {
			err.format("Variant.fromArrayList: expected an ArrayList");
			return 0;
		}
		push_variant(L, a);
	}
	return 1;
}

// from*Array: builds a packed array from a 1-based Lua table. Upvalue is
// the packed Variant::Type (or ARRAY for types Godot 2 cannot pack).
SUNABA_LUA_FUNC(variant_from_packed) {

	Variant::Type type = (Variant::Type)lua_tointeger(L, lua_upvalueindex(1));
	if (lua_type(L, 1) != LUA_TTABLE) {
		err.format("expected a table");
		return 0;
	}
	{
		Array a;
		to_array(L, 1, a);
		if (type == Variant::ARRAY) {
			push_variant(L, a);
		} else {
			const Variant *arg = NULL;
			Variant av = a;
			arg = &av;
			Variant::CallError ce;
			push_variant(L, Variant::construct(type, &arg, 1, ce, false));
		}
	}
	return 1;
}

static void push_from_packed(lua_State *L, const char *p_name, Variant::Type p_type) {

	lua_pushinteger(L, p_type);
	lua_pushcclosure(L, variant_from_packed, 1);
	lua_setfield(L, -2, p_name);
}

SUNABA_LUA_FUNC(variant_from_byte_array) {

	ByteArrayBox *ba = (ByteArrayBox *)test_kind(L, 1, KIND_BYTEARRAY);
	if (!ba) {
		err.format("Variant.fromByteArray: expected a ByteArray");
		return 0;
	}
	{
		push_variant(L, to_variant(L, 1));
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_from_string) {

	{
		push_variant(L, to_string(L, 1));
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_from_object) {

	if (get_kind(L, 1) != KIND_OBJECT) {
		err.format("Variant.fromObject: expected a NativeObject or NativeReference");
		return 0;
	}
	{
		push_variant(L, to_variant(L, 1));
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_from_int64) {

	{
		push_variant(L, (int)lua_tointeger(L, 1));
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_from_float64) {

	{
		push_variant(L, (double)lua_tonumber(L, 1));
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_from_table) {

	if (lua_type(L, 1) != LUA_TTABLE) {
		err.format("Variant.fromTable: expected a table");
		return 0;
	}
	{
		push_variant(L, to_variant(L, 1));
	}
	return 1;
}

SUNABA_LUA_FUNC(variant_from_function) {

	if (lua_type(L, 1) != LUA_TFUNCTION) {
		err.format("Variant.fromFunction: expected a function");
		return 0;
	}
	{
		push_variant(L, to_variant(L, 1));
	}
	return 1;
}

static const luaL_Reg variant_methods[] = {
	{ "new", variant_new },
	{ "eq", variant_eq },
	{ "getType", variant_get_type },
	{ "getTypeName", variant_get_type_name },
	{ "asString", variant_as_string },
	{ "tostring", variant_as_string },
	{ "asInt", variant_as_int },
	{ "asInt32", variant_as_int },
	{ "asFloat", variant_as_float },
	{ "asBool", variant_as_bool },
	{ "asProjection", variant_unsupported },
	{ "asSignal", variant_unsupported },
	{ "asObject", variant_as_object },
	{ "asReference", variant_as_reference },
	{ "asTable", variant_as_table },
	{ "asFunction", variant_as_function },
	{ "asCallable", variant_as_callable },
	{ "asByteArray", variant_as_byte_array },
	{ "fromArrayList", variant_from_array_list },
	{ "fromByteArray", variant_from_byte_array },
	{ "fromString", variant_from_string },
	{ "fromObject", variant_from_object },
	{ "fromReference", variant_from_object },
	{ "fromInt64", variant_from_int64 },
	{ "fromFloat64", variant_from_float64 },
	{ "fromTable", variant_from_table },
	{ "fromFunction", variant_from_function },
	{ NULL, NULL }
};

static const luaL_Reg variant_meta[] = {
	{ "__gc", box_gc_func },
	{ "__tostring", box_tostring_func },
	{ "__eq", box_eq_func },
	{ NULL, NULL }
};

void open_variant(lua_State *L, bool p_sandboxed, const Array &p_classnames) {

	new_type(L, SUNABA_MT_VARIANT, KIND_VARIANT_BOX, variant_methods, variant_meta);

	push_methods(L, SUNABA_MT_VARIANT);

	push_as_math(L, "asVector2", Variant::VECTOR2, false, SUNABA_MT_VECTOR2);
	push_as_math(L, "asVector2i", Variant::VECTOR2, true, SUNABA_MT_VECTOR2);
	push_as_math(L, "asRect2", Variant::RECT2, false, SUNABA_MT_RECT2);
	push_as_math(L, "asRect2i", Variant::RECT2, true, SUNABA_MT_RECT2);
	push_as_math(L, "asVector3", Variant::VECTOR3, false, SUNABA_MT_VECTOR3);
	push_as_math(L, "asVector3i", Variant::VECTOR3, true, SUNABA_MT_VECTOR3);
	push_as_math(L, "asTransform2D", Variant::MATRIX32, false, SUNABA_MT_TRANSFORM2D);
	push_as_math(L, "asVector4", Variant::QUAT, false, SUNABA_MT_QUATERNION);
	push_as_math(L, "asVector4i", Variant::QUAT, true, SUNABA_MT_QUATERNION);
	push_as_math(L, "asPlane", Variant::PLANE, false, SUNABA_MT_PLANE);
	push_as_math(L, "asQuaternion", Variant::QUAT, false, SUNABA_MT_QUATERNION);
	push_as_math(L, "asAABB", Variant::_AABB, false, SUNABA_MT_AABB);
	push_as_math(L, "asBasis", Variant::MATRIX3, false, SUNABA_MT_BASIS);
	push_as_math(L, "asTransform3D", Variant::TRANSFORM, false, SUNABA_MT_TRANSFORM3D);
	push_as_math(L, "asColor", Variant::COLOR, false, SUNABA_MT_COLOR);
	push_as_math(L, "asArrayList", Variant::ARRAY, false, SUNABA_MT_ARRAYLIST);
	push_as_math(L, "asDictionary", Variant::DICTIONARY, false, SUNABA_MT_DICTIONARY);

	push_as_packed(L, "asIntArray", Variant::INT_ARRAY);
	push_as_packed(L, "asIntArray64", Variant::INT_ARRAY);
	push_as_packed(L, "asFloatArray", Variant::REAL_ARRAY);
	push_as_packed(L, "asFloatArray64", Variant::REAL_ARRAY);
	push_as_packed(L, "asStringArray", Variant::STRING_ARRAY);
	push_as_packed(L, "asVector2Array", Variant::VECTOR2_ARRAY);
	push_as_packed(L, "asVector3Array", Variant::VECTOR3_ARRAY);
	push_as_packed(L, "asColorArray", Variant::COLOR_ARRAY);
	// Godot 2 has no PackedVector4Array; Vector4 arrays are plain Arrays.
	push_as_packed(L, "asVector4Array", Variant::ARRAY);

	push_from_packed(L, "fromIntArray", Variant::INT_ARRAY);
	push_from_packed(L, "fromIntArray64", Variant::INT_ARRAY);
	push_from_packed(L, "fromFloatArray", Variant::REAL_ARRAY);
	push_from_packed(L, "fromFloatArray64", Variant::REAL_ARRAY);
	push_from_packed(L, "fromStringArray", Variant::STRING_ARRAY);
	push_from_packed(L, "fromVector2Array", Variant::VECTOR2_ARRAY);
	push_from_packed(L, "fromVector3Array", Variant::VECTOR3_ARRAY);
	push_from_packed(L, "fromColorArray", Variant::COLOR_ARRAY);
	push_from_packed(L, "fromVector4Array", Variant::ARRAY);

	lua_pop(L, 1);

	(void)p_sandboxed;
	(void)p_classnames;
}

} // namespace sunaba
