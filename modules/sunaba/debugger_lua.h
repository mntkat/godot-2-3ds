// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Scott Lembcke and Howling Moon Software

/*
	Using debugger.lua from C code is pretty straightforward.
	Basically you just need to call one of the setup functions to make the debugger available.
	Then you can reference the debugger in your Lua code as normal.
	If you want to wrap the lua code from your C entrypoints, you can use
	dbg_pcall() or dbg_dofile() instead.
	
	That's it!!
	
	#include <stdio.h>
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
	
	#define DEBUGGER_LUA_IMPLEMENTATION
	#include "debugger_lua.h"
	
	int main(int argc, char **argv){
		lua_State *lua = luaL_newstate();
		luaL_openlibs(lua);
		
		// This defines a module named 'debugger' which is assigned to a global named 'dbg'.
		// If you want to change these values or redirect the I/O, then use dbg_setup() instead.
		dbg_setup_default(lua);
		
		luaL_loadstring(lua,
			"local num = 1\n"
			"local str = 'one'\n"
			"local res = num + str\n"
		);
		
		// Call into the lua code, and catch any unhandled errors in the debugger.
		if(dbg_pcall(lua, 0, 0, 0)){
			fprintf(stderr, "Lua Error: %s\n", lua_tostring(lua, -1));
		}
	}
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lua_State lua_State;
typedef int (*lua_CFunction)(lua_State *L);

// This function must be called before calling dbg_pcall() to set up the debugger module.
// 'name' must be the name of the module to register the debugger as. (to use with require 'module')
// 'globalName' can either be NULL or a global variable name to assign the debugger to. (I use "dbg")
// 'readFunc' is a lua_CFunction that returns a line of input when called. Pass NULL if you want to read from stdin.
// 'writeFunc' is a lua_CFunction that takes a single string as an argument. Pass NULL if you want to write to stdout.
void dbg_setup(lua_State *lua, const char *name, const char *globalName, lua_CFunction readFunc, lua_CFunction writeFunc);

// Same as 'dbg_setup(lua, "debugger", "dbg", NULL, NULL)'
void dbg_setup_default(lua_State *lua);

// Drop in replacement for lua_pcall() that attaches the debugger on an error if 'msgh' is 0.
int dbg_pcall(lua_State *lua, int nargs, int nresults, int msgh);

// Drop in replacement for luaL_dofile()
#define dbg_dofile(lua, filename) (luaL_loadfile(lua, filename) || dbg_pcall(lua, 0, LUA_MULTRET, 0))

#ifdef DEBUGGER_LUA_IMPLEMENTATION

#include <stdbool.h>
#include <assert.h>
#include <string.h>

static const char DEBUGGER_SRC[] =
"-- SPDX-License-Identifier: MIT\n-- Copyright (c) 2026 Scott Lembcke and Howling Moon Software\n\nlocal dbg\n\nlocal function"
" pretty(obj, max_depth)\n if max_depth == nil then max_depth = dbg.pretty_depth end\n \n -- Returns true if a table has a _"
"_tostring metamethod.\n local function coerceable(tbl)\n  local meta = getmetatable(tbl)\n  return (meta and meta.__tostrin"
"g)\n end\n \n local function recurse(obj, depth)\n  if type(obj) == \"string\" then\n   -- Dump the string so that escape seque"
"nces are printed.\n   return string.format(\"%q\", obj)\n  elseif type(obj) == \"table\" and depth < max_depth and not coercea"
"ble(obj) then\n   local str = \"{\"\n   \n   for k, v in pairs(obj) do\n    local pair = pretty(k, 0)..\" = \"..recurse(v, depth"
" + 1)\n    str = str..(str == \"{\" and pair or \", \"..pair)\n   end\n   \n   return str..\"}\"\n  else\n   -- tostring() can fail "
"if there is an error in a __tostring metamethod.\n   local success, value = pcall(function() return tostring(obj) end)\n  "
" return (success and value or \"<!!error in __tostring metamethod!!>\")\n  end\n end\n \n return recurse(obj, 0)\nend\n\n-- The s"
"tack level that cmd_* functions use to access locals or info\n-- The structure of the debugger's code *very* carefully en"
"sures this.\nlocal CMD_STACK_LEVEL = 6\n\n-- Location of the top of the stack outside of the debugger.\n-- Adjusted by some "
"debugger entrypoints.\nlocal stack_top = 0\n\n-- The current stack frame index.\n-- Changed using the up/down commands\nlocal"
" stack_inspect_offset = 0\n\n-- LuaJIT has an off by one bug when setting local variables.\nlocal LUA_JIT_SETLOCAL_WORKAROU"
"ND = 0\n\n-- Default dbg.read function\nlocal function dbg_read(prompt)\n dbg.write(prompt)\n io.flush()\n return io.read()\nen"
"d\n\n-- Default dbg.write function\nlocal function dbg_write(str)\n io.stderr:write(str)\nend\n\nlocal function dbg_writeln(str"
", ...)\n if select(\"#\", ...) == 0 then\n  dbg.write((str or \"<NULL>\")..\"\\n\")\n else\n  dbg.write(string.format(str..\"\\n\", .."
".))\n end\nend\n\nlocal function format_loc(info, line)\n local filename = info.source:match(\"^@(.*)\")\n local source = filena"
"me and dbg.shorten_path(filename) or info.short_src\n return (dbg.COLOR_BLUE)..source..(dbg.COLOR_RESET)..\":\"..(dbg.COLOR"
"_YELLOW)..line..(dbg.COLOR_RESET)\nend\n\nlocal function format_stack_frame_info(info)\n local namewhat = (info.namewhat == "
"\"\" and \"chunk at\" or info.namewhat)\n local name = (info.name and \"'\"..(dbg.COLOR_BLUE)..(info.name)..(dbg.COLOR_RESET).."
"\"'\" or dbg.format_loc(info, info.linedefined))\n return dbg.format_loc(info, info.currentline)..\" in \"..namewhat..\" \"..na"
"me\nend\n\nlocal repl\n\n-- Return false for stack frames without source,\n-- which includes C frames, Lua bytecode, and `load"
"string` functions\nlocal function frame_has_line(info) return info.currentline >= 0 end\n\nlocal function hook_factory(repl"
"_threshold)\n return function(offset, reason)\n  return function(event, _)\n   -- Skip events that don't have line informat"
"ion.\n   if not frame_has_line(debug.getinfo(2)) then return end\n   \n   -- Tail calls are specifically ignored since they"
" also will have tail returns to balance out.\n   if event == \"call\" then\n    offset = offset + 1\n   elseif event == \"retu"
"rn\" and offset > repl_threshold then\n    offset = offset - 1\n   elseif event == \"line\" and offset <= repl_threshold then"
"\n    repl(reason)\n   end\n  end\n end\nend\n\nlocal hook_step = hook_factory(1)\nlocal hook_next = hook_factory(0)\nlocal hook_"
"finish = hook_factory(-1)\n\n-- Create a table of all the locally accessible variables.\n-- Globals are not included when r"
"unning the locals command, but are when running the print command.\nlocal function local_bindings(offset, include_globals"
")\n local level = offset + stack_inspect_offset + CMD_STACK_LEVEL\n local func = debug.getinfo(level).func\n local bindings"
" = {}\n \n -- Retrieve the upvalues\n do local i = 1; while true do\n  local name, value = debug.getupvalue(func, i)\n  if no"
"t name then break end\n  bindings[name] = value\n  i = i + 1\n end end\n \n -- Retrieve the locals (overwriting any upvalues)"
"\n do local i = 1; while true do\n  local name, value = debug.getlocal(level, i)\n  if not name then break end\n  bindings[n"
"ame] = value\n  i = i + 1\n end end\n \n -- Retrieve the varargs (works in Lua 5.2 and LuaJIT)\n local varargs = {}\n do local"
" i = 1; while true do\n  local name, value = debug.getlocal(level, -i)\n  if not name then break end\n  varargs[i] = value\n"
"  i = i + 1\n end end\n if #varargs > 0 then bindings[\"...\"] = varargs end\n \n if include_globals then\n  -- In Lua 5.2, you"
" have to get the environment table from the function's locals.\n  local env = (_VERSION <= \"Lua 5.1\" and getfenv(func) or"
" bindings._ENV)\n  return setmetatable(bindings, {__index = env or _G})\n else\n  return bindings\n end\nend\n\n-- Used as a __"
"newindex metamethod to modify variables in cmd_eval().\nlocal function mutate_bindings(_, name, value)\n local FUNC_STACK_"
"OFFSET = 3 -- Stack depth of this function.\n local level = stack_inspect_offset + FUNC_STACK_OFFSET + CMD_STACK_LEVEL\n \n"
" -- Set a local.\n do local i = 1; repeat\n  local var = debug.getlocal(level, i)\n  if name == var then\n   dbg_writeln((db"
"g.COLOR_YELLOW)..\"debugger.lua\"..(dbg.GREEN_CARET)..\"Set local variable \"..(dbg.COLOR_BLUE)..name..(dbg.COLOR_RESET))\n  "
" return debug.setlocal(level + LUA_JIT_SETLOCAL_WORKAROUND, i, value)\n  end\n  i = i + 1\n until var == nil end\n \n -- Set "
"an upvalue.\n local func = debug.getinfo(level).func\n do local i = 1; repeat\n  local var = debug.getupvalue(func, i)\n  if"
" name == var then\n   dbg_writeln((dbg.COLOR_YELLOW)..\"debugger.lua\"..(dbg.GREEN_CARET)..\"Set upvalue \"..(dbg.COLOR_BLUE)"
"..name..(dbg.COLOR_RESET))\n   return debug.setupvalue(func, i, value)\n  end\n  i = i + 1\n until var == nil end\n \n -- Set "
"a global.\n dbg_writeln((dbg.COLOR_YELLOW)..\"debugger.lua\"..(dbg.GREEN_CARET)..\"Set global variable \"..(dbg.COLOR_BLUE).."
"name..(dbg.COLOR_RESET))\n _G[name] = value\nend\n\n-- Compile an expression with the given variable bindings.\nlocal functio"
"n compile_chunk(block, env)\n local source = \"debugger.lua REPL\"\n local chunk = nil\n \n if _VERSION <= \"Lua 5.1\" then\n  ch"
"unk = loadstring(block, source)\n  if chunk then setfenv(chunk, env) end\n else\n  -- The Lua 5.2 way is a bit cleaner\n  ch"
"unk = load(block, source, \"t\", env)\n end\n \n if not chunk then dbg_writeln((dbg.COLOR_RED)..\"Error: Could not compile blo"
"ck:\\n\"..(dbg.COLOR_RESET)..block) end\n return chunk\nend\n\nlocal SOURCE_CACHE = {}\n\nlocal function where(info, context_lin"
"es)\n local source = SOURCE_CACHE[info.source]\n if not source then\n  source = {}\n  local filename = info.source:match(\"^@"
"(.*)\")\n  if filename then\n   pcall(function() for line in io.lines(filename) do table.insert(source, line) end end)\n  el"
"seif info.source then\n   for line in info.source:gmatch(\"([^\\n]*)\\n?\") do table.insert(source, line) end\n  end\n  SOURCE_"
"CACHE[info.source] = source\n end\n \n if source and source[info.currentline] then\n  for i = info.currentline - context_lin"
"es, info.currentline + context_lines do\n   local tab_or_caret = (i == info.currentline and  (dbg.GREEN_CARET) or \"    \")"
"\n   local line = source[i]\n   if line then dbg_writeln((dbg.COLOR_GRAY)..\"% 4d\"..tab_or_caret..\"%s\", i, line) end\n  end\n"
" else\n  dbg_writeln((dbg.COLOR_RED)..\"Error: Source not available for \"..(dbg.COLOR_BLUE)..(info.short_src));\n end\n \n re"
"turn false\nend\n\nlocal function cmd_step()\n stack_inspect_offset = stack_top\n return true, hook_step\nend\n\nlocal function "
"cmd_next()\n stack_inspect_offset = stack_top\n return true, hook_next\nend\n\nlocal function cmd_finish()\n local offset = st"
"ack_top - stack_inspect_offset\n stack_inspect_offset = stack_top\n return true, offset < 0 and hook_factory(offset - 1) o"
"r hook_finish\nend\n\n-- Wee Lua version differences\nlocal pack = function(...) return select(\"#\", ...), {...} end\nlocal un"
"pack = unpack or table.unpack\n\nlocal function cmd_print(expr)\n local env = local_bindings(1, true)\n local chunk = compil"
"e_chunk(\"return \"..expr, env)\n if chunk == nil then return false end\n \n -- Call the chunk and collect the results.\n loca"
"l nresults, results = pack(pcall(chunk, unpack(rawget(env, \"...\") or {})))\n \n -- The first result is the pcall error.\n i"
"f not results[1] then\n  dbg_writeln((dbg.COLOR_RED)..\"Error:\"..(dbg.COLOR_RESET)..\" \"..results[2])\n else\n  local output "
"= \"\"\n  for i = 2, nresults do\n   output = output..(i ~= 2 and \", \" or \"\")..dbg.pretty(results[i])\n  end\n  \n  if output ="
"= \"\" then output = \"<no result>\" end\n  dbg_writeln((dbg.COLOR_BLUE)..expr.. (dbg.GREEN_CARET)..output)\n end\n \n return fa"
"lse\nend\n\nlocal function cmd_eval(code)\n local env = local_bindings(1, true)\n local mutable_env = setmetatable({}, {\n  __"
"index = env,\n  __newindex = mutate_bindings,\n })\n \n local chunk = compile_chunk(code, mutable_env)\n if chunk == nil then"
" return false end\n \n -- Call the chunk and collect the results.\n local success, err = pcall(chunk, unpack(rawget(env, \"."
"..\") or {}))\n if not success then\n  dbg_writeln((dbg.COLOR_RED)..\"Error:\"..(dbg.COLOR_RESET)..\" \"..tostring(err))\n end\n "
"\n return false\nend\n\nlocal function cmd_down()\n local offset = stack_inspect_offset\n local info\n \n repeat -- Find the nex"
"t frame with a file.\n  offset = offset + 1\n  info = debug.getinfo(offset + CMD_STACK_LEVEL)\n until not info or frame_has"
"_line(info)\n \n if info then\n  stack_inspect_offset = offset\n  dbg_writeln(\"Inspecting frame: \"..dbg.format_stack_frame_i"
"nfo(info))\n  if tonumber(dbg.auto_where) then where(info, dbg.auto_where) end\n else\n  info = debug.getinfo(stack_inspect"
"_offset + CMD_STACK_LEVEL)\n  dbg_writeln(\"Already at the bottom of the stack.\")\n end\n \n return false\nend\n\nlocal function"
" cmd_up()\n local offset = stack_inspect_offset\n local info\n \n repeat -- Find the next frame with a file.\n  offset = offs"
"et - 1\n  if offset < stack_top then info = nil; break end\n  info = debug.getinfo(offset + CMD_STACK_LEVEL)\n until frame_"
"has_line(info)\n \n if info then\n  stack_inspect_offset = offset\n  dbg_writeln(\"Inspecting frame: \"..dbg.format_stack_fram"
"e_info(info))\n  if tonumber(dbg.auto_where) then where(info, dbg.auto_where) end\n else\n  info = debug.getinfo(stack_insp"
"ect_offset + CMD_STACK_LEVEL)\n  dbg_writeln(\"Already at the top of the stack.\")\n end\n \n return false\nend\n\nlocal function"
" cmd_inspect(offset)\n offset = stack_top + tonumber(offset)\n local info = debug.getinfo(offset + CMD_STACK_LEVEL)\n if in"
"fo then\n  stack_inspect_offset = offset\n  dbg.writeln(\"Inspecting frame: \"..dbg.format_stack_frame_info(info))\n else\n  d"
"bg.writeln((dbg.COLOR_RED)..\"ERROR: \"..(dbg.COLOR_BLUE)..\"Invalid stack frame index.\"..(dbg.COLOR_RESET))\n end\nend\n\nloca"
"l function cmd_where(context_lines)\n local info = debug.getinfo(stack_inspect_offset + CMD_STACK_LEVEL)\n return (info an"
"d where(info, tonumber(context_lines) or 5))\nend\n\nlocal function cmd_trace()\n dbg_writeln(\"Inspecting frame %d\", stack_i"
"nspect_offset - stack_top)\n local i = 0; while true do\n  local info = debug.getinfo(stack_top + CMD_STACK_LEVEL + i)\n  i"
"f not info then break end\n  \n  local is_current_frame = (i + stack_top == stack_inspect_offset)\n  local tab_or_caret = ("
"is_current_frame and  (dbg.GREEN_CARET) or \"    \")\n  dbg_writeln((dbg.COLOR_GRAY)..\"% 4d\"..(dbg.COLOR_RESET)..tab_or_car"
"et..\"%s\", i, dbg.format_stack_frame_info(info))\n  i = i + 1\n end\n \n return false\nend\n\nlocal function cmd_locals()\n local"
" bindings = local_bindings(1, false)\n \n -- Get all the variable binding names and sort them\n local keys = {}\n for k, _ i"
"n pairs(bindings) do table.insert(keys, k) end\n table.sort(keys)\n \n for _, k in ipairs(keys) do\n  local v = bindings[k]\n"
"  \n  -- Skip the debugger object itself, \"(*internal)\" values, and Lua 5.2's _ENV object.\n  if not rawequal(v, dbg) and "
"k ~= \"_ENV\" and not k:match(\"%(.*%)\") then\n   dbg_writeln(\"  \"..(dbg.COLOR_BLUE)..k.. (dbg.GREEN_CARET)..dbg.pretty(v))\n"
"  end\n end\n \n return false\nend\n\nlocal function cmd_help()\n dbg.write(\"\"\n  ..(dbg.COLOR_BLUE)..\"  <return>\"..(dbg.GREEN_C"
"ARET)..\"re-run last command\\n\"\n  ..(dbg.COLOR_BLUE)..\"  c\"..(dbg.COLOR_YELLOW)..\"(ontinue)\"..(dbg.GREEN_CARET)..\"continu"
"e execution\\n\"\n  ..(dbg.COLOR_BLUE)..\"  s\"..(dbg.COLOR_YELLOW)..\"(tep)\"..(dbg.GREEN_CARET)..\"step forward by one line (i"
"nto functions)\\n\"\n  ..(dbg.COLOR_BLUE)..\"  n\"..(dbg.COLOR_YELLOW)..\"(ext)\"..(dbg.GREEN_CARET)..\"step forward by one line"
" (skipping over functions)\\n\"\n  ..(dbg.COLOR_BLUE)..\"  f\"..(dbg.COLOR_YELLOW)..\"(inish)\"..(dbg.GREEN_CARET)..\"step forwa"
"rd until exiting the current function\\n\"\n  ..(dbg.COLOR_BLUE)..\"  u\"..(dbg.COLOR_YELLOW)..\"(p)\"..(dbg.GREEN_CARET)..\"mov"
"e up the stack by one frame\\n\"\n  ..(dbg.COLOR_BLUE)..\"  d\"..(dbg.COLOR_YELLOW)..\"(own)\"..(dbg.GREEN_CARET)..\"move down t"
"he stack by one frame\\n\"\n  ..(dbg.COLOR_BLUE)..\"  i\"..(dbg.COLOR_YELLOW)..\"(nspect) \"..(dbg.COLOR_BLUE)..\"[index]\"..(dbg"
".GREEN_CARET)..\"move to a specific stack frame\\n\"\n  ..(dbg.COLOR_BLUE)..\"  w\"..(dbg.COLOR_YELLOW)..\"(here) \"..(dbg.COLOR"
"_BLUE)..\"[line count]\"..(dbg.GREEN_CARET)..\"print source code around the current line\\n\"\n  ..(dbg.COLOR_BLUE)..\"  e\"..(d"
"bg.COLOR_YELLOW)..\"(val) \"..(dbg.COLOR_BLUE)..\"[statement]\"..(dbg.GREEN_CARET)..\"execute the statement\\n\"\n  ..(dbg.COLOR"
"_BLUE)..\"  p\"..(dbg.COLOR_YELLOW)..\"(rint) \"..(dbg.COLOR_BLUE)..\"[expression]\"..(dbg.GREEN_CARET)..\"execute the expressi"
"on and print the result\\n\"\n  ..(dbg.COLOR_BLUE)..\"  t\"..(dbg.COLOR_YELLOW)..\"(race)\"..(dbg.GREEN_CARET)..\"print the stac"
"k trace\\n\"\n  ..(dbg.COLOR_BLUE)..\"  l\"..(dbg.COLOR_YELLOW)..\"(ocals)\"..(dbg.GREEN_CARET)..\"print the function arguments,"
" locals and upvalues.\\n\"\n  ..(dbg.COLOR_BLUE)..\"  h\"..(dbg.COLOR_YELLOW)..\"(elp)\"..(dbg.GREEN_CARET)..\"print this messag"
"e\\n\"\n  ..(dbg.COLOR_BLUE)..\"  q\"..(dbg.COLOR_YELLOW)..\"(uit)\"..(dbg.GREEN_CARET)..\"halt execution\\n\"\n )\n return false\nen"
"d\n\nlocal last_cmd = false\n\nlocal commands = {\n [\"^c$\"] = function() return true end,\n [\"^s$\"] = cmd_step,\n [\"^n$\"] = cmd"
"_next,\n [\"^f$\"] = cmd_finish,\n [\"^p%s+(.*)$\"] = cmd_print,\n [\"^e%s+(.*)$\"] = cmd_eval,\n [\"^u$\"] = cmd_up,\n [\"^d$\"] = cmd"
"_down,\n [\"i%s*(%d+)\"] = cmd_inspect,\n [\"^w%s*(%d*)$\"] = cmd_where,\n [\"^t$\"] = cmd_trace,\n [\"^l$\"] = cmd_locals,\n [\"^h$\"]"
" = cmd_help,\n [\"^q$\"] = function() dbg.exit(0); return true end,\n}\n\nlocal function match_command(line)\n for pat, func in"
" pairs(commands) do\n  -- Return the matching command and capture argument.\n  if line:find(pat) then return func, line:ma"
"tch(pat) end\n end\nend\n\n-- Run a command line\n-- Returns true if the REPL should exit and the hook function factory\nlocal"
" function run_command(line)\n -- GDB/LLDB exit on ctrl-d\n if line == nil then dbg.exit(1); return true end\n \n -- Re-execu"
"te the last command if you press return.\n if line == \"\" then line = last_cmd or \"h\" end\n \n local command, command_arg = "
"match_command(line)\n if command then\n  last_cmd = line\n  -- unpack({...}) prevents tail call elimination so the stack fr"
"ame indices are predictable.\n  return unpack({command(command_arg)})\n elseif dbg.auto_eval then\n  return unpack({cmd_eva"
"l(line)})\n else\n  dbg_writeln((dbg.COLOR_RED)..\"Error:\"..(dbg.COLOR_RESET)..\" command '%s' not recognized.\\nType 'h' and"
" press return for a command list.\", line)\n  return false\n end\nend\n\nrepl = function(reason)\n -- Skip frames without sourc"
"e info.\n while not frame_has_line(debug.getinfo(stack_inspect_offset + CMD_STACK_LEVEL - 3)) do\n  stack_inspect_offset ="
" stack_inspect_offset + 1\n end\n \n local info = debug.getinfo(stack_inspect_offset + CMD_STACK_LEVEL - 3)\n reason = reaso"
"n and ((dbg.COLOR_YELLOW)..\"break via \"..(dbg.COLOR_RED)..reason..(dbg.GREEN_CARET)) or \"\"\n dbg_writeln(reason..dbg.form"
"at_stack_frame_info(info))\n \n if tonumber(dbg.auto_where) then where(info, dbg.auto_where) end\n \n repeat\n  local success"
", done, hook = pcall(run_command, dbg.read((dbg.COLOR_RED)..\"debugger.lua> \"..(dbg.COLOR_RESET)))\n  if success then\n   d"
"ebug.sethook(hook and hook(0), \"crl\")\n  else\n   local message = (dbg.COLOR_RED)..\"INTERNAL DEBUGGER.LUA ERROR. ABORTING\\"
"n:\"..(dbg.COLOR_RESET)..\" \"..done\n   dbg_writeln(message)\n   error(message)\n  end\n until done\nend\n\n-- Make the debugger "
"object callable like a function.\ndbg = setmetatable({}, {\n __call = function(_, condition, top_offset, source)\n  if cond"
"ition then return end\n  \n  top_offset = (top_offset or 0)\n  stack_inspect_offset = top_offset\n  stack_top = top_offset\n "
" \n  debug.sethook(hook_next(1, source or \"dbg()\"), \"crl\")\n  return\n end,\n})\n\n-- Expose the debugger's IO functions.\ndbg."
"read = dbg_read\ndbg.write = dbg_write\ndbg.shorten_path = function (path) return path end\ndbg.format_loc = format_loc\ndbg"
".format_stack_frame_info = format_stack_frame_info\ndbg.exit = function(err) os.exit(err) end\n\ndbg.writeln = dbg_writeln\n"
"\ndbg.pretty_depth = 3\ndbg.pretty = pretty\ndbg.pp = function(value, depth) dbg_writeln(dbg.pretty(value, depth)) end\n\ndbg"
".auto_where = false\ndbg.auto_eval = false\n\nlocal lua_error, lua_assert = error, assert\n\n-- Works like error(), but invok"
"es the debugger.\nfunction dbg.error(err, level)\n level = level or 1\n dbg_writeln((dbg.COLOR_RED)..\"ERROR: \"..(dbg.COLOR_"
"RESET)..dbg.pretty(err))\n dbg(false, level, \"dbg.error()\")\n \n lua_error(err, level)\nend\n\n-- Works like assert(), but inv"
"okes the debugger on a failure.\nfunction dbg.assert(condition, message, ...)\n if not condition then\n  message = message "
"or \"assertion failed!\"\n  dbg_writeln((dbg.COLOR_RED)..\"ERROR: \"..(dbg.COLOR_RESET)..message)\n  dbg(false, 1, \"dbg.assert"
"()\")\n end\n \n return lua_assert(condition, message, ...)\nend\n\n-- Works like pcall(), but invokes the debugger on an error"
".\nfunction dbg.call(f, ...)\n return xpcall(f, function(err)\n  dbg_writeln((dbg.COLOR_RED)..\"ERROR: \"..(dbg.COLOR_RESET)."
".dbg.pretty(err))\n  dbg(false, 1, \"dbg.call()\")\n  \n  return err\n end, ...)\nend\n\n-- Error message handler that can be use"
"d with lua_pcall().\nfunction dbg.msgh(...)\n if debug.getinfo(2) then\n  dbg_writeln((dbg.COLOR_RED)..\"ERROR: \"..(dbg.COLO"
"R_RESET)..dbg.pretty(...))\n  dbg(false, 1, \"dbg.msgh()\")\n else\n  dbg_writeln((dbg.COLOR_RED)..\"debugger.lua: \"..(dbg.COL"
"OR_RESET)..\"Error did not occur in Lua code. Execution will continue after dbg_pcall().\")\n end\n \n return ...\nend\n\nfuncti"
"on dbg.use_color(value)\n local esc = string.char(27)\n dbg.COLOR_GRAY = value and (esc..\"[90m\") or \"\"\n dbg.COLOR_RED = va"
"lue and (esc..\"[91m\") or \"\"\n dbg.COLOR_BLUE = value and (esc..\"[94m\") or \"\"\n dbg.COLOR_YELLOW = value and (esc..\"[33m\") "
"or \"\"\n dbg.COLOR_RESET = value and (esc..\"[0m\") or \"\"\n dbg.GREEN_CARET = value and (esc..\"[92m => \"..(dbg.COLOR_RESET)) "
"or \" => \"\nend\n\n-- Conditionally enable color support.\nlocal color_maybe_supported = (os.getenv(\"TERM\") and os.getenv(\"TE"
"RM\") ~= \"dumb\")\ndbg.use_color(color_maybe_supported and not os.getenv(\"DBG_NOCOLOR\"))\n\n-- Detect Lua version.\nif jit the"
"n -- LuaJIT\n LUA_JIT_SETLOCAL_WORKAROUND = -1\nelseif _VERSION < \"Lua 5.1\" or _VERSION > \"Lua 5.5\" then\n dbg_writeln((dbg"
".COLOR_YELLOW)..\"debugger.lua: \"..(dbg.COLOR_RESET)..\"Not tested against \".._VERSION)\n dbg_writeln(\"Please send me feedb"
"ack!\")\nend\n\nreturn dbg\n";

int luaopen_debugger(lua_State *lua){
	if(
		luaL_loadbufferx(lua, DEBUGGER_SRC, sizeof(DEBUGGER_SRC) - 1, "<debugger.lua>", NULL) ||
		lua_pcall(lua, 0, LUA_MULTRET, 0)
	) lua_error(lua);
	
	// Or you could load it from disk:
	// if(luaL_dofile(lua, "debugger.lua")) lua_error(lua);
	
	return 1;
}

static const char *MODULE_NAME = "DEBUGGER_LUA_MODULE";
static const char *MSGH = "DEBUGGER_LUA_MSGH";

void dbg_setup(lua_State *lua, const char *name, const char *globalName, lua_CFunction readFunc, lua_CFunction writeFunc){
	// Check that the module name was not already defined.
	lua_getfield(lua, LUA_REGISTRYINDEX, MODULE_NAME);
	assert(lua_isnil(lua, -1) || strcmp(name, luaL_checkstring(lua, -1)));
	lua_pop(lua, 1);
	
	// Push the module name into the registry.
	lua_pushstring(lua, name);
	lua_setfield(lua, LUA_REGISTRYINDEX, MODULE_NAME);
	
	// Preload the module
	luaL_requiref(lua, name, luaopen_debugger, false);
	
	// Insert the msgh function into the registry.
	lua_getfield(lua, -1, "msgh");
	lua_setfield(lua, LUA_REGISTRYINDEX, MSGH);
	
	if(readFunc){
		lua_pushcfunction(lua, readFunc);
		lua_setfield(lua, -2, "read");
	}
	
	if(writeFunc){
		lua_pushcfunction(lua, writeFunc);
		lua_setfield(lua, -2, "write");
	}
	
	if(globalName){
		lua_setglobal(lua, globalName);
	} else {
		lua_pop(lua, 1);
	}
}

void dbg_setup_default(lua_State *lua){
	dbg_setup(lua, "debugger", "dbg", NULL, NULL);
}

int dbg_pcall(lua_State *lua, int nargs, int nresults, int msgh){
	// Call regular lua_pcall() if a message handler is provided.
	if(msgh) return lua_pcall(lua, nargs, nresults, msgh);
	
	// Grab the msgh function out of the registry.
	lua_getfield(lua, LUA_REGISTRYINDEX, MSGH);
	if(lua_isnil(lua, -1)){
		luaL_error(lua, "Tried to call dbg_call() before calling dbg_setup().");
	}
	
	// Move the error handler just below the function.
	msgh = lua_gettop(lua) - (1 + nargs);
	lua_insert(lua, msgh);
	
	// Call the function.
	int err = lua_pcall(lua, nargs, nresults, msgh);
	
	// Remove the debug handler.
	lua_remove(lua, msgh);
	
	return err;
}

#endif

#ifdef __cplusplus
}
#endif
