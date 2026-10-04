-- Exercised by test_runtime.gd. Each check raises on failure; the last line
-- prints "ALL OK".

local function check(cond, what)
  if not cond then error("check failed: " .. what, 2) end
end
local function near(a, b) return math.abs(a - b) < 1e-4 end

-- Vector2 / Vector2i
local v = Vector2.new(3, 4)
check(v.x == 3 and v.y == 4, "Vector2 fields")
check(near(v:length(), 5), "Vector2.length")
check(near(v:normalized().x, 0.6), "Vector2.normalized")
check(v:isNormalized() == false and v:normalized():isNormalized(), "Vector2.isNormalized")
local w = v + Vector2.new(1, 1)
check(w.x == 4 and w.y == 5, "Vector2 __add")
check((v * 2).y == 8, "Vector2 __mul")
check((-v).x == -3, "Vector2 __unm")
check(near(v:lerp(Vector2.new(5, 4), 0.5).x, 4), "Vector2.lerp alias")
check(v:distanceTo(Vector2.new(0, 0)) == 5, "Vector2.distanceTo")
v.x = 10
check(v.x == 10, "Vector2 field assignment")
check(Vector2i.new(1.7, -2.2).x == 1 and Vector2i.new(1.7, -2.2).y == -2, "Vector2i truncation")
check(Vector2.new(1, 0):reflect(Vector2.new(0, 1)).x == -1, "Godot 4 reflect semantics")
check(Vector2.new(1, 2):gt(Vector2.new(0, 5)) and Vector2.new(0, 5):lt(Vector2.new(1, 2)), "Vector2 gt/lt")
check(v == Vector2.new(10, 4), "Vector2 __eq")
check(type(v:tostring()) == "string", "Vector2.tostring")
check(not pcall(function() return v:noSuchMethod() end), "missing method raises")

-- Vector3 / Basis / Transform3D / Quaternion
local v3 = Vector3.new(1, 2, 2)
check(near(v3:length(), 3), "Vector3.length")
check(v3:cross(Vector3.new(0, 0, 1)).x == 2, "Vector3.cross")
check(Vector3.new(1, 2, 3):max(Vector3.new(3, 2, 1)).x == 3, "Vector3.max (prelude)")
local t = Transform3D.new(Basis.new(), Vector3.new(1, 2, 3))
check(t.origin.z == 3, "Transform3D.origin")
check(near(Quaternion.new(0, 0, 0, 1):length(), 1), "Quaternion.length")
check(Vector4.new(1, 2, 3, 4).w == 4, "Vector4 (Quat-backed)")

-- Rect2 / AABB / Plane / Transform2D
local r = Rect2.new(0, 0, 10, 20)
check(r.position.x == 0 and r.size.y == 20, "Rect2.position alias")
check(r:getCenter().y == 10, "Rect2.getCenter")
check(r:hasPoint(Vector2.new(5, 5)), "Rect2.hasPoint")
check(AABB.new(Vector3.new(0, 0, 0), Vector3.new(2, 2, 2)):getCenter().x == 1, "AABB.getCenter")
check(Plane.new(0, 1, 0, 0):distanceTo(Vector3.new(0, 5, 0)) == 5, "Plane.distanceTo")
check(Transform2D.new(0, Vector2.new(3, 4)).origin.x == 3, "Transform2D.origin alias")

-- Color
local c = Color.html("#ff0000")
check(c.r == 1 and c.g == 0, "Color.html")
check(near(Color.new(c, 0.5).a, 0.5), "Color.new(color, alpha)")
check(c:toHtml(false) == "ff0000", "Color.toHtml")
check(Color.htmlIsValid("#00ff00") and not Color.htmlIsValid("nope"), "Color.htmlIsValid")
check(near(Color.new(0.5, 0.5, 0.5):lightened(1).r, 1), "Color.lightened")

-- ArrayList
local a = ArrayList.new()
a:append(3); a:append(1); a:append(2)
check(a:size() == 3 and #a == 3, "ArrayList size")
check(a:get(0):asInt() == 3 and a[1]:asInt() == 1, "ArrayList get/index (0-based)")
a:sort()
check(a[0]:asInt() == 1, "ArrayList.sort")
check(a:slice(1):size() == 2, "ArrayList.slice")
check(a:popBack():asInt() == 3 and a:size() == 2, "ArrayList.popBack")
local sum = 0
for i, val in pairs(a) do sum = sum + val:asInt() end
check(sum == 3, "ArrayList pairs")
check(a:max():asInt() == 2, "ArrayList.max")
check(a:filter(function(x) return x:asInt() > 1 end):size() == 1, "ArrayList.filter")

-- Dictionary
local d = Dictionary.new()
d:set("k", 5)
d.other = "x"
check(d:get("k"):asInt() == 5 and d.k:asInt() == 5, "Dictionary get/index")
check(d:has("other") and d:size() == 2, "Dictionary has/size")
check(d:keys():size() == 2, "Dictionary.keys")
local n = 0
for key, val in pairs(d) do n = n + 1 end
check(n == 2, "Dictionary pairs")
check(d:get("missing"):getType() == 0, "Dictionary missing key -> nil Variant")

-- Variant
check(Variant.new(5):getType() == 2, "Variant int type (Godot 4 numbering)")
check(Variant.new(1.5):getType() == 3, "Variant float type")
check(Variant.new(Vector2.new(1, 2)):getType() == 5, "Variant Vector2 type")
check(Variant.getTypeName(18) == "Transform3D", "Variant.getTypeName")
check(Variant.fromString("hi"):asString() == "hi", "Variant.fromString")
check(Variant.new(Vector2.new(1, 2)):asVector2().y == 2, "Variant.asVector2")
check(Variant.new(Vector2.new(1.9, 2)):asVector2i().x == 1, "Variant.asVector2i")
local ints = Variant.fromIntArray({ 1, 2, 3 }):asIntArray()
check(#ints == 3 and ints[3] == 3, "Variant int arrays")
check(Variant.fromArrayList(a):asArrayList():size() == 2, "Variant ArrayList round trip")

-- Values from GDScript (set_var)
check(config.a == 1 and #config.list == 3, "set_var converts to Lua tables")

-- NativeObject / NativeReference
local node = NativeObject.new("Node")
check(not node:isNull() and node:getClass() == "Node", "NativeObject.new")
local args = ArrayList.new(); args:append("foo")
node:call("set_name", args)
check(node:call("get_name", ArrayList.new()):asString() == "foo", "NativeObject.call")
local res = NativeReference.new("Resource")
res:set("resource/name", "bar")
check(res:get("resource/name"):asString() == "bar", "NativeReference get/set")
check(node:hasMethod("add_child") and node:isClass("Object"), "NativeObject reflection")
node:setMeta("m", 7)
check(node:getMeta("m"):asInt() == 7 and node:hasMeta("m"), "NativeObject meta")
check(node:getMethodArgumentCount("set_name") == 1, "getMethodArgumentCount")
local ref = NativeReference.new("Reference")
check(ref:isValid(), "NativeReference.new")
local os_service = NativeObject.getService("OS")
check(not os_service:isNull() and #os_service:call("get_name", ArrayList.new()):asString() > 0, "getService")
check(__rootNode:getClass() == "Runtime", "__rootNode")

-- Callable / Signal
local sargs = ArrayList.new(); sargs:append("ping")
node:call("add_user_signal", sargs)
local got = nil
local cb = Callable.new(function(x) got = x end)
local sig = Signal.new(node, "ping")
check(sig:connect(cb) == 0, "Signal.connect")
check(sig:isConnected(cb) and sig:hasConnections(), "Signal.isConnected")
local eargs = ArrayList.new(); eargs:append(99)
sig:emit(eargs)
-- Arguments from Godot arrive as plain Lua values (libsunaba's gdToSol).
check(got == 99, "Signal.emit reaches Lua callback")
sig:disconnect(cb)
check(not sig:isConnected(cb), "Signal.disconnect")
local bound = Callable.new(function(x, y) return x + y end):bind(eargs)
local cargs = ArrayList.new(); cargs:append(1)
check(bound:call(cargs):asInt() == 100, "Callable.bind/call")
node:free()
check(node:isNull(), "NativeObject.free")

-- ScriptFunction / ScriptObject seen from GDScript
local fargs = ArrayList.new()
fargs:append(Variant.fromFunction(function(x, y) return x + y end))
check(__rootNode:call("take_function", fargs):asFloat() == 42, "ScriptFunction.call_func")
local targs = ArrayList.new()
targs:append(Variant.fromTable({ name = "obj", double = function(x) return x * 2 end }))
check(__rootNode:call("take_table", targs):asString() == "obj:42", "ScriptObject get_var/call_function")

-- require through Runtime._require
check(require("mymod").answer == 42, "require via _require")

-- Byte / ByteArray
local bytes = ByteArray.new({ 1, 2, 255 })
check(bytes:size() == 3 and bytes:get(2):getInt64() == 255, "ByteArray.new(table)")
bytes:append(Byte.new(7))
local raw = Variant.fromByteArray(bytes)
check(raw:getType() == 29, "Variant.fromByteArray")
local back = raw:asByteArray()
check(back:size() == 4 and back:get(3):getInt() == 7, "ByteArray round trip")
check(Byte.new(1.5):getFloat() == 1.5, "Byte float cell")

-- print formatting
print("ALL OK")
