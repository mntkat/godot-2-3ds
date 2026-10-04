/*************************************************************************/
/*  lua_io.cpp                                                           */
/*************************************************************************/
/*  Byte and BinaryData (published as both `BinaryData` and `ByteArray`, */
/*  the name the Haxe extern uses).                                      */
/*                                                                       */
/*  A Byte is an 8-byte cell: Byte.new(integer) stores an int, and       */
/*  Byte.new(float) stores a double; getInt/getFloat reinterpret it, as  */
/*  libsunaba's io::Byte did. Arrays store cells by value, so get()      */
/*  returns a copy. Converting to a raw byte array keeps each cell's     */
/*  first (low) byte.                                                    */
/*************************************************************************/

#include "lua_bridge.h"

namespace sunaba {

static void cell_set_int(ByteBox &r_cell, int p_value) {

	memset(r_cell.data, 0, sizeof(r_cell.data));
	memcpy(r_cell.data, &p_value, sizeof(int));
}

static void cell_set_double(ByteBox &r_cell, double p_value) {

	memcpy(r_cell.data, &p_value, sizeof(double));
}

static int cell_get_int(const ByteBox &p_cell) {

	int v;
	memcpy(&v, p_cell.data, sizeof(int));
	return v;
}

static double cell_get_double(const ByteBox &p_cell) {

	double v;
	memcpy(&v, p_cell.data, sizeof(double));
	return v;
}

// Reads a Byte or number at p_idx into r_cell.
static bool to_cell(lua_State *L, int p_idx, ByteBox &r_cell) {

	ByteBox *b = (ByteBox *)test_kind(L, p_idx, KIND_BYTE);
	if (b) {
		r_cell = *b;
		return true;
	}
	if (lua_type(L, p_idx) == LUA_TNUMBER) {
		if (lua_isinteger(L, p_idx))
			cell_set_int(r_cell, (int)lua_tointeger(L, p_idx));
		else
			cell_set_double(r_cell, lua_tonumber(L, p_idx));
		return true;
	}
	return false;
}

static ByteBox *push_byte(lua_State *L, const ByteBox &p_cell) {

	ByteBox *b = (ByteBox *)lua_newuserdatauv(L, sizeof(ByteBox), 0);
	*b = p_cell;
	luaL_setmetatable(L, SUNABA_MT_BYTE);
	return b;
}

/* Byte */

static ByteBox *check_byte(lua_State *L) {

	return (ByteBox *)test_kind(L, 1, KIND_BYTE);
}

#define BYTE_SELF(m_name)                              \
	ByteBox *self_box = check_byte(L);                 \
	if (!self_box) {                                   \
		err.format("Byte.%s: expected a Byte", m_name); \
		return 0;                                      \
	}

SUNABA_LUA_FUNC(byte_new) {

	ByteBox cell;
	memset(cell.data, 0, sizeof(cell.data));
	if (!lua_isnoneornil(L, 1) && !to_cell(L, 1, cell)) {
		err.format("Byte.new: expected a number");
		return 0;
	}
	push_byte(L, cell);
	return 1;
}

SUNABA_LUA_FUNC(byte_get_int) {

	BYTE_SELF("getInt");
	lua_pushinteger(L, cell_get_int(*self_box));
	return 1;
}

SUNABA_LUA_FUNC(byte_get_float) {

	BYTE_SELF("getFloat");
	lua_pushnumber(L, cell_get_double(*self_box));
	return 1;
}

SUNABA_LUA_FUNC(byte_set_int) {

	BYTE_SELF("setInt");
	cell_set_int(*self_box, (int)lua_tointeger(L, 2));
	return 0;
}

SUNABA_LUA_FUNC(byte_set_float) {

	BYTE_SELF("setFloat");
	cell_set_double(*self_box, lua_tonumber(L, 2));
	return 0;
}

static const luaL_Reg byte_methods[] = {
	{ "new", byte_new },
	{ "getInt", byte_get_int },
	{ "getInt64", byte_get_int },
	{ "getFloat", byte_get_float },
	{ "setInt", byte_set_int },
	{ "setInt64", byte_set_int },
	{ "setFloat", byte_set_float },
	{ NULL, NULL }
};

/* BinaryData */

static ByteArrayBox *check_bytes(lua_State *L) {

	return (ByteArrayBox *)test_kind(L, 1, KIND_BYTEARRAY);
}

#define BYTES_SELF(m_name)                                     \
	ByteArrayBox *self_box = check_bytes(L);                   \
	if (!self_box) {                                           \
		err.format("ByteArray.%s: expected a ByteArray", m_name); \
		return 0;                                              \
	}

SUNABA_LUA_FUNC(bytes_new) {

	bool from_table = lua_type(L, 1) == LUA_TTABLE;
	int n = from_table ? (int)lua_rawlen(L, 1) : 0;
	ByteArrayBox *ba = (ByteArrayBox *)lua_newuserdatauv(L, sizeof(ByteArrayBox), 0);
	memnew_placement(ba, ByteArrayBox);
	luaL_setmetatable(L, SUNABA_MT_BYTEARRAY);
	if (from_table) {
		ba->cells.resize(n);
		for (int i = 0; i < n; i++) {
			lua_rawgeti(L, 1, i + 1);
			ByteBox cell;
			memset(cell.data, 0, sizeof(cell.data));
			to_cell(L, -1, cell);
			ba->cells[i] = cell;
			lua_pop(L, 1);
		}
	}
	return 1;
}

SUNABA_LUA_FUNC(bytes_size) {

	BYTES_SELF("size");
	lua_pushinteger(L, self_box->cells.size());
	return 1;
}

SUNABA_LUA_FUNC(bytes_get) {

	BYTES_SELF("get");
	int i = (int)lua_tointeger(L, 2);
	if (i < 0 || i >= self_box->cells.size()) {
		err.format("ByteArray.get: index %d out of range (size %d)", i, self_box->cells.size());
		return 0;
	}
	push_byte(L, self_box->cells[i]);
	return 1;
}

SUNABA_LUA_FUNC(bytes_set) {

	BYTES_SELF("set");
	int i = (int)lua_tointeger(L, 2);
	ByteBox cell;
	if (i < 0 || i >= self_box->cells.size() || !to_cell(L, 3, cell)) {
		err.format("ByteArray.set: invalid index or value");
		return 0;
	}
	self_box->cells[i] = cell;
	return 0;
}

SUNABA_LUA_FUNC(bytes_resize) {

	BYTES_SELF("resize");
	int old = self_box->cells.size();
	int n = (int)lua_tointeger(L, 2);
	if (n < 0) {
		err.format("ByteArray.resize: negative size");
		return 0;
	}
	self_box->cells.resize(n);
	for (int i = old; i < n; i++)
		memset(self_box->cells[i].data, 0, sizeof(self_box->cells[i].data));
	return 0;
}

SUNABA_LUA_FUNC(bytes_append) {

	BYTES_SELF("append");
	ByteBox cell;
	if (!to_cell(L, 2, cell)) {
		err.format("ByteArray.append: expected a Byte or number");
		return 0;
	}
	self_box->cells.push_back(cell);
	return 0;
}

SUNABA_LUA_FUNC(bytes_insert) {

	BYTES_SELF("insert");
	int i = (int)lua_tointeger(L, 2);
	ByteBox cell;
	if (i < 0 || i > self_box->cells.size() || !to_cell(L, 3, cell)) {
		err.format("ByteArray.insert: invalid index or value");
		return 0;
	}
	self_box->cells.insert(i, cell);
	return 0;
}

SUNABA_LUA_FUNC(bytes_remove_at) {

	BYTES_SELF("removeAt");
	int i = (int)lua_tointeger(L, 2);
	if (i < 0 || i >= self_box->cells.size()) {
		err.format("ByteArray.removeAt: index out of range");
		return 0;
	}
	self_box->cells.remove(i);
	return 0;
}

SUNABA_LUA_FUNC(bytes_clear) {

	BYTES_SELF("clear");
	self_box->cells.clear();
	return 0;
}

// 1-based table of each cell's integer value.
SUNABA_LUA_FUNC(bytes_to_table) {

	BYTES_SELF("toTable");
	lua_createtable(L, self_box->cells.size(), 0);
	for (int i = 0; i < self_box->cells.size(); i++) {
		lua_pushinteger(L, cell_get_int(self_box->cells[i]));
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

static int bytes_gc(lua_State *L) {

	ByteArrayBox *ba = (ByteArrayBox *)lua_touserdata(L, 1);
	if (ba)
		ba->~ByteArrayBox();
	return 0;
}

static const luaL_Reg bytes_methods[] = {
	{ "new", bytes_new },
	{ "size", bytes_size },
	{ "get", bytes_get },
	{ "set", bytes_set },
	{ "resize", bytes_resize },
	{ "append", bytes_append },
	{ "insert", bytes_insert },
	{ "removeAt", bytes_remove_at },
	{ "clear", bytes_clear },
	{ "toTable", bytes_to_table },
	{ NULL, NULL }
};

static const luaL_Reg bytes_meta[] = {
	{ "__gc", bytes_gc },
	{ "__len", bytes_size },
	{ NULL, NULL }
};

void open_io(lua_State *L) {

	new_type(L, SUNABA_MT_BYTE, KIND_BYTE, byte_methods, NULL);
	new_type(L, SUNABA_MT_BYTEARRAY, KIND_BYTEARRAY, bytes_methods, bytes_meta);

	lua_getglobal(L, SUNABA_MT_BYTEARRAY);
	lua_setglobal(L, "ByteArray");
}

} // namespace sunaba
