# Headless smoke test for the sunaba module.
# Run from this directory with a Godot 2 build that includes the module:
#   godot_server -s test_runtime.gd
extends SceneTree

class TestRuntime:
	extends Runtime

	var printed = []
	var errors = []

	func _print(msgarr):
		for m in msgarr:
			printed.append(m)

	func _errord(msg, title):
		errors.append(title + ": " + msg)

	func _require(path):
		if path == "mymod":
			return "return { answer = 42 }"
		return ""

	func take_function(fn):
		return fn.call_func([20, 22])

	func take_table(obj):
		return obj.get_var("name") + ":" + str(obj.call_function("double", [21]))

func _init():
	var failures = 0

	var rt = TestRuntime.new()
	get_root().add_child(rt)
	rt.init_state(false, [])
	rt.set_var("config", {"a": 1, "list": [1, 2, 3]})

	var f = File.new()
	f.open("res://test_runtime.lua", File.READ)
	var code = f.get_as_text()
	f.close()
	rt.do_string(code)

	for e in rt.errors:
		print("LUA ERROR: " + e)
		failures += 1
	if rt.printed.size() == 0 or rt.printed[rt.printed.size() - 1] != "ALL OK":
		print("Lua tests did not finish: ", rt.printed)
		failures += 1
	else:
		print("unsandboxed: ALL OK")

	# Runtime errors are reported through _errord with a traceback.
	rt.errors.clear()
	rt.do_string("error('boom')")
	if rt.errors.size() != 1 or rt.errors[0].find("boom") == -1:
		print("error reporting failed: ", rt.errors)
		failures += 1

	# Sandboxed runtime: only allowed classes, no io/os/debug.
	var sb = TestRuntime.new()
	sb.init_state(true, ["Node"])
	sb.do_string("print(io == nil, os == nil, NativeObject.new('Reference') == nil, (function() local n = NativeObject.new('Node'); local null = n:isNull(); n:free(); return null end)())")
	var expected = ["true", "true", "true", "false"]
	var sandbox_ok = sb.errors.size() == 0 and sb.printed.size() == expected.size()
	for i in range(min(sb.printed.size(), expected.size())):
		sandbox_ok = sandbox_ok and sb.printed[i] == expected[i]
	if not sandbox_ok:
		print("sandbox failed: ", sb.printed, sb.errors)
		failures += 1
	else:
		print("sandboxed: OK")
	sb.free()

	rt.free()
	if failures == 0:
		print("SUNABA TESTS PASSED")
	else:
		print("SUNABA TESTS FAILED: ", failures)
	quit()
