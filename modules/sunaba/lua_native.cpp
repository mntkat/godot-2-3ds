/*************************************************************************/
/*  lua_native.cpp                                                       */
/*************************************************************************/
/*  NativeObject / NativeReference: handles to engine objects.           */
/*                                                                       */
/*  Differences from libsunaba on Godot 4:                               */
/*  - scriptType 2 (C#) is unsupported; Godot 2 has no Mono.             */
/*  - callStatic only reaches engine singletons; Godot 2 has no static   */
/*    class methods (ClassDB.class_call_static).                         */
/*  - getService looks up Globals singletons (OS, Input, Globals, ...).  */
/*************************************************************************/

#include "lua_bridge.h"

#include "globals.h"
#include "godot4_compat.h"
#include "input_event_ref.h"
#include "io/resource_loader.h"
#include "lua_runtime.h"
#include "object_type_db.h"
#include "script_language.h"

namespace sunaba {

enum ScriptType {
	SCRIPT_NONE = 0,
	SCRIPT_GDSCRIPT = 1,
	SCRIPT_CSHARP = 2,
};

static bool sandbox_allows(lua_State *L, const String &p_class) {

	Runtime *rt = get_runtime(L);
	if (!rt || !rt->is_sandboxed())
		return true;
	return rt->get_sandbox_classes().has(p_class);
}

static ObjectBox *check_object(lua_State *L, int p_idx) {

	return (ObjectBox *)test_kind(L, p_idx, KIND_OBJECT);
}

#define OBJECT_SELF(m_name)                                                   \
	ObjectBox *self_box = check_object(L, 1);                                 \
	if (!self_box) {                                                          \
		err.format("%s: expected a NativeObject (use ':' to call methods)", m_name); \
		return 0;                                                             \
	}                                                                         \
	Object *self = self_box->get();

static void push_null_object(lua_State *L, const char *p_metatable) {

	ObjectBox *b = (ObjectBox *)lua_newuserdatauv(L, sizeof(ObjectBox), 0);
	memnew_placement(b, ObjectBox);
	b->id = 0;
	luaL_setmetatable(L, p_metatable);
}

static Object *create_object(const String &p_name, const Array &p_args, int p_script_type, Ref<Reference> &r_ref) {

	Object *obj = NULL;
	switch (p_script_type) {
		case SCRIPT_NONE:
			obj = ObjectTypeDB::instance(compat::to_godot2_class(p_name));
			break;
		case SCRIPT_GDSCRIPT: {
			RES res = ResourceLoader::load(p_name);
			Ref<Script> script = res;
			if (script.is_valid()) {
				Variant v = script->callv("new", p_args);
				obj = v;
				if (obj && obj->cast_to<Reference>())
					r_ref = Ref<Reference>(obj->cast_to<Reference>());
				return obj;
			}
			ERR_PRINT(String("Could not load script: " + p_name).utf8().get_data());
		} break;
		case SCRIPT_CSHARP:
			ERR_PRINT("C# scripts are not supported by the Godot 2 runtime.");
			break;
	}
	if (obj && obj->cast_to<Reference>())
		r_ref = Ref<Reference>(obj->cast_to<Reference>());
	return obj;
}

// new(name, [args], [scriptType]); upvalue 1 is the metatable name.
SUNABA_LUA_FUNC(native_new) {

	const char *mt = lua_tostring(L, lua_upvalueindex(1));
	{
		String name = to_string(L, 1);
		if (!sandbox_allows(L, name) && !sandbox_allows(L, compat::to_godot2_class(name))) {
			lua_pushnil(L);
			return 1;
		}
		Array args;
		if (!to_array(L, 2, args)) {
			err.format("%s.new: args must be an ArrayList", mt);
			return 0;
		}
		int script_type = lua_isnoneornil(L, 3) ? SCRIPT_NONE : (int)lua_tointeger(L, 3);

		Ref<Reference> ref;
		Object *obj = create_object(name, args, script_type, ref);
		if (!obj) {
			push_null_object(L, mt);
			return 1;
		}
		ObjectBox *b = (ObjectBox *)lua_newuserdatauv(L, sizeof(ObjectBox), 0);
		memnew_placement(b, ObjectBox);
		b->id = obj->get_instance_ID();
		b->ref = ref;
		luaL_setmetatable(L, mt);
	}
	return 1;
}

SUNABA_LUA_FUNC(native_call_static) {

	{
		String cls = to_string(L, 1);
		String method = to_string(L, 2);
		if (!sandbox_allows(L, cls)) {
			push_variant(L, Variant());
			return 1;
		}
		Array args;
		if (!to_array(L, 3, args)) {
			err.format("NativeObject.callStatic: args must be an ArrayList");
			return 0;
		}
		Object *singleton = Globals::get_singleton()->has_singleton(cls) ? Globals::get_singleton()->get_singleton_object(cls) : NULL;
		if (!singleton) {
			ERR_PRINT(String("callStatic(" + cls + ", " + method + "): Godot 2 has no static class methods").utf8().get_data());
			push_variant(L, Variant());
			return 1;
		}
		push_variant(L, singleton->callv(method, args));
	}
	return 1;
}

SUNABA_LUA_FUNC(native_get_service) {

	{
		String cls = to_string(L, 1);
		if (!sandbox_allows(L, cls)) {
			lua_pushnil(L);
			return 1;
		}
		Object *singleton = Globals::get_singleton()->has_singleton(cls) ? Globals::get_singleton()->get_singleton_object(cls) : NULL;
		if (singleton) {
			ObjectBox *b = (ObjectBox *)lua_newuserdatauv(L, sizeof(ObjectBox), 0);
			memnew_placement(b, ObjectBox);
			b->id = singleton->get_instance_ID();
			luaL_setmetatable(L, SUNABA_MT_NATIVEOBJECT);
		} else {
			push_null_object(L, SUNABA_MT_NATIVEOBJECT);
		}
	}
	return 1;
}

SUNABA_LUA_FUNC(native_call) {

	OBJECT_SELF("call");
	{
		String method = to_string(L, 2);
		Array args;
		if (!to_array(L, 3, args)) {
			err.format("call: args must be an ArrayList");
			return 0;
		}
		Variant ret;
		if (self)
			compat::call(self, method, args, ret);
		push_variant(L, ret);
	}
	return 1;
}

SUNABA_LUA_FUNC(native_get) {

	OBJECT_SELF("get");
	{
		push_variant(L, self ? compat::get(self, to_string(L, 2)) : Variant());
	}
	return 1;
}

SUNABA_LUA_FUNC(native_set) {

	OBJECT_SELF("set");
	if (self) {
		compat::set(self, to_string(L, 2), to_variant(L, 3));
	}
	return 0;
}

SUNABA_LUA_FUNC(native_get_class) {

	OBJECT_SELF("getClass");
	{
		String cls = self ? compat::godot4_class(self) : String();
		InputEventRef *ev = self ? self->cast_to<InputEventRef>() : NULL;
		if (ev)
			cls = ev->get_godot4_class();
		CharString cs = cls.utf8();
		lua_pushlstring(L, cs.get_data(), cs.length());
	}
	return 1;
}

SUNABA_LUA_FUNC(native_is_class) {

	OBJECT_SELF("isClass");
	bool is;
	{
		String cls = to_string(L, 2);
		InputEventRef *ev = self ? self->cast_to<InputEventRef>() : NULL;
		is = ev ? ev->is_godot4_class(cls) : (self && compat::is_class(self, cls));
	}
	lua_pushboolean(L, is);
	return 1;
}

SUNABA_LUA_FUNC(native_get_meta) {

	OBJECT_SELF("getMeta");
	{
		String name = to_string(L, 2);
		Variant def = to_variant(L, 3);
		push_variant(L, (self && self->has_meta(name)) ? self->get_meta(name) : def);
	}
	return 1;
}

SUNABA_LUA_FUNC(native_has_meta) {

	OBJECT_SELF("hasMeta");
	bool has;
	{
		has = self && self->has_meta(to_string(L, 2));
	}
	lua_pushboolean(L, has);
	return 1;
}

SUNABA_LUA_FUNC(native_set_meta) {

	OBJECT_SELF("setMeta");
	if (self) {
		self->set_meta(to_string(L, 2), to_variant(L, 3));
	}
	return 0;
}

// Pushes the Array returned by a bound Object method as an ArrayList.
static int push_bound_list(lua_State *L, Object *p_self, const char *p_method) {

	{
		Array list = p_self ? p_self->call(p_method).operator Array() : Array();
		push_box(L, list, SUNABA_MT_ARRAYLIST);
	}
	return 1;
}

SUNABA_LUA_FUNC(native_get_meta_list) {

	OBJECT_SELF("getMetaList");
	return push_bound_list(L, self, "get_meta_list");
}

SUNABA_LUA_FUNC(native_get_method_list) {

	OBJECT_SELF("getMethodList");
	return push_bound_list(L, self, "get_method_list");
}

SUNABA_LUA_FUNC(native_get_property_list) {

	OBJECT_SELF("getPropertyList");
	return push_bound_list(L, self, "get_property_list");
}

SUNABA_LUA_FUNC(native_get_method_argument_count) {

	OBJECT_SELF("getMethodArgumentCount");
	int count = 0;
	{
		String method = to_string(L, 2);
		if (self) {
			List<MethodInfo> methods;
			self->get_method_list(&methods);
			for (List<MethodInfo>::Element *E = methods.front(); E; E = E->next()) {
				if (E->get().name == method) {
					count = E->get().arguments.size();
					break;
				}
			}
		}
	}
	lua_pushinteger(L, count);
	return 1;
}

SUNABA_LUA_FUNC(native_has_method) {

	OBJECT_SELF("hasMethod");
	bool has;
	{
		has = self && compat::has_method(self, to_string(L, 2));
	}
	lua_pushboolean(L, has);
	return 1;
}

SUNABA_LUA_FUNC(native_is_null) {

	OBJECT_SELF("isNull");
	lua_pushboolean(L, self == NULL);
	return 1;
}

SUNABA_LUA_FUNC(native_is_valid) {

	OBJECT_SELF("isValid");
	lua_pushboolean(L, self != NULL && self_box->ref.is_valid());
	return 1;
}

SUNABA_LUA_FUNC(native_free) {

	OBJECT_SELF("free");
	if (self && !self->cast_to<Reference>()) {
		self_box->id = 0;
		memdelete(self);
	}
	return 0;
}

static int native_eq(lua_State *L) {

	ObjectBox *a = check_object(L, 1);
	ObjectBox *b = check_object(L, 2);
	lua_pushboolean(L, a && b && a->get() == b->get());
	return 1;
}

SUNABA_LUA_FUNC(native_tostring) {

	OBJECT_SELF("tostring");
	{
		String s = self ? "[" + self->get_type() + ":" + itos(self->get_instance_ID()) + "]" : String("[null]");
		CharString cs = s.utf8();
		lua_pushlstring(L, cs.get_data(), cs.length());
	}
	return 1;
}

static int native_gc(lua_State *L) {

	ObjectBox *b = (ObjectBox *)lua_touserdata(L, 1);
	if (b)
		b->~ObjectBox();
	return 0;
}

#define COMMON_NATIVE_METHODS                                    \
	{ "call", native_call },                                     \
			{ "get", native_get },                               \
			{ "set", native_set },                               \
			{ "getClass", native_get_class },                    \
			{ "isClass", native_is_class },                      \
			{ "getMeta", native_get_meta },                      \
			{ "getMetaList", native_get_meta_list },             \
			{ "getMethodArgumentCount", native_get_method_argument_count }, \
			{ "getMethodList", native_get_method_list },         \
			{ "getPropertyList", native_get_property_list },     \
			{ "hasMeta", native_has_meta },                      \
			{ "hasMethod", native_has_method },                  \
			{ "setMeta", native_set_meta },                      \
			{ "isNull", native_is_null },                        \
			{ "eq", native_eq },                                 \
			{ "tostring", native_tostring }

static const luaL_Reg object_methods[] = {
	COMMON_NATIVE_METHODS,
	{ "callStatic", native_call_static },
	{ "getService", native_get_service },
	{ "free", native_free },
	{ NULL, NULL }
};

static const luaL_Reg reference_methods[] = {
	COMMON_NATIVE_METHODS,
	{ "isValid", native_is_valid },
	{ NULL, NULL }
};

static const luaL_Reg native_meta[] = {
	{ "__gc", native_gc },
	{ "__eq", native_eq },
	{ "__tostring", native_tostring },
	{ NULL, NULL }
};

static void set_new(lua_State *L, const char *p_name) {

	push_methods(L, p_name);
	lua_pushstring(L, p_name);
	lua_pushcclosure(L, native_new, 1);
	lua_setfield(L, -2, "new");
	lua_pop(L, 1);
}

void open_native(lua_State *L, bool p_sandboxed, const Array &p_classnames) {

	new_type(L, SUNABA_MT_NATIVEOBJECT, KIND_OBJECT, object_methods, native_meta);
	new_type(L, SUNABA_MT_NATIVEREFERENCE, KIND_OBJECT, reference_methods, native_meta);
	set_new(L, SUNABA_MT_NATIVEOBJECT);
	set_new(L, SUNABA_MT_NATIVEREFERENCE);

	(void)p_sandboxed;
	(void)p_classnames;
}

} // namespace sunaba
