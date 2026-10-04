/*************************************************************************/
/*  script_object.h                                                      */
/*************************************************************************/
/*  Godot-side handles to Lua values (libsunaba ScriptObject,            */
/*  ScriptFunction, DisposableObject and RefObject).                     */
/*************************************************************************/

#ifndef SUNABA_SCRIPT_OBJECT_H
#define SUNABA_SCRIPT_OBJECT_H

#include "object.h"
#include "reference.h"

struct lua_State;

// Shared, refcounted pointer to a Runtime's lua_State. The Runtime clears
// `L` when it closes the state, so handles that outlive it become inert
// instead of touching freed memory.
struct LuaStateHandle {
	lua_State *L;
	int refcount;

	static LuaStateHandle *create(lua_State *p_L);
	void reference();
	void unreference();
};

// Owns one Lua registry reference.
class LuaRef {
	LuaStateHandle *handle;
	int ref;

	LuaRef(const LuaRef &);
	LuaRef &operator=(const LuaRef &);

public:
	// Takes a reference to the value at p_idx on p_L's stack.
	void set(lua_State *p_L, int p_idx);
	void clear();
	// Pushes the value onto the main thread's stack and returns that thread,
	// or NULL if the reference is empty or its Runtime is gone.
	lua_State *push() const;
	bool is_valid() const;
	const void *pointer() const;

	LuaRef();
	~LuaRef();
};

// A Lua table exposed to Godot.
class ScriptObject : public Reference {
	OBJ_TYPE(ScriptObject, Reference);

	LuaRef table;

protected:
	static void _bind_methods();

public:
	void set_table(lua_State *p_L, int p_idx);
	// Pushes the table, returning the thread it was pushed on (or NULL).
	lua_State *push_table() const;

	Variant get_var(const String &p_name);
	void set_var(const String &p_name, const Variant &p_value);
	bool has_var(const String &p_name);
	bool has_function(const String &p_name);
	Variant call_function(const String &p_name, const Array &p_args);

	ScriptObject();
};

// A Lua function exposed to Godot. If a table is set, the function is
// looked up on it by name at call time and the table is passed as `self`.
class ScriptFunction : public Reference {
	OBJ_TYPE(ScriptFunction, Reference);

	LuaRef func;
	LuaRef self_table;
	String member;

protected:
	static void _bind_methods();

public:
	void set_function(lua_State *p_L, int p_idx);
	void set_method(lua_State *p_L, int p_table_idx, const String &p_member);
	// Pushes the function, returning the thread it was pushed on (or NULL).
	lua_State *push_function() const;
	const void *get_function_pointer() const;
	bool is_valid() const;

	Variant call_argv(const Variant **p_args, int p_argcount, Variant::CallError &r_error);

	// libsunaba API: call_func(args: Array)
	Variant call_func(const Array &p_args);
	// Varargs entry point; this is what signal connections call.
	Variant invoke(const Variant **p_args, int p_argcount, Variant::CallError &r_error);

	ScriptFunction();
};

class DisposableObject : public Object {
	OBJ_TYPE(DisposableObject, Object);

protected:
	static void _bind_methods() {}
};

class RefObject : public Reference {
	OBJ_TYPE(RefObject, Reference);

protected:
	static void _bind_methods() {}
};

#endif // SUNABA_SCRIPT_OBJECT_H
