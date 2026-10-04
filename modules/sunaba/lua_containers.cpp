/*************************************************************************/
/*  lua_containers.cpp                                                   */
/*************************************************************************/
/*  ArrayList (godot Array) and Dictionary. Element reads return boxed   */
/*  Variants, as libsunaba's sol2 bindings did. Godot 4-only methods are */
/*  implemented on top of the Godot 2 API.                               */
/*************************************************************************/

#include "lua_bridge.h"

#include "math_funcs.h"

namespace sunaba {

static Variant deep_copy(const Variant &p_value) {

	if (p_value.get_type() == Variant::ARRAY) {
		Array src = p_value;
		Array dst(true);
		dst.resize(src.size());
		for (int i = 0; i < src.size(); i++)
			dst[i] = deep_copy(src[i]);
		return dst;
	}
	if (p_value.get_type() == Variant::DICTIONARY) {
		Dictionary src = p_value;
		Dictionary dst(true);
		const Variant *k = NULL;
		while ((k = src.next(k)))
			dst[*k] = deep_copy(src[*k]);
		return dst;
	}
	return p_value;
}

static bool less_than(const Variant &p_a, const Variant &p_b) {

	Variant ret;
	bool valid = false;
	Variant::evaluate(Variant::OP_LESS, p_a, p_b, ret, valid);
	return valid && (bool)ret;
}

static bool variants_equal(const Variant &p_a, const Variant &p_b);

static bool arrays_equal(const Array &p_a, const Array &p_b) {

	if (p_a.size() != p_b.size())
		return false;
	for (int i = 0; i < p_a.size(); i++) {
		if (!variants_equal(p_a[i], p_b[i]))
			return false;
	}
	return true;
}

static bool dictionaries_equal(const Dictionary &p_a, const Dictionary &p_b) {

	if (p_a.size() != p_b.size())
		return false;
	const Variant *k = NULL;
	while ((k = p_a.next(k))) {
		const Variant *other = p_b.getptr(*k);
		if (!other || !variants_equal(p_a[*k], *other))
			return false;
	}
	return true;
}

// Godot 2 compares containers by identity; Godot 4 compares contents.
static bool variants_equal(const Variant &p_a, const Variant &p_b) {

	if (p_a.get_type() == Variant::ARRAY && p_b.get_type() == Variant::ARRAY)
		return arrays_equal(p_a, p_b);
	if (p_a.get_type() == Variant::DICTIONARY && p_b.get_type() == Variant::DICTIONARY)
		return dictionaries_equal(p_a, p_b);
	return p_a == p_b;
}

/* ArrayList */

static VariantBox *test_array(lua_State *L, int p_idx) {

	VariantBox *b = test_box(L, p_idx);
	if (!b || b->value.get_type() != Variant::ARRAY)
		return NULL;
	return b;
}

#define ARRAY_SELF(m_name)                                       \
	VariantBox *self_box = test_array(L, 1);                     \
	if (!self_box) {                                             \
		err.format("ArrayList.%s: expected an ArrayList", m_name); \
		return 0;                                                \
	}

static int normalize_index(int p_index, int p_size) {

	return p_index < 0 ? p_index + p_size : p_index;
}

SUNABA_LUA_FUNC(array_new) {

	push_box(L, Array(true), SUNABA_MT_ARRAYLIST);
	return 1;
}

SUNABA_LUA_FUNC(array_append) {

	ARRAY_SELF("append");
	{
		Array a = self_box->value;
		a.push_back(to_variant(L, 2));
	}
	return 0;
}

SUNABA_LUA_FUNC(array_push_front) {

	ARRAY_SELF("pushFront");
	{
		Array a = self_box->value;
		a.push_front(to_variant(L, 2));
	}
	return 0;
}

SUNABA_LUA_FUNC(array_append_array) {

	ARRAY_SELF("appendArray");
	{
		Array a = self_box->value;
		Array other;
		if (!to_array(L, 2, other)) {
			err.format("ArrayList.appendArray: expected an array");
			return 0;
		}
		for (int i = 0; i < other.size(); i++)
			a.push_back(other[i]);
	}
	return 0;
}

SUNABA_LUA_FUNC(array_assign) {

	ARRAY_SELF("assign");
	{
		Array a = self_box->value;
		Array other;
		if (!to_array(L, 2, other)) {
			err.format("ArrayList.assign: expected an array");
			return 0;
		}
		Array copy = deep_copy(other).get_type() == Variant::ARRAY ? other : Array();
		a.resize(copy.size());
		for (int i = 0; i < copy.size(); i++)
			a[i] = copy[i];
	}
	return 0;
}

SUNABA_LUA_FUNC(array_back) {

	ARRAY_SELF("back");
	{
		Array a = self_box->value;
		push_variant(L, a.empty() ? Variant() : a[a.size() - 1]);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_front) {

	ARRAY_SELF("front");
	{
		Array a = self_box->value;
		push_variant(L, a.empty() ? Variant() : a[0]);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_bsearch) {

	ARRAY_SELF("bsearch");
	int lo = 0;
	{
		Array a = self_box->value;
		Variant v = to_variant(L, 2);
		bool before = lua_isnoneornil(L, 3) ? true : lua_toboolean(L, 3);
		int hi = a.size();
		while (lo < hi) {
			int mid = (lo + hi) / 2;
			bool go_right = before ? less_than(a[mid], v) : !less_than(v, a[mid]);
			if (go_right)
				lo = mid + 1;
			else
				hi = mid;
		}
	}
	lua_pushinteger(L, lo);
	return 1;
}

SUNABA_LUA_FUNC(array_clear) {

	ARRAY_SELF("clear");
	{
		Array a = self_box->value;
		a.clear();
	}
	return 0;
}

SUNABA_LUA_FUNC(array_count) {

	ARRAY_SELF("count");
	int n;
	{
		Array a = self_box->value;
		n = a.count(to_variant(L, 2));
	}
	lua_pushinteger(L, n);
	return 1;
}

SUNABA_LUA_FUNC(array_duplicate) {

	ARRAY_SELF("duplicate");
	{
		bool deep = lua_toboolean(L, 2);
		Array a = self_box->value;
		Variant copy;
		if (deep) {
			copy = deep_copy(a);
		} else {
			Array dst(true);
			dst.resize(a.size());
			for (int i = 0; i < a.size(); i++)
				dst[i] = a[i];
			copy = dst;
		}
		push_box(L, copy, SUNABA_MT_ARRAYLIST);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_erase) {

	ARRAY_SELF("erase");
	{
		Array a = self_box->value;
		a.erase(to_variant(L, 2));
	}
	return 0;
}

SUNABA_LUA_FUNC(array_fill) {

	ARRAY_SELF("fill");
	{
		Array a = self_box->value;
		Variant v = to_variant(L, 2);
		for (int i = 0; i < a.size(); i++)
			a[i] = v;
	}
	return 0;
}

// Calls the Lua function or Callable at p_fn with one Variant argument and
// returns the truthiness of the result. Sets r_failed on a script error.
static bool truthy(const Variant &p_value) {

	bool valid = false;
	return p_value.booleanize(valid);
}

static bool call_predicate(lua_State *L, int p_fn, const Variant &p_arg, bool &r_failed) {

	r_failed = false;
	if (lua_type(L, p_fn) == LUA_TFUNCTION) {
		lua_pushvalue(L, p_fn);
		push_variant(L, p_arg);
		if (pcall_traceback(L, 1, 1) != LUA_OK) {
			report_error(L, "Script Error");
			r_failed = true;
			return false;
		}
		bool ret;
		VariantBox *b = test_box(L, -1);
		if (b)
			ret = truthy(b->value);
		else
			ret = lua_toboolean(L, -1);
		lua_pop(L, 1);
		return ret;
	}
	CallableBox *c = (CallableBox *)test_kind(L, p_fn, KIND_CALLABLE);
	if (c) {
		Array args;
		args.push_back(p_arg);
		Variant::CallError ce;
		Variant ret = c->call(args, ce);
		if (ce.error != Variant::CallError::CALL_OK) {
			r_failed = true;
			return false;
		}
		return truthy(ret);
	}
	r_failed = true;
	return false;
}

SUNABA_LUA_FUNC(array_filter) {

	ARRAY_SELF("filter");
	{
		Array a = self_box->value;
		Array out(true);
		for (int i = 0; i < a.size(); i++) {
			bool failed = false;
			bool keep = call_predicate(L, 2, a[i], failed);
			if (failed) {
				err.format("ArrayList.filter: predicate failed");
				return 0;
			}
			if (keep)
				out.push_back(a[i]);
		}
		push_box(L, out, SUNABA_MT_ARRAYLIST);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_find) {

	ARRAY_SELF("find");
	int idx;
	{
		Array a = self_box->value;
		int from = lua_isnoneornil(L, 3) ? 0 : (int)lua_tointeger(L, 3);
		idx = a.find(to_variant(L, 2), normalize_index(from, a.size()));
	}
	lua_pushinteger(L, idx);
	return 1;
}

SUNABA_LUA_FUNC(array_rfind) {

	ARRAY_SELF("rfind");
	int idx;
	{
		Array a = self_box->value;
		int from = lua_isnoneornil(L, 3) ? -1 : (int)lua_tointeger(L, 3);
		idx = a.rfind(to_variant(L, 2), from);
	}
	lua_pushinteger(L, idx);
	return 1;
}

SUNABA_LUA_FUNC(array_get) {

	ARRAY_SELF("get");
	{
		Array a = self_box->value;
		int i = (int)lua_tointeger(L, 2);
		if (i < 0 || i >= a.size()) {
			err.format("ArrayList.get: index %d out of range (size %d)", i, a.size());
			return 0;
		}
		push_variant(L, a[i]);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_set) {

	ARRAY_SELF("set");
	{
		Array a = self_box->value;
		int i = (int)lua_tointeger(L, 2);
		if (i < 0 || i >= a.size()) {
			err.format("ArrayList.set: index %d out of range (size %d)", i, a.size());
			return 0;
		}
		a[i] = to_variant(L, 3);
	}
	return 0;
}

SUNABA_LUA_FUNC(array_has) {

	ARRAY_SELF("has");
	bool has;
	{
		Array a = self_box->value;
		has = a.has(to_variant(L, 2));
	}
	lua_pushboolean(L, has);
	return 1;
}

SUNABA_LUA_FUNC(array_hash) {

	ARRAY_SELF("hash");
	uint32_t h;
	{
		Array a = self_box->value;
		h = a.hash();
	}
	lua_pushinteger(L, h);
	return 1;
}

SUNABA_LUA_FUNC(array_insert) {

	ARRAY_SELF("insert");
	{
		Array a = self_box->value;
		int i = (int)lua_tointeger(L, 2);
		if (i < 0 || i > a.size()) {
			err.format("ArrayList.insert: index %d out of range (size %d)", i, a.size());
			return 0;
		}
		a.insert(i, to_variant(L, 3));
	}
	return 0;
}

SUNABA_LUA_FUNC(array_is_empty) {

	ARRAY_SELF("isEmpty");
	bool empty;
	{
		Array a = self_box->value;
		empty = a.empty();
	}
	lua_pushboolean(L, empty);
	return 1;
}

static int array_is_read_only(lua_State *L) {

	lua_pushboolean(L, false);
	return 1;
}

static int noop(lua_State *L) {

	return 0;
}

static int array_extreme(lua_State *L, LuaError &err, bool p_max) {

	ARRAY_SELF(p_max ? "max" : "min");
	{
		Array a = self_box->value;
		if (a.empty()) {
			push_variant(L, Variant());
			return 1;
		}
		Variant best = a[0];
		for (int i = 1; i < a.size(); i++) {
			if (p_max ? less_than(best, a[i]) : less_than(a[i], best))
				best = a[i];
		}
		push_variant(L, best);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_max) {

	return array_extreme(L, err, true);
}

SUNABA_LUA_FUNC(array_min) {

	return array_extreme(L, err, false);
}

SUNABA_LUA_FUNC(array_pick_random) {

	ARRAY_SELF("pickRandom");
	{
		Array a = self_box->value;
		push_variant(L, a.empty() ? Variant() : a[Math::rand() % a.size()]);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_pop_at) {

	ARRAY_SELF("popAt");
	{
		Array a = self_box->value;
		int i = normalize_index((int)lua_tointeger(L, 2), a.size());
		if (i < 0 || i >= a.size()) {
			push_variant(L, Variant());
			return 1;
		}
		Variant v = a[i];
		a.remove(i);
		push_variant(L, v);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_pop_back) {

	ARRAY_SELF("popBack");
	{
		Array a = self_box->value;
		Variant v;
		if (!a.empty()) {
			v = a[a.size() - 1];
			a.pop_back();
		}
		push_variant(L, v);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_pop_front) {

	ARRAY_SELF("popFront");
	{
		Array a = self_box->value;
		Variant v;
		if (!a.empty()) {
			v = a[0];
			a.pop_front();
		}
		push_variant(L, v);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_remove_at) {

	ARRAY_SELF("removeAt");
	{
		Array a = self_box->value;
		int i = normalize_index((int)lua_tointeger(L, 2), a.size());
		if (i < 0 || i >= a.size()) {
			err.format("ArrayList.removeAt: index out of range");
			return 0;
		}
		a.remove(i);
	}
	return 0;
}

SUNABA_LUA_FUNC(array_resize) {

	ARRAY_SELF("resize");
	{
		Array a = self_box->value;
		a.resize((int)lua_tointeger(L, 2));
	}
	return 0;
}

SUNABA_LUA_FUNC(array_reverse) {

	ARRAY_SELF("reverse");
	{
		Array a = self_box->value;
		a.invert();
	}
	return 0;
}

SUNABA_LUA_FUNC(array_shuffle) {

	ARRAY_SELF("shuffle");
	{
		Array a = self_box->value;
		for (int i = a.size() - 1; i > 0; i--) {
			int j = Math::rand() % (i + 1);
			Variant tmp = a[i];
			a[i] = a[j];
			a[j] = tmp;
		}
	}
	return 0;
}

SUNABA_LUA_FUNC(array_size) {

	ARRAY_SELF("size");
	int n;
	{
		Array a = self_box->value;
		n = a.size();
	}
	lua_pushinteger(L, n);
	return 1;
}

// Godot 4 semantics: [begin, end) with negative indices counted from the end.
SUNABA_LUA_FUNC(array_slice) {

	ARRAY_SELF("slice");
	{
		Array a = self_box->value;
		int size = a.size();
		int begin = lua_isnoneornil(L, 2) ? 0 : (int)lua_tointeger(L, 2);
		int end = lua_isnoneornil(L, 3) ? size : (int)lua_tointeger(L, 3);
		int step = lua_isnoneornil(L, 4) ? 1 : (int)lua_tointeger(L, 4);
		bool deep = lua_toboolean(L, 5);
		if (step == 0) {
			err.format("ArrayList.slice: step cannot be 0");
			return 0;
		}
		begin = CLAMP(normalize_index(begin, size), 0, size);
		end = CLAMP(normalize_index(end, size), 0, size);

		Array out(true);
		if (step > 0) {
			for (int i = begin; i < end; i += step)
				out.push_back(deep ? deep_copy(a[i]) : a[i]);
		} else {
			if (begin >= size)
				begin = size - 1;
			for (int i = begin; i > end; i += step)
				out.push_back(deep ? deep_copy(a[i]) : a[i]);
		}
		push_box(L, out, SUNABA_MT_ARRAYLIST);
	}
	return 1;
}

SUNABA_LUA_FUNC(array_sort) {

	ARRAY_SELF("sort");
	{
		Array a = self_box->value;
		a.sort();
	}
	return 0;
}

SUNABA_LUA_FUNC(array_eq) {

	bool eq = false;
	{
		Array a, b;
		if (to_array(L, 1, a) && to_array(L, 2, b))
			eq = arrays_equal(a, b);
	}
	lua_pushboolean(L, eq);
	return 1;
}

SUNABA_LUA_FUNC(array_index) {

	if (lua_type(L, 2) == LUA_TNUMBER) {
		ARRAY_SELF("__index");
		{
			Array a = self_box->value;
			int i = (int)lua_tointeger(L, 2);
			if (i < 0 || i >= a.size()) {
				lua_pushnil(L);
				return 1;
			}
			push_variant(L, a[i]);
		}
		return 1;
	}
	push_methods(L, SUNABA_MT_ARRAYLIST);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	return 1;
}

SUNABA_LUA_FUNC(array_newindex) {

	ARRAY_SELF("__newindex");
	if (lua_type(L, 2) != LUA_TNUMBER) {
		err.format("ArrayList: index must be a number");
		return 0;
	}
	{
		Array a = self_box->value;
		int i = (int)lua_tointeger(L, 2);
		if (i < 0 || i >= a.size()) {
			err.format("ArrayList: index %d out of range (size %d)", i, a.size());
			return 0;
		}
		a[i] = to_variant(L, 3);
	}
	return 0;
}

// Stateless iterator: (self, key) -> key + 1, value. Keys are 0-based.
SUNABA_LUA_FUNC(array_next) {

	ARRAY_SELF("__pairs");
	int i = lua_isnil(L, 2) ? 0 : (int)lua_tointeger(L, 2) + 1;
	{
		Array a = self_box->value;
		if (i >= a.size()) {
			lua_pushnil(L);
			return 1;
		}
		lua_pushinteger(L, i);
		push_variant(L, a[i]);
	}
	return 2;
}

static int array_pairs(lua_State *L) {

	lua_pushcfunction(L, array_next);
	lua_pushvalue(L, 1);
	lua_pushnil(L);
	return 3;
}

static const luaL_Reg array_methods[] = {
	{ "new", array_new },
	{ "append", array_append },
	{ "appendArray", array_append_array },
	{ "assign", array_assign },
	{ "back", array_back },
	{ "bsearch", array_bsearch },
	{ "clear", array_clear },
	{ "count", array_count },
	{ "duplicate", array_duplicate },
	{ "erase", array_erase },
	{ "fill", array_fill },
	{ "filter", array_filter },
	{ "find", array_find },
	{ "front", array_front },
	{ "get", array_get },
	{ "has", array_has },
	{ "hash", array_hash },
	{ "insert", array_insert },
	{ "isEmpty", array_is_empty },
	{ "isReadOnly", array_is_read_only },
	{ "makeReadOnly", noop },
	{ "max", array_max },
	{ "min", array_min },
	{ "pickRandom", array_pick_random },
	{ "popAt", array_pop_at },
	{ "popBack", array_pop_back },
	{ "popFront", array_pop_front },
	{ "pushBack", array_append },
	{ "pushFront", array_push_front },
	{ "removeAt", array_remove_at },
	{ "resize", array_resize },
	{ "reverse", array_reverse },
	{ "rfind", array_rfind },
	{ "set", array_set },
	{ "shuffle", array_shuffle },
	{ "size", array_size },
	{ "slice", array_slice },
	{ "sort", array_sort },
	{ "eq", array_eq },
	{ NULL, NULL }
};

static const luaL_Reg array_meta[] = {
	{ "__gc", box_gc_func },
	{ "__index", array_index },
	{ "__newindex", array_newindex },
	{ "__len", array_size },
	{ "__pairs", array_pairs },
	{ "__tostring", box_tostring_func },
	{ "__eq", array_eq },
	{ NULL, NULL }
};

/* Dictionary */

static VariantBox *test_dict(lua_State *L, int p_idx) {

	VariantBox *b = test_box(L, p_idx);
	if (!b || b->value.get_type() != Variant::DICTIONARY)
		return NULL;
	return b;
}

#define DICT_SELF(m_name)                                           \
	VariantBox *self_box = test_dict(L, 1);                         \
	if (!self_box) {                                                \
		err.format("Dictionary.%s: expected a Dictionary", m_name); \
		return 0;                                                   \
	}

static bool to_dictionary(lua_State *L, int p_idx, Dictionary &r_dict) {

	VariantBox *b = test_box(L, p_idx);
	if (!b || b->value.get_type() != Variant::DICTIONARY)
		return false;
	r_dict = b->value;
	return true;
}

SUNABA_LUA_FUNC(dict_new) {

	push_box(L, Dictionary(true), SUNABA_MT_DICTIONARY);
	return 1;
}

SUNABA_LUA_FUNC(dict_assign) {

	DICT_SELF("assign");
	{
		Dictionary d = self_box->value;
		Dictionary other;
		if (!to_dictionary(L, 2, other)) {
			err.format("Dictionary.assign: expected a Dictionary");
			return 0;
		}
		Dictionary copy = deep_copy(other);
		d.clear();
		const Variant *k = NULL;
		while ((k = copy.next(k)))
			d[*k] = copy[*k];
	}
	return 0;
}

SUNABA_LUA_FUNC(dict_clear) {

	DICT_SELF("clear");
	{
		Dictionary d = self_box->value;
		d.clear();
	}
	return 0;
}

SUNABA_LUA_FUNC(dict_duplicate) {

	DICT_SELF("duplicate");
	{
		Dictionary d = self_box->value;
		Variant copy;
		if (lua_toboolean(L, 2)) {
			copy = deep_copy(d);
		} else {
			Dictionary dst(true);
			const Variant *k = NULL;
			while ((k = d.next(k)))
				dst[*k] = d[*k];
			copy = dst;
		}
		push_box(L, copy, SUNABA_MT_DICTIONARY);
	}
	return 1;
}

SUNABA_LUA_FUNC(dict_erase) {

	DICT_SELF("erase");
	bool had;
	{
		Dictionary d = self_box->value;
		Variant key = to_variant(L, 2);
		had = d.has(key);
		d.erase(key);
	}
	lua_pushboolean(L, had);
	return 1;
}

SUNABA_LUA_FUNC(dict_find_key) {

	DICT_SELF("findKey");
	{
		Dictionary d = self_box->value;
		Variant value = to_variant(L, 2);
		const Variant *k = NULL;
		while ((k = d.next(k))) {
			if (variants_equal(d[*k], value)) {
				push_variant(L, *k);
				return 1;
			}
		}
		push_variant(L, Variant());
	}
	return 1;
}

SUNABA_LUA_FUNC(dict_get) {

	DICT_SELF("get");
	{
		Dictionary d = self_box->value;
		const Variant *v = d.getptr(to_variant(L, 2));
		push_variant(L, v ? *v : to_variant(L, 3));
	}
	return 1;
}

SUNABA_LUA_FUNC(dict_get_or_add) {

	DICT_SELF("getOrAdd");
	{
		Dictionary d = self_box->value;
		Variant key = to_variant(L, 2);
		if (!d.has(key))
			d[key] = to_variant(L, 3);
		push_variant(L, d[key]);
	}
	return 1;
}

SUNABA_LUA_FUNC(dict_has) {

	DICT_SELF("has");
	bool has;
	{
		Dictionary d = self_box->value;
		has = d.has(to_variant(L, 2));
	}
	lua_pushboolean(L, has);
	return 1;
}

SUNABA_LUA_FUNC(dict_has_all) {

	DICT_SELF("hasAll");
	bool has;
	{
		Dictionary d = self_box->value;
		Array keys;
		if (!to_array(L, 2, keys)) {
			err.format("Dictionary.hasAll: expected an array of keys");
			return 0;
		}
		has = d.has_all(keys);
	}
	lua_pushboolean(L, has);
	return 1;
}

SUNABA_LUA_FUNC(dict_hash) {

	DICT_SELF("hash");
	uint32_t h;
	{
		Dictionary d = self_box->value;
		h = d.hash();
	}
	lua_pushinteger(L, h);
	return 1;
}

SUNABA_LUA_FUNC(dict_is_empty) {

	DICT_SELF("isEmpty");
	bool empty;
	{
		Dictionary d = self_box->value;
		empty = d.empty();
	}
	lua_pushboolean(L, empty);
	return 1;
}

SUNABA_LUA_FUNC(dict_keys) {

	DICT_SELF("keys");
	{
		Dictionary d = self_box->value;
		push_box(L, d.keys(), SUNABA_MT_ARRAYLIST);
	}
	return 1;
}

SUNABA_LUA_FUNC(dict_values) {

	DICT_SELF("values");
	{
		Dictionary d = self_box->value;
		push_box(L, d.values(), SUNABA_MT_ARRAYLIST);
	}
	return 1;
}

static void merge_into(Dictionary &p_dst, const Dictionary &p_src, bool p_overwrite) {

	const Variant *k = NULL;
	while ((k = p_src.next(k))) {
		if (p_overwrite || !p_dst.has(*k))
			p_dst[*k] = p_src[*k];
	}
}

SUNABA_LUA_FUNC(dict_merge) {

	DICT_SELF("merge");
	{
		Dictionary d = self_box->value;
		Dictionary other;
		if (!to_dictionary(L, 2, other)) {
			err.format("Dictionary.merge: expected a Dictionary");
			return 0;
		}
		merge_into(d, other, lua_toboolean(L, 3));
	}
	return 0;
}

SUNABA_LUA_FUNC(dict_merged) {

	DICT_SELF("merged");
	{
		Dictionary d = self_box->value;
		Dictionary other;
		if (!to_dictionary(L, 2, other)) {
			err.format("Dictionary.merged: expected a Dictionary");
			return 0;
		}
		Dictionary out(true);
		merge_into(out, d, true);
		merge_into(out, other, lua_toboolean(L, 3));
		push_box(L, out, SUNABA_MT_DICTIONARY);
	}
	return 1;
}

SUNABA_LUA_FUNC(dict_recursive_equal) {

	bool eq = false;
	{
		Dictionary a, b;
		if (to_dictionary(L, 1, a) && to_dictionary(L, 2, b))
			eq = dictionaries_equal(a, b);
	}
	lua_pushboolean(L, eq);
	return 1;
}

SUNABA_LUA_FUNC(dict_set) {

	DICT_SELF("set");
	{
		Dictionary d = self_box->value;
		d[to_variant(L, 2)] = to_variant(L, 3);
	}
	lua_pushboolean(L, true);
	return 1;
}

SUNABA_LUA_FUNC(dict_size) {

	DICT_SELF("size");
	int n;
	{
		Dictionary d = self_box->value;
		n = d.size();
	}
	lua_pushinteger(L, n);
	return 1;
}

SUNABA_LUA_FUNC(dict_index) {

	if (lua_type(L, 2) == LUA_TSTRING) {
		push_methods(L, SUNABA_MT_DICTIONARY);
		lua_pushvalue(L, 2);
		lua_rawget(L, -2);
		if (!lua_isnil(L, -1))
			return 1;
		lua_pop(L, 2);
	}
	DICT_SELF("__index");
	{
		Dictionary d = self_box->value;
		const Variant *v = d.getptr(to_variant(L, 2));
		push_variant(L, v ? *v : Variant());
	}
	return 1;
}

SUNABA_LUA_FUNC(dict_newindex) {

	DICT_SELF("__newindex");
	{
		Dictionary d = self_box->value;
		d[to_variant(L, 2)] = to_variant(L, 3);
	}
	return 0;
}

SUNABA_LUA_FUNC(dict_next) {

	DICT_SELF("__pairs");
	{
		Dictionary d = self_box->value;
		const Variant *k = NULL;
		if (!lua_isnil(L, 2)) {
			Variant prev = to_variant(L, 2);
			k = d.next(&prev);
		} else {
			k = d.next(NULL);
		}
		if (!k) {
			lua_pushnil(L);
			return 1;
		}
		push_variant(L, *k);
		push_variant(L, d[*k]);
	}
	return 2;
}

static int dict_pairs(lua_State *L) {

	lua_pushcfunction(L, dict_next);
	lua_pushvalue(L, 1);
	lua_pushnil(L);
	return 3;
}

static const luaL_Reg dict_methods[] = {
	{ "new", dict_new },
	{ "assign", dict_assign },
	{ "clear", dict_clear },
	{ "duplicate", dict_duplicate },
	{ "erase", dict_erase },
	{ "findKey", dict_find_key },
	{ "get", dict_get },
	{ "getOrAdd", dict_get_or_add },
	{ "has", dict_has },
	{ "hasAll", dict_has_all },
	{ "hash", dict_hash },
	{ "isEmpty", dict_is_empty },
	{ "isReadOnly", array_is_read_only },
	{ "keys", dict_keys },
	{ "makeReadOnly", noop },
	{ "merge", dict_merge },
	{ "merged", dict_merged },
	{ "recursiveEqual", dict_recursive_equal },
	{ "set", dict_set },
	{ "size", dict_size },
	{ "sort", noop },
	{ "values", dict_values },
	{ "eq", dict_recursive_equal },
	{ NULL, NULL }
};

static const luaL_Reg dict_meta[] = {
	{ "__gc", box_gc_func },
	{ "__index", dict_index },
	{ "__newindex", dict_newindex },
	{ "__len", dict_size },
	{ "__pairs", dict_pairs },
	{ "__tostring", box_tostring_func },
	{ "__eq", dict_recursive_equal },
	{ NULL, NULL }
};

void open_containers(lua_State *L) {

	new_type(L, SUNABA_MT_ARRAYLIST, KIND_VARIANT_BOX, array_methods, array_meta);
	new_type(L, SUNABA_MT_DICTIONARY, KIND_VARIANT_BOX, dict_methods, dict_meta);
}

} // namespace sunaba
