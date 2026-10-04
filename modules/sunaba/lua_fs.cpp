/*************************************************************************/
/*  lua_fs.cpp                                                           */
/*************************************************************************/
/*  `GodotFS`: file system access for Lua through Godot's FileAccess and */
/*  DirAccess, so res://, user:// and absolute paths work the same on    */
/*  every platform (including the 3DS, whose C library has no popen or   */
/*  directory iteration that Lua could use). Haxe's sys.FileSystem and   */
/*  sys.io.File are implemented on top of it (see libsunaba api-godot2). */
/*                                                                       */
/*  Functions return nil (plus a message where useful) on failure instead */
/*  of raising, like Lua's io library.                                   */
/*************************************************************************/

#include "lua_bridge.h"

#include "globals.h"
#include "os/dir_access.h"
#include "os/file_access.h"
#include "os/os.h"

namespace sunaba {

#define SUNABA_MT_FILE "GodotFile"

struct FileBox {
	FileAccess *file;
};

static String arg_path(lua_State *L, int p_idx) {

	return to_string(L, p_idx);
}

static bool is_dir_path(const String &p_path) {

	DirAccess *d = DirAccess::create_for_path(p_path);
	bool ret = d->dir_exists(p_path);
	memdelete(d);
	return ret;
}

SUNABA_LUA_FUNC(fs_exists) {

	bool ret;
	{
		String p = arg_path(L, 1);
		ret = FileAccess::exists(p) || is_dir_path(p);
	}
	lua_pushboolean(L, ret);
	return 1;
}

SUNABA_LUA_FUNC(fs_is_dir) {

	bool ret;
	{
		ret = is_dir_path(arg_path(L, 1));
	}
	lua_pushboolean(L, ret);
	return 1;
}

SUNABA_LUA_FUNC(fs_is_file) {

	bool ret;
	{
		ret = FileAccess::exists(arg_path(L, 1));
	}
	lua_pushboolean(L, ret);
	return 1;
}

SUNABA_LUA_FUNC(fs_size) {

	lua_Integer size = -1;
	{
		FileAccess *f = FileAccess::open(arg_path(L, 1), FileAccess::READ);
		if (f) {
			size = f->get_len();
			memdelete(f);
		}
	}
	if (size < 0)
		lua_pushnil(L);
	else
		lua_pushinteger(L, size);
	return 1;
}

SUNABA_LUA_FUNC(fs_mtime) {

	lua_Integer t;
	{
		String p = arg_path(L, 1);
		t = (FileAccess::exists(p) || is_dir_path(p)) ? (lua_Integer)FileAccess::get_modified_time(p) : -1;
	}
	if (t < 0)
		lua_pushnil(L);
	else
		lua_pushinteger(L, t);
	return 1;
}

// Names in a directory, without "." and "..". nil if it is not a directory.
SUNABA_LUA_FUNC(fs_list) {

	bool ok = false;
	{
		String p = arg_path(L, 1);
		DirAccess *d = DirAccess::create_for_path(p);
		if (d->change_dir(p) == OK && !d->list_dir_begin()) {
			lua_newtable(L);
			int i = 1;
			String n = d->get_next();
			while (n != "") {
				if (n != "." && n != "..") {
					CharString cs = n.utf8();
					lua_pushlstring(L, cs.get_data(), cs.length());
					lua_rawseti(L, -2, i++);
				}
				n = d->get_next();
			}
			d->list_dir_end();
			ok = true;
		}
		memdelete(d);
	}
	if (!ok)
		lua_pushnil(L);
	return 1;
}

SUNABA_LUA_FUNC(fs_mkdir) {

	bool ok;
	{
		String p = arg_path(L, 1);
		DirAccess *d = DirAccess::create_for_path(p);
		ok = d->dir_exists(p) || d->make_dir_recursive(p) == OK;
		memdelete(d);
	}
	lua_pushboolean(L, ok);
	return 1;
}

SUNABA_LUA_FUNC(fs_remove) {

	bool ok;
	{
		String p = arg_path(L, 1);
		DirAccess *d = DirAccess::create_for_path(p);
		ok = d->remove(p) == OK;
		memdelete(d);
	}
	lua_pushboolean(L, ok);
	return 1;
}

SUNABA_LUA_FUNC(fs_rename) {

	bool ok;
	{
		String a = arg_path(L, 1);
		DirAccess *d = DirAccess::create_for_path(a);
		ok = d->rename(a, arg_path(L, 2)) == OK;
		memdelete(d);
	}
	lua_pushboolean(L, ok);
	return 1;
}

SUNABA_LUA_FUNC(fs_copy) {

	bool ok;
	{
		String a = arg_path(L, 1);
		DirAccess *d = DirAccess::create_for_path(a);
		ok = d->copy(a, arg_path(L, 2)) == OK;
		memdelete(d);
	}
	lua_pushboolean(L, ok);
	return 1;
}

SUNABA_LUA_FUNC(fs_read_all) {

	bool ok = false;
	{
		FileAccess *f = FileAccess::open(arg_path(L, 1), FileAccess::READ);
		if (f) {
			int len = f->get_len();
			Vector<uint8_t> buf;
			buf.resize(len > 0 ? len : 1);
			int n = len > 0 ? f->get_buffer(&buf[0], len) : 0;
			memdelete(f);
			lua_pushlstring(L, (const char *)&buf[0], n);
			ok = true;
		}
	}
	if (!ok) {
		lua_pushnil(L);
		lua_pushstring(L, "cannot open file");
		return 2;
	}
	return 1;
}

SUNABA_LUA_FUNC(fs_write_all) {

	size_t len = 0;
	const char *data = luaL_checklstring(L, 2, &len);
	bool append = lua_toboolean(L, 3);
	bool ok = false;
	{
		String p = arg_path(L, 1);
		FileAccess *f = NULL;
		if (append && FileAccess::exists(p)) {
			f = FileAccess::open(p, FileAccess::READ_WRITE);
			if (f)
				f->seek_end();
		} else {
			f = FileAccess::open(p, FileAccess::WRITE);
		}
		if (f) {
			f->store_buffer((const uint8_t *)data, (int)len);
			memdelete(f);
			ok = true;
		}
	}
	lua_pushboolean(L, ok);
	return 1;
}

SUNABA_LUA_FUNC(fs_cwd) {

	{
		DirAccess *d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		CharString cs = d->get_current_dir().utf8();
		memdelete(d);
		lua_pushlstring(L, cs.get_data(), cs.length());
	}
	return 1;
}

// Real filesystem path for res:// and user:// paths; relative paths are
// joined to the current directory, and absolute ones returned unchanged.
SUNABA_LUA_FUNC(fs_absolute) {

	{
		String p = arg_path(L, 1);
		if (p.begins_with("res://"))
			p = Globals::get_singleton()->globalize_path(p);
		else if (p.begins_with("user://"))
			p = OS::get_singleton()->get_data_dir().plus_file(p.substr(7, p.length()));
		else if (p.is_rel_path()) {
			DirAccess *d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
			p = d->get_current_dir().plus_file(p).simplify_path();
			memdelete(d);
		}
		CharString cs = p.utf8();
		lua_pushlstring(L, cs.get_data(), cs.length());
	}
	return 1;
}

/* File handles */

static FileBox *check_file(lua_State *L, int p_idx) {

	return (FileBox *)luaL_checkudata(L, p_idx, SUNABA_MT_FILE);
}

static FileAccess *open_file(lua_State *L, int p_idx) {

	FileBox *b = check_file(L, p_idx);
	if (!b->file)
		luaL_error(L, "attempt to use a closed file");
	return b->file;
}

SUNABA_LUA_FUNC(fs_open) {

	bool ok = false;
	FileBox *box = NULL;
	{
		String p = arg_path(L, 1);
		String mode = lua_isnoneornil(L, 2) ? String("r") : to_string(L, 2);
		bool plus = mode.find("+") >= 0;
		FileAccess *f = NULL;
		if (mode.begins_with("r")) {
			f = FileAccess::open(p, plus ? FileAccess::READ_WRITE : FileAccess::READ);
		} else if (mode.begins_with("w")) {
			f = FileAccess::open(p, FileAccess::WRITE);
		} else if (mode.begins_with("a")) {
			if (FileAccess::exists(p)) {
				f = FileAccess::open(p, FileAccess::READ_WRITE);
				if (f)
					f->seek_end();
			} else {
				f = FileAccess::open(p, FileAccess::WRITE);
			}
		}
		if (f) {
			box = (FileBox *)lua_newuserdatauv(L, sizeof(FileBox), 0);
			box->file = f;
			luaL_setmetatable(L, SUNABA_MT_FILE);
			ok = true;
		}
	}
	if (!ok) {
		lua_pushnil(L);
		lua_pushstring(L, "cannot open file");
		return 2;
	}
	return 1;
}

static int file_gc(lua_State *L) {

	FileBox *b = (FileBox *)lua_touserdata(L, 1);
	if (b && b->file) {
		memdelete(b->file);
		b->file = NULL;
	}
	return 0;
}

SUNABA_LUA_FUNC(file_close) {

	FileBox *b = check_file(L, 1);
	if (b->file) {
		memdelete(b->file);
		b->file = NULL;
	}
	lua_pushboolean(L, true);
	return 1;
}

// read(n): up to n bytes, nil at EOF. read(): everything that is left.
SUNABA_LUA_FUNC(file_read) {

	FileAccess *f = open_file(L, 1);
	int want;
	if (lua_isnoneornil(L, 2)) {
		want = (int)f->get_len() - (int)f->get_pos();
	} else {
		want = (int)luaL_checkinteger(L, 2);
		if (f->eof_reached() || (int)f->get_pos() >= (int)f->get_len()) {
			lua_pushnil(L);
			return 1;
		}
	}
	if (want <= 0) {
		lua_pushliteral(L, "");
		return 1;
	}
	{
		Vector<uint8_t> buf;
		buf.resize(want);
		int n = f->get_buffer(&buf[0], want);
		lua_pushlstring(L, (const char *)&buf[0], n);
	}
	return 1;
}

SUNABA_LUA_FUNC(file_read_line) {

	FileAccess *f = open_file(L, 1);
	if ((int)f->get_pos() >= (int)f->get_len()) {
		lua_pushnil(L);
		return 1;
	}
	{
		// Raw bytes up to the newline, so binary-safe and encoding-agnostic.
		luaL_Buffer lb;
		luaL_buffinit(L, &lb);
		while ((int)f->get_pos() < (int)f->get_len()) {
			uint8_t c = f->get_8();
			if (c == '\n')
				break;
			luaL_addchar(&lb, (char)c);
		}
		luaL_pushresult(&lb);
	}
	return 1;
}

SUNABA_LUA_FUNC(file_write) {

	FileAccess *f = open_file(L, 1);
	size_t len = 0;
	const char *data = luaL_checklstring(L, 2, &len);
	f->store_buffer((const uint8_t *)data, (int)len);
	lua_pushvalue(L, 1);
	return 1;
}

SUNABA_LUA_FUNC(file_seek) {

	FileAccess *f = open_file(L, 1);
	const char *whence = luaL_optstring(L, 2, "cur");
	lua_Integer off = luaL_optinteger(L, 3, 0);
	if (strcmp(whence, "set") == 0)
		f->seek((size_t)MAX(off, (lua_Integer)0));
	else if (strcmp(whence, "end") == 0)
		f->seek_end(off);
	else
		f->seek((size_t)MAX((lua_Integer)f->get_pos() + off, (lua_Integer)0));
	lua_pushinteger(L, f->get_pos());
	return 1;
}

SUNABA_LUA_FUNC(file_tell) {

	lua_pushinteger(L, open_file(L, 1)->get_pos());
	return 1;
}

SUNABA_LUA_FUNC(file_size) {

	lua_pushinteger(L, open_file(L, 1)->get_len());
	return 1;
}

SUNABA_LUA_FUNC(file_eof) {

	FileAccess *f = open_file(L, 1);
	lua_pushboolean(L, f->eof_reached() || (int)f->get_pos() >= (int)f->get_len());
	return 1;
}

static int file_flush(lua_State *L) {

	open_file(L, 1);
	return 0; // FileAccess writes through
}

static const luaL_Reg fs_funcs[] = {
	{ "exists", fs_exists },
	{ "is_dir", fs_is_dir },
	{ "is_file", fs_is_file },
	{ "size", fs_size },
	{ "mtime", fs_mtime },
	{ "list", fs_list },
	{ "mkdir", fs_mkdir },
	{ "remove", fs_remove },
	{ "rename", fs_rename },
	{ "copy", fs_copy },
	{ "read_all", fs_read_all },
	{ "write_all", fs_write_all },
	{ "cwd", fs_cwd },
	{ "absolute", fs_absolute },
	{ "open", fs_open },
	{ NULL, NULL }
};

static const luaL_Reg file_methods[] = {
	{ "close", file_close },
	{ "read", file_read },
	{ "read_line", file_read_line },
	{ "write", file_write },
	{ "seek", file_seek },
	{ "tell", file_tell },
	{ "size", file_size },
	{ "eof", file_eof },
	{ "flush", file_flush },
	{ NULL, NULL }
};

void open_fs(lua_State *L) {

	luaL_newmetatable(L, SUNABA_MT_FILE);
	lua_newtable(L);
	luaL_setfuncs(L, file_methods, 0);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, file_gc);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);

	luaL_newlib(L, fs_funcs);
	lua_setglobal(L, "GodotFS");
}

} // namespace sunaba
