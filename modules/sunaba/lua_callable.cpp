/*************************************************************************/
/*  lua_callable.cpp                                                     */
/*************************************************************************/
/*  Callable and Signal emulation. Godot 2 connects signals to an         */
/*  (object, method) pair, so a Callable is (target, method, binds).     */
/*  Lua functions become a ScriptFunction target whose `invoke` method   */
/*  forwards to Lua; the Runtime keeps connected ScriptFunctions alive.  */
/*************************************************************************/

#include "lua_bridge.h"

#include "lua_runtime.h"
#include "script_object.h"

namespace sunaba {

/* CallableBox */

Object *CallableBox::get_object() const {

	if (target == 0)
		return NULL;
	return ObjectDB::get_instance(target);
}

bool CallableBox::is_valid() const {

	Object *o = get_object();
	return o && o->has_method(method);
}

Variant CallableBox::call(const Array &p_args, Variant::CallError &r_error) const {

	Object *o = get_object();
	if (!o) {
		r_error.error = Variant::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Variant();
	}
	int argc = p_args.size() + binds.size();
	Vector<const Variant *> argptrs;
	argptrs.resize(argc);
	for (int i = 0; i < p_args.size(); i++)
		argptrs[i] = &p_args[i];
	for (int i = 0; i < binds.size(); i++)
		argptrs[p_args.size() + i] = &binds[i];
	return o->call(method, argc ? &argptrs[0] : NULL, argc, r_error);
}

static CallableBox *check_callable(lua_State *L, int p_idx) {

	return (CallableBox *)test_kind(L, p_idx, KIND_CALLABLE);
}

#define CALLABLE_SELF(m_name)                                  \
	CallableBox *self_box = check_callable(L, 1);              \
	if (!self_box) {                                           \
		err.format("Callable.%s: expected a Callable", m_name); \
		return 0;                                              \
	}

static CallableBox *new_callable(lua_State *L) {

	CallableBox *c = (CallableBox *)lua_newuserdatauv(L, sizeof(CallableBox), 0);
	memnew_placement(c, CallableBox);
	c->target = 0;
	luaL_setmetatable(L, SUNABA_MT_CALLABLE);
	return c;
}

static void target_script_function(CallableBox *c, const Ref<ScriptFunction> &p_func) {

	c->target = p_func->get_instance_ID();
	c->ref = p_func;
	c->method = "invoke";
}

// Callable.new(func) | Callable.new(table, methodName) | Callable.new(object, methodName)
SUNABA_LUA_FUNC(callable_new) {

	int t = lua_type(L, 1);
	if (t == LUA_TFUNCTION) {
		Ref<ScriptFunction> f;
		f.instance();
		f->set_function(L, 1);
		target_script_function(new_callable(L), f);
		return 1;
	}
	if (t == LUA_TTABLE) {
		Ref<ScriptFunction> f;
		f.instance();
		f->set_method(L, 1, to_string(L, 2));
		target_script_function(new_callable(L), f);
		return 1;
	}
	ObjectBox *ob = (ObjectBox *)test_kind(L, 1, KIND_OBJECT);
	if (ob) {
		CallableBox *c = new_callable(L);
		c->target = ob->id;
		c->ref = ob->ref;
		c->method = to_string(L, 2);
		return 1;
	}
	err.format("Callable.new: expected a function, a table and method name, or an object and method name");
	return 0;
}

SUNABA_LUA_FUNC(callable_bind) {

	CALLABLE_SELF("bind");
	{
		Array extra;
		if (!to_array(L, 2, extra)) {
			err.format("Callable.bind: expected an ArrayList");
			return 0;
		}
		CallableBox *c = new_callable(L);
		c->target = self_box->target;
		c->ref = self_box->ref;
		c->method = self_box->method;
		// Godot 4 passes bound arguments after the call arguments.
		c->binds = Array(true);
		for (int i = 0; i < extra.size(); i++)
			c->binds.push_back(extra[i]);
		for (int i = 0; i < self_box->binds.size(); i++)
			c->binds.push_back(self_box->binds[i]);
	}
	return 1;
}

SUNABA_LUA_FUNC(callable_call) {

	CALLABLE_SELF("call");
	{
		Array args;
		if (!to_array(L, 2, args)) {
			err.format("Callable.call: expected an ArrayList");
			return 0;
		}
		Variant::CallError ce;
		Variant ret = self_box->call(args, ce);
		if (ce.error != Variant::CallError::CALL_OK) {
			err.format("%s", call_error_text(ce, self_box->method).utf8().get_data());
			return 0;
		}
		push_variant(L, ret);
	}
	return 1;
}

SUNABA_LUA_FUNC(callable_get_argument_count) {

	CALLABLE_SELF("getArgumentCount");
	int count = 0;
	{
		Object *o = self_box->get_object();
		ScriptFunction *sf = o ? o->cast_to<ScriptFunction>() : NULL;
		if (sf) {
			lua_State *FL = sf->push_function();
			if (FL) {
				lua_Debug ar;
				lua_getinfo(FL, ">u", &ar);
				count = ar.nparams;
			}
		} else if (o) {
			List<MethodInfo> methods;
			o->get_method_list(&methods);
			for (List<MethodInfo>::Element *E = methods.front(); E; E = E->next()) {
				if (E->get().name == String(self_box->method)) {
					count = E->get().arguments.size();
					break;
				}
			}
		}
		count -= self_box->binds.size();
	}
	lua_pushinteger(L, count < 0 ? 0 : count);
	return 1;
}

SUNABA_LUA_FUNC(callable_get_bound_arguments) {

	CALLABLE_SELF("getBoundArguments");
	{
		Array copy(true);
		for (int i = 0; i < self_box->binds.size(); i++)
			copy.push_back(self_box->binds[i]);
		push_box(L, copy, SUNABA_MT_ARRAYLIST);
	}
	return 1;
}

SUNABA_LUA_FUNC(callable_get_bound_arguments_count) {

	CALLABLE_SELF("getBoundArgumentsCount");
	lua_pushinteger(L, self_box->binds.size());
	return 1;
}

SUNABA_LUA_FUNC(callable_get_method) {

	CALLABLE_SELF("getMethod");
	{
		CharString cs = String(self_box->method).utf8();
		lua_pushlstring(L, cs.get_data(), cs.length());
	}
	return 1;
}

SUNABA_LUA_FUNC(callable_get_object) {

	CALLABLE_SELF("getObject");
	push_object(L, self_box->get_object());
	return 1;
}

static int callable_zero(lua_State *L) {

	lua_pushinteger(L, 0);
	return 1;
}

SUNABA_LUA_FUNC(callable_hash) {

	CALLABLE_SELF("hash");
	uint32_t h;
	{
		h = String(self_box->method).hash() ^ (uint32_t)self_box->target;
	}
	lua_pushinteger(L, h);
	return 1;
}

static bool callable_is_custom_impl(CallableBox *c) {

	Object *o = c->get_object();
	return o && o->cast_to<ScriptFunction>();
}

SUNABA_LUA_FUNC(callable_is_custom) {

	CALLABLE_SELF("isCustom");
	lua_pushboolean(L, callable_is_custom_impl(self_box));
	return 1;
}

SUNABA_LUA_FUNC(callable_is_standard) {

	CALLABLE_SELF("isStandard");
	lua_pushboolean(L, !callable_is_custom_impl(self_box));
	return 1;
}

SUNABA_LUA_FUNC(callable_is_null) {

	CALLABLE_SELF("isNull");
	lua_pushboolean(L, self_box->get_object() == NULL);
	return 1;
}

SUNABA_LUA_FUNC(callable_is_valid) {

	CALLABLE_SELF("isValid");
	lua_pushboolean(L, self_box->is_valid());
	return 1;
}

// Godot 2 cannot drop arguments from a call; returns an equivalent copy.
SUNABA_LUA_FUNC(callable_unbind) {

	CALLABLE_SELF("unbind");
	{
		WARN_PRINT("Callable.unbind is not supported by the Godot 2 runtime");
		CallableBox *c = new_callable(L);
		c->target = self_box->target;
		c->ref = self_box->ref;
		c->method = self_box->method;
		c->binds = self_box->binds;
	}
	return 1;
}

static int callable_eq(lua_State *L) {

	CallableBox *a = check_callable(L, 1);
	CallableBox *b = check_callable(L, 2);
	lua_pushboolean(L, a && b && a->target == b->target && a->method == b->method);
	return 1;
}

static int callable_gc(lua_State *L) {

	CallableBox *c = (CallableBox *)lua_touserdata(L, 1);
	if (c)
		c->~CallableBox();
	return 0;
}

static int callable_tostring(lua_State *L) {

	CallableBox *c = check_callable(L, 1);
	if (c && callable_is_custom_impl(c))
		lua_pushstring(L, "[Lua callable]");
	else
		lua_pushstring(L, "[Callable]");
	return 1;
}

static const luaL_Reg callable_methods[] = {
	{ "new", callable_new },
	{ "bind", callable_bind },
	{ "call", callable_call },
	{ "getArgumentCount", callable_get_argument_count },
	{ "getBoundArguments", callable_get_bound_arguments },
	{ "getBoundArgumentsCount", callable_get_bound_arguments_count },
	{ "getMethod", callable_get_method },
	{ "getObject", callable_get_object },
	{ "getReference", callable_get_object },
	{ "getUnboundArgumentsCount", callable_zero },
	{ "hash", callable_hash },
	{ "isCustom", callable_is_custom },
	{ "isNull", callable_is_null },
	{ "isStandard", callable_is_standard },
	{ "isValid", callable_is_valid },
	{ "unbind", callable_unbind },
	{ "eq", callable_eq },
	{ NULL, NULL }
};

static const luaL_Reg callable_meta[] = {
	{ "__gc", callable_gc },
	{ "__eq", callable_eq },
	{ "__tostring", callable_tostring },
	{ NULL, NULL }
};

/* Signal */

static SignalBox *check_signal(lua_State *L, int p_idx) {

	return (SignalBox *)test_kind(L, p_idx, KIND_SIGNAL);
}

#define SIGNAL_SELF(m_name)                                  \
	SignalBox *self_box = check_signal(L, 1);                \
	if (!self_box) {                                         \
		err.format("Signal.%s: expected a Signal", m_name);  \
		return 0;                                            \
	}                                                        \
	Object *self = self_box->object ? ObjectDB::get_instance(self_box->object) : NULL;

static SignalBox *new_signal(lua_State *L) {

	SignalBox *s = (SignalBox *)lua_newuserdatauv(L, sizeof(SignalBox), 0);
	memnew_placement(s, SignalBox);
	s->object = 0;
	luaL_setmetatable(L, SUNABA_MT_SIGNAL);
	return s;
}

SUNABA_LUA_FUNC(signal_new) {

	if (lua_gettop(L) == 0) {
		new_signal(L);
		return 1;
	}
	ObjectBox *ob = (ObjectBox *)test_kind(L, 1, KIND_OBJECT);
	if (!ob) {
		err.format("Signal.new: expected an object and a signal name");
		return 0;
	}
	{
		String name = to_string(L, 2);
		SignalBox *s = new_signal(L);
		s->object = ob->id;
		s->name = name;
	}
	return 1;
}

SUNABA_LUA_FUNC(signal_connect) {

	SIGNAL_SELF("connect");
	CallableBox *c = check_callable(L, 2);
	if (!self || !c) {
		err.format("Signal.connect: expected a valid signal and a Callable");
		return 0;
	}
	int ret;
	{
		Object *target = c->get_object();
		if (!target) {
			err.format("Signal.connect: callable target is null");
			return 0;
		}
		uint32_t flags = lua_isnoneornil(L, 3) ? 0 : (uint32_t)lua_tointeger(L, 3);
		Vector<Variant> binds;
		for (int i = 0; i < c->binds.size(); i++)
			binds.push_back(c->binds[i]);
		ret = self->connect(self_box->name, target, c->method, binds, flags);
		Runtime *rt = get_runtime(L);
		if (ret == OK && rt && c->ref.is_valid() && target->cast_to<ScriptFunction>())
			rt->keep_alive(c->ref);
	}
	lua_pushinteger(L, ret);
	return 1;
}

SUNABA_LUA_FUNC(signal_disconnect) {

	SIGNAL_SELF("disconnect");
	CallableBox *c = check_callable(L, 2);
	if (!self || !c)
		return 0;
	{
		Object *target = c->get_object();
		if (target && self->is_connected(self_box->name, target, c->method))
			self->disconnect(self_box->name, target, c->method);
	}
	return 0;
}

SUNABA_LUA_FUNC(signal_emit) {

	SIGNAL_SELF("emit");
	if (!self)
		return 0;
	{
		Array args;
		if (!to_array(L, 2, args)) {
			err.format("Signal.emit: expected an ArrayList");
			return 0;
		}
		Vector<const Variant *> argptrs;
		argptrs.resize(args.size());
		for (int i = 0; i < args.size(); i++)
			argptrs[i] = &args[i];
		self->emit_signal(self_box->name, args.size() ? &argptrs[0] : NULL, args.size());
	}
	return 0;
}

SUNABA_LUA_FUNC(signal_get_connections) {

	SIGNAL_SELF("getConnections");
	{
		Array list = self ? self->call("get_signal_connection_list", String(self_box->name)).operator Array() : Array();
		push_box(L, list, SUNABA_MT_ARRAYLIST);
	}
	return 1;
}

SUNABA_LUA_FUNC(signal_has_connections) {

	SIGNAL_SELF("hasConnections");
	bool has = false;
	if (self) {
		List<Object::Connection> conns;
		self->get_signal_connection_list(self_box->name, &conns);
		has = conns.size() > 0;
	}
	lua_pushboolean(L, has);
	return 1;
}

SUNABA_LUA_FUNC(signal_is_connected) {

	SIGNAL_SELF("isConnected");
	CallableBox *c = check_callable(L, 2);
	bool connected = false;
	if (self && c) {
		Object *target = c->get_object();
		connected = target && self->is_connected(self_box->name, target, c->method);
	}
	lua_pushboolean(L, connected);
	return 1;
}

SUNABA_LUA_FUNC(signal_get_name) {

	SIGNAL_SELF("getName");
	{
		CharString cs = String(self_box->name).utf8();
		lua_pushlstring(L, cs.get_data(), cs.length());
	}
	return 1;
}

SUNABA_LUA_FUNC(signal_get_object) {

	SIGNAL_SELF("getObject");
	push_object(L, self);
	return 1;
}

SUNABA_LUA_FUNC(signal_is_null) {

	SIGNAL_SELF("isNull");
	lua_pushboolean(L, self == NULL || self_box->name == StringName());
	return 1;
}

static int signal_eq(lua_State *L) {

	SignalBox *a = check_signal(L, 1);
	SignalBox *b = check_signal(L, 2);
	lua_pushboolean(L, a && b && a->object == b->object && a->name == b->name);
	return 1;
}

static int signal_gc(lua_State *L) {

	SignalBox *s = (SignalBox *)lua_touserdata(L, 1);
	if (s)
		s->~SignalBox();
	return 0;
}

static const luaL_Reg signal_methods[] = {
	{ "new", signal_new },
	{ "connect", signal_connect },
	{ "disconnect", signal_disconnect },
	{ "emit", signal_emit },
	{ "getConnections", signal_get_connections },
	{ "getName", signal_get_name },
	{ "getObject", signal_get_object },
	{ "getReference", signal_get_object },
	{ "hasConnections", signal_has_connections },
	{ "isConnected", signal_is_connected },
	{ "isNull", signal_is_null },
	{ "eq", signal_eq },
	{ NULL, NULL }
};

static const luaL_Reg signal_meta[] = {
	{ "__gc", signal_gc },
	{ "__eq", signal_eq },
	{ NULL, NULL }
};

void open_callable(lua_State *L) {

	new_type(L, SUNABA_MT_CALLABLE, KIND_CALLABLE, callable_methods, callable_meta);
	new_type(L, SUNABA_MT_SIGNAL, KIND_SIGNAL, signal_methods, signal_meta);
}

} // namespace sunaba
