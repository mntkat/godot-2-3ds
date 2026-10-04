/*************************************************************************/
/*  script_object.cpp                                                    */
/*************************************************************************/

#include "script_object.h"

#include "lua_bridge.h"

static const char *HANDLE_KEY = "sunaba.handle";

/* LuaStateHandle */

LuaStateHandle *LuaStateHandle::create(lua_State *p_L) {

	LuaStateHandle *h = memnew(LuaStateHandle);
	h->L = p_L;
	h->refcount = 1;
	lua_pushlightuserdata(p_L, h);
	lua_setfield(p_L, LUA_REGISTRYINDEX, HANDLE_KEY);
	return h;
}

void LuaStateHandle::reference() {

	refcount++;
}

void LuaStateHandle::unreference() {

	refcount--;
	if (refcount == 0)
		memdelete(this);
}

static LuaStateHandle *handle_from_state(lua_State *p_L) {

	lua_getfield(p_L, LUA_REGISTRYINDEX, HANDLE_KEY);
	LuaStateHandle *h = (LuaStateHandle *)lua_touserdata(p_L, -1);
	lua_pop(p_L, 1);
	return h;
}

/* LuaRef */

void LuaRef::set(lua_State *p_L, int p_idx) {

	clear();
	LuaStateHandle *h = handle_from_state(p_L);
	ERR_FAIL_COND(!h);
	lua_pushvalue(p_L, p_idx);
	ref = luaL_ref(p_L, LUA_REGISTRYINDEX);
	handle = h;
	handle->reference();
}

void LuaRef::clear() {

	if (!handle)
		return;
	if (handle->L && ref != LUA_NOREF && ref != LUA_REFNIL)
		luaL_unref(handle->L, LUA_REGISTRYINDEX, ref);
	handle->unreference();
	handle = NULL;
	ref = LUA_NOREF;
}

lua_State *LuaRef::push() const {

	if (!is_valid())
		return NULL;
	lua_rawgeti(handle->L, LUA_REGISTRYINDEX, ref);
	return handle->L;
}

bool LuaRef::is_valid() const {

	return handle && handle->L && ref != LUA_NOREF;
}

const void *LuaRef::pointer() const {

	lua_State *L = push();
	if (!L)
		return NULL;
	const void *p = lua_topointer(L, -1);
	lua_pop(L, 1);
	return p;
}

LuaRef::LuaRef() {

	handle = NULL;
	ref = LUA_NOREF;
}

LuaRef::~LuaRef() {

	clear();
}

/* ScriptObject */

void ScriptObject::set_table(lua_State *p_L, int p_idx) {

	table.set(p_L, p_idx);
}

lua_State *ScriptObject::push_table() const {

	return table.push();
}

Variant ScriptObject::get_var(const String &p_name) {

	lua_State *L = table.push();
	if (!L)
		return Variant();
	sunaba::safe_getfield(L, lua_gettop(L), p_name.utf8().get_data());
	Variant ret = sunaba::to_script_variant(L, -1);
	lua_pop(L, 2);
	return ret;
}

void ScriptObject::set_var(const String &p_name, const Variant &p_value) {

	lua_State *L = table.push();
	if (!L)
		return;
	int t = lua_gettop(L);
	sunaba::push_script_value(L, p_value);
	sunaba::safe_setfield(L, t, p_name.utf8().get_data());
	lua_pop(L, 1);
}

bool ScriptObject::has_var(const String &p_name) {

	lua_State *L = table.push();
	if (!L)
		return false;
	sunaba::safe_getfield(L, lua_gettop(L), p_name.utf8().get_data());
	bool ret = !lua_isnil(L, -1);
	lua_pop(L, 2);
	return ret;
}

bool ScriptObject::has_function(const String &p_name) {

	lua_State *L = table.push();
	if (!L)
		return false;
	sunaba::safe_getfield(L, lua_gettop(L), p_name.utf8().get_data());
	bool ret = lua_isfunction(L, -1);
	lua_pop(L, 2);
	return ret;
}

Variant ScriptObject::call_function(const String &p_name, const Array &p_args) {

	lua_State *L = table.push();
	if (!L)
		return Variant();
	sunaba::safe_getfield(L, lua_gettop(L), p_name.utf8().get_data());
	lua_remove(L, -2);
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		ERR_EXPLAIN("ScriptObject has no function: " + p_name);
		ERR_FAIL_V(Variant());
	}
	for (int i = 0; i < p_args.size(); i++)
		sunaba::push_script_value(L, p_args[i]);

	if (sunaba::pcall_traceback(L, p_args.size(), 1) != LUA_OK) {
		sunaba::report_error(L, "Script Error");
		return Variant();
	}
	Variant ret = sunaba::to_script_variant(L, -1);
	lua_pop(L, 1);
	return ret;
}

void ScriptObject::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("get_var", "name"), &ScriptObject::get_var);
	ObjectTypeDB::bind_method(_MD("set_var", "name", "variant"), &ScriptObject::set_var);
	ObjectTypeDB::bind_method(_MD("has_var", "name"), &ScriptObject::has_var);
	ObjectTypeDB::bind_method(_MD("has_function", "name"), &ScriptObject::has_function);
	ObjectTypeDB::bind_method(_MD("call_function", "name", "args"), &ScriptObject::call_function);
}

ScriptObject::ScriptObject() {
}

/* ScriptFunction */

void ScriptFunction::set_function(lua_State *p_L, int p_idx) {

	self_table.clear();
	member = String();
	func.set(p_L, p_idx);
}

void ScriptFunction::set_method(lua_State *p_L, int p_table_idx, const String &p_member) {

	func.clear();
	self_table.set(p_L, p_table_idx);
	member = p_member;
}

lua_State *ScriptFunction::push_function() const {

	if (func.is_valid())
		return func.push();

	lua_State *L = self_table.push();
	if (!L)
		return NULL;
	sunaba::safe_getfield(L, lua_gettop(L), member.utf8().get_data());
	lua_remove(L, -2);
	return L;
}

const void *ScriptFunction::get_function_pointer() const {

	lua_State *L = push_function();
	if (!L)
		return NULL;
	const void *p = lua_topointer(L, -1);
	lua_pop(L, 1);
	return p;
}

bool ScriptFunction::is_valid() const {

	return func.is_valid() || self_table.is_valid();
}

Variant ScriptFunction::call_argv(const Variant **p_args, int p_argcount, Variant::CallError &r_error) {

	r_error.error = Variant::CallError::CALL_OK;

	lua_State *L = push_function();
	if (!L) {
		r_error.error = Variant::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Variant();
	}
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}

	int nargs = p_argcount;
	if (self_table.is_valid()) {
		self_table.push();
		nargs++;
	}
	for (int i = 0; i < p_argcount; i++)
		sunaba::push_script_value(L, *p_args[i]);

	if (sunaba::pcall_traceback(L, nargs, 1) != LUA_OK) {
		sunaba::report_error(L, "Script Error");
		return Variant();
	}
	Variant ret = sunaba::to_script_variant(L, -1);
	lua_pop(L, 1);
	return ret;
}

Variant ScriptFunction::call_func(const Array &p_args) {

	Vector<const Variant *> argptrs;
	argptrs.resize(p_args.size());
	for (int i = 0; i < p_args.size(); i++)
		argptrs[i] = &p_args[i];

	Variant::CallError ce;
	return call_argv(argptrs.size() ? &argptrs[0] : NULL, argptrs.size(), ce);
}

Variant ScriptFunction::invoke(const Variant **p_args, int p_argcount, Variant::CallError &r_error) {

	return call_argv(p_args, p_argcount, r_error);
}

void ScriptFunction::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("call_func", "args"), &ScriptFunction::call_func);

	MethodInfo mi;
	mi.name = "invoke";
	ObjectTypeDB::bind_native_method(METHOD_FLAGS_DEFAULT, "invoke", &ScriptFunction::invoke, mi);
}

ScriptFunction::ScriptFunction() {
}
