/*************************************************************************/
/*  lua_runtime.cpp                                                      */
/*************************************************************************/

#include "lua_runtime.h"

#include "globals.h"
#include "lua_bridge.h"
#include "os/dir_access.h"
#include "os/os.h"
#include "print_string.h"
#include "script_language.h"
#include "script_object.h"
#include "version.h"

#define DEBUGGER_LUA_IMPLEMENTATION
#include "debugger_lua.h"

using namespace sunaba;

/* Lua globals installed by init_state */

// print(...): converts arguments without invoking metamethods, so it cannot
// raise while C++ temporaries are alive.
static void append_print_arg(lua_State *L, int p_idx, StringArray &r_out) {

	switch (lua_type(L, p_idx)) {
		case LUA_TNIL: r_out.push_back("nil"); return;
		case LUA_TBOOLEAN: r_out.push_back(lua_toboolean(L, p_idx) ? "true" : "false"); return;
		case LUA_TNUMBER:
		case LUA_TSTRING: r_out.push_back(to_string(L, p_idx)); return;
		case LUA_TTABLE: r_out.push_back("table"); return;
		case LUA_TUSERDATA: break;
		default: r_out.push_back(luaL_typename(L, p_idx)); return;
	}

	VariantBox *b = test_box(L, p_idx);
	if (b) {
		switch (b->value.get_type()) {
			case Variant::DICTIONARY: r_out.push_back(b->value.operator Dictionary().to_json()); return;
			case Variant::COLOR: r_out.push_back(Color(b->value).to_html()); return;
			default: r_out.push_back(String(b->value)); return;
		}
	}
	ObjectBox *ob = (ObjectBox *)test_kind(L, p_idx, KIND_OBJECT);
	if (ob) {
		Object *o = ob->get();
		r_out.push_back(o ? "[" + o->get_type() + ":" + itos(o->get_instance_ID()) + "]" : String("[null]"));
		return;
	}
	r_out.push_back("userdata");
}

SUNABA_LUA_FUNC(l_print) {

	Runtime *rt = get_runtime(L);
	{
		StringArray messages;
		int n = lua_gettop(L);
		for (int i = 1; i <= n; i++)
			append_print_arg(L, i, messages);
		if (rt)
			rt->print_messages(messages);
	}
	return 0;
}

static void get_two_strings(lua_State *L, String &r_msg, String &r_title) {

	r_msg = to_string(L, 1);
	r_title = lua_isnoneornil(L, 2) ? String() : to_string(L, 2);
}

SUNABA_LUA_FUNC(l_errord) {

	Runtime *rt = get_runtime(L);
	{
		String msg, title;
		get_two_strings(L, msg, title);
		if (rt)
			rt->errord(msg, title);
	}
	return 0;
}

SUNABA_LUA_FUNC(l_warnd) {

	Runtime *rt = get_runtime(L);
	{
		String msg, title;
		get_two_strings(L, msg, title);
		if (rt)
			rt->warnd(msg, title);
	}
	return 0;
}

SUNABA_LUA_FUNC(l_infod) {

	Runtime *rt = get_runtime(L);
	{
		String msg, title;
		get_two_strings(L, msg, title);
		if (rt)
			rt->infod(msg, title);
	}
	return 0;
}

static int l_exit(lua_State *L) {

	Runtime *rt = get_runtime(L);
	if (rt)
		rt->exit((int)lua_tointeger(L, 1));
	return 0;
}

// package.searchers entry: asks the Runtime's _require hook for source.
SUNABA_LUA_FUNC(l_require_searcher) {

	const char *name = luaL_checkstring(L, 1);
	Runtime *rt = get_runtime(L);
	int status;
	{
		String code = rt ? rt->require(String::utf8(name)) : String();
		if (code.empty()) {
			lua_pushfstring(L, "\n\tno source from Runtime._require('%s')", name);
			return 1;
		}
		CharString cs = code.utf8();
		status = luaL_loadbuffer(L, cs.get_data(), cs.length(), name);
	}
	if (status != LUA_OK)
		return luaL_error(L, "error loading module '%s':\n\t%s", name, lua_tostring(L, -1));
	lua_pushstring(L, name);
	return 2;
}

static int setup_debugger(lua_State *L) {

	dbg_setup_default(L);
	return 0;
}

static void append_package_path(lua_State *L, const String &p_dir) {

	lua_getglobal(L, "package");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}
	lua_getfield(L, -1, "path");
	String path = to_string(L, -1);
	lua_pop(L, 1);
	path += ";" + p_dir + "/?.lua";
	CharString cs = path.utf8();
	lua_pushstring(L, cs.get_data());
	lua_setfield(L, -2, "path");
	lua_pop(L, 1);
}

static void set_global_string(lua_State *L, const char *p_name, const String &p_value) {

	CharString cs = p_value.utf8();
	lua_pushstring(L, cs.get_data());
	lua_setglobal(L, p_name);
}

static void set_global_int(lua_State *L, const char *p_name, int p_value) {

	lua_pushinteger(L, p_value);
	lua_setglobal(L, p_name);
}

static int panic_handler(lua_State *L) {

	const char *msg = lua_tostring(L, -1);
	ERR_PRINT(String(String("Unprotected Lua error: ") + (msg ? msg : "(not a string)")).utf8().get_data());
	return 0;
}

/* Runtime */

bool Runtime::_script_has(const StringName &p_method) const {

	ScriptInstance *si = get_script_instance();
	return si && si->has_method(p_method);
}

void Runtime::keep_alive(const Ref<Reference> &p_ref) {

	for (int i = 0; i < kept_alive.size(); i++) {
		if (kept_alive[i] == p_ref)
			return;
	}
	kept_alive.push_back(p_ref);
}

void Runtime::init_state(bool p_sandboxed, const Array &p_classnames) {

	ERR_FAIL_COND(!L);
	if (initialized) {
		WARN_PRINT("Runtime.init_state called more than once");
	}
	initialized = true;
	sandboxed = p_sandboxed;
	sandbox_classes = p_classnames;

	open_math(L);
	open_containers(L);
	open_callable(L);
	open_io(L);
	open_variant(L, p_sandboxed, p_classnames);
	open_native(L, p_sandboxed, p_classnames);

	static const luaL_Reg common_libs[] = {
		{ LUA_GNAME, luaopen_base },
		{ LUA_LOADLIBNAME, luaopen_package },
		{ LUA_COLIBNAME, luaopen_coroutine },
		{ LUA_TABLIBNAME, luaopen_table },
		{ LUA_STRLIBNAME, luaopen_string },
		{ LUA_MATHLIBNAME, luaopen_math },
		{ LUA_UTF8LIBNAME, luaopen_utf8 },
		{ NULL, NULL }
	};
	static const luaL_Reg unsandboxed_libs[] = {
		{ LUA_IOLIBNAME, luaopen_io },
		{ LUA_OSLIBNAME, luaopen_os },
		{ LUA_DBLIBNAME, luaopen_debug },
		{ NULL, NULL }
	};
	for (const luaL_Reg *lib = common_libs; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);
	}
	if (!p_sandboxed) {
		for (const luaL_Reg *lib = unsandboxed_libs; lib->func; lib++) {
			luaL_requiref(L, lib->name, lib->func, 1);
			lua_pop(L, 1);
		}
	}
	lua_pushboolean(L, p_sandboxed);
	lua_setglobal(L, "sandboxed");

	// debugger.lua needs io and os, so sandboxed runtimes go without it.
	if (!p_sandboxed) {
		lua_pushcfunction(L, setup_debugger);
		if (lua_pcall(L, 0, 0, 0) != LUA_OK)
			report_error(L, "Debugger");
	}

	String exec_path = OS::get_singleton()->get_executable_path();
	set_global_string(L, "execPath", exec_path);

	bool osx = OS::get_singleton()->get_name() == "OSX";
	if (osx) {
		String res_dir = Globals::get_singleton()->globalize_path("res://");
		set_global_string(L, "resDir", res_dir);
		append_package_path(L, res_dir);
	}

	String exec_dir = exec_path.get_base_dir();
	if (osx) {
		// The executable lives in Contents/MacOS inside the app bundle.
		exec_dir = exec_dir.replace("/MacOS", "/Frameworks/").replace("\\MacOS", "\\Resources");
	}
	String exec_file = exec_path.get_file();
	String share_dir = exec_path.replace("bin/" + exec_file, "share/sunaba");
	if (osx) {
		share_dir = exec_dir.replace("/MacOS", "/Resources/").replace("\\MacOS", "\\Resources");
	}
	if (share_dir != String() && DirAccess::exists(share_dir)) {
		exec_dir = share_dir;
		set_global_string(L, "shareDir", share_dir);
		append_package_path(L, share_dir);
	} else {
		lua_pushnil(L);
		lua_setglobal(L, "shareDir");
	}
	set_global_string(L, "execDir", exec_dir);
	append_package_path(L, exec_dir);

	set_global_int(L, "SUNABA_LUA_RUNTIME_LUAJIT", 0);
	set_global_int(L, "SUNABA_LUA_RUNTIME_PUCRIO_54", 1);
	set_global_int(L, "SUNABA_LUA_RUNTIME_LUA_CSHARP", 2);
	set_global_int(L, "LUAJIT", 0);
	set_global_int(L, "PUCRIO_54", 1);
	set_global_int(L, "LUA_CSHARP", 2);
	set_global_int(L, "SUNABA_LUA_RUNTIME", 1);
	// Lets Lua code detect the engine generation it runs on.
	set_global_int(L, "SUNABA_GODOT_MAJOR", VERSION_MAJOR);

	lua_pushcfunction(L, l_print);
	lua_setglobal(L, "print");
	lua_pushcfunction(L, l_errord);
	lua_setglobal(L, "__errord");
	lua_pushcfunction(L, l_warnd);
	lua_setglobal(L, "__warnd");
	lua_pushcfunction(L, l_infod);
	lua_setglobal(L, "__infod");
	lua_pushcfunction(L, l_exit);
	lua_setglobal(L, "__exit");

	lua_getglobal(L, "package");
	lua_getfield(L, -1, "searchers");
	lua_pushcfunction(L, l_require_searcher);
	lua_rawseti(L, -2, (lua_Integer)lua_rawlen(L, -2) + 1);
	lua_pop(L, 2);

	bind_object("__rootNode", this);

	run_prelude(L);
}

void Runtime::do_string(const String &p_code) {

	ERR_FAIL_COND(!L);

	lua_createtable(L, args.size(), 0);
	for (int i = 0; i < args.size(); i++) {
		CharString cs = args[i].utf8();
		lua_pushstring(L, cs.get_data());
		lua_rawseti(L, -2, i + 1);
	}
	lua_setglobal(L, "__args");

	CharString code = p_code.utf8();
	if (luaL_loadbuffer(L, code.get_data(), code.length(), "=main") != LUA_OK) {
		report_error(L, "Error");
		return;
	}
	if (pcall_traceback(L, 0, 0) != LUA_OK)
		report_error(L, "Error");
}

void Runtime::set_var(const String &p_name, const Variant &p_value) {

	ERR_FAIL_COND(!L);
	push_script_value(L, p_value);
	lua_setglobal(L, p_name.utf8().get_data());
}

void Runtime::bind_object(const String &p_name, Object *p_object) {

	ERR_FAIL_COND(!L);
	push_object(L, p_object);
	lua_setglobal(L, p_name.utf8().get_data());
}

StringArray Runtime::get_args() const {

	return args;
}

void Runtime::set_args(const StringArray &p_args) {

	args = p_args;
}

void Runtime::print_messages(const StringArray &p_messages) {

	if (_script_has("_print"))
		get_script_instance()->call("_print", p_messages);

	String msg;
	for (int i = 0; i < p_messages.size(); i++) {
		if (i > 0)
			msg += ", ";
		msg += p_messages[i];
	}
	print_line(msg);
}

void Runtime::errord(const String &p_msg, const String &p_title) {

	if (_script_has("_errord"))
		get_script_instance()->call("_errord", p_msg, p_title);
	ERR_PRINT(String(p_title + ": " + p_msg).utf8().get_data());
}

void Runtime::warnd(const String &p_msg, const String &p_title) {

	if (_script_has("_warnd"))
		get_script_instance()->call("_warnd", p_msg, p_title);
	WARN_PRINT(String(p_title + ": " + p_msg).utf8().get_data());
}

void Runtime::infod(const String &p_msg, const String &p_title) {

	if (_script_has("_infod"))
		get_script_instance()->call("_infod", p_msg, p_title);
	print_line(p_title + ": " + p_msg);
}

void Runtime::exit(int p_code) {

	if (_script_has("_exit"))
		get_script_instance()->call("_exit", p_code);
}

String Runtime::require(const String &p_path) {

	if (_script_has("_require"))
		return get_script_instance()->call("_require", p_path);
	return String();
}

void Runtime::_notification(int p_what) {

	switch (p_what) {
		case NOTIFICATION_READY:
			set_process(true);
			break;
		case NOTIFICATION_PROCESS:
			// libsunaba ran a full collection every frame; an incremental
			// step is much cheaper on the 3DS.
			if (L)
				lua_gc(L, LUA_GCSTEP, 0);
			break;
	}
}

void Runtime::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("init_state", "sandboxed", "classes"), &Runtime::init_state, DEFVAL(false), DEFVAL(Array()));
	ObjectTypeDB::bind_method(_MD("do_string", "code"), &Runtime::do_string);
	ObjectTypeDB::bind_method(_MD("get_args"), &Runtime::get_args);
	ObjectTypeDB::bind_method(_MD("set_args", "args"), &Runtime::set_args);
	ObjectTypeDB::bind_method(_MD("bind_object", "name", "obj"), &Runtime::bind_object);
	ObjectTypeDB::bind_method(_MD("set_var", "name", "variant"), &Runtime::set_var);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_ARRAY, "args"), _SCS("set_args"), _SCS("get_args"));

	BIND_VMETHOD(MethodInfo(Variant::STRING, "_require", PropertyInfo(Variant::STRING, "path")));
	BIND_VMETHOD(MethodInfo("_errord", PropertyInfo(Variant::STRING, "msg"), PropertyInfo(Variant::STRING, "title")));
	BIND_VMETHOD(MethodInfo("_warnd", PropertyInfo(Variant::STRING, "msg"), PropertyInfo(Variant::STRING, "title")));
	BIND_VMETHOD(MethodInfo("_infod", PropertyInfo(Variant::STRING, "msg"), PropertyInfo(Variant::STRING, "title")));
	BIND_VMETHOD(MethodInfo("_print", PropertyInfo(Variant::STRING_ARRAY, "msgarr")));
	BIND_VMETHOD(MethodInfo("_exit", PropertyInfo(Variant::INT, "exitcode")));
}

Runtime::Runtime() {

	initialized = false;
	sandboxed = false;
	L = luaL_newstate();
	handle = NULL;
	ERR_FAIL_COND(!L);
	lua_atpanic(L, panic_handler);
	handle = LuaStateHandle::create(L);
	set_runtime(L, this);
}

Runtime::~Runtime() {

	if (!L)
		return;
	// Detach first so handles released while the state closes stay inert.
	handle->L = NULL;
	set_runtime(L, NULL);
	lua_close(L);
	L = NULL;
	handle->unreference();
	kept_alive.clear();
}
