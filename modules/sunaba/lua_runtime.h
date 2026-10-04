/*************************************************************************/
/*  lua_runtime.h                                                        */
/*************************************************************************/
/*  The Sunaba Lua runtime node (libsunaba's Runtime).                   */
/*                                                                       */
/*  Scripts extending Runtime may implement these virtual hooks:         */
/*    _require(path) -> String   source for `require`                    */
/*    _print(msgarr)             print() output                          */
/*    _errord / _warnd / _infod(msg, title)                              */
/*    _exit(exitcode)                                                    */
/*************************************************************************/

#ifndef SUNABA_LUA_RUNTIME_H
#define SUNABA_LUA_RUNTIME_H

#include "scene/main/node.h"

struct lua_State;
struct LuaStateHandle;

class Runtime : public Node {
	OBJ_TYPE(Runtime, Node);

	lua_State *L;
	LuaStateHandle *handle;
	bool initialized;
	bool sandboxed;
	Array sandbox_classes;
	StringArray args;
	// ScriptFunctions connected to signals; Godot 2 connections do not hold
	// a reference to their target.
	Vector<Ref<Reference> > kept_alive;

	bool _script_has(const StringName &p_method) const;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	lua_State *get_lua_state() const { return L; }
	bool is_sandboxed() const { return sandboxed; }
	const Array &get_sandbox_classes() const { return sandbox_classes; }
	void keep_alive(const Ref<Reference> &p_ref);

	void init_state(bool p_sandboxed = false, const Array &p_classnames = Array());
	void do_string(const String &p_code);
	void set_var(const String &p_name, const Variant &p_value);
	void bind_object(const String &p_name, Object *p_object);

	StringArray get_args() const;
	void set_args(const StringArray &p_args);

	// Hooks into the script's virtual methods, then the engine's output.
	void print_messages(const StringArray &p_messages);
	void errord(const String &p_msg, const String &p_title);
	void warnd(const String &p_msg, const String &p_title);
	void infod(const String &p_msg, const String &p_title);
	void exit(int p_code);
	String require(const String &p_path);

	Runtime();
	~Runtime();
};

#endif // SUNABA_LUA_RUNTIME_H
