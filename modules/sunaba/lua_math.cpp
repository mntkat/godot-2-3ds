/*************************************************************************/
/*  lua_math.cpp                                                         */
/*************************************************************************/
/*  Vector2, Rect2, Vector3, Transform2D, Plane, Quaternion, AABB, Basis, */
/*  Transform3D and Color.                                               */
/*                                                                       */
/*  libsunaba bound each method by hand with sol2. Here members and      */
/*  methods resolve through Godot 2's Variant reflection instead, with   */
/*  camelCase names mapped to snake_case and a table of Godot 4 -> 2     */
/*  renames. Methods Godot 2 lacks are filled in by the Lua prelude.     */
/*                                                                       */
/*  Godot 2 has no integer vectors, Vector4 or Projection:               */
/*  Vector2i/Vector3i/Rect2i are truncated Vector2/Vector3/Rect2,        */
/*  Vector4/Vector4i are stored as Quaternion, Projection is absent.     */
/*************************************************************************/

#include "lua_bridge.h"

#include "math_funcs.h"

namespace sunaba {

struct NameAlias {
	Variant::Type type;
	const char *lua;
	const char *godot;
};

static const NameAlias member_aliases[] = {
	{ Variant::RECT2, "position", "pos" },
	{ Variant::_AABB, "position", "pos" },
	{ Variant::MATRIX32, "origin", "o" },
	{ Variant::NIL, NULL, NULL }
};

static const NameAlias method_aliases[] = {
	{ Variant::VECTOR2, "lerp", "linear_interpolate" },
	{ Variant::VECTOR2, "limitLength", "clamped" },
	{ Variant::VECTOR2, "orthogonal", "tangent" },
	{ Variant::VECTOR2, "aspect", "get_aspect" },
	{ Variant::VECTOR2, "snap", "snapped" },
	{ Variant::VECTOR3, "lerp", "linear_interpolate" },
	{ Variant::VECTOR3, "minAxisIndex", "min_axis" },
	{ Variant::VECTOR3, "maxAxisIndex", "max_axis" },
	{ Variant::RECT2, "intersection", "clip" },
	{ Variant::RECT2, "growSide", "grow_margin" },
	{ Variant::_AABB, "getVolume", "get_area" },
	{ Variant::PLANE, "getCenter", "center" },
	{ Variant::PLANE, "intersect3", "intersect_3" },
	{ Variant::PLANE, "intersectRay", "intersects_ray" },
	{ Variant::QUAT, "sphericalCubicInterpolate", "cubic_slerp" },
	{ Variant::COLOR, "lerp", "linear_interpolate" },
	{ Variant::COLOR, "toArgb32", "to_ARGB32" },
	{ Variant::COLOR, "toRgba32", "to_32" },
	{ Variant::NIL, NULL, NULL }
};

static String camel_to_snake(const char *p_name) {

	String ret;
	for (const char *c = p_name; *c; c++) {
		if (*c >= 'A' && *c <= 'Z') {
			if (c != p_name)
				ret += "_";
			ret += String::chr(*c - 'A' + 'a');
		} else {
			ret += String::chr(*c);
		}
	}
	return ret;
}

static String map_name(const NameAlias *p_aliases, Variant::Type p_type, const char *p_name, bool p_snake) {

	for (const NameAlias *a = p_aliases; a->lua; a++) {
		if (a->type == p_type && strcmp(a->lua, p_name) == 0)
			return a->godot;
	}
	return p_snake ? camel_to_snake(p_name) : String(p_name);
}

/* Shared box metamethods */

static int box_gc(lua_State *L) {

	VariantBox *b = (VariantBox *)lua_touserdata(L, 1);
	if (b)
		b->~VariantBox();
	return 0;
}

SUNABA_LUA_FUNC(box_tostring) {

	VariantBox *b = test_box(L, 1);
	if (!b) {
		err.format("expected a boxed value");
		return 0;
	}
	String s;
	switch (b->value.get_type()) {
		case Variant::DICTIONARY:
			s = b->value.operator Dictionary().to_json();
			break;
		case Variant::ARRAY:
			s = "<Array size=" + itos(b->value.operator Array().size()) + ">";
			break;
		default:
			s = b->value;
	}
	CharString cs = s.utf8();
	lua_pushlstring(L, cs.get_data(), cs.length());
	return 1;
}

SUNABA_LUA_FUNC(box_eq) {

	VariantBox *a = test_box(L, 1);
	VariantBox *b = test_box(L, 2);
	lua_pushboolean(L, a && b && a->value == b->value);
	return 1;
}

SUNABA_LUA_FUNC(box_method_call) {

	const char *method = lua_tostring(L, lua_upvalueindex(1));
	VariantBox *b = test_box(L, 1);
	if (!b) {
		err.format("method '%s' called without a valid self (use ':' to call methods)", method);
		return 0;
	}

	int argc = lua_gettop(L) - 1;
	{
		Vector<Variant> args;
		args.resize(argc);
		Vector<const Variant *> argptrs;
		argptrs.resize(argc);
		for (int i = 0; i < argc; i++) {
			args[i] = to_variant(L, i + 2);
			argptrs[i] = &args[i];
		}

		Variant::CallError ce;
		Variant ret = b->value.call(method, argc ? &argptrs[0] : NULL, argc, ce);
		if (ce.error != Variant::CallError::CALL_OK) {
			err.format("%s", call_error_text(ce, method).utf8().get_data());
			return 0;
		}
		push_typed(L, ret);
	}
	return 1;
}

// __index for math boxes: explicit methods, then cached Godot methods,
// then members (x, y, pos, ...), then Godot methods by mapped name.
SUNABA_LUA_FUNC(box_index) {

	lua_getmetatable(L, 1);
	int mt = lua_gettop(L);

	lua_getfield(L, mt, SUNABA_METHODS_KEY);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnil(L, -1))
		return 1;
	lua_pop(L, 2);

	if (lua_type(L, 2) != LUA_TSTRING) {
		lua_pushnil(L);
		return 1;
	}

	lua_getfield(L, mt, "__cache");
	int cache = lua_gettop(L);
	lua_pushvalue(L, 2);
	lua_rawget(L, cache);
	if (!lua_isnil(L, -1))
		return 1;
	lua_pop(L, 1);

	VariantBox *b = test_box(L, 1);
	if (!b) {
		lua_pushnil(L);
		return 1;
	}

	const char *key = lua_tostring(L, 2);
	Variant::Type type = b->value.get_type();
	{
		bool valid = false;
		Variant member = b->value.get(map_name(member_aliases, type, key, false), &valid);
		if (valid) {
			push_typed(L, member);
			return 1;
		}

		String method = map_name(method_aliases, type, key, true);
		if (b->value.has_method(method)) {
			CharString cs = method.utf8();
			lua_pushstring(L, cs.get_data());
			lua_pushcclosure(L, box_method_call, 1);
			lua_pushvalue(L, 2);
			lua_pushvalue(L, -2);
			lua_rawset(L, cache);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

SUNABA_LUA_FUNC(box_newindex) {

	VariantBox *b = test_box(L, 1);
	if (!b || lua_type(L, 2) != LUA_TSTRING) {
		err.format("invalid field assignment");
		return 0;
	}
	const char *key = lua_tostring(L, 2);
	{
		bool valid = false;
		b->value.set(map_name(member_aliases, b->value.get_type(), key, false), to_variant(L, 3), &valid);
		if (!valid)
			err.format("cannot set field '%s' on %s", key, Variant::get_type_name(b->value.get_type()).utf8().get_data());
	}
	return 0;
}

SUNABA_LUA_FUNC(box_arith) {

	Variant::Operator op = (Variant::Operator)lua_tointeger(L, lua_upvalueindex(1));
	{
		Variant a = to_variant(L, 1);
		// Unary minus is called with the operand twice.
		Variant b = op == Variant::OP_NEGATE ? Variant() : to_variant(L, 2);
		Variant ret;
		bool valid = false;
		Variant::evaluate(op, a, b, ret, valid);
		if (!valid) {
			err.format("invalid operands for arithmetic: %s and %s",
					Variant::get_type_name(a.get_type()).utf8().get_data(),
					Variant::get_type_name(b.get_type()).utf8().get_data());
			return 0;
		}
		push_typed(L, ret);
	}
	return 1;
}

static void push_arith(lua_State *L, int p_mt, const char *p_event, Variant::Operator p_op) {

	lua_pushinteger(L, p_op);
	lua_pushcclosure(L, box_arith, 1);
	lua_setfield(L, p_mt, p_event);
}

/* Explicit methods shared by every math type */

static Variant truncate_components(const Variant &p_value) {

	switch (p_value.get_type()) {
		case Variant::VECTOR2: {
			Vector2 v = p_value;
			return Vector2((int)v.x, (int)v.y);
		}
		case Variant::VECTOR3: {
			Vector3 v = p_value;
			return Vector3((int)v.x, (int)v.y, (int)v.z);
		}
		case Variant::RECT2: {
			Rect2 r = p_value;
			return Rect2((int)r.pos.x, (int)r.pos.y, (int)r.size.x, (int)r.size.y);
		}
		case Variant::QUAT: {
			Quat q = p_value;
			return Quat((int)q.x, (int)q.y, (int)q.z, (int)q.w);
		}
		default:
			return p_value;
	}
}

// Upvalues: Variant::Type, truncate (bool), metatable name.
SUNABA_LUA_FUNC(type_new) {

	Variant::Type type = (Variant::Type)lua_tointeger(L, lua_upvalueindex(1));
	bool truncate = lua_toboolean(L, lua_upvalueindex(2));
	const char *mt = lua_tostring(L, lua_upvalueindex(3));

	int argc = lua_gettop(L);
	{
		Vector<Variant> args;
		args.resize(argc);
		Vector<const Variant *> argptrs;
		argptrs.resize(argc);
		for (int i = 0; i < argc; i++) {
			args[i] = to_variant(L, i + 1);
			argptrs[i] = &args[i];
		}

		Variant::CallError ce;
		Variant ret = Variant::construct(type, argc ? &argptrs[0] : NULL, argc, ce, false);
		if (ce.error != Variant::CallError::CALL_OK) {
			err.format("invalid arguments for %s.new", mt);
			return 0;
		}
		if (truncate)
			ret = truncate_components(ret);
		push_box(L, ret, mt);
	}
	return 1;
}

SUNABA_LUA_FUNC(box_tostring_method) {

	VariantBox *b = test_box(L, 1);
	if (!b) {
		err.format("tostring: expected a boxed value");
		return 0;
	}
	{
		CharString cs = String(b->value).utf8();
		lua_pushlstring(L, cs.get_data(), cs.length());
	}
	return 1;
}

static int compare(lua_State *L, Variant::Operator p_op, bool p_swap) {

	bool result = false;
	{
		Variant a = to_variant(L, p_swap ? 2 : 1);
		Variant b = to_variant(L, p_swap ? 1 : 2);
		Variant ret;
		bool valid = false;
		Variant::evaluate(p_op, a, b, ret, valid);
		result = valid && (bool)ret;
	}
	lua_pushboolean(L, result);
	return 1;
}

static int box_eq_method(lua_State *L) { return compare(L, Variant::OP_EQUAL, false); }
static int box_gt(lua_State *L) { return compare(L, Variant::OP_LESS, true); }
static int box_lt(lua_State *L) { return compare(L, Variant::OP_LESS, false); }
static int box_gte(lua_State *L) { return compare(L, Variant::OP_LESS_EQUAL, true); }
static int box_lte(lua_State *L) { return compare(L, Variant::OP_LESS_EQUAL, false); }

static const luaL_Reg common_methods[] = {
	{ "tostring", box_tostring_method },
	{ "eq", box_eq_method },
	{ NULL, NULL }
};

static const luaL_Reg ordered_methods[] = {
	{ "gt", box_gt },
	{ "lt", box_lt },
	{ "gte", box_gte },
	{ "lte", box_lte },
	{ NULL, NULL }
};

static const luaL_Reg math_meta[] = {
	{ "__gc", box_gc },
	{ "__index", box_index },
	{ "__newindex", box_newindex },
	{ "__tostring", box_tostring },
	{ "__eq", box_eq },
	{ NULL, NULL }
};

static void set_constructor(lua_State *L, const char *p_global, Variant::Type p_type, bool p_truncate, const char *p_metatable) {

	lua_getglobal(L, p_global);
	lua_pushinteger(L, p_type);
	lua_pushboolean(L, p_truncate);
	lua_pushstring(L, p_metatable);
	lua_pushcclosure(L, type_new, 3);
	lua_setfield(L, -2, "new");
	lua_pop(L, 1);
}

static void register_math_type(lua_State *L, const char *p_name, Variant::Type p_type, const luaL_Reg *p_extra) {

	new_type(L, p_name, KIND_VARIANT_BOX, common_methods, math_meta);

	luaL_getmetatable(L, p_name);
	int mt = lua_gettop(L);
	push_arith(L, mt, "__add", Variant::OP_ADD);
	push_arith(L, mt, "__sub", Variant::OP_SUBSTRACT);
	push_arith(L, mt, "__mul", Variant::OP_MULTIPLY);
	push_arith(L, mt, "__div", Variant::OP_DIVIDE);
	push_arith(L, mt, "__unm", Variant::OP_NEGATE);
	push_arith(L, mt, "__lt", Variant::OP_LESS);
	push_arith(L, mt, "__le", Variant::OP_LESS_EQUAL);
	lua_pop(L, 1);

	if (p_extra) {
		push_methods(L, p_name);
		luaL_setfuncs(L, p_extra, 0);
		lua_pop(L, 1);
	}

	set_constructor(L, p_name, p_type, false, p_name);
}

// Publishes a global (e.g. Vector2i) whose `new` builds a truncated value
// of an existing type; instances use that type's metatable.
static void register_alias(lua_State *L, const char *p_alias, const char *p_target, Variant::Type p_type, bool p_truncate) {

	lua_newtable(L);
	lua_newtable(L);
	push_methods(L, p_target);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);
	lua_pushinteger(L, p_type);
	lua_pushboolean(L, p_truncate);
	lua_pushstring(L, p_target);
	lua_pushcclosure(L, type_new, 3);
	lua_setfield(L, -2, "new");
	lua_setglobal(L, p_alias);
}

/* Color statics */

static Color color_from_string(const String &p_str, const Color &p_default) {

	if (Color::html_is_valid(p_str))
		return Color::html(p_str);
	return p_default;
}

SUNABA_LUA_FUNC(color_html) {

	{
		Color c = Color::html(to_string(L, 1));
		push_box(L, c, SUNABA_MT_COLOR);
	}
	return 1;
}

SUNABA_LUA_FUNC(color_html_is_valid) {

	bool valid;
	{
		valid = Color::html_is_valid(to_string(L, 1));
	}
	lua_pushboolean(L, valid);
	return 1;
}

SUNABA_LUA_FUNC(color_code) {

	{
		Color c = Color::html(to_string(L, 1));
		if (lua_gettop(L) >= 2)
			c.a = lua_tonumber(L, 2);
		push_box(L, c, SUNABA_MT_COLOR);
	}
	return 1;
}

SUNABA_LUA_FUNC(color_from_string_l) {

	{
		Color def;
		VariantBox *d = test_box(L, 2);
		if (d && d->value.get_type() == Variant::COLOR)
			def = d->value;
		push_box(L, color_from_string(to_string(L, 1), def), SUNABA_MT_COLOR);
	}
	return 1;
}

SUNABA_LUA_FUNC(color_from_hsv) {

	{
		Color c;
		c.set_hsv(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_isnoneornil(L, 4) ? 1.0 : lua_tonumber(L, 4));
		push_box(L, c, SUNABA_MT_COLOR);
	}
	return 1;
}

SUNABA_LUA_FUNC(color_hex) {

	{
		push_box(L, Color::hex((uint32_t)lua_tointeger(L, 1)), SUNABA_MT_COLOR);
	}
	return 1;
}

SUNABA_LUA_FUNC(color_to_html) {

	VariantBox *b = test_box(L, 1);
	if (!b || b->value.get_type() != Variant::COLOR) {
		err.format("toHtml: expected a Color");
		return 0;
	}
	{
		Color c = b->value;
		bool alpha = lua_isnoneornil(L, 2) ? true : lua_toboolean(L, 2);
		CharString cs = c.to_html(alpha).utf8();
		lua_pushlstring(L, cs.get_data(), cs.length());
	}
	return 1;
}

static const luaL_Reg color_methods[] = {
	{ "html", color_html },
	{ "htmlIsValid", color_html_is_valid },
	{ "code", color_code },
	{ "fromString", color_from_string_l },
	{ "fromHSV", color_from_hsv },
	{ "hex", color_hex },
	{ "toHtml", color_to_html },
	{ NULL, NULL }
};

/* Basis statics */

SUNABA_LUA_FUNC(basis_from_euler) {

	VariantBox *b = test_box(L, 1);
	{
		Vector3 euler = b ? Vector3(b->value) : Vector3();
		push_box(L, Matrix3(euler), SUNABA_MT_BASIS);
	}
	return 1;
}

SUNABA_LUA_FUNC(basis_from_scale) {

	VariantBox *b = test_box(L, 1);
	{
		Matrix3 m;
		m.scale(b ? Vector3(b->value) : Vector3(1, 1, 1));
		push_box(L, m, SUNABA_MT_BASIS);
	}
	return 1;
}

SUNABA_LUA_FUNC(basis_get_rotation_quaternion) {

	VariantBox *b = test_box(L, 1);
	if (!b || b->value.get_type() != Variant::MATRIX3) {
		err.format("getRotationQuaternion: expected a Basis");
		return 0;
	}
	{
		Matrix3 m = b->value;
		push_box(L, Quat(m.orthonormalized()), SUNABA_MT_QUATERNION);
	}
	return 1;
}

static const luaL_Reg basis_methods[] = {
	{ "fromEuler", basis_from_euler },
	{ "fromScale", basis_from_scale },
	{ "getRotationQuaternion", basis_get_rotation_quaternion },
	{ NULL, NULL }
};

void open_math(lua_State *L) {

	register_math_type(L, SUNABA_MT_VECTOR2, Variant::VECTOR2, ordered_methods);
	register_math_type(L, SUNABA_MT_RECT2, Variant::RECT2, NULL);
	register_math_type(L, SUNABA_MT_VECTOR3, Variant::VECTOR3, ordered_methods);
	register_math_type(L, SUNABA_MT_TRANSFORM2D, Variant::MATRIX32, NULL);
	register_math_type(L, SUNABA_MT_PLANE, Variant::PLANE, NULL);
	register_math_type(L, SUNABA_MT_QUATERNION, Variant::QUAT, NULL);
	register_math_type(L, SUNABA_MT_AABB, Variant::_AABB, NULL);
	register_math_type(L, SUNABA_MT_BASIS, Variant::MATRIX3, basis_methods);
	register_math_type(L, SUNABA_MT_TRANSFORM3D, Variant::TRANSFORM, NULL);
	register_math_type(L, SUNABA_MT_COLOR, Variant::COLOR, color_methods);

	register_alias(L, "Vector2i", SUNABA_MT_VECTOR2, Variant::VECTOR2, true);
	register_alias(L, "Vector3i", SUNABA_MT_VECTOR3, Variant::VECTOR3, true);
	register_alias(L, "Rect2i", SUNABA_MT_RECT2, Variant::RECT2, true);
	register_alias(L, "Vector4", SUNABA_MT_QUATERNION, Variant::QUAT, false);
	register_alias(L, "Vector4i", SUNABA_MT_QUATERNION, Variant::QUAT, true);
}

/* Shared with the other box types */

int box_gc_func(lua_State *L) {

	return box_gc(L);
}

int box_tostring_func(lua_State *L) {

	return box_tostring(L);
}

int box_eq_func(lua_State *L) {

	return box_eq(L);
}

} // namespace sunaba
