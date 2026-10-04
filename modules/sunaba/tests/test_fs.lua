-- GodotFS: file system access through Godot's FileAccess/DirAccess.
local function check(c, w) if not c then error("check failed: " .. w, 2) end end

local dir = "user://fs_test"
if GodotFS.exists(dir) then
  for _, n in ipairs(GodotFS.list(dir) or {}) do
    local p = dir .. "/" .. n
    if GodotFS.is_dir(p) then
      for _, m in ipairs(GodotFS.list(p) or {}) do GodotFS.remove(p .. "/" .. m) end
    end
    GodotFS.remove(p)
  end
  GodotFS.remove(dir)
end

check(not GodotFS.exists(dir), "directory is gone")
check(GodotFS.mkdir(dir .. "/sub/deep"), "mkdir is recursive")
check(GodotFS.is_dir(dir .. "/sub/deep") and not GodotFS.is_file(dir .. "/sub/deep"), "is_dir")

check(GodotFS.write_all(dir .. "/a.txt", "hello\nworld\n"), "write_all")
check(GodotFS.read_all(dir .. "/a.txt") == "hello\nworld\n", "read_all")
check(GodotFS.size(dir .. "/a.txt") == 12, "size")
check(GodotFS.write_all(dir .. "/a.txt", "more\n", true), "append")
check(GodotFS.read_all(dir .. "/a.txt") == "hello\nworld\nmore\n", "append result")
local bin = ""
for i = 0, 255 do bin = bin .. string.char(i) end
check(GodotFS.write_all(dir .. "/b.bin", bin), "write binary")
check(GodotFS.read_all(dir .. "/b.bin") == bin, "binary round trip (all 256 bytes, incl. NUL)")
check(GodotFS.read_all(dir .. "/missing") == nil, "read_all missing")
check(GodotFS.mtime(dir .. "/a.txt") > 0, "mtime")

local names = GodotFS.list(dir)
table.sort(names)
check(#names == 3 and names[1] == "a.txt" and names[2] == "b.bin" and names[3] == "sub", "list")
check(GodotFS.list(dir .. "/a.txt") == nil, "list on a file")

-- handles
local f = GodotFS.open(dir .. "/a.txt", "r")
check(f:read_line() == "hello" and f:read_line() == "world", "read_line")
check(f:tell() == 12, "tell")
check(f:read(100) == "more\n", "read(n)")
check(f:read(1) == nil and f:eof(), "eof")
f:seek("set", 6)
check(f:read(5) == "world", "seek set")
f:seek("end", -5)
check(f:read() == "more\n", "seek end + read all")
check(f:size() == 17, "size")
f:close()
check(not pcall(function() f:read() end), "closed file raises")

local w = GodotFS.open(dir .. "/c.txt", "w")
w:write("abc"):write("def")
w:close()
check(GodotFS.read_all(dir .. "/c.txt") == "abcdef", "write handle")
local ap = GodotFS.open(dir .. "/c.txt", "a")
ap:write("ghi")
ap:close()
check(GodotFS.read_all(dir .. "/c.txt") == "abcdefghi", "append handle")
check(GodotFS.open(dir .. "/nope/none.txt", "r") == nil, "open missing")

check(GodotFS.rename(dir .. "/c.txt", dir .. "/d.txt") and GodotFS.exists(dir .. "/d.txt") and not GodotFS.exists(dir .. "/c.txt"), "rename")
check(GodotFS.copy(dir .. "/d.txt", dir .. "/e.txt") and GodotFS.read_all(dir .. "/e.txt") == "abcdefghi", "copy")
check(GodotFS.remove(dir .. "/e.txt") and not GodotFS.exists(dir .. "/e.txt"), "remove")
check(GodotFS.absolute("user://x"):sub(1, 6) ~= "user:/", "absolute resolves user://")
check(#GodotFS.cwd() > 0, "cwd")
print("FS OK")
